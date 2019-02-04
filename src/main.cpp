#include <windows.h>
#include <tchar.h>

#include <stdio.h>

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

int main(int argc, char ** argv)
{
    long hwnd = create_window("vk sample");

    auto renderer = std::make_unique<Vkrenderer>();
    if (!renderer->initialize(hwnd))
        return 0;

    while (process_msg())
    {
        renderer->begin();
        renderer->end();
        renderer->present();
    };

    return 0;
}