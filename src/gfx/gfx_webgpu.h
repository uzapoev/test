#ifndef __gfx_webgpu_h__
#define __gfx_webgpu_h__


///https://developer.chrome.com/docs/web-platform/webgpu/build-app?hl=ru

#include "gfx.h"

#ifdef __EMSCRIPTEN__
    #define WEBGPU_AVAILABLE
    #include <emscripten/html5.h>  // js call
    #include <emscripten/html5_webgpu.h>
#elif __has_include(<webgpu/webgpu.h>)
    #define WEBGPU_AVAILABLE
    #include <webgpu/webgpu.h>
#endif

#ifdef WEBGPU_AVAILABLE

typedef struct wgpu_context_t {
    gfx_context_t               handle;

    WGPUInstance                instance;
    WGPUAdapter                 adapter;
    WGPUDevice                  device;
    WGPUQueue                   queue;
    WGPUSupportedLimits         limits;
    WGPUAdapterProperties       properties;

    WGPUTextureFormat           surface_format; // 

    gfx_callback                dbglog;
    

    gfx_handle_pool_t *         sampler_pool;
    gfx_handle_pool_t *         texture_pool;
    gfx_handle_pool_t *         buffer_pool;
    gfx_handle_pool_t *         shader_pool; 
    gfx_handle_pool_t *         pipeline_pool;

    gfx_buffer_t *              staging_buffer = nullptr;

    gfx_texture_t *             default_texture = nullptr;
    gfx_sampler_t *             default_sampler = nullptr;
} wgpu_context_t;


// shader
typedef struct wgpu_shader_t {
    gfx_shader_t                    handle;

    wgpu_context_t*                 context;

    uint16_t                        hash; // stage flags
    uint32_t                        flags; // stage flags
    uint32_t                        count;

    uint32_t                        binding_count;
    WGPUBindGroupLayoutEntry        bindings[16];

    WGPUFragmentState               fragment;
    WGPUVertexState                 vertex;
    WGPUProgrammableStageDescriptor compute;

    WGPUBindGroupLayout             layout;
    WGPUPipelineLayout              pipeline_layout;

    uint32_t                        uniform_count;
    gfx_uniform_t                   uniforms[16];


    struct wgpu_descriptor_set_pool_t*     current_pool;
    gfx_handle_pool_t *             descriptor_set_pool;

  //  gfx_buffer_t *                  buffer;
} wgpu_shader_t;


// buffer
typedef struct wgpu_buffer_t {
    gfx_buffer_t                handle;
    WGPUBuffer                  buffer;
    WGPUBufferUsage             usage;
    uint32_t                    size;
    uint32_t                    flags;
    void*                       data_ptr;
} wgpu_buffer_t;


// sampler
typedef struct wgpu_sampler_t {
   gfx_sampler_t                handle;
   WGPUSampler                  sampler;
}wgpu_sampler_t;


// texture
typedef struct wgpu_texture_t {
    gfx_texture_t               handle;
    WGPUTexture                 texture;
    WGPUTextureView             texture_view;
} wgpu_texture_t;


// pipeline
typedef struct wgpu_pipeline_t {
    gfx_pipeline_t              handle;
    WGPURenderPipeline          pipeline;
} wgpu_pipeline_t;


// render target
typedef struct wgpu_render_target_t {
    gfx_render_target_t         handle;
    WGPUTextureView             view;           // color attachments
    wgpu_texture_t *            color_atachment;
    wgpu_texture_t *            depth_stencil_atachment;
} wgpu_render_target_t;


#pragma pack(push, 1)
// descriptor set
typedef struct wgpu_descriptor_set_t {
    gfx_descriptor_set_t            handle;

    wgpu_shader_t *                 shader;
    int8_t                          isfree;
    WGPUBindGroup                   bind_group;

    uint32_t                        binding_count;
    WGPUBindGroupEntry              bindings[8];

    struct wgpu_descriptor_set_pool_t *    owner_pool;
    uint32_t                        index_in_pool;
    uint8_t*                        ubo_data_ptr;
    uint32_t                        dynamic_offset;
} wgpu_descriptor_set_t;
#pragma pack(pop)

typedef struct wgpu_descriptor_set_pool_t {
    gfx_buffer_t*               ubo_buffer;
    uint32_t                    capacity;
    uint32_t                    count;
    uint32_t                    free_set_count;
    uint32_t                    next_free;

    uint8_t                     dirty;

    wgpu_descriptor_set_t *     descriptor_sets;
    uint32_t                    ubo_buffer_data_size;
    uint8_t*                    ubo_buffer_data_ptr;
} wgpu_descriptor_set_pool_t;

// cmd
typedef struct wgpu_command_buffer_t {
    gfx_command_buffer_t        handle;
    wgpu_context_t *            ctx;

    WGPUCommandEncoder          encoder;
    WGPURenderPassEncoder       pass;
    WGPUCommandBuffer           commands;

    WGPUComputePassEncoder      compute_pass;
} wgpu_command_buffer_t;

// swapchain
typedef struct wgpu_swapchain_t {
    gfx_swapchain_t             handle;

    intptr_t                    window_handle;

    wgpu_render_target_t*       target;

    WGPUTextureFormat           format;
    WGPUSurface                 surface;
    WGPUSwapChain               swapchain;

    WGPUTexture                 backbuffer;
    WGPUTextureView             backbuffer_view;

    WGPUSurfaceConfiguration    config;
} wgpu_swapchain_t;


gfx_api void     wgpu_init(gfx_settings_t * settings, gfx_context_t** ctx);
gfx_api void     wgpu_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t ** swapchain);

gfx_api int32_t  wgpu_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, gfx_render_target_t** target);
gfx_api void     wgpu_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx);

gfx_api void     wgpu_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
gfx_api void     wgpu_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
gfx_api void     wgpu_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
gfx_api void     wgpu_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
gfx_api void     wgpu_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void     wgpu_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline);
gfx_api void     wgpu_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_mesh_t** pipeline);
gfx_api void     wgpu_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
gfx_api void     wgpu_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** target);
gfx_api void     wgpu_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, gfx_descriptor_set_t** descriptor);
gfx_api void     wgpu_create_cmd(gfx_context_t* ctx, gfx_command_buffer_t** cmd);

gfx_api void     wgpu_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer);
gfx_api void     wgpu_destroy_shader(gfx_context_t* ctx, gfx_shader_t* buffer);
gfx_api void     wgpu_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler);
gfx_api void     wgpu_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void     wgpu_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline);
gfx_api void     wgpu_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);
gfx_api void     wgpu_destroy_mesh_pipeline(gfx_context_t* ctx, gfx_pipeline_mesh_t* pipeline);
gfx_api void     wgpu_destroy_raytrace_pipeline(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);
gfx_api void     wgpu_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* _target);
gfx_api void     wgpu_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);
gfx_api void     wgpu_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd);

gfx_api void     wgpu_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);
gfx_api void     wgpu_update_texture_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset);
gfx_api void     wgpu_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd);
gfx_api void     wgpu_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void     wgpu_blit_image(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst);
gfx_api void     wgpu_update_bindless_texture(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx);

gfx_api uint64_t wgpu_uniform_location(gfx_shader_t* shader, const char* name);
gfx_api void     wgpu_uniform_update_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t offset);
gfx_api void     wgpu_uniform_set_buffer(gfx_descriptor_set_t* set,  uint64_t handle,  gfx_buffer_t* data, uint32_t offset);
gfx_api void     wgpu_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
gfx_api void     wgpu_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);


gfx_api void     wgpu_cmd_begin(gfx_command_buffer_t* cmd);
gfx_api void     wgpu_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_render_target_t* target);
gfx_api void     wgpu_cmd_end_pass(gfx_command_buffer_t* cmd);
                 
gfx_api void     wgpu_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void     wgpu_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void     wgpu_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
gfx_api void     wgpu_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);
gfx_api void     wgpu_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void     wgpu_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void     wgpu_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
gfx_api void     wgpu_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t index_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset);
gfx_api void     wgpu_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t * buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
gfx_api void     wgpu_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

gfx_api void     wgpu_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker);
gfx_api void     wgpu_cmd_pop_marker(gfx_command_buffer_t* cmd);

gfx_api void     wgpu_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
gfx_api void     wgpu_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);
                 
gfx_api void     wgpu_cmd_end(gfx_command_buffer_t* cmd);
gfx_api void     wgpu_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options);

#endif

#endif //__gfx_webgpu_h__