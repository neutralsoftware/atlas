#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <memory>
#include <string>
#include <napi.h>

struct EditorHandle;

using EditorCreateFn = EditorHandle *(*)(NSView * parentView,
                                         void *sdlInputWindow);
using EditorDestroyFn = void (*)(EditorHandle *handle);
using EditorStepFn = void (*)(EditorHandle *handle);

struct BridgeState {
    void *dylibHandle = nullptr;

    EditorCreateFn createFn = nullptr;
    EditorDestroyFn destroyFn = nullptr;
    EditorStepFn stepFn = nullptr;

    EditorHandle *editor = nullptr;

    NSView *hostView = nil;
    NSView *childView = nil;
};

struct BridgeState bridgeState;

static void unloadEditorIfNeeded() {
    if (bridgeState.editor && bridgeState.destroyFn) {
        bridgeState.destroyFn(bridgeState.editor);
        bridgeState.editor = nullptr;
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
    bridgeState.destroyFn = nullptr;
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
    Napi::Env env = info.env();

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
    bridgeState.createFn = reinterpret_cast<EditorCreateFn>(
        requireSymbol(handle, "atlas_runtime_create_metal_view_context"));
    bridgeState.destroyFn = reinterpret_cast<EditorDestroyFn>(
        requireSymbol(handle, "atlas_runtime_destroy_context"));
    bridgeState.stepFn = reinterpret_cast<EditorStepFn>(
        requireSymbol(handle, "atlas_runtime_step_frame"));

    return env.Undefined();
}

Napi::Value AttachToNativeWindow(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (!bridgeState.createFn) {
        throw Napi::Error::New(env, "Library not loaded");
    }

    if (info.Length() < 1 || !info[0].IsBuffer()) {
        throw Napi::TypeError::New(
            env, "attachToNativeWindow(handleBuffer) requires a Buffer");
    }

    auto buf = info[0].As<Napi::Buffer<uint8_t>>();
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

    bridgeState.editor = bridgeState.createFn(child);
    if (!bridgeState.editor) {
        throw Napi::Error::New(env, "editor_create returned null");
    }

    CGFloat scale = 1.0;
    if ([hostView window]) {
        scale = [[hostView window] backingScaleFactor];
    }

    if (bridgeState.stepFn) {
        NSRect childBounds = [child bounds];
        bridgeState.stepFn(bridgeState.editor);
    }

    return env.Undefined();
}

Napi::Value Resize(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    // if (!bridgeState.editor || !bridgeState.resize) {
    //     return env.Undefined();
    // }

    // if (info.Length() < 3) {
    //     throw Napi::TypeError::New(env, "resize(width, height, scale)");
    // }

    // int width = info[0].As<Napi::Number>().Int32Value();
    // int height = info[1].As<Napi::Number>().Int32Value();
    // float scale = info[2].As<Napi::Number>().FloatValue();

    // if (bridgeState.childView) {
    //     dispatch_async(dispatch_get_main_queue(), ^{
    //       [bridgeState.childView setFrame:NSMakeRect(0, 0, width, height)];
    //     });
    // }

    // bridgeState.editor_resize(bridgeState.editor, width, height, scale);
    // return env.Undefined();
}

Napi::Value Step(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();

    if (bridgeState.editor && bridgeState.stepFn) {
        bridgeState.stepFn(bridgeState.editor);
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
    exports.Set("step", Napi::Function::New(env, Step));
    exports.Set("shutdown", Napi::Function::New(env, Shutdown));
    return exports;
}

NODE_API_MODULE(engine_bridge, Init)