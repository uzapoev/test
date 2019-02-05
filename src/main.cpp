#include <windows.h>
#include <tchar.h>

#include <stdio.h>
#include <sys/stat.h> // stat

#include <memory>
#include "vkrenderer.h"

#include <shaderc/shaderc.h>


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

const char * vertex =
"in float4 position;"
"out float4 pos;"
"void main() {"
"gl_Position = position;"
"}";

const char * fragment =
"out float4 outcolor;"
"void main() {"
"   outcolor = float4(1.0, 0.0, 1.0, 1.0);"
"}";


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

#if 0
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();

    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    if (/*debug*/1)
        shaderc_compile_options_set_generate_debug_info(options);

    unsigned int ver, rev;
    shaderc_get_spv_version(&ver, &rev);
#endif

    uint64_t shader = renderer->create_shader(vert, vsize, frag, fsize);
    uint64_t pipeline = renderer->create_pipeline(shader, nullptr, nullptr);

    while (process_msg())
    {
        renderer->begin();
        renderer->end();
        renderer->present();
    };

    return 0;
}