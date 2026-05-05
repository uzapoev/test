#ifndef __gfx_metal_h__
#define __gfx_metal_h__

#include "gfx.h"

#if METAL_AVAILABLE
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

struct metal_descriptor_set_pool_t;

typedef struct metal_context_t {
    gfx_context_t                   handle;
    MTKView* view;
    CAMetalLayer* metal_layer;
    id<MTLDevice>                   device;
    id<MTLCommandQueue>             cmd_queue;
    id<CAMetalDrawable>             drawable;
    id<MTLTexture>                  depth_texture;

    gfx_caps_t                      caps;
    gfx_callback                    dbglog;

    gfx_sampler_t*                  default_sampler;
    gfx_texture_t*                  default_texture;
} metal_context_t;


typedef struct metal_render_target_t {
    gfx_render_target_t             handle;
    id<MTLTexture>                  color;
    id<MTLTexture>                  depth;
} metal_render_target_t;


typedef struct metal_swapchain_t {
    gfx_swapchain_t                 handle;
    metal_render_target_t*          target;
    id<CAMetalDrawable>             drawable;

    id<MTLTexture>                  color_texture;
    id<MTLTexture>                  depth_texture;
} metal_swapchain_t;


typedef struct metal_command_buffer_t {
    gfx_command_buffer_t            handle;
    metal_context_t* mctx;
    id<MTLCommandBuffer>            cmd_buffer;

    id<CAMetalDrawable>             drawable;
    id<MTLRenderCommandEncoder>     encoder;



    metal_descriptor_set_pool_t*    active_pool; //
    MTLIndexType                    index_type;
    id<MTLBuffer>                   index_buffer;
} metal_command_buffer_t;


typedef struct metal_buffer_t {
    gfx_buffer_t                    handle;
    gfx_buffer_usage                usage;
    id<MTLBuffer>                   buffer;
} metal_buffer_t;


typedef struct metal_sampler_t {
    gfx_sampler_t                   handle;
    id<MTLSamplerState>             sampler;
} metal_sampler_t;

typedef struct metal_texture_t {
    gfx_texture_t                   handle;
    id<MTLTexture>                  texture;
} metal_texture_t;


typedef struct metal_pipeline_t {
    gfx_pipeline_t                  handle;

    MTLWinding                      face;   //ccw, cw
    MTLCullMode                     cull_mode;
    id <MTLRenderPipelineState>     pipeline;
    id <MTLDepthStencilState>       depth_state;
} metal_pipeline_t;


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

    uint8_t                         dirty;
    metal_writes_t*                 writes;
    struct metal_descriptor_set_t*  descriptor_sets;

    gfx_buffer_t*                   ubo;
    uint32_t                        ubo_buffer_data_size;
    void* ubo_buffer_data_ptr;

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
    metal_descriptor_set_pool_t* current_pool;

    uint32_t                        uniform_count;
    gfx_uniform_t                   uniforms[16];


    id<MTLFunction>                 vertex_func;
    id<MTLFunction>                 fragment_func;
    id<MTLFunction>                 compute_func;
} metal_shader_t;


metal_context_t* from_ctx(gfx_context_t* ctx)
{
    return (metal_context_t*)(ctx);
}


gfx_api void     metal_init(gfx_settings_t* settings, gfx_context_t** ctx);
gfx_api void     metal_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t** swapchain);

gfx_api int32_t  metal_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, gfx_render_target_t** target);
gfx_api void     metal_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx);

gfx_api void     metal_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
gfx_api void     metal_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
gfx_api void     metal_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
gfx_api void     metal_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
gfx_api void     metal_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void     metal_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** target);
gfx_api void     metal_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, gfx_descriptor_set_t** descriptor);
gfx_api void     metal_create_cmd(gfx_context_t* ctx, uint32_t count, gfx_command_buffer_t** cmd);

gfx_api void     metal_destroy_buffer(gfx_buffer_t* buffer);
gfx_api void     metal_destroy_shader(gfx_shader_t* buffer);
gfx_api void     metal_destroy_sampler(gfx_sampler_t* sampler);
gfx_api void     metal_destroy_texture(gfx_texture_t* texture);
gfx_api void     metal_destroy_pipeline(gfx_pipeline_t* pipeline);
gfx_api void     metal_destroy_render_target(gfx_render_target_t* _target);
gfx_api void     metal_destroy_descriptor_set(gfx_descriptor_set_t* descriptor);
gfx_api void     metal_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd);


#ifdef METAL_IMPLEMENTATION

bool is_compressed_format(gfx_pixel_format format)
{
    switch (format)
    {
        case gfx_pixel_format_pvrtc_rgb_2bpp:
        case gfx_pixel_format_pvrtc_rgba_2bpp:

        case gfx_pixel_format_pvrtc_rgb_4bpp:
        case gfx_pixel_format_pvrtc_rgba_4bpp: return true;
        default: return false;
    }
    return true;
}

static uint32_t metal_data_type_stride(MTLDataType type)
{
    switch (type)
    {
        case MTLDataTypeInt:  case MTLDataTypeUInt:   case MTLDataTypeFloat:  return sizeof(float) * 1;
        case MTLDataTypeInt2: case MTLDataTypeUInt2:  case MTLDataTypeFloat2: return sizeof(float) * 2;
        case MTLDataTypeInt3: case MTLDataTypeUInt3:  case MTLDataTypeFloat3: return sizeof(float) * 3;
        case MTLDataTypeInt4: case MTLDataTypeUInt4:  case MTLDataTypeFloat4: return sizeof(float) * 4;

        case MTLDataTypeHalf:  case MTLDataTypeShort:  case MTLDataTypeUShort:  return sizeof(short) * 1;
        case MTLDataTypeHalf2: case MTLDataTypeShort2: case MTLDataTypeUShort2: return sizeof(short) * 2;
        case MTLDataTypeHalf3: case MTLDataTypeShort3: case MTLDataTypeUShort3: return sizeof(short) * 3;
        case MTLDataTypeHalf4: case MTLDataTypeShort4: case MTLDataTypeUShort4: return sizeof(short) * 4;

        case MTLDataTypeFloat3x3: return sizeof(float) * 9;
        case MTLDataTypeFloat4x4: return sizeof(float) * 16;
        default: assert(false); break;
    }
    return 0;
}

static MTLVertexFormat gfx_vertex_format_2_metal_type(gfx_vertex_format format)
{
    switch (format)
    {
        case gfx_vertex_format_float1:  return MTLVertexFormatFloat;
        case gfx_vertex_format_float2:  return MTLVertexFormatFloat2;
        case gfx_vertex_format_float4:  return MTLVertexFormatFloat4;

        case gfx_vertex_format_int2:    return MTLVertexFormatInt2;
        case gfx_vertex_format_int4:    return MTLVertexFormatInt4;
        case gfx_vertex_format_uint2:   return MTLVertexFormatUInt2;
        case gfx_vertex_format_uint4:   return MTLVertexFormatUInt4;

        case gfx_vertex_format_half2:   return MTLVertexFormatHalf2;
        case gfx_vertex_format_half4:   return MTLVertexFormatHalf4;

        case gfx_vertex_format_short2:  return MTLVertexFormatShort2;
        case gfx_vertex_format_short4:  return MTLVertexFormatShort4;
        case gfx_vertex_format_ushort2:  return MTLVertexFormatUShort2;
        case gfx_vertex_format_ushort4:  return MTLVertexFormatUShort4;

        case gfx_vertex_format_byte4:    return MTLVertexFormatUChar4;
    }
}


static MTLPixelFormat metal_pixel_format(gfx_pixel_format format)
{
    switch (format)
    {
    case gfx_pixel_format_a8:               return MTLPixelFormatR8Unorm;
    case gfx_pixel_format_rgba4444:         return MTLPixelFormatInvalid;
    case gfx_pixel_format_rgb5a1:           return MTLPixelFormatInvalid;
    case gfx_pixel_format_rgb565:           return MTLPixelFormatInvalid;
    case gfx_pixel_format_rgba8:            return MTLPixelFormatRGBA8Unorm;

    case gfx_pixel_format_etc1:             return MTLPixelFormatETC2_RGB8;
    case gfx_pixel_format_etc2_rgba8:       return MTLPixelFormatEAC_RGBA8;
    case gfx_pixel_format_etc2_rgb8a1:      return MTLPixelFormatETC2_RGB8A1;

    case gfx_pixel_format_pvrtc_rgb_2bpp:   return MTLPixelFormatPVRTC_RGB_2BPP;
    case gfx_pixel_format_pvrtc_rgba_2bpp:  return MTLPixelFormatPVRTC_RGBA_2BPP;

    case gfx_pixel_format_pvrtc_rgb_4bpp:   return MTLPixelFormatPVRTC_RGB_4BPP;
    case gfx_pixel_format_pvrtc_rgba_4bpp:  return MTLPixelFormatPVRTC_RGBA_4BPP;

#if __OSX__
    case gfx_pixel_format_bc1:              return MTLPixelFormatBC1_RGBA;
    case gfx_pixel_format_bc2:              return MTLPixelFormatBC2_RGBA;
    case gfx_pixel_format_bc3:              return MTLPixelFormatBC3_RGBA;

#else
    case gfx_pixel_format_bc1:
    case gfx_pixel_format_bc2:
    case gfx_pixel_format_bc3:              return MTLPixelFormatInvalid;
#endif

    case gfx_pixel_format_astc4x4:     return MTLPixelFormatASTC_4x4_LDR;
    case gfx_pixel_format_astc5x5:     return MTLPixelFormatASTC_5x5_LDR;
    case gfx_pixel_format_astc6x6:     return MTLPixelFormatASTC_6x6_LDR;
    case gfx_pixel_format_astc8x8:     return MTLPixelFormatASTC_8x8_LDR;
    case gfx_pixel_format_astc10x10:   return MTLPixelFormatASTC_10x10_sRGB;
    case gfx_pixel_format_astc12x12:   return MTLPixelFormatASTC_12x12_sRGB;

    case gfx_pixel_format_r16f:             return MTLPixelFormatR16Float;
    case gfx_pixel_format_rg16f:            return MTLPixelFormatRG16Float;
    case gfx_pixel_format_rgba16f:          return MTLPixelFormatRGBA16Float;

    case gfx_pixel_format_r32f:             return MTLPixelFormatR32Float;
    case gfx_pixel_format_rg32f:            return MTLPixelFormatRG32Float;
    case gfx_pixel_format_rgba32f:          return MTLPixelFormatRGBA32Float;

    case gfx_pixel_format_d24x8:            return MTLPixelFormatDepth16Unorm;
    case gfx_pixel_format_d24s8:            return MTLPixelFormatDepth32Float_Stencil8;

    default:                                return MTLPixelFormatInvalid;
    };

    return MTLPixelFormatInvalid;
}

static MTLSamplerMinMagFilter   metal_filter[] = { MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterLinear };
static MTLSamplerMipFilter      metal_mipmap[] = { MTLSamplerMipFilterNearest,    MTLSamplerMipFilterLinear };
static MTLSamplerAddressMode    metal_adress[] = { MTLSamplerAddressModeRepeat,   MTLSamplerAddressModeMirrorRepeat, MTLSamplerAddressModeClampToEdge };

static void defaultlog(gfx_msg, const char* format, ...)
{
    printf("%s", format);
};

extern "C" void metal_create_sampler(gfx_context_t * ctx, gfx_sampler_desc_t * desc, gfx_sampler_t * *out_sampler);
extern "C" void metal_create_texture(gfx_context_t * ctx, gfx_texture_desc_t * desc, gfx_texture_t * *out_texture);


void metal_init(gfx_settings_t* settings, gfx_context_t** ctx)
{
    metal_context_t* mctx = (metal_context_t*)calloc(1, sizeof(metal_context_t));

    MTKView* view = CFBridgingRelease((void*)settings->handle);
    CAMetalLayer* layer = (CAMetalLayer*)view.layer;

    if (view.device == nil)
        view.device = MTLCreateSystemDefaultDevice();

    mctx->view = view;
    mctx->metal_layer = layer;
    mctx->device = view.device;
    mctx->cmd_queue = [mctx->device newCommandQueue];
    mctx->dbglog = settings->dbglog ? settings->dbglog : defaultlog;

    mctx->caps.support_pvr = true;
    mctx->caps.support_etc = true;
    mctx->caps.support_astc = true;
    mctx->caps.support_raytrace = view.device.supportsRaytracing;
    mctx->caps.support_compute = true;
#if __OSX__
    mctx->caps.support_bc = view.device.supportsBCTextureCompression;
#else
    mctx->caps.support_bc = false;
#endif

    gfx_sampler_desc_t sampler = {};
    sampler.anisotropy = 1;
    sampler.minmag = gfx_filter_point;
    sampler.mipmap = gfx_filter_point;
    sampler.mode = gfx_address_mode_repeat;
    metal_create_sampler(&mctx->handle, &sampler, &mctx->default_sampler);


    uint32_t _colors[] = { 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF, 0xFF808080,
                            0xFF808080, 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF,
                            0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF, 0xFF808080,
                            0xFF808080, 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF,
                            0xFFFF0000, 0xFF000000,
                            0xFF000000, 0xFFFF0000,
                            0xFF00FFFF
    };

    gfx_texture_desc_t image = { 0 };
    image.label = "default_4x4";
    image.width = 4;
    image.height = 4;
    image.depth = 1;
    image.format = gfx_pixel_format_rgba8;
    image.mip_levels = 3;
    image.data = (uint8_t*)_colors;
    metal_create_texture(&mctx->handle, &image, &mctx->default_texture);

    *ctx = &mctx->handle;
}

void metal_get_caps(gfx_context_t* ctx, gfx_caps_t* caps)
{
    auto mctx = from_ctx(ctx);
    memcpy(caps, &mctx->caps, sizeof(gfx_caps_t));
}

void metal_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t** swapchain)
{
    __unused auto mctx = from_ctx(ctx);

    metal_swapchain_t* mswapchain = (metal_swapchain_t*)calloc(1, sizeof(metal_swapchain_t));
    metal_render_target_t* target = (metal_render_target_t*)calloc(1, sizeof(metal_render_target_t));

    mswapchain->target = target;

    *swapchain = &mswapchain->handle;
}


int32_t metal_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, gfx_render_target_t** target)
{
    auto mctx = from_ctx(ctx);
    metal_swapchain_t* mswapchain = (metal_swapchain_t*)swapchain;

    if (mctx->depth_texture == nullptr)
    {
        CGSize drawableSize = mctx->metal_layer.drawableSize;
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : MTLPixelFormatDepth32Float
            width : drawableSize.width
            height : drawableSize.height
            mipmapped : NO];
        descriptor.usage = MTLTextureUsageRenderTarget;
        descriptor.storageMode = MTLStorageModePrivate;

        mctx->depth_texture = [mctx->device newTextureWithDescriptor : descriptor];
        [mctx->depth_texture setLabel : @"Depth Texture"] ;
    }

    mswapchain->drawable = [mctx->metal_layer nextDrawable];
    mswapchain->target->color = mswapchain->drawable.texture;
    mswapchain->target->depth = mctx->depth_texture;


    mctx->drawable = mswapchain->drawable;
    *target = &mswapchain->target->handle;

    return 0;
}

void metal_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx)
{
}


void metal_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    auto mctx = from_ctx(ctx);

    id <MTLBuffer> buffer;

    MTLResourceOptions options = MTLResourceOptionCPUCacheModeDefault;

    switch (desc->usage)
    {
        case gfx_buffer_usage_index:    options = MTLResourceOptionCPUCacheModeDefault; break;
        case gfx_buffer_usage_vertex:   options = MTLResourceOptionCPUCacheModeDefault; break;
        case gfx_buffer_usage_uniform:  options = MTLResourceStorageModeShared; break;
        case gfx_buffer_usage_storage:  options = MTLResourceOptionCPUCacheModeDefault; break;
        case gfx_buffer_usage_indirect: options = MTLResourceOptionCPUCacheModeDefault; break;
    }

    if (desc->data == nullptr)
        buffer = [mctx->device newBufferWithLength : desc->size 
                                           options : options];
    else
        buffer = [mctx->device newBufferWithBytes : desc->data 
                                           length : desc->size 
                                          options : options];

    if (buffer == nullptr)
        mctx->dbglog(gfx_msg_error, "failed create buffer");

    if (desc->label != nullptr)
        buffer.label = [NSString stringWithCString : desc->label encoding : NSASCIIStringEncoding]; ;// @"UniformBuffer";

    metal_buffer_t* mbuffer = (metal_buffer_t*)calloc(1, sizeof(metal_buffer_t));
    mbuffer->buffer = buffer;
    mbuffer->usage = desc->usage;

    *out_buffer = &mbuffer->handle;
}

const char* metal_entry_point(const char* data, const char* stage, char* buff)
{
    const char* ptr = strstr(data, stage);
    if (ptr != nullptr)
    {
        ptr += strlen(stage);
        while (isspace(*ptr)) ++ptr; // skip whitespace
        while (isalpha(*ptr) || isdigit(*ptr))++ptr; // skip type
        while (isspace(*ptr)) ++ptr; // skip whitespace
        const char* start = ptr;
        while (isalpha(*ptr) || isdigit(*ptr))++ptr; // skip type
        const char* end = ptr;

        strncpy(buff, start, end - start);

        return ptr;
    }
    return nullptr;
}

void parse_mtl_arg(MTLArgument* arg, gfx_uniform_t* uniform, gfx_shader_stage stage)
{
    strcpy(uniform->name, [arg.name cStringUsingEncoding : NSASCIIStringEncoding]);

    uniform->binding = arg.index;
    uniform->stage_mask = 1 << stage;

    if (arg.type == MTLArgumentTypeTexture)
    {
        uniform->type = gfx_uniform_texture2d;
    }
    if (arg.type == MTLArgumentTypeSampler)
    {
        uniform->type = gfx_uniform_texture2d;
    }
    if (arg.type == MTLArgumentTypeBuffer)
    {
        uniform->size = arg.bufferDataSize;
        uniform->type = gfx_uniform_ubo;

        for (MTLStructMember* member in arg.bufferStructType.members)
        {
            uint16_t field_id = uniform->field_count;
            strcpy((char*)uniform->fields[field_id].name, [member.name cStringUsingEncoding : NSASCIIStringEncoding]);
            uniform->fields[field_id].stride = metal_data_type_stride(member.dataType);
            uniform->fields[field_id].offset = member.offset;
            uniform->field_count++;
        }
    }
};

uint32_t merge_uniforms(gfx_uniform_t* uniforms, uint32_t count)
{
    auto cmp_uniform = [](gfx_uniform_t* a, gfx_uniform_t* b) -> int
        {
            int res = !strcmp(a->name, b->name) &&
                a->binding == b->binding &&
                a->type == b->type &&
                a->binding == b->binding &&
                a->field_count == b->field_count;
            return res;
        };

    for (uint32_t i = 0; i < count; ++i)
    {
        for (uint32_t j = i + 1; j < count; ++j)
        {
            if (cmp_uniform(&uniforms[i], &uniforms[j]))
            {
                uniforms[i].stage_mask = uniforms[i].stage_mask | uniforms[j].stage_mask;
                memcpy(&uniforms[j], &uniforms[count - 1], sizeof(gfx_uniform_t));
                memset(&uniforms[count - 1], 0, sizeof(gfx_uniform_t));
                count--;
            }
        }
    }
    return count;
}

void metal_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader)
{
    auto mctx = from_ctx(ctx);

    metal_shader_t* mshader = (metal_shader_t*)calloc(1, sizeof(metal_shader_t));

    *out_shader = &mshader->handle;

    NSError* error = nil;
    for (uint32_t i = 0; i < desc->stages_count; ++i)
    {
        gfx_shader_stage stage = desc->stages[i].stage;

        // dispatch_data_t lib_data = dispatch_data_create(desc->stages[i].data, desc->stages[i].size,
        //                                                         dispatch_get_main_queue(), ^{} /* must be non-default */);
         //MTLRenderPipelineReflection* reflectionObj
         //id<MTLLibrary> mtllib = [mctx->device newLibraryWithData:lib_data error:&error];
         //id<MTLLibrary> defaultLibrary = [mctx->device newLibraryWithSource:[NSString stringWithUTF8String:(const char*)stage->data, ]]

        MTLCompileOptions* options = [MTLCompileOptions new];
        options.languageVersion = MTLLanguageVersion1_1;

        NSString* src = [NSString stringWithUTF8String : (char*)desc->stages[i].data];
        id<MTLLibrary> mtllib = [mctx->device newLibraryWithSource : src options : options error : &error];

        const char entry[PATH_MAX] = "";
        switch (stage)
        {
            case gfx_shader_vertex: {
                metal_entry_point((char*)desc->stages[i].data, "vertex", (char*)entry);
                mshader->vertex_func = [mtllib newFunctionWithName : [NSString stringWithUTF8String : entry] ];
            } break;

            case gfx_shader_fragment: {
                metal_entry_point((char*)desc->stages[i].data, "fragment", (char*)entry);
                mshader->fragment_func = [mtllib newFunctionWithName : [NSString stringWithUTF8String : entry] ];
            } break;

            case gfx_shader_compute: {
                metal_entry_point((char*)desc->stages[i].data, "compute", (char*)entry);
                mshader->compute_func = [mtllib newFunctionWithName : [NSString stringWithUTF8String : entry] ];
            } break;

            default: assert(false); break;
        }
    }

    auto is_arg_buffer_suppurt = mctx->device.argumentBuffersSupport;

    //  id<MTLArgumentEncoder> vertexArgEncoder = [mshader->fragment_func newArgumentEncoderWithBufferIndex:2];
    //  int f = vertexArgEncoder.encodedLength;


    MTLRenderPipelineReflection* reflectionObj;
    MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;

    MTLVertexDescriptor* assembly = [[MTLVertexDescriptor alloc]init];
    for (int i = 0; i < 5; ++i)
    {
        assembly.attributes[i].format = MTLVertexFormatFloat4;
        assembly.attributes[i].offset = 0;
        assembly.attributes[i].bufferIndex = 0;
    }

    assembly.layouts[0].stride = 16;
    assembly.layouts[0].stepRate = 1;
    assembly.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor* pipeline = [MTLRenderPipelineDescriptor new];
    pipeline.vertexDescriptor = assembly;
    pipeline.vertexFunction = mshader->vertex_func;
    pipeline.fragmentFunction = mshader->fragment_func;
    pipeline.colorAttachments[0].pixelFormat = mctx->view.colorPixelFormat;
    pipeline.depthAttachmentPixelFormat = mctx->view.depthStencilPixelFormat;
    pipeline.stencilAttachmentPixelFormat = mctx->view.depthStencilPixelFormat;
    __unused id <MTLRenderPipelineState> pso = [mctx->device newRenderPipelineStateWithDescriptor : pipeline options : option reflection : &reflectionObj error : &error];
    if (pso == nil)
        NSLog(@"%@", error);

    uint32_t idx = 0;
    gfx_uniform_t* uniforms = &mshader->uniforms[0];
    for (int i = 0; i < reflectionObj.vertexArguments.count; ++i)
    {
        MTLArgument* arg = reflectionObj.vertexArguments[i];
        const char* name = [arg.name cStringUsingEncoding : NSASCIIStringEncoding];
        if (strstr(name, "vertexBuffer.") != nullptr)
            continue;
        parse_mtl_arg(arg, &uniforms[idx], gfx_shader_vertex);
        ++idx;
    }
    for (int i = 0; i < reflectionObj.fragmentArguments.count; ++i, ++idx)
    {
        parse_mtl_arg(reflectionObj.fragmentArguments[i], &uniforms[idx], gfx_shader_fragment);
    }

    mshader->hash = pso.hash;
    mshader->uniform_count = merge_uniforms(uniforms, idx);

    pso = nullptr;
}


void metal_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler)
{
    auto mctx = from_ctx(ctx);

    MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
    samplerDescriptor.minFilter = metal_filter[desc->minmag];   // MTLSamplerMinMagFilterNearest;
    samplerDescriptor.magFilter = metal_filter[desc->minmag];   // MTLSamplerMinMagFilterLinear;
    samplerDescriptor.mipFilter = metal_mipmap[desc->mipmap];   // MTLSamplerMipFilterLinear;
    samplerDescriptor.sAddressMode = metal_adress[desc->mode];  // MTLSamplerAddressModeRepeat;
    samplerDescriptor.tAddressMode = metal_adress[desc->mode];  // MTLSamplerAddressModeRepeat;
    samplerDescriptor.maxAnisotropy = desc->anisotropy;

    metal_sampler_t* sampler = (metal_sampler_t*)calloc(1, sizeof(metal_sampler_t));
    sampler->sampler = [mctx->device newSamplerStateWithDescriptor : samplerDescriptor];

    *out_sampler = &sampler->handle;
}


void metal_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** out_texture)
{
    auto mctx = from_ctx(ctx);

    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : metal_pixel_format(desc->format)
        width : desc->width
        height : desc->height
        mipmapped : desc->mip_levels > 0];
    descriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [mctx->device newTextureWithDescriptor : descriptor];

    uint32_t layersize0 = *((uint32_t*)desc->data);
    uint32_t layersize1 = gfx_utils_image_layer_size(desc->width, desc->height, desc->depth, desc->format);
    bool has_size_info = layersize0 == layersize1;
    has_size_info = false;

    uintptr_t offset = (has_size_info) ? 4 : 0;
    for (uint32_t face = 0; face < desc->depth; ++face)
    {
        for (uint32_t level = 0; level < desc->mip_levels; ++level)
        {
            uint32_t width  = MAX(desc->width  >> level, 1);
            uint32_t height = MAX(desc->height >> level, 1);
            uint32_t depth  = MAX(desc->depth  >> level, 1);

            uint32_t size = gfx_utils_image_layer_size(width, height, depth, desc->format);
            uint32_t bytes_per_row = !is_compressed_format(desc->format) ? gfx_utils_image_row_pitch(desc->format, width) : 0;
            uint8_t* pixels = (uint8_t*)desc->data + offset;
            offset += size;// + ((has_size_info)?4:0);

            size = MAX(size, 32);
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [texture replaceRegion : region mipmapLevel : level withBytes : pixels bytesPerRow : bytes_per_row] ;
        }
    }

    metal_texture_t* mtexture = (metal_texture_t*)calloc(1, sizeof(metal_texture_t));
    mtexture->texture = texture;

    *out_texture = &mtexture->handle;
}


void metal_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline)
{
    metal_context_t* mctx = from_ctx(ctx);
    metal_shader_t* msahder = (metal_shader_t*)desc->shader;

    NSError* error = nil;

    if (msahder == nullptr)
        mctx->dbglog(gfx_msg_error, "creating pipieline failed: shader is null");


    MTLVertexDescriptor* vertex_descriptor = [[MTLVertexDescriptor alloc]init];
    for (uint32_t i = 0; i < desc->assembly.attributes_count; ++i)
    {
        auto format = gfx_vertex_format_2_metal_type(desc->assembly.attributes[i].format);
        vertex_descriptor.attributes[i].format = format;
        vertex_descriptor.attributes[i].offset = desc->assembly.attributes[i].offset;
        vertex_descriptor.attributes[i].bufferIndex = desc->assembly.attributes[i].binding;
    }
    for (uint32_t i = 0; i < desc->assembly.slot_count; ++i)
    {
        vertex_descriptor.layouts[i].stride = desc->assembly.slots[i].stride;
        vertex_descriptor.layouts[i].stepRate = 1;
        switch (desc->assembly.slots[i].rate)
        {
        case gfx_vertex_rate_vertex:
            vertex_descriptor.layouts[i].stepFunction = MTLVertexStepFunctionPerVertex;
            break;

        case gfx_vertex_rate_instance:
            vertex_descriptor.layouts[i].stepFunction = MTLVertexStepFunctionPerInstance;
            break;
        }
    }

    MTLDepthStencilDescriptor* depthStateDesc = [[MTLDepthStencilDescriptor alloc]init];
    depthStateDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStateDesc.depthWriteEnabled = YES;

    MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
    descriptor.vertexFunction = msahder->vertex_func;
    descriptor.fragmentFunction = msahder->fragment_func;
    descriptor.vertexDescriptor = vertex_descriptor;
    descriptor.sampleCount = mctx->view.sampleCount;
    descriptor.colorAttachments[0].pixelFormat = mctx->view.colorPixelFormat;
    descriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    descriptor.stencilAttachmentPixelFormat = mctx->view.depthStencilPixelFormat;

    metal_pipeline_t* mpipeline = (metal_pipeline_t*)calloc(1, sizeof(metal_pipeline_t));
    mpipeline->pipeline = [mctx->device newRenderPipelineStateWithDescriptor : descriptor error : &error];
    mpipeline->depth_state = [mctx->device newDepthStencilStateWithDescriptor : depthStateDesc];

    *out_pipeline = &mpipeline->handle;
    NSLog(@"%@", error);
}


void metal_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** out_target)
{

}


void metal_create_descriptor_set_pool(metal_context_t* ctx, metal_shader_t* shader, uint32_t capacity, metal_descriptor_set_pool_t** out_pool)
{
    metal_descriptor_set_pool_t* pool = (metal_descriptor_set_pool_t*)calloc(1, sizeof(metal_descriptor_set_pool_t));
    *out_pool = pool;

    uint32_t ubo_size = 0;
    for (uint32_t i = 0; i < shader->uniform_count; ++i)
    {
        ubo_size += shader->uniforms[i].size;
    }

    pool->capacity = capacity;
    pool->free_set_count = capacity;
    pool->next_free = 0;

    gfx_buffer_desc_t desc = {};
    desc.usage = gfx_buffer_usage_uniform;
    desc.size = ubo_size * capacity;
    metal_create_buffer(&ctx->handle, &desc, &pool->ubo);
    auto mbuffer = (metal_buffer_t*)pool->ubo;

    pool->free_set_count = capacity;
    pool->ubo_buffer_data_size = ubo_size;
    pool->ubo_buffer_data_ptr = mbuffer->buffer.contents;
    pool->writes = (metal_writes_t*)calloc(capacity * shader->uniform_count, sizeof(metal_writes_t));
    pool->descriptor_sets = (metal_descriptor_set_t*)calloc(capacity, sizeof(metal_descriptor_set_t));
    for (uint32_t i = 0; i < capacity; ++i)
    {
        auto descriptor_set = &pool->descriptor_sets[i];
        descriptor_set->isfree = true;
        descriptor_set->pool = pool;
        descriptor_set->shader = shader;
        descriptor_set->ubo_ptr = (uint8_t*)pool->ubo_buffer_data_ptr + i * ubo_size;
        descriptor_set->writes = pool->writes + i * shader->uniform_count;
        descriptor_set->writes_count = shader->uniform_count;
        for (uint32_t j = 0; j < shader->uniform_count; ++j)
        {
            descriptor_set->writes[j].stage_mask = shader->uniforms[j].stage_mask;
            descriptor_set->writes[j].type = shader->uniforms[j].type;
            descriptor_set->writes[j].slot = shader->uniforms[j].binding;
            switch (shader->uniforms[j].type)
            {
            case gfx_uniform_ubo:
                descriptor_set->writes[j].offset = i * ubo_size;
                descriptor_set->writes[j].buffer = mbuffer->buffer;
                break;

            case gfx_uniform_sampler:
                descriptor_set->writes[j].sampler = ((metal_sampler_t*)ctx->default_sampler)->sampler;
                break;

            case gfx_uniform_texture2d:
            case gfx_uniform_texture2d_cube:
            case gfx_uniform_texture2d_array:
            case gfx_uniform_texture3d:
                descriptor_set->writes[j].texture = ((metal_texture_t*)ctx->default_texture)->texture;
                break;

            default:
                break;
            }
        }
    }
}


void metal_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, gfx_descriptor_set_t** out_descriptor)
{
    auto mctx = from_ctx(ctx);
    auto mshader = (metal_shader_t*)shader;

    if (mshader->current_pool == nullptr)
    {
        metal_create_descriptor_set_pool(mctx, mshader, 2048, &mshader->current_pool);
    }

    if (mshader->current_pool->free_set_count == 0)
    {
        mctx->dbglog(gfx_msg_warning, "\nTODO: push pool before allocarte new one");
        metal_create_descriptor_set_pool(mctx, mshader, 2048, &mshader->current_pool);
    }

    for (uint32_t i = mshader->current_pool->next_free; i < mshader->current_pool->capacity; i++)
    {
        if (mshader->current_pool->descriptor_sets[i].isfree) {
            mshader->current_pool->descriptor_sets[i].isfree = false;

            mshader->current_pool->count++;
            mshader->current_pool->free_set_count--;
            mshader->current_pool->next_free = i + 1;

            *out_descriptor = (gfx_descriptor_set_t*)(&mshader->current_pool->descriptor_sets[i].handle);
            return;
        }
    }

    metal_descriptor_set_t* set = (metal_descriptor_set_t*)calloc(1, sizeof(metal_descriptor_set_t));
    *out_descriptor = &set->handle;
}

void metal_create_cmd(gfx_context_t* ctx, uint32_t count, gfx_command_buffer_t** cmd)
{
    metal_context_t* mctx = from_ctx(ctx);
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)calloc(1, sizeof(metal_command_buffer_t));
    mcmd->mctx = mctx;
    //mcmd->cmd_buffer = [mctx->cmd_queue commandBuffer];

    *cmd = &mcmd->handle;
}


void metal_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer)
{
    metal_buffer_t* mbuffer = (metal_buffer_t*)buffer;

    [mbuffer->buffer setPurgeableState : MTLPurgeableStateEmpty] ;
#if !__has_feature(objc_arc)
    [mbuffer->buffer release];
#endif
    mbuffer->buffer = nullptr;
}


void metal_destroy_shader(gfx_context_t* ctx, gfx_shader_t* shader)
{
    metal_shader_t* mshader = (metal_shader_t*)shader;
    mshader->vertex_func = nullptr;
    mshader->fragment_func = nullptr;
    mshader->compute_func = nullptr;
    // todo: destroy buffers, pools descriptors etc
    free(mshader);
}


void metal_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler)
{
    metal_sampler_t* msampler = (metal_sampler_t*)sampler;
#if !__has_feature(objc_arc)
    [msampler->sampler release];
#endif
    msampler->sampler = nullptr;
    free(msampler);
}


void metal_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture)
{
    metal_texture_t* mtexture = (metal_texture_t*)texture;
    [mtexture->texture setPurgeableState : MTLPurgeableStateEmpty] ;
#if !__has_feature(objc_arc)
    [mtexture->texture release];
#endif
    mtexture->texture = nullptr;
    free(mtexture);
}


void metal_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
    metal_pipeline_t* mpipeline = (metal_pipeline_t*)pipeline;
    mpipeline->pipeline = nullptr;
    mpipeline->depth_state = nullptr;
    free(mpipeline);
}


void metal_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* _target)
{

}


void metal_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor)
{

}


void metal_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd)
{

}

struct uniform_handle_t
{
    static uint64_t create(uint16_t hash, uint16_t type, uint16_t idx, uint16_t field)
    {
        uniform_handle_t result = {};
        result.hash = hash;
        result.type = type;
        result.idx = idx;
        result.field = field;
        return result.handle;
    }

    static uniform_handle_t create(uint64_t handle)
    {
        return uniform_handle_t{ handle };
    }

    union
    {
        uint64_t handle;
        struct
        {
            uint16_t hash;  // shader hash
            uint16_t type;  // uniform type
            uint16_t idx;   // uniform idx
            uint16_t field; // field idx
        };
    };
};

uint64_t metal_uniform_location(gfx_shader_t* shader, const char* name)
{
    metal_shader_t* mshader = (metal_shader_t*)shader;

    for (uint32_t i = 0; i < mshader->uniform_count; ++i)
    {
        auto uniform = &mshader->uniforms[i];
        if (strstr(uniform->name, name)) {
            return uniform_handle_t::create(mshader->hash, uniform->type, i, 0);
        }

        for (uint32_t j = 0; j < mshader->uniforms[i].field_count; ++j)
        {
            if (strstr(uniform->fields[j].name, name)) {
                return uniform_handle_t::create(mshader->hash, uniform->type, i, j);
            }
        }
    }
    return 0;
}

void metal_uniform_set_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size)
{
    if (set == NULL || handle == 0)
        return;

    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = (metal_shader_t*)mset->shader;

    auto uniform_handle = uniform_handle_t::create(handle);
    if (uniform_handle.hash != mshader->hash || uniform_handle.type != gfx_uniform_ubo)
        return;

    uint16_t unform_id = uniform_handle.idx;
    uint16_t child_id = uniform_handle.field;

    uint32_t offset = mshader->uniforms[unform_id].fields[child_id].offset;
    memcpy((uint8_t*)mset->ubo_ptr + offset, data, size);
}

void metal_uniform_set_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset)
{
    if (set == NULL || handle == 0)
        return;

    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = (metal_shader_t*)mset->shader;

    auto uniform_handle = uniform_handle_t::create(handle);
    if (uniform_handle.hash != mshader->hash || uniform_handle.type != gfx_uniform_ubo)
        return;
}

void metal_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture)
{
    if (set == NULL || handle == 0)
        return;

    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = (metal_shader_t*)mset->shader;

    auto uniform_handle = uniform_handle_t::create(handle);
    if (uniform_handle.hash != mshader->hash || uniform_handle.type != gfx_uniform_texture2d)
        return;

    mset->writes[uniform_handle.idx].texture = ((metal_texture_t*)texture)->texture;
}

void metal_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler)
{
    if (set == NULL || handle == 0)
        return;

    metal_descriptor_set_t* mset = (metal_descriptor_set_t*)set;
    metal_shader_t* mshader = (metal_shader_t*)mset->shader;

    auto uniform_handle = uniform_handle_t::create(handle);
    if (uniform_handle.hash != mshader->hash || uniform_handle.type != gfx_uniform_sampler)
        return;
}


void metal_cmd_begin(gfx_command_buffer_t* cmd)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;

    if (mcmd->encoder != nullptr) {
        mcmd->mctx->dbglog(gfx_msg_error, "metal_cmd_begin: cmd already begined");
        return;
    }
    if (mcmd->cmd_buffer == nullptr)
        mcmd->cmd_buffer = [mcmd->mctx->cmd_queue commandBuffer];
}


void metal_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_render_target_t* target)
{
    metal_render_target_t* mtarget = (metal_render_target_t*)target;
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;

    MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = mtarget->color;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.3f, 0.6f, 0.9f, 1.0f);

    passDescriptor.depthAttachment.texture = mtarget->depth;
    passDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
    passDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
    passDescriptor.depthAttachment.clearDepth = 1.0;

    mcmd->encoder = [mcmd->cmd_buffer renderCommandEncoderWithDescriptor : passDescriptor];
}


void metal_cmd_end_pass(gfx_command_buffer_t* cmd)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    [mcmd->encoder endEncoding] ;
}


void metal_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline)
{
    auto mcmd = (metal_command_buffer_t*)cmd;
    auto mpipeline = (metal_pipeline_t*)pipeline;
    if (mpipeline == nullptr)
        return;

    [mcmd->encoder setFrontFacingWinding : mpipeline->face] ; //MTLWindingCounterClockwise
    [mcmd->encoder setCullMode : mpipeline->cull_mode] ;
    [mcmd->encoder setRenderPipelineState : mpipeline->pipeline] ;
    [mcmd->encoder setDepthStencilState : mpipeline->depth_state] ;
}

void metal_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, gfx_descriptor_set_t* descriptor)
{
    auto mcmd = (metal_command_buffer_t*)cmd;
    auto mdescriptor = (metal_descriptor_set_t*)descriptor;

    bool change_offset_only = mcmd->active_pool == mdescriptor->pool;
    mcmd->active_pool = mdescriptor->pool;

    for (uint32_t i = 0; i < mdescriptor->writes_count; ++i)
    {
        //    [mcmd->encoder useResource:<#(nonnull id<MTLResource>)#> usage:<#(MTLResourceUsage)#>]
        auto write = &mdescriptor->writes[i];
        switch (write->type)
        {
        default: break;
        case gfx_uniform_ubo: {
            if (write->stage_mask & 1 << gfx_shader_vertex)
            {
                if (change_offset_only)
                    [mcmd->encoder setVertexBufferOffset : write->offset atIndex : write->slot];
                else
                    [mcmd->encoder setVertexBuffer : write->buffer offset : write->offset atIndex : write->slot];
            }
            if (write->stage_mask & 1 << gfx_shader_fragment)
            {
                if (change_offset_only)
                    [mcmd->encoder setFragmentBufferOffset : write->offset atIndex : write->slot];
                else
                    [mcmd->encoder setFragmentBuffer : write->buffer offset : write->offset atIndex : write->slot];
            }
        } break;

        case gfx_uniform_sampler: {
            if (write->stage_mask & 1 << gfx_shader_vertex)
                [mcmd->encoder setVertexSamplerState : write->sampler atIndex : write->slot];
            if (write->stage_mask & 1 << gfx_shader_fragment)
                [mcmd->encoder setFragmentSamplerState : write->sampler atIndex : write->slot];
        } break;

        case gfx_uniform_texture2d:
        case gfx_uniform_texture2d_cube:
        case gfx_uniform_texture2d_array:
        case gfx_uniform_texture3d: {
            if (write->stage_mask & 1 << gfx_shader_vertex)
                [mcmd->encoder setVertexTexture : write->texture atIndex : write->slot];
            if (write->stage_mask & 1 << gfx_shader_fragment)
                [mcmd->encoder setFragmentTexture : write->texture atIndex : write->slot];
        } break;
        }
    }
}

void metal_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, gfx_buffer_t* buffer)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_buffer_t* mbuffer = (metal_buffer_t*)buffer;
    mcmd->index_type = (format == gfx_index_format_16) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
    mcmd->index_buffer = mbuffer->buffer;
}

void metal_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, gfx_buffer_t* buffer)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    metal_buffer_t* mbuffer = (metal_buffer_t*)buffer;

    [mcmd->encoder setVertexBuffer : mbuffer->buffer offset : 0 atIndex : slot] ;
}

void metal_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;

    [mcmd->encoder drawPrimitives : MTLPrimitiveTypeTriangle
        vertexStart : 0
        vertexCount : vertex_count
        instanceCount : instance_count] ;
}

void metal_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;

    [mcmd->encoder drawIndexedPrimitives : MTLPrimitiveTypeTriangle
        indexCount : idx_count
        indexType : mcmd->index_type
        indexBuffer : mcmd->index_buffer
        indexBufferOffset : 0];
}

void metal_cmd_end(gfx_command_buffer_t* cmd)
{
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;
    mcmd->encoder = nullptr;
    mcmd->active_pool = nullptr;
    mcmd->index_buffer = nullptr;
}


void metal_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options)
{
    metal_context_t* mctx = from_ctx(ctx);
    metal_command_buffer_t* mcmd = (metal_command_buffer_t*)cmd;

    [mcmd->cmd_buffer presentDrawable : mctx->drawable] ;
    [mcmd->cmd_buffer commit] ;
    mcmd->cmd_buffer = nullptr;
}


inline void gfx_init_metal(gfx_api_pfn* func_table)
{
    func_table->pfn_init = metal_init;
    func_table->pfn_create_swapchain = metal_create_swapchain;

    func_table->pfn_get_caps = metal_get_caps;

    func_table->pfn_acquire_img = metal_acquire_img;
    func_table->pfn_present_img = metal_present_img;

    func_table->pfn_create_buffer = metal_create_buffer;
    func_table->pfn_create_shader = metal_create_shader;
    func_table->pfn_create_sampler = metal_create_sampler;
    func_table->pfn_create_texture = metal_create_texture;
    func_table->pfn_create_pipeline = metal_create_pipeline;
    func_table->pfn_create_render_target = metal_create_render_target;
    func_table->pfn_create_descriptor_set = metal_create_descriptor_set;
    func_table->pfn_create_cmd = metal_create_cmd;

    func_table->pfn_destroy_buffer = metal_destroy_buffer;
    func_table->pfn_destroy_shader = metal_destroy_shader;
    func_table->pfn_destroy_sampler = metal_destroy_sampler;
    func_table->pfn_destroy_texture = metal_destroy_texture;
    func_table->pfn_destroy_pipeline = metal_destroy_pipeline;
    func_table->pfn_destroy_render_target = metal_destroy_render_target;
    func_table->pfn_destroy_descriptor_set = metal_destroy_descriptor_set;
    func_table->pfn_destroy_cmd = metal_destroy_cmd;

    func_table->pfn_uniform_location = metal_uniform_location;
    func_table->pfn_uniform_set_buffer = metal_uniform_set_buffer;
    func_table->pfn_uniform_set_buffer_data = metal_uniform_set_buffer_data;
    func_table->pfn_uniform_set_texture = metal_uniform_set_texture;
    func_table->pfn_uniform_set_sampler = metal_uniform_set_sampler;


    func_table->pfn_cmd_begin = metal_cmd_begin;
    func_table->pfn_cmd_begin_pass = metal_cmd_begin_pass;
    func_table->pfn_cmd_end_pass = metal_cmd_end_pass;

    //  void     (*pfn_cmd_scissor) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    //  void     (*pfn_cmd_viewport) (gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    func_table->pfn_cmd_bind_pipeline = metal_cmd_bind_pipeline;//gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
    func_table->pfn_cmd_bind_descriptor_set = metal_cmd_bind_descriptor_set;
    func_table->pfn_cmd_bind_buffer_ib = metal_cmd_bind_buffer_ib;
    func_table->pfn_cmd_bind_buffer_vb = metal_cmd_bind_buffer_vb;
    func_table->pfn_cmd_draw = metal_cmd_draw;
    func_table->pfn_cmd_draw_indexed = metal_cmd_draw_indexed;
    //func_table->pfn_cmd_dispatch_compute

    func_table->pfn_cmd_end = metal_cmd_end;
    func_table->pfn_submit_cmd = metal_submit_cmd;
}

#endif METAL_IMPLEMENTATION

#endif //METAL_AVAILABLE

#endif
