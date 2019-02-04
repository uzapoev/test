#ifndef __VulkanRenderer_h__
#define __VulkanRenderer_h__

#include <stdint.h>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>


struct RenderStates{

};

struct VertexDeclaration{

};

class Vkrenderer
{
public:
    Vkrenderer(){}
    virtual             ~Vkrenderer(){}

    bool                initialize(long handle);
    void                release();

    void                begin();
    void                end();
    void                present();

    VertexDeclaration * create_vdecl();
    uint64_t            create_vb(void * data, size_t size, bool dynamic);
    uint64_t            create_ib(void * data, size_t size, bool dynamic);

    uint64_t            create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size);
    uint64_t            create_shader(void * vdata, size_t vsize, void * pdata, size_t psize );
    uint64_t            create_pipeline(uint64_t shader, RenderStates * rstates, VertexDeclaration * vdecl);
    uint64_t            create_renderpass(/**/);

    void                bind_pipeline(uint64_t pip);
    void                bind_vb(uint64_t vb);
    void                bind_ib(uint64_t ib);
    void                bind_texture(uint64_t texture);

    void                draw_array(int start, int end);
    void                draw_indexed(int start, int end);

protected:
    struct SwapchainInfo{
        VkFormat        format;
        uint16_t        width;
        uint16_t        height;
        VkSwapchainKHR  swapchain;
        uint32_t        image_count;
        VkImage         images[8];
        VkImageView     image_views[8];
    };

    static VkInstance          vk_create_instance(bool isdebug);
    static VkPhysicalDevice    vk_create_physdevice(VkInstance instance);
    static VkSurfaceKHR        vk_create_surface(VkInstance instance, long hanle);
    static VkDevice            vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface);
    static SwapchainInfo       vk_create_swapchain(VkPhysicalDevice physdevice, VkDevice device, VkSurfaceKHR surface);
private:
    VkInstance          m_instance = nullptr;
    VkPhysicalDevice    m_physicaldevice = nullptr;
    VkDevice            m_device = nullptr;
    VkSurfaceKHR        m_surface = 0;

    SwapchainInfo       m_swapchain;

    VkQueue                         m_graphicsQueue = 0;
    VkQueue                         m_presentQueue = 0;
    VkSemaphore                     m_imageAvailableSemaphore = 0;
    VkSemaphore                     m_renderingFinishedSemaphore = 0;

    VkRenderPass                    m_renderPass = 0;
    VkCommandPool                   m_commandPool = 0;
    VkCommandPool                   m_transientCommandPool = 0;

    uint32_t                        m_currCmdBuffer = 0;
    std::vector<VkCommandBuffer>    m_commandBuffers;
    std::vector<VkFence>            m_commandBufferFences;
    std::vector<VkSemaphore>        m_commandBufferDrawCompleteSemaphores;
    std::vector<VkFramebuffer>      m_swapChainFramebuffers;
};

#endif