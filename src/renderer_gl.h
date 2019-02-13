#ifndef __RendererOpenGL_h__
#define __RendererOpenGL_h__

#include <windows.h>
#include <gl/gl.h>

#include <vector>
#include "renderer.h"


class RendererGl : public iRenderer
{
public:
                                RendererGl(){}
    virtual                    ~RendererGl(){}

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

    void                        draw_array(uint32_t start_vert, uint32_t vert_count);
    void                        draw_indexed(uint32_t idxcount);
private:
    void                        apply_vdecl(uint64_t vdecl);

private:

#ifdef _WIN32
    HGLRC                       m_hrc = 0;
    HWND                        m_hwnd = 0;
    HDC                         m_hdc = 0;
#endif

    struct VDeclaration
    {
        size_t              flag;
        size_t              stride;
        size_t              count;
        VertexAttribute     attributes[eVertexAttrib_Count];
    };
    struct glResource
    {
        GLuint id = 0;
      //  GLuint vao = 0;
        GLenum target = 0;
        //union {
        //    const VertexDeclaration * vdecl = nullptr;
        //    uint16_t    width;
        //    uint16_t    height;
      //  };
    };

    struct Pipeline{
        RenderStates        states;
        uint64_t            shader;
        uint64_t            vdecl;
    };

    bool                        m_reset_vdecl = false;
    uint64_t                    m_curr_vdecl = 0;

    std::vector<VDeclaration>   m_declarations;
    std::vector<glResource>     m_resources;
    std::vector<Pipeline>       m_pipelines;
    
};

#endif