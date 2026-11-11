// Android platform layer (NativeActivity + android_native_app_glue).
//
// Same contract as the other platforms: platform_main(handle, argc, argv) once the window exists, platform_tick() every
// frame, platform_destroy() at the end. `handle` is the ANativeWindow* (what gfx_vulkan needs for vkCreateAndroidSurfaceKHR).
// Frame pacing uses AChoreographer, the Android counterpart of CAMetalDisplayLink / CADisplayLink: a callback per vsync that
// re-posts itself. Frames only run while the app is resumed and has a window.
//
// Window lifetime: Android destroys the window when the app goes to the background and gives a new one on return. The engine
// has no "recreate the surface" call yet, so TERM_WINDOW runs platform_destroy() and the next INIT_WINDOW runs platform_main()
// again, i.e. the app restarts its graphics state with the new window.
//
// Build: link android, log and native_app_glue (add android_native_app_glue.c from the NDK to the sources), min API 24
// (AChoreographer). setFrameRate needs API 30 and is used when available, older devices get a software frame skip.
#include "platform.h"

#ifdef PLATFORM_ANDROID

#include <android/api-level.h>
#include <android/choreographer.h>
#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <stdio.h>
#include <time.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "platform", __VA_ARGS__)

platform_ctx_t* g_platform_ctx = nullptr;

static android_app*  g_app = nullptr;
static int           g_target_fps = 0;          // <= 0: display's native rate
static bool          g_resumed = false;         // between APP_CMD_RESUME and APP_CMD_PAUSE
static bool          g_has_window = false;      // platform_main() was called for the current window
static bool          g_frame_pending = false;   // a choreographer callback is queued
static bool          g_exit = false;
static int64_t       g_last_tick_ns = 0;

// ---------------------------------------------------------------------------------------------------------------------
// Frame pacing
// ---------------------------------------------------------------------------------------------------------------------

static bool window_frame_rate_supported()
{
    return __builtin_available(android 30, *);
}

// Hint for the compositor (API 30+): the display picks the closest rate it can do. 0 = no preference.
static void apply_window_frame_rate()
{
    if (!g_app || !g_app->window) return;
    if (__builtin_available(android 30, *))
        ANativeWindow_setFrameRate(g_app->window, (float)(g_target_fps > 0 ? g_target_fps : 0), ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT);
}

static void post_frame();

#if __ANDROID_API__ >= 29
static void on_frame(int64_t frame_time_ns, void*)
#else
static void on_frame(long frame_time_ns, void*)
#endif
{
    g_frame_pending = false;
    if (!g_resumed || !g_has_window || g_exit) return;

    // before API 30 the system doesn't know our rate, so skip vsyncs to approximate it
    bool skip = false;
    if (g_target_fps > 0 && !window_frame_rate_supported()) {
        const int64_t period = 1000000000LL / g_target_fps;
        skip = (int64_t)frame_time_ns - g_last_tick_ns < period * 9 / 10;
    }
    if (!skip) {
        g_last_tick_ns = (int64_t)frame_time_ns;
        platform_tick(nullptr);
        g_platform_ctx->g_point_states[0].dx = 0;
        g_platform_ctx->g_point_states[0].dy = 0;
    }
    post_frame();
}

static void post_frame()
{
    if (g_frame_pending || !g_resumed || !g_has_window) return;
    g_frame_pending = true;
#if __ANDROID_API__ >= 29
    AChoreographer_postFrameCallback64(AChoreographer_getInstance(), on_frame, nullptr);
#else
    AChoreographer_postFrameCallback(AChoreographer_getInstance(), on_frame, nullptr);
#endif
}

void platform_set_target_fps(int fps)
{
    g_target_fps = fps;
    apply_window_frame_rate();
}

// Current refresh rate of the default display through JNI: activity.getWindowManager().getDefaultDisplay().getRefreshRate().
// Android doesn't have a cheap "maximum" query, on variable refresh devices this is the rate of the active mode.
int platform_get_max_fps(void)
{
    if (!g_app || !g_app->activity) return 60;
    JavaVM* vm = g_app->activity->vm;
    JNIEnv* env = nullptr;
    const bool attached = vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK;
    if (!attached && vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return 60;

    float rate = 0.0f;
    jobject activity = g_app->activity->clazz;
    jclass activity_cls = env->GetObjectClass(activity);
    jmethodID get_wm = env->GetMethodID(activity_cls, "getWindowManager", "()Landroid/view/WindowManager;");
    jobject wm = get_wm ? env->CallObjectMethod(activity, get_wm) : nullptr;
    if (wm) {
        jclass wm_cls = env->GetObjectClass(wm);
        jmethodID get_display = env->GetMethodID(wm_cls, "getDefaultDisplay", "()Landroid/view/Display;");
        jobject display = get_display ? env->CallObjectMethod(wm, get_display) : nullptr;
        if (display) {
            jclass display_cls = env->GetObjectClass(display);
            jmethodID get_rate = env->GetMethodID(display_cls, "getRefreshRate", "()F");
            if (get_rate) rate = env->CallFloatMethod(display, get_rate);
            env->DeleteLocalRef(display_cls);
            env->DeleteLocalRef(display);
        }
        env->DeleteLocalRef(wm_cls);
        env->DeleteLocalRef(wm);
    }
    env->DeleteLocalRef(activity_cls);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!attached) vm->DetachCurrentThread();

    return rate >= 1.0f ? (int)(rate + 0.5f) : 60;
}

// ---------------------------------------------------------------------------------------------------------------------
// Local notifications: the work is done by android/PlatformNotifications.java (AlarmManager + BroadcastReceiver)
// ---------------------------------------------------------------------------------------------------------------------

// Runs `fn(env, notifications_class, activity)` on an attached thread. The class is looked up through the activity's class
// loader, because FindClass on a native thread only sees system classes.
template <typename F>
static void with_notifications(F fn)
{
    if (!g_app || !g_app->activity) return;
    JavaVM* vm = g_app->activity->vm;
    JNIEnv* env = nullptr;
    const bool attached = vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK;
    if (!attached && vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;

    jobject activity = g_app->activity->clazz;
    jclass activity_cls = env->GetObjectClass(activity);
    jobject loader = env->CallObjectMethod(activity, env->GetMethodID(activity_cls, "getClassLoader", "()Ljava/lang/ClassLoader;"));
    jclass loader_cls = env->GetObjectClass(loader);
    jstring name = env->NewStringUTF("platform.PlatformNotifications");
    jclass cls = (jclass)env->CallObjectMethod(loader, env->GetMethodID(loader_cls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"), name);
    if (env->ExceptionCheck()) { env->ExceptionClear(); LOGI("platform.PlatformNotifications is missing from the app"); cls = nullptr; }

    if (cls) {
        fn(env, cls, activity);
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
        env->DeleteLocalRef(cls);
    }
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(loader_cls);
    env->DeleteLocalRef(loader);
    env->DeleteLocalRef(activity_cls);
    if (!attached) vm->DetachCurrentThread();
}

void platform_notification_add(long secondsToDate, const char* message, const char* id)
{
    if (!message || !id) return;
    with_notifications([&](JNIEnv* env, jclass cls, jobject activity) {
        jmethodID m = env->GetStaticMethodID(cls, "add", "(Landroid/app/Activity;JLjava/lang/String;Ljava/lang/String;)V");
        if (!m) return;
        jstring jmsg = env->NewStringUTF(message), jid = env->NewStringUTF(id);
        env->CallStaticVoidMethod(cls, m, activity, (jlong)secondsToDate, jmsg, jid);
        env->DeleteLocalRef(jmsg);
        env->DeleteLocalRef(jid);
    });
}

// Same id replaces the pending notification, so update is add.
void platform_notification_update(long secondsToDate, const char* message, const char* id)
{
    platform_notification_add(secondsToDate, message, id);
}

void platform_notification_remove(const char* id)
{
    if (!id) return;
    with_notifications([&](JNIEnv* env, jclass cls, jobject activity) {
        jmethodID m = env->GetStaticMethodID(cls, "remove", "(Landroid/app/Activity;Ljava/lang/String;)V");
        if (!m) return;
        jstring jid = env->NewStringUTF(id);
        env->CallStaticVoidMethod(cls, m, activity, jid);
        env->DeleteLocalRef(jid);
    });
}

// ---------------------------------------------------------------------------------------------------------------------
// Window size / monitors
// ---------------------------------------------------------------------------------------------------------------------

rect_t platform_get_window_size(uintptr_t handle)
{
    ANativeWindow* window = (ANativeWindow*)handle;
    if (!window) return { 0, 0, 0, 0 };
    return { 0, 0, (int16_t)ANativeWindow_getWidth(window), (int16_t)ANativeWindow_getHeight(window) };
}

int platform_get_monitors(monitor_info_t* infos)
{
    if (!infos) return 1;
    infos[0] = {};
    snprintf(infos[0].name, sizeof(infos[0].name), "%s", "main");
    if (g_app && g_app->window)
        infos[0].rect[0] = platform_get_window_size((uintptr_t)g_app->window);
    return 1;
}

void platform_set_window_size(rect_t rect, bool fullscreen)
{
    (void)rect; (void)fullscreen;       // the activity owns the screen
}

// ---------------------------------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------------------------------

// Android key code -> Windows VK numbers (Input::Keyboard), 0 = not mapped.
static uint8_t android_key_to_vk(int32_t code)
{
    if (code >= AKEYCODE_A && code <= AKEYCODE_Z)   return (uint8_t)('A' + (code - AKEYCODE_A));
    if (code >= AKEYCODE_0 && code <= AKEYCODE_9)   return (uint8_t)('0' + (code - AKEYCODE_0));
    if (code >= AKEYCODE_F1 && code <= AKEYCODE_F12) return (uint8_t)(Input::Keyboard::F1 + (code - AKEYCODE_F1));
    switch (code) {
        case AKEYCODE_SPACE:        return Input::Keyboard::Space;
        case AKEYCODE_ENTER:        return Input::Keyboard::Enter;
        case AKEYCODE_TAB:          return Input::Keyboard::Tab;
        case AKEYCODE_DEL:          return Input::Keyboard::Back;          // Android's "DEL" is backspace
        case AKEYCODE_FORWARD_DEL:  return Input::Keyboard::Delete;
        case AKEYCODE_ESCAPE:       return 27;
        case AKEYCODE_DPAD_LEFT:    return Input::Keyboard::Left;
        case AKEYCODE_DPAD_UP:      return Input::Keyboard::Up;
        case AKEYCODE_DPAD_RIGHT:   return Input::Keyboard::Right;
        case AKEYCODE_DPAD_DOWN:    return Input::Keyboard::Down;
        case AKEYCODE_MOVE_HOME:    return Input::Keyboard::Home;
        case AKEYCODE_MOVE_END:     return Input::Keyboard::End;
        case AKEYCODE_PAGE_UP:      return Input::Keyboard::PageUp;
        case AKEYCODE_PAGE_DOWN:    return Input::Keyboard::PageDown;
        case AKEYCODE_SHIFT_LEFT:   case AKEYCODE_SHIFT_RIGHT: return Input::Keyboard::Shift;
        case AKEYCODE_CTRL_LEFT:    case AKEYCODE_CTRL_RIGHT:  return Input::Keyboard::Ctrl;
        case AKEYCODE_ALT_LEFT:     return Input::Keyboard::Alt_L;
        case AKEYCODE_ALT_RIGHT:    return Input::Keyboard::Alt_R;
        default:                    return 0;
    }
}

static void handle_touch(AInputEvent* ev)
{
    static float prev_x[PLATFORM_MAX_MULTITOUCH], prev_y[PLATFORM_MAX_MULTITOUCH];

    const int32_t action = AMotionEvent_getAction(ev);
    const int32_t type   = action & AMOTION_EVENT_ACTION_MASK;
    const size_t  index  = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const size_t  count  = AMotionEvent_getPointerCount(ev);

    for (size_t i = 0; i < count; ++i) {
        const int32_t id = AMotionEvent_getPointerId(ev, i);
        const float x = AMotionEvent_getX(ev, i), y = AMotionEvent_getY(ev, i);
        const int slot = id < PLATFORM_MAX_MULTITOUCH ? id : PLATFORM_MAX_MULTITOUCH - 1;

        input_state state = input_state_none;
        switch (type) {
            case AMOTION_EVENT_ACTION_DOWN:         state = input_state_down; break;
            case AMOTION_EVENT_ACTION_UP:           state = input_state_up;   break;
            case AMOTION_EVENT_ACTION_POINTER_DOWN: state = (i == index) ? input_state_down : input_state_none; break;
            case AMOTION_EVENT_ACTION_POINTER_UP:   state = (i == index) ? input_state_up   : input_state_none; break;
            case AMOTION_EVENT_ACTION_MOVE:         state = input_state_move; break;
            case AMOTION_EVENT_ACTION_CANCEL:       state = input_state_up;   break;
        }
        if (state == input_state_none) continue;

        const bool first = state == input_state_down;
        const int16_t dx = first ? 0 : (int16_t)(x - prev_x[slot]);
        const int16_t dy = first ? 0 : (int16_t)(y - prev_y[slot]);
        prev_x[slot] = x; prev_y[slot] = y;
        // pointer ids start at 0, which touch_id_2_id treats as "free slot": use id + 1 as the finger key
        push_input_touch_event((int16_t)x, (int16_t)y, dx, dy, state, (uint64_t)(id + 1));
    }
}

static void handle_mouse(AInputEvent* ev)
{
    static int32_t prev_buttons = 0;
    static float   prev_x = 0, prev_y = 0;

    const float x = AMotionEvent_getX(ev, 0), y = AMotionEvent_getY(ev, 0);
    const int16_t dx = (int16_t)(prev_x - x), dy = (int16_t)(prev_y - y);   // same sign as platform_windows.cpp
    prev_x = x; prev_y = y;

    const int32_t buttons = AMotionEvent_getButtonState(ev);
    const int32_t changed = buttons ^ prev_buttons;
    prev_buttons = buttons;

    static const struct { int32_t mask; mouse_button btn; } map[] = {
        { AMOTION_EVENT_BUTTON_PRIMARY,   mouse_btn_left   },
        { AMOTION_EVENT_BUTTON_SECONDARY, mouse_btn_right  },
        { AMOTION_EVENT_BUTTON_TERTIARY,  mouse_btn_middle },
        { AMOTION_EVENT_BUTTON_BACK,      mouse_btn_extra0 },
        { AMOTION_EVENT_BUTTON_FORWARD,   mouse_btn_extra1 },
    };
    bool any = false;
    for (const auto& m : map)
        if (changed & m.mask) {
            push_input_mouse_event((int16_t)x, (int16_t)y, dx, dy, (buttons & m.mask) ? input_state_down : input_state_up, m.btn);
            any = true;
        }
    if (!any) push_input_mouse_event((int16_t)x, (int16_t)y, dx, dy, input_state_move, none);
}

static int32_t on_input(android_app*, AInputEvent* ev)
{
    switch (AInputEvent_getType(ev)) {
        case AINPUT_EVENT_TYPE_MOTION: {
            const int32_t source = AInputEvent_getSource(ev);
            if ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE) handle_mouse(ev);
            else                                                       handle_touch(ev);
            return 1;
        }
        case AINPUT_EVENT_TYPE_KEY: {
            const uint8_t vk = android_key_to_vk(AKeyEvent_getKeyCode(ev));
            if (!vk) return 0;                                           // e.g. the system Back key stays with the OS
            const int32_t action = AKeyEvent_getAction(ev);
            if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(ev) == 0) push_input_keyboard_event(vk, input_state_down);
            if (action == AKEY_EVENT_ACTION_UP)                                        push_input_keyboard_event(vk, input_state_up);
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------------------------------------------------

static void on_cmd(android_app* app, int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window && !g_has_window) {
                apply_window_frame_rate();
                platform_main((uintptr_t)app->window, 0, nullptr);
                g_has_window = true;
                post_frame();
            }
            break;
        case APP_CMD_TERM_WINDOW:
            if (g_has_window) {
                g_has_window = false;
                platform_destroy(nullptr);
            }
            break;
        case APP_CMD_RESUME:
            g_resumed = true;
            post_frame();
            break;
        case APP_CMD_PAUSE:
            g_resumed = false;
            break;
        case APP_CMD_DESTROY:
            g_exit = true;
            break;
    }
}

void android_main(android_app* app)
{
    g_app = app;
    g_platform_ctx = (platform_ctx_t*)calloc(1, sizeof(platform_ctx_t));
    app->onAppCmd = on_cmd;
    app->onInputEvent = on_input;
    LOGI("android_main");

    while (!g_exit && !app->destroyRequested) {
        int events = 0;
        android_poll_source* source = nullptr;
        // block until something happens: frames arrive as choreographer callbacks, dispatched by the looper
        const int ident = ALooper_pollOnce(-1, nullptr, &events, (void**)&source);
        if (ident == ALOOPER_POLL_ERROR) break;
        if (source) source->process(app, source);
    }

    if (g_has_window) {
        g_has_window = false;
        platform_destroy(nullptr);
    }
}

#endif // PLATFORM_ANDROID
