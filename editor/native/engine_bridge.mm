#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <cmath>
#include <memory>
#include <string>
#include <napi.h>

using RuntimeCreateFn = void *(*)(const char *projectFile, void *metalView,
                                  void *sdlInputWindow);
using RuntimeEndFn = void (*)(void *runtimeContext);
using RuntimeDestroyFn = void (*)(void *runtimeContext);
using RuntimeResizeFn = bool (*)(void *runtimeContext, int width, int height,
                                 float scale);
using RuntimeSetEditorControlsEnabledFn = bool (*)(void *runtimeContext,
                                                   bool enabled);
using RuntimeSetEditorSimulationEnabledFn = bool (*)(void *runtimeContext,
                                                     bool enabled);
using RuntimeSetEditorControlModeFn = bool (*)(void *runtimeContext, int mode);
using RuntimeEditorPointerEventFn = bool (*)(void *runtimeContext, int action,
                                             float x, float y, int button,
                                             float scale);
using RuntimeEditorScrollEventFn = bool (*)(void *runtimeContext, float delta,
                                            float scale);
using RuntimeEditorKeyEventFn = bool (*)(void *runtimeContext, int key,
                                         bool pressed);
using RuntimeGetSelectedObjectIdFn = int (*)(void *runtimeContext);
using RuntimeGetSelectedObjectNameFn = const char *(*)(void *runtimeContext);
using RuntimeStepFn = bool (*)(void *runtimeContext);

struct BridgeState {
    void *dylibHandle = nullptr;

    RuntimeCreateFn createFn = nullptr;
    RuntimeEndFn endFn = nullptr;
    RuntimeDestroyFn destroyFn = nullptr;
    RuntimeResizeFn resizeFn = nullptr;
    RuntimeSetEditorControlsEnabledFn setEditorControlsEnabledFn = nullptr;
    RuntimeSetEditorSimulationEnabledFn setEditorSimulationEnabledFn = nullptr;
    RuntimeSetEditorControlModeFn setEditorControlModeFn = nullptr;
    RuntimeEditorPointerEventFn editorPointerEventFn = nullptr;
    RuntimeEditorScrollEventFn editorScrollEventFn = nullptr;
    RuntimeEditorKeyEventFn editorKeyEventFn = nullptr;
    RuntimeGetSelectedObjectIdFn getSelectedObjectIdFn = nullptr;
    RuntimeGetSelectedObjectNameFn getSelectedObjectNameFn = nullptr;
    RuntimeStepFn stepFn = nullptr;

    void *runtimeContext = nullptr;

    NSView *hostView = nil;
    NSView *childView = nil;
    NSView *dragView = nil;
    id scrollMonitor = nil;
};

struct BridgeState bridgeState;
static constexpr CGFloat EditorTitlebarHeight = 40.0;
static constexpr CGFloat EditorWindowControlsWidth = 86.0;

static void sendEditorPointerEvent(NSEvent *event, NSView *view, int action) {
    if (!bridgeState.runtimeContext || !bridgeState.editorPointerEventFn ||
        !view) {
        return;
    }

    NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
    NSRect bounds = [view bounds];
    CGFloat y = bounds.size.height - point.y;
    NSInteger buttonNumber = [event buttonNumber];
    int button = buttonNumber <= 0 ? 1 : static_cast<int>(buttonNumber + 1);
    CGFloat scale = 1.0;
    if ([view window]) {
        CGFloat backingScale = [[view window] backingScaleFactor];
        if (backingScale > 0.0) {
            scale = backingScale;
        }
    }
    bridgeState.editorPointerEventFn(bridgeState.runtimeContext, action,
                                     static_cast<float>(point.x),
                                     static_cast<float>(y), button,
                                     static_cast<float>(scale));
}

static void sendEditorScrollEvent(NSEvent *event, NSView *view) {
    if (!bridgeState.runtimeContext || !view) {
        return;
    }

    CGFloat scale = 1.0;
    if ([view window]) {
        CGFloat backingScale = [[view window] backingScaleFactor];
        if (backingScale > 0.0) {
            scale = backingScale;
        }
    }

    CGFloat delta = [event scrollingDeltaY];
    if (std::abs(delta) < 0.0001) {
        delta = [event deltaY];
    }
    if ([event hasPreciseScrollingDeltas]) {
        delta *= 0.75;
    } else {
        delta *= 4.0;
    }
    if (std::abs(delta) < 0.0001) {
        return;
    }

    if (bridgeState.editorScrollEventFn) {
        bridgeState.editorScrollEventFn(bridgeState.runtimeContext,
                                        static_cast<float>(delta),
                                        static_cast<float>(scale));
        return;
    }

    if (bridgeState.editorPointerEventFn) {
        bridgeState.editorPointerEventFn(bridgeState.runtimeContext, 3, 0.0f,
                                         static_cast<float>(delta), 0,
                                         static_cast<float>(scale));
    }
}

static bool eventIsInsideView(NSEvent *event, NSView *view) {
    if (!event || !view || ![view window] || [event window] != [view window]) {
        return false;
    }
    NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
    return NSPointInRect(point, [view bounds]);
}

static void installScrollMonitor() {
    if (bridgeState.scrollMonitor || !bridgeState.childView) {
        return;
    }

    bridgeState.scrollMonitor =
        [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                             handler:^NSEvent *(NSEvent *event) {
                                               if (eventIsInsideView(
                                                       event,
                                                       bridgeState.childView)) {
                                                   sendEditorScrollEvent(
                                                       event,
                                                       bridgeState.childView);
                                               }
                                               return event;
                                             }];
}

static void removeScrollMonitor() {
    if (!bridgeState.scrollMonitor) {
        return;
    }
    [NSEvent removeMonitor:bridgeState.scrollMonitor];
    bridgeState.scrollMonitor = nil;
}

static int editorKeyFromEvent(NSEvent *event) {
    switch ([event keyCode]) {
    case 126:
        return 0;
    case 125:
        return 1;
    case 123:
        return 2;
    case 124:
        return 3;
    default:
        return -1;
    }
}

static bool sendEditorKeyEvent(NSEvent *event, bool pressed) {
    if (!bridgeState.runtimeContext || !bridgeState.editorKeyEventFn) {
        return false;
    }
    int key = editorKeyFromEvent(event);
    if (key < 0) {
        return false;
    }
    bridgeState.editorKeyEventFn(bridgeState.runtimeContext, key, pressed);
    return true;
}

@interface AtlasWindowDragView : NSView
@end

@implementation AtlasWindowDragView
- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setWantsLayer:YES];
        [[self layer] setBackgroundColor:[[NSColor clearColor] CGColor]];
    }
    return self;
}

- (BOOL)isOpaque {
    return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    (void)event;
    return YES;
}

- (NSView *)hitTest:(NSPoint)point {
    if (point.x >= 0.0 && point.x < EditorWindowControlsWidth &&
        point.y >= 0.0 && point.y < EditorTitlebarHeight) {
        return nil;
    }
    return [super hitTest:point];
}

- (void)mouseDown:(NSEvent *)event {
    [[self window] performWindowDragWithEvent:event];
}
@end

@interface AtlasRuntimeView : NSView
@end

@implementation AtlasRuntimeView
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    (void)event;
    return YES;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if ([self window]) {
        [[self window] setAcceptsMouseMovedEvents:YES];
        [[self window] makeFirstResponder:self];
    }
}

- (BOOL)isFlipped {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    [[self window] makeFirstResponder:self];
    sendEditorPointerEvent(event, self, 0);
}

- (void)mouseDragged:(NSEvent *)event {
    sendEditorPointerEvent(event, self, 1);
}

- (void)mouseMoved:(NSEvent *)event {
    sendEditorPointerEvent(event, self, 1);
}

- (void)mouseUp:(NSEvent *)event {
    sendEditorPointerEvent(event, self, 2);
}

- (void)rightMouseDown:(NSEvent *)event {
    [[self window] makeFirstResponder:self];
    sendEditorPointerEvent(event, self, 0);
}

- (void)rightMouseDragged:(NSEvent *)event {
    sendEditorPointerEvent(event, self, 1);
}

- (void)rightMouseUp:(NSEvent *)event {
    sendEditorPointerEvent(event, self, 2);
}

- (void)scrollWheel:(NSEvent *)event {
    sendEditorScrollEvent(event, self);
}

- (BOOL)wantsScrollEventsForSwipeTrackingOnAxis:(NSEventGestureAxis)axis {
    (void)axis;
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    if (!bridgeState.runtimeContext || !bridgeState.setEditorControlModeFn) {
        if (!sendEditorKeyEvent(event, true)) {
            [super keyDown:event];
        }
        return;
    }

    NSString *characters = [event charactersIgnoringModifiers];
    if ([characters length] == 0) {
        if (!sendEditorKeyEvent(event, true)) {
            [super keyDown:event];
        }
        return;
    }

    unichar key = [[characters lowercaseString] characterAtIndex:0];
    int mode = -1;
    if (key == 'q') {
        mode = 0;
    } else if (key == 'w') {
        mode = 1;
    } else if (key == 'e') {
        mode = 2;
    } else if (key == 'r') {
        mode = 3;
    }

    if (mode >= 0) {
        bridgeState.setEditorControlModeFn(bridgeState.runtimeContext, mode);
    } else if (sendEditorKeyEvent(event, true)) {
        return;
    } else {
        [super keyDown:event];
    }
}

- (void)keyUp:(NSEvent *)event {
    if (!sendEditorKeyEvent(event, false)) {
        [super keyUp:event];
    }
}
@end

static NSRect titlebarDragFrame(NSView *hostView) {
    NSView *parentView = [hostView superview] ?: hostView;
    NSRect bounds = [parentView bounds];
    CGFloat y = [parentView isFlipped] ? NSMinY(bounds)
                                      : NSMaxY(bounds) - EditorTitlebarHeight;
    return NSMakeRect(NSMinX(bounds), y, NSWidth(bounds), EditorTitlebarHeight);
}

static void installTitlebarDragView(NSView *hostView) {
    if (!hostView || bridgeState.dragView) {
        return;
    }

    NSView *parentView = [hostView superview] ?: hostView;
    NSView *dragView =
        [[AtlasWindowDragView alloc] initWithFrame:titlebarDragFrame(hostView)];
    [dragView setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin)];

    if ([hostView superview]) {
        [parentView addSubview:dragView
                    positioned:NSWindowAbove
                    relativeTo:hostView];
    } else {
        [hostView addSubview:dragView positioned:NSWindowAbove relativeTo:nil];
    }

    bridgeState.dragView = dragView;
    if ([hostView window]) {
        [[hostView window] setMovableByWindowBackground:YES];
    }
}

static void resizeTitlebarDragView() {
    if (!bridgeState.hostView || !bridgeState.dragView) {
        return;
    }
    [bridgeState.dragView setFrame:titlebarDragFrame(bridgeState.hostView)];
}

static void unloadEditorIfNeeded() {
    removeScrollMonitor();

    if (bridgeState.runtimeContext && bridgeState.endFn) {
        bridgeState.endFn(bridgeState.runtimeContext);
    }

    if (bridgeState.runtimeContext && bridgeState.destroyFn) {
        bridgeState.destroyFn(bridgeState.runtimeContext);
        bridgeState.runtimeContext = nullptr;
    }

    if (bridgeState.childView) {
        [bridgeState.childView removeFromSuperview];
        bridgeState.childView = nil;
    }

    if (bridgeState.dragView) {
        [bridgeState.dragView removeFromSuperview];
        bridgeState.dragView = nil;
    }

    if (bridgeState.dylibHandle) {
        dlclose(bridgeState.dylibHandle);
        bridgeState.dylibHandle = nullptr;
    }

    bridgeState.createFn = nullptr;
    bridgeState.endFn = nullptr;
    bridgeState.destroyFn = nullptr;
    bridgeState.resizeFn = nullptr;
    bridgeState.setEditorControlsEnabledFn = nullptr;
    bridgeState.setEditorSimulationEnabledFn = nullptr;
    bridgeState.setEditorControlModeFn = nullptr;
    bridgeState.editorPointerEventFn = nullptr;
    bridgeState.editorScrollEventFn = nullptr;
    bridgeState.editorKeyEventFn = nullptr;
    bridgeState.getSelectedObjectIdFn = nullptr;
    bridgeState.getSelectedObjectNameFn = nullptr;
    bridgeState.stepFn = nullptr;
    bridgeState.hostView = nil;
}

static void *requireSymbol(void *handle, const char *name) {
    void *sym = dlsym(handle, name);
    if (!sym) {
        throw std::runtime_error(std::string("Failed to load symbol: ") + name);
    }
    return sym;
}

Napi::Value LoadLibrary(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        throw Napi::TypeError::New(env,
                                   "loadLibrary(path) requires a string path");
    }

    unloadEditorIfNeeded();

    std::string path = info[0].As<Napi::String>().Utf8Value();

    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        throw Napi::Error::New(env, dlerror() ? dlerror() : "dlopen failed");
    }

    bridgeState.dylibHandle = handle;
    bridgeState.createFn = reinterpret_cast<RuntimeCreateFn>(
        requireSymbol(handle, "atlas_runtime_create_metal_view_context"));
    bridgeState.endFn = reinterpret_cast<RuntimeEndFn>(
        requireSymbol(handle, "atlas_runtime_end_context"));
    bridgeState.destroyFn = reinterpret_cast<RuntimeDestroyFn>(
        requireSymbol(handle, "atlas_runtime_destroy_context"));
    bridgeState.resizeFn = reinterpret_cast<RuntimeResizeFn>(
        requireSymbol(handle, "atlas_runtime_resize_context"));
    bridgeState.setEditorControlsEnabledFn =
        reinterpret_cast<RuntimeSetEditorControlsEnabledFn>(requireSymbol(
            handle, "atlas_runtime_set_editor_controls_enabled"));
    bridgeState.setEditorSimulationEnabledFn =
        reinterpret_cast<RuntimeSetEditorSimulationEnabledFn>(requireSymbol(
            handle, "atlas_runtime_set_editor_simulation_enabled"));
    bridgeState.setEditorControlModeFn =
        reinterpret_cast<RuntimeSetEditorControlModeFn>(
            requireSymbol(handle, "atlas_runtime_set_editor_control_mode"));
    bridgeState.editorPointerEventFn =
        reinterpret_cast<RuntimeEditorPointerEventFn>(
            requireSymbol(handle, "atlas_runtime_editor_pointer_event"));
    bridgeState.editorScrollEventFn =
        reinterpret_cast<RuntimeEditorScrollEventFn>(
            dlsym(handle, "atlas_runtime_editor_scroll_event"));
    bridgeState.editorKeyEventFn = reinterpret_cast<RuntimeEditorKeyEventFn>(
        requireSymbol(handle, "atlas_runtime_editor_key_event"));
    bridgeState.getSelectedObjectIdFn =
        reinterpret_cast<RuntimeGetSelectedObjectIdFn>(
            requireSymbol(handle, "atlas_runtime_get_selected_object_id"));
    bridgeState.getSelectedObjectNameFn =
        reinterpret_cast<RuntimeGetSelectedObjectNameFn>(
            requireSymbol(handle, "atlas_runtime_get_selected_object_name"));
    bridgeState.stepFn = reinterpret_cast<RuntimeStepFn>(
        requireSymbol(handle, "atlas_runtime_step_frame"));

    return env.Undefined();
}

Napi::Value AttachToNativeWindow(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.createFn) {
        throw Napi::Error::New(env, "Library not loaded");
    }

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBuffer()) {
        throw Napi::TypeError::New(
            env,
            "attachToNativeWindow(projectFile, handleBuffer[, sdlInputWindow]) "
            "requires a string project path and a Buffer");
    }

    std::string projectFile = info[0].As<Napi::String>().Utf8Value();
    if (projectFile.empty()) {
        throw Napi::Error::New(env, "Project file path cannot be empty");
    }

    auto buf = info[1].As<Napi::Buffer<uint8_t>>();
    if (buf.Length() < sizeof(void *)) {
        throw Napi::Error::New(env, "Native handle buffer too small");
    }

    void *rawPtr = *reinterpret_cast<void **>(buf.Data());
    NSView *hostView = (__bridge NSView *)rawPtr;
    if (!hostView) {
        throw Napi::Error::New(env, "Host NSView is null");
    }

    bridgeState.hostView = hostView;

    NSView *parentView = [hostView superview] ?: hostView;
    bool childIsSibling = parentView != hostView;
    NSRect frame = childIsSibling ? [hostView frame] : [hostView bounds];
    NSView *child = [[AtlasRuntimeView alloc] initWithFrame:frame];
    [child setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    if (childIsSibling) {
        [parentView addSubview:child
                    positioned:NSWindowBelow
                    relativeTo:hostView];
    } else {
        [hostView addSubview:child positioned:NSWindowBelow relativeTo:nil];
    }
    bridgeState.childView = child;
    installTitlebarDragView(hostView);
    if ([child window]) {
        [[child window] makeFirstResponder:child];
    }
    installScrollMonitor();

    NSLog(@"[runtime] create_metal_view_context called");
    NSLog(@"[runtime] parentView=%p", child);
    NSLog(@"[runtime] isMainThread=%@",
          [NSThread isMainThread] ? @"YES" : @"NO");
    NSLog(@"[runtime] parentView.window=%p", child ? [child window] : nil);
    NSLog(@"[runtime] bounds=%@",
          child ? NSStringFromRect([child bounds]) : @"<null>");

    void *sdlInputWindow = nullptr;
    if (info.Length() >= 3 && info[2].IsBuffer()) {
        auto sdlWindowBuf = info[2].As<Napi::Buffer<uint8_t>>();
        if (sdlWindowBuf.Length() < sizeof(void *)) {
            throw Napi::Error::New(env, "SDL input window buffer too small");
        }
        sdlInputWindow = *reinterpret_cast<void **>(sdlWindowBuf.Data());
    }

    bridgeState.runtimeContext =
        bridgeState.createFn(projectFile.c_str(), child, sdlInputWindow);
    if (!bridgeState.runtimeContext) {
        throw Napi::Error::New(
            env,
            "atlas_runtime_create_metal_view_context returned null");
    }

    if (bridgeState.setEditorControlsEnabledFn) {
        bridgeState.setEditorControlsEnabledFn(bridgeState.runtimeContext, true);
    }
    if (bridgeState.setEditorSimulationEnabledFn) {
        bridgeState.setEditorSimulationEnabledFn(bridgeState.runtimeContext,
                                                 false);
    }
    if (bridgeState.setEditorControlModeFn) {
        bridgeState.setEditorControlModeFn(bridgeState.runtimeContext, 1);
    }

    if (bridgeState.stepFn) {
        bridgeState.stepFn(bridgeState.runtimeContext);
    }

    return env.Undefined();
}

static void resizeChildView(NSView *childView, int width, int height) {
    if (!childView) {
        return;
    }

    auto resizeBlock = ^{
      NSRect frame = NSMakeRect(0, 0, width, height);
      if (bridgeState.hostView && [childView superview] == [bridgeState.hostView superview]) {
          frame = [bridgeState.hostView frame];
      }
      [childView setFrame:frame];
    };

    if ([NSThread isMainThread]) {
        resizeBlock();
        return;
    }

    dispatch_sync(dispatch_get_main_queue(), ^{
      resizeBlock();
    });
}

Napi::Value Resize(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext || !bridgeState.resizeFn) {
        return env.Undefined();
    }

    if (info.Length() < 3) {
        throw Napi::TypeError::New(env, "resize(width, height, scale)");
    }

    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    float scale = info[2].As<Napi::Number>().FloatValue();
    float effectiveScale = scale > 0.0f ? scale : 1.0f;

    if (bridgeState.hostView && [bridgeState.hostView window]) {
        CGFloat backingScale = [[bridgeState.hostView window] backingScaleFactor];
        if (backingScale > 0.0) {
            effectiveScale = static_cast<float>(backingScale);
        }
    }

    resizeChildView(bridgeState.childView, width, height);
    resizeTitlebarDragView();

    if (!bridgeState.resizeFn(bridgeState.runtimeContext, width, height,
                              effectiveScale)) {
        throw Napi::Error::New(env, "atlas_runtime_resize_context failed");
    }

    return env.Undefined();
}

Napi::Value SetEditorControlMode(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext || !bridgeState.setEditorControlModeFn) {
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsNumber()) {
        throw Napi::TypeError::New(env, "setEditorControlMode(mode)");
    }

    int mode = info[0].As<Napi::Number>().Int32Value();
    if (!bridgeState.setEditorControlModeFn(bridgeState.runtimeContext, mode)) {
        throw Napi::Error::New(env, "atlas_runtime_set_editor_control_mode failed");
    }

    return env.Undefined();
}

Napi::Value SetEditorControlsEnabled(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext ||
        !bridgeState.setEditorControlsEnabledFn) {
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        throw Napi::TypeError::New(env, "setEditorControlsEnabled(enabled)");
    }

    bool enabled = info[0].As<Napi::Boolean>().Value();
    if (!bridgeState.setEditorControlsEnabledFn(bridgeState.runtimeContext,
                                                enabled)) {
        throw Napi::Error::New(
            env, "atlas_runtime_set_editor_controls_enabled failed");
    }

    return env.Undefined();
}

Napi::Value SetEditorSimulationEnabled(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext ||
        !bridgeState.setEditorSimulationEnabledFn) {
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsBoolean()) {
        throw Napi::TypeError::New(env,
                                   "setEditorSimulationEnabled(enabled)");
    }

    bool enabled = info[0].As<Napi::Boolean>().Value();
    if (!bridgeState.setEditorSimulationEnabledFn(bridgeState.runtimeContext,
                                                  enabled)) {
        throw Napi::Error::New(
            env, "atlas_runtime_set_editor_simulation_enabled failed");
    }

    return env.Undefined();
}

Napi::Value EditorScroll(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext) {
        return env.Undefined();
    }

    if (info.Length() < 1 || !info[0].IsNumber()) {
        throw Napi::TypeError::New(env, "editorScroll(delta[, scale])");
    }

    float delta = info[0].As<Napi::Number>().FloatValue();
    float scale = 1.0f;
    if (info.Length() >= 2 && info[1].IsNumber()) {
        scale = info[1].As<Napi::Number>().FloatValue();
    }

    if (std::abs(delta) < 0.0001f) {
        return env.Undefined();
    }

    if (bridgeState.editorScrollEventFn) {
        if (!bridgeState.editorScrollEventFn(bridgeState.runtimeContext, delta,
                                             scale)) {
            throw Napi::Error::New(
                env, "atlas_runtime_editor_scroll_event failed");
        }
        return env.Undefined();
    }

    if (bridgeState.editorPointerEventFn) {
        if (!bridgeState.editorPointerEventFn(bridgeState.runtimeContext, 3,
                                              0.0f, delta, 0, scale)) {
            throw Napi::Error::New(env,
                                   "atlas_runtime_editor_pointer_event failed");
        }
    }

    return env.Undefined();
}

Napi::Value EditorPointer(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext || !bridgeState.editorPointerEventFn) {
        return env.Undefined();
    }

    if (info.Length() < 5 || !info[0].IsNumber() || !info[1].IsNumber() ||
        !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber()) {
        throw Napi::TypeError::New(
            env, "editorPointer(action, x, y, button, scale)");
    }

    int action = info[0].As<Napi::Number>().Int32Value();
    float x = info[1].As<Napi::Number>().FloatValue();
    float y = info[2].As<Napi::Number>().FloatValue();
    int button = info[3].As<Napi::Number>().Int32Value();
    float scale = info[4].As<Napi::Number>().FloatValue();

    if (!bridgeState.editorPointerEventFn(bridgeState.runtimeContext, action, x,
                                          y, button, scale)) {
        throw Napi::Error::New(env,
                               "atlas_runtime_editor_pointer_event failed");
    }

    return env.Undefined();
}

Napi::Value GetSelectedObjectId(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext || !bridgeState.getSelectedObjectIdFn) {
        return Napi::Number::New(env, -1);
    }

    return Napi::Number::New(
        env, bridgeState.getSelectedObjectIdFn(bridgeState.runtimeContext));
}

Napi::Value GetSelectedObjectName(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.runtimeContext || !bridgeState.getSelectedObjectNameFn) {
        return Napi::String::New(env, "");
    }

    const char *name =
        bridgeState.getSelectedObjectNameFn(bridgeState.runtimeContext);
    return Napi::String::New(env, name ? name : "");
}

Napi::Value Step(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (bridgeState.runtimeContext && bridgeState.stepFn) {
        bridgeState.stepFn(bridgeState.runtimeContext);
    }

    return env.Undefined();
}

Napi::Value Shutdown(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    unloadEditorIfNeeded();
    return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("loadLibrary", Napi::Function::New(env, LoadLibrary));
    exports.Set("attachToNativeWindow",
                Napi::Function::New(env, AttachToNativeWindow));
    exports.Set("resize", Napi::Function::New(env, Resize));
    exports.Set("resizeEditor", Napi::Function::New(env, Resize));
    exports.Set("setEditorControlsEnabled",
                Napi::Function::New(env, SetEditorControlsEnabled));
    exports.Set("setEditorSimulationEnabled",
                Napi::Function::New(env, SetEditorSimulationEnabled));
    exports.Set("setEditorControlMode",
                Napi::Function::New(env, SetEditorControlMode));
    exports.Set("editorScroll", Napi::Function::New(env, EditorScroll));
    exports.Set("editorPointer", Napi::Function::New(env, EditorPointer));
    exports.Set("getSelectedObjectId",
                Napi::Function::New(env, GetSelectedObjectId));
    exports.Set("getSelectedObjectName",
                Napi::Function::New(env, GetSelectedObjectName));
    exports.Set("step", Napi::Function::New(env, Step));
    exports.Set("shutdown", Napi::Function::New(env, Shutdown));
    return exports;
}

NODE_API_MODULE(engine_bridge, Init)
