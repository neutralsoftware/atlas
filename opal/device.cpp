/*
 device.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Device and context functions
 Copyright (c) 2025 maxvdec
*/

#include "opal/opal.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <stdexcept>
#ifdef METAL
#include "metal_state.h"
#include <objc/message.h>
#include <objc/runtime.h>
#endif
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

Context::~Context() {
#ifdef METAL
    metal::releaseContextState(this);
#endif
    if (glContext != nullptr) {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }
    if (window != nullptr && ownsWindow) {
        SDL_DestroyWindow(window);
    }
    window = nullptr;
}

Device::~Device() {
#ifdef METAL
    metal::releaseDeviceState(this);
#endif
}

#ifdef METAL
namespace {

using CocoaObj = void *;

inline CocoaObj sendObjCId(CocoaObj object, const char *selector) {
    return reinterpret_cast<CocoaObj (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

inline double sendObjCDouble(CocoaObj object, const char *selector) {
    return reinterpret_cast<double (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

struct CocoaPoint {
    double x;
    double y;
};

struct CocoaSize {
    double width;
    double height;
};

struct CocoaRect {
    CocoaPoint origin;
    CocoaSize size;
};

inline CocoaRect sendObjCRect(CocoaObj object, const char *selector) {
    return reinterpret_cast<CocoaRect (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

CocoaObj getWindowContentView(SDL_Window *window) {
    if (window == nullptr) {
        return nullptr;
    }

    const SDL_PropertiesID properties = SDL_GetWindowProperties(window);
    CocoaObj nsWindow = SDL_GetPointerProperty(
        properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (nsWindow == nullptr) {
        return nullptr;
    }

    return sendObjCId(nsWindow, "contentView");
}

void queryTargetPixelSize(CocoaObj targetView, int fallbackWidth,
                          int fallbackHeight, int &width, int &height) {
    width = std::max(1, fallbackWidth);
    height = std::max(1, fallbackHeight);
    if (targetView == nullptr) {
        return;
    }

    CocoaRect bounds = sendObjCRect(targetView, "bounds");
    double scale = 1.0;
    CocoaObj window = sendObjCId(targetView, "window");
    if (window != nullptr) {
        const double backingScale =
            sendObjCDouble(window, "backingScaleFactor");
        if (backingScale > 0.0) {
            scale = backingScale;
        }
    }

    width = std::max(1, static_cast<int>(std::lround(
                            std::max(1.0, bounds.size.width) * scale)));
    height = std::max(1, static_cast<int>(std::lround(
                             std::max(1.0, bounds.size.height) * scale)));
}

void attachMetalLayerToView(CocoaObj targetView, CA::MetalLayer *layer) {
    if (targetView == nullptr || layer == nullptr) {
        throw std::runtime_error("Cannot attach CAMetalLayer to null view");
    }

    reinterpret_cast<void (*)(CocoaObj, SEL, bool)>(objc_msgSend)(
        targetView, sel_registerName("setWantsLayer:"), true);

    CocoaRect bounds = sendObjCRect(targetView, "bounds");
    reinterpret_cast<void (*)(CocoaObj, SEL, CocoaRect)>(objc_msgSend)(
        reinterpret_cast<CocoaObj>(layer), sel_registerName("setFrame:"),
        bounds);

    double scale = 1.0;
    CocoaObj window = sendObjCId(targetView, "window");
    if (window != nullptr) {
        const double backingScale =
            sendObjCDouble(window, "backingScaleFactor");
        if (backingScale > 0.0) {
            scale = backingScale;
        }
    }
    reinterpret_cast<void (*)(CocoaObj, SEL, double)>(objc_msgSend)(
        reinterpret_cast<CocoaObj>(layer),
        sel_registerName("setContentsScale:"), scale);

    reinterpret_cast<void (*)(CocoaObj, SEL, CocoaObj)>(objc_msgSend)(
        targetView, sel_registerName("setLayer:"),
        reinterpret_cast<CocoaObj>(layer));
}

} // namespace
#endif

void Context::adoptWindow(SDL_Window *existingWindow, bool takeOwnership) {
    if (existingWindow == nullptr) {
        throw std::runtime_error("Cannot adopt a null SDL window");
    }
    if (window != nullptr && ownsWindow) {
        SDL_DestroyWindow(window);
    }
    window = existingWindow;
    ownsWindow = takeOwnership;
}

void Context::setMetalTargetView(void *view) { metalTargetView = view; }

void *Context::getMetalTargetView() const { return metalTargetView; }

std::shared_ptr<Context> Context::create(ContextConfiguration config) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        atlas_error("Failed to initialize SDL");
        throw std::runtime_error("Failed to initialize SDL");
    }

    atlas_log("Initializing graphics context");

    auto context = std::make_shared<Context>();
    context->config = config;

#ifdef METAL
    config.useOpenGL = false;
    context->config.useOpenGL = false;
#endif

#ifdef VULKAN
    context->createInstance();
    if (config.createValidationLayers) {
        context->setupMessenger();
    }
#endif

    return context;
}

void Context::setDecorated(bool enabled) { decorated = enabled; }

void Context::setResizable(bool enabled) { resizable = enabled; }

void Context::setTransparent(bool enabled) { transparent = enabled; }

void Context::setAlwaysOnTop(bool enabled) { alwaysOnTop = enabled; }

void Context::setSamples(int value) { samples = value; }

void Context::setHighPixelDensity(bool enabled) { highPixelDensity = enabled; }

void Context::setHidden(bool enabled) { hidden = enabled; }

void Context::makeCurrent() {
    if (this->window != nullptr && this->glContext != nullptr) {
        SDL_GL_MakeCurrent(this->window, this->glContext);
    }
}

SDL_Window *Context::makeWindow(int width, int height, const char *title,
                                SDL_DisplayID displayID) {
    Uint64 windowFlags = 0;

    if (config.useOpenGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.majorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.minorVersion);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples > 0 ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            config.profile == OpenGLProfile::Core
                                ? SDL_GL_CONTEXT_PROFILE_CORE
                                : SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        windowFlags |= SDL_WINDOW_OPENGL;
    }
#ifdef VULKAN
    if (!config.useOpenGL) {
        windowFlags |= SDL_WINDOW_VULKAN;
    }
#endif
#ifdef METAL
    if (!config.useOpenGL) {
        windowFlags |= SDL_WINDOW_METAL;
    }
#endif
    if (resizable) {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }
    if (transparent) {
        windowFlags |= SDL_WINDOW_TRANSPARENT;
    }
    if (!decorated) {
        windowFlags |= SDL_WINDOW_BORDERLESS;
    }
    if (highPixelDensity) {
        windowFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    }
    if (hidden) {
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    this->window = SDL_CreateWindow(title, width, height, windowFlags);
    if (this->window == nullptr) {
        atlas_error("Failed to create SDL window");
        throw std::runtime_error("Failed to create SDL window");
    }
    this->ownsWindow = true;

    if (config.useOpenGL) {
        glContext = SDL_GL_CreateContext(this->window);
        if (glContext == nullptr) {
            atlas_error("Failed to create SDL OpenGL context");
            throw std::runtime_error("Failed to create SDL OpenGL context");
        }
    }

    if (displayID != 0) {
        SDL_Rect displayBounds{};
        if (SDL_GetDisplayBounds(displayID, &displayBounds)) {
            SDL_SetWindowPosition(this->window, displayBounds.x,
                                  displayBounds.y);
        }
    }

    SDL_SetWindowAlwaysOnTop(this->window, alwaysOnTop);
#ifdef VULKAN
    this->setupSurface();
#endif
    return this->window;
}

SDL_Window *Context::getWindow() const {
    if (this->window == nullptr)
        throw std::runtime_error("Cannot obtain a window before created");
    return this->window;
}

DeviceInfo Device::getDeviceInfo() {
    DeviceInfo info;
#ifdef OPENGL
    info.deviceName = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    info.vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    info.driverVersion = "N/A";
    info.renderingVersion = reinterpret_cast<const char *>(
        glGetString(GL_SHADING_LANGUAGE_VERSION));
    info.opalVersion = OPAL_VERSION;
    return info;
#elif defined(VULKAN)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(this->physicalDevice, &deviceProperties);
    info.driverVersion = std::to_string(deviceProperties.driverVersion);
    info.deviceName = deviceProperties.deviceName;
    info.vendorName = std::to_string(deviceProperties.vendorID);
    info.renderingVersion = std::to_string(deviceProperties.apiVersion);
    info.opalVersion = OPAL_VERSION;
    return info;
#elif defined(METAL)
    auto &state = metal::deviceState(this);
    if (state.device != nullptr && state.device->name() != nullptr) {
        info.deviceName = state.device->name()->utf8String();
    } else {
        info.deviceName = "Unknown Metal Device";
    }
    info.vendorName = "Apple";
    info.driverVersion = "N/A";
    info.renderingVersion = "Metal 4.0";
    info.opalVersion = OPAL_VERSION;
    return info;
#else
    info.deviceName = "Unknown";
    info.vendorName = "Unknown";
    info.driverVersion = "N/A";
    info.renderingVersion = "Unknown";
    info.opalVersion = OPAL_VERSION;
    return info;
#endif
}

#ifdef METAL
MTL::Device *Device::getMetalDevice() const {
    return metal::deviceState(const_cast<Device *>(this)).device;
}
#endif

std::shared_ptr<Device>
Device::acquire([[maybe_unused]] const std::shared_ptr<Context> &context) {
#ifdef OPENGL
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        atlas_error("Failed to initialize GLAD");
        throw std::runtime_error("Failed to initialize GLAD");
    }

    atlas_log("Graphics device acquired (OpenGL)");

    auto device = std::make_shared<Device>();
    device->context = context;

    Device::globalInstance = device.get();
    return device;
#elif defined(VULKAN)
    auto device = std::make_shared<Device>();
    Device::globalInstance = device.get();
    device->context = context;
    device->pickPhysicalDevice(context);
    device->createLogicalDevice(context);
    device->createSwapChain(context);
    device->createImageViews();
    return device;
#elif defined(METAL)
    auto device = std::make_shared<Device>();
    Device::globalInstance = device.get();
    device->context = context;

    auto &deviceState = metal::deviceState(device.get());
    deviceState.context = device->context.get();
    deviceState.device = MTL::CreateSystemDefaultDevice();
    if (deviceState.device == nullptr) {
        throw std::runtime_error("Failed to create default Metal device");
    }

    deviceState.queue = deviceState.device->newCommandQueue();
    if (deviceState.queue == nullptr) {
        throw std::runtime_error("Failed to create Metal command queue");
    }

    auto &contextState = metal::contextState(context.get());
    contextState.layer = CA::MetalLayer::layer();
    if (contextState.layer == nullptr) {
        throw std::runtime_error("Failed to create CAMetalLayer");
    }

    contextState.layer->setDevice(deviceState.device);
    contextState.layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    contextState.layer->setFramebufferOnly(false);
    contextState.layer->setDisplaySyncEnabled(true);
    contextState.layer->setMaximumDrawableCount(3);

    int fallbackWidth = 1;
    int fallbackHeight = 1;
    if (context->window != nullptr) {
        atlasGetWindowSizeInPixels(context->window, &fallbackWidth,
                                   &fallbackHeight);
    }
    int fbWidth = fallbackWidth;
    int fbHeight = fallbackHeight;
    CocoaObj targetView =
        reinterpret_cast<CocoaObj>(context->getMetalTargetView());
    if (targetView == nullptr) {
        targetView = getWindowContentView(context->window);
    }
    queryTargetPixelSize(targetView, fallbackWidth, fallbackHeight, fbWidth,
                         fbHeight);
    contextState.layer->setDrawableSize(CGSizeMake(
        static_cast<double>(fbWidth), static_cast<double>(fbHeight)));

    attachMetalLayerToView(targetView, contextState.layer);

    atlas_log("Graphics device acquired (Metal)");
    return device;
#else
    throw std::runtime_error("No rendering backend selected");
#endif
}

std::shared_ptr<Framebuffer> Device::getDefaultFramebuffer() {
    if (defaultFramebuffer == nullptr) {
        defaultFramebuffer = std::make_shared<Framebuffer>();
        defaultFramebuffer->framebufferID = 0;
        defaultFramebuffer->width = 0;
        defaultFramebuffer->height = 0;
        defaultFramebuffer->isDefaultFramebuffer = true;
    }
    return defaultFramebuffer;
}

Device *Device::globalInstance = nullptr;

#ifdef VULKAN
VkDevice Device::globalDevice = VK_NULL_HANDLE;
#endif

#ifdef VULKAN
std::shared_ptr<Buffer> Device::getDefaultInstanceBuffer() {
    if (defaultInstanceBuffer != nullptr) {
        return defaultInstanceBuffer;
    }

    static const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                       0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f};

    defaultInstanceBuffer =
        Buffer::create(BufferUsage::VertexBuffer, sizeof(identity), identity,
                       MemoryUsageType::GPUOnly);
    return defaultInstanceBuffer;
}
#endif

} // namespace opal
