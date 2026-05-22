#include "gfx_vulkan.h"
#include "gfx_memory.h"

#ifdef VULKAN_AVAILABLE

#include <vulkan/vk_enum_string_helper.h>
#include <spirv_cross/spirv.h>

#ifdef _WIN32
    #pragma comment(lib, "../lib/vulkan-1.lib")
#endif

extern const char* gfx_to_string(gfx_buffer_usage usage);
extern const char* gfx_to_string(gfx_shader_stage stage);
extern const char* gfx_to_string(gfx_texture_type type);
extern const char* gfx_to_string(gfx_pixel_format format);


VkFormat gfx_pixel_format_2_vk(gfx_pixel_format format)
{
    switch (format) {
        case gfx_pixel_format_a8:               return VK_FORMAT_R8_UNORM;
        case gfx_pixel_format_rgba4444:         return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        case gfx_pixel_format_rgb5a1:           return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        case gfx_pixel_format_rgb565:           return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case gfx_pixel_format_rgba8:            return VK_FORMAT_R8G8B8A8_UNORM;

        case gfx_pixel_format_etc1:             return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case gfx_pixel_format_etc2_rgb8a1:      return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case gfx_pixel_format_etc2_rgba8:       return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

        case gfx_pixel_format_bc1:              return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;      //! bc1 8
        case gfx_pixel_format_bc3:              return VK_FORMAT_BC3_UNORM_BLOCK;           //! bc3 16
        case gfx_pixel_format_bc4:              return VK_FORMAT_BC4_UNORM_BLOCK;           //! bc3 16
        case gfx_pixel_format_bc5:              return VK_FORMAT_BC5_UNORM_BLOCK;           //! bc3 16
        case gfx_pixel_format_bc6h:             return VK_FORMAT_BC6H_UFLOAT_BLOCK;         //! bc6 16
        case gfx_pixel_format_bc7:              return VK_FORMAT_BC7_UNORM_BLOCK;           //! bc7 16

        case gfx_pixel_format_astc4x4:          return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case gfx_pixel_format_astc5x5:          return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case gfx_pixel_format_astc6x6:          return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case gfx_pixel_format_astc8x8:          return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case gfx_pixel_format_astc10x10:        return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case gfx_pixel_format_astc12x12:        return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;

        case gfx_pixel_format_astc4x4_hdr:      return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK;

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
    switch (fromat) {
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
    switch(format) {
        case gfx_index_format_16:           return VK_INDEX_TYPE_UINT16;
        case gfx_index_format_32:           return VK_INDEX_TYPE_UINT32;
    }
    return VK_INDEX_TYPE_UINT32;
}

VkPrimitiveTopology gfx_topology_2_vk(gfx_topology topogy)
{
    switch(topogy) {
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


VkFilter gfx_filter_2_vk(gfx_filter filter) {
    switch(filter) {
        case gfx_filter_point:              return VK_FILTER_NEAREST;
        case gfx_filter_linear:             return VK_FILTER_LINEAR;
    }
    return VK_FILTER_NEAREST;
}

VkSamplerMipmapMode gfx_mipmap_2_vk(gfx_filter mipmap) {
    switch (mipmap) {
        case gfx_filter_point:              return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case gfx_filter_linear:             return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

VkSamplerAddressMode gfx_address_mode_2_vk(gfx_address_mode mode) {
    switch (mode) {
        case gfx_address_mode_repeat:           return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case gfx_address_mode_mirror_repeat:    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case gfx_address_mode_clamp_to_edge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        default:                                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkCompareOp gfx_cmp_2_vk(gfx_cmp cmp) {
    switch(cmp) {
        case gfx_cmp_never:                     return VK_COMPARE_OP_NEVER;
        case gfx_cmp_less:                      return VK_COMPARE_OP_LESS;
        case gfx_cmp_equal:                     return VK_COMPARE_OP_EQUAL;
        case gfx_cmp_lequal:                    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case gfx_cmp_greater:                   return VK_COMPARE_OP_GREATER;
        case gfx_cmp_not_equal:                 return VK_COMPARE_OP_NOT_EQUAL;
        case gfx_cmp_gequal:                    return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case gfx_cmp_always:                    return VK_COMPARE_OP_ALWAYS;   
    }
    return VK_COMPARE_OP_ALWAYS;
};

VkStencilOp gfx_stencil_op_2_vk(gfx_stencil_op op) {
    switch(op) {
        case gfx_stencil_op_zero:               return VK_STENCIL_OP_ZERO;
        case gfx_stencil_op_keep:               return VK_STENCIL_OP_KEEP;
        case gfx_stencil_op_replace:            return VK_STENCIL_OP_REPLACE;
        case gfx_stencil_op_incr:               return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case gfx_stencil_op_incr_wrap:          return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case gfx_stencil_op_decr:               return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case gfx_stencil_op_decr_wrap:          return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case gfx_stencil_op_invert:             return VK_STENCIL_OP_INVERT;
    }
    return VK_STENCIL_OP_KEEP;
}

VkBlendFactor gfx_blend_mode_2_vk(gfx_blend_mode mode) {
    switch(mode) {
        case gfx_blend_mode_zero:               return VK_BLEND_FACTOR_ZERO;
        case gfx_blend_mode_one:                return VK_BLEND_FACTOR_ONE;
        case gfx_blend_mode_src_color:          return VK_BLEND_FACTOR_SRC_COLOR;
        case gfx_blend_mode_inv_src_color:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case gfx_blend_mode_src_alpha:          return VK_BLEND_FACTOR_SRC_ALPHA;
        case gfx_blend_mode_inv_src_alpha:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case gfx_blend_mode_dst_alpha:          return VK_BLEND_FACTOR_DST_ALPHA;
        case gfx_blend_mode_inv_dest_alpha:     return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case gfx_blend_mode_dst_color:          return VK_BLEND_FACTOR_DST_COLOR;
        case gfx_blend_mode_inv_dst_color:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    }
    return VK_BLEND_FACTOR_ONE;
}

VkBlendOp gfx_blend_op_2_vk(gfx_blend_op op) {
    switch(op) {
        case gfx_blend_op_add:                  return VK_BLEND_OP_ADD;
        case gfx_blend_op_min:                  return VK_BLEND_OP_MIN;
        case gfx_blend_op_max:                  return VK_BLEND_OP_MAX;
        case gfx_blend_op_subtract:             return VK_BLEND_OP_SUBTRACT;
        case gfx_blend_op_rev_subtract:        return VK_BLEND_OP_REVERSE_SUBTRACT;
    }
    return VK_BLEND_OP_ADD;
}

VkImageAspectFlags determine_aspect_mask(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:              return VK_IMAGE_ASPECT_DEPTH_BIT;

        case VK_FORMAT_S8_UINT:                 return VK_IMAGE_ASPECT_STENCIL_BIT;

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        default:                                return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}


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


typedef struct vk_state_mapping_t {
    VkPipelineStageFlags2 stage;
    VkAccessFlags2        access;
    VkImageLayout         image_layout;
} vulkan_state_mapping_t;

static vk_state_mapping_t get_vulkan_state(gfx_barrier state, VkImageAspectFlags aspect) {
    vk_state_mapping_t out = { 0 };

    switch (state) {
    case gfx_barrier_transfer:
        out.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        out.access = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_GENERAL; // or TRANSFER_DST_OPTIMAL, but GENERAL univeral for RW
        break;

    case gfx_barrier_compute:
        out.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        out.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_GENERAL;
        break;
/*
    case gfx_barrier_compute_read:
        out.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        out.access = VK_ACCESS_2_SHADER_READ_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        break;

    case gfx_barrier_compute_write:
        out.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        out.access = VK_ACCESS_2_SHADER_WRITE_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_GENERAL;
        break;*/

     // Reading in graphics shaders (VS, FS, and also Mesh/Task shaders!)
    case gfx_barrier_graphics: // read
        // enable for the Vertex, Fragment and  Mesh stages for full cross reading
        out.stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
        out.access = VK_ACCESS_2_SHADER_READ_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        break;

    case gfx_barrier_render_target:
        out.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        out.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        break;

    case gfx_barrier_depth_stencil:
        // Important: depth tests occur at early and late stages of fragments 
        out.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        out.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        break;

        //Reading indirect arguments (Compute Cull -> ExecuteIndirect)
    case gfx_barrier_indirect:
        out.stage = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
        out.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        out.image_layout = VK_IMAGE_LAYOUT_UNDEFINED; // buffers don't have a layout
        break;

    case gfx_barrier_present:
        out.stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        out.access = VK_ACCESS_2_NONE;
        out.image_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        break;

    default:
        out.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        out.access = VK_ACCESS_2_NONE;
        out.image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        break;
    }

    return out;
}


static void gfx_default_log(gfx_msg type, const char * msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("\n");
    vprintf(msg, args);
    fflush(stdout);
    va_end(args);
};

static void* _gfx_alloc(vk_context_t* ctx, size_t size) {
    void * ptr = ctx->allocator.gfx_alloc(size, ctx);
    return memset(ptr, 0, size);
}
static void _gfx_free(vk_context_t* ctx, void* ptr) {
    ctx->allocator.gfx_free(ptr, ctx);
}


static vk_context_t* from_ctx(gfx_context_t* ctx) 
{
    return (vk_context_t*)ctx;
}



VkAllocationCallbacks *vk_default_allocation_callbacks()
{
    static VkAllocationCallbacks cb;
    if(cb.pfnAllocation == nullptr)
        cb.pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
            return malloc(size);
        };

    if (cb.pfnReallocation == nullptr)
        cb.pfnReallocation = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
            return realloc(pOriginal, size);
        };

    if (cb.pfnFree == nullptr)
        cb.pfnFree = [](void* pUserData, void* ptr) {
            free(ptr);
        };

    if (cb.pfnInternalAllocation == nullptr)
        cb.pfnInternalAllocation = [] (void*  pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
            printf("\npInternalAllocation(%zd)", size);
        };

    if (cb.pfnInternalFree == nullptr)
        cb.pfnInternalFree = [](void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
            printf("\npInternalFree(%zd)", size);
         };

    return &cb;
}


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

   // if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        printf("\n%s: %s,%s \033[0m", msgtype, layerPrefix, msg);
    }
    return VK_FALSE;
}

static VkBool32  vkDebugCallback2( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    printf( "Vulkan-Validation", "[ID: %d, Name: %s] -> %s",
        pCallbackData->messageIdNumber,
        pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "UNKNOWN",
        pCallbackData->pMessage);
    return VK_FALSE;
}



static VkFormat _vk_find_supported_format(vk_context_t* ctx, VkFormat* candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx->vk_physical_device, format, &props);

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

static const char* _vk_resolve_vendor_name(uint32_t vendor_id) {
    switch (vendor_id) {
        case 0x10DE: return "NVIDIA";
        case 0x1002: return "AMD";
        case 0x8086: return "Intel";
        case 0x13B5: return "ARM";
        case 0x1014: return "IBM";
        case 0x14E4: return "Broadcom";
        case 0x106B: return "Apple";
        case 0x5143: return "Qualcomm";
        default:     return "Unknown Vendor";
    }
}

void vk_fill_device_caps(vk_context_t* ctx, VkPhysicalDevice physical_device, gfx_caps_t* caps)
{
    if (!physical_device || !caps) return;
    memset(caps, 0, sizeof(gfx_caps_t));

    // =========================================================================
    // 1. BASE PROPERTIES AND LIMITS
    // =========================================================================
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    // Copy device strings safely
    strncpy(caps->gpu_name, properties.deviceName, sizeof(caps->gpu_name) - 1);
    strncpy(caps->gpu_vendor, _vk_resolve_vendor_name(properties.vendorID), sizeof(caps->gpu_vendor) - 1);

    // Identify GPU architectural type
    caps->gpu_type = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)? gfx_gpu_discrete: gfx_gpu_integrated;

    // Hardware limits mapping
    caps->max_texture_dimension_2d = properties.limits.maxImageDimension2D;
    caps->max_compute_work_group_invocations = properties.limits.maxComputeWorkGroupInvocations;
    caps->min_uniform_buffer_offset_alignment = properties.limits.minUniformBufferOffsetAlignment;
    caps->min_storage_buffer_offset_alignment = properties.limits.minStorageBufferOffsetAlignment;
    caps->max_uniform_buffer_range = properties.limits.maxUniformBufferRange;

    // SPIR-V support is guaranteed for core Vulkan devices
    caps->supported_shader_formats = gfx_shader_format_spirv;

    // =========================================================================
    // 2. FEATURE EXTRACTION VIA pNext CHAIN (Vulkan 1.1+)
    // =========================================================================

    // Allocate structures on the stack and link them into the pNext chain
    VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

    // Vulkan 1.2 features (Bindless/Descriptor Indexing, 8-bit/16-bit storage)
    VkPhysicalDeviceVulkan12Features feats12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features2.pNext = &feats12;

    // Vulkan 1.3 features (Additional core features/atomics)
    VkPhysicalDeviceVulkan13Features feats13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    feats12.pNext = &feats13;

    // Extension: Mesh Shaders (EXT/NV)
    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_feats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    feats13.pNext = &mesh_feats;

    // Extension: Floating-point atomics
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomic_float_feats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };
    mesh_feats.pNext = &atomic_float_feats;

    // Extension: Fragment shader barycentrics
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV barycentric_feats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV };
    atomic_float_feats.pNext = &barycentric_feats;

    // Properties chain to fetch extended limits (like max bindless descriptor array size)
    VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

    // Vulkan 1.2 Extended Properties (contains Descriptor Indexing limits)
    VkPhysicalDeviceVulkan12Properties props12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
    properties2.pNext = &props12;

    // Extension: Conservative Rasterization properties
    VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservative_props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
    props12.pNext = &conservative_props;

    // Query features and extended properties from the driver
    vkGetPhysicalDeviceFeatures2(physical_device, &features2);
    vkGetPhysicalDeviceProperties2(physical_device, &properties2);

    // Vulkan 1.0 core capabilities
    caps->support_compute = true; // Assumes compute queues are validated during device creation
    caps->support_indirect = (bool)features2.features.inheritedQueries; // Basic multi-draw indirect feature

    // Texture compression block support
    caps->support_bc = (bool)features2.features.textureCompressionBC;
    caps->support_astc = (bool)features2.features.textureCompressionASTC_LDR;
    caps->support_etc = (bool)features2.features.textureCompressionETC2;
    caps->support_pvr = false; // PVRTC is legacy/non-standard in native Vulkan core

    // Vulkan 1.2 core math and storage types
    caps->support_shader_float16 = (bool)feats12.shaderFloat16;
    caps->support_shader_int8 = (bool)feats12.shaderInt8;

    // Unbounded bindless validation (Requires full descriptor indexing layout support)
    if (feats12.descriptorIndexing &&
        feats12.descriptorBindingVariableDescriptorCount &&
        feats12.descriptorBindingPartiallyBound)
    {
        caps->support_bindless = true;
        // Extract the actual maximum size for texture arrays bound as bindless resources.
        // Usually matches maxDescriptorSetUpdateAfterBindSampledImages.
        caps->max_bindless_sampleable_textures = props12.maxDescriptorSetUpdateAfterBindSampledImages;
    }
    else
    {
        caps->support_bindless = false;
        caps->max_bindless_sampleable_textures = 0;
    }

    // Subgroup/Wave-level operation support inside Compute stage
    VkPhysicalDeviceSubgroupProperties subgroup_props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
    VkPhysicalDeviceProperties2 sub_props2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    sub_props2.pNext = &subgroup_props;
    vkGetPhysicalDeviceProperties2(physical_device, &sub_props2);

    if (subgroup_props.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) {
        caps->support_shader_subgroup_ops = true;
    }

    // Hardware Ray Tracing validation (Requires separate feature query)
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_feats = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    VkPhysicalDeviceFeatures2 rt_feats2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    rt_feats2.pNext = &rt_feats;
    vkGetPhysicalDeviceFeatures2(physical_device, &rt_feats2);
    caps->support_raytrace = (bool)rt_feats.rayTracingPipeline;

    // Next-Gen Geometry Pipeline features
    caps->support_mesh_shader = (bool)mesh_feats.meshShader;
    caps->support_mesh_amplification_shader = (bool)mesh_feats.taskShader;

    // Extended types atomics and hardware barycentrics
    caps->support_shader_atomic_float32 = (bool)atomic_float_feats.shaderBufferFloat32Atomics;
    caps->support_barycentrics = (bool)barycentric_feats.fragmentShaderBarycentric;

    // Overestimation rasterization verification
    if (conservative_props.fullyCoveredFragmentShaderInputVariable) {
        caps->support_conservative_rasterization = true;
    }

    // =========================================================================
    // 3. OPTIMIZATION HINTS & MEMORY TOPOLOGY (Unified Memory Architecture)
    // =========================================================================
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    // Heuristic for Unified Memory Architecture (UMA):
    // Integrated GPUs naturally share the system memory pool. For dedicated architectures,
    // we inspect if local heaps overlap or present a small budget matching APU layouts.
    caps->has_unified_memory = false;
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        caps->has_unified_memory = true;
    }
    else {
        for (uint32_t i = 0; i < memory_properties.memoryHeapCount; ++i) {
            if ((memory_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) &&
                (memory_properties.memoryHeaps[i].size <= 4ULL * 1024 * 1024 * 1024)) {
                caps->has_unified_memory = true;
            }
        }
    }

    // Engine runtime notifications
    ctx->dbg_log(gfx_msg_info, "Initialized GPU backend: %s (%s)", caps->gpu_name, caps->gpu_vendor);
    ctx->dbg_log(gfx_msg_info, "Capabilities - Bindless: %s (Max Textures: %u), Mesh Shaders: %s, UMA: %s",
        caps->support_bindless ? "ENABLED" : "DISABLED",
        caps->max_bindless_sampleable_textures,
        caps->support_mesh_shader ? "ENABLED" : "DISABLED",
        caps->has_unified_memory ? "TRUE" : "FALSE");
}


static VkInstance _vk_create_instance(bool isdebug)
{
#if defined(_WIN32)
    #define SURFACE_EXTENSION_NAME      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
    #define SURFACE_EXTENSION_NAME      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif
    const char * VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";//VK_EXT_DEBUG_MARKER_EXTENSION_NAME
    
    char* extensions_debug[]  =  { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME    };
    char* extensions_release[] = { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME };

   //char* extensions_debug[] =   { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
   // char* extensions_release[] = { VK_KHR_SURFACE_EXTENSION_NAME, SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

    char**   extension_names = isdebug ? extensions_debug : extensions_release;
    uint32_t extension_count = isdebug ? _countof(extensions_debug) : _countof(extensions_release);

    uint32_t property_layer_count = 0;
    VkLayerProperties* properties = (VkLayerProperties*)calloc(128, sizeof(VkLayerProperties));;
    vkEnumerateInstanceLayerProperties(&property_layer_count, nullptr);
    vkEnumerateInstanceLayerProperties(&property_layer_count, properties);

    const char* validation_layer_name = nullptr;
    for(uint32_t i = 0; i < property_layer_count; ++i)
    {
        if (properties != nullptr && !strcmp(VK_LAYER_KHRONOS_validation, properties[i].layerName)) {
            validation_layer_name = VK_LAYER_KHRONOS_validation;
            break;
        }
    }
    free(properties);

    const char* debug_layer_names[] = { validation_layer_name};
    uint32_t    debug_layer_count   = validation_layer_name ? 1 : 0;

    VkDebugUtilsMessengerCreateInfoEXT debug_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, nullptr };
    if (isdebug) {
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_info.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)vkDebugCallback2;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkInstanceCreateInfo create_info        = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        create_info.pApplicationInfo        = nullptr;
        create_info.enabledExtensionCount   = extension_count;
        create_info.ppEnabledExtensionNames = extension_names;
        create_info.enabledLayerCount       = isdebug ? debug_layer_count : 0;
        create_info.ppEnabledLayerNames     = isdebug ? debug_layer_names : nullptr;
        create_info.pNext                   = isdebug ? &debug_info       : nullptr;
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);

    return instance;
}

static VkSurfaceKHR _vk_create_surface(VkInstance instance, intptr_t handle)
{
    VkSurfaceKHR surface = 0;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr };
        create_info.flags        = 0;
        create_info.hinstance    = GetModuleHandle(NULL);
        create_info.hwnd         = (HWND)handle;
    VkResult result = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkAndroidSurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr };
        create_info.flags       = 0;
        create_info.window      = (struct ANativeWindow*)(handle);
    VkResult result = vkCreateAndroidSurfaceKHR(instance, &create_info, NULL, &surface);
#else
    static_assert(false, "vk_create_surface: not implemented");
#endif
    return surface;
}

static VkPhysicalDevice _vk_create_physical_device(VkInstance instance, VkPhysicalDeviceType preferred_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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

        if (prop.deviceType == preferred_type)
        {
            physicalDevice = devices[i];
        }
    }
    return physicalDevice;
}

static VkDevice _vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface, uint32_t* out_graphics, uint32_t* out_present)
{
    VkBool32 supports_present[16] = { 0 };

    uint32_t queue_properties_count = 0;
    VkQueueFamilyProperties queue_properties[16] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queue_properties_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queue_properties_count, queue_properties);

    uint32_t graphics_queue_index = UINT32_MAX;
    uint32_t present_queue_index  = UINT32_MAX;
    for (uint32_t i = 0; i < queue_properties_count; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physdevice, i, surface, &supports_present[i]);
        bool support_grpaphics  = (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        bool support_compute    = (queue_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)  != 0;
        bool support_transfer   = (queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;

        if (support_grpaphics && graphics_queue_index == UINT32_MAX) {
            graphics_queue_index = i;
        }

        if(support_grpaphics && support_compute && supports_present[i]) {
            graphics_queue_index = i;
            present_queue_index = i;
           break;
        }
    }

    // If didn't find a queue that supports both graphics and present, then find a separate present queue.
    if (present_queue_index == UINT32_MAX)
    {
        for (uint32_t i = 0; i < queue_properties_count; ++i) {
            if (supports_present[i] == VK_TRUE) {
                present_queue_index = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphics_queue_index == UINT32_MAX || present_queue_index == UINT32_MAX) {
        gfx_default_log(gfx_msg_error, "Swapchain Initialization Failure: Could not find both graphics and present queues");
        return VK_NULL_HANDLE;
    }

    *out_graphics = graphics_queue_index;
    *out_present = present_queue_index;

    bool separate_present_queue = (graphics_queue_index != present_queue_index);

    //create device
    float queue_priorities[] = { 0.0 };

    VkDeviceQueueCreateInfo queues[] = {
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, graphics_queue_index, 1, queue_priorities },
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, present_queue_index,  1, queue_priorities }
    };

    const char* device_extension [] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME/*, VK_EXT_DEBUG_MARKER_EXTENSION_NAME */};
    const char* device_validation_layers[] = { "VK_LAYER_LUNARG_mem_tracker", "VK_LAYER_GOOGLE_unique_objects" };

    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(physdevice, &features);

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features =   { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES, NULL};
    VkPhysicalDeviceDescriptorIndexingFeatures bindless_features =          { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, &dynamic_rendering_features };

    VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &bindless_features };
    vkGetPhysicalDeviceFeatures2(physdevice, &deviceFeatures2);

    bool support_bindless = bindless_features.shaderSampledImageArrayNonUniformIndexing &&
                            bindless_features.descriptorBindingSampledImageUpdateAfterBind &&
                            bindless_features.descriptorBindingPartiallyBound &&
                            bindless_features.runtimeDescriptorArray;

    if(support_bindless)
    {
        VkPhysicalDeviceDescriptorIndexingProperties indexing_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES };
        VkPhysicalDeviceProperties2 device_properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &indexing_properties };
        vkGetPhysicalDeviceProperties2(physdevice, &device_properties2);
        uint32_t maxSupportedSampledImages = indexing_properties.maxPerStageDescriptorUpdateAfterBindSampledImages;
    }

    VkDevice device = nullptr;
    VkDeviceCreateInfo create_info      = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    create_info.pNext = support_bindless ? &bindless_features :nullptr;
    create_info.queueCreateInfoCount    = separate_present_queue ? 2 : 1;
    create_info.pQueueCreateInfos       = queues;
    create_info.enabledLayerCount       = _countof(device_validation_layers);
    create_info.ppEnabledLayerNames     = device_validation_layers;
    create_info.enabledExtensionCount   = _countof(device_extension);
    create_info.ppEnabledExtensionNames = device_extension;
    create_info.pEnabledFeatures        = &features;
    VkResult result = vkCreateDevice(physdevice, &create_info, NULL, &device);
    if (result != VK_SUCCESS)
    {
        gfx_default_log(gfx_msg_error, "Vk: Error in vkCreateDevice(%d)", result);
    }

    return device;
}

static VkSemaphore _vk_create_semaphore(VkDevice device, bool istimeline = false)
{
    int num_frames = 2;
    const uint64_t initial_value = (num_frames - 1);

    VkSemaphoreTypeCreateInfo timeline_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
    timeline_create_info.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timeline_create_info.pNext          = nullptr;
    timeline_create_info.semaphoreType  = VK_SEMAPHORE_TYPE_TIMELINE;
    timeline_create_info.initialValue   = initial_value;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo create_info   = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    create_info.pNext                   = istimeline? &timeline_create_info : nullptr;
    vkCreateSemaphore(device, &create_info, NULL, &semaphore);
    return semaphore;
}

static VkDeviceMemory _vk_allocate_and_bind_memory(vk_context_t* ctx, VkMemoryPropertyFlags memflag, uint32_t *size, VkImage image, VkBuffer buffer)
{
   // bool failed = image != nullptr && buffer != nullptr;
    assert(!(image != nullptr && buffer != nullptr)); // only one should be active

    VkMemoryRequirements requirements = {};

    if(image != VK_NULL_HANDLE)
        vkGetImageMemoryRequirements(ctx->vk_device, image, &requirements);

    if(buffer != VK_NULL_HANDLE)
        vkGetBufferMemoryRequirements(ctx->vk_device, buffer, &requirements);

    if(size != nullptr)
        *size = (uint32_t)requirements.size;

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = _vk_find_memory_type(ctx->memory_properties, requirements.memoryTypeBits, memflag);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (auto result = vkAllocateMemory(ctx->vk_device, &alloc_info, nullptr, &memory)) {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to allocate memory!(%s)", err_str);
        return nullptr;
    }

    if (image != VK_NULL_HANDLE)
        vkBindImageMemory(ctx->vk_device, image, memory, 0);

    if (buffer != VK_NULL_HANDLE)
        vkBindBufferMemory(ctx->vk_device, buffer, memory, 0);

    return memory;
}

static void _vk_create_buffer(vk_context_t* ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memflag, vk_buffer_t* buffer)
{
    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size         = size;
    create_info.usage        = usage;
    create_info.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    if (auto result = vkCreateBuffer(ctx->vk_device, &create_info, nullptr, &buffer->buffer))
    {
        auto err_str = string_VkResult(result);
        ctx->dbg_log(gfx_msg_error, "failed to create buffer! (%s)", err_str);
        return;
    }
    uint32_t allocated_size = 0;
    buffer->memory = _vk_allocate_and_bind_memory(ctx, memflag, &allocated_size, nullptr, buffer->buffer);
    buffer->buffer_size = allocated_size;
}

static VkImage _vk_create_image(vk_context_t* ctx, VkImageType type, VkExtent3D extend, uint32_t mips, VkFormat format, VkImageTiling tiling,
                                VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* imageMemory, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
{
    VkImage image = VK_NULL_HANDLE;
    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        create_info.imageType     = type;
        create_info.extent        = extend;
        create_info.mipLevels     = (mips >= 1) ? mips : 1;
        create_info.arrayLayers   = 1; 
        create_info.format        = format;
        create_info.tiling        = tiling;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage         = usage;
        create_info.samples       = samples;
        create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (auto result = vkCreateImage(ctx->vk_device, &create_info, nullptr, &image))
    {
        ctx->dbg_log(gfx_msg_error, "failed to create image!(%s)", string_VkResult(result));
        return nullptr;
    }

    uint32_t allocated_size = 0;
    *imageMemory = _vk_allocate_and_bind_memory(ctx, properties, &allocated_size, image, nullptr);
    
    return image;
}

static VkImageView _vk_create_image_view(vk_context_t* ctx, VkImageViewType type, VkImage image, VkFormat format, uint32_t mipLevels)
{
    VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    create_info.image      = image;
    create_info.viewType   = type;
    create_info.format     = format;
    create_info.subresourceRange.aspectMask        = determine_aspect_mask(format);
    create_info.subresourceRange.baseMipLevel      = 0;
    create_info.subresourceRange.levelCount        = mipLevels;
    create_info.subresourceRange.baseArrayLayer    = 0;
    create_info.subresourceRange.layerCount        = 1;

    VkImageView image_view = VK_NULL_HANDLE;
    if (vkCreateImageView(ctx->vk_device, &create_info, nullptr, &image_view) != VK_SUCCESS) {
        ctx->dbg_log(gfx_msg_error, "failed to create texture image view!");
        return nullptr;
    }
    return image_view;
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

    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(&ctx->handle, &cmd);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    vk_cmd_begin(cmd);

    vkCmdPipelineBarrier(vk_cmd->cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

    vk_cmd_end(cmd);
    vk_cmd_submit(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(&ctx->handle, cmd);
}

static void _vk_copy_buffer_to(vk_context_t* ctx, VkBuffer src, vk_copy_info_t * dst_info)
{
    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(&ctx->handle, &cmd);
    vk_cmd_begin(cmd);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    if (dst_info->dst_buffer != VK_NULL_HANDLE && dst_info->dst_buffer_size > 0)
    {
        VkBufferCopy region = { 0, dst_info->dst_buffer_offset, dst_info->dst_buffer_size };
        vkCmdCopyBuffer(vk_cmd->cmd, src, dst_info->dst_buffer, 1, &region);
    }

    VkBufferImageCopy regions[16] = {}; // maximum 16 mip levels(65536)
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
    vk_cmd_end(cmd);
    vk_cmd_submit(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(&ctx->handle, cmd);
}

static void _vk_copy_buffer_to_image(vk_context_t* ctx, VkBuffer src, VkImage image, uint32_t mips, gfx_pixel_format format, VkExtent3D extend)
{
    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(&ctx->handle, &cmd);
    vk_cmd_begin(cmd);

    VkBufferImageCopy regions[16] = {};
    uint32_t offset = 0;
    for (uint8_t i = 0; i < mips; i++)
    {
        uint32_t width  = max(extend.width  >> i, 1);
        uint32_t height = max(extend.height >> i, 1);
        uint32_t depth  = max(extend.depth  >> i, 1);
        
        regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[i].imageSubresource.baseArrayLayer = 0;
        regions[i].imageSubresource.layerCount = 1;
        regions[i].imageSubresource.mipLevel = i;
        
        regions[i].imageOffset = { 0, 0, 0 };
        regions[i].imageExtent = { width, height, depth };
        regions[i].bufferOffset = offset;

        offset += gfx_utils_image_layer_size(width, height, depth, format);
    }

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdCopyBufferToImage(vk_cmd->cmd, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mips, regions);

    vk_cmd_end(cmd);
    vk_cmd_submit(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(&ctx->handle, cmd);
}

static void _vk_copy_buffer_to_buffer(vk_context_t* ctx, VkBuffer src, VkBuffer dst_buffer, VkDeviceSize dst_buffer_offset, VkDeviceSize dst_buffer_size)
{
    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(&ctx->handle, &cmd);
    vk_cmd_begin(cmd);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    VkBufferCopy region = { };
        region.size = dst_buffer_size;
        region.dstOffset = dst_buffer_offset;
        region.srcOffset = 0;
    vkCmdCopyBuffer(vk_cmd->cmd, src, dst_buffer, 1, &region);

    vk_cmd_end(cmd);
    vk_cmd_submit(&ctx->handle, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(&ctx->handle, cmd);
}

void _destroy_vk_texture(vk_context_t * ctx, vk_texture_t* texture)
{
    if(texture->view)
        vkDestroyImageView(ctx->vk_device, texture->view, nullptr);

    if (texture->image)
        vkDestroyImage(ctx->vk_device, texture->image, nullptr);

    if (texture->memory)
        vkFreeMemory(ctx->vk_device, texture->memory, nullptr);

    texture->view = VK_NULL_HANDLE;
    texture->image = VK_NULL_HANDLE;
    texture->memory = VK_NULL_HANDLE;
}

uint32_t gfx_gpu_ram_usage(gfx_context_t* ctx)
{
    auto vkctx = from_ctx(ctx);
    VkPhysicalDeviceMemoryBudgetPropertiesEXT physical_device_memory_budget_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT };
    VkPhysicalDeviceMemoryProperties2 device_memory_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
    device_memory_properties.pNext = &physical_device_memory_budget_properties;
    vkGetPhysicalDeviceMemoryProperties2(vkctx->vk_physical_device, &device_memory_properties);
    uint32_t device_memory_heap_count = device_memory_properties.memoryProperties.memoryHeapCount;
    uint32_t device_memory_total_usage = 0;
    for (uint32_t i = 0; i < device_memory_heap_count; i++)
    {
        device_memory_total_usage += (uint32_t)physical_device_memory_budget_properties.heapUsage[i];
    }
    return device_memory_total_usage;
}

static void vk_gpu_memstatus(gfx_context_t * ctx)
{
    auto vkctx = from_ctx(ctx);

    VkPhysicalDeviceMemoryBudgetPropertiesEXT physical_device_memory_budget_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT };
    VkPhysicalDeviceMemoryProperties2 device_memory_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
    device_memory_properties.pNext = &physical_device_memory_budget_properties;
    vkGetPhysicalDeviceMemoryProperties2(vkctx->vk_physical_device, &device_memory_properties);
    uint32_t device_memory_heap_count = device_memory_properties.memoryProperties.memoryHeapCount;
    uint32_t device_memory_total_usage = 0;
    uint32_t device_memory_total_budget = 0;
    for (uint32_t i = 0; i < device_memory_heap_count; i++)
    {
        device_memory_total_usage += (uint32_t)physical_device_memory_budget_properties.heapUsage[i];
        device_memory_total_budget += (uint32_t)physical_device_memory_budget_properties.heapBudget[i];
    }/**/
    vkctx->dbg_log(gfx_msg_info, "gpu memory usage: %10d Kb  budget: %10d Mb", device_memory_total_usage/1024, device_memory_total_budget/1024/1024);
}


// --- CONTEXT ---

void vk_create_renderer(gfx_settings_t* cfg, gfx_context_t** out_ctx)
{
    assert(cfg && out_ctx);

    vk_context_t* vctx = (vk_context_t*)calloc(1, sizeof(vk_context_t));
    if (vctx == nullptr)
        return;

    if( cfg->allocator != nullptr )
        vctx->allocator = *cfg->allocator;

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

    vctx->vk_instance         = instance;
    vctx->vk_physical_device   = physdevice;
    vctx->vk_surface          = surface;
    vctx->vk_device           = device;
    vctx->graphics_queue   = { gfamily, gqueue };
    vctx->present_queue    = { pfamily, pqueue };
    vctx->dbg_log          = cfg->dbglog ? cfg->dbglog : gfx_default_log;
    vctx->extensions       = (VkExtensionProperties*)_gfx_alloc(vctx, sizeof(VkExtensionProperties) * 1024);

    vctx->vk_dbg_set_object_name    = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(vctx->vk_device, "vkSetDebugUtilsObjectNameEXT");
    vctx->vk_dbg_cmd_push_label     = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(vctx->vk_device, "vkCmdBeginDebugUtilsLabelEXT");
    vctx->vk_dbg_cmd_pop_label      = (PFN_vkCmdEndDebugUtilsLabelEXT)  vkGetDeviceProcAddr(vctx->vk_device, "vkCmdEndDebugUtilsLabelEXT");

    vkGetPhysicalDeviceFeatures(physdevice, &vctx->device_features);
    vkGetPhysicalDeviceProperties(physdevice, &vctx->device_properties);
    vkGetPhysicalDeviceMemoryProperties(physdevice, &vctx->memory_properties);
    vkEnumerateDeviceExtensionProperties(physdevice, NULL, &vctx->extension_count, NULL);
    vkEnumerateDeviceExtensionProperties(physdevice, NULL, &vctx->extension_count, vctx->extensions);

    vk_fill_device_caps(vctx, vctx->vk_physical_device, &vctx->gpu_caps);

    auto maxUniformBufferRange = vctx->device_properties.limits.maxUniformBufferRange;
    
    gfx_pool_create(sizeof(vk_surface_t),       16, &vctx->surface_pool, &vctx->allocator);
    gfx_pool_create(sizeof(vk_render_target_t), 256, &vctx->render_target_pool, &vctx->allocator);

    gfx_pool_create(sizeof(vk_sampler_t),        128,  &vctx->sampler_pool, &vctx->allocator);
    gfx_pool_create(sizeof(vk_command_buffer_t), 512,  &vctx->cmd_pool,     &vctx->allocator);

    gfx_pool_create(sizeof(vk_texture_t),   cfg->limits.texture_pool_capacity, &vctx->texture_pool,  &vctx->allocator);
    gfx_pool_create(sizeof(vk_shader_t),    cfg->limits.shader_pool_capacity,  &vctx->shaders_pool,  &vctx->allocator);
    gfx_pool_create(sizeof(vk_buffer_t),    cfg->limits.buffer_pool_capacity,   &vctx->buffers_pool,  &vctx->allocator);
    gfx_pool_create(sizeof(vk_pipeline_t),  cfg->limits.pipeline_pool_capacity, &vctx->pipeline_pool, &vctx->allocator);
    gfx_pool_create(sizeof(vk_compute_pipeline_t),  cfg->limits.compute_pipeline_pool_capacity, &vctx->compute_pipeline_pool, &vctx->allocator);


    gfx_buffer_t* uniform_buffer = nullptr;
    gfx_buffer_desc_t ubo_descriptor = {};
        ubo_descriptor.label                = "unifrom_buffer_object";
        ubo_descriptor.mapped               = true;
        ubo_descriptor.size                 = cfg->limits.uniform_buffer_size;
        ubo_descriptor.usage                = gfx_buffer_usage_uniform;
    vk_buffer_create(&vctx->handle, &ubo_descriptor, &uniform_buffer);
    vctx->uniform_buffer = (vk_buffer_t*)gfx_pool_map(vctx->buffers_pool, uniform_buffer->idx);

    //staging buffer
    gfx_buffer_t* staging_buffer = nullptr;
    gfx_buffer_desc_t staging_descriptor    = {};
        staging_descriptor.label            = "staging_buffer";
        staging_descriptor.mapped           = true;
        staging_descriptor.size             = cfg->limits.staging_buffer_size;
        staging_descriptor.usage            = gfx_buffer_usage_staging;
    vk_buffer_create(&vctx->handle, &staging_descriptor, &staging_buffer);
    vctx->staging_buffer = (vk_buffer_t*)gfx_pool_map(vctx->buffers_pool, staging_buffer->idx);

    // default sampler
    gfx_sampler_desc_t sampler_descriptor   = {};
        sampler_descriptor.anisotropy       = 1;
        sampler_descriptor.minmag           = gfx_filter_point;
        sampler_descriptor.mipmap           = gfx_filter_point;
        sampler_descriptor.mode             = gfx_address_mode_repeat;
    vk_sampler_create(&vctx->handle, &sampler_descriptor, &vctx->default_sampler);
    
    // default texture
    extern uint32_t gfx_failover_texture_data[];
    gfx_texture_t* default_texture = nullptr;
    gfx_texture_desc_t  default_texture_desc = { 0 };
        default_texture_desc.label        = "_default_texture";
        default_texture_desc.width        = 16;
        default_texture_desc.height       = 16;
        default_texture_desc.depth        = 1;
        default_texture_desc.format       = gfx_pixel_format_rgba8;
        default_texture_desc.mip_levels   = 5;
        default_texture_desc.array_layers = 1;
        default_texture_desc.data         = &gfx_failover_texture_data[0];
    vk_texture_create(&vctx->handle, &default_texture_desc, &default_texture);
    vctx->default_texture = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, default_texture->idx);

    // default texture storage
    gfx_texture_desc_t default_texture_storage_desc = { 0 };
        default_texture_storage_desc.label    = "_default_texture_storage";
        default_texture_storage_desc.width    = 32;
        default_texture_storage_desc.height   = 32;
        default_texture_storage_desc.depth    = 1;
        default_texture_storage_desc.format   = gfx_pixel_format_rgba8;
        default_texture_storage_desc.usage_flags  = gfx_texture_usage_storage;
    vk_texture_create(&vctx->handle, &default_texture_storage_desc, &vctx->default_storage_texture);

    // bindless texture pool and set
    if(vctx->bindless_max_texture_count > 0)
    {
        // 1. Define the binding for the massive texture array
        VkDescriptorSetLayoutBinding binding = {0};
        binding.binding             = 0;
        binding.descriptorType      = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding.descriptorCount     = vctx->bindless_max_texture_count; // Must match the maximum array size
        binding.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // 2. Set specific bindless flags for this binding
        VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                                  VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                                  VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT; // The array can be sized dynamically on allocation
        VkDescriptorSetLayoutBindingFlagsCreateInfo extended_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        extended_info.bindingCount  = 1;
        extended_info.pBindingFlags = &bindless_flags;

        // 3. Create the layout
        VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.pNext           = &extended_info; // Pass flags via pNext chain
        layout_info.flags           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;// CRITICAL: Layout must be compatible with the update-after-bind pool
        layout_info.bindingCount    = 1;
        layout_info.pBindings       = &binding;
        vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &vctx->bindless_descriptor_set_layout);

        VkDescriptorPoolSize bindless_pool_size = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, vctx->bindless_max_texture_count };
        VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags            = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;  // CRITICAL: Allows updating descriptors while command buffers are recording/pending execution
        pool_info.maxSets          = 1; // We only need a single global descriptor set
        pool_info.poolSizeCount    = 1;
        pool_info.pPoolSizes       = &bindless_pool_size;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &vctx->bindless_descriptor_pool);

        VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        variable_count_info.descriptorSetCount    = 1;
        variable_count_info.pDescriptorCounts     = &vctx->bindless_max_texture_count; // Set dynamic array bounds

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType             = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext             = &variable_count_info; // Pass variable count info via pNext
        allocInfo.descriptorPool    = vctx->bindless_descriptor_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts       = &vctx->bindless_descriptor_set_layout;

        vkAllocateDescriptorSets(device, &allocInfo, &vctx->bindless_descriptor_set);
    }
    
    *out_ctx = &vctx->handle;
}

void vk_get_caps(gfx_context_t* _ctx, gfx_caps_t *caps)
{
    vk_context_t * ctx = from_ctx(_ctx);
    *caps = ctx->gpu_caps;
}

void vk_destroy_renderer(gfx_context_t * ctx)
{
    assert(ctx);
    vk_context_t* vctx = (vk_context_t*)ctx;
    
    vk_sampler_destroy(ctx, vctx->default_sampler);
    vk_texture_destroy(ctx, &vctx->default_texture->handle);
    _gfx_free(vctx, vctx->extensions);

    vkDestroyDevice(vctx->vk_device, nullptr);
    vkDestroySurfaceKHR(vctx->vk_instance, vctx->vk_surface, nullptr);
    vkDestroyInstance(vctx->vk_instance, nullptr);

    free(vctx);
}

static vk_texture_t _vk_create_target_texture(vk_context_t* ctx, VkExtent3D extend, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples)
{
    vk_texture_t result = {};

    result.image = _vk_create_image(ctx, VK_IMAGE_TYPE_2D, extend, 1, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                    &result.memory, 
                                    samples);

    result.view = _vk_create_image_view(ctx, VK_IMAGE_VIEW_TYPE_2D, result.image, format, 1);

    return result;
}

void _vk_reset_surface(vk_context_t* ctx, vk_surface_t* surface)
{
    uint32_t format_count = 0;
    VkSurfaceFormatKHR formats[32] = {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, surface->surface, &format_count, NULL);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, surface->surface, &format_count, formats);

    uint32_t present_mode_count = 0;
    VkPresentModeKHR present_modes[8] = {};
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_physical_device, surface->surface, &present_mode_count, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->vk_physical_device, surface->surface, &present_mode_count, present_modes);

    VkSurfaceCapabilitiesKHR capabilities = { 0 };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_physical_device, surface->surface, &capabilities);

    uint32_t image_count = GFX_MAX_FRAME_IN_FLIGHT;
    surface->width = capabilities.currentExtent.width;
    surface->height = capabilities.currentExtent.height;

    VkSwapchainKHR prev_swapchain = surface->swapchain;
    VkSurfaceFormatKHR surface_format = formats[0];
    VkSwapchainCreateInfoKHR info   = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        info.surface                = surface->surface;
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
        info.presentMode            = surface->vsync? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        info.presentMode            = VK_PRESENT_MODE_MAILBOX_KHR;
        info.clipped                = VK_TRUE;
        info.oldSwapchain           = prev_swapchain;
    VkResult result = vkCreateSwapchainKHR(ctx->vk_device, &info, NULL, &surface->swapchain);

    // destroy old swapchain and attachment if resized
    if (prev_swapchain != VK_NULL_HANDLE){
        vkDestroySwapchainKHR(ctx->vk_device, prev_swapchain, nullptr);
        for(uint32_t i = 0; i < image_count; ++i) {
            
            vk_render_target_t * target = surface->targets[i];
            _destroy_vk_texture(ctx, &target->depth_attachments);
            _destroy_vk_texture(ctx, &target->resolve_attachments);

            vkDestroyFramebuffer(ctx->vk_device, target->framebuffer, nullptr);

            for(uint32_t j = 0; j < target->color_attachment_count; ++j)
                _destroy_vk_texture(ctx, &target->color_attachments[j]);
         }
    }

    //depth attachment
    VkExtent3D extend = { surface->width, surface->height, 1 };
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    auto depth_target = _vk_create_target_texture(ctx, extend, surface->depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, samples);

    // render pass
    if (surface->render_pass == VK_NULL_HANDLE)
        _vk_create_renderpass(ctx, surface_format.format, surface->depth_format, VK_SAMPLE_COUNT_1_BIT, &surface->render_pass);

    // color attachments
    VkImage images[8] = {};
    vkGetSwapchainImagesKHR(ctx->vk_device, surface->swapchain, &image_count, NULL);
    vkGetSwapchainImagesKHR(ctx->vk_device, surface->swapchain, &image_count, images);
    for (uint32_t i = 0; i < image_count; ++i)
    {
        vk_render_target_t* target = surface->targets[i];

        VkImageView color_view = _vk_create_image_view(ctx, VK_IMAGE_VIEW_TYPE_2D, images[i], surface_format.format, 1);

        VkImageView  attachments [] = { 
            color_view, 
            depth_target.view
        };

        VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        info.renderPass         = surface->render_pass;
        info.attachmentCount    = _countof(attachments);
        info.pAttachments       = attachments;
        info.width              = extend.width;
        info.height             = extend.height;
        info.layers             = 1;
        result = vkCreateFramebuffer(ctx->vk_device, &info, nullptr, &target->framebuffer);

        target->renderpass = surface->render_pass;
        target->extent = { surface->width, surface->height };
        target->depth_format = surface->depth_format;
        target->color_format = surface_format.format;

        target->color_attachment_count = 1;
        target->color_attachments[0].view = color_view;
        target->color_attachments[0].format = surface_format.format;
    }
    //surface->target.depth_attachments = depth_target;
}

void vk_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** out_surface)
{
    vk_context_t * vctx = from_ctx(ctx);

    uint64_t handle = gfx_pool_alloc(vctx->surface_pool);
    vk_surface_t * surface = (vk_surface_t*)gfx_pool_map(vctx->surface_pool, handle);

    // create semaphores and fences
    for (int i = 0; i < GFX_MAX_FRAME_IN_FLIGHT; ++i)
    {
        VkFenceCreateInfo fence_ci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
        vkCreateFence(vctx->vk_device, &fence_ci, nullptr, &surface->fences[i]);

        surface->semaphore_image_available[i] = _vk_create_semaphore(vctx->vk_device);
        surface->semaphore_rendering_finished[i] = _vk_create_semaphore(vctx->vk_device);
    }
   // vkResetFences(vctx->device, MAX_FRAME_IN_FLIGHT, surface->fences);

    VkFormat depth_formats[] = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT };

    surface->window_handle  = desc->window_handle;
    surface->vsync          = desc->vsync;
    surface->format         = desc->preferred_format;
    surface->sample_count   = desc->sample_count;
    surface->depth_format   = _vk_find_supported_format(vctx, depth_formats, _countof(depth_formats), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    surface->surface        = _vk_create_surface(vctx->vk_instance, desc->window_handle);
    for(int i = 0; i < GFX_MAX_FRAME_IN_FLIGHT; ++i) {

        uint64_t target_handle = 0;
        surface->targets[i] = (vk_render_target_t*)gfx_pool_alloc_data(vctx->render_target_pool, &target_handle);
        surface->targets[i]->handle = { target_handle };
    }
    _vk_reset_surface(vctx, surface);

    vctx->vk_default_renderpass = surface->render_pass;
    
    surface->handle = { handle };
    *out_surface = &surface->handle;
}

void vk_surface_destroy(gfx_context_t* ctx, gfx_surface_t* _surface)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_surface_t* surface = (vk_surface_t*)gfx_pool_map(vctx->surface_pool, _surface->idx);

    for (size_t i = 0; i < _countof(surface->targets); i++) {
        if (surface->targets[i]->framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(vctx->vk_device, surface->targets[i]->framebuffer, nullptr);
        surface->targets[i] = VK_NULL_HANDLE;

        for (size_t j = 0; j < _countof(surface->targets[j]->color_attachments); j++) {
            _destroy_vk_texture(vctx, &surface->targets[i]->color_attachments[i]);
        }
        _destroy_vk_texture(vctx, &surface->targets[i]->depth_attachments);
    }

    if (surface->swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(vctx->vk_device, surface->swapchain, nullptr);
    surface->swapchain = VK_NULL_HANDLE;

    gfx_pool_free(vctx->surface_pool, _surface->idx);
    _surface->idx = 0;
}


void vk_frame_begin(gfx_context_t* ctx, gfx_surface_t* in_surface, gfx_frame_t** out_frame)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_surface_t* surface = (vk_surface_t*)gfx_pool_map(vctx->surface_pool, in_surface->idx);

    uint32_t frame_idx = surface->current_frame;

    VkFence fence = surface->fences[frame_idx];
    vkWaitForFences(vctx->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vctx->vk_device, 1, &fence);

    VkSemaphore image_available_sem = surface->semaphore_image_available[frame_idx];

    uint32_t img_idx = 0;
    while (auto result = vkAcquireNextImageKHR(vctx->vk_device, surface->swapchain, UINT64_MAX, image_available_sem, VK_NULL_HANDLE, &img_idx))
    {
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            _vk_reset_surface(vctx, surface);
            continue;
        }
        else {
            vctx->dbg_log(gfx_msg_error, "vk_acquire_img failed with %s error", string_VkResult(result));
            return;
        }
    }

    gfx_frame_t* frame = &surface->frames[frame_idx];
    *out_frame = frame;

    frame->ctx = ctx;
    frame->surface = in_surface;
    frame->frame_index = frame_idx;    
    frame->swapchain_image_index = img_idx;      
    frame->target = &surface->targets[img_idx]->handle; 

    surface->swapchain_image_index = img_idx;

    if(frame->cmd == nullptr)
        vk_cmd_create(ctx, &frame->cmd);
    vk_cmd_begin(frame->cmd);
}

void vk_frame_end(gfx_frame_t* frame)
{
    vk_context_t* vctx = from_ctx(frame->ctx);
    vk_surface_t* surface = (vk_surface_t*)gfx_pool_map(vctx->surface_pool, frame->surface->idx);
    vk_command_buffer_t* vcmd = (vk_command_buffer_t*)gfx_pool_map(vctx->cmd_pool, frame->cmd->idx);

    if (auto result = vkEndCommandBuffer(vcmd->cmd)) {
        vctx->dbg_log(gfx_msg_error, "vk_frame_end : vkEndCommandBuffer failed!(%s)", string_VkResult(result));
    }

    VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    // 
    uint32_t frame_idx = frame->frame_index;

    VkFence fence = surface->fences[frame_idx];
    VkSemaphore image_available_semaphore = surface->semaphore_image_available[frame_idx];
    VkSemaphore rendering_finished_semaphore = surface->semaphore_rendering_finished[frame_idx];

    VkSubmitInfo submit             = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.commandBufferCount   = 1;
        submit.pCommandBuffers      = &vcmd->cmd;
        submit.pWaitDstStageMask    = stages;
        submit.waitSemaphoreCount   = 1;
        submit.pWaitSemaphores      = &image_available_semaphore;   // 
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &rendering_finished_semaphore; // 
    if (auto result = vkQueueSubmit(vctx->graphics_queue.queue, 1, &submit, fence)) {
        vctx->dbg_log(gfx_msg_error, "vkQueueSubmit failed!(%s)", string_VkResult(result));
    }

    // 
    VkPresentInfoKHR present_info       = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &rendering_finished_semaphore;
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &surface->swapchain;
        present_info.pImageIndices      = &frame->swapchain_image_index;
    if (auto result = vkQueuePresentKHR(vctx->present_queue.queue, &present_info)) {
        vctx->dbg_log(gfx_msg_error, "vkQueuePresentKHR failed!(%s)", string_VkResult(result));
    }

    // 
    if (vcmd->stamp_count > 0)
    {
        uint64_t timestamps[256] = {};
        VkQueryPool qpool = vcmd->time_query_pool;
        uint32_t qindex   = vcmd->time_query_current_index;
        vkGetQueryPoolResults(vctx->vk_device, qpool, 0, qindex, _countof(timestamps) * sizeof(uint64_t), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        VkPhysicalDeviceLimits device_limits = vctx->device_properties.limits;
        float delta_in_ms = float(timestamps[1] - timestamps[0]) * device_limits.timestampPeriod / 1000000.0f;

        vcmd->stamp_count = 0;
        vcmd->time_query_stack_top = 0;
        vcmd->time_query_current_index = 0;
        vctx->dbg_log(gfx_msg_info, "gpu time %.3f ", delta_in_ms);
    }

    surface->current_frame = (surface->current_frame + 1) % GFX_MAX_FRAME_IN_FLIGHT;
}


// --- SWAPCHAIN ---

void _vk_create_renderpass(vk_context_t* vctx, VkFormat colorformat, VkFormat depthformat, VkSampleCountFlagBits samples, VkRenderPass* renderpass)
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format           = colorformat;
    color_attachment.samples          = samples;
    color_attachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format           = depthformat;
    depth_attachment.samples          = samples;
    depth_attachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolve_attachment ={};
    resolve_attachment.format           = colorformat;
    resolve_attachment.samples          = VK_SAMPLE_COUNT_1_BIT;
    resolve_attachment.loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve_attachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    resolve_attachment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve_attachment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolve_attachment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    resolve_attachment.finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference attachment_refs[] = {
        {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
   //     {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    VkAttachmentDescription attachment_descs[] = {
        color_attachment,
        depth_attachment,
     //   resolve_attachment
    };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &attachment_refs[0];
    subpass.pDepthStencilAttachment = &attachment_refs[1];
//    subpass.pResolveAttachments     = &attachment_refs[2];

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.pAttachments     = attachment_descs;
    renderPassInfo.attachmentCount  = _countof(attachment_descs);
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;

    if (auto result = vkCreateRenderPass(vctx->vk_device, &renderPassInfo, nullptr, renderpass))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create render pass!");
    }
}


// --- BUFFER ---
void vk_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** out_buffer)
{
    vk_context_t * vctx = from_ctx(ctx);

    bool mapped = desc->mapped;

    VkBufferUsageFlags usage_flag = 0;
    VkMemoryPropertyFlags memory_flag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    switch(desc->usage)
    {
        case gfx_buffer_usage_staging:
            mapped = true;
            usage_flag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            memory_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case gfx_buffer_usage_uniform: 
            mapped = true;
            usage_flag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            memory_flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
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
            usage_flag = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
    }

    uint64_t handle = 0;
    vk_buffer_t* buffer = (vk_buffer_t*)gfx_pool_alloc_data(vctx->buffers_pool, &handle);
    buffer->handle = { handle };

    if(!buffer)
    {
        vctx->dbg_log(gfx_msg_error, "failed to allocate vk_buffer_t %s", desc->label ? desc->label : "");
        return;
    }

    buffer->is_mapped = mapped;

    _vk_create_buffer(vctx, desc->size, usage_flag, memory_flag, buffer);
    vk_buffer_update_data(ctx, &buffer->handle, desc->data, desc->size, 0);

    *out_buffer = &buffer->handle;
    vk_gpu_memstatus(ctx);
}


void vk_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset)
{
    auto vkctx = (vk_context_t*)(ctx);
    auto vkbuf = (vk_buffer_t*)gfx_pool_map(vkctx->buffers_pool, buffer->idx);
    //assert(data != nullptr);
    //assert(size == 0);

    if(vkbuf->is_mapped)
    {
        if(vkbuf->data_ptr == nullptr)
            vkMapMemory(vkctx->vk_device, vkbuf->memory, offset, size, 0, &vkbuf->data_ptr);

        if(size > 0 && data != nullptr)
            memcpy(vkbuf->data_ptr, (char*)data + offset, size);
    }
    else if(data != nullptr)
    {
        auto staging = vkctx->staging_buffer;
        if (size < staging->buffer_size )
        {
            if (staging->data_ptr == nullptr)
                vkMapMemory(vkctx->vk_device, staging->memory, 0, staging->buffer_size, 0, &staging->data_ptr);
            memcpy(staging->data_ptr, data, size);
            // vkUnmapMemory(ctx->device, s_staging.memory);
             //vkFlushMappedMemoryRanges();

            vk_copy_info_t info = { };
                info.dst_buffer = vkbuf->buffer;
                info.dst_buffer_size = (uint32_t)size;
                info.dst_buffer_offset = offset;
            _vk_copy_buffer_to(vkctx, vkctx->staging_buffer->buffer, &info);
        }
        else
        {
            vk_buffer_t staging = {};
            auto memflag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            _vk_create_buffer(vkctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memflag, &staging);

            if (data)
            {
                vkMapMemory(vkctx->vk_device, staging.memory, 0, size, 0, &staging.data_ptr);
                memcpy(staging.data_ptr, data, size);
                vkUnmapMemory(vkctx->vk_device, staging.memory);
                staging.data_ptr = nullptr;
            }

            vk_copy_info_t info = { };
            info.dst_buffer = vkbuf->buffer;
            info.dst_buffer_size = (uint32_t)size;
            _vk_copy_buffer_to(vkctx, staging.buffer, &info);

            vkDestroyBuffer(vkctx->vk_device, staging.buffer, nullptr);
            vkFreeMemory(vkctx->vk_device, staging.memory, nullptr);
        }
    }
}


void vk_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_buffer_t* vkbuffer = (vk_buffer_t*)gfx_pool_map(vctx->buffers_pool, buffer->idx);
    if(vkbuffer)
    {
        if (vkbuffer->is_mapped && vkbuffer->buffer != nullptr)
            vkUnmapMemory(vctx->vk_device, vkbuffer->memory);

        if (vkbuffer->buffer != nullptr)
            vkDestroyBuffer(vctx->vk_device, vkbuffer->buffer, nullptr);

        if (vkbuffer->memory != nullptr)
            vkFreeMemory(vctx->vk_device, vkbuffer->memory, nullptr);

        gfx_pool_free(vctx->buffers_pool, vkbuffer->handle.idx);
    }
}



// --- SHADER ---

void vk_create_descriptor_pool(vk_context_t* ctx, vk_shader_t* shader, uint32_t set_idx, uint32_t capacity, vk_descriptor_pool_t** out_pool)
{
    vk_descriptor_pool_t* pool = (vk_descriptor_pool_t*)_gfx_alloc(ctx, sizeof(vk_descriptor_pool_t));
    vk_descriptor_set_t*  sets = (vk_descriptor_set_t*)_gfx_alloc(ctx, sizeof(vk_descriptor_set_t) * capacity);

    if (pool == nullptr || sets == nullptr) {
        _gfx_free(ctx, pool);
        _gfx_free(ctx, sets);
        return;
    }

    uint32_t binding_count = shader->set_binding_count[set_idx];
    VkDescriptorSetLayoutBinding* set_layout_bindings = shader->set_bindings[set_idx];

    const uint32_t VK_DESCRIPTOR_TYPE_RANGE_SIZE = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    VkDescriptorPoolSize pool_sizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};
    VkDescriptorPoolSize pool_sizes_by_type[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {};

    for (int i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
        pool_sizes_by_type[i].type = (VkDescriptorType)i;

    for (size_t i = 0; i < binding_count; ++i)
        pool_sizes_by_type[set_layout_bindings[i].descriptorType].descriptorCount++;

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
    auto result = vkCreateDescriptorPool(ctx->vk_device, &poolCreateInfo, NULL, &pool->pool);

    uint32_t ubo_buffer_size = 256;

    uint32_t alignment = (uint32_t)ctx->device_properties.limits.minUniformBufferOffsetAlignment;
    uint32_t aligned_size = gfx_utils_align_up(ubo_buffer_size, alignment);
    ubo_buffer_size = aligned_size * capacity;

    if (ubo_buffer_size > 0)
    {
        gfx_buffer_t* buffer = nullptr;
        gfx_buffer_desc_t buff_desc = {};
            buff_desc.label = "ubo";
            buff_desc.usage = gfx_buffer_usage_uniform;
            buff_desc.size  = ubo_buffer_size;
        vk_buffer_create(&ctx->handle, &buff_desc, &buffer);

        pool->ubo_buffer = (vk_buffer_t*)gfx_pool_map(ctx->buffers_pool, buffer->idx);
    }

    pool->capacity = capacity;
    pool->free_set_count = capacity;
    pool->descriptor_sets = sets;

    for (uint32_t i = 0; i < capacity; i++)
    {
        VkWriteDescriptorSet    descriptor_writes[GFX_MAX_DESCRIPTOR_BINDINGS] = { };
        VkDescriptorImageInfo   descriptor_image_info[GFX_MAX_DESCRIPTOR_BINDINGS] = { };
        VkDescriptorBufferInfo  descriptor_buffer_info[GFX_MAX_DESCRIPTOR_BINDINGS] = { };

        vk_descriptor_set_t* current_set = &sets[i];

        VkDescriptorSetAllocateInfo allocate_info   = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocate_info.descriptorPool            = pool->pool;
            allocate_info.descriptorSetCount        = 1;
            allocate_info.pSetLayouts               = &shader->set_layouts[set_idx];
        if (auto result = vkAllocateDescriptorSets(ctx->vk_device, &allocate_info, &current_set->descriptor_set))
        {
            ctx->dbg_log(gfx_msg_error, "failed to create descriptor pool (%s)", string_VkResult(result));
        }

        vk_buffer_t * ubo_buffer = pool->ubo_buffer;

        current_set->is_free     = true;
        current_set->shader      = shader;
        current_set->pool        = pool;

        if(ubo_buffer_size > 0 && pool->ubo_buffer != nullptr)
            current_set->ubo_mapped_data = (uint8_t*)pool->ubo_buffer->data_ptr + aligned_size * i;

        for(uint32_t j = 0;  j < binding_count; ++j)
        {
            auto& binding = set_layout_bindings[j];

            uint32_t writes_idx = binding.binding;

            if (writes_idx >= GFX_MAX_DESCRIPTOR_BINDINGS) {
                ctx->dbg_log(gfx_msg_error, "binding index %d exceeds GFX_MAX_DESCRIPTOR_BINDINGS", writes_idx);
                assert(false);
                continue;
            }

            descriptor_writes[writes_idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[writes_idx].dstSet = sets[i].descriptor_set;
            descriptor_writes[writes_idx].dstBinding = binding.binding;
            descriptor_writes[writes_idx].descriptorType = binding.descriptorType;
            descriptor_writes[writes_idx].descriptorCount = binding.descriptorCount;

            switch (binding.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    descriptor_buffer_info[writes_idx].buffer = ubo_buffer ? ubo_buffer->buffer : nullptr;
                    descriptor_buffer_info[writes_idx].range  = aligned_size;
                    descriptor_buffer_info[writes_idx].offset = aligned_size * i;

                    descriptor_writes[writes_idx].pBufferInfo = &descriptor_buffer_info[writes_idx];
                    break;

                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    descriptor_image_info[writes_idx].sampler = ((vk_sampler_t*)ctx->default_sampler)->sampler;
                    descriptor_image_info[writes_idx].imageView = ((vk_texture_t*)ctx->default_texture)->view;
                    descriptor_image_info[writes_idx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    descriptor_writes[writes_idx].pImageInfo = &descriptor_image_info[writes_idx];
                    break;

                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    // todo: 
                    //sets[i].write_infos[j].buffer_info.buffer = ;
                    assert(false);
                    break;

                default: assert(false); break;
            }
        }
        
        VkWriteDescriptorSet valid_writes[16];
        uint32_t valid_write_count = 0;

        for (uint32_t b = 0; b < 16; ++b)
        {
            if (descriptor_writes[b].sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
            {
                valid_writes[valid_write_count] = descriptor_writes[b];
                valid_write_count++;
            }
        }
        vkUpdateDescriptorSets(ctx->vk_device, valid_write_count, valid_writes, 0, NULL);
    }
    *out_pool = pool;
}




static bool _vk_init_shader_stages(vk_context_t* vctx, gfx_shader_desc_t* desc, vk_shader_t* shader)
{
    for (uint32_t i = 0; i < desc->stage_count; ++i) {
        VkPipelineShaderStageCreateInfo* stage = &shader->stages[i];

        if (desc->stages[i].data && *(uint32_t*)desc->stages[i].data != SpvMagicNumber)
        {
            vctx->dbg_log(gfx_msg_error, "error in shader blob(SpvMagicNumber incorrect)");
            return false;
        }

        // find execution model and entry point in bytecode
        const uint32_t* ptr = (uint32_t*)desc->stages[i].data + 5; // skip header(MagicNumber, Version, Revision, OpCodeMask, WordCountShift)
        const uint32_t* end = (uint32_t*)desc->stages[i].data + desc->stages[i].size;
        for (ptr; ptr < end; ptr += (*ptr >> SpvWordCountShift) & SpvOpCodeMask)
        {
            if ((*ptr & SpvOpCodeMask) != SpvOpEntryPoint) continue;

            stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage->stage = VkShaderStageFlagBits(1 << *(SpvExecutionModel*)(ptr + 1));
            stage->pName = _strdup((const char*)(ptr + 3)); // todo: memleak here
            break;
        }

        if(stage->pName == nullptr) {
            vctx->dbg_log(gfx_msg_error, "no entry point in shader module for stage %d", i);
            return false;
         }

        VkShaderModuleCreateInfo shader_module_ci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            shader_module_ci.codeSize   = desc->stages[i].size;
            shader_module_ci.pCode      = (const uint32_t*)desc->stages[i].data;
        if (vkCreateShaderModule(vctx->vk_device, &shader_module_ci, NULL, &stage->module) != VK_SUCCESS) {
            vctx->dbg_log(gfx_msg_error, "failed to create shader module for stage %d", i);
            return false;
        }

        uint32_t stage_hash = gfx_utils_hash_32((const char*)desc->stages[i].data, desc->stages[i].size);
        shader->hash32 = gfx_utils_hash_combine(shader->hash32, stage_hash);
    }
    return true;
}


static void _vk_create_descriptor_layouts(vk_context_t * ctx, vk_shader_t* shader)
{
    if (shader->uniform_count == 0) {
        shader->set_count = 0;
        return;
    }

    uint32_t max_set = 0;
    for (uint32_t i = 0; i < shader->uniform_count; ++i) {
        if (shader->uniforms[i].group > max_set)
            max_set = shader->uniforms[i].group;
    }
    shader->set_count = max_set + 1;

    // sort bindings by set's
    VkDescriptorSetLayoutBinding* bindings_per_set[8] = { 0 };
  //  uint32_t binding_count[8] = { 0 };
    uint32_t bindings_per_set_idx[8] = { 0 };

    uint32_t * binding_count = shader->set_binding_count;

    for (uint32_t i = 0; i < shader->uniform_count; ++i) {
        gfx_uniform_t* u = &shader->uniforms[i];
        binding_count[u->group]++;
    }

    // allocate VkDescriptorSetLayoutBinding per set
    for (uint32_t s = 0; s < shader->set_count; ++s) {
        if (binding_count[s] == 0) continue;
        bindings_per_set[s] = (VkDescriptorSetLayoutBinding*)_gfx_alloc(ctx, sizeof(VkDescriptorSetLayoutBinding) * binding_count[s]);
        shader->set_bindings[s] = bindings_per_set[s];
    }

    for (uint32_t i = 0; i < shader->uniform_count; ++i) {
        gfx_uniform_t* u = &shader->uniforms[i];

        uint32_t s       = u->group;
        uint32_t sidx    = bindings_per_set_idx[s];

        VkShaderStageFlags stage_flags = 0;
        #define  HAS_FLAG(v, f) ((v & f) == f)

        stage_flags |= HAS_FLAG(u->stage_mask, 1 << gfx_shader_vertex)   ? VK_SHADER_STAGE_VERTEX_BIT   : 0;
        stage_flags |= HAS_FLAG(u->stage_mask, 1 << gfx_shader_fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        stage_flags |= HAS_FLAG(u->stage_mask, 1 << gfx_shader_compute)  ? VK_SHADER_STAGE_COMPUTE_BIT  : 0;

        bindings_per_set[s][sidx].stageFlags = stage_flags;
        bindings_per_set[s][sidx].binding = u->binding;
        bindings_per_set[s][sidx].descriptorCount = 1;

        VkDescriptorType texture_type = HAS_FLAG(u->stage_mask, 1 << gfx_shader_compute) ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                                                                                         : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        switch (u->type) {
            case gfx_uniform_ubo:       bindings_per_set[s][sidx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;   break;
            case gfx_uniform_sampler:   bindings_per_set[s][sidx].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;          break;
            case gfx_uniform_storage_buffer:   bindings_per_set[s][sidx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;   break;

            case gfx_uniform_texture2d:         bindings_per_set[s][sidx].descriptorType = texture_type;    break;
            case gfx_uniform_texture3d:         bindings_per_set[s][sidx].descriptorType = texture_type;    break;
            case gfx_uniform_texture2d_cube:    bindings_per_set[s][sidx].descriptorType = texture_type;    break;
            case gfx_uniform_texture2d_array:   bindings_per_set[s][sidx].descriptorType = texture_type;    break;
        };

        bindings_per_set_idx[s]++;
    }

    for (uint32_t s = 0; s < shader->set_count; ++s) {
        if (binding_count[s] == 0) continue;

        VkDescriptorSetLayoutCreateInfo ci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            ci.bindingCount = binding_count[s];
            ci.pBindings = bindings_per_set[s];
        vkCreateDescriptorSetLayout(shader->ctx->vk_device, &ci, NULL, &shader->set_layouts[s]);
    }

    // Pipeline Layout
    VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        plci.setLayoutCount = shader->set_count;
        plci.pSetLayouts    = shader->set_layouts;
    vkCreatePipelineLayout(shader->ctx->vk_device, &plci, NULL, &shader->pipeline_layout);
}



void vk_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** out_shader)
{
    vk_context_t* vctx = from_ctx(ctx);

    if(desc->stage_count == 0)
    {
        *out_shader = nullptr;
        vctx->dbg_log(gfx_msg_error, "error in shader (stages_count == 0)");
        return;
    }

    uint64_t handle = 0;
    vk_shader_t* shader = (vk_shader_t*)gfx_pool_alloc_data(vctx->shaders_pool, &handle);
    if (shader == NULL) {
        vctx->dbg_log(gfx_msg_error, "failed to allocate shader %s", desc->label);
        return;
    }
    shader->handle          = { handle };
    shader->stage_count    = desc->stage_count;
    shader->label           = _strdup(desc->label ? desc->label : "shader");
    shader->ctx             = vctx;

    shader->uniform_count   = desc->uniform_count;
    shader->uniforms        = (gfx_uniform_t*)_gfx_alloc(vctx, desc->uniform_count * sizeof(gfx_uniform_t));
    memcpy(shader->uniforms, desc->uniforms, sizeof(gfx_uniform_t) * desc->uniform_count);

    _vk_init_shader_stages(vctx, desc, shader);
    _vk_create_descriptor_layouts(vctx, shader);

    for(uint32_t i = 0; i < gfx_shader_get_descriptor_set_count(&shader->handle); ++i) {
        gfx_descriptor_set_t * default_set = nullptr;
        vk_descriptor_set_create(ctx, &shader->handle, i, &default_set);
        shader->default_sets[i] = (vk_descriptor_set_t*)default_set;
    }
    *out_shader = &shader->handle;
}

static gfx_uniform_loc_t make_uniform_loc(uint32_t shader_hash, gfx_uniform_type type, uint8_t set_idx,uint8_t binding_idx, uint8_t field_idx = 0)
{
    gfx_uniform_loc_t location  = {0};
        location.shader_hash    = shader_hash;
        location.type           = (uint8_t)type;
        location.set_index      = set_idx;
        location.binding        = binding_idx;
        location.member_idx     = field_idx;
    return location;
}

uint32_t vk_shader_get_descriptor_set_count(gfx_shader_t* _shader)
{
    vk_shader_t* shader = (vk_shader_t*)_shader;
    return shader->set_count;
}

uint64_t vk_uniform_location(gfx_shader_t* shader, const char* name)
{
    if(shader == nullptr || name == nullptr)
        return 0;

    vk_shader_t * vkshader = (vk_shader_t*)shader;
    uint32_t hash = vkshader->hash32;

    for (uint32_t uniform_id = 0; uniform_id < vkshader->uniform_count; ++uniform_id)
    {
        gfx_uniform_t* uniform = &vkshader->uniforms[uniform_id];

        if(uniform->type == gfx_uniform_ubo)
        {
            for (int16_t field_id = 0; field_id < uniform->buffer.field_count; ++field_id)
            {
                if (strcmp(name, uniform->buffer.fields[field_id].name))
                    continue;

                 return make_uniform_loc(hash, uniform->type, (uint8_t)uniform->group, (uint8_t)uniform_id, (uint8_t)field_id).handle;
            }
        }

        if (strcmp(name, uniform->name))
            continue;

        return  make_uniform_loc(hash, uniform->type, uniform->group, uniform_id, 0).handle;
    }
    return 0;
}


void vk_shader_destroy(gfx_context_t* ctx, gfx_shader_t* _shader)
{
    vk_context_t* vkctx = from_ctx(ctx);

    vk_shader_t * shader = (vk_shader_t*)gfx_pool_map(vkctx->shaders_pool, _shader->idx);
   // shader->pool

    //destroy all pools and pool datas
}



// --- SAMPLER ---

void vk_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** out_sampler)
{
    assert(ctx && desc && out_sampler);
    *out_sampler = nullptr;

    vk_context_t* vctx = (vk_context_t*)ctx;

    uint64_t handle = gfx_pool_alloc(vctx->sampler_pool);
    vk_sampler_t* sampler = (vk_sampler_t*)gfx_pool_map(vctx->sampler_pool, handle);
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
    if (auto result = vkCreateSampler(vctx->vk_device, &create_info, NULL, &sampler->sampler))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create sampler");

        gfx_pool_free(vctx->sampler_pool, handle);
        return;
    }

    sampler->handle = { handle };
    *out_sampler = &sampler->handle;
}


void vk_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_sampler_t* vks = (vk_sampler_t*)sampler;

    vkDestroySampler(vctx->vk_device, vks->sampler, nullptr);
}



// --- TEXTURE ---

void vk_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** out_texture)
{
    if( (ctx == nullptr) || (desc == nullptr) || (out_texture == nullptr))
        return;

    *out_texture = nullptr;

    vk_context_t* vctx = from_ctx(ctx);
    vk_buffer_t staging = {};

    desc->mip_levels = max(1, desc->mip_levels);
   // size_t size = desc->size;
    uint32_t mem_size = 0;
    for(uint32_t i = 0; i < desc->mip_levels; ++i) 
        mem_size += gfx_utils_image_layer_size(desc->width >> i, desc->height >>i, desc->depth, desc->format);

    VkFormat format = gfx_pixel_format_2_vk(desc->format);
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(vctx->vk_physical_device, format, &properties);
    if((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
        vctx->dbg_log(gfx_msg_error, "not supported pixel format %s", gfx_to_string(desc->format));
        return;
    }

    auto snap0 = gfx_gpu_ram_usage(ctx);

    bool use_default_staging = mem_size < vctx->staging_buffer->buffer_size;

    if(use_default_staging)
    {
        staging = *vctx->staging_buffer;
        if((staging.data_ptr != nullptr) && (desc->data != nullptr))
            memcpy(staging.data_ptr, desc->data, mem_size);
    }
    else
    {
        auto memprop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        _vk_create_buffer(vctx, mem_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memprop, &staging);

        auto snap1 = gfx_gpu_ram_usage(ctx) - snap0;

        void* dataptr = nullptr;
        vkMapMemory(vctx->vk_device, staging.memory, 0, mem_size, 0, &dataptr);
        memcpy(dataptr, desc->data, mem_size);
        vkUnmapMemory(vctx->vk_device, staging.memory);
    }

    bool host_visible = false;
    auto tiling = (host_visible) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

    VkExtent3D extend = {};
    extend.width  = desc->width;
    extend.height = desc->height;
    extend.depth  = desc->depth;
   
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkDeviceMemory memory   = VK_NULL_HANDLE;
    VkImage image = _vk_create_image(vctx, VK_IMAGE_TYPE_2D, extend, desc->mip_levels, format, tiling, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory);

    _vk_image_transition(vctx, image, desc->mip_levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _vk_copy_buffer_to_image(vctx, staging.buffer, image, desc->mip_levels, desc->format, extend);
    _vk_image_transition(vctx, image, desc->mip_levels, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if(!use_default_staging)
    {
        vkDestroyBuffer(vctx->vk_device, staging.buffer, nullptr);
        vkFreeMemory(vctx->vk_device, staging.memory, nullptr);
    }

    VkImageView image_view = _vk_create_image_view(vctx, VK_IMAGE_VIEW_TYPE_2D, image, format, desc->mip_levels);

    auto snap3 = gfx_gpu_ram_usage(ctx) - snap0;

    uint64_t handle = gfx_pool_alloc(vctx->texture_pool);
    vk_texture_t* texture = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, handle);
    if (texture != nullptr)
    {
        texture->handle      = {handle};
        texture->view        = image_view;
        texture->image       = image;
        texture->memory      = memory;
        texture->memory_size = mem_size;
        texture->width       = desc->width;
        texture->height      = desc->height;
        texture->mip_count  = desc->mip_levels;
        *out_texture = &texture->handle;

        vk_debug_set_texture_name(vctx, texture, desc->label);
    }
    vk_gpu_memstatus(ctx);
}

void vk_texture_update_data(gfx_context_t* ctx, gfx_texture_t* /*texture*/, void* /*data*/, uint32_t /*size*/, uint32_t /*offset*/)
{
    auto vkctx = (vk_context_t*)ctx;
    gfx_stub_not_implemented(vkctx ? vkctx->dbg_log : nullptr, "vk_update_image_data");
}

void vk_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* _texture, uint32_t slot_idx)
{
    auto vkctx = (vk_context_t*)ctx;

    if(slot_idx == 0 /*|| slot_idx > vkctx->max_bindles_texture_count*/){
        vkctx->dbg_log(gfx_msg_error, "vk_update_bindless_texture: incorrect slot index %d", slot_idx);
        return;
    }

    if (vkctx->bindless_descriptor_set == VK_NULL_HANDLE) {
        vkctx->dbg_log(gfx_msg_error, "vk_update_bindless_texture: bindless not supported");
        return;
    }

    // if texture null and idx != 0, set default texture
    uint64_t handle = _texture ? _texture->idx : vkctx->default_texture->handle.idx;
    vk_texture_t* texture = (vk_texture_t*)gfx_pool_map(vkctx->texture_pool, handle);

    VkDescriptorImageInfo image_info = { 0 };
    image_info.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView             = texture->view;
    //image_info.sampler             = texture->sampler;

    VkWriteDescriptorSet write  = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet            = vkctx->bindless_descriptor_set;
        write.dstBinding        = 0;            // Target our texture array binding
        write.dstArrayElement   = slot_idx;     // Dynamic index within the global array
        write.descriptorType    = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.descriptorCount   = 1;
        write.pImageInfo        = &image_info;
    vkUpdateDescriptorSets(vkctx->vk_device, 1, &write, 0, nullptr);    // Safe to call at any time (even mid-frame) due to UpdateAfterBind flags
}

void vk_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_texture_t* vtex = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, texture->idx);
    if (!vtex || vtex->mip_count <= 1) return;

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(vctx->vk_physical_device, vtex->format, &props);
    if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
        !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
        vctx->dbg_log(gfx_msg_warning, "vk_texture_generate_mipmap: format does not support blit");
        return;
    }

    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(ctx, &cmd);
    vk_cmd_begin(cmd);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image                           = vtex->image;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    int32_t mip_w = (int32_t)vtex->width;
    int32_t mip_h = (int32_t)vtex->height;

    for (uint32_t i = 1; i < vtex->mip_count; ++i) {
        // transition mip (i-1): SHADER_READ -> TRANSFER_SRC
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(vk_cmd->cmd,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // transition mip i: UNDEFINED -> TRANSFER_DST
        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(vk_cmd->cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        int32_t next_w = mip_w > 1 ? mip_w / 2 : 1;
        int32_t next_h = mip_h > 1 ? mip_h / 2 : 1;

        VkImageBlit blit = {};
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 };
        blit.srcOffsets[0]  = { 0, 0, 0 };
        blit.srcOffsets[1]  = { mip_w, mip_h, 1 };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };
        blit.dstOffsets[0]  = { 0, 0, 0 };
        blit.dstOffsets[1]  = { next_w, next_h, 1 };
        vkCmdBlitImage(vk_cmd->cmd,
            vtex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vtex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        // transition mip (i-1) back to SHADER_READ
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(vk_cmd->cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        mip_w = next_w;
        mip_h = next_h;
    }

    // transition last mip: TRANSFER_DST -> SHADER_READ
    barrier.subresourceRange.baseMipLevel = vtex->mip_count - 1;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(vk_cmd->cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vk_cmd_end(cmd);
    vk_cmd_submit(ctx, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(ctx, cmd);
}

void vk_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_texture_t* vsrc = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, src->idx);
    vk_texture_t* vdst = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, dst->idx);
    if (!vsrc || !vdst) return;

    gfx_command_buffer_t* cmd = nullptr;
    vk_cmd_create(ctx, &cmd);
    vk_cmd_begin(cmd);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    auto make_barrier = [](VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
                           VkAccessFlags src_access, VkAccessFlags dst_access) -> VkImageMemoryBarrier {
        VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        b.image                           = image;
        b.oldLayout                       = old_layout;
        b.newLayout                       = new_layout;
        b.srcAccessMask                   = src_access;
        b.dstAccessMask                   = dst_access;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange                = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        return b;
    };

    VkImageMemoryBarrier pre[2] = {
        make_barrier(vsrc->image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT),
        make_barrier(vdst->image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT),
    };
    vkCmdPipelineBarrier(vk_cmd->cmd,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 2, pre);

    VkImageBlit blit = {};
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.srcOffsets[1]  = { (int32_t)vsrc->width, (int32_t)vsrc->height, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.dstOffsets[1]  = { (int32_t)vdst->width, (int32_t)vdst->height, 1 };
    vkCmdBlitImage(vk_cmd->cmd,
        vsrc->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vdst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

    VkImageMemoryBarrier post[2] = {
        make_barrier(vsrc->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT),
        make_barrier(vdst->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT),
    };
    vkCmdPipelineBarrier(vk_cmd->cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 2, post);

    vk_cmd_end(cmd);
    vk_cmd_submit(ctx, cmd, gfx_submit_wait_for_fence);
    vk_cmd_destroy(ctx, cmd);
}

void vk_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* /*cmd*/)
{
    auto vkctx = (vk_context_t*)ctx;
    gfx_stub_not_implemented(vkctx ? vkctx->dbg_log : nullptr, "vk_texture_get_data");
}


void vk_texture_destroy(gfx_context_t* ctx, gfx_texture_t* _texture)
{
    vk_context_t* vctx = from_ctx(ctx);

    vk_texture_t* texture = (vk_texture_t*)gfx_pool_map(vctx->texture_pool, _texture->idx);
    if(texture != nullptr)
    {
        _destroy_vk_texture(vctx, texture);

        gfx_pool_free(vctx->texture_pool, _texture->idx);
    }
}



// --- PIPELINE ---

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

    uint32_t attribute_count = desc->assembly.attribute_count;
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
    rasterizer.depthClampEnable             = VK_FALSE;
    rasterizer.rasterizerDiscardEnable      = VK_FALSE;
    rasterizer.polygonMode                  = VK_POLYGON_MODE_FILL;
  //  rasterizer.polygonMode                  = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth                    = 1.0f;
    rasterizer.cullMode                     = gfx_cull_2_vk(desc->render_states.culling);
    rasterizer.frontFace                    = gfx_face_2_vk(desc->render_states.face);
    rasterizer.depthBiasEnable              = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.sampleShadingEnable       = VK_FALSE;//vctx->msaa_samples != VK_SAMPLE_COUNT_1_BIT;
    multisampling.rasterizationSamples      = VK_SAMPLE_COUNT_1_BIT;
  //  multisampling.sampleShadingEnable       = VK_TRUE; // enable sample shading in the pipeline
  //  multisampling.minSampleShading          = .2f; //

    VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_state.viewportCount            = 1;
    viewport_state.pViewports               = NULL;
    viewport_state.scissorCount             = 1;
    viewport_state.pScissors                = NULL;

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
        VK_DYNAMIC_STATE_SCISSOR,
    //    VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT, //rasterizationSamples  vkCmdSetRasterizationSamplesEXT
    };
    VkPipelineDynamicStateCreateInfo dyn_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn_state_info.dynamicStateCount    = _countof(dynamic_states);
    dyn_state_info.pDynamicStates       = dynamic_states;

    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount             = vkshader->stage_count;
    pipelineInfo.pStages                = vkshader->stages;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pViewportState         = &viewport_state;
    pipelineInfo.pColorBlendState       = &blend_state;
    pipelineInfo.pDepthStencilState     = &depth_stencil;
    pipelineInfo.pInputAssemblyState    = &assembly;
    pipelineInfo.pVertexInputState      = &vertex_input;
    pipelineInfo.pDynamicState          = &dyn_state_info;
    pipelineInfo.layout                 = vkshader->pipeline_layout;
    pipelineInfo.renderPass             = vctx->vk_default_renderpass;///!!! fuck this shit

    uint64_t handle = gfx_pool_alloc(vctx->pipeline_pool);
    vk_pipeline_t* vkpipeline = (vk_pipeline_t*)gfx_pool_map(vctx->pipeline_pool, handle);
    if(vkpipeline == nullptr)
    {
        vctx->dbg_log(gfx_msg_error, "failed to allocate vk_pipeline_t");
        return;
    }
    vkCreateGraphicsPipelines(vctx->vk_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkpipeline->pipeline);
    vkpipeline->handle = { handle };
    vkpipeline->shader = vkshader;
    *out_pipeline = &vkpipeline->handle;
}


void vk_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** out_pipeline)
{
    assert(ctx);
    vk_context_t* vctx = (vk_context_t*)ctx;
    vk_shader_t* vkshader = (vk_shader_t*)desc->shader;

    if (vkshader == nullptr) {
        vctx->dbg_log(gfx_msg_error, "failed to create pipeline: shader is NULL");
        return;
    }

    VkPipelineShaderStageCreateInfo * compute_stage = nullptr;
    for(uint32_t i = 0; i < vkshader->stage_count; ++i) {
        if (vkshader->stages[i].stage == VK_SHADER_STAGE_COMPUTE_BIT) {
            compute_stage = &vkshader->stages[i];
        }
    }

    if(compute_stage == nullptr) {
        vctx->dbg_log(gfx_msg_error, "shader(%s) doesn't have compute stage", vkshader->label);
        return;
    }

    // Pipeline
    VkPipeline  pipeline = VK_NULL_HANDLE;
    {
        VkComputePipelineCreateInfo create_info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        create_info.stage = *compute_stage;
        create_info.layout = vkshader->pipeline_layout;
        VkResult result = vkCreateComputePipelines(vctx->vk_device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline);
        if (result != VK_SUCCESS) {
            auto err_str = string_VkResult(result);
            vctx->dbg_log(gfx_msg_error, "vkCreateComputePipelines failed with %s error", err_str);
            return;
        }
    }

    uint64_t handle = gfx_pool_alloc(vctx->compute_pipeline_pool);
    vk_compute_pipeline_t * vk_pipeline = (vk_compute_pipeline_t*)gfx_pool_map(vctx->compute_pipeline_pool, handle);

    if(vk_pipeline == nullptr) {
        vctx->dbg_log(gfx_msg_error, "failed to allocate vk_compute_pipeline_t");
        return;
    }

    vk_pipeline->handle     = { handle };
    vk_pipeline->shader     = vkshader;
    vk_pipeline->pipeline   = pipeline;

    *out_pipeline = &vk_pipeline->handle;
}

void vk_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline)
{
    if(!ctx || !pipeline)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_pipeline_t * vkpipeline = (vk_pipeline_t*)pipeline;
    vkDestroyPipeline(vctx->vk_device, vkpipeline->pipeline, nullptr);

    _gfx_free(vctx, vkpipeline);
}

void vk_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline)
{
    if (!ctx || !pipeline)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_compute_pipeline_t* vkpipeline = (vk_compute_pipeline_t*)gfx_pool_map(vctx->compute_pipeline_pool, pipeline->idx);
    if (vkpipeline)
    {
        vkDestroyPipeline(vctx->vk_device, vkpipeline->pipeline, nullptr);
        gfx_pool_free(vctx->compute_pipeline_pool, vkpipeline->handle.idx);
    }
}

void vk_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline)
{
    vk_context_t* vctx = from_ctx(ctx);
    gfx_stub_not_implemented(vctx->dbg_log, "vk_create_mesh_pipeline");
}

void vk_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline)
{
    vk_context_t* vctx = from_ctx(ctx);
    gfx_stub_not_implemented(vctx->dbg_log, "vk_create_raytrace_pipeline");
}

void vk_destroy_mesh_pipeline(gfx_context_t* ctx, gfx_pipeline_t* desc)
{
}

void vk_destroy_raytrace_pipeline(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline)
{
}



// --- DESCRIPTOR SET ---

vk_descriptor_pool_t * _get_or_create_descriptor_set_pool(vk_context_t* ctx, uint32_t hash)
{
    return nullptr;
}

uint64_t _descriptor_set_layout_hash(VkDescriptorSetLayoutBinding* bindings, size_t count)
{
    uint64_t hash = (uint64_t)count * 0x100000001B3ULL;   // FNV prime

    for (size_t i = 0; i < count; ++i)
    {
        const VkDescriptorSetLayoutBinding* b = &bindings[i];

        uint64_t v = ((uint64_t)b->binding) |
                     ((uint64_t)b->descriptorType << 16) |
                     ((uint64_t)b->descriptorCount << 24) |
                     ((uint64_t)b->stageFlags << 32);

        hash ^= v;
        hash *= 0x100000001B3ULL;        // FNV prime 64-bit
        hash ^= hash >> 32;
    }

    return hash;
};


void vk_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** out_set)
{
    assert(ctx && shader && out_set);

    vk_context_t* vctx = from_ctx(ctx);
    vk_shader_t* vk_shader = (vk_shader_t*)shader;

    if(set_idx > (vk_shader->set_count - 1)) {
        vctx->dbg_log(gfx_msg_error, "Descriptor set index(%u) is out of bounds(valid range is 0 to %u)", set_idx,  vk_shader->set_count-1);
        return;
    }

    // 
    char * data_ptr = (char*)vk_shader->set_bindings[set_idx];
    size_t data_size = sizeof(VkDescriptorSetLayoutBinding) * vk_shader->set_binding_count[set_idx];
    uint32_t bindings_hash = gfx_utils_hash_32(data_ptr, data_size);
    vk_descriptor_pool_t * pool = _get_or_create_descriptor_set_pool(vctx, bindings_hash);

    if(vk_shader->pool == nullptr)
    {
       // vk_create_descriptor_pool(vctx, vk_shader, 0, MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->pool);
        vk_create_descriptor_pool(vctx, vk_shader, 0, MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->pool);
    }

    if(vk_shader->pool->free_set_count == 0)
    {
        vk_create_descriptor_pool(vctx, vk_shader, 0, MAX_DESCRIPTOR_POOL_SET_SIZE, &vk_shader->pool);
    }

    uint32_t next_free = vk_shader->pool->next_free_index;

    for(uint32_t i = next_free; i < vk_shader->pool->capacity; ++i)
    {
        if(!vk_shader->pool->descriptor_sets[i].is_free)
            continue;

        vk_shader->pool->descriptor_sets[i].is_free = false;
        vk_shader->pool->free_set_count--;
        vk_shader->pool->next_free_index = i + 1;

        *out_set = &vk_shader->pool->descriptor_sets[i].handle;
        break;
    }
}


void vk_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor)
{
    if (!ctx || !descriptor)
        return;

    vk_context_t* vctx = from_ctx(ctx);
    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)descriptor;

    vkset->is_free = true;
    vkset->pool->next_free_index = vkset->index_in_pool;
    vkset->pool->free_set_count++;
}


void vk_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;
    vk_shader_t* shader   = vkset->shader;
    vk_context_t* vkctx     = shader->ctx;

    gfx_uniform_loc_t loc = { handle };

    if (shader->hash32 != loc.shader_hash) {
        vkctx->dbg_log(gfx_msg_warning, "handle from another shader");
        return;
    }

    gfx_uniform_type type = (gfx_uniform_type)loc.type;
    if (type != gfx_uniform_ubo) {
        vkctx->dbg_log(gfx_msg_warning, "type mismatch");
        return;
    }

    const gfx_uniform_t* uniform = loc.binding < shader->uniform_count ? &shader->uniforms[loc.binding] : nullptr;

    if ((uniform != nullptr) && (uniform->type == gfx_uniform_ubo) && (loc.member_idx < uniform->buffer.field_count))
    {
        uint32_t offset = uniform->buffer.fields[loc.member_idx].offset;
        memcpy(vkset->ubo_mapped_data + offset, data, size);
    }
}


void vk_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;

    vk_shader_t* vkshader = vkset->shader;
    vk_context_t* vkctx = vkshader->ctx;

    gfx_uniform_loc_t loc = { handle };

    if (vkshader->hash32 != loc.shader_hash) {
        vkctx->dbg_log(gfx_msg_warning, "handle from another shader");
        return;
    }
    assert(false);
}


void vk_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset = (vk_descriptor_set_t*)set;

    vk_shader_t * vkshader      = vkset->shader;
    vk_context_t* vkctx         = vkshader->ctx;

    gfx_uniform_loc_t loc = { handle };

    if (vkshader->hash32 != loc.shader_hash) {
        vkctx->dbg_log(gfx_msg_warning, "handle from another shader");
        return;
    }

    gfx_uniform_type type = (gfx_uniform_type)loc.type;
    if(type != gfx_uniform_texture2d && type != gfx_uniform_texture3d && type != gfx_uniform_texture2d_cube && type != gfx_uniform_texture2d_array) {
        vkctx->dbg_log(gfx_msg_warning, "type mismatch: expected gfx_uniform_textureXXX");
        return;
    }

    auto binding = vkshader->uniforms[loc.binding].binding;

    vk_texture_t* vktexture = texture ? (vk_texture_t*)gfx_pool_map(vkctx->texture_pool, texture->idx) : vkctx->default_texture;
    VkImageView image_view  = vktexture->view;
    auto sampler = (vk_sampler_t*)gfx_pool_map(vkctx->sampler_pool, vkctx->default_sampler->idx);

    VkDescriptorImageInfo image_info = { 0 };
        image_info.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView    = image_view;
        image_info.sampler      = sampler->sampler;

     VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.descriptorType    = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.dstSet            = vkset->descriptor_set;
        write.dstBinding        = binding;
        write.descriptorCount   = 1;
        write.pImageInfo        = &image_info;
    vkUpdateDescriptorSets(vkctx->vk_device, 1, &write, 0, nullptr);
}


void vk_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler)
{
    if (set == NULL || handle == 0)
        return;

    vk_descriptor_set_t* vkset  = (vk_descriptor_set_t*)set;
    vk_shader_t* vkshader       = vkset->shader;
    vk_context_t* vkctx         = vkshader->ctx;
    vk_sampler_t* vksampler     = (vk_sampler_t*)sampler;

    gfx_uniform_loc_t loc = { handle };

    if (vkshader->hash32 != loc.shader_hash) {
        vkctx->dbg_log(gfx_msg_warning, "handle from another shader");
        return;
    }

    gfx_uniform_type type = (gfx_uniform_type)loc.type;
    if (type != gfx_uniform_sampler) {
        vkctx->dbg_log(gfx_msg_warning, "type mismatch: expected gfx_uniform_textureXXX");
        return;
    }

    auto binding = vkshader->uniforms[loc.binding].binding;

    VkDescriptorImageInfo image_info = { 0 };
        image_info.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.sampler      = vksampler->sampler;
        image_info.imageView    = vkctx->default_texture->view;

    VkWriteDescriptorSet write  = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.descriptorType    = VK_DESCRIPTOR_TYPE_SAMPLER;
        write.dstSet            = vkset->descriptor_set;
        write.dstBinding        = binding;
        write.descriptorCount   = 1;
        write.pImageInfo        = &image_info;
    vkUpdateDescriptorSets(vkctx->vk_device, 1, &write, 0, nullptr);
}



// --- COMMAND BUFFER ---

void vk_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* pass)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_render_target_t* vk_target = (vk_render_target_t*)pass->target;

    VkClearColorValue color = { 0 };
        color.float32[0] = ((pass->clear_color_value >> 24) & 0xFF) / 255.0f; // R
        color.float32[1] = ((pass->clear_color_value >> 16) & 0xFF) / 255.0f; // G
        color.float32[2] = ((pass->clear_color_value >> 8) & 0xFF) / 255.0f; // B
        color.float32[3] = (pass->clear_color_value & 0xFF) / 255.0f; // A

    VkClearValue clear_value[] = {
            color,
            { pass->clear_depth_value, pass->clear_stencil_value },
    };

    VkRect2D render_area = {
        {0,0},                          // offset
        vk_target->extent               // extent
    };

    VkRenderPassBeginInfo info  = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        info.renderPass         = vk_target->renderpass;
        info.framebuffer        = vk_target->framebuffer;
        info.renderArea         = render_area;
        info.clearValueCount    = 2;
        info.pClearValues       = clear_value; 
    vkCmdBeginRenderPass(vk_cmd->cmd, &info, VK_SUBPASS_CONTENTS_INLINE);

    vk_cmd_viewport(cmd, 0, 0, vk_target->extent.width, vk_target->extent.height);
    vk_cmd_scissor(cmd, 0, 0, vk_target->extent.width, vk_target->extent.height);
}

void vk_cmd_end_pass(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
   // vk_cmd->ctx->

    vkCmdEndRenderPass(vk_cmd->cmd);

//    vkCmdEndRendering(vk_cmd->cmd);
}

void vk_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkRect2D scissor        = {};
    scissor.offset.x        = x;
    scissor.offset.y        = y;
    scissor.extent.width    = w;
    scissor.extent.height   = h;

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

void vk_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor)
{
    if(cmd == nullptr || descriptor == nullptr){
        gfx_default_log(gfx_msg_error, "invalid arguments vk_cmd_bind_descriptor_set(cmd:0x%x, slot:%d, descriptor:0x%x)", cmd, slot, descriptor);
        return;
    }

    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_descriptor_set_t* set = (vk_descriptor_set_t*)descriptor;
    vk_shader_t * shader = set->shader;

    if(slot > shader->set_count - 1) {
        gfx_default_log(gfx_msg_error, "vk_cmd_bind_descriptor_set: out of bounds: slot(%d) shader_max_set(%d) )", slot, shader->set_count - 1);
    }

    auto count = 1u;
    VkDescriptorSet descriptor_set = set->descriptor_set;
    VkPipelineLayout pipeline_layout = set->shader->pipeline_layout;

    vkCmdBindDescriptorSets(vk_cmd->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, slot, 1u, &descriptor_set, 0, nullptr);
}

void vk_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_buffer_t* vkbuffer = (vk_buffer_t*)gfx_pool_map(vk_cmd->ctx->buffers_pool, buffer->idx);

    VkIndexType index_type = gfx_index_format_2_vk(format);
    vkCmdBindIndexBuffer(vk_cmd->cmd, vkbuffer->buffer, offset, index_type);
}

void vk_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t binding, uint32_t offset, gfx_buffer_t* buffer)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    vk_buffer_t* vkbuffer = (vk_buffer_t*)gfx_pool_map(vk_cmd->ctx->buffers_pool, buffer->idx);
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(vk_cmd->cmd, binding, 1, &vkbuffer->buffer, offsets);
}

void vk_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdDraw(vk_cmd->cmd, vertex_count, instance_count, 0, 0);
}

void vk_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vkCmdDrawIndexed(vk_cmd->cmd, idx_count, instance_count, first_idx, vertex_offset, 0);
}

void vk_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_buffer_t* vkbuffer = (vk_buffer_t*)gfx_pool_map(vk_cmd->ctx->buffers_pool, buffer->idx);
    vkCmdDrawIndexedIndirect(vk_cmd->cmd, vkbuffer->buffer, offset, draw_count, stride);
}

void vk_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z)
{
    vk_command_buffer_t* vkcmd = (vk_command_buffer_t*)cmd;
    vkCmdDispatch(vkcmd->cmd, x, y, z);
}

void vk_cmd_push_marker(gfx_command_buffer_t* cmd, const char* marker)
{
    vk_command_buffer_t* vkcmd = (vk_command_buffer_t*)cmd;
    vk_context_t* ctx = vkcmd->ctx;

    if(ctx->vk_dbg_cmd_push_label != nullptr)
    {
        VkDebugUtilsLabelEXT label  = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        label.pLabelName            = marker;
        ctx->vk_dbg_cmd_push_label(vkcmd->cmd, &label);
    }

    uint32_t start_idx = vkcmd->time_query_current_index;

    vkCmdWriteTimestamp(vkcmd->cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkcmd->time_query_pool, start_idx);

    vkcmd->marker_names[start_idx] = marker;
    vkcmd->time_query_stack[++vkcmd->time_query_stack_top] = start_idx;
    vkcmd->time_query_current_index += 2; // reserve pair (start, end)
}

void vk_cmd_pop_marker(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vkcmd = (vk_command_buffer_t*)cmd;
    vk_context_t* ctx = vkcmd->ctx;

    if (vkcmd->time_query_stack_top > 0)
    {
        uint32_t start_idx = vkcmd->time_query_stack[vkcmd->time_query_stack_top--];
        uint32_t end_idx = start_idx + 1;

        vkCmdWriteTimestamp(vkcmd->cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkcmd->time_query_pool, end_idx);
        vkcmd->stamp_count++;
        vkcmd->time_query_index--;
    }

    if (ctx->vk_dbg_cmd_pop_label != nullptr)
        ctx->vk_dbg_cmd_pop_label(vkcmd->cmd);
}

void vk_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier old_state, gfx_barrier new_state)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_context_t *ctx = vk_cmd->ctx;

    vk_state_mapping_t src_state = get_vulkan_state(old_state, 0);
    vk_state_mapping_t dst_state = get_vulkan_state(new_state, 0);

    uint32_t buffer_barrier_count = 0;
    VkBufferMemoryBarrier2 buffer_barriers[GFX_MAX_BATCH_BARRIERS] = {};
    for(uint32_t i = 0; i < count; ++i)
    {
        vk_buffer_t *vk_buffer = (vk_buffer_t*)gfx_pool_map(ctx->buffers_pool, buffers[i]->idx);

        buffer_barriers[i].sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        buffer_barriers[i].pNext         = NULL;
        buffer_barriers[i].srcStageMask  = src_state.stage;
        buffer_barriers[i].srcAccessMask = src_state.access;
        buffer_barriers[i].dstStageMask  = dst_state.stage;
        buffer_barriers[i].dstAccessMask = dst_state.access;
        buffer_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        buffer_barriers[i].buffer   = vk_buffer->buffer;
        buffer_barriers[i].offset   = 0;
        buffer_barriers[i].size     = VK_WHOLE_SIZE;
        buffer_barrier_count++;
    }

    VkDependencyInfo dependency_info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO, NULL };
    dependency_info.bufferMemoryBarrierCount = buffer_barrier_count;
    dependency_info.pBufferMemoryBarriers = buffer_barrier_count > 0 ? buffer_barriers : NULL;
    vkCmdPipelineBarrier2(vk_cmd->cmd, &dependency_info);
}

void vk_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier old_state, gfx_barrier new_state)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;
    vk_context_t* ctx = vk_cmd->ctx;

    uint32_t image_barrier_count = 0;
    VkImageMemoryBarrier2 image_barriers[GFX_MAX_BATCH_BARRIERS] = {};
    for (uint32_t i = 0; i < count; ++i)
    {
        vk_texture_t* vk_texture = (vk_texture_t*)gfx_pool_map(ctx->texture_pool, textures[i]->idx);

        VkImageAspectFlags aspect_mask = determine_aspect_mask(vk_texture->format);
        vk_state_mapping_t src = get_vulkan_state(old_state, aspect_mask);
        vk_state_mapping_t dst = get_vulkan_state(new_state, aspect_mask);

        // Protection: newLayout should never be UNDEFINED for a texture with data
        if (dst.image_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            dst.image_layout = VK_IMAGE_LAYOUT_GENERAL;
        }

        // Adjusting aspect ratio for combined depth-stencil textures when reading in shader
        VkImageAspectFlags barrier_aspect = aspect_mask;
        if (dst.image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
            (barrier_aspect & VK_IMAGE_ASPECT_STENCIL_BIT))
        {
            // On mobile devices, stencil is rarely read in the texture sampler, so we leave only depth
            barrier_aspect &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        image_barriers[i].sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        image_barriers[i].pNext         = NULL;
        image_barriers[i].srcStageMask  = src.stage;
        image_barriers[i].srcAccessMask = src.access;
        image_barriers[i].dstStageMask  = dst.stage;
        image_barriers[i].dstAccessMask = dst.access;
        image_barriers[i].oldLayout     = src.image_layout;
        image_barriers[i].newLayout     = dst.image_layout;
        image_barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_barriers[i].image         = vk_texture->image;
        image_barriers[i].subresourceRange.aspectMask       = aspect_mask;
        image_barriers[i].subresourceRange.baseMipLevel     = 0;//vk_texture->base_mip;
        image_barriers[i].subresourceRange.levelCount       = vk_texture->mip_count;
        image_barriers[i].subresourceRange.baseArrayLayer   = 0;//vk_texture->base_layer;
        image_barriers[i].subresourceRange.layerCount       = 1;//vk_texture->layer_count;
        image_barrier_count++;
    }

    VkDependencyInfo dependency_info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO, NULL };
    dependency_info.imageMemoryBarrierCount = image_barrier_count;
    dependency_info.pImageMemoryBarriers    = image_barrier_count > 0 ? image_barriers : NULL;;
    vkCmdPipelineBarrier2(vk_cmd->cmd, &dependency_info);
}


///////////////////////
void vk_cmd_create(gfx_context_t* ctx, gfx_command_buffer_t** out_cmd)
{
    vk_context_t* vctx = (vk_context_t*)ctx;

    uint32_t thread_id = gfx_utils_thread_id();

    if (vctx->cmd_pool_size != 0)
    {
        for (int j = 0; j < _countof(vctx->cmd_buffer_pool); ++j)
        {
            if (vctx->cmd_buffer_pool[j] != nullptr && vctx->cmd_buffer_pool[j]->thread_id == thread_id)
            {
                vk_command_buffer_t* tmp = vctx->cmd_buffer_pool[j];
                *out_cmd = &tmp->handle;

                vkResetCommandBuffer(tmp->cmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
                vctx->cmd_pool_size--;
                vctx->cmd_buffer_pool[j] = nullptr;
                return;
            }
        }
    }

    uint64_t handle = gfx_pool_alloc(vctx->cmd_pool);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)gfx_pool_map(vctx->cmd_pool, handle);
    if (vk_cmd == nullptr)
        return;

    // create pool
    VkCommandPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = vctx->graphics_queue.family;
    if (auto result = vkCreateCommandPool(vctx->vk_device, &create_info, nullptr, &vk_cmd->pool))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create command pool!(%s)", result);
    }

    // create command buffers
    VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = vk_cmd->pool;
    alloc_info.commandBufferCount = 1;
    if (auto result = vkAllocateCommandBuffers(vctx->vk_device, &alloc_info, &vk_cmd->cmd))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to allocate command buffers! (%s)", err_str);
    }

    // create timestamps
    VkQueryPoolCreateInfo timestamp_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    timestamp_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    timestamp_info.queryCount = 128;
    if (auto result = vkCreateQueryPool(vctx->vk_device, &timestamp_info, nullptr, &vk_cmd->time_query_pool))
    {
        auto err_str = string_VkResult(result);
        vctx->dbg_log(gfx_msg_error, "failed to create timestamps for command buffers! (%s)", err_str);
    }

    vk_cmd->handle = { handle };
    vk_cmd->ctx = vctx;
    vk_cmd->device = vctx->vk_device;
    vk_cmd->thread_id = gfx_utils_thread_id();
    *out_cmd = &vk_cmd->handle;
}

void vk_cmd_destroy(gfx_context_t* ctx, gfx_command_buffer_t* cmd)
{
    vk_context_t* vctx = from_ctx(ctx);
    vk_command_buffer_t* vkcmd = (vk_command_buffer_t*)cmd;
    if (vctx == nullptr || vkcmd == nullptr)
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

    vkResetCommandPool(vctx->vk_device, vkcmd->pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    vkFreeCommandBuffers(vctx->vk_device, vkcmd->pool, 1, &vkcmd->cmd);
    vkDestroyCommandPool(vctx->vk_device, vkcmd->pool, nullptr);
}

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

    vkCmdResetQueryPool(vk_cmd->cmd, vk_cmd->time_query_pool, 0, MAX_TIMESTAMP_QUERIES);
}

void vk_cmd_end(gfx_command_buffer_t* cmd)
{
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    if (auto result = vkEndCommandBuffer(vk_cmd->cmd)) {
        vk_cmd->ctx->dbg_log(gfx_msg_error, "vkEndCommandBuffer failed!(s)", string_VkResult(result));
    }
}

void vk_cmd_submit(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options)
{
    vk_context_t* vkctx = from_ctx(ctx);
    vk_command_buffer_t* vk_cmd = (vk_command_buffer_t*)cmd;

    VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkFence fence = VK_NULL_HANDLE;
    if (options == gfx_submit_wait_for_fence)
    {
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(vkctx->vk_device, &fenceInfo, nullptr, &fence);
    }

    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &vk_cmd->cmd;

    if (auto result = vkQueueSubmit(vkctx->graphics_queue.queue, 1, &submit, fence))
    {
        auto err_str = string_VkResult(result);
        vkctx->dbg_log(gfx_msg_error, "vkQueueSubmit failed!(%s)", err_str);
    }

    VkResult wait_result = VK_SUCCESS;

    switch (options)
    {
        case gfx_submit_nowait: break;
        case gfx_submit_wait_for_fence:         wait_result = vkWaitForFences(vkctx->vk_device, 1, &fence, VK_TRUE, 1000000000); break;
        case gfx_submit_wait_for_queue_idle:    wait_result = vkQueueWaitIdle(vkctx->graphics_queue.queue); break;
        case gfx_submit_wait_for_device_idle:   wait_result = vkDeviceWaitIdle(vkctx->vk_device); break;
    }

    if(wait_result != VK_SUCCESS)
    {
        vkctx->dbg_log(gfx_msg_error, "wait failed!(%s)", string_VkResult(wait_result));
    }

    if (options == gfx_submit_wait_for_fence)
    {
        vkResetFences(vkctx->vk_device, 1, &fence);
        vkDestroyFence(vkctx->vk_device, fence, nullptr);
    }
}
//////////////////////


// --- DEBUG ---
void vk_debug_set_name(vk_context_t* ctx, uint64_t vkobject, VkObjectType type, const char* name)
{
    if(ctx->vk_dbg_set_object_name != nullptr) {
        VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        name_info.objectType    = type;
        name_info.objectHandle  = vkobject;
        name_info.pObjectName   = name;
        ctx->vk_dbg_set_object_name(ctx->vk_device, &name_info);
    }
}

void vk_debug_set_texture_name(vk_context_t* ctx, vk_texture_t* texture, const char* name)
{
    vk_debug_set_name(ctx, (uint64_t)texture->image, VK_OBJECT_TYPE_IMAGE, name);
}

void vk_debug_set_buffer_name(vk_context_t* ctx, vk_buffer_t* buffer, const char* name)
{
    vk_debug_set_name(ctx, (uint64_t)buffer->buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void vk_debug_set_shader_name(vk_context_t* ctx, vk_shader_t* shader, const char* name)
{
    for (uint32_t i = 0; i < shader->stage_count; ++i)
    {
        vk_debug_set_name(ctx, (uint64_t)shader->stages[i].module, VK_OBJECT_TYPE_SHADER_MODULE, name);
    }
}


#endif