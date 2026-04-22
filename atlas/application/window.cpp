/*
 window.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window implementation
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/input.h"
#include "atlas/light.h"
#include "atlas/network/pipe.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "hydra/fluid.h"
#include "bezel/bezel.h"
#include "photon/illuminate.h"
#ifndef BEZEL_NATIVE
#include "bezel/jolt/world.h"
#endif
#include "finewave/audio.h"
#include <atlas/window.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sys/resource.h>
#include <utility>
#include <vector>
#if defined(METAL) && defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <tuple>
#include <opal/opal.h>

Window *Window::mainWindow = nullptr;

namespace {
template <typename T> void hashCombine(std::size_t &seed, const T &value) {
    seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (seed << 6) +
            (seed >> 2);
}

#if defined(METAL) && defined(__APPLE__)
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
#endif

std::unordered_map<SDL_JoystickID, SDL_Gamepad *> &openGamepads() {
    static std::unordered_map<SDL_JoystickID, SDL_Gamepad *> handles;
    return handles;
}

std::unordered_map<SDL_JoystickID, SDL_Joystick *> &openJoysticks() {
    static std::unordered_map<SDL_JoystickID, SDL_Joystick *> handles;
    return handles;
}

std::vector<SDL_JoystickID> getConnectedJoystickIds() {
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    std::vector<SDL_JoystickID> result;
    result.reserve(std::max(count, 0));
    if (ids != nullptr) {
        for (int i = 0; i < count; ++i) {
            result.push_back(ids[i]);
        }
        SDL_free(ids);
    }
    return result;
}

void closeGamepadHandle(SDL_JoystickID id) {
    auto &handles = openGamepads();
    auto it = handles.find(id);
    if (it == handles.end()) {
        return;
    }
    SDL_CloseGamepad(it->second);
    handles.erase(it);
}

void closeJoystickHandle(SDL_JoystickID id) {
    auto &handles = openJoysticks();
    auto it = handles.find(id);
    if (it == handles.end()) {
        return;
    }
    SDL_CloseJoystick(it->second);
    handles.erase(it);
}

void closeAllInputDeviceHandles() {
    auto gamepadIds = std::vector<SDL_JoystickID>{};
    for (const auto &[id, _] : openGamepads()) {
        gamepadIds.push_back(id);
    }
    for (SDL_JoystickID id : gamepadIds) {
        closeGamepadHandle(id);
    }

    auto joystickIds = std::vector<SDL_JoystickID>{};
    for (const auto &[id, _] : openJoysticks()) {
        joystickIds.push_back(id);
    }
    for (SDL_JoystickID id : joystickIds) {
        closeJoystickHandle(id);
    }
}

SDL_Gamepad *getOpenGamepad(SDL_JoystickID id) {
    auto &handles = openGamepads();
    if (auto it = handles.find(id); it != handles.end()) {
        return it->second;
    }
    if (!SDL_IsGamepad(id)) {
        return nullptr;
    }
    SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
    if (gamepad != nullptr) {
        handles[id] = gamepad;
    }
    return gamepad;
}

SDL_Joystick *getOpenJoystick(SDL_JoystickID id) {
    auto &handles = openJoysticks();
    if (auto it = handles.find(id); it != handles.end()) {
        return it->second;
    }
    if (SDL_IsGamepad(id)) {
        SDL_Gamepad *gamepad = getOpenGamepad(id);
        if (gamepad != nullptr) {
            return SDL_GetGamepadJoystick(gamepad);
        }
    }
    SDL_Joystick *joystick = SDL_OpenJoystick(id);
    if (joystick != nullptr) {
        handles[id] = joystick;
    }
    return joystick;
}

SDL_GamepadButton toSDLGamepadButton(int buttonIndex) {
    switch (buttonIndex) {
    case static_cast<int>(ControllerButton::A):
        return SDL_GAMEPAD_BUTTON_SOUTH;
    case static_cast<int>(ControllerButton::B):
        return SDL_GAMEPAD_BUTTON_EAST;
    case static_cast<int>(ControllerButton::X):
        return SDL_GAMEPAD_BUTTON_WEST;
    case static_cast<int>(ControllerButton::Y):
        return SDL_GAMEPAD_BUTTON_NORTH;
    case static_cast<int>(ControllerButton::LeftBumper):
        return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    case static_cast<int>(ControllerButton::RightBumper):
        return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    case static_cast<int>(ControllerButton::Back):
        return SDL_GAMEPAD_BUTTON_BACK;
    case static_cast<int>(ControllerButton::Start):
        return SDL_GAMEPAD_BUTTON_START;
    case static_cast<int>(ControllerButton::Guide):
        return SDL_GAMEPAD_BUTTON_GUIDE;
    case static_cast<int>(ControllerButton::LeftThumb):
        return SDL_GAMEPAD_BUTTON_LEFT_STICK;
    case static_cast<int>(ControllerButton::RightThumb):
        return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    case static_cast<int>(ControllerButton::DPadUp):
        return SDL_GAMEPAD_BUTTON_DPAD_UP;
    case static_cast<int>(ControllerButton::DPadRight):
        return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    case static_cast<int>(ControllerButton::DPadDown):
        return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    case static_cast<int>(ControllerButton::DPadLeft):
        return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    default:
        return SDL_GAMEPAD_BUTTON_INVALID;
    }
}

float normalizeGamepadAxisValue(int axisIndex, Sint16 value) {
    float normalized = atlasNormalizeAxisValue(value);
    if (axisIndex == CONTROLLER_AXIS_LEFT_TRIGGER ||
        axisIndex == CONTROLLER_AXIS_RIGHT_TRIGGER) {
        return std::max(0.0f, normalized);
    }
    return normalized;
}

std::vector<float> getJoystickAxes(SDL_JoystickID joystickID) {
    std::vector<float> axes;
    SDL_Joystick *joystick = getOpenJoystick(joystickID);
    if (joystick == nullptr) {
        return axes;
    }

    const int axisCount = SDL_GetNumJoystickAxes(joystick);
    if (axisCount <= 0) {
        return axes;
    }

    axes.reserve(axisCount);
    for (int i = 0; i < axisCount; ++i) {
        axes.push_back(
            atlasNormalizeAxisValue(SDL_GetJoystickAxis(joystick, i)));
    }
    return axes;
}

float getControllerAxisValueForDevice(int controllerID, int axisIndex) {
    if (SDL_IsGamepad(controllerID)) {
        SDL_Gamepad *gamepad = getOpenGamepad(controllerID);
        const SDL_GamepadAxis axis = atlasToSDLGamepadAxis(axisIndex);
        if (gamepad != nullptr && axis != SDL_GAMEPAD_AXIS_INVALID) {
            return normalizeGamepadAxisValue(axisIndex,
                                             SDL_GetGamepadAxis(gamepad, axis));
        }
    }

    SDL_Joystick *joystick = getOpenJoystick(controllerID);
    if (joystick != nullptr) {
        const int axisCount = SDL_GetNumJoystickAxes(joystick);
        if (axisIndex >= 0 && axisIndex < axisCount) {
            return atlasNormalizeAxisValue(
                SDL_GetJoystickAxis(joystick, axisIndex));
        }
    }
    return 0.0f;
}

std::pair<float, float> getControllerAxisPairValueForDevice(int controllerID,
                                                            int axisIndexX,
                                                            int axisIndexY) {
    auto readStandardPair = [&](int joystickID) {
        return std::pair<float, float>{
            getControllerAxisValueForDevice(joystickID, axisIndexX),
            getControllerAxisValueForDevice(joystickID, axisIndexY)};
    };

    auto readJoystickPair = [&](int joystickID) {
        const std::vector<float> axes = getJoystickAxes(joystickID);
        if (axes.size() < 2) {
            return std::pair<float, float>{0.0f, 0.0f};
        }

        struct AxisPairCandidate {
            int x;
            int y;
            float magnitude;
        };

        std::vector<AxisPairCandidate> candidates;
        int limit = std::min(static_cast<int>(axes.size()), 8);
        for (int start = 0; start + 1 < limit; start += 2) {
            float x = axes.at(start);
            float y = axes.at(start + 1);
            candidates.push_back(
                {start, start + 1, std::sqrt((x * x) + (y * y))});
        }

        if (candidates.empty()) {
            return std::pair<float, float>{0.0f, 0.0f};
        }

        std::sort(
            candidates.begin(), candidates.end(),
            [](const AxisPairCandidate &left, const AxisPairCandidate &right) {
                return left.magnitude > right.magnitude;
            });

        bool hasPreferredPair = axisIndexX >= 0 && axisIndexY >= 0 &&
                                axisIndexX < static_cast<int>(axes.size()) &&
                                axisIndexY < static_cast<int>(axes.size());
        if (hasPreferredPair) {
            float preferredX = axes.at(axisIndexX);
            float preferredY = axes.at(axisIndexY);
            float preferredMagnitude = std::sqrt((preferredX * preferredX) +
                                                 (preferredY * preferredY));
            if (preferredMagnitude > 0.01f) {
                return std::pair<float, float>{preferredX, preferredY};
            }
        }

        int requestedPairIndex = axisIndexX >= 0 ? axisIndexX / 2 : 0;
        if (requestedPairIndex >= 0 &&
            requestedPairIndex < static_cast<int>(candidates.size()) &&
            candidates[requestedPairIndex].magnitude > 0.01f) {
            const auto &candidate = candidates[requestedPairIndex];
            return std::pair<float, float>{axes.at(candidate.x),
                                           axes.at(candidate.y)};
        }

        if (requestedPairIndex == 0 && candidates.front().magnitude > 0.01f) {
            const auto &candidate = candidates.front();
            return std::pair<float, float>{axes.at(candidate.x),
                                           axes.at(candidate.y)};
        }

        return std::pair<float, float>{0.0f, 0.0f};
    };

    auto readPair = [&](int joystickID) {
        if (!SDL_IsGamepad(joystickID)) {
            return readJoystickPair(joystickID);
        }
        return readStandardPair(joystickID);
    };

    if (controllerID != CONTROLLER_UNDEFINED) {
        return readPair(controllerID);
    }

    std::pair<float, float> selected{0.0f, 0.0f};
    float bestMagnitude = -1.0f;
    for (SDL_JoystickID joystickID : getConnectedJoystickIds()) {
        auto pair = readPair(joystickID);
        float magnitude =
            std::sqrt(pair.first * pair.first + pair.second * pair.second);
        if (magnitude > bestMagnitude) {
            selected = pair;
            bestMagnitude = magnitude;
        }
    }

    return selected;
}

void registerGamepadMappings() {
    static const char *const mappings[] = {
        "030000007e0500000920000011810000,Nintendo Switch Pro "
        "Controller,a:b0,b:b1,back:b9,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "dpup:h0.1,guide:b11,leftshoulder:b5,leftstick:b12,lefttrigger:b7,"
        "leftx:a0,lefty:a1,misc1:b4,rightshoulder:b6,rightstick:b13,"
        "righttrigger:b8,rightx:a2,righty:a3,start:b10,x:b3,y:b2,"
        "hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,",
        "050000004c69632050726f20436f6e00,Nintendo Switch Pro "
        "Controller,crc:15b7,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,"
        "dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,"
        "lefttrigger:b6,leftx:a0,lefty:a1,misc1:b13,rightshoulder:b5,"
        "rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,"
        "x:b2,y:b3,hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,",
        "050000007e0500000920000001000000,Nintendo Switch Pro "
        "Controller,a:b1,b:b0,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,"
        "leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,"
        "rightx:a2,righty:a3,start:b9,x:b3,y:b2,"
        "hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,",
        "050000007e0500000920000001800000,Nintendo Switch Pro "
        "Controller,a:b0,b:b1,back:b9,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "dpup:h0.1,guide:b11,leftshoulder:b5,leftstick:b12,lefttrigger:b7,"
        "leftx:a0,lefty:a1,rightshoulder:b6,rightstick:b13,righttrigger:b8,"
        "rightx:a2,righty:a3,start:b10,x:b3,y:b2,"
        "hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,",
        "030000007e0500000920000000000000,Nintendo Switch Pro "
        "Controller,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,"
        "leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,"
        "rightx:a2,righty:a3,start:b9,x:b2,y:b3,",
        "050000007e05000009200000ff070000,Nintendo Switch Pro "
        "Controller,a:b1,b:b0,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
        "dpup:h0.1,guide:b9,leftshoulder:b4,leftstick:b6,lefttrigger:a2,"
        "leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b7,righttrigger:a5,"
        "rightx:a3,righty:a4,start:b10,x:b3,y:b2,"
        "hint:!SDL_GAMECONTROLLER_USE_BUTTON_LABELS:=1,",
    };

    for (const char *mapping : mappings) {
        SDL_AddGamepadMapping(mapping);
    }
}

void appendShadowCaster(Renderable *obj, std::unordered_set<Renderable *> &seen,
                        std::vector<Renderable *> &casters) {
    if (obj == nullptr) {
        return;
    }
    if (auto *model = dynamic_cast<Model *>(obj)) {
        const auto &meshes = static_cast<const Model *>(model)->getObjects();
        for (const auto &mesh : meshes) {
            Renderable *meshRenderable = mesh.get();
            if (meshRenderable == nullptr ||
                !meshRenderable->canCastShadows()) {
                continue;
            }
            if (seen.insert(meshRenderable).second) {
                casters.push_back(meshRenderable);
            }
        }
        return;
    }
    if (!obj->canCastShadows()) {
        return;
    }
    if (seen.insert(obj).second) {
        casters.push_back(obj);
    }
}

std::vector<Renderable *>
collectShadowCasters(const std::vector<Renderable *> &firstRenderables,
                     const std::vector<Renderable *> &renderables,
                     const std::vector<Renderable *> &lateForwardRenderables) {
    std::vector<Renderable *> casters;
    casters.reserve(firstRenderables.size() + renderables.size() +
                    lateForwardRenderables.size());
    std::unordered_set<Renderable *> seen;
    seen.reserve(firstRenderables.size() + renderables.size() +
                 lateForwardRenderables.size());

    for (auto *obj : firstRenderables) {
        appendShadowCaster(obj, seen, casters);
    }
    for (auto *obj : renderables) {
        if (obj != nullptr && obj->renderLateForward) {
            continue;
        }
        appendShadowCaster(obj, seen, casters);
    }
    for (auto *obj : lateForwardRenderables) {
        appendShadowCaster(obj, seen, casters);
    }

    return casters;
}

std::size_t
computeShadowCasterSignature(const std::vector<Renderable *> &shadowCasters) {
    std::size_t signature = 1469598103934665603ULL;

    for (auto *obj : shadowCasters) {
        if (obj == nullptr) {
            continue;
        }
        hashCombine(signature, reinterpret_cast<std::uintptr_t>(obj));

        const Position3d position = obj->getPosition();
        hashCombine(signature, position.x);
        hashCombine(signature, position.y);
        hashCombine(signature, position.z);

        const Size3d scale = obj->getScale();
        hashCombine(signature, scale.x);
        hashCombine(signature, scale.y);
        hashCombine(signature, scale.z);

        if (auto *gameObject = dynamic_cast<GameObject *>(obj);
            gameObject != nullptr) {
            const Rotation3d rotation = gameObject->getRotation();
            hashCombine(signature, rotation.pitch);
            hashCombine(signature, rotation.yaw);
            hashCombine(signature, rotation.roll);
        }
    }

    return signature;
}
} // namespace

Window::Window(const WindowConfiguration &config)
    : title(config.title), width(config.width), height(config.height) {
    atlas_log("Initializing window: " + config.title);
#ifdef METAL
    this->externalMetalView = config.metalTargetView;
    this->renderToExternalMetalView = this->externalMetalView != nullptr;
    this->showHostWindow = !this->renderToExternalMetalView;
#else
    (void)config.metalTargetView;
    this->externalMetalView = nullptr;
    this->renderToExternalMetalView = false;
    this->showHostWindow = true;
#endif

#ifdef VULKAN
    auto context = opal::Context::create({.useOpenGL = false});
    atlas_log("Using Vulkan backend");
#elif defined(METAL)
    auto context = opal::Context::create({.useOpenGL = false,
                                          .applicationName = config.title,
                                          .applicationVersion = ""});
    atlas_log("Using Metal backend");
#else
    auto context = opal::Context::create({.useOpenGL = true,
                                          .majorVersion = 4,
                                          .minorVersion = 1,
                                          .profile = opal::OpenGLProfile::Core,
                                          .applicationName = config.title,
                                          .applicationVersion = "0.0"});
    atlas_log("Using OpenGL backend");
#endif

#ifdef VULKAN
    this->frontFace = opal::FrontFace::Clockwise;
    this->deferredFrontFace = opal::FrontFace::Clockwise;
#elif defined(METAL)
    frontFace = opal::FrontFace::
        CounterClockwise; // NOLINT(*-prefer-member-initializer)
    deferredFrontFace = opal::FrontFace::
        CounterClockwise; // NOLINT(*-prefer-member-initializer)
#else
    this->frontFace = opal::FrontFace::CounterClockwise;
    this->deferredFrontFace = opal::FrontFace::CounterClockwise;
#endif

    context->setDecorated(config.decorations);
    context->setResizable(config.resizable);
    context->setTransparent(config.transparent);
    context->setAlwaysOnTop(config.alwaysOnTop);
    context->setSamples(config.multisampling ? 4 : 0);
    context->setHighPixelDensity(true);

#ifdef METAL
    if (this->renderToExternalMetalView) {
        context->setMetalTargetView(this->externalMetalView);
    }
#endif

    SDL_Window *window = nullptr;
    if (config.sdlInputWindow != nullptr) {
        context->adoptWindow(config.sdlInputWindow, false);
        window = config.sdlInputWindow;
    } else {
        window = context->makeWindow(config.width, config.height,
                                     config.title.c_str());
    }

    registerGamepadMappings();

    context->makeCurrent();

    device = opal::Device::acquire(context);
    this->windowRef = window;
    this->shouldClose = false;

    SDL_SetWindowOpacity(window, config.opacity);
    if (config.mouseCaptured) {
        captureMouse();
    } else {
        releaseMouse();
    }

    int fbWidth = 0;
    int fbHeight = 0;
    this->queryDrawableSizeInPixels(&fbWidth, &fbHeight);
    device->getDefaultFramebuffer()->setViewport(0, 0, fbWidth, fbHeight);
    this->viewportWidth = fbWidth;
    this->viewportHeight = fbHeight;

    if (config.posX != WINDOW_CENTERED && config.posY != WINDOW_CENTERED) {
        SDL_SetWindowPosition(window, config.posX, config.posY);
    }

    if (config.aspectRatioX != DEFAULT_ASPECT_RATIO &&
        config.aspectRatioY != DEFAULT_ASPECT_RATIO) {
        float aspectRatio = static_cast<float>(config.aspectRatioX) /
                            static_cast<float>(config.aspectRatioY);
        SDL_SetWindowAspectRatio(window, aspectRatio, aspectRatio);
    }

    this->renderScale = std::clamp(config.renderScale, 0.5f, 1.0f);
    this->ssaoRenderScale = std::clamp(config.ssaoScale, 0.25f, 1.0f);
    this->useMultisampling = config.multisampling;
    this->metalUpscalingRatio = this->renderScale;

    Window::mainWindow = this;

    float initialMouseX = 0.0f;
    float initialMouseY = 0.0f;
    SDL_GetMouseState(&initialMouseX, &initialMouseY);
    lastMouseX = initialMouseX;
    lastMouseY = initialMouseY;
    relativeMousePos = {0.0f, 0.0f};

    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Depth);
    vertexShader.compile();
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Empty);
    fragmentShader.compile();
    ShaderProgram program = ShaderProgram();
    program.vertexShader = vertexShader;
    program.fragmentShader = fragmentShader;
    program.compile();
    this->depthProgram = program;

#ifdef __APPLE__
    this->useMultiPassPointShadows = true;
#else
    this->useMultiPassPointShadows = false;
#endif

#ifdef METAL
    this->shadowUpdateInterval = 1.0f / 6.0f;
    this->ssaoUpdateInterval = 1.0f / 24.0f;
    this->ssaoKernelSize = 64;
    this->bloomBlurPasses = 4;
#endif

    ShaderProgram pointProgram = ShaderProgram();
    VertexShader pointVertexShader = VertexShader::fromDefaultShader(
        AtlasVertexShader::PointLightShadowNoGeom);
    pointVertexShader.compile();
    FragmentShader pointFragmentShader = FragmentShader::fromDefaultShader(
        AtlasFragmentShader::PointLightShadowNoGeom);
    pointFragmentShader.compile();
    pointProgram.vertexShader = pointVertexShader;
    pointProgram.fragmentShader = pointFragmentShader;
    pointProgram.compile();

    this->pointDepthProgram = pointProgram;

    this->deferredProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Deferred, AtlasFragmentShader::Deferred);
    this->lightProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Light);
    this->bloomBlurProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Fullscreen, AtlasFragmentShader::GaussianBlur);
    this->volumetricProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Volumetric, AtlasFragmentShader::Volumetric);
    this->ssrProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSR);

    this->setupSSAO();

    audioEngine = std::make_shared<AudioEngine>();
    bool result = audioEngine->initialize();
    if (!result) {
        atlas_error("Failed to initialize audio engine");
    }
    atlas_log("Audio engine initialized successfully");

    opal::DeviceInfo info = device->getDeviceInfo();

    std::cout << "\033[1m\033[36mAtlas Engine\033[0m" << std::endl;
    std::cout << "\033[1m\033[36mVersion " << ATLAS_VERSION << " \033[0m"
              << std::endl;
    std::cout << "\033[1m\033[31mUsing Opal Graphics Library - Version "
              << info.opalVersion << " \033[0m" << std::endl;
#ifdef OPENGL
    std::cout << "\033[1m\033[32mUsing OpenGL Backend\033[0m" << std::endl;
#elif defined(VULKAN)
    std::cout << "\033[1m\033[32mUsing Vulkan Backend\033[0m" << std::endl;
#elif defined(METAL)
    std::cout << "\033[1m\033[32mUsing Metal Backend\033[0m" << std::endl;
#else
    std::cout << "\033[1m\033[32mUsing Unknown Backend\033[0m" << std::endl;
#endif
#if defined(VULKAN) || defined(METAL)
    std::cout << "\033[1m\033[35m---------------\033[0m" << std::endl;
    std::cout << "\033[1m\033[35mUsing GPU: " << info.deviceName << "\033[0m"
              << std::endl;
    std::cout << "\033[1m\033[35mVendor ID: " << info.vendorName << "\033[0m"
              << std::endl;
    std::cout << "\033[1m\033[35mDriver Version: " << info.driverVersion
              << "\033[0m" << std::endl;
    std::cout << "\033[1m\033[35mAPI Version: " << info.renderingVersion
              << "\033[0m" << std::endl;
#endif

    if (this->waitForTracer) {
        TracerServices::getInstance().startTracing(TRACER_PORT);
        atlas_log("Atlas Tracer initialized.");
    }
}

std::tuple<int, int> Window::getCursorPosition() {
    float xpos = 0.0f;
    float ypos = 0.0f;
    SDL_GetMouseState(&xpos, &ypos);

    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(this->windowRef, &windowWidth, &windowHeight);

    int pixelWidth = 0;
    int pixelHeight = 0;
    this->queryDrawableSizeInPixels(&pixelWidth, &pixelHeight);

    const float scaleX = windowWidth > 0 ? static_cast<float>(pixelWidth) /
                                               static_cast<float>(windowWidth)
                                         : 1.0f;
    const float scaleY = windowHeight > 0 ? static_cast<float>(pixelHeight) /
                                                static_cast<float>(windowHeight)
                                          : 1.0f;

    return {static_cast<int>(std::lround(xpos * scaleX)),
            static_cast<int>(std::lround(ypos * scaleY))};
}

void Window::queryDrawableSizeInPixels(int *width, int *height) const {
    int pixelWidth = 0;
    int pixelHeight = 0;
    atlasGetWindowSizeInPixels(this->windowRef, &pixelWidth, &pixelHeight);

#if defined(METAL) && defined(__APPLE__)
    if (this->externalMetalView != nullptr) {
        CocoaObj targetView = this->externalMetalView;
        CocoaRect bounds = sendObjCRect(targetView, "bounds");
        double scale = 1.0;
        CocoaObj hostWindow = sendObjCId(targetView, "window");
        if (hostWindow != nullptr) {
            const double backingScale =
                sendObjCDouble(hostWindow, "backingScaleFactor");
            if (backingScale > 0.0) {
                scale = backingScale;
            }
        }

        if (bounds.size.width > 0.0 && bounds.size.height > 0.0) {
            pixelWidth = std::max(
                1, static_cast<int>(std::lround(bounds.size.width * scale)));
            pixelHeight = std::max(
                1, static_cast<int>(std::lround(bounds.size.height * scale)));
        }
    }
#endif

    if (width != nullptr) {
        *width = pixelWidth;
    }
    if (height != nullptr) {
        *height = pixelHeight;
    }
}

void Window::initializeRunLoop() {
    if (this->runLoopInitialized) {
        return;
    }

    if (this->camera == nullptr) {
        this->camera = new Camera();
    }
    this->physicsWorld = std::make_shared<bezel::PhysicsWorld>();
    this->physicsWorld->init();

    for (auto &obj : this->renderables) {
        obj->initialize();
    }
    for (auto &obj : this->preferenceRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->firstRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->lateForwardRenderables) {
        obj->initialize();
    }
    for (auto &obj : this->uiRenderables) {
        obj->initialize();
    }

    SDL_Window *window = this->windowRef;
    this->runLoopWindowID = SDL_GetWindowID(window);
    if (this->showHostWindow) {
        if (!SDL_ShowWindow(window)) {
            atlas_warning("Failed to show window");
        }
        if (!SDL_SyncWindow(window)) {
            atlas_warning("Failed to synchronize window state");
        }
        if (!SDL_RaiseWindow(window)) {
            atlas_warning("Failed to focus window");
        }
    } else {
        SDL_HideWindow(window);
    }

    this->activeCommandBuffer = device->acquireCommandBuffer();
    this->lastTime = atlasGetTimeSeconds();

    updatePipelineStateField(useMultisampling, this->useMultisampling);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->useBlending, true);
    updatePipelineStateField(this->srcBlend, opal::BlendFunc::SrcAlpha);
    updatePipelineStateField(this->dstBlend, opal::BlendFunc::OneMinusSrcAlpha);

    this->framesPerSecond = 0.0f;
    auto defaultFramebuffer = device->getDefaultFramebuffer();
    this->runLoopRenderPass = opal::RenderPass::create();
    this->runLoopRenderPass->setFramebuffer(defaultFramebuffer);
    this->firstFrame = true;
    this->runLoopInitialized = true;
}

void Window::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            shouldClose = true;
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == this->runLoopWindowID) {
                shouldClose = true;
            }
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (event.window.windowID == this->runLoopWindowID) {
                int resizedWidth = 0;
                int resizedHeight = 0;
                this->queryDrawableSizeInPixels(&resizedWidth, &resizedHeight);
                this->viewportWidth = resizedWidth;
                this->viewportHeight = resizedHeight;
                this->shadowMapsDirty = true;
                this->ssaoMapsDirty = true;
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.windowID == this->runLoopWindowID) {
                const int scancode = static_cast<int>(event.key.scancode);
                if (scancode >= 0 &&
                    scancode <
                        static_cast<int>(this->keysPressedThisFrame.size())) {
                    this->keysPressedThisFrame[scancode] = true;
                }
            }
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (event.text.windowID == this->runLoopWindowID &&
                event.text.text != nullptr) {
                this->textInputBuffer += event.text.text;
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (event.motion.windowID == this->runLoopWindowID) {
                Position2d movement = {.x = event.motion.xrel,
                                       .y = -event.motion.yrel};
                this->relativeMousePos.x += movement.x;
                this->relativeMousePos.y += movement.y;
                if (this->currentScene != nullptr) {
                    this->currentScene->onMouseMove(*this, movement);
                }
                this->lastMouseX = event.motion.x;
                this->lastMouseY = event.motion.y;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.windowID == this->runLoopWindowID) {
                const std::size_t button =
                    static_cast<std::size_t>(event.button.button);
                if (button < this->mouseButtonsPressedThisFrame.size()) {
                    this->mouseButtonsPressedThisFrame[button] = true;
                }
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            if (event.wheel.windowID == this->runLoopWindowID &&
                this->currentScene != nullptr) {
                float offsetX = event.wheel.x;
                float offsetY = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    offsetX = -offsetX;
                    offsetY = -offsetY;
                }
                Position2d offset = {.x = offsetX, .y = offsetY};
                this->currentScene->onMouseScroll(*this, offset);
            }
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closeGamepadHandle(event.gdevice.which);
            break;
        case SDL_EVENT_JOYSTICK_REMOVED:
            closeJoystickHandle(event.jdevice.which);
            break;
        default:
            break;
        }
    }
}

bool Window::stepFrame() {
    this->initializeRunLoop();
    if (this->shouldClose) {
        return false;
    }

    SDL_Window *window = this->windowRef;
    auto commandBuffer = this->activeCommandBuffer;
    auto renderPass = this->runLoopRenderPass;
    constexpr float MAX_DELTA_TIME = 1.0f / 30.0f;

    currentFrame++;
    this->relativeMousePos = {.x = 0.0f, .y = 0.0f};
    this->keysPressedThisFrame.fill(false);
    this->mouseButtonsPressedThisFrame.fill(false);
    this->textInputBuffer.clear();
    this->pollEvents();

    if (this->hasPendingSceneChange) {
        this->applyScene(this->pendingScene);
        this->pendingScene = nullptr;
        this->hasPendingSceneChange = false;
    }

    float currentTime = atlasGetTimeSeconds();
    float rawDelta = currentTime - this->lastTime;
    this->lastTime = currentTime;

    const bool isFirstFrame = this->firstFrame;
    if (isFirstFrame) {
        this->deltaTime = 0.0f;
    } else {
        rawDelta = std::max(rawDelta, 0.0f);
        this->deltaTime = std::min(rawDelta, MAX_DELTA_TIME);
    }

    if (this->deltaTime > 0.0f) {
        this->framesPerSecond = 1.0f / this->deltaTime;
    }

    device->frameCount++;

    for (auto *obj : this->pendingRemovals) {
        this->renderables.erase(std::remove(this->renderables.begin(),
                                            this->renderables.end(), obj),
                                this->renderables.end());
        this->lateForwardRenderables.erase(
            std::remove(this->lateForwardRenderables.begin(),
                        this->lateForwardRenderables.end(), obj),
            this->lateForwardRenderables.end());
        this->preferenceRenderables.erase(
            std::remove(this->preferenceRenderables.begin(),
                        this->preferenceRenderables.end(), obj),
            this->preferenceRenderables.end());
        this->firstRenderables.erase(std::remove(this->firstRenderables.begin(),
                                                 this->firstRenderables.end(),
                                                 obj),
                                     this->firstRenderables.end());
        this->uiRenderables.erase(std::remove(this->uiRenderables.begin(),
                                              this->uiRenderables.end(), obj),
                                  this->uiRenderables.end());
        if (auto *fluid = dynamic_cast<Fluid *>(obj)) {
            this->lateFluids.erase(std::remove(this->lateFluids.begin(),
                                               this->lateFluids.end(), fluid),
                                   this->lateFluids.end());
        }
    }
    this->pendingRemovals.clear();

    for (auto *obj : this->pendingObjects) {
        obj->initialize();
        this->renderables.push_back(obj);
        if (obj->renderLateForward) {
            this->addLateForwardObject(obj);
        }
    }
    this->pendingObjects.clear();

    DebugTimer cpuTimer("Cpu Data");
    DebugTimer mainTimer("Main Loop");

    for (auto &obj : this->renderables) {
        obj->beforePhysics();
    }

    this->physicsWorld->update(this->deltaTime);

    if (this->hasPendingSceneChange) {
        this->applyScene(this->pendingScene);
        this->pendingScene = nullptr;
        this->hasPendingSceneChange = false;
        return !this->shouldClose;
    }

    if (this->currentScene == nullptr) {
        commandBuffer->start();
        commandBuffer->beginPass(renderPass);
        commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                                  this->clearColor.b, this->clearColor.a);
        commandBuffer->clearDepth(1.0);
        commandBuffer->endPass();
        commandBuffer->commit();
#ifdef OPENGL
        SDL_GL_SwapWindow(window);
#endif
        return !this->shouldClose;
    }

    commandBuffer->start();

    currentScene->updateScene(this->deltaTime);

    for (auto &obj : this->firstRenderables) {
        if (obj == nullptr) {
            continue;
        }
        obj->update(*this);
    }

    for (auto &obj : this->renderables) {
        if (obj->renderLateForward) {
            continue;
        }
        obj->update(*this);
    }

    for (auto &obj : this->lateForwardRenderables) {
        obj->update(*this);
    }

    currentScene->update(*this);

    uint64_t cpuTime = cpuTimer.stop();

    DebugTimer gpuTimer("Gpu Data");

    renderLightsToShadowMaps(commandBuffer);

    static std::unique_ptr<RenderTarget> modeScreenTarget = nullptr;
    std::vector<RenderTarget *> activeRenderTargets = this->renderTargets;
    bool usesModeScreenTarget = false;
    if (activeRenderTargets.empty() &&
        (this->usePathTracing || this->usesDeferred)) {
        if (!modeScreenTarget) {
            modeScreenTarget =
                std::make_unique<RenderTarget>(*this, RenderTargetType::Scene);
        }
        modeScreenTarget->display(*this, 0.0f);
        modeScreenTarget->show();
        activeRenderTargets.push_back(modeScreenTarget.get());
        usesModeScreenTarget = true;
    } else if (modeScreenTarget) {
        modeScreenTarget->hide();
    }

    for (auto &target : activeRenderTargets) {
        if (target == nullptr) {
            continue;
        }
        this->currentRenderTarget = target;
        setViewportState(0, 0, target->getWidth(), target->getHeight());
        updatePipelineStateField(this->depthCompareOp, opal::CompareOp::Less);
        updatePipelineStateField(this->writeDepth, true);
        updatePipelineStateField(this->cullMode, opal::CullMode::Back);

        auto newRenderPass = opal::RenderPass::create();
        newRenderPass->setFramebuffer(target->getFramebuffer());
        if (target->brightTexture.id != 0) {
            target->getFramebuffer()->setDrawBuffers(2);
        }

        if (this->usePathTracing) {
            if (pathTracer == nullptr) {
                continue;
            }
#ifdef METAL
            pathTracer->resizeOutput(target->getWidth(), target->getHeight());
            pathTracer->render(commandBuffer);
            pathTracer->copySrcFramebuffer->attachTexture(
                pathTracer->pathTracingTexture->texture, 0);
            pathTracer->copyDstFramebuffer->attachTexture(
                target->texture.texture, 0);
            auto copy = opal::ResolveAction::createForColorAttachment(
                pathTracer->copySrcFramebuffer, pathTracer->copyDstFramebuffer,
                0);
            commandBuffer->performResolve(copy);
#endif

            continue;
        }

        if (this->usesDeferred) {
            if (this->gBuffer == nullptr) {
                this->useDeferredRendering();
            }

            this->deferredRendering(target, commandBuffer);

            auto forwardFramebuffer = target->getFramebuffer();
            if (target->type == RenderTargetType::Multisampled &&
                target->getResolveFramebuffer() != nullptr) {
                forwardFramebuffer = target->getResolveFramebuffer();
            }
            auto resolveCommand = opal::ResolveAction::createForDepth(
                this->gBuffer->getFramebuffer(), forwardFramebuffer);
            commandBuffer->performResolve(resolveCommand);

            auto forwardRenderPass = opal::RenderPass::create();
            forwardRenderPass->setFramebuffer(forwardFramebuffer);
            forwardFramebuffer->setDrawBuffers(2);
            commandBuffer->beginPass(forwardRenderPass);
            forwardFramebuffer->setViewport(0, 0, target->getWidth(),
                                            target->getHeight());

            updatePipelineStateField(this->useDepth, true);
            updatePipelineStateField(this->depthCompareOp,
                                     opal::CompareOp::Less);
            updatePipelineStateField(this->writeDepth, true);
            updatePipelineStateField(this->cullMode, opal::CullMode::Back);

            auto renderForwardOnly = [&](Renderable *obj) {
                if (obj == nullptr) {
                    return;
                }
                if (auto *model = dynamic_cast<Model *>(obj)) {
                    const auto &meshes =
                        static_cast<const Model *>(model)->getObjects();
                    for (const auto &mesh : meshes) {
                        CoreObject *meshObject = mesh.get();
                        if (meshObject == nullptr) {
                            continue;
                        }
                        bool hasAnyTexture = !meshObject->textures.empty();
                        if (!hasAnyTexture) {
                            meshObject->material = model->material;
                        }
                        meshObject->material.useNormalMap =
                            model->material.useNormalMap;
                        meshObject->material.normalMapStrength =
                            model->material.normalMapStrength;
                        meshObject->useDeferredRendering =
                            model->useDeferredRendering;
                        if (meshObject->canUseDeferredRendering()) {
                            continue;
                        }
                        meshObject->setViewMatrix(
                            this->camera->calculateViewMatrix());
                        meshObject->setProjectionMatrix(
                            calculateProjectionMatrix());
                        meshObject->render(getDeltaTime(), commandBuffer,
                                           shouldRefreshPipeline(meshObject));
                    }
                    return;
                }
                if (obj->canUseDeferredRendering()) {
                    return;
                }
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            };

            for (auto &obj : this->firstRenderables) {
                renderForwardOnly(obj);
            }

            for (auto &obj : this->renderables) {
                if (obj != nullptr && obj->renderLateForward) {
                    continue;
                }
                renderForwardOnly(obj);
            }

            for (auto &obj : this->lateForwardRenderables) {
                obj->setViewMatrix(this->camera->calculateViewMatrix());
                obj->setProjectionMatrix(calculateProjectionMatrix());
                obj->render(getDeltaTime(), commandBuffer,
                            shouldRefreshPipeline(obj));
            }

            commandBuffer->endPass();
            continue;
        }
        commandBuffer->beginPass(newRenderPass);
        commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                                  this->clearColor.b, this->clearColor.a);
        commandBuffer->clearDepth(1.0f);

        for (auto &obj : this->firstRenderables) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }

        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
        updateFluidCaptures(commandBuffer);
        for (auto &obj : this->lateForwardRenderables) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
        commandBuffer->endPass();
        target->resolve();
    }

    for (auto &obj : this->preferenceRenderables) {
        RenderTarget *target = dynamic_cast<RenderTarget *>(obj);
        if (target != nullptr && target->brightTexture.id != 0) {
            this->renderPhysicalBloom(target);
        }
    }

    commandBuffer->beginPass(renderPass);
    int fbWidth, fbHeight;
    this->queryDrawableSizeInPixels(&fbWidth, &fbHeight);
    setViewportState(0, 0, fbWidth, fbHeight);
    commandBuffer->clearColor(this->clearColor.r, this->clearColor.g,
                              this->clearColor.b, this->clearColor.a);
    commandBuffer->clearDepth(1.0f);

    if (this->renderTargets.empty() && !usesModeScreenTarget) {
        updateBackbufferTarget(fbWidth, fbHeight);
        this->currentRenderTarget = this->screenRenderTarget.get();
        for (auto &obj : this->firstRenderables) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }

        for (auto &obj : this->renderables) {
            if (obj->renderLateForward) {
                continue;
            }
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }

        updateFluidCaptures(commandBuffer);

        for (auto &obj : this->lateForwardRenderables) {
            obj->setViewMatrix(this->camera->calculateViewMatrix());
            obj->setProjectionMatrix(calculateProjectionMatrix());
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
    } else {
        this->currentRenderTarget = nullptr;
    }

    for (auto &obj : this->preferenceRenderables) {
        obj->setViewMatrix(this->camera->calculateViewMatrix());
        obj->setProjectionMatrix(calculateProjectionMatrix());
        obj->render(getDeltaTime(), commandBuffer, shouldRefreshPipeline(obj));
    }

    updatePipelineStateField(this->useBlending, true);

    for (auto &obj : this->uiRenderables) {
        obj->render(getDeltaTime(), commandBuffer, shouldRefreshPipeline(obj));
    }

    this->lastViewMatrix = this->camera->calculateViewMatrix();

    commandBuffer->endPass();
    commandBuffer->commit();
#ifdef OPENGL
    SDL_GL_SwapWindow(window);
#endif

    uint64_t gpuTime = gpuTimer.stop();
    uint64_t mainTime = mainTimer.stop();

    if (TracerServices::getInstance().isOk()) {
        FrameDrawInfo frameInfo{};
        frameInfo.drawCallCount = commandBuffer->getAndResetDrawCallCount();
        frameInfo.frameTimeMs = this->deltaTime * 1000.0f;
        frameInfo.frameNumber = device->frameCount;
        frameInfo.fps = this->framesPerSecond;
        frameInfo.send();

        FrameResourcesInfo frameResourcesInfo{};
        frameResourcesInfo.frameNumber = device->frameCount;
        frameResourcesInfo.resourcesCreated =
            ResourceTracker::getInstance().createdResources;
        frameResourcesInfo.resourcesUnloaded =
            ResourceTracker::getInstance().unloadedResources;
        frameResourcesInfo.resourcesLoaded =
            ResourceTracker::getInstance().loadedResources;
        frameResourcesInfo.totalMemoryMb =
            ResourceTracker::getInstance().totalMemoryMb;

        FrameMemoryPacket memoryPacket{};
        memoryPacket.frameNumber = device->frameCount;
        memoryPacket.allocationCount =
            ResourceTracker::getInstance().createdResources -
            ResourceTracker::getInstance().unloadedResources;
        memoryPacket.totalAllocatedMb =
            ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.totalCPUMb = ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.totalGPUMb = ResourceTracker::getInstance().totalMemoryMb;
        memoryPacket.deallocationCount =
            ResourceTracker::getInstance().unloadedResources;
        memoryPacket.send();

        rusage usage{};
        getrusage(RUSAGE_SELF, &usage);

        double normalCpuTime =
            usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1e6) +
            usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1e6);

        TimingEventPacket timingEvent;
        timingEvent.frameNumber = device->frameCount;
        timingEvent.durationMs = static_cast<float>(gpuTime) / 1'000'000.0f;
        timingEvent.name = "Main Loop";
        timingEvent.subsystem = TimingEventSubsystem::Rendering;
        timingEvent.send();

        FrameTimingPacket timingPacket{};
        timingPacket.frameNumber = device->frameCount;
        timingPacket.cpuFrameTimeMs =
            static_cast<float>(cpuTime) / 1'000'000.0f;
        timingPacket.gpuFrameTimeMs =
            static_cast<float>(gpuTime) / 1'000'000.0f;
        timingPacket.workerThreadTimeMs = 0.0f;
        timingPacket.mainThreadTimeMs =
            static_cast<float>(mainTime) / 1'000'000.0f;
        timingPacket.memoryMb = ResourceTracker::getInstance().totalMemoryMb;
        timingPacket.cpuUsagePercent =
            static_cast<float>(normalCpuTime / this->deltaTime * 100.0);
        timingPacket.gpuUsagePercent = 0.0f;
        timingPacket.send();

        frameResourcesInfo.send();
    } else {
        commandBuffer->getAndResetDrawCallCount();
    }

    ResourceTracker::getInstance().createdResources = 0;
    ResourceTracker::getInstance().loadedResources = 0;
    ResourceTracker::getInstance().unloadedResources = 0;
    ResourceTracker::getInstance().totalMemoryMb = 0.0f;

    if (this->firstFrame) {
        this->firstFrame = false;
    }

    return !this->shouldClose;
}

void Window::resize(int width, int height, float scale) {
    const int clampedWidth = std::max(1, width);
    const int clampedHeight = std::max(1, height);
    const float clampedScale = scale > 0.0f ? scale : 1.0f;

    this->width = clampedWidth;
    this->height = clampedHeight;

    if (this->windowRef != nullptr) {
        SDL_SetWindowSize(this->windowRef, clampedWidth, clampedHeight);
    }

    const int pixelWidth = std::max(
        1, static_cast<int>(std::lround(clampedWidth * clampedScale)));
    const int pixelHeight = std::max(
        1, static_cast<int>(std::lround(clampedHeight * clampedScale)));

    if (device == nullptr) {
        return;
    }

    device->getDefaultFramebuffer()->setViewport(0, 0, pixelWidth, pixelHeight);
    setViewportState(0, 0, pixelWidth, pixelHeight);
    this->shadowMapsDirty = true;
    this->ssaoMapsDirty = true;
}

void Window::endRunLoop() {
    if (!this->runLoopInitialized) {
        return;
    }
    this->activeCommandBuffer = nullptr;
    this->runLoopRenderPass = nullptr;
    this->runLoopWindowID = 0;
    this->runLoopInitialized = false;
}

void Window::run() {
    while (this->stepFrame()) {
    }
    this->endRunLoop();
}

void Window::addObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }

    const bool inRenderables =
        std::ranges::find(this->renderables, obj) != this->renderables.end();
    const bool inPending = std::ranges::find(this->pendingObjects, obj) !=
                           this->pendingObjects.end();
    if (inRenderables || inPending) {
        return;
    }

    if (this->physicsWorld != nullptr) {
        this->pendingObjects.push_back(obj);
    } else {
        this->renderables.push_back(obj);
        if (obj->renderLateForward) {
            this->addLateForwardObject(obj);
        }
    }
    this->shadowMapsDirty = true;
    this->shadowUpdateCooldown = 0.0f;
    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
}

void Window::removeObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }

    if (this->physicsWorld != nullptr) {
        this->pendingRemovals.push_back(obj);
    } else {
        this->renderables.erase(std::remove(this->renderables.begin(),
                                            this->renderables.end(), obj),
                                this->renderables.end());
        this->lateForwardRenderables.erase(
            std::remove(this->lateForwardRenderables.begin(),
                        this->lateForwardRenderables.end(), obj),
            this->lateForwardRenderables.end());
        this->preferenceRenderables.erase(
            std::remove(this->preferenceRenderables.begin(),
                        this->preferenceRenderables.end(), obj),
            this->preferenceRenderables.end());
        this->firstRenderables.erase(std::remove(this->firstRenderables.begin(),
                                                 this->firstRenderables.end(),
                                                 obj),
                                     this->firstRenderables.end());
        this->uiRenderables.erase(std::remove(this->uiRenderables.begin(),
                                              this->uiRenderables.end(), obj),
                                  this->uiRenderables.end());
        if (auto *fluid = dynamic_cast<Fluid *>(obj)) {
            this->lateFluids.erase(std::remove(this->lateFluids.begin(),
                                               this->lateFluids.end(), fluid),
                                   this->lateFluids.end());
        }
    }
    this->shadowMapsDirty = true;
    this->shadowUpdateCooldown = 0.0f;
    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
}

void Window::addLateForwardObject(Renderable *object) {
    if (object == nullptr) {
        return;
    }

    if (std::ranges::find(lateForwardRenderables, object) ==
        lateForwardRenderables.end()) {
        lateForwardRenderables.push_back(object);
        const bool inRenderables =
            std::ranges::find(this->renderables, object) !=
            this->renderables.end();
        const bool inPending =
            std::ranges::find(this->pendingObjects, object) !=
            this->pendingObjects.end();
        if (this->physicsWorld != nullptr && !inRenderables && !inPending) {
            object->initialize();
        }
    }

    if (auto *fluid = dynamic_cast<Fluid *>(object)) {
        if (std::ranges::find(lateFluids, fluid) == lateFluids.end()) {
            lateFluids.push_back(fluid);
        }
    }
}

void Window::addPreferencedObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }
    if (std::ranges::find(this->preferenceRenderables, obj) !=
        this->preferenceRenderables.end()) {
        return;
    }
    this->preferenceRenderables.push_back(obj);
    const bool inRenderables =
        std::ranges::find(this->renderables, obj) != this->renderables.end();
    const bool inPending = std::ranges::find(this->pendingObjects, obj) !=
                           this->pendingObjects.end();
    if (this->physicsWorld != nullptr && !inRenderables && !inPending) {
        obj->initialize();
    }
}

void Window::addPreludeObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }
    if (std::ranges::find(this->firstRenderables, obj) !=
        this->firstRenderables.end()) {
        return;
    }
    this->firstRenderables.push_back(obj);
    const bool inRenderables =
        std::ranges::find(this->renderables, obj) != this->renderables.end();
    const bool inPending = std::ranges::find(this->pendingObjects, obj) !=
                           this->pendingObjects.end();
    if (this->physicsWorld != nullptr && !inRenderables && !inPending) {
        obj->initialize();
    }
    this->shadowMapsDirty = true;
    this->shadowUpdateCooldown = 0.0f;
    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
}

void Window::addUIObject(Renderable *obj) {
    if (obj == nullptr) {
        return;
    }
    if (std::ranges::find(this->uiRenderables, obj) !=
        this->uiRenderables.end()) {
        return;
    }
    this->uiRenderables.push_back(obj);
    const bool inRenderables =
        std::ranges::find(this->renderables, obj) != this->renderables.end();
    const bool inPending = std::ranges::find(this->pendingObjects, obj) !=
                           this->pendingObjects.end();
    if (this->physicsWorld != nullptr && !inRenderables && !inPending) {
        obj->initialize();
    }
}

void Window::setLogOutput(bool showLogs, bool showWarnings, bool showErrors) {
    Logger::getInstance().setConsoleFilter(showLogs, showWarnings, showErrors);
}

void Window::close() { this->shouldClose = true; }

void Window::setCamera(Camera *newCamera) { this->camera = newCamera; }

void Window::applyScene(Scene *scene) {
    atlas_log("Setting active scene");
    this->pendingScene = nullptr;
    this->hasPendingSceneChange = false;

#ifndef BEZEL_NATIVE
    for (auto &[bodyId, body] : bezel_jolt::bodyIdToRigidbodyMap) {
        (void)bodyId;
        if (body != nullptr) {
            body->id.joltId = bezel::INVALID_JOLT_ID;
        }
    }
    bezel_jolt::bodyIdToRigidbodyMap.clear();
#endif

    if (this->physicsWorld != nullptr) {
        this->physicsWorld = std::make_shared<bezel::PhysicsWorld>();
        this->physicsWorld->init();
        this->physicsWorld->setGravity({0.0f, -this->gravity, 0.0f});
    }

    this->pendingRemovals.clear();
    this->renderables.clear();
    this->pendingObjects.clear();
    this->preferenceRenderables.clear();
    this->firstRenderables.clear();
    this->uiRenderables.clear();
    this->lateForwardRenderables.clear();
    this->lateFluids.clear();
    this->renderTargets.clear();
    this->screenRenderTarget.reset();
    this->gBuffer = nullptr;
    this->ssaoBuffer = nullptr;
    this->ssaoBlurBuffer = nullptr;
    this->volumetricBuffer = nullptr;
    this->lightBuffer = nullptr;
    this->ssrFramebuffer = nullptr;
    this->bloomBuffer = nullptr;
    this->currentScene = scene;
    this->firstFrame = true;
    this->deltaTime = 0.0f;
    this->framesPerSecond = 0.0f;

    if (scene != nullptr) {
        scene->initialize(*this);
    }
    this->shadowMapsDirty = true;
    this->shadowUpdateCooldown = 0.0f;
    this->lastShadowCameraPosition.reset();
    this->lastShadowCameraDirection.reset();
    this->cachedDirectionalLightDirections.clear();
    this->cachedPointLightPositions.clear();
    this->cachedSpotlightPositions.clear();
    this->cachedSpotlightDirections.clear();
    this->cachedAreaLightPositions.clear();
    this->cachedAreaLightNormals.clear();
    this->cachedAreaLightProperties.clear();
    this->lastShadowCasterSignature = 0;
    this->hasShadowCasterSignature = false;
    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
    this->lastSSAOCameraPosition.reset();
    this->lastSSAOCameraDirection.reset();
}

void Window::setScene(Scene *scene) {
    if (!this->hasPendingSceneChange && scene == this->currentScene) {
        return;
    }
    if (this->hasPendingSceneChange) {
        return;
    }
    if (this->physicsWorld != nullptr) {
        this->pendingScene = scene;
        this->hasPendingSceneChange = true;
        return;
    }
    this->applyScene(scene);
}

glm::mat4 Window::calculateProjectionMatrix() {
    glm::mat4 projection;
    if (!this->camera->useOrthographic) {
        int fbWidth, fbHeight;
        this->queryDrawableSizeInPixels(&fbWidth, &fbHeight);

        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        projection = glm::perspective(glm::radians(camera->fov), aspectRatio,
                                      camera->nearClip, camera->farClip);
    } else {
        float orthoSize = this->camera->orthographicSize;
        int fbWidth, fbHeight;
        this->queryDrawableSizeInPixels(&fbWidth, &fbHeight);
        float aspectRatio =
            static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        projection = glm::ortho(-orthoSize * aspectRatio,
                                orthoSize * aspectRatio, -orthoSize, orthoSize,
                                camera->nearClip, camera->farClip);
    }

    // For Vulkan, flip Y in projection to match clip-space conventions
#ifdef VULKAN
    projection[1][1] *= -1.0f;
#endif
    return projection;
}

void Window::setFullscreen(bool enable) {
    atlas_log(enable ? "Switching to fullscreen mode"
                     : "Switching to windowed mode");
    SDL_Window *window = this->windowRef;
    if (enable) {
        SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(displayID);
        if (mode != nullptr) {
            SDL_SetWindowFullscreenMode(window, mode);
        }
        SDL_SetWindowFullscreen(window, true);
    } else {
        SDL_SetWindowFullscreen(window, false);
        SDL_SetWindowSize(window, this->width, this->height);
        SDL_SetWindowPosition(window, 100, 100);
    }
}

void Window::setFullscreen(Monitor &monitor) {
    SDL_Window *window = this->windowRef;
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(monitor.monitorRef);
    auto [posX, posY] = monitor.getPosition();
    SDL_SetWindowPosition(window, posX, posY);
    if (mode != nullptr) {
        SDL_SetWindowFullscreenMode(window, mode);
    }
    SDL_SetWindowFullscreen(window, true);
}

void Window::setWindowed(const WindowConfiguration &config) {
    SDL_Window *window = this->windowRef;
    int windowWidth = config.width;
    int windowHeight = config.height;
    this->renderScale = std::clamp(config.renderScale, 0.5f, 1.0f);
    this->ssaoRenderScale = std::clamp(config.ssaoScale, 0.25f, 1.0f);
    this->useMultisampling = config.multisampling;
    this->metalUpscalingRatio = this->renderScale;
    int posX = config.posX != WINDOW_CENTERED ? config.posX : 100;
    int posY = config.posY != WINDOW_CENTERED ? config.posY : 100;
    SDL_SetWindowFullscreen(window, false);
    SDL_SetWindowSize(window, windowWidth, windowHeight);
    SDL_SetWindowPosition(window, posX, posY);
    this->shadowMapsDirty = true;
    this->ssaoMapsDirty = true;
}

void Window::useMetalUpscaling(float ratio) {
    float clampedRatio = std::clamp(ratio, 0.25f, 1.0f);
#ifdef METAL
    this->metalUpscalingEnabled = clampedRatio < 1.0f;
    this->metalUpscalingRatio = clampedRatio;
    this->renderScale = clampedRatio;
#else
    this->metalUpscalingEnabled = false;
    this->metalUpscalingRatio = 1.0f;
    this->renderScale = std::clamp(clampedRatio, 0.5f, 1.0f);
#endif
    this->shadowMapsDirty = true;
    this->ssaoMapsDirty = true;
}

std::vector<Monitor> Window::enumerateMonitors() {
    int count = 0;
    SDL_DisplayID *monitors = SDL_GetDisplays(&count);
    std::vector<Monitor> monitorList;
    monitorList.reserve(std::max(count, 0));
    for (int i = 0; monitors != nullptr && i < count; ++i) {
        bool isPrimary = (monitors[i] == SDL_GetPrimaryDisplay());
        monitorList.emplace_back(monitors[i], i, isPrimary);
    }
    SDL_free(monitors);
    return monitorList;
}

Window::~Window() {
    this->pingpongFramebuffers.at(0) = nullptr;
    this->pingpongFramebuffers.at(1) = nullptr;
    this->pingpongTextures.at(1) = nullptr;
    this->pingpongTextures.at(1) = nullptr;
    this->pingpongWidth = 0;
    this->pingpongHeight = 0;
    closeAllInputDeviceHandles();
    Window::mainWindow = nullptr;
}

Monitor::Monitor(CoreMonitorReference ref, int id, bool isPrimary)
    : monitorID(id), primary(isPrimary), monitorRef(ref) {}

std::vector<VideoMode> Monitor::queryVideoModes() const {
    int count = 0;
    SDL_DisplayMode **modes =
        SDL_GetFullscreenDisplayModes(this->monitorRef, &count);
    std::vector<VideoMode> videoModes;
    videoModes.reserve(std::max(count, 0));
    for (int i = 0; modes != nullptr && i < count; ++i) {
        videoModes.push_back({.width = modes[i]->w,
                              .height = modes[i]->h,
                              .refreshRate = static_cast<int>(
                                  std::lround(modes[i]->refresh_rate))});
    }
    SDL_free(modes);
    return videoModes;
}

VideoMode Monitor::getCurrentVideoMode() const {
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(this->monitorRef);
    if (mode == nullptr) {
        return {};
    }
    return {.width = mode->w,
            .height = mode->h,
            .refreshRate = static_cast<int>(std::lround(mode->refresh_rate))};
}

std::tuple<int, int> Monitor::getPhysicalSize() const { return {0, 0}; }

std::tuple<int, int> Monitor::getPosition() const {
    SDL_Rect rect{};
    if (!SDL_GetDisplayBounds(this->monitorRef, &rect)) {
        return {0, 0};
    }
    return {rect.x, rect.y};
}

std::tuple<float, float> Monitor::getContentScale() const {
    float scale = SDL_GetDisplayContentScale(this->monitorRef);
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    return {scale, scale};
}

std::string Monitor::getName() const {
    const char *name = SDL_GetDisplayName(this->monitorRef);
    return name != nullptr ? std::string(name) : std::string();
}

float Window::getTime() { return atlasGetTimeSeconds(); }

bool Window::isKeyActive(Key key) {
    int keyCount = 0;
    const bool *state = SDL_GetKeyboardState(&keyCount);
    const int scancode = static_cast<int>(key);
    return state != nullptr && scancode >= 0 && scancode < keyCount &&
           state[scancode];
}

bool Window::isKeyPressed(Key key) {
    const int scancode = static_cast<int>(key);
    return scancode >= 0 &&
           scancode < static_cast<int>(this->keysPressedThisFrame.size()) &&
           this->keysPressedThisFrame[scancode];
}

bool Window::isMouseButtonActive(MouseButton button) {
    const SDL_MouseButtonFlags state = SDL_GetMouseState(nullptr, nullptr);
    return (state & SDL_BUTTON_MASK(static_cast<int>(button))) != 0;
}

bool Window::isMouseButtonPressed(MouseButton button) {
    const int index = static_cast<int>(button);
    return index >= 0 &&
           index <
               static_cast<int>(this->mouseButtonsPressedThisFrame.size()) &&
           this->mouseButtonsPressedThisFrame[static_cast<std::size_t>(index)];
}

void Window::startTextInput() {
    if (SDL_StartTextInput(this->windowRef)) {
        this->textInputActive = true;
    } else {
        this->textInputActive = SDL_TextInputActive(this->windowRef);
    }
}

void Window::stopTextInput() {
    if (SDL_StopTextInput(this->windowRef)) {
        this->textInputActive = false;
    } else {
        this->textInputActive = SDL_TextInputActive(this->windowRef);
    }
}

void Window::releaseMouse() {
    SDL_SetWindowRelativeMouseMode(this->windowRef, false);
    SDL_SetWindowMouseGrab(this->windowRef, false);
    SDL_ShowCursor();
}

void Window::captureMouse() {
    SDL_SetWindowMouseGrab(this->windowRef, true);
    SDL_SetWindowRelativeMouseMode(this->windowRef, true);
    SDL_HideCursor();
}

void Window::addRenderTarget(RenderTarget *target) {
    this->renderTargets.push_back(target);
}

void Window::renderLightsToShadowMaps(
    std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (this->currentScene == nullptr) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    this->shadowUpdateCooldown =
        std::max(0.0f, this->shadowUpdateCooldown - this->deltaTime);

    bool cameraMoved = false;
    if (this->camera != nullptr) {
        glm::vec3 currentPos = this->camera->position.toGlm();
        glm::vec3 currentDir = this->camera->getFrontVector().toGlm();
        if (!this->lastShadowCameraPosition.has_value() ||
            !this->lastShadowCameraDirection.has_value()) {
            cameraMoved = true;
        } else {
            glm::vec3 lastPos = this->lastShadowCameraPosition->toGlm();
            glm::vec3 lastDir = this->lastShadowCameraDirection->toGlm();
            if ((glm::length(currentPos - lastPos) > 0.25f) ||
                glm::length(currentDir - lastDir) > 0.25f) {
                cameraMoved = true;
            }
        }
    }

    bool lightsChanged = false;
    const float positionThreshold = 0.1f;
    const float directionThreshold = 0.02f;

    const auto &directionalLights = this->currentScene->directionalLights;
    if (this->cachedDirectionalLightDirections.size() !=
        directionalLights.size()) {
        lightsChanged = true;
    } else {
        for (size_t i = 0; i < directionalLights.size(); ++i) {
            if (directionalLights.at(i) == nullptr) {
                if (glm::length(this->cachedDirectionalLightDirections.at(i)) >
                    directionThreshold) {
                    lightsChanged = true;
                    break;
                }
                continue;
            }
            glm::vec3 dir = directionalLights.at(i)->direction.toGlm();
            if (glm::length(dir - this->cachedDirectionalLightDirections.at(
                                      i)) > directionThreshold) {
                lightsChanged = true;
                break;
            }
        }
    }

    const auto &pointLights = this->currentScene->pointLights;
    if (!lightsChanged) {
        if (this->cachedPointLightPositions.size() != pointLights.size()) {
            lightsChanged = true;
        } else {
            for (size_t i = 0; i < pointLights.size(); ++i) {
                if (pointLights.at(i) == nullptr) {
                    if (glm::length(this->cachedPointLightPositions.at(i)) >
                        positionThreshold) {
                        lightsChanged = true;
                        break;
                    }
                    continue;
                }
                glm::vec3 pos = pointLights.at(i)->position.toGlm();
                if (glm::length(pos - this->cachedPointLightPositions.at(i)) >
                    positionThreshold) {
                    lightsChanged = true;
                    break;
                }
            }
        }
    }

    const auto &spotLights = this->currentScene->spotlights;
    if (!lightsChanged) {
        if (this->cachedSpotlightPositions.size() != spotLights.size() ||
            this->cachedSpotlightDirections.size() != spotLights.size()) {
            lightsChanged = true;
        } else {
            for (size_t i = 0; i < spotLights.size(); ++i) {
                if (spotLights.at(i) == nullptr) {
                    bool cachedPosNonZero =
                        glm::length(this->cachedSpotlightPositions.at(i)) >
                        positionThreshold;
                    bool cachedDirNonZero =
                        glm::length(this->cachedSpotlightDirections.at(i)) >
                        directionThreshold;
                    if (cachedPosNonZero || cachedDirNonZero) {
                        lightsChanged = true;
                        break;
                    }
                    continue;
                }
                glm::vec3 pos = spotLights.at(i)->position.toGlm();
                glm::vec3 dir = spotLights.at(i)->direction.toGlm();
                if (glm::length(pos - this->cachedSpotlightPositions.at(i)) >
                        positionThreshold ||
                    glm::length(dir - this->cachedSpotlightDirections.at(i)) >
                        directionThreshold) {
                    lightsChanged = true;
                    break;
                }
            }
        }
    }

    const auto &areaLights = this->currentScene->areaLights;
    if (!lightsChanged) {
        if (this->cachedAreaLightPositions.size() != areaLights.size() ||
            this->cachedAreaLightNormals.size() != areaLights.size() ||
            this->cachedAreaLightProperties.size() != areaLights.size()) {
            lightsChanged = true;
        } else {
            for (size_t i = 0; i < areaLights.size(); ++i) {
                if (areaLights.at(i) == nullptr) {
                    bool cachedPosNonZero =
                        glm::length(this->cachedAreaLightPositions.at(i)) >
                        positionThreshold;
                    bool cachedNormalNonZero =
                        glm::length(this->cachedAreaLightNormals.at(i)) >
                        directionThreshold;
                    bool cachedPropsNonZero =
                        glm::length(this->cachedAreaLightProperties.at(i)) >
                        0.01f;
                    if (cachedPosNonZero || cachedNormalNonZero ||
                        cachedPropsNonZero) {
                        lightsChanged = true;
                        break;
                    }
                    continue;
                }

                glm::vec3 pos = areaLights.at(i)->position.toGlm();
                glm::vec3 normal = areaLights.at(i)->getNormal().toGlm();
                if (glm::length(normal) < 1e-6f) {
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                } else {
                    normal = glm::normalize(normal);
                }
                glm::vec4 props(
                    static_cast<float>(areaLights.at(i)->size.width),
                    static_cast<float>(areaLights.at(i)->size.height),
                    areaLights.at(i)->range, areaLights.at(i)->angle);
                if (glm::length(pos - this->cachedAreaLightPositions.at(i)) >
                        positionThreshold ||
                    glm::length(normal - this->cachedAreaLightNormals.at(i)) >
                        directionThreshold ||
                    glm::length(props - this->cachedAreaLightProperties.at(i)) >
                        0.01f) {
                    lightsChanged = true;
                    break;
                }
            }
        }
    }

    const std::vector<Renderable *> shadowCasters =
        collectShadowCasters(this->firstRenderables, this->renderables,
                             this->lateForwardRenderables);
    const std::size_t shadowCasterSignature =
        computeShadowCasterSignature(shadowCasters);
    const bool castersMoved =
        !this->hasShadowCasterSignature ||
        this->lastShadowCasterSignature != shadowCasterSignature;

    if (cameraMoved || lightsChanged || castersMoved) {
        this->shadowMapsDirty = true;
    }

    if (!this->shadowMapsDirty) {
        return;
    }

    if (this->shadowUpdateCooldown > 0.0f && !cameraMoved && !lightsChanged &&
        !castersMoved) {
        return;
    }

    this->shadowMapsDirty = false;
    this->shadowUpdateCooldown = this->shadowUpdateInterval;

    bool renderedShadows = false;

    std::vector<std::shared_ptr<opal::Pipeline>> originalPipelines;
    originalPipelines.reserve(shadowCasters.size());
    for (auto *obj : shadowCasters) {
        if (obj->getPipeline() != std::nullopt) {
            originalPipelines.push_back(obj->getPipeline().value());
        } else {
            originalPipelines.push_back(opal::Pipeline::create());
        }
    }

    std::shared_ptr<opal::Pipeline> depthPipeline = opal::Pipeline::create();

    for (auto &light : this->currentScene->directionalLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        if (shadowRenderTarget == nullptr ||
            shadowRenderTarget->getFramebuffer() == nullptr) {
            continue;
        }
        renderedShadows = true;

        depthPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        depthPipeline->setCullMode(opal::CullMode::None);
        depthPipeline->setFrontFace(this->frontFace);
        depthPipeline->enableDepthTest(true);
        depthPipeline->setDepthCompareOp(opal::CompareOp::Less);
        depthPipeline->enablePolygonOffset(true);
        depthPipeline->setPolygonOffset(1.0f, 1.0f);

        depthPipeline = this->depthProgram.requestPipeline(depthPipeline);

        // Set up render pass for shadow framebuffer
        auto shadowRenderPass = opal::RenderPass::create();
        shadowRenderPass->setFramebuffer(shadowRenderTarget->getFramebuffer());
        commandBuffer->beginPass(shadowRenderPass);

        shadowRenderTarget->bind();
        commandBuffer->clearDepth(1.0f);
        ShadowParams lightParams =
            light->calculateLightSpaceMatrix(shadowCasters);
        glm::mat4 lightView = lightParams.lightView;
        glm::mat4 lightProjection = lightParams.lightProjection;
        light->lastShadowParams = lightParams;
        for (auto *obj : shadowCasters) {
            if (!obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(depthPipeline);

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }

        commandBuffer->endPass();
    }

    std::shared_ptr<opal::Pipeline> spotlightsPipeline =
        opal::Pipeline::create();

    for (auto &light : this->currentScene->spotlights) {
        if (!light->doesCastShadows) {
            continue;
        }
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        if (shadowRenderTarget == nullptr ||
            shadowRenderTarget->getFramebuffer() == nullptr) {
            continue;
        }
        renderedShadows = true;
        spotlightsPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        spotlightsPipeline->setCullMode(opal::CullMode::None);
        spotlightsPipeline->setFrontFace(this->frontFace);
        spotlightsPipeline->enableDepthTest(true);
        spotlightsPipeline->setDepthCompareOp(opal::CompareOp::Less);
        spotlightsPipeline->enablePolygonOffset(true);
        spotlightsPipeline->setPolygonOffset(1.0f, 1.0f);
        spotlightsPipeline =
            this->depthProgram.requestPipeline(spotlightsPipeline);

        // Set up render pass for shadow framebuffer
        auto shadowRenderPass = opal::RenderPass::create();
        shadowRenderPass->setFramebuffer(shadowRenderTarget->getFramebuffer());
        commandBuffer->beginPass(shadowRenderPass);

        shadowRenderTarget->bind();
        commandBuffer->clearDepth(1.0f);
        std::tuple<glm::mat4, glm::mat4> lightSpace =
            light->calculateLightSpaceMatrix();
        glm::mat4 lightView = std::get<0>(lightSpace);
        glm::mat4 lightProjection = std::get<1>(lightSpace);
        ShadowParams cached;
        cached.lightView = lightView;
        cached.lightProjection = lightProjection;
        cached.bias = 0.001f;
        light->lastShadowParams = cached;
        for (auto *obj : shadowCasters) {
            if (!obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(spotlightsPipeline);

            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }

        commandBuffer->endPass();
    }

    std::shared_ptr<opal::Pipeline> areaLightsPipeline =
        opal::Pipeline::create();

    for (auto &light : this->currentScene->areaLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        if (shadowRenderTarget == nullptr ||
            shadowRenderTarget->getFramebuffer() == nullptr) {
            continue;
        }
        renderedShadows = true;
        areaLightsPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        areaLightsPipeline->setCullMode(opal::CullMode::Front);
        areaLightsPipeline->setFrontFace(this->frontFace);
        areaLightsPipeline->enableDepthTest(true);
        areaLightsPipeline->setDepthCompareOp(opal::CompareOp::Less);
        areaLightsPipeline->enablePolygonOffset(true);
        areaLightsPipeline->setPolygonOffset(2.0f, 4.0f);
        areaLightsPipeline =
            this->depthProgram.requestPipeline(areaLightsPipeline);

        auto shadowRenderPass = opal::RenderPass::create();
        shadowRenderPass->setFramebuffer(shadowRenderTarget->getFramebuffer());
        commandBuffer->beginPass(shadowRenderPass);

        shadowRenderTarget->bind();
        commandBuffer->clearDepth(1.0f);
        ShadowParams lightParams = light->calculateLightSpaceMatrix();
        glm::mat4 lightView = lightParams.lightView;
        glm::mat4 lightProjection = lightParams.lightProjection;
        light->lastShadowParams = lightParams;

        for (auto *obj : shadowCasters) {
            if (!obj->canCastShadows()) {
                continue;
            }

            obj->setPipeline(areaLightsPipeline);
            obj->setProjectionMatrix(lightProjection);
            obj->setViewMatrix(lightView);
            obj->render(getDeltaTime(), commandBuffer, false);
        }

        commandBuffer->endPass();
    }

    std::shared_ptr<opal::Pipeline> pointLightPipeline =
        opal::Pipeline::create();

    for (auto &light : this->currentScene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        RenderTarget *shadowRenderTarget = light->shadowRenderTarget;
        if (shadowRenderTarget == nullptr ||
            shadowRenderTarget->getFramebuffer() == nullptr) {
            continue;
        }
        renderedShadows = true;
        pointLightPipeline->setViewport(
            0, 0, shadowRenderTarget->texture.creationData.width,
            shadowRenderTarget->texture.creationData.height);
        pointLightPipeline->setCullMode(opal::CullMode::None);
        pointLightPipeline->setFrontFace(this->frontFace);
        pointLightPipeline->enableDepthTest(true);
        pointLightPipeline->setDepthCompareOp(opal::CompareOp::Less);
        pointLightPipeline->enablePolygonOffset(true);
        pointLightPipeline->setPolygonOffset(1.0f, 1.0f);
        pointLightPipeline =
            this->pointDepthProgram.requestPipeline(pointLightPipeline);

        std::vector<glm::mat4> shadowTransforms =
            light->calculateShadowTransforms();

        pointLightPipeline->setUniform3f("lightPos", light->position.x,
                                         light->position.y, light->position.z);
        pointLightPipeline->setUniform1f("far_plane", light->distance);
        light->lastShadowParams.farPlane = light->distance;

        if (this->useMultiPassPointShadows) {
            // Multi-pass rendering: render 6 times, once per cubemap face
            for (int face = 0; face < 6; ++face) {
                shadowRenderTarget->bindCubemapFace(face);

                // Set up render pass for this cubemap face
                auto shadowRenderPass = opal::RenderPass::create();
                shadowRenderPass->setFramebuffer(
                    shadowRenderTarget->getFramebuffer());
                commandBuffer->beginPass(shadowRenderPass);

                commandBuffer->clearDepth(1.0f);

                // Set the shadow matrix for this face
                pointLightPipeline->setUniformMat4f("shadowMatrix",
                                                    shadowTransforms.at(face));
                pointLightPipeline->setUniform1i("faceIndex", face);

                for (auto *obj : shadowCasters) {
                    if (!obj->canCastShadows()) {
                        continue;
                    }

                    obj->setProjectionMatrix(glm::mat4(1.0));
                    obj->setViewMatrix(glm::mat4(1.0));
                    obj->setPipeline(pointLightPipeline);
                    obj->render(getDeltaTime(), commandBuffer, false);
                }

                commandBuffer->endPass();
            }
        } else {
            // Single-pass rendering with geometry shader
            auto shadowRenderPass = opal::RenderPass::create();
            shadowRenderPass->setFramebuffer(
                shadowRenderTarget->getFramebuffer());
            commandBuffer->beginPass(shadowRenderPass);

            shadowRenderTarget->bind();
            commandBuffer->clearDepth(1.0f);

            for (size_t i = 0; i < shadowTransforms.size(); ++i) {
                pointLightPipeline->setUniformMat4f("shadowMatrices[" +
                                                        std::to_string(i) + "]",
                                                    shadowTransforms.at(i));
            }

            for (auto *obj : shadowCasters) {
                if (!obj->canCastShadows()) {
                    continue;
                }

                obj->setProjectionMatrix(glm::mat4(1.0));
                obj->setViewMatrix(glm::mat4(1.0));
                obj->setPipeline(pointLightPipeline);
                obj->render(getDeltaTime(), commandBuffer, false);
            }

            commandBuffer->endPass();
        }
    }

    if (this->camera != nullptr) {
        this->lastShadowCameraPosition = this->camera->position;
        this->lastShadowCameraDirection = this->camera->getFrontVector();
    }

    this->cachedDirectionalLightDirections.clear();
    this->cachedDirectionalLightDirections.reserve(directionalLights.size());
    for (auto *light : directionalLights) {
        if (light == nullptr) {
            this->cachedDirectionalLightDirections.emplace_back(0.0f, 0.0f,
                                                                0.0f);
            continue;
        }
        this->cachedDirectionalLightDirections.push_back(
            light->direction.toGlm());
    }

    this->cachedPointLightPositions.clear();
    this->cachedPointLightPositions.reserve(pointLights.size());
    for (auto *light : pointLights) {
        if (light == nullptr) {
            this->cachedPointLightPositions.emplace_back(0.0f, 0.0f, 0.0f);
            continue;
        }
        this->cachedPointLightPositions.push_back(light->position.toGlm());
    }

    this->cachedSpotlightPositions.clear();
    this->cachedSpotlightDirections.clear();
    this->cachedSpotlightPositions.reserve(spotLights.size());
    this->cachedSpotlightDirections.reserve(spotLights.size());
    for (auto *light : spotLights) {
        if (light == nullptr) {
            this->cachedSpotlightPositions.emplace_back(0.0f, 0.0f, 0.0f);
            this->cachedSpotlightDirections.emplace_back(0.0f, 0.0f, 0.0f);
            continue;
        }
        this->cachedSpotlightPositions.push_back(light->position.toGlm());
        this->cachedSpotlightDirections.push_back(light->direction.toGlm());
    }

    this->cachedAreaLightPositions.clear();
    this->cachedAreaLightNormals.clear();
    this->cachedAreaLightProperties.clear();
    this->cachedAreaLightPositions.reserve(areaLights.size());
    this->cachedAreaLightNormals.reserve(areaLights.size());
    this->cachedAreaLightProperties.reserve(areaLights.size());
    for (auto *light : areaLights) {
        if (light == nullptr) {
            this->cachedAreaLightPositions.emplace_back(0.0f, 0.0f, 0.0f);
            this->cachedAreaLightNormals.emplace_back(0.0f, 0.0f, 0.0f);
            this->cachedAreaLightProperties.emplace_back(0.0f, 0.0f, 0.0f,
                                                         0.0f);
            continue;
        }
        glm::vec3 normal = light->getNormal().toGlm();
        if (glm::length(normal) < 1e-6f) {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            normal = glm::normalize(normal);
        }
        this->cachedAreaLightPositions.push_back(light->position.toGlm());
        this->cachedAreaLightNormals.push_back(normal);
        this->cachedAreaLightProperties.emplace_back(
            static_cast<float>(light->size.width),
            static_cast<float>(light->size.height), light->range, light->angle);
    }

    this->lastShadowCasterSignature = shadowCasterSignature;
    this->hasShadowCasterSignature = true;

    // Polygon offset is controlled per-pipeline, no need to disable globally
    if (!renderedShadows) {
        return;
    }

    for (size_t i = 0; i < shadowCasters.size() && i < originalPipelines.size();
         ++i) {
        shadowCasters[i]->setPipeline(originalPipelines[i]);
    }
}

void Window::renderPingpong(RenderTarget *target) {
    if (target->brightTexture.id == 0) {
        return;
    }

    if (!target->object || !target->object->isVisible) {
        return;
    }

    const int blurDownscaleFactor = 2;
    int blurWidth = std::max(1, target->brightTexture.creationData.width /
                                    blurDownscaleFactor);
    int blurHeight = std::max(1, target->brightTexture.creationData.height /
                                     blurDownscaleFactor);

    if (this->pingpongFramebuffers.at(0) == nullptr ||
        this->pingpongFramebuffers.at(1) == nullptr ||
        blurWidth != this->pingpongWidth ||
        blurHeight != this->pingpongHeight) {
        this->pingpongFramebuffers.at(0) = nullptr;
        this->pingpongFramebuffers.at(1) = nullptr;
        this->pingpongTextures.at(0) = nullptr;
        this->pingpongTextures.at(1) = nullptr;

        this->pingpongWidth = blurWidth;
        this->pingpongHeight = blurHeight;

        for (unsigned int i = 0; i < 2; i++) {
            auto texture = opal::Texture::create(
                opal::TextureType::Texture2D, opal::TextureFormat::Rgba16F,
                blurWidth, blurHeight, opal::TextureDataFormat::Rgba, nullptr,
                1);
            texture->setFilterMode(opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);
            texture->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
            texture->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

            auto framebuffer = opal::Framebuffer::create(blurWidth, blurHeight);
            opal::Attachment colorAttachment;
            colorAttachment.texture = texture;
            colorAttachment.type = opal::Attachment::Type::Color;
            framebuffer->addAttachment(colorAttachment);

            if (!framebuffer->getStatus()) {
                std::cerr << "Pingpong Framebuffer not complete!" << std::endl;
            }

            this->pingpongFramebuffers.at(i) = framebuffer;
            this->pingpongTextures.at(i) = texture;
        }
    }

    device->getDefaultFramebuffer()->bind();
    device->frameCount++;

    bool horizontal = true;
    bool firstIteration = true;
    const unsigned int blurIterations = std::max(1u, this->bloomBlurPasses);

    auto originalPipeline = target->object->getPipeline();
    if (!originalPipeline.has_value()) {
        return;
    }

    auto targetProgram = originalPipeline.value();

    auto blurPipeline = opal::Pipeline::create();

    blurPipeline->setViewport(0, 0, blurWidth, blurHeight);
    blurPipeline->enableDepthTest(false);
    blurPipeline->enableBlending(false);

    blurPipeline = this->bloomBlurProgram.requestPipeline(blurPipeline);

    target->setPipeline(blurPipeline);

    blurPipeline->bind();
    blurPipeline->setUniform1f("radius", 2.5f);
    blurPipeline->setUniform1i("image", 0);

    target->object->vao->bind();
    target->object->ebo->bind();

    for (unsigned int i = 0; i < blurIterations; ++i) {
        this->pingpongFramebuffers.at(horizontal)->bind();
        activeCommandBuffer->clearColor(0.0f, 0.0f, 0.0f, 1.0f);

        blurPipeline->setUniform1i("horizontal", horizontal ? 1 : 0);

        blurPipeline->bindTexture2D(
            "image",
            firstIteration ? target->brightTexture.id
                           : this->pingpongTextures.at(!horizontal)->textureID,
            0);

        activeCommandBuffer->bindPipeline(blurPipeline);
        if (!target->object->indices.empty()) {
            activeCommandBuffer->drawIndexed(
                static_cast<uint>(target->object->indices.size()));
        } else {
            activeCommandBuffer->draw(
                static_cast<uint>(target->object->vertices.size()));
        }

        horizontal = !horizontal;
        firstIteration = false;
    }

    target->object->vao->unbind();

    target->object->setPipeline(targetProgram);
    device->getDefaultFramebuffer()->bind();

    updatePipelineStateField(this->useDepth, true);

    int fbWidth, fbHeight;
    this->queryDrawableSizeInPixels(&fbWidth, &fbHeight);
    setViewportState(0, 0, fbWidth, fbHeight);

    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width = this->pingpongWidth;
    target->blurredTexture.creationData.height = this->pingpongHeight;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id =
        this->pingpongTextures.at(!horizontal)->textureID;
    target->blurredTexture.texture = this->pingpongTextures.at(!horizontal);
}

void Window::useDeferredRendering() {
    atlas_log("Enabling deferred rendering");
    this->usePathTracing = false;
    this->usesDeferred = true;
    auto target = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::GBuffer));
    this->gBuffer = target;
    auto volumetricTarget = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::Scene));
    this->volumetricBuffer = volumetricTarget;
    auto ssrTarget = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::Scene));
    this->ssrFramebuffer = ssrTarget;
    this->ssaoMapsDirty = true;
}

void Window::renderPhysicalBloom(RenderTarget *target) {
    if (target == nullptr || target->brightTexture.id == 0 ||
        this->currentScene == nullptr ||
        this->currentScene->environment.lightBloom.radius <= 0.0f ||
        this->currentScene->environment.lightBloom.maxSamples <= 0) {
        return;
    }

    int sizeX = std::max(1, target->brightTexture.creationData.width);
    int sizeY = std::max(1, target->brightTexture.creationData.height);
    int chainLength =
        std::max(1, currentScene->environment.lightBloom.maxSamples);

    if (this->bloomBuffer == nullptr) {
        this->bloomBuffer =
            std::make_shared<BloomRenderTarget>(BloomRenderTarget());
    }

    bool needsRebuild =
        !this->bloomBuffer->initialized ||
        this->bloomBuffer->srcViewportSize.x != sizeX ||
        this->bloomBuffer->srcViewportSize.y != sizeY ||
        std::cmp_not_equal(this->bloomBuffer->elements.size(), chainLength);
    if (needsRebuild) {
        this->bloomBuffer->destroy();
        this->bloomBuffer->init(sizeX, sizeY, chainLength);
    }

    if (!this->bloomBuffer->initialized ||
        this->bloomBuffer->elements.empty()) {
        target->blurredTexture = target->brightTexture;
        return;
    }

    float filterRadius = currentScene->environment.lightBloom.radius *
                         static_cast<float>(std::min(sizeX, sizeY)) * 0.15f;
    filterRadius = std::clamp(filterRadius, 0.5f, 2.0f);
    this->bloomBuffer->renderBloomTexture(
        target->brightTexture.id, filterRadius, this->activeCommandBuffer);
    target->blurredTexture = Texture();
    target->blurredTexture.creationData.width =
        this->bloomBuffer->srcViewportSize.x;
    target->blurredTexture.creationData.height =
        this->bloomBuffer->srcViewportSize.y;
    target->blurredTexture.type = TextureType::Color;
    target->blurredTexture.id = this->bloomBuffer->getBloomTexture();
    target->blurredTexture.texture = this->bloomBuffer->elements.at(0).texture;
}

void Window::updateFluidCaptures(
    std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }
    for (auto *fluid : lateFluids) {
        if (fluid == nullptr) {
            continue;
        }
        if (fluid->captureDirty) {
            fluid->updateCapture(*this, commandBuffer);
        }
    }
}

void Window::captureFluidReflection(
    Fluid &fluid, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (!fluid.reflectionTarget) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    RenderTarget &target = *fluid.reflectionTarget;

    Camera *originalCamera = this->camera;
    Camera reflectionCamera = *originalCamera;

    glm::vec3 planePoint = fluid.calculatePlanePoint();
    glm::vec3 planeNormal = fluid.calculatePlaneNormal();

    glm::vec3 cameraPos = originalCamera->position.toGlm();
    float distance = glm::dot(planeNormal, cameraPos - planePoint);
    glm::vec3 reflectedPos = cameraPos - 2.0f * distance * planeNormal;

    glm::vec3 front = originalCamera->getFrontVector().toGlm();
    glm::vec3 reflectedDir =
        front - 2.0f * glm::dot(front, planeNormal) * planeNormal;
    glm::vec3 reflectedTarget = reflectedPos + reflectedDir;

    reflectionCamera.setPosition(Position3d::fromGlm(reflectedPos));
    reflectionCamera.lookAt(Position3d::fromGlm(reflectedTarget));
    reflectionCamera.fov = originalCamera->fov * 1.2f;
    reflectionCamera.nearClip = originalCamera->nearClip;
    reflectionCamera.farClip = originalCamera->farClip;
    reflectionCamera.useOrthographic = originalCamera->useOrthographic;
    reflectionCamera.orthographicSize = originalCamera->orthographicSize;

    Camera *cameraBackup = this->camera;
    this->camera = originalCamera;

    glm::vec4 plane = fluid.calculateClipPlane();
    bool clipBackup = clipPlaneEnabled;
    glm::vec4 clipEquationBackup = clipPlaneEquation;
    clipPlaneEnabled = true;
    clipPlaneEquation = plane;

    RenderTarget *previousTarget = currentRenderTarget;
    const int previousViewportX = this->viewportX;
    const int previousViewportY = this->viewportY;
    const int previousViewportWidth = this->viewportWidth;
    const int previousViewportHeight = this->viewportHeight;

    target.bind();
    auto pipeline = opal::Pipeline::create();
    pipeline->enableClipDistance(0, true);

    target.getFramebuffer()->setViewport(0, 0, target.getWidth(),
                                         target.getHeight());
    setViewportState(0, 0, target.getWidth(), target.getHeight());
    target.getFramebuffer()->setDrawBuffers(2);

    const bool previousUseDepth = this->useDepth;
    const bool previousWriteDepth = this->writeDepth;
    const bool previousUseBlending = this->useBlending;
    const opal::CullMode previousCullMode = this->cullMode;
    const opal::CompareOp previousDepthCompare = this->depthCompareOp;

    pipeline->enableBlending(false);
    pipeline->enableDepthTest(true);
    pipeline->enableDepthWrite(true);
    pipeline->setCullMode(opal::CullMode::Front);
    pipeline->setDepthCompareOp(opal::CompareOp::Less);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, false);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->writeDepth, true);
    updatePipelineStateField(this->cullMode, opal::CullMode::Front);
    updatePipelineStateField(this->depthCompareOp, opal::CompareOp::Less);

    commandBuffer->clear(fluid.color.r, fluid.color.g, fluid.color.b, 1.0f,
                         1.0f);

    currentRenderTarget = &target;

    glm::mat4 view = this->camera->calculateViewMatrix();
    glm::mat4 projection = calculateProjectionMatrix();

    auto renderQueue = [&](const std::vector<Renderable *> &queue,
                           bool skipLate) {
        for (auto *obj : queue) {
            if (obj == nullptr) {
                continue;
            }
            if (skipLate && obj->renderLateForward) {
                continue;
            }
            if (dynamic_cast<Fluid *>(obj) == &fluid) {
                continue;
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    if (previousTarget && previousTarget->getFramebuffer()) {
        previousTarget->bind();
        previousTarget->getFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    } else {
        device->getDefaultFramebuffer()->bind();
        device->getDefaultFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    }
    setViewportState(previousViewportX, previousViewportY,
                     previousViewportWidth, previousViewportHeight);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    pipeline->enableClipDistance(0, clipBackup);

    pipeline->enableBlending(previousUseBlending);
    pipeline->enableDepthTest(previousUseDepth);
    pipeline->enableDepthWrite(previousWriteDepth);
    pipeline->setCullMode(previousCullMode);
    pipeline->setDepthCompareOp(previousDepthCompare);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, previousUseBlending);
    updatePipelineStateField(this->useDepth, previousUseDepth);
    updatePipelineStateField(this->writeDepth, previousWriteDepth);
    updatePipelineStateField(this->cullMode, previousCullMode);
    updatePipelineStateField(this->depthCompareOp, previousDepthCompare);

    this->camera = cameraBackup;
}

void Window::captureFluidRefraction(
    Fluid &fluid, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (!fluid.refractionTarget) {
        return;
    }

    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }

    RenderTarget &target = *fluid.refractionTarget;

    glm::vec3 planePoint = fluid.calculatePlanePoint();
    glm::vec3 planeNormal = fluid.calculatePlaneNormal();

    const float clipBias = 0.02f;
    glm::vec4 plane =
        glm::vec4(planeNormal, -glm::dot(planeNormal, planePoint) - clipBias);

    bool clipBackup = clipPlaneEnabled;
    glm::vec4 clipEquationBackup = clipPlaneEquation;
    clipPlaneEnabled = true;
    clipPlaneEquation = plane;

    RenderTarget *previousTarget = currentRenderTarget;
    const int previousViewportX = this->viewportX;
    const int previousViewportY = this->viewportY;
    const int previousViewportWidth = this->viewportWidth;
    const int previousViewportHeight = this->viewportHeight;

    target.bind();
    auto pipeline = opal::Pipeline::create();
    pipeline->enableClipDistance(0, true);

    target.getFramebuffer()->setViewport(0, 0, target.getWidth(),
                                         target.getHeight());
    setViewportState(0, 0, target.getWidth(), target.getHeight());
    target.getFramebuffer()->setDrawBuffers(2);

    const bool previousUseDepth = this->useDepth;
    const bool previousWriteDepth = this->writeDepth;
    const bool previousUseBlending = this->useBlending;
    const opal::CullMode previousCullMode = this->cullMode;
    const opal::CompareOp previousDepthCompare = this->depthCompareOp;

    pipeline->enableBlending(false);
    pipeline->enableDepthTest(true);
    pipeline->enableDepthWrite(true);
    pipeline->setCullMode(opal::CullMode::Back);
    pipeline->setDepthCompareOp(opal::CompareOp::Less);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, false);
    updatePipelineStateField(this->useDepth, true);
    updatePipelineStateField(this->writeDepth, true);
    updatePipelineStateField(this->cullMode, opal::CullMode::Back);
    updatePipelineStateField(this->depthCompareOp, opal::CompareOp::Less);

    commandBuffer->clear(this->clearColor.r, this->clearColor.g,
                         this->clearColor.b, this->clearColor.a, 1.0f);

    currentRenderTarget = &target;

    glm::mat4 view = this->camera->calculateViewMatrix();
    glm::mat4 projection = calculateProjectionMatrix();

    auto renderQueue = [&](const std::vector<Renderable *> &queue,
                           bool skipLate) {
        for (auto *obj : queue) {
            if (obj == nullptr) {
                continue;
            }
            if (skipLate && obj->renderLateForward) {
                continue;
            }
            if (dynamic_cast<Fluid *>(obj) == &fluid) {
                continue;
            }
            obj->setViewMatrix(view);
            obj->setProjectionMatrix(projection);
            obj->render(getDeltaTime(), commandBuffer,
                        shouldRefreshPipeline(obj));
        }
    };

    renderQueue(firstRenderables, false);
    renderQueue(renderables, true);

    if (previousTarget && previousTarget->getFramebuffer()) {
        previousTarget->bind();
        previousTarget->getFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    } else {
        device->getDefaultFramebuffer()->bind();
        device->getDefaultFramebuffer()->setViewport(
            previousViewportX, previousViewportY, previousViewportWidth,
            previousViewportHeight);
    }
    setViewportState(previousViewportX, previousViewportY,
                     previousViewportWidth, previousViewportHeight);
    currentRenderTarget = previousTarget;

    clipPlaneEnabled = clipBackup;
    clipPlaneEquation = clipEquationBackup;
    pipeline->enableClipDistance(0, clipBackup);

    pipeline->enableBlending(previousUseBlending);
    pipeline->enableDepthTest(previousUseDepth);
    pipeline->enableDepthWrite(previousWriteDepth);
    pipeline->setCullMode(previousCullMode);
    pipeline->setDepthCompareOp(previousDepthCompare);
    pipeline->bind();

    updatePipelineStateField(this->useBlending, previousUseBlending);
    updatePipelineStateField(this->useDepth, previousUseDepth);
    updatePipelineStateField(this->writeDepth, previousWriteDepth);
    updatePipelineStateField(this->cullMode, previousCullMode);
    updatePipelineStateField(this->depthCompareOp, previousDepthCompare);
}

void Window::markPipelineStateDirty() { ++pipelineStateVersion; }

bool Window::shouldRefreshPipeline(Renderable *renderable) {
    if (renderable == nullptr) {
        return false;
    }
    auto &version = renderablePipelineVersions[renderable];
    if (version != pipelineStateVersion) {
        version = pipelineStateVersion;
        return true;
    }
    return false;
}

void Window::setViewportState(int x, int y, int newViewportWidth,
                              int newViewportHeight) {
    updatePipelineStateField(this->viewportX, x);
    updatePipelineStateField(this->viewportY, y);
    updatePipelineStateField(this->viewportWidth, newViewportWidth);
    updatePipelineStateField(this->viewportHeight, newViewportHeight);
}

void Window::updateBackbufferTarget(int backbufferWidth, int backbufferHeight) {
    if (!this->screenRenderTarget) {
        this->screenRenderTarget = std::make_unique<RenderTarget>();
        this->screenRenderTarget->type = RenderTargetType::Scene;
    }

    RenderTarget &target = *this->screenRenderTarget;
    target.texture.creationData.width = backbufferWidth;
    target.texture.creationData.height = backbufferHeight;
    target.depthTexture.creationData.width = backbufferWidth;
    target.depthTexture.creationData.height = backbufferHeight;
    target.type = RenderTargetType::Scene;
}

BoundingBox Window::getSceneBoundingBox() {
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());
    bool any = false;

    for (auto *obj : renderables) {
        if (!obj)
            continue;

        const auto &vertices = obj->getVertices();
        if (vertices.empty())
            continue;

        glm::mat4 model(1.0f);

        if (const auto *coreObj = dynamic_cast<const CoreObject *>(obj)) {
            model = glm::translate(model, coreObj->getPosition().toGlm());
            model *= glm::mat4_cast(
                glm::normalize(coreObj->getRotation().toGlmQuat()));
            model = glm::scale(model, coreObj->getScale().toGlm());
        } else {
            model = glm::translate(model, obj->getPosition().toGlm());
            model = glm::scale(model, obj->getScale().toGlm());
        }

        for (const auto &v : vertices) {
            glm::vec3 p =
                glm::vec3(model * glm::vec4(v.position.toGlm(), 1.0f));
            worldMin = glm::min(worldMin, p);
            worldMax = glm::max(worldMax, p);
            any = true;
        }
    }

    if (!any)
        return {};

    return {Position3d::fromGlm(worldMin), Position3d::fromGlm(worldMax)};
}

bool Window::isActionTriggered(const std::string &actionName) {
    auto inputAction = getInputAction(actionName);
    if (inputAction) {
        if (inputAction->isAxis) {
            atlas_warning("Tried to get a boolean value for an action with an "
                          "axis value assigned to it.");
            return false;
        } else {
            for (const auto &trigger : inputAction->buttonTriggers) {
                if (isTriggerPressed(trigger)) {
                    return true;
                }
            }
        }
    } else {
        atlas_warning(
            "Tried to get the state of an action that doesn't exist: " +
            actionName);
    }
    return false;
}

bool Window::isActionCurrentlyActive(const std::string &actionName) {
    auto inputAction = getInputAction(actionName);
    if (inputAction) {
        if (inputAction->isAxis) {
            atlas_warning("Tried to get a boolean value for an action with an "
                          "axis value assigned to it.");
            return false;
        } else {
            for (const auto &trigger : inputAction->buttonTriggers) {
                if (isTriggerActive(trigger)) {
                    return true;
                }
            }
        }
    } else {
        atlas_warning(
            "Tried to get the state of an action that doesn't exist: " +
            actionName);
    }
    return false;
}

AxisPacket Window::getAxisActionValue(const std::string &actionName) {
    AxisPacket packet;
    auto inputAction = getInputAction(actionName);
    if (inputAction) {
        if (!inputAction->isAxis) {
            atlas_warning("Tried to get an axis value for an action with a "
                          "boolean value assigned to it.");
            return {};
        } else {
            float previousX = inputAction->axisX;
            float previousY = inputAction->axisY;
            float accumulatedValueX = 0.0f;
            float accumulatedValueY = 0.0f;
            float accumulatedDeltaX = 0.0f;
            float accumulatedDeltaY = 0.0f;
            bool hasValueInput = false;
            bool hasDeltaInput = false;
            bool usesPointerPosition = false;

            auto applyDeadzone = [&](float value) {
                if (std::abs(value) < inputAction->controllerDeadzone) {
                    return 0.0f;
                }
                return value;
            };

            for (const auto &axisTrigger : inputAction->axisTriggers) {
                if (axisTrigger.type == AxisTriggerType::KeyCustom) {
                    if (inputAction->isAxisSingle) {
                        float val = 0.0f;

                        if (isTriggerActive(axisTrigger.positiveX)) {
                            val += 1.0f;
                        }
                        if (isTriggerActive(axisTrigger.negativeX)) {
                            val -= 1.0f;
                        }
                        accumulatedValueX += val;
                        hasValueInput = true;
                    } else {
                        float x = 0.0f;
                        float y = 0.0f;

                        if (isTriggerActive(axisTrigger.positiveX)) {
                            x += 1.0f;
                        }
                        if (isTriggerActive(axisTrigger.negativeX)) {
                            x -= 1.0f;
                        }
                        if (isTriggerActive(axisTrigger.positiveY)) {
                            y += 1.0f;
                        }
                        if (isTriggerActive(axisTrigger.negativeY)) {
                            y -= 1.0f;
                        }

                        accumulatedValueX += x;
                        accumulatedValueY += y;
                        hasValueInput = true;
                    }
                } else if (axisTrigger.type == AxisTriggerType::MouseAxis) {
                    accumulatedDeltaX += this->relativeMousePos.x;
                    accumulatedDeltaY += this->relativeMousePos.y;
                    hasDeltaInput = true;
                    usesPointerPosition = true;
                } else if (axisTrigger.type ==
                               AxisTriggerType::ControllerAxis &&
                           axisTrigger.controllerAxisSingle) {
                    float val = getControllerAxisValue(axisTrigger.controllerID,
                                                       axisTrigger.axisIndex);
                    val = applyDeadzone(val);
                    if (inputAction->invertControllerY &&
                        atlasIsControllerYAxis(axisTrigger.axisIndex)) {
                        val = -val;
                    }
                    accumulatedValueX += val;
                    hasValueInput = true;
                } else if (axisTrigger.type ==
                           AxisTriggerType::ControllerAxis) {
                    auto pair = this->getControllerAxisPairValue(
                        axisTrigger.controllerID, axisTrigger.axisIndex,
                        axisTrigger.axisIndexY);
                    float x = applyDeadzone(pair.first);
                    float y = applyDeadzone(pair.second);
                    if (inputAction->invertControllerY) {
                        y = -y;
                    }
                    accumulatedValueX += x;
                    accumulatedValueY += y;
                    hasValueInput = true;
                }
            }

            if (hasValueInput) {
                float axisX = accumulatedValueX * inputAction->axisScaleX;
                float axisY = accumulatedValueY * inputAction->axisScaleY;

                if (inputAction->isAxisSingle) {
                    axisY = axisX;
                }

                if (inputAction->normalize2D && !inputAction->isAxisSingle) {
                    float magnitude = std::sqrt(axisX * axisX + axisY * axisY);
                    if (magnitude > 1.0f) {
                        axisX /= magnitude;
                        axisY /= magnitude;
                    }
                }

                if (inputAction->clampAxis) {
                    axisX = std::clamp(axisX, inputAction->axisClampMin,
                                       inputAction->axisClampMax);
                    axisY = std::clamp(axisY, inputAction->axisClampMin,
                                       inputAction->axisClampMax);
                }

                inputAction->axisX = axisX;
                inputAction->axisY = axisY;
            } else if (hasDeltaInput) {
                inputAction->axisX = this->lastMouseX;
                inputAction->axisY = this->lastMouseY;
            } else {
                inputAction->axisX = 0.0f;
                inputAction->axisY = 0.0f;
            }

            if (hasDeltaInput && !hasValueInput) {
                inputAction->axisDeltaX = accumulatedDeltaX;
                inputAction->axisDeltaY = accumulatedDeltaY;
                if (!usesPointerPosition) {
                    inputAction->axisX = accumulatedDeltaX;
                    inputAction->axisY = accumulatedDeltaY;
                }
            } else {
                float valueDeltaX = inputAction->axisX - previousX;
                float valueDeltaY = inputAction->axisY - previousY;
                inputAction->axisDeltaX = valueDeltaX;
                inputAction->axisDeltaY = valueDeltaY;
            }

            packet.deltaX = inputAction->axisDeltaX;
            packet.deltaY = inputAction->axisDeltaY;
            packet.x = inputAction->axisX;
            packet.y = inputAction->axisY;
            packet.valueX = hasValueInput ? inputAction->axisX : 0.0f;
            packet.valueY = hasValueInput ? inputAction->axisY : 0.0f;
            packet.inputDeltaX = hasDeltaInput ? accumulatedDeltaX : 0.0f;
            packet.inputDeltaY = hasDeltaInput ? accumulatedDeltaY : 0.0f;
            packet.hasValueInput = hasValueInput;
            packet.hasDeltaInput = hasDeltaInput;
        }
    } else {
        atlas_warning("Tried to get an axis value for an action that doesn't "
                      "exist: " +
                      actionName);
        return {};
    }
    return packet;
}

bool Window::isTriggerActive(const Trigger &trigger) {
    if (trigger.type == TriggerType::Key) {
        return isKeyActive(trigger.key);
    } else if (trigger.type == TriggerType::MouseButton) {
        return isMouseButtonActive(trigger.mouseButton);
    } else if (trigger.type == TriggerType::ControllerButton) {
        return isControllerButtonPressed(trigger.controllerButton.controllerID,
                                         trigger.controllerButton.buttonIndex);
    } else {
        return false;
    }
}

bool Window::isTriggerPressed(const Trigger &trigger) {
    if (trigger.type == TriggerType::Key) {
        return isKeyPressed(trigger.key);
    } else if (trigger.type == TriggerType::MouseButton) {
        return isMouseButtonPressed(trigger.mouseButton);
    } else if (trigger.type == TriggerType::ControllerButton) {
        return isControllerButtonPressed(trigger.controllerButton.controllerID,
                                         trigger.controllerButton.buttonIndex);
    } else {
        return false;
    }
}

std::vector<ControllerID> Window::getControllers() const {
    std::vector<ControllerID> controllers;
    const auto joystickIds = getConnectedJoystickIds();
    controllers.reserve(joystickIds.size());
    for (SDL_JoystickID joystickID : joystickIds) {
        ControllerID id;
        id.id = joystickID;
        id.isJoystick = !SDL_IsGamepad(joystickID);
        if (id.isJoystick) {
            SDL_Joystick *joystick = getOpenJoystick(joystickID);
            const char *name =
                joystick != nullptr ? SDL_GetJoystickName(joystick) : nullptr;
            id.name = name != nullptr ? std::string(name) : std::string();
        } else {
            SDL_Gamepad *gamepad = getOpenGamepad(joystickID);
            const char *name =
                gamepad != nullptr ? SDL_GetGamepadName(gamepad) : nullptr;
            id.name = name != nullptr ? std::string(name) : std::string();
        }
        controllers.push_back(id);
    }
    return controllers;
}

Controller Window::getController(const ControllerID &id) const {
    if (id.isJoystick) {
        atlas_warning(
            "Tried to get a controller that is actually a joystick: " +
            id.name);
        Controller controller;
        controller.connected = false;
        return controller;
    }
    Controller controller;
    controller.controllerID = id.id;
    controller.name = id.name;
    controller.connected = getOpenGamepad(id.id) != nullptr;
    return controller;
}

Joystick Window::getJoystick(const ControllerID &id) const {
    if (!id.isJoystick) {
        atlas_warning(
            "Tried to get a joystick that is actually a controller: " +
            id.name);
        Joystick joystick;
        joystick.connected = false;
        return joystick;
    }
    Joystick joystick;
    joystick.joystickID = id.id;
    joystick.name = id.name;
    joystick.connected = getOpenJoystick(id.id) != nullptr;
    return joystick;
}

bool Window::isControllerButtonPressed(int controllerID, int buttonIndex) {
    if (controllerID == CONTROLLER_UNDEFINED) {
        for (SDL_JoystickID joystickID : getConnectedJoystickIds()) {
            if (isControllerButtonPressed(joystickID, buttonIndex)) {
                return true;
            }
        }
        return false;
    }

    if (!SDL_IsGamepad(controllerID)) {
        SDL_Joystick *joystick = getOpenJoystick(controllerID);
        if (joystick == nullptr) {
            return false;
        }
        const int buttonCount = SDL_GetNumJoystickButtons(joystick);
        if (buttonIndex >= 0 && buttonIndex < buttonCount) {
            return SDL_GetJoystickButton(joystick, buttonIndex);
        }
        return false;
    }

    SDL_Gamepad *gamepad = getOpenGamepad(controllerID);
    if (gamepad == nullptr) {
        return false;
    }
    const SDL_GamepadButton button = toSDLGamepadButton(buttonIndex);
    if (button == SDL_GAMEPAD_BUTTON_INVALID) {
        return false;
    }
    return SDL_GetGamepadButton(gamepad, button);
}

float Window::getControllerAxisValue(int controllerID, int axisIndex) {
    if (controllerID == CONTROLLER_UNDEFINED) {
        float selected = 0.0f;
        bool foundAny = false;
        for (SDL_JoystickID joystickID : getConnectedJoystickIds()) {
            float val = getControllerAxisValue(joystickID, axisIndex);
            if (!foundAny || std::abs(val) > std::abs(selected)) {
                selected = val;
                foundAny = true;
            }
        }
        return foundAny ? selected : 0.0f;
    }
    return getControllerAxisValueForDevice(controllerID, axisIndex);
}

std::pair<float, float> Window::getControllerAxisPairValue(int controllerID,
                                                           int axisIndexX,
                                                           int axisIndexY) {
    return getControllerAxisPairValueForDevice(controllerID, axisIndexX,
                                               axisIndexY);
}

#ifdef METAL
void Window::enablePathTracing() {
    this->usesDeferred = false;
    this->usePathTracing = true;
    this->pathTracer = std::make_shared<photon::PathTracing>();
    pathTracer->init();
}
#endif
