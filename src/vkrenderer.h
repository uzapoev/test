#ifndef __VulkanRenderer_h__
#define __VulkanRenderer_h__

#include <stdint.h>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_PROTOTYPES


#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

enum eResurceType : uint8_t 
{
    eResurceType_invalid = 0,
    eResurceType_vdecl,
    eResurceType_vb,
    eResurceType_ib,
    eResurceType_shader,
    eResurceType_texture,
    eResurceType_sampler,
    eResurceType_pipeline,
    eResurceType_renerpass,
    eResurceType_fbo, //todo: is it part of renerpass ???
};

enum eVertexFormat
{
    eVertexFormat_Invalid = 0,    // MTLVertexFormatInvalid = 0,
    eVertexFormat_float1,         // float
    eVertexFormat_float2,         // vec2f
    eVertexFormat_float3,         // vec3f
    eVertexFormat_float4,         // vec4f

    eVertexFormat_half2,          // Two 16 bit floating value
    eVertexFormat_half4,          // Four 16 bit floating value

    eVertexFormat_short2,         // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    eVertexFormat_short4,         // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)

    eVertexFormat_ushort2,        // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    eVertexFormat_ushort4,        // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)

    eVertexFormat_byte4,          // Each of 4 bytes is normalized by dividing to 255.0
    eVertexFormat_Count
};

enum eVertexAttrib
{
    eVertexAttrib_Invalid = -1,
    eVertexAttrib_Position = 0,   //! vertex position (3 float)
    eVertexAttrib_Color,            //! vertex color (4 byte)
    eVertexAttrib_Normal,           //! vertex normal (3 float)
    eVertexAttrib_Tangent,          //! vertex tangent (3 float)
    eVertexAttrib_TexCoords0,       //! texture 0 layer
    eVertexAttrib_TexCoords1,       //! texture 1 layer
    eVertexAttrib_TexCoords2,       //! texture 2 layer
    eVertexAttrib_TexCoords3,       //! texture 3 layer
    eVertexAttrib_TexCoords4,       //! texture 4 layer
    eVertexAttrib_TexCoords5,       //! texture 5 layer
    eVertexAttrib_TexCoords6,       //! texture 6 layer
    eVertexAttrib_TexCoords7,       //! texture 7 layer
    eVertexAttrib_BoneWeight,       //! bone matrix weight(float 4)(max 4 bones per vertex)
    eVertexAttrib_BoneIndices,      //! bone matrix indexes(float 4)(max 4 bones per vertex)
    eVertexAttrib_Count
};

enum eCullMode
{
    eCull_None,
    eCull_Backface,
    eCull_Frontface
};


enum eCompareMode
{
    eCompareMode_Never,
    eCompareMode_Less,
    eCompareMode_Equal,
    eCompareMode_LessEqual,
    eCompareMode_Greater,
    eCompareMode_NotEqual,      // NOTEQUAL,
    eCompareMode_GreaterEqual,  // GREATEREQUAL
    eCompareMode_Always,
};


enum eStencilOp
{
    eStencilOp_Zero,        // D3D10_STENCIL_OP_ZERO            D3DSTENCILOP_ZERO        GL_ZERO
    eStencilOp_Keep,        // D3D10_STENCIL_OP_KEEP            D3DSTENCILOP_KEEP        GL_KEEP
    eStencilOp_Replace,     // D3D10_STENCIL_OP_REPLACE        D3DSTENCILOP_REPLACE      GL_REPLACE
    eStencilOp_Incr,        // D3D10_STENCIL_OP_INCR_SAT        D3DSTENCILOP_INCRSAT     GL_INCR
    eStencilOp_IncrWarp,    // D3D10_STENCIL_OP_INCR            D3DSTENCILOP_INCR        GL_INCR_WRAP
    eStencilOp_Decr,        // D3D10_STENCIL_OP_DECR_SAT        D3DSTENCILOP_DECRSAT     GL_DECR
    eStencilOp_DecrWarp,    // D3D10_STENCIL_OP_DECR            D3DSTENCILOP_DECR        GL_DECR_WRAP
    eStencilOp_Invert,      // D3D10_STENCIL_OP_INVERT        D3DSTENCILOP_INVERT        GL_INVERT
};


enum eBlendMode
{
    eBlendMode_Zero,
    eBlendMode_One,
    eBlendMode_SrcColor,
    eBlendMode_InvSrcColor,
    eBlendMode_SrcAlpha,
    eBlendMode_InvSrcAlpha,
    eBlendMode_DestAlpha,
    eBlendMode_InvDestAlpha,
    eBlendMode_DestColor,
    eBlendMode_InvDestColor,
};


enum eBlendOp
{
    kBlendOp_Add,
    kBlendOp_Min,
    kBlendOp_Max,
    kBlendOp_Subtract,
    kBlendOp_RevSubstract,
};


struct RenderStates
{
    struct BlendState
    {
        bool        enable = false;
        eBlendMode  srcColor = eBlendMode_One;  // srcColor
        eBlendMode  dstColor = eBlendMode_One;  // dstColor
        eBlendOp    opColor  = kBlendOp_Add;    // opColor

        eBlendMode  alphaDst = eBlendMode_One;  // srcAlpha
        eBlendMode  alphaSrc = eBlendMode_One;  // dstAlpha
        eBlendOp    opAlpha  = kBlendOp_Add;    // opAlpha
    }blend;

    struct DepthState
    {
        bool            enable = true;
        bool            write = true;
        eCompareMode    mode = eCompareMode_LessEqual;
    }depth;

    struct StencilState
    {
        bool            enable = false;
        eCompareMode    func = eCompareMode_Always;
        uint8_t         ref = 0;
        uint8_t         pass = 0xFF;

        eStencilOp      opPass = eStencilOp_Keep;
        eStencilOp      opFail = eStencilOp_Keep;
        eStencilOp      opZFail = eStencilOp_Keep;
    }stencil;

    struct ColorMask
    {
        bool red = true;
        bool green = true;
        bool blue = true;
        bool alpha = true;
    }color;

    eCullMode culling = eCull_None;
};


struct VertexAttribute
{
    eVertexAttrib   type;
    eVertexFormat   format;
};


class Vkrenderer
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

    uint32_t                    uniform(uint64_t shader, const char * name);
    void                        update_uniform(uint32_t id, const void *data);

    void                        destroy_resource(uint64_t id);

    void                        bind_pipeline(uint64_t pip);
    void                        bind_vb(uint64_t vb);
    void                        bind_ib(uint64_t ib);
    void                        bind_texture(uint64_t texture);

    void                        draw_array(int start_vert, int vert_count);
    void                        draw_indexed(int start, int end);

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

    union resourserunion
    {
        uint64_t id;
        struct{
            uint8_t  type;
            uint8_t  reserved;
            uint16_t internalid;
            uint32_t userdata;
        };
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

private:
    static eResurceType restype(uint64_t id)
    {
        resourserunion u = { id };
        return static_cast<eResurceType>(u.type);
    }

    static uint64_t makeresource(eResurceType type, uint16_t internalid, uint32_t userdata)
    {
        resourserunion u = {};
        u.type = type;
        u.internalid = internalid;
        u.userdata = userdata;
        return u.id;
    }

public:

    static VkInstance           vk_create_instance(bool isdebug);
    static VkPhysicalDevice     vk_create_physdevice(VkInstance instance);
    static VkSurfaceKHR         vk_create_surface(VkInstance instance, long hanle);
    static VkDevice             vk_create_device(VkPhysicalDevice physdevice, VkSurfaceKHR surface);

    static SwapchainInfo        vk_create_swapchain(VkPhysicalDevice physdevice, VkDevice device, VkSurfaceKHR surface);
    static VkRenderPass         vk_create_renderpass(VkDevice device, VkFormat format, VkFormat depthformat);

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
//    std::vector<VkFence>            m_commandBufferFences;
//    std::vector<VkSemaphore>        m_commandBufferDrawCompleteSemaphores;
    std::vector<VkFramebuffer>      m_framebuffers;

    std::vector<Shader>         m_shaders;
    std::vector<VDeclaration>   m_vdecls;
    std::vector<VkBuffer>       m_buffers;
    std::vector<VkPipeline>     m_pipelines;
};

#endif