/******************************************************************************
* 
*   gfx_config_t cfg = {0};
*     cfg.handle                        = hwnd;
*     cfg.options                       = GFX_DEBUG | GFX_VALIDATE;
*     cfg.backend                       = gfx_backend_auto;
*     cfg.gpu_type                      = gfx_discrete_gpu;
*   gfx_context_t * ctx = gfx_create(cfg);
* 
*   gfx_surface_desc_t surface_desc = { 0 };
*       surface_desc.label              = "main_view";
*       surface_desc.window_handle      = handle;
*       surface_desc.vsync              = true;
*       surface_desc.preferred_format   = gfx_pixel_format_rgba8;
*       surface_desc.sample_count       = gfx_sample_1x;
*   auto surface = gfx_surface_create(ctx, &surface_desc);
* 
*   gfx_pass_info_t pass = {0}
*       pass.clear_color = rgba(255,255,255,255);
*       pass.clear_depth = 0;
* 
*   auto frame = gfx_begin_frame(ctx, &surface);
*       gfx_cmd_begin_pass(frame->cmd, &pass);
* 
*       // draw here
* 
*       gfx_cmd_end_pass(frame->cmd);
*   gfx_end_frame(frame);
*******************************************************************************/

#ifndef __gfx_h__
#define __gfx_h__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h> // uintXX_t 
#include <stdlib.h> // uintXX_t 


#if defined(__cplusplus)
#define gfx_api extern "C"
#else 
#define gfx_api 
#endif

#ifndef _countof
    #define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif


/**
 * @brief API function execution results.
 */
typedef enum gfx_result {
    gfx_ok,                                 /**< Operation completed successfully */
    gfx_error                               /**< Generic execution error */
} gfx_result;


/**
 * @brief Bitmask flags for context initialization and runtime behavior.
 */
typedef enum gfx_options {
    gfx_options_debug       = 1 << 0,   /**< Enable validation layers and graphics debugging features */
    gfx_options_verbose     = 1 << 1,   /**< Enable detailed verbose log output */
    gfx_options_callstack   = 1 << 2,   /**< Dump callstack on critical errors or validation failures */
} gfx_options;


typedef enum gfx_gpu_type {
    gfx_gpu_discrete,                   /**< High-performance standalone GPU with dedicated VRAM */
    gfx_gpu_integrated                  /**< Power-efficient GPU integrated into the CPU/SoC sharing system RAM */
} gfx_gpu_type;


typedef enum gfx_msg {
    gfx_msg_info,                       /**< Informational status message */
    gfx_msg_warning,                    /**< Non-critical warning, execution can continue */
    gfx_msg_error,                      /**< Critical runtime error */
} gfx_msg;


typedef enum gfx_backend {
    gfx_backend_auto,                   /**< Auto-select: Metal on Apple, WebGPU on HTML5, Vulkan on Win/Android, DX12 on Win */
    gfx_backend_vulkan,                 /**< Vulkan API backend */
    gfx_backend_d3d12,                  /**< Direct3D 12 backend */
    gfx_backend_metal,                  /**< Apple Metal backend */
    gfx_backend_webgpu,                 /**< WebGPU backend */
} gfx_backend;


typedef enum gfx_shader_format_flags {
    gfx_shader_format_unknown   = 0,
    gfx_shader_format_spirv     = 1 << 0,   /**< SPIR-V binary formats (Standard for Vulkan) */
    gfx_shader_format_dxil      = 1 << 1,   /**< DXIL binary formats (Standard for DirectX 12 DXR) */
    gfx_shader_format_msl       = 1 << 2,   /**< Metal Shading Language precompiled binaries (Apple Metal) */
    gfx_shader_format_wgsl      = 1 << 3,   /**< WebGPU Shading Language text/binary tokens formats */

    gfx_shader_format_glsl      = 1 << 4,   /**< */
    gfx_shader_format_hlsl      = 1 << 5,   /**< */
} gfx_shader_format_flags;


typedef enum gfx_buffer_usage {
    gfx_buffer_usage_staging,           /**< Host-visible memory used for CPU-to-GPU memory transfers */
    gfx_buffer_usage_index,             /**< Device-local memory holding geometry indices */
    gfx_buffer_usage_vertex,            /**< Device-local memory holding vertex attributes */
    gfx_buffer_usage_uniform,           /**< Constrained size buffer for constant/uniform shader parameters */
    gfx_buffer_usage_storage,           /**< Unbounded size buffer for read/write compute and shader structured storage */
    gfx_buffer_usage_indirect,          /**< Buffer storing draw arguments for GPU indirect command execution */
} gfx_buffer_usage;


typedef enum gfx_memory_hint {
    gfx_memory_auto,                    /**< Automatically determine optimal memory placement based on usage */
    gfx_memory_gpu_only,                /**< VRAM allocation, inaccessible by CPU, maximum performance */
    gfx_memory_cpu_to_gpu,              /**< Host-visible memory mapped for fast CPU writes to GPU */
    gfx_memory_gpu_to_cpu,              /**< Host-visible memory optimized for reading data back from the GPU to CPU */
} gfx_memory_hint;


typedef enum gfx_access_type {
    gfx_access_read,                    /**< Read-only access */
    gfx_access_write,                   /**< Write-only access */
    gfx_access_rw                       /**< Read and Write access */
} gfx_access_type;


typedef enum gfx_texture_type {
    gfx_texture2d,                      /**< Standard 2D texture */
    gfx_texture2d_cube,                 /**< Cubemap texture containing 6 faces */
    gfx_texture2d_array,                /**< Array of independent 2D texture layers */
    gfx_texture3d,                      /**< Volumetric 3D texture */
} gfx_texture_type;


typedef enum gfx_texture_usage_flags {
    gfx_texture_usage_none          = 0,        /**< Defaults to gfx_texture_usage_shader_read for standard assets */
    gfx_texture_usage_shader_read   = 1 << 0,   /**< Texture can be sampled inside shaders (e.g., texture2D in Slang) */
    gfx_texture_usage_render_target = 1 << 1,   /**< Texture can be bound as a Color or Depth attachment in dynamic passes */
    gfx_texture_usage_storage       = 1 << 2,   /**< Texture can be used as a Read-Write Compute image object (e.g., RWTexture2D) */
    gfx_texture_usage_transfer_src  = 1 << 3,   /**< Texture can be used as a source for blit/copy operations */
    gfx_texture_usage_transfer_dst  = 1 << 4,   /**< Texture can be used as a destination for blit/copy operations */
} gfx_texture_usage_flags;

/**
 * @brief Layout binding roles for shader resource variables.
 */
typedef enum gfx_uniform_type {
    gfx_uniform_undefined,              /**< Uninitialized or invalid binding type */
    gfx_uniform_ubo,                    /**< Uniform Buffer Object block binding */
    gfx_uniform_storage_buffer,         /**< Structured or ByteAddress storage buffer binding (RW/ReadOnly) */
    gfx_uniform_storage_image,          /**< Read-Write texture image slot (e.g., RWTexture2D) */
    gfx_uniform_sampler,                /**< Texture state sampler (filtering, addressing) */
    gfx_uniform_texture2d,              /**< Standard 2D texture view binding */
    gfx_uniform_texture2d_cube,         /**< Cubemap resource view binding */
    gfx_uniform_texture2d_array,        /**< Texture array resource view binding */
    gfx_uniform_texture3d,              /**< 3D volumetric texture view binding */
    gfx_uniform_ubo_field,              /**< Individual field data type inside a UBO block (e.g., vec4, mat4) */
} gfx_uniform_type;


typedef enum gfx_pixel_format {
    gfx_pixel_format_unknown,           /**< Unspecified or invalid format */
    
    // Uncompressed 8/16-bit low-precision formats
    gfx_pixel_format_a8,                /**< 8-bit alpha-only mask texture */
    gfx_pixel_format_rgba4444,          /**< Packed 16-bit texture: 4 bits per channel */
    gfx_pixel_format_rgb5a1,            /**< Packed 16-bit texture: 5 bits RGB, 1 bit Alpha */
    gfx_pixel_format_rgb565,            /**< Packed 16-bit texture: 5 bits Red, 6 bits Green, 5 bits Blue */
    gfx_pixel_format_rgba8,             /**< Standard 32-bit texture: 8 bits per channel (RGBA) */

    // Mobile ETC compressed formats
    gfx_pixel_format_etc1,              /**< Ericsson Texture Compression: 4bpp RGB8 fallback */
    gfx_pixel_format_etc2_rgb8a1,       /**< ETC2 variant supporting RGB8 with 1-bit punch-through Alpha */
    gfx_pixel_format_etc2_rgba8,        /**< ETC2 variant supporting full 8-bit Alpha channel */

    // Desktop Block Compression (DXTC / BC) formats
    gfx_pixel_format_bc1,               /**< DXT1: 4 bpp compressed, opaque Albedo/Diffuse maps */
    gfx_pixel_format_bc3,               /**< DXT5: 8 bpp compressed, Albedo/Diffuse maps with full Alpha */
    gfx_pixel_format_bc4,               /**< BC4:  4 bpp compressed, single-channel grayscale mask/height maps */
    gfx_pixel_format_bc5,               /**< BC5:  8 bpp compressed, two-channel format optimal for Normal maps */
    gfx_pixel_format_bc6h,              /**< BC6H: 8 bpp compressed, High Dynamic Range (HDR) floating-point data */
    gfx_pixel_format_bc7,               /**< BC7:  8 bpp compressed, ultra-high quality RGBA texture maps */

    // ASTC scalable compressed formats
    gfx_pixel_format_astc4x4,           /**< Adaptive Scalable Texture Compression: 4x4 block size (8.00 bpp) */
    gfx_pixel_format_astc5x5,           /**< ASTC 5x5 block footprint allocation (5.12 bpp) */
    gfx_pixel_format_astc6x6,           /**< ASTC 6x6 block footprint allocation (3.56 bpp) */
    gfx_pixel_format_astc8x8,           /**< ASTC 8x8 block footprint allocation (2.00 bpp) */
    gfx_pixel_format_astc10x10,         /**< ASTC 10x10 block footprint allocation (1.28 bpp) */
    gfx_pixel_format_astc12x12,         /**< ASTC 12x12 block footprint allocation (0.89 bpp) */
    gfx_pixel_format_astc4x4_hdr,       /**< ASTC 4x4 block footprint configured for High Dynamic Range data */

    // High precision floating-point formats
    gfx_pixel_format_r16f,              /**< Half-precision 16-bit float Red channel */
    gfx_pixel_format_rg16f,             /**< Half-precision 16-bit float Red and Green channels */
    gfx_pixel_format_rgba16f,           /**< Half-precision 16-bit float full RGBA channels */
    gfx_pixel_format_r32f,              /**< Full-precision 32-bit float Red channel */
    gfx_pixel_format_rg32f,             /**< Full-precision 32-bit float Red and Green channels */
    gfx_pixel_format_rgba32f,           /**< Full-precision 32-bit float full RGBA channels */

    // Depth and Stencil target formats
    gfx_pixel_format_d24x8,             /**< 24-bit Depth target with 8 bits unused */
    gfx_pixel_format_d24s8,             /**< 24-bit Depth combined with 8-bit Stencil channel */
    gfx_pixel_format_d32,               /**< High-precision 32-bit pure floating-point Depth target */
} gfx_pixel_format;


typedef enum gfx_vertex_format {
    gfx_vertex_format_float1,           /**< Single 32-bit floating-point component scalar */
    gfx_vertex_format_float2,           /**< Two-dimensional 32-bit float vector (vec2f) */
    gfx_vertex_format_float4,           /**< Four-dimensional 32-bit float vector (vec4f) */
    
    gfx_vertex_format_int2,             /**< Two-dimensional 32-bit signed integer vector */
    gfx_vertex_format_int4,             /**< Four-dimensional 32-bit signed integer vector */
    gfx_vertex_format_uint2,            /**< Two-dimensional 32-bit unsigned integer vector */
    gfx_vertex_format_uint4,            /**< Four-dimensional 32-bit unsigned integer vector */
    
    gfx_vertex_format_half2,            /**< Two 16-bit half-precision floating-point components */
    gfx_vertex_format_half4,            /**< Four 16-bit half-precision floating-point components */
    
    gfx_vertex_format_short2,           /**< 2D signed short components normalized to [-1.0, 1.0] range */
    gfx_vertex_format_short4,           /**< 4D signed short components normalized to [-1.0, 1.0] range */
    gfx_vertex_format_ushort2,          /**< 2D unsigned short components normalized to [0.0, 1.0] range */
    gfx_vertex_format_ushort4,          /**< 4D unsigned short components normalized to [0.0, 1.0] range */
    
    gfx_vertex_format_byte4,            /**< Four 8-bit unsigned bytes normalized to [0.0, 1.0] (optimal for vertex colors) */
} gfx_vertex_format;


typedef enum gfx_vertex_rate {
    gfx_vertex_rate_vertex,             /**< Step data per vertex (standard pipeline behavior) */
    gfx_vertex_rate_instance            /**< Step data per instance (used in instanced rendering techniques) */
} gfx_vertex_rate;


typedef enum gfx_index_format {
    gfx_index_format_16,                /**< 16-bit unsigned short indices (max 65535 vertices) */
    gfx_index_format_32                 /**< 32-bit unsigned integer indices */
} gfx_index_format;


typedef enum gfx_sample_count {
    gfx_sample_1x   = 1,                /**< MSAA disabled, single sample per pixel */
    gfx_sample_2x   = 2,                /**< 2x multisampling */
    gfx_sample_4x   = 4,                /**< 4x multisampling (recommended quality sweet-spot) */
    gfx_sample_8x   = 8,                /**< 8x multisampling */
    gfx_sample_16x  = 16,               /**< 16x multisampling high-end quality level */
} gfx_sample_count;


typedef enum gfx_topology {
    gfx_topology_points,                /**< List of disconnected points */
    gfx_topology_lines,                 /**< List of independent line segments */
    gfx_topology_lines_strip,           /**< Strip of connected line segments sharing vertices */
    gfx_topology_triangles,             /**< List of independent triangles */
    gfx_topology_triangles_strip,       /**< Strip of connected triangles sharing vertices */
} gfx_topology;


typedef enum gfx_cull {
    gfx_cull_none,                      /**< Culling disabled, render both sides of geometry */
    gfx_cull_back,                      /**< Cull backward-facing geometry polygons */
    gfx_cull_front,                     /**< Cull forward-facing geometry polygons */
} gfx_cull;


typedef enum gfx_face {
    gfx_face_cw,                        /**< Clockwise vertex winding order is front-facing */
    gfx_face_ccw,                       /**< Counter-clockwise vertex winding order is front-facing */
} gfx_face;


typedef enum gfx_filter {
    gfx_filter_point,                   /**< Nearest-neighbor pixel matching, pixelated look */
    gfx_filter_linear,                  /**< Bilinear/Trilinear texture blending filtering */
} gfx_filter;


typedef enum gfx_address_mode {
    gfx_address_mode_repeat,            /**< Repeat texture tiles infinitely outside [0, 1] UV space */
    gfx_address_mode_mirror_repeat,     /**< Repeat texture tiles, mirroring coordinates at every edge junction */
    gfx_address_mode_clamp_to_edge      /**< Clamp texture coordinates to the edge pixel value */
} gfx_address_mode;


typedef enum gfx_cmp {
    gfx_cmp_never,                      /**< Comparison test always fails */
    gfx_cmp_less,                       /**< Passes if source value is less than destination value */
    gfx_cmp_equal,                      /**< Passes if values are mathematically equal */
    gfx_cmp_lequal,                     /**< Passes if source is less than or equal to destination value */
    gfx_cmp_greater,                    /**< Passes if source value is greater than destination value */
    gfx_cmp_not_equal,                  /**< Passes if source value does not equal destination value */
    gfx_cmp_gequal,                     /**< Passes if source is greater than or equal to destination value */
    gfx_cmp_always,                     /**< Comparison test always passes */
} gfx_cmp;


typedef enum gfx_stencil_op {
    gfx_stencil_op_zero,                /**< Set stencil buffer bit value to 0 */
    gfx_stencil_op_keep,                /**< Maintain the current stencil buffer value */
    gfx_stencil_op_replace,             /**< Overwrite value with the test reference integer */
    gfx_stencil_op_incr,                /**< Increment stencil value, clamping at maximum boundary */
    gfx_stencil_op_incr_wrap,           /**< Increment stencil value, wrapping back to 0 on overflow */
    gfx_stencil_op_decr,                /**< Decrement stencil value, clamping at 0 boundary */
    gfx_stencil_op_decr_wrap,           /**< Decrement stencil value, wrapping to max value on underflow */
    gfx_stencil_op_invert,              /**< Bitwise invert the current value in the stencil buffer */
} gfx_stencil_op;


typedef enum gfx_blend_mode {
    gfx_blend_mode_zero,                /**< Blend factor is 0 */
    gfx_blend_mode_one,                 /**< Blend factor is 1 */
    gfx_blend_mode_src_color,           /**< Factor based on source fragment color values */
    gfx_blend_mode_inv_src_color,       /**< Factor based on inverted source fragment color values (1 - src) */
    gfx_blend_mode_src_alpha,           /**< Factor based on source fragment alpha value */
    gfx_blend_mode_inv_src_alpha,       /**< Factor based on inverted source fragment alpha value (1 - alpha) */
    gfx_blend_mode_dst_alpha,           /**< Factor based on destination target buffer alpha value */
    gfx_blend_mode_inv_dest_alpha,      /**< Factor based on inverted destination target buffer alpha value (1 - alpha) */
    gfx_blend_mode_dst_color,           /**< Factor based on destination target buffer color values */
    gfx_blend_mode_inv_dst_color,       /**< Factor based on inverted destination target buffer color values (1 - dst) */
} gfx_blend_mode;


typedef enum gfx_blend_op {
    gfx_blend_op_add,                   /**< Add components together: Source + Destination */
    gfx_blend_op_min,                   /**< Choose the minimum component value: min(Source, Destination) */
    gfx_blend_op_max,                   /**< Choose the maximum component value: max(Source, Destination) */
    gfx_blend_op_subtract,              /**< Subtract destination from source: Source - Destination */
    gfx_blend_op_rev_subtract,          /**< Subtract source from destination: Destination - Source */
} gfx_blend_op;


typedef enum gfx_pipeline_flags {
    gfx_colormask_r     = 1 << 0,       /**< Enable Red color channel output writes */
    gfx_colormask_g     = 1 << 1,       /**< Enable Green color channel output writes */
    gfx_colormask_b     = 1 << 2,       /**< Enable Blue color channel output writes */
    gfx_colormask_a     = 1 << 3,       /**< Enable Alpha color channel output writes */
    gfx_colormask_rgba  = gfx_colormask_r | gfx_colormask_g | gfx_colormask_b | gfx_colormask_a, /**< Full RGBA color write permissions enabled */
    gfx_depth_test      = 1 << 4,       /**< Enable hardware depth testing */
    gfx_depth_write     = 1 << 5,       /**< Enable writing calculated fragment depth values into the depth buffer */
    gfx_blend           = 1 << 6,       /**< Enable color alpha blending equations for the render target */
    gfx_stencil         = 1 << 7,       /**< Enable stencil test pipeline operations */
} gfx_pipeline_flags;


typedef enum gfx_semantic {
    gfx_position,                       /**< Vertex positions data array (e.g., location/slot 0) */
    gfx_color,                          /**< Vertex primary diffuse/albedo color array data */
    gfx_normal,                         /**< Geometry surface shading normal vector arrays */
    gfx_tangent,                        /**< Surface tangent vector arrays for normal mapping calculations */
    gfx_bnormal,                        /**< Surface bitangent/binormal vector arrays */
    gfx_uv0,                            /**< Primary texture coordinate coordinate mapping channel */
    gfx_uv1,                            /**< Secondary texture coordinate coordinate mapping channel */
    gfx_uv2,                            /**< Tertiary texture coordinate coordinate mapping channel */
    gfx_uv3,                            /**< Quaternary texture coordinate coordinate mapping channel */
    gfx_weight,                         /**< Vertex skinning influence matrix blend weights data */
    gfx_index,                          /**< Vertex skinning joint/bone array blend indices data */
} gfx_semantic;


typedef enum gfx_shader_stage {
    // Standard Graphics Pipeline
    gfx_shader_vertex,                  /**< Vertex shader stage (transforms per-vertex data) */
    gfx_shader_fragment,                /**< Fragment/Pixel shader stage (calculates pixel color output) */

    // Compute Pipeline
    gfx_shader_compute,                 /**< Compute shader stage for general GPU data processing */

    // Hardware Ray Tracing Pipeline (VK_KHR_ray_tracing_pipeline / DX12 DXR)
    gfx_shader_rt_raygen,               /**< Ray Generation shader (entry point for executing ray traces) */
    gfx_shader_rt_intersect,            /**< Intersection shader (defines custom primitive intersections) */
    gfx_shader_rt_any_hit,              /**< Any-Hit shader (executed when a ray intersection filters transparent/shadow pixels) */
    gfx_shader_rt_closest_hit,          /**< Closest-Hit shader (executed on the nearest ray intersection point for shading) */
    gfx_shader_rt_miss,                 /**< Miss shader (executed when a ray fails to intersect any geometry) */
    gfx_shader_rt_callable,             /**< Callable shader (invoked from other shaders for dynamic branch execution) */

    // Modern Next-Gen Geometry Pipelines (VK_EXT_mesh_shader / DX12 Mesh Shading)
    gfx_shader_amplify,                 /**< Amplify/Task shader stage (performs coarse culling and launches mesh groups) */
    gfx_shader_mesh,                    /**< Mesh shader stage (generates vertex and primitive topologies directly on GPU) */

    gfx_shader_count,                   /**< Total number of shader stages available */
} gfx_shader_stage;


// todo: replace gfx_barrier_compute to gfx_barrier_compute_read + gfx_barrier_compute_write
// todo: replace gfx_barrier_graphics to gfx_barrier_graphics_read + gfx_barrier_graphics_write - do not use write on gpu with TBDRA
typedef enum gfx_barrier {
    gfx_barrier_indirect,               /**< Barrier for buffers driving indirect command execution arguments */
    gfx_barrier_compute,                /**< Barrier separating compute shader read/write hazards */
    gfx_barrier_graphics,               /**< General graphics barrier executed before vertex work or after fragment shading */
    gfx_barrier_render_target,          /**< Layout transition barrier optimized for attachment color buffers writes */
    gfx_barrier_depth_stencil,          /**< Layout transition barrier optimized for depth/stencil buffers writes */
    gfx_barrier_transfer,               /**< Barrier for explicit host/device memory transfer operations (copy/blit) */
    gfx_barrier_present,                /**< Layout transition barrier releasing ownership to the OS swapchain presentation system */
} gfx_barrier;


typedef enum gfx_submit_options {
    gfx_submit_nowait,                  /**< Asynchronously fire commands onto the queue without any host stalling */
    gfx_submit_wait_for_fence,          /**< Block host thread execution until the associated submission fence is signaled */
    gfx_submit_wait_for_queue_idle,     /**< Block host execution until the target execution queue finishes all active items */
    gfx_submit_wait_for_device_idle,    /**< Stall host execution completely until the entire GPU device settles into an idle state */
    gfx_submit_wait_for_image_ready,    /**< Hold execution until the swapchain engine acquires and releases ownership of the next image */
} gfx_submit_options;

typedef struct { uint64_t idx; } gfx_context_t;                 /**< Handle to the primary graphics subsystem instance */
typedef struct { uint64_t idx; } gfx_surface_t;                 /**< Handle managing the OS-specific display window association */

typedef struct { uint64_t idx; } gfx_swapchain_t;               /**< @deprecated Managed directly inside gfx_surface_t now */

typedef struct { uint64_t idx; } gfx_buffer_t;                  /**< Handle referencing allocated hardware data buffers */
typedef struct { uint64_t idx; } gfx_texture_t;                 /**< Handle referencing 2D, 3D, or Cubemap image texture memory */
typedef struct { uint64_t idx; } gfx_sampler_t;                 /**< Handle referencing filtering and texture state configuration blocks */
typedef struct { uint64_t idx; } gfx_shader_t;                  /**< Handle mapping to compiled multi-stage shader program binaries */
typedef struct { uint64_t idx; } gfx_descriptor_set_t;          /**< Handle linking an updated set of bound resource variables */
typedef struct { uint64_t idx; } gfx_pipeline_t;                /**< Handle representing an active fixed-function vertex+fragment state */
typedef struct { uint64_t idx; } gfx_pipeline_compute_t;        /**< Handle representing an active compute state machine layout */
typedef struct { uint64_t idx; } gfx_pipeline_raytrace_t;       /**< Handle representing a hardware-accelerated ray tracing state machine layout */
typedef struct { uint64_t idx; } gfx_render_target_t;           /**< Handle configuring multiple bound color and depth attachments targets */
typedef struct { uint64_t idx; } gfx_command_buffer_t;          /**< Handle capturing rendering execution tokens for execution queue submission */

typedef struct { uint64_t idx; } gfx_acceleration_structure_t;  /**< Handle to a Top or Bottom level acceleration structure */
typedef struct { uint64_t idx; } gfx_sbt_t;                     /**< Handle to a Shader Binding Table mapping ray tracing groups */


// ============================================================================
// --- Callbacks, Custom Allocators & Error States ---
// ============================================================================

/**
 * @brief Function pointer signature for receiving diagnostic engine messages.
 * @param type Severity classification of the triggered log event.
 * @param msg Format string reference mapping the detailed diagnostic data.
 */
typedef void (*gfx_callback)(gfx_msg type, const char* msg, ...);

/**
 * @brief Interface providing custom memory allocation hooks to bypass native system malloc/free.
 */
typedef struct gfx_allocator_t {
    void*   (*gfx_alloc) (size_t size, void* userdata)  = nullptr; /**< Function pointer allocating a discrete block of heap memory */
    void    (*gfx_free)  (void* ptr, void* userdata)    = nullptr; /**< Function pointer releasing a previously allocated memory address block */
    void*   user_data                                   = nullptr; /**< Custom runtime context state passed down into callbacks */
} gfx_allocator_t;

/**
 * @brief Comprehensive payload encapsulating API runtime failures.
 */
typedef struct gfx_error_t {
    const char*     message = nullptr;  /**< Explanatory string explaining the crash or validation root cause context */
    uint32_t        code    = 0;        /**< Unique numerical integer identifier linked to the error state classification */
} gfx_error_t;


// ============================================================================
// --- System Initialization & Hardware Capabilities ---
// ============================================================================

/**
 * @brief Primary configuration settings used to initialize the graphics engine context.
 */
typedef struct gfx_settings_t {
    uint32_t                options                 = 0;                /**< Bitmask configuration flags (see gfx_options) */

    gfx_backend             backend                 = gfx_backend_auto; /**< Preferred graphics API backend rendering system */
    gfx_gpu_type            prefer_gpu              = gfx_gpu_discrete; /**< Preferred hardware GPU architecture type */
    intptr_t                handle                  = 0;                /**< Optional OS-specific application/window instance handle */

    /**
     * @brief Fixed limits and pool capacities allocated for resource management.
     */
    struct {
        uint32_t            staging_buffer_size             = 8 * 1024 * 1024;  /**< Size of host-visible staging memory in bytes */
        uint32_t            uniform_buffer_size             = 8 * 1024 * 1024;  /**< Size of persistent uniform buffer ring in bytes */
        uint32_t            buffer_pool_capacity            = 1  * 1024;        /**< Maximum number of concurrent buffer allocations */
        uint32_t            shader_pool_capacity            = 1  * 1024;        /**< Maximum number of concurrent shader allocations */
        uint32_t            texture_pool_capacity           = 2  * 1024;        /**< Maximum number of concurrent texture allocations */
        uint32_t            pipeline_pool_capacity          = 1  * 1024;        /**< Maximum number of concurrent graphics pipelines */
        uint32_t            compute_pipeline_pool_capacity  = 1  * 1024;        /**< Maximum number of concurrent compute pipelines */
    } limits;

    gfx_allocator_t*        allocator;                                  /**< Custom memory allocator hook (nullptr for system default) */
    gfx_callback            dbglog;                                     /**< Call pointer for routing diagnostic logging events */
} gfx_settings_t;


typedef struct gfx_caps_t {
    gfx_gpu_type            gpu_type;                           /**< Hardware architecture type (discrete or integrated) */
    char                    gpu_name[64];                       /**< Human-readable product name string of the GPU */
    char                    gpu_vendor[64];                     /**< GPU manufacturer name string (e.g., NVIDIA, AMD, Intel, Apple) */

    // Hardware Texture Compression Layout Support
    bool                    support_pvr;                        /**< True if PVRTC mobile texture compression formats are supported */
    bool                    support_etc;                        /**< True if ETC1/ETC2 mobile texture compression formats are supported */
    bool                    support_astc;                       /**< True if ASTC scalable mobile texture compression formats are supported */
    bool                    support_bc;                         /**< True if desktop Block Compression formats (DXTC / BC1-BC7) are supported */

    // Advanced Pipeline & Architecture Features
    bool                    support_compute;                    /**< True if general purpose compute shader pipelines are available */
    bool                    support_raytrace;                   /**< True if hardware-accelerated ray tracing pipeline is available */
    bool                    support_indirect;                   /**< True if GPU indirect command argument drawing is available */
    bool                    support_bindless;                   /**< True if unbounded bindless descriptor indexing is supported */

    // Next-Gen Geometry Pipeline Support
    bool                    support_mesh_shader;                /**< True if advanced geometry mesh shaders are supported */
    bool                    support_mesh_amplification_shader;  /**< True if amplification/task mesh shaders are supported */

    // --- Shader Binary Capabilities ---
    uint32_t                supported_shader_formats;           /**< Bitmask matching combinations of gfx_shader_format_flags */

    // Hardware Constraints & Limits Boundaries
    uint32_t                max_texture_dimension_2d;           /**< Maximum allowed width/height for a 2D texture allocation */
    uint32_t                max_compute_work_group_invocations; /**< Maximum total number of threads inside a single compute work group */
    uint64_t                min_uniform_buffer_offset_alignment;/**< Required byte alignment offset multiplier for dynamic UBO bindings */
    uint64_t                min_storage_buffer_offset_alignment;/**< Required byte alignment offset multiplier for dynamic SSBO bindings */
    uint64_t                max_uniform_buffer_range;           /**< Maximum byte allocation size range that can be bound to a single UBO slot */

    // Extended Shader Data Types & Atomics
    bool                    support_shader_float16;             /**< True if shader supports native 16-bit floating-point math (half) */
    bool                    support_shader_int8;                /**< True if shader supports native 8-bit integer math (int8_t) */
    bool                    support_shader_atomic_float32;      /**< True if atomic operations are supported on float variables in SSBOs */
    bool                    support_shader_subgroup_ops;        /**< True if wave/subgroup voting and shuffling operations are available */

    // Architecture & Optimization Architectural Hints
    bool                    has_unified_memory;                 /**< True if CPU and GPU share the same system RAM pool (Unified Memory) */
    bool                    support_barycentrics;               /**< True if fragment shader can access raw hardware barycentric coordinates */
    bool                    support_conservative_rasterization; /**< True if conservative overestimation rasterization features are enabled */
} gfx_caps_t;


// ============================================================================
// --- Core Resource Descriptors ---
// ============================================================================


typedef struct gfx_surface_desc_t {
    const char*             label;              /**< Optional debug metadata string literal identifier */
    intptr_t                window_handle;      /**< Native window reference token (e.g., HWND on Win32, NSWindow* on macOS) */
    bool                    vsync;              /**< Vertically synchronize presentation to refresh rates (default = true) */
    bool                    enable_hdr;         /**< Reserved for high dynamic range surface configuration layouts */
    
    gfx_pixel_format        preferred_format;   /**< Preferred surface pixel component output format layout */
    gfx_sample_count        sample_count;       /**< Surface output multisampling anti-aliasing count */
} gfx_surface_desc_t;

 
typedef struct gfx_sampler_desc_t {
    gfx_filter              minmag;             /**< Minification and magnification pixel processing filter mode */
    gfx_filter              mipmap;             /**< Mipmap transition blending interpolation filter mode */
    gfx_address_mode        mode;               /**< Out-of-bounds UV coordinates edge addressing wrapping policy */
    uint32_t                anisotropy;         /**< Anisotropic filtering clamp level (0 to disable) */
} gfx_sampler_desc_t;


typedef struct gfx_texture_desc_t {
    const char*             label;              /**< Optional debug metadata string literal identifier */
    uint32_t                width;              /**< Horizontal pixel boundary allocation measurement */
    uint32_t                height;             /**< Vertical pixel boundary allocation measurement */
    uint32_t                depth;              /**< Volumetric depth layers, array slices, or cubemap faces count */
    void*                   data;               /**< Optional raw memory pointer payload mapped to populate initial mipmaps levels */
    uint32_t                mip_levels;         /**< Total requested mipmap chains level allocations */  
    uint32_t                usage_flags;        /**< Bitmask matching combinations of gfx_texture_usage_flags */
    
    gfx_texture_type        type;               /**< Structural dimensional class layout type */
    gfx_pixel_format        format;             /**< Compressed or uncompressed element bit layout design */
    uint32_t                swizzle_mask;       /**< Formatted channel swizzle code (e.g., generated via gfx_make_swizzle_mask) */
} gfx_texture_desc_t;


typedef struct gfx_buffer_desc_t {
    const char*             label;              /**< Optional debug metadata string literal identifier */
    gfx_memory_hint         memory_hint;        /**< Explicit hardware memory allocation priority placement suggestion */
    gfx_buffer_usage        usage;              /**< Target pipeline hardware binding role mask configuration */
    bool                    mapped;             /**< Flag enabling persistent mapping of host memory for fast writes */
    uint32_t                size;               /**< Total capacity boundaries allocated measured in bytes */
    void*                   data;               /**< Optional host pointer used to clone initial data directly into allocation */
} gfx_buffer_desc_t;


// ============================================================================
// --- Shader Reflection & Layout Descriptors ---
// ============================================================================

/**
 * @brief Layout metadata payload extracted from automated shader reflection parsing.
 */
typedef struct gfx_uniform_t {
    char                    name[32];           /**< Unique shader variable layout name token identifier */
    gfx_uniform_type        type;               /**< Layout component parameter block or binding type */
    uint16_t                stage_mask;         /**< Bitmask of bound programmable shader stages executing this variable */
    uint16_t                binding;            /**< Hardware resource registration binding index slot */
    uint16_t                group;              /**< Hardware descriptor set layout grouping index register */
    
    struct {
        uint16_t            size;               /**< Total byte capacity footprint bounds of the uniform constant block */
        uint16_t            field_count;        /**< Active member fields count located inside the uniform block constant */
        struct {
            char            name[32];           /**< Variable label assigned to individual component member fields */
            uint16_t        stride;             /**< Memory padding footprint stride offset between array indices */
            uint16_t        offset;             /**< Byte offset address start position relative to uniform base address */
            uint16_t        type;               /**< Variable data element primitive type classification */
        } fields[16];
    } buffer;
    
    struct {
        gfx_access_type     access;             /**< Compute buffer storage layout read/write access visibility */
    } storage;
    
    struct {
        gfx_texture_type    dimension;          /**< Layout dimensional requirements bound to image texture sampling */
        gfx_access_type     access;             /**< Writable storage read/write accessibility state mapping flags */
    } texture;
    
    struct {
        // Reserved for future explicit cross-backend sampler states reflection
    } sampler;
} gfx_uniform_t;


typedef struct gfx_uniform_loc_t {
    union {
        uint64_t handle;                        /**< Compressed opaque 64-bit lookup hash value */
        struct {
            uint32_t        shader_hash;        /**< Parent compiled shader tracking identifier code */
            uint8_t         type;               /**< Underlying uniform binding variable classification role */
            uint8_t         binding;            /**< Pipeline layout registration register location index */
            uint8_t         set_index;          /**< Descriptor array grouping allocation layer pointer */
            uint8_t         member_idx;         /**< Struct member inner field offset index index */
        };
    };
} gfx_uniform_loc_t;


typedef struct gfx_shader_stage_data {
    gfx_shader_stage        stage;              /**< Target hardware pipeline stage execution block assignment */
    void*                   data;               /**< Pointer referencing native compiled shader source or SPIR-V/DXIL bytes */
    uint32_t                size;               /**< Memory boundary footprint size of the data source byte array in bytes */
    const char*             entry;              /**< Function name string literal identifying the main entry execution point */
} gfx_shader_stage_data;


typedef struct gfx_shader_desc_t {
    const char*             label;                      /**< Optional debug metadata string literal identifier */
    uint32_t                descriptor_pool_capacity;   /**< Pre-allocated local descriptor pool count scaling hint limits */

    uint32_t                stage_count;                /**< Total programmable bytecode stages arrays length */
    gfx_shader_stage_data*  stages;                     /**< Array of shader source execution packages */

    uint32_t                uniform_count;              /**< Total reflected variable metadata layouts array length */
    gfx_uniform_t*          uniforms;                   /**< Array of active binding configuration metadata structures */
} gfx_shader_desc_t;


// ============================================================================
// --- Input Assembly & Vertex Layout Descriptors ---
// ============================================================================

typedef struct gfx_vertex_attribute {
//  gfx_semantic            semantic;
    uint32_t                location;           /**< Hardware layout shader location registration index slot (layout(location = X)) */
    uint32_t                binding;            /**< Input slot buffer source binding index register alignment link */
    gfx_vertex_format       format;             /**< Primitive vector component layout size encoding and configuration classification */
    uint32_t                offset;             /**< Byte offset starting boundaries address relative to current vertex index row start */
} gfx_vertex_attribute;


typedef struct gfx_vertex_slot_t {
    uint32_t                binding;            /**< Target input source buffer binding index slot registration alignment link */
    uint32_t                stride;             /**< Byte dimensions footprint width separating individual sequential vertex array blocks */
    gfx_vertex_rate         rate;               /**< Stepping frequency behavior rules (per-vertex or per-instance data steps) */
} gfx_vertex_slot_t;


typedef struct gfx_vertex_assembly {
    gfx_topology            topology = gfx_topology_points; /**< Primitive topology reconstruction layout instructions assembly */

    uint32_t                slot_count;         /**< Length of active input source buffer streams array */
    gfx_vertex_slot_t*      slots;              /**< Array configuring layout stream strides parameters */

    uint32_t                attribute_count;    /**< Length of mapped hardware vector variable bindings array */
    gfx_vertex_attribute*   attributes;         /**< Array mapping element locations layout configurations */
} gfx_vertex_assembly;


// ============================================================================
// --- Fixed-Function Render States Descriptors ---
// ============================================================================

typedef struct gfx_render_states_desc_t {
    gfx_topology            topology    = gfx_topology_triangles;   /**< Hardware fallback primitive reconstruction topology rules */
    gfx_cull                culling     = gfx_cull_none;            /**< Hardware backface culling execution boundary options policies */
    gfx_face                face        = gfx_face_ccw;             /**< Winding evaluation determining vertex normal orientation directions */
    uint32_t                states      = gfx_colormask_rgba | gfx_depth_test | gfx_depth_write; /**< Bitmask combinations mapping gfx_pipeline_flags */

    struct {
        gfx_pixel_format*   color;              /**< Array of pixel formats matching output color render pass attachments formats */
    } attachments;

    struct {
        bool                enable      = false;                /**< Globally toggles frame buffer blend interpolation math engines */
        gfx_blend_mode      color_src   = gfx_blend_mode_one;   /**< Color equation scaling factor values source input multipliers */
        gfx_blend_mode      color_dst   = gfx_blend_mode_one;   /**< Color equation scaling factor values target destination multipliers */
        gfx_blend_op        color_op    = gfx_blend_op_add;     /**< Arithmetic operator equation combining calculated color factors */

        gfx_blend_mode      alpha_src   = gfx_blend_mode_one;   /**< Alpha equation scaling factor values source input multipliers */
        gfx_blend_mode      alpha_dst   = gfx_blend_mode_one;   /**< Alpha equation scaling factor values target destination multipliers */
        gfx_blend_op        alpha_op    = gfx_blend_op_add;     /**< Arithmetic operator equation combining calculated alpha factors */
    } blend;
    
    struct {
        bool                enable      = true;                 /**< Globally toggles geometry visibility depth testing routines */
        bool                write       = true;                 /**< Toggles locking z-buffer outputs for updating pixel depth values */
        gfx_cmp             mode        = gfx_cmp_lequal;       /**< Evaluation comparison math operators matching test conditions pass */
    } depth;

    struct {
        gfx_cmp             func        = gfx_cmp_always;       /**< Testing condition comparison math operators matching stencil bits */
        uint8_t             ref         = 0;                    /**< Reference integer mask bit matching stencil validation checks */
        uint8_t             pass        = 0xFF;                 /**< Bitwise verification logic mask filters applied on dynamic reads */

        gfx_stencil_op      opPass      = gfx_stencil_op_keep;  /**< Updates executed when depth testing and stencil validations both pass */
        gfx_stencil_op      opFail      = gfx_stencil_op_keep;  /**< Updates executed when stencil validation fails immediately */
        gfx_stencil_op      opZFail     = gfx_stencil_op_keep;  /**< Updates executed when stencil validation passes but depth testing fails */
    } stencil;

    struct {
        bool                red         = true;                 /**< Allow calculations to write pixels to output Red color channels */
        bool                green       = true;                 /**< Allow calculations to write pixels to output Green color channels */
        bool                blue        = true;                 /**< Allow calculations to write pixels to output Blue color channels */
        bool                alpha       = true;                 /**< Allow calculations to write pixels to output Alpha color channels */
    } color_mask;
} gfx_render_states_desc_t;


// ============================================================================
// --- Render Target & Pipeline Descriptors ---
// ============================================================================

typedef struct gfx_render_target_desc_t {
    uint16_t                    width;                      /**< Target frame buffer width boundary in pixels */
    uint16_t                    height;                     /**< Target frame buffer height boundary in pixels */
    
    uint32_t                    color_attachment_count;     /**< Length of active color attachment pixel formats array */
    gfx_pixel_format*           color_attachment_formats;   /**< Array defining pixel formats for each bound color output view */
    gfx_pixel_format            depth_attachment_format;    /**< Selected depth/stencil attachment pixel format layout */

    gfx_sample_count            sample_count;               /**< Multisample anti-aliasing sample count configurations */
} gfx_render_target_desc_t;


typedef struct gfx_pipeline_desc_t {
    gfx_shader_t*               shader;                     /**< Pointer to the compiled multi-stage graphics shader program */
    gfx_render_states_desc_t    render_states;              /**< Comprehensive fixed-function hardware blending, depth, and stencil states */
//  gfx_render_pass_desc_t      render_pass;
    gfx_vertex_assembly         assembly;                   /**< Input assembly topology layouts and vertex streaming buffers strides attributes */
} gfx_pipeline_desc_t;


typedef struct gfx_compute_pipeline_desc_t {
    gfx_shader_t*               shader;                     /**< Pointer to the compiled single-stage compute shader state machine */
} gfx_compute_pipeline_desc_t;

/**
 * @brief Configuration payload reserved for modern next-gen geometry mesh shading pipeline state machine structures.
 * @todo Implement data layouts matching mesh/amplification shaders combinations.
 */
typedef struct gfx_mesh_pipeline_desc_t{
    const char*                 label;                  /**< Optional debug metadata string literal identifier */
    gfx_shader_t*               shader;                 /**< Pointer to the compiled shader containing gfx_shader_mesh (and optionally gfx_shader_amplify) */
    gfx_render_states_desc_t    render_states;          /**< Comprehensive fixed-function blending, depth, and stencil states */
} gfx_mesh_pipeline_desc_t;

/**
 * @brief Configuration payload reserved for hardware-accelerated ray tracing pipeline state machine structures.
 * @todo Implement tracking properties driving shader binding tables generation.
 */
struct gfx_raytrace_pipeline_desc_t;

typedef struct gfx_pass_info_t {
    uint32_t                    clear_color_value;   /**< Packed hexadecimal RGBA8 color value used to scrub color targets on load */
    float                       clear_depth_value;      /**< Packed depth precision scaling token used to wipe z-buffer values on load */
    uint32_t                    clear_stencil_value;    /**< Packed depth precision scaling token used to wipe z-buffer values on load */

    gfx_render_target_t*        target;                     // todo: replace to color + depth attachment
    gfx_texture_t*              color_attachment[8];        // todo: switch to this after getting rid of gfx_render_target_t
    gfx_texture_t*              depth_attachment;           // todo: switch to this after getting rid of gfx_render_target_t

    // todo: load operator - clear, store, dont_care
    //gfx_load_op               color_load_op[8];
    //gfx_load_op               depth_load_op;
} gfx_pass_info_t;


// ============================================================================
// --- GPU Execution Argument Command Layouts ---
// ============================================================================

/**
 * @brief Fixed hardware memory layout driving indexed indirect draw argument evaluations.
 * @note Binary layout directly matches VkDrawIndexedIndirectCommand and D3D12_DRAW_INDEXED_ARGUMENTS.
 */
typedef struct indirect_data_t {
    uint32_t                    index_count;                /**< Number of indices to read from the bound index buffer */
    uint32_t                    instance_count;             /**< Number of geometry instances to draw via instanced rendering */
    uint32_t                    first_index;                /**< Element offset position location mapped within the bound index buffer */
    int32_t                     base_vertex;                /**< Signed constant value added to vertex index indices inside hardware streams */
    uint32_t                    first_instance;             /**< Starting base identification instance register value id */
} indirect_data_t;


typedef struct gfx_frame_t {
    gfx_context_t*              ctx;                        /**< Primary reference link referencing active subsystem graphics instance */
    uint32_t                    frame_index;                /**< Linear monotonically increasing frame execution tracker index (0,1..n) */
    uint32_t                    swapchain_image_index;      /**< Current active texture surface image target slot returned from swapchain engine */
    gfx_surface_t*              surface;                    /**< Active presentation window viewport surface abstraction */
    gfx_render_target_t*        target;                     /**< Active render target frame buffer containing current color and depth views */

    gfx_texture_t*              color_attachment;           // todo: switch to this after getting rid of gfx_render_target_t
    gfx_texture_t*              depth_attachment;           // todo: switch to this after getting rid of gfx_render_target_t
    gfx_command_buffer_t*       cmd;                        /**< Primary command buffer instance logging commands generated during this frame step */
} gfx_frame_t;



// ============================================================================
// --- Core Subsystem Context & Surface Lifetime ---
// ============================================================================

/**
 * @brief Initializes the primary graphics subsystem context based on requested settings.
 * @param settings Pointer to the initial runtime and limits configuration.
 * @param[out] ctx Pointer to a pointer that will store the allocated context handle.
 */
gfx_api void gfx_init(gfx_settings_t* settings, gfx_context_t** ctx);

/**
 * @brief Queries the hardware capabilities matrix and supported features of the active GPU.
 * @param ctx Reference to the active graphics context.
 * @param[out] caps Pointer to the structure that will be populated with device capabilities.
 */
gfx_api void gfx_get_caps(gfx_context_t* ctx, gfx_caps_t* caps);

/**
 * @brief Creates an OS-tied presentation surface viewport.
 * @param ctx Reference to the active graphics context.
 * @param desc Configuration parameters for the presentation window.
 * @return A unique type-safe handle to the created surface.
 */
gfx_api gfx_surface_t gfx_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc);

/**
 * @brief Safely tears down and releases an active presentation surface viewport.
 * @param surface Handle of the surface to destroy.
 */
gfx_api void gfx_surface_destroy(gfx_context_t* ctx, gfx_surface_t surface);

/**
 * @brief Begins a new frame cycle, acquiring the next available swapchain image.
 * @param ctx Reference to the active graphics context.
 * @param surface Pointer to the presentation surface bound to the current frame thread.
 * @return Pointer to a populated frame orchestration transport package.
 */
gfx_api gfx_frame_t* gfx_begin_frame(gfx_context_t* ctx, gfx_surface_t* surface);

/**
 * @brief Concludes the current frame cycle and submits the image for presentation.
 * @param frame Pointer to the tracking frame data package to be closed.
 * @return Execution result tracking success or device-loss states.
 */
gfx_api gfx_result gfx_end_frame(gfx_frame_t* frame);



// ============================================================================
// --- Hardware Data Buffers ---
// ============================================================================

/**
 * @brief Allocates a new hardware data buffer (Vertex, Index, Uniform, or Storage).
 * @param ctx Reference to the active graphics context.
 * @param desc Properties and usage flags driving the allocation layout.
 * @return Pointer to the allocated internal buffer wrapper state.
 */
gfx_api gfx_buffer_t* gfx_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc);

/**
 * @brief Synchronously uploads raw host memory data into an existing hardware buffer slot.
 * @param ctx Reference to the active graphics context.
 * @param buffer Target hardware buffer allocation to update.
 * @param data Source host memory address pointer containing the update payload.
 * @param size Total number of bytes to copy from the source pointer.
 * @param offset Byte offset destination position inside the hardware buffer memory.
 */
gfx_api void gfx_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);

/**
 * @brief Reclaims hardware memory allocations bound to an active data buffer.
 * @param ctx Reference to the active graphics context.
 * @param buffer Reference pointer to the buffer wrapper to be destroyed.
 */
gfx_api void gfx_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer);



// ============================================================================
// --- Programmable Shader States ---
// ============================================================================

/**
 * @brief Compiles and links an executable multi-stage shader program binary.
 * @param ctx Reference to the active graphics context.
 * @param desc Structural assembly bytecode configurations.
 * @return Pointer to the compiled internal shader state mapping metadata.
 */
gfx_api gfx_shader_t* gfx_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc);

/**
 * @brief Queries the total number of unique descriptor set layout groupings used by the shader.
 * @param shader Reference pointer to the compiled shader program.
 * @return Integer length of active resource group configurations layout blocks.
 */
gfx_api uint32_t gfx_shader_get_descriptor_set_count(gfx_shader_t* shader);

/**
 * @brief Populates metadata fields for all variables associated with a resource group index.
 * @param shader Reference pointer to the compiled shader program.
 * @param set_index Target layout grouping/set index to query.
 * @param[out] uniforms Array that will be populated with reflected uniform data.
 * @return Total number of uniforms written into the output array.
 */
gfx_api uint32_t gfx_shader_get_uniforms(gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms);

/**
 * @brief Generates a pre-packed location handle for ultra-fast uniform variable lookup operations.
 * @param shader Reference pointer to the compiled shader program.
 * @param name String literal identity token matching the shader variable name.
 * @return Compressed 64-bit packed uniform location mapping parameters token.
 */
gfx_api uint64_t gfx_uniform_location(gfx_shader_t* shader, const char* name);

/**
 * @brief Disposes of binary module states bound to a compiled shader program.
 * @param ctx Reference to the active graphics context.
 * @param shader Reference pointer to the shader program instance to be destroyed.
 */
gfx_api void gfx_shader_destroy(gfx_context_t* ctx, gfx_shader_t* shader);


// ============================================================================
// --- Descriptor Resource Binding Sets ---
// ============================================================================

/**
 * @brief Allocates a mutable resource layout binding set matching a specific shader layout.
 * @param ctx Reference to the active graphics context.
 * @param shader Reference pointer to the compiled shader program defining the layout.
 * @param set_idx Mapping index of the descriptor group set register (e.g., set = X).
 * @return Pointer to the allocated resource descriptor set binding tracker.
 */
gfx_api gfx_descriptor_set_t* gfx_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx);

/**
 * @brief Schedules an immediate update writing host-provided raw values into a set's UBO memory slice.
 * @param set Reference pointer to the descriptor binding set wrapper.
 * @param handle Dynamic packed layout location handle token generated via uniform reflection lookup.
 * @param data Address pointer containing host update payload arrays.
 * @param size Total payload data array length footprint bounds measured in bytes.
 */
gfx_api void gfx_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);

/**
 * @brief Binds a specific hardware buffer allocation into a set's register binding uniform slot.
 * @param set Reference pointer to the descriptor binding set wrapper.
 * @param handle Dynamic packed layout location handle token generated via uniform reflection lookup.
 * @param buffer Reference pointer to the hardware buffer allocation to link.
 * @param offset Starting byte offset position alignment parameter mapped inside the buffer.
 */
gfx_api void gfx_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset);

/**
 * @brief Binds a specific hardware image texture allocation view into a set's sample variable slot.
 * @param set Reference pointer to the descriptor binding set wrapper.
 * @param handle Dynamic packed layout location handle token generated via uniform reflection lookup.
 * @param texture Reference pointer to the target texture asset object to link.
 */
gfx_api void gfx_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);

/**
 * @brief Binds a filtering configuration block sampler state into a set's sampler variable slot.
 * @param set Reference pointer to the descriptor binding set wrapper.
 * @param handle Dynamic packed layout location handle token generated via uniform reflection lookup.
 * @param sampler Reference pointer to the filtering configurations sampler asset object to link.
 */
gfx_api void gfx_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);

/**
 * @brief Unbinds and releases tracking pools structures allocated to an active descriptor binding set.
 * @param ctx Reference to the active graphics context.
 * @param descriptor Reference pointer to the descriptor set wrapper instance to be destroyed.
 */
gfx_api void gfx_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);


// ============================================================================
// --- Sampler & Texture Resources ---
// ============================================================================

/**
 * @brief Allocates an immutable hardware state block for filtering texture coordinate lookups.
 * @param ctx Reference to the active graphics context.
 * @param desc Properties configuring edge wrapping and anisotropy rules.
 * @return Pointer to the allocated internal sampler asset structure.
 */
gfx_api gfx_sampler_t* gfx_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc);

/**
 * @brief Deallocates hardware states tracking an active sampler asset block.
 * @param ctx Reference to the active graphics context.
 * @param sampler Reference pointer to the sampler instance to be destroyed.
 */
gfx_api void gfx_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler);

/**
 * @brief Allocates an empty or populated dimensional image texture memory object.
 * @param ctx Reference to the active graphics context.
 * @param desc Resolution dimensions, pixel format layouts, and capability rules flags.
 * @return Pointer to the allocated internal texture asset structure.
 */
gfx_api gfx_texture_t* gfx_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc);

/**
 * @brief Synchronously pushes raw pixel host arrays directly into an active texture block.
 * @param ctx Reference to the active graphics context.
 * @param texture Target hardware texture allocation to update.
 * @param data Source host memory pointer containing the raw pixel data payload.
 * @param size Total number of image bytes to copy from the source pointer.
 * @param offset Pixel byte offset location within the allocated texture region.
 */
gfx_api void gfx_texture_update_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset);

/**
 * @brief Binds a texture allocation into a specific slot within the global bindless table.
 * @param ctx Reference to the active graphics context.
 * @param texture Reference pointer to the target texture asset.
 * @param idx Dedicated layout index slot within the global bindless array table.
 */
gfx_api void gfx_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx);

/**
 * @brief Dispatches hardware routines to dynamically generate downstream mipmap downsamples.
 * @param ctx Reference to the active graphics context.
 * @param texture Target texture asset to generate mipmaps for.
 */
gfx_api void gfx_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture);

/**
 * @brief Performs a fast GPU blit copy transfer operation separating two texture surface boundaries.
 * @param ctx Reference to the active graphics context.
 * @param src Reference pointer to the source texture allocation view.
 * @param dst Reference pointer to the destination texture allocation view.
 */
gfx_api void gfx_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst);

/**
 * @brief Submits a read-back request to clone hardware texture pixel arrays back onto host-visible space.
 * @param ctx Reference to the active graphics context.
 * @param cmd Active command buffer instance scheduling execution tracking.
 */
gfx_api void gfx_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd);

/**
 * @brief Releases hardware allocations bound to an active image texture layout memory block.
 * @param ctx Reference to the active graphics context.
 * @param texture Reference pointer to the texture asset instance to be destroyed.
 */
gfx_api void gfx_texture_destroy(gfx_context_t* ctx, gfx_texture_t* texture);


// ============================================================================
// --- Pipeline State Machines (PSO) ---
// ============================================================================

/**
 * @brief Compiles a full fixed+programmable vertex and fragment state machine pipeline.
 * @param ctx Reference to the active graphics context.
 * @param desc Inputs layouts assembly links, render states, and shaders packages.
 * @return Pointer to the allocated monolithic graphics pipeline state object.
 */
gfx_api gfx_pipeline_t* gfx_pipeline_create(gfx_context_t* ctx, gfx_pipeline_desc_t* desc);

/**
 * @brief Destroys and cleans up an active graphics pipeline state object.
 * @param ctx Reference to the active graphics context.
 * @param pipeline Reference pointer to the graphics pipeline instance to be destroyed.
 */
gfx_api void gfx_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_t* pipeline);

/**
 * @brief Compiles a hardware data-parallel compute pipeline state machine layout.
 * @param ctx Reference to the active graphics context.
 * @param desc Properties wrapping target single-stage compute shader modules.
 * @return Pointer to the allocated monolithic compute pipeline state object.
 */
gfx_api gfx_pipeline_compute_t* gfx_compute_pipeline_create(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc);

/**
 * @brief Destroys and cleans up an active compute pipeline state object.
 * @param ctx Reference to the active graphics context.
 * @param pipeline Reference pointer to the compute pipeline instance to be destroyed.
 */
gfx_api void gfx_compute_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);

/**
 * @brief Compiles a next-gen task and mesh geometry acceleration pipeline state layout.
 * @param ctx Reference to the active graphics context.
 * @param desc Geometry processing properties packing mesh shader configurations.
 * @return Pointer to the allocated monolithic mesh shading pipeline state object.
 */
gfx_api gfx_pipeline_t* gfx_pipeline_mesh_create(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc);

/**
 * @brief Destroys and cleans up an active mesh shading pipeline state object.
 * @param ctx Reference to the active graphics context.
 * @param pipeline Reference pointer to the mesh shading pipeline instance to be destroyed.
 */
gfx_api void gfx_pipeline_mesh_destroy(gfx_context_t* ctx, gfx_pipeline_t* pipeline);

/**
 * @brief Compiles a hardware-accelerated ray tracing raygen/hit/miss pipeline state object.
 * @param ctx Reference to the active graphics context.
 * @param desc Intersection acceleration properties driving shader binding tables.
 * @return Pointer to the allocated monolithic ray tracing pipeline state object.
 */
gfx_api gfx_pipeline_raytrace_t* gfx_pipeline_raytrace_create(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc);

/**
 * @brief Destroys and cleans up an active ray tracing pipeline state object.
 * @param ctx Reference to the active graphics context.
 * @param pipeline Reference pointer to the ray tracing pipeline instance to be destroyed.
 */
gfx_api void gfx_pipeline_raytrace_destroy(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);



// ============================================================================
// --- Command Buffer Recording & Recording Passes ---
// ============================================================================

/**
 * @brief Pushes a named visual region marker onto the GPU execution tracking profile stack.
 * @param cmd Active command buffer token capturing logging context.
 * @param marker String literal label naming the diagnostic block region.
 */
gfx_api void gfx_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker);

/**
 * @brief Pops the top diagnostic region marker block from the active GPU execution profile stack.
 * @param cmd Active command buffer token capturing logging context.
 */
gfx_api void gfx_cmd_pop_marker(gfx_command_buffer_t* cmd);

/**
 * @brief Opens a new hardware rendering pass block clearing and binding active attachments views targets.
 * @param cmd Active command buffer token recording graphic draw state sequences tokens.
 * @param target Reference pointer to the frame buffer target wrapper container to execute inside.
 */
gfx_api void gfx_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* desc);

/**
 * @brief Closes the active rendering pass block, resolving multisample targets and executing layout transitions.
 * @param cmd Active command buffer token recording graphic draw state sequences tokens.
 */
gfx_api void gfx_cmd_end_pass(gfx_command_buffer_t* cmd);

// ============================================================================
// --- Dynamic States, Bindings & Draw Dispatches ---
// ============================================================================

/**
 * @brief Updates hardware scissor rect boundaries regions filtering out out-of-bounds rendering pixels.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param x Horizontal pixel position offset start anchor.
 * @param y Vertical pixel position offset start anchor.
 * @param w Scissor box width dimensions measured in pixels.
 * @param h Scissor box height dimensions measured in pixels.
 */
gfx_api void gfx_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

/**
 * @brief Updates hardware viewport transformation scaling properties mapping clip space dimensions.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param x Horizontal viewport start floating coordinates location anchor.
 * @param y Vertical viewport start floating coordinates location anchor.
 * @param w Viewport width layout dimensions mapping span resolution.
 * @param h Viewport height layout dimensions mapping span resolution.
 */
gfx_api void gfx_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

/**
 * @brief Binds a compiled pipeline layout state object regulating fixed-function assembly routines.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param pipeline Reference pointer to the pipeline state object to activate.
 */
gfx_api void gfx_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);

/**
 * @brief Links an updated descriptor binding group set into a pipeline layout resource register slot.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param slot Registration array set registry layout index location (e.g., set = slot).
 * @param descriptor Reference pointer to the resource variables group wrapper package to link.
 */
gfx_api void gfx_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);

/**
 * @brief Binds an allocated hardware data buffer to drive geometry index stream layouts assembly lookups.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param format Bit width size encoding of index elements data streams rows (16 or 32-bit).
 * @param offset Starting byte offset position within the bound index buffer.
 * @param buffer Reference pointer to the index buffer allocation.
 */
gfx_api void gfx_cmd_bind_index_buffer(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);

/**
 * @brief Binds an allocated hardware data buffer to drive geometry vertex attribute streams processing layouts.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param slot Stream input registration slot layout mapping link alignment index.
 * @param offset Starting byte offset position within the bound vertex buffer.
 * @param buffer Reference pointer to the vertex buffer allocation.
 */
gfx_api void gfx_cmd_bind_vertex_buffer(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);

/**
 * @brief Commits a non-indexed direct drawing command token onto the graphics queue streams.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param vertex_count Total sequential vertex positions to read from the bound vertex stream.
 * @param instance_count Total geometry duplicates to generate via instanced hardware routines.
 */
gfx_api void gfx_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);

/**
 * @brief Commits an indexed drawing command token optimal for vertex-sharing topology architectures.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param index_count Total element indices to fetch from the bound index buffer.
 * @param first_index Elements starting index lookup offset index within the index buffer.
 * @param instance_count Total geometry duplicates to generate via instanced hardware routines.
 * @param vertex_offset Signed scalar offset added directly onto vertex layout indicators indices inside streams.
 */
gfx_api void gfx_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t index_count, uint32_t first_index, uint32_t instance_count, uint32_t vertex_offset);

/**
 * @brief Commits a GPU indirect drawing command fetching rendering draw arguments arrays out from hardware buffers.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param buffer Reference pointer to the storage buffer holding structured arrays of indirect_data_t arguments primitives.
 * @param offset Byte starting offset position matching draw data arrays positions layout properties.
 * @param draw_count Total sequence length of independent indirect draw tokens to execute in sequence loops blocks.
 * @param stride Memory stride spacing separating independent argument packets elements properties.
 */
gfx_api void gfx_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);

/**
 * @brief Dispatches a 3D thread grid blocks payload onto a bound active parallel compute shader layout.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param x Length grid count size multiplier matching thread block groupings layout widths.
 * @param y Length grid count size multiplier matching thread block groupings layout heights.
 * @param z Length grid count size multiplier matching thread block groupings layout depths layers.
 */
gfx_api void gfx_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

// ============================================================================
// --- Pipeline Resource Execution Barriers ---
// ============================================================================

/**
 * @brief Injects an execution and pipeline memory hazard barrier token across a set of active hardware data buffers.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param buffers Array pointer tracking allocated buffers wrappers affected by transition changes.
 * @param count Length of active buffers pointers array blocks.
 * @param src Previous layout access state domain pipeline role block rule.
 * @param dst Target next layout access state domain pipeline role block rule.
 */
gfx_api void gfx_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);

/**
 * @brief Injects an execution and texture layout hazard barrier token across a set of active image textures memory blocks.
 * @param cmd Active command buffer token logging states parameters changes.
 * @param textures Array pointer tracking allocated textures assets affected by transition layout modifications.
 * @param count Length of active textures pointers array blocks.
 * @param src Previous layout access state domain pipeline role block rule.
 * @param dst Target next layout access state domain pipeline role block rule.
 */
gfx_api void gfx_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);




// ============================================================================
// --- Mesh Shading Command Recording Dispatches ---
// ============================================================================

/**
 * @brief Launches hardware mesh shading execution blocks (Task/Mesh threads) to generate geometry on the GPU.
 * @note Directly maps to vkCmdDrawMeshTasksEXT and DX12 DispatchMesh behavior.
 * @param cmd Active command buffer token capturing execution streams changes.
 * @param task_count_x Number of local workgroups dispatched in the X dimension (launches Task/Amplify shader if present, otherwise Mesh shader directly).
 * @param task_count_y Number of local workgroups dispatched in the Y dimension.
 * @param task_count_z Number of local workgroups dispatched in the Z dimension.
 */
gfx_api void gfx_cmd_draw_mesh_tasks(gfx_command_buffer_t* cmd, uint32_t task_count_x, uint32_t task_count_y, uint32_t task_count_z);

/**
 * @brief Launches hardware mesh shading execution blocks where dispatch parameters are read dynamically from a GPU buffer.
 * @note Directly maps to vkCmdDrawMeshTasksIndirectEXT. Binary layout of the arguments in buffer must match VkDrawMeshTasksIndirectCommandEXT.
 * @param cmd Active command buffer token capturing execution streams changes.
 * @param buffer Reference pointer to the hardware buffer containing draw argument data arrays.
 * @param offset Byte starting offset position inside the bound argument buffer.
 * @param draw_count Total sequence length of independent indirect draw tokens to execute in sequence loops blocks.
 * @param stride Memory stride spacing separating independent argument packets elements properties in bytes.
 */
gfx_api void gfx_cmd_draw_mesh_tasks_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);



// ============================================================================
// --- Ray Tracing Geometry & Acceleration Structures ---
// ============================================================================

/**
 * @brief Configuration definitions for ray tracing geometry flags.
 */
typedef enum gfx_rt_geometry_flags {
    gfx_rt_geometry_opaque = 1 << 0, /**< Geometry contains no transparent pixels; skips any-hit shader execution */
    gfx_rt_geometry_no_duplicate_any_hit = 1 << 1, /**< Prevents duplicate any-hit shader invocations on a single primitive */
} gfx_rt_geometry_flags;

/**
 * @brief Describes the vertex and index buffer inputs used to build a Bottom-Level Acceleration Structure (BLAS).
 */
typedef struct gfx_rt_geometry_desc_t {
    gfx_buffer_t*               vertex_buffer;          /**< Hardware buffer containing triangle vertex position data */
    uint32_t                    vertex_stride;          /**< Stride spacing in bytes separating vertex data rows */
    uint32_t                    vertex_count;           /**< Total number of vertices in the stream */
    gfx_vertex_format           vertex_format;          /**< Data layout component encoding format (usually float3 or float4) */

    gfx_buffer_t*               index_buffer;           /**< Optional hardware buffer containing geometry indices data */
    uint32_t                    index_count;            /**< Number of indices (set to 0 for non-indexed triangle lists) */
    gfx_index_format            index_format;           /**< Width format specification of index elements rows */

    uint32_t                    flags;                  /**< Bitmask configurations combining gfx_rt_geometry_flags */
} gfx_rt_geometry_desc_t;

/**
 * @brief Ray tracing hardware instance transformation matrix layout.
 * @note Directly maps to VkTransformMatrixKHR and D3D12_RAYTRACING_INSTANCE_DESC row-major 3x4 layout.
 */
typedef struct gfx_rt_transform_t {
    float matrix[3][4];                                 /**< affine 3x4 transformation matrix array row-major data */
} gfx_rt_transform_t;

/**
 * @brief Describes a single BLAS instance placed inside a Top-Level Acceleration Structure (TLAS).
 */
typedef struct gfx_rt_instance_desc_t {
    gfx_rt_transform_t          transform;              /**< Spatial affine 3x4 transformation matrix mapping instance space */
    uint32_t                    instance_id : 24; /**< Custom user 24-bit identifier accessible inside shaders via gl_InstanceCustomIndexEXT */
    uint32_t                    mask : 8;  /**< 8-bit visibility test visibility mask to filter ray intersections testing */
    uint32_t                    instance_offset : 24; /**< Shader Binding Table hit group index calculation offset mapping multiplier */
    uint32_t                    flags : 8;  /**< Geometry culling flag overrides (e.g., force opaque, cull backface) */
    gfx_acceleration_structure_t blas;                  /**< Handle referencing the target Bottom-Level Acceleration Structure asset */
} gfx_rt_instance_desc_t;

/**
 * @brief Properties required to allocate and generate an Acceleration Structure (TLAS or BLAS).
 */
typedef struct gfx_acceleration_structure_desc_t {
    const char* label;                  /**< Optional debug metadata string literal identifier */
    bool                        is_top_level;           /**< True to build a TLAS (instancing), False to build a BLAS (geometry triangles) */
    bool                        allow_update;           /**< Flag enabling fast incremental updates (refitting) instead of full rebuilds */

    // BLAS execution inputs configuration properties
    uint32_t                    geometry_count;         /**< Length of active geometries description blocks array (BLAS input data) */
    gfx_rt_geometry_desc_t* geometries;             /**< Array containing geometries description blocks arrays references */

    // TLAS execution inputs configuration properties
    uint32_t                    instance_count;         /**< Total number of physical BLAS instances to pack inside the structure (TLAS input data) */
    gfx_buffer_t* instance_buffer;        /**< Hardware buffer holding populated arrays of gfx_rt_instance_desc_t elements */
} gfx_acceleration_structure_desc_t;



// ============================================================================
// --- Ray Tracing Pipelines & Shader Binding Tables ---
// ============================================================================

/**
 * @brief Describes a grouped collection of shaders combined inside the ray tracing pipeline state.
 */
typedef struct gfx_rt_shader_group_t {
    gfx_shader_stage            type;                   /**< Functional role identifier mapping groups (RAYGEN, MISS, or CALLABLE) */
    uint32_t                    general_shader_idx;     /**< Index of the raygen/miss shader entry in the parent pipeline descriptor */

    // Hit Group configuration properties (for closest-hit, any-hit, intersection combinations)
    uint32_t                    closest_hit_idx;        /**< Index of closest-hit shader, set to 0xFFFFFFFF if unused */
    uint32_t                    any_hit_idx;            /**< Index of any-hit shader, set to 0xFFFFFFFF if unused */
    uint32_t                    intersection_idx;       /**< Index of intersection shader, set to 0xFFFFFFFF if unused */
} gfx_rt_shader_group_t;

/**
 * @brief Configuration parameters required to compile a full Ray Tracing pipeline state machine.
 */
typedef struct gfx_raytrace_pipeline_desc_t {
    const char*                 label;                  /**< Optional debug metadata string literal identifier */
    uint32_t                    max_recursion_depth;    /**< Maximum allowed trace depth recursion limits (usually 1 or 2 for performance) */

    uint32_t                    shader_count;           /**< Total independent compiled ray tracing shader binaries length array */
    gfx_shader_stage_data*      shaders;                /**< Array containing target shader binary sources packages */

    uint32_t                    group_count;            /**< Total compiled pipeline shader linkage combinations groups length array */
    gfx_rt_shader_group_t*      groups;                 /**< Array defining explicit shader table associations linkage loops rules */
} gfx_raytrace_pipeline_desc_t;

/**
 * @brief Configuration descriptor defining Shader Binding Table (SBT) memory allocations maps.
 */
typedef struct gfx_sbt_desc_t {
    gfx_pipeline_raytrace_t*    pipeline;               /**< Compile ray tracing pipeline state mapping layouts generation handles */

    uint32_t                    raygen_group_idx;       /**< Pipeline group index mapped into the Ray Generation SBT record section */
    uint32_t                    miss_group_start_idx;   /**< Pipeline group starting index mapped to the Miss SBT records section */
    uint32_t                    miss_group_count;       /**< Total sequence length of independent Miss shader records entries arrays */
    uint32_t                    hit_group_start_idx;    /**< Pipeline group starting index mapped to the Hit Group SBT records section */
    uint32_t                    hit_group_count;        /**< Total sequence length of independent Hit Group shader records entries arrays */
} gfx_sbt_desc_t;


// ============================================================================
// --- Ray Tracing Resource Allocation & Construction Routines ---
// ============================================================================

/**
 * @brief Allocates empty internal buffers required to drive hardware acceleration structure builds.
 * @param ctx Reference to the active graphics context.
 * @param desc Properties configuring geometries lists data weights bounds or instancing scales.
 * @return A unique type-safe handle to the created acceleration structure.
 */
gfx_api gfx_acceleration_structure_t gfx_acceleration_structure_create(gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc);

/**
 * @brief Releases hardware allocations bound to an active acceleration structure object.
 * @param ctx Reference to the active graphics context.
 * @param acceleration_structure Handle of the structure to destroy.
 */
gfx_api void gfx_acceleration_structure_destroy(gfx_context_t* ctx, gfx_acceleration_structure_t acceleration_structure);

/**
 * @brief Allocates an organized Shader Binding Table (SBT) buffer backing ray tracing shader dispatches.
 * @param ctx Reference to the active graphics context.
 * @param desc Properties defining pipeline group index layouts associations maps rules.
 * @return A unique type-safe handle to the initialized shader binding table.
 */
gfx_api gfx_sbt_t gfx_sbt_create(gfx_context_t* ctx, gfx_sbt_desc_t* desc);

/**
 * @brief Releases hardware storage allocations bound to an active Shader Binding Table buffer.
 * @param ctx Reference to the active graphics context.
 * @param sbt Handle of the shader binding table instance to destroy.
 */
gfx_api void gfx_sbt_destroy(gfx_context_t* ctx, gfx_sbt_t sbt);

// ============================================================================
// --- Ray Tracing Command Recording Dispatches ---
// ============================================================================

/**
 * @brief Records a command token requesting full GPU acceleration structure geometry generation builds or refit updates.
 * @param cmd Active command buffer token capturing execution streams steps changes.
 * @param dst Target acceleration structure handle to build or update.
 * @param src Optional source acceleration structure handle required if performing incremental refit updates.
 */
gfx_api void gfx_cmd_build_acceleration_structure(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t dst, gfx_acceleration_structure_t src);

/**
 * @brief Dispatches threads executing Ray Generation shader entry loops to trace primitives inside a TLAS.
 * @param cmd Active command buffer token capturing execution streams steps changes.
 * @param pipeline Active compiled ray tracing pipeline state engine layout to bind.
 * @param sbt Active Shader Binding Table buffer supplying memory jumps pointers locations maps.
 * @param width Horizontal dimension thread grid count dispatch width bounds metrics resolution.
 * @param height Vertical dimension thread grid count dispatch height bounds metrics resolution.
 * @param depth Volumetric thickness layer dimension thread grid count dispatch depth bounds metrics resolution.
 */
gfx_api void gfx_cmd_trace_rays(gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth);





// utility
uint32_t            gfx_utils_thread_id();
uint32_t            gfx_utils_hash_32(const char * data, uint32_t size);
uint32_t            gfx_utils_hash_combine(uint32_t hash1, uint32_t hash2);
gfx_api uint32_t    gfx_utils_image_layer_size(uint32_t width, uint32_t height, uint32_t depth, gfx_pixel_format format);
gfx_api uint32_t    gfx_utils_image_row_pitch(gfx_pixel_format fmt, uint32_t width);
gfx_api uint32_t    gfx_utils_align_up(uint32_t n, uint32_t alignment);

// pool 
struct gfx_handle_pool_t;
gfx_api void        gfx_pool_create(size_t stride, size_t capacity, gfx_handle_pool_t** out_pool, gfx_allocator_t * allocator);
gfx_api void        gfx_pool_destroy(gfx_handle_pool_t* pool);

gfx_api uint64_t    gfx_pool_alloc(gfx_handle_pool_t* pool);
gfx_api void*       gfx_pool_alloc_data(gfx_handle_pool_t* pool, uint64_t * out_handle); // return pointer to allocated data, out parameter returns handle

gfx_api void        gfx_pool_free(gfx_handle_pool_t* pool, uint64_t handle);
gfx_api void*       gfx_pool_map(gfx_handle_pool_t* pool, uint64_t handle);
gfx_api size_t      gfx_pool_get_size(gfx_handle_pool_t* pool);
gfx_api size_t      gfx_pool_get_capacity(gfx_handle_pool_t* pool);


/*
* 
* */
//gfx_api void _gfx_error(ctx, GFX_ERROR_OUT_OF_MEMORY, "Buffer pool exhausted!");
inline void _gfx_error(gfx_context_t * ctx, uint32_t type, const char * msg, ...) {
    //ctx->vtbl
   // ctx->dbg_callback(, "")
}

// Helper for backend stubs:
// - Logs warning if callback exists
// - Asserts in debug builds (so stubs don't silently ship into "working" code paths)
static inline void gfx_stub_not_implemented(gfx_callback dbglog, const char* what)
{
    if (dbglog && what)
        dbglog(gfx_msg_warning, "%s: not implemented", what);

#if !defined(NDEBUG)
    assert(!"gfx backend function not implemented");
#endif
}

static uint32_t gfx_make_swizzle_mask(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return  (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r));
}


typedef struct gfx_api_pfn
{
    // CONTEXT
    void     (*pfn_init)     (gfx_settings_t* settings, gfx_context_t** ctx);
    void     (*pfn_destroy)  (gfx_context_t* ctx);
    void     (*pfn_get_caps) (gfx_context_t* ctx, gfx_caps_t* caps);

    // SWAPCHAIN
    void     (*pfn_surface_create)(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t ** out_surface);
    void     (*pfn_surface_destroy)(gfx_context_t* ctx, gfx_surface_t * surface);

    void     (*pfn_frame_begin) (gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t ** out_frame);
    void     (*pfn_frame_end) (gfx_frame_t* frame);

    // BUFFER
    void     (*pfn_buffer_create)      (gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
    void     (*pfn_buffer_update_data) (gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);
    void     (*pfn_buffer_destroy)     (gfx_context_t* ctx, gfx_buffer_t* buffer);

    // SHADER
    void     (*pfn_shader_create)    (gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
    uint32_t (*pfn_shader_get_descriptor_set_count)(gfx_shader_t* shader);
    uint64_t (*pfn_uniform_location) (gfx_shader_t* shader, const char* name);
    void     (*pfn_shader_destroy)   (gfx_context_t* ctx, gfx_shader_t* shader);

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
    void     (*pfn_mesh_pipeline_create)     (gfx_context_t* ctx, gfx_mesh_pipeline_desc_t * desc, gfx_pipeline_t * *pipeline);
    void     (*pfn_pipeline_destroy)         (gfx_context_t* ctx, gfx_pipeline_t* pipeline);

    void     (*pfn_pipeline_compute_create)  (gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline);
    void     (*pfn_pipeline_compute_destroy) (gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);
  
    void     (*pfn_pipeline_raytrace_create) (gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
    void     (*pfn_pipeline_raytrace_destroy)(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);

    // DESCRIPTOR SET
    void     (*pfn_descriptor_set_create)   (gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** descriptor);
    void     (*pfn_descriptor_set_write_buffer_data) (gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
    void     (*pfn_descriptor_set_write_buffer)      (gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset);
    void     (*pfn_descriptor_set_write_texture)     (gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
    void     (*pfn_descriptor_set_write_sampler)     (gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);
    void     (*pfn_descriptor_set_destroy)  (gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);

    void     (*pfn_cmd_begin_pass) (gfx_command_buffer_t* cmd, gfx_pass_info_t* pass_info);
    void     (*pfn_cmd_end_pass)   (gfx_command_buffer_t* cmd);

    void     (*pfn_cmd_scissor)             (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void     (*pfn_cmd_viewport)            (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void     (*pfn_cmd_bind_pipeline)       (gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
    void     (*pfn_cmd_bind_descriptor_set) (gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);
    void     (*pfn_cmd_bind_buffer_ib)      (gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
    void     (*pfn_cmd_bind_buffer_vb)      (gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
    void     (*pfn_cmd_draw)                (gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
    void     (*pfn_cmd_draw_indexed)        (gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset);
    void     (*pfn_cmd_draw_indexed_indirect)(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
    void     (*pfn_cmd_dispatch_compute)    (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);

    void     (*pfn_cmd_push_marker)         (gfx_command_buffer_t* cmd, const char* marker);
    void     (*pfn_cmd_pop_marker)          (gfx_command_buffer_t* cmd);

    void     (*pfn_cmd_buffer_barrier)  (gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
    void     (*pfn_cmd_texture_barrier) (gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);

    // wip
    void    (*pfn_acceleration_structure_create)(gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc, gfx_acceleration_structure_t **acceleration_struct);
    void    (*pfn_acceleration_structure_destroy)(gfx_context_t* ctx, gfx_acceleration_structure_t * acceleration_structure);
    void    (*pfn_sbt_create)(gfx_context_t* ctx, gfx_sbt_desc_t* desc, gfx_sbt_t **);
    void    (*pfn_sbt_destroy)(gfx_context_t* ctx, gfx_sbt_t *sbt);
    void    (*pfn_cmd_build_acceleration_structure)(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t* dst, gfx_acceleration_structure_t* src);
    void    (*pfn_cmd_trace_rays)(gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth);
} gfx_api_pfn;


#endif // __gfx_webgpu_h__