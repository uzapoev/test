#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <hidusage.h>
#include <tchar.h>

#include <string>
#include <vector>


static bool is_window_active = false;

platform_ctx_t *g_platform_ctx = nullptr;
/*
input_event_t    g_events[1024] = {};
int              g_current_event_idx = 0;
uint8_t          g_keyboard_key_states[256] = {};
input_point_t    g_point_states[8] = {};
input_state      g_mouse_btn_states[16] = {};*/


static void _fetch_monitors_info()
{
    EnumDisplayMonitors(NULL, NULL, [](HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM param)->BOOL {
   
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);

        DISPLAY_DEVICE dd = { sizeof(dd) };
        DEVMODE devmode = { sizeof(DEVMODE) };
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        EnumDisplayDevices(info.szDevice, 0, &dd, 0);

        int w = rect->right - rect->left;
        int h = rect->bottom - rect->top;
        int originw = devmode.dmPelsWidth;

        float scale = float(originw) / float(w);
        return true;
    }, NULL);

    DISPLAYCONFIG_PATH_INFO paths[8] = {};
    DISPLAYCONFIG_MODE_INFO modes[8] = {};
    UINT32 requiredPaths = _countof(paths), requiredModes = _countof(modes);

    //  GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, &requiredModes);
    QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &requiredPaths, paths, &requiredModes, modes, nullptr);
    for (UINT32 i = 0; i < requiredPaths; ++i)
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME name = {};
        name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        name.header.size = sizeof(name);
        name.header.adapterId = paths[i].sourceInfo.adapterId;
        name.header.id = paths[i].targetInfo.id;
        DisplayConfigGetDeviceInfo(&name.header);
        printf("");
    }
}

int main(int argc, char ** argv)
{
    g_platform_ctx = (platform_ctx_t*)calloc(1, sizeof(platform_ctx_t));

    _fetch_monitors_info();

    WNDCLASS wndclass = { 0 };
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.hInstance = GetModuleHandle(NULL);
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        wndclass.lpszClassName = _T("wnd");
        wndclass.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
                switch (msg)
                {
                    case WM_DESTROY: PostQuitMessage(0); break;
                    //        case WM_ACTIVATE: is_window_active = LOWORD(wParam); break;
                }
                return DefWindowProc(hwnd, msg, wParam, lParam);
            };

    if (!RegisterClass(&wndclass))
        return 0;

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HINSTANCE instance = GetModuleHandle(NULL);

    int x = width >> 2;
    int y = height >> 2;
    int w = width >> 1;
    int h = height >> 1;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    style = WS_OVERLAPPEDWINDOW;
    RECT rect = { 0, 0, w, h };
    AdjustWindowRect(&rect, style, false);

    HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW, wndclass.lpszClassName, _T(""), style,
        x, y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0, 0, instance, 0);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    UnregisterClass(wndclass.lpszClassName, instance);

    RAWINPUTDEVICE rids[] = {
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK, hwnd },//RIDEV_INPUTSINK | RIDEV_NOLEGACY
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE,    RIDEV_INPUTSINK, hwnd },
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_GAMEPAD,  RIDEV_INPUTSINK, hwnd },
    };
    RegisterRawInputDevices(rids, _countof(rids), sizeof(RAWINPUTDEVICE));

    platform_main((uintptr_t)hwnd, argc, argv);

    bool exit = false;
    while(!exit)
    {
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            switch (msg.message)
            {
                case WM_QUIT:
                {
                    exit = true;
                    break;
                }
                case WM_INPUT:
                {
                    HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(msg.lParam);

                    RAWINPUT raw = {};
                    UINT size = sizeof(RAWINPUT);
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
                    if (raw.header.dwType == RIM_TYPEKEYBOARD)
                    {
                        RAWKEYBOARD& keyboard = raw.data.keyboard;

                        // Ignore key overrun state and keys not mapped to any virtual key code
                        if (keyboard.MakeCode == KEYBOARD_OVERRUN_MAKE_CODE || keyboard.VKey >= UCHAR_MAX)
                            return 0;

                        int keyUp = (keyboard.Flags & RI_KEY_BREAK);
                        push_input_keyboard_event((uint8_t)keyboard.VKey, keyUp ? input_state_up : input_state_down);
                    }
                    if (raw.header.dwType == RIM_TYPEMOUSE)
                    {
                        RAWMOUSE& mouse = raw.data.mouse;

                        POINT pos = {};
                        GetCursorPos(&pos);
                        ScreenToClient(msg.hwnd, &pos);

                        int16_t x = (int16_t)pos.x;
                        int16_t y = (int16_t)pos.y;

                        int16_t dx = -(int16_t)mouse.lLastX;
                        int16_t dy = -(int16_t)mouse.lLastY;

                        USHORT btn_flags = mouse.usButtonFlags;

                        // new
                        if (!btn_flags)                                 push_input_mouse_event(x, y, dx, dy, input_state_move, none);
                        if (btn_flags & RI_MOUSE_LEFT_BUTTON_DOWN)      push_input_mouse_event(x, y, dx, dy, input_state_down, mouse_btn_left);
                        if (btn_flags & RI_MOUSE_LEFT_BUTTON_UP)        push_input_mouse_event(x, y, dx, dy, input_state_up,   mouse_btn_left);

                        if (btn_flags & RI_MOUSE_RIGHT_BUTTON_DOWN)     push_input_mouse_event(x, y, dx, dy, input_state_down, mouse_btn_right);
                        if (btn_flags & RI_MOUSE_RIGHT_BUTTON_UP)       push_input_mouse_event(x, y, dx, dy, input_state_up,   mouse_btn_right);

                        if (btn_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN)    push_input_mouse_event(x, y, dx, dy, input_state_down, mouse_btn_middle);
                        if (btn_flags & RI_MOUSE_MIDDLE_BUTTON_UP)      push_input_mouse_event(x, y, dx, dy, input_state_up,   mouse_btn_middle);

                        if (btn_flags & RI_MOUSE_BUTTON_4_DOWN)         push_input_mouse_event(x, y, dx, dy, input_state_down, mouse_btn_extra0);
                        if (btn_flags & RI_MOUSE_BUTTON_4_UP)           push_input_mouse_event(x, y, dx, dy, input_state_up,   mouse_btn_extra0);

                        if (btn_flags & RI_MOUSE_BUTTON_5_DOWN)         push_input_mouse_event(x, y, dx, dy, input_state_down, mouse_btn_extra1);
                        if (btn_flags & RI_MOUSE_BUTTON_5_UP)           push_input_mouse_event(x, y, dx, dy, input_state_up,   mouse_btn_extra1);
                    }

                } break;
            }
            assert(!(msg.message == WM_DISPLAYCHANGE));
        }

        if(!exit)
        {
            platform_tick(nullptr);

            g_platform_ctx->g_point_states[0].dx = 0;
            g_platform_ctx->g_point_states[0].dy = 0;
        }
    }

    platform_destroy(nullptr);
}


rect_t platform_get_window_size(uintptr_t handle)
{
    RECT rect = {};
    GetWindowRect((HWND)handle, &rect);
    return { (int16_t)rect.left, (int16_t)rect.top, (int16_t)rect.right, (int16_t)rect.bottom };
}

#endif