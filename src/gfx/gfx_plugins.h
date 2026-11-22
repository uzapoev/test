// gfx_plugins.h - optional vendor SDK integrations on top of gfx (GPU capture, temporal upscaling)
//
// Role:     thin glue between gfx handles and vendor SDKs. One common upscaler API
//           (DLSS / FSR / XeSS / MetalFX behind a type enum) + RenderDoc capture control.
// Contents: [1] RenderDoc capture   [2] upscaler (common API)
//           [3] TODO: what gfx.h must expose for this file to be implementable
// Thread:   load/create/destroy - main (render) thread; gfx_cmd_upscale - the thread recording the cmd.
// Not here: jitter in projection + motion vectors pass (renderer); frame generation
//           (DLSS-G / FSR3 FG hook the swapchain - separate topic); PIX (later, same pattern as RenderDoc).
// Uses:     gfx.h (+ native getters from [3]), vendor SDK headers only in gfx_plugins.cpp,
//           each behind its own CMake flag (GFX_WITH_RENDERDOC / _DLSS / _FSR / _XESS / _METALFX).
//           SDK compiled out -> gfx_upscaler_supported() returns false.
// Tests:    none yet.
//
// STATUS: declarations only, no implementation (gfx_plugins.cpp does not exist yet).

#ifndef __gfx_plugins_h__
#define __gfx_plugins_h__

#include "gfx.h"


// ============================================================================
// [1] RenderDoc
// ============================================================================
// renderdoc.dll must be loaded BEFORE gfx_init, otherwise device creation is not hooked.
// Hence load() takes no context.

gfx_api bool    gfx_renderdoc_load();                               /**< Call before gfx_init. false if renderdoc.dll not found */
gfx_api bool    gfx_renderdoc_is_attached();                        /**< true if RenderDoc API is loaded and active */
gfx_api void    gfx_renderdoc_trigger_capture();                    /**< Capture the next presented frame (most common use) */
gfx_api void    gfx_renderdoc_capture_begin(gfx_context_t* ctx);    /**< Manual range capture, device taken from gfx_get_native_device */
gfx_api void    gfx_renderdoc_capture_end(gfx_context_t* ctx);
gfx_api void    gfx_renderdoc_unload();


// ============================================================================
// [2] Upscaler (temporal, common API)
// ============================================================================
//
//  backend support:          vulkan  dx12  metal  webgpu
//    dlss    (NVIDIA, NGX)     Y       Y     -      -
//    fsr     (AMD, ffx-api)    Y       Y     -      -
//    xess    (Intel)           Y       Y     -      -
//    metalfx (Apple)           -       -     Y      -
//
//  Usage:
//    gfx_upscaler_desc_t d = {};
//        d.type = gfx_upscaler_dlss; d.quality = gfx_upscaler_quality_balanced;
//        d.output_width = 2560; d.output_height = 1440;
//    gfx_upscaler_t* up = gfx_upscaler_create(ctx, &d);
//    gfx_upscaler_get_render_size(up, &rw, &rh);           // allocate color/depth/mv at rw x rh
//
//    per frame:
//      gfx_upscaler_get_jitter(up, frame_index, &jx, &jy); // renderer puts jitter into projection
//      ... render scene at rw x rh, write motion vectors ...
//      barrier color/depth/mv -> gfx_barrier_compute_read, output -> gfx_barrier_compute_write
//      gfx_cmd_upscale(cmd, up, &inputs);                  // outside any render / compute pass
//
//  Rules:
//    - quality / output size change -> destroy + create (DLSS requires recreating the feature anyway).
//    - destroy only after frames in flight have finished using it.
//    - textures must already be in the states above; the plugin does not track or transition them.
//    - output needs gfx_texture_usage_storage; motion vectors are gfx_pixel_format_rg16f.
//    - MetalFX: textures must be created with matching usage (output: render_target | storage),
//      depth/mv mandatory, no reactive/exposure mask; quality maps to scale factor only.

typedef struct { uint64_t idx; } gfx_upscaler_t;

typedef enum gfx_upscaler_type {
    gfx_upscaler_none,
    gfx_upscaler_dlss,                      /**< NVIDIA DLSS via NGX directly (not Streamline - too invasive) */
    gfx_upscaler_fsr,                       /**< AMD FSR 3.1 via FidelityFX ffx-api */
    gfx_upscaler_xess,                      /**< Intel XeSS */
    gfx_upscaler_metalfx,                   /**< Apple MetalFX temporal scaler (MTLFXTemporalScaler) */
} gfx_upscaler_type;

typedef enum gfx_upscaler_quality {
    gfx_upscaler_quality_native_aa,         /**< 1.0x  - AA only (DLAA / FSR native AA) */
    gfx_upscaler_quality_quality,           /**< ~1.5x */
    gfx_upscaler_quality_balanced,          /**< ~1.7x */
    gfx_upscaler_quality_performance,       /**< 2.0x  */
    gfx_upscaler_quality_ultra_performance, /**< 3.0x  */
} gfx_upscaler_quality;

typedef enum gfx_upscaler_flags {
    gfx_upscaler_flag_hdr               = 1 << 0,   /**< color input is linear HDR */
    gfx_upscaler_flag_depth_inverted    = 1 << 1,   /**< reversed-Z */
    gfx_upscaler_flag_mv_jittered       = 1 << 2,   /**< motion vectors include jitter */
    gfx_upscaler_flag_auto_exposure     = 1 << 3,   /**< SDK computes exposure, exposure texture ignored */
} gfx_upscaler_flags;

typedef struct gfx_upscaler_desc_t {
    gfx_upscaler_type       type            = gfx_upscaler_none;
    gfx_upscaler_quality    quality         = gfx_upscaler_quality_balanced;
    uint32_t                output_width    = 0;
    uint32_t                output_height   = 0;
    uint32_t                flags           = 0;    /**< gfx_upscaler_flags */
} gfx_upscaler_desc_t;

typedef struct gfx_upscale_inputs_t {
    gfx_texture_t*  color           = nullptr;  /**< render size */
    gfx_texture_t*  depth           = nullptr;  /**< render size */
    gfx_texture_t*  motion_vectors  = nullptr;  /**< render size, rg16f, pixels */
    gfx_texture_t*  exposure        = nullptr;  /**< optional 1x1 (ignored with auto_exposure / metalfx) */
    gfx_texture_t*  reactive        = nullptr;  /**< optional, FSR only */
    gfx_texture_t*  output          = nullptr;  /**< output size, storage */

    float           jitter_x        = 0.0f;     /**< pixels, same values as put into projection */
    float           jitter_y        = 0.0f;
    float           mv_scale_x      = 1.0f;
    float           mv_scale_y      = 1.0f;
    float           sharpness       = 0.0f;     /**< 0..1, ignored where unsupported */
    float           delta_time_ms   = 0.0f;     /**< FSR */
    float           camera_near     = 0.0f;     /**< FSR */
    float           camera_far      = 0.0f;     /**< FSR */
    float           camera_fov_y    = 0.0f;     /**< FSR, radians */
    bool            reset           = false;    /**< drop history (camera cut / teleport) */
} gfx_upscale_inputs_t;

gfx_api bool            gfx_upscaler_supported(gfx_context_t* ctx, gfx_upscaler_type type);     /**< backend + SDK compiled in + driver/hw ok */
gfx_api gfx_upscaler_t* gfx_upscaler_create(gfx_context_t* ctx, gfx_upscaler_desc_t* desc);   /**< nullptr if unsupported */
gfx_api void            gfx_upscaler_get_render_size(gfx_upscaler_t* up, uint32_t* width, uint32_t* height);
gfx_api uint32_t        gfx_upscaler_get_jitter(gfx_upscaler_t* up, uint32_t frame_index, float* x, float* y);  /**< returns phase count */
gfx_api void            gfx_cmd_upscale(gfx_command_buffer_t* cmd, gfx_upscaler_t* up, gfx_upscale_inputs_t* inputs);
gfx_api void            gfx_upscaler_destroy(gfx_context_t* ctx, gfx_upscaler_t* up);


// ============================================================================
// [3] TODO: required additions to gfx.h (not done yet)
// ============================================================================
//
// 1) Native object access (3 getters, a few lines per backend):
//
//    typedef struct gfx_native_device_t {
//        gfx_backend backend;
//        void*       instance;          // VkInstance       | IDXGIFactory*        | -
//        void*       physical_device;   // VkPhysicalDevice | IDXGIAdapter*        | -
//        void*       device;            // VkDevice         | ID3D12Device*        | id<MTLDevice>
//        void*       queue;             // VkQueue          | ID3D12CommandQueue*  | id<MTLCommandQueue>
//        uint32_t    queue_family;      // vulkan only
//        void*       get_proc_addr;     // vkGetDeviceProcAddr (ffx-api, XeSS need it)
//    } gfx_native_device_t;
//
//    typedef struct gfx_native_texture_t {
//        void*       resource;          // VkImage     | ID3D12Resource* | id<MTLTexture>
//        void*       view;              // VkImageView | -               | -
//        uint32_t    format;            // VkFormat    | DXGI_FORMAT     | MTLPixelFormat
//    } gfx_native_texture_t;
//
//    gfx_api void  gfx_get_native_device (gfx_context_t* ctx, gfx_native_device_t* out);
//    gfx_api void  gfx_texture_get_native(gfx_texture_t* texture, gfx_native_texture_t* out);
//    gfx_api void* gfx_cmd_get_native    (gfx_command_buffer_t* cmd);
//                  // VkCommandBuffer | ID3D12GraphicsCommandList* | id<MTLCommandBuffer>
//                  // metal: no open encoder at call time (MetalFX encodes into the command buffer itself)
//
// 2) Extra Vulkan extensions at init (NGX / XeSS require them before device creation;
//    currently the list in gfx_vulkan.cpp is hardcoded). Add to gfx_settings_t:
//
//    const char**  vk_instance_extensions        = nullptr;
//    uint32_t      vk_instance_extension_count   = 0;
//    const char**  vk_device_extensions          = nullptr;
//    uint32_t      vk_device_extension_count     = 0;
//
// 3) State cache reset after foreign commands (plugin binds its own pipelines / descriptors):
//
//    gfx_api void gfx_cmd_invalidate_state(gfx_command_buffer_t* cmd);
//
// 4) Contract only (no API change): gfx does not track texture layouts, caller transitions
//    inputs to gfx_barrier_compute_read and output to gfx_barrier_compute_write before
//    gfx_cmd_upscale. If MetalFX / DX12 need extra usage bits, revisit gfx_texture_usage_flags.

#endif // __gfx_plugins_h__
fx_plugins_h__