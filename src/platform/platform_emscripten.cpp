#include "platform.h"

#ifdef PLATFORM_EMSCRIPTEN
#include <stdio.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>

static const char *canvas = "#canvas";
    
input_event_t    g_events[1024];
int              g_current_event_idx;
uint8_t          g_keyboard_key_states[256];
input_point_t    g_point_states[8];

static EM_BOOL _mouse_move_cb(int event_type, const EmscriptenMouseEvent * mouse_event, void * notused)
{    
    int16_t x = mouse_event->clientX;
    int16_t y = mouse_event->clientX;
    
    int16_t dx = mouse_event-> movementX;
    int16_t dy = mouse_event-> movementY;

    push_input_mouse_event(x, y, dx, dy, input_state_move, none);
   /* ctx.touch.x = mouse_event->canvasX;
    ctx.touch.y = mouse_event->canvasY;
    ctx.touch.multitouch[0].x = ctx.touch.x;
    ctx.touch.multitouch[0].y = ctx.touch.y;
    */
    return false;
}

static EM_BOOL _mouse_key_cb(int event_type, const EmscriptenMouseEvent * mouse_event, void * notused)
{
    int16_t x = mouse_event->clientX;
    int16_t y = mouse_event->clientX;
    
    if(event_type == EMSCRIPTEN_EVENT_MOUSEDOWN)
        emscripten_request_pointerlock(canvas, true);
    if(event_type == EMSCRIPTEN_EVENT_MOUSEUP)
        emscripten_exit_pointerlock();
    
    push_input_mouse_event(x, y, 0, 0, input_state_down, none);
    /*printf("_mouse_key_cb\n");
    bool v = event_type == EMSCRIPTEN_EVENT_MOUSEDOWN;
    if(mouse_event->button == 0) ctx.touch.left = v;
    else if(mouse_event->button == 1) ctx.touch.middle = v;
    else if(mouse_event->button == 2) ctx.touch.right = v;
    ctx.touch.multitouch[0].touched = ctx.touch.left;*/
    return true;
}

static EM_BOOL _touch_cb(int event_type, const EmscriptenTouchEvent * touch_event, void * notused)
{
    printf("_touch_cb\n");
    // TODO support multitouch here
  /*  if(touch_event->numTouches > 0)
    {
        ctx.touch.x = touch_event->touches[0].canvasX;
        ctx.touch.y = touch_event->touches[0].canvasY;
        ctx.touch.multitouch[0].x = ctx.touch.x;
        ctx.touch.multitouch[0].y = ctx.touch.y;
    }
    ctx.touch.left = event_type == EMSCRIPTEN_EVENT_TOUCHSTART || event_type == EMSCRIPTEN_EVENT_TOUCHMOVE;
    ctx.touch.multitouch[0].touched = ctx.touch.left;*/
    return true;
}

int platform_get_monitors(monitor_info_t* infos)
{
    return 0;
}

rect_t platform_get_window_size(uintptr_t handle)
{
    double wh = 0.0, hh = 0.0;
    emscripten_get_element_css_size(canvas, &wh, &hh);
    return { 0, 0, (int16_t)wh, (int16_t)hh };
}

void platform_set_window_size(rect_t rect, bool fullscreen)
{
}

int main(int argc, char ** argv)
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    emscripten_webgl_create_context(canvas, &attrs);
    
    double width = 0.0, heigth = 0.0;
    emscripten_get_element_css_size(canvas, &width, &heigth);
    emscripten_set_canvas_element_size((char*)canvas, width, heigth);
    
    emscripten_set_mousemove_callback(canvas, NULL, 0, _mouse_move_cb);
    emscripten_set_mousedown_callback(canvas, NULL, 0, _mouse_key_cb);
    emscripten_set_mouseup_callback(canvas, NULL, 0, _mouse_key_cb);

    platform_main((intptr_t)(canvas), argc, argv);

    printf("emscripten_set_main_loop\n");
    emscripten_set_main_loop([](){
        double wh = 0.0, hh = 0.0;
        emscripten_get_element_css_size(canvas, &wh, &hh);
        
        //printf("emscripten_get_element_css_size %.3f   %.3f\n", wh, hh);
        platform_tick(nullptr); 
    }, 10, 1);

    return 0;
}

#endif // PLATFORM_EMSCRIPTEN