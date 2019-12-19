#ifndef __Renderer_h__
#define __Renderer_h__

#include <assert.h>
#include <stdint.h>

#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif



enum eRenderApi
{
    eRenderApi_gl,
//    eRenderApi_gles,
    eRenderApi_vk,
    eRenderApi_dx11,
//    eRenderApi_dx12,
//    eRenderApi_metal
};


enum eResourceType : uint8_t 
{
    eResourceType_invalid = 0,
    eResourceType_vdecl,
    eResourceType_vb,
    eResourceType_ib,
    eResourceType_shader,
    eResourceType_texture,
    eResourceType_sampler,
    eResourceType_pipeline,
    eResourceType_renerpass,
    eResourceType_fbo, //todo: is it part of renerpass ???
};


//uint64_t make_res(eResourceType, uint32_t id)

enum ePixelFormat : uint8_t
{
    ePixelFormat_Unknown,
    ePixelFormat_A8,           //! 8-bit textures used as masks
    ePixelFormat_RGBA4444,     //! 16-bit textures: RGBA4444
    ePixelFormat_RGB5A1,       //! 16-bit textures: RGB5A1
    ePixelFormat_RGB565,       //! 16-bit texture without Alpha channel
    ePixelFormat_RGB8,         //! 24-bit texture: RGBA888
    ePixelFormat_RGBA8,        //! 32-bit texture: RGBA8888

    ePixelFormat_ETC1,         //! Compresses RGB888 data without Alpha channel
    ePixelFormat_ETC2,         //! Compresses RGB888 data without Alpha channel
    ePixelFormat_ETC2_EAC,     //! Compresses RGBA8888 data with full alpha support

    ePixelFormat_PVRTC2,       //! 2-bit PVRTC-compressed texture: PVRTC2
    ePixelFormat_PVRTC4,       //! 4-bit PVRTC-compressed texture: PVRTC4

    ePixelFormat_DXT1,         //! 8
    ePixelFormat_DXT3,         //! 16
    ePixelFormat_DXT5,         //! 16

    ePixelFormat_R16F,         //!
    ePixelFormat_RG16F,        //!
    ePixelFormat_RGB16F,       //!
    ePixelFormat_RGBA16F,      //!

    ePixelFormat_R32F,         //!
    ePixelFormat_RG32F,        //!
    ePixelFormat_RGB32F,       //!
    ePixelFormat_RGBA32F,      //!

    ePixelFormat_R11F_G11F_B10F,    //! exotic :)

    ePixelFormat_D24X8,        //! depth buffer
    ePixelFormat_D24S8,        //! depth buffer
};


enum eVertexFormat : uint8_t
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


enum eVertexAttrib : int8_t
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


enum eCullMode : uint8_t
{
    eCull_None,
    eCull_Backface,
    eCull_Frontface
};


enum eCompareMode : uint8_t
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


enum eStencilOp : uint8_t
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


enum eBlendMode : uint8_t
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


enum eBlendOp : uint8_t
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


struct UniformInfo
{
    int8_t  buffId; // if shader has few cb
    int8_t  unused; // for future/alignment
    int16_t offset; // 
};

struct ShaderInfo
{
    VertexAttribute attributes;
    UniformInfo     uniforms;
};


class iRenderer
{
public:
    virtual             ~iRenderer() {};

    virtual bool        initialize(long handle) = 0;
    virtual void        release() = 0;

    virtual void        begin() = 0;
    virtual void        end() = 0;
    virtual void        present() = 0;

    //virtual void      clear(uint32_t flags, uint32_t color, float depth, uint8_t stencil);
    virtual uint32_t    create_swapchain(long handle) = 0;
    virtual void        bind_swapchain(uint32_t swapchain) = 0;

    virtual uint64_t    create_vdecl(VertexAttribute * atribs, size_t count) = 0;
    virtual uint64_t    create_vb(void * data, size_t size, bool dynamic) = 0;
    virtual uint64_t    create_ib(void * data, size_t size, bool dynamic) = 0;
//    virtual uint64_t    create_fbo(uint16_t width, uint16_t height, ePixelFormat format) = 0;
    virtual uint64_t    create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size) = 0;
  
    //virtual uint64_t    create_texture2d(uint16_t width, uint16_t height, int format, int mips, void * data) = 0;
    //virtual uint64_t    create_texture3d(uint16_t width, uint16_t height, uint16_t depth, int format, int mips, void * data) = 0;

    virtual uint64_t    create_shader(void * vdata, size_t vsize, void * pdata, size_t psize) = 0;
    virtual uint64_t    create_pipeline(uint64_t vdecl, uint64_t shader, RenderStates * rstates /*uint64_t renderpass*/) = 0;
    virtual uint64_t    create_renderpass(/*uint64_t * colorFbo, size_t count, uint64_t depthFbo*/) = 0;

    virtual uint32_t    uniform(uint64_t shader, const char * name) = 0;
    virtual void        update_uniform(uint32_t id, const void *data) = 0;
//  virtual void        update_uniform(uint32_t id, int type, const void *data, size_t size) = 0;
//  virtual uint64_t    update_bufferdata(uint64_t id, void * data, size_t size, size_t offset) = 0;

    virtual void        destroy_resource(uint64_t id) = 0;

    virtual void        bind_pipeline(uint64_t pip) = 0;
    virtual void        bind_vb(uint64_t vb) = 0;
    virtual void        bind_ib(uint64_t ib) = 0;
    virtual void        bind_texture(uint64_t texture, uint16_t slot) = 0;

    virtual void        draw_array(uint32_t start_vert, uint32_t vert_count) = 0;
    virtual void        draw_indexed(uint32_t idxcount) = 0;
};

#endif