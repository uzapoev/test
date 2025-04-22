/*
* gfx_device_info_t {
*   gpu_device_type     type;       // discrete/embbed
*   char                name[64]    // 
* } gfx_device_info_t;
* gfx_device_info_t infos [8]  = 0;
* int count = gfx_enumerate_devices(&infos);
* 
* gfx_config_t cfg      = {0};
*   cfg.handle          = hwnd;
*   cfg.options         = GFX_DEBUG | GFX_VALIDATE;
*   cfg.backend         = gfx_backend_auto;
*   cfg.gpu_type        = gfx_discrete_gpu;
* gfx_context_t * ctx = gfx_create(cfg);
**/

#ifndef __gfx_h__
#define __gfx_h__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h> // uintXX_t 
#include <stdlib.h> // uintXX_t 
#include <stdio.h> // uintXX_t 


#if defined(__cplusplus)
#define gfx_api extern "C"
#else 
#define gfx_api 
#endif

#ifndef _countof
    #define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif


#undef GFX_PLATFORM_WIN         // vulkan, dx12, webgpu
#undef GFX_PLATFORM_APPLE       // metal
#undef GFX_PLATFORM_ANDROID     // vulkan
#undef GFX_PLATFORM_WEB         // webgpu

#ifdef _WIN32
    #define     GFX_PLATFORM_WIN
    #define     VULKAN_AVAILABLE
#elif defined(__APPLE__)
    #define     GFX_PLATFORM_APPLE
#elif defined(__ANDROID__)
    #define     GFX_PLATFORM__ANDROID
    #define     VULKAN_AVAILABLE
#elif defined(EMSCRIPTEN)
    #define     GFX_PLATFORM_WEB
#else
    #error gfx unsupported platform
#endif


#if GFX_ENABLE_VERBOSE
    #define GFX_VERBOSE(exp)            { exp; }
    #define GFX_VERBOSE_IF(cond, exp)   { if(cond) exp; }
#else
    #define GFX_VERBOSE(exp)            {}
    #define GFX_VERBOSE_IF(cond, exp)   {}
#endif


typedef enum gfx_options {
    gfx_options_debug       = 1 << 0,
    gfx_options_verbose     = 1 << 1,
    gfx_options_callstack   = 1 << 2,
} gfx_options;


typedef enum gfx_gpu_type {
    gfx_gpu_discrete,
    gfx_gpu_integrated
} gfx_gpu_type;


typedef enum gfx_msg {
    gfx_msg_info,
    gfx_msg_warning,
    gfx_msg_error,
} gfx_msg;


typedef enum gfx_backend {
    gfx_backend_auto,               //metal for apple, webgpu - html, vulkan - win/android, dx12 - win
    gfx_backend_vulkan,
    gfx_backend_d3d12,
    gfx_backend_metal,
    gfx_backend_webgpu,
} gfx_backend;


typedef enum gfx_buffer_usage {
    gfx_buffer_usage_staging,       // cpu mapped
    gfx_buffer_usage_index,         // gpu
    gfx_buffer_usage_vertex,        // gpu 
    gfx_buffer_usage_uniform,       // cpu mapped
    gfx_buffer_usage_storage,
    gfx_buffer_usage_indirect,
} gfx_buffer_usage;


typedef enum gfx_access_type {
    gfx_access_read,
    gfx_access_write,
    gfx_access_rw
} gfx_access_type;


typedef enum gfx_texture_type {
    gfx_texture2d,
    gfx_texture2d_cube,
    gfx_texture2d_array,
    gfx_texture3d,
} gfx_texture_type;


typedef enum gfx_uniform_type {
    gfx_uniform_undefined,
    gfx_uniform_ubo,
    gfx_uniform_storage,        // (RW)StructuredBuffer, (RW)ByteAddressBuffer
    gfx_uniform_storage_image,  // RWTexture2D
    gfx_uniform_sampler,
    gfx_uniform_texture2d,
    gfx_uniform_texture2d_cube,
    gfx_uniform_texture2d_array,
    gfx_uniform_texture3d,
    gfx_uniform_ubo_field, // vec4, mat4, color4
} gfx_uniform_type;


typedef enum gfx_pixel_format {
    gfx_pixel_format_unknown,
    gfx_pixel_format_a8,                //! 8-bit textures used as masks
    gfx_pixel_format_rgba4444,          //! 16-bit textures: RGBA4444
    gfx_pixel_format_rgb5a1,            //! 16-bit textures: RGB5A1
    gfx_pixel_format_rgb565,            //! 16-bit texture without Alpha channel 
    gfx_pixel_format_rgba8,             //! 32-bit texture: RGBA8888

    gfx_pixel_format_etc1,              //!  RGB888 
    gfx_pixel_format_etc2_rgb8a1,       //!  RGB8 A1 
    gfx_pixel_format_etc2_rgba8,        //!  RGBA 8 

    gfx_pixel_format_pvrtc_rgb_2bpp,
    gfx_pixel_format_pvrtc_rgba_2bpp,
    gfx_pixel_format_pvrtc_rgb_4bpp,
    gfx_pixel_format_pvrtc_rgba_4bpp,

    gfx_pixel_format_bc1,               //! dxt1 8  
    gfx_pixel_format_bc2,               //! dxt3 16
    gfx_pixel_format_bc3,               //! dxt5 16
 //   gfx_pixel_format_bc4,               //! ATI1N
 //   gfx_pixel_format_bc5,               //! ATI2N
    gfx_pixel_format_bc6,               // 
    gfx_pixel_format_bc7,               //

    gfx_pixel_format_astc4x4,           //
    gfx_pixel_format_astc5x5,           //
    gfx_pixel_format_astc6x6,           // 
    gfx_pixel_format_astc8x8,           //
    gfx_pixel_format_astc10x10,         //
    gfx_pixel_format_astc12x12,         //

    gfx_pixel_format_r16f,              //!
    gfx_pixel_format_rg16f,             //!
    gfx_pixel_format_rgba16f,           //!

    gfx_pixel_format_r32f,              //!
    gfx_pixel_format_rg32f,             //!
    gfx_pixel_format_rgba32f,           //!

    gfx_pixel_format_d24x8,             //! depth buffer
    gfx_pixel_format_d24s8,             //! depth buffer
} gfx_pixel_format;


typedef enum gfx_vertex_format {
    gfx_vertex_format_float1,       // float
    gfx_vertex_format_float2,       // vec2f
    gfx_vertex_format_float4,       // vec4f

    gfx_vertex_format_int2,         // 
    gfx_vertex_format_int4,         // 
    gfx_vertex_format_uint2,        // 
    gfx_vertex_format_uint4,        // 

    gfx_vertex_format_half2,        // Two 16 bit floating value
    gfx_vertex_format_half4,        // Four 16 bit floating value

    gfx_vertex_format_short2,       // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    gfx_vertex_format_short4,       // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
    gfx_vertex_format_ushort2,      // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    gfx_vertex_format_ushort4,      // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)

    gfx_vertex_format_byte4,        // Each of 4 bytes is normalized by dividing to 255.0
} gfx_vertex_format;


typedef enum gfx_vertex_rate {
    gfx_vertex_rate_vertex,
    gfx_vertex_rate_instance
} gfx_vertex_rate;


typedef enum gfx_index_format {
    gfx_index_format_16,
    gfx_index_format_32
} gfx_index_format;


typedef enum gfx_sample_count {
    gfx_sample_1x   = 1,
    gfx_sample_2x   = 2,
    gfx_sample_4x   = 4,
    gfx_sample_8x   = 8,
    gfx_sample_16x  = 16,
} gfx_sample_count;


typedef enum gfx_topology {
    gfx_topology_points,
    gfx_topology_lines,
    gfx_topology_lines_strip,
    gfx_topology_triangles,
    gfx_topology_triangles_strip,
} gfx_topology;


typedef enum gfx_cull {
    gfx_cull_none,
    gfx_cull_back,
    gfx_cull_front,
} gfx_cull;


typedef enum gfx_face {
    gfx_face_cw,
    gfx_face_ccw,
} gfx_face;


typedef enum gfx_filter {
    gfx_filter_point,
    gfx_filter_linear,
} gfx_filter;


typedef enum gfx_address_mode {
    gfx_address_mode_repeat,
    gfx_address_mode_mirror_repeat,
    gfx_address_mode_clamp_to_edge
} gfx_address_mode;


typedef enum gfx_cmp {
    gfx_cmp_never,
    gfx_cmp_less,
    gfx_cmp_equal,
    gfx_cmp_lequal,
    gfx_cmp_greater,
    gfx_cmp_not_equal,
    gfx_cmp_gequal,
    gfx_cmp_always,
} gfx_cmp;


typedef enum gfx_stencil_op {
    gfx_stencil_op_zero,        // D3D10_STENCIL_OP_ZERO         D3DSTENCILOP_ZERO       GL_ZERO
    gfx_stencil_op_keep,        // D3D10_STENCIL_OP_KEEP         D3DSTENCILOP_KEEP       GL_KEEP
    gfx_stencil_op_replace,     // D3D10_STENCIL_OP_REPLACE      D3DSTENCILOP_REPLACE    GL_REPLACE
    gfx_stencil_op_incr,        // D3D10_STENCIL_OP_INCR_SAT     D3DSTENCILOP_INCRSAT    GL_INCR
    gfx_stencil_op_incr_wrap,   // D3D10_STENCIL_OP_INCR         D3DSTENCILOP_INCR       GL_INCR_WRAP
    gfx_stencil_op_decr,        // D3D10_STENCIL_OP_DECR_SAT     D3DSTENCILOP_DECRSAT    GL_DECR
    gfx_stencil_op_decr_wrap,   // D3D10_STENCIL_OP_DECR         D3DSTENCILOP_DECR       GL_DECR_WRAP
    gfx_stencil_op_invert,      // D3D10_STENCIL_OP_INVERT       D3DSTENCILOP_INVERT     GL_INVERT
} gf_stencil_op;


typedef enum gfx_blend_mode {
    gfx_blend_mode_zero,
    gfx_blend_mode_one,
    gfx_blend_mode_src_color,
    gfx_blend_mode_inv_src_color,
    gfx_blend_mode_src_alpha,
    gfx_blend_mode_inv_src_alpha,
    gfx_blend_mode_dst_alpha,
    gfx_blend_mode_inv_dest_alpha,
    gfx_blend_mode_dst_color,
    gfx_blend_mode_inv_dst_color,
} gfx_blend_mode;


typedef enum gfx_blend_op {
    gfx_blend_op_add,
    gfx_blend_op_min,
    gfx_blend_op_max,
    gfx_blend_op_subtract,
    gfx_blend_op_rev_substract,
} gfx_blend_op;


typedef enum gfx_pipeline_flags {
    gfx_colormask_r = 1 << 0,
    gfx_colormask_g = 1 << 1,
    gfx_colormask_b = 1 << 2,
    gfx_colormask_a = 1 << 3,
    gfx_colormask_all = gfx_colormask_r | gfx_colormask_g | gfx_colormask_b | gfx_colormask_a,
    gfx_depth_test = 1 << 4,
    gfx_depth_write = 1 << 5,
    gfx_blend = 1 << 6,
    gfx_stencil = 1 << 7,
} gfx_pipeline_flags;


typedef enum gfx_semantic {
    gfx_position,
    gfx_color,
    gfx_normal,
    gfx_uv0,
} gfx_semantic;


typedef enum gfx_shader_stage {
    gfx_shader_vertex,
    gfx_shader_hull,
    gfx_shader_domain,
    gfx_shader_geometry,
    gfx_shader_fragment,

    gfx_shader_compute,

    gfx_shader_rt_raygen,      // = 0x0100,
    gfx_shader_rt_intersect,   // = 0x1000,
    gfx_shader_rt_any_hit,     // = 0x0200,
    gfx_shader_rt_closest_hit, // = 0x0400,
    gfx_shader_rt_miss,        // = 0x0800,
    gfx_shader_rt_callable,    // = 0x2000,

    gfx_shader_amplify,         // amplify + mesh + fragment
    gfx_shader_mesh,            // mesh + fragment 

    gfx_shader_count,
} gfx_shader_stage;


typedef enum gfx_submit_options {
    gfx_submit_nowait,
    gfx_submit_wait_for_fence,
    gfx_submit_wait_for_queue_idle,
    gfx_submit_wait_for_device_idle,
    gfx_submit_wait_for_image_ready,
} gfx_submit_options;


struct gfx_handle_pool_t;

typedef struct { uint64_t idx; } gfx_context_t;
typedef struct { uint64_t idx; } gfx_swapchain_t;
typedef struct { uint64_t idx; } gfx_buffer_t;
typedef struct { uint64_t idx; } gfx_texture_t;
typedef struct { uint64_t idx; } gfx_sampler_t;
typedef struct { uint64_t idx; } gfx_shader_t;  
typedef struct { uint64_t idx; } gfx_descriptor_set_t;
typedef struct { uint64_t idx; } gfx_pipeline_t;
typedef struct { uint64_t idx; } gfx_pipeline_compute_t;
typedef struct { uint64_t idx; } gfx_render_target_t;
typedef struct { uint64_t idx; } gfx_command_buffer_t;


// type - info/warning/error
typedef void (*gfx_callback)(gfx_msg type, const char* msg, ...);

typedef struct gfx_allocator_t {
    void*   (*gfx_alloc) (size_t size, void* userdata)  = nullptr;
    void    (*gfx_free)  (void* ptr, void* userdata)    = nullptr;
    void    *user_data                                  = nullptr;
} gfx_allocator_t;

typedef struct gfx_settings_t {
    uint32_t                options                 = 0;

    gfx_backend             backend                 = gfx_backend_auto;
    intptr_t                handle                  = 0;

    struct {
        uint32_t            staging_buffer_size     = 16 * 1024 * 1024;
        uint32_t            buffer_pool_capacity    = 1  * 1024;
        uint32_t            shaders_pool_capacity   = 1  * 1024;
        uint32_t            textures_pool_capacity  = 2  * 1024;
    } limits;

    gfx_allocator_t *       allocator;
    gfx_callback            dbglog;
} gfx_settings_t;


typedef struct gfx_device_info_t {
    gfx_gpu_type            type;               // discrete/embbed
    char                    name[64];           // 
    char                    gpu_vendor[64];
} gfx_device_info_t;


typedef struct gfx_caps_t {
    gfx_device_info_t       device_info;    

    uint8_t                 support_pvr;        //
    uint8_t                 support_etc;        //
    uint8_t                 support_astc;       //
    uint8_t                 support_bc;         // block compression dxt

    uint8_t                 support_compute;    //
    uint8_t                 support_raytrace;   //

    uint8_t                 support_mesh_shader;
    uint8_t                 support_mesh_amplification_shader;
} gfx_caps_t;

 
typedef struct gfx_sampler_desc_t {
    gfx_filter              minmag;
    gfx_filter              mipmap;
    gfx_address_mode        mode;
    uint32_t                anisotropy;
} gfx_sampler_desc_t;


typedef struct gfx_texture_desc_t {
    const char *            label;
    uint32_t                width;
    uint32_t                height;
    uint32_t                depth;      // depth | array slice | cube side(+/-x, +/-y, +/-z) 
    void *                  data;       // cubemap and cube - data[slice, mipmaps], [slice, mipmaps]
    uint32_t                mip_levels;  
    uint32_t                storage;    // qreater 0 - use as storage
    
    gfx_texture_type        type;
    gfx_pixel_format        format;
} gfx_texture_desc_t;


typedef struct gfx_buffer_desc_t {
    const char*             label;
    gfx_buffer_usage        usage;
    bool                    mapped;
    uint32_t                size;
    void *                  data;
} gfx_buffer_desc_t;


typedef struct gfx_uniform_t {
    char                    name[32];
    gfx_uniform_type        type;
    uint16_t                stage_mask;     // fragment|vertex
    uint16_t                binding;
    uint16_t                group;          //
    
    struct {
        uint16_t            size;           //
        uint16_t            field_count;    //
        struct {
            char            name[32];
            uint16_t        stride;     // for ubo field
            uint16_t        offset;     // for ubo field
            uint16_t        type;       // for ubo field
        } fields[16];
    } buffer;
    
    struct  {
        gfx_access_type     access;
    } storage;
    
    struct {
        gfx_texture_type    dimension;
        gfx_access_type     access;       // read/write/read_wite
    } texture;
    
    struct {
        
    } sampler;
} gfx_uniform_t;


typedef struct gfx_shader_stage_data{
    gfx_shader_stage        stage;
    void*                   data;
    uint32_t                size;

    const char *            entry;
} gfx_shader_stage_data;


typedef struct gfx_shader_desc_t {
    const char*             label;
    uint32_t                descriptor_pool_capacity;

    uint32_t                stages_count;
    gfx_shader_stage_data*  stages;

    uint32_t                uniform_count;
    gfx_uniform_t*          uniforms;
} gfx_shader_desc_t;


typedef struct gfx_vertex_attribute {
//  gfx_semantic            semantic;
    uint32_t                location;       // attribute location
    uint32_t                binding;        // buffer binding
    gfx_vertex_format       format;         // 
    uint32_t                offset;
} gfx_vertex_attribute;


typedef struct gfx_vertex_slot_t {
    uint32_t                binding;
    uint32_t                stride;
    gfx_vertex_rate         rate;
} gfx_vertex_slot_t;


typedef struct gfx_vertex_assembly {
    gfx_topology            topology = gfx_topology_points;

    uint32_t                slot_count;
    gfx_vertex_slot_t *     slots;

    uint32_t                attributes_count;
    gfx_vertex_attribute *  attributes;

} gfx_vertex_assembly;


typedef struct gfx_render_states_desc_t {
    gfx_topology            topology    = gfx_topology_triangles;
    gfx_cull                culling     = gfx_cull_none;
    gfx_face                face        = gfx_face_ccw;
    uint32_t                states      = gfx_colormask_all | gfx_depth_test | gfx_depth_write;

    struct {
        gfx_pixel_format*   color;
    } attacments;

    struct {
        bool                enable      = false;
        gfx_blend_mode      color_src   = gfx_blend_mode_one;   // srcColor
        gfx_blend_mode      color_dst   = gfx_blend_mode_one;   // dstColor
        gfx_blend_op        color_op    = gfx_blend_op_add;     // opColor

        gfx_blend_mode      alpha_dst   = gfx_blend_mode_one;   // srcAlpha
        gfx_blend_mode      alpha_src   = gfx_blend_mode_one;   // dstAlpha
        gfx_blend_op        alpha_op    = gfx_blend_op_add;     // opAlpha
    } blend;
    
    struct {
        bool                enable      = true;
        bool                write       = true;
        gfx_cmp             mode        = gfx_cmp_lequal;
    } depth;

    struct {
        gfx_cmp             func        = gfx_cmp_always;
        uint8_t             ref         = 0;
        uint8_t             pass        = 0xFF;

        gfx_stencil_op      opPass      = gfx_stencil_op_keep;
        gfx_stencil_op      opFail      = gfx_stencil_op_keep;
        gfx_stencil_op      opZFail     = gfx_stencil_op_keep;
    } stencil;

    struct {
        bool                red         = true;
        bool                green       = true;
        bool                blue        = true;
        bool                alpha       = true; 
    } color_mask;
} gfx_render_states_desc_t;
 

typedef struct gfx_render_target_desc_t {
    uint16_t                    width;
    uint16_t                    height;
    uint32_t                    color_attachement_count;
    gfx_pixel_format *          color_attachement_formats;
    gfx_pixel_format            depth_attachement_format;

    gfx_sample_count            sample_count;
} gfx_render_target_desc_t;


typedef struct gfx_pipeline_desc_t {
    gfx_shader_t *              shader;
    gfx_render_states_desc_t    render_states;
//  gfx_render_pass_desc_t      render_pass;
    gfx_vertex_assembly         assembly;
} gfx_pipeline_desc_t;


typedef struct gfx_compute_pipeline_desc_t {
    gfx_shader_t* shader;
} gfx_compute_pipeline_desc_t;


typedef struct gfx_render_pass_desc_t {
    uint32_t                    clear_color;
    uint32_t                    clear_depth;
} gfx_render_pass_desc_t;


gfx_api int32_t                 gfx_enumerate_devices(gfx_device_info_t * infos, int32_t capacity);
gfx_api gfx_backend             gfx_detect_bakend(gfx_backend * backends, uint32_t size);
gfx_api void                    gfx_get_caps(gfx_context_t* ctx, gfx_caps_t* caps);

gfx_api void                    gfx_init(gfx_settings_t* settings, gfx_context_t** ctx);
gfx_api void                    gfx_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t** swapchain);

gfx_api int32_t                 gfx_acquire_img(gfx_context_t* ctx, gfx_swapchain_t * swapchain, gfx_render_target_t** target);
gfx_api void                    gfx_present_img(gfx_context_t* ctx, gfx_swapchain_t * swapchain, uint32_t idx);

gfx_api gfx_buffer_t *          gfx_create_buffer2(gfx_context_t* ctx, gfx_buffer_desc_t* desc);
gfx_api gfx_shader_t *          gfx_create_shader2(gfx_context_t* ctx, gfx_shader_desc_t* desc);
gfx_api gfx_sampler_t *         gfx_create_sampler2(gfx_context_t* ctx, gfx_sampler_desc_t* desc);
gfx_api gfx_texture_t *         gfx_create_texture2(gfx_context_t* ctx, gfx_texture_desc_t* desc);
gfx_api gfx_pipeline_t *        gfx_create_pipeline2(gfx_context_t* ctx, gfx_pipeline_desc_t* desc);
gfx_api gfx_pipeline_compute_t* gfx_create_compute_pipeline2(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc);
gfx_api gfx_render_target_t *   gfx_create_render_target2(gfx_context_t* ctx, gfx_render_target_desc_t* desc);
gfx_api gfx_descriptor_set_t *  gfx_create_descriptor_set2(gfx_context_t* ctx, gfx_shader_t* shader);
gfx_api gfx_command_buffer_t *  gfx_create_cmd2(gfx_context_t* ctx);

gfx_api void                    gfx_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);

gfx_api void                    gfx_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer);
gfx_api void                    gfx_destroy_shader(gfx_context_t* ctx, gfx_shader_t* buffer);
gfx_api void                    gfx_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler);
gfx_api void                    gfx_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void                    gfx_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline);
gfx_api void                    gfx_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* _target);
gfx_api void                    gfx_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);
gfx_api void                    gfx_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd);

// handle - shader|type|id
gfx_api uint32_t                gfx_shader_get_uniforms     (gfx_shader_t* shader, gfx_uniform_t * uniforms);
gfx_api uint64_t                gfx_uniform_location        (gfx_shader_t* shader, const char* name);
gfx_api void                    gfx_uniform_set_buffer_data (gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
gfx_api void                    gfx_uniform_set_buffer      (gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset);
gfx_api void                    gfx_uniform_set_texture     (gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t * texture);
gfx_api void                    gfx_uniform_set_sampler     (gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t * sampler);

gfx_api void                    gfx_cmd_begin_marker        (gfx_command_buffer_t* cmd, const char * marker);
gfx_api void                    gfx_cmd_insert_marker       (gfx_command_buffer_t* cmd, const char * marker);
gfx_api void                    gfx_cmd_end_marker          (gfx_command_buffer_t* cmd);

gfx_api void                    gfx_cmd_begin               (gfx_command_buffer_t* cmd);
gfx_api void                    gfx_cmd_begin_pass          (gfx_command_buffer_t* cmd, gfx_render_target_t* target);
gfx_api void                    gfx_cmd_end_pass            (gfx_command_buffer_t* cmd);

gfx_api void                    gfx_cmd_scissor             (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void                    gfx_cmd_viewport            (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void                    gfx_cmd_bind_pipeline       (gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
gfx_api void                    gfx_cmd_bind_descriptor_set (gfx_command_buffer_t* cmd, gfx_descriptor_set_t* descriptor);
gfx_api void                    gfx_cmd_bind_index_buffer   (gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void                    gfx_cmd_bind_vertex_buffer  (gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void                    gfx_cmd_draw                (gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
gfx_api void                    gfx_cmd_draw_indexed        (gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count);
gfx_api void                    gfx_cmd_dispatch_compute    (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

gfx_api void                    gfx_cmd_end(gfx_command_buffer_t* cmd);
gfx_api void                    gfx_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options);


// https://www.khronos.org/blog/understanding-vulkan-synchronization
// https://github.com/khronosgroup/vulkan-docs/wiki/synchronization-examples

typedef enum gfx_barrier {
    gfx_barrier_compute_src,
    gfx_barrier_compute_dst,
    gfx_barrier_vertex_src,
    gfx_barrier_vertex_dst,
    gfx_barrier_fragment_src,
    gfx_barrier_fragment_dst
} gfx_barrier;

typedef struct gfx_barrier_desc_t {
    gfx_buffer_t  * buffer;
    gfx_texture_t * texture;
} gfx_barrier_desc_t;


gfx_api void gfx_cmd_barrier(gfx_command_buffer_t* cmd, gfx_barrier barrier, gfx_barrier_desc_t * desc);


gfx_api void gfx_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, gfx_barrier src, gfx_barrier dst); // 
gfx_api void gfx_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t* texture, gfx_barrier src, gfx_barrier dst);

// WIP: occlusion query, timestamp, mipmap, raytracing
// 
// typedef struct gfx_rt_acceleration_struct {
//     struct {
//         gfx_buffer_t *      vertex_buffer = nullptr;
//         uint64_t            vertex_offset = 0;
//         uint32_t            vertex_count = 0;
//         uint64_t            vertex_stride = 0;
//         gfx_vertex_format   vertex_format = gfx_vertex_format_byte4;
//         gfx_buffer_t *      index_buffer = nullptr;
//         uint64_t            index_offset = 0;
//         uint32_t            index_count = 0;
//         gfx_index_format    index_format = gfx_index_format_16;
//         gfx_buffer_t *      transform_buffer = nullptr;
//         uint64_t            transform_offset = 0;
//     } triangles;
// 
//     struct {
//         gfx_buffer_t *      buffer = nullptr;
//         uint64_t            offset = 0;
//         uint32_t            count = 0;
//         uint64_t            stride = 0;
//     } aabbs;
// 
//     struct {
//         gfx_buffer_t *      buffer = nullptr;
//         uint64_t            offset = 0;
//         uint32_t            count = 0;
//         bool                array_of_pointers = false;
//     } instances;
// } gfx_acceleration_struct;


// gfx_api void     gfx_cmd_begin_timer(gfx_context_t* ctx, gfx_command_buffer_t* cmd);
// gfx_api void     gfx_cmd_write_querry_stamp(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_pipeline_stage);
// 
// gfx_api void     gfx_update_image_data(gfx_context_t* ctx, gfx_texture_t *texture, void* data, uint32_t size, uint32_t offset);
// 
// gfx_api void     gfx_create_pipeline_cash(gfx_context_t* ctx, struct gfx_pipeline_cash_t* cmd);
// gfx_api void     gfx_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd);
// gfx_api void     gfx_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture);
// gfx_api void     gfx_blit_image(gfx_context_t* ctx, /*gfx_blit_info_t*/gfx_texture_t* src, gfx_texture_t* dst);
// 

extern const char*  gfx_to_string(gfx_buffer_usage usage);
extern const char*  gfx_to_string(gfx_shader_stage stage);
extern const char*  gfx_to_string(gfx_texture_type type);
extern const char*  gfx_to_string(gfx_pixel_format format);


// utility
uint32_t            gfx_utils_thread_id();
uint32_t            gfx_utils_stack_trace(uint32_t skip, uintptr_t* frames, uint64_t count);
void                gfx_utils_stack_trace_names(uintptr_t* frames, uint64_t count, const char** names);
uint16_t            gfx_utils_hash_16(const char * data, uint32_t size);
gfx_api uint32_t    gfx_utils_image_layer_size(uint32_t width, uint32_t height, uint32_t depth, gfx_pixel_format format);
gfx_api uint32_t    gfx_utils_image_row_pitch(gfx_pixel_format fmt, uint32_t width);
gfx_api uint32_t    gfx_utils_align_up(uint32_t n, uint32_t alignment);

// pool 
gfx_api void        gfx_pool_create(size_t stride, size_t capacity, gfx_handle_pool_t** pool, gfx_allocator_t * allocator);
gfx_api void        gfx_pool_destroy(gfx_handle_pool_t* pool);
gfx_api uint64_t    gfx_pool_alloc(gfx_handle_pool_t* pool);
gfx_api void        gfx_pool_free(gfx_handle_pool_t* pool, uint64_t handle);
gfx_api void*       gfx_pool_map(gfx_handle_pool_t* pool, uint64_t handle);

gfx_api void*       gfx_pool_get_data(gfx_handle_pool_t* pool);
gfx_api size_t      gfx_pool_get_stride(gfx_handle_pool_t* pool);
gfx_api size_t      gfx_pool_get_size(gfx_handle_pool_t* pool);
gfx_api size_t      gfx_pool_get_capacity(gfx_handle_pool_t* pool);
gfx_api size_t      gfx_pool_has_free(gfx_handle_pool_t* pool);


/*
* gfx_handle_pool_t* pool = gfx_pool_create(sizeof(vk_buffer_t), capacity);
* vk_buffer_t * buffer = gfx_pool_allocate(pool);
* 
*/

#endif // __gfx_webgpu_h__