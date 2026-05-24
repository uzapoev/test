/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <yuriy> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return 
 * ---------------------------------------------------------------------------- */
//int size = (length+3)&~3;
#include <assert.h>
#include "memmgr.h"
#include <algorithm>
#include <unordered_set>

#include "common.h"


#define TRACE_MEMORY_ALLOCATION     0
#define USE_CUSTOM_NEW_ALLOCATION   1
#define USE_ALLOCATION_CALLBACK     1

bool                        s_allocatorDeepLogEnable    = true;
allocation_callback_pfn     s_allocationCallback        = nullptr;
void*                       s_allocationCallbackData    = nullptr;

static volatile size_t  g_total_allocated_memory = 0;
static volatile bool    g_trace_allocations = 0;



#define sys_malloc(x)           ::malloc(x)
#define sys_free(x)             ::free(x)


inline uint32_t ctz64(uint64_t x) {
    assert(x != 0);

#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, x);
    return (uint32_t)index;
#else
    return (uint32_t)__builtin_ctzll(x);
#endif
}


#ifdef _WIN32
    #pragma warning(disable: 6387 28183 28196 28251 28252 28253 )

    #define sys_msize(x)            ::_msize(x)
    #define memlog(...)             ::printf(__VA_ARGS__)


#elif defined (__APPLE__)
    #define sys_msize(x)            ::malloc_size(x)
    #define memlog(...)             ::printf(__VA_ARGS__)

#elif defined (__ANDROID__)
    #include <android/log.h>

    #define sys_msize(x)            ::malloc_usable_size(x)
    #define memlog(...)             __android_log_print(ANDROID_LOG_INFO, "memory", __VA_ARGS__)

#elif defined (__NINTENDO__)
    #define sys_msize(x)            (0)
    #define memlog(...)             ::printf(__VA_ARGS__)

#elif defined (EMSCRIPTEN)
    #define memlog(...)
    #define sys_msize(x)            ::malloc_usable_size(x)
#else
    #error unknown platform
#endif


#ifdef _MSC_VER

#endif // _MSC_VER


int clzll(uint64_t x) {
    if (x == 0) return 64;
    int n = 0;
    if ((x >> 32) == 0) { n += 32; x <<= 32; }
    if ((x >> 48) == 0) { n += 16; x <<= 16; }
    if ((x >> 56) == 0) { n += 8; x <<= 8; }
    if ((x >> 60) == 0) { n += 4; x <<= 4; }
    if ((x >> 62) == 0) { n += 2; x <<= 2; }
    if ((x >> 63) == 0) { n += 1; }
    return n;
}


static bool is_pow(size_t x) {
    return x && !(x & (x - 1));
}

static size_t next_pot(size_t x) {
    if (is_pow(x)) return x;
    return 1ull << (64 - clzll(x));
}

static size_t align_up(size_t val, size_t align) {
    return (val + align - 1) & ~(align - 1);
}

static int log2(size_t x) {
    return static_cast<int>(std::log2(x));
}


inline void bitmask_set(uint64_t * _bitmask, size_t index, bool value) {
    size_t w = index / 64;
    size_t b = index % 64;
    if (value)  _bitmask[w] |=  (1ull << b);
    else        _bitmask[w] &= ~(1ull << b);
}

inline bool bitmask_value(uint64_t* _bitmask, size_t index) {
    size_t w = index / 64;
    size_t b = index % 64;
    return (_bitmask[w]) >> b & 1ull;
}


struct bitmask {
    bitmask(size_t size) {
        init(size);
    }

    void init(size_t size) {
        _word_count = align_up(size, 64) / 64;
        _bitmask = new uint64_t[_word_count]();
    }

    size_t bits_size() const {
        return _word_count * 64;
    }

    void set(size_t index, bool value) {
        bitmask_set(_bitmask, index, value);
    }

    bool value(size_t index) {
        return bitmask_value(_bitmask, index);
    }

    bool has_free() const {
        for (uint16_t i = 0; i < _word_count; ++i) {
            uint64_t inv = ~_bitmask[i];
            if (inv != 0)
                return true;
        }
        return false;
    }

    int first_free_index() {
        for (uint16_t i = 0; i < _word_count; ++i) {
            uint64_t inv = ~_bitmask[i];
            if (inv != 0)
                return 64 * i + ctz(inv);
        }
        return -1;
    }

    static uint32_t ctz(uint64_t x) {
        return ctz64(x);
    }
private:
    size_t    _word_count = 0;
    uint64_t* _bitmask = nullptr;
};



struct mem_traker
{
    void track_allocation(void * ptr, size_t size) {
        _traked_allocations.emplace(ptr, size);
    }

    void track_dealocation(void* ptr) {
        auto it = _traked_allocations.find({ ptr, 0 });
        if(it != _traked_allocations.end())
            _traked_allocations.erase(it);
    }

    struct memblock
    {
        memblock(void* p, size_t sz) :ptr(p), size(sz)  { memset(frames, 0, sizeof(frames)); }
        bool operator == (const memblock& a) const     { return a.ptr == ptr; }

        void*       ptr;
        size_t      size;
        intptr_t    frames[16] = {}; // replace to stack frames 
    };

    struct memblock_hash {
        inline std::size_t operator() (const memblock& s) const {
            return reinterpret_cast<std::size_t>(s.ptr);
        }
    };

    std::unordered_set<memblock, memblock_hash> _traked_allocations;
};



//
//  aligned_allocator
//

aligned_allocator::aligned_allocator(const char* tag)
{
    strcpy(m_name, tag);
    m_traker = new mem_traker();
}

aligned_allocator::~aligned_allocator()
{
    delete m_traker;
    m_traker = nullptr;
}


void* aligned_allocator::allocate(size_t size, size_t alignment) 
{
    void* ptr = _aligned_malloc(size, alignment);
    m_allocated_size += _aligned_msize(ptr, alignment, 0);
    m_traker->track_allocation(ptr, size);
    return ptr ? memset(ptr, 0, size) : nullptr;
}

void aligned_allocator::deallocate(void* memory) 
{
    m_traker->track_dealocation(memory);
    _aligned_free(memory);
}



//
// paged_pool_allocator
//

struct page {
    bitmask     _bitmask;
    page*       _next = nullptr;
    uint64_t*   _bitset = nullptr;
    char*       _data = nullptr;
};

paged_pool_allocator::paged_pool_allocator(iallocator* memory_resource, size_t allocation_size, size_t allocations_per_page)
:m_allocator(memory_resource)
,m_allocation_size(allocation_size)
,m_allocations_per_page(allocations_per_page)
{
    m_allocations_per_page = align_up(allocations_per_page, 64);
    m_bitset_word_count = m_allocations_per_page / 64;
    m_page_size = m_allocations_per_page * allocation_size;
    m_page_head = allocate_page();
    m_page_current = m_page_head;
}

paged_pool_allocator::~paged_pool_allocator()
{
    while (m_page_head != nullptr) {
        auto next = m_page_head->_next;
        m_allocator->deallocate(m_page_head);
        m_page_head = next;
    }
    m_page_head = nullptr;
}

page* paged_pool_allocator::allocate_page()
{
    // allocate page info + data size + bitset size
    size_t size = sizeof(page) + // page info
                  sizeof(uint64_t) + m_bitset_word_count + // bitmask size
                  (m_allocation_size * m_allocations_per_page); // data size

    auto result = (page*)m_allocator->allocate(size, alignof(page));

    result->_bitmask.init(m_allocations_per_page);
    result->_bitset = (uint64_t*)((char*)result + sizeof(page));
    result->_data   = (char*)    ((char*)result + sizeof(page) + sizeof(uint64_t) + m_bitset_word_count);

    debug::log_error("allocated page(%s): %u  data: %u", m_allocator->tag(), result, result->_data);
    return result;
}

struct page* paged_pool_allocator::find_page_with_free_blocs()
{   
    page * tmp = m_page_head;
    while (tmp != nullptr)    {
        if(tmp->_bitmask.has_free())
            return tmp;

        tmp = tmp->_next;
    }
    return nullptr;
}

struct page* paged_pool_allocator::find_page_for_ptr(void* ptr)
{
    page* tmp = m_page_head;
    while (tmp != nullptr) {
        if ( ((char*)ptr >= tmp->_data) && 
             ((char*)ptr < (tmp->_data + m_page_size)))
            return tmp;

        tmp = tmp->_next;
    }
    return nullptr;
}

void* paged_pool_allocator::allocate(size_t size, size_t alignment)
{
    auto page = find_page_with_free_blocs();

    if(page == nullptr)
    {
        auto newpage = allocate_page();
        m_page_current->_next = newpage;
        m_page_current = newpage;
    }

    int index = m_page_current->_bitmask.first_free_index();
    m_page_current->_bitmask.set(index, true);

    return m_page_current->_data + m_allocation_size * index;
}

void paged_pool_allocator::deallocate(void* ptr)
{
    auto page = find_page_for_ptr(ptr);
    if(!page)
        return;

    ptrdiff_t index = ((char*)ptr - page->_data)/m_allocation_size;
    page->_bitmask.set(index, false);
}



void memory::enable_tracking()
{
  //  if(g_pMemoryManager == nullptr)
  //      g_pMemoryManager = new(_reserved) MemoryManager();
}

void memory::enable_allocation_traking(bool value)
{
    g_trace_allocations = value;
}

void memory::dump(memory_stats_t* stats)
{
   // if(stats != nullptr)
    //    memcpy(stats, &mem_total_allocs, sizeof(memory_stats_t));
}

size_t memory::allocated()
{
    return g_total_allocated_memory;
}



struct FreeList 
{
    size_t* data = nullptr;
    int count = 0;
    int m_capacity = 0;

    void init(int cap) {
        m_capacity = cap;
        data = new size_t[cap];
        count = 0;
    }

    void destroy() {
        delete[] data;
        data = nullptr;
        count = m_capacity = 0;
    }

    bool empty() const { return count == 0; }

    void push(size_t offset) {
        assert(count < m_capacity);
        data[count++] = offset;
    }

    size_t pop() {
        assert(count > 0);
        return data[--count];
    }

    int find(size_t offset) const {
        for (int i = 0; i < count; ++i) {
            if (data[i] == offset) return i;
        }
        return -1;
    }

    void remove(int index) {
        assert(index >= 0 && index < count);
        data[index] = data[--count];
    }
};


buddy_allocator::buddy_allocator(void* buffer, size_t totalSize, size_t minBlockSize)
    : m_buffer(reinterpret_cast<std::uint8_t*>(buffer)),
    m_totalSize(totalSize),
    m_minBlockSize(minBlockSize)
{
    assert((totalSize & (totalSize - 1)) == 0);
    assert((minBlockSize & (minBlockSize - 1)) == 0);
    assert(minBlockSize <= totalSize);

    m_maxLevel = log2(totalSize) - log2(minBlockSize);
    m_levelCount = m_maxLevel + 1;

    m_freeLists = new FreeList[m_levelCount];

    for (int i = 0; i < m_levelCount; ++i) {
        m_freeLists[i].init(256);
    }
 
    m_freeLists[m_maxLevel].push(0);
}

buddy_allocator::~buddy_allocator() 
{
    for (int i = 0; i < m_levelCount; ++i) {
        m_freeLists[i].destroy();
    }
    delete[] m_freeLists;
}

void* buddy_allocator::allocate(size_t size, size_t aligment) 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    size = align_up(size, m_minBlockSize);
    if (!is_pow(size)) 
        size = next_pot(size);
    int level = getLevel(size);

    for (int i = level; i <= m_maxLevel; ++i) {
        if (!m_freeLists[i].empty()) {
            size_t offset = m_freeLists[i].pop();

            while (i > level) {
                i--;
                size_t buddyOffset = offset + block_size(i);
                m_freeLists[i].push(buddyOffset);
            }

            return m_buffer + offset;
        }
    }

    return nullptr; 
}

void buddy_allocator::deallocate(void* ptr) 
{
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t offset = reinterpret_cast<std::uint8_t*>(ptr) - m_buffer - sizeof(size_t);
    size_t size = *reinterpret_cast<size_t*>(m_buffer + offset);

    int level = getLevel(size);

    while (level < m_maxLevel) {
        size_t buddy = get_buddy(offset, level);
        int index = m_freeLists[level].find(buddy);
        if (index == -1) break;

        m_freeLists[level].remove(index);
        offset = std::min(offset, buddy);
        level++;
    }

    m_freeLists[level].push(offset);
}




offset_allocator::offset_allocator(size_t size, size_t min_size)
:m_buffer_size(size)
{
    size_t block_count = size/ (min_size*4);
    m_min_size = align_up(min_size, sizeof(void*));
    m_free_blocks.reserve(64);
    m_free_blocks.emplace_back(0, size);
    m_allocated_blocks.reserve(64);
}


ptrdiff_t offset_allocator::allocate(size_t size, size_t alignment)
{
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return -1;
    }

    size = align_up(size, m_min_size);

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_free_blocks.begin(); it != m_free_blocks.end(); ++it) {
        size_t block_offset = it->offset;
        size_t block_size = it->size;
        size_t aligned_offset = align_up(block_offset, alignment);
        size_t padding = aligned_offset - block_offset;

        if (block_size >= size + padding) {

            m_allocated_blocks.emplace_back(aligned_offset, size);

            if (block_size == size + padding)
            {
                m_free_blocks.erase(it);
            }
            else 
            {
                if (padding > 0) 
                {
                    it->size = padding;
                    if (size < block_size - padding) 
                    {
                        m_free_blocks.emplace_back(aligned_offset + size, block_size - size - padding);
                    }
                }
                else 
                {
                    it->offset = aligned_offset + size;
                    it->size = block_size - size;
                }
            }
        
            std::sort(m_free_blocks.begin(), m_free_blocks.end(),
                [](const Block& a, const Block& b) { return a.offset < b.offset; });

            return static_cast<ptrdiff_t>(aligned_offset);
        }
    }

    return -1;
}

void offset_allocator::deallocate(size_t offset)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto it = m_allocated_blocks.begin(); it != m_allocated_blocks.end(); ++it) {
        if (it->offset == offset) {
            m_free_blocks.emplace_back(it->offset, it->size);
            m_allocated_blocks.erase(it);

            merge_free_blocks();
            return;
        }
    }
}

void offset_allocator::merge_free_blocks() 
{
    if (m_free_blocks.empty())
        return;

    std::sort(m_free_blocks.begin(), m_free_blocks.end(),
        [](const Block& a, const Block& b) { return a.offset < b.offset; });

    size_t write_idx = 0;
    for (size_t read_idx = 1; read_idx < m_free_blocks.size(); ++read_idx) {
        auto& last = m_free_blocks[write_idx];
        auto& current = m_free_blocks[read_idx];

        if (last.offset + last.size == current.offset) {
            last.size += current.size;
        }
        else {
            ++write_idx;
            m_free_blocks[write_idx] = current;
        }
    }
    m_free_blocks.resize(write_idx + 1);
/*
    std::vector<Block> merged;
    merged.push_back(m_free_blocks[0]);

    for (size_t i = 1; i < m_free_blocks.size(); ++i) {
        Block& last = merged.back();
        const Block& current = m_free_blocks[i];

        if (last.offset + last.size >= current.offset) {
            last.size = std::max(last.size, (current.offset + current.size) - last.offset);
        }
        else {
            merged.push_back(current);
        }
    }

    m_free_blocks = std::move(merged);*/
}


#if 0



static memory_stats_t   mem_total_allocs = { 0, 0x0fffffff, 0, 0 };
static memory_stats_t   mem_frame_allocs;
static memory_stats_t   mem_frame_frees;

void Mem_UpdateAllocStats(size_t size);
void Mem_UpdateFreeStats(size_t size);


static void setAllocationCallback(allocation_callback_pfn cb, void* data)
{
    s_allocationCallback = cb;
    s_allocationCallbackData = data;
}

MemoryManager* g_pMemoryManager = nullptr;
static char _reserved[sizeof(MemoryManager)] = {};

MemoryManager::MemoryManager()
{
#if USE_ALLOCATION_CALLBACK
    setAllocationCallback(MemoryManager::callback, this);
#endif
}


MemoryManager::~MemoryManager()
{
    g_pMemoryManager = nullptr;
    setAllocationCallback(nullptr, nullptr);

    {
        std::lock_guard lock(m_mutex);

        mem_block_map(m_blocks).swap(m_blocks);//the STL swap trick to trim memory
        memlog("\n----------------------------------------------------------");
        memlog("\nMemoryTrackerReport");
        memlog("\nLeaked Memory"       );
        memlog("\n----------------------------------------------------------\n");
    
        size_t totalLeak = 0;    
    //    MemBlocks::const_iterator it, ite = m_blocks.end();
        FILE * file = fopen("memlog.log", "w");

        //char  framesinfo[stack_size][16] = { 0 };
        char * framesinfo[memblock_t::k_max_stack_size] = { };

        for(int i = 0; i < memblock_t::k_max_stack_size; ++i)
            framesinfo[i] = (char*)malloc(sizeof(char)*2048);
         
        for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
        {
            for(int i = 0; i< memblock_t::k_max_stack_size; ++i)
                if(framesinfo[i] != nullptr)
                    memset(framesinfo[i], 0, sizeof(char)* 2048);
    //        StackInfoBlocks::iterator infoIt = m_infoblocks.find(it->first);

            const memblock_t& block = (*it).second;
            memlog("\n %llx  size: %zd", (uintptr_t)block.ptr, block.size);

            if(file)
                fprintf(file, "\n %llx  size: %zd", (uintptr_t)block.ptr, block.size);

            if(block.frames[0] != 0)
            {
                int framescount = sizeof(block.frames)/sizeof(block.frames[0]);
             //   debug::stacktrace_names(block.frames, framescount, (char**)framesinfo);

                for (int i = 0; i < framescount; i++)
                {
                    if(block.frames[i] == 0)
                        break;

                    if(file)
                        fprintf(file, "\n\t%s", framesinfo[i]);
                }
            }
            totalLeak += block.size;
        }
        if (file)fclose(file);
    
        for(int i = 0; i< memblock_t::k_max_stack_size; ++i)
            ::free(framesinfo[i]);
    
        memlog("\n************************************************************");
        memlog("\n* Total leak is: %zd kB, , bytes in %zd blocks", totalLeak/1024, m_blocks.size() );
        memlog("\n************************************************************\n");
    }
#ifdef _DEBUG
//    system("pause");
#endif/**/


    m_blocks.clear();
}

void MemoryManager::initialize()
{    
//    mem_heap = new Heap();
//    Mem_ClearFrameStats();
}

void MemoryManager::destroy()
{
}

void * MemoryManager::alloc(size_t size )
{
    std::lock_guard lock(m_mutex);
    
    if ( !size )
        return nullptr;

    if(size > 1024*64)
        printf("");

    void *mem = nullptr;

#ifdef CRASH_ON_STATIC_ALLOCATION
        *((int*)0x0) = 1;
#endif
    //  size = ( size + 3 & ~3); 4 bit align
    size = ( size + 15 & ~15 );//16 bit align
    mem = sys_malloc(size);
    g_total_allocated_memory += size;

    if(s_allocationCallback)
    {
        Mem_UpdateAllocStats(size);
        callback(size, mem, this);
    }
    
    return mem;
}

void * MemoryManager::alloc16(size_t size)
{
    if ( !size )
        return nullptr;
    //  size = ( size + 3 & ~3); 4 bit align
    size = (size + 15 & ~15);//16 bit align

#ifdef CRASH_ON_STATIC_ALLOCATION
    *((int*)0x0) = 1;
#endif
    g_total_allocated_memory += size;
    return sys_malloc(size);
}


void MemoryManager::free(void *ptr)
{
    std::lock_guard lock(m_mutex);

    if ( !ptr )
        return;
    
    if(s_allocationCallback )
        callback(0, ptr, this);

    size_t originsize = sys_msize(ptr);
    g_total_allocated_memory -= originsize;
    Mem_UpdateFreeStats(originsize);
    sys_free( ptr );
}

void   MemoryManager::free16(void *ptr)
{
    std::lock_guard lock(m_mutex);

    if ( !ptr )
        return;

    sys_free( ptr );
}

void MemoryManager::dump()
{
    std::lock_guard lock(m_mutex);
    
    size_t totalMemUsage = 0;
    for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
    {
        totalMemUsage += it->second.size;
    }
    memlog("\nsize: %zd kB", totalMemUsage /1024);
}


void MemoryManager::callback(size_t sz, void* ptr, void* data)
{
    MemoryManager* t = static_cast<MemoryManager*>(data);
    if(sz > 0) 
    {
        if(s_allocatorDeepLogEnable)
        {
            //skip next frames:
            //  [0] dbg_backtrace(...)
            //  [1] MemoryManager::callback(...)
            //  [2] MemoryManager::alloc(...)
            //  [3] new
         //   Debug::stacktrace_frames(memBlock.frames, sizeof(memBlock.frames)/sizeof(intptr_t), 4);
        }
        t->m_blocks.emplace(intptr_t(ptr), memblock_t(sz, ptr));
    }
    else 
    {
        if(t->m_blocks.find((intptr_t)(ptr)) != t->m_blocks.end())
            t->m_blocks.erase((intptr_t)(ptr));
        else
            memlog("\nmemoryTracker: freeing memory that wasn't previously registered");
    }
}


void MemoryManager::Mem_ClearFrameStats( void )
{
    mem_frame_allocs.active_allocations = mem_frame_frees.active_allocations = 0;
    mem_frame_allocs.min_size = mem_frame_frees.min_size = 0x0fffffff;
    mem_frame_allocs.max_size = mem_frame_frees.max_size = -1;
    mem_frame_allocs.total_allocated_size = mem_frame_frees.total_allocated_size = 0;
}

void MemoryManager::Mem_GetFrameStats(memory_stats_t&allocs, memory_stats_t&frees )
{
    allocs = mem_frame_allocs;
    frees = mem_frame_frees;
}

void MemoryManager::Mem_GetStats(memory_stats_t&stats )
{
    stats = mem_total_allocs;
}



void Mem_UpdateStats(memory_stats_t&stats, size_t size )
{
    stats.active_allocations++;
    stats.min_size = std::min(size, stats.min_size);
    stats.max_size = std::min(size, stats.max_size);

    stats.total_allocated_size += size;
}

void Mem_UpdateAllocStats( size_t size )
{
    Mem_UpdateStats( mem_frame_allocs, size );
    Mem_UpdateStats( mem_total_allocs, size );
}


void Mem_UpdateFreeStats( size_t size )
{
    Mem_UpdateStats( mem_frame_frees, size );
    mem_total_allocs.active_allocations--;
    mem_total_allocs.total_allocated_size -= size;
}





//#define TRACE_MEMORY_ALLOCATION 1
//volatile const unsigned int memoryUsed = 0;

////////////////////////////////////////////////////////////////
//
//MemoryTracker ttr;

#if USE_CUSTOM_NEW_ALLOCATION

void *operator new(size_t size)/* noexcept*/
{
    if (g_trace_allocations)
        memlog("+");

    void* ptr = nullptr;
    if(g_pMemoryManager)
    {
        ptr = g_pMemoryManager->alloc(size);
    }
    else
    {
        ptr = sys_malloc(size);
        if(ptr != nullptr)
        {
            size_t originsize = sys_msize(ptr);
            Mem_UpdateAllocStats(originsize);
            g_total_allocated_memory += originsize;
        }
    }
    return ptr;
}

void * operator new[](size_t size) /*noexcept*/
{
    if (g_trace_allocations)
        memlog("*");

    void* ptr = nullptr;
    if(g_pMemoryManager)
    {
        ptr = g_pMemoryManager->alloc(size);
    }
    else
    {
        ptr = sys_malloc(size);
        size_t originsize = sys_msize(ptr);
        Mem_UpdateAllocStats(originsize);
        g_total_allocated_memory += originsize;
    }
    
    return ptr;
}

void operator delete(void *ptr) noexcept
{
    if (g_trace_allocations)
        memlog("-");

    if(ptr)
    {
        if(g_pMemoryManager)
        {
            g_pMemoryManager->free(ptr);
        }
        else
        {
            size_t originsize = sys_msize(ptr);
            Mem_UpdateFreeStats(originsize);
            g_total_allocated_memory -= originsize;
            sys_free(ptr);
        }
    }
}

void operator delete[](void * ptr) noexcept
{
    if (g_trace_allocations)
        memlog("/");

    if(ptr)
    {
        if(g_pMemoryManager)
        {
            g_pMemoryManager->free(ptr);
        }
        else
        {
            size_t originsize = sys_msize(ptr);
            g_total_allocated_memory -= originsize;
            Mem_UpdateFreeStats(originsize);
            sys_free(ptr);
        }
    }
}
 #endif // #if USE_CUSTOM_NEW_ALLOCATION



#endif