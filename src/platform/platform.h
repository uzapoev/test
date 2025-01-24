#ifndef __platform_h__
#define __platform_h__

#include <stdlib.h>
#include <stdint.h>


#ifndef _countof
    #define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef  __APPLE__
    #define     PLATFORM_APPLE
#elif defined (__ANDROID__)
    #define     PLATFORM_ANDROID
#elif defined (_WIN32)
    #define     PLATFORM_WINDOWS
#elif defined (EMSCRIPTEN)
    #define     PLATFORM_EMSCRIPTEN
#elif
    #define     PLATFORM_UNDEFINED
#endif

#define PLATFORM_MAX_MULTITOUCH                 (16)
#define PLATFORM_MAX_RINGBUFFER_CAPPACITY       (255)


typedef struct rect_t {
    int16_t   x, y, w, h;
} rect_t;


typedef struct input_point_t {
    int16_t   x, y, dx, dy;  // x,y - position, dx,dy - delta position from prev input
}input_point_t;


typedef struct monitor_info_t {
    char        name[64];
    rect_t      rect[4];
    intptr_t    handle;
} monitor_info_t;


typedef enum input_device_type {
    input_device_keyboard,
    input_device_mouse,
    input_device_touch,
    input_device_gamepad,
    input_device_accelerometer
} input_device_type;


typedef enum input_state {
    input_state_none,
    input_state_down,
    input_state_up,
    input_state_move
} input_state;


typedef enum touch_id {
    touch_id_1 = 1,
    touch_id_2,
    touch_id_3,
    touch_id_4,
    touch_id_5
} touch_id;


typedef enum mouse_button {
    none,
    mouse_btn_left,
    mouse_btn_right,
    mouse_btn_middle,
    mouse_btn_extra0,
    mouse_btn_extra1
} mouse_button;


typedef struct input_event_t {
    input_device_type                                       type;
    input_state                                             state;

    struct { touch_id id; input_point_t pos; float force; } touch;
    struct { mouse_button btn; input_point_t pos; }         mouse;
    struct { uint8_t key; }                                 keyboard;
} input_event_t;


typedef struct platform_ctx_t {
    input_event_t    g_events[1024];
    int              g_current_event_idx;
    uint8_t          g_keyboard_key_states[256];
    input_point_t    g_point_states[5];
    input_point_t    g_touch_states[10];
} platform_ctx_t;

extern void             platform_main(uintptr_t handle, int argc, char**argv);
extern void             platform_tick(void * userdata); 

int                     platform_get_monitors(monitor_info_t * infos);
rect_t                  platform_get_window_size(uintptr_t handle);
void                    platform_set_window_size(rect_t rect, bool fullscreen);

//void                  platform_notification_add(long secondsToDate, const char* message, const char* id);
//void                  platform_notification_update(long secondsToDate, const char* message, const char* id);
//void                  platform_notification_remove(const char* id);

static void             input_push_event(input_event_t event);
static int              input_pop_event(input_event_t* event);
static int              input_kb_state(uint8_t key);
static input_point_t    input_point_pos(touch_id id);
static input_point_t    input_touch_pos(touch_id id);

static void             push_input_touch_event(int16_t x, int16_t y, int16_t dx, int16_t dy, input_state state, uint64_t touchid);
static void             push_input_mouse_event(int16_t x, int16_t y, int16_t dx, int16_t dy, input_state state, mouse_button button);
static void             push_input_keyboard_event(uint8_t key, input_state state);


// arg:  1 - add; 
//       0 - get;
//      -1 - remove;
static touch_id touch_id_2_id(uint64_t touchid, int arg)
{
    static uint64_t _touchids[10] = {};
    uint64_t local_id = (arg > 0) ? 0 : touchid; // find empty
    for (int i = 0; i < _countof(_touchids); ++i)
    {
        if (_touchids[i] == local_id)
        {
            if (arg < 0)
                _touchids[i] = 0;
            if (arg > 0)
                _touchids[i] = touchid;

            return (touch_id)(i + 1);
        }
    }
    return touch_id_1;
}


extern input_event_t    g_events[];
extern int              g_current_event_idx;
extern uint8_t          g_keyboard_key_states[];
extern input_point_t    g_point_states[];

static void input_push_event(input_event_t event)
{
    int idx = (g_current_event_idx++) % 1024;
    g_events[idx] = event;
}

static int input_pop_event(input_event_t* event)
{
    if (g_current_event_idx > 0)
    {
        *event = g_events[--g_current_event_idx];
        return 1;
    }
    return 0;
}

static input_point_t input_point_pos(int id = 0)
{
    return g_point_states[id];
}

static int input_kb_state(uint8_t key)
{
    return g_keyboard_key_states[key];
}


// type begin/end/move
static void push_input_touch_event(int16_t x, int16_t y, int16_t dx, int16_t dy, input_state state, uint64_t touchid)
{
    touch_id tid = touch_id_1;
    switch (state)
    {
        case input_state_down: tid = touch_id_2_id(touchid,  1); break;
        case input_state_move: tid = touch_id_2_id(touchid,  0); break;
        case input_state_up:   tid = touch_id_2_id(touchid, -1); break;
        case input_state_none:                                   break;
    }

    input_event_t event = { input_device_touch, state };
        event.touch.id = tid;
        event.touch.pos = { x, y, dx, dy };
    input_push_event(event);
}


static void push_input_mouse_event(int16_t x, int16_t y, int16_t dx, int16_t dy, input_state state, mouse_button button)
{
    input_event_t event = { input_device_mouse, state };
        event.mouse.btn = button;
        event.mouse.pos = { x, y, dx, dy };
    input_push_event(event);
    g_point_states[button] = event.mouse.pos;
}


static void push_input_keyboard_event(uint8_t key, input_state state)
{
    input_event_t event = { input_device_keyboard, state };
    event.keyboard.key = key;
    input_push_event(event);

    g_keyboard_key_states[key] = (state == input_state_down) ? 1 : 0;
}



#ifdef PLATFORM_UNDEFINED
static int      platform_get_monitors(monitor_info_t* infos) {}
static rect_t   platform_get_window_size(long handle) {};
#endif


struct Input
{
    typedef struct input_event_t
    {
        enum InputType {
            eUnknown, eKeyboard, eMouse, eTouch, eGamepad, eAccelerometer
        } type;

        union
        {
            struct { uint8_t key; bool  state; } keyboard; // key code + pressed
            struct { uint8_t key; bool  state; float value[3]; } mouse;    // mouse btn + position(x,y,scroll)
            struct { uint8_t id;  float value[3]; } touch;    // touch id + position(x,y,3dtouch)
            struct { uint8_t key; float value[3]; } gamepad;  // key + value if sticker
            struct { float value[3]; } accelerometer;
        };
    } input_event_t;

    enum State { None, Down, Up, Move };

    struct MouseButton { 
        enum { Left = 1, Right, Middle, Extra0, Extra1 };
    };

    struct Touch {
        enum { Touch0, Touch1, Touch2, Touch3, Touch4 };
    };

    struct GamePad {
        enum {
            Up, Down, Right, Left,
            A, B, X, Y,
            L1, L2, R1, R2,
            StickL, StickLLeft, StickLUp, StickLRight, sStickLDown,
            StickR, StickRLeft, StickRUp, StickRRight, StickRDown,
        };
    } ;

    struct Keyboard {
        enum {
            F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
            Space = 32, PageUp, PageDown, End, Home, Left, Up, Right, Down, Select, Print, Execute, PrintScreen, Insert, Delete, Help,
            Key_0, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
            A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            Win_L, Win_R, Apps, 
 
        
            NumPad_0 = 96, NumPad_1, NumPad_2, NumPad_3, NumPad_4, NumPad_5, NumPad_6, NumPad_7, NumPad_8, NumPad_9, 
            NumPad_Multiply, NumPad_Add, NumPad_Subtract = 109, NumPad_Decimal, NumPad_Divide,
            NumLock = 144,
            Back = 8, Tab, 
            Enter = 13, 
            Shift = 16, Ctrl, Menu, Pause, CapsLock, 
            Alt_L = 164,
            Alt_R = 165,
        };
    };
};

#endif