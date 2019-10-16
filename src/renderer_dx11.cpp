#include "renderer_dx11.h"

#pragma comment (lib, "D3D11.lib")
//#pragma comment (lib, "d3dx11.lib")


static const DXGI_FORMAT _vformat2dxformat[eVertexFormat_Count] =
{
    DXGI_FORMAT_UNKNOWN, //eVertexFormat_Invalid
    DXGI_FORMAT_R32_FLOAT,    DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,  //half floats
    DXGI_FORMAT_R16G16_SINT,  DXGI_FORMAT_R16G16B16A16_SINT,    // short
    DXGI_FORMAT_R16G16_UINT,  DXGI_FORMAT_R16G16B16A16_UINT,    // ushort
    DXGI_FORMAT_R8G8B8A8_UINT    // byte
};

static const char * _vattrib2dxname[eVertexAttrib_Count] =
{
    "POSITION", "Color", "Normal", "eVertexAttrib_Tangent",
    "TexCoords0", "TexCoords1", "TexCoords2", "TexCoords3",
    "TexCoords4", "TexCoords5", "TexCoords6", "TexCoords7"
    "BoneWeight", "BoneIndices", "INVALID",
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

static_assert(_countof(_vformat2dxformat) == eVertexFormat_Count, "");
static_assert(_countof(_vattrib2dxname) == eVertexAttrib_Count, "");

bool RendererDx11::initialize(long handle)
{
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT createDeviceFlags = 0;

    #ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    HWND hWnd = reinterpret_cast<HWND>(handle);
    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    DXGI_SWAP_CHAIN_DESC sd = {};
    {
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
    }

    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

    for (int i = 0; i < _countof(driverTypes); i++)
    {
        hr = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[i], NULL, createDeviceFlags, featureLevels, _countof(featureLevels),
            D3D11_SDK_VERSION, &sd, &m_swapchain, &m_device, &featureLevel, &m_context);

        if (SUCCEEDED(hr))
            break;
    }


    ID3D11Texture2D* pBackBuffer = NULL;
    hr = m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_rendertargetview);
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    D3D11_VIEWPORT viewport;
    {
        viewport.Width = (FLOAT)width;
        viewport.Height = (FLOAT)height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
    }
    m_context->RSSetViewports(1, &viewport);

    return true;
}

void RendererDx11::release()
{

}

void RendererDx11::begin()
{
    m_context->OMSetRenderTargets(1, &m_rendertargetview, NULL);

    float ClearColor[4] = { 0.1f, 0.2f, 0.3f, 1.0f }; //red,green,blue,alpha
    m_context->ClearRenderTargetView(m_rendertargetview, ClearColor);
}

void RendererDx11::end()
{

}

void RendererDx11::present()
{
    m_swapchain->Present(0, 0);
}


uint32_t RendererDx11::create_swapchain(long handle)
{
    return 0;
}

void RendererDx11::bind_swapchain(uint32_t swapchain)
{

}


uint64_t RendererDx11::create_vdecl(VertexAttribute * atribs, size_t count)
{
    dxVertexLayout layout;
    layout.count = count;

    size_t stride = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const char * name = _vattrib2dxname[atribs[i].type];
        DXGI_FORMAT format = _vformat2dxformat[atribs[i].format];
        
        layout.attributes[i] = { name, 0, format, 0, stride, D3D11_INPUT_PER_VERTEX_DATA, 0 };

        stride += _vformatstrides[atribs[i].format];
    }
    layout.stride = stride;

    m_layouts.push_back(layout);
    return m_layouts.size();
}

uint64_t RendererDx11::create_vb(void * data, size_t size, bool dynamic)
{
    D3D11_BUFFER_DESC desc = {};
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = size;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA subresdata = { data, 0, 0 };

    ID3D11Buffer * buffer = NULL;
    HRESULT result = m_device->CreateBuffer(&desc, &subresdata, &buffer);

    d3dResource res;
    res.buffer = buffer;
    m_resources.push_back(res);

    return m_resources.size();
}

uint64_t RendererDx11::create_ib(void * data, size_t size, bool dynamic)
{
    D3D11_BUFFER_DESC desc = {};
    {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = size;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA subresdata = { data, 0, 0 };
    ID3D11Buffer * buffer = NULL;
    HRESULT result = m_device->CreateBuffer(&desc, &subresdata, &buffer);

    if (result != S_OK){
        printf("\nerror while creating ib");
        return 0;
    }

    d3dResource res;
    res.buffer = buffer;
    m_resources.push_back(res);

    return m_resources.size();
}

uint64_t RendererDx11::create_texture(uint16_t width, uint16_t height, uint16_t depth, int format, void * data, size_t size)
{
    return 0;
}

uint64_t RendererDx11::create_shader(void * vdata, size_t vsize, void * pdata, size_t psize)
{
    ID3D11VertexShader *    vertexShader = NULL;
    ID3D11PixelShader *     pixelShader = NULL;

    HRESULT result = S_OK;
    result = m_device->CreateVertexShader(vdata, vsize, NULL, &vertexShader);

    if (result != S_OK){
        printf("\nerror while vertex shader create");
        return 0;
    }

    result = m_device->CreatePixelShader(pdata, psize, NULL, &pixelShader);

    if (result != S_OK){
        printf("\nerror while fragment shader create");
        return 0;
    }

    d3dResource res;
    {
        res.shader.pixel = pixelShader;
        res.shader.vertex = vertexShader;
        res.shader.vblob = (char*)vdata;
        res.shader.vblobsize = vsize;
    }
    m_resources.push_back(res);

    return m_resources.size();
}

uint64_t RendererDx11::create_pipeline(uint64_t vdeclid, uint64_t shaderid, RenderStates * rstates /*uint64_t renderpass*/)
{
    auto & layout = m_layouts[vdeclid - 1];
    auto & shader = m_resources[shaderid - 1].shader;

    ID3D11InputLayout * inputlayout = nullptr;
    const void* vdata = shader.vblob; // pVSBlob->GetBufferPointer()
    size_t size = shader.vblobsize; // pVSBlob->GetBufferSize()
    HRESULT result = m_device->CreateInputLayout(layout.attributes, layout.count, vdata, size, &inputlayout);

    D3D11_RASTERIZER_DESC raster = {};
    {
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
    }

    D3D11_RENDER_TARGET_BLEND_DESC blend = {};
    {

    }

    ID3D11BlendState * blendstate = NULL;
    ID3D11RasterizerState * rasterstate = NULL;
    result = m_device->CreateRasterizerState(&raster, &rasterstate);
   // result = m_device->CreateBlendState(&blend, &blendstate);


    d3dResource res = {};
    res.pipeline.stride = layout.stride;
    res.pipeline.inputlayout = inputlayout;
    res.pipeline.shader = shader;
    res.pipeline.rasterstate = rasterstate;

    m_resources.push_back(res);
    return m_resources.size();
}

uint64_t RendererDx11::create_renderpass(/*colorformats * formats, siz_t count, VkFormat depthFormat*/)
{
    return 0;
}

uint32_t RendererDx11::uniform(uint64_t shader, const char * name)
{
    return 0;
}

void RendererDx11::update_uniform(uint32_t id, const void *data)
{

}

void RendererDx11::destroy_resource(uint64_t id)
{

}

void RendererDx11::bind_pipeline(uint64_t pipid)
{
    m_pipeline = pipid;
    auto & pipeline = m_resources[pipid - 1].pipeline;

    m_context->IASetInputLayout(pipeline.inputlayout);
    m_context->VSSetShader(pipeline.shader.vertex, NULL, 0);
    m_context->PSSetShader(pipeline.shader.pixel, NULL, 0);

    m_context->RSSetState(pipeline.rasterstate);
}

void RendererDx11::bind_vb(uint64_t vb)
{
    auto & pipeline = m_resources[m_pipeline - 1].pipeline;
    auto & buffer = m_resources[vb - 1].buffer;
    UINT stride = pipeline.stride;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);

    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void RendererDx11::bind_ib(uint64_t ib)
{

}

void RendererDx11::bind_texture(uint64_t texture, uint16_t slot)
{

}

void RendererDx11::draw_array(uint32_t start_vert, uint32_t vert_count)
{
    if (!m_pipeline)
        return;

   // m_context->Draw(vert_count, start_vert);
    m_context->Draw(vert_count, start_vert);
}

void RendererDx11::draw_indexed(uint32_t idxcount)
{
    if (!m_pipeline)
        return;
}