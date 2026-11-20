
#include "gfx_webgpu.h"

#ifdef WEBGPU_AVAILABLE

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __EMSCRIPTEN__
  #include <emscripten/html5.h>     // canvas size
#else
  #ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
      #define NOMINMAX
    #endif
    #include <windows.h>
    #pragma comment(lib, "x64/webgpu_dawn.lib")
  #endif
#endif


#include "gfx_api.h"    // gfx_pixel_format_linear / gfx_pixel_format_is_srgb

#if GFX_ENABLE_VERBOSE
#define GFX_VERBOSE(exp)            { exp; }
#else
#define GFX_VERBOSE(exp)            {}
#endif

// gfx_pass_info_t::target is [[deprecated]] but still the way frames hand out their back buffer
#if defined(_MSC_VER) && !defined(__clang__)
  #define GFX_DISABLE_DEPRECATED_WARNING_BEGIN __pragma(warning(push)) __pragma(warning(disable: 4996))
  #define GFX_DISABLE_DEPRECATED_WARNING_END   __pragma(warning(pop))
#else
  #define GFX_DISABLE_DEPRECATED_WARNING_BEGIN _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
  #define GFX_DISABLE_DEPRECATED_WARNING_END   _Pragma("GCC diagnostic pop")
#endif

extern const char* gfx_to_string(gfx_buffer_usage usage);
extern const char* gfx_to_string(gfx_shader_stage stage);
extern const char* gfx_to_string(gfx_texture_type type);
extern const char* gfx_to_string(gfx_pixel_format format);

static wgpu_context_t* from_ctx(gfx_context_t* ctx) {
    return (wgpu_context_t*)ctx;
}

// ============================================================================
// helpers
// ============================================================================

static WGPUStringView wgpu_str(const char* s)
{
    WGPUStringView v = { s, s ? WGPU_STRLEN : 0 };
    return v;
}

static int wgpu_str_len(WGPUStringView v)
{
    if (!v.data) return 0;
    return (int)(v.length == WGPU_STRLEN ? strlen(v.data) : v.length);
}

static void default_log(gfx_msg type, const char* msg, ...)
{
    (void)type; (void)msg;
}

// Blocks until the future completes (instance created with TimedWaitAny). On the web this needs ASYNCIFY/JSPI.
static bool wgpu_wait(WGPUInstance instance, WGPUFuture future)
{
    WGPUFutureWaitInfo info = {};
    info.future = future;
    return wgpuInstanceWaitAny(instance, 1, &info, UINT64_MAX) == WGPUWaitStatus_Success;
}

static WGPUPrimitiveTopology gfx_topology_2_webgpu(gfx_topology topology)
{
    switch (topology)
    {
        case gfx_topology_points:           return WGPUPrimitiveTopology_PointList;
        case gfx_topology_lines:            return WGPUPrimitiveTopology_LineList;
        case gfx_topology_lines_strip:      return WGPUPrimitiveTopology_LineStrip;
        case gfx_topology_triangles:        return WGPUPrimitiveTopology_TriangleList;
        case gfx_topology_triangles_strip:  return WGPUPrimitiveTopology_TriangleStrip;
        default:                            return WGPUPrimitiveTopology_TriangleList;
    }
}

static WGPUTextureFormat gfx_pixel_format_2_webgpu(gfx_pixel_format format)
{
    switch (format)
    {
        case gfx_pixel_format_a8:               return WGPUTextureFormat_R8Unorm;
        case gfx_pixel_format_rgba8:            return WGPUTextureFormat_RGBA8Unorm;
        case gfx_pixel_format_rgba8_srgb:       return WGPUTextureFormat_RGBA8UnormSrgb;

        case gfx_pixel_format_etc1:             return WGPUTextureFormat_ETC2RGB8Unorm;
        case gfx_pixel_format_etc2_rgb8a1:      return WGPUTextureFormat_ETC2RGB8A1Unorm;
        case gfx_pixel_format_etc2_rgba8:       return WGPUTextureFormat_ETC2RGBA8Unorm;
        case gfx_pixel_format_etc1_srgb:        return WGPUTextureFormat_ETC2RGB8UnormSrgb;
        case gfx_pixel_format_etc2_rgb8a1_srgb: return WGPUTextureFormat_ETC2RGB8A1UnormSrgb;
        case gfx_pixel_format_etc2_rgba8_srgb:  return WGPUTextureFormat_ETC2RGBA8UnormSrgb;

        case gfx_pixel_format_bc1:              return WGPUTextureFormat_BC1RGBAUnorm;
        case gfx_pixel_format_bc3:              return WGPUTextureFormat_BC3RGBAUnorm;
        case gfx_pixel_format_bc4:              return WGPUTextureFormat_BC4RUnorm;
        case gfx_pixel_format_bc5:              return WGPUTextureFormat_BC5RGUnorm;
        case gfx_pixel_format_bc6h:             return WGPUTextureFormat_BC6HRGBUfloat;    // same as Vulkan/Metal path (unsigned, DXGI 95)
        case gfx_pixel_format_bc7:              return WGPUTextureFormat_BC7RGBAUnorm;
        case gfx_pixel_format_bc1_srgb:         return WGPUTextureFormat_BC1RGBAUnormSrgb;
        case gfx_pixel_format_bc3_srgb:         return WGPUTextureFormat_BC3RGBAUnormSrgb;
        case gfx_pixel_format_bc7_srgb:         return WGPUTextureFormat_BC7RGBAUnormSrgb;

        case gfx_pixel_format_astc4x4:          return WGPUTextureFormat_ASTC4x4Unorm;
        case gfx_pixel_format_astc5x5:          return WGPUTextureFormat_ASTC5x5Unorm;
        case gfx_pixel_format_astc6x6:          return WGPUTextureFormat_ASTC6x6Unorm;
        case gfx_pixel_format_astc8x8:          return WGPUTextureFormat_ASTC8x8Unorm;
        case gfx_pixel_format_astc10x10:        return WGPUTextureFormat_ASTC10x10Unorm;
        case gfx_pixel_format_astc12x12:        return WGPUTextureFormat_ASTC12x12Unorm;
        case gfx_pixel_format_astc4x4_srgb:     return WGPUTextureFormat_ASTC4x4UnormSrgb;
        case gfx_pixel_format_astc5x5_srgb:     return WGPUTextureFormat_ASTC5x5UnormSrgb;
        case gfx_pixel_format_astc6x6_srgb:     return WGPUTextureFormat_ASTC6x6UnormSrgb;
        case gfx_pixel_format_astc8x8_srgb:     return WGPUTextureFormat_ASTC8x8UnormSrgb;
        case gfx_pixel_format_astc10x10_srgb:   return WGPUTextureFormat_ASTC10x10UnormSrgb;
        case gfx_pixel_format_astc12x12_srgb:   return WGPUTextureFormat_ASTC12x12UnormSrgb;

        case gfx_pixel_format_r16f:             return WGPUTextureFormat_R16Float;
        case gfx_pixel_format_rg16f:            return WGPUTextureFormat_RG16Float;
        case gfx_pixel_format_rgba16f:          return WGPUTextureFormat_RGBA16Float;

        case gfx_pixel_format_r32f:             return WGPUTextureFormat_R32Float;
        case gfx_pixel_format_rg32f:            return WGPUTextureFormat_RG32Float;
        case gfx_pixel_format_rgba32f:          return WGPUTextureFormat_RGBA32Float;

        case gfx_pixel_format_d24x8:            return WGPUTextureFormat_Depth24Plus;
        case gfx_pixel_format_d24s8:            return WGPUTextureFormat_Depth24PlusStencil8;
        case gfx_pixel_format_d32:              return WGPUTextureFormat_Depth32Float;
        default:                                return WGPUTextureFormat_Undefined;    // rgba4444/rgb5a1/rgb565 have no WebGPU format
    }
}

static bool wgpu_format_is_depth(WGPUTextureFormat f)
{
    return f == WGPUTextureFormat_Depth16Unorm || f == WGPUTextureFormat_Depth24Plus || f == WGPUTextureFormat_Depth24PlusStencil8 ||
           f == WGPUTextureFormat_Depth32Float || f == WGPUTextureFormat_Depth32FloatStencil8 || f == WGPUTextureFormat_Stencil8;
}

static bool wgpu_format_has_stencil(WGPUTextureFormat f)
{
    return f == WGPUTextureFormat_Depth24PlusStencil8 || f == WGPUTextureFormat_Depth32FloatStencil8 || f == WGPUTextureFormat_Stencil8;
}

static WGPUTextureFormat wgpu_format_srgb(WGPUTextureFormat f)
{
    switch (f) {
        case WGPUTextureFormat_BGRA8Unorm: return WGPUTextureFormat_BGRA8UnormSrgb;
        case WGPUTextureFormat_RGBA8Unorm: return WGPUTextureFormat_RGBA8UnormSrgb;
        default:                           return f;
    }
}

// block width/height of compressed formats, 1 for everything else
static uint32_t gfx_pixel_format_block(gfx_pixel_format f)
{
    switch (gfx_pixel_format_linear(f)) {
        case gfx_pixel_format_etc1: case gfx_pixel_format_etc2_rgb8a1: case gfx_pixel_format_etc2_rgba8:
        case gfx_pixel_format_bc1:  case gfx_pixel_format_bc3: case gfx_pixel_format_bc4: case gfx_pixel_format_bc5:
        case gfx_pixel_format_bc6h: case gfx_pixel_format_bc7: case gfx_pixel_format_astc4x4:   return 4;
        case gfx_pixel_format_astc5x5:                                                          return 5;
        case gfx_pixel_format_astc6x6:                                                          return 6;
        case gfx_pixel_format_astc8x8:                                                          return 8;
        case gfx_pixel_format_astc10x10:                                                        return 10;
        case gfx_pixel_format_astc12x12:                                                        return 12;
        default:                                                                                return 1;
    }
}

static WGPUVertexFormat gfx_vertex_format_2_webgpu(gfx_format format)
{
    switch (format)
    {
        case gfx_format_float1:  return WGPUVertexFormat_Float32;
        case gfx_format_float2:  return WGPUVertexFormat_Float32x2;
        case gfx_format_float4:  return WGPUVertexFormat_Float32x4;
        case gfx_format_int2:    return WGPUVertexFormat_Sint32x2;
        case gfx_format_int4:    return WGPUVertexFormat_Sint32x4;
        case gfx_format_uint2:   return WGPUVertexFormat_Uint32x2;
        case gfx_format_uint4:   return WGPUVertexFormat_Uint32x4;
        case gfx_format_half2:   return WGPUVertexFormat_Float16x2;
        case gfx_format_half4:   return WGPUVertexFormat_Float16x4;
        case gfx_format_short2:  return WGPUVertexFormat_Snorm16x2;     // gfx.h: normalized to [-1, 1]
        case gfx_format_short4:  return WGPUVertexFormat_Snorm16x4;
        case gfx_format_ushort2: return WGPUVertexFormat_Unorm16x2;     // gfx.h: normalized to [0, 1]
        case gfx_format_ushort4: return WGPUVertexFormat_Unorm16x4;
        case gfx_format_byte4:   return WGPUVertexFormat_Unorm8x4;      // gfx.h: normalized to [0, 1] (vertex colors)
    }
    return WGPUVertexFormat_Float32x4;
}

static WGPUFrontFace gfx_face_2_webgpu(gfx_face face)
{
    return face == gfx_face_cw ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
}

static WGPUCullMode gfx_cull_2_webgpu(gfx_cull cull)
{
    switch (cull) {
        case gfx_cull_back:  return WGPUCullMode_Back;
        case gfx_cull_front: return WGPUCullMode_Front;
        default:             return WGPUCullMode_None;
    }
}

static WGPUAddressMode gfx_address_mode_2_webgpu(gfx_address_mode mode)
{
    switch (mode) {
        case gfx_address_mode_mirror_repeat:    return WGPUAddressMode_MirrorRepeat;
        case gfx_address_mode_clamp_to_edge:    return WGPUAddressMode_ClampToEdge;
        default:                                return WGPUAddressMode_Repeat;
    }
}

static WGPUFilterMode gfx_filter_2_webgpu(gfx_filter mode)
{
    return mode == gfx_filter_linear ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
}

static WGPUMipmapFilterMode gfx_mipmap_filter_2_webgpu(gfx_filter mode)
{
    return mode == gfx_filter_linear ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
}

static WGPUCompareFunction gfx_cmp_2_webgpu(gfx_cmp mode)
{
    switch(mode)
    {
        case gfx_cmp_never:     return WGPUCompareFunction_Never;
        case gfx_cmp_less:      return WGPUCompareFunction_Less;
        case gfx_cmp_equal:     return WGPUCompareFunction_Equal;
        case gfx_cmp_lequal:    return WGPUCompareFunction_LessEqual;
        case gfx_cmp_greater:   return WGPUCompareFunction_Greater;
        case gfx_cmp_not_equal: return WGPUCompareFunction_NotEqual;
        case gfx_cmp_gequal:    return WGPUCompareFunction_GreaterEqual;
        case gfx_cmp_always:    return WGPUCompareFunction_Always;
        default:                return WGPUCompareFunction_LessEqual;
    }
}

static WGPUStencilOperation gfx_op_2_webgpu(gfx_stencil_op op)
{
    switch (op) {
        case gfx_stencil_op_zero:        return WGPUStencilOperation_Zero;
        case gfx_stencil_op_keep:        return WGPUStencilOperation_Keep;
        case gfx_stencil_op_replace:     return WGPUStencilOperation_Replace;
        case gfx_stencil_op_incr:        return WGPUStencilOperation_IncrementClamp;
        case gfx_stencil_op_incr_wrap:   return WGPUStencilOperation_IncrementWrap;
        case gfx_stencil_op_decr:        return WGPUStencilOperation_DecrementClamp;
        case gfx_stencil_op_decr_wrap:   return WGPUStencilOperation_DecrementWrap;
        case gfx_stencil_op_invert:      return WGPUStencilOperation_Invert;
    }
    return WGPUStencilOperation_Keep;
}

static WGPUBlendFactor gfx_blend_mode_2_webgpu(gfx_blend_mode mode)
{
    switch(mode)
    {
        case gfx_blend_mode_zero:           return WGPUBlendFactor_Zero;
        case gfx_blend_mode_one:            return WGPUBlendFactor_One;
        case gfx_blend_mode_src_color:      return WGPUBlendFactor_Src;
        case gfx_blend_mode_inv_src_color:  return WGPUBlendFactor_OneMinusSrc;
        case gfx_blend_mode_src_alpha:      return WGPUBlendFactor_SrcAlpha;
        case gfx_blend_mode_inv_src_alpha:  return WGPUBlendFactor_OneMinusSrcAlpha;
        case gfx_blend_mode_dst_alpha:      return WGPUBlendFactor_DstAlpha;
        case gfx_blend_mode_inv_dest_alpha: return WGPUBlendFactor_OneMinusDstAlpha;
        case gfx_blend_mode_dst_color:      return WGPUBlendFactor_Dst;
        case gfx_blend_mode_inv_dst_color:  return WGPUBlendFactor_OneMinusDst;
    }
    return WGPUBlendFactor_One;
}

static WGPUBlendOperation gfx_blend_op_2_webgpu(gfx_blend_op op)
{
    switch(op)
    {
        case gfx_blend_op_add:          return WGPUBlendOperation_Add;
        case gfx_blend_op_min:          return WGPUBlendOperation_Min;
        case gfx_blend_op_max:          return WGPUBlendOperation_Max;
        case gfx_blend_op_subtract:     return WGPUBlendOperation_Subtract;
        case gfx_blend_op_rev_subtract: return WGPUBlendOperation_ReverseSubtract;
    }
    return WGPUBlendOperation_Add;
}

static const char* wgpu_backend_name(WGPUBackendType type)
{
    switch (type) {
        case WGPUBackendType_Null:      return "Null";
        case WGPUBackendType_WebGPU:    return "WebGPU";
        case WGPUBackendType_D3D11:     return "D3D11";
        case WGPUBackendType_D3D12:     return "D3D12";
        case WGPUBackendType_Metal:     return "Metal";
        case WGPUBackendType_Vulkan:    return "Vulkan";
        case WGPUBackendType_OpenGL:    return "OpenGL";
        case WGPUBackendType_OpenGLES:  return "OpenGLES";
        default:                        return "unknown";
    }
}

static const char* wgpu_adapter_type_name(WGPUAdapterType type)
{
    switch (type) {
        case WGPUAdapterType_DiscreteGPU:   return "discrete GPU";
        case WGPUAdapterType_IntegratedGPU: return "integrated GPU";
        case WGPUAdapterType_CPU:           return "CPU";
        default:                            return "unknown";
    }
}

static void get_window_size(intptr_t handle, uint32_t* width, uint32_t* height)
{
    *width = *height = 0;
#ifdef __EMSCRIPTEN__
    int w = 0, h = 0;
    emscripten_get_canvas_element_size(handle ? (const char*)handle : "#canvas", &w, &h);
    *width = (uint32_t)(w > 0 ? w : 0);
    *height = (uint32_t)(h > 0 ? h : 0);
#elif defined(_WIN32)
    RECT rect = {};
    if (GetClientRect((HWND)handle, &rect)) {
        *width = (uint32_t)(rect.right - rect.left);
        *height = (uint32_t)(rect.bottom - rect.top);
    }
#else
    (void)handle;
#endif
}

static wgpu_texture_t* wgpu_texture_alloc(WGPUTexture texture, WGPUTextureView view, WGPUTextureFormat format, uint32_t w, uint32_t h, uint32_t samples)
{
    wgpu_texture_t* t = (wgpu_texture_t*)calloc(1, sizeof(wgpu_texture_t));
    if (!t) return nullptr;
    t->texture = texture;
    t->texture_view = view;
    t->format = format;
    t->width = w;
    t->height = h;
    t->sample_count = samples;
    return t;
}

static void create_default_resources(gfx_context_t* ctx)
{
    wgpu_context_t* wctx = from_ctx(ctx);

    const uint32_t white = 0xFFFFFFFFu;
    gfx_texture_desc_t desc = {};
        desc.label      = "default";
        desc.width      = 1;
        desc.height     = 1;
        desc.depth      = 1;
        desc.format     = gfx_pixel_format_rgba8;
        desc.data       = (void*)&white;
        desc.mip_levels = 1;
    wgpu_create_texture(ctx, &desc, &wctx->default_texture);

    gfx_sampler_desc_t sampler_desc = {};
        sampler_desc.minmag     = gfx_filter_point;
        sampler_desc.mipmap     = gfx_filter_point;
        sampler_desc.mode       = gfx_address_mode_repeat;
        sampler_desc.anisotropy = 1;
    wgpu_create_sampler(ctx, &sampler_desc, &wctx->default_sampler);
}

// ============================================================================
// CONTEXT
// ============================================================================

void wgpu_get_caps(gfx_context_t* ctx, gfx_caps_t* caps)
{
    if (!caps) return;
    memset(caps, 0, sizeof(*caps));
    wgpu_context_t* wctx = from_ctx(ctx);

    // browsers accept WGSL only; native Dawn would also take SPIR-V, but one path for both keeps them identical
    caps->supported_shader_formats              = gfx_shader_format_wgsl;
    caps->support_compute                       = true;
    caps->support_indirect                      = true;
    if (wctx) {
        caps->max_texture_dimension_2d              = wctx->limits.maxTextureDimension2D;
        caps->max_compute_work_group_invocations    = wctx->limits.maxComputeInvocationsPerWorkgroup;
        caps->min_uniform_buffer_offset_alignment   = wctx->limits.minUniformBufferOffsetAlignment;
        caps->min_storage_buffer_offset_alignment   = wctx->limits.minStorageBufferOffsetAlignment;
        caps->max_uniform_buffer_range              = (uint32_t)gfx_min(wctx->limits.maxUniformBufferBindingSize, (uint64_t)0xFFFFFFFFu);
    }
}

// Dawn's backend on desktop: automatic, or GFX_WEBGPU_BACKEND=d3d12 | d3d11 | vulkan | metal | opengl | opengles
static WGPUBackendType wgpu_requested_backend()
{
#ifndef __EMSCRIPTEN__
    const char* env = getenv("GFX_WEBGPU_BACKEND");
    if (env) {
        if (!strcmp(env, "d3d12"))    return WGPUBackendType_D3D12;
        if (!strcmp(env, "d3d11"))    return WGPUBackendType_D3D11;
        if (!strcmp(env, "vulkan"))   return WGPUBackendType_Vulkan;
        if (!strcmp(env, "metal"))    return WGPUBackendType_Metal;
        if (!strcmp(env, "opengl"))   return WGPUBackendType_OpenGL;
        if (!strcmp(env, "opengles")) return WGPUBackendType_OpenGLES;
    }
#endif
    return WGPUBackendType_Undefined;
}

void wgpu_init(gfx_settings_t* settings, gfx_context_t** ctx)
{
    *ctx = nullptr;
    wgpu_context_t* wctx = (wgpu_context_t*)calloc(1, sizeof(wgpu_context_t));
    if (!wctx)
        return;
    wctx->dbglog = settings->dbglog ? settings->dbglog : default_log;

    // --- instance: TimedWaitAny lets the init wait for the adapter/device futures
    WGPUInstanceFeatureName instance_features[] = { WGPUInstanceFeatureName_TimedWaitAny };
    WGPUInstanceDescriptor instance_desc = {};
        instance_desc.requiredFeatureCount = 1;
        instance_desc.requiredFeatures     = instance_features;
    wctx->instance = wgpuCreateInstance(&instance_desc);
    if (!wctx->instance) {
        wctx->dbglog(gfx_msg_error, "webgpu: wgpuCreateInstance failed");
        free(wctx);
        return;
    }

    // --- adapter
    struct adapter_result_t { WGPUAdapter adapter; WGPURequestAdapterStatus status; char message[256]; } ar = {};
    WGPURequestAdapterOptions adapter_options = {};
        adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;
        adapter_options.backendType     = wgpu_requested_backend();
    WGPURequestAdapterCallbackInfo adapter_cb = {};
        adapter_cb.mode      = WGPUCallbackMode_WaitAnyOnly;
        adapter_cb.userdata1 = &ar;
        adapter_cb.callback  = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* u1, void*) {
            adapter_result_t* r = (adapter_result_t*)u1;
            r->status = status;
            r->adapter = adapter;
            snprintf(r->message, sizeof(r->message), "%.*s", wgpu_str_len(message), message.data ? message.data : "");
        };
    if (!wgpu_wait(wctx->instance, wgpuInstanceRequestAdapter(wctx->instance, &adapter_options, adapter_cb)) || !ar.adapter) {
        wctx->dbglog(gfx_msg_error, "webgpu: no adapter (%d) %s", (int)ar.status, ar.message);
        wgpuInstanceRelease(wctx->instance);
        free(wctx);
        return;
    }
    wctx->adapter = ar.adapter;

    // --- features: the useful optional ones the adapter has
    WGPUSupportedFeatures supported = {};
    wgpuAdapterGetFeatures(wctx->adapter, &supported);
    const WGPUFeatureName wanted[] = {
        WGPUFeatureName_Float32Filterable,          // RGBA32F sampled with a filtering sampler (saltline slug curves)
        WGPUFeatureName_TextureCompressionBC,
        WGPUFeatureName_TextureCompressionETC2,
        WGPUFeatureName_TextureCompressionASTC,
        WGPUFeatureName_IndirectFirstInstance,
        WGPUFeatureName_ShaderF16,
        WGPUFeatureName_Depth32FloatStencil8,
        WGPUFeatureName_RG11B10UfloatRenderable,
        WGPUFeatureName_BGRA8UnormStorage,
    };
    WGPUFeatureName features[16] = {};
    uint32_t feature_count = 0;
    for (WGPUFeatureName f : wanted)
        for (size_t i = 0; i < supported.featureCount; ++i)
            if (supported.features[i] == f) { features[feature_count++] = f; break; }
    wgpuSupportedFeaturesFreeMembers(supported);
    for (uint32_t i = 0; i < feature_count; ++i)
        wctx->float32_filterable = wctx->float32_filterable || features[i] == WGPUFeatureName_Float32Filterable;

    // --- device: the adapter's limits (defaults are the WebGPU minimums), errors go to dbglog
    WGPULimits adapter_limits = {};
    wgpuAdapterGetLimits(wctx->adapter, &adapter_limits);

    struct device_result_t { WGPUDevice device; WGPURequestDeviceStatus status; char message[256]; } dr = {};
    WGPUDeviceDescriptor device_desc = {};
        device_desc.label                = wgpu_str("gfx device");
        device_desc.requiredFeatureCount = feature_count;
        device_desc.requiredFeatures     = features;
        device_desc.requiredLimits       = &adapter_limits;
        device_desc.uncapturedErrorCallbackInfo.userdata1 = wctx;
        device_desc.uncapturedErrorCallbackInfo.callback  = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void* u1, void*) {
            wgpu_context_t* c = (wgpu_context_t*)u1;
            c->dbglog(gfx_msg_error, "webgpu error (%d): %.*s", (int)type, wgpu_str_len(message), message.data ? message.data : "");
        };
        device_desc.deviceLostCallbackInfo.mode      = WGPUCallbackMode_AllowSpontaneous;
        device_desc.deviceLostCallbackInfo.userdata1 = wctx;
        device_desc.deviceLostCallbackInfo.callback  = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void* u1, void*) {
            if (reason == WGPUDeviceLostReason_Destroyed || reason == WGPUDeviceLostReason_CallbackCancelled) return;
            wgpu_context_t* c = (wgpu_context_t*)u1;
            c->dbglog(gfx_msg_error, "webgpu device lost (%d): %.*s", (int)reason, wgpu_str_len(message), message.data ? message.data : "");
        };
    WGPURequestDeviceCallbackInfo device_cb = {};
        device_cb.mode      = WGPUCallbackMode_WaitAnyOnly;
        device_cb.userdata1 = &dr;
        device_cb.callback  = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* u1, void*) {
            device_result_t* r = (device_result_t*)u1;
            r->status = status;
            r->device = device;
            snprintf(r->message, sizeof(r->message), "%.*s", wgpu_str_len(message), message.data ? message.data : "");
        };
    if (!wgpu_wait(wctx->instance, wgpuAdapterRequestDevice(wctx->adapter, &device_desc, device_cb)) || !dr.device) {
        wctx->dbglog(gfx_msg_error, "webgpu: no device (%d) %s", (int)dr.status, dr.message);
        wgpuAdapterRelease(wctx->adapter);
        wgpuInstanceRelease(wctx->instance);
        free(wctx);
        return;
    }
    wctx->device = dr.device;
    wctx->queue  = wgpuDeviceGetQueue(wctx->device);
    wgpuDeviceGetLimits(wctx->device, &wctx->limits);

#ifndef __EMSCRIPTEN__
    WGPULoggingCallbackInfo logging = {};
        logging.userdata1 = wctx;
        logging.callback  = [](WGPULoggingType type, WGPUStringView message, void* u1, void*) {
            wgpu_context_t* c = (wgpu_context_t*)u1;
            const gfx_msg level = type == WGPULoggingType_Error ? gfx_msg_error : type == WGPULoggingType_Warning ? gfx_msg_warning : gfx_msg_info;
            c->dbglog(level, "webgpu: %.*s", wgpu_str_len(message), message.data ? message.data : "");
        };
    wgpuDeviceSetLoggingCallback(wctx->device, logging);
#endif

    WGPUAdapterInfo info = {};
    wgpuAdapterGetInfo(wctx->adapter, &info);
    wctx->backend_type = info.backendType;
    wctx->adapter_type = info.adapterType;
    wctx->dbglog(gfx_msg_info, "gfx init with webgpu(%s): %.*s (%s), float32-filterable %d",
                 wgpu_backend_name(info.backendType), wgpu_str_len(info.device), info.device.data ? info.device.data : "",
                 wgpu_adapter_type_name(info.adapterType), (int)wctx->float32_filterable);
    wgpuAdapterInfoFreeMembers(info);

    // until a surface says otherwise
    wctx->surface_format = WGPUTextureFormat_BGRA8Unorm;
    wctx->depth_format   = WGPUTextureFormat_Depth24PlusStencil8;
    wctx->sample_count   = 1;

    gfx_handle_pool_create(sizeof(wgpu_buffer_t),  4096, &wctx->buffer_pool,  nullptr);
    gfx_handle_pool_create(sizeof(wgpu_surface_t), 8,    &wctx->surface_pool, nullptr);

    *ctx = &wctx->handle;
    create_default_resources(*ctx);
}

static void wgpu_release_pool(wgpu_context_t* wctx, wgpu_descriptor_set_pool_t* pool);

void wgpu_destroy(gfx_context_t* ctx)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    if (!wctx) return;

    while (wctx->pools)
        wgpu_release_pool(wctx, wctx->pools);

    if (wctx->default_texture) wgpu_destroy_texture(ctx, wctx->default_texture);
    if (wctx->default_sampler) wgpu_destroy_sampler(ctx, wctx->default_sampler);
    free(wctx->frame);

    if (wctx->queue)    wgpuQueueRelease(wctx->queue);
    if (wctx->device) { wgpuDeviceDestroy(wctx->device); wgpuDeviceRelease(wctx->device); }
    if (wctx->adapter)  wgpuAdapterRelease(wctx->adapter);
    if (wctx->instance) wgpuInstanceRelease(wctx->instance);

    gfx_handle_pool_destroy(wctx->buffer_pool);
    gfx_handle_pool_destroy(wctx->surface_pool);
    free(wctx);
}

// ============================================================================
// SURFACE / FRAME
// ============================================================================

static wgpu_surface_t* wgpu_map_surface(wgpu_context_t* wctx, gfx_surface_t* surface)
{
    return (wctx && surface) ? (wgpu_surface_t*)gfx_handle_pool_map(wctx->surface_pool, surface->idx) : nullptr;
}

static void wgpu_surface_release_targets(wgpu_surface_t* s)
{
    if (s->msaa_color_view) { wgpuTextureViewRelease(s->msaa_color_view); s->msaa_color_view = nullptr; }
    if (s->msaa_color)      { wgpuTextureRelease(s->msaa_color);          s->msaa_color = nullptr; }
    if (s->depth_view)      { wgpuTextureViewRelease(s->depth_view);      s->depth_view = nullptr; }
    if (s->depth)           { wgpuTextureRelease(s->depth);               s->depth = nullptr; }
}

static WGPUPresentMode wgpu_surface_present_mode(const wgpu_surface_t* s)
{
    if (s->vsync) return WGPUPresentMode_Fifo;      // the one mode every implementation has
    for (uint32_t i = 0; i < s->present_mode_count; ++i) if (s->present_modes[i] == WGPUPresentMode_Mailbox)   return WGPUPresentMode_Mailbox;
    for (uint32_t i = 0; i < s->present_mode_count; ++i) if (s->present_modes[i] == WGPUPresentMode_Immediate) return WGPUPresentMode_Immediate;
    return WGPUPresentMode_Fifo;
}

// (re)configure for the current window size: back buffer, MSAA color and depth
static bool wgpu_surface_configure(wgpu_surface_t* s, uint32_t width, uint32_t height)
{
    wgpu_context_t* wctx = s->ctx;
    wgpu_surface_release_targets(s);

    s->config.width       = width;
    s->config.height      = height;
    s->config.presentMode = wgpu_surface_present_mode(s);
    wgpuSurfaceConfigure(s->surface, &s->config);
    s->needs_configure = false;

    WGPUTextureDescriptor td = {};
        td.dimension     = WGPUTextureDimension_2D;
        td.size          = { width, height, 1 };
        td.mipLevelCount = 1;
        td.sampleCount   = s->sample_count;
        td.usage         = WGPUTextureUsage_RenderAttachment;

    if (s->sample_count > 1) {
        td.label  = wgpu_str("surface msaa color");
        td.format = s->view_format;
        s->msaa_color = wgpuDeviceCreateTexture(wctx->device, &td);
        s->msaa_color_view = wgpuTextureCreateView(s->msaa_color, nullptr);
    }

    td.label  = wgpu_str("surface depth");
    td.format = wctx->depth_format;
    s->depth = wgpuDeviceCreateTexture(wctx->device, &td);
    s->depth_view = wgpuTextureCreateView(s->depth, nullptr);
    return s->depth_view != nullptr;
}

void wgpu_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_surface = nullptr;
    if (!wctx || !desc) return;

    WGPUSurfaceDescriptor surface_desc = {};
    surface_desc.label = wgpu_str(desc->label);
#ifdef __EMSCRIPTEN__
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector source = {};
        source.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
        source.selector    = wgpu_str(desc->window_handle ? (const char*)desc->window_handle : "#canvas");
    surface_desc.nextInChain = &source.chain;
#elif defined(_WIN32)
    WGPUSurfaceSourceWindowsHWND source = {};
        source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
        source.hinstance   = GetModuleHandle(nullptr);
        source.hwnd        = (void*)desc->window_handle;
    surface_desc.nextInChain = &source.chain;
#else
    wctx->dbglog(gfx_msg_error, "wgpu_surface_create: no surface source for this platform");
    return;
#endif

    WGPUSurface surface = wgpuInstanceCreateSurface(wctx->instance, &surface_desc);
    if (!surface) {
        wctx->dbglog(gfx_msg_error, "wgpu_surface_create: wgpuInstanceCreateSurface failed");
        return;
    }

    uint64_t handle = 0;
    wgpu_surface_t* s = (wgpu_surface_t*)gfx_handle_pool_allocate_data(wctx->surface_pool, &handle);
    if (!s) {
        wgpuSurfaceRelease(surface);
        return;
    }
    memset(s, 0, sizeof(*s));
    s->handle        = { handle };
    s->ctx           = wctx;
    s->window_handle = desc->window_handle;
    s->surface       = surface;
    s->vsync         = desc->vsync;

    // WebGPU core has 1x and 4x MSAA only
    const uint32_t wanted_samples = desc->sample_count ? (uint32_t)desc->sample_count : 1u;
    s->sample_count = wanted_samples >= 4 ? 4u : 1u;
    if (s->sample_count != wanted_samples)
        wctx->dbglog(gfx_msg_warning, "wgpu_surface_create: %ux MSAA is not available in WebGPU, using %ux", wanted_samples, s->sample_count);

    WGPUSurfaceCapabilities caps = {};
    wgpuSurfaceGetCapabilities(surface, wctx->adapter, &caps);

    // an 8-bit UNORM format the surface has; sRGB requests get the sRGB view of it (encoding done by the hardware)
    WGPUTextureFormat format = caps.formatCount ? caps.formats[0] : WGPUTextureFormat_BGRA8Unorm;
    for (size_t i = 0; i < caps.formatCount; ++i)
        if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm || caps.formats[i] == WGPUTextureFormat_RGBA8Unorm) { format = caps.formats[i]; break; }
    const bool srgb = gfx_pixel_format_is_srgb(desc->preferred_format ? desc->preferred_format : gfx_pixel_format_rgba8);
    s->view_format = srgb ? wgpu_format_srgb(format) : format;

    WGPUCompositeAlphaMode alpha = caps.alphaModeCount ? caps.alphaModes[0] : WGPUCompositeAlphaMode_Auto;
    for (size_t i = 0; i < caps.alphaModeCount; ++i)
        if (caps.alphaModes[i] == WGPUCompositeAlphaMode_Opaque) { alpha = WGPUCompositeAlphaMode_Opaque; break; }

    s->present_mode_count = (uint32_t)gfx_min(caps.presentModeCount, (size_t)8);
    for (uint32_t i = 0; i < s->present_mode_count; ++i) s->present_modes[i] = caps.presentModes[i];
    wgpuSurfaceCapabilitiesFreeMembers(caps);

    s->config.device          = wctx->device;
    s->config.format          = format;
    s->config.usage           = WGPUTextureUsage_RenderAttachment;
    s->config.alphaMode       = alpha;
    s->config.viewFormatCount = s->view_format != format ? 1 : 0;
    s->config.viewFormats     = &s->view_format;

    // pipelines are built for the last created surface (as the Vulkan backend's default render pass)
    wctx->surface_format = s->view_format;
    wctx->sample_count   = s->sample_count;

    uint32_t w = 0, h = 0;
    get_window_size(s->window_handle, &w, &h);
    if (w && h) wgpu_surface_configure(s, w, h);
    else        s->needs_configure = true;

    GFX_VERBOSE(wctx->dbglog(gfx_msg_info, "wgpu_surface_create %ux%u format %d (view %d) %ux", w, h, (int)format, (int)s->view_format, s->sample_count));
    *out_surface = &s->handle;
}

void wgpu_surface_destroy(gfx_context_t* ctx, gfx_surface_t* surface)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_surface_t* s = wgpu_map_surface(wctx, surface);
    if (!s) return;
    wgpu_surface_release_targets(s);
    if (s->surface) {
        wgpuSurfaceUnconfigure(s->surface);
        wgpuSurfaceRelease(s->surface);
    }
    gfx_handle_pool_free(wctx->surface_pool, surface->idx);
}

gfx_result wgpu_surface_set_vsync(gfx_context_t* ctx, gfx_surface_t* surface, bool vsync)
{
    wgpu_surface_t* s = wgpu_map_surface(from_ctx(ctx), surface);
    if (!s) return gfx_error_invalid_handle;
    if (s->vsync != vsync) {
        s->vsync = vsync;
        s->needs_configure = true;      // applied by the next frame_begin
    }
    return gfx_ok;
}

void wgpu_frame_begin(gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_frame = nullptr;
    wgpu_surface_t* s = wgpu_map_surface(wctx, surface);
    if (!s) return;

    uint32_t w = 0, h = 0;
    get_window_size(s->window_handle, &w, &h);
    if (w == 0 || h == 0)
        return;                         // minimized
    if (s->needs_configure || w != s->config.width || h != s->config.height)
        wgpu_surface_configure(s, w, h);

    WGPUSurfaceTexture st = {};
    wgpuSurfaceGetCurrentTexture(s->surface, &st);
    switch (st.status) {
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
            break;
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
            s->needs_configure = true;  // usable now, rebuilt next frame
            break;
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        case WGPUSurfaceGetCurrentTextureStatus_Lost:
            if (st.texture) wgpuTextureRelease(st.texture);
            s->needs_configure = true;
            return;
        default:
            if (st.texture) wgpuTextureRelease(st.texture);
            wctx->dbglog(gfx_msg_error, "wgpu_frame_begin: wgpuSurfaceGetCurrentTexture failed (%d)", (int)st.status);
            return;
    }

    WGPUTextureViewDescriptor vd = {};
        vd.format          = s->view_format;
        vd.dimension       = WGPUTextureViewDimension_2D;
        vd.mipLevelCount   = 1;
        vd.arrayLayerCount = 1;
        vd.aspect          = WGPUTextureAspect_All;
    WGPUTextureView view = wgpuTextureCreateView(st.texture, &vd);

    if (!wctx->frame)
        wctx->frame = (wgpu_frame_t*)calloc(1, sizeof(wgpu_frame_t));
    wgpu_frame_t* f = wctx->frame;
    const uint32_t frame_index = f->public_frame.frame_index;
    memset(f, 0, sizeof(*f));

    f->surface          = s;
    f->back_buffer      = st.texture;
    f->back_buffer_view = view;

    f->target.color   = s->sample_count > 1 ? s->msaa_color_view : view;
    f->target.resolve = s->sample_count > 1 ? view : nullptr;
    f->target.depth   = s->depth_view;
    f->target.width   = w;
    f->target.height  = h;

    f->color_view.texture      = st.texture;
    f->color_view.texture_view = view;
    f->color_view.format       = s->view_format;
    f->color_view.width        = w;
    f->color_view.height       = h;
    f->color_view.sample_count = 1;
    f->color_view.external     = true;

    f->depth_view.texture      = s->depth;
    f->depth_view.texture_view = s->depth_view;
    f->depth_view.format       = wctx->depth_format;
    f->depth_view.width        = w;
    f->depth_view.height       = h;
    f->depth_view.sample_count = s->sample_count;
    f->depth_view.external     = true;

    f->cmd.ctx            = wctx;
    f->cmd.default_target = &f->target;
    WGPUCommandEncoderDescriptor ed = {};
        ed.label = wgpu_str("frame");
    f->cmd.encoder = wgpuDeviceCreateCommandEncoder(wctx->device, &ed);

    f->public_frame.ctx                   = ctx;
    f->public_frame.frame_index           = frame_index + 1;
    f->public_frame.swapchain_image_index = 0;
    f->public_frame.surface               = surface;
    f->public_frame.target                = &f->target.handle;
    f->public_frame.color_attachment      = &f->color_view.handle;
    f->public_frame.depth_attachment      = &f->depth_view.handle;
    f->public_frame.cmd                   = &f->cmd.handle;

    *out_frame = &f->public_frame;
}

// UBO slices written by the CPU this frame -> GPU, before the frame's commands are submitted
static void wgpu_flush_uniforms(wgpu_context_t* wctx)
{
    for (wgpu_descriptor_set_pool_t* pool = wctx->pools; pool; pool = pool->ctx_next) {
        if (!pool->dirty || !pool->ubo_buffer) continue;
        wgpu_buffer_t* buffer = (wgpu_buffer_t*)pool->ubo_buffer;
        wgpuQueueWriteBuffer(wctx->queue, buffer->buffer, 0, pool->ubo_buffer_data_ptr, pool->ubo_buffer_data_size);
        pool->dirty = false;
    }
}

void wgpu_frame_end(gfx_frame_t* frame)
{
    if (!frame) return;
    wgpu_frame_t* f = (wgpu_frame_t*)frame;
    wgpu_context_t* wctx = f->cmd.ctx;

    if (f->cmd.pass)         { wgpuRenderPassEncoderEnd(f->cmd.pass);   wgpuRenderPassEncoderRelease(f->cmd.pass);   f->cmd.pass = nullptr; }
    if (f->cmd.compute_pass) { wgpuComputePassEncoderEnd(f->cmd.compute_pass); wgpuComputePassEncoderRelease(f->cmd.compute_pass); f->cmd.compute_pass = nullptr; }

    wgpu_flush_uniforms(wctx);

    if (f->cmd.encoder) {
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(f->cmd.encoder, nullptr);
        wgpuQueueSubmit(wctx->queue, 1, &commands);
        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(f->cmd.encoder);
        f->cmd.encoder = nullptr;
    }

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(f->surface->surface);    // the browser presents on its own when the frame callback returns
#endif

    if (f->back_buffer_view) wgpuTextureViewRelease(f->back_buffer_view);
    if (f->back_buffer)      wgpuTextureRelease(f->back_buffer);
    f->back_buffer_view = nullptr;
    f->back_buffer = nullptr;

    wgpuInstanceProcessEvents(wctx->instance);
}

// ============================================================================
// BUFFER
// ============================================================================

void wgpu_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_buffer = nullptr;

    const uint32_t size = gfx_utils_align_up(gfx_max(desc->size, 4u), 4);

    WGPUBufferUsage usage = WGPUBufferUsage_None;
    switch(desc->usage) {
        case gfx_buffer_usage_staging:   usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc; break;
        case gfx_buffer_usage_index:     usage = WGPUBufferUsage_Index    | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_vertex:    usage = WGPUBufferUsage_Vertex   | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_uniform:   usage = WGPUBufferUsage_Uniform  | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_storage:   usage = WGPUBufferUsage_Storage  | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc; break;
        case gfx_buffer_usage_indirect:  usage = WGPUBufferUsage_Indirect | WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst; break;
        default:                         usage = WGPUBufferUsage_CopyDst; break;
    }

    WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.label = wgpu_str(desc->label ? desc->label : gfx_to_string(desc->usage));
        buffer_desc.usage = usage;
        buffer_desc.size  = size;
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(wctx->device, &buffer_desc);
    if (!buffer) return;

    uint64_t handle = 0;
    wgpu_buffer_t* wbuffer = (wgpu_buffer_t*)gfx_handle_pool_allocate_data(wctx->buffer_pool, &handle);
    if (!wbuffer) {
        wgpuBufferRelease(buffer);
        return;
    }
    wbuffer->handle = { handle };
    wbuffer->buffer = buffer;
    wbuffer->size   = size;
    *out_buffer = &wbuffer->handle;

    if (desc->data != nullptr && desc->size > 0 && desc->usage != gfx_buffer_usage_staging)
        wgpu_update_buffer_data(ctx, &wbuffer->handle, desc->data, desc->size, 0);
}

// contract: works for any buffer, data only has to live during the call (wgpuQueueWriteBuffer copies it)
void wgpu_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* dst_buffer, void* data, uint32_t size, uint32_t offset)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_buffer_t* dst = dst_buffer ? (wgpu_buffer_t*)gfx_handle_pool_map(wctx->buffer_pool, dst_buffer->idx) : nullptr;
    if (!dst || !data || size == 0) return;

    if (offset >= dst->size) return;
    size = gfx_min(size, dst->size - offset);
    if ((size & 3) == 0) {
        wgpuQueueWriteBuffer(wctx->queue, dst->buffer, offset, data, size);
        return;
    }
    // WebGPU wants a multiple of 4: copy through a padded temporary (the tail bytes keep their value only up to the padding)
    const uint32_t padded = gfx_min(gfx_utils_align_up(size, 4), dst->size - offset);
    uint8_t* tmp = (uint8_t*)calloc(1, padded);
    if (!tmp) return;
    memcpy(tmp, data, size);
    wgpuQueueWriteBuffer(wctx->queue, dst->buffer, offset, tmp, padded);
    free(tmp);
}

void wgpu_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_buffer_t* wbuffer = buffer ? (wgpu_buffer_t*)gfx_handle_pool_map(wctx->buffer_pool, buffer->idx) : nullptr;
    if (!wbuffer) return;
    if (wbuffer->buffer) wgpuBufferRelease(wbuffer->buffer);
    gfx_handle_pool_free(wctx->buffer_pool, buffer->idx);
}

// ============================================================================
// SHADER
// ============================================================================

// desc->uniforms come from the shader package reflection (Slang layout + per stage usage): one bind group layout entry
// per uniform, in the same order, so uniforms[i] <-> bindings[i] (the descriptor set code relies on it).
void wgpu_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_shader = nullptr;
    const char* label = desc->label ? desc->label : "";

    if (desc->uniform_count > WGPU_MAX_BINDINGS) {
        wctx->dbglog(gfx_msg_error, "wgpu_create_shader %s: %u uniforms, the backend supports %u per shader", label, desc->uniform_count, WGPU_MAX_BINDINGS);
        return;
    }

    wgpu_shader_t* shader = (wgpu_shader_t*)calloc(1, sizeof(wgpu_shader_t));
    if (!shader) return;
    shader->context = wctx;

    // --- modules: WGSL keeps all entry points in one text, stages pointing to the same data share one module
    for (uint32_t i = 0; i < desc->stage_count; ++i)
    {
        const gfx_shader_stage_data* st = &desc->stages[i];
        if (st->data == nullptr || st->size == 0)
            continue;
        if ((uint32_t)st->stage >= 3) {
            wctx->dbglog(gfx_msg_error, "wgpu_create_shader %s: stage %u is not supported", label, (uint32_t)st->stage);
            continue;
        }

        WGPUShaderModule module = nullptr;
        for (uint32_t j = 0; j < i && !module; ++j)
            if (desc->stages[j].data == st->data && (uint32_t)desc->stages[j].stage < 3)
                module = shader->modules[desc->stages[j].stage];

        if (!module) {
            const bool is_spirv = st->size >= 4 && *(const uint32_t*)st->data == 0x07230203;

            WGPUShaderSourceWGSL wgsl = {};
                wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
                wgsl.code        = { (const char*)st->data, strnlen((const char*)st->data, st->size) };
            WGPUShaderSourceSPIRV spirv = {};
                spirv.chain.sType = WGPUSType_ShaderSourceSPIRV;
                spirv.codeSize    = st->size / 4;
                spirv.code        = (const uint32_t*)st->data;

            WGPUShaderModuleDescriptor module_desc = {};
                module_desc.label       = wgpu_str(desc->label);
                module_desc.nextInChain = is_spirv ? &spirv.chain : &wgsl.chain;
            module = wgpuDeviceCreateShaderModule(wctx->device, &module_desc);
        }

        shader->modules[st->stage] = module;
        strncpy(shader->entry_names[st->stage], (st->entry && st->entry[0]) ? st->entry : "main", sizeof(shader->entry_names[0]) - 1);
        shader->flags |= 1u << st->stage;
    }

    // --- bind group layout from the reflection
    uint32_t count = 0;
    for (uint32_t u = 0; u < desc->uniform_count; ++u)
    {
        const gfx_uniform_t* uniform = &desc->uniforms[u];
        if (uniform->group != 0)
            wctx->dbglog(gfx_msg_warning, "wgpu_create_shader %s: '%s' is in group %u, only group 0 is bound", label, uniform->name, uniform->group);

        WGPUShaderStage visibility = WGPUShaderStage_None;
        if (uniform->stage_mask & (1u << gfx_shader_vertex))   visibility |= WGPUShaderStage_Vertex;
        if (uniform->stage_mask & (1u << gfx_shader_fragment)) visibility |= WGPUShaderStage_Fragment;
        if (uniform->stage_mask & (1u << gfx_shader_compute))  visibility |= WGPUShaderStage_Compute;

        WGPUBindGroupLayoutEntry* e = &shader->bindings[count];
        memset(e, 0, sizeof(*e));
        e->binding    = uniform->binding;
        e->visibility = visibility;

        // sampleType Float covers every filterable format; 32-bit float textures are filterable with float32-filterable
        switch (uniform->type)
        {
            case gfx_uniform_ubo:
                e->buffer.type = WGPUBufferBindingType_Uniform;
                break;
            case gfx_uniform_storage_buffer:
                e->buffer.type = uniform->storage.access == gfx_access_read ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;
                break;
            case gfx_uniform_sampler:
                e->sampler.type = WGPUSamplerBindingType_Filtering;
                break;
            case gfx_uniform_texture2d:
                e->texture.viewDimension = WGPUTextureViewDimension_2D;
                e->texture.sampleType    = WGPUTextureSampleType_Float;
                break;
            case gfx_uniform_texture2d_array:
                e->texture.viewDimension = WGPUTextureViewDimension_2DArray;
                e->texture.sampleType    = WGPUTextureSampleType_Float;
                break;
            case gfx_uniform_texture3d:
                e->texture.viewDimension = WGPUTextureViewDimension_3D;
                e->texture.sampleType    = WGPUTextureSampleType_Float;
                break;
            case gfx_uniform_texture2d_cube:
                e->texture.viewDimension = WGPUTextureViewDimension_Cube;
                e->texture.sampleType    = WGPUTextureSampleType_Float;
                break;
            default:
                wctx->dbglog(gfx_msg_error, "wgpu_create_shader %s: '%s' uniform type %d is not supported by the webgpu backend", label, uniform->name, uniform->type);
                continue;
        }
        shader->uniforms[count] = *uniform;
        count++;
    }
    shader->binding_count = count;
    shader->uniform_count = count;

    WGPUBindGroupLayoutDescriptor layout_desc = {};
        layout_desc.label      = wgpu_str(desc->label);
        layout_desc.entryCount = count;
        layout_desc.entries    = shader->bindings;
    shader->layout = wgpuDeviceCreateBindGroupLayout(wctx->device, &layout_desc);

    WGPUPipelineLayoutDescriptor pipeline_layout_desc = {};
        pipeline_layout_desc.label                = wgpu_str(desc->label);
        pipeline_layout_desc.bindGroupLayoutCount = 1;
        pipeline_layout_desc.bindGroupLayouts     = &shader->layout;
    shader->pipeline_layout = wgpuDeviceCreatePipelineLayout(wctx->device, &pipeline_layout_desc);

    *out_shader = &shader->handle;
}

uint32_t wgpu_shader_get_descriptor_set_count(gfx_shader_t* shader)
{
    wgpu_shader_t* s = (wgpu_shader_t*)shader;
    return (s && s->uniform_count) ? 1u : 0u;      // one bind group (group 0)
}

uint32_t wgpu_shader_get_uniforms(gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms)
{
    wgpu_shader_t* s = (wgpu_shader_t*)shader;
    if (!s || set_index != 0) return 0;
    if (uniforms) memcpy(uniforms, s->uniforms, s->uniform_count * sizeof(gfx_uniform_t));
    return s->uniform_count;
}

// handle: u16 hash | u16 type | u16 uniform idx | u16 field idx (+1, so that a valid location is never 0)
struct uniform_handle_t
{
    static uint64_t create(uint16_t hash, uint16_t type, uint16_t idx, uint16_t field)
    {
        uniform_handle_t result = {};
            result.hash  = hash;
            result.type  = type;
            result.idx   = idx;
            result.field = (uint16_t)(field + 1);
        return result.handle;
    }

    union {
        uint64_t handle;
        struct {
            uint16_t hash;
            uint16_t type;
            uint16_t idx;
            uint16_t field;
        };
    };
};

uint64_t wgpu_uniform_location(gfx_shader_t* shader, const char* name)
{
    wgpu_shader_t* s = (wgpu_shader_t*)shader;
    if (!s || !name) return 0;

    for (uint32_t i = 0; i < s->uniform_count; ++i)
    {
        const gfx_uniform_t* uniform = &s->uniforms[i];
        if (!strcmp(uniform->name, name))       // exact match: "u_mat" must not find "u_mat2"
            return uniform_handle_t::create((uint16_t)s->hash, (uint16_t)uniform->type, (uint16_t)i, 0);

        if (uniform->type == gfx_uniform_ubo)
            for (uint32_t j = 0; j < uniform->buffer.field_count; ++j)
                if (!strcmp(uniform->buffer.fields[j].name, name))
                    return uniform_handle_t::create((uint16_t)s->hash, (uint16_t)uniform->type, (uint16_t)i, (uint16_t)j);
    }
    return 0;
}

void wgpu_destroy_shader(gfx_context_t* ctx, gfx_shader_t* shader)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_shader_t* s = (wgpu_shader_t*)shader;
    if (!s) return;

    while (s->current_pool) {
        wgpu_descriptor_set_pool_t* pool = s->current_pool;
        s->current_pool = pool->next;
        wgpu_release_pool(wctx, pool);
    }

    // stages built from one WGSL text share a module: release each module once
    for (int i = 0; i < 3; ++i) {
        if (!s->modules[i]) continue;
        bool seen = false;
        for (int j = 0; j < i; ++j) seen = seen || s->modules[j] == s->modules[i];
        if (!seen) wgpuShaderModuleRelease(s->modules[i]);
    }
    if (s->pipeline_layout) wgpuPipelineLayoutRelease(s->pipeline_layout);
    if (s->layout)          wgpuBindGroupLayoutRelease(s->layout);
    free(s);
}

// ============================================================================
// SAMPLER
// ============================================================================

void wgpu_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_sampler = nullptr;

    WGPUSamplerDescriptor sd = {};
        sd.addressModeU  = gfx_address_mode_2_webgpu(desc->mode);
        sd.addressModeV  = gfx_address_mode_2_webgpu(desc->mode);
        sd.addressModeW  = gfx_address_mode_2_webgpu(desc->mode);
        sd.minFilter     = gfx_filter_2_webgpu(desc->minmag);
        sd.magFilter     = gfx_filter_2_webgpu(desc->minmag);
        sd.mipmapFilter  = gfx_mipmap_filter_2_webgpu(desc->mipmap);
        sd.lodMinClamp   = 0.0f;
        sd.lodMaxClamp   = 32.0f;
        // anisotropy needs linear min/mag/mip filtering in WebGPU
        sd.maxAnisotropy = (desc->minmag == gfx_filter_linear && desc->mipmap == gfx_filter_linear) ? (uint16_t)gfx_max(desc->anisotropy, 1u) : 1;
    WGPUSampler sampler = wgpuDeviceCreateSampler(wctx->device, &sd);

    wgpu_sampler_t* s = (wgpu_sampler_t*)calloc(1, sizeof(wgpu_sampler_t));
    if (!s) { wgpuSamplerRelease(sampler); return; }
    s->sampler = sampler;
    *out_sampler = &s->handle;
}

void wgpu_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler)
{
    (void)ctx;
    wgpu_sampler_t* s = (wgpu_sampler_t*)sampler;
    if (!s) return;
    if (s->sampler) wgpuSamplerRelease(s->sampler);
    free(s);
}

// ============================================================================
// TEXTURE
// ============================================================================

// desc->data: per layer (cube face / array slice) the full mip chain, as the loaders produce it
static void wgpu_write_texture_mips(wgpu_context_t* wctx, WGPUTexture texture, const gfx_texture_desc_t* desc, uint32_t layers, uint32_t mips)
{
    const uint32_t block = gfx_pixel_format_block(desc->format);
    const bool is_3d = desc->type == gfx_texture3d;
    const uint8_t* src = (const uint8_t*)desc->data;

    for (uint32_t layer = 0; layer < layers; ++layer) {
        for (uint32_t level = 0; level < mips; ++level) {
            const uint32_t w = gfx_max(1u, desc->width  >> level);
            const uint32_t h = gfx_max(1u, desc->height >> level);
            const uint32_t d = is_3d ? gfx_max(1u, desc->depth >> level) : 1u;
            const uint32_t size = gfx_utils_image_layer_size(w, h, d, desc->format);

            WGPUTexelCopyTextureInfo dst = {};
                dst.texture  = texture;
                dst.mipLevel = level;
                dst.origin   = { 0, 0, is_3d ? 0 : layer };
                dst.aspect   = WGPUTextureAspect_All;
            WGPUTexelCopyBufferLayout layout = {};
                layout.bytesPerRow  = gfx_utils_image_row_pitch(desc->format, w);
                layout.rowsPerImage = (h + block - 1) / block;
            // compressed mips smaller than a block are copied as the whole (physical) block
            WGPUExtent3D extent = { (w + block - 1) / block * block, (h + block - 1) / block * block, d };
            wgpuQueueWriteTexture(wctx->queue, &dst, src, size, &layout, &extent);
            src += size;
        }
    }
}

void wgpu_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** out_texture)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_texture = nullptr;

    const WGPUTextureFormat format = gfx_pixel_format_2_webgpu(desc->format);
    if (format == WGPUTextureFormat_Undefined) {
        wctx->dbglog(gfx_msg_error, "wgpu_create_texture %s: pixel format %s has no WebGPU equivalent", desc->label ? desc->label : "", gfx_to_string(desc->format));
        return;
    }

    const uint32_t mips = gfx_max(1u, desc->mip_levels);
    uint32_t layers = 1;
    WGPUTextureDimension dimension = WGPUTextureDimension_2D;
    WGPUTextureViewDimension view_dimension = WGPUTextureViewDimension_2D;
    switch (desc->type) {
        case gfx_texture2d_cube:  layers = 6; view_dimension = WGPUTextureViewDimension_Cube; break;
        case gfx_texture2d_array: layers = gfx_max(1u, gfx_max(desc->array_layers, desc->depth)); view_dimension = WGPUTextureViewDimension_2DArray; break;
        case gfx_texture3d:       dimension = WGPUTextureDimension_3D; view_dimension = WGPUTextureViewDimension_3D; break;
        default:                  break;
    }

    WGPUTextureUsage usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc;
    if ((desc->usage_flags & gfx_texture_usage_render_target) || wgpu_format_is_depth(format)) usage |= WGPUTextureUsage_RenderAttachment;
    if (desc->usage_flags & gfx_texture_usage_storage)                                         usage |= WGPUTextureUsage_StorageBinding;

    WGPUTextureDescriptor td = {};
        td.label         = wgpu_str(desc->label);
        td.usage         = usage;
        td.dimension     = dimension;
        td.size          = { desc->width, desc->height, dimension == WGPUTextureDimension_3D ? gfx_max(1u, desc->depth) : layers };
        td.format        = format;
        td.mipLevelCount = mips;
        td.sampleCount   = 1;
    WGPUTexture texture = wgpuDeviceCreateTexture(wctx->device, &td);
    if (!texture) return;

    WGPUTextureViewDescriptor vd = {};
        vd.label           = wgpu_str(desc->label);
        vd.format          = format;
        vd.dimension       = view_dimension;
        vd.mipLevelCount   = mips;
        vd.arrayLayerCount = dimension == WGPUTextureDimension_3D ? 1 : layers;
        vd.aspect          = WGPUTextureAspect_All;
    WGPUTextureView view = wgpuTextureCreateView(texture, &vd);

    if (desc->data)
        wgpu_write_texture_mips(wctx, texture, desc, dimension == WGPUTextureDimension_3D ? 1 : layers, mips);

    wgpu_texture_t* t = wgpu_texture_alloc(texture, view, format, desc->width, desc->height, 1);
    if (!t) { wgpuTextureViewRelease(view); wgpuTextureRelease(texture); return; }
    *out_texture = &t->handle;
}

// Replaces mip 0 of layer 0 (the whole level: size must be its byte size). Partial updates are not supported.
void wgpu_update_texture_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_texture_t* t = (wgpu_texture_t*)texture;
    if (!t || !data || t->external) return;
    if (offset != 0) {
        wctx->dbglog(gfx_msg_warning, "wgpu_update_texture_data: offset %u is not supported, only whole mip 0 updates", offset);
        return;
    }
    WGPUTexelCopyTextureInfo dst = {};
        dst.texture = t->texture;
        dst.aspect  = WGPUTextureAspect_All;
    const uint32_t rows = t->height ? t->height : 1;
    WGPUTexelCopyBufferLayout layout = {};
        layout.bytesPerRow  = size / rows;
        layout.rowsPerImage = rows;
    WGPUExtent3D extent = { t->width, t->height, 1 };
    wgpuQueueWriteTexture(wctx->queue, &dst, data, size, &layout, &extent);
}

void wgpu_update_bindless_texture(gfx_context_t* ctx, gfx_texture_t* /*texture*/, uint32_t /*idx*/)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    gfx_stub_not_implemented(wctx ? wctx->dbglog : nullptr, "wgpu_update_bindless_texture (WebGPU has no bindless)");
}

void wgpu_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* /*texture*/)
{
    // WebGPU has no built-in mipmap generation command: needs a downsample render/compute pass per level
    wgpu_context_t* wctx = from_ctx(ctx);
    if (wctx) wctx->dbglog(gfx_msg_warning, "wgpu_texture_generate_mipmap: not supported natively, use pre-generated mips");
}

void wgpu_blit_image(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_texture_t* wsrc = (wgpu_texture_t*)src;
    wgpu_texture_t* wdst = (wgpu_texture_t*)dst;
    if (!wctx || !wsrc || !wdst) return;

    WGPUTexelCopyTextureInfo src_info = {};
        src_info.texture = wsrc->texture;
        src_info.aspect  = WGPUTextureAspect_All;
    WGPUTexelCopyTextureInfo dst_info = {};
        dst_info.texture = wdst->texture;
        dst_info.aspect  = WGPUTextureAspect_All;
    WGPUExtent3D extent = {
        gfx_min(wgpuTextureGetWidth(wsrc->texture),  wgpuTextureGetWidth(wdst->texture)),
        gfx_min(wgpuTextureGetHeight(wsrc->texture), wgpuTextureGetHeight(wdst->texture)),
        gfx_min(wgpuTextureGetDepthOrArrayLayers(wsrc->texture), wgpuTextureGetDepthOrArrayLayers(wdst->texture)) };

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wctx->device, nullptr);
    wgpuCommandEncoderCopyTextureToTexture(encoder, &src_info, &dst_info, &extent);
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(wctx->queue, 1, &commands);
    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(encoder);
}

void wgpu_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* /*cmd*/)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    gfx_stub_not_implemented(wctx ? wctx->dbglog : nullptr, "wgpu_texture_get_data");
}

void wgpu_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture)
{
    (void)ctx;
    wgpu_texture_t* t = (wgpu_texture_t*)texture;
    if (!t || t->external) return;
    if (t->texture_view) wgpuTextureViewRelease(t->texture_view);
    if (t->texture)      wgpuTextureRelease(t->texture);
    free(t);
}

// ============================================================================
// PIPELINE
// ============================================================================

void wgpu_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_shader_t* shader = (wgpu_shader_t*)desc->shader;
    *out_pipeline = nullptr;
    if (!shader || !shader->modules[gfx_shader_vertex]) {
        wctx->dbglog(gfx_msg_error, "wgpu_create_pipeline: shader without a vertex stage");
        return;
    }
    const gfx_render_states_desc_t& rs = desc->render_states;

    // attachments: explicit color formats render into textures (1x), otherwise the surface (its format and MSAA)
    const bool to_surface = rs.attachments.color == nullptr;
    const WGPUTextureFormat color_format = to_surface ? wctx->surface_format : gfx_pixel_format_2_webgpu(rs.attachments.color[0]);
    const uint32_t sample_count = to_surface ? wctx->sample_count : 1;

    WGPUBlendState blend = {};
        blend.color.operation = gfx_blend_op_2_webgpu(rs.blend.color_op);
        blend.color.srcFactor = gfx_blend_mode_2_webgpu(rs.blend.color_src);
        blend.color.dstFactor = gfx_blend_mode_2_webgpu(rs.blend.color_dst);
        blend.alpha.operation = gfx_blend_op_2_webgpu(rs.blend.alpha_op);
        blend.alpha.srcFactor = gfx_blend_mode_2_webgpu(rs.blend.alpha_src);
        blend.alpha.dstFactor = gfx_blend_mode_2_webgpu(rs.blend.alpha_dst);

    WGPUColorTargetState color_state = {};
        color_state.format    = color_format;
        color_state.writeMask = (rs.color_mask.red   ? WGPUColorWriteMask_Red   : WGPUColorWriteMask_None)
                              | (rs.color_mask.green ? WGPUColorWriteMask_Green : WGPUColorWriteMask_None)
                              | (rs.color_mask.blue  ? WGPUColorWriteMask_Blue  : WGPUColorWriteMask_None)
                              | (rs.color_mask.alpha ? WGPUColorWriteMask_Alpha : WGPUColorWriteMask_None);
        color_state.blend     = rs.blend.enable ? &blend : nullptr;

    // a pass always has the depth attachment of its target: every pipeline declares it, disabled depth = always/no write
    WGPUStencilFaceState stencil = {};
        stencil.compare     = gfx_cmp_2_webgpu(rs.stencil.func);
        stencil.failOp      = gfx_op_2_webgpu(rs.stencil.opFail);
        stencil.depthFailOp = gfx_op_2_webgpu(rs.stencil.opZFail);
        stencil.passOp      = gfx_op_2_webgpu(rs.stencil.opPass);
    const bool has_stencil = wgpu_format_has_stencil(wctx->depth_format);

    WGPUDepthStencilState depth_state = {};
        depth_state.format            = wctx->depth_format;
        depth_state.depthWriteEnabled = rs.depth.write ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depth_state.depthCompare      = rs.depth.enable ? gfx_cmp_2_webgpu(rs.depth.mode) : WGPUCompareFunction_Always;
        depth_state.stencilFront      = stencil;
        depth_state.stencilBack       = stencil;
        depth_state.stencilReadMask   = has_stencil ? rs.stencil.pass : 0;
        depth_state.stencilWriteMask  = has_stencil ? 0xFF : 0;

    // vertex buffers: attributes grouped by the slot they read from
    WGPUVertexAttribute    attributes[16] = {};
    WGPUVertexBufferLayout buffers[8]     = {};
    const uint32_t slot_count = gfx_min(desc->assembly.slot_count, 8u);
    uint32_t attribute_count = 0;
    for (uint32_t i = 0; i < slot_count; ++i)
    {
        buffers[i].arrayStride = desc->assembly.slots[i].stride;
        buffers[i].stepMode    = desc->assembly.slots[i].rate == gfx_vertex_rate_instance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
        buffers[i].attributes  = &attributes[attribute_count];
        for (uint32_t j = 0; j < desc->assembly.attribute_count && attribute_count < 16; ++j) {
            if (desc->assembly.attributes[j].binding != i && slot_count > 1) continue;
            WGPUVertexAttribute* a = &attributes[attribute_count++];
            a->offset         = desc->assembly.attributes[j].offset;
            a->shaderLocation = desc->assembly.attributes[j].location;
            a->format         = gfx_vertex_format_2_webgpu(desc->assembly.attributes[j].format);
            buffers[i].attributeCount++;
        }
    }

    WGPUVertexState vertex = {};
        vertex.module      = shader->modules[gfx_shader_vertex];
        vertex.entryPoint  = wgpu_str(shader->entry_names[gfx_shader_vertex]);
        vertex.bufferCount = slot_count;
        vertex.buffers     = buffers;

    WGPUFragmentState fragment = {};
    const bool has_fragment = shader->modules[gfx_shader_fragment] != nullptr;
    if (has_fragment) {
        fragment.module      = shader->modules[gfx_shader_fragment];
        fragment.entryPoint  = wgpu_str(shader->entry_names[gfx_shader_fragment]);
        fragment.targetCount = 1;
        fragment.targets     = &color_state;
    }

    WGPURenderPipelineDescriptor pd = {};
        pd.label                  = wgpu_str("gfx pipeline");
        pd.layout                 = shader->pipeline_layout;
        pd.vertex                 = vertex;
        pd.primitive.topology     = gfx_topology_2_webgpu(desc->assembly.topology);
        pd.primitive.frontFace    = gfx_face_2_webgpu(rs.face);
        pd.primitive.cullMode     = gfx_cull_2_webgpu(rs.culling);
        pd.depthStencil           = &depth_state;
        pd.multisample.count      = sample_count;
        pd.multisample.mask       = 0xFFFFFFFFu;
        pd.fragment               = has_fragment ? &fragment : nullptr;
    WGPURenderPipeline render_pipeline = wgpuDeviceCreateRenderPipeline(wctx->device, &pd);
    if (!render_pipeline) return;

    wgpu_pipeline_t* pipeline = (wgpu_pipeline_t*)calloc(1, sizeof(wgpu_pipeline_t));
    if (!pipeline) { wgpuRenderPipelineRelease(render_pipeline); return; }
    pipeline->pipeline = render_pipeline;
    *out_pipeline = &pipeline->handle;
}

void wgpu_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
    (void)ctx;
    wgpu_pipeline_t* p = (wgpu_pipeline_t*)pipeline;
    if (!p) return;
    if (p->pipeline) wgpuRenderPipelineRelease(p->pipeline);
    free(p);
}

void wgpu_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** out_pipeline)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    *out_pipeline = nullptr;
    wgpu_shader_t* shader = desc ? (wgpu_shader_t*)desc->shader : nullptr;
    if (!shader || !shader->modules[gfx_shader_compute]) {
        wctx->dbglog(gfx_msg_error, "wgpu_create_compute_pipeline: shader without a compute stage");
        return;
    }

    WGPUComputePipelineDescriptor cd = {};
        cd.label              = wgpu_str("gfx compute pipeline");
        cd.layout             = shader->pipeline_layout;
        cd.compute.module     = shader->modules[gfx_shader_compute];
        cd.compute.entryPoint = wgpu_str(shader->entry_names[gfx_shader_compute]);
    WGPUComputePipeline compute = wgpuDeviceCreateComputePipeline(wctx->device, &cd);
    if (!compute) return;

    wgpu_compute_pipeline_t* p = (wgpu_compute_pipeline_t*)calloc(1, sizeof(wgpu_compute_pipeline_t));
    if (!p) { wgpuComputePipelineRelease(compute); return; }
    p->pipeline = compute;
    *out_pipeline = &p->handle;
}

void wgpu_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline)
{
    (void)ctx;
    wgpu_compute_pipeline_t* p = (wgpu_compute_pipeline_t*)pipeline;
    if (!p) return;
    if (p->pipeline) wgpuComputePipelineRelease(p->pipeline);
    free(p);
}

void wgpu_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* /*desc*/, gfx_pipeline_t** pipeline)
{
    *pipeline = nullptr;
    from_ctx(ctx)->dbglog(gfx_msg_error, "wgpu_create_mesh_pipeline: WebGPU has no mesh shaders");
}

void wgpu_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* /*desc*/, gfx_pipeline_raytrace_t** pipeline)
{
    *pipeline = nullptr;
    from_ctx(ctx)->dbglog(gfx_msg_error, "wgpu_create_raytrace_pipeline: WebGPU has no ray tracing");
}

void wgpu_destroy_raytrace_pipeline(gfx_context_t* /*ctx*/, gfx_pipeline_raytrace_t* /*pipeline*/)
{
}

// ============================================================================
// DESCRIPTOR SET
// ============================================================================

static void wgpu_release_pool(wgpu_context_t* wctx, wgpu_descriptor_set_pool_t* pool)
{
    for (wgpu_descriptor_set_pool_t** p = &wctx->pools; *p; p = &(*p)->ctx_next)
        if (*p == pool) { *p = pool->ctx_next; break; }

    for (uint32_t i = 0; i < pool->capacity; ++i)
        if (pool->descriptor_sets[i].bind_group) wgpuBindGroupRelease(pool->descriptor_sets[i].bind_group);
    if (pool->ubo_buffer) wgpu_destroy_buffer(&wctx->handle, pool->ubo_buffer);
    if (pool->shader && pool->shader->current_pool == pool) pool->shader->current_pool = pool->next;
    free(pool->ubo_buffer_data_ptr);
    free(pool->descriptor_sets);
    free(pool);
}

static void wgpu_create_descriptor_set_pool(wgpu_context_t* wctx, wgpu_shader_t* shader, uint32_t capacity, wgpu_descriptor_set_pool_t** out_pool)
{
    // every UBO of a set gets its own aligned slice of the set's block: ubo_offsets[i] relative to the block start
    const uint32_t alignment = gfx_max(wctx->limits.minUniformBufferOffsetAlignment, 16u);
    uint32_t ubo_size = 0;
    uint32_t ubo_offsets[WGPU_MAX_BINDINGS] = {};
    for (uint32_t i = 0; i < shader->uniform_count; ++i) {
        if (shader->uniforms[i].type != gfx_uniform_ubo) continue;
        ubo_offsets[i] = ubo_size;
        ubo_size = gfx_utils_align_up(ubo_size + shader->uniforms[i].buffer.size, alignment);
    }

    wgpu_descriptor_set_pool_t* pool = (wgpu_descriptor_set_pool_t*)calloc(1, sizeof(wgpu_descriptor_set_pool_t));
    pool->capacity       = capacity;
    pool->free_set_count = capacity;
    pool->shader         = shader;

    if (ubo_size > 0) {
        gfx_buffer_desc_t desc = {};
            desc.label = "descriptor set uniforms";
            desc.usage = gfx_buffer_usage_uniform;
            desc.size  = ubo_size * capacity;
        wgpu_create_buffer(&wctx->handle, &desc, &pool->ubo_buffer);
        pool->ubo_buffer_data_size = ubo_size * capacity;
        pool->ubo_buffer_data_ptr  = (uint8_t*)calloc(pool->ubo_buffer_data_size, 1);
    }
    pool->descriptor_sets = (wgpu_descriptor_set_t*)calloc(capacity, sizeof(wgpu_descriptor_set_t));
    WGPUBuffer ubo = pool->ubo_buffer ? ((wgpu_buffer_t*)pool->ubo_buffer)->buffer : nullptr;

    for (uint32_t j = 0; j < capacity; ++j)
    {
        wgpu_descriptor_set_t* set = &pool->descriptor_sets[j];
        set->isfree           = true;
        set->shader           = shader;
        set->owner_pool       = pool;
        set->index_in_pool    = j;
        set->bind_group_dirty = true;

        // defaults until the user writes the entries: the context's texture/sampler, the set's UBO slices
        for (uint32_t i = 0; i < shader->binding_count; ++i) {
            const WGPUBindGroupLayoutEntry* layout = &shader->bindings[i];
            WGPUBindGroupEntry* e = &set->bindings[i];
            e->binding = layout->binding;
            if (layout->sampler.type != WGPUSamplerBindingType_BindingNotUsed && layout->sampler.type != WGPUSamplerBindingType_Undefined)
                e->sampler = ((wgpu_sampler_t*)wctx->default_sampler)->sampler;
            if (layout->texture.sampleType != WGPUTextureSampleType_BindingNotUsed && layout->texture.sampleType != WGPUTextureSampleType_Undefined)
                e->textureView = ((wgpu_texture_t*)wctx->default_texture)->texture_view;
            if (layout->buffer.type == WGPUBufferBindingType_Uniform && ubo) {
                e->buffer = ubo;
                e->offset = (uint64_t)ubo_size * j + ubo_offsets[i];
                e->size   = shader->uniforms[i].buffer.size;
            }
        }
    }

    pool->ctx_next = wctx->pools;
    wctx->pools = pool;
    *out_pool = pool;
}

void wgpu_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* gshader, uint32_t set_idx, gfx_descriptor_set_t** out_descriptor)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_shader_t* shader = (wgpu_shader_t*)gshader;
    *out_descriptor = nullptr;
    if (!shader) {
        wctx->dbglog(gfx_msg_error, "wgpu_create_descriptor_set: shader is null");
        return;
    }
    if (set_idx != 0)
        wctx->dbglog(gfx_msg_warning, "wgpu_create_descriptor_set: set %u requested, the webgpu backend has set 0 only", set_idx);

    // a free set in any pool of the shader, else a new (bigger) pool in front of the list
    for (wgpu_descriptor_set_pool_t* pool = shader->current_pool; pool; pool = pool->next) {
        if (pool->free_set_count == 0) continue;
        for (uint32_t n = 0; n < pool->capacity; ++n) {
            const uint32_t i = (pool->next_free + n) % pool->capacity;
            wgpu_descriptor_set_t* set = &pool->descriptor_sets[i];
            if (!set->isfree) continue;
            set->isfree = false;
            pool->count++;
            pool->free_set_count--;
            pool->next_free = (i + 1) % pool->capacity;
            *out_descriptor = &set->handle;
            return;
        }
    }

    wgpu_descriptor_set_pool_t* pool = nullptr;
    wgpu_create_descriptor_set_pool(wctx, shader, shader->current_pool ? shader->current_pool->capacity * 2 : 64, &pool);
    pool->next = shader->current_pool;
    shader->current_pool = pool;
    wgpu_create_descriptor_set(ctx, gshader, set_idx, out_descriptor);
}

void wgpu_uniform_update_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size)
{
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)set;
    if (!wset || !data || handle == 0) return;
    wgpu_descriptor_set_pool_t* pool = wset->owner_pool;
    wgpu_shader_t* shader = wset->shader;

    uniform_handle_t uh = { handle };
    if (uh.hash != (uint16_t)shader->hash || uh.field == 0) return;
    const uint32_t idx = uh.idx, field = uh.field - 1u;

    if (idx >= shader->uniform_count || shader->uniforms[idx].type != gfx_uniform_ubo || !pool->ubo_buffer ||
        wset->bindings[idx].buffer != ((wgpu_buffer_t*)pool->ubo_buffer)->buffer || field >= shader->uniforms[idx].buffer.field_count)
        return;     // not one of the pool's UBO slices (a user buffer was bound with wgpu_uniform_set_buffer)

    // CPU copy of the pool, uploaded at frame end; bindings[idx].offset already includes this set's and UBO's slice
    const uint32_t field_offset = shader->uniforms[idx].buffer.fields[field].offset;
    const uint32_t ubo_bytes    = shader->uniforms[idx].buffer.size;
    if (field_offset >= ubo_bytes) return;
    size = gfx_min(size, ubo_bytes - field_offset);
    memcpy(pool->ubo_buffer_data_ptr + wset->bindings[idx].offset + field_offset, data, size);
    pool->dirty = true;
}

void wgpu_uniform_set_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buffer, uint32_t offset)
{
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)set;
    wgpu_buffer_t* wbuffer = (wgpu_buffer_t*)buffer;
    if (!wset || !wbuffer || handle == 0) return;
    uniform_handle_t uh = { handle };
    if (uh.idx >= wset->shader->uniform_count) return;

    WGPUBindGroupEntry* e = &wset->bindings[uh.idx];
    e->buffer = wbuffer->buffer;
    e->offset = offset;
    e->size   = WGPU_WHOLE_SIZE;
    wset->bind_group_dirty = true;
}

void wgpu_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture)
{
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)set;
    wgpu_texture_t* wtexture = (wgpu_texture_t*)texture;
    if (!wset || !wtexture || handle == 0) return;
    uniform_handle_t uh = { handle };
    if (uh.idx >= wset->shader->uniform_count) return;

    if (wset->bindings[uh.idx].textureView != wtexture->texture_view) {
        wset->bindings[uh.idx].textureView = wtexture->texture_view;
        wset->bind_group_dirty = true;
    }
}

void wgpu_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler)
{
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)set;
    wgpu_sampler_t* wsampler = (wgpu_sampler_t*)sampler;
    if (!wset || !wsampler || handle == 0) return;
    uniform_handle_t uh = { handle };
    if (uh.hash != (uint16_t)wset->shader->hash || uh.idx >= wset->shader->uniform_count) return;

    if (wset->bindings[uh.idx].sampler != wsampler->sampler) {
        wset->bindings[uh.idx].sampler = wsampler->sampler;
        wset->bind_group_dirty = true;
    }
}

void wgpu_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor)
{
    (void)ctx;
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)descriptor;
    if (!wset || wset->isfree) return;
    wgpu_descriptor_set_pool_t* pool = wset->owner_pool;

    // back to the defaults of a fresh set (the UBO slice entries stay, their data is reset)
    wgpu_context_t* wctx = wset->shader->context;
    for (uint32_t i = 0; i < wset->shader->binding_count; ++i) {
        WGPUBindGroupEntry* e = &wset->bindings[i];
        if (e->sampler)     e->sampler     = ((wgpu_sampler_t*)wctx->default_sampler)->sampler;
        if (e->textureView) e->textureView = ((wgpu_texture_t*)wctx->default_texture)->texture_view;
    }
    wset->bind_group_dirty = true;
    wset->isfree = true;
    pool->next_free = wset->index_in_pool;
    pool->count--;
    pool->free_set_count++;
}

static bool wgpu_prepare_bind_group(wgpu_descriptor_set_t* set)
{
    if (!set->bind_group_dirty && set->bind_group) return true;
    wgpu_shader_t* shader = set->shader;
    for (uint32_t i = 0; i < shader->binding_count; ++i) {
        const WGPUBindGroupEntry* e = &set->bindings[i];
        if (!e->buffer && !e->sampler && !e->textureView) {
            shader->context->dbglog(gfx_msg_warning, "descriptor set: '%s' (binding %u) has no resource, draw skipped", shader->uniforms[i].name, e->binding);
            return false;
        }
    }
    WGPUBindGroupDescriptor bg = {};
        bg.layout     = shader->layout;
        bg.entryCount = shader->binding_count;
        bg.entries    = set->bindings;
    WGPUBindGroup group = wgpuDeviceCreateBindGroup(shader->context->device, &bg);
    if (set->bind_group) wgpuBindGroupRelease(set->bind_group);
    set->bind_group = group;
    set->bind_group_dirty = false;
    return group != nullptr;
}

// ============================================================================
// COMMANDS
// ============================================================================

static WGPUColor wgpu_unpack_color(uint32_t rgba)     // 0xRRGGBBAA, as the Vulkan backend
{
    WGPUColor c = { ((rgba >> 24) & 0xFF) / 255.0, ((rgba >> 16) & 0xFF) / 255.0, ((rgba >> 8) & 0xFF) / 255.0, (rgba & 0xFF) / 255.0 };
    return c;
}

// target: pass->color_attachment[] (textures), else pass->target, else the frame's back buffer.
// gfx_load_op_dont_care clears (WebGPU has no "don't care" load, clear is the cheap one).
void wgpu_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* pass)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_context_t* wctx = wcmd->ctx;
    if (!wcmd->encoder) {
        wctx->dbglog(gfx_msg_error, "wgpu_cmd_begin_pass: command buffer is not recording (use the frame's cmd)");
        return;
    }
    if (wcmd->pass) wgpu_cmd_end_pass(cmd);

    WGPURenderPassColorAttachment colors[4] = {};
    uint32_t color_count = 0;
    WGPUTextureView depth_view = nullptr;
    WGPUTextureFormat depth_format = WGPUTextureFormat_Undefined;
    uint32_t width = 0, height = 0;

    if (pass->color_attachment[0]) {
        for (uint32_t i = 0; i < 4 && pass->color_attachment[i]; ++i) {
            wgpu_texture_t* t = (wgpu_texture_t*)pass->color_attachment[i];
            colors[color_count].view    = t->texture_view;
            colors[color_count].loadOp  = pass->color_load_op[i] == gfx_load_op_load ? WGPULoadOp_Load : WGPULoadOp_Clear;
            color_count++;
            width = t->width; height = t->height;
        }
        if (pass->depth_attachment) {
            wgpu_texture_t* d = (wgpu_texture_t*)pass->depth_attachment;
            depth_view = d->texture_view;
            depth_format = d->format;
        }
    } else {
        GFX_DISABLE_DEPRECATED_WARNING_BEGIN
        wgpu_render_target_t* rt = pass->target ? (wgpu_render_target_t*)pass->target : wcmd->default_target;
        GFX_DISABLE_DEPRECATED_WARNING_END
        if (!rt) {
            wctx->dbglog(gfx_msg_error, "wgpu_cmd_begin_pass: no target");
            return;
        }
        colors[0].view          = rt->color;
        colors[0].resolveTarget = rt->resolve;
        colors[0].loadOp        = pass->color_load_op[0] == gfx_load_op_load ? WGPULoadOp_Load : WGPULoadOp_Clear;
        color_count  = 1;
        depth_view   = rt->depth;
        depth_format = wctx->depth_format;
        width = rt->width; height = rt->height;
    }

    for (uint32_t i = 0; i < color_count; ++i) {
        colors[i].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        // MSAA: the multisampled image is only needed until it is resolved
        colors[i].storeOp    = colors[i].resolveTarget ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
        colors[i].clearValue = wgpu_unpack_color(pass->color_clear_value);
    }

    WGPURenderPassDepthStencilAttachment depth = {};
        depth.view            = depth_view;
        depth.depthLoadOp     = pass->depth_load_op == gfx_load_op_load ? WGPULoadOp_Load : WGPULoadOp_Clear;
        depth.depthStoreOp    = WGPUStoreOp_Store;
        depth.depthClearValue = pass->depth_clear_value;
    if (wgpu_format_has_stencil(depth_format)) {
        depth.stencilLoadOp     = depth.depthLoadOp;
        depth.stencilStoreOp    = WGPUStoreOp_Store;
        depth.stencilClearValue = pass->stencil_clear_value;
    }

    WGPURenderPassDescriptor rp = {};
        rp.colorAttachmentCount   = color_count;
        rp.colorAttachments       = colors;
        rp.depthStencilAttachment = depth_view ? &depth : nullptr;
    wcmd->pass = wgpuCommandEncoderBeginRenderPass(wcmd->encoder, &rp);

    // as the Vulkan backend: the whole target
    if (width && height) {
        wgpuRenderPassEncoderSetViewport(wcmd->pass, 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f);
        wgpuRenderPassEncoderSetScissorRect(wcmd->pass, 0, 0, width, height);
    }
}

void wgpu_cmd_end_pass(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (!wcmd->pass) return;
    wgpuRenderPassEncoderEnd(wcmd->pass);
    wgpuRenderPassEncoderRelease(wcmd->pass);
    wcmd->pass = nullptr;
}

void wgpu_cmd_begin_compute_pass(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (!wcmd->encoder || wcmd->pass || wcmd->compute_pass) return;     // another pass is active
    wcmd->compute_pass = wgpuCommandEncoderBeginComputePass(wcmd->encoder, nullptr);
}

void wgpu_cmd_end_compute_pass(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (!wcmd->compute_pass) return;
    wgpuComputePassEncoderEnd(wcmd->compute_pass);
    wgpuComputePassEncoderRelease(wcmd->compute_pass);
    wcmd->compute_pass = nullptr;
}

void wgpu_cmd_bind_compute_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_compute_t* pipeline)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_compute_pipeline_t* p = (wgpu_compute_pipeline_t*)pipeline;
    if (wcmd->compute_pass && p) wgpuComputePassEncoderSetPipeline(wcmd->compute_pass, p->pipeline);
}

void wgpu_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass) wgpuRenderPassEncoderSetScissorRect(wcmd->pass, x, y, w, h);
}

void wgpu_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass) wgpuRenderPassEncoderSetViewport(wcmd->pass, (float)x, (float)y, (float)w, (float)h, 0.0f, 1.0f);
}

void wgpu_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_pipeline_t* p = (wgpu_pipeline_t*)pipeline;
    if (wcmd->pass && p) wgpuRenderPassEncoderSetPipeline(wcmd->pass, p->pipeline);
}

void wgpu_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* set)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_descriptor_set_t* wset = (wgpu_descriptor_set_t*)set;
    if (!wset || !wgpu_prepare_bind_group(wset)) return;

    if (wcmd->pass)              wgpuRenderPassEncoderSetBindGroup(wcmd->pass, slot, wset->bind_group, 0, nullptr);
    else if (wcmd->compute_pass) wgpuComputePassEncoderSetBindGroup(wcmd->compute_pass, slot, wset->bind_group, 0, nullptr);
}

void wgpu_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t* b = (wgpu_buffer_t*)buffer;
    if (!wcmd->pass || !b) return;
    const WGPUIndexFormat index_format = (format == gfx_index_format_16) ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
    wgpuRenderPassEncoderSetIndexBuffer(wcmd->pass, b->buffer, index_format, offset, WGPU_WHOLE_SIZE);
}

void wgpu_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t* b = (wgpu_buffer_t*)buffer;
    if (wcmd->pass && b) wgpuRenderPassEncoderSetVertexBuffer(wcmd->pass, slot, b->buffer, offset, WGPU_WHOLE_SIZE);
}

void wgpu_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass) wgpuRenderPassEncoderDraw(wcmd->pass, vertex_count, gfx_max(instance_count, 1u), 0, 0);
}

void wgpu_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t index_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass) wgpuRenderPassEncoderDrawIndexed(wcmd->pass, index_count, gfx_max(instance_count, 1u), first_idx, (int32_t)vertex_offset, 0);
}

void wgpu_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride)
{
    // WebGPU has no multi-draw: draw_count indirect draws at offset + i * stride (indirect_data_t layout, see the contract)
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t* b = (wgpu_buffer_t*)buffer;
    if (!wcmd->pass || !b) return;
    for (uint32_t i = 0; i < draw_count; ++i)
        wgpuRenderPassEncoderDrawIndexedIndirect(wcmd->pass, b->buffer, (uint64_t)offset + (uint64_t)i * stride);
}

void wgpu_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->compute_pass) wgpuComputePassEncoderDispatchWorkgroups(wcmd->compute_pass, x, y, z);
}

void wgpu_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass)              wgpuRenderPassEncoderPushDebugGroup(wcmd->pass, wgpu_str(marker));
    else if (wcmd->compute_pass) wgpuComputePassEncoderPushDebugGroup(wcmd->compute_pass, wgpu_str(marker));
    else if (wcmd->encoder)      wgpuCommandEncoderPushDebugGroup(wcmd->encoder, wgpu_str(marker));
}

void wgpu_cmd_pop_marker(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wcmd = (wgpu_command_buffer_t*)cmd;
    if (wcmd->pass)              wgpuRenderPassEncoderPopDebugGroup(wcmd->pass);
    else if (wcmd->compute_pass) wgpuComputePassEncoderPopDebugGroup(wcmd->compute_pass);
    else if (wcmd->encoder)      wgpuCommandEncoderPopDebugGroup(wcmd->encoder);
}

// contract B: WebGPU tracks resource states itself, barriers are hints for explicit APIs only
void wgpu_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst)
{
    (void)cmd; (void)buffers; (void)count; (void)src; (void)dst;
}

void wgpu_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst)
{
    (void)cmd; (void)textures; (void)count; (void)src; (void)dst;
}

#endif // WEBGPU_AVAILABLE
