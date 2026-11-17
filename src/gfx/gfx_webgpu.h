#ifndef __gfx_webgpu_h__
#define __gfx_webgpu_h__

// WebGPU backend on the standard webgpu.h API (WGPUStringView, CallbackInfo + futures, wgpuSurfaceGetCurrentTexture):
//   desktop: Dawn (external/dawn: include/webgpu.h, bin/win/x64/webgpu_dawn.lib/.dll)
//   web:     emscripten >= 4.0.10 with --use-port=emdawnwebgpu (same header, <webgpu/webgpu.h>)

#include "gfx.h"

#if __has_include(<webgpu/webgpu.h>)
    #define WEBGPU_AVAILABLE
    #include <webgpu/webgpu.h>
#elif __has_include(<webgpu.h>)
    #define WEBGPU_AVAILABLE
    #include <webgpu.h>
#endif

#ifdef WEBGPU_AVAILABLE

#define WGPU_MAX_BINDINGS   16      // per shader / descriptor set (one bind group, group 0)

typedef struct wgpu_context_t {
    gfx_context_t               handle;

    WGPUInstance                instance;
    WGPUAdapter                 adapter;
    WGPUDevice                  device;
    WGPUQueue                   queue;
    WGPULimits                  limits;
    WGPUBackendType             backend_type;
    WGPUAdapterType             adapter_type;
    bool                        float32_filterable;

    // what pipelines are built for: the format / sample count of the last created surface
    // (same model as the Vulkan backend's default render pass)
    WGPUTextureFormat           surface_format;     // view format of the back buffer (may be the sRGB view of the surface format)
    WGPUTextureFormat           depth_format;
    uint32_t                    sample_count;

    gfx_callback                dbglog;

    gfx_handle_pool_t *         buffer_pool;
    gfx_handle_pool_t *         surface_pool;

    gfx_texture_t *             default_texture;
    gfx_sampler_t *             default_sampler;

    struct wgpu_frame_t *       frame;              // the one frame in flight on the CPU side (WebGPU queues the GPU work)
    struct wgpu_descriptor_set_pool_t* pools;       // every descriptor set pool: dirty UBO data is uploaded at frame end
} wgpu_context_t;


// shader
typedef struct wgpu_shader_t {
    gfx_shader_t                    handle;
    wgpu_context_t*                 context;

    uint32_t                        hash;
    uint32_t                        flags;                  // 1 << gfx_shader_stage of the present stages

    WGPUShaderModule                modules[3];             // vertex / fragment / compute (may be the same module)
    char                            entry_names[3][64];

    uint32_t                        binding_count;
    WGPUBindGroupLayoutEntry        bindings[WGPU_MAX_BINDINGS];    // bindings[i] <-> uniforms[i]
    uint32_t                        uniform_count;
    gfx_uniform_t                   uniforms[WGPU_MAX_BINDINGS];

    WGPUBindGroupLayout             layout;
    WGPUPipelineLayout              pipeline_layout;

    struct wgpu_descriptor_set_pool_t* current_pool;
} wgpu_shader_t;


// buffer
typedef struct wgpu_buffer_t {
    gfx_buffer_t                handle;
    WGPUBuffer                  buffer;
    uint32_t                    size;
} wgpu_buffer_t;


// sampler
typedef struct wgpu_sampler_t {
    gfx_sampler_t               handle;
    WGPUSampler                 sampler;
} wgpu_sampler_t;


// texture
typedef struct wgpu_texture_t {
    gfx_texture_t               handle;
    WGPUTexture                 texture;
    WGPUTextureView             texture_view;
    WGPUTextureFormat           format;
    uint32_t                    width, height;
    uint32_t                    sample_count;
    bool                        external;           // back buffer views of a frame: not released by wgpu_destroy_texture
} wgpu_texture_t;


// pipeline
typedef struct wgpu_pipeline_t {
    gfx_pipeline_t              handle;
    WGPURenderPipeline          pipeline;
} wgpu_pipeline_t;

typedef struct wgpu_compute_pipeline_t {
    gfx_pipeline_compute_t      handle;
    WGPUComputePipeline         pipeline;
} wgpu_compute_pipeline_t;


// render target: what a pass renders into (the frame's back buffer, or user textures)
typedef struct wgpu_render_target_t {
    gfx_render_target_t         handle;
    WGPUTextureView             color;              // multisampled color when resolve != nullptr
    WGPUTextureView             resolve;            // back buffer view when rendering with MSAA
    WGPUTextureView             depth;
    uint32_t                    width, height;
} wgpu_render_target_t;


#pragma pack(push, 1)
// descriptor set
typedef struct wgpu_descriptor_set_t {
    gfx_descriptor_set_t            handle;

    wgpu_shader_t *                 shader;
    int8_t                          isfree;
    WGPUBindGroup                   bind_group;
    bool                            bind_group_dirty;      // an entry changed: the bind group is rebuilt on the next bind

    WGPUBindGroupEntry              bindings[WGPU_MAX_BINDINGS];

    struct wgpu_descriptor_set_pool_t *    owner_pool;
    uint32_t                        index_in_pool;
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

    wgpu_shader_t*              shader;
    struct wgpu_descriptor_set_pool_t* next;        // older, full pools of the same shader
    struct wgpu_descriptor_set_pool_t* ctx_next;    // wgpu_context_t::pools list
} wgpu_descriptor_set_pool_t;


// cmd
typedef struct wgpu_command_buffer_t {
    gfx_command_buffer_t        handle;
    wgpu_context_t *            ctx;

    WGPUCommandEncoder          encoder;
    WGPURenderPassEncoder       pass;
    WGPUComputePassEncoder      compute_pass;

    wgpu_render_target_t*       default_target;     // the frame's back buffer, used when a pass names no target
} wgpu_command_buffer_t;


// surface
typedef struct wgpu_surface_t {
    gfx_surface_t               handle;
    wgpu_context_t*             ctx;

    intptr_t                    window_handle;
    WGPUSurface                 surface;
    WGPUSurfaceConfiguration    config;
    WGPUTextureFormat           view_format;        // sRGB view of config.format when the surface was asked for an sRGB format
    bool                        vsync;
    bool                        needs_configure;
    uint32_t                    sample_count;

    WGPUPresentMode             present_modes[8];
    uint32_t                    present_mode_count;

    WGPUTexture                 msaa_color;         // sample_count > 1
    WGPUTextureView             msaa_color_view;
    WGPUTexture                 depth;
    WGPUTextureView             depth_view;
} wgpu_surface_t;


// frame
typedef struct wgpu_frame_t {
    gfx_frame_t                 public_frame;       // first: gfx_frame_t* <-> wgpu_frame_t*
    wgpu_command_buffer_t       cmd;
    wgpu_render_target_t        target;
    wgpu_texture_t              color_view;         // gfx_frame_t::color_attachment / depth_attachment
    wgpu_texture_t              depth_view;

    wgpu_surface_t*             surface;
    WGPUTexture                 back_buffer;
    WGPUTextureView             back_buffer_view;
} wgpu_frame_t;


gfx_api void     wgpu_init(gfx_settings_t * settings, gfx_context_t** ctx);
gfx_api void     wgpu_destroy(gfx_context_t* ctx);
gfx_api void     wgpu_get_caps(gfx_context_t* ctx, gfx_caps_t* caps);

gfx_api void     wgpu_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface);
gfx_api void     wgpu_surface_destroy(gfx_context_t* ctx, gfx_surface_t* surface);
gfx_api gfx_result wgpu_surface_set_vsync(gfx_context_t* ctx, gfx_surface_t* surface, bool vsync);
gfx_api void     wgpu_frame_begin(gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame);
gfx_api void     wgpu_frame_end(gfx_frame_t* frame);

gfx_api void     wgpu_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
gfx_api void     wgpu_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
gfx_api uint32_t wgpu_shader_get_descriptor_set_count(gfx_shader_t* shader);
gfx_api uint32_t wgpu_shader_get_uniforms(gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms);
gfx_api void     wgpu_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
gfx_api void     wgpu_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
gfx_api void     wgpu_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void     wgpu_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline);
gfx_api void     wgpu_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void     wgpu_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
gfx_api void     wgpu_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t se_idx, gfx_descriptor_set_t** descriptor);

gfx_api void     wgpu_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer);
gfx_api void     wgpu_destroy_shader(gfx_context_t* ctx, gfx_shader_t* buffer);
gfx_api void     wgpu_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler);
gfx_api void     wgpu_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void     wgpu_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline);
gfx_api void     wgpu_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);
gfx_api void     wgpu_destroy_raytrace_pipeline(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);
gfx_api void     wgpu_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);

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

gfx_api void     wgpu_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* pass_info);
gfx_api void     wgpu_cmd_end_pass(gfx_command_buffer_t* cmd);
gfx_api void     wgpu_cmd_begin_compute_pass(gfx_command_buffer_t* cmd);
gfx_api void     wgpu_cmd_end_compute_pass(gfx_command_buffer_t* cmd);
gfx_api void     wgpu_cmd_bind_compute_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_compute_t* pipeline);

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

#endif

#endif //__gfx_webgpu_h__
