#ifndef __RendererDirectX11_h__
#define __RendererDirectX11_h__

#include <windows.h>
#include <d3d11.h>
#include <vector>

#include "renderer.h"


class RendererDx11 : public iRenderer
{
public:
                                RendererDx11(){}
    virtual                    ~RendererDx11(){}

    bool                        initialize(long handle);
    void                        release();

    void                        begin();
    void                        end();
    void                        present();

    virtual uint32_t            create_swapchain(long handle);
    virtual void                bind_swapchain(uint32_t swapchain);

    uint64_t                    create_vdecl(VertexAttribute * atribs, size_t count);
    uint64_t                    create_vb(void * data, size_t size, bool dynamic);
    uint64_t                    create_ib(void * data, size_t size, bool dynamic);

    uint64_t                    create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size);
    uint64_t                    create_shader(void * vdata, size_t vsize, void * pdata, size_t psize );
    uint64_t                    create_pipeline(uint64_t vdecl, uint64_t shader, RenderStates * rstates /*uint64_t renderpass*/);
    uint64_t                    create_renderpass(/*colorformats * formats, siz_t count, VkFormat depthFormat*/);

    uint32_t                    uniform(uint64_t shader, const char * name);
    void                        update_uniform(uint32_t id, const void *data);
    void                        update_bufferdata(uint64_t id, void * data, size_t size, size_t offset);

    void                        destroy_resource(uint64_t id);

    void                        bind_pipeline(uint64_t pip);
    void                        bind_vb(uint64_t vb);
    void                        bind_ib(uint64_t ib);
    void                        bind_texture(uint64_t texture, uint16_t slot);

    void                        draw_array(uint32_t start_vert, uint32_t vert_count);
    void                        draw_indexed(uint32_t idxcount);
private:
    ID3D11Device *              m_device = nullptr;
    ID3D11DeviceContext *       m_context = nullptr;
    IDXGISwapChain *            m_swapchain = nullptr;
    ID3D11RenderTargetView *    m_rendertargetview = nullptr;

    struct d3dResource
    {
        union
        {
            ID3D11Buffer* buffer;

            struct Shader{
                const char * vblob;
                size_t vblobsize;
                ID3D11VertexShader * vertex;
                ID3D11PixelShader * pixel;
            }shader;

            struct Pipeline {
                size_t                  stride;
                ID3D11InputLayout *     inputlayout;
                Shader                  shader;
                ID3D11RasterizerState * rasterstate;
            }pipeline;
        };
    };

    struct dxVertexLayout
    {
        size_t stride;
        size_t count;
        D3D11_INPUT_ELEMENT_DESC attributes[eVertexAttrib_Count];
    };

    uint64_t                    m_pipeline = 0;
    std::vector<d3dResource>    m_resources;
    std::vector<dxVertexLayout> m_layouts;
};

#endif