#include <windows.h>
#include <tchar.h>

#include <stdio.h>
#include <sys/stat.h> // stat

#include <memory>

#include "vkrenderer.h"

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
        wndclass.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT{
            if (msg == WM_DESTROY) ::PostQuitMessage(0);
            return DefWindowProc(hwnd, msg, wParam, lParam);
        };//WinProc;       // Pass our function pointer as the window procedure
    }
    if (!RegisterClass(&wndclass))
    {
        return 0;
    }

    int width = GetSystemMetrics(SM_CXSCREEN) >> 1;
    int height = GetSystemMetrics(SM_CYSCREEN) >> 1;

    DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    RECT win_rect = { 0, 0, width, height };
    AdjustWindowRect(&win_rect, wndStyle, false);

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW, _T("vkwnd"), _T(caption), wndStyle, 0, 0,
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

    *buff = (char*)malloc(st.st_size);

    FILE * file = fopen(path, "rb");
    fread(*buff, 1, st.st_size, file);
    fclose(file);
    return st.st_size;
}


int main(int argc, char ** argv)
{
    long hwnd = create_window("vk sample");

    auto renderer = std::make_unique<Vkrenderer>();
    if (!renderer->initialize(hwnd))
        return 0;

    char * frag = NULL;
    char * vert = NULL;
    size_t fsize = filedata("../data/shaders/simple.frag.spv", &frag);
    size_t vsize = filedata("../data/shaders/simple.vert.spv", &vert);

    VertexAttribute attributes[] = {
        { eVertexAttrib_Position,   eVertexFormat_float4},
   //     { eVertexAttrib_Color,      eVertexFormat_byte4 }
    };
    float vertexes[] = {
         0.0f,  0.0f, 0.0, 1.0,
         0.0f, -0.5f, 0.0, 1.0,
         0.5f,  0.5f, 0.0, 1.0,
        -0.5f,  0.5f, 0.0, 1.0,
    };
    uint16_t indexes[] = {
        0, 1, 2, 1, 2, 3
    };


    RenderStates states;

    uint64_t vdecl = renderer->create_vdecl(attributes, _countof(attributes));
    uint64_t shader = renderer->create_shader(vert, vsize, frag, fsize);
    uint64_t pipeline = renderer->create_pipeline(vdecl, shader, &states);

    uint64_t vb = renderer->create_vb(vertexes, _countof(vertexes) * sizeof(float), false);
    uint64_t ib = renderer->create_ib(indexes,  _countof(indexes)  * sizeof(uint16_t), false);

    while (process_msg())
    {
        renderer->begin();

        renderer->bind_pipeline(pipeline);

     //   uint32_t mvp = renderer->uniform(shader, "mvp");
     //   renderer->update_uniform(mvp, nullptr);

        renderer->bind_vb(vb);
        renderer->draw_array(1, _countof(vertexes)/4);

        renderer->end();
        renderer->present();
    };

    return 0;
}