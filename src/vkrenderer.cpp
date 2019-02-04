#include "vkrenderer.h"

#pragma comment(lib, "Lib32/vulkan-1.lib")

#if defined(_WIN32)
    #define SURFACE_EXTENSION_NAME      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
    #define SURFACE_EXTENSION_NAME      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(_DIRECT2DISPLAY)
    #define SURFACE_EXTENSION_NAME      VK_KHR_DISPLAY_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    #define SURFACE_EXTENSION_NAME      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(__linux__)
    #define SURFACE_EXTENSION_NAME      VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    #define SURFACE_EXTENSION_NAME      VK_MVK_IOS_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    #define SURFACE_EXTENSION_NAME      VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#endif

// The callback returns a VkBool32 that indicates to the calling layer if the Vulkan call should be aborted or not. 
// Applications should always return VK_FALSE so that they see the same behavior with and without validation layers enabled.
// If the application returns VK_TRUE from its callback and the Vulkan call being aborted returns a VkResult, the layer will return VK_ERROR_VALIDATION_FAILED_EXT
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
    const char * msgtype = "";
    switch (flags)
    {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:           msgtype = "info"; break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:               msgtype = "warn"; break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:   msgtype = "perf"; break;
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:                 msgtype = "err "; break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:                 msgtype = "dbg "; break;
        default:                                            msgtype = "    "; break;
    }

    printf("\nvk %s: %s,%s", msgtype, layerPrefix, msg);
    return VK_FALSE;
}

/*
static VKAPI_ATTR VkBool32 VKAPI_CALL _vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*pCallbackData,
    void* pUserData)
{
    printf("\nvk %s", pCallbackData->pMessage);
    return VK_FALSE;
}
*/


bool Vkrenderer::initialize(long handle)
{
    m_instance       = vk_create_instance(true);
    m_physicaldevice = vk_create_physdevice(m_instance);
    m_surface        = vk_create_surface(m_instance, handle);
    m_device         = vk_create_device(m_physicaldevice, m_surface);
    m_swapchain      = vk_create_swapchain(m_physicaldevice, m_device, m_surface);

    //todo: refactor bullshit below
    // create_renderpasses
    // create_framebuffer( union with renderpass??)
    // create_cmdbuffers
    vkGetDeviceQueue(m_device, 0/*graphicsQueueFamily*/, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, 0/*presentQueueFamily*/, 0, &m_presentQueue);

    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_renderingFinishedSemaphore) != VK_SUCCESS)
    {
        printf("\nfailed to create semaphores");
        return false;
    }

    m_commandBuffers.resize(m_swapchain.image_count);

    VkCommandPoolCreateInfo poolInfo = {};
    {
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = 0;////!!!graphicsFamily;
    }

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        printf("failed to create command pool!");
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    {
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();
    }

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        printf("failed to allocate command buffers!");
        return false;
    }

    {
        VkAttachmentDescription colorAttachment = {};
        {
            colorAttachment.format = m_swapchain.format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        VkAttachmentReference colorAttachmentRef = {};
        {
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass = {};
        {
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
        }

        VkSubpassDependency dependency = {};
        {
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        VkRenderPassCreateInfo renderPassInfo = {};
        {
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;
        }

        if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        {
            printf("failed to create render pass!");
            return false;
        }
    }

    {
        m_swapChainFramebuffers.resize(m_swapchain.image_count);

        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++)
        {
            VkImageView attachments[] = {
                m_swapchain.image_views[i]
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain.width;
            framebufferInfo.height = m_swapchain.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                printf("failed to create framebuffer!");
                return false;
            }
        }
    }

    return true;
}



void Vkrenderer::release()
{

}

void Vkrenderer::begin()
{
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_currCmdBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    {
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VkCommandBuffer cmd = m_commandBuffers[m_currCmdBuffer];
    result = vkBeginCommandBuffer(cmd, &commandBufferBeginInfo);

    VkClearValue clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    VkRenderPassBeginInfo renderPassInfo = {};
    {
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[m_currCmdBuffer];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent.width = m_swapchain.width;
        renderPassInfo.renderArea.extent.height = m_swapchain.height;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
    }
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Vkrenderer::end()
{
    VkCommandBuffer cmdBuf = m_commandBuffers[m_currCmdBuffer];

    vkCmdEndRenderPass(cmdBuf);
    VkResult result = vkEndCommandBuffer(cmdBuf);

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    {
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = &waitStages;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_renderingFinishedSemaphore;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf;
    }
    result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {};
    {
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderingFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain.swapchain;
        presentInfo.pImageIndices = &m_currCmdBuffer;
    }

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    result = vkQueueWaitIdle(m_presentQueue);
}

void Vkrenderer::present()
{
    vkDeviceWaitIdle(m_device);
}

VertexDeclaration * Vkrenderer::create_vdecl()
{
    return nullptr;
}

uint64_t Vkrenderer::create_vb(void * data, size_t size, bool dynamic)
{
    return 0;
}

uint64_t Vkrenderer::create_ib(void * data, size_t size, bool dynamic)
{
    return 0;
}

uint64_t Vkrenderer::create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size)
{
    return 0;
}

uint64_t Vkrenderer::create_shader(void * vdata, size_t vsize, void * pdata, size_t psize)
{
    return 0;
}

uint64_t Vkrenderer::create_pipeline(uint64_t shader, RenderStates * rstates, VertexDeclaration * vdecl)
{
    return 0;
}

uint64_t Vkrenderer::create_renderpass(/**/)
{
    return 0;
}

void Vkrenderer::bind_pipeline(uint64_t pip)
{

}
void Vkrenderer::bind_vb(uint64_t vb)
{

}
void Vkrenderer::bind_ib(uint64_t ib)
{

}
void Vkrenderer::bind_texture(uint64_t texture)
{

}

void Vkrenderer::draw_array(int start, int end)
{

}

void Vkrenderer::draw_indexed(int start, int end)
{

}




VkInstance Vkrenderer::vk_create_instance(bool isdebug)
{
    char *instance_validation_layers_alt1[] = { "VK_LAYER_LUNARG_standard_validation" };

    char *instance_validation_layers_alt2[] = { "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects" };

    std::vector<char*> enabledInstanceLayers;
    std::vector<char*> enabledInstanceExtensions;

    enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    enabledInstanceExtensions.push_back(SURFACE_EXTENSION_NAME);
    if (isdebug){
        enabledInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME/*" VK_EXT_debug_report "*/);
        enabledInstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
    }

    VkApplicationInfo vkAppInfo = {};
    {
        vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vkAppInfo.apiVersion = VK_MAKE_VERSION(1, 0, 5);

        vkAppInfo.pApplicationName = "vksample";
        vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        vkAppInfo.pEngineName = "No Engine";
        vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    }

    VkInstanceCreateInfo info = {};
    {
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &vkAppInfo;
        info.enabledExtensionCount = enabledInstanceExtensions.size();
        info.ppEnabledExtensionNames = enabledInstanceExtensions.data();

        info.enabledLayerCount = enabledInstanceLayers.size();
        info.ppEnabledLayerNames = enabledInstanceLayers.data();
    }

    /*VkDebugUtilsMessengerCreateInfoEXT dbg_info;
    if (isdebug) {
        auto messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_info.pNext = NULL;
        dbg_info.flags = 0;
        dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_info.messageType = messageType;
        dbg_info.pfnUserCallback = &_vkDebugUtilsMessengerCallbackEXT;
        dbg_info.pUserData = nullptr;
        info.pNext = &dbg_info;
    }
    */

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&info, nullptr, &instance);

    auto vkDbgReportCallbackFunc = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (isdebug && vkDbgReportCallbackFunc != nullptr)
    {
        auto flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT
            | VK_DEBUG_REPORT_DEBUG_BIT_EXT;

        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        {
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            createInfo.flags = flags;
            createInfo.pfnCallback = vkDebugCallback;
        }
        VkDebugReportCallbackEXT callback = 0;
        vkDbgReportCallbackFunc(instance, &createInfo, NULL, &callback);
    }

    return instance;
}

VkPhysicalDevice Vkrenderer::vk_create_physdevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

   // if (deviceCount == 1)
   //     return devices.front();

    uint32_t outputGraphicsQueueIndex = 0;
    VkPhysicalDevice physicalDevice = devices.front(); // set device by default
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        VkPhysicalDeviceProperties prop = {};
        vkGetPhysicalDeviceProperties(devices[i], &prop);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
        {
            if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                physicalDevice = devices[i];
            }
        }
    }

    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDeviceMemoryProperties memproperies;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memproperies);

    return physicalDevice;
}


VkSurfaceKHR Vkrenderer::vk_create_surface(VkInstance instance, long handle)
{
    VkSurfaceKHR surface = 0;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = (HWND)handle;
    VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VkAndroidSurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.window = (struct ANativeWindow *)(handle);
    VkResult result = vkCreateAndroidSurfaceKHR(demo->inst, &createInfo, NULL, &surface);
#else
    assert(false);
    static_assert(false, "not implemented");
#endif
    return surface;
}

VkDevice Vkrenderer::vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface)
{
    uint32_t queueFamilyPropertyCount = 0;
    VkBool32 supportsPresent[8] = { 0 };
    VkQueueFamilyProperties queueFamilyProperties[8] = { 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queueFamilyPropertyCount, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(physdevice, &queueFamilyPropertyCount, queueFamilyProperties);

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(physdevice, i, surface, &supportsPresent[i]);

        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
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
        printf("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
        return false;
    }

    bool separate_present_queue = (graphicsQueueFamilyIndex != presentQueueFamilyIndex);

    //create device
    float queue_priorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo queues[2] = {};
    {
        queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[0].queueFamilyIndex = graphicsQueueFamilyIndex;
        queues[0].queueCount = 1;
        queues[0].pQueuePriorities = queue_priorities;

        queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[1].queueFamilyIndex = presentQueueFamilyIndex;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = queue_priorities;
    }

    const char* deviceSwapchainExtension[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const char* device_validation_layers[] = { "VK_LAYER_LUNARG_mem_tracker", "VK_LAYER_GOOGLE_unique_objects"/*, "VK_LAYER_RENDERDOC_Capture" */ };

    VkDeviceCreateInfo device_info = {};
    {
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = NULL;
        device_info.queueCreateInfoCount = separate_present_queue ? 2 : 1;
        device_info.pQueueCreateInfos = queues;

        device_info.enabledLayerCount = _countof(device_validation_layers);
        device_info.ppEnabledLayerNames = device_validation_layers;

        device_info.enabledExtensionCount = _countof(deviceSwapchainExtension);
        device_info.ppEnabledExtensionNames = deviceSwapchainExtension;
        device_info.pEnabledFeatures = NULL;  // If specific features are required, pass them in here
    }
    VkDevice device = nullptr;
    auto err = vkCreateDevice(physdevice, &device_info, NULL, &device);
    return device;
}


Vkrenderer::SwapchainInfo Vkrenderer::vk_create_swapchain(VkPhysicalDevice physdevice, VkDevice device, VkSurfaceKHR surface)
{
    uint32_t formatCount = 0;
    VkSurfaceFormatKHR surfaceFormats[256] = {};

    vkGetPhysicalDeviceSurfaceFormatsKHR(physdevice, surface, &formatCount, NULL);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physdevice, surface, &formatCount, surfaceFormats);

    uint32_t presentModeCount = 0;
    VkPresentModeKHR presentModes[256] = {};
    vkGetPhysicalDeviceSurfacePresentModesKHR(physdevice, surface, &presentModeCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physdevice, surface, &presentModeCount, presentModes);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physdevice, surface, &surfaceCapabilities);

    auto surfaceFormat = surfaceFormats[0];
    uint32_t imageCount = surfaceCapabilities.minImageCount;

    // Finally, create the swap chain
    VkSwapchainCreateInfoKHR createInfo = {};
    {
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = surfaceCapabilities.currentExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
        createInfo.preTransform = surfaceCapabilities.currentTransform; //VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;; ///!!! presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;/**/
    }
    VkSwapchainKHR swapchain = 0;
    VkResult res = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);

    SwapchainInfo scinfo = {};
        scinfo.format = surfaceFormat.format;
        scinfo.width = surfaceCapabilities.currentExtent.width;
        scinfo.height = surfaceCapabilities.currentExtent.height;
        scinfo.swapchain = swapchain;

    vkGetSwapchainImagesKHR(device, swapchain, &scinfo.image_count, NULL);
    vkGetSwapchainImagesKHR(device, swapchain, &scinfo.image_count, scinfo.images);
    for (size_t i = 0; i < scinfo.image_count; i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = scinfo.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &scinfo.image_views[i]) != VK_SUCCESS) {
            return{};
        }
    }
    return scinfo;
}