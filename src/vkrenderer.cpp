#include "vkrenderer.h"

#include "spirvanalyzer.h"
#pragma comment(lib, "Lib32/vulkan-1.lib")

#include <vulkan/spirv.h>

#if defined(_WIN32)
    #define SURFACE_EXTENSION_NAME      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
    #define SURFACE_EXTENSION_NAME      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    #define SURFACE_EXTENSION_NAME      VK_MVK_IOS_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    #define SURFACE_EXTENSION_NAME      VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#endif


static const VkFormat _vformat2vkformat[eVertexFormat_Count] =
{
    VK_FORMAT_UNDEFINED, //eVertexFormat_Invalid
    VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT,
    VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT,  //half floats
    VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16A16_SINT,    // short
    VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16A16_UINT,    // ushort
    VK_FORMAT_R8G8B8A8_UINT    // byte
};

static const uint16_t _vformatstrides[eVertexFormat_Count] =
{
    0, //eVertexFormat_Invalid
    4, 8, 12, 16,
    4, 8, //half floats
    4, 8, // short
    4, 8, // ushort
    4    // byte4
};

static_assert(sizeof(float) == 4, "wrong float size");
static_assert(sizeof(short) == 2, "wrong short size");
static_assert(_countof(_vformat2vkformat) == eVertexFormat_Count, "incorrect array size");
static_assert(_countof(_vformatstrides) == eVertexFormat_Count, "incorrect array size");




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

static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = 0;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        printf("failed to create texture image view!");
        return 0;
    }
    return imageView;
}

static VkShaderModule createShaderModule(VkDevice device, const char * code, size_t size)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

    VkShaderModule shaderModule = 0;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module!");
        return 0;
    }

    return shaderModule;
}


uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    printf("failed to find suitable memory type!");
    return 0;
}

static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        printf("failed to create buffer!");
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        printf("failed to allocate buffer memory!");
        return;
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

static void copyBuffer(VkDevice device, VkCommandPool cmdpool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdpool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    /////////////////////////////

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);


    ///////////
    //endSingleTimeCommands(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, 0/*graphicsQueueFamily*/, 0, &graphicsQueue);

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, cmdpool, 1, &commandBuffer);
}


namespace vkPipelineBuildHelper
{
    static VkPipelineViewportStateCreateInfo createVierwportStateInfo(int w, int h)
    {
        static VkViewport viewport = { 0, 0, w, h, 0, 1 };
        static VkRect2D scissor = { { 0, 0 }, { w, h } };

        viewport.width = static_cast<float>(w);
        viewport.height = static_cast<float>(h);

        VkPipelineViewportStateCreateInfo viewportState = {};
        {
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;
        }
        return viewportState;
    }

    static VkPipelineMultisampleStateCreateInfo createMultismapleInfo()
    {
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        {
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        }
        return multisampling;
    }
    static VkPipelineRasterizationStateCreateInfo createRasterizationSateInfo()
    {
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        {
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
        }
        return rasterizer;
    }
    static VkPipelineDepthStencilStateCreateInfo createDepthStencilStateInfo()
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        {
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
        }
        return depthStencil;
    }

    static VkPipelineColorBlendStateCreateInfo createColorBlendStateInfo()
    {
        static VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        {
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        {
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;
        }
        return colorBlending;
    }
};



bool Vkrenderer::initialize(long handle)
{
    bool debug = true;
    m_instance       = vk_create_instance(debug);
    m_physicaldevice = vk_create_physdevice(m_instance);
    m_surface        = vk_create_surface(m_instance, handle);
    m_device         = vk_create_device(m_physicaldevice, m_surface);
    m_swapchain      = vk_create_swapchain(m_physicaldevice, m_device, m_surface);
    m_renderPass     = vk_create_renderpass(m_device, m_swapchain.format, VK_FORMAT_D24_UNORM_S8_UINT); 

    VkPhysicalDeviceFeatures features = {};
    VkPhysicalDeviceMemoryProperties memproperies = {};
    VkPhysicalDeviceProperties properties = {};

    vkGetPhysicalDeviceProperties(m_physicaldevice, &properties);
    vkGetPhysicalDeviceFeatures(m_physicaldevice, &features);
    vkGetPhysicalDeviceMemoryProperties(m_physicaldevice, &memproperies);
    
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

    m_commandbuffers.resize(m_swapchain.image_count + 2);

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
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = (uint32_t)m_commandbuffers.size();
    }

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandbuffers.data()) != VK_SUCCESS)
    {
        printf("failed to allocate command buffers!");
        return false;
    }

    {
        m_framebuffers.resize(m_swapchain.image_count);

        for (size_t i = 0; i < m_framebuffers.size(); i++)
        {
            VkImageView attachments[] = {
                m_swapchain.image_views[i],
                m_swapchain.depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = (m_swapchain.depthImageView == 0)?1:2;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain.width;
            framebufferInfo.height = m_swapchain.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
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

    VkCommandBuffer cmd = m_commandbuffers[m_currCmdBuffer];
    result = vkBeginCommandBuffer(cmd, &commandBufferBeginInfo);

    VkClearValue clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    VkRenderPassBeginInfo renderPassInfo = {};
    {
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_framebuffers[m_currCmdBuffer];
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
    VkCommandBuffer cmdBuf = m_commandbuffers[m_currCmdBuffer];

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


uint64_t Vkrenderer::create_vdecl(VertexAttribute * atribs, size_t count)
{
    assert(atribs && count && (count < eVertexAttrib_Count));

    uint32_t flags = 0;
    uint32_t offset = 0;

    VDeclaration decl;
    for (size_t i = 0; i < count; ++i)
    {
        long attrflag = 1 << atribs[i].type;
        if ((flags &  attrflag) == attrflag){
            printf("\nerror: duplicated vertex attributes");
            return 0;
        }
        flags = flags | (1 << atribs[i].type);
        decl.descriptions[i].binding = 0;

        decl.descriptions[i].location = atribs[i].type; ///!!!
        decl.descriptions[i].format = _vformat2vkformat[atribs[i].format];
        decl.descriptions[i].offset = offset;

        offset += _vformatstrides[atribs[i].format];
    }
    decl.count = count;

    decl.bining.binding = 0;
    decl.bining.stride = offset;//sizeof(Vertex);
    decl.bining.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    m_vdecls.push_back(decl);

    return makeresource(eResurceType_vdecl, m_vdecls.size(), flags);
}


uint64_t Vkrenderer::create_vb(void * data, size_t size, bool dynamic)
{
   /* VkBuffer vertexBuffer = NULL;
    VkDeviceMemory vertexBufferMemory = NULL;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicaldevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_device, vertexBuffer, vertexBufferMemory, 0);

    void *tmpdata = NULL;
    vkMapMemory(m_device, vertexBufferMemory, 0, size, 0, &tmpdata);
    memcpy(tmpdata, data, size);
    vkUnmapMemory(m_device, vertexBufferMemory);

    m_buffers.push_back(vertexBuffer);
    */
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(m_device, m_physicaldevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* staging_data;
    vkMapMemory(m_device, stagingBufferMemory, 0, size, 0, &staging_data);
    memcpy(staging_data, data, size);
    vkUnmapMemory(m_device, stagingBufferMemory);


    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    createBuffer(m_device, m_physicaldevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(m_device, m_commandPool, stagingBuffer, vertexBuffer, size);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    m_buffers.push_back(vertexBuffer);

    return makeresource(eResurceType_vb, m_buffers.size(), 0);
}


uint64_t Vkrenderer::create_ib(void * data, size_t size, bool dynamic)
{
    // auto buff =  create_bufer( data, size, dynamic);
    // m_buffers.push_back(buff);
    // return makeresource(eResurceType_ib, m_buffers.size(), 0);
    return 0;
}


uint64_t Vkrenderer::create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size)
{
    return 0;
}


uint64_t Vkrenderer::create_shader(void * vdata, size_t vsize, void * fdata, size_t fsize)
{
    VkShaderModule vertexShader = createShaderModule(m_device, (char*)vdata, vsize);
    VkShaderModule fragmentShader = createShaderModule(m_device, (char*)fdata, fsize);

    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    {
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertexShader;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragmentShader;
        shaderStages[1].pName = "main";
    }

    bool vertOk = SpirvAnalyzer::analyze(vdata, vsize);
    bool fragOk = SpirvAnalyzer::analyze(fdata, fsize);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    //vertex
    for (int i = 0; i < 0; ++i) {
        VkDescriptorSetLayoutBinding binding = {};
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      //  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        bindings.push_back(binding);
    }
    //fragment
    for (int i = 0; i < 0; ++i) {
        VkDescriptorSetLayoutBinding binding = {};
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();


    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    //   pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    Shader shader = { pipelineLayout, shaderStages[0], shaderStages[1] };
    m_shaders.emplace_back(shader);

    return makeresource(eResurceType_shader, m_shaders.size(), 0);
}

uint64_t Vkrenderer::create_pipeline(uint64_t vdecl, uint64_t shader, RenderStates * rstates)
{
    if ( (restype(vdecl) != eResurceType_vdecl) || (restype(shader) != eResurceType_shader)){
        assert(false);
    }

    resourserunion shaderunion = { shader };
    Shader * shaderinfo = &m_shaders[shaderunion.internalid - 1];

    VkPipelineLayout pipelineLayout = shaderinfo->pipline_layout;
    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        shaderinfo->vertex,
        shaderinfo->fragment
    };

    resourserunion vdeclunion = { vdecl };
    VDeclaration * decl = &m_vdecls[vdeclunion.internalid - 1];

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    {
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
     //   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
     //   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    int width = m_swapchain.width;
    int height = m_swapchain.height;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    {
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &decl->bining;

        vertexInputInfo.vertexAttributeDescriptionCount = decl->count;
        vertexInputInfo.pVertexAttributeDescriptions = decl->descriptions;
    }

    VkPipelineViewportStateCreateInfo       viewportState   = vkPipelineBuildHelper::createVierwportStateInfo(width, height);
    VkPipelineRasterizationStateCreateInfo  rasterizer      = vkPipelineBuildHelper::createRasterizationSateInfo();
    VkPipelineMultisampleStateCreateInfo    multisampling   = vkPipelineBuildHelper::createMultismapleInfo();
    VkPipelineColorBlendStateCreateInfo     colorBlending   = vkPipelineBuildHelper::createColorBlendStateInfo();
    VkPipelineDepthStencilStateCreateInfo   depthStencil    = vkPipelineBuildHelper::createDepthStencilStateInfo();

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    {
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pViewportState = &viewportState;

        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &depthStencil;

        pipelineInfo.pInputAssemblyState = &inputAssembly;

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = m_renderPass; ///!!! todo:
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    }

    VkPipeline pipeline = 0;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        printf("failed to create graphics pipeline!");
        return 0;
    }

    m_pipelines.push_back(pipeline);
    return makeresource(eResurceType_pipeline, m_pipelines.size(), 0);
}


uint64_t Vkrenderer::create_renderpass(/**/)
{
    return 0;
}


uint32_t Vkrenderer::uniform(uint64_t shader, const char * name)
{
    return 0;
}


void Vkrenderer::update_uniform(uint32_t id, const void *data)
{

}


void Vkrenderer::bind_pipeline(uint64_t pipeline)
{
    if (restype(pipeline) != eResurceType_pipeline){
        printf("");
        return;
    }

    resourserunion pipelineunion = { pipeline };
    auto graphicsPipeline= m_pipelines[pipelineunion.internalid - 1];

    VkCommandBuffer cmd = m_commandbuffers[m_currCmdBuffer];
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}


void Vkrenderer::bind_vb(uint64_t vb)
{
    if (restype(vb) != eResurceType_vb){
        printf("");
        return;
    }

    resourserunion vbunion = { vb };
    VkBuffer vertexBuffers[] = { m_buffers[vbunion.internalid-1] };
    VkDeviceSize offsets[] = { 0 };

    VkCommandBuffer cmd = m_commandbuffers[m_currCmdBuffer];
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
}

void Vkrenderer::bind_ib(uint64_t ib)
{
    resourserunion ibunion = { ib };
    VkBuffer vertexBuffers[] = { m_buffers[ibunion.internalid - 1] };
    VkDeviceSize offsets[] = { 0 };
}

void Vkrenderer::bind_texture(uint64_t texture)
{

}

void Vkrenderer::draw_array(int start_vert, int vert_count)
{
    VkCommandBuffer cmd = m_commandbuffers[m_currCmdBuffer];
    vkCmdDraw(cmd, vert_count, 1, start_vert, 0);
}

void Vkrenderer::draw_indexed(int start, int end)
{
    VkCommandBuffer cmd = m_commandbuffers[m_currCmdBuffer];
   // vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}




VkInstance Vkrenderer::vk_create_instance(bool isdebug)
{
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

    if (deviceCount == 1)
         return devices.front();

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
        return 0;
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

    const char* device_swapchain_extension[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const char* device_validation_layers[] = { "VK_LAYER_LUNARG_mem_tracker", "VK_LAYER_GOOGLE_unique_objects"/*, "VK_LAYER_RENDERDOC_Capture" */ };

    VkDeviceCreateInfo device_info = {};
    {
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = NULL;
        device_info.queueCreateInfoCount = separate_present_queue ? 2 : 1;
        device_info.pQueueCreateInfos = queues;

        device_info.enabledLayerCount = _countof(device_validation_layers);
        device_info.ppEnabledLayerNames = device_validation_layers;

        device_info.enabledExtensionCount = _countof(device_swapchain_extension);
        device_info.ppEnabledExtensionNames = device_swapchain_extension;
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
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;// VK_PRESENT_MODE_IMMEDIATE_KHR;// VK_PRESENT_MODE_FIFO_KHR;; ///!!! presentMode;
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
        VkImage image = scinfo.images[i];
        VkFormat format = surfaceFormat.format;

        scinfo.image_views[i] = createImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
  //  scinfo.depthImageView = createImageView(device, image, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    return scinfo;
}


VkRenderPass Vkrenderer::vk_create_renderpass(VkDevice device, VkFormat colorformat, VkFormat depthformat)
{
    VkAttachmentDescription attachments[2] = {};
    {
        // color attachment
        attachments[0].format = colorformat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // depth attachment
        attachments[1].format = depthformat;// VK_FORMAT_D24_UNORM_S8_UINT;//findDepthFormat();
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference attachmentRefs[2] = {};
    {
        // color
        attachmentRefs[0].attachment = 0;
        attachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        //depth
        attachmentRefs[1].attachment = 1;
        attachmentRefs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    {
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRefs[0];
        //       subpass.pDepthStencilAttachment = &attachmentRefs[0];
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
   // vkCmdPushConstants()
    VkRenderPassCreateInfo renderPassInfo = {};
    {
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;///_countof(attachments);
        renderPassInfo.pAttachments = attachments;

        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
    }

    VkRenderPass renderPass = 0;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        printf("failed to create render pass!");
        return 0;
    }
    return renderPass;
}