#include "gfx_vulkan.h"
#ifdef VULKAN_AVAILABLE


#if __has_include(<vma/vk_mem_alloc.h>)
    #define VMA_IMPLEMENTATION
    #include <vma/vk_mem_alloc.h>
   // #define VMA_IMPLEMENTATION
#endif

#ifdef _WIN32
    #pragma comment(lib, "../lib/vulkan-1.lib")

   // #ifdef max
  //  #undef max
   // #endif


#endif


VkFormat gfx_pixel_format_2_vk(gfx_pixel_format format)
{
    switch (format)
    {
        case gfx_pixel_format_a8:               return VK_FORMAT_R8_UNORM;
        case gfx_pixel_format_rgba4444:         return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        case gfx_pixel_format_rgb5a1:           return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        case gfx_pixel_format_rgb565:           return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case gfx_pixel_format_rgba8:            return VK_FORMAT_R8G8B8A8_UNORM;

        case gfx_pixel_format_etc1:             return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case gfx_pixel_format_etc2_rgb8a1:      return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case gfx_pixel_format_etc2_rgba8:       return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

        case gfx_pixel_format_pvrtc_rgb_2bpp:   return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case gfx_pixel_format_pvrtc_rgba_2bpp:  return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case gfx_pixel_format_pvrtc_rgb_4bpp:   return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        case gfx_pixel_format_pvrtc_rgba_4bpp:  return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;

        case gfx_pixel_format_bc1:              return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;      //! bc1 8
        case gfx_pixel_format_bc2:              return VK_FORMAT_BC2_UNORM_BLOCK;           //! bc2 16
        case gfx_pixel_format_bc3:              return VK_FORMAT_BC3_UNORM_BLOCK;           //! bc3 16
        case gfx_pixel_format_bc6:              return VK_FORMAT_BC6H_UFLOAT_BLOCK;         //! bc6 16
        case gfx_pixel_format_bc7:              return VK_FORMAT_BC7_UNORM_BLOCK;           //! bc7 16

        case gfx_pixel_format_astc4x4:          return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case gfx_pixel_format_astc5x5:          return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case gfx_pixel_format_astc6x6:          return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case gfx_pixel_format_astc8x8:          return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case gfx_pixel_format_astc10x10:        return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case gfx_pixel_format_astc12x12:        return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;

        case gfx_pixel_format_r16f:             return VK_FORMAT_R16_SFLOAT;
        case gfx_pixel_format_rg16f:            return VK_FORMAT_R16G16_SFLOAT;
        case gfx_pixel_format_rgba16f:          return VK_FORMAT_R16G16B16A16_SFLOAT;

        case gfx_pixel_format_r32f:             return VK_FORMAT_R32_SFLOAT;
        case gfx_pixel_format_rg32f:            return VK_FORMAT_R32G32_SFLOAT;
        case gfx_pixel_format_rgba32f:          return VK_FORMAT_R32G32B32A32_SFLOAT;

        case gfx_pixel_format_d24x8:            return VK_FORMAT_X8_D24_UNORM_PACK32;   //! depth buffer
        case gfx_pixel_format_d24s8:            return VK_FORMAT_D24_UNORM_S8_UINT;   //! depth buffer
    };

    return VK_FORMAT_UNDEFINED;
}

VkFormat gfx_vertex_format_2_vk(gfx_vertex_format fromat)
{
    switch (fromat)
    {
        case gfx_vertex_format_float1:      return VK_FORMAT_R32_SFLOAT;
        case gfx_vertex_format_float2:      return VK_FORMAT_R32G32_SFLOAT;
        case gfx_vertex_format_float4:      return VK_FORMAT_R32G32B32A32_SFLOAT;

        case gfx_vertex_format_int2:        return VK_FORMAT_R32G32_SINT;
        case gfx_vertex_format_int4:        return VK_FORMAT_R32G32B32A32_SINT;

        case gfx_vertex_format_uint2:       return VK_FORMAT_R32G32_UINT;
        case gfx_vertex_format_uint4:       return VK_FORMAT_R32G32B32A32_UINT;

        case gfx_vertex_format_half2:       return VK_FORMAT_R16G16_SFLOAT;
        case gfx_vertex_format_half4:       return VK_FORMAT_R16G16B16A16_SFLOAT;

        case gfx_vertex_format_short2:      return VK_FORMAT_R16G16_SINT;
        case gfx_vertex_format_short4:      return VK_FORMAT_R16G16B16A16_SINT;

        case gfx_vertex_format_ushort2:     return VK_FORMAT_R16G16_UINT;
        case gfx_vertex_format_ushort4:     return VK_FORMAT_R16G16B16A16_UINT;

        case gfx_vertex_format_byte4:       return VK_FORMAT_B8G8R8A8_UNORM;
    }
    return VK_FORMAT_UNDEFINED;
}

VkIndexType gfx_index_format_2_vk(gfx_index_format format)
{
    switch(format)
    {
        case gfx_index_format_16:           return VK_INDEX_TYPE_UINT16;
        case gfx_index_format_32:           return VK_INDEX_TYPE_UINT32;
    }
    return VK_INDEX_TYPE_UINT32;
}

VkPrimitiveTopology gfx_topology_2_vk(gfx_topology topogy)
{
    switch(topogy) 
    {
        case gfx_topology_points:           return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case gfx_topology_lines:            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case gfx_topology_lines_strip:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case gfx_topology_triangles:        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case gfx_topology_triangles_strip:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

VkCullModeFlags gfx_cull_2_vk(gfx_cull mode)
{
    switch (mode)
    {
        case gfx_cull_none:                 return VK_CULL_MODE_NONE;
        case gfx_cull_back:                 return VK_CULL_MODE_FRONT_BIT;
        case gfx_cull_front:                return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_NONE;
}

VkFrontFace gfx_face_2_vk(gfx_face face)
{
    switch (face)
    {
        case gfx_face_cw:                   return VK_FRONT_FACE_CLOCKWISE;
        case gfx_face_ccw:                  return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

#define vctx_alloc(ctx, _size)  ((vk_context_t*)(ctx))->allocator.allocate_pfn(_size)
#define vctx_free(ctx, _ptr)    ((vk_context_t*)(ctx))->allocator.free_pfn(_ptr)


VkIndexType          vk_index[]   = { VK_INDEX_TYPE_UINT16,           VK_INDEX_TYPE_UINT32 };
VkPrimitiveTopology  vk_topology[]= { VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP };
VkFrontFace          vk_face[]    = { VK_FRONT_FACE_CLOCKWISE,        VK_FRONT_FACE_COUNTER_CLOCKWISE };
VkFilter             vk_filter[]  = { VK_FILTER_NEAREST,              VK_FILTER_LINEAR };
VkSamplerMipmapMode  vk_mipmap[]  = { VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR };
VkSamplerAddressMode vk_address[] = { VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT , VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE };

VkFilter gfx_filter_2_vk(gfx_filter filter) 
{
    switch(filter) 
    {
        case gfx_filter_point:              return VK_FILTER_NEAREST;
        case gfx_filter_linear:             return VK_FILTER_LINEAR;
    }
    return VK_FILTER_NEAREST;
}

VkSamplerMipmapMode gfx_mipmap_2_vk(gfx_filter mipmap) 
{
    switch (mipmap) 
    {
        case gfx_filter_point:              return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case gfx_filter_linear:             return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

VkSamplerAddressMode gfx_address_mode_2_vk(gfx_address_mode mode) 
{
    switch (mode) 
    {
        case gfx_address_mode_repeat:           return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case gfx_address_mode_mirror_repeat:    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case gfx_address_mode_clamp_to_edge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        default:                                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkCompareOp gfx_cmp_2_vk(gfx_cmp cmp) 
{
    switch(cmp)
    {
        case gfx_cmp_never:                 return VK_COMPARE_OP_NEVER;
        case gfx_cmp_less:                  return VK_COMPARE_OP_LESS;
        case gfx_cmp_equal:                 return VK_COMPARE_OP_EQUAL;
        case gfx_cmp_lequal:                return VK_COMPARE_OP_LESS_OR_EQUAL;
        case gfx_cmp_greater:               return VK_COMPARE_OP_GREATER;
        case gfx_cmp_not_equal:             return VK_COMPARE_OP_NOT_EQUAL;
        case gfx_cmp_gequal:                return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case gfx_cmp_always:                return VK_COMPARE_OP_ALWAYS;   
    }
    return VK_COMPARE_OP_ALWAYS;
};

VkStencilOp gfx_stencil_op_2_vk(gfx_stencil_op op)
{
    switch(op)
    {
        case gfx_stencil_op_zero:           return VK_STENCIL_OP_ZERO;
        case gfx_stencil_op_keep:           return VK_STENCIL_OP_KEEP;
        case gfx_stencil_op_replace:        return VK_STENCIL_OP_REPLACE;
        case gfx_stencil_op_incr:           return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case gfx_stencil_op_incr_wrap:      return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case gfx_stencil_op_decr:           return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case gfx_stencil_op_decr_wrap:      return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case gfx_stencil_op_invert:         return VK_STENCIL_OP_INVERT;
    }
    return VK_STENCIL_OP_KEEP;
}

VkBlendFactor gfx_blend_mode_2_vk(gfx_blend_mode mode) 
{
    switch(mode)
    {
        case gfx_blend_mode_zero:           return VK_BLEND_FACTOR_ZERO;
        case gfx_blend_mode_one:            return VK_BLEND_FACTOR_ONE;
        case gfx_blend_mode_src_color:      return VK_BLEND_FACTOR_SRC_COLOR;
        case gfx_blend_mode_inv_src_color:  return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case gfx_blend_mode_src_alpha:      return VK_BLEND_FACTOR_SRC_ALPHA;
        case gfx_blend_mode_inv_src_alpha:  return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case gfx_blend_mode_dst_alpha:      return VK_BLEND_FACTOR_DST_ALPHA;
        case gfx_blend_mode_inv_dest_alpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case gfx_blend_mode_dst_color:      return VK_BLEND_FACTOR_DST_COLOR;
        case gfx_blend_mode_inv_dst_color:  return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    }
    return VK_BLEND_FACTOR_ONE;
}

VkBlendOp gfx_blend_op_2_vk(gfx_blend_op op)
{
    switch(op)
    {
        case gfx_blend_op_add:              return VK_BLEND_OP_ADD;
        case gfx_blend_op_min:              return VK_BLEND_OP_MIN;
        case gfx_blend_op_max:              return VK_BLEND_OP_MAX;
        case gfx_blend_op_subtract:         return VK_BLEND_OP_SUBTRACT;
        case gfx_blend_op_rev_substract:    return VK_BLEND_OP_REVERSE_SUBTRACT;
    }
    return VK_BLEND_OP_ADD;
}

VkImageAspectFlags determine_aspect_mask(VkFormat format)
{
    switch (format) {
        // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:              return VK_IMAGE_ASPECT_DEPTH_BIT;

        // Stencil
        case VK_FORMAT_S8_UINT:                 return VK_IMAGE_ASPECT_STENCIL_BIT;

        // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        // Assume everything else is Color
        default:                                return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

struct vk_write_info_t 
{
    VkWriteDescriptorSet        write;
    union 
    {
        VkDescriptorImageInfo   image_info;
        VkDescriptorBufferInfo  buffer_info;
    };
};

typedef struct vk_copy_info_t
{
    VkImage             dst_image;
    uint32_t            dst_image_mips;
    gfx_pixel_format    dst_image_format;
    VkExtent3D          dst_image_extend;

    VkBuffer            dst_buffer;
    uint32_t            dst_buffer_offset;
    uint32_t            dst_buffer_size;
}vk_copy_info_t;


static auto g_defaultlogfunc = [](gfx_msg type, const char * msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("\n");
    vprintf(msg, args);
    fflush(stdout);
    va_end(args);
};

static vk_context_t* from_ctx(gfx_context_t* ctx) 
{
    return (vk_context_t*)ctx;
}

#define LOG_ERROR(msg, ...) g_defaultlogfunc(gfx_msg_error, msg, __VA_ARGS__)


// The callback returns a VkBool32 that indicates to the calling layer if the Vulkan call should be aborted or not. 
// Applications should always return VK_FALSE so that they see the same behavior with and without validation layers enabled.
// If the application returns VK_TRUE from its callback and the Vulkan call being aborted returns a VkResult, the layer will return VK_ERROR_VALIDATION_FAILED_EXT
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, 
                                                        const char* layerPrefix, const char* msg, void* userData)
{
    const char * msgtype = "";
    switch (flags)
    {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:           msgtype = "\x1b[37m vk info"; break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:               msgtype = "\x1b[33m vk warn"; break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:   msgtype = "\x1b[34m vk perf"; break;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:                 msgtype = "\x1b[31m vk err "; break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:                 msgtype = "\x1b[32m vk dbg "; break;
        default:                                            msgtype = "\x1b[0m"; break;
    }

    if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        uintptr_t adresses[128] = {}; 

        uint32_t size = gfx_utils_stack_trace(0, adresses, sizeof(adresses));
        gfx_utils_stack_trace_names(adresses, size, nullptr);

        printf("\n%s: %s,%s \033[0m", msgtype, layerPrefix, msg);
    }
    return VK_FALSE;
}


static uint32_t _vk_pad_buffer_aligment(vk_context_t* ctx, uint32_t buffer_size)
{
    uint32_t minUboAlignment = (uint32_t)ctx->device_properties.limits.minUniformBufferOffsetAlignment;
    uint32_t alignedSize = buffer_size;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

static VkFormat _vk_find_supported_format(vk_context_t* ctx, VkFormat* candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx->physicaldevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    return VK_FORMAT_UNDEFINED;
}

static uint32_t _vk_find_memory_type(VkPhysicalDeviceMemoryProperties properties, VkMemoryPropertyFlags requrement, VkMemoryPropertyFlags preffer)
{
    auto mem_type_count = properties.memoryTypeCount;

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((requrement & (1 << i)) && (properties.memoryTypes[i].propertyFlags & preffer) == preffer) {
            return i;
        }
    }

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
        VkMemoryType type = properties.memoryTypes[i];

        const char * dbg_name = "";
        switch(1 << i)
        {
            case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:        dbg_name = "DEVICE_LOCAL_BIT";         break;
            case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:        dbg_name = "HOST_VISIBLE_BIT";         break;
            case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:       dbg_name = "HOST_COHERENT_BIT";        break;
            case VK_MEMORY_PROPERTY_HOST_CACHED_BIT:         dbg_name = "HOST_CACHED_BIT";          break;
            case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:    dbg_name = "LAZILY_ALLOCATED_BIT";     break;
            case VK_MEMORY_PROPERTY_PROTECTED_BIT:           dbg_name = "PROTECTED_BIT";            break;
            case VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD: dbg_name = "DEVICE_COHERENT_BIT_AMD";  break;
            case VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD: dbg_name = "DEVICE_UNCACHED_BIT_AMD";  break;
        }

        bool valid_flag = (type.propertyFlags & preffer) == preffer;

        if(valid_flag)
            return i;
    }
    return 0;
}

static VkInstance _vk_create_instance(bool isdebug)
{
#if defined(_WIN32)
    #define SURFACE_EXTENSION_NAME      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
    #define SURFACE_EXTENSION_NAME      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif

    char* extensionsDebug[] = { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
    char* extensionsRelease[] = { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME };

    char**   extensionNames = isdebug ? extensionsDebug : extensionsRelease;
    uint32_t extensionCount = isdebug ? _countof(extensionsDebug) : _countof(extensionsRelease);

    uint32_t property_layer_count = 0;
    VkLayerProperties* properties = nullptr;
    vkEnumerateInstanceLayerProperties(&property_layer_count, nullptr);
    properties = (VkLayerProperties*)calloc(property_layer_count, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&property_layer_count, properties);

    char* validationLayerName = nullptr;
    for(uint32_t i = 0; i < property_layer_count; ++i)
    {
        if (properties != nullptr && properties[i].layerName != nullptr && !strcmp("VK_LAYER_KHRONOS_validation", properties[i].layerName))
                validationLayerName = "VK_LAYER_KHRONOS_validation";

       // if (!strcmp("VK_LAYER_KHRONOS_validation", properties[i].layerName))
       //     validationLayerName = "VK_LAYER_LUNARG_standard_validation";

        if(validationLayerName != nullptr)
            break;
    }
    free(properties);

    char* layersDebug[] = { validationLayerName, "" };
    char* layersRelease[] = { nullptr };

    char**   layerNames = isdebug ? layersDebug : layersRelease;
    uint32_t layerCount = (isdebug ? _countof(layersDebug) : _countof(layersRelease)) - 1;

    VkApplicationInfo app_info      = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);
        app_info.pApplicationName   = "vksample";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName        = "No Engine";
        app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);

    VkDebugReportCallbackCreateInfoEXT dbg_info = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, nullptr };
        dbg_info.pfnCallback = vkDebugCallback;
        dbg_info.flags       = VK_DEBUG_REPORT_WARNING_BIT_EXT | 
                               VK_DEBUG_REPORT_ERROR_BIT_EXT | 
                               VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                               VK_DEBUG_REPORT_DEBUG_BIT_EXT | 
                               VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

    VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    {
        info.pApplicationInfo           = &app_info;
        info.enabledExtensionCount      = extensionCount;
        info.ppEnabledExtensionNames    = extensionNames;
        info.enabledLayerCount          = layerCount;
        info.ppEnabledLayerNames        = layerNames;
        info.pNext                      = isdebug? &dbg_info : nullptr;
    }

    VkInstance instance = VK_NULL_HANDLE;
    vkCreateInstance(&info, nullptr, &instance);

    return instance;
}

static VkSurfaceKHR _vk_create_surface(VkInstance instance, intptr_t handle)
{
    VkSurfaceKHR surface = 0;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr };
    createInfo.flags = 0;
    createInfo.hinstance = GetModuleHandle(NULL);
    createInfo.hwnd = (HWND)handle;
    VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface);

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkAndroidSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr };
    createInfo.flags = 0;
    createInfo.window = (struct ANativeWindow*)(handle);
    VkResult result = vkCreateAndroidSurfaceKHR(instance, &createInfo, NULL, &surface);
#else
    static_assert(false, "vk_create_surface: not implemented");
#endif
    return surface;
}

static VkPhysicalDevice _vk_create_physical_device(VkInstance instance)
{
    uint32_t count = 0;
    VkPhysicalDevice devices[16] = {};

    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    vkEnumeratePhysicalDevices(instance, &count, devices);

    VkPhysicalDevice physicalDevice = devices[0]; // set device by default
    for (uint32_t i = 0; i < count; ++i)
    {
        VkPhysicalDeviceProperties prop = {};
        vkGetPhysicalDeviceProperties(devices[i], &prop);

        if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = devices[i];
        }
    }
    return physicalDevice;
}

static VkDevice _vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface, uint32_t* graphics, uint32_t* present)
{
    uint32_t queueFamilyPropertyCount = 0;
    VkBool32 supportsPresent[16] = { 0 };
    VkQueueFamilyProperties queueFamilyProperties[16] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queueFamilyPropertyCount, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queueFamilyPropertyCount, queueFamilyProperties);

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physdevice, i, surface, &supportsPresent[i]);
        bool support_grpaphics = (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        bool support_compute   = (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
        
        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueFamilyIndex == UINT32_MAX) {
                graphicsQueueFamilyIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // If didn't find a queue that supports both graphics and present, then find a separate present queue.
    if (presentQueueFamilyIndex == UINT32_MAX)
    {
        for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
        LOG_ERROR("Swapchain Initialization Failure: Could not find both graphics and present queues");
        return VK_NULL_HANDLE;
    }

    if (graphics)
        *graphics = graphicsQueueFamilyIndex;
    if (present)
        *present = presentQueueFamilyIndex;

    bool separate_present_queue = (graphicsQueueFamilyIndex != presentQueueFamilyIndex);

    //create device
    float queue_priorities[] = { 0.0 };

    VkDeviceQueueCreateInfo queues[] = {
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, graphicsQueueFamilyIndex, 1, queue_priorities },
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, presentQueueFamilyIndex,  1, queue_priorities }
    };

    const char* device_extension [] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME/*, VK_EXT_DEBUG_MARKER_EXTENSION_NAME */};
    const char* device_validation_layers[] = { "VK_LAYER_LUNARG_mem_tracker", "VK_LAYER_GOOGLE_unique_objects" };

    VkPhysicalDeviceFeatures features = {};
    features.sampleRateShading = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    {
        device_info.queueCreateInfoCount    = separate_present_queue ? 2 : 1;
        device_info.pQueueCreateInfos       = queues;
        device_info.enabledLayerCount       = _countof(device_validation_layers);
        device_info.ppEnabledLayerNames     = device_validation_layers;
        device_info.enabledExtensionCount   = _countof(device_extension);
        device_info.ppEnabledExtensionNames = device_extension;
        device_info.pEnabledFeatures        = &features;
    }

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(physdevice, &device_info, NULL, &device);
    if (result != VK_SUCCESS)
    {
        LOG_ERROR("Vk: Error in vkCreateDevice(%d)", result);
    }

    return device;
}

static VkSemaphore _vk_create_semaphore(VkDevice device, bool istimeline = false)
{
    int numFrames = 2;
    const uint64_t initialValue = (numFrames - 1);

    VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = initialValue;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        create_info.pNext = istimeline? &timelineCreateInfo : nullptr;
    vkCreateSemaphore(device, &create_info, NULL, &semaphore);
    return semaphore;
}

static void _vk_create_buffer(vk_context_t* ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memflag, vk_buffer_t* buffer)
{
    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.size         = size;
        buffer_info.usage        = usage;
        buffer_info.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

#ifdef VMA
    VkBuffer vkb;
    VmaAllocation allocation;
    VmaAllocationCreateInfo vmaalloc_info = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateBuffer(ctx->vma_allocator, &buffer_info, &vmaalloc_info, &buffer->buffer, &allocation, nullptr);

    buffer->memory = allocation->GetMemory();
    return;
#endif

    if (auto result = vkCreateBuffer(ctx->device, &buffer_info, nullptr, &buffer->buffer))
    {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to create buffer! (%s)", err_str);
        return;
    }

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(ctx->device, buffer->buffer, &requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = _vk_find_memory_type(ctx->memory_properties, requirements.memoryTypeBits, memflag);
    if (auto result = vkAllocateMemory(ctx->device, &alloc_info, nullptr, &buffer->memory))
    {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to allocate buffer memory!(s)", err_str);
        return;
    }

    buffer->buffer_size = (uint32_t)alloc_info.allocationSize;
    vkBindBufferMemory(ctx->device, buffer->buffer, buffer->memory, 0);
}

static VkImage _vk_create_image(vk_context_t* ctx, VkImageType type, VkExtent3D extend, uint32_t mips, VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* imageMemory, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
{
    VkImage image = VK_NULL_HANDLE;
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType     = type;
        imageInfo.extent        = extend;
        imageInfo.mipLevels     = (mips >= 1) ? mips : 1;
        imageInfo.arrayLayers   = 1; 
        imageInfo.format        = format;
        imageInfo.tiling        = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.samples       = samples;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (auto result = vkCreateImage(ctx->device, &imageInfo, nullptr, &image))
    {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to create image!(%s)", err_str);
        return nullptr;
    }

    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(ctx->device, image, &memRequirements);

    VkMemoryAllocateInfo alloc_info  = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize    = memRequirements.size;
        alloc_info.memoryTypeIndex   = _vk_find_memory_type(ctx->memory_properties, memRequirements.memoryTypeBits, properties);
    if (auto result = vkAllocateMemory(ctx->device, &alloc_info, nullptr, imageMemory)) {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to allocate image memory!(%s)", err_str);
        return nullptr;
    } 

    vkBindImageMemory(ctx->device, image, *imageMemory, 0);
    return image;
}

static VkImageView _vk_create_image_view(vk_context_t* ctx, VkImageViewType type, VkImage image, VkFormat format, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image      = image;
    viewInfo.viewType   = type;
    viewInfo.format     = format;
    viewInfo.subresourceRange.aspectMask        = determine_aspect_mask(format);
    viewInfo.subresourceRange.baseMipLevel      = 0;
    viewInfo.subresourceRange.levelCount        = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer    = 0;
    viewInfo.subresourceRange.layerCount        = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(ctx->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LOG_ERROR("failed to create texture image view!");
        return nullptr;
    }
    return imageView;
}


static void _vk_image_transition(vk_context_t* ctx, VkImage image, uint16_t mips, VkImageLayout  oldLayout, VkImageLayout newLayout)
{
    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_NONE;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_NONE;

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mips;
    barrier.subresourceRange.layerCount     = 1;

    gfx_command_buffer_t *cmd = nullptr;
    vk_create_cmd(&ctx->handle, 1, &cmd);
    vk_cmd_begin(cmd);

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
        assert(false);

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdPipelineBarrier(vk_cmd->cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

    gfx_cmd_end(cmd);
    gfx_submit_cmd(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    gfx_destroy_cmd(&ctx->handle, cmd);
}

static void _vk_copy_buffer_to(vk_context_t* ctx, VkBuffer src, vk_copy_info_t * dst_info)
{
    gfx_command_buffer_t* cmd = nullptr;
    vk_create_cmd(&ctx->handle, 1, &cmd);
    vk_cmd_begin(cmd/*, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    if (dst_info->dst_buffer != VK_NULL_HANDLE && dst_info->dst_buffer_size > 0)
    {
        VkBufferCopy region = { 0, dst_info->dst_buffer_offset, dst_info->dst_buffer_size };
        vkCmdCopyBuffer(vk_cmd->cmd, src, dst_info->dst_buffer, 1, &region);
    }

    VkBufferImageCopy regions[16] = {};
    if (dst_info->dst_image != VK_NULL_HANDLE)
    {
        uint32_t offset = 0;
        for (uint8_t i = 0; i < dst_info->dst_image_mips; i++)
        {
            uint32_t width  = max(dst_info->dst_image_extend.width >> i, 1);
            uint32_t height = max(dst_info->dst_image_extend.height >> i, 1);
            uint32_t depth  = max(dst_info->dst_image_extend.depth >> i, 1);

            regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[i].imageSubresource.baseArrayLayer = 0;
            regions[i].imageSubresource.layerCount = 1;
            regions[i].imageSubresource.mipLevel = i;

            regions[i].imageOffset = { 0, 0, 0 };
            regions[i].imageExtent = { width, height, depth };
            regions[i].bufferOffset = offset;

            offset += gfx_utils_image_layer_size(width, height, depth, dst_info->dst_image_format);
        }
        vkCmdCopyBufferToImage(vk_cmd->cmd, src, dst_info->dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_info->dst_image_mips, regions);
    }
    gfx_cmd_end(cmd);
    gfx_submit_cmd(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    gfx_destroy_cmd(&ctx->handle, cmd);
}


// vulkan
void vk_create_renderer(gfx_settings_t* cfg, gfx_context_t** out_ctx)
{
    assert(cfg && out_ctx);

    vk_context_t* vctx = (vk_context_t*)calloc(1, sizeof(vk_context_t));
    if (vctx == nullptr)
        return;

  //  memcpy(&vctx->settings, cfg, sizeof(gfx_settings_t));

    vctx->allocator.allocate_pfn = cfg->allocator.allocate_pfn;
    vctx->allocator.realloc_pfn = cfg->allocator.realloc_pfn;
    vctx->allocator.free_pfn = cfg->allocator.free_pfn;

    uint32_t gfamily = 0; // graphic family
    uint32_t pfamily = 0; // present family
    VkQueue  gqueue = nullptr;
    VkQueue  pqueue = nullptr;

    auto isdebug = (cfg->options & gfx_options_debug) == gfx_options_debug;

    auto instance   = _vk_create_instance(isdebug);
    auto physdevice = _vk_create_physical_device(instance);
    auto surface    = _vk_create_surface(instance, cfg->handle);
    auto device     = _vk_create_device(physdevice, surface, &gfamily, &pfamily);

    vkGetDeviceQueue(device, gfamily, 0, &gqueue);
    vkGetDeviceQueue(device, pfamily, 0, &pqueue);

    vctx->instance         = instance;
    vctx->physicaldevice   = physdevice;
    vctx->surface          = surface;
    vctx->device           = device;
    vctx->graphics_queue   = { gfamily, gqueue };
    vctx->present_queue    = { pfamily, pqueue };
    vctx->dbg_log          = cfg->dbglog ? cfg->dbglog : g_defaultlogfunc;
    vctx->msaa_samples     = VK_SAMPLE_COUNT_1_BIT;

    vkGetPhysicalDeviceFeatures(physdevice, &vctx->device_features);
    vkGetPhysicalDeviceProperties(physdevice, &vctx->device_properties);
    vkGetPhysicalDeviceMemoryProperties(physdevice, &vctx->memory_properties);
    vkEnumerateDeviceExtensionProperties(physdevice, NULL, &vctx->extensions_count, NULL);
    vctx->extensions = (VkExtensionProperties*)calloc(vctx->extensions_count, sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physdevice, NULL, &vctx->extensions_count, vctx->extensions);

    for(int i = 0; i < _countof(vctx->semaphores); ++i)
    {
        vctx->semaphores[i].image_available = _vk_create_semaphore(vctx->device);
        vctx->semaphores[i].rendering_finished = _vk_create_semaphore(vctx->device);

        vk_debug_set_name(vctx, (uint64_t)vctx->semaphores[i].image_available, VK_OBJECT_TYPE_SEMAPHORE, "image_available_sem");
        vk_debug_set_name(vctx, (uint64_t)vctx->semaphores[i].rendering_finished, VK_OBJECT_TYPE_SEMAPHORE, "rendering_finished_sem");

        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(vctx->device, &fenceInfo, nullptr, &vctx->semaphores[i].wait_fence);
        vkResetFences(vctx->device, 1, &vctx->semaphores[i].wait_fence);
    }

    gfx_pool_create(sizeof(vk_sampler_t), 128,  &vctx->sampler_pool, &vctx->allocator);

    gfx_pool_create(sizeof(vk_texture_t), cfg->limits.textures_pool_capacity, &vctx->texture_pool, &vctx->allocator);
    gfx_pool_create(sizeof(vk_shader_t), cfg->limits.shaders_pool_capacity, &vctx->shaders_pool, &vctx->allocator);
    gfx_pool_create(sizeof(vk_buffer_t), cfg->limits.buffer_pool_capacity, &vctx->buffers_pool, &vctx->allocator);


    VkPhysicalDeviceMemoryBudgetPropertiesEXT physical_device_memory_budget_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT };
    VkPhysicalDeviceMemoryProperties2 device_memory_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
    device_memory_properties.pNext = &physical_device_memory_budget_properties;
    vkGetPhysicalDeviceMemoryProperties2(physdevice, &device_memory_properties);
   /*device_memory_heap_count = device_memory_properties.memoryProperties.memoryHeapCount;
    device_memory_total_usage = 0;
    device_memory_total_budget = 0;
    for (uint32_t i = 0; i < device_memory_heap_count; i++)
    {
        device_memory_total_usage += physical_device_memory_budget_properties.heapUsage[i];
        device_memory_total_budget += physical_device_memory_budget_properties.heapBudget[i];
    }*/



    uint32_t _colors [] = { 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF, 0xFF808080,
                            0xFF808080, 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF,
                            0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF, 0xFF808080,
                            0xFF808080, 0xFFFFFFFF, 0xFF808080, 0xFFFFFFFF,
                            0xFFFF0000, 0xFF000000,
                            0xFF000000, 0xFFFF0000,
                            0xFF00FFFF
    };

    gfx_sampler_desc_t sampler = {};
        sampler.anisotropy = 1;
        sampler.minmag = gfx_filter_point;
        sampler.mipmap = gfx_filter_point;
        sampler.mode = gfx_address_mode_repeat;
    vk_create_sampler(&vctx->handle, &sampler, &vctx->default_sampler);
    
    gfx_texture_desc_t image = { 0 };
        image.label = "default_4x4";
        image.width = 4;
        image.height = 4;
        image.depth = 1;
        image.format = gfx_pixel_format_rgba8;
        image.mip_levels = 3;
        image.data = (uint8_t*)_colors;

    gfx_texture_t * default_texture = nullptr;
    vk_create_texture(&vctx->handle, &image, &default_texture);
    vctx->default_texture = (vk_texture_t*)default_texture;

    uint32_t sbuffer_size = 8 * 1024 * 1024; // 8mb
    vctx->staging_buffer = (vk_buffer_t*)vctx_alloc(vctx, sizeof(vk_buffer_t));

    _vk_create_buffer(vctx, sbuffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vctx->staging_buffer);

    vk_debug_set_buffer_name(vctx, vctx->staging_buffer, "staging_buffer");
  //  vk_debug_set_texture_name(vctx, vctx->default_texture, "_default_texture");
    
    *out_ctx = &vctx->handle;
}

void vk_destroy_renderer(gfx_context_t * ctx)
{
    assert(ctx);
    vk_context_t* vctx = (vk_context_t*)ctx;
    
    vk_destroy_sampler(ctx, vctx->default_sampler);
    vk_destroy_texture(ctx, &vctx->default_texture->handle);
    vctx_free(ctx, vctx->extensions);

    vkDestroySemaphore(vctx->device, vctx->semaphores[0].image_available, NULL);
    vkDestroySemaphore(vctx->device, vctx->semaphores[0].rendering_finished, NULL);
    vkDestroySemaphore(vctx->device, vctx->semaphores[1].image_available, NULL);
    vkDestroySemaphore(vctx->device, vctx->semaphores[1].rendering_finished, NULL);

    vkDestroyDevice(vctx->device, nullptr);
    vkDestroySurfaceKHR(vctx->instance, vctx->surface, nullptr);
    vkDestroyInstance(vctx->instance, nullptr);

    free(vctx);
}

void vk_create_renderpass(gfx_context_t* ctx, VkFormat colorformat, VkFormat depthformat, VkRenderPass* renderpass)
{
    vk_context_t* vctx = from_ctx(ctx);

    VkAttachmentDescription attachment_descs[2] = {};
    // color attachment
    attachment_descs[0].format           = colorformat;
    attachment_descs[0].samples          = vctx->msaa_samples;
    attachment_descs[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // depth attachment
    attachment_descs[1].format           = depthformat;
    attachment_descs[1].samples          = vctx->msaa_samples;
    attachment_descs[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachment_refs[] = {
        {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
    };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &attachment_refs[0];
    subpass.pDepthStencilAttachment = &attachment_refs[1];

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.pAttachments     = attachment_descs;
    renderPassInfo.attachmentCount  = _countof(attachment_descs);
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;

    if (auto result = vkCreateRenderPass(vctx->device, &renderPassInfo, nullptr, renderpass))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create render pass!");
    }
}

static vk_texture_t _vk_create_target_texture(vk_context_t* ctx, VkExtent3D extend, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples)
{
    vk_texture_t result = {};

    result.image = _vk_create_image(ctx, VK_IMAGE_TYPE_2D, extend, 1, format, VK_IMAGE_TILING_OPTIMAL, 
                                    usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &result.memory, samples);

    result.view = _vk_create_image_view(ctx, VK_IMAGE_VIEW_TYPE_2D, result.image, format, 1);

    return result;
}



void vk_recreate_swapchain(gfx_context_t* ctx, vk_swapchain_t* swapchain)
{
    vk_context_t* vctx = from_ctx(ctx);

    if (vctx == nullptr || swapchain == nullptr)
        return;

    uint32_t format_count = 0;
    VkSurfaceFormatKHR formats[256] = {};
    VkSurfaceCapabilitiesKHR capabilities = { 0 };

    vkGetPhysicalDeviceSurfaceFormatsKHR(vctx->physicaldevice, vctx->surface, &format_count, NULL);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vctx->physicaldevice, vctx->surface, &format_count, formats);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vctx->physicaldevice, vctx->surface, &capabilities);

    swapchain->extend = capabilities.currentExtent;
    swapchain->target.extend = capabilities.currentExtent;

    auto prev_swapchain = swapchain->swapchain;
    auto surface_format = formats[0];
    uint32_t image_count = max(capabilities.minImageCount, 2);

    VkSwapchainCreateInfoKHR info   = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        info.surface                = vctx->surface;
        info.minImageCount          = image_count;
        info.imageFormat            = surface_format.format;
        info.imageColorSpace        = surface_format.colorSpace;
        info.imageExtent            = capabilities.currentExtent;
        info.imageArrayLayers       = 1;
        info.imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount  = 0;
        info.pQueueFamilyIndices    = NULL;
        info.preTransform           = capabilities.currentTransform; //VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        info.compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode            = VK_PRESENT_MODE_FIFO_KHR;// VK_PRESENT_MODE_IMMEDIATE_KHR VK_PRESENT_MODE_FIFO_KHR;
        info.clipped                = VK_TRUE;
        info.oldSwapchain           = prev_swapchain;
    VkResult result = vkCreateSwapchainKHR(vctx->device, &info, NULL, &swapchain->swapchain);

    if (prev_swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(vctx->device, prev_swapchain, nullptr);

    //dept attachment
    VkExtent3D extend = { capabilities.currentExtent.width, capabilities.currentExtent.height, 1 };
    VkFormat depth_formats[] = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT };
    VkFormat depth_format = _vk_find_supported_format(vctx, depth_formats, _countof(depth_formats), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if (swapchain->target.depth_attachments.view)
    {
        vkDestroyImageView(vctx->device, swapchain->target.depth_attachments.view, nullptr);
        vkDestroyImage(vctx->device, swapchain->target.depth_attachments.image, nullptr);
        vkFreeMemory(vctx->device, swapchain->target.depth_attachments.memory, nullptr);

        swapchain->target.depth_attachments.view = VK_NULL_HANDLE;
        swapchain->target.depth_attachments.image = VK_NULL_HANDLE;
        swapchain->target.depth_attachments.memory = VK_NULL_HANDLE;
    }

    auto depth_target = _vk_create_target_texture(vctx, extend, depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, vctx->msaa_samples);
    swapchain->target.depth_attachments = depth_target;

    // color attachments
    VkImage images[8] = {};
    vkGetSwapchainImagesKHR(vctx->device, swapchain->swapchain, &image_count, NULL);
    vkGetSwapchainImagesKHR(vctx->device, swapchain->swapchain, &image_count, images);

    // render pass
    if (swapchain->renderpass == VK_NULL_HANDLE)
        vk_create_renderpass(ctx, surface_format.format, depth_format, &swapchain->renderpass);

    for (uint32_t i = 0; i < image_count; ++i)
    {
        VkImageView color_view = _vk_create_image_view(vctx, VK_IMAGE_VIEW_TYPE_2D, images[i], surface_format.format, 1);
        VkImageView attachments[] = {
            color_view,
            depth_target.view
        };

        if (swapchain->framebuffers[i] != VK_NULL_HANDLE)
            vkDestroyFramebuffer(vctx->device, swapchain->framebuffers[i], nullptr);
        swapchain->framebuffers[i] = VK_NULL_HANDLE;

        VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            info.renderPass         = swapchain->renderpass;
            info.attachmentCount    = _countof(attachments);
            info.pAttachments       = attachments;
            info.width              = extend.width;
            info.height             = extend.height;
            info.layers             = 1;
        result = vkCreateFramebuffer(vctx->device, &info, nullptr, &(swapchain)->framebuffers[i]);
        if(result != VK_SUCCESS)
        {
            auto err_str = string_VkResult(result);
            vctx->dbg_log(gfx_msg_error, "failed to create framebuffer: %s", err_str);
        }
        swapchain->target.renderpass = swapchain->renderpass;
        swapchain->target.framebuffer = swapchain->framebuffers[i];
        swapchain->target.color_attachments[i].format = surface_format.format;
        swapchain->target.color_attachments[i].view = color_view;
        swapchain->target.color_attachments[i].image = images[i];
    }
}


void vk_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t** out_swapchain)
{
    vk_context_t* vctx = from_ctx(ctx);

    vk_swapchain_t* swapchain = (vk_swapchain_t*)vctx_alloc(ctx, sizeof(vk_swapchain_t));
    if (swapchain == nullptr)
        return;

    *out_swapchain = &swapchain->handle;
    vk_recreate_swapchain(ctx, swapchain);
    vctx->default_renderpass = swapchain->target.renderpass;
}

void vk_destroy_swapchain(gfx_context_t* ctx, gfx_swapchain_t* swapchain)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_swapchain_t* vk_swapchain = (vk_swapchain_t*)swapchain;

    for (size_t i = 0; i < _countof(vk_swapchain->framebuffers); i++) {
        if(vk_swapchain->framebuffers[i] != VK_NULL_HANDLE)
            vkDestroyFramebuffer(vctx->device, vk_swapchain->framebuffers[i], nullptr);
        vk_swapchain->framebuffers[i] = VK_NULL_HANDLE;
    }

    if(vk_swapchain->target.depth_attachments.view) {
        vkDestroyImageView(vctx->device, vk_swapchain->target.depth_attachments.view, nullptr);
        vkDestroyImage(vctx->device, vk_swapchain->target.depth_attachments.image, nullptr);
        vkFreeMemory(vctx->device, vk_swapchain->target.depth_attachments.memory, nullptr);

        vk_swapchain->target.depth_attachments.view = VK_NULL_HANDLE;
        vk_swapchain->target.depth_attachments.image = VK_NULL_HANDLE;
        vk_swapchain->target.depth_attachments.memory = VK_NULL_HANDLE;
     }

    for (size_t i = 0; i < vk_swapchain->target.color_attachment_count; i++) {
        vkDestroyImageView(vctx->device, vk_swapchain->target.color_attachments[i].view, nullptr);
        vk_swapchain->target.color_attachments[i].view = VK_NULL_HANDLE;
    }

    if(vk_swapchain->swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(vctx->device, vk_swapchain->swapchain, nullptr);
    vk_swapchain->swapchain = VK_NULL_HANDLE;
}


int32_t vk_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, gfx_render_target_t** target)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_swapchain_t* vk_swapchain = (vk_swapchain_t*)swapchain;
    
    do
    {
        uint32_t idx = 0;
        auto semaphore = vctx->semaphores[vctx->frame_idx].image_available;
        auto result = vkAcquireNextImageKHR(vctx->device, vk_swapchain->swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &idx);
        switch(result)
        {
            case VK_ERROR_OUT_OF_DATE_KHR: 
                vk_recreate_swapchain(ctx, vk_swapchain); 
                continue;

            case VK_SUCCESS: 
                *target = &vk_swapchain->target.handle;
                vk_swapchain->target.framebuffer = vk_swapchain->framebuffers[idx];
                return idx;

            default: 
                return -1;
        }

    }while(true);
}


void vk_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_swapchain_t* vk_swapchain = (vk_swapchain_t*)swapchain;

    auto semaphore = vctx->semaphores[vctx->frame_idx].rendering_finished;
    auto fence = vctx->semaphores[vctx->frame_idx].wait_fence;

    vkWaitForFences(vctx->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vctx->device, 1, &fence);

    VkResult result0 = VK_SUCCESS;
    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = &semaphore;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = &vk_swapchain->swapchain;
        presentInfo.pImageIndices       = &idx;
    auto result = vkQueuePresentKHR(vctx->present_queue.queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        vk_recreate_swapchain(ctx, vk_swapchain);
    }

    vctx->frame_idx = (vctx->frame_idx+1)%2;
    vctx->frame_number++;
}


// create render stuff
void vk_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    vk_context_t * vctx = from_ctx(ctx);

    bool mapped = desc->mapped;
    VkBufferUsageFlags usage_flag = 0;
    switch(desc->usage)
    {
        case gfx_buffer_usage_uniform: 
            mapped = true;
            usage_flag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;

        case gfx_buffer_usage_vertex:
            usage_flag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;

        case gfx_buffer_usage_index:
            usage_flag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;

        case gfx_buffer_usage_storage:
            usage_flag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;

        case gfx_buffer_usage_indirect:
            usage_flag = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            break;
    }

    VkMemoryPropertyFlags memory_flag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // mapped - hostvisible,
    if (mapped)
    {
        usage_flag |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memory_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    auto handle = gfx_pool_alloc(vctx->buffers_pool);
    vk_buffer_t* buffer = (vk_buffer_t*)gfx_pool_map(vctx->buffers_pool, handle);
    buffer->handle.idx = handle;

    if(!buffer)
    {
        vctx->dbg_log(gfx_msg_error, "failed to allocate vk_buffer_t %s", desc->label? desc->label:"");
        return;
    }

    buffer->mapped = mapped;

    _vk_create_buffer(vctx, desc->size, usage_flag, memory_flag, buffer);
    vk_update_buffer_data(ctx, &buffer->handle, desc->data, desc->size, 0);

    *out_buffer = &buffer->handle;
}

void vk_create_descriptor_pool(vk_context_t* ctx, vk_shader_t* shader, uint32_t capacity, vk_descriptor_pool_t** out_pool)
{
    vk_descriptor_pool_t* pool = (vk_descriptor_pool_t*)vctx_alloc(ctx, sizeof(vk_descriptor_pool_t));
    vk_descriptor_set_t*  sets = (vk_descriptor_set_t*)vctx_alloc(ctx, sizeof(vk_descriptor_set_t) * capacity);

    if (pool == nullptr || sets == nullptr) {
        vctx_free(ctx, pool);
        vctx_free(ctx, sets);
        return;
    }

    const uint32_t VK_DESCRIPTOR_TYPE_RANGE_SIZE = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    VkDescriptorPoolSize pool_sizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};
    VkDescriptorPoolSize pool_sizes_by_type[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};

    for (int i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
        pool_sizes_by_type[i].type = (VkDescriptorType)i;

    for (size_t i = 0; i < shader->uniform_count; ++i)
        pool_sizes_by_type[shader->bindings[i].descriptorType].descriptorCount++;

    uint32_t pool_size_count = 0;

    for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
    {
        if (pool_sizes_by_type[i].descriptorCount > 0)
        {
            pool_sizes[pool_size_count].type = pool_sizes_by_type[i].type;
            pool_sizes[pool_size_count].descriptorCount = pool_sizes_by_type[i].descriptorCount * capacity;
            pool_size_count++;
        }
    }

    // Descriptor pool
    VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCreateInfo.maxSets          = capacity;
        poolCreateInfo.poolSizeCount    = pool_size_count;
        poolCreateInfo.pPoolSizes       = pool_sizes;
    auto result = vkCreateDescriptorPool(ctx->device, &poolCreateInfo, NULL, &pool->pool);


    uint32_t ubo_buffer_size = 0;
    for (uint32_t i = 0; i < shader->uniform_count; ++i)
    {
        if(shader->uniforms[i].type == gfx_uniform_ubo)
            ubo_buffer_size += shader->uniforms[i].buffer.size;
    }

    uint32_t alignment = (uint32_t)ctx->device_properties.limits.minUniformBufferOffsetAlignment;
    uint32_t aligned_size = gfx_utils_align_up(ubo_buffer_size, alignment);
   // uint32_t aligned_size = _vk_pad_buffer_aligment(ctx, ubo_buffer_size);
    ubo_buffer_size = aligned_size * capacity;

    vk_buffer_t* ubo_buffer = nullptr;
    if (ubo_buffer_size > 0)
    {
        gfx_buffer_t* buffer = nullptr;
        gfx_buffer_desc_t buff_desc = {};
            buff_desc.label = "ubo";
            buff_desc.usage = gfx_buffer_usage_uniform;
            buff_desc.size  = ubo_buffer_size;
        vk_create_buffer(&ctx->handle, &buff_desc, &buffer);

        ubo_buffer = (vk_buffer_t*)buffer;
    }

    VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool     = pool->pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts        = &shader->layout;

    pool->capacity = capacity;
    pool->free_set_count = capacity;
    pool->sets = sets;
    pool->writes = (VkWriteDescriptorSet*)vctx_alloc(ctx, (capacity * shader->uniform_count) * sizeof(VkWriteDescriptorSet));
    pool->write_infos = (vk_write_info_t*)vctx_alloc(ctx, (capacity * shader->uniform_count) * sizeof(vk_write_info_t));

    for (uint32_t i = 0; i < capacity; i++)
    {
        result = vkAllocateDescriptorSets(ctx->device, &allocateInfo, &sets[i].descriptor_set);

        sets[i].isfree = true;
        sets[i].dirty = true;
        sets[i].shader = shader;
        sets[i].pool = pool;
        sets[i].uboptr = (uint8_t*)ubo_buffer->data_ptr + aligned_size * i;
        sets[i].writes = pool->writes + i * shader->uniform_count;
        sets[i].write_infos =  pool->write_infos + i * shader->uniform_count;

        for(uint32_t j = 0;  j < shader->uniform_count; ++j)
        {
            auto& binding = shader->bindings[j];

            sets[i].writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sets[i].writes[j].dstSet = sets[i].descriptor_set;
            sets[i].writes[j].dstBinding = binding.binding;
            sets[i].writes[j].descriptorType = binding.descriptorType;
            sets[i].writes[j].descriptorCount = binding.descriptorCount;

            switch (binding.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    sets[i].write_infos[j].buffer_info.buffer = ubo_buffer->buffer;
                    sets[i].write_infos[j].buffer_info.range = aligned_size;
                    sets[i].write_infos[j].buffer_info.offset = aligned_size * i;

                    sets[i].writes[j].pBufferInfo = &sets[i].write_infos[j].buffer_info;
                    break;

                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    sets[i].write_infos[j].image_info.sampler = ((vk_sampler_t*)ctx->default_sampler)->sampler;
                    sets[i].write_infos[j].image_info.imageView = ((vk_texture_t*)ctx->default_texture)->view;
                    sets[i].write_infos[j].image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    sets[i].writes[j].pImageInfo = &sets[i].write_infos[j].image_info;
                    break;

                default: assert(false); break;
            }
        }

        vkUpdateDescriptorSets(ctx->device, shader->uniform_count, sets[i].writes, 0, NULL);
    }
    *out_pool = pool;

    if (result != VK_SUCCESS)
    {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to create descriptor pool (%s)", err_str);
    }
}

void vk_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader)
{
    vk_context_t* vctx = from_ctx(ctx);

    VkPipelineShaderStageCreateInfo stages[gfx_shader_count] = {};

    uint16_t hash = 0;
    for (uint32_t i = 0; i < desc->stages_count; ++i)
    {
        if (desc->stages[i].data && *(uint32_t*)desc->stages[i].data != SpvMagicNumber)
        {
            vctx->dbg_log(gfx_msg_error, "error in shader blob(SpvMagicNumber incorrect)");
            return;
        }

        // find execution model and entry point in bytecode
        VkPipelineShaderStageCreateInfo* stage = &stages[i];
        stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        const uint32_t* ptr = (uint32_t*)desc->stages[i].data + 5; // skip header(MagicNumber, Version, Revision, OpCodeMask, WordCountShift)
        const uint32_t* end = (uint32_t*)desc->stages[i].data + desc->stages[i].size;
        for (ptr; ptr < end; ptr += (*ptr >> SpvWordCountShift) & SpvOpCodeMask)
        {
            if ((*ptr & SpvOpCodeMask) != SpvOpEntryPoint) continue;

            stage->stage = VkShaderStageFlagBits(1 << *(SpvExecutionModel*)(ptr + 1));
            stage->pName = _strdup((const char*)(ptr + 3));
            break;
        }

        // create shader module
        VkShaderModuleCreateInfo cfg = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            cfg.codeSize = desc->stages[i].size;
            cfg.pCode    = (uint32_t*)desc->stages[i].data;
        vkCreateShaderModule(vctx->device, &cfg, nullptr, &stage->module);

        hash ^= gfx_utils_hash_16((const char*)desc->stages[i].data, desc->stages[i].size);
    }

    vk_shader_t* vk_shader = (vk_shader_t*)vctx_alloc(ctx, sizeof(vk_shader_t));
    if (vk_shader == NULL)
        return;

    *out_shader = &vk_shader->handle;
    vk_shader->ctx = vctx;
    vk_shader->hash = hash;
    vk_shader->stages_count = desc->stages_count;

    memcpy(vk_shader->stages, stages, sizeof(VkPipelineShaderStageCreateInfo) * vk_shader->stages_count);

    vk_shader->uniform_count = desc->uniform_count;
    vk_shader->bindings = (VkDescriptorSetLayoutBinding*)vctx_alloc(ctx, desc->uniform_count * sizeof(VkDescriptorSetLayoutBinding));

    uint32_t ubo_size = 0;
    for (uint32_t i = 0; i < desc->uniform_count; ++i)
    {
        vk_shader->uniforms[i] = desc->uniforms[i];

        uint32_t stage_mask = vk_shader->uniforms[i].stage_mask;

        #define  test_flag(v, f) ((v & f) == f)

        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_vertex)      ? VK_SHADER_STAGE_VERTEX_BIT   : 0;
        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_hull)        ? 0 : 0; //
        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_domain)      ? 0 : 0; //
        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_geometry)    ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_fragment)    ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        vk_shader->bindings[i].stageFlags |= test_flag(stage_mask, 1 << gfx_shader_compute)     ? VK_SHADER_STAGE_COMPUTE_BIT  : 0;

        #undef test_flag

        vk_shader->bindings[i].binding          = vk_shader->uniforms[i].binding;
        vk_shader->bindings[i].descriptorCount  = 1;
        switch (vk_shader->uniforms[i].type)
        {
            case gfx_uniform_sampler:
                vk_shader->bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; 
                break;

            case gfx_uniform_texture2d:
            case gfx_uniform_texture3d:
            case gfx_uniform_texture2d_cube:
                vk_shader->bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
                break;

            case gfx_uniform_ubo:
                vk_shader->bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
                ubo_size += vk_shader->uniforms[i].buffer.size; 
                break;

            case gfx_uniform_storage:
                vk_shader->bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;

            default: assert(false); break;
        }
    }

    // create descripor layout
    VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = vk_shader->uniform_count;
        layout_info.pBindings    = vk_shader->bindings;
    auto result = vkCreateDescriptorSetLayout(vctx->device, &layout_info, nullptr, &vk_shader->layout);
    if (result != VK_SUCCESS)
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create descriptor set layout! (%s)", err_str);
        return;
    }

    // create pipeline layout
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        createInfo.setLayoutCount   = 1;
        createInfo.pSetLayouts      = &vk_shader->layout;
    result = vkCreatePipelineLayout(vctx->device, &createInfo, nullptr, &vk_shader->pipeline_layout);
    if (result != VK_SUCCESS)
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create pipeline layout! (%s)", err_str);
        return;
    }
 //   gfx_pool_create(sizeof(vk_descriptor_set_t), MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->descriptor_set_pool);
}

void vk_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, gfx_descriptor_set_t** out_set)
{
    assert(ctx && shader && out_set);

    vk_context_t* vctx = from_ctx(ctx);
    vk_shader_t* vk_shader = (vk_shader_t*)shader;

    if(vk_shader->pool == nullptr)
    {
        vk_create_descriptor_pool(vctx, vk_shader, MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->pool);
    }

    if(vk_shader->pool->free_set_count == 0)
    {
        vk_create_descriptor_pool(vctx, vk_shader, MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->pool);
    }

    uint32_t next_free = vk_shader->pool->next_free;

    for(uint32_t i = next_free; i < vk_shader->pool->capacity; ++i)
    {
        if(!vk_shader->pool->sets[i].isfree)
            continue;

        vk_shader->pool->sets[i].isfree = false;
        vk_shader->pool->free_set_count--;
        vk_shader->pool->next_free = i + 1;

        *out_set = &vk_shader->pool->sets[i].handle;
        break;
    }
}

void vk_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler)
{
    assert(ctx && desc && out_sampler);

    vk_context_t* vctx = (vk_context_t*)ctx;

    vk_sampler_t* sampler = (vk_sampler_t*)vctx_alloc(ctx, sizeof(vk_sampler_t));
    if (sampler == nullptr)
        return;

    // anisotropy doen't work with point filtration
    VkBool32 use_aniso = (desc->minmag != gfx_filter_point);
    float aniso_level = use_aniso ? desc->anisotropy : 0.0f;

    VkSamplerCreateInfo create_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        create_info.magFilter       = gfx_filter_2_vk(desc->minmag);
        create_info.minFilter       = gfx_filter_2_vk(desc->minmag);
        create_info.mipmapMode      = gfx_mipmap_2_vk(desc->mipmap);
        create_info.addressModeU    = gfx_address_mode_2_vk(desc->mode);
        create_info.addressModeV    = gfx_address_mode_2_vk(desc->mode);
        create_info.addressModeW    = gfx_address_mode_2_vk(desc->mode);

        create_info.mipLodBias      = 0.0f;
        create_info.anisotropyEnable = aniso_level > 0;
        create_info.maxAnisotropy   = aniso_level;
        create_info.compareEnable   = (desc->minmag != gfx_filter_point);  //VK_FALSE for point
        create_info.compareOp       = VK_COMPARE_OP_ALWAYS; //VK_COMPARE_OP_NEVER;
        create_info.minLod          = 0.0f;
        create_info.maxLod          = VK_LOD_CLAMP_NONE;
        create_info.borderColor     = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        create_info.unnormalizedCoordinates = VK_FALSE;
    if (auto result = vkCreateSampler(vctx->device, &create_info, NULL, &sampler->sampler))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create sampler");
        vctx_free(ctx, sampler);
    }
    *out_sampler = &sampler->handle;
}

void vk_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** out_texture)
{
    if(out_texture == nullptr)
        return;
    *out_texture = nullptr;

    vk_context_t* vctx = from_ctx(ctx);
    vk_buffer_t staging = {};

   // size_t size = desc->size;
    uint32_t mem_size = 0;
    for(uint32_t i = 0; i < desc->mip_levels; ++i) 
        mem_size += gfx_utils_image_layer_size(desc->width >> i, desc->height >>i, desc->depth, desc->format);

    VkFormat format = gfx_pixel_format_2_vk(desc->format);
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(vctx->physicaldevice, format, &properties);
    bool support_format = (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
    if(!support_format) {
        vctx->dbg_log(gfx_msg_error, "not supported pixel format %s", gfx_to_string(desc->format));
     //   *out_texture = vctx->default_texture;
        return;
    }

    auto memprop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    _vk_create_buffer(vctx, mem_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memprop, &staging);

    void* dataptr = nullptr;
    vkMapMemory(vctx->device, staging.memory, 0, mem_size, 0, &dataptr);
    memcpy(dataptr, desc->data, mem_size);
    vkUnmapMemory(vctx->device, staging.memory);

    bool host_visible = true;
    auto tiling = (host_visible) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

    VkDeviceMemory memory = VK_NULL_HANDLE;

    // auto format = _vk_find_supported_format(ctx, {image->format}, tiling, VK_FORMAT_FEATURE_TRANSFER_DST_BIT);
    // format = format | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

    VkExtent3D extend { desc->width, desc->height, desc->depth };
   
    VkImage vkimg = _vk_create_image(vctx, VK_IMAGE_TYPE_2D, extend, desc->mip_levels, 
        format,
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory);

    vk_copy_info_t info = { };
        info.dst_image = vkimg;
        info.dst_image_mips = desc->mip_levels;
        info.dst_image_format = desc->format;
        info.dst_image_extend = extend;

    _vk_image_transition(vctx, vkimg, desc->mip_levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _vk_copy_buffer_to(vctx, staging.buffer, &info);
    _vk_image_transition(vctx, vkimg, desc->mip_levels, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vctx->device, staging.buffer, nullptr);
    vkFreeMemory(vctx->device, staging.memory, nullptr);

    VkImageView imageView = _vk_create_image_view(vctx, VK_IMAGE_VIEW_TYPE_2D, vkimg, format, desc->mip_levels);

    uint64_t handle = gfx_pool_alloc(vctx->texture_pool);
    vk_texture_t* texture = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, handle);
    if (texture != nullptr)
    {
        texture->view = imageView;
        texture->image = vkimg;
        texture->memory = memory;
        texture->memory_size = mem_size;
        *out_texture = &texture->handle;
    }
}

void vk_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** out_pipeline)
{
    assert(ctx);
    vk_context_t* vctx = (vk_context_t*)ctx;
    vk_shader_t* vkshader = (vk_shader_t*)desc->shader;

    if(vkshader == nullptr)
    {
        vctx->dbg_log(gfx_msg_error, "failed to create pipeline: shader is NULL");
        return;
    }

    VkVertexInputBindingDescription vertex_descriptor[8] = {};
    for(uint32_t i = 0; i < desc->assembly.slot_count; ++i)
    {
        vertex_descriptor[i].binding   = desc->assembly.slots[i].binding;
        vertex_descriptor[i].stride    = desc->assembly.slots[i].stride;
        switch (desc->assembly.slots[i].rate)
        {
            case gfx_vertex_rate_vertex:    vertex_descriptor[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;   break;
            case gfx_vertex_rate_instance:  vertex_descriptor[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; break;
        }
     }

    uint32_t attribute_count = desc->assembly.attributes_count;
    VkVertexInputAttributeDescription attributes[16] = {};
    for( uint32_t i = 0; i < attribute_count; ++i)
    {
        attributes[i].binding   = desc->assembly.attributes[i].binding;
        attributes[i].location  = desc->assembly.attributes[i].location;
        attributes[i].offset    = desc->assembly.attributes[i].offset;
        attributes[i].format    = gfx_vertex_format_2_vk(desc->assembly.attributes[i].format);
    }

    VkPipelineInputAssemblyStateCreateInfo  assembly = {  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    assembly.topology = gfx_topology_2_vk(desc->assembly.topology);
    assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input.vertexBindingDescriptionCount      = desc->assembly.slot_count;
    vertex_input.pVertexBindingDescriptions         = vertex_descriptor;

    vertex_input.vertexAttributeDescriptionCount    = attribute_count;
    vertex_input.pVertexAttributeDescriptions       = attributes;

    VkPipelineRasterizationStateCreateInfo  rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.depthClampEnable         = VK_FALSE;
    rasterizer.rasterizerDiscardEnable  = VK_FALSE;
    rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth                = 1.0f;
    rasterizer.cullMode                 = gfx_cull_2_vk(desc->render_states.culling);
    rasterizer.frontFace                = vk_face[desc->render_states.face];
    rasterizer.depthBiasEnable          = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.sampleShadingEnable   = VK_FALSE;//vctx->msaa_samples != VK_SAMPLE_COUNT_1_BIT;
    multisampling.rasterizationSamples  = vctx->msaa_samples;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount         = 1;
    viewportState.pViewports            = NULL;
    viewportState.scissorCount          = 1;
    viewportState.pScissors             = NULL;

    // blend
    VkPipelineColorBlendAttachmentState blend_attachment    = {};
    blend_attachment.blendEnable            = desc->render_states.blend.enable;
    blend_attachment.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    blend_attachment.srcColorBlendFactor    = gfx_blend_mode_2_vk(desc->render_states.blend.color_src);
    blend_attachment.dstColorBlendFactor    = gfx_blend_mode_2_vk(desc->render_states.blend.color_dst);
    blend_attachment.srcAlphaBlendFactor    = gfx_blend_mode_2_vk(desc->render_states.blend.alpha_src);
    blend_attachment.dstAlphaBlendFactor    = gfx_blend_mode_2_vk(desc->render_states.blend.alpha_dst);


    VkPipelineColorBlendStateCreateInfo blend_state = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        blend_state.logicOpEnable               = VK_FALSE;
        blend_state.logicOp                     = VK_LOGIC_OP_COPY;
        blend_state.attachmentCount             = 1;
        blend_state.pAttachments                = &blend_attachment;

    // depth
    VkPipelineDepthStencilStateCreateInfo depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil.depthTestEnable           = VK_TRUE;
    depth_stencil.depthWriteEnable          = VK_TRUE;
    depth_stencil.depthCompareOp            = gfx_cmp_2_vk(desc->render_states.depth.mode);// VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable     = VK_FALSE;
    depth_stencil.stencilTestEnable         = VK_FALSE;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dyn_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn_state_info.dynamicStateCount    = _countof(dynamic_states);
    dyn_state_info.pDynamicStates       = dynamic_states;

    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount             = vkshader->stages_count;
    pipelineInfo.pStages                = vkshader->stages;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pColorBlendState       = &blend_state;
    pipelineInfo.pDepthStencilState     = &depth_stencil;
    pipelineInfo.pInputAssemblyState    = &assembly;
    pipelineInfo.pVertexInputState      = &vertex_input;
    pipelineInfo.pDynamicState          = &dyn_state_info;
    pipelineInfo.layout                 = vkshader->pipeline_layout;
    pipelineInfo.renderPass             = vctx->default_renderpass;

    vk_pipeline_t* vkpipeline = (vk_pipeline_t*)vctx_alloc(ctx, sizeof(vk_pipeline_t));
    if(vkpipeline == nullptr)
    {
        vctx->dbg_log(gfx_msg_error, "failed to allocate vk_pipeline_t");
        return;
    }
    vkCreateGraphicsPipelines(vctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkpipeline->pipeline);

    *out_pipeline = &vkpipeline->handle;
}

void vk_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** target)
{
    vk_context_t* vctx = from_ctx(ctx);
}

void vk_create_cmd(gfx_context_t* ctx, uint32_t count, gfx_command_buffer_t** out_cmd)
{
    vk_context_t* vctx = (vk_context_t*)ctx;

    uint32_t thread_id = gfx_utils_thread_id();

    if(count == 1 && vctx->cmd_pool_size != 0)
    {
        for(int j = 0; j < _countof(vctx->cmd_buffer_pool); ++j)
        {
            if( vctx->cmd_buffer_pool[j] != nullptr && vctx->cmd_buffer_pool[j]->thread_id == thread_id)
            {
                vk_command_buffer_t * tmp =  vctx->cmd_buffer_pool[j];
                *out_cmd = &tmp->handle;

                vctx->cmd_pool_size--;
                vctx->cmd_buffer_pool[j] = nullptr;
                return;
            }
        }
    }

    // create pool
    VkCommandPool pool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        pool_info.flags             = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex  = vctx->graphics_queue.family;
 
    if (auto result = vkCreateCommandPool(vctx->device, &pool_info, nullptr, &pool))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create command pool!(%s)", result);
    }

    // create command buffers
    VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool           = pool;
        alloc_info.commandBufferCount    = count;

    VkCommandBuffer tmpbuffers[16] = {};
    if (auto result = vkAllocateCommandBuffers(vctx->device, &alloc_info, tmpbuffers))
    {
        vctx->dbg_log(gfx_msg_error, "failed to allocate command buffers!");
    }

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)vctx_alloc(ctx, count * sizeof(vk_command_buffer_t));
    if(vk_cmd == nullptr)
        return;

    for (uint32_t i = 0; i < count; ++i)
    {
        vk_cmd[i].cmd       = tmpbuffers[i];
        vk_cmd[i].ctx       = vctx;
        vk_cmd[i].pool      = pool;
        vk_cmd[i].device    = vctx->device;
        vk_cmd[i].thread_id = gfx_utils_thread_id();

        out_cmd[i] = &vk_cmd[i].handle;
    }
}



// destroy render stuff

void vk_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_buffer_t* vkbuffer = (vk_buffer_t*)buffer;

    vk_buffer_t * buf = (vk_buffer_t*)gfx_pool_map(vctx->buffers_pool, vkbuffer->handle.idx);
    if(buf)
    {
        if (vkbuffer->mapped && vkbuffer->buffer != nullptr)
            vkUnmapMemory(vctx->device, vkbuffer->memory);

        if (vkbuffer->buffer != nullptr)
            vkDestroyBuffer(vctx->device, vkbuffer->buffer, nullptr);

        if (vkbuffer->memory != nullptr)
            vkFreeMemory(vctx->device, vkbuffer->memory, nullptr);

        gfx_pool_free(vctx->buffers_pool, vkbuffer->handle.idx);
    }
    else
    {
        vctx->dbg_log(gfx_msg_error, "invalid buffer handle %d", vkbuffer->handle.idx);
    }
}

void vk_destroy_shader(gfx_context_t* ctx, gfx_shader_t* buffer)
{
    vk_context_t* vkctx = from_ctx(ctx);

    //destroy all pools and pool datas
}

void vk_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_sampler_t* vks = (vk_sampler_t*)sampler;

    vkDestroySampler(vctx->device, vks->sampler, nullptr);
}

void vk_destroy_texture(gfx_context_t* ctx, gfx_texture_t* _texture)
{
    vk_context_t* vctx = from_ctx(ctx);

    vk_texture_t* texture = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, _texture->idx);
    if(texture != nullptr)
    {
        vkDestroyImageView(vctx->device, texture->view, nullptr);
        vkDestroyImage(vctx->device, texture->image, nullptr);
        vkFreeMemory(vctx->device, texture->memory, nullptr);

        texture->view     = VK_NULL_HANDLE;
        texture->image    = VK_NULL_HANDLE;
        texture->memory   = VK_NULL_HANDLE;

        gfx_pool_free(vctx->texture_pool, _texture->idx);
    }
}

void vk_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
    if(!ctx || !pipeline)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_pipeline_t * vkpipeline = (vk_pipeline_t*)pipeline;
    vkDestroyPipeline(vctx->device, vkpipeline->pipeline, nullptr);

    vctx_free(ctx, vkpipeline);
}

void vk_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* target)
{
    if (!ctx || !target)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_render_target_t *vkrt = (vk_render_target_t*)target;

    vkDestroyRenderPass(vctx->device, vkrt->renderpass, nullptr);
    vkDestroyFramebuffer(vctx->device, vkrt->framebuffer, nullptr);

    vctx_free(ctx, vkrt);
}

void vk_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor)
{
    if (!ctx || !descriptor)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_descriptor_set_t * vkset = (vk_descriptor_set_t*)descriptor;

    vkset->isfree = true;
    vkset->pool->next_free = vkset->index_in_pool;
    vkset->pool->free_set_count++;
}

void vk_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_command_buffer_t * vkcmd = (vk_command_buffer_t*)cmd;
    if(vctx == nullptr || vkcmd == nullptr)
        return;

    for (int i = 0; i < _countof(vctx->cmd_buffer_pool); ++i)
    {
        if (vctx->cmd_buffer_pool[i] == nullptr)
        {
            vkResetCommandBuffer(vkcmd->cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
            vctx->cmd_buffer_pool[i] = vkcmd;
            vctx->cmd_pool_size++;
            return;
        }
    }

    vkResetCommandPool(vctx->device, vkcmd->pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    vkFreeCommandBuffers(vctx->device, vkcmd->pool, 1, &vkcmd->cmd);
    vkDestroyCommandPool(vctx->device, vkcmd->pool, nullptr);
}

void vk_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset)
{
    auto vkctx = (vk_context_t*)(ctx);
    auto vkbuf = (vk_buffer_t*) (buffer);

    if(vkbuf->mapped)
    {
        if(vkbuf->data_ptr == nullptr)
            vkMapMemory(vkctx->device, vkbuf->memory, offset, size, 0, &vkbuf->data_ptr);

        if(size > 0 && data != nullptr)
            memcpy(vkbuf->data_ptr, (char*)data + offset, size);
    }
    else if(data != nullptr)
    {
        auto staging = vkctx->staging_buffer;
        if (size < staging->buffer_size )
        {
            if (staging->data_ptr == nullptr)
                vkMapMemory(vkctx->device, staging->memory, 0, staging->buffer_size, 0, &staging->data_ptr);
            memcpy(staging->data_ptr, data, size);
            // vkUnmapMemory(ctx->device, s_staging.memory);
             //vkFlushMappedMemoryRanges();

            vk_copy_info_t info = { };
                info.dst_buffer = vkbuf->buffer;
                info.dst_buffer_size = (uint32_t)size;
                info.dst_buffer_offset = offset;
            _vk_copy_buffer_to(vkctx, vkctx->staging_buffer->buffer, &info);
            return;
        }
        else
        {
            vk_buffer_t staging = {};
            auto memflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            _vk_create_buffer(vkctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memflag, &staging);

            if (data)
            {
                vkMapMemory(vkctx->device, staging.memory, 0, size, 0, &staging.data_ptr);
                memcpy(staging.data_ptr, data, size);
                vkUnmapMemory(vkctx->device, staging.memory);
                staging.data_ptr = nullptr;
            }

            vk_copy_info_t info = { };
            info.dst_buffer = vkbuf->buffer;
            info.dst_buffer_size = (uint32_t)size;
            _vk_copy_buffer_to(vkctx, staging.buffer, &info);

            vkDestroyBuffer(vkctx->device, staging.buffer, nullptr);
            vkFreeMemory(vkctx->device, staging.memory, nullptr);
        }
    }
}

uint64_t vk_uniform_location(gfx_shader_t* shader, const char* name)
{
    if(shader == nullptr || name == nullptr)
        return 0;

    vk_shader_t * vkshader = (vk_shader_t*)shader;
    uint16_t  hash = vkshader->hash;

    for (uint32_t uniform_id = 0; uniform_id < vkshader->uniform_count; ++uniform_id)
    {
        gfx_uniform_t* uniform = &vkshader->uniforms[uniform_id];

        if(uniform->type == gfx_uniform_ubo)
        {
            for (int16_t field_id = 0; field_id < uniform->buffer.field_count; ++field_id)
            {
                if (strcmp(name, uniform->buffer.fields[field_id].name))
                    continue;

                return  (uint64_t)(hash & 0xFFFF) |
                        (((uint64_t)uniform_id & 0xFFFF) << 16) |
                        (((uint64_t)field_id & 0xFFFF) << 32);
            }
        }

        if (strcmp(name, uniform->name))
            continue;

        return  (hash & 0xFFFF) | 
                (uniform_id & 0xFFFF) << 16;
    }
    return 0;
}

void vk_uniform_set_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;
    vk_shader_t* vkshader = (vk_shader_t*)vkset->shader;

    uint16_t hash       = (handle >> 00) & 0xFFFF;
    uint16_t unform_id  = (handle >> 16) & 0xFFFF;
    uint16_t child_id   = (handle >> 32) & 0xFFFF;

    if(vkshader->hash != hash)
        return;

  //  const spirvflect_uniform_t* uniform = unform_id < vkset->shader->binding_count ? &vkset->shader->uniforms[unform_id] : nullptr;
    const gfx_uniform_t* uniform = unform_id < vkset->shader->uniform_count ? &vkset->shader->uniforms[unform_id] : nullptr;
/*
    if(_uniform == nullptr)
        return;

    if (_uniform->type != gfx_uniform_ubo)
    {
        vkshader->ctx->dbg_log(gfx_msg_error, "uniform_set_buffer_data() incorrect type for %s uniform name", _uniform->name);
        return;
    }*/

    if ((uniform != nullptr) && (uniform->type == gfx_uniform_ubo) && (child_id < uniform->buffer.field_count))
    {
        uint32_t offset = uniform->buffer.fields[child_id].offset;
        memcpy(vkset->uboptr + offset, data, size);
    }
}

void vk_uniform_set_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset)
{
}

void vk_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;

    vk_shader_t * vkshader = (vk_shader_t*)vkset->shader;
    vk_texture_t* vktexture = (vk_texture_t*)texture;
    vk_context_t* vkctx = vkshader->ctx;
    uint16_t hash = handle & 0xFFFF;
    uint16_t unform_id = (handle >> 16) & 0xFFFF;
    uint16_t child_id = (handle >> 32) & 0xFFFF;

    if (vkshader->hash != hash)
        return;


    vkset->write_infos[unform_id].image_info.imageView = texture?vktexture->view : vkctx->default_texture->view;
    vkset->dirty = true;

    vkUpdateDescriptorSets(vkctx->device, vkshader->uniform_count, vkset->writes, 0, NULL);
}

void vk_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;
    vk_shader_t* vkshader = (vk_shader_t*)vkset->shader;
    vk_sampler_t* vksampler = (vk_sampler_t*)sampler;
    vk_context_t* vkctx = vkshader->ctx;

    uint16_t hash = handle & 0xFFFF;
    uint16_t unform_id = (handle >> 16) & 0xFFFF;
    uint16_t child_id = (handle >> 32) & 0xFFFF;

    vkset->write_infos[unform_id].image_info.sampler = vksampler->sampler;
    vkset->dirty = true;

    vkUpdateDescriptorSets(vkctx->device, vkshader->uniform_count, vkset->writes, 0, NULL);
}


// rendering 

void vk_cmd_begin(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkResetCommandPool(vk_cmd->device, vk_cmd->pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    if (auto result = vkBeginCommandBuffer(vk_cmd->cmd, &begin)) {
        auto err_str = string_VkResult(result);
        vk_cmd->ctx->dbg_log(gfx_msg_error, "vkBeginCommandBuffer failed!(s)", err_str);
    }
}

void vk_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_render_target_t* target)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_render_target_t* vk_target = (vk_render_target_t*)target;

    VkClearValue clear_value[] = {
        { 0.21f, 0.21f, 0.21f, 1.0f },  // color
        { 1.0f, 0 }                     // depth stencil
    };
    VkRect2D render_area = {
        {0,0},                          // offset
        vk_target->extend               // extent
    };

    VkRenderPassBeginInfo info  = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        info.renderPass         = vk_target->renderpass;
        info.framebuffer        = vk_target->framebuffer;
        info.renderArea         = render_area;
        info.clearValueCount    = 2;
        info.pClearValues       = clear_value;
    vkCmdBeginRenderPass(vk_cmd->cmd, &info, VK_SUBPASS_CONTENTS_INLINE);

    vk_cmd_viewport(cmd, 0, 0, vk_target->extend.width, vk_target->extend.height);
    vk_cmd_scissor(cmd, 0, 0, vk_target->extend.width, vk_target->extend.height);
}

void vk_cmd_end_pass(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdEndRenderPass(vk_cmd->cmd);
}

void vk_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkRect2D scissor;
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = w;
    scissor.extent.height = h;

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdSetScissor(vk_cmd->cmd, 0, 1, &scissor);
}

void vk_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkRect2D vp = { {(int32_t)x,(int32_t)y}, {w,h}};
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    VkViewport viewport = {};
    viewport.x = (float)vp.offset.x;
    viewport.y = (float)vp.offset.y;
    viewport.width = (float)vp.extent.width;
    viewport.height = (float)vp.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vk_cmd->cmd, 0, 1, &viewport);
}

void vk_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline)
{
    assert(cmd && pipeline);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_pipeline_t * vkpipeline = (vk_pipeline_t*)pipeline;

    vkCmdBindPipeline(vk_cmd->cmd, vkpipeline->bind_point, vkpipeline->pipeline);
}

void vk_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, gfx_descriptor_set_t* descriptor)
{
    if(descriptor == nullptr)
        return;

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_descriptor_set_t* set = (vk_descriptor_set_t*)descriptor;

    auto count = 1u;
    VkDescriptorSet descriptor_set = set->descriptor_set;
    VkPipelineLayout pipeline_layout = set->shader->pipeline_layout;

    vkCmdBindDescriptorSets(vk_cmd->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        set->shader->pipeline_layout, 0,
        1u, &descriptor_set, 0, nullptr);
}

void vk_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, gfx_buffer_t* buffer)
{
    vk_buffer_t* vkbuffer = (vk_buffer_t*)buffer;
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    VkIndexType index_type = gfx_index_format_2_vk(format);
    vkCmdBindIndexBuffer(vk_cmd->cmd, vkbuffer->buffer, 0, index_type);
}

void vk_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t binding, gfx_buffer_t* buffer)
{
    vk_buffer_t* vkbuffer = (vk_buffer_t*)buffer;
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(vk_cmd->cmd, binding, 1, &vkbuffer->buffer, offsets);
}

void vk_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdDraw(vk_cmd->cmd, vertex_count, instance_count, 0, 0);
}

void vk_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdDrawIndexed(vk_cmd->cmd, idx_count, instance_count, first_idx, 0, 0);
}

void vk_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z)
{
    vk_command_buffer_t* vkcmd = (vk_command_buffer_t*)cmd;
    vkCmdDispatch(vkcmd->cmd, x, y, z);
}

void vk_cmd_end(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    if (auto result = vkEndCommandBuffer(vk_cmd->cmd)) {
        auto err_str = string_VkResult(result);
       // ctx->dbg_log(gfx_msg_error, "vkEndCommandBuffer failed!(s)", err_str);
    }
}


void vk_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options)
{
    vk_context_t* vkctx = from_ctx(ctx);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

  //  VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
    VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkFence fence = VK_NULL_HANDLE;
    if (options == gfx_submit_wait_for_fence)
    {
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(vkctx->device, &fenceInfo, nullptr, &fence);
    }

    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &vk_cmd->cmd;

    if (options == gfx_submit_wait_for_image_ready)
    {
        fence = vkctx->semaphores[vkctx->frame_idx].wait_fence;
        submit.pWaitDstStageMask    = stages;
        submit.waitSemaphoreCount   = 1;
        submit.pWaitSemaphores      = &vkctx->semaphores[vkctx->frame_idx].image_available;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &vkctx->semaphores[vkctx->frame_idx].rendering_finished;
    }

    if (auto result = vkQueueSubmit(vkctx->graphics_queue.queue, 1, &submit, fence))
    {
        auto err_str = string_VkResult(result);
        vkctx->dbg_log(gfx_msg_error, "vkQueueSubmit failed!(%s)", err_str);
    }

    VkResult wait_result = VK_SUCCESS;

    switch (options)
    {
        case gfx_submit_nowait:break;
        case gfx_submit_wait_for_fence:         wait_result = vkWaitForFences(vkctx->device, 1, &fence, VK_TRUE, 1000000000); break;
        case gfx_submit_wait_for_queue_idle:    wait_result = vkQueueWaitIdle(vkctx->graphics_queue.queue); break;
        case gfx_submit_wait_for_device_idle:   wait_result = vkDeviceWaitIdle(vkctx->device); break;
        case gfx_submit_wait_for_image_ready:   wait_result = vkQueueWaitIdle(vkctx->graphics_queue.queue); break;
      //  case gfx_submit_wait_for_image_ready:   wait_result = vkDeviceWaitIdle(vkctx->device); break;
    }

    if(wait_result != VK_SUCCESS)
    {
        vkctx->dbg_log(gfx_msg_error, "wait failed!(%s)", string_VkResult(wait_result));
    }

    if (options == gfx_submit_wait_for_fence)
    {
        vkResetFences(vkctx->device, 1, &fence);
        vkDestroyFence(vkctx->device, fence, nullptr);
    }
}



//////////////////////////////// utils /////////////////////////////////////////////////

void vk_debug_set_name(vk_context_t* ctx, uint64_t vkobject, VkObjectType type, const char* name)
{
/*
    static PFN_vkSetDebugUtilsObjectNameEXT pfnvkSetDebugUtilsObjectNameEXT = nullptr;
    if(pfnvkSetDebugUtilsObjectNameEXT == nullptr)
        pfnvkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(ctx->instance, "vkDebugMarkerSetObjectNameEXT");

    if(pfnvkSetDebugUtilsObjectNameEXT != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type,
                                    (uint64_t)vkobject, name };
        pfnvkSetDebugUtilsObjectNameEXT(ctx->device, &nameInfo);
    }

    static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;

    if (pfnDebugMarkerSetObjectName == VK_NULL_HANDLE)
        vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)load(context, "vkSetDebugUtilsObjectNameEXT");
        pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetInstanceProcAddr(ctx->instance, "vkDebugMarkerSetObjectNameEXT");

    if (pfnDebugMarkerSetObjectName == VK_NULL_HANDLE)
        return;*/



    /*VkDebugMarkerObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType     = type;
    nameInfo.object         = vkobject;
    nameInfo.pObjectName    = name;
    pfnDebugMarkerSetObjectName(ctx->device, &nameInfo);*/
    //vkDebugMarkerSetObjectNameEXT(ctx->device, &nameInfo);
}


void vk_debug_begin_region(vk_context_t* ctx, VkCommandBuffer cmd, const char* name, uint32_t color)
{
    static PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;

    if (pfnCmdDebugMarkerBegin == VK_NULL_HANDLE)
        pfnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)(vkGetDeviceProcAddr(ctx->device, "vkCmdDebugMarkerBeginEXT"));

    if (pfnCmdDebugMarkerBegin == VK_NULL_HANDLE)
        return;

    VkDebugMarkerMarkerInfoEXT markerInfo = { VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT };
    markerInfo.color[0] = ((color >> 0) & 0xFF) / 255.0f;
    markerInfo.color[1] = ((color >> 8) & 0xFF) / 255.0f;
    markerInfo.color[2] = ((color >> 16) & 0xFF) / 255.0f;
    markerInfo.color[3] = ((color >> 24) & 0xFF) / 255.0f;
    markerInfo.pMarkerName = name;
    pfnCmdDebugMarkerBegin(cmd, &markerInfo);
}


void vk_debug_end_region(vk_context_t* ctx, VkCommandBuffer cmd, const char* name)
{
    static PFN_vkCmdDebugMarkerEndEXT pfnvkCmdDebugMarkerEnd = VK_NULL_HANDLE;

    if (pfnvkCmdDebugMarkerEnd == VK_NULL_HANDLE)
        pfnvkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)(vkGetDeviceProcAddr(ctx->device, "vkCmdDebugMarkerEndEXT"));

    pfnvkCmdDebugMarkerEnd(cmd);
}


void vk_debug_set_texture_name(vk_context_t* ctx, vk_texture_t* texture, const char* name)
{
    //vk_debug_set_name(ctx, (uint64_t)texture->image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
}


void vk_debug_set_buffer_name(vk_context_t* ctx, vk_buffer_t* buffer, const char* name)
{
   // vk_debug_set_name(ctx, (uint64_t)buffer->buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
}


void vk_debug_set_shader_name(vk_context_t* ctx, vk_shader_t* shader, const char* name)
{
    for (uint32_t i = 0; i < shader->stages_count; ++i)
    {
  //      vk_debug_set_name(ctx, (uint64_t)shader->stages[i].module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name);
    }
}

#endif