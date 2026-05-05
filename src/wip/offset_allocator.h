#include <assert.h>
#include <stdint.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

static uint32_t tzcnt_nonzero(uint32_t v)
{
#ifdef _MSC_VER
    unsigned long retVal;
    _BitScanForward(&retVal, v);
    return retVal;
#else
    return __builtin_ctz(v);
#endif
}

static uint32_t lzcnt_nonzero(uint32_t v)
{
#ifdef _MSC_VER
    unsigned long retVal = 0;
    _BitScanReverse(&retVal, v);
    return 31 - retVal;
#else
    return __builtin_clz(v);
#endif
}



namespace SmallFloat
{
    static constexpr uint32_t MANTISSA_BITS = 3;
    static constexpr uint32_t MANTISSA_VALUE = 1 << MANTISSA_BITS;
    static constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

    // Bin sizes follow floating point (exponent + mantissa) distribution (piecewise linear log approx)
    // This ensures that for each size class, the average overhead percentage stays the same
    uint32_t uintToFloatRoundUp(uint32_t size)
    {
        uint32_t exp = 0;
        uint32_t mantissa = 0;

        if (size < MANTISSA_VALUE)
        {
            // Denorm: 0..(MANTISSA_VALUE-1)
            mantissa = size;
        }
        else
        {
            // Normalized: Hidden high bit always 1. Not stored. Just like float.
            uint32_t leadingZeros = lzcnt_nonzero(size);
            uint32_t highestSetBit = 31 - leadingZeros;

            uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
            exp = mantissaStartBit + 1;
            mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;

            uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

            // Round up!
            if ((size & lowBitsMask) != 0)
                mantissa++;
        }

        return (exp << MANTISSA_BITS) + mantissa; // + allows mantissa->exp overflow for round up
    }

    uint32_t uintToFloatRoundDown(uint32_t size)
    {
        uint32_t exp = 0;
        uint32_t mantissa = 0;

        if (size < MANTISSA_VALUE)
        {
            // Denorm: 0..(MANTISSA_VALUE-1)
            mantissa = size;
        }
        else
        {
            // Normalized: Hidden high bit always 1. Not stored. Just like float.
            uint32_t leadingZeros = lzcnt_nonzero(size);
            uint32_t highestSetBit = 31 - leadingZeros;

            uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
            exp = mantissaStartBit + 1;
            mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;
        }

        return (exp << MANTISSA_BITS) | mantissa;
    }

    uint32_t floatToUint(uint32_t floatValue)
    {
        uint32_t exponent = floatValue >> MANTISSA_BITS;
        uint32_t mantissa = floatValue & MANTISSA_MASK;
        return (exponent == 0) ? mantissa : (mantissa | MANTISSA_VALUE) << (exponent - 1);
    }
}


// Utility functions
static uint32_t findLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex)
{
    uint32_t maskBeforeStartIndex = (1 << startBitIndex) - 1;
    uint32_t maskAfterStartIndex = ~maskBeforeStartIndex;
    uint32_t bitsAfter = bitMask & maskAfterStartIndex;
    if (bitsAfter == 0) return OffsetAllocator::Allocation::NO_SPACE;
    return tzcnt_nonzero(bitsAfter);
}


namespace OffsetAllocator
{
    // 16 bit offsets mode will halve the metadata storage cost
    // But it only supports up to 65536 maximum allocation count
#ifdef USE_16_BIT_NODE_INDICES
    typedef uint16_t NodeIndex;
#else
    typedef uint32_t NodeIndex;
#endif

    static constexpr uint32_t NUM_TOP_BINS = 32;
    static constexpr uint32_t BINS_PER_LEAF = 8;
    static constexpr uint32_t TOP_BINS_INDEX_SHIFT = 3;
    static constexpr uint32_t LEAF_BINS_INDEX_MASK = 0x7;
    static constexpr uint32_t NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;

    struct Allocation
    {
        static constexpr uint32_t NO_SPACE = 0xffffffff;

        uint32_t offset = NO_SPACE;
        NodeIndex metadata = NO_SPACE; // internal: node index
    };

    struct StorageReport
    {
        uint32_t totalFreeSpace;
        uint32_t largestFreeRegion;
    };

    struct StorageReportFull
    {
        struct Region
        {
            uint32_t size;
            uint32_t count;
        };

        Region freeRegions[NUM_LEAF_BINS];
    };

    class Allocator
    {
    public:
        Allocator(uint32_t size, uint32_t maxAllocs = 128 * 1024) :
            m_size(size),
            m_maxAllocs(maxAllocs),
            m_nodes(nullptr),
            m_freeNodes(nullptr)
        {
            if (sizeof(NodeIndex) == 2)
            {
                assert(maxAllocs <= 65536);
            }
            reset();
        }
      //  Allocator(Allocator&& other);

        ~Allocator()
        {
            delete[] m_nodes;
            delete[] m_freeNodes;
        }

        void reset()
        {
            m_freeStorage = 0;
            m_usedBinsTop = 0;
            m_freeOffset = m_maxAllocs - 1;

            for (uint32_t i = 0; i < NUM_TOP_BINS; i++)
                m_usedBins[i] = 0;

            for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
                m_binIndices[i] = Node::unused;

            if (m_nodes) delete[] m_nodes;
            if (m_freeNodes) delete[] m_freeNodes;

            m_nodes = new Node[m_maxAllocs];
            m_freeNodes = new NodeIndex[m_maxAllocs];

            // Freelist is a stack. Nodes in inverse order so that [0] pops first.
            for (uint32_t i = 0; i < m_maxAllocs; i++)
            {
                m_freeNodes[i] = m_maxAllocs - i - 1;
            }

            // Start state: Whole storage as one big node
            // Algorithm will split remainders and push them back as smaller nodes
            insertNodeIntoBin(m_size, 0);
        }

        Allocation allocate(uint32_t size)
        {
            Allocation allocation = {};
            // Out of allocations?
            if (m_freeOffset == 0)
            {
                return allocation;
            }

            // Round up to bin index to ensure that alloc >= bin
            // Gives us min bin index that fits the size
            uint32_t minBinIndex = SmallFloat::uintToFloatRoundUp(size);

            uint32_t minTopBinIndex = minBinIndex >> TOP_BINS_INDEX_SHIFT;
            uint32_t minLeafBinIndex = minBinIndex & LEAF_BINS_INDEX_MASK;

            uint32_t topBinIndex = minTopBinIndex;
            uint32_t leafBinIndex = Allocation::NO_SPACE;

            // If top bin exists, scan its leaf bin. This can fail (NO_SPACE).
            if (m_usedBinsTop & (1 << topBinIndex))
            {
                leafBinIndex = findLowestSetBitAfter(m_usedBins[topBinIndex], minLeafBinIndex);
            }

            // If we didn't find space in top bin, we search top bin from +1
            if (leafBinIndex == Allocation::NO_SPACE)
            {
                topBinIndex = findLowestSetBitAfter(m_usedBinsTop, minTopBinIndex + 1);

                // Out of space?
                if (topBinIndex == Allocation::NO_SPACE)
                {
                    return {  Allocation::NO_SPACE, Allocation::NO_SPACE };
                }

                // All leaf bins here fit the alloc, since the top bin was rounded up. Start leaf search from bit 0.
                // NOTE: This search can't fail since at least one leaf bit was set because the top bit was set.
                leafBinIndex = tzcnt_nonzero(m_usedBins[topBinIndex]);
            }

            uint32_t binIndex = (topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex;

            // Pop the top node of the bin. Bin top = node.next.
            uint32_t nodeIndex = m_binIndices[binIndex];
            Node& node = m_nodes[nodeIndex];
            uint32_t nodeTotalSize = node.dataSize;
            node.dataSize = size;
            node.used = true;
            m_binIndices[binIndex] = node.binListNext;
            if (node.binListNext != Node::unused) m_nodes[node.binListNext].binListPrev = Node::unused;
            m_freeStorage -= nodeTotalSize;
#ifdef DEBUG_VERBOSE
            printf("Free storage: %u (-%u) (allocate)\n", m_freeStorage, nodeTotalSize);
#endif

            // Bin empty?
            if (m_binIndices[binIndex] == Node::unused)
            {
                // Remove a leaf bin mask bit
                m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);

                // All leaf bins empty?
                if (m_usedBins[topBinIndex] == 0)
                {
                    // Remove a top bin mask bit
                    m_usedBinsTop &= ~(1 << topBinIndex);
                }
            }

            // Push back reminder N elements to a lower bin
            uint32_t reminderSize = nodeTotalSize - size;
            if (reminderSize > 0)
            {
                uint32_t newNodeIndex = insertNodeIntoBin(reminderSize, node.dataOffset + size);

                // Link nodes next to each other so that we can merge them later if both are free
                // And update the old next neighbor to point to the new node (in middle)
                if (node.neighborNext != Node::unused) m_nodes[node.neighborNext].neighborPrev = newNodeIndex;
                m_nodes[newNodeIndex].neighborPrev = nodeIndex;
                m_nodes[newNodeIndex].neighborNext = node.neighborNext;
                node.neighborNext = newNodeIndex;
            }

            allocation.offset = node.dataOffset;
            allocation.metadata = nodeIndex;

            return allocation;
        }

        void free(Allocation allocation)
        {
            assert(allocation.metadata != Allocation::NO_SPACE);
            if (!m_nodes) return;

            uint32_t nodeIndex = allocation.metadata;
            Node& node = m_nodes[nodeIndex];

            // Double delete check
            assert(node.used == true);

            // Merge with neighbors...
            uint32_t offset = node.dataOffset;
            uint32_t size = node.dataSize;

            if ((node.neighborPrev != Node::unused) && (m_nodes[node.neighborPrev].used == false))
            {
                // Previous (contiguous) free node: Change offset to previous node offset. Sum sizes
                Node& prevNode = m_nodes[node.neighborPrev];
                offset = prevNode.dataOffset;
                size += prevNode.dataSize;

                // Remove node from the bin linked list and put it in the freelist
                removeNodeFromBin(node.neighborPrev);

                assert(prevNode.neighborNext == nodeIndex);
                node.neighborPrev = prevNode.neighborPrev;
            }

            if ((node.neighborNext != Node::unused) && (m_nodes[node.neighborNext].used == false))
            {
                // Next (contiguous) free node: Offset remains the same. Sum sizes.
                Node& nextNode = m_nodes[node.neighborNext];
                size += nextNode.dataSize;

                // Remove node from the bin linked list and put it in the freelist
                removeNodeFromBin(node.neighborNext);

                assert(nextNode.neighborPrev == nodeIndex);
                node.neighborNext = nextNode.neighborNext;
            }

            uint32_t neighborNext = node.neighborNext;
            uint32_t neighborPrev = node.neighborPrev;

            // Insert the removed node to freelist
#ifdef DEBUG_VERBOSE
            printf("Putting node %u into freelist[%u] (free)\n", nodeIndex, m_freeOffset + 1);
#endif
            m_freeNodes[++m_freeOffset] = nodeIndex;

            // Insert the (combined) free node to bin
            uint32_t combinedNodeIndex = insertNodeIntoBin(size, offset);

            // Connect neighbors with the new combined node
            if (neighborNext != Node::unused)
            {
                m_nodes[combinedNodeIndex].neighborNext = neighborNext;
                m_nodes[neighborNext].neighborPrev = combinedNodeIndex;
            }
            if (neighborPrev != Node::unused)
            {
                m_nodes[combinedNodeIndex].neighborPrev = neighborPrev;
                m_nodes[neighborPrev].neighborNext = combinedNodeIndex;
            }
        }


        inline uint32_t allocationSize(Allocation allocation) const
        {
            if (allocation.metadata == Allocation::NO_SPACE) 
                return 0;

            if (!m_nodes) 
                return 0;

            return m_nodes[allocation.metadata].dataSize;
        }


        inline StorageReport storageReport() const
        {
            uint32_t largestFreeRegion = 0;
            uint32_t freeStorage = 0;

            // Out of allocations? -> Zero free space
            if (m_freeOffset > 0)
            {
                freeStorage = m_freeStorage;
                if (m_usedBinsTop)
                {
                    uint32_t topBinIndex = 31 - lzcnt_nonzero(m_usedBinsTop);
                    uint32_t leafBinIndex = 31 - lzcnt_nonzero(m_usedBins[topBinIndex]);
                    largestFreeRegion = SmallFloat::floatToUint((topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex);
                    assert(freeStorage >= largestFreeRegion);
                }
            }

            StorageReport report = {};
            report.largestFreeRegion = largestFreeRegion;
            report.totalFreeSpace = freeStorage;
            return report;
        }


        inline StorageReportFull storageReportFull() const
        {
            StorageReportFull report;
            for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
            {
                uint32_t count = 0;
                uint32_t nodeIndex = m_binIndices[i];
                while (nodeIndex != Node::unused)
                {
                    nodeIndex = m_nodes[nodeIndex].binListNext;
                    count++;
                }
                report.freeRegions[i].size = SmallFloat::floatToUint(i);
                report.freeRegions[i].count = count;
            }
            return report;
        }

    private:
        uint32_t insertNodeIntoBin(uint32_t size, uint32_t dataOffset)
        {
            // Round down to bin index to ensure that bin >= alloc
            uint32_t binIndex = SmallFloat::uintToFloatRoundDown(size);

            uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
            uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

            // Bin was empty before?
            if (m_binIndices[binIndex] == Node::unused)
            {
                // Set bin mask bits
                m_usedBins[topBinIndex] |= 1 << leafBinIndex;
                m_usedBinsTop |= 1 << topBinIndex;
            }

            // Take a freelist node and insert on top of the bin linked list (next = old top)
            uint32_t topNodeIndex = m_binIndices[binIndex];
            uint32_t nodeIndex = m_freeNodes[m_freeOffset--];
#ifdef DEBUG_VERBOSE
            printf("Getting node %u from freelist[%u]\n", nodeIndex, m_freeOffset + 1);
#endif
            m_nodes[nodeIndex].dataOffset = dataOffset;
            m_nodes[nodeIndex].dataSize = size;
            m_nodes[nodeIndex].binListNext = topNodeIndex;

            if (topNodeIndex != Node::unused) m_nodes[topNodeIndex].binListPrev = nodeIndex;
            m_binIndices[binIndex] = nodeIndex;

            m_freeStorage += size;
#ifdef DEBUG_VERBOSE
            printf("Free storage: %u (+%u) (insertNodeIntoBin)\n", m_freeStorage, size);
#endif

            return nodeIndex;
        }

        void removeNodeFromBin(uint32_t nodeIndex)
        {
            Node& node = m_nodes[nodeIndex];

            if (node.binListPrev != Node::unused)
            {
                // Easy case: We have previous node. Just remove this node from the middle of the list.
                m_nodes[node.binListPrev].binListNext = node.binListNext;
                if (node.binListNext != Node::unused) m_nodes[node.binListNext].binListPrev = node.binListPrev;
            }
            else
            {
                // Hard case: We are the first node in a bin. Find the bin.

                // Round down to bin index to ensure that bin >= alloc
                uint32_t binIndex = SmallFloat::uintToFloatRoundDown(node.dataSize);

                uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
                uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

                m_binIndices[binIndex] = node.binListNext;
                if (node.binListNext != Node::unused) m_nodes[node.binListNext].binListPrev = Node::unused;

                // Bin empty?
                if (m_binIndices[binIndex] == Node::unused)
                {
                    // Remove a leaf bin mask bit
                    m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);

                    // All leaf bins empty?
                    if (m_usedBins[topBinIndex] == 0)
                    {
                        // Remove a top bin mask bit
                        m_usedBinsTop &= ~(1 << topBinIndex);
                    }
                }
            }
        }

        struct Node
        {
            static constexpr NodeIndex unused = 0xffffffff;

            uint32_t    dataOffset = 0;
            uint32_t    dataSize = 0;
            NodeIndex   binListPrev = unused;
            NodeIndex   binListNext = unused;
            NodeIndex   neighborPrev = unused;
            NodeIndex   neighborNext = unused;
            bool        used = false; // TODO: Merge as bit flag
        };

        uint32_t        m_size;
        uint32_t        m_maxAllocs;
        uint32_t        m_freeStorage;

        uint32_t        m_usedBinsTop;
        uint8_t         m_usedBins[NUM_TOP_BINS];
        NodeIndex       m_binIndices[NUM_LEAF_BINS];

        Node*           m_nodes;
        NodeIndex*      m_freeNodes;
        uint32_t        m_freeOffset;
    };
}