// Apple platform layer (macOS + iOS/iPadOS in one file, they share Metal / QuartzCore and the app structure).
//
// Contract is the same as platform_windows.cpp: the app calls platform_main(handle, argc, argv) once, then
// platform_tick() every frame, then platform_destroy(). `handle` is the content view (NSView on macOS, UIView on iOS)
// backed by a CAMetalLayer, which is exactly what gfx_metal's surface creation accepts as window_handle.
//
// Frame pacing uses CAMetalDisplayLink (macOS 14+, iOS 17+). On older systems it falls back to a CADisplayLink (iOS)
// or a 60 Hz timer (macOS). Build as Objective-C++ with ARC (-fobjc-arc), link Cocoa/UIKit, Metal, QuartzCore.
#include "platform.h"

#ifdef PLATFORM_APPLE

#include <TargetConditionals.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#if TARGET_OS_IPHONE
    #import <UIKit/UIKit.h>
    #import <UserNotifications/UserNotifications.h>
#else
    #import <Cocoa/Cocoa.h>
#endif

platform_ctx_t* g_platform_ctx = nullptr;

static int                      g_argc = 0;
static char**                   g_argv = nullptr;
@class PlatformFrameDriver;
static PlatformFrameDriver*     g_driver = nil;
static int                      g_target_fps = 0;       // <= 0: display's native rate
static id<CAMetalDrawable>      g_drawable = nil;       // drawable of the frame being ticked (CAMetalDisplayLink path only)

// The drawable CAMetalDisplayLink prepared for the current platform_tick(), or nil (fallback timers, outside a tick).
// A Metal backend should present this one instead of calling [layer nextDrawable], that is the point of the display link.
id<CAMetalDrawable> platform_apple_current_drawable(void) { return g_drawable; }

static void platform_frame(void)
{
    platform_tick(nullptr);
    g_platform_ctx->g_point_states[0].dx = 0;
    g_platform_ctx->g_point_states[0].dy = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Frame drivers
// ---------------------------------------------------------------------------------------------------------------------

@interface PlatformFrameDriver : NSObject
@property (nonatomic) BOOL paused;
+ (PlatformFrameDriver*)driverForLayer:(CAMetalLayer*)layer;
- (void)applyTargetFPS;         // re-reads g_target_fps
- (void)stop;
@end

void platform_set_target_fps(int fps)
{
    g_target_fps = fps;
    [g_driver applyTargetFPS];      // g_driver is nil until platform_main() returned; the driver reads g_target_fps when created
}

int platform_get_max_fps(void)
{
#if TARGET_OS_IPHONE
    return (int)[UIScreen mainScreen].maximumFramesPerSecond;
#else
    if (@available(macOS 12.0, *)) return (int)[NSScreen mainScreen].maximumFramesPerSecond;
    return 60;
#endif
}

// CAMetalDisplayLink: the callback hands over the drawable that is going to be shown for this vsync.
API_AVAILABLE(macos(14.0), ios(17.0))
@interface PlatformMetalLinkDriver : PlatformFrameDriver <CAMetalDisplayLinkDelegate>
- (instancetype)initWithLayer:(CAMetalLayer*)layer;
@end

// Pre-14 / pre-17 fallback.
@interface PlatformTimerDriver : PlatformFrameDriver
- (instancetype)init;
@end

@implementation PlatformFrameDriver
+ (PlatformFrameDriver*)driverForLayer:(CAMetalLayer*)layer
{
    if (@available(macOS 14.0, iOS 17.0, *))
        return [[PlatformMetalLinkDriver alloc] initWithLayer:layer];
    return [[PlatformTimerDriver alloc] init];
}
- (void)applyTargetFPS {}
- (void)stop {}
@end

@implementation PlatformMetalLinkDriver
{
    CAMetalDisplayLink* _link;
}

- (instancetype)initWithLayer:(CAMetalLayer*)layer
{
    if ((self = [super init])) {
        _link = [[CAMetalDisplayLink alloc] initWithMetalLayer:layer];
        _link.delegate = self;
        _link.preferredFrameLatency = 2;                                              // frames in flight
        [self applyTargetFPS];
        [_link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
    return self;
}

- (void)metalDisplayLink:(CAMetalDisplayLink*)link needsUpdate:(CAMetalDisplayLinkUpdate*)update
{
    g_drawable = update.drawable;
    platform_frame();
    g_drawable = nil;
}

// The range is what the system schedules from: it picks the best supported rate inside [min, max] (ProMotion: 120/80/60/40/30...).
- (void)applyTargetFPS
{
    const float max_fps = (float)platform_get_max_fps();
    if (g_target_fps <= 0) _link.preferredFrameRateRange = CAFrameRateRangeMake(30.0f, max_fps, max_fps);
    else {
        const float fps = (float)g_target_fps;
        _link.preferredFrameRateRange = CAFrameRateRangeMake(fps * 0.5f, fps, fps);
    }
}

- (void)setPaused:(BOOL)paused { _link.paused = paused; [super setPaused:paused]; }
- (void)stop                   { [_link invalidate]; _link = nil; }
@end

@implementation PlatformTimerDriver
{
#if TARGET_OS_IPHONE
    CADisplayLink* _link;
#else
    NSTimer*       _timer;
#endif
}

- (instancetype)init
{
    if ((self = [super init])) {
#if TARGET_OS_IPHONE
        _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick)];
        [_link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
#endif
        [self applyTargetFPS];
    }
    return self;
}

- (void)tick { platform_frame(); }

#if TARGET_OS_IPHONE
- (void)applyTargetFPS { _link.preferredFramesPerSecond = g_target_fps > 0 ? g_target_fps : 0; }       // 0 = maximum
#else
- (void)applyTargetFPS
{
    [_timer invalidate];
    _timer = [NSTimer timerWithTimeInterval:1.0 / (g_target_fps > 0 ? g_target_fps : 60) target:self selector:@selector(tick) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
}
#endif

#if TARGET_OS_IPHONE
- (void)setPaused:(BOOL)paused { _link.paused = paused; [super setPaused:paused]; }
- (void)stop                   { [_link invalidate]; _link = nil; }
#else
- (void)setPaused:(BOOL)paused { [super setPaused:paused]; if (paused) { [_timer invalidate]; _timer = nil; } else if (!_timer) [self applyTargetFPS]; }
- (void)stop                   { [_timer invalidate]; _timer = nil; }
#endif
@end

static void platform_start(id view, CAMetalLayer* layer)
{
    platform_main((uintptr_t)(__bridge void*)view, g_argc, g_argv);
    g_driver = [PlatformFrameDriver driverForLayer:layer];
}

static void platform_stop(void)
{
    [g_driver stop];
    g_driver = nil;
    platform_destroy(nullptr);
}

// ---------------------------------------------------------------------------------------------------------------------
#if TARGET_OS_IPHONE
// ---------------------------------------------------------------------------------------------------------------------

@interface PlatformView : UIView
@end

@implementation PlatformView
+ (Class)layerClass { return [CAMetalLayer class]; }

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = YES;
        self.contentScaleFactor = [UIScreen mainScreen].nativeScale;
        self.opaque = YES;
    }
    return self;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    const CGFloat s = self.contentScaleFactor;
    layer.contentsScale = s;
    layer.drawableSize  = CGSizeMake(self.bounds.size.width * s, self.bounds.size.height * s);
}

- (void)pushTouches:(NSSet<UITouch*>*)touches state:(input_state)state
{
    const CGFloat s = self.contentScaleFactor;
    for (UITouch* t in touches) {
        const CGPoint p = [t locationInView:self];
        const CGPoint q = [t previousLocationInView:self];
        push_input_touch_event((int16_t)(p.x * s), (int16_t)(p.y * s), (int16_t)((p.x - q.x) * s), (int16_t)((p.y - q.y) * s),
                               state, (uint64_t)(uintptr_t)(__bridge void*)t);   // the UITouch object identifies a finger
    }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self pushTouches:touches state:input_state_down]; }
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self pushTouches:touches state:input_state_move]; }
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event     { [self pushTouches:touches state:input_state_up]; }
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event { [self pushTouches:touches state:input_state_up]; }
@end

@interface PlatformViewController : UIViewController
@end

@implementation PlatformViewController
- (BOOL)prefersStatusBarHidden                         { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden                 { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskAll; }
@end

@interface PlatformAppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow* window;
@end

@implementation PlatformAppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options
{
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    PlatformView* view = [[PlatformView alloc] initWithFrame:self.window.bounds];
    PlatformViewController* vc = [[PlatformViewController alloc] init];
    vc.view = view;
    self.window.rootViewController = vc;
    [self.window makeKeyAndVisible];
    [view layoutIfNeeded];

    platform_start(view, (CAMetalLayer*)view.layer);
    return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application   { g_driver.paused = YES; }
- (void)applicationDidBecomeActive:(UIApplication*)application    { g_driver.paused = NO; }
- (void)applicationWillTerminate:(UIApplication*)application      { platform_stop(); }
@end

int main(int argc, char** argv)
{
    @autoreleasepool {
        g_platform_ctx = (platform_ctx_t*)calloc(1, sizeof(platform_ctx_t));
        g_argc = argc; g_argv = argv;
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([PlatformAppDelegate class]));
    }
}

int platform_get_monitors(monitor_info_t* infos)
{
    if (!infos) return 1;
    UIScreen* screen = [UIScreen mainScreen];
    const CGFloat s = screen.nativeScale;
    monitor_info_t& m = infos[0];
    m = {};
    snprintf(m.name, sizeof(m.name), "%s", "main");
    m.rect[0] = { 0, 0, (int16_t)(screen.bounds.size.width * s), (int16_t)(screen.bounds.size.height * s) };
    return 1;
}

rect_t platform_get_window_size(uintptr_t handle)
{
    UIView* view = (__bridge UIView*)(void*)handle;
    const CGFloat s = view.contentScaleFactor;
    return { 0, 0, (int16_t)(view.bounds.size.width * s), (int16_t)(view.bounds.size.height * s) };
}

void platform_set_window_size(rect_t rect, bool fullscreen)
{
    (void)rect; (void)fullscreen;       // iOS apps always own the whole screen
}

// Local notifications (UserNotifications). The permission prompt is shown the first time something is scheduled.
// A request with an identifier that is already pending replaces it, so add and update are the same call.
void platform_notification_add(long secondsToDate, const char* message, const char* id)
{
    if (!message || !id) return;
    NSString* identifier = [NSString stringWithUTF8String:id];
    NSString* body       = [NSString stringWithUTF8String:message];
    const NSTimeInterval delay = secondsToDate > 0 ? (NSTimeInterval)secondsToDate : 1.0;   // the trigger needs > 0

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge)
                          completionHandler:^(BOOL granted, NSError* error) {
        if (!granted) return;
        UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
        content.title = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"] ?: [[NSProcessInfo processInfo] processName];
        content.body  = body;
        content.sound = [UNNotificationSound defaultSound];
        UNTimeIntervalNotificationTrigger* trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:delay repeats:NO];
        UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:identifier content:content trigger:trigger];
        [center addNotificationRequest:request withCompletionHandler:nil];
    }];
}

void platform_notification_update(long secondsToDate, const char* message, const char* id)
{
    platform_notification_add(secondsToDate, message, id);
}

void platform_notification_remove(const char* id)
{
    if (!id) return;
    NSArray* ids = @[ [NSString stringWithUTF8String:id] ];
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center removePendingNotificationRequestsWithIdentifiers:ids];
    [center removeDeliveredNotificationsWithIdentifiers:ids];
}

// ---------------------------------------------------------------------------------------------------------------------
#else   // macOS
// ---------------------------------------------------------------------------------------------------------------------

// macOS virtual key code -> the Windows VK numbers the engine uses (Input::Keyboard).
static uint8_t mac_key_to_vk(unsigned short code)
{
    switch (code) {
        case 0: return 'A'; case 11: return 'B'; case 8: return 'C'; case 2: return 'D'; case 14: return 'E'; case 3: return 'F';
        case 5: return 'G'; case 4: return 'H'; case 34: return 'I'; case 38: return 'J'; case 40: return 'K'; case 37: return 'L';
        case 46: return 'M'; case 45: return 'N'; case 31: return 'O'; case 35: return 'P'; case 12: return 'Q'; case 15: return 'R';
        case 1: return 'S'; case 17: return 'T'; case 32: return 'U'; case 9: return 'V'; case 13: return 'W'; case 7: return 'X';
        case 16: return 'Y'; case 6: return 'Z';
        case 29: return '0'; case 18: return '1'; case 19: return '2'; case 20: return '3'; case 21: return '4';
        case 23: return '5'; case 22: return '6'; case 26: return '7'; case 28: return '8'; case 25: return '9';
        case 49: return Input::Keyboard::Space;
        case 36: case 76: return Input::Keyboard::Enter;
        case 48: return Input::Keyboard::Tab;
        case 51: return Input::Keyboard::Back;
        case 53: return 27;                                  // Esc (VK_ESCAPE)
        case 117: return Input::Keyboard::Delete;
        case 115: return Input::Keyboard::Home;   case 119: return Input::Keyboard::End;
        case 116: return Input::Keyboard::PageUp; case 121: return Input::Keyboard::PageDown;
        case 123: return Input::Keyboard::Left;   case 126: return Input::Keyboard::Up;
        case 124: return Input::Keyboard::Right;  case 125: return Input::Keyboard::Down;
        case 122: return Input::Keyboard::F1;  case 120: return Input::Keyboard::F2;  case 99:  return Input::Keyboard::F3;
        case 118: return Input::Keyboard::F4;  case 96:  return Input::Keyboard::F5;  case 97:  return Input::Keyboard::F6;
        case 98:  return Input::Keyboard::F7;  case 100: return Input::Keyboard::F8;  case 101: return Input::Keyboard::F9;
        case 109: return Input::Keyboard::F10; case 103: return Input::Keyboard::F11; case 111: return Input::Keyboard::F12;
        case 82: return Input::Keyboard::NumPad_0; case 83: return Input::Keyboard::NumPad_1; case 84: return Input::Keyboard::NumPad_2;
        case 85: return Input::Keyboard::NumPad_3; case 86: return Input::Keyboard::NumPad_4; case 87: return Input::Keyboard::NumPad_5;
        case 88: return Input::Keyboard::NumPad_6; case 89: return Input::Keyboard::NumPad_7; case 91: return Input::Keyboard::NumPad_8;
        case 92: return Input::Keyboard::NumPad_9;
        default: return 0;
    }
}

@interface PlatformView : NSView
@end

@implementation PlatformView
- (BOOL)isFlipped                { return YES; }            // y grows downwards like on Windows
- (BOOL)acceptsFirstResponder    { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)e { return YES; }
- (BOOL)wantsUpdateLayer         { return YES; }
- (CALayer*)makeBackingLayer     { return [CAMetalLayer layer]; }

- (instancetype)initWithFrame:(NSRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
    }
    return self;
}

- (void)updateDrawableSize
{
    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    const CGFloat s = self.window ? self.window.backingScaleFactor : [NSScreen mainScreen].backingScaleFactor;
    layer.contentsScale = s;
    layer.drawableSize  = CGSizeMake(self.bounds.size.width * s, self.bounds.size.height * s);
}

- (void)setFrameSize:(NSSize)size                { [super setFrameSize:size]; [self updateDrawableSize]; }
- (void)viewDidChangeBackingProperties           { [super viewDidChangeBackingProperties]; [self updateDrawableSize]; }
- (void)viewDidMoveToWindow                      { [super viewDidMoveToWindow]; [self updateDrawableSize]; }

// mouse: position in backing pixels, delta sign is the same as platform_windows.cpp (it negates the raw delta)
- (void)pushMouse:(NSEvent*)e state:(input_state)state button:(mouse_button)button
{
    const NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    const CGFloat s = self.window.backingScaleFactor;
    push_input_mouse_event((int16_t)(p.x * s), (int16_t)(p.y * s), (int16_t)(-e.deltaX * s), (int16_t)(-e.deltaY * s), state, button);
}

- (void)mouseMoved:(NSEvent*)e          { [self pushMouse:e state:input_state_move button:none]; }
- (void)mouseDragged:(NSEvent*)e        { [self pushMouse:e state:input_state_move button:none]; }
- (void)rightMouseDragged:(NSEvent*)e   { [self pushMouse:e state:input_state_move button:none]; }
- (void)otherMouseDragged:(NSEvent*)e   { [self pushMouse:e state:input_state_move button:none]; }
- (void)mouseDown:(NSEvent*)e           { [self pushMouse:e state:input_state_down button:mouse_btn_left]; }
- (void)mouseUp:(NSEvent*)e             { [self pushMouse:e state:input_state_up   button:mouse_btn_left]; }
- (void)rightMouseDown:(NSEvent*)e      { [self pushMouse:e state:input_state_down button:mouse_btn_right]; }
- (void)rightMouseUp:(NSEvent*)e        { [self pushMouse:e state:input_state_up   button:mouse_btn_right]; }

static mouse_button other_button(NSEvent* e)
{
    switch (e.buttonNumber) { case 2: return mouse_btn_middle; case 3: return mouse_btn_extra0; default: return mouse_btn_extra1; }
}
- (void)otherMouseDown:(NSEvent*)e      { [self pushMouse:e state:input_state_down button:other_button(e)]; }
- (void)otherMouseUp:(NSEvent*)e        { [self pushMouse:e state:input_state_up   button:other_button(e)]; }

- (void)keyDown:(NSEvent*)e
{
    const uint8_t vk = mac_key_to_vk(e.keyCode);
    if (vk && !e.isARepeat) push_input_keyboard_event(vk, input_state_down);
}

- (void)keyUp:(NSEvent*)e
{
    const uint8_t vk = mac_key_to_vk(e.keyCode);
    if (vk) push_input_keyboard_event(vk, input_state_up);
}

// modifier keys only arrive as flagsChanged; the flag says whether the key went down or up
- (void)flagsChanged:(NSEvent*)e
{
    uint8_t vk = 0;
    NSEventModifierFlags flag = 0;
    switch (e.keyCode) {
        case 56: case 60: vk = Input::Keyboard::Shift;    flag = NSEventModifierFlagShift;    break;
        case 59: case 62: vk = Input::Keyboard::Ctrl;     flag = NSEventModifierFlagControl;  break;
        case 58:          vk = Input::Keyboard::Alt_L;    flag = NSEventModifierFlagOption;   break;
        case 61:          vk = Input::Keyboard::Alt_R;    flag = NSEventModifierFlagOption;   break;
        case 57:          vk = Input::Keyboard::CapsLock; flag = NSEventModifierFlagCapsLock; break;
        default: return;
    }
    push_input_keyboard_event(vk, (e.modifierFlags & flag) ? input_state_down : input_state_up);
}
@end

@interface PlatformAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (nonatomic, strong) NSWindow* window;
@end

@implementation PlatformAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)n
{
    const NSRect screen = [NSScreen mainScreen].frame;
    const NSRect rect = NSMakeRect(screen.size.width * 0.25, screen.size.height * 0.25, screen.size.width * 0.5, screen.size.height * 0.5);
    const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

    self.window = [[NSWindow alloc] initWithContentRect:rect styleMask:style backing:NSBackingStoreBuffered defer:NO];
    self.window.delegate = self;
    self.window.title = [[NSProcessInfo processInfo] processName];
    self.window.acceptsMouseMovedEvents = YES;
    self.window.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;

    PlatformView* view = [[PlatformView alloc] initWithFrame:[self.window contentRectForFrameRect:self.window.frame]];
    self.window.contentView = view;
    [self.window makeFirstResponder:view];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    [view updateDrawableSize];

    platform_start(view, (CAMetalLayer*)view.layer);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app { return YES; }
- (void)applicationWillTerminate:(NSNotification*)n                         { platform_stop(); }
- (void)windowDidMiniaturize:(NSNotification*)n                             { g_driver.paused = YES; }
- (void)windowDidDeminiaturize:(NSNotification*)n                           { g_driver.paused = NO; }
@end

static PlatformAppDelegate* g_delegate = nil;       // NSApplication.delegate is a weak reference

int main(int argc, char** argv)
{
    @autoreleasepool {
        g_platform_ctx = (platform_ctx_t*)calloc(1, sizeof(platform_ctx_t));
        g_argc = argc; g_argv = argv;

        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSMenu* menubar = [[NSMenu alloc] init];
        NSMenuItem* app_item = [[NSMenuItem alloc] init];
        [menubar addItem:app_item];
        NSMenu* app_menu = [[NSMenu alloc] init];
        NSString* quit = [@"Quit " stringByAppendingString:[[NSProcessInfo processInfo] processName]];
        [app_menu addItem:[[NSMenuItem alloc] initWithTitle:quit action:@selector(terminate:) keyEquivalent:@"q"]];
        app_item.submenu = app_menu;
        app.mainMenu = menubar;

        g_delegate = [[PlatformAppDelegate alloc] init];
        app.delegate = g_delegate;
        [app run];
    }
    return 0;
}

int platform_get_monitors(monitor_info_t* infos)
{
    NSArray<NSScreen*>* screens = [NSScreen screens];
    if (!infos) return (int)screens.count;
    int n = 0;
    for (NSScreen* screen in screens) {
        monitor_info_t& m = infos[n];
        m = {};
        snprintf(m.name, sizeof(m.name), "%s", screen.localizedName.UTF8String);
        const CGFloat s = screen.backingScaleFactor;
        m.rect[0] = { (int16_t)(screen.frame.origin.x * s), (int16_t)(screen.frame.origin.y * s),
                      (int16_t)(screen.frame.size.width * s), (int16_t)(screen.frame.size.height * s) };
        m.handle = (intptr_t)[[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
        ++n;
    }
    return n;
}

// Client area in pixels, {0, 0, width, height}: callers use (w - x) and (h - y), which works with this and with platform_windows.cpp.
rect_t platform_get_window_size(uintptr_t handle)
{
    NSView* view = (__bridge NSView*)(void*)handle;
    const CGFloat s = view.window ? view.window.backingScaleFactor : [NSScreen mainScreen].backingScaleFactor;
    return { 0, 0, (int16_t)(view.bounds.size.width * s), (int16_t)(view.bounds.size.height * s) };
}

void platform_set_window_size(rect_t rect, bool fullscreen)
{
    NSWindow* window = g_delegate.window;
    if (!window) return;
    const bool is_full = (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if (is_full != fullscreen) [window toggleFullScreen:nil];
    if (!fullscreen && rect.w > 0 && rect.h > 0) {
        const CGFloat s = window.backingScaleFactor;
        [window setContentSize:NSMakeSize((rect.w - rect.x) / s, (rect.h - rect.y) / s)];
    }
}

#endif  // TARGET_OS_IPHONE

#endif  // PLATFORM_APPLE
