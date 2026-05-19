#include "gfx.h"

#ifdef GFX_PLATFORM_WIN
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#endif

#include <stdio.h>
#include <math.h>
#include <memory.h> // memset
#include <atomic>
//#include <thread>

#include "gfx_stub.h"

#ifndef __cplusplus
    #define nullptr     NULL
#endif

#if __has_include(<vulkan/vulkan.h>)
#define VULKAN_AVAILABLE
#endif

#if __has_include(<webgpu/webgpu.h>)
#define WEBGPU_AVAILABLE
#endif


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
            uint16_t            index;
            uint16_t            hash;
            uint16_t            generation;
            uint16_t            flag;
        };
    };
} gfx_handle_t;
static_assert(sizeof(gfx_handle_t) == sizeof(uint64_t));


typedef struct gfx_handle_pool_t {
    size_t              size;
    void*               data;

    size_t              capacity;
    size_t              stride;
    uint16_t            hash;

    size_t              used_chunks;
    gfx_handle_t*       handles;
    uint32_t*           free_list;
    uint32_t*           generation_counters;

    gfx_allocator_t*    allocator;
} gfx_handle_pool_t;


static uint16_t hash16(const char* str, size_t len)
{
    int hash = 0;
    for (int i = 0; i < len; i++)
    {
        hash = hash + ((hash) << 5) + (str[i] + i) + (((str[i] + i)) << 7);
    }

    return ((hash) ^ (hash >> 16)) & 0xffff;
}


void gfx_pool_create(size_t stride, size_t capacity, gfx_handle_pool_t** out_pool, gfx_allocator_t* allocator)
{
    if(allocator == nullptr)
        allocator = gfx_default_allocator();

    gfx_handle_pool_t * pool = (gfx_handle_pool_t*)allocator->gfx_alloc(sizeof(gfx_handle_pool_t), allocator->user_data);

    if(pool == nullptr || allocator == nullptr)
        return;

    pool->allocator             = allocator;
    pool->size                  = stride * capacity;
    pool->stride                = stride;
    pool->capacity              = capacity;
    pool->used_chunks           = 0;
    pool->hash                  = hash16((char*)pool, sizeof(intptr_t));
    pool->data                  =                allocator->gfx_alloc(capacity * stride, allocator->user_data);
    pool->handles               = (gfx_handle_t*)allocator->gfx_alloc(capacity * sizeof(gfx_handle_t), allocator->user_data);
    pool->free_list             = (uint32_t*)    allocator->gfx_alloc(capacity * sizeof(uint32_t), allocator->user_data);
    pool->generation_counters   = (uint32_t*)    allocator->gfx_alloc(capacity * sizeof(uint32_t), allocator->user_data);

    if (!pool->data || !pool->handles || !pool->free_list || !pool->generation_counters) {
        gfx_pool_destroy(pool);
        return;
    }

    for (uint32_t i = 0; i < capacity; ++i) {
        pool->free_list[i] = i;
        pool->handles[i].index = i;
        pool->handles[i].flag = 0;
        pool->handles[i].hash = pool->hash;
        pool->handles[i].generation = pool->generation_counters[i];
    }

    *out_pool = pool;
}

void gfx_pool_destroy(gfx_handle_pool_t* pool)
{
    if(pool == nullptr)
        return;

    gfx_allocator_t* allocator = pool->allocator;
    pool->allocator = nullptr;
    allocator->gfx_free(pool->data, allocator->user_data);
    allocator->gfx_free(pool->handles, allocator->user_data);
    allocator->gfx_free(pool->free_list, allocator->user_data);
    allocator->gfx_free(pool->generation_counters, allocator->user_data);
    allocator->gfx_free(pool, allocator->user_data);
}

uint64_t gfx_pool_alloc(gfx_handle_pool_t* pool)
{
    if(pool->used_chunks >= pool->capacity)
        return 0;

    gfx_handle_t invalid_handle = {  };
    if (pool->used_chunks == pool->capacity) return 0; // No available chunk

    uint32_t index = pool->free_list[pool->used_chunks++];
    pool->handles[index].generation = pool->generation_counters[index];

    return pool->handles[index].handle;
}

void gfx_pool_free(gfx_handle_pool_t* pool, uint64_t _handle)
{
    gfx_handle_t handle = { _handle };

    if (pool->used_chunks == 0 || handle.index >= pool->capacity)
        return;

    if (pool->generation_counters[handle.index] != handle.generation) 
        return; // Handle is invalid

    pool->free_list[--pool->used_chunks] = handle.index;
    pool->generation_counters[handle.index]++;
}

void * gfx_pool_map(gfx_handle_pool_t* pool, uint64_t _handle)
{
    gfx_handle_t handle = { _handle };
    if(handle.hash != pool->hash)
        return nullptr;

    if (handle.index >= pool->capacity) 
        return nullptr;

    if (pool->generation_counters[handle.index] != handle.generation) 
        return nullptr; // Handle is invalid

    return (char*)pool->data + (handle.index * pool->stride);
};

void* gfx_pool_get_data(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->data : 0;
}

size_t gfx_pool_get_stride(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->stride : 0;
}

size_t gfx_pool_get_size(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? pool->used_chunks : 0;
}

 size_t gfx_pool_get_capacity(gfx_handle_pool_t* pool) {
     return (pool != nullptr) ? pool->capacity : 0;
 }

size_t gfx_pool_has_free(gfx_handle_pool_t* pool) {
    return (pool != nullptr) ? (pool->used_chunks < pool->capacity) : 0;
}

#pragma endregion

#pragma region gfx


static gfx_api_pfn * g_tbl = nullptr;

extern void gfx_init_webgpu(gfx_api_pfn* func_table);
extern void gfx_init_vulkan(gfx_api_pfn* func_table);
extern void gfx_init_metal(gfx_api_pfn* func_table);
extern void gfx_init_dx12(gfx_api_pfn* func_table);


gfx_api gfx_backend  gfx_detect_backend()
{
#if defined(GFX_PLATFORM_APPLE)
    return gfx_backend_metal;
#elif defined(GFX_PLATFORM_ANDROID)
    return gfx_backend_vulkan;
#elif defined(GFX_PLATFORM_WEB)
    return gfx_backend_webgpu;
#elif defined(GFX_PLATFORM_WIN)
    return gfx_backend_vulkan;
#else
    #error "unknown gfx platform"
#endif
}

void gfx_init(gfx_settings_t* settings, gfx_context_t** ctx)
{
    g_tbl = (gfx_api_pfn*)calloc(1, sizeof(gfx_api_pfn));
    if(!g_tbl)
        return;

    #ifndef _WIN32
    settings->backend = gfx_backend_auto;
    #endif

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
    g_tbl->pfn_create_buffer(ctx, desc, buffer);
}
gfx_api gfx_buffer_t* gfx_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc) {
    gfx_buffer_t* result = nullptr;
    g_tbl->pfn_create_buffer(ctx, desc, &result);
    return result;
}
gfx_api void gfx_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset) {
    g_tbl->pfn_update_buffer_data(ctx, buffer, data, size, offset);
}

void gfx_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer) {
    g_tbl->pfn_destroy_buffer(ctx, buffer);
}

// --- SHADER ---
gfx_api gfx_shader_t* gfx_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc) {
    gfx_shader_t* result = nullptr;
    g_tbl->pfn_create_shader(ctx, desc, &result);
    return result;
}
uint32_t gfx_shader_get_uniforms(gfx_shader_t* shader, uint32_t group, gfx_uniform_t* uniforms) {
    assert(false);
    return 0;
}
uint64_t gfx_uniform_location(gfx_shader_t* shader, const char* name) {
    return g_tbl->pfn_uniform_location(shader, name);
}
void gfx_shader_destroy(gfx_context_t* ctx, gfx_shader_t* buffer) {
    g_tbl->pfn_destroy_shader(ctx, buffer);
}

// --- SAMPLER ---
gfx_api gfx_sampler_t* gfx_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc) {
    gfx_sampler_t* result = nullptr;
    g_tbl->pfn_create_sampler(ctx, desc, &result);
    return result;
}

void gfx_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler) {
    g_tbl->pfn_destroy_sampler(ctx, sampler);
}

// --- TEXTURE ---
gfx_api gfx_texture_t* gfx_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc) {
    gfx_texture_t* result = nullptr;
    g_tbl->pfn_create_texture(ctx, desc, &result);
    return result;
}

gfx_api void gfx_texture_update_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset) {
    if (!g_tbl || !g_tbl->pfn_update_texture_data) { gfx_stub_not_implemented(nullptr, "gfx_update_image_data"); return; }
    g_tbl->pfn_update_texture_data(ctx, texture, data, size, offset);
}

gfx_api void gfx_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx) {
    if (!g_tbl || !g_tbl->pfn_update_bindless_texture) { gfx_stub_not_implemented(nullptr, "gfx_update_bindless_texture"); return; }
    g_tbl->pfn_update_bindless_texture(ctx, texture, idx);
}

gfx_api void gfx_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture) {
    if (!g_tbl || !g_tbl->pfn_texture_generate_mipmap) { gfx_stub_not_implemented(nullptr, "gfx_texture_generate_mipmap"); return; }
    g_tbl->pfn_texture_generate_mipmap(ctx, texture);
}

gfx_api void gfx_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst) {
    if (!g_tbl || !g_tbl->pfn_blit_image) { gfx_stub_not_implemented(nullptr, "gfx_blit_image"); return; }
    g_tbl->pfn_blit_image(ctx, src, dst);
}

gfx_api void gfx_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd) {
    if (!g_tbl || !g_tbl->pfn_texture_get_data) { gfx_stub_not_implemented(nullptr, "gfx_texture_get_data"); return; }
    g_tbl->pfn_texture_get_data(ctx, cmd);
}

void gfx_texture_destroy(gfx_context_t* ctx, gfx_texture_t* texture) {
    g_tbl->pfn_destroy_texture(ctx, texture);
}

// --- PIPELINE ---
gfx_api gfx_pipeline_t* gfx_pipeline_create(gfx_context_t* ctx, gfx_pipeline_desc_t* desc) {
    gfx_pipeline_t* result = nullptr;
    g_tbl->pfn_create_pipeline(ctx, desc, &result);
    return result;
}


gfx_api gfx_pipeline_compute_t* gfx_compute_pipeline_create(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc) {
    gfx_pipeline_compute_t* pipeline = nullptr;
    g_tbl->pfn_create_compute_pipeline(ctx, desc, &pipeline);
    return pipeline;
}
void gfx_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_t* pipeline) {
    g_tbl->pfn_destroy_pipeline(ctx, pipeline);
}
void gfx_compute_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline) {
    if (!g_tbl || !g_tbl->pfn_destroy_compute_pipeline) { gfx_stub_not_implemented(nullptr, "gfx_compute_pipeline_destroy"); return; }
    g_tbl->pfn_destroy_compute_pipeline(ctx, pipeline);
}
gfx_api gfx_pipeline_mesh_t* gfx_pipeline_mesh_create(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc) {
    gfx_pipeline_mesh_t* result = nullptr;
    if (!g_tbl || !g_tbl->pfn_create_mesh_pipeline) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_mesh_create"); return result; }
    g_tbl->pfn_create_mesh_pipeline(ctx, desc, &result);
    return result;
}
gfx_api void gfx_pipeline_mesh_destroy(gfx_context_t* ctx, gfx_pipeline_mesh_t* pipeline) {
    if (!g_tbl || !g_tbl->pfn_destroy_mesh_pipeline) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_mesh_destroy"); return; }
    g_tbl->pfn_destroy_mesh_pipeline(ctx, pipeline);
}
gfx_api gfx_pipeline_raytrace_t* gfx_pipeline_raytrace_create(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc) {
    gfx_pipeline_raytrace_t* result = nullptr;
    if (!g_tbl || !g_tbl->pfn_create_raytrace_pipeline) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_raytrace_create"); return result; }
    g_tbl->pfn_create_raytrace_pipeline(ctx, desc, &result);
    return result;
}
gfx_api void gfx_pipeline_raytrace_destroy(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline) {
    if (!g_tbl || !g_tbl->pfn_destroy_raytrace_pipeline) { gfx_stub_not_implemented(nullptr, "gfx_pipeline_raytrace_destroy"); return; }
    g_tbl->pfn_destroy_raytrace_pipeline(ctx, pipeline);
}

// --- RENDER TARGET ---
gfx_api gfx_render_target_t* gfx_render_target_create(gfx_context_t* ctx, gfx_render_target_desc_t* desc) {
    gfx_render_target_t* result = nullptr;
    g_tbl->pfn_create_render_target(ctx, desc, &result);
    return result;
}
void gfx_render_target_destroy(gfx_context_t* ctx, gfx_render_target_t* target) {
    g_tbl->pfn_destroy_render_target(ctx, target);
}

// --- DESCRIPTOR SET ---
gfx_api gfx_descriptor_set_t* gfx_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx) {
    gfx_descriptor_set_t* result = nullptr;
    g_tbl->pfn_create_descriptor_set(ctx, shader, set_idx, &result);
    return result;
}
void gfx_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size) {
    g_tbl->pfn_uniform_set_buffer_data(set, handle, data, size);
}
void gfx_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t size) {
    g_tbl->pfn_uniform_set_buffer(set, handle, buffer, size);
}
void gfx_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture) {
    g_tbl->pfn_uniform_set_texture(set, handle, texture);
}
void gfx_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler) {
    g_tbl->pfn_uniform_set_sampler(set, handle, sampler);
}
void gfx_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor) {
    g_tbl->pfn_destroy_descriptor_set(ctx, descriptor);
}

// --- COMMAND BUFFER ---
gfx_api gfx_command_buffer_t* gfx_cmd_create(gfx_context_t* ctx) {
    gfx_command_buffer_t* result = nullptr;
    g_tbl->pfn_create_cmd(ctx, &result);
    return result;
}

void gfx_cmd_destroy(gfx_context_t* ctx, gfx_command_buffer_t* cmd) {
    g_tbl->pfn_destroy_cmd(ctx, cmd);
}

void gfx_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker) {
    g_tbl->pfn_cmd_push_marker(cmd, marker);
}
void gfx_cmd_pop_marker(gfx_command_buffer_t* cmd) {
    g_tbl->pfn_cmd_pop_marker(cmd);
}
void gfx_cmd_begin(gfx_command_buffer_t* cmd) {
    g_tbl->pfn_cmd_begin(cmd);
}
void gfx_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_render_target_t* target) {
    g_tbl->pfn_cmd_begin_pass(cmd, target);
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
    g_tbl->pfn_cmd_bind_buffer_ib(cmd, format, offset, buffer);
}
void gfx_cmd_bind_vertex_buffer(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer) {
    g_tbl->pfn_cmd_bind_buffer_vb(cmd, slot, offset, buffer);
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
void gfx_cmd_end(gfx_command_buffer_t* cmd) {
    g_tbl->pfn_cmd_end(cmd);
}
void gfx_cmd_submit(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options) {
    g_tbl->pfn_submit_cmd(ctx, cmd, options);
}


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

uint32_t gfx_utils_stack_trace(uint32_t skip, uintptr_t* frames, uint64_t count)
{
#ifdef GFX_PLATFORM_WIN
    static bool lazyinit = false;
    if (!lazyinit) {
        SymInitialize(GetCurrentProcess(), NULL, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        lazyinit = true;
    }
    return RtlCaptureStackBackTrace(skip, (DWORD)count, (PVOID*)frames, NULL);
#endif
    return 0;
}

void gfx_utils_stack_trace_names(uintptr_t* frames, uint64_t count, const char** names)
{
#ifdef GFX_PLATFORM_WIN
    HANDLE hprocess = GetCurrentProcess();
    char tmpbuffer[sizeof(SYMBOL_INFO) + 64] = "";
    for (uint64_t i = 0; i < count; ++i)
    {
        DWORD ldsp = 0;
        IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)tmpbuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 64;

        // SymGetLineFromAddr64(hprocess, adress, &ldsp, &line);
        SymFromAddr(hprocess, frames[i], 0, symbol);
        printf("\n\t%s", symbol->Name);
    }
#endif
}

uint16_t gfx_utils_hash_16(const char* data, uint32_t size)
{
    return hash16(data, size);
}


static uint32_t gfx_max(uint32_t a, uint32_t b)            { return  (a > b ? a : b); }

static uint32_t gfx_block_count(uint32_t s, uint32_t b)    { return ((s + b - 1) / b); }

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

        case gfx_pixel_format_astc4x4:          return gfx_block_count(w, 4)  * gfx_block_count(h, 4)  * gfx_block_count(d, 4) * 16;
        case gfx_pixel_format_astc5x5:          return gfx_block_count(w, 5)  * gfx_block_count(h, 5)  * gfx_block_count(d, 5) * 16;
        case gfx_pixel_format_astc6x6:          return gfx_block_count(w, 6)  * gfx_block_count(h, 6)  * gfx_block_count(d, 6) * 16;
        case gfx_pixel_format_astc8x8:          return gfx_block_count(w, 8)  * gfx_block_count(h, 8)  * gfx_block_count(d, 8) * 16;
        case gfx_pixel_format_astc10x10:        return gfx_block_count(w, 10) * gfx_block_count(h, 10) * gfx_block_count(d, 10) * 16;
        case gfx_pixel_format_astc12x12:        return gfx_block_count(w, 12) * gfx_block_count(h, 12) * gfx_block_count(d, 12) * 16;

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

        case gfx_pixel_format_etc1:             return (gfx_max(2, (width >> 2)) * 8);
        case gfx_pixel_format_etc2_rgba8:       return (gfx_max(2, (width >> 2)) * 16);
        case gfx_pixel_format_etc2_rgb8a1:      return (gfx_max(2, (width >> 2)) * 8);


        case gfx_pixel_format_bc1:              return gfx_max(1, width >> 2) * 8;
        case gfx_pixel_format_bc3:              return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc4:              return gfx_max(1, width >> 2) * 8;
        case gfx_pixel_format_bc5:              return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc6h:             return gfx_max(1, width >> 2) * 16;
        case gfx_pixel_format_bc7:              return gfx_max(1, width >> 2) * 16;

        case gfx_pixel_format_astc4x4:          return gfx_block_count(width, 4)  * 16;
        case gfx_pixel_format_astc5x5:          return gfx_block_count(width, 5)  * 16;
        case gfx_pixel_format_astc6x6:          return gfx_block_count(width, 6)  * 16;
        case gfx_pixel_format_astc8x8:          return gfx_block_count(width, 8)  * 16;
        case gfx_pixel_format_astc10x10:        return gfx_block_count(width, 10) * 16;
        case gfx_pixel_format_astc12x12:        return gfx_block_count(width, 12) * 16;

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
  //  return (n + alignment - 1) & ~(alignment - 1);
}
#pragma endregion


#ifdef VULKAN_AVAILABLE
#include "gfx_vulkan.h"

void gfx_init_vulkan(gfx_api_pfn* func_table)
{
    memset(func_table, 0, sizeof(gfx_api_pfn));

    // CONTEXT
    func_table->pfn_init                    = vk_create_renderer;
    func_table->pfn_get_caps                = vk_get_caps;

    func_table->pfn_surface_create          = vk_surface_create;

    func_table->pfn_frame_begin             = vk_frame_begin;
    func_table->pfn_frame_end               = vk_frame_end;

    // BUFFER
    func_table->pfn_create_buffer           = vk_create_buffer;
    func_table->pfn_update_buffer_data      = vk_update_buffer_data;
    func_table->pfn_destroy_buffer          = vk_destroy_buffer;

    // SHADER
    func_table->pfn_create_shader           = vk_create_shader;
    func_table->pfn_uniform_location        = vk_uniform_location;
    func_table->pfn_destroy_shader          = vk_destroy_shader;

    // SAMPLER
    func_table->pfn_create_sampler          = vk_create_sampler;
    func_table->pfn_destroy_sampler         = vk_destroy_sampler;

    // TEXTURE
    func_table->pfn_create_texture          = vk_create_texture;
    func_table->pfn_update_texture_data     = vk_update_texture_data;
    func_table->pfn_update_bindless_texture = vk_texture_update_bindless;
    func_table->pfn_texture_generate_mipmap = vk_texture_generate_mipmap;
    func_table->pfn_blit_image              = vk_texture_blit;
    func_table->pfn_texture_get_data        = vk_texture_get_data;
    func_table->pfn_destroy_texture         = vk_destroy_texture;

    // PIPELINE
    func_table->pfn_create_pipeline         = vk_create_pipeline;
    func_table->pfn_create_compute_pipeline  = vk_create_compute_pipeline;
    func_table->pfn_create_mesh_pipeline     = vk_create_mesh_pipeline;
    func_table->pfn_create_raytrace_pipeline = vk_create_raytrace_pipeline;
    func_table->pfn_destroy_pipeline         = vk_destroy_pipeline;
    func_table->pfn_destroy_compute_pipeline = vk_destroy_compute_pipeline;
    func_table->pfn_destroy_mesh_pipeline    = vk_destroy_mesh_pipeline;
    func_table->pfn_destroy_raytrace_pipeline= vk_destroy_raytrace_pipeline;

    // RENDER TARGET
    func_table->pfn_create_render_target    = vk_create_render_target;
    func_table->pfn_destroy_render_target   = vk_destroy_render_target;

    // DESCRIPTOR SET
    func_table->pfn_create_descriptor_set   = vk_create_descriptor_set;
    func_table->pfn_uniform_set_buffer      = vk_uniform_set_buffer;
    func_table->pfn_uniform_set_buffer_data = vk_uniform_set_buffer_data;
    func_table->pfn_uniform_set_texture     = vk_uniform_set_texture;
    func_table->pfn_uniform_set_sampler     = vk_uniform_set_sampler;
    func_table->pfn_destroy_descriptor_set  = vk_destroy_descriptor_set;

    // COMMAND BUFFER
    func_table->pfn_create_cmd              = vk_create_cmd;
    func_table->pfn_destroy_cmd             = vk_destroy_cmd;

    func_table->pfn_cmd_begin               = vk_cmd_begin;
    func_table->pfn_cmd_begin_pass          = vk_cmd_begin_pass;
    func_table->pfn_cmd_end_pass            = vk_cmd_end_pass;

    func_table->pfn_cmd_scissor               = vk_cmd_scissor;
    func_table->pfn_cmd_viewport              = vk_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline         = vk_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set   = vk_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_buffer_ib        = vk_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_buffer_vb        = vk_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw                  = vk_cmd_draw;
    func_table->pfn_cmd_draw_indexed          = vk_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect = vk_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute      = vk_cmd_dispatch_compute;

    func_table->pfn_cmd_push_marker         = vk_cmd_push_marker;
    func_table->pfn_cmd_pop_marker          = vk_cmd_pop_marker;

    func_table->pfn_cmd_buffer_barrier      = vk_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier     = vk_cmd_texture_barrier;

    func_table->pfn_cmd_end                 = vk_cmd_end;
    func_table->pfn_submit_cmd              = vk_submit_cmd;
}
#else
void gfx_init_vulkan(gfx_api_pfn* func_table) { 
    memset(func_table, 0, sizeof(gfx_api_pfn)); 
}
#endif


#ifdef WEBGPU_AVAILABLE
#include "gfx_webgpu.h"

void gfx_init_webgpu(gfx_api_pfn* func_table)
{
    // CONTEXT
    func_table->pfn_init                    = wgpu_init;


    // BUFFER
    func_table->pfn_create_buffer           = wgpu_create_buffer;
    func_table->pfn_update_buffer_data      = wgpu_update_buffer_data;
    func_table->pfn_destroy_buffer          = wgpu_destroy_buffer;

    // SHADER
    func_table->pfn_create_shader           = wgpu_create_shader;
    func_table->pfn_uniform_location        = wgpu_uniform_location;
    func_table->pfn_destroy_shader          = wgpu_destroy_shader;

    // SAMPLER
    func_table->pfn_create_sampler          = wgpu_create_sampler;
    func_table->pfn_destroy_sampler         = wgpu_destroy_sampler;

    // TEXTURE
    func_table->pfn_create_texture          = wgpu_create_texture;
    func_table->pfn_update_texture_data     = wgpu_update_texture_data;
    func_table->pfn_update_bindless_texture = wgpu_update_bindless_texture;
    func_table->pfn_texture_generate_mipmap = wgpu_texture_generate_mipmap;
    func_table->pfn_blit_image              = wgpu_blit_image;
    func_table->pfn_texture_get_data        = wgpu_texture_get_data;
    func_table->pfn_destroy_texture         = wgpu_destroy_texture;

    // PIPELINE
    func_table->pfn_create_pipeline         = wgpu_create_pipeline;
    func_table->pfn_create_compute_pipeline  = wgpu_create_compute_pipeline;
    func_table->pfn_create_mesh_pipeline     = wgpu_create_mesh_pipeline;
    func_table->pfn_create_raytrace_pipeline = wgpu_create_raytrace_pipeline;
    func_table->pfn_destroy_pipeline         = wgpu_destroy_pipeline;
    func_table->pfn_destroy_compute_pipeline = wgpu_destroy_compute_pipeline;
    func_table->pfn_destroy_mesh_pipeline    = wgpu_destroy_mesh_pipeline;
    func_table->pfn_destroy_raytrace_pipeline= wgpu_destroy_raytrace_pipeline;

    // RENDER TARGET
    func_table->pfn_create_render_target    = wgpu_create_render_target;
    func_table->pfn_destroy_render_target   = wgpu_destroy_render_target;

    // DESCRIPTOR SET
    func_table->pfn_create_descriptor_set   = wgpu_create_descriptor_set;
    func_table->pfn_uniform_set_buffer      = wgpu_uniform_set_buffer;
    func_table->pfn_uniform_set_buffer_data = wgpu_uniform_update_buffer_data;
    func_table->pfn_uniform_set_texture     = wgpu_uniform_set_texture;
    func_table->pfn_uniform_set_sampler     = wgpu_uniform_set_sampler;
    func_table->pfn_destroy_descriptor_set  = wgpu_destroy_descriptor_set;

    // COMMAND BUFFER
    func_table->pfn_create_cmd              = wgpu_create_cmd;
    func_table->pfn_destroy_cmd             = wgpu_destroy_cmd;

    func_table->pfn_cmd_begin               = wgpu_cmd_begin;
    func_table->pfn_cmd_begin_pass          = wgpu_cmd_begin_pass;
    func_table->pfn_cmd_end_pass            = wgpu_cmd_end_pass;

    func_table->pfn_cmd_scissor               = wgpu_cmd_scissor;
    func_table->pfn_cmd_viewport              = wgpu_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline         = wgpu_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set   = wgpu_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_buffer_ib        = wgpu_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_buffer_vb        = wgpu_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw                  = wgpu_cmd_draw;
    func_table->pfn_cmd_draw_indexed          = wgpu_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect = wgpu_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute      = wgpu_cmd_dispatch_compute;

    func_table->pfn_cmd_push_marker         = wgpu_cmd_push_marker;
    func_table->pfn_cmd_pop_marker          = wgpu_cmd_pop_marker;

    func_table->pfn_cmd_buffer_barrier      = wgpu_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier     = wgpu_cmd_texture_barrier;

    func_table->pfn_cmd_end                 = wgpu_cmd_end;
    func_table->pfn_submit_cmd              = wgpu_submit_cmd;
}
#else 
void gfx_init_webgpu(gfx_api_pfn* func_table) {
    memset(func_table, 0, sizeof(gfx_api_pfn));
}
#endif