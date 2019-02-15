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


class iRenderer
{
public:
    virtual             ~iRenderer() {};
    virtual bool        initialize(long handle) = 0;
    virtual void        release() = 0;

    virtual void        begin() = 0;
    virtual void        end() = 0;
    virtual void        present() = 0;

    virtual uint64_t    create_vdecl(VertexAttribute * atribs, size_t count) = 0;
    virtual uint64_t    create_vb(void * data, size_t size, bool dynamic) = 0;
    virtual uint64_t    create_ib(void * data, size_t size, bool dynamic) = 0;
//    virtual uint64_t    create_fbo(uint16_t width, uint16_t height, ePixelFormat format) = 0;
    virtual uint64_t    create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size) = 0;
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