#ifndef __gfx_memory_h__
#define __gfx_memory_h__

#include <stdint.h> // uintXX_t 
#include <stdlib.h> // allocations
//#include <string.h> // allocations

struct gfx_offset_allocator_t;

extern void         gfx_offset_allocator_create(gfx_offset_allocator_t* allocator, uint32_t size, uint32_t min_size);

extern void         gfx_offset_allocator_destroy(gfx_offset_allocator_t* allocator);

extern ptrdiff_t    gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size, uint32_t aligment);

extern void         gfx_offset_allocator_free(gfx_offset_allocator_t* allocator, ptrdiff_t offset);



typedef struct gfx_offset_block_t {
    uint32_t                offset;
    uint32_t                size;
} gfx_offset_block_t;

typedef struct gfx_offset_allocator_t {
    uint32_t                size;              // total size
    uint32_t                min_size;          // pow of two

    uint32_t                blocks_count;

    uint32_t                allocated_blocks_count;
    gfx_offset_block_t*     allocated_blocks;

    uint32_t                free_blocks_count;
    gfx_offset_block_t*     free_blocks;

} gfx_offset_allocator_t;


static uint32_t align_up(uint32_t val, uint32_t align) {
    return (val + align - 1) & ~(align - 1);
}

static int block_sort(const void* a, const void* b) {
    return ((gfx_offset_block_t*)a)->offset > ((gfx_offset_block_t*)b)->offset;
}

static void block_erase(gfx_offset_block_t* bloks, uint32_t size, uint32_t idx) {
    bloks[idx] = bloks[size];
    bloks[size] = { 0,0 };
}

static void gfx_offset_allocator_create(gfx_offset_allocator_t* allocator, uint32_t size, uint32_t min_size)
{
    memset(allocator, 0, sizeof(gfx_offset_allocator_t));
    allocator->size = size;
    allocator->min_size = align_up(min_size, sizeof(void*));

    allocator->blocks_count = size / align_up(min_size, sizeof(void*));
    allocator->allocated_blocks = (gfx_offset_block_t*)calloc(allocator->blocks_count, sizeof(gfx_offset_block_t));
    allocator->free_blocks = (gfx_offset_block_t*)calloc(allocator->blocks_count, sizeof(gfx_offset_block_t));

    if(allocator->free_blocks)
        allocator->free_blocks[0] = {0, size};
    allocator->free_blocks_count++;
}

static void gfx_offset_allocator_destroy(gfx_offset_allocator_t* allocator)
{
    free(allocator->free_blocks);
    free(allocator->allocated_blocks);
}

static ptrdiff_t gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size, uint32_t alignment)
{
    size = align_up(size, allocator->min_size);
    for (uint32_t i = 0; allocator->free_blocks_count; ++i) {

        uint32_t block_offset = allocator->free_blocks[i].offset;
        uint32_t block_size = allocator->free_blocks[i].size;
        uint32_t aligned_offset = align_up(block_offset, alignment);
        uint32_t padding = aligned_offset - block_offset;

        if (block_size >= size + padding) {

            uint32_t idx = allocator->allocated_blocks_count++;
            allocator->allocated_blocks[idx].size = size;
            allocator->allocated_blocks[idx].offset = aligned_offset;

            if (block_size == size + padding) {
                block_erase(allocator->free_blocks, allocator->free_blocks_count, i);
                allocator->free_blocks_count--;
            } else {
                if (padding > 0) {
                    allocator->free_blocks[i].size = padding;
                    if (size < block_size - padding) {
                        int idx = allocator->free_blocks_count;
                        allocator->free_blocks[idx].offset = aligned_offset + size;
                        allocator->free_blocks[idx].size = block_size - size - padding;
                        allocator->free_blocks_count++;
                    }
                } else {
                    allocator->free_blocks[i].offset = aligned_offset + size;
                    allocator->free_blocks[i].size = block_size - size;
                }
            }

            qsort(allocator->free_blocks, allocator->free_blocks_count, sizeof(gfx_offset_block_t), block_sort);

            return static_cast<ptrdiff_t>(aligned_offset);
        }
    }
    return -1;
}

static void dump(gfx_offset_allocator_t* allocator)
{
    printf("\n\n gfx_offset_allocator info:");
    printf("\n allocated blocks:");

    for (size_t i = 0; i < allocator->allocated_blocks_count; ++i) {
        printf("\n   offset: %8d  size: %4u", allocator->allocated_blocks[i].offset, allocator->allocated_blocks[i].size);
    }
    printf("\n free blocks:");
    for (size_t i = 0; i < allocator->free_blocks_count; ++i) {
        printf("\n   offset: %8d  size: %4u", allocator->free_blocks[i].offset, allocator->free_blocks[i].size);
    }

}

static void merge_free_blocks(gfx_offset_allocator_t* allocator)
{
    if (allocator->free_blocks_count == 0)
        return;

    qsort(allocator->free_blocks, allocator->free_blocks_count, sizeof(gfx_offset_block_t), block_sort);

    uint32_t write_idx = 0;
    for (uint32_t read_idx = 1; read_idx < allocator->free_blocks_count; ++read_idx) {
        auto& last = allocator->free_blocks[write_idx];
        auto& current = allocator->free_blocks[read_idx];

        if (last.offset + last.size == current.offset)
            last.size += current.size;
        else
            allocator->free_blocks[++write_idx] = current;
    }
    allocator->free_blocks_count = write_idx + 1;
}

static void gfx_offset_allocator_free(gfx_offset_allocator_t* allocator, ptrdiff_t offset)
{
    for (uint32_t i = 0; allocator->allocated_blocks_count; ++i) {
        if (allocator->allocated_blocks[i].offset == offset) {
            int idx = allocator->free_blocks_count;
            allocator->free_blocks[idx].offset = allocator->allocated_blocks[i].offset;
            allocator->free_blocks[idx].size = allocator->allocated_blocks[i].size;
            allocator->free_blocks_count++;

            block_erase(allocator->allocated_blocks, allocator->allocated_blocks_count, i);
            allocator->allocated_blocks_count--;

            merge_free_blocks(allocator);
            return;
        }
    }
}


static void gfx_offset_allocator_test()
{
    gfx_offset_allocator_t oa = {};
    gfx_offset_allocator_create(&oa, 16*1024*1024, 64);

    auto a0 = gfx_offset_allocator_allocate(&oa, 128, 64);
    auto a1 = gfx_offset_allocator_allocate(&oa, 64, 64);
    auto a2 = gfx_offset_allocator_allocate(&oa, 13, 64);
    auto a3 = gfx_offset_allocator_allocate(&oa, 256, 64);

    dump(&oa);

    gfx_offset_allocator_free(&oa, a1);
    gfx_offset_allocator_free(&oa, a0);
    gfx_offset_allocator_free(&oa, a2);

    dump(&oa);

    auto a4 = gfx_offset_allocator_allocate(&oa, 256, 64);
    dump(&oa);

    gfx_offset_allocator_destroy(&oa);
}

#ifdef GFX_MEMORY_IMPLEMENTATION
/*
typedef struct gfx_o_allocator_t {
    uint32_t size;              // total size
    uint32_t min_block_size;    // powe oo two
    uint32_t levels;            // levels (log2(size / min_block_size) + 1)

    uint8_t* tree;              // bitmask (0=free, 1=busy)
} gfx_allocator_t;


static inline uint32_t log2_uint32(uint32_t x) {
    uint32_t r = 0;
    while (x >>= 1) r++;
    return r;
}

static inline uint32_t index_offset(uint32_t level, uint32_t i) {
    return (1 << level) - 1 + i;
}

static void buddy_init(buddy_allocator_t* alloc, uint32_t total_size, uint32_t min_block_size) {
    alloc->size = total_size;
    alloc->min_block_size = min_block_size;
    alloc->levels = log2_uint32(total_size / min_block_size) + 1;

    int tree_size = (1 << alloc->levels) - 1;
    alloc->tree = (uint8_t*)calloc(tree_size, 1);
    memset(alloc->tree, 0, (1 << alloc->levels) - 1);
}

static void mark_parents(buddy_allocator_t* alloc, uint32_t index) {
    while (index) {
        index = (index - 1) >> 1;
        alloc->tree[index] = 1;
    }
}

static void unmark_parents(buddy_allocator_t* alloc, uint32_t index) {
    while (index) {
        uint32_t parent = (index - 1) >> 1;
        uint32_t left = parent * 2 + 1;
        uint32_t right = left + 1;
        if (alloc->tree[left] || alloc->tree[right]) break;
        alloc->tree[parent] = 0;
        index = parent;
    }
}

static int32_t buddy_alloc(buddy_allocator_t* alloc, uint32_t size) {
    if (size > alloc->size) return -1;

    uint32_t level = 0;
    uint32_t block_size = alloc->size;

    while (block_size > size && block_size > alloc->min_block_size) {
        block_size >>= 1;
        level++;
    }

    uint32_t first = (1 << level) - 1;
    uint32_t last = (1 << (level + 1)) - 2;

    for (uint32_t i = first; i <= last; ++i) {
        if (alloc->tree[i] == 0) {
            uint32_t parent = (i - 1) >> 1;
            bool ok = true;
            while (i) {
                if (alloc->tree[parent]) {
                    ok = false;
                    break;
                }
                parent = (parent - 1) >> 1;
                i = (i - 1) >> 1;
            }
            if (!ok) continue;

            alloc->tree[i] = 1;
            mark_parents(alloc, i);
            uint32_t block_index = i - ((1 << level) - 1);
            return block_index * block_size;
        }
    }

    return -1;
}

static void buddy_free(buddy_allocator_t* alloc, uint32_t offset) {
    uint32_t level = 0;
    uint32_t block_size = alloc->size;

    while (block_size > alloc->min_block_size) {
        if (offset % block_size == 0) break;
        block_size >>= 1;
        level++;
    }

    uint32_t index_in_level = offset / block_size;
    uint32_t index = index_offset(level, index_in_level);
    alloc->tree[index] = 0;
    unmark_parents(alloc, index);
}*/
#endif // GFX_MEMORY_IMPLEMENTATION

#endif 