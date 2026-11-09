#ifndef __gfx_metal_h__
#define __gfx_metal_h__

#include "gfx.h"

#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#if defined(__APPLE__) && __has_include(<Metal/Metal.h>)
    #ifndef METAL_AVAILABLE
        #define METAL_AVAILABLE
    #endif
#endif

#if defined(METAL_AVAILABLE)

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

// ============================================================================
// Internal types
// ============================================================================

struct metal_descriptor_set_pool_t;

typedef struct metal_context_t {
    gfx_context_t                   handle;

    id<MTLDevice>                   device;
    id<MTLCommandQueue>             cmd_queue;

    gfx_caps_t                      caps;
    gfx_callback                    dbglog;
    gfx_allocator_t*                allocator;

    gfx_sampler_t*                  default_sampler;
    gfx_texture_t*                  default_texture;

    // Cached frame object (single-threaded presentation for now)
    struct metal_frame_internal_t*  current_frame;
} metal_context_t;

typedef struct metal_surface_t {
    gfx_surface_t                   handle;
    metal_context_t*                mctx;

    CAMetalLayer*                   layer;
    MTKView*                        view;           // optional, if window_handle is MTKView*
    id<MTLTexture>                  depth_texture;

    bool                            vsync;
    gfx_pixel_format                preferred_format;
    gfx_sample_count                sample_count;
    CGSize                          drawable_size;
} metal_surface_t;

typedef struct metal_command_buffer_t {
    gfx_command_buffer_t            handle;
    metal_context_t*                mctx;
    id<MTLCommandBuffer>            cmd_buffer;
    id<MTLRenderCommandEncoder>     encoder;
    id<MTLComputeCommandEncoder>    compute_encoder;
    id<MTLComputePipelineState>     bound_compute_pso;

    // Optional swapchain targets (filled by frame_begin for default pass)
    id<MTLTexture>                  default_color;
    id<MTLTexture>                  default_depth;

    struct metal_descriptor_set_pool_t* active_pool;
    MTLIndexType                    index_type;
    id<MTLBuffer>                   index_buffer;
    NSUInteger                      index_buffer_offset;
    MTLPrimitiveType                topology;
} metal_command_buffer_t;

typedef struct metal_buffer_t {
    gfx_buffer_t                    handle;
    gfx_buffer_usage                usage;
    id<MTLBuffer>                   buffer;
    uint32_t                        size;
} metal_buffer_t;

typedef struct metal_sampler_t {
    gfx_sampler_t                   handle;
    id<MTLSamplerState>             sampler;
} metal_sampler_t;

typedef struct metal_texture_t {
    gfx_texture_t                   handle;
    id<MTLTexture>                  texture;
    gfx_pixel_format                format;
    uint32_t                        width;
    uint32_t                        height;
    uint32_t                        mip_levels;
    bool                            is_swapchain_owned; // do not release
} metal_texture_t;

typedef struct metal_frame_internal_t {
    gfx_frame_t                     public_frame;
    metal_surface_t*                msurface;
    id<CAMetalDrawable>             drawable;
    metal_command_buffer_t          mcmd;
    // transient views wrapping swapchain textures for pass_info
    metal_texture_t*                color_view;
    metal_texture_t*                depth_view;
} metal_frame_internal_t;

typedef struct metal_pipeline_t {
    gfx_pipeline_t                  handle;
    MTLWinding                      face;
    MTLCullMode                     cull_mode;
    MTLPrimitiveType                topology;
    id<MTLRenderPipelineState>      pipeline;
    id<MTLDepthStencilState>        depth_state;
} metal_pipeline_t;

typedef struct metal_pipeline_compute_t {
    gfx_pipeline_compute_t          handle;
    id<MTLComputePipelineState>     pipeline;
} metal_pipeline_compute_t;

typedef struct metal_writes_t {
    gfx_uniform_type                type;
    uint32_t                        stage_mask;
    uint16_t                        slot;
    uint16_t                        offset;
    union {
        id<MTLBuffer>               buffer;
        id<MTLTexture>              texture;
        id<MTLSamplerState>         sampler;
    };
} metal_writes_t;

typedef struct metal_descriptor_set_pool_t {
    uint32_t                        capacity;
    uint32_t                        count;
    uint32_t                        free_set_count;
    uint32_t                        next_free;

    metal_writes_t*                 writes;
    struct metal_descriptor_set_t*  descriptor_sets;

    gfx_buffer_t*                   ubo;
    uint32_t                        ubo_buffer_data_size;
    void*                           ubo_buffer_data_ptr;
} metal_descriptor_set_pool_t;

typedef struct metal_descriptor_set_t {
    gfx_descriptor_set_t            handle;
    struct metal_shader_t*          shader;
    metal_descriptor_set_pool_t*    pool;
    int8_t                          isfree;
    uint32_t                        writes_count;
    metal_writes_t*                 writes;
    void*                           ubo_ptr;
} metal_descriptor_set_t;

typedef struct metal_shader_t {
    gfx_shader_t                    handle;
    uint16_t                        hash;
    metal_descriptor_set_pool_t*    current_pool;

    uint32_t                        uniform_count;
    gfx_uniform_t                   uniforms[32];

    id<MTLFunction>                 vertex_func;
    id<MTLFunction>                 fragment_func;
    id<MTLFunction>                 compute_func;
} metal_shader_t;

// ============================================================================
// Helpers
// ============================================================================

static inline metal_context_t* from_ctx(gfx_context_t* ctx) {
    return (metal_context_t*)ctx;
}

static void metal_default_log(gfx_msg type, const char* format, ...) {
    (void)type;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static MTLSamplerMinMagFilter metal_filter_map[] = {
    MTLSamplerMinMagFilterNearest,
    MTLSamplerMinMagFilterLinear
};
static MTLSamplerMipFilter metal_mip_map[] = {
    MTLSamplerMipFilterNearest,
    MTLSamplerMipFilterLinear
};
static MTLSamplerAddressMode metal_address_map[] = {
    MTLSamplerAddressModeRepeat,
    MTLSamplerAddressModeMirrorRepeat,
    MTLSamplerAddressModeClampToEdge
};

static MTLCompareFunction metal_cmp_map[] = {
    MTLCompareFunctionNever,
    MTLCompareFunctionLess,
    MTLCompareFunctionEqual,
    MTLCompareFunctionLessEqual,
    MTLCompareFunctionGreater,
    MTLCompareFunctionNotEqual,
    MTLCompareFunctionGreaterEqual,
    MTLCompareFunctionAlways
};

static MTLPrimitiveType metal_topology_map[] = {
    MTLPrimitiveTypePoint,
    MTLPrimitiveTypeLine,
    MTLPrimitiveTypeLineStrip,
    MTLPrimitiveTypeTriangle,
    MTLPrimitiveTypeTriangleStrip
};

static MTLCullMode metal_cull_map[] = {
    MTLCullModeNone,
    MTLCullModeBack,
    MTLCullModeFront
};

static MTLWinding metal_face_map[] = {
    MTLWindingClockwise,
    MTLWindingCounterClockwise
};

static MTLPixelFormat metal_pixel_format(gfx_pixel_format format) {
    switch (format) {
        case gfx_pixel_format_a8:           return MTLPixelFormatR8Unorm;
        case gfx_pixel_format_rgba8:        return MTLPixelFormatRGBA8Unorm;
        case gfx_pixel_format_etc1:         return MTLPixelFormatETC2_RGB8;
        case gfx_pixel_format_etc2_rgba8:   return MTLPixelFormatEAC_RGBA8;
        case gfx_pixel_format_etc2_rgb8a1:  return MTLPixelFormatETC2_RGB8A1;
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
        case gfx_pixel_format_bc1:          return MTLPixelFormatBC1_RGBA;
        case gfx_pixel_format_bc3:          return MTLPixelFormatBC3_RGBA;
        case gfx_pixel_format_bc4:          return MTLPixelFormatBC4_RUnorm;
        case gfx_pixel_format_bc5:          return MTLPixelFormatBC5_RGUnorm;
        case gfx_pixel_format_bc6h:         return MTLPixelFormatBC6H_RGBFloat;
        case gfx_pixel_format_bc7:          return MTLPixelFormatBC7_RGBAUnorm;
#endif
        case gfx_pixel_format_astc4x4:      return MTLPixelFormatASTC_4x4_LDR;
        case gfx_pixel_format_astc5x5:      return MTLPixelFormatASTC_5x5_LDR;
        case gfx_pixel_format_astc6x6:      return MTLPixelFormatASTC_6x6_LDR;
        case gfx_pixel_format_astc8x8:      return MTLPixelFormatASTC_8x8_LDR;
        case gfx_pixel_format_astc10x10:    return MTLPixelFormatASTC_10x10_sRGB;
        case gfx_pixel_format_astc12x12:    return MTLPixelFormatASTC_12x12_sRGB;
        case gfx_pixel_format_r16f:         return MTLPixelFormatR16Float;
        case gfx_pixel_format_rg16f:        return MTLPixelFormatRG16Float;
        case gfx_pixel_format_rgba16f:      return MTLPixelFormatRGBA16Float;
        case gfx_pixel_format_r32f:         return MTLPixelFormatR32Float;
        case gfx_pixel_format_rg32f:        return MTLPixelFormatRG32Float;
        case gfx_pixel_format_rgba32f:      return MTLPixelFormatRGBA32Float;
        case gfx_pixel_format_d24x8:        return MTLPixelFormatDepth32Float;
        case gfx_pixel_format_d24s8:        return MTLPixelFormatDepth32Float_Stencil8;
        case gfx_pixel_format_d32:          return MTLPixelFormatDepth32Float;
        default:                            return MTLPixelFormatInvalid;
    }
}

static bool metal_is_compressed(gfx_pixel_format format) {
    switch (format) {
        case gfx_pixel_format_etc1:
        case gfx_pixel_format_etc2_rgb8a1:
        case gfx_pixel_format_etc2_rgba8:
        case gfx_pixel_format_bc1:
        case gfx_pixel_format_bc3:
        case gfx_pixel_format_bc4:
        case gfx_pixel_format_bc5:
        case gfx_pixel_format_bc6h:
        case gfx_pixel_format_bc7:
        case gfx_pixel_format_astc4x4:
        case gfx_pixel_format_astc5x5:
        case gfx_pixel_format_astc6x6:
        case gfx_pixel_format_astc8x8:
        case gfx_pixel_format_astc10x10:
        case gfx_pixel_format_astc12x12:
            return true;
        default:
            return false;
    }
}

static MTLVertexFormat metal_vertex_format(gfx_format format) {
    switch (format) {
        case gfx_format_float1:  return MTLVertexFormatFloat;
        case gfx_format_float2:  return MTLVertexFormatFloat2;
        case gfx_format_float4:  return MTLVertexFormatFloat4;
        case gfx_format_int2:    return MTLVertexFormatInt2;
        case gfx_format_int4:    return MTLVertexFormatInt4;
        case gfx_format_uint2:   return MTLVertexFormatUInt2;
        case gfx_format_uint4:   return MTLVertexFormatUInt4;
        case gfx_format_half2:   return MTLVertexFormatHalf2;
        case gfx_format_half4:   return MTLVertexFormatHalf4;
        case gfx_format_short2:  return MTLVertexFormatShort2Normalized;
        case gfx_format_short4:  return MTLVertexFormatShort4Normalized;
        case gfx_format_ushort2: return MTLVertexFormatUShort2Normalized;
        case gfx_format_ushort4: return MTLVertexFormatUShort4Normalized;
        case gfx_format_byte4:   return MTLVertexFormatUChar4Normalized;
        default:                 return MTLVertexFormatInvalid;
    }
}

static uint32_t metal_data_type_stride(MTLDataType type) {
    switch (type) {
        case MTLDataTypeInt:
        case MTLDataTypeUInt:
        case MTLDataTypeFloat:     return sizeof(float);
        case MTLDataTypeInt2:
        case MTLDataTypeUInt2:
        case MTLDataTypeFloat2:    return sizeof(float) * 2;
        case MTLDataTypeInt3:
        case MTLDataTypeUInt3:
        case MTLDataTypeFloat3:    return sizeof(float) * 3;
        case MTLDataTypeInt4:
        case MTLDataTypeUInt4:
        case MTLDataTypeFloat4:    return sizeof(float) * 4;
        case MTLDataTypeFloat3x3:  return sizeof(float) * 12; // padded
        case MTLDataTypeFloat4x4:  return sizeof(float) * 16;
        default:                   return sizeof(float);
    }
}

static gfx_format metal_data_type_to_gfx(MTLDataType type) {
    switch (type) {
        case MTLDataTypeFloat:  return gfx_format_float1;
        case MTLDataTypeFloat2: return gfx_format_float2;
        case MTLDataTypeFloat4: return gfx_format_float4;
        case MTLDataTypeInt2:   return gfx_format_int2;
        case MTLDataTypeInt4:   return gfx_format_int4;
        case MTLDataTypeUInt2:  return gfx_format_uint2;
        case MTLDataTypeUInt4:  return gfx_format_uint4;
        case MTLDataTypeHalf2:  return gfx_format_half2;
        case MTLDataTypeHalf4:  return gfx_format_half4;
        default:                return gfx_format_float4;
    }
}

static void metal_ensure_depth_texture(metal_surface_t* surface) {
    if (!surface || !surface->layer || !surface->mctx) return;
    CGSize size = surface->layer.drawableSize;
    if (size.width < 1 || size.height < 1) return;

    if (surface->depth_texture &&
        surface->depth_texture.width  == (NSUInteger)size.width &&
        surface->depth_texture.height == (NSUInteger)size.height) {
        return;
    }

    surface->depth_texture = nil;

    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                     width:(NSUInteger)size.width
                                    height:(NSUInteger)size.height
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageRenderTarget;
    desc.storageMode = MTLStorageModePrivate;
    surface->depth_texture = [surface->mctx->device newTextureWithDescriptor:desc];
    surface->depth_texture.label = @"gfx_metal_depth";
    surface->drawable_size = size;
}

// ============================================================================
// CONTEXT
// ============================================================================

static void metal_init(gfx_settings_t* settings, gfx_context_t** out_ctx) {
    if (!settings || !out_ctx) return;

    metal_context_t* mctx = (metal_context_t*)calloc(1, sizeof(metal_context_t));
    if (!mctx) return;

    mctx->device = MTLCreateSystemDefaultDevice();
    if (!mctx->device) {
        free(mctx);
        return;
    }

    mctx->cmd_queue = [mctx->device newCommandQueue];
    mctx->dbglog = settings->dbglog ? settings->dbglog : metal_default_log;
    mctx->allocator = settings->allocator;

    // Caps
    mctx->caps.gpu_type = gfx_gpu_integrated; // refined below if possible
    strncpy(mctx->caps.gpu_name, [[mctx->device name] UTF8String], sizeof(mctx->caps.gpu_name) - 1);
    strncpy(mctx->caps.gpu_vendor, "Apple", sizeof(mctx->caps.gpu_vendor) - 1);

    mctx->caps.support_etc  = true;
    mctx->caps.support_astc = true;
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    mctx->caps.support_bc = mctx->device.supportsBCTextureCompression;
#else
    mctx->caps.support_bc = false;
#endif
    mctx->caps.support_pvr = false;
    mctx->caps.support_compute = true;
    mctx->caps.support_indirect = true;
    mctx->caps.support_bindless = false;
    if (@available(macOS 11.0, iOS 14.0, *)) {
        mctx->caps.support_raytrace = mctx->device.supportsRaytracing;
    }
    mctx->caps.support_mesh_shader = false;
    mctx->caps.support_mesh_amplification_shader = false;
    mctx->caps.supported_shader_formats = gfx_shader_format_msl;
    mctx->caps.max_texture_dimension_2d = (uint32_t)mctx->device.maxBufferLength > 16384 ? 16384 : 8192;
    mctx->caps.min_uniform_buffer_offset_alignment = 256;
    mctx->caps.min_storage_buffer_offset_alignment = 16;
    mctx->caps.max_uniform_buffer_range = 64 * 1024;
    mctx->caps.has_unified_memory = true;
    mctx->caps.support_shader_float16 = true;

    // Default sampler
    gfx_sampler_desc_t sdesc = {};
    sdesc.minmag = gfx_filter_linear;
    sdesc.mipmap = gfx_filter_linear;
    sdesc.mode = gfx_address_mode_repeat;
    sdesc.anisotropy = 1;

    MTLSamplerDescriptor* sd = [MTLSamplerDescriptor new];
    sd.minFilter = MTLSamplerMinMagFilterLinear;
    sd.magFilter = MTLSamplerMinMagFilterLinear;
    sd.mipFilter = MTLSamplerMipFilterLinear;
    sd.sAddressMode = MTLSamplerAddressModeRepeat;
    sd.tAddressMode = MTLSamplerAddressModeRepeat;
    sd.maxAnisotropy = 1;

    metal_sampler_t* def_samp = (metal_sampler_t*)calloc(1, sizeof(metal_sampler_t));
    def_samp->sampler = [mctx->device newSamplerStateWithDescriptor:sd];
    mctx->default_sampler = &def_samp->handle;

    // Default 1x1 white texture
    uint32_t white = 0xFFFFFFFFu;
    MTLTextureDescriptor* td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:1 height:1 mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> tex = [mctx->device newTextureWithDescriptor:td];
    [tex replaceRegion:MTLRegionMake2D(0, 0, 1, 1) mipmapLevel:0 withBytes:&white bytesPerRow:4];

    metal_texture_t* def_tex = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
    def_tex->texture = tex;
    def_tex->format = gfx_pixel_format_rgba8;
    def_tex->width = 1;
    def_tex->height = 1;
    def_tex->mip_levels = 1;
    mctx->default_texture = &def_tex->handle;

    *out_ctx = &mctx->handle;
}

static void metal_destroy(gfx_context_t* ctx) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx) return;

    if (mctx->default_sampler) {
        metal_sampler_t* s = (metal_sampler_t*)mctx->default_sampler;
        s->sampler = nil;
        free(s);
    }
    if (mctx->default_texture) {
        metal_texture_t* t = (metal_texture_t*)mctx->default_texture;
        t->texture = nil;
        free(t);
    }
    if (mctx->current_frame) {
        free(mctx->current_frame);
        mctx->current_frame = nullptr;
    }
    mctx->cmd_queue = nil;
    mctx->device = nil;
    free(mctx);
}

static void metal_get_caps(gfx_context_t* ctx, gfx_caps_t* caps) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !caps) return;
    memcpy(caps, &mctx->caps, sizeof(gfx_caps_t));
}

// ============================================================================
// SURFACE / FRAME
// ============================================================================

static void metal_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_surface) return;

    metal_surface_t* surface = (metal_surface_t*)calloc(1, sizeof(metal_surface_t));
    surface->mctx = mctx;
    surface->vsync = desc->vsync;
    surface->preferred_format = desc->preferred_format ? desc->preferred_format : gfx_pixel_format_rgba8;
    surface->sample_count = desc->sample_count ? desc->sample_count : gfx_sample_1x;

    // window_handle may be MTKView* or CAMetalLayer*
    id handle = (__bridge id)(void*)desc->window_handle;
    if ([handle isKindOfClass:[MTKView class]]) {
        surface->view = (MTKView*)handle;
        if (surface->view.device == nil)
            surface->view.device = mctx->device;
        surface->layer = (CAMetalLayer*)surface->view.layer;
    } else if ([handle isKindOfClass:[CAMetalLayer class]]) {
        surface->layer = (CAMetalLayer*)handle;
    } else {
        // Assume NSView / UIView with metal layer
        surface->layer = (CAMetalLayer*)[(id)handle layer];
    }

    if (surface->layer) {
        surface->layer.device = mctx->device;
        surface->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        surface->layer.framebufferOnly = YES;
        if (@available(macOS 10.13, iOS 11.0, *)) {
            surface->layer.displaySyncEnabled = desc->vsync;
        }
    }

    metal_ensure_depth_texture(surface);
    *out_surface = &surface->handle;
}

static void metal_surface_destroy(gfx_context_t* ctx, gfx_surface_t* surface) {
    (void)ctx;
    if (!surface) return;
    metal_surface_t* s = (metal_surface_t*)surface;
    s->depth_texture = nil;
    s->layer = nil;
    s->view = nil;
    free(s);
}

static void metal_frame_begin(gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_surface_t* msurface = (metal_surface_t*)surface;
    if (!mctx || !msurface || !out_frame) return;

    if (!mctx->current_frame) {
        mctx->current_frame = (metal_frame_internal_t*)calloc(1, sizeof(metal_frame_internal_t));
        mctx->current_frame->color_view = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
        mctx->current_frame->depth_view = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
        mctx->current_frame->color_view->is_swapchain_owned = true;
        mctx->current_frame->depth_view->is_swapchain_owned = true;
    }

    metal_frame_internal_t* frame = mctx->current_frame;
    frame->msurface = msurface;

    metal_ensure_depth_texture(msurface);

    id<CAMetalDrawable> drawable = [msurface->layer nextDrawable];
    if (!drawable) {
        *out_frame = nullptr;
        return;
    }
    frame->drawable = drawable;

    // Wrap swapchain textures
    frame->color_view->texture = drawable.texture;
    frame->color_view->width = (uint32_t)drawable.texture.width;
    frame->color_view->height = (uint32_t)drawable.texture.height;
    frame->color_view->format = gfx_pixel_format_rgba8;
    frame->color_view->mip_levels = 1;

    frame->depth_view->texture = msurface->depth_texture;
    frame->depth_view->width = msurface->depth_texture ? (uint32_t)msurface->depth_texture.width : 0;
    frame->depth_view->height = msurface->depth_texture ? (uint32_t)msurface->depth_texture.height : 0;
    frame->depth_view->format = gfx_pixel_format_d32;
    frame->depth_view->mip_levels = 1;

    // Command buffer
    memset(&frame->mcmd, 0, sizeof(frame->mcmd));
    frame->mcmd.mctx = mctx;
    frame->mcmd.cmd_buffer = [mctx->cmd_queue commandBuffer];
    frame->mcmd.topology = MTLPrimitiveTypeTriangle;
    frame->mcmd.index_type = MTLIndexTypeUInt16;
    frame->mcmd.default_color = drawable.texture;
    frame->mcmd.default_depth = msurface->depth_texture;

    // Public frame fields
    frame->public_frame.ctx = ctx;
    frame->public_frame.frame_index++;
    frame->public_frame.swapchain_image_index = 0;
    frame->public_frame.surface = surface;
    frame->public_frame.target = nullptr;
    frame->public_frame.color_attachment = &frame->color_view->handle;
    frame->public_frame.depth_attachment = &frame->depth_view->handle;
    frame->public_frame.cmd = &frame->mcmd.handle;

    *out_frame = &frame->public_frame;
}

static void metal_frame_end(gfx_frame_t* frame) {
    if (!frame) return;
    metal_frame_internal_t* mframe = (metal_frame_internal_t*)frame;
    metal_command_buffer_t* mcmd = &mframe->mcmd;

    if (mcmd->encoder) {
        [mcmd->encoder endEncoding];
        mcmd->encoder = nil;
    }
    if (mcmd->compute_encoder) {
        [mcmd->compute_encoder endEncoding];
        mcmd->compute_encoder = nil;
    }

    if (mframe->drawable && mcmd->cmd_buffer) {
        [mcmd->cmd_buffer presentDrawable:mframe->drawable];
        [mcmd->cmd_buffer commit];
    }

    mcmd->cmd_buffer = nil;
    mframe->drawable = nil;
    mcmd->active_pool = nullptr;
    mcmd->index_buffer = nil;
}

// ============================================================================
// BUFFER
// ============================================================================

static void metal_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_buffer) return;

    MTLResourceOptions options = MTLResourceStorageModeShared;
    switch (desc->memory_hint) {
        case gfx_memory_gpu_only:
            options = MTLResourceStorageModePrivate;
            break;
        case gfx_memory_cpu_to_gpu:
        case gfx_memory_gpu_to_cpu:
        case gfx_memory_auto:
        default:
            options = MTLResourceStorageModeShared;
            break;
    }
    // Uniforms prefer shared for CPU updates
    if (desc->usage == gfx_buffer_usage_uniform || desc->mapped)
        options = MTLResourceStorageModeShared;

    id<MTLBuffer> buffer = nil;
    if (desc->data)
        buffer = [mctx->device newBufferWithBytes:desc->data length:desc->size options:options];
    else
        buffer = [mctx->device newBufferWithLength:desc->size options:options];

    if (!buffer) {
        mctx->dbglog(gfx_msg_error, "metal: failed to create buffer size=%u", desc->size);
        *out_buffer = nullptr;
        return;
    }
    if (desc->label)
        buffer.label = [NSString stringWithUTF8String:desc->label];

    metal_buffer_t* mbuf = (metal_buffer_t*)calloc(1, sizeof(metal_buffer_t));
    mbuf->buffer = buffer;
    mbuf->usage = desc->usage;
    mbuf->size = desc->size;
    *out_buffer = &mbuf->handle;
}

static void metal_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_buffer_t* mbuf = (metal_buffer_t*)buffer;
    if (!mbuf || !mbuf->buffer || !data || size == 0) return;

    void* dst = [mbuf->buffer contents];
    if (!dst) {
        // Private buffer — need staging blit (stub for now)
        if (mctx) gfx_stub_not_implemented(mctx->dbglog, "metal_buffer_update_data (private)");
        return;
    }
    if (offset + size > mbuf->size) return;
    memcpy((uint8_t*)dst + offset, data, size);
}

static void metal_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer) {
    (void)ctx;
    metal_buffer_t* mbuf = (metal_buffer_t*)buffer;
    if (!mbuf) return;
    mbuf->buffer = nil;
    free(mbuf);
}

// ============================================================================
// SHADER
// ============================================================================

static const char* metal_find_entry(const char* src, const char* stage_keyword, char* out, size_t out_sz) {
    // Very simple: look for "vertex void name(" / "fragment void name(" / "kernel void name("
    const char* p = strstr(src, stage_keyword);
    if (!p) return nullptr;
    p += strlen(stage_keyword);
    while (*p && isspace((unsigned char)*p)) ++p;
    // skip return type
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) ++p;
    while (*p && isspace((unsigned char)*p)) ++p;
    const char* start = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) ++p;
    size_t n = (size_t)(p - start);
    if (n == 0 || n >= out_sz) return nullptr;
    memcpy(out, start, n);
    out[n] = 0;
    return p;
}

static void metal_parse_arg(MTLArgument* arg, gfx_uniform_t* uniform, gfx_shader_stage stage) {
    memset(uniform, 0, sizeof(*uniform));
    const char* name = [arg.name UTF8String];
    if (name) strncpy(uniform->name, name, sizeof(uniform->name) - 1);
    uniform->binding = (uint16_t)arg.index;
    uniform->stage_mask = (uint16_t)(1u << stage);
    uniform->group = 0;

    if (arg.type == MTLArgumentTypeTexture) {
        uniform->type = gfx_uniform_texture2d;
        uniform->texture.dimension = gfx_texture2d;
        uniform->texture.access = gfx_access_read;
    } else if (arg.type == MTLArgumentTypeSampler) {
        uniform->type = gfx_uniform_sampler;
    } else if (arg.type == MTLArgumentTypeBuffer) {
        uniform->type = gfx_uniform_ubo;
        uniform->buffer.size = (uint16_t)arg.bufferDataSize;
        if (arg.bufferStructType) {
            for (MTLStructMember* member in arg.bufferStructType.members) {
                if (uniform->buffer.field_count >= 16) break;
                uint16_t fi = uniform->buffer.field_count++;
                const char* mn = [member.name UTF8String];
                if (mn) strncpy(uniform->buffer.fields[fi].name, mn, 31);
                uniform->buffer.fields[fi].offset = (uint16_t)member.offset;
                uniform->buffer.fields[fi].stride = (uint16_t)metal_data_type_stride(member.dataType);
                uniform->buffer.fields[fi].type = metal_data_type_to_gfx(member.dataType);
                uniform->buffer.fields[fi].count = 1;
            }
        }
    }
}

static uint32_t metal_merge_uniforms(gfx_uniform_t* uniforms, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        for (uint32_t j = i + 1; j < count; ) {
            if (strcmp(uniforms[i].name, uniforms[j].name) == 0 &&
                uniforms[i].binding == uniforms[j].binding &&
                uniforms[i].type == uniforms[j].type) {
                uniforms[i].stage_mask |= uniforms[j].stage_mask;
                uniforms[j] = uniforms[count - 1];
                memset(&uniforms[count - 1], 0, sizeof(gfx_uniform_t));
                --count;
            } else {
                ++j;
            }
        }
    }
    return count;
}

static void metal_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_shader) return;

    metal_shader_t* mshader = (metal_shader_t*)calloc(1, sizeof(metal_shader_t));
    NSError* error = nil;

    for (uint32_t i = 0; i < desc->stage_count; ++i) {
        gfx_shader_stage_data* st = &desc->stages[i];
        if (!st->data || st->size == 0) continue;

        NSString* src = [[NSString alloc] initWithBytes:st->data length:st->size encoding:NSUTF8StringEncoding];
        if (!src) src = [NSString stringWithUTF8String:(const char*)st->data];

        MTLCompileOptions* opts = [MTLCompileOptions new];
        id<MTLLibrary> lib = [mctx->device newLibraryWithSource:src options:opts error:&error];
        if (!lib) {
            mctx->dbglog(gfx_msg_error, "metal shader compile failed: %s",
                         error ? [[error localizedDescription] UTF8String] : "?");
            continue;
        }

        char entry[256] = {0};
        if (st->entry && st->entry[0]) {
            strncpy(entry, st->entry, sizeof(entry) - 1);
        } else {
            const char* raw = (const char*)st->data;
            if (st->stage == gfx_shader_vertex)
                metal_find_entry(raw, "vertex", entry, sizeof(entry));
            else if (st->stage == gfx_shader_fragment)
                metal_find_entry(raw, "fragment", entry, sizeof(entry));
            else if (st->stage == gfx_shader_compute)
                metal_find_entry(raw, "kernel", entry, sizeof(entry));
        }
        if (entry[0] == 0) strncpy(entry, "main0", sizeof(entry) - 1);

        id<MTLFunction> fn = [lib newFunctionWithName:[NSString stringWithUTF8String:entry]];
        if (!fn) {
            mctx->dbglog(gfx_msg_error, "metal: entry '%s' not found", entry);
            continue;
        }

        switch (st->stage) {
            case gfx_shader_vertex:   mshader->vertex_func = fn; break;
            case gfx_shader_fragment: mshader->fragment_func = fn; break;
            case gfx_shader_compute:  mshader->compute_func = fn; break;
            default: break;
        }
    }

    // Optional reflection via temporary PSO (vertex+fragment only)
    if (mshader->vertex_func && mshader->fragment_func) {
        MTLRenderPipelineDescriptor* pd = [MTLRenderPipelineDescriptor new];
        pd.vertexFunction = mshader->vertex_func;
        pd.fragmentFunction = mshader->fragment_func;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        MTLRenderPipelineReflection* reflection = nil;
        NSError* perr = nil;
        id<MTLRenderPipelineState> tmp = [mctx->device
            newRenderPipelineStateWithDescriptor:pd
                                         options:MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
                                      reflection:&reflection
                                           error:&perr];
        (void)tmp;

        uint32_t idx = 0;
        if (reflection) {
            for (MTLArgument* arg in reflection.vertexArguments) {
                if (idx >= 32) break;
                const char* n = [arg.name UTF8String];
                if (n && strstr(n, "vertexBuffer.")) continue;
                metal_parse_arg(arg, &mshader->uniforms[idx], gfx_shader_vertex);
                ++idx;
            }
            for (MTLArgument* arg in reflection.fragmentArguments) {
                if (idx >= 32) break;
                metal_parse_arg(arg, &mshader->uniforms[idx], gfx_shader_fragment);
                ++idx;
            }
        }
        mshader->uniform_count = metal_merge_uniforms(mshader->uniforms, idx);
        mshader->hash = (uint16_t)(gfx_utils_hash(mshader->uniforms,
            mshader->uniform_count * sizeof(gfx_uniform_t), 0xC0FFEEu) & 0xFFFF);
    }

    // If user provided reflection, prefer it
    if (desc->uniform_count && desc->uniforms) {
        uint32_t n = desc->uniform_count < 32 ? desc->uniform_count : 32;
        memcpy(mshader->uniforms, desc->uniforms, n * sizeof(gfx_uniform_t));
        mshader->uniform_count = n;
    }

    *out_shader = &mshader->handle;
}

static uint32_t metal_shader_get_descriptor_set_count(gfx_shader_t* shader) {
    (void)shader;
    return 1; // Metal maps everything into set 0 style bindings
}

static uint32_t metal_shader_get_uniforms(gfx_shader_t* shader, uint32_t set_index, gfx_uniform_t* uniforms) {
    metal_shader_t* mshader = (metal_shader_t*)shader;
    if (!mshader || set_index != 0) return 0;
    if (uniforms) {
        memcpy(uniforms, mshader->uniforms, mshader->uniform_count * sizeof(gfx_uniform_t));
    }
    return mshader->uniform_count;
}

struct metal_uniform_handle_t {
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

static uint64_t metal_uniform_location(gfx_shader_t* shader, const char* name) {
    metal_shader_t* mshader = (metal_shader_t*)shader;
    if (!mshader || !name) return 0;

    for (uint32_t i = 0; i < mshader->uniform_count; ++i) {
        gfx_uniform_t* u = &mshader->uniforms[i];
        if (strcmp(u->name, name) == 0 || strstr(u->name, name)) {
            metal_uniform_handle_t h = {};
            h.hash = mshader->hash;
            h.type = (uint16_t)u->type;
            h.idx = (uint16_t)i;
            h.field = 0;
            return h.handle;
        }
        for (uint32_t j = 0; j < u->buffer.field_count; ++j) {
            if (strcmp(u->buffer.fields[j].name, name) == 0 || strstr(u->buffer.fields[j].name, name)) {
                metal_uniform_handle_t h = {};
                h.hash = mshader->hash;
                h.type = (uint16_t)u->type;
                h.idx = (uint16_t)i;
                h.field = (uint16_t)j;
                return h.handle;
            }
        }
    }
    return 0;
}

static void metal_shader_destroy(gfx_context_t* ctx, gfx_shader_t* shader) {
    (void)ctx;
    metal_shader_t* mshader = (metal_shader_t*)shader;
    if (!mshader) return;
    mshader->vertex_func = nil;
    mshader->fragment_func = nil;
    mshader->compute_func = nil;
    // pools intentionally leaked for now if still referenced — destroy pool when all free
    free(mshader);
}

// ============================================================================
// SAMPLER / TEXTURE
// ============================================================================

static void metal_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_sampler) return;

    MTLSamplerDescriptor* sd = [MTLSamplerDescriptor new];
    sd.minFilter = metal_filter_map[desc->minmag];
    sd.magFilter = metal_filter_map[desc->minmag];
    sd.mipFilter = metal_mip_map[desc->mipmap];
    sd.sAddressMode = metal_address_map[desc->mode];
    sd.tAddressMode = metal_address_map[desc->mode];
    sd.rAddressMode = metal_address_map[desc->mode];
    sd.maxAnisotropy = desc->anisotropy > 0 ? desc->anisotropy : 1;

    metal_sampler_t* s = (metal_sampler_t*)calloc(1, sizeof(metal_sampler_t));
    s->sampler = [mctx->device newSamplerStateWithDescriptor:sd];
    *out_sampler = &s->handle;
}

static void metal_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler) {
    (void)ctx;
    metal_sampler_t* s = (metal_sampler_t*)sampler;
    if (!s) return;
    s->sampler = nil;
    free(s);
}

static void metal_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** out_texture) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_texture) return;

    MTLPixelFormat fmt = metal_pixel_format(desc->format);
    if (fmt == MTLPixelFormatInvalid) {
        mctx->dbglog(gfx_msg_error, "metal: unsupported pixel format %d", (int)desc->format);
        *out_texture = nullptr;
        return;
    }

    uint32_t mips = desc->mip_levels ? desc->mip_levels : 1;
    MTLTextureDescriptor* td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:fmt
                                     width:desc->width
                                    height:desc->height
                                 mipmapped:(mips > 1)];
    td.mipmapLevelCount = mips;
    td.usage = MTLTextureUsageShaderRead;
    if (desc->usage_flags & gfx_texture_usage_render_target)
        td.usage |= MTLTextureUsageRenderTarget;
    if (desc->usage_flags & gfx_texture_usage_storage)
        td.usage |= MTLTextureUsageShaderWrite;
    td.storageMode = MTLStorageModeShared;

    if (desc->type == gfx_texture2d_cube) {
        td.textureType = MTLTextureTypeCube;
        td.arrayLength = 1;
    } else if (desc->type == gfx_texture2d_array) {
        td.textureType = MTLTextureType2DArray;
        td.arrayLength = desc->array_layers ? desc->array_layers : (desc->depth ? desc->depth : 1);
    } else if (desc->type == gfx_texture3d) {
        td.textureType = MTLTextureType3D;
        td.depth = desc->depth ? desc->depth : 1;
    }

    id<MTLTexture> texture = [mctx->device newTextureWithDescriptor:td];
    if (!texture) {
        mctx->dbglog(gfx_msg_error, "metal: texture create failed");
        *out_texture = nullptr;
        return;
    }
    if (desc->label)
        texture.label = [NSString stringWithUTF8String:desc->label];

    if (desc->data) {
        uintptr_t offset = 0;
        uint32_t layers = 1;
        if (desc->type == gfx_texture2d_cube) layers = 6;
        else if (desc->type == gfx_texture2d_array) layers = td.arrayLength;
        else if (desc->type == gfx_texture3d) layers = 1;

        for (uint32_t layer = 0; layer < layers; ++layer) {
            for (uint32_t level = 0; level < mips; ++level) {
                uint32_t w = gfx_max(desc->width >> level, 1u);
                uint32_t h = gfx_max(desc->height >> level, 1u);
                uint32_t d = (desc->type == gfx_texture3d) ? gfx_max(desc->depth >> level, 1u) : 1u;
                uint32_t size = gfx_utils_image_layer_size(w, h, d, desc->format);
                uint32_t bpr = metal_is_compressed(desc->format) ? 0 : gfx_utils_image_row_pitch(desc->format, w);
                const uint8_t* pixels = (const uint8_t*)desc->data + offset;
                offset += size;

                if (desc->type == gfx_texture3d) {
                    MTLRegion region = MTLRegionMake3D(0, 0, 0, w, h, d);
                    uint32_t bpi = bpr * h;
                    [texture replaceRegion:region mipmapLevel:level slice:0
                                 withBytes:pixels bytesPerRow:bpr bytesPerImage:bpi];
                } else {
                    MTLRegion region = MTLRegionMake2D(0, 0, w, h);
                    [texture replaceRegion:region mipmapLevel:level slice:layer
                                 withBytes:pixels bytesPerRow:bpr bytesPerImage:0];
                }
            }
        }
    }

    metal_texture_t* mtex = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
    mtex->texture = texture;
    mtex->format = desc->format;
    mtex->width = desc->width;
    mtex->height = desc->height;
    mtex->mip_levels = mips;
    *out_texture = &mtex->handle;
}

static void metal_texture_update_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_texture_t* mtex = (metal_texture_t*)texture;
    if (!mtex || !mtex->texture || !data) return;
    (void)size; (void)offset;

    // Full-mip0 upload for simplicity
    uint32_t bpr = metal_is_compressed(mtex->format) ? 0
                    : gfx_utils_image_row_pitch(mtex->format, mtex->width);
    MTLRegion region = MTLRegionMake2D(0, 0, mtex->width, mtex->height);
    [mtex->texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bpr];
    (void)mctx;
}

static void metal_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)texture; (void)idx;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_texture_update_bindless");
}

static void metal_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_texture_t* mtex = (metal_texture_t*)texture;
    if (!mctx || !mtex || !mtex->texture) return;
    if (mtex->texture.mipmapLevelCount <= 1) return;

    id<MTLCommandBuffer> cbuf = [mctx->cmd_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cbuf blitCommandEncoder];
    [blit generateMipmapsForTexture:mtex->texture];
    [blit endEncoding];
    [cbuf commit];
    [cbuf waitUntilCompleted];
}

static void metal_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_texture_t* msrc = (metal_texture_t*)src;
    metal_texture_t* mdst = (metal_texture_t*)dst;
    if (!mctx || !msrc || !mdst || !msrc->texture || !mdst->texture) return;

    id<MTLCommandBuffer> cbuf = [mctx->cmd_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cbuf blitCommandEncoder];
    [blit copyFromTexture:msrc->texture toTexture:mdst->texture];
    [blit endEncoding];
    [cbuf commit];
    [cbuf waitUntilCompleted];
}

static void metal_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)cmd;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_texture_get_data");
}

static void metal_texture_destroy(gfx_context_t* ctx, gfx_texture_t* texture) {
    (void)ctx;
    metal_texture_t* mtex = (metal_texture_t*)texture;
    if (!mtex) return;
    if (!mtex->is_swapchain_owned)
        mtex->texture = nil;
    free(mtex);
}

// ============================================================================
// PIPELINE
// ============================================================================

static MTLBlendFactor metal_blend_factor(gfx_blend_mode m) {
    switch (m) {
        case gfx_blend_mode_zero:          return MTLBlendFactorZero;
        case gfx_blend_mode_one:           return MTLBlendFactorOne;
        case gfx_blend_mode_src_color:     return MTLBlendFactorSourceColor;
        case gfx_blend_mode_inv_src_color: return MTLBlendFactorOneMinusSourceColor;
        case gfx_blend_mode_src_alpha:     return MTLBlendFactorSourceAlpha;
        case gfx_blend_mode_inv_src_alpha: return MTLBlendFactorOneMinusSourceAlpha;
        case gfx_blend_mode_dst_alpha:     return MTLBlendFactorDestinationAlpha;
        case gfx_blend_mode_inv_dest_alpha:return MTLBlendFactorOneMinusDestinationAlpha;
        case gfx_blend_mode_dst_color:     return MTLBlendFactorDestinationColor;
        case gfx_blend_mode_inv_dst_color: return MTLBlendFactorOneMinusDestinationColor;
        default:                           return MTLBlendFactorOne;
    }
}

static MTLBlendOperation metal_blend_op(gfx_blend_op op) {
    switch (op) {
        case gfx_blend_op_add:          return MTLBlendOperationAdd;
        case gfx_blend_op_min:          return MTLBlendOperationMin;
        case gfx_blend_op_max:          return MTLBlendOperationMax;
        case gfx_blend_op_subtract:     return MTLBlendOperationSubtract;
        case gfx_blend_op_rev_subtract: return MTLBlendOperationReverseSubtract;
        default:                        return MTLBlendOperationAdd;
    }
}

static void metal_pipeline_create(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_pipeline) return;

    metal_shader_t* mshader = (metal_shader_t*)desc->shader;
    if (!mshader || !mshader->vertex_func || !mshader->fragment_func) {
        mctx->dbglog(gfx_msg_error, "metal: pipeline requires vertex+fragment functions");
        *out_pipeline = nullptr;
        return;
    }

    MTLVertexDescriptor* vd = [[MTLVertexDescriptor alloc] init];
    for (uint32_t i = 0; i < desc->assembly.attribute_count; ++i) {
        gfx_vertex_attribute* a = &desc->assembly.attributes[i];
        vd.attributes[a->location].format = metal_vertex_format(a->format);
        vd.attributes[a->location].offset = a->offset;
        vd.attributes[a->location].bufferIndex = a->binding;
    }
    for (uint32_t i = 0; i < desc->assembly.slot_count; ++i) {
        gfx_vertex_slot_t* s = &desc->assembly.slots[i];
        vd.layouts[s->binding].stride = s->stride;
        vd.layouts[s->binding].stepRate = 1;
        vd.layouts[s->binding].stepFunction =
            (s->rate == gfx_vertex_rate_instance)
                ? MTLVertexStepFunctionPerInstance
                : MTLVertexStepFunctionPerVertex;
    }

    MTLRenderPipelineDescriptor* pd = [MTLRenderPipelineDescriptor new];
    pd.vertexFunction = mshader->vertex_func;
    pd.fragmentFunction = mshader->fragment_func;
    pd.vertexDescriptor = vd;
    pd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    gfx_render_states_desc_t* rs = &desc->render_states;
    if ((rs->states & gfx_blend) || rs->blend.enable) {
        pd.colorAttachments[0].blendingEnabled = YES;
        pd.colorAttachments[0].sourceRGBBlendFactor = metal_blend_factor(rs->blend.color_src);
        pd.colorAttachments[0].destinationRGBBlendFactor = metal_blend_factor(rs->blend.color_dst);
        pd.colorAttachments[0].rgbBlendOperation = metal_blend_op(rs->blend.color_op);
        pd.colorAttachments[0].sourceAlphaBlendFactor = metal_blend_factor(rs->blend.alpha_src);
        pd.colorAttachments[0].destinationAlphaBlendFactor = metal_blend_factor(rs->blend.alpha_dst);
        pd.colorAttachments[0].alphaBlendOperation = metal_blend_op(rs->blend.alpha_op);
    }

    MTLColorWriteMask mask = MTLColorWriteMaskNone;
    if (rs->color_mask.red)   mask |= MTLColorWriteMaskRed;
    if (rs->color_mask.green) mask |= MTLColorWriteMaskGreen;
    if (rs->color_mask.blue)  mask |= MTLColorWriteMaskBlue;
    if (rs->color_mask.alpha) mask |= MTLColorWriteMaskAlpha;
    if (rs->states & gfx_colormask_rgba)
        mask = MTLColorWriteMaskAll;
    if (mask == MTLColorWriteMaskNone)
        mask = MTLColorWriteMaskAll;
    pd.colorAttachments[0].writeMask = mask;

    NSError* error = nil;
    id<MTLRenderPipelineState> pso = [mctx->device newRenderPipelineStateWithDescriptor:pd error:&error];
    if (!pso) {
        mctx->dbglog(gfx_msg_error, "metal PSO failed: %s",
                     error ? [[error localizedDescription] UTF8String] : "?");
        *out_pipeline = nullptr;
        return;
    }

    MTLDepthStencilDescriptor* dd = [[MTLDepthStencilDescriptor alloc] init];
    bool depth_test = (rs->states & gfx_depth_test) || rs->depth.enable;
    bool depth_write = (rs->states & gfx_depth_write) || rs->depth.write;
    dd.depthCompareFunction = depth_test ? metal_cmp_map[rs->depth.mode] : MTLCompareFunctionAlways;
    dd.depthWriteEnabled = depth_write ? YES : NO;

    metal_pipeline_t* mp = (metal_pipeline_t*)calloc(1, sizeof(metal_pipeline_t));
    mp->pipeline = pso;
    mp->depth_state = [mctx->device newDepthStencilStateWithDescriptor:dd];
    mp->face = metal_face_map[rs->face];
    mp->cull_mode = metal_cull_map[rs->culling];
    mp->topology = metal_topology_map[rs->topology];
    *out_pipeline = &mp->handle;
}

static void metal_pipeline_destroy(gfx_context_t* ctx, gfx_pipeline_t* pipeline) {
    (void)ctx;
    metal_pipeline_t* mp = (metal_pipeline_t*)pipeline;
    if (!mp) return;
    mp->pipeline = nil;
    mp->depth_state = nil;
    free(mp);
}

static void metal_pipeline_compute_create(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** out_pipeline) {
    metal_context_t* mctx = from_ctx(ctx);
    if (!mctx || !desc || !out_pipeline) return;

    metal_shader_t* mshader = (metal_shader_t*)desc->shader;
    if (!mshader || !mshader->compute_func) {
        mctx->dbglog(gfx_msg_error, "metal: compute pipeline needs compute function");
        *out_pipeline = nullptr;
        return;
    }

    NSError* error = nil;
    id<MTLComputePipelineState> pso =
        [mctx->device newComputePipelineStateWithFunction:mshader->compute_func error:&error];
    if (!pso) {
        mctx->dbglog(gfx_msg_error, "metal compute PSO failed: %s",
                     error ? [[error localizedDescription] UTF8String] : "?");
        *out_pipeline = nullptr;
        return;
    }

    metal_pipeline_compute_t* mp = (metal_pipeline_compute_t*)calloc(1, sizeof(metal_pipeline_compute_t));
    mp->pipeline = pso;
    *out_pipeline = &mp->handle;
}

static void metal_cmd_begin_compute_pass(gfx_command_buffer_t* cmd) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd) return;
    if (mcmd->encoder || mcmd->compute_encoder) {
        mcmd->mctx->dbglog(gfx_msg_error, "metal: begin_compute_pass while another pass is active");
        return;
    }
    mcmd->compute_encoder = [mcmd->cmd_buffer computeCommandEncoder];
    mcmd->bound_compute_pso = nil;
}

static void metal_cmd_end_compute_pass(gfx_command_buffer_t* cmd) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->compute_encoder) return;
    [mcmd->compute_encoder endEncoding];
    mcmd->compute_encoder = nil;
    mcmd->bound_compute_pso = nil;
}

static void metal_cmd_bind_compute_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_compute_t* pipeline) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_pipeline_compute_t* mp = (metal_pipeline_compute_t*)pipeline;
    if (!mcmd || !mp || !mcmd->compute_encoder) {
        if (mcmd) mcmd->mctx->dbglog(gfx_msg_error, "metal: bind_compute_pipeline outside compute pass");
        return;
    }
    mcmd->bound_compute_pso = mp->pipeline;
    [mcmd->compute_encoder setComputePipelineState:mp->pipeline];
}

static void metal_pipeline_compute_destroy(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline) {
    (void)ctx;
    metal_pipeline_compute_t* mp = (metal_pipeline_compute_t*)pipeline;
    if (!mp) return;
    mp->pipeline = nil;
    free(mp);
}

static void metal_mesh_pipeline_create(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)desc;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_mesh_pipeline_create");
    if (out_pipeline) *out_pipeline = nullptr;
}

static void metal_pipeline_raytrace_create(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** out_pipeline) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)desc;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_pipeline_raytrace_create");
    if (out_pipeline) *out_pipeline = nullptr;
}

static void metal_pipeline_raytrace_destroy(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline) {
    (void)ctx; (void)pipeline;
}

// ============================================================================
// DESCRIPTOR SET
// ============================================================================

static void metal_create_descriptor_set_pool(metal_context_t* mctx, metal_shader_t* shader,
                                            uint32_t capacity, metal_descriptor_set_pool_t** out_pool) {
    metal_descriptor_set_pool_t* pool = (metal_descriptor_set_pool_t*)calloc(1, sizeof(metal_descriptor_set_pool_t));
    *out_pool = pool;

    uint32_t ubo_size = 0;
    for (uint32_t i = 0; i < shader->uniform_count; ++i) {
        if (shader->uniforms[i].type == gfx_uniform_ubo)
            ubo_size += shader->uniforms[i].buffer.size;
    }
    if (ubo_size == 0) ubo_size = 256;
    ubo_size = gfx_utils_align_up(ubo_size, 256);

    pool->capacity = capacity;
    pool->free_set_count = capacity;
    pool->next_free = 0;

    gfx_buffer_desc_t bdesc = {};
    bdesc.usage = gfx_buffer_usage_uniform;
    bdesc.memory_hint = gfx_memory_cpu_to_gpu;
    bdesc.mapped = true;
    bdesc.size = ubo_size * capacity;
    metal_buffer_create(&mctx->handle, &bdesc, &pool->ubo);
    metal_buffer_t* mbuffer = (metal_buffer_t*)pool->ubo;

    pool->ubo_buffer_data_size = ubo_size;
    pool->ubo_buffer_data_ptr = mbuffer ? [mbuffer->buffer contents] : nullptr;
    pool->writes = (metal_writes_t*)calloc(capacity * shader->uniform_count, sizeof(metal_writes_t));
    pool->descriptor_sets = (metal_descriptor_set_t*)calloc(capacity, sizeof(metal_descriptor_set_t));

    for (uint32_t i = 0; i < capacity; ++i) {
        metal_descriptor_set_t* ds = &pool->descriptor_sets[i];
        ds->isfree = 1;
        ds->pool = pool;
        ds->shader = shader;
        ds->ubo_ptr = pool->ubo_buffer_data_ptr
                        ? (uint8_t*)pool->ubo_buffer_data_ptr + i * ubo_size
                        : nullptr;
        ds->writes = pool->writes + i * shader->uniform_count;
        ds->writes_count = shader->uniform_count;

        for (uint32_t j = 0; j < shader->uniform_count; ++j) {
            ds->writes[j].stage_mask = shader->uniforms[j].stage_mask;
            ds->writes[j].type = shader->uniforms[j].type;
            ds->writes[j].slot = shader->uniforms[j].binding;
            switch (shader->uniforms[j].type) {
                case gfx_uniform_ubo:
                    ds->writes[j].offset = (uint16_t)(i * ubo_size);
                    ds->writes[j].buffer = mbuffer ? mbuffer->buffer : nil;
                    break;
                case gfx_uniform_sampler:
                    ds->writes[j].sampler = ((metal_sampler_t*)mctx->default_sampler)->sampler;
                    break;
                case gfx_uniform_texture2d:
                case gfx_uniform_texture2d_cube:
                case gfx_uniform_texture2d_array:
                case gfx_uniform_texture3d:
                    ds->writes[j].texture = ((metal_texture_t*)mctx->default_texture)->texture;
                    break;
                default:
                    break;
            }
        }
    }
}

static void metal_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx,
                                        gfx_descriptor_set_t** out_descriptor) {
    metal_context_t* mctx = from_ctx(ctx);
    metal_shader_t* mshader = (metal_shader_t*)shader;
    (void)set_idx;
    if (!mctx || !mshader || !out_descriptor) return;

    if (!mshader->current_pool)
        metal_create_descriptor_set_pool(mctx, mshader, 2048, &mshader->current_pool);

    if (mshader->current_pool->free_set_count == 0)
        metal_create_descriptor_set_pool(mctx, mshader, 2048, &mshader->current_pool);

    metal_descriptor_set_pool_t* pool = mshader->current_pool;
    for (uint32_t i = pool->next_free; i < pool->capacity; ++i) {
        if (pool->descriptor_sets[i].isfree) {
            pool->descriptor_sets[i].isfree = 0;
            pool->count++;
            pool->free_set_count--;
            pool->next_free = i + 1;
            *out_descriptor = &pool->descriptor_sets[i].handle;
            return;
        }
    }
    *out_descriptor = nullptr;
}

static void metal_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle,
                                                   void* data, uint32_t size) {
    if (!set || !handle || !data) return;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = mset->shader;
    metal_uniform_handle_t uh = { handle };
    if (uh.hash != mshader->hash) return;
    if (uh.type != gfx_uniform_ubo) return;

    gfx_uniform_t* u = &mshader->uniforms[uh.idx];
    uint32_t offset = 0;
    if (uh.field < u->buffer.field_count)
        offset = u->buffer.fields[uh.field].offset;
    if (mset->ubo_ptr)
        memcpy((uint8_t*)mset->ubo_ptr + offset, data, size);
}

static void metal_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle,
                                              gfx_buffer_t* buffer, uint32_t offset) {
    if (!set || !handle || !buffer) return;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = mset->shader;
    metal_uniform_handle_t uh = { handle };
    if (uh.hash != mshader->hash) return;

    metal_buffer_t* mbuf = (metal_buffer_t*)buffer;
    mset->writes[uh.idx].buffer = mbuf->buffer;
    mset->writes[uh.idx].offset = (uint16_t)offset;
}

static void metal_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle,
                                               gfx_texture_t* texture) {
    if (!set || !handle || !texture) return;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = mset->shader;
    metal_uniform_handle_t uh = { handle };
    if (uh.hash != mshader->hash) return;

    metal_texture_t* mtex = (metal_texture_t*)texture;
    mset->writes[uh.idx].texture = mtex->texture;
}

static void metal_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle,
                                               gfx_sampler_t* sampler) {
    if (!set || !handle || !sampler) return;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = mset->shader;
    metal_uniform_handle_t uh = { handle };
    if (uh.hash != mshader->hash) return;

    metal_sampler_t* ms = (metal_sampler_t*)sampler;
    mset->writes[uh.idx].sampler = ms->sampler;
}

static void metal_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor) {
    (void)ctx;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)descriptor;
    if (!mset || !mset->pool) return;
    mset->isfree = 1;
    mset->pool->free_set_count++;
    if (mset->pool->next_free > 0)
        mset->pool->next_free = 0; // simple reclaim
}

// ============================================================================
// COMMAND BUFFER
// ============================================================================

static void metal_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* info) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !info) return;

    if (!mcmd->cmd_buffer)
        mcmd->cmd_buffer = [mcmd->mctx->cmd_queue commandBuffer];

    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];

    // Color attachments (explicit or fallback to swapchain drawable)
    bool any_color = false;
    for (int i = 0; i < 4; ++i) {
        id<MTLTexture> tex = nil;
        if (info->color_attachment[i]) {
            metal_texture_t* mtex = (metal_texture_t*)info->color_attachment[i];
            tex = mtex->texture;
        } else if (i == 0 && mcmd->default_color) {
            tex = mcmd->default_color;
        }
        if (!tex) continue;
        any_color = true;

        pass.colorAttachments[i].texture = tex;
        pass.colorAttachments[i].storeAction = MTLStoreActionStore;

        switch (info->color_load_op[i]) {
            case gfx_load_op_clear: {
                // Packed RGBA8 as 0xRRGGBBAA in high-to-low byte order of the uint32 value
                uint32_t c = info->color_clear_value;
                float r = ((c >> 24) & 0xFF) / 255.0f;
                float g = ((c >> 16) & 0xFF) / 255.0f;
                float b = ((c >> 8)  & 0xFF) / 255.0f;
                float a = ((c >> 0)  & 0xFF) / 255.0f;
                pass.colorAttachments[i].loadAction = MTLLoadActionClear;
                pass.colorAttachments[i].clearColor = MTLClearColorMake(r, g, b, a);
            } break;
            case gfx_load_op_load:
                pass.colorAttachments[i].loadAction = MTLLoadActionLoad;
                break;
            default:
                // Default to clear for swapchain-style first use if unset
                if (i == 0 && !info->color_attachment[0]) {
                    pass.colorAttachments[i].loadAction = MTLLoadActionClear;
                    pass.colorAttachments[i].clearColor = MTLClearColorMake(0.1, 0.1, 0.12, 1.0);
                } else {
                    pass.colorAttachments[i].loadAction = MTLLoadActionDontCare;
                }
                break;
        }
    }
    (void)any_color;

    // Depth
    id<MTLTexture> depth_tex = nil;
    if (info->depth_attachment) {
        metal_texture_t* dtex = (metal_texture_t*)info->depth_attachment;
        depth_tex = dtex->texture;
    } else if (mcmd->default_depth) {
        depth_tex = mcmd->default_depth;
    }
    if (depth_tex) {
        pass.depthAttachment.texture = depth_tex;
        pass.depthAttachment.storeAction = MTLStoreActionStore;
        switch (info->depth_load_op) {
            case gfx_load_op_clear:
                pass.depthAttachment.loadAction = MTLLoadActionClear;
                pass.depthAttachment.clearDepth = info->depth_clear_value != 0.0f
                    ? info->depth_clear_value : 1.0;
                break;
            case gfx_load_op_load:
                pass.depthAttachment.loadAction = MTLLoadActionLoad;
                break;
            default:
                pass.depthAttachment.loadAction = MTLLoadActionClear;
                pass.depthAttachment.clearDepth = 1.0;
                break;
        }
    }

    mcmd->encoder = [mcmd->cmd_buffer renderCommandEncoderWithDescriptor:pass];
}

static void metal_cmd_end_pass(gfx_command_buffer_t* cmd) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder) return;
    [mcmd->encoder endEncoding];
    mcmd->encoder = nil;
    mcmd->active_pool = nullptr;
}

static void metal_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder) return;
    MTLScissorRect rc = { x, y, w, h };
    [mcmd->encoder setScissorRect:rc];
}

static void metal_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder) return;
    MTLViewport vp = { (double)x, (double)y, (double)w, (double)h, 0.0, 1.0 };
    [mcmd->encoder setViewport:vp];
}

static void metal_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_pipeline_t* mp = (metal_pipeline_t*)pipeline;
    if (!mcmd || !mcmd->encoder || !mp) return;

    [mcmd->encoder setFrontFacingWinding:mp->face];
    [mcmd->encoder setCullMode:mp->cull_mode];
    [mcmd->encoder setRenderPipelineState:mp->pipeline];
    if (mp->depth_state)
        [mcmd->encoder setDepthStencilState:mp->depth_state];
    mcmd->topology = mp->topology;
}

static void metal_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor) {
    (void)slot;
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)descriptor;
    if (!mcmd || !mcmd->encoder || !mset) return;

    bool offset_only = (mcmd->active_pool == mset->pool);
    mcmd->active_pool = mset->pool;

    for (uint32_t i = 0; i < mset->writes_count; ++i) {
        metal_writes_t* w = &mset->writes[i];
        switch (w->type) {
            case gfx_uniform_ubo: {
                if (w->stage_mask & (1u << gfx_shader_vertex)) {
                    if (offset_only)
                        [mcmd->encoder setVertexBufferOffset:w->offset atIndex:w->slot];
                    else
                        [mcmd->encoder setVertexBuffer:w->buffer offset:w->offset atIndex:w->slot];
                }
                if (w->stage_mask & (1u << gfx_shader_fragment)) {
                    if (offset_only)
                        [mcmd->encoder setFragmentBufferOffset:w->offset atIndex:w->slot];
                    else
                        [mcmd->encoder setFragmentBuffer:w->buffer offset:w->offset atIndex:w->slot];
                }
            } break;
            case gfx_uniform_sampler: {
                if (w->stage_mask & (1u << gfx_shader_vertex))
                    [mcmd->encoder setVertexSamplerState:w->sampler atIndex:w->slot];
                if (w->stage_mask & (1u << gfx_shader_fragment))
                    [mcmd->encoder setFragmentSamplerState:w->sampler atIndex:w->slot];
            } break;
            case gfx_uniform_texture2d:
            case gfx_uniform_texture2d_cube:
            case gfx_uniform_texture2d_array:
            case gfx_uniform_texture3d: {
                if (w->stage_mask & (1u << gfx_shader_vertex))
                    [mcmd->encoder setVertexTexture:w->texture atIndex:w->slot];
                if (w->stage_mask & (1u << gfx_shader_fragment))
                    [mcmd->encoder setFragmentTexture:w->texture atIndex:w->slot];
            } break;
            default: break;
        }
    }
}

static void metal_cmd_bind_index_buffer(gfx_command_buffer_t* cmd, gfx_index_format format,
                                        uint32_t offset, gfx_buffer_t* buffer) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_buffer_t* mbuf = (metal_buffer_t*)buffer;
    if (!mcmd || !mbuf) return;
    mcmd->index_type = (format == gfx_index_format_16) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
    mcmd->index_buffer = mbuf->buffer;
    mcmd->index_buffer_offset = offset;
}

static void metal_cmd_bind_vertex_buffer(gfx_command_buffer_t* cmd, uint32_t slot,
                                         uint32_t offset, gfx_buffer_t* buffer) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_buffer_t* mbuf = (metal_buffer_t*)buffer;
    if (!mcmd || !mcmd->encoder || !mbuf) return;
    [mcmd->encoder setVertexBuffer:mbuf->buffer offset:offset atIndex:slot];
}

static void metal_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder) return;
    [mcmd->encoder drawPrimitives:mcmd->topology
                      vertexStart:0
                      vertexCount:vertex_count
                    instanceCount:instance_count ? instance_count : 1];
}

static void metal_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx,
                                   uint32_t instance_count, uint32_t vertex_offset) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder || !mcmd->index_buffer) return;

    NSUInteger index_size = (mcmd->index_type == MTLIndexTypeUInt16) ? 2 : 4;
    NSUInteger off = mcmd->index_buffer_offset + first_idx * index_size;

    [mcmd->encoder drawIndexedPrimitives:mcmd->topology
                              indexCount:idx_count
                               indexType:mcmd->index_type
                             indexBuffer:mcmd->index_buffer
                       indexBufferOffset:off
                           instanceCount:instance_count ? instance_count : 1
                              baseVertex:(NSInteger)vertex_offset
                            baseInstance:0];
}

static void metal_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer,
                                            uint32_t offset, uint32_t draw_count, uint32_t stride) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)buffer; (void)offset; (void)draw_count; (void)stride;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr,
                             "metal_cmd_draw_indexed_indirect");
}

static void metal_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->compute_encoder || !mcmd->bound_compute_pso) {
        if (mcmd && mcmd->mctx) mcmd->mctx->dbglog(gfx_msg_error, "metal: dispatch needs active compute pass and bound pipeline");
        return;
    }
    // TODO: threadgroup size should come from shader reflection (numthreads); this is a placeholder
    // matching the pipeline's SIMD width, and only correct if the shader declares the same local size.
    NSUInteger w = mcmd->bound_compute_pso.threadExecutionWidth;
    [mcmd->compute_encoder dispatchThreadgroups:MTLSizeMake(x, y, z)
                          threadsPerThreadgroup:MTLSizeMake(w, 1, 1)];
}

static void metal_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder || !marker) return;
    [mcmd->encoder pushDebugGroup:[NSString stringWithUTF8String:marker]];
}

static void metal_cmd_pop_marker(gfx_command_buffer_t* cmd) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    if (!mcmd || !mcmd->encoder) return;
    [mcmd->encoder popDebugGroup];
}

static void metal_cmd_get_timestamps(gfx_command_buffer_t* cmd, gfx_timestamp_t* timestamps) {
    (void)cmd; (void)timestamps;
}

static void metal_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count,
                                     gfx_barrier src, gfx_barrier dst) {
    (void)cmd; (void)buffers; (void)count; (void)src; (void)dst;
    // Metal tracks hazards via encoder boundaries / resource usage in most cases
}

static void metal_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count,
                                      gfx_barrier src, gfx_barrier dst) {
    (void)cmd; (void)textures; (void)count; (void)src; (void)dst;
}

static void metal_cmd_draw_mesh_tasks(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)x; (void)y; (void)z;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr, "metal_cmd_draw_mesh_tasks");
}

static void metal_cmd_draw_mesh_tasks_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer,
                                               uint32_t offset, uint32_t draw_count, uint32_t stride) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)buffer; (void)offset; (void)draw_count; (void)stride;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr, "metal_cmd_draw_mesh_tasks_indirect");
}

static void metal_acceleration_structure_create(gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc,
                                                gfx_acceleration_structure_t** out_as) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)desc;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_acceleration_structure_create");
    if (out_as) *out_as = nullptr;
}

static void metal_acceleration_structure_destroy(gfx_context_t* ctx, gfx_acceleration_structure_t* as) {
    (void)ctx; (void)as;
}

static void metal_sbt_create(gfx_context_t* ctx, gfx_sbt_desc_t* desc, gfx_sbt_t** out_sbt) {
    metal_context_t* mctx = from_ctx(ctx);
    (void)desc;
    gfx_stub_not_implemented(mctx ? mctx->dbglog : nullptr, "metal_sbt_create");
    if (out_sbt) *out_sbt = nullptr;
}

static void metal_sbt_destroy(gfx_context_t* ctx, gfx_sbt_t* sbt) {
    (void)ctx; (void)sbt;
}

static void metal_cmd_build_acceleration_structure(gfx_command_buffer_t* cmd,
                                                   gfx_acceleration_structure_t* dst,
                                                   gfx_acceleration_structure_t* src) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)dst; (void)src;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr,
                             "metal_cmd_build_acceleration_structure");
}

static void metal_cmd_trace_rays(gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline,
                                 gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)pipeline; (void)sbt; (void)width; (void)height; (void)depth;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr, "metal_cmd_trace_rays");
}

static void metal_cmd_trace_ray_query(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t tlas,
                                      uint32_t width, uint32_t height, uint32_t depth) {
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    (void)tlas; (void)width; (void)height; (void)depth;
    gfx_stub_not_implemented(mcmd && mcmd->mctx ? mcmd->mctx->dbglog : nullptr, "metal_cmd_trace_ray_query");
}

// ============================================================================
// Function table
// ============================================================================

#ifdef __cplusplus
extern "C"
#endif
void gfx_init_metal(gfx_api_pfn* func_table) {
    if (!func_table) return;
    memset(func_table, 0, sizeof(gfx_api_pfn));

    func_table->pfn_init     = metal_init;
    func_table->pfn_destroy  = metal_destroy;
    func_table->pfn_get_caps = metal_get_caps;

    func_table->pfn_surface_create  = metal_surface_create;
    func_table->pfn_surface_destroy = metal_surface_destroy;
    func_table->pfn_frame_begin     = metal_frame_begin;
    func_table->pfn_frame_end       = metal_frame_end;

    func_table->pfn_buffer_create      = metal_buffer_create;
    func_table->pfn_buffer_update_data = metal_buffer_update_data;
    func_table->pfn_buffer_destroy     = metal_buffer_destroy;

    func_table->pfn_shader_create                   = metal_shader_create;
    func_table->pfn_shader_get_descriptor_set_count = metal_shader_get_descriptor_set_count;
    func_table->pfn_shader_get_uniforms             = metal_shader_get_uniforms;
    func_table->pfn_uniform_location                = metal_uniform_location;
    func_table->pfn_shader_destroy                  = metal_shader_destroy;

    func_table->pfn_descriptor_set_create            = metal_descriptor_set_create;
    func_table->pfn_descriptor_set_write_buffer_data = metal_descriptor_set_write_buffer_data;
    func_table->pfn_descriptor_set_write_buffer      = metal_descriptor_set_write_buffer;
    func_table->pfn_descriptor_set_write_texture     = metal_descriptor_set_write_texture;
    func_table->pfn_descriptor_set_write_sampler     = metal_descriptor_set_write_sampler;
    func_table->pfn_descriptor_set_destroy           = metal_descriptor_set_destroy;

    func_table->pfn_sampler_create  = metal_sampler_create;
    func_table->pfn_sampler_destroy = metal_sampler_destroy;

    func_table->pfn_texture_create          = metal_texture_create;
    func_table->pfn_texture_update_data     = metal_texture_update_data;
    func_table->pfn_texture_update_bindless = metal_texture_update_bindless;
    func_table->pfn_texture_generate_mipmap = metal_texture_generate_mipmap;
    func_table->pfn_texture_blit            = metal_texture_blit;
    func_table->pfn_texture_get_data        = metal_texture_get_data;
    func_table->pfn_texture_destroy         = metal_texture_destroy;

    func_table->pfn_pipeline_create          = metal_pipeline_create;
    func_table->pfn_mesh_pipeline_create     = metal_mesh_pipeline_create;
    func_table->pfn_pipeline_destroy         = metal_pipeline_destroy;
    func_table->pfn_pipeline_compute_create  = metal_pipeline_compute_create;
    func_table->pfn_pipeline_compute_destroy = metal_pipeline_compute_destroy;
    func_table->pfn_pipeline_raytrace_create = metal_pipeline_raytrace_create;
    func_table->pfn_pipeline_raytrace_destroy= metal_pipeline_raytrace_destroy;

    func_table->pfn_cmd_begin_pass   = metal_cmd_begin_pass;
    func_table->pfn_cmd_end_pass     = metal_cmd_end_pass;
    func_table->pfn_cmd_begin_compute_pass    = metal_cmd_begin_compute_pass;
    func_table->pfn_cmd_end_compute_pass      = metal_cmd_end_compute_pass;
    func_table->pfn_cmd_bind_compute_pipeline = metal_cmd_bind_compute_pipeline;
    func_table->pfn_cmd_scissor      = metal_cmd_scissor;
    func_table->pfn_cmd_viewport     = metal_cmd_viewport;
    func_table->pfn_cmd_bind_pipeline        = metal_cmd_bind_pipeline;
    func_table->pfn_cmd_bind_descriptor_set  = metal_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_index_buffer    = metal_cmd_bind_index_buffer;
    func_table->pfn_cmd_bind_vertex_buffer   = metal_cmd_bind_vertex_buffer;
    func_table->pfn_cmd_draw                 = metal_cmd_draw;
    func_table->pfn_cmd_draw_indexed         = metal_cmd_draw_indexed;
    func_table->pfn_cmd_draw_indexed_indirect= metal_cmd_draw_indexed_indirect;
    func_table->pfn_cmd_dispatch_compute     = metal_cmd_dispatch_compute;

    func_table->pfn_cmd_draw_mesh_tasks          = metal_cmd_draw_mesh_tasks;
    func_table->pfn_cmd_draw_mesh_tasks_indirect = metal_cmd_draw_mesh_tasks_indirect;

    func_table->pfn_acceleration_structure_create  = metal_acceleration_structure_create;
    func_table->pfn_acceleration_structure_destroy = metal_acceleration_structure_destroy;
    func_table->pfn_sbt_create                     = metal_sbt_create;
    func_table->pfn_sbt_destroy                    = metal_sbt_destroy;
    func_table->pfn_cmd_build_acceleration_structure = metal_cmd_build_acceleration_structure;
    func_table->pfn_cmd_trace_rays                 = metal_cmd_trace_rays;
    func_table->pfn_cmd_trace_ray_query            = metal_cmd_trace_ray_query;

    func_table->pfn_cmd_buffer_barrier  = metal_cmd_buffer_barrier;
    func_table->pfn_cmd_texture_barrier = metal_cmd_texture_barrier;
    func_table->pfn_cmd_push_marker     = metal_cmd_push_marker;
    func_table->pfn_cmd_pop_marker      = metal_cmd_pop_marker;
    func_table->pfn_cmd_get_timestamps  = metal_cmd_get_timestamps;
}

#endif // METAL_AVAILABLE
#endif // __gfx_metal_h__
