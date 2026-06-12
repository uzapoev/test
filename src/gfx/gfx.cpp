#include "gfx.h"

#include <stdio.h>
#include <memory.h> // memset
#include <atomic>

#if __has_include(<vulkan/vulkan.h>)
    #define VULKAN_AVAILABLE
#endif

#if __has_include(<webgpu/webgpu.h>)
    #define WEBGPU_AVAILABLE
#endif

#if __has_include(<d3d12.h>)
    #define DX12_AVAILABLE
#endif

#if __has_include(<Metal/Metal.h>)
    #define METAL_AVAILABLE
#endif


// The full MIP level chain for debugging looks like this: 16x16 -> 8x8 -> 4x4 -> 2x2 -> 1x1
uint32_t gfx_failover_texture_data[] = {
    // --- MIP 0: 16x16 (Magenta / Black / White) ---
    // Pattern: A large magenta checkerboard of 4x4 pixels per cell,
    // but separated by thin white lines along the axes for tiling checks.
    0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFFFFFF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFFFFFF, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFFFFFF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFFFFFF, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFFFFF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,

    // --- MIP 1: 8x8 (Yellow / Black) ---
    // Pattern: classic 2x2 pixel contrast checkerboard.
    0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF,
    0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000,
    0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF,
    0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF000000, 0xFF000000, 0xFF00FFFF, 0xFF00FFFF,

    // --- MIP 2: 4x4 (Cyan/Black) --- 
    // Pattern: 1x1 pixel checkerboard.
    0xFFFF0000, 0xFF000000, 0xFFFF0000, 0xFF000000,
    0xFF000000, 0xFFFF0000, 0xFF000000, 0xFFFF0000,
    0xFFFF0000, 0xFF000000, 0xFFFF0000, 0xFF000000,
    0xFF000000, 0xFFFF0000, 0xFF000000, 0xFFFF0000,

    // --- MIP 3: 2x2 (Pure Green / Black) ---
    0xFF00FF00, 0xFF000000,
    0xFF000000, 0xFF00FF00,

    // --- MIP 4: 1x1 (Pure Blue) ---
    // Filtering tail when the object is very far away.
    0xFFFF0000
};


#pragma region to sting
const char* gfx_to_string(gfx_buffer_usage usage)
{
    switch (usage)
    {
        case gfx_buffer_usage_index:        return  "index";
        case gfx_buffer_usage_vertex:       return  "vertex";
        case gfx_buffer_usage_uniform:      return  "uniform";
        case gfx_buffer_usage_storage:      return  "storage";
        case gfx_buffer_usage_indirect:     return  "indirect";
    }
    return "invalid arg in gfx_to_string(gfx_buffer_usage usage)";
}

const char* gfx_to_string(gfx_shader_stage stage)
{
    switch (stage)
    {
        case gfx_shader_vertex:           return "vertex";
        case gfx_shader_fragment:         return "fragment";

        case gfx_shader_amplify:          return "amplify";
        case gfx_shader_mesh:             return "mesh";

        case gfx_shader_compute:          return "compute";

        case gfx_shader_rt_raygen:        return "raygen";
        case gfx_shader_rt_any_hit:       return "any_hit";
        case gfx_shader_rt_closest_hit:   return "closest_hit";
        case gfx_shader_rt_miss:          return "miss";
        case gfx_shader_rt_intersect:     return "intersect";
        case gfx_shader_rt_callable:      return "callable";

        default: return "invalid arg in gfx_to_string(gfx_shader_stage stage)";
    }
    return "invalid arg in gfx_to_string(gfx_shader_stage stage)";
}

const char* gfx_to_string(gfx_texture_type type)
{
    switch (type)
    {
        case gfx_texture2d:         return "texture2d";
        case gfx_texture2d_cube:    return "texture2d_cube";
        case gfx_texture2d_array:   return "texture2d_array";
        case gfx_texture3d:         return "texture3d";
    }
    return "invalid arg in gfx_to_string(gfx_texture_type type)";
}

const char* gfx_to_string(gfx_pixel_format format)
{
    switch (format)
    {
        case gfx_pixel_format_unknown:      return "unknown";
        case gfx_pixel_format_a8:           return "a8";
        case gfx_pixel_format_rgba4444:     return "rgba4444";
        case gfx_pixel_format_rgb5a1:       return "rgb5a1";
        case gfx_pixel_format_rgb565:       return "rgb565";
        case gfx_pixel_format_rgba8:        return "rgba8";

        case gfx_pixel_format_etc1:         return "etc1";
        case gfx_pixel_format_etc2_rgb8a1:  return "etc2_rgb8a1";
        case gfx_pixel_format_etc2_rgba8:   return "etc2_rgba8";

        case gfx_pixel_format_bc1:          return "bc1";
        case gfx_pixel_format_bc3:          return "bc3";
        case gfx_pixel_format_bc4:          return "bc4";
        case gfx_pixel_format_bc5:          return "bc5";
        case gfx_pixel_format_bc6h:         return "bc6h";
        case gfx_pixel_format_bc7:          return "bc7";

        case gfx_pixel_format_astc4x4:      return "astc4x4";
        case gfx_pixel_format_astc5x5:      return "astc5x5";
        case gfx_pixel_format_astc6x6:      return "astc6x6";
        case gfx_pixel_format_astc8x8:      return "astc8x8";
        case gfx_pixel_format_astc10x10:    return "astc10x10_srgb";
        case gfx_pixel_format_astc12x12:    return "astc12x12_srgb";

        case gfx_pixel_format_r16f:         return "r16";
        case gfx_pixel_format_rg16f:        return "rg16";
        case gfx_pixel_format_rgba16f:      return "rgba16";

        case gfx_pixel_format_r32f:         return "r32";
        case gfx_pixel_format_rg32f:        return "rg32";
        case gfx_pixel_format_rgba32f:      return "rgba32";

        case gfx_pixel_format_d24x8:        return "d24x8";
        case gfx_pixel_format_d24s8:        return "d24s8";
    }

    return "invalid arg in gfx_to_string(gfx_pixel_format format)";
}

#pragma endregion


#pragma region gfx utils
uint32_t gfx_utils_thread_id()
{
    static thread_local const uint32_t s_unique_id = []() {
        static std::atomic<uint32_t> s_counter = 1;
        return s_counter.fetch_add(1, std::memory_order_relaxed);
        }();
        return s_unique_id;
}


uint32_t gfx_utils_hash(const void* data, uint32_t size, uint32_t seed)
{
    const uint8_t* bytes = (const uint8_t*)data;
    const int nblocks = size / 4;
    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(bytes + nblocks * 4);

    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t* tail = (const uint8_t*)(bytes + nblocks * 4);
    uint32_t k1 = 0;

    switch (size & 3) {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
    };

    h1 ^= size;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}


static uint32_t block_count(uint32_t s, uint32_t b) { return ((s + b - 1) / b); }

uint32_t gfx_utils_image_layer_size(uint32_t width, uint32_t height, uint32_t depth, gfx_pixel_format format)
{
    uint32_t w = width;
    uint32_t h = height;
    uint32_t d = gfx_max(1, depth);//(depth > 0)?depth:1; // clamp [1..depth]
    switch (format)
    {
        case gfx_pixel_format_rgb5a1:
        case gfx_pixel_format_rgb565:
        case gfx_pixel_format_rgba4444:         return w * h * d * sizeof(uint16_t);

        case gfx_pixel_format_rgba8:            return w * h * d * 4;

        case gfx_pixel_format_etc1:             return (w >> 2) * (h >> 2) * 8;     //! Compresses RGB888 data without Alpha channel
        case gfx_pixel_format_etc2_rgb8a1:		return (w >> 2) * (h >> 2) * 16;    //! Compresses RGB888 data without Alpha channel
        case gfx_pixel_format_etc2_rgba8:		return (w >> 2) * (h >> 2) * 8;     //! Compresses RGBA8888 data with full alpha support

        case gfx_pixel_format_bc1:              return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 8;
        case gfx_pixel_format_bc4:              return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 8;
        case gfx_pixel_format_bc3:              return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 16;
        case gfx_pixel_format_bc5:              return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 16;
        case gfx_pixel_format_bc6h:             return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 16;
        case gfx_pixel_format_bc7:              return gfx_max(1, (w + 3) >> 2) * gfx_max(1, (h + 3) >> 2) * gfx_max(1, d) * 16;

        case gfx_pixel_format_astc4x4:          return block_count(w, 4) * block_count(h, 4) * block_count(d, 4) * 16;
        case gfx_pixel_format_astc5x5:          return block_count(w, 5) * block_count(h, 5) * block_count(d, 5) * 16;
        case gfx_pixel_format_astc6x6:          return block_count(w, 6) * block_count(h, 6) * block_count(d, 6) * 16;
        case gfx_pixel_format_astc8x8:          return block_count(w, 8) * block_count(h, 8) * block_count(d, 8) * 16;
        case gfx_pixel_format_astc10x10:        return block_count(w, 10) * block_count(h, 10) * block_count(d, 10) * 16;
        case gfx_pixel_format_astc12x12:        return block_count(w, 12) * block_count(h, 12) * block_count(d, 12) * 16;

        case gfx_pixel_format_r16f:             return w * h * d * sizeof(uint16_t);
        case gfx_pixel_format_rg16f:            return w * h * d * sizeof(uint16_t) * 2;
        case gfx_pixel_format_rgba16f:          return w * h * d * sizeof(uint16_t) * 4;

        case gfx_pixel_format_r32f:             return w * h * d * sizeof(float);
        case gfx_pixel_format_rg32f:            return w * h * d * sizeof(float) * 2;
        case gfx_pixel_format_rgba32f:          return w * h * d * sizeof(float) * 4;

        case gfx_pixel_format_d24x8:            return w * h * d * sizeof(uint32_t);
        case gfx_pixel_format_d24s8:            return w * h * d * sizeof(uint32_t);

        default: assert(0); break;
    }
    return 0;
}

uint32_t gfx_utils_image_row_pitch(gfx_pixel_format fmt, uint32_t width)
{
    switch (fmt)
    {
        case gfx_pixel_format_a8:               return width * sizeof(uint8_t);
        case gfx_pixel_format_rgb5a1:
        case gfx_pixel_format_rgb565:
        case gfx_pixel_format_rgba4444:         return width * sizeof(uint16_t);

        case gfx_pixel_format_rgba8:            return width * sizeof(uint32_t);

        case gfx_pixel_format_etc1:             return gfx_max(2, (width >> 2)) * 8;
        case gfx_pixel_format_etc2_rgba8:       return gfx_max(2, (width >> 2)) * 16;
        case gfx_pixel_format_etc2_rgb8a1:      return gfx_max(2, (width >> 2)) * 8;

        case gfx_pixel_format_bc1:              return gfx_max(1, width >> 2) * 8;
        case gfx_pixel_format_bc3:              return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc4:              return gfx_max(1, width >> 2) * 8;
        case gfx_pixel_format_bc5:              return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc6h:             return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc7:              return gfx_max(1, width >> 2) * 16;

        case gfx_pixel_format_astc4x4:          return block_count(width, 4) * 16;
        case gfx_pixel_format_astc5x5:          return block_count(width, 5) * 16;
        case gfx_pixel_format_astc6x6:          return block_count(width, 6) * 16;
        case gfx_pixel_format_astc8x8:          return block_count(width, 8) * 16;
        case gfx_pixel_format_astc10x10:        return block_count(width, 10) * 16;
        case gfx_pixel_format_astc12x12:        return block_count(width, 12) * 16;

        case gfx_pixel_format_r16f:             return width * sizeof(uint16_t);
        case gfx_pixel_format_rg16f:            return width * sizeof(uint16_t) * 2;
        case gfx_pixel_format_rgba16f:          return width * sizeof(uint16_t) * 4;

        case gfx_pixel_format_r32f:             return width * sizeof(float);
        case gfx_pixel_format_rg32f:            return width * sizeof(float) * 2;
        case gfx_pixel_format_rgba32f:          return width * sizeof(float) * 4;

        case gfx_pixel_format_d24x8:            return width * sizeof(uint32_t);
        case gfx_pixel_format_d24s8:            return width * sizeof(uint32_t);
    }
    return 0;
}

uint32_t gfx_utils_align_up(uint32_t n, uint32_t alignment)
{
    return ((n + alignment - 1) / alignment) * alignment;
}
#pragma endregion


static gfx_allocator_t* gfx_default_allocator()
{
    static gfx_allocator_t s_allocator = {};

    if(s_allocator.gfx_alloc == nullptr)
        s_allocator.gfx_alloc = [](size_t size, void * userdata)    { return calloc(1, size);  };
        
    if (s_allocator.gfx_free == nullptr)
        s_allocator.gfx_free = [](void* ptr, void* userdata)        { free(ptr);            };

    return &s_allocator;
}


#pragma region handle pool

typedef struct gfx_handle_t {
    union {
        uint64_t                handle;
        struct {
            uint32_t            pool_hash;
            uint16_t            index;
            uint16_t            generation;
        };
    };
} gfx_handle_t;
static_assert(sizeof(gfx_handle_t) == sizeof(uint64_t));


typedef struct gfx_handle_pool_t {
    size_t              size;
    void*               data;
    uint32_t            hash;

    size_t              capacity;
    size_t              item_size;

    size_t              used_slots;
    gfx_handle_t*       handles;
    uint32_t*           generation_counters;

    uint64_t*           bitmask;           // Number of uint64_t elements needed for the bitmask
    uint32_t            bitmask_words;     // Pointer to the bit arrays (0 = free, 1 = allocated)

    gfx_allocator_t*    allocator;
} gfx_handle_pool_t;


static uint32_t hash32(const char* str, size_t len)
{
    const uint8_t* data = (const uint8_t*)str;
    uint32_t hash = 0x811c9dc5; // FNV_offset_basis
    
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x01000193; // FNV_prime
    }
    
    return hash;
}


void gfx_handle_pool_create(uint32_t stride, uint32_t capacity, gfx_handle_pool_t** out_pool, gfx_allocator_t* allocator)
{
    if(allocator == nullptr)
        allocator = gfx_default_allocator();

    uint32_t alignment = alignof(void*);

    capacity = gfx_max(64, capacity);   // minimum, 64 elements

    uint32_t bitmask_words  = gfx_utils_align_up(capacity/64, 1);

    uint32_t pool_size      = gfx_utils_align_up(sizeof(gfx_handle_pool_t), alignment);
    uint32_t data_size      = gfx_utils_align_up(capacity * stride, alignment);
    uint32_t handles_size   = gfx_utils_align_up(capacity * sizeof(gfx_handle_t), alignment);
    uint32_t counters_size  = gfx_utils_align_up(capacity * sizeof(uint32_t), alignment);
    uint32_t bitmask_size   = gfx_utils_align_up(sizeof(uint64_t) * bitmask_words, 1);
    uint32_t total_size     = data_size + handles_size + counters_size + pool_size + bitmask_size;

    void* buffer = allocator->gfx_alloc(total_size, allocator->user_data);
    if (buffer == nullptr)
        return;
    memset(buffer, 0, total_size);

    uint8_t* byte_ptr = (uint8_t*)buffer;

    gfx_handle_pool_t* pool = (gfx_handle_pool_t*)byte_ptr;     byte_ptr += pool_size;
    pool->data = byte_ptr;                                      byte_ptr += data_size;
    pool->handles = (gfx_handle_t*)byte_ptr;                    byte_ptr += handles_size;
    pool->generation_counters = (uint32_t*)byte_ptr;            byte_ptr += counters_size;
    pool->bitmask = (uint64_t*)byte_ptr;                        byte_ptr += bitmask_size;

    pool->allocator     = allocator;
    pool->size          = stride * capacity;
    pool->item_size        = stride;
    pool->capacity      = capacity;
    pool->used_slots   = 0;
    pool->bitmask_words = bitmask_words;
    pool->hash          = hash32((char*)pool, sizeof(intptr_t));

    uintptr_t pool_address = (uintptr_t)pool;
    pool->hash = hash32((char*)&pool_address, sizeof(pool_address));

    for (uint32_t i = 0; i < capacity; ++i) {
        pool->handles[i].pool_hash  = pool->hash;
        pool->handles[i].index      = i;
        pool->handles[i].generation = pool->generation_counters[i];
    }

    *out_pool = pool;
}

void gfx_handle_pool_destroy(gfx_handle_pool_t* pool)
{
    if(pool == nullptr)
        return;

    gfx_allocator_t* allocator = pool->allocator;
    allocator->gfx_free(pool, allocator->user_data);
}

uint64_t gfx_pool_alloc(gfx_handle_pool_t* pool)
{
    if (pool == nullptr) return 0;
    if (pool->used_slots == pool->capacity) return 0; // No available chunk

    gfx_handle_t invalid_handle = {  };
    for(uint32_t i = 0; i < pool->bitmask_words; ++i)
    {
        uint64_t inv = ~pool->bitmask[i];
        if(inv == 0)
            continue;

        uint32_t index = gfx_utils_ctz64(inv) + i * 64;

        if (index >= pool->capacity) return 0;

        pool->handles[index].generation = pool->generation_counters[index];

        gfx_utils_bitmask_set(pool->bitmask, index, true);
        pool->used_slots++;
        return pool->handles[index].handle;
    }

    assert(false); // how do we get there?
    return 0;
}

void * gfx_handle_pool_allocate_data(gfx_handle_pool_t* pool, uint64_t* out_handle)
{
    uint64_t handle = gfx_pool_alloc(pool);
    if(out_handle) *out_handle = handle;
    return gfx_handle_pool_map(pool, handle);
}


void gfx_handle_pool_free(gfx_handle_pool_t* pool, uint64_t _handle)
{
    gfx_handle_t handle = { _handle };

    if (pool->used_slots == 0 || handle.index >= pool->capacity)
        return;

    if (pool->generation_counters[handle.index] != handle.generation) 
        return; // Handle is invalid

    gfx_utils_bitmask_set(pool->bitmask, handle.index, false);
    pool->generation_counters[handle.index]++;
    pool->used_slots--;
}

void* gfx_handle_pool_find_if(gfx_handle_pool_t* pool, gfx_handle_pool_predicate_fn predicate, void* userdata, uint64_t* out_handle)
{
    if(!pool || !predicate) return nullptr;
    if(out_handle)
        *out_handle = 0;

    for (uint32_t i = 0; i < pool->capacity; ++i) {
        if ((i % 64 == 0) && (pool->bitmask[i / 64] == 0)) {
            i += 63;
            continue;
        }
        if(gfx_utils_bitmask_value(pool->bitmask, i)){
         void* element_ptr = (void*)((char*)pool->data + (i * pool->item_size));
             if(predicate(element_ptr, userdata)){
                 if (out_handle)
                     *out_handle = pool->handles[i].handle;
                return element_ptr;
             }
        }
    }
    return nullptr;
}

void * gfx_handle_pool_map(gfx_handle_pool_t* pool, uint64_t _handle)
{
    gfx_handle_t handle = { _handle };
    if(handle.pool_hash != pool->hash)
        return nullptr;

    if (handle.index >= pool->capacity) 
        return nullptr;

    if (pool->generation_counters[handle.index] != handle.generation) 
        return nullptr; // Handle is invalid

    return (char*)pool->data + (handle.index * pool->item_size);
};

void* gfx_pool_get_data(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->data : 0;
}

size_t gfx_pool_get_stride(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->item_size : 0;
}

size_t gfx_handle_pool_get_size(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->used_slots : 0;
}

 size_t gfx_handle_pool_get_capacity(gfx_handle_pool_t* pool) {
     return (pool != nullptr) ? pool->capacity : 0;
 }

size_t gfx_pool_has_free(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? (pool->used_slots < pool->capacity) : 0;
}



/// <summary>
/// offset allocator
/// </summary>
typedef struct gfx_offset_allocator_t {
    uint32_t  size;             // Total managed size (e.g., in bytes)
    uint32_t  block_size;       // Quantum/granularity size of a single block
    uint32_t  blocks_count;     // Total number of blocks (size / block_size)
    uint32_t  bitmask_words;    // Number of uint64_t elements needed for the bitmask
    uint64_t* bitmask;          // Pointer to the bit arrays (0 = free, 1 = allocated)
    uint32_t* block_sizes;      // Track allocation lengths (in blocks) for large mesh buffers
} gfx_offset_allocator_t;


void gfx_offset_allocator_create(uint32_t size, uint32_t block_size, gfx_offset_allocator_t** allocator)
{
    if (!allocator || size == 0 || block_size == 0) return;

    if (size < block_size) {
        size = block_size;  // silently adjust or return error - decide based on policy
    }

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


void gfx_offset_allocator_destroy(gfx_offset_allocator_t* allocator)
{
    if (allocator) {
        free(allocator);
    }
}

intptr_t gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size)
{
    if (!allocator || size == 0)
        return -1;

    if (size > allocator->size)
        return -1;

    uint32_t needed_blocks =
        (size + allocator->block_size - 1) / allocator->block_size;

    if (needed_blocks > allocator->blocks_count)
        return -1;

    uint32_t run_length = 0;
    uint32_t start_block = 0;

    const uint32_t total_words = allocator->bitmask_words;

    for (uint32_t word_idx = 0; word_idx < total_words; ++word_idx)
    {
        uint64_t word = allocator->bitmask[word_idx];

        if (word == UINT64_MAX)
        {
            run_length = 0;
            continue;
        }

        if (word == 0)
        {
            if (run_length == 0)
                start_block = word_idx * 64;

            run_length += 64;

            if (run_length >= needed_blocks)
            {
                uint32_t start = start_block;

                // allocate range
                uint32_t end = start + needed_blocks;
                for (uint32_t b = start; b < end; ++b)
                    allocator->bitmask[b / 64] |= (1ULL << (b % 64));

                allocator->block_sizes[start] = needed_blocks;

                return (intptr_t)(start * allocator->block_size);
            }

            continue;
        }

        // PARTIAL WORD
        for (uint32_t bit = 0; bit < 64; ++bit)
        {
            uint32_t global_bit = word_idx * 64 + bit;
            if (global_bit >= allocator->blocks_count)
                break;

            if (!(word & (1ULL << bit)))
            {
                if (run_length == 0)
                    start_block = global_bit;

                run_length++;

                if (run_length >= needed_blocks)
                {
                    uint32_t start = start_block;
                    uint32_t end = start + needed_blocks;

                    for (uint32_t b = start; b < end; ++b)
                        allocator->bitmask[b / 64] |= (1ULL << (b % 64));

                    allocator->block_sizes[start] = needed_blocks;

                    return (intptr_t)(start * allocator->block_size);
                }
            }
            else
            {
                run_length = 0;
            }
        }
    }

    return -1;
}

#if 0
intptr_t gfx_offset_allocator_allocate(gfx_offset_allocator_t* allocator, uint32_t size)
{
    if (!allocator || size == 0) return -1;

    // Calculate how many blocks are needed for the requested size
    uint32_t needed_blocks = (size + allocator->block_size - 1) / allocator->block_size;

    uint32_t run_length = 0;
    uint32_t start_block = 0;
    uint32_t total_bits = allocator->bitmask_words * 64;

  /*  for(uint32_t i = 0; i < allocator->bitmask_words; ++i)
    {   
        uint64_t inv = ~allocator->bitmask[i];

        if (inv == 0)
            continue;

        int idx = 64 * i + ctz64(inv);
        start_block = idx;

        for(int j = idx; j < needed_blocks; ++j)
        {
            uint32_t bit_idx = i % 64;
            if (allocator->bitmask[j] & (1ULL << bit_idx))
            {
            //    start_block =
            }
            run_length++;
        }

        printf("");
        break;
    }*/

    // Linear scan through the bitmask to find a contiguous sequence of 0s
    for (uint32_t i = 0; i < total_bits; ++i) {
        if (i >= allocator->blocks_count) break;

        uint32_t word_idx = i / 64;
        uint32_t bit_idx  = i % 64;

        if (allocator->bitmask[word_idx] == UINT64_MAX) {
            run_length = 0;
            i += 63;
            continue;
        }

        if (allocator->bitmask[word_idx] & (1ULL << bit_idx)) {
            run_length = 0; // Block is occupied
        } else {
            if (run_length == 0) 
                start_block = i;
            run_length++;

            if (run_length == needed_blocks) {
                for (uint32_t b = start_block; b < start_block + needed_blocks; ++b) {
                    allocator->bitmask[b / 64] |= (1ULL << (b % 64));
                }

                allocator->block_sizes[start_block] = needed_blocks;

                return (intptr_t)(start_block * allocator->block_size);
            }
        }
    }
    return -1; // OOM
}
#endif

// Releases the allocated region back to the pool in true O(1) time
 void gfx_offset_allocator_free(gfx_offset_allocator_t* allocator, intptr_t offset)
{
    if (!allocator || offset < 0 || offset >= allocator->size)
        return;

    if (offset % allocator->block_size != 0)
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
#pragma endregion

#pragma region gfx



 typedef struct gfx_api_pfn
 {
     // CONTEXT
     void     (*pfn_init)            (gfx_settings_t* settings, gfx_context_t** ctx);
     void     (*pfn_destroy)         (gfx_context_t* ctx);
     void     (*pfn_get_caps)        (gfx_context_t* ctx, gfx_caps_t* caps);

     // SWAPCHAIN / SURFACE
     void     (*pfn_surface_create)  (gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface);
     void     (*pfn_surface_destroy) (gfx_context_t* ctx, gfx_surface_t* surface);

     void     (*pfn_frame_begin)     (gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame);
     void     (*pfn_frame_end)       (gfx_frame_t* frame);

     // BUFFER
     void     (*pfn_buffer_create)       (gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
     void     (*pfn_buffer_update_data)  (gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);
     void     (*pfn_buffer_destroy)      (gfx_context_t* ctx, gfx_buffer_t* buffer);

     // SHADER
     void    (*pfn_shader_create)                   (gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
     uint32_t(*pfn_shader_get_descriptor_set_count) (gfx_shader_t* shader);
     uint32_t(*pfn_shader_get_uniforms)             (gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms); // new
     uint64_t(*pfn_uniform_location)                (gfx_shader_t* shader, const char* name);
     void    (*pfn_shader_destroy)                  (gfx_context_t* ctx, gfx_shader_t* shader);

     // DESCRIPTOR SET
     void     (*pfn_descriptor_set_create)               (gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** descriptor);
     void     (*pfn_descriptor_set_write_buffer_data)    (gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
     void     (*pfn_descriptor_set_write_buffer)         (gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset);
     void     (*pfn_descriptor_set_write_texture)        (gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
     void     (*pfn_descriptor_set_write_sampler)        (gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);
     void     (*pfn_descriptor_set_destroy)              (gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);

     // SAMPLER
     void     (*pfn_sampler_create)  (gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
     void     (*pfn_sampler_destroy) (gfx_context_t* ctx, gfx_sampler_t* sampler);

     // TEXTURE
     void     (*pfn_texture_create)          (gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
     void     (*pfn_texture_update_data)     (gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset);
     void     (*pfn_texture_update_bindless) (gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx);
     void     (*pfn_texture_generate_mipmap) (gfx_context_t* ctx, gfx_texture_t* texture);
     void     (*pfn_texture_blit)            (gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst);
     void     (*pfn_texture_get_data)        (gfx_context_t* ctx, gfx_command_buffer_t* cmd);
     void     (*pfn_texture_destroy)         (gfx_context_t* ctx, gfx_texture_t* texture);

     // PIPELINE
     void     (*pfn_pipeline_create)          (gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
     void     (*pfn_mesh_pipeline_create)     (gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
     void     (*pfn_pipeline_destroy)         (gfx_context_t* ctx, gfx_pipeline_t* pipeline);

     void     (*pfn_pipeline_compute_create)  (gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline);
     void     (*pfn_pipeline_compute_destroy) (gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);

     void     (*pfn_pipeline_raytrace_create) (gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
     void     (*pfn_pipeline_raytrace_destroy)(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);

     // COMMAND BUFFER
     void     (*pfn_cmd_begin_pass)              (gfx_command_buffer_t* cmd, gfx_pass_info_t* pass_info);
     void     (*pfn_cmd_end_pass)                (gfx_command_buffer_t* cmd);

     void     (*pfn_cmd_scissor) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
     void     (*pfn_cmd_viewport) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
     void     (*pfn_cmd_bind_pipeline) (gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
     void     (*pfn_cmd_bind_descriptor_set) (gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);
     void     (*pfn_cmd_bind_index_buffer) (gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
     void     (*pfn_cmd_bind_vertex_buffer) (gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
     void     (*pfn_cmd_draw) (gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
     void     (*pfn_cmd_draw_indexed) (gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset);
     void     (*pfn_cmd_draw_indexed_indirect) (gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
     void     (*pfn_cmd_dispatch_compute) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

     // Mesh Shading
     void     (*pfn_cmd_draw_mesh_tasks)             (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);                    // new
     void     (*pfn_cmd_draw_mesh_tasks_indirect)    (gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride); // new

     // Ray Tracing
     void     (*pfn_acceleration_structure_create)   (gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc, gfx_acceleration_structure_t** acceleration_struct);
     void     (*pfn_acceleration_structure_destroy)  (gfx_context_t* ctx, gfx_acceleration_structure_t* acceleration_structure);
     void     (*pfn_sbt_create)                      (gfx_context_t* ctx, gfx_sbt_desc_t* desc, gfx_sbt_t** sbt);
     void     (*pfn_sbt_destroy)                     (gfx_context_t* ctx, gfx_sbt_t* sbt);
     void     (*pfn_cmd_build_acceleration_structure)(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t* dst, gfx_acceleration_structure_t* src);
     void     (*pfn_cmd_trace_rays)                  (gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth);
     void     (*pfn_cmd_trace_ray_query)             (gfx_command_buffer_t* cmd, gfx_acceleration_structure_t tlas, uint32_t width, uint32_t height, uint32_t depth); // new

     // Barriers
     void     (*pfn_cmd_buffer_barrier)      (gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
     void     (*pfn_cmd_texture_barrier)     (gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);

     // Debug
     void     (*pfn_cmd_push_marker)         (gfx_command_buffer_t* cmd, const char* marker);
     void     (*pfn_cmd_pop_marker)          (gfx_command_buffer_t* cmd);
     void     (*pfn_cmd_get_timestamps)      (gfx_command_buffer_t* cmd, gfx_timestamp_t* timestamps);

 } gfx_api_pfn;


static gfx_api_pfn * g_tbl = nullptr;

extern "C" void gfx_init_webgpu(gfx_api_pfn* func_table);
extern "C" void gfx_init_vulkan(gfx_api_pfn* func_table);
extern "C" void gfx_init_metal(gfx_api_pfn* func_table);
extern "C" void gfx_init_dx12(gfx_api_pfn* func_table);

gfx_api gfx_backend  gfx_detect_backend()
{
#if defined(__APPLE__)
    return gfx_backend_metal;
#elif defined(__ANDROID__)
    return gfx_backend_vulkan;
#elif defined(__EMSCRIPTEN__)
    return gfx_backend_webgpu;
#elif defined(_WIN32)
    return gfx_backend_vulkan;
#else
    #error "unknown gfx platform"
#endif
}

void gfx_init(gfx_settings_t* settings, gfx_context_t** ctx)
{
    if(g_tbl != nullptr) {
        _gfx_error(nullptr, gfx_msg_error, "Graphics subsystem is already initialized. Only one instance is allowed.");
        return;
    }

    g_tbl = (gfx_api_pfn*)calloc(1, sizeof(gfx_api_pfn));
    if(!g_tbl) {
        _gfx_error(nullptr, gfx_msg_error, "Failed to allocate memory for the graphics API function table.");
        return;
    }

    gfx_backend backend = settings->backend;
    if( backend == gfx_backend_auto )
        backend = gfx_detect_backend();

    settings->backend = backend;

    switch(settings->backend)
    {
        #ifdef VULKAN_AVAILABLE
        case gfx_backend_vulkan:    gfx_init_vulkan(g_tbl); break;
        #endif

        #ifdef WEBGPU_AVAILABLE
        case gfx_backend_webgpu:    gfx_init_webgpu(g_tbl); break;
        #endif

        #ifdef METAL_AVAILABLE
        case gfx_backend_metal:     gfx_init_metal(g_tbl);  break;
        #endif

        #ifdef DX12_AVAILABLE
        case gfx_backend_d3d12:     gfx_init_dx12(g_tbl);   break;
        #endif
    }

    g_tbl->pfn_init(settings, ctx);
}


// --- CONTEXT ---
void gfx_get_caps(gfx_context_t* ctx, gfx_caps_t* caps) {
    g_tbl->pfn_get_caps(ctx, caps);
}

// --- SWAPCHAIN ---

gfx_surface_t gfx_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc)
{
    gfx_surface_t * surface = nullptr;
    g_tbl->pfn_surface_create(ctx, desc, &surface);
    return { surface ->idx };
}

void gfx_surface_destroy(gfx_context_t* ctx, gfx_surface_t surface)
{
    g_tbl->pfn_surface_destroy(ctx, &surface);
}

gfx_frame_t*  gfx_begin_frame(gfx_context_t* ctx, gfx_surface_t * surface)
{
    gfx_frame_t * out_frame = nullptr;
    g_tbl->pfn_frame_begin(ctx, surface, &out_frame);
    return out_frame;
}

gfx_result gfx_end_frame(gfx_frame_t* frame)
{
    g_tbl->pfn_frame_end(frame);
    return gfx_ok;
}

// --- BUFFER ---
void gfx_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer) {
    g_tbl->pfn_buffer_create(ctx, desc, buffer);
}
gfx_api gfx_buffer_t* gfx_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc) {
    gfx_buffer_t* result = nullptr;
    g_tbl->pfn_buffer_create(ctx, desc, &result);
    return result;
}
gfx_api void gfx_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset) {
    g_tbl->pfn_buffer_update_data(ctx, buffer, data, size, offset);
}

void gfx_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer) {
    g_tbl->pfn_buffer_destroy(ctx, buffer);
}

// --- SHADER ---
gfx_api gfx_shader_t* gfx_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc) {
    gfx_shader_t* result = nullptr;
    g_tbl->pfn_shader_create(ctx, desc, &result);
    return result;
}

uint32_t gfx_shader_get_descriptor_set_count(gfx_shader_t* shader) {
    return g_tbl->pfn_shader_get_descriptor_set_count(shader);
}


uint32_t gfx_shader_get_uniforms(gfx_shader_t* shader, uint32_t group, gfx_uniform_t* uniforms) {
    assert(false);
    return 0;
}
uint64_t gfx_uniform_location(gfx_shader_t* shader, const char* name) {
    return g_tbl->pfn_uniform_location(shader, name);
}
void gfx_shader_destroy(gfx_context_t* ctx, gfx_shader_t* buffer) {
    g_tbl->pfn_shader_destroy(ctx, buffer);
}

// --- SAMPLER ---
gfx_api gfx_sampler_t* gfx_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc) {
    gfx_sampler_t* result = nullptr;
    g_tbl->pfn_sampler_create(ctx, desc, &result);
    return result;
}

void gfx_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler) {
    g_tbl->pfn_sampler_destroy(ctx, sampler);
}

// --- TEXTURE ---
gfx_api gfx_texture_t* gfx_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc) {
    gfx_texture_t* result = nullptr;
    g_tbl->pfn_texture_create(ctx, desc, &result);
    return result;
}

gfx_api void gfx_texture_update_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset) {
    if (!g_tbl || !g_tbl->pfn_texture_update_data) { gfx_stub_not_implemented(nullptr, "gfx_update_image_data"); return; }
    g_tbl->pfn_texture_update_data(ctx, texture, data, size, offset);
}

gfx_api void gfx_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx) {
    if (!g_tbl || !g_tbl->pfn_texture_update_bindless) { gfx_stub_not_implemented(nullptr, "gfx_update_bindless_texture"); return; }
    g_tbl->pfn_texture_update_bindless(ctx, texture, idx);
}

gfx_api void gfx_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture) {
    if (!g_tbl || !g_tbl->pfn_texture_generate_mipmap) { gfx_stub_not_implemented(nullptr, "gfx_texture_generate_mipmap"); return; }
    g_tbl->pfn_texture_generate_mipmap(ctx, texture);
}

gfx_api void gfx_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst) {
    if (!g_tbl || !g_tbl->pfn_texture_blit) { gfx_stub_not_implemented(nullptr, "gfx_blit_image"); return; }
    g_tbl->pfn_texture_blit(ctx, src, dst);
}

gfx_api void gfx_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd) {
    if (!g_tbl || !g_tbl->pfn_texture_get_data) { gfx_stub_not_implemented(nullptr, "gfx_texture_get_data"); return; }
    g_tbl->pfn_texture_get_data(ctx, cmd);
}

void gfx_texture_destroy(gfx_context_t* ctx, gfx_texture_t* texture) {
    g_tbl->pfn_texture_destroy(ctx, texture);
}

// --- PIPELINE ---
gfx_api gfx_pipeline_t* gfx_pipeline_create(gfx_context_t* ctx, gfx_pipeline_desc_t* desc) {
    gfx_pipeline_t* result = nullptr;
    g_tbl->pfn_pipeline_create(ctx, desc, &result);
    return result;
}


gfx_api gfx_pipeline_compute_t* gfx_compute_pipeline_create(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc) {
    gfx_pipeline_compute_t* pipeline = nullptr;
    g_tbl->pfn_pipeline_compute_create(ctx, desc, &pipeline);
    return pipeline;
}
void gfx_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_t* pipeline) {
    g_tbl->pfn_pipeline_destroy(ctx, pipeline);
}
void gfx_compute_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline) {
    if (!g_tbl || !g_tbl->pfn_pipeline_compute_destroy) { gfx_stub_not_implemented(nullptr, "gfx_compute_pipeline_destroy"); return; }
    g_tbl->pfn_pipeline_compute_destroy(ctx, pipeline);
}
gfx_api gfx_pipeline_t* gfx_pipeline_mesh_create(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc) {
    gfx_pipeline_t* result = nullptr;
    if (!g_tbl || !g_tbl->pfn_mesh_pipeline_create) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_mesh_create"); return result; }
    g_tbl->pfn_mesh_pipeline_create(ctx, desc, &result);
    return result;
}

gfx_api gfx_pipeline_raytrace_t* gfx_pipeline_raytrace_create(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc) {
    gfx_pipeline_raytrace_t* result = nullptr;
    if (!g_tbl || !g_tbl->pfn_pipeline_raytrace_create) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_raytrace_create"); return result; }
    g_tbl->pfn_pipeline_raytrace_create(ctx, desc, &result);
    return result;
}
gfx_api void gfx_pipeline_raytrace_destroy(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline) {
    if (!g_tbl || !g_tbl->pfn_pipeline_raytrace_destroy) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_raytrace_destroy"); return; }
    g_tbl->pfn_pipeline_raytrace_destroy(ctx, pipeline);
}


// --- DESCRIPTOR SET ---
gfx_api gfx_descriptor_set_t* gfx_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx) {
    gfx_descriptor_set_t* result = nullptr;
    g_tbl->pfn_descriptor_set_create(ctx, shader, set_idx, &result);
    return result;
}
void gfx_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size) {
    g_tbl->pfn_descriptor_set_write_buffer_data(set, handle, data, size);
}
void gfx_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset) {
    g_tbl->pfn_descriptor_set_write_buffer(set, handle, buffer, offset);
}
void gfx_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture) {
    g_tbl->pfn_descriptor_set_write_texture(set, handle, texture);
}
void gfx_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler) {
    g_tbl->pfn_descriptor_set_write_sampler(set, handle, sampler);
}
void gfx_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor) {
    g_tbl->pfn_descriptor_set_destroy(ctx, descriptor);
}

// --- COMMAND BUFFER ---

void gfx_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker) {
    g_tbl->pfn_cmd_push_marker(cmd, marker);
}
void gfx_cmd_pop_marker(gfx_command_buffer_t* cmd) {
    g_tbl->pfn_cmd_pop_marker(cmd);
}

void gfx_cmd_get_timestamps(gfx_command_buffer_t* cmd, gfx_timestamp_t* timestamps){
    return g_tbl->pfn_cmd_get_timestamps(cmd, timestamps);
}

void gfx_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* info) {
    g_tbl->pfn_cmd_begin_pass(cmd, info);
}
void gfx_cmd_end_pass(gfx_command_buffer_t* cmd) {
    g_tbl->pfn_cmd_end_pass(cmd);
}
void gfx_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    g_tbl->pfn_cmd_scissor(cmd, x, y, w, h);
}
void gfx_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    g_tbl->pfn_cmd_viewport(cmd, x, y, w, h);
}
void gfx_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline) {
    g_tbl->pfn_cmd_bind_pipeline(cmd, pipeline);
}
void gfx_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor) {
    g_tbl->pfn_cmd_bind_descriptor_set(cmd, slot, descriptor);
}
void gfx_cmd_bind_index_buffer(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer) {
    g_tbl->pfn_cmd_bind_index_buffer(cmd, format, offset, buffer);
}
void gfx_cmd_bind_vertex_buffer(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer) {
    g_tbl->pfn_cmd_bind_vertex_buffer(cmd, slot, offset, buffer);
}
void gfx_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count) {
    g_tbl->pfn_cmd_draw(cmd, vertex_count, instance_count);
}
void gfx_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset) {
    g_tbl->pfn_cmd_draw_indexed(cmd, idx_count, first_idx, instance_count, vertex_offset);
}
void gfx_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride) {
    g_tbl->pfn_cmd_draw_indexed_indirect(cmd, buffer, offset, draw_count, stride);
}
void gfx_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z) {
    g_tbl->pfn_cmd_dispatch_compute(cmd, x, y, z);
}
void gfx_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst) {
    g_tbl->pfn_cmd_buffer_barrier(cmd, buffers, count, src, dst);
}
void gfx_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst) {
    g_tbl->pfn_cmd_texture_barrier(cmd, textures, count, src, dst);
}

gfx_acceleration_structure_t gfx_acceleration_structure_create(gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc)
{
    gfx_acceleration_structure_t *as = nullptr;
    g_tbl->pfn_acceleration_structure_create(ctx, desc, &as);
    return *as;
}


void gfx_acceleration_structure_destroy(gfx_context_t* ctx, gfx_acceleration_structure_t acceleration_structure)
{
    g_tbl->pfn_acceleration_structure_destroy(ctx, &acceleration_structure);
}


gfx_sbt_t gfx_sbt_create(gfx_context_t* ctx, gfx_sbt_desc_t* desc)
{
    gfx_sbt_t *sbt = nullptr;
    g_tbl->pfn_sbt_create(ctx, desc, &sbt);
    return *sbt;
}


void gfx_sbt_destroy(gfx_context_t* ctx, gfx_sbt_t sbt)
{
    g_tbl->pfn_sbt_destroy(ctx, &sbt);
}


void gfx_cmd_build_acceleration_structure(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t dst, gfx_acceleration_structure_t src)
{
    g_tbl->pfn_cmd_build_acceleration_structure(cmd, &dst, &src);
}


void gfx_cmd_trace_rays(gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth)
{
    g_tbl->pfn_cmd_trace_rays(cmd, pipeline, sbt, width, height, depth);
}


void gfx_cmd_trace_ray_query(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t tlas, uint32_t width, uint32_t height, uint32_t depth)
{
    g_tbl->pfn_cmd_trace_ray_query(cmd, tlas, width, height, depth);
}


#ifdef VULKAN_AVAILABLE
#include "gfx_vulkan.h"
#endif

#define assign_extern(pfn_dst, ret_type, func_name, func_args)  { extern ret_type func_name func_args; pfn_dst = func_name; }

void gfx_init_vulkan(gfx_api_pfn* func_table)
{
    memset(func_table, 0, sizeof(gfx_api_pfn));
#ifdef VULKAN_AVAILABLE
    assign_extern( func_table->pfn_init,     void, vk_create_renderer,  (gfx_settings_t * settings, gfx_context_t ** ctx) );
    assign_extern( func_table->pfn_destroy,  void, vk_destroy_renderer, (gfx_context_t * ctx) );
    assign_extern( func_table->pfn_get_caps, void, vk_get_caps,         (gfx_context_t * ctx, gfx_caps_t * caps) );

    // SURFACE
    assign_extern(func_table->pfn_surface_create,  void, vk_surface_create, (gfx_context_t * ctx, gfx_surface_desc_t * desc, gfx_surface_t ** out_surface) );
    assign_extern(func_table->pfn_surface_destroy, void, vk_surface_destroy, (gfx_context_t * ctx, gfx_surface_t * surface) );

    // CONTEXT
    assign_extern(func_table->pfn_frame_begin,  void, vk_frame_begin, (gfx_context_t * ctx, gfx_surface_t * in_surface, gfx_frame_t * *out_frame));
    assign_extern(func_table->pfn_frame_end,    void, vk_frame_end, (gfx_frame_t * frame));

    // BUFFER
    assign_extern(func_table->pfn_buffer_create,      void, vk_buffer_create,       (gfx_context_t * ctx, gfx_buffer_desc_t * desc, gfx_buffer_t * *out_buffer));
    assign_extern(func_table->pfn_buffer_update_data, void, vk_buffer_update_data,  (gfx_context_t * ctx, gfx_buffer_t * buffer, void* data, uint32_t size, uint32_t offset));
    assign_extern(func_table->pfn_buffer_destroy,     void, vk_buffer_destroy,      (gfx_context_t * ctx, gfx_buffer_t * buffer));

    // SHADER
    func_table->pfn_shader_create           = vk_shader_create;
    func_table->pfn_shader_get_descriptor_set_count = vk_shader_get_descriptor_set_count;
    func_table->pfn_uniform_location        = vk_uniform_location;
    func_table->pfn_shader_destroy          = vk_shader_destroy;

    // SAMPLER
    func_table->pfn_sampler_create          = vk_sampler_create;
    func_table->pfn_sampler_destroy         = vk_sampler_destroy;

    // TEXTURE
    func_table->pfn_texture_create          = vk_texture_create;
    func_table->pfn_texture_update_data     = vk_texture_update_data;
    func_table->pfn_texture_update_bindless = vk_texture_update_bindless;
    func_table->pfn_texture_generate_mipmap = vk_texture_generate_mipmap;
    func_table->pfn_texture_blit            = vk_texture_blit;
    func_table->pfn_texture_get_data        = vk_texture_get_data;
    func_table->pfn_texture_destroy         = vk_texture_destroy;

    // PIPELINE
    func_table->pfn_pipeline_create          = vk_create_pipeline;
    func_table->pfn_pipeline_compute_create  = vk_create_compute_pipeline;
    func_table->pfn_mesh_pipeline_create     = vk_create_mesh_pipeline;
    func_table->pfn_pipeline_raytrace_create = vk_create_raytrace_pipeline;
    func_table->pfn_pipeline_destroy         = vk_destroy_pipeline;
    func_table->pfn_pipeline_compute_destroy = vk_destroy_compute_pipeline;
    func_table->pfn_pipeline_raytrace_destroy= vk_destroy_raytrace_pipeline;

    // DESCRIPTOR SET
    func_table->pfn_descriptor_set_create       = vk_descriptor_set_create;
    func_table->pfn_descriptor_set_write_buffer = vk_descriptor_set_write_buffer;
    func_table->pfn_descriptor_set_write_buffer_data = vk_descriptor_set_write_buffer_data;
    func_table->pfn_descriptor_set_write_texture     = vk_descriptor_set_write_texture;
    func_table->pfn_descriptor_set_write_sampler     = vk_descriptor_set_write_sampler;
    func_table->pfn_descriptor_set_destroy = vk_descriptor_set_destroy;


    // COMMAND BUFFER
    func_table->pfn_cmd_begin_pass          = vk_cmd_begin_pass;
    func_table->pfn_cmd_end_pass            = vk_cmd_end_pass;

    func_table->pfn_cmd_scissor             = vk_cmd_scissor;
    func_table->pfn_cmd_viewport            = vk_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline       = vk_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set = vk_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_index_buffer   = vk_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_vertex_buffer  = vk_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw                = vk_cmd_draw;
    func_table->pfn_cmd_draw_indexed        = vk_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect = vk_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute    = vk_cmd_dispatch_compute;

    func_table->pfn_cmd_push_marker         = vk_cmd_push_marker;
    func_table->pfn_cmd_pop_marker          = vk_cmd_pop_marker;

    func_table->pfn_cmd_buffer_barrier      = vk_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier     = vk_cmd_texture_barrier;
#endif
}



#if __has_include("gfx_webgpu.h")
 #include "gfx_webgpu.h"
#endif 

void gfx_init_webgpu(gfx_api_pfn* func_table)
{
    memset(func_table, 0, sizeof(gfx_api_pfn));

#if __has_include("gfx_webgpu.h")
    // CONTEXT
    func_table->pfn_init                    = wgpu_init;

    // BUFFER
    func_table->pfn_buffer_create           = wgpu_create_buffer;
    func_table->pfn_buffer_update_data      = wgpu_update_buffer_data;
    func_table->pfn_buffer_destroy          = wgpu_destroy_buffer;

    // SHADER
    func_table->pfn_shader_create           = wgpu_create_shader;
    func_table->pfn_uniform_location        = wgpu_uniform_location;
    func_table->pfn_shader_destroy          = wgpu_destroy_shader;

    // SAMPLER
    func_table->pfn_sampler_create          = wgpu_create_sampler;
    func_table->pfn_sampler_destroy         = wgpu_destroy_sampler;

    // TEXTURE
    func_table->pfn_texture_create          = wgpu_create_texture;
    func_table->pfn_texture_update_data     = wgpu_update_texture_data;
    func_table->pfn_texture_update_bindless = wgpu_update_bindless_texture;
    func_table->pfn_texture_generate_mipmap = wgpu_texture_generate_mipmap;
    func_table->pfn_texture_blit              = wgpu_blit_image;
    func_table->pfn_texture_get_data        = wgpu_texture_get_data;
    func_table->pfn_texture_destroy         = wgpu_destroy_texture;

    // PIPELINE
    func_table->pfn_pipeline_create         = wgpu_create_pipeline;
    func_table->pfn_pipeline_compute_create  = wgpu_create_compute_pipeline;
    func_table->pfn_mesh_pipeline_create     = wgpu_create_mesh_pipeline;
    func_table->pfn_pipeline_raytrace_create = wgpu_create_raytrace_pipeline;
    func_table->pfn_pipeline_destroy         = wgpu_destroy_pipeline;
    func_table->pfn_pipeline_compute_destroy = wgpu_destroy_compute_pipeline;
    func_table->pfn_pipeline_raytrace_destroy= wgpu_destroy_raytrace_pipeline;


    // DESCRIPTOR SET
    func_table->pfn_descriptor_set_create   = wgpu_create_descriptor_set;
    func_table->pfn_descriptor_set_write_buffer      = wgpu_uniform_set_buffer;
    func_table->pfn_descriptor_set_write_buffer_data = wgpu_uniform_update_buffer_data;
    func_table->pfn_descriptor_set_write_texture     = wgpu_uniform_set_texture;
    func_table->pfn_descriptor_set_write_sampler     = wgpu_uniform_set_sampler;
    func_table->pfn_descriptor_set_destroy  = wgpu_destroy_descriptor_set;

    // COMMAND BUFFER
    func_table->pfn_cmd_begin_pass          = wgpu_cmd_begin_pass;
    func_table->pfn_cmd_end_pass            = wgpu_cmd_end_pass;

    func_table->pfn_cmd_scissor               = wgpu_cmd_scissor;
    func_table->pfn_cmd_viewport              = wgpu_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline         = wgpu_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set   = wgpu_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_index_buffer        = wgpu_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_vertex_buffer        = wgpu_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw                  = wgpu_cmd_draw;
    func_table->pfn_cmd_draw_indexed          = wgpu_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect = wgpu_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute      = wgpu_cmd_dispatch_compute;

    func_table->pfn_cmd_push_marker         = wgpu_cmd_push_marker;
    func_table->pfn_cmd_pop_marker          = wgpu_cmd_pop_marker;

    func_table->pfn_cmd_buffer_barrier      = wgpu_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier     = wgpu_cmd_texture_barrier;
#endif
}

