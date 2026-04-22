#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <memory>
#include <string>
#include <napi.h>

using RuntimeCreateFn = void *(*)(const char *projectFile, void *metalView,
                                  void *sdlInputWindow);
using RuntimeEndFn = void (*)(void *runtimeContext);
using RuntimeDestroyFn = void (*)(void *runtimeContext);
using RuntimeResizeFn = bool (*)(void *runtimeContext, int width, int height,
                                 float scale);
using RuntimeStepFn = bool (*)(void *runtimeContext);

struct BridgeState {
    void *dylibHandle = nullptr;

    RuntimeCreateFn createFn = nullptr;
    RuntimeEndFn endFn = nullptr;
    RuntimeDestroyFn destroyFn = nullptr;
    RuntimeResizeFn resizeFn = nullptr;
    RuntimeStepFn stepFn = nullptr;

    void *runtimeContext = nullptr;

    NSView *hostView = nil;
    NSView *childView = nil;
};

struct BridgeState bridgeState;

static void unloadEditorIfNeeded() {
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

    if (bridgeState.dylibHandle) {
        dlclose(bridgeState.dylibHandle);
        bridgeState.dylibHandle = nullptr;
    }

    bridgeState.createFn = nullptr;
    bridgeState.endFn = nullptr;
    bridgeState.destroyFn = nullptr;
    bridgeState.resizeFn = nullptr;
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

    NSRect bounds = [hostView bounds];
    NSView *child = [[NSView alloc] initWithFrame:bounds];
    [child setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    [hostView addSubview:child positioned:NSWindowAbove relativeTo:nil];
    bridgeState.childView = child;

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

    if (bridgeState.stepFn) {
        bridgeState.stepFn(bridgeState.runtimeContext);
    }

    return env.Undefined();
}

static void resizeChildView(NSView *childView, int width, int height) {
    if (!childView) {
        return;
    }

    NSRect frame = NSMakeRect(0, 0, width, height);
    if ([NSThread isMainThread]) {
        [childView setFrame:frame];
        return;
    }

    dispatch_sync(dispatch_get_main_queue(), ^{
        [childView setFrame:frame];
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

    if (!bridgeState.resizeFn(bridgeState.runtimeContext, width, height,
                              effectiveScale)) {
        throw Napi::Error::New(env, "atlas_runtime_resize_context failed");
    }

    return env.Undefined();
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
    exports.Set("step", Napi::Function::New(env, Step));
    exports.Set("shutdown", Napi::Function::New(env, Shutdown));
    return exports;
}

NODE_API_MODULE(engine_bridge, Init)
