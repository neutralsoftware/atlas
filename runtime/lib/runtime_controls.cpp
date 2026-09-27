#include "atlas/runtime/context.h"
#include "window_activation_scope.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <stdexcept>

bool Context::resize(int width, int height, float scale) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    WindowActivationScope activeWindow(*window);
    window->resize(width, height, scale);
    if (materialPreviewRuntime && camera != nullptr) {
        const float aspect = static_cast<float>(std::max(1, width)) /
                             static_cast<float>(std::max(1, height));
        const float verticalHalfFov = glm::radians(camera->fov * 0.5f);
        const float horizontalHalfFov =
            std::atan(std::tan(verticalHalfFov) * aspect);
        const float limitingHalfFov = std::max(
            glm::radians(5.0f), std::min(verticalHalfFov, horizontalHalfFov));
        const float distance = 0.82f / std::sin(limitingHalfFov);
        camera->setPosition({0.0f, 0.0f, distance});
        camera->lookAt(Position3d::zero());
    }
    return true;
}

bool Context::setEditorControlsEnabled(bool enabled) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->setEditorControlsEnabled(enabled);
    return true;
}

bool Context::setEditorSimulationEnabled(bool enabled) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->setEditorSimulationEnabled(enabled);
    if (editorRuntime) {
        window->setCamera(enabled || editorCameraFocused ||
                                  editorViewCamera == nullptr
                              ? camera.get()
                              : editorViewCamera.get());
        window->setEditorCameraFocused(enabled || editorCameraFocused);
    }
    return true;
}

bool Context::setEditorCameraFocused(bool focused) {
    if (window == nullptr || !editorRuntime || camera == nullptr ||
        editorViewCamera == nullptr) {
        return false;
    }
    editorCameraFocused = focused;
    window->setCamera(focused ? camera.get() : editorViewCamera.get());
    window->setEditorCameraFocused(focused);
    return true;
}

bool Context::setEditorControlMode(int mode) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    if (mode < 0 || mode > 3) {
        return false;
    }
    window->setEditorControlMode(static_cast<EditorControlMode>(mode));
    return true;
}

bool Context::setEditorShadingMode(int mode) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    if (mode < 0 || mode > 2) {
        return false;
    }
    window->setEditorShadingMode(static_cast<EditorShadingMode>(mode));
    return true;
}

bool Context::setEditorPathTracingPreview(bool enabled) {
    editorPathTracingPreview = enabled;
    if (window == nullptr || !editorRuntime) {
        return false;
    }
#ifdef METAL
    return window->setEditorPathTracingPreview(enabled);
#else
    (void)enabled;
    return false;
#endif
}

bool Context::configurePathTracing(int samplesPerPixel, int bounceLimit,
                                   bool denoising, int accumulationFrames,
                                   bool useUpscaling, float upscalingRatio,
                                   uint32_t featureFlags) {
    if (window == nullptr) {
        return false;
    }
    config.pathTracingSamples = std::clamp(samplesPerPixel, 1, 256);
    config.pathTracingBounces = std::clamp(bounceLimit, 1, 16);
    config.pathTracingDenoising = denoising;
    config.pathTracingAccumulationFrames =
        std::clamp(accumulationFrames, 1, 2048);
    config.pathTracingFeatureFlags = featureFlags;
    config.realtimePBRFeatureFlags =
        config.globalIllumination ? featureFlags
                                 : photon::NormalRealtimePBRFeatures;
    config.useUpscaling = useUpscaling;
    config.upscalingRatio = std::clamp(upscalingRatio, 0.25f, 1.0f);
    window->setRealtimePBRFeatures(config.realtimePBRFeatureFlags);
#ifdef METAL
    window->useMetalUpscaling(useUpscaling ? config.upscalingRatio : 1.0f);
    window->configurePathTracing(
        config.pathTracingSamples, config.pathTracingBounces,
        config.pathTracingDenoising, config.pathTracingAccumulationFrames,
        config.pathTracingFeatureFlags);
    return true;
#else
    return false;
#endif
}

std::string Context::getPathTracingError() const {
#ifdef METAL
    return window != nullptr ? window->getPathTracingError() : std::string();
#else
    return {};
#endif
}

float Context::frameRate() const {
    return window != nullptr ? window->getFramesPerSecond() : 0.0f;
}

bool Context::editorPointerEvent(int action, float x, float y, int button,
                                 float scale) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->editorPointerEvent(action, x, y, button, scale);
    return true;
}

bool Context::editorScrollEvent(float delta, float scale) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->editorScrollEvent(delta, scale);
    return true;
}

bool Context::editorKeyEvent(int key, bool pressed) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->editorKeyEvent(key, pressed);
    return true;
}

bool Context::editorRuntimeKeyEvent(int key, bool pressed) {
    if (window == nullptr)
        return false;
    window->editorRuntimeKeyEvent(key, pressed);
    return true;
}

bool Context::editorRuntimeMouseMove(float x, float y, float deltaX,
                                     float deltaY) {
    if (window == nullptr)
        return false;
    window->editorRuntimeMouseMove(x, y, deltaX, deltaY);
    return true;
}

bool Context::editorRuntimeMouseButtonEvent(int action, int button) {
    if (window == nullptr)
        return false;
    window->editorRuntimeMouseButtonEvent(action, button);
    return true;
}

bool Context::editorRuntimeScrollEvent(float x, float y) {
    if (window == nullptr)
        return false;
    window->editorRuntimeScrollEvent(x, y);
    return true;
}

bool Context::clearEditorRuntimeInput() {
    if (window == nullptr)
        return false;
    window->clearEditorRuntimeInput();
    return true;
}

bool Context::beginEditorKeyboardTransform(int mode, float x, float y,
                                           float scale) {
    if (window == nullptr || mode < 1 || mode > 3)
        return false;
    return window->beginEditorKeyboardTransform(
        static_cast<EditorControlMode>(mode), x, y, scale);
}

bool Context::setEditorKeyboardTransformAxes(int axes) {
    if (window == nullptr || axes < 1 || axes > 7)
        return false;
    window->setEditorKeyboardTransformAxes(axes);
    return true;
}

bool Context::finishEditorKeyboardTransform(bool commit) {
    if (window == nullptr)
        return false;
    window->finishEditorKeyboardTransform(commit);
    return true;
}

bool Context::toggleEditorTransformSpace() {
    return window != nullptr && window->toggleEditorTransformSpace();
}

bool Context::toggleEditorTransformSnapping() {
    return window != nullptr && window->toggleEditorTransformSnapping();
}

float Context::changeEditorTransformSnapIncrement(float factor) {
    return window != nullptr
               ? window->changeEditorTransformSnapIncrement(factor)
               : 0.0f;
}
