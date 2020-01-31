#include <windows.h>
#include <tchar.h>

#include <stdio.h>
#include <time.h> 
#include <sys/stat.h> // stat

#include <d3dcompiler.h>

#include <memory>


#include "mathlib.h"
#include "renderer_gl.h"
#include "renderer_vk.h"
#include "renderer_dx11.h"

#include "assets/asset_fbx.h"

//#ifndef _countof
//#endif


#pragma comment(lib, "d3dcompiler.lib")

long create_window(const char * caption)
{
    auto wndClassName = _T("vkwnd");
    HINSTANCE instance = GetModuleHandle(NULL);
    WNDCLASS wndclass = { 0 };
    {
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.hInstance = instance;                      // Assign our hInstance
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);   // General icon
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);     // An arrow for the cursor
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW);    // A white window
        wndclass.lpszClassName = wndClassName;              // Assign the class name
        wndclass.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
                if (msg == WM_DESTROY) 
                    ::PostQuitMessage(0);
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }; 
    }
    if (!RegisterClass(&wndclass))
    {
        return 0;
    }

    mat4 ms, mp, mr, m;
  //  ms.scale(vec3(1, 1, 3));
    mp.translate(vec3(10, 20, 30));

    vec3 p, s;
    quat q;
    mr = mat4::FromQuat(quat::FromEulers(30, 45, 90));

    m = ms * mp * mr;
    m.decompose(p, q, s);

    int width = GetSystemMetrics(SM_CXSCREEN) >> 1;
    int height = GetSystemMetrics(SM_CYSCREEN) >> 1;

    DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    RECT win_rect = { 0, 0, width, height };
    AdjustWindowRect(&win_rect, wndStyle, false);

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW, wndClassName, _T(""), wndStyle, 0, 0,
        win_rect.right - win_rect.left,
        win_rect.bottom - win_rect.top,
        0, 0, GetModuleHandle(NULL), 0);

    ShowWindow(hWnd, SW_SHOWNORMAL);
    SetForegroundWindow(hWnd);
    UnregisterClass(wndClassName, instance);

    return long(hWnd);
}

bool process_msg()
{
    MSG msg;
    // Application::shared()->kbrdBtnDown(msg.wParam);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}


size_t filedata(const char * path, char **buff)
{
#ifdef _WIN32
    struct _stat32 st = { 0 };
    _stat32(path, &st);
#else
    struct stat st = { 0 };
    stat(path, &st);
#endif
    if (st.st_size == 0)
        return st.st_size;

    *buff = (char*)malloc(st.st_size+1);
    memset(*buff, 0, st.st_size + 1);

    FILE * file = fopen(path, "rb");
    fread(*buff, 1, st.st_size, file);
    fclose(file);
    return st.st_size;
}

size_t dxcompiledshader(const char * data, size_t size, const char* entry, const char * target, char **blob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG;
#endif
    const D3D_SHADER_MACRO defines[] = { /*"EXAMPLE_DEFINE", "1",*/ NULL, NULL };
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3DCompile(data, size, NULL, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0, &shaderBlob, &errorBlob);

    if (errorBlob)
    {
        const char *msg = (const char*)errorBlob->GetBufferPointer();
        printf("\n%s", msg);
    }

    *blob = (char*)shaderBlob->GetBufferPointer();
    return shaderBlob->GetBufferSize();
}

struct vertex {
    vec4    position;
    color32 color;
};



int main(int argc, char ** argv)
{
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { 200, 600 });

    long hwnd = create_window("vk sample");


    AssetFbx fbx;

    fbx.load("../art/fbx/blendshapes/Recording_RawData.fbx");




  //  srand((unsigned int)time(0));
    srand(time(0));

    auto apitype = eRenderApi_vk;

    std::unique_ptr<iRenderer> renderer;
    switch (apitype)
    {
        case eRenderApi_gl: renderer = std::make_unique<RendererGl>(); break;
        case eRenderApi_vk: renderer = std::make_unique<RendererVk>(); break;
        case eRenderApi_dx11: renderer = std::make_unique<RendererDx11>(); break;
    }

    if (!renderer->initialize(hwnd))
        return 0;

    char * frag = NULL;
    char * vert = NULL;
    size_t fsize = 0;
    size_t vsize = 0;

    switch (apitype)
    {
        case eRenderApi_gl: {
            size_t fsize = filedata("../data/shaders/simple.frag", &frag);
            size_t vsize = filedata("../data/shaders/simple.vert", &vert);
        }break;

        case eRenderApi_vk: {
            fsize = filedata("../data/shaders/simple.frag.spv", &frag);
            vsize = filedata("../data/shaders/simple.vert.spv", &vert);
        }break;

        case eRenderApi_dx11: {
            char * dxfx = NULL;
            size_t dxsize = filedata("../data/shaders/simple.fx", &dxfx);
            vsize = dxcompiledshader(dxfx, dxsize, "VS", "vs_4_0", &vert);
            fsize = dxcompiledshader(dxfx, dxsize, "PS", "ps_4_0", &frag);
        }break;

        default: 
            assert(false);
        break;
    }
    
    VertexAttribute attributes[] = {
        { eVertexAttrib_Position,   eVertexFormat_float4},
        { eVertexAttrib_Color,      eVertexFormat_byte4 }
    };
    vertex vertexes[] = {
        { { 0.0f, 0.0f, 0.0, 1.0 }, { 0xFFFFFFFF } },
        { { 0.0f, 1.0f, 0.0, 1.0 }, { 0xFFFFFFFF } },
        { { 1.0f, 1.0f, 0.0, 1.0 }, { 0xFFFFFFFF } },
    };

    vertex vertexes2[] = {
        { { 0.0f, 0.0f, 0.0, 1.0, }, { 0xFFFFFFFF } },
        { { 0.0f, -1.0f, 0.0, 1.0, }, { 0xFFFFFFFF } },
        { { -1.0f, -1.0f, 0.0, 1.0, }, { 0xFFFFFFFF } },
    };

    uint16_t indexes[] = {
        0, 1, 2, 1, 2, 3
    };

    RenderStates states;

    uint64_t vdecl = renderer->create_vdecl(attributes, _countof(attributes));
    uint64_t shader = renderer->create_shader(vert, vsize, frag, fsize);
    uint64_t pipeline = renderer->create_pipeline(vdecl, shader, &states);

    uint64_t vb = renderer->create_vb(vertexes, _countof(vertexes) * sizeof(vertexes[0]), false);
    uint64_t vb1 = renderer->create_vb(vertexes2, _countof(vertexes2) * sizeof(vertexes[0]), false);

    uint64_t ib = renderer->create_ib(indexes,  _countof(indexes)  * sizeof(uint16_t), false);

    while (process_msg())
    {
        renderer->begin();

        if (pipeline)
        {
            renderer->bind_pipeline(pipeline);

            auto mat = mat4::lookAtRH(vec3(0.0f, 0.0f, -10.0f), vec3::Zero, vec3::Up);
            uint32_t mvp = renderer->uniform(shader, "mvp");
            //   renderer->update_uniform(mvp, eUniform_mat4, &mat);

            renderer->bind_vb(vb);
            renderer->draw_array(0, _countof(vertexes));

            renderer->bind_vb(vb1);
            renderer->draw_array(0, _countof(vertexes2));
        }

        renderer->end();
        renderer->present();
    };

    return 0;
}