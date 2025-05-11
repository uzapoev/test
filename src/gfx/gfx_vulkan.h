#ifndef __gfx_vulkan_h__
#define __gfx_vulkan_h__

#include "gfx.h"

#ifdef VULKAN_AVAILABLE

#define VK_USE_PLATFORM_WIN32_KHR


#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
//#include <vulkan/vk_sdk_platform.h>
#include <spirv_cross/spirv.h>

#ifdef __cplusplus 
extern "C" {
#endif

#if __has_include(<vma/vk_mem_alloc.h>)
   // #include <vma/vk_mem_alloc.h>
#endif
 
#define   MAX_DESCRIPTOR_POOL_SET_SIZE      (1024)

 
struct vk_descriptor_pool_t;
struct vk_descriptor_set_t;
struct vk_command_buffer_t;
struct vk_buffer_t;
struct vk_texture_t;

typedef struct vk_context_t
{
    gfx_context_t                       handle;
    VkInstance                          instance            = nullptr;
    VkDevice                            device              = nullptr;
    VkPhysicalDevice                    physicaldevice      = nullptr;
    VkSurfaceKHR                        surface             = nullptr;
    uint32_t                            frame_idx           = 0;
    uint32_t                            frame_number        = 0;

    gfx_allocator_t                     allocator;

    struct { 
        uint32_t family;
        VkQueue  queue;
    }                                   graphics_queue,
                                        present_queue,
                                        compute_queue;
    VkSampleCountFlagBits               msaa_samples;

    VkExtensionProperties *             extensions;
    uint32_t                            extensions_count;

    VkPhysicalDeviceFeatures            device_features     = {};
    VkPhysicalDeviceMemoryProperties    memory_properties   = {};
    VkPhysicalDeviceProperties          device_properties   = {};

    struct {
        VkSemaphore                     image_available;        // Wait Semaphores
        VkSemaphore                     rendering_finished;     // Signal Semaphores
        VkFence                         wait_fence;
    } semaphores[2];

    VkSemaphore                         frame_timeline_semaphore;

    gfx_callback                        dbg_log                 = nullptr;

    vk_buffer_t*                        staging_buffer          = nullptr;
    gfx_sampler_t*                      default_sampler         = nullptr;
    vk_texture_t*                       default_texture         = nullptr;
    gfx_texture_t*                      default_storage_texture = nullptr;
    gfx_buffer_t*                       default_storage_buffer  = nullptr;

    VkRenderPass                        default_renderpass      = nullptr;


 #ifdef AMD_VULKAN_MEMORY_ALLOCATOR_H
    VmaAllocator                        vma_allocator;
 #endif

    vk_command_buffer_t*                cmd_buffer_pool[32];
    uint32_t                            cmd_pool_size;

    gfx_handle_pool_t*                  cmd_pool;
    gfx_handle_pool_t*                  sampler_pool;
    gfx_handle_pool_t*                  texture_pool;
    gfx_handle_pool_t*                  buffers_pool;
    gfx_handle_pool_t*                  shaders_pool;
    gfx_handle_pool_t *                 pipeline_pool;
    gfx_handle_pool_t *                 compute_pipeline_pool;
} vk_context_t;


typedef struct vk_pipeline_t {
    gfx_pipeline_t                      handle;

    struct vk_shader_t*                 shader;
//  render_states_t                     states;
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
    uint32_t                            memory_size;

    //gfx_linked_list_t*                consumers; // secriptor_sets
} vk_texture_t;


typedef struct vk_buffer_t {
    gfx_buffer_t                        handle;
    VkBuffer                            buffer;
    VkDeviceMemory                      memory;
    VkBufferUsageFlagBits               usage;

    bool                                mapped;
    uint32_t                            buffer_size;
    void*                               data_ptr;
} vk_buffer_t;


typedef struct vk_shader_t {
    gfx_shader_t                        handle;

    vk_context_t *                      ctx;
    const char*                         lable = nullptr;
    uint32_t                            stages_count;
    VkPipelineShaderStageCreateInfo     stages[gfx_shader_count];

    uint16_t                            hash;
    uint32_t                            uniform_count;
    gfx_uniform_t *                     uniforms;
    VkDescriptorSetLayoutBinding*       bindings;

    VkDescriptorSetLayout               layout;
    VkPipelineLayout                    pipeline_layout;

    vk_descriptor_pool_t *              pool;
    gfx_handle_pool_t *                 descriptor_set_pool;
} vk_shader_t;


typedef struct vk_render_target_t {
    gfx_render_target_t                 handle;
    VkFormat                            color_format;
    VkFormat                            depth_format;

    uint32_t                            color_attachment_count;
    vk_texture_t                        color_attachments[16];  // max attachment
    vk_texture_t                        depth_attachments;
    vk_texture_t                        resolve_attachments;    // surface swapchain if msaa

   // tr_render_target
    VkExtent2D                          extend;
    VkRenderPass                        renderpass;
    VkFramebuffer                       framebuffer;
} vk_render_target_t;


typedef struct vk_swapchain_t{
    gfx_swapchain_t                     handle;

    intptr_t                            window_handle;
    VkSwapchainKHR                      swapchain;
    VkRenderPass                        renderpass;
    VkFramebuffer                       framebuffers[4];
    VkExtent2D                          extend;
    vk_render_target_t                  target;
} vk_swapchain_t;


typedef struct vk_descriptor_pool_t {
    VkDescriptorPool                    pool;
    uint32_t                            capacity;
    uint32_t                            free_set_count;
    uint32_t                            next_free;

    vk_buffer_t *                       ubo_buffer;
    vk_descriptor_set_t *               sets;

    VkWriteDescriptorSet *              writes;
    struct vk_write_info_t*             write_infos;

} vk_descriptor_pool_t;


typedef struct vk_descriptor_set_t
{
    gfx_descriptor_set_t                handle;

    vk_shader_t *                       shader;
    vk_descriptor_pool_t *              pool;
    uint8_t *                           uboptr;

    VkBool32                            isfree;
    VkBool32                            dirty;
    uint32_t                            index_in_pool;
    VkDescriptorSet                     descriptor_set;

    VkWriteDescriptorSet *              writes;
    struct vk_write_info_t *            write_infos;

} vk_descriptor_set_t;


typedef struct vk_command_buffer_t {
    gfx_command_buffer_t    handle;

    vk_context_t *          ctx;
    uint32_t                thread_id;

    VkDevice                device;
    VkCommandPool           pool;
    VkCommandBuffer         cmd;

} vk_command_buffer_t;


gfx_api void     vk_create_renderer(gfx_settings_t* cfg, gfx_context_t** ctx);
gfx_api void     vk_destroy_renderer(gfx_context_t* ctx);

gfx_api void     vk_create_swapchain(gfx_context_t* ctx, intptr_t handle, gfx_swapchain_t** swapchain); // todo: add prefered options for surface format
gfx_api void     vk_destroy_swapchain(gfx_context_t* ctx, gfx_swapchain_t* swapchain); // todo: add prefered options for surface format
gfx_api void     vk_create_renderpass(gfx_context_t* ctx, VkFormat format, VkFormat depthformat, VkRenderPass * renderpass);

gfx_api int32_t  vk_acquire_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, gfx_render_target_t** target);
gfx_api void     vk_present_img(gfx_context_t* ctx, gfx_swapchain_t* swapchain, uint32_t idx);

gfx_api void     vk_create_buffer(gfx_context_t* ctx, gfx_buffer_desc_t* desc, gfx_buffer_t** buffer);
gfx_api void     vk_create_shader(gfx_context_t* ctx, gfx_shader_desc_t* desc, gfx_shader_t** shader);
gfx_api void     vk_create_sampler(gfx_context_t* ctx, gfx_sampler_desc_t* desc, gfx_sampler_t** sampler);
gfx_api void     vk_create_texture(gfx_context_t* ctx, gfx_texture_desc_t* desc, gfx_texture_t** texture);
gfx_api void     vk_create_pipeline(gfx_context_t* ctx, gfx_pipeline_desc_t* desc, gfx_pipeline_t** pipeline);
gfx_api void     vk_create_compute_pipeline(gfx_context_t* ctx, gfx_compute_pipeline_desc_t* desc, gfx_pipeline_compute_t** texture);
gfx_api void     vk_create_render_target(gfx_context_t* ctx, gfx_render_target_desc_t* desc, gfx_render_target_t** target);
gfx_api void     vk_create_descriptor_set(gfx_context_t* ctx, gfx_shader_t* shader, gfx_descriptor_set_t** descriptor);
gfx_api void     vk_create_cmd(gfx_context_t* ctx, gfx_command_buffer_t** cmd);

gfx_api void     vk_destroy_buffer(gfx_context_t* ctx, gfx_buffer_t* buffer);
gfx_api void     vk_destroy_shader(gfx_context_t* ctx, gfx_shader_t* buffer);
gfx_api void     vk_destroy_sampler(gfx_context_t* ctx, gfx_sampler_t* sampler);
gfx_api void     vk_destroy_texture(gfx_context_t* ctx, gfx_texture_t* texture);
gfx_api void     vk_destroy_pipeline(gfx_context_t* ctx, gfx_pipeline_t* pipeline);
gfx_api void     vk_destroy_render_target(gfx_context_t* ctx, gfx_render_target_t* _target);
gfx_api void     vk_destroy_descriptor_set(gfx_context_t* ctx, gfx_descriptor_set_t* descriptor);
gfx_api void     vk_destroy_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd);

gfx_api void     vk_update_buffer_data(gfx_context_t* ctx, gfx_buffer_t* buffer, void* data, uint32_t size, uint32_t offset);

gfx_api uint64_t vk_uniform_location(gfx_shader_t* shader, const char* name);
gfx_api void     vk_uniform_set_buffer_data(gfx_descriptor_set_t* set, uint64_t handle, void* data, uint32_t size);
gfx_api void     vk_uniform_set_buffer(gfx_descriptor_set_t* set, uint64_t handle, gfx_buffer_t* data, uint32_t offset);
gfx_api void     vk_uniform_set_texture(gfx_descriptor_set_t* set, uint64_t handle, gfx_texture_t* texture);
gfx_api void     vk_uniform_set_sampler(gfx_descriptor_set_t* set, uint64_t handle, gfx_sampler_t* sampler);

gfx_api void     vk_cmd_begin(gfx_command_buffer_t* cmd);
gfx_api void     vk_cmd_begin_pass(gfx_command_buffer_t* cmd, gfx_render_target_t* target);
gfx_api void     vk_cmd_end_pass(gfx_command_buffer_t* cmd);

gfx_api void     vk_cmd_scissor(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void     vk_cmd_viewport(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
gfx_api void     vk_cmd_bind_pipeline(gfx_command_buffer_t* cmd, gfx_pipeline_t* pipeline);
gfx_api void     vk_cmd_bind_descriptor_set(gfx_command_buffer_t* cmd, gfx_descriptor_set_t* descriptor);
gfx_api void     vk_cmd_bind_buffer_ib(gfx_command_buffer_t* cmd, gfx_index_format format, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void     vk_cmd_bind_buffer_vb(gfx_command_buffer_t* cmd, uint32_t slot, uint32_t offset, gfx_buffer_t* buffer);
gfx_api void     vk_cmd_draw(gfx_command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count);
gfx_api void     vk_cmd_draw_indexed(gfx_command_buffer_t* cmd, uint32_t idx_count, uint32_t first_idx, uint32_t instance_count);
gfx_api void     vk_cmd_dispatch_compute(gfx_command_buffer_t* cmd, uint32_t x, uint32_t y, uint32_t z);
gfx_api void     vk_cmd_buffer_barrier(gfx_command_buffer_t* cmd, gfx_buffer_t** buffers, uint32_t count, gfx_barrier src, gfx_barrier dst);
gfx_api void     vk_cmd_texture_barrier(gfx_command_buffer_t* cmd, gfx_texture_t** textures, uint32_t count, gfx_barrier src, gfx_barrier dst);


gfx_api void     vk_cmd_end(gfx_command_buffer_t* cmd);
gfx_api void     vk_submit_cmd(gfx_context_t* ctx, gfx_command_buffer_t* cmd, gfx_submit_options options);



extern void     vk_debug_set_name(vk_context_t* ctx, uint64_t vkobject, VkObjectType type, const char* name);
extern void     vk_debug_begin_region(vk_context_t* ctx, VkCommandBuffer cmd, const char* name, uint32_t color = 0xFFFFFFFF);
extern void     vk_debug_end_region(vk_context_t* ctx, VkCommandBuffer cmd);
extern void     vk_debug_set_texture_name(vk_context_t* ctx, vk_texture_t* texture, const char* name);
extern void     vk_debug_set_buffer_name(vk_context_t* ctx, vk_buffer_t* buffer, const char* name);
extern void     vk_debug_set_shader_name(vk_context_t* ctx, vk_shader_t* shader, const char* name);
#ifdef __cplusplus 
}
#endif

#endif
#endif