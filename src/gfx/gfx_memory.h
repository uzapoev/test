#ifndef __gfx_memory_h__
#define __gfx_memory_h__

#include <stdint.h> // uintXX_t 
#include <stdlib.h> // calloc/free
#include <stdio.h>  // printf
#include <string.h> // memset

// offset allocator
struct gfx_offset_allocator_t;

static void     gfx_offset_allocator_create(uint32_t size, uint32_t block_size, gfx_offset_allocator_t** allocator);

static void     gfx_offset_allocator_destroy(gfx_offset_allocator_t* allocator);

static intptr_t gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size);

static void     gfx_offset_allocator_free(gfx_offset_allocator_t* allocator, intptr_t offset);




typedef struct gfx_offset_allocator_t {
    uint32_t  size;             // Total managed size (e.g., in bytes)
    uint32_t  block_size;       // Quantum/granularity size of a single block
    uint32_t  blocks_count;     // Total number of blocks (size / block_size)
    uint32_t  bitmask_words;    // Number of uint64_t elements needed for the bitmask
    uint64_t* bitmask;          // Pointer to the bit arrays (0 = free, 1 = allocated)
    uint32_t* block_sizes;      // Track allocation lengths (in blocks) for large mesh buffers
} gfx_offset_allocator_t;


static uint32_t align_up(uint32_t val, uint32_t align) {
    return (val + align - 1) & ~(align - 1);
}

static void gfx_offset_allocator_create(uint32_t size, uint32_t block_size, gfx_offset_allocator_t** allocator) 
{
    if (!allocator) return;

    uint32_t blocks_count = size / block_size;
    uint32_t mask_words = (blocks_count + 63) / 64;

    // Calculate total memory for: Header + Bitmask (uint64) + Size Tracker (uint32)
    size_t bitmask_bytes = mask_words * sizeof(uint64_t);
    size_t sizes_bytes = blocks_count * sizeof(uint32_t);
    size_t total_mem = sizeof(gfx_offset_allocator_t) + bitmask_bytes + sizes_bytes;

    void* buffer = calloc(1, total_mem);
    if (!buffer) {
        *allocator = NULL;
        return;
    }

    // Initialize the header at the very beginning of the allocated buffer
    gfx_offset_allocator_t* header = (gfx_offset_allocator_t*)buffer;
    header->size = size;
    header->block_size = block_size;
    header->blocks_count = blocks_count;
    header->bitmask_words = mask_words;

    // Layout arrays sequentially right after the header structure
    uint8_t* mem_ptr = (uint8_t*)buffer + sizeof(gfx_offset_allocator_t);
    header->bitmask = (uint64_t*)mem_ptr;
    header->block_sizes = (uint32_t*)(mem_ptr + bitmask_bytes);

    *allocator = header;
}


static void gfx_offset_allocator_destroy(gfx_offset_allocator_t* allocator) 
{
    if (allocator) {
        free(allocator);
    }
}


static intptr_t gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size) 
{
    if (!allocator) return -1;

    // Calculate how many blocks are needed for the requested size
    uint32_t needed_blocks = (size + allocator->block_size - 1) / allocator->block_size;

    uint32_t run_length = 0;
    uint32_t start_block = 0;
    uint32_t total_bits = allocator->bitmask_words * 64;

    // Linear scan through the bitmask to find a contiguous sequence of 0s
    for (uint32_t i = 0; i < total_bits; ++i) {
        if (i >= allocator->blocks_count) break;

        uint32_t word_idx = i / 64;
        uint32_t bit_idx = i % 64;

        if (allocator->bitmask[word_idx] & (1ULL << bit_idx)) {
            run_length = 0; // Block is occupied
        }
        else {
            if (run_length == 0) start_block = i;
            run_length++;

            if (run_length == needed_blocks) {
                // Mark bits as allocated (1)
                for (uint32_t b = start_block; b < start_block + needed_blocks; ++b) {
                    allocator->bitmask[b / 64] |= (1ULL << (b % 64));
                }

                // Store the allocation length at the starting block index
                allocator->block_sizes[start_block] = needed_blocks;

                return (intptr_t)(start_block * allocator->block_size);
            }
        }
    }
    return -1; // OOM
}

// Releases the allocated region back to the pool in true O(1) time
static void gfx_offset_allocator_free(gfx_offset_allocator_t* allocator, intptr_t offset) 
{
    if (!allocator || offset < 0) 
        return;

    uint32_t start_block = (uint32_t)offset / allocator->block_size;

    // Lookup how many blocks this allocation actually owns
    uint32_t needed_blocks = allocator->block_sizes[start_block];
    if (needed_blocks == 0) 
        return; 

    // Clear the tracking slot
    allocator->block_sizes[start_block] = 0;

    // Clear bits back to 0. Merging happens automatically for the next allocation scan.
    for (uint32_t b = start_block; b < start_block + needed_blocks; ++b) {
        allocator->bitmask[b / 64] &= ~(1ULL << (b % 64));
    }
}

#endif 