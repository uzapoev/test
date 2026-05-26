#ifndef __gfx_vulkan_h__
#define __gfx_vulkan_h__

#include "gfx.h"

#if defined(_WIN32) && __has_include(<vulkan/vulkan.h>)
    #define VULKAN_AVAILABLE
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#if defined(__ANDROID__) && __has_include(<vulkan/vulkan.h>)
    #define VULKAN_AVAILABLE
    #define VK_USE_PLATFORM_ANDROID_KHR
#endif

#if defined(__APPLE__) && __has_include(<vulkan/vulkan.h>)
    #define VULKAN_AVAILABLE
    #error todo: VK_USE_PLATFORM_ 
#endif


#ifdef VULKAN_AVAILABLE

#include <vulkan/vulkan.h>


#ifdef __cplusplus 
extern "C" {
#endif

#define     GFX_MAX_FRAME_IN_FLIGHT             (2)

#define     GFX_MAX_DESCRIPTOR_SETS             (8)

#define     GFX_MAX_DESCRIPTOR_BINDINGS         (16)    // Max binding slots allocated per set. Handles non-contiguous binding indices 
                                                        // caused by cross-compiler global flattening (e.g. Slang ParameterBlock)

#define     MAX_TIMESTAMP_QUERIES               (128)

#define     MAX_TIMESTAMP_NESTING_LEVEL         (16)

#define     GFX_MAX_BATCH_BARRIERS              (16)

#define     MAX_DESCRIPTOR_POOL_SET_SIZE        (1024)

/**/
struct vk_descriptor_pool_t;
struct vk_descriptor_set_t;
struct vk_command_buffer_t;
struct vk_buffer_t;
struct vk_texture_t;

typedef struct vk_context_t
{
    gfx_context_t                       handle;

    // --- Core Vulkan Handles ---
    VkInstance                          vk_instance            = VK_NULL_HANDLE;
    VkDevice                            vk_device              = VK_NULL_HANDLE;
    VkPhysicalDevice                    vk_physical_device     = VK_NULL_HANDLE;
    VkSurfaceKHR                        vk_surface             = VK_NULL_HANDLE;

    gfx_caps_t                          gpu_caps;
    gfx_allocator_t                     allocator;

    // --- Device Queues ---
    struct { 
        uint32_t family;
        VkQueue  queue;
    }                                   graphics_queue,
                                        present_queue,
                                        compute_queue;

    // --- Device Properties & Features ---
    VkPhysicalDeviceFeatures            device_features     = {};
    VkPhysicalDeviceMemoryProperties    memory_properties   = {};
    VkPhysicalDeviceProperties          device_properties   = {};



    // --- Bindless Resources ---
    VkDescriptorSet                     bindless_descriptor_set         = VK_NULL_HANDLE;
    VkDescriptorPool                    bindless_descriptor_pool        = VK_NULL_HANDLE;
    VkDescriptorSetLayout               bindless_descriptor_set_layout  = VK_NULL_HANDLE;

    // --- Default / Fallback Resources ---
    vk_buffer_t*                        uniform_buffer          = nullptr;
    vk_buffer_t*                        staging_buffer          = nullptr;
    gfx_sampler_t*                      default_sampler         = nullptr;
    vk_texture_t*                       default_texture         = nullptr;
    gfx_texture_t*                      default_storage_texture = nullptr;
    gfx_buffer_t*                       default_storage_buffer  = nullptr;

    VkRenderPass                        vk_default_renderpass      = nullptr; //fuuuuu!

    gfx_offset_allocator_t*             uniform_buffer_allocator = nullptr;

    // --- Command Buffers ---
    vk_command_buffer_t*                cmd_buffer_pool[32];
    uint32_t                            cmd_pool_size;

    // --- Global Object Resource Pools --
    gfx_handle_pool_t*                  surface_pool;
    gfx_handle_pool_t*                  render_target_pool;

    gfx_handle_pool_t*                  cmd_pool;
    gfx_handle_pool_t*                  sampler_pool;
    gfx_handle_pool_t*                  texture_pool;
    gfx_handle_pool_t*                  buffers_pool;
    gfx_handle_pool_t*                  shaders_pool;
    gfx_handle_pool_t *                 pipeline_pool;
    gfx_handle_pool_t *                 compute_pipeline_pool;
    gfx_handle_pool_t *                 descriptor_set_holder_pools;

    //gfx_linked_list_t*                  descriptor_set_pool_list; //

        // --- Debug & Callbacks ---
    gfx_callback                        dbg_log = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT    vk_dbg_set_object_name;
    PFN_vkCmdBeginDebugUtilsLabelEXT    vk_dbg_cmd_push_label;
    PFN_vkCmdEndDebugUtilsLabelEXT      vk_dbg_cmd_pop_label;

    // mesh shading extension
    PFN_vkCmdDrawMeshTasksEXT           vk_cmd_draw_mesh_tasks_pfn;
    PFN_vkCmdDrawMeshTasksIndirectEXT   vk_cmd_draw_mesh_tasks_indirect_pfn;

    // raytracing
    PFN_vkCreateAccelerationStructureKHR        vkCreateAccelerationStructureKHR = nullptr;
    PFN_vkDestroyAccelerationStructureKHR       vkDestroyAccelerationStructureKHR = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR     vkCmdBuildAccelerationStructuresKHR = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    PFN_vkCmdTraceRaysKHR                       vkCmdTraceRaysKHR = nullptr;

} vk_context_t;


typedef struct vk_surface_t {
    gfx_surface_t                       handle;

    intptr_t                            window_handle;  // OS-specific window handle (HWND, NSWindow, etc.)
    uint32_t                            width;          // Framebuffer width in pixels
    uint32_t                            height;         // Framebuffer height in pixels
    VkBool32                            vsync;          // Vertical synchronization flag

    gfx_pixel_format                    format;         // High-level surface pixel format
    gfx_sample_count                    sample_count;   // MSAA sample count

    VkFormat                            depth_format;    // Selected depth/stencil format for the surface

    VkSurfaceKHR                        surface;
    VkSwapchainKHR                      swapchain;
    VkRenderPass                        render_pass;
    VkSampleCountFlags                  vk_msaa_samples;

    VkFence                             fences[GFX_MAX_FRAME_IN_FLIGHT];                        // CPU-GPU frame execution fences
    VkSemaphore                         semaphore_image_available[GFX_MAX_FRAME_IN_FLIGHT];     // Wait Semaphores
    VkSemaphore                         semaphore_rendering_finished[GFX_MAX_FRAME_IN_FLIGHT];  // Signal Semaphores

    uint32_t                            current_frame;          // Index of the current CPU frame (0 to MAX_FRAME_IN_FLIGHT - 1)
    uint32_t                            swapchain_image_index;  // Index of the acquired swapchain image
    
    gfx_frame_t                         frames[GFX_MAX_FRAME_IN_FLIGHT];

    struct vk_render_target_t*          targets[GFX_MAX_FRAME_IN_FLIGHT];
} vk_surface_t;


typedef struct vk_pipeline_t {
    gfx_pipeline_t                      handle;

    struct vk_shader_t*                 shader;

    vk_descriptor_set_t*                default_sets[GFX_MAX_DESCRIPTOR_SETS];

    VkPipelineBindPoint                 bind_point;
    VkPipeline                          pipeline;
} vk_pipeline_t;


typedef struct vk_compute_pipeline_t {
    gfx_pipeline_compute_t              handle;
    struct vk_shader_t*                 shader;
    VkPipeline                          pipeline;
} vk_compute_pipeline_t;


typedef struct vk_sampler_t {
    gfx_sampler_t                       handle;
    VkSampler                           sampler;
} vk_sampler_t;


typedef struct vk_texture_t {
    gfx_texture_t                       handle;
    VkFormat                            format;
    VkImage                             image;
    VkImageView                         view;
    VkDeviceMemory                      memory;
    VkDeviceSize                        memory_size;
    uint32_t                            width;
    uint32_t                            height;
    uint32_t                            mip_count;
} vk_texture_t;


typedef struct vk_buffer_t {
    gfx_buffer_t                        handle;
    VkBuffer                            buffer;
    VkDeviceMemory                      memory;
    VkBufferUsageFlags                  usage;

    bool                                is_mapped;
    VkDeviceSize                        buffer_size;
    void*                               data_ptr;
} vk_buffer_t;


typedef struct vk_shader_t {
    gfx_shader_t                        handle;

    vk_context_t *                      ctx;
    const char*                         label = nullptr;

    uint32_t                            stage_count;               // used in pipeline creation
    VkPipelineShaderStageCreateInfo     stages[gfx_shader_count];   //

    uint32_t                            hash32;
    uint32_t                            uniform_count;
    gfx_uniform_t *                     uniforms;

    uint32_t                            set_count;          // max set + 1
    VkDescriptorSetLayout               set_layouts[GFX_MAX_DESCRIPTOR_SETS];
    uint32_t                            set_binding_count[GFX_MAX_DESCRIPTOR_SETS] = { 0 }; // per set
    VkDescriptorSetLayoutBinding*       set_bindings[GFX_MAX_DESCRIPTOR_SETS] = { 0 };      // per set

    VkPipelineLayout                    pipeline_layout;

    vk_descriptor_set_t *               default_sets[GFX_MAX_DESCRIPTOR_SETS] = { 0 };

    vk_descriptor_pool_t *              pool;                   //todo: outdated, current pool
    gfx_handle_pool_t *                 descriptor_set_pool;    //todo: outdated, pool of pools
} vk_shader_t;


typedef struct vk_render_target_t {
    gfx_render_target_t                 handle;
    VkFormat                            color_format;
    VkFormat                            depth_format;

    uint32_t                            color_attachment_count;
    vk_texture_t                        color_attachments[8];  // max attachment
    vk_texture_t                        depth_attachments;
    vk_texture_t                        resolve_attachments;    // surface swapchain if msaa

   // tr_render_target
    VkExtent2D                          extent;
    VkRenderPass                        renderpass;
    VkFramebuffer                       framebuffer;
} vk_render_target_t;



typedef struct vk_descriptor_pool_t {
    uint32_t                            bindings_hash;          // Hash of descriptor layout bindings for validation

    uint32_t                            set_layout_binding_count = { 0 };  // per set
    VkDescriptorSetLayoutBinding        set_layout_bindings[8];            // per set

    VkDescriptorPool                    pool;                   // Native Vulkan descriptor pool handle
    uint32_t                            capacity;               // Total number of descriptor sets available in this pool
    uint32_t                            free_set_count;         // Remaining number of unallocated descriptor sets

    uint64_t*                           bitset_mask;            // Array of bitmasks tracking allocation status per set
    uint32_t                            bitset_word_count;      // Number of 64-bit words in the bitset_mask array

    vk_descriptor_set_t*                descriptor_sets;        // Array of managed descriptor set wrappers (size equals capacity)
} vk_descriptor_pool_t;



typedef struct vk_descriptor_set_t {
    gfx_descriptor_set_t                handle;

    vk_shader_t *                       shader;                 
    vk_descriptor_pool_t *              pool;                   // Owner pool from which this set was allocated
    uint16_t                            index_in_pool;
    uint16_t                            index_in_sets;

    uint32_t                            ubo_offset;             // Byte offset inside the pool's global UBO buffer
    uint32_t                            ubo_offsets[GFX_MAX_FRAME_IN_FLIGHT];
    VkDescriptorSet                     descriptor_set;
} vk_descriptor_set_t;


typedef struct vk_command_buffer_t {
    gfx_command_buffer_t                handle;

    vk_context_t *                      ctx;                    // Pointer to the parent Vulkan context
    uint32_t                            thread_id;              // ID of the CPU thread owning this command buffer

    VkDevice                            device              = VK_NULL_HANDLE;
    VkCommandPool                       pool                = VK_NULL_HANDLE;
    VkCommandBuffer                     cmd                 = VK_NULL_HANDLE;

    VkFence                             vk_fence            = VK_NULL_HANDLE;

    // 
    uint32_t                            user_bound_mask;
    VkDescriptorSet                     bound_sets[GFX_MAX_DESCRIPTOR_SETS];

    // replace to struct begin+end+name
    VkQueryPool                         time_query_pool     = VK_NULL_HANDLE;
    uint32_t                            time_query_index    = 0;
    uint32_t                            stamp_count         = 0;

    uint32_t                            time_query_stack[MAX_TIMESTAMP_NESTING_LEVEL];
    int32_t                             time_query_stack_top = 0; // -1 if empty

    uint32_t                            time_query_current_index = 0;
    uint64_t                            time_query_results[MAX_TIMESTAMP_QUERIES];
    const char*                         marker_names[MAX_TIMESTAMP_QUERIES];

    uint32_t                            resolved_stamp_count = 0;
    gfx_timestamp_t                     resolved_stamps[MAX_TIMESTAMP_QUERIES];

} vk_command_buffer_t;


gfx_api void vk_create_renderer(gfx_settings_t* cfg, gfx_context_t** ctx);
gfx_api void vk_destroy_renderer(gfx_context_t* ctx);
gfx_api void vk_get_caps(gfx_context_t* ctx, gfx_caps_t* caps);

// surface
gfx_api void vk_surface_create(gfx_context_t* ctx, gfx_surface_desc_t* desc, gfx_surface_t** surface);
gfx_api void vk_surface_destroy(gfx_context_t* ctx, gfx_surface_t* surface);

// frame
gfx_api void vk_frame_begin(gfx_context_t* ctx, gfx_surface_t* surface, gfx_frame_t** out_frame);
gfx_api void vk_frame_end(gfx_frame_t* surface);

// buffer
gfx_api void vk_buffer_create(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
gfx_api void vk_buffer_update_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);
gfx_api void vk_buffer_destroy(gfx_context_t* ctx, gfx_buffer_t* buffer);

// shaders
gfx_api void     vk_shader_create(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
gfx_api uint32_t vk_shader_get_descriptor_set_count(gfx_shader_t* shader);
gfx_api uint64_t vk_uniform_location(gfx_shader_t* shader, const char* name);
gfx_api void     vk_shader_destroy(gfx_context_t* ctx, gfx_shader_t* buffer);

// descripor set
gfx_api void vk_descriptor_set_create(gfx_context_t* ctx, gfx_shader_t* shader, uint32_t set_idx, gfx_descriptor_set_t** descriptor);
gfx_api void vk_descriptor_set_destroy(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);
gfx_api void vk_descriptor_set_write_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
gfx_api void vk_descriptor_set_write_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset);
gfx_api void vk_descriptor_set_write_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
gfx_api void vk_descriptor_set_write_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);

// sampler
gfx_api void vk_sampler_create(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
gfx_api void vk_sampler_destroy(gfx_context_t* ctx, gfx_sampler_t* sampler);

// texture
gfx_api void vk_texture_create(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
gfx_api void vk_texture_destroy(gfx_context_t* ctx, gfx_texture_t* texture);

gfx_api void vk_texture_update_data(gfx_context_t* ctx, gfx_texture_t* texture, void* data, uint32_t size, uint32_t offset);
gfx_api void vk_texture_get_data(gfx_context_t* ctx, gfx_command_buffer_t* cmd);
gfx_api void vk_texture_generate_mipmap(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void vk_texture_blit(gfx_context_t* ctx, gfx_texture_t* src, gfx_texture_t* dst);
gfx_api void vk_texture_update_bindless(gfx_context_t* ctx, gfx_texture_t* texture, uint32_t idx);

// pipeline
gfx_api void vk_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void vk_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline);

// compute pipeline
gfx_api void vk_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** texture);
gfx_api void vk_destroy_compute_pipeline(gfx_context_t* ctx, gfx_pipeline_compute_t* pipeline);

// mesh pipeline
gfx_api void vk_create_mesh_pipeline(gfx_context_t* ctx, gfx_mesh_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void vk_destroy_mesh_pipeline(gfx_context_t* ctx, gfx_pipeline_t* desc);

// raytrace pipeline
gfx_api void vk_create_raytrace_pipeline(gfx_context_t* ctx, gfx_raytrace_pipeline_desc_t* desc, gfx_pipeline_raytrace_t** pipeline);
gfx_api void vk_destroy_raytrace_pipeline(gfx_context_t* ctx, gfx_pipeline_raytrace_t* pipeline);


gfx_api [[deprecated("internal usge only")]] void vk_cmd_create(gfx_context_t* ctx, gfx_command_buffer_t** cmd);
gfx_api [[deprecated("internal usge only")]] void vk_cmd_destroy(gfx_context_t* ctx, gfx_command_buffer_t* cmd);
gfx_api [[deprecated("internal usge only")]] void vk_cmd_begin(gfx_command_buffer_t* cmd);
gfx_api [[deprecated("internal usge only")]] void vk_cmd_end(gfx_command_buffer_t* cmd);
gfx_api [[deprecated("internal usge only")]] void vk_cmd_submit(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options);


gfx_api void vk_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_pass_info_t* pass);
             
gfx_api void vk_cmd_end_pass(gfx_command_buffer_t* cmd);
             
gfx_api void vk_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
             
gfx_api void vk_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
             
gfx_api void vk_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
             
gfx_api void vk_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, uint32_t slot, gfx_descriptor_set_t* descriptor);
             
gfx_api void vk_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
             
gfx_api void vk_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
             
gfx_api void vk_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
             
gfx_api void vk_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count, uint32_t vertex_offset);
             
gfx_api void vk_cmd_draw_indexed_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
             
gfx_api void vk_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);
             
gfx_api void vk_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
             
gfx_api void vk_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);
             
gfx_api void vk_cmd_push_marker(gfx_command_buffer_t* cmd, const char * marker);
             
gfx_api void vk_cmd_pop_marker(gfx_command_buffer_t* cmd);


/// wip mesh shaders
gfx_api void vk_cmd_draw_mesh_tasks(gfx_command_buffer_t* cmd, uint32_t task_count_x, uint32_t task_count_y, uint32_t task_count_z);
gfx_api void vk_cmd_draw_mesh_tasks_indirect(gfx_command_buffer_t* cmd, gfx_buffer_t* buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);

// wip ray tracing
gfx_api void vk_acceleration_structure_create(gfx_context_t* ctx, gfx_acceleration_structure_desc_t* desc, gfx_acceleration_structure_t ** out_acc);
gfx_api void vk_acceleration_structure_destroy(gfx_context_t* ctx, gfx_acceleration_structure_t acceleration_structure);

gfx_api void vk_sbt_create(gfx_context_t* ctx, gfx_sbt_desc_t* desc, gfx_sbt_t ** out);
gfx_api void vk_sbt_destroy(gfx_context_t* ctx, gfx_sbt_t sbt);

gfx_api void vk_cmd_build_acceleration_structure(gfx_command_buffer_t* cmd, gfx_acceleration_structure_t dst, gfx_acceleration_structure_t src);
gfx_api void vk_cmd_trace_rays(gfx_command_buffer_t* cmd, gfx_pipeline_raytrace_t* pipeline, gfx_sbt_t sbt, uint32_t width, uint32_t height, uint32_t depth);



// internal utility funcs
extern void    _vk_create_renderpass(vk_context_t* ctx, VkFormat format, VkFormat depthformat, VkSampleCountFlagBits samples, VkRenderPass* renderpass);
extern void     vk_debug_set_name(vk_context_t* ctx, uint64_t vkobject, VkObjectType type, const char* name);
extern void     vk_debug_set_texture_name(vk_context_t* ctx, vk_texture_t* texture, const char* name);
extern void     vk_debug_set_buffer_name(vk_context_t* ctx, vk_buffer_t* buffer, const char* name);
extern void     vk_debug_set_shader_name(vk_context_t* ctx, vk_shader_t* shader, const char* name);
#ifdef __cplusplus 
}
#endif

#endif //VULKAN_AVAILABLE

#endif //__gfx_vulkan_h__