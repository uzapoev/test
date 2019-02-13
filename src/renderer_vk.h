#ifndef __RendererVulkan_h__
#define __RendererVulkan_h__

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PROTOTYPES

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#include <vector>

#include "renderer.h"

class Vkrenderer : public iRenderer
{
public:
    Vkrenderer(){}
    virtual             ~Vkrenderer(){}

    bool                        initialize(long handle);
    void                        release();

    void                        begin();
    void                        end();
    void                        present();

    uint64_t                    create_vdecl(VertexAttribute * atribs, size_t count);
    uint64_t                    create_vb(void * data, size_t size, bool dynamic);
    uint64_t                    create_ib(void * data, size_t size, bool dynamic);

    uint64_t                    create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size);
    uint64_t                    create_shader(void * vdata, size_t vsize, void * pdata, size_t psize );
    uint64_t                    create_pipeline(uint64_t vdecl, uint64_t shader, RenderStates * rstates /*uint64_t renderpass*/);
    uint64_t                    create_renderpass(/*colorformats * formats, siz_t count, VkFormat depthFormat*/);

    void                        destroy_resource(uint64_t id);

    uint32_t                    uniform(uint64_t shader, const char * name);
    void                        update_uniform(uint32_t id, const void *data);

    void                        bind_pipeline(uint64_t pip);
    void                        bind_vb(uint64_t vb);
    void                        bind_ib(uint64_t ib);
    void                        bind_texture(uint64_t texture);

    void                        draw_array(uint32_t start_vert, uint32_t vert_count);
    void                        draw_indexed(uint32_t idxcount);

protected:
    struct SwapchainInfo
    {
        VkFormat        format;
        uint16_t        width;
        uint16_t        height;
        VkSwapchainKHR  swapchain;
        uint32_t        image_count;
        VkImage         images[8];
        VkImageView     image_views[8];
        VkImageView     depthImageView;// = createImageView(depthImage, depthFormat);
    };

    struct DeviceInfo
    {
        VkDevice            device = nullptr;
        VkCommandPool       cmdpool = 0;

        VkQueue             graphics_queue = 0;
        VkQueue             present_queue = 0;
        VkSemaphore         image_available_semaphore = 0;
        VkSemaphore         rendering_finished_semaphore = 0;
    };

    struct VDeclaration
    {
        size_t                              count;
        VkVertexInputAttributeDescription   descriptions[eVertexAttrib_Count];
        VkVertexInputBindingDescription     bining;
    };

    struct Shader
    {
        VkPipelineLayout                    pipline_layout;
        VkPipelineShaderStageCreateInfo     vertex;
        VkPipelineShaderStageCreateInfo     fragment;
        VkPipelineShaderStageCreateInfo     geometry;
    };

    struct VkResource
    {
        eResourceType type;
        union 
        {/*
            struct Shader
            {
                VkPipelineLayout                    pipline_layout;
                VkPipelineShaderStageCreateInfo     vertex;
                VkPipelineShaderStageCreateInfo     fragment;
                VkPipelineShaderStageCreateInfo     geometry;
            } shader;

            struct VDeclaration
            {
                size_t                              count;
                VkVertexInputAttributeDescription   descriptions[eVertexAttrib_Count];
                VkVertexInputBindingDescription     bining;
            } vdecl;*/

            struct Buffer
            {
                VkBuffer        buffer;
                VkDeviceMemory  memory;
            }buffer;

            struct Image
            {
                VkImage         image;
                VkImageView     view;
            }image;
        };
    };

private:

    static VkInstance           vk_create_instance(bool isdebug);
    static VkPhysicalDevice     vk_create_physdevice(VkInstance instance);
    static VkSurfaceKHR         vk_create_surface(VkInstance instance, long hanle);
    static VkDevice             vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface);

    static SwapchainInfo        vk_create_swapchain(VkPhysicalDevice physdevice, VkDevice device, VkSurfaceKHR surface);
    static VkRenderPass         vk_create_renderpass(VkDevice device, VkFormat format, VkFormat depthformat);


    static void                 vk_create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                                 VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    static void                 vk_destroy_buffer(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory);

    static void                 vk_copy_buffer(VkDevice device, VkCommandPool cmdpool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

//  static Image                ck_create_image(Device * device);
//  static Buffer               vk_create_buffer(Device * device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
//  static Buffer               vk_copy_buffer(Device * device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
//  static VkCommandBuffer      vk_begin_singletime_cmd(Device * device);
//  static void                 vk_end_singletime_cmd(Device * device, VkCommandBuffer * cmd);

private:
    VkInstance                  m_instance = nullptr;
    VkPhysicalDevice            m_physicaldevice = nullptr;
    VkDevice                    m_device = nullptr;
    VkSurfaceKHR                m_surface = 0;

    DeviceInfo                  m_deviceinfo;
    SwapchainInfo               m_swapchain;

    VkQueue                     m_graphicsQueue = 0;
    VkQueue                     m_presentQueue = 0;
    VkSemaphore                 m_imageAvailableSemaphore = 0;
    VkSemaphore                 m_renderingFinishedSemaphore = 0;

    VkRenderPass                m_renderPass = 0;
    VkCommandPool               m_commandPool = 0;
    //VkCommandPool             m_transientCommandPool = 0;

    uint32_t                        m_currCmdBuffer = 0;
    std::vector<VkCommandBuffer>    m_commandbuffers;
    std::vector<VkFramebuffer>      m_framebuffers;

    std::vector<Shader>         m_shaders;
    std::vector<VDeclaration>   m_vdecls;

    std::vector<VkResource>     m_resources;
    
    std::vector<VkPipeline>     m_pipelines;
};

#endif