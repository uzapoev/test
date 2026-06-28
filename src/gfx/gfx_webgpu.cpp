
#include "gfx_webgpu.h"

#ifdef WEBGPU_AVAILABLE

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

#ifndef __EMSCRIPTEN__
  #ifdef _WIN32
    #include <windows.h>
  #endif

  #pragma comment(lib, "x64/webgpu.lib")
#endif


#if GFX_ENABLE_VERBOSE
#define GFX_VERBOSE(exp)            { exp; }
#define GFX_VERBOSE_IF(cond, exp)   { if(cond) exp; }
#else
#define GFX_VERBOSE(exp)            {}
#define GFX_VERBOSE_IF(cond, exp)   {}
#endif

extern const char* gfx_to_string(gfx_buffer_usage usage);
extern const char* gfx_to_string(gfx_shader_stage stage);
extern const char* gfx_to_string(gfx_texture_type type);
extern const char* gfx_to_string(gfx_pixel_format format);

static wgpu_context_t* from_ctx(gfx_context_t* ctx) {
    return (wgpu_context_t*)ctx;
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
        default: return WGPUPrimitiveTopology_TriangleList;
    }
}

static WGPUTextureFormat gfx_pixel_format_2_webgpu(gfx_pixel_format format, bool srgb)
{
    switch (format)
    {
        case gfx_pixel_format_a8:               return WGPUTextureFormat_R8Unorm;
        case gfx_pixel_format_rgba4444:         return WGPUTextureFormat_Undefined;
        case gfx_pixel_format_rgb5a1:           return WGPUTextureFormat_Undefined;
        case gfx_pixel_format_rgb565:           return WGPUTextureFormat_Undefined;
        case gfx_pixel_format_rgba8:            return WGPUTextureFormat_RGBA8Unorm;

        case gfx_pixel_format_etc1:             return WGPUTextureFormat_ETC2RGB8Unorm;
        case gfx_pixel_format_etc2_rgb8a1:      return WGPUTextureFormat_ETC2RGB8A1Unorm;
        case gfx_pixel_format_etc2_rgba8:       return WGPUTextureFormat_ETC2RGBA8Unorm;


        case gfx_pixel_format_bc1:              return srgb ? WGPUTextureFormat_BC1RGBAUnormSrgb : WGPUTextureFormat_BC1RGBAUnorm;
        case gfx_pixel_format_bc3:              return srgb ? WGPUTextureFormat_BC3RGBAUnormSrgb : WGPUTextureFormat_BC3RGBAUnorm;
        case gfx_pixel_format_bc4:              return srgb ? WGPUTextureFormat_BC4RSnorm        : WGPUTextureFormat_BC4RUnorm;
        case gfx_pixel_format_bc5:              return srgb ? WGPUTextureFormat_BC5RGUnorm       : WGPUTextureFormat_BC5RGUnorm;
        case gfx_pixel_format_bc6h:             return srgb ? WGPUTextureFormat_BC6HRGBUfloat    : WGPUTextureFormat_BC6HRGBFloat;
        case gfx_pixel_format_bc7:              return srgb ? WGPUTextureFormat_BC7RGBAUnormSrgb : WGPUTextureFormat_BC7RGBAUnorm;

        case gfx_pixel_format_astc4x4:          return WGPUTextureFormat_ASTC4x4Unorm;
        case gfx_pixel_format_astc5x5:          return WGPUTextureFormat_ASTC5x5Unorm;
        case gfx_pixel_format_astc6x6:          return WGPUTextureFormat_ASTC6x6Unorm;
        case gfx_pixel_format_astc8x8:          return WGPUTextureFormat_ASTC8x8Unorm;
        case gfx_pixel_format_astc10x10:        return WGPUTextureFormat_ASTC10x10Unorm;
        case gfx_pixel_format_astc12x12:        return WGPUTextureFormat_ASTC12x12Unorm;

        case gfx_pixel_format_r16f:             return WGPUTextureFormat_R16Float;
        case gfx_pixel_format_rg16f:            return WGPUTextureFormat_RG16Float;
        case gfx_pixel_format_rgba16f:          return WGPUTextureFormat_RGBA16Float;

        case gfx_pixel_format_r32f:             return WGPUTextureFormat_R32Float;
        case gfx_pixel_format_rg32f:            return WGPUTextureFormat_RG32Float;
        case gfx_pixel_format_rgba32f:          return WGPUTextureFormat_RGBA32Float;

        case gfx_pixel_format_d24x8:            return WGPUTextureFormat_Depth24Plus;
        case gfx_pixel_format_d24s8:            return WGPUTextureFormat_Depth24PlusStencil8;
        default: return WGPUTextureFormat_Undefined;
    }
    return WGPUTextureFormat_Undefined;
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

        case gfx_format_short2:  return WGPUVertexFormat_Sint16x2;
        case gfx_format_short4:  return WGPUVertexFormat_Sint16x4;

        case gfx_format_ushort2: return WGPUVertexFormat_Uint16x2;
        case gfx_format_ushort4: return WGPUVertexFormat_Uint16x4;

        case gfx_format_byte4:   return WGPUVertexFormat_Sint8x4;
    };
    return WGPUVertexFormat_Float32x4;
}

WGPUFrontFace gfx_face_2_webgpu(gfx_face face)
{
    switch (face)
    {
        case gfx_face_cw: return WGPUFrontFace_CW;
        case gfx_face_ccw: return WGPUFrontFace_CCW;
    }
    return WGPUFrontFace_CCW;
}

WGPUCullMode gfx_cull_2_webgpu(gfx_cull cull)
{
    switch (cull)
    {
        case gfx_cull_none:     return WGPUCullMode_None;
        case gfx_cull_back:     return WGPUCullMode_Back;
        case gfx_cull_front:    return WGPUCullMode_Front;
    }
    return WGPUCullMode_None;
}

static WGPUShaderStage gfx_shader_stage_2_webgpu(gfx_shader_stage stage)
{
    switch(stage)
    {
        case gfx_shader_vertex:   return WGPUShaderStage_Vertex;
        case gfx_shader_fragment: return WGPUShaderStage_Fragment;
        case gfx_shader_compute:  return WGPUShaderStage_Compute;
        default: return WGPUShaderStage::WGPUShaderStage_None;
    }
}

static WGPUAddressMode gfx_address_mode_2_webgpu(gfx_address_mode mode)
{
    switch (mode)
    {
        case gfx_address_mode_repeat:           return WGPUAddressMode_Repeat;
        case gfx_address_mode_mirror_repeat:    return WGPUAddressMode_MirrorRepeat;
        case gfx_address_mode_clamp_to_edge:    return WGPUAddressMode_ClampToEdge;
    }
    return WGPUAddressMode_Repeat;
}

static WGPUFilterMode gfx_filter_2_webgpu(gfx_filter mode)
{
    switch (mode)
    {
        case gfx_filter_point:  return WGPUFilterMode_Nearest;
        case gfx_filter_linear: return WGPUFilterMode_Linear;
    }
    return WGPUFilterMode_Nearest;
}

static WGPUMipmapFilterMode gfx_mipmap_filter_2_webgpu(gfx_filter mode)
{
    switch (mode)
    {
        case gfx_filter_point:  return WGPUMipmapFilterMode_Nearest;
        case gfx_filter_linear: return WGPUMipmapFilterMode_Linear;
    }
    return WGPUMipmapFilterMode_Nearest;
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
    return WGPUStencilOperation_Zero;
}

static WGPUBufferUsage gfx_buffer_usage_2_webgpu(gfx_buffer_usage usage)
{
    switch (usage)
    {
        case gfx_buffer_usage_index:    return WGPUBufferUsage_Index;
        case gfx_buffer_usage_vertex:   return WGPUBufferUsage_Vertex;
        case gfx_buffer_usage_uniform:  return WGPUBufferUsage_Uniform;
        case gfx_buffer_usage_storage:  return WGPUBufferUsage_Storage;
        case gfx_buffer_usage_indirect: return WGPUBufferUsage_Indirect;
        default: return WGPUBufferUsage_None;
    }
    return WGPUBufferUsage_None;
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
        case gfx_blend_op_add:             return WGPUBlendOperation_Add;
        case gfx_blend_op_min:             return WGPUBlendOperation_Min;
        case gfx_blend_op_max:             return WGPUBlendOperation_Max;
        case gfx_blend_op_subtract:        return WGPUBlendOperation_Subtract;
        case gfx_blend_op_rev_subtract:   return WGPUBlendOperation_ReverseSubtract;
    }
    return WGPUBlendOperation_Add;
}


WGPUBufferUsage gfx_usage_2_wgpu(gfx_buffer_usage usage)
{
    switch (usage) {
        case gfx_buffer_usage_index:    return WGPUBufferUsage_Index;
        case gfx_buffer_usage_vertex:   return WGPUBufferUsage_Vertex;
        case gfx_buffer_usage_uniform:  return WGPUBufferUsage_Uniform;
        case gfx_buffer_usage_storage:  return WGPUBufferUsage_Storage;
        case gfx_buffer_usage_indirect: return WGPUBufferUsage_Indirect;
    }
    return WGPUBufferUsage_Index;
}

static const char * to_string(WGPUAdapterType type)
{
    switch(type){
        case WGPUAdapterType_DiscreteGPU:   return "WGPUAdapterType_DiscreteGPU";
        case WGPUAdapterType_IntegratedGPU: return "WGPUAdapterType_IntegratedGPU";
        case WGPUAdapterType_CPU:           return "WGPUAdapterType_CPU";
        case WGPUAdapterType_Unknown:       return "WGPUAdapterType_Unknown";
        default: return "WGPUAdapterType_Force32";
    }
    return "WGPUAdapterType_Unknown";
}

static const char* to_string(WGPUBackendType type)
{
    switch (type) {
        case WGPUBackendType_Null:      return "WGPUBackendType_Null";
        case WGPUBackendType_WebGPU:    return "WGPUBackendType_WebGPU";
        case WGPUBackendType_D3D11:     return "WGPUBackendType_D3D11";
        case WGPUBackendType_D3D12:     return "WGPUBackendType_D3D12";
        case WGPUBackendType_Metal:     return "WGPUBackendType_Metal";
        case WGPUBackendType_Vulkan:    return "WGPUBackendType_Vulkan";
        case WGPUBackendType_OpenGL:    return "WGPUBackendType_OpenGL";
        case WGPUBackendType_OpenGLES:  return "WGPUBackendType_OpenGLES";
        default: return "WGPUBackendType_Force32";
    }
    return "WGPUBackendType_Null";
}



static void get_window_size(intptr_t handle, int * width, int * height)
{
#ifdef EMSCRIPTEN
    emscripten_get_canvas_element_size((const char*)handle, width, height);
#else
    RECT rect = {};
    GetClientRect((HWND)handle, &rect);
    *width = rect.right;
    *height = rect.bottom;
#endif
}

typedef struct wgsl_info {
    uint32_t            group;
    uint32_t            binding;
    char                name[32];
    gfx_uniform_type    type;
} wgsl_info;

// out -
static inline int match(const char* pattern, const char* str, const char **out) throw()
{
    if(!pattern || !str) return 0;
    while (str && *str != '\0' && *str == ' ') str++;
    if(out) *out = str;
    if (*pattern == '\0') return 1;//*str;
    if (*pattern == '*')  return match(pattern + 1, str, out) || (*str && match(pattern, str + 1, out));
    if (*pattern == '?')  return *str && (*str != '.') && match(pattern + 1, str + 1, out);
    return (*str == *pattern) && match(pattern + 1, str + 1, out);
}

static uint32_t wglsl_reflect(const char *data, gfx_uniform_t* uniforms, WGPUBindGroupLayoutEntry * bgle)
{
    const char k_group_key [] = "@group(";
    const char k_binding_key [] = "@binding(";
    const size_t k_group_key_len = sizeof(k_group_key) - 1;
    const size_t k_binding_key_len = sizeof(k_binding_key) - 1;

    uint32_t count = 0;
    const char* ptr = data;
    while(ptr)
    {
        char buffer[512] = {};
        const char* start_group = strstr(ptr, k_group_key);
        const char* end_group = start_group ? strstr(start_group, ";"):nullptr;

        const char* start_bind = strstr(ptr, k_binding_key);
        const char* end_bind = start_group ? strstr(start_group, ";") : nullptr;

        if(start_group == nullptr || start_bind == nullptr)
            return count;
        if(end_group != end_bind)
            return count;

        const char* start = start_group < start_bind ? start_group : start_bind;
        const char* end = end_group;

        intptr_t len = end - start;
        strncpy(buffer, start, len);
        ptr = end;

        uniforms[count].group = atoi(start_group + k_group_key_len);
        uniforms[count].binding = atoi(start_bind + k_binding_key_len);

      //  bgle[count].group = infos[count].group;
        bgle[count].binding = uniforms[count].binding;

        const char * last = nullptr;
        const char * var = strstr(buffer, "var");
        if(var && match("var<uniform>", var, &last)) {
            printf("");
        } else if (var && match("var <storage, read>", buffer, &last)) {
            uniforms[count].type = gfx_uniform_storage_buffer;
            bgle[count].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
        } else if (var && match("var <storage", buffer, &last)) {
            uniforms[count].type = gfx_uniform_storage_buffer;
            bgle[count].buffer.type = WGPUBufferBindingType_Storage;
        } else if (var && strstr(var, "var ")) {
            const char * dblpoint = strstr(buffer, ":");
            strncpy(uniforms[count].name, var + sizeof("var"), dblpoint - var - sizeof("var"));
            dblpoint++;
            while(dblpoint && *dblpoint == ' ' ) dblpoint++;

            // https://www.w3.org/TR/WGSL/#sampled-texture-type
            if(dblpoint && !strncmp(dblpoint, "sampler", 7)) {
                uniforms[count].type = gfx_uniform_sampler;
                bgle[count].sampler.type = WGPUSamplerBindingType_Filtering;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_2d", strlen("texture_2d"))) {
                uniforms[count].type = gfx_uniform_texture2d;
                bgle[count].texture.viewDimension = WGPUTextureViewDimension_2D;
                bgle[count].texture.sampleType = WGPUTextureSampleType_Float;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_3d", strlen("texture_3d"))) {
                uniforms[count].type = gfx_uniform_texture3d;
                bgle[count].texture.viewDimension = WGPUTextureViewDimension_3D;
                bgle[count].texture.sampleType = WGPUTextureSampleType_Float;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_cube", strlen("texture_cube"))) {
                uniforms[count].type = gfx_uniform_texture2d_cube;
                bgle[count].texture.viewDimension = WGPUTextureViewDimension_Cube;
                bgle[count].texture.sampleType = WGPUTextureSampleType_Float;
            }
            else if (dblpoint && !strncmp(dblpoint, "texture_2d_array", strlen("texture_2d_array"))) {
                uniforms[count].type = gfx_uniform_texture2d_array;
                bgle[count].texture.viewDimension = WGPUTextureViewDimension_2DArray;
                bgle[count].texture.sampleType = WGPUTextureSampleType_Float;
            }
            else {
                assert(false);
            }

        }
        count++;
    }
    return count;
}

static uint32_t get_shader_uniforms(const void* data, uint32_t size, gfx_uniform_t* uniforms, const char** entry) {
    uint32_t uniform_count = 0;
    /*bool is_spirv = *(uint32_t*)data == 0x07230203;
    if (is_spirv) {
        spirvflect_t* reflect = nullptr;
        spirvflect_create((uint32_t*)data, size, &reflect);
        *entry = reflect->entry_point;
        for (uint32_t idx = 0; idx < reflect->uniform_count; ++idx, uniform_count++)
        {
            auto current_uniform = reflect->uniforms[idx];
            strncpy(uniforms[uniform_count].name, current_uniform.name, sizeof(uniforms[uniform_count].name) - 2);
            uniforms[uniform_count].binding = current_uniform.binding;
            if (reflect->uniforms[idx].type == SpvOpTypeStruct)
            {
                uniforms[uniform_count].type = gfx_uniform_ubo;
                uniforms[uniform_count].buffer.size = current_uniform.size;
                uniforms[uniform_count].buffer.field_count = current_uniform.field_count;
                for (size_t field_idx = 0; field_idx < current_uniform.field_count; field_idx++)
                {
                    auto current_field = current_uniform.fields[field_idx];
                    strcpy(uniforms[uniform_count].buffer.fields[field_idx].name, current_field.name);
                    uniforms[uniform_count].buffer.fields[field_idx].offset = current_field.offset;
                }
            }
            if (reflect->uniforms[idx].type == SpvOpTypeSampler) {
                uniforms[uniform_count].type = gfx_uniform_sampler;
            }
            if (reflect->uniforms[idx].type == SpvOpTypeImage) {
                uniforms[uniform_count].type = gfx_uniform_texture2d;
            }
        }
    } else if (!is_spirv) {
        WGPUBindGroupLayoutEntry bgle[16] = {};
        uniform_count = wglsl_reflect((const char*)data, uniforms, bgle);
    }
    */
    return uniform_count;
}

static void create_default_resources(gfx_context_t* ctx) {
    wgpu_context_t * wctx = from_ctx(ctx);
    uint8_t* pixels = (uint8_t*)calloc(16 * 16 * 4, sizeof(uint8_t));
    if (pixels != nullptr) {
        for (int i = 0; i < 16 * 16; ++i) {
            pixels[i * 4 + 0] = rand() % 255;
            pixels[i * 4 + 1] = rand() % 255;
            pixels[i * 4 + 2] = rand() % 255;
            pixels[i * 4 + 3] = 255;
        }

        gfx_texture_desc_t desc = {"default", 16, 16, 1};
        desc.format = gfx_pixel_format_rgba8;
        desc.data = pixels;
        desc.mip_levels = 1;
        wgpu_create_texture(ctx, &desc, &wctx->default_texture);

        free(pixels);
    }

    gfx_sampler_desc_t sampler_desc = {};
        sampler_desc.minmag = gfx_filter_point;
        sampler_desc.mipmap = gfx_filter_point;
        sampler_desc.mode = gfx_address_mode_repeat;
        sampler_desc.anisotropy = 1;
    wgpu_create_sampler(ctx, &sampler_desc, &wctx->default_sampler);
}


static void request_adaprer_cb(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata)
{
    printf("request_adaprer_cb status:(%d)\n", status);
    WGPUAdapter* result = (WGPUAdapter*)userdata;
    *result = adapter;
}

static void request_device_cb(WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata)
{
    printf("request_device_cb status:(%d)\n", status);
    WGPUDevice* result = (WGPUDevice*)userdata;
    *result = device;
}

static void shader_compilation_cb(WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const* compilationInfo, void* userdata)
{
    wgpu_context_t* wctx = from_ctx((gfx_context_t*)userdata);
    for (uint32_t i = 0; i < compilationInfo->messageCount; i++)
    {
        auto message = compilationInfo->messages[i];
        wctx->dbglog(gfx_msg_info,"(%llu:%llu)%s", message.lineNum, message.linePos, message.message);
    }
}

static void default_log(gfx_msg type, const char* msg, ...)
{
}

// --- CONTEXT ---

void wgpu_init(gfx_settings_t* settings, gfx_context_t** ctx)
{
#ifdef EMSCRIPTEN
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);

    auto yield = [](int ms) { emscripten_sleep(ms); };
#else
    auto yield = [](int ms) { Sleep(ms); };
#endif

    wgpu_context_t* wctx = (wgpu_context_t*)calloc(1, sizeof(wgpu_context_t));
    if (!wctx)
        return;

    *ctx = &wctx->handle;

    WGPUInstance instance = wgpuCreateInstance(nullptr);

    WGPURequestAdapterOptions adapter_options = {};
    adapter_options.powerPreference =  WGPUPowerPreference_HighPerformance;
    adapter_options.backendType = WGPUBackendType_D3D12;//WGPUBackendType_Vulkan;

    WGPUAdapter adapter = nullptr;
    wgpuInstanceRequestAdapter(instance, &adapter_options, request_adaprer_cb, &adapter);
    while (adapter == nullptr)
        yield(10);

    WGPUFeatureName features[128] = {};
    uint32_t supported_features = (uint32_t)wgpuAdapterEnumerateFeatures(adapter, features);

   // WGPUFeatureName features[] = { WGPUFeatureName_TextureCompressionBC, WGPUFeatureName_BGRA8UnormStorage };
    WGPUDeviceDescriptor descriptor = {};
    descriptor.requiredFeatures = features;

#ifdef EMSCRIPTEN
    descriptor.requiredFeaturesCount = supported_features;
#else
    descriptor.requiredFeatureCount = supported_features;
#endif

    WGPUDevice device = nullptr;
    wgpuAdapterRequestDevice(adapter, &descriptor, request_device_cb, &device);
    while (device == nullptr)
        yield(10);

    wctx->instance = instance;
    wctx->adapter = adapter;
    wctx->device = device;
    wctx->queue = wgpuDeviceGetQueue(wctx->device);
    wctx->dbglog = settings->dbglog ? settings->dbglog : default_log;


    GFX_VERBOSE(wctx->dbglog(gfx_msg_info, "wgpu_init()"))

    wgpuDeviceGetLimits(device, &wctx->limits);
    wgpuAdapterGetProperties(adapter, &wctx->properties);

    // wgpuDevicePoll(device, true, NULL);

    auto adaptertype = [](WGPUAdapterType type) -> const char* {
        switch(type) {
            case WGPUAdapterType_DiscreteGPU:   return "discrete GPU";
            case WGPUAdapterType_IntegratedGPU: return "integrated GPU";
            case WGPUAdapterType_CPU:           return "CPU";
            default: break;
        }
        return "unknown";
    };

    auto backendtype = [](WGPUBackendType type) -> const char * {
        switch (type) {
            case WGPUBackendType_WebGPU:    return "WebGPU";
            case WGPUBackendType_D3D11:     return "D3D11";
            case WGPUBackendType_D3D12:     return "D3D12";
            case WGPUBackendType_Metal:     return "Metal";
            case WGPUBackendType_Vulkan:    return "Vulkan";
            case WGPUBackendType_OpenGL:    return "OpenGL";
            case WGPUBackendType_OpenGLES:  return "OpenGLES";
            default: break;
        }
        return "unknown";
    };

    wctx->dbglog(gfx_msg_info, "gfx init with webgpu(%s) backend: \n\
                                device: %s(%s) \n\
                                driver: %s",
        backendtype(wctx->properties.backendType),
        adaptertype(wctx->properties.adapterType),
        wctx->properties.name, wctx->properties.driverDescription
    );

#ifndef EMSCRIPTEN
    wgpuDeviceSetLoggingCallback(device, [](WGPULoggingType type, char const* message, void* userdata) {
        printf("%d: %s\n", type, message);
    }, wctx);

    wgpuDeviceSetUncapturedErrorCallback(device, [](WGPUErrorType type, const char* message, void* userdata) {
        printf("%d: %s\n", type, message);
    }, wctx);
#endif

    gfx_handle_pool_create(sizeof(wgpu_sampler_t),  16,    &wctx->sampler_pool,  nullptr);
    gfx_handle_pool_create(sizeof(wgpu_texture_t),  1024,  &wctx->texture_pool,  nullptr);
    gfx_handle_pool_create(sizeof(wgpu_buffer_t),   4096,  &wctx->buffer_pool,   nullptr);
    gfx_handle_pool_create(sizeof(wgpu_shader_t),   512,   &wctx->shader_pool,   nullptr);
    gfx_handle_pool_create(sizeof(wgpu_pipeline_t), 512,   &wctx->pipeline_pool, nullptr);


    gfx_buffer_desc_t desc = {};
    desc.usage  = gfx_buffer_usage_staging;
    desc.label  = "staging";
    desc.size   = settings->limits.staging_buffer_size;
    wgpu_create_buffer(*ctx, &desc, &wctx->staging_buffer);

    create_default_resources(*ctx);
}

// --- SWAPCHAIN ---

void wgpu_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t ** out_swapchain)
{
    wgpu_context_t* wctx = from_ctx(ctx);

    GFX_VERBOSE(wctx->dbglog(gfx_msg_info, "wgpu_create_swapchain(%p, %d)", ctx, handle));

    WGPUSwapChainDescriptor desc = {0};
        desc.usage = WGPUTextureUsage_RenderAttachment;
        desc.format = WGPUTextureFormat_BGRA8Unorm;
        desc.presentMode = WGPUPresentMode_Fifo;
    WGPUSurfaceDescriptor surface_descriptor = {0};

    get_window_size(handle, (int*)&desc.width, (int*)&desc.height);

#ifdef EMSCRIPTEN
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvDesc = { nullptr, WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector };
        canvDesc.selector = "canvas";
    surface_descriptor.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvDesc);
#else
    WGPUSurfaceDescriptorFromWindowsHWND wnddesc = { nullptr, WGPUSType_SurfaceDescriptorFromWindowsHWND };
        wnddesc.hwnd = (void*)handle;
        wnddesc.hinstance = GetModuleHandle(nullptr);
    surface_descriptor.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wnddesc);
#endif
    // wgpuDeviceCreateSwapChain is outdated
    //
    // https://github.com/gfx-rs/wgpu-native/blob/trunk/examples/triangle/main.c
    // wgpuSurfaceConfigure(surface, &config);
    // WGPUSurfaceTexture surface_texture;
    // loop
    //      wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
    //      WGPUTextureView frame = wgpuTextureCreateView(surface_texture.texture, NULL);
    //

    WGPUSurface surface = wgpuInstanceCreateSurface(wctx->instance, &surface_descriptor);

    WGPUSurfaceCapabilities capabilities = { 0 };
    wgpuSurfaceGetCapabilities(surface, wctx->adapter, &capabilities);
    desc.format = capabilities.formats ? capabilities.formats[0] : wgpuSurfaceGetPreferredFormat(surface, wctx->adapter);
    desc.format = capabilities.formats ? capabilities.formats[0] : wgpuSurfaceGetPreferredFormat(surface, wctx->adapter);


    WGPUSwapChain swapchain = wgpuDeviceCreateSwapChain(wctx->device, surface, &desc);

    GFX_VERBOSE(wctx->dbglog(gfx_msg_info, "    (%d   %d   %d)",
                      desc.width, desc.height, desc.format));

    wctx->surface_format = desc.format;
    wgpu_render_target_t* target = (wgpu_render_target_t*)calloc(1, sizeof(wgpu_render_target_t));
    wgpu_swapchain_t * chain = (wgpu_swapchain_t*)calloc(1, sizeof(wgpu_swapchain_t));
    if(chain != nullptr) {
        chain->window_handle    = handle;
        chain->format           = desc.format;
        chain->surface          = surface;
        chain->swapchain        = swapchain;
        chain->target           = target;

        chain->config.device        = wctx->device;
        chain->config.usage         = WGPUTextureUsage_RenderAttachment;
        chain->config.format        = desc.format;
        chain->config.presentMode   = WGPUPresentMode_Fifo;
        chain->config.alphaMode     = capabilities.alphaModes[0];
        chain->config.width         = desc.width;
        chain->config.height        = desc.height;
    }

    wgpu_texture_t* depth_texture = (wgpu_texture_t*)calloc(1, sizeof(wgpu_texture_t));
    if(depth_texture == nullptr)
        return;

    uint32_t sample_count = 1;
    WGPUTextureFormat depth_format = WGPUTextureFormat_Depth24PlusStencil8;

    WGPUTextureDescriptor depth_texture_desc = { };
        depth_texture_desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        depth_texture_desc.format = depth_format;
        depth_texture_desc.dimension = WGPUTextureDimension_2D;
        depth_texture_desc.mipLevelCount = 1;
        depth_texture_desc.sampleCount = sample_count;
        depth_texture_desc.size = { desc.width, desc.height, 1};
        depth_texture->texture = wgpuDeviceCreateTexture(wctx->device, &depth_texture_desc);

    WGPUTextureViewDescriptor depth_view_desc = { };
        depth_view_desc.format = depth_texture_desc.format;
        depth_view_desc.dimension = WGPUTextureViewDimension_2D;
        depth_view_desc.baseMipLevel = 0;
        depth_view_desc.mipLevelCount = 1;
        depth_view_desc.baseArrayLayer = 0;
        depth_view_desc.arrayLayerCount = 1;
        depth_view_desc.aspect = WGPUTextureAspect_All;
    depth_texture->texture_view = wgpuTextureCreateView(depth_texture->texture, &depth_view_desc);

    if(chain != nullptr && chain->target != nullptr)
        chain->target->depth_stencil_atachment = depth_texture;

    if(chain != nullptr)
        *out_swapchain = &chain->handle;
}


int32_t wgpu_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* in_swapchain, gfx_render_target_t** target)
{
    wgpu_context_t* wgpu_context = from_ctx(ctx);
    wgpu_swapchain_t* wgpu_swapchain = (wgpu_swapchain_t*)in_swapchain;

    WGPUSwapChain swapchain = wgpu_swapchain->swapchain;
    wgpu_swapchain->backbuffer_view = wgpuSwapChainGetCurrentTextureView(swapchain);
    wgpu_swapchain->target->view = wgpu_swapchain->backbuffer_view;
    *target = &wgpu_swapchain->target->handle;
    return 0;
}


void wgpu_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx)
{
    wgpu_context_t* wgpu_context = from_ctx(ctx);
    wgpu_swapchain_t* wgpu_swapchain = (wgpu_swapchain_t*)swapchain;

    wgpuSwapChainPresent(wgpu_swapchain->swapchain);
    wgpuTextureViewRelease(wgpu_swapchain->backbuffer_view);

    int width = 0;
    int height = 0;
    get_window_size(wgpu_swapchain->window_handle, &width, &height);
    if (wgpu_swapchain->config.width != width || wgpu_swapchain->config.height != height) {
        wgpu_swapchain->config.width = width;
        wgpu_swapchain->config.height = height;
        wgpuSurfaceConfigure(wgpu_swapchain->surface, &wgpu_swapchain->config);
    }
}

// --- BUFFER ---

void wgpu_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    wgpu_context_t* wgpu_ctx = from_ctx(ctx);

    GFX_VERBOSE(wgpu_ctx->dbglog(gfx_msg_info, "wgpu_create_buffer(%s, %d, %p,)",
        gfx_to_string(desc->usage), desc->size, desc->size));

    uint32_t size = gfx_utils_align_up(desc->size, 4);

    void* data_ptr = nullptr;
    bool mapped = desc->data != nullptr && desc->size > 0;

    if (desc->mapped /*|| desc->usage == gfx_buffer_usage_uniform*/)
        mapped = true;

    WGPUBufferUsageFlags usage = WGPUBufferUsage_None;
    switch(desc->usage) {
        case gfx_buffer_usage_staging:   usage = WGPUBufferUsage_MapWrite   | WGPUBufferUsage_CopySrc; break;
        case gfx_buffer_usage_index:     usage = WGPUBufferUsage_Index      | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_vertex:    usage = WGPUBufferUsage_Vertex     | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_uniform:   usage = WGPUBufferUsage_Uniform    | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_storage:   usage = WGPUBufferUsage_Storage    | WGPUBufferUsage_CopyDst; break;
        case gfx_buffer_usage_indirect:  usage = WGPUBufferUsage_Indirect   | WGPUBufferUsage_CopyDst; break;
    };

    WGPUBufferDescriptor buffer_desc = {};
        buffer_desc.label   = gfx_to_string(desc->usage);//"user vertex buffer";
        buffer_desc.usage   = usage;
        buffer_desc.size    = size;
        buffer_desc.mappedAtCreation = mapped;
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(wgpu_ctx->device, &buffer_desc);

    uint64_t handle = 0;
    auto wgpu_buffer = (wgpu_buffer_t*)gfx_handle_pool_allocate_data(wgpu_ctx->buffer_pool, &handle);
    if (wgpu_buffer != nullptr) {
        wgpu_buffer->handle = { handle };
        wgpu_buffer->buffer = buffer;
        wgpu_buffer->usage = gfx_usage_2_wgpu(desc->usage);
        wgpu_buffer->data_ptr = data_ptr;
        wgpu_buffer->size = size;
        *out_buffer = &wgpu_buffer->handle;
    }

    if (wgpu_buffer != nullptr && (desc->data != nullptr && desc->size > 0))
        wgpu_update_buffer_data(ctx, &wgpu_buffer->handle, desc->data, desc->size, 0);
}


void wgpu_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* dst_buffer, void* data, uint32_t size, uint32_t offset)
{
    wgpu_context_t* wctx = from_ctx(ctx);

    auto dst_buff = (wgpu_buffer_t*)gfx_handle_pool_map(wctx->buffer_pool, dst_buffer->idx);
    auto src_buff = (wgpu_buffer_t*)gfx_handle_pool_map(wctx->buffer_pool, wctx->staging_buffer->idx);

    auto asize = gfx_utils_align_up(size, 4);

    wgpuQueueWriteBuffer(wctx->queue, dst_buff->buffer, offset, data, asize);
}

void wgpu_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer)
{
    wgpu_buffer_t* wgpu_buffer = (wgpu_buffer_t*)buffer;
    wgpuBufferRelease(wgpu_buffer->buffer);
}

// --- SHADER ---

void wgpu_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader)
{
    wgpu_context_t* wgpu_ctx = from_ctx(ctx);

    GFX_VERBOSE(wgpu_ctx->dbglog(gfx_msg_info, "wgpu_create_shader : %s", desc->label ? desc->label : ""));

    for (uint32_t i = 0; i < desc->stage_count; ++i)
    {
        if (desc->stages[i].data != nullptr && desc->stages[i].size > 0) {
            GFX_VERBOSE( wgpu_ctx->dbglog(gfx_msg_info, "    %s", gfx_to_string((gfx_shader_stage)i)) );
        }
    }

    wgpu_shader_t* wgpu_shader = (wgpu_shader_t*)calloc(1, sizeof(wgpu_shader_t));
    if(wgpu_shader == nullptr) {
        wgpu_ctx->dbglog(gfx_msg_error, "failed allocating wgpu_shader_t");
        return;
    }

    *out_shader = &wgpu_shader->handle;
    wgpu_shader->context = wgpu_ctx;
    wgpu_shader->hash = 0;

    uint32_t ubo_size = 0;
    uint32_t uniform_count = 0;
    uint32_t wgsl_info_count = 0;
    WGPUBindGroupLayoutEntry* layout_entries = wgpu_shader->bindings;

    for (size_t stageIdx = 0; stageIdx < desc->stage_count; ++stageIdx)
    {
        if (desc->stages[stageIdx].data == nullptr || desc->stages[stageIdx].size == 0)
            continue;

        uint32_t stage_hash = gfx_utils_hash((char*)desc->stages[stageIdx].data, desc->stages[stageIdx].size, wgpu_shader->hash);

        gfx_shader_stage stage = desc->stages[stageIdx].stage;
        bool is_spirv = *(uint32_t*)desc->stages[stageIdx].data == 0x07230203;

        wgpu_shader->flags = wgpu_shader->flags | 1 << stage;

        WGPUShaderModuleWGSLDescriptor wgsl_desc = { nullptr , WGPUSType_ShaderModuleWGSLDescriptor };
        wgsl_desc.code = (const char*)desc->stages[stageIdx].data;

        WGPUShaderModuleSPIRVDescriptor spirv_desc = {nullptr, WGPUSType_ShaderModuleSPIRVDescriptor};
        spirv_desc.code = (uint32_t*)desc->stages[stageIdx].data;
        spirv_desc.codeSize = desc->stages[stageIdx].size / 4;

        WGPUShaderModuleDescriptor descriptor = {};
        descriptor.nextInChain = is_spirv ? reinterpret_cast<WGPUChainedStruct*>(&spirv_desc):
                                            reinterpret_cast<WGPUChainedStruct*>(&wgsl_desc);

        WGPUShaderModule module = wgpuDeviceCreateShaderModule(wgpu_ctx->device, &descriptor);
/*
#ifndef EMSCRIPTEN
        wgpuShaderModuleGetCompilationInfo(module, shader_compilation_cb, wgpu_ctx);
#endif*/

        gfx_uniform_t* uniforms = &wgpu_shader->uniforms[wgsl_info_count];
        const char* entry = nullptr;
        uniform_count = get_shader_uniforms((uint32_t*)desc->stages[stageIdx].data,
                                            desc->stages[stageIdx].size, uniforms, &entry);

        WGPUShaderStageFlags visibility = WGPUShaderStage_None;
        switch(stage)
        {
            case gfx_shader_vertex:
                visibility = WGPUShaderStage_Vertex;
                wgpu_shader->vertex.module = module;
                wgpu_shader->vertex.entryPoint = entry ? entry: "main";
                break;

            case gfx_shader_fragment:
                visibility = WGPUShaderStage_Fragment;
                wgpu_shader->fragment.module = module;
                wgpu_shader->fragment.entryPoint = entry ? entry : "main";
                break;

            case gfx_shader_compute:
                visibility = WGPUShaderStage_Compute;
                wgpu_shader->compute.module = module;
                wgpu_shader->compute.entryPoint = entry ? entry : "main";
                break;

            default:
                assert(false);
                break;
        }

        for (uint32_t u = 0; u < uniform_count; ++u)
        {
            if(uniforms[u].type == gfx_uniform_ubo)
                ubo_size += uniforms[u].buffer.size;
            layout_entries[wgsl_info_count].binding = uniforms[u].binding;
            layout_entries[wgsl_info_count].visibility = visibility;

            switch (uniforms[u].type)
            {
                case gfx_uniform_storage_buffer:
                    layout_entries[wgsl_info_count].buffer.type = WGPUBufferBindingType_Storage;
                    break;

                case gfx_uniform_ubo:
                    layout_entries[wgsl_info_count].buffer.type = WGPUBufferBindingType_Uniform;
                    break;

                case gfx_uniform_sampler:
                    layout_entries[wgsl_info_count].sampler.type = WGPUSamplerBindingType_Filtering;
                    break;

                case gfx_uniform_texture2d:
                    layout_entries[wgsl_info_count].texture.viewDimension = WGPUTextureViewDimension_2D;
                    layout_entries[wgsl_info_count].texture.sampleType    = WGPUTextureSampleType_Float;
                    break;

                case gfx_uniform_texture3d:
                    layout_entries[wgsl_info_count].texture.viewDimension = WGPUTextureViewDimension_3D;
                    layout_entries[wgsl_info_count].texture.sampleType    = WGPUTextureSampleType_Float;
                    break;

                case gfx_uniform_texture2d_array:
                    layout_entries[wgsl_info_count].texture.viewDimension = WGPUTextureViewDimension_2DArray;
                    layout_entries[wgsl_info_count].texture.sampleType    = WGPUTextureSampleType_Float;
                    break;

                case gfx_uniform_texture2d_cube:
                    layout_entries[wgsl_info_count].texture.viewDimension = WGPUTextureViewDimension_Cube;
                    layout_entries[wgsl_info_count].texture.sampleType    = WGPUTextureSampleType_Float;
                    break;

                default: assert(false); break;
             }
            wgsl_info_count++;
        }
    } // for (size_t stageIdx = 0; stageIdx < gfx_shader_count; ++stageIdx)
    wgpu_shader->binding_count = wgsl_info_count;
    wgpu_shader->uniform_count = wgsl_info_count;

    WGPUBindGroupLayoutDescriptor layout_descriptor = {};
    layout_descriptor.entryCount = wgsl_info_count;
    layout_descriptor.entries = layout_entries;
    wgpu_shader->layout = wgpuDeviceCreateBindGroupLayout(wgpu_ctx->device, &layout_descriptor);

    WGPUPipelineLayoutDescriptor pipeline_layout_desc = {};
    pipeline_layout_desc.bindGroupLayoutCount = 1;
    pipeline_layout_desc.bindGroupLayouts = &wgpu_shader->layout;
    wgpu_shader->pipeline_layout = wgpuDeviceCreatePipelineLayout(wgpu_ctx->device, &pipeline_layout_desc);

    /*
    uint32_t descriptor_count = 64;
    uint32_t buffer_size = descriptor_count * ubo_size;

    gfx_buffer_desc_t buffer_desc = {};
        buffer_desc.usage = gfx_buffer_usage_uniform;
        buffer_desc.mapped = true;
        buffer_desc.size = buffer_size;  //
    wgpu_create_buffer(ctx, &buffer_desc, &wgpu_shader->buffer);

    gfx_pool_create(sizeof(wgpu_descriptor_set_t), descriptor_count, &wgpu_shader->descriptor_set_pool);*/
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

uint64_t wgpu_uniform_location(gfx_shader_t * shader, const char* name)
{
    wgpu_shader_t* wgpu_shader = (wgpu_shader_t*)shader;

    if(wgpu_shader == nullptr || name == nullptr)
        return 0;

    for(uint32_t i = 0; i < wgpu_shader->uniform_count; ++i)
    {
        auto uniform = &wgpu_shader->uniforms[i];
        if (strstr(uniform->name, name)) {
            return uniform_handle_t::create(wgpu_shader->hash, 0, i, 0);
        }

        if(wgpu_shader->uniforms[i].type == gfx_uniform_ubo)
        {
            for (uint32_t j = 0; j < wgpu_shader->uniforms[i].buffer.field_count; ++j)
            {
                if (strstr(uniform->buffer.fields[j].name, name)) {
                    return uniform_handle_t::create(wgpu_shader->hash, 0, i, j);
                }
            }
        }
    }

    return 0;
}

void wgpu_destroy_shader(gfx_context_t* ctx, gfx_shader_t* shader)
{
    wgpu_shader_t* wgpu_shader = (wgpu_shader_t*)shader;

    if(wgpu_shader->fragment.module != nullptr)
        wgpuShaderModuleRelease(wgpu_shader->fragment.module);

    if (wgpu_shader->vertex.module != nullptr)
        wgpuShaderModuleRelease(wgpu_shader->vertex.module);
}

// --- SAMPLER ---

void wgpu_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler)
{
    wgpu_context_t* wgpu_context = from_ctx(ctx);

    WGPUSamplerDescriptor  descriptor = {0};
       // descriptor.label = desc-> "LUT BRDF texture sampler";
        descriptor.addressModeU = gfx_address_mode_2_webgpu(desc->mode);
        descriptor.addressModeV = gfx_address_mode_2_webgpu(desc->mode);
        descriptor.addressModeW = gfx_address_mode_2_webgpu(desc->mode);
        descriptor.minFilter = gfx_filter_2_webgpu(desc->minmag);
        descriptor.magFilter = gfx_filter_2_webgpu(desc->minmag);
        descriptor.mipmapFilter = gfx_mipmap_filter_2_webgpu(desc->mipmap);
        descriptor.lodMinClamp = 0.0f;
        descriptor.lodMaxClamp = 10.0f;
        descriptor.maxAnisotropy = desc->anisotropy;
    auto wgpu_sampler =  wgpuDeviceCreateSampler( wgpu_context->device, &descriptor);

    wgpu_sampler_t * sampler = (wgpu_sampler_t*)calloc(1, sizeof(wgpu_sampler_t));
    if(sampler != nullptr)
    {
        sampler->sampler = wgpu_sampler;
        *out_sampler = &sampler->handle;
    }
}


void wgpu_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler)
{
    wgpu_sampler_t* wgpu_sampler = (wgpu_sampler_t*) sampler;
    wgpuSamplerRelease(wgpu_sampler->sampler);
}

// --- TEXTURE ---

void wgpu_create_texture(gfx_context_t* ctx, gfx_texture_desc_t * desc, gfx_texture_t** out_texture)
{
    wgpu_context_t* wgpu_ctx = from_ctx(ctx);

    GFX_VERBOSE(wgpu_ctx->dbglog(gfx_msg_info, "wgpu_create_texture %s(%s %d, %d, %d, %s)",
                                 desc->label ? desc->label : "",
                                 gfx_to_string(desc->type),
                                 desc->width, desc->height, desc->depth, gfx_to_string(desc->format)));

    if(desc->format == gfx_pixel_format_bc6h || desc->format == gfx_pixel_format_bc7)
        printf("");

    bool is_srgb = (desc->format == gfx_pixel_format_bc6h || desc->format == gfx_pixel_format_bc7);

    WGPUTextureDescriptor texture_desc = {};
        texture_desc.label = desc->label ? desc->label : "";
        texture_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc | WGPUTextureUsage_TextureBinding;
        texture_desc.dimension = WGPUTextureDimension_2D;
        texture_desc.size = { desc->width, desc->height, desc->depth == 0 ? 1 : desc->depth };
        texture_desc.format = gfx_pixel_format_2_webgpu(desc->format, is_srgb);
        texture_desc.mipLevelCount = desc->mip_levels;
        texture_desc.sampleCount = 1;
    WGPUTexture texture = wgpuDeviceCreateTexture(wgpu_ctx->device, &texture_desc);

    WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.label = desc->label ? desc->label : "";
        texture_view_desc.format = gfx_pixel_format_2_webgpu(desc->format, is_srgb);
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = desc->mip_levels;
        texture_view_desc.baseArrayLayer = 0;
        texture_view_desc.arrayLayerCount = 1;
        texture_view_desc.aspect = WGPUTextureAspect_All;
    WGPUTextureView texture_view = wgpuTextureCreateView(texture, &texture_view_desc);

    #define webgpu_max(a, b)       ( a > b ? a : b )

    ptrdiff_t offset = 0;
    for (uint32_t face = 0; face < desc->depth; ++face) {
        for (uint32_t level = 0; level < desc->mip_levels; ++level) {
            uint32_t width  = webgpu_max(4, desc->width  >> level);
            uint32_t height = webgpu_max(4, desc->height >> level);
            uint32_t depth  = webgpu_max(1, desc->depth  >> level);

            uint32_t size = gfx_utils_image_layer_size(width, height, depth, desc->format);
            uint8_t* pixels = (uint8_t*)desc->data + offset;

            WGPUImageCopyTexture image = {};
            image.texture = texture;
            image.mipLevel = level;
            image.aspect = WGPUTextureAspect_All;

            WGPUTextureDataLayout layout = {};
            layout.rowsPerImage = height;
            layout.bytesPerRow = gfx_utils_image_row_pitch(desc->format, width);

            WGPUExtent3D copy_size = { (uint32_t)width, (uint32_t)height, 1 };
            wgpuQueueWriteTexture(wgpu_ctx->queue, &image, pixels, size, &layout, &copy_size);

            offset += size;
        }
    }

    wgpu_texture_t * wgpu_texture = (wgpu_texture_t*)calloc(1, sizeof(wgpu_texture_t));
    if(wgpu_texture != nullptr) {
        wgpu_texture->texture = texture;
        wgpu_texture->texture_view = texture_view;

        *out_texture = &wgpu_texture->handle;
     }
}


void wgpu_update_texture_data(gfx_context_t* ctx, gfx_texture_t* /*texture*/, void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    gfx_stub_not_implemented(wctx ? wctx->dbglog : nullptr, "wgpu_update_image_data");
}

void wgpu_update_bindless_texture(gfx_context_t* ctx, gfx_texture_t* /*texture*/, uint32_t /*idx*/)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    gfx_stub_not_implemented(wctx ? wctx->dbglog : nullptr, "wgpu_update_bindless_texture");
}

void wgpu_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* /*texture*/)
{
    // WebGPU has no built-in mipmap generation command. Requires a custom
    // downsample compute/render pass per level -- not implemented yet.
    wgpu_context_t* wctx = from_ctx(ctx);
    if (wctx) wctx->dbglog(gfx_msg_warning, "wgpu_texture_generate_mipmap: not supported natively, use pre-generated mips");
}

void wgpu_blit_image(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_texture_t* wsrc = (wgpu_texture_t*)src;
    wgpu_texture_t* wdst = (wgpu_texture_t*)dst;
    if (!wctx || !wsrc || !wdst) return;

    WGPUImageCopyTexture src_info = {};
    src_info.texture  = wsrc->texture;
    src_info.mipLevel = 0;
    src_info.aspect   = WGPUTextureAspect_All;

    WGPUImageCopyTexture dst_info = {};
    dst_info.texture  = wdst->texture;
    dst_info.mipLevel = 0;
    dst_info.aspect   = WGPUTextureAspect_All;

    WGPUExtent3D extent = {};
    extent.width               = wgpuTextureGetWidth(wsrc->texture);
    extent.height              = wgpuTextureGetHeight(wsrc->texture);
    extent.depthOrArrayLayers  = wgpuTextureGetDepthOrArrayLayers(wsrc->texture);

    WGPUCommandEncoderDescriptor enc_desc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wctx->device, &enc_desc);
    wgpuCommandEncoderCopyTextureToTexture(encoder, &src_info, &dst_info, &extent);

    WGPUCommandBufferDescriptor cmd_desc = {};
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &cmd_desc);
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
    wgpu_texture_t * wgpu_texture = (wgpu_texture_t*)texture;
    wgpuTextureRelease(wgpu_texture->texture);
    wgpuTextureViewRelease(wgpu_texture->texture_view);
}

// --- PIPELINE ---

void wgpu_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline)
{
    wgpu_context_t* wgpu_context = from_ctx(ctx);
    wgpu_shader_t* wgpu_shader = (wgpu_shader_t*)desc->shader;

    gfx_render_states_desc_t render_states = desc->render_states;

    WGPUBlendState blend = {};
    blend.color.operation       = gfx_blend_op_2_webgpu(render_states.blend.color_op);      // WGPUBlendOperation_Add;
    blend.color.srcFactor       = gfx_blend_mode_2_webgpu(render_states.blend.color_src);   // WGPUBlendFactor_One;
    blend.color.dstFactor       = gfx_blend_mode_2_webgpu(render_states.blend.color_dst);   // WGPUBlendFactor_One;
    blend.alpha.operation       = gfx_blend_op_2_webgpu(render_states.blend.alpha_op);      // WGPUBlendOperation_Add;
    blend.alpha.srcFactor       = gfx_blend_mode_2_webgpu(render_states.blend.alpha_src);   // WGPUBlendFactor_One;
    blend.alpha.dstFactor       = gfx_blend_mode_2_webgpu(render_states.blend.alpha_dst);   // WGPUBlendFactor_One;

    WGPUColorTargetState color_state = {};
    color_state.format          = wgpu_context->surface_format;
    color_state.writeMask       = WGPUColorWriteMask_All;
    color_state.blend           = render_states.blend.enable ? &blend : nullptr;

    WGPUStencilFaceState stencil_state = {};
    stencil_state.compare       = gfx_cmp_2_webgpu(render_states.stencil.func);     // always;
    stencil_state.failOp        = gfx_op_2_webgpu(render_states.stencil.opFail);    // WGPUStencilOperation_Keep;
    stencil_state.depthFailOp   = gfx_op_2_webgpu(render_states.stencil.opZFail);   // WGPUStencilOperation_Keep;
    stencil_state.passOp        = gfx_op_2_webgpu(render_states.stencil.opPass);    // WGPUStencilOperation_Keep;

    WGPUDepthStencilState depth_state = {};
    depth_state.depthWriteEnabled   = render_states.depth.write;
    depth_state.depthCompare        = WGPUCompareFunction_Less;//gfx_cmp_2_webgpu(render_states.depth.mode);   // lequal
    depth_state.stencilFront        = stencil_state;
    depth_state.stencilBack         = stencil_state;
    depth_state.stencilReadMask     = 0x0;
    depth_state.stencilWriteMask    = 0x0;
    depth_state.depthBias           = 0;
    depth_state.depthBiasSlopeScale = 0.0f;
    depth_state.depthBiasClamp      = 0.0f;
    depth_state.format              = WGPUTextureFormat_Depth24PlusStencil8;


    WGPUVertexAttribute attributes[16] = {};
    WGPUVertexBufferLayout vertex_assembly[8] = {};
    for(uint32_t i = 0; i < desc->assembly.slot_count; ++i)
    {
        vertex_assembly[i].arrayStride     = desc->assembly.slots[i].stride;
        vertex_assembly[i].attributeCount  = desc->assembly.attribute_count;
        vertex_assembly[i].attributes      = attributes;
        for (uint32_t j = 0; j < desc->assembly.attribute_count; ++j) {
            attributes[j].offset          = desc->assembly.attributes[j].offset;
            attributes[j].shaderLocation  = desc->assembly.attributes[j].location;
            attributes[j].format          = gfx_vertex_format_2_webgpu(desc->assembly.attributes[j].format);
        }
        switch (desc->assembly.slots[i].rate)
        {
            case gfx_vertex_rate_vertex:    vertex_assembly[i].stepMode = WGPUVertexStepMode_Vertex;   break;
            case gfx_vertex_rate_instance:  vertex_assembly[i].stepMode = WGPUVertexStepMode_Instance; break;
        }
    }

    WGPUVertexState vertex_state = wgpu_shader->vertex;
    vertex_state.bufferCount = desc->assembly.slot_count;
    vertex_state.buffers = vertex_assembly;

    WGPUFragmentState fragment = {};
    uint32_t fragment_mask = 1u << gfx_shader_fragment;
    if((wgpu_shader->flags & fragment_mask) == fragment_mask)
    {
        wgpu_shader->fragment.targetCount = 1;
        wgpu_shader->fragment.targets = &color_state;
    }

    WGPUMultisampleState multisample    = { nullptr, 1, 0xFFFFFFFF , false};

    WGPUPrimitiveState   primitive      = { nullptr };
        primitive.topology  = gfx_topology_2_webgpu(desc->assembly.topology);
        primitive.frontFace = gfx_face_2_webgpu(render_states.face);
        primitive.cullMode  = gfx_cull_2_webgpu(render_states.culling);
        primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPURenderPipelineDescriptor descriptor = {};
    descriptor.layout       = wgpu_shader->pipeline_layout;
    descriptor.depthStencil = &depth_state;
    descriptor.multisample  = multisample;
    descriptor.primitive    = primitive;
    descriptor.vertex       = vertex_state;
    descriptor.fragment     = &wgpu_shader->fragment;
    descriptor.label        = "fuck";
    WGPURenderPipeline render_pipeline = wgpuDeviceCreateRenderPipeline(wgpu_context->device, &descriptor);

    wgpu_pipeline_t* pipeline = (wgpu_pipeline_t*)calloc(1, sizeof(wgpu_pipeline_t));
    if(pipeline != nullptr)
    {
        pipeline->pipeline = render_pipeline;
        *out_pipeline = &pipeline->handle;
    }
}

void wgpu_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** pipeline)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wctx->dbglog(gfx_msg_error, "wgpu_create_compute_pipeline not implemented");
}

void wgpu_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
    wgpu_pipeline_t * wgpu_pipeline = (wgpu_pipeline_t*)pipeline;
    wgpuRenderPipelineRelease(wgpu_pipeline->pipeline);
}

void wgpu_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline)
{
    // wgpu_create_compute_pipeline is not yet implemented — nothing to release
}

void wgpu_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wctx->dbglog(gfx_msg_error, "wgpu_create_mesh_pipeline not implemented");
}

void wgpu_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wctx->dbglog(gfx_msg_error, "wgpu_create_raytrace_pipeline not implemented");
}

void wgpu_destroy_mesh_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
}

void wgpu_destroy_raytrace_pipeline(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline)
{
}

// --- RENDER TARGET ---

void wgpu_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** out_target)
{
    wgpu_context_t * wctx = from_ctx(ctx);
    wctx->dbglog(gfx_msg_error, "wgpu_create_render_target not implemented");
}


void wgpu_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* texture)
{
}

// --- DESCRIPTOR SET ---

void wgpu_create_descriptor_set_pool(gfx_context_t* ctx, wgpu_shader_t * shader, uint32_t capacity, wgpu_descriptor_set_pool_t** out_pool)
{
    wgpu_context_t* wctx = from_ctx(ctx);

    uint32_t ubo_size = 0;
    for(uint32_t i = 0; i < shader->uniform_count; ++i)
    {
        if(shader->uniforms[i].type == gfx_uniform_ubo)
            ubo_size += shader->uniforms[i].buffer.size;
    }

    uint32_t aligment = wctx->limits.limits.minUniformBufferOffsetAlignment;
    ubo_size = gfx_utils_align_up(ubo_size, aligment);

    wgpu_descriptor_set_pool_t* pool = (wgpu_descriptor_set_pool_t*)calloc(1, sizeof(wgpu_descriptor_set_pool_t));
    pool->capacity = capacity;
    pool->free_set_count = capacity;
    pool->next_free = 0;

    uint32_t ubo_buffer_size = ubo_size * capacity;
    if(ubo_buffer_size > 0 )
    {
        gfx_buffer_desc_t desc = {};
            desc.usage = gfx_buffer_usage_uniform;
            desc.size = ubo_buffer_size;
        wgpu_create_buffer(ctx, &desc, &pool->ubo_buffer);
        pool->ubo_buffer_data_size = ubo_buffer_size;
        pool->ubo_buffer_data_ptr = (uint8_t*)calloc(ubo_buffer_size, 1);
    }
    pool->descriptor_sets = (wgpu_descriptor_set_t*)calloc(sizeof(wgpu_descriptor_set_t), capacity);

    for(uint32_t j = 0; j < capacity; ++j)
    {
        auto descriptor_set = &pool->descriptor_sets[j];
        descriptor_set->isfree = true;
        descriptor_set->shader = shader;
        descriptor_set->owner_pool = pool;
        descriptor_set->index_in_pool = j;

        auto ubo_offset = ubo_size * j;
        for (uint32_t i = 0; i < shader->binding_count; ++i)
        {
            descriptor_set->bindings[i].binding = shader->bindings[i].binding;

            if (shader->bindings[i].sampler.type != WGPUSamplerBindingType_Undefined)
                descriptor_set->bindings[i].sampler = ((wgpu_sampler_t*)wctx->default_sampler)->sampler;

            if (shader->bindings[i].texture.sampleType != WGPUTextureSampleType_Undefined)
                descriptor_set->bindings[i].textureView = ((wgpu_texture_t*)wctx->default_texture)->texture_view;

            if (shader->bindings[i].buffer.type != WGPUBufferBindingType_Undefined) {
                descriptor_set->bindings[i].buffer = ((wgpu_buffer_t*)pool->ubo_buffer)->buffer;
                descriptor_set->bindings[i].size = shader->uniforms[i].buffer.size;
                descriptor_set->bindings[i].offset = ubo_offset;
                descriptor_set->ubo_data_ptr = pool->ubo_buffer_data_ptr + ubo_offset;
                descriptor_set->dynamic_offset = ubo_offset;
            }
        }
        WGPUBindGroupDescriptor bg_desc = {};
            bg_desc.label       = "uniform_buffer_bind_group";
            bg_desc.layout      = shader->layout;
            bg_desc.entryCount  = shader->binding_count;
            bg_desc.entries     = descriptor_set->bindings;
        descriptor_set->bind_group = wgpuDeviceCreateBindGroup(wctx->device, &bg_desc);
    }

    *out_pool = pool;
}


void wgpu_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** out_descriptor)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_shader_t*  wgpu_shader = (wgpu_shader_t*)shader;

    if(wgpu_shader == nullptr)
    {
        wctx->dbglog(gfx_msg_error, "wgpu_create_descriptor_set - shader is null");
        return;
    }

    if(wgpu_shader->current_pool == nullptr)
    {
        wgpu_create_descriptor_set_pool(ctx, wgpu_shader, 256, &wgpu_shader->current_pool);
    }

    if(wgpu_shader->current_pool->free_set_count == 0)
    {
        wctx->dbglog(gfx_msg_warning, "TODO: push pool before allocarte new one");
        wgpu_create_descriptor_set_pool(ctx, wgpu_shader, 1024, &wgpu_shader->current_pool);
    }

    for (uint32_t i = wgpu_shader->current_pool->next_free; i < wgpu_shader->current_pool->capacity; i++)
    {
        if(wgpu_shader->current_pool->descriptor_sets[i].isfree){
            wgpu_shader->current_pool->descriptor_sets[i].isfree = false;

            wgpu_shader->current_pool->count++;
            wgpu_shader->current_pool->free_set_count--;
            wgpu_shader->current_pool->next_free = i + 1;

            *out_descriptor = (gfx_descriptor_set_t*)(&wgpu_shader->current_pool->descriptor_sets[i].handle);
            return;
        }
    }
}


void wgpu_uniform_update_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size)
{
    wgpu_descriptor_set_t* wgpu_set = (wgpu_descriptor_set_t*)set;
    wgpu_descriptor_set_pool_t * pool = wgpu_set->owner_pool;
    wgpu_shader_t* wgpu_shader = wgpu_set->shader;
    wgpu_context_t* wctx = wgpu_shader->context;

    uniform_handle_t uhandle = { handle };
    if(uhandle.hash != wgpu_shader->hash)
        return;

    uint64_t idx = uhandle.idx;
    uint16_t child_id = uhandle.field;

    pool->dirty = true;
    wgpu_buffer_t* wgpu_buffer = (wgpu_buffer_t*)wgpu_set->bindings[idx].buffer;
    if(wgpu_set->bindings[idx].buffer != nullptr)
    {
        WGPUBuffer buffer = wgpu_set->bindings[idx].buffer;
        uint64_t offset = wgpu_set->bindings[idx].offset;

        uint64_t field_offset = wgpu_shader->uniforms[idx].buffer.fields[child_id].offset;

        memcpy(wgpu_set->ubo_data_ptr + field_offset, data, size);

        //wgpuQueueWriteBuffer(wctx->queue, buffer, offset + field_offset, data, size);
    }
}

void wgpu_uniform_set_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* buff, uint32_t offset)
{
    wgpu_descriptor_set_t* wgpu_set = (wgpu_descriptor_set_t*)set;

    wgpu_shader_t*  wgpu_shader  = wgpu_set->shader;
    wgpu_context_t* wctx = wgpu_shader->context;
    wgpu_buffer_t*  wgpu_buffer  = (wgpu_buffer_t*)buff;

    uint64_t idx = handle;
    if(wgpu_set->bindings[idx].buffer != wgpu_buffer->buffer)
    {
        wgpu_set->bindings[idx].buffer = wgpu_buffer->buffer;
        wgpu_set->bindings[idx].offset = offset;

        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout = wgpu_shader->layout;
        bg_desc.entryCount = wgpu_shader->binding_count;
        bg_desc.entries = wgpu_set->bindings;
        WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(wctx->device, &bg_desc);

        if (wgpu_set->bind_group)
            wgpuBindGroupRelease(wgpu_set->bind_group);

        wgpu_set->bind_group = bind_group;
    }
}


void wgpu_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture)
{
    wgpu_descriptor_set_t * wgpu_set = (wgpu_descriptor_set_t*)set;

    wgpu_shader_t*  wgpu_shader  = wgpu_set->shader;
    wgpu_context_t* wctx = wgpu_shader->context;
    wgpu_texture_t* wgpu_texture = (wgpu_texture_t*)texture;

    if(wgpu_texture == nullptr)
    {
        return;
    }

    uniform_handle_t uh { handle };
    uint64_t idx = uh.idx;
    if(wgpu_set->bindings[idx].textureView != wgpu_texture->texture_view)
    {
        wgpu_set->bindings[idx].textureView = wgpu_texture->texture_view;

        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout     = wgpu_shader->layout;
        bg_desc.entryCount = wgpu_shader->binding_count;
        bg_desc.entries    = wgpu_set->bindings;
        WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(wctx->device, &bg_desc);

        if(wgpu_set->bind_group)
            wgpuBindGroupRelease(wgpu_set->bind_group);

        wgpu_set->bind_group = bind_group;
    }
}


void wgpu_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler)
{
    wgpu_descriptor_set_t* wgpu_set = (wgpu_descriptor_set_t*)set;
    wgpu_shader_t* wgpu_shader = wgpu_set->shader;
    wgpu_context_t* wgpu_ctx = wgpu_shader->context;
    wgpu_sampler_t* wgpu_sampler = (wgpu_sampler_t*)sampler;

    uniform_handle_t uh{ handle };
    if(uh.hash != wgpu_shader->hash)
        return;

    uint64_t idx = uh.idx;
    if (wgpu_set->bindings[idx].sampler != wgpu_sampler->sampler)
    {
        wgpu_set->bindings[idx].sampler = wgpu_sampler->sampler;

        WGPUBindGroupDescriptor bg_desc = {};
            bg_desc.layout = wgpu_shader->layout;
            bg_desc.entryCount = wgpu_shader->binding_count;
            bg_desc.entries = wgpu_set->bindings;
        WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(wgpu_ctx->device, &bg_desc);
        if (wgpu_set->bind_group)
            wgpuBindGroupRelease(wgpu_set->bind_group);

        wgpu_set->bind_group = bind_group;
    }
}


void wgpu_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor)
{
    wgpu_descriptor_set_t* wgpu_descriptor = (wgpu_descriptor_set_t*)descriptor;
    wgpu_descriptor_set_pool_t * pool = wgpu_descriptor->owner_pool;

    wgpu_descriptor->isfree = true;
    pool->next_free = wgpu_descriptor->index_in_pool;
    pool->count--;

    wgpu_shader_t* shader = wgpu_descriptor->shader;
}

// --- COMMAND BUFFER ---

void wgpu_create_cmd(gfx_context_t* ctx, gfx_command_buffer_t** out_cmd)
{
    wgpu_context_t* wctx = from_ctx(ctx);

    wgpu_command_buffer_t * wgpu_cmd = (wgpu_command_buffer_t*)calloc(1, sizeof(wgpu_command_buffer_t));
    if(wgpu_cmd != nullptr) {
        wgpu_cmd->ctx = wctx;
        *out_cmd = &wgpu_cmd->handle;
    }
    //wgpu_cmd->encoder = wgpuDeviceCreateCommandEncoder(wgpu_context->device, nullptr);
}


void wgpu_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    printf("gfx_destroy_cmd not inmplemented\n");
}

void wgpu_cmd_begin(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_context_t* wctx = wgpu_cmd->ctx;

    if(wgpu_cmd->encoder != nullptr)
        assert(false);

    wgpu_cmd->encoder = wgpuDeviceCreateCommandEncoder(wctx->device, nullptr);
}


void wgpu_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* pass)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_render_target_t* wgpu_target = (wgpu_render_target_t*)pass->target;

    WGPUColor clearcolor = { 0.2f, 0.3f, 0.4f, 1.0f };

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = wgpu_target->view;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.2f, 0.3f, 0.4f, 1.0f};
    #ifndef EMSCRIPTEN
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    #endif

    bool has_depth = wgpu_target->depth_stencil_atachment != nullptr;
    WGPUTextureView depht_view = has_depth ? wgpu_target->depth_stencil_atachment->texture_view : nullptr;

    WGPURenderPassDepthStencilAttachment depthAttachment = {};
        depthAttachment.view = depht_view;

        depthAttachment.depthClearValue = 1.0;
        depthAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthAttachment.depthReadOnly = false;

        depthAttachment.stencilClearValue = 0;
        depthAttachment.stencilLoadOp = WGPULoadOp_Clear;
        depthAttachment.stencilStoreOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor renderPass = {};
    renderPass.colorAttachmentCount = 1;
    renderPass.colorAttachments = &colorAttachment;
    renderPass.depthStencilAttachment = has_depth ? &depthAttachment : nullptr;

    wgpu_cmd->pass = wgpuCommandEncoderBeginRenderPass(wgpu_cmd->encoder, &renderPass);
}


void wgpu_cmd_end_pass(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;

    wgpuRenderPassEncoderEnd(wgpu_cmd->pass);
    wgpuRenderPassEncoderRelease(wgpu_cmd->pass);
    wgpu_cmd->pass = nullptr;
}


void wgpu_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpuRenderPassEncoderSetScissorRect(wgpu_cmd->pass, x, y, w, h);
}


void wgpu_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpuRenderPassEncoderSetViewport(wgpu_cmd->pass, (float)x, (float)y, (float)w, (float)h, 0.0f, 1.0f);
}


void wgpu_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_pipeline_t * wgpu_pipeline = (wgpu_pipeline_t*)pipeline;

    wgpuRenderPassEncoderSetPipeline(wgpu_cmd->pass, wgpu_pipeline->pipeline);
}


void wgpu_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* set)
{
    wgpu_command_buffer_t*  wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_descriptor_set_t*  wgpu_set = (wgpu_descriptor_set_t*)set;
    wgpu_descriptor_set_pool_t*  pool = wgpu_set->owner_pool;
    wgpu_context_t *        wgpu_ctx  = wgpu_cmd->ctx;

    if(pool->dirty)
    {
        wgpu_buffer_t *buffer =  (wgpu_buffer_t*)pool->ubo_buffer;
        wgpuQueueWriteBuffer(wgpu_ctx->queue, buffer->buffer, 0, pool->ubo_buffer_data_ptr, pool->ubo_buffer_data_size);
        pool->dirty = false;
    }

    wgpuRenderPassEncoderSetBindGroup(wgpu_cmd->pass, slot, wgpu_set->bind_group, 0, &wgpu_set->dynamic_offset);
}


void wgpu_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t*  wgpu_buffer = (wgpu_buffer_t*)buffer;

    WGPUIndexFormat idx_format = (format == gfx_index_format_16) ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;

    wgpuRenderPassEncoderSetIndexBuffer(wgpu_cmd->pass, wgpu_buffer->buffer, idx_format, offset, WGPU_WHOLE_SIZE);
}


void wgpu_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t * wgpu_buffer = (wgpu_buffer_t*)buffer;

    wgpuRenderPassEncoderSetVertexBuffer(wgpu_cmd->pass, slot, wgpu_buffer->buffer, offset, WGPU_WHOLE_SIZE);
}

//
//  wgpu_cmd_draw
//
void wgpu_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;

    wgpuRenderPassEncoderDraw(wgpu_cmd->pass, vertex_count, instance_count, 0, 0);
}

//
//  gfx_cmd_draw_indexed
//
void wgpu_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t index_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;

    wgpuRenderPassEncoderDrawIndexed(wgpu_cmd->pass, index_count, instance_count, first_idx, vertex_offset, 0);
}

void wgpu_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride)
{
    // WebGPU supports indirect draws, but does not support multi-draw in one call in all implementations.
    // We emulate draw_count by issuing draw_count calls, each at offset + i*stride.
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_buffer_t* wgpu_buffer = (wgpu_buffer_t*)buffer;
    if (wgpu_cmd == nullptr || wgpu_cmd->pass == nullptr || wgpu_buffer == nullptr)
        return;

    for (uint32_t i = 0; i < draw_count; ++i)
    {
        uint64_t off = (uint64_t)offset + (uint64_t)i * (uint64_t)stride;
        wgpuRenderPassEncoderDrawIndexedIndirect(wgpu_cmd->pass, wgpu_buffer->buffer, off);
    }
}


//
//  gfx_cmd_dispatch_compute
//
void wgpu_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;

    wgpuComputePassEncoderDispatchWorkgroups(wgpu_cmd->compute_pass, x, y, z);
}


void wgpu_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    (void)wgpu_cmd; (void)marker;
}

void wgpu_cmd_pop_marker(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    (void)wgpu_cmd;
}

void wgpu_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst)
{
    // WebGPU resource states are tracked by the implementation; explicit barriers are not exposed.
    (void)cmd; (void)buffers; (void)count; (void)src; (void)dst;
}

void wgpu_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst)
{
    (void)cmd; (void)textures; (void)count; (void)src; (void)dst;
}


//
// gfx_cmd_end
//
void wgpu_cmd_end(gfx_command_buffer_t* cmd)
{
    wgpu_command_buffer_t* wgpu_cmd = (wgpu_command_buffer_t*)cmd;
    wgpu_context_t* wctx = wgpu_cmd->ctx;

    if(wgpu_cmd->encoder != nullptr)
    {
        wgpu_cmd->commands = wgpuCommandEncoderFinish(wgpu_cmd->encoder, nullptr);
        wgpuCommandEncoderRelease(wgpu_cmd->encoder);
        wgpu_cmd->encoder = nullptr;
    }
}

//
// gfx_cmd_end
//
void wgpu_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options)
{
    wgpu_context_t* wctx = from_ctx(ctx);
    wgpu_command_buffer_t * cmd_buffer = (wgpu_command_buffer_t *)cmd;

    wgpuInstanceProcessEvents(wctx->instance);

    if(cmd_buffer->commands != nullptr)
    {
        wgpuQueueSubmit(wctx->queue, 1, &cmd_buffer->commands);
        wgpuCommandBufferRelease(cmd_buffer->commands);

        cmd_buffer->encoder = nullptr;
    }
}

#endif
