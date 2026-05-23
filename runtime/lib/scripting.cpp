//
// scripting.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Scripting definitions and code for running scripts
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/scripting.h"
#include "aurora/procedural.h"
#include "aurora/terrain.h"
#include "atlas/audio.h"
#include "atlas/effect.h"
#include "atlas/input.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/particle.h"
#include "atlas/physics.h"
#include "atlas/runtime/context.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "graphite/image.h"
#include "graphite/input.h"
#include "graphite/layout.h"
#include "graphite/style.h"
#include "graphite/text.h"
#include "hydra/atmosphere.h"
#include "hydra/fluid.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <memory>
#include <quickjs.h>
#include <string>
#include <utility>
#include <vector>

const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";
const std::string YELLOW = "\033[33m";

namespace {

constexpr const char *ATLAS_OBJECT_ID_PROP = "__atlasObjectId";
constexpr const char *ATLAS_COMPONENT_ID_PROP = "__atlasComponentId";
constexpr const char *ATLAS_AUDIO_PLAYER_ID_PROP = "__atlasAudioPlayerId";
constexpr const char *ATLAS_AUDIO_DATA_ID_PROP = "__atlasAudioDataId";
constexpr const char *ATLAS_AUDIO_SOURCE_ID_PROP = "__atlasAudioSourceId";
constexpr const char *ATLAS_REVERB_ID_PROP = "__atlasReverbId";
constexpr const char *ATLAS_ECHO_ID_PROP = "__atlasEchoId";
constexpr const char *ATLAS_DISTORTION_ID_PROP = "__atlasDistortionId";
constexpr const char *ATLAS_FONT_ID_PROP = "__atlasFontId";
constexpr const char *ATLAS_RIGIDBODY_ID_PROP = "__atlasRigidbodyId";
constexpr const char *ATLAS_VEHICLE_ID_PROP = "__atlasVehicleId";
constexpr const char *ATLAS_FIXED_JOINT_ID_PROP = "__atlasFixedJointId";
constexpr const char *ATLAS_HINGE_JOINT_ID_PROP = "__atlasHingeJointId";
constexpr const char *ATLAS_SPRING_JOINT_ID_PROP = "__atlasSpringJointId";
constexpr const char *ATLAS_INSTANCE_OWNER_ID_PROP = "__atlasInstanceOwnerId";
constexpr const char *ATLAS_INSTANCE_INDEX_PROP = "__atlasInstanceIndex";
constexpr const char *ATLAS_TEXTURE_ID_PROP = "__atlasTextureId";
constexpr const char *ATLAS_CUBEMAP_ID_PROP = "__atlasCubemapId";
constexpr const char *ATLAS_SKYBOX_ID_PROP = "__atlasSkyboxId";
constexpr const char *ATLAS_RENDER_TARGET_ID_PROP = "__atlasRenderTargetId";
constexpr const char *ATLAS_POINT_LIGHT_ID_PROP = "__atlasPointLightId";
constexpr const char *ATLAS_DIRECTIONAL_LIGHT_ID_PROP =
    "__atlasDirectionalLightId";
constexpr const char *ATLAS_SPOT_LIGHT_ID_PROP = "__atlasSpotLightId";
constexpr const char *ATLAS_AREA_LIGHT_ID_PROP = "__atlasAreaLightId";
constexpr const char *ATLAS_PARTICLE_EMITTER_ID_PROP =
    "__atlasParticleEmitterId";
constexpr const char *ATLAS_WORLEY_NOISE_ID_PROP = "__atlasWorleyNoiseId";
constexpr const char *ATLAS_CLOUDS_ID_PROP = "__atlasCloudsId";
constexpr const char *ATLAS_ATMOSPHERE_ID_PROP = "__atlasAtmosphereId";
constexpr const char *ATLAS_GENERATION_PROP = "__atlasGeneration";
constexpr const char *ATLAS_IS_CORE_OBJECT_PROP = "__atlasIsCoreObject";
constexpr const char *ATLAS_WINDOW_PROP = "__atlasWindow";
constexpr const char *ATLAS_CAMERA_PROP = "__atlasCamera";
constexpr const char *ATLAS_SCENE_PROP = "__atlasScene";
constexpr const char *ATLAS_AUDIO_ENGINE_PROP = "__atlasAudioEngine";
constexpr const char *ATLAS_NATIVE_COMPONENT_KIND_PROP =
    "__atlasNativeComponentKind";
constexpr const char *GRAPHITE_ON_CHANGE_PROP = "__graphiteOnChange";
constexpr const char *GRAPHITE_ON_CLICK_PROP = "__graphiteOnClick";
constexpr const char *GRAPHITE_ON_TOGGLE_PROP = "__graphiteOnToggle";

const auto ATLAS_KEY_ENTRIES = std::to_array<std::pair<const char *, Key>>({
    {"Unknown", Key::Unknown},
    {"Space", Key::Space},
    {"Apostrophe", Key::Apostrophe},
    {"Comma", Key::Comma},
    {"Minus", Key::Minus},
    {"Period", Key::Period},
    {"Slash", Key::Slash},
    {"Key0", Key::Key0},
    {"Key1", Key::Key1},
    {"Key2", Key::Key2},
    {"Key3", Key::Key3},
    {"Key4", Key::Key4},
    {"Key5", Key::Key5},
    {"Key6", Key::Key6},
    {"Key7", Key::Key7},
    {"Key8", Key::Key8},
    {"Key9", Key::Key9},
    {"Semicolon", Key::Semicolon},
    {"Equal", Key::Equal},
    {"A", Key::A},
    {"B", Key::B},
    {"C", Key::C},
    {"D", Key::D},
    {"E", Key::E},
    {"F", Key::F},
    {"G", Key::G},
    {"H", Key::H},
    {"I", Key::I},
    {"J", Key::J},
    {"K", Key::K},
    {"L", Key::L},
    {"M", Key::M},
    {"N", Key::N},
    {"O", Key::O},
    {"P", Key::P},
    {"Q", Key::Q},
    {"R", Key::R},
    {"S", Key::S},
    {"T", Key::T},
    {"U", Key::U},
    {"V", Key::V},
    {"W", Key::W},
    {"X", Key::X},
    {"Y", Key::Y},
    {"Z", Key::Z},
    {"LeftBracket", Key::LeftBracket},
    {"Backslash", Key::Backslash},
    {"RightBracket", Key::RightBracket},
    {"GraveAccent", Key::GraveAccent},
    {"Escape", Key::Escape},
    {"Enter", Key::Enter},
    {"Tab", Key::Tab},
    {"Backspace", Key::Backspace},
    {"Insert", Key::Insert},
    {"Delete", Key::Delete},
    {"Right", Key::Right},
    {"Left", Key::Left},
    {"Down", Key::Down},
    {"Up", Key::Up},
    {"PageUp", Key::PageUp},
    {"PageDown", Key::PageDown},
    {"Home", Key::Home},
    {"End", Key::End},
    {"CapsLock", Key::CapsLock},
    {"ScrollLock", Key::ScrollLock},
    {"NumLock", Key::NumLock},
    {"PrintScreen", Key::PrintScreen},
    {"Pause", Key::Pause},
    {"F1", Key::F1},
    {"F2", Key::F2},
    {"F3", Key::F3},
    {"F4", Key::F4},
    {"F5", Key::F5},
    {"F6", Key::F6},
    {"F7", Key::F7},
    {"F8", Key::F8},
    {"F9", Key::F9},
    {"F10", Key::F10},
    {"F11", Key::F11},
    {"F12", Key::F12},
    {"F13", Key::F13},
    {"F14", Key::F14},
    {"F15", Key::F15},
    {"F16", Key::F16},
    {"F17", Key::F17},
    {"F18", Key::F18},
    {"F19", Key::F19},
    {"F20", Key::F20},
    {"F21", Key::F21},
    {"F22", Key::F22},
    {"F23", Key::F23},
    {"F24", Key::F24},
    {"F25", Key::F25},
    {"KP0", Key::KP0},
    {"KP1", Key::KP1},
    {"KP2", Key::KP2},
    {"KP3", Key::KP3},
    {"KP4", Key::KP4},
    {"KP5", Key::KP5},
    {"KP6", Key::KP6},
    {"KP7", Key::KP7},
    {"KP8", Key::KP8},
    {"KP9", Key::KP9},
    {"KPDecimal", Key::KPDecimal},
    {"KPDivide", Key::KPDivide},
    {"KPMultiply", Key::KPMultiply},
    {"KPSubtract", Key::KPSubtract},
    {"KPAdd", Key::KPAdd},
    {"KPEnter", Key::KPEnter},
    {"KPEqual", Key::KPEqual},
    {"LeftShift", Key::LeftShift},
    {"LeftControl", Key::LeftControl},
    {"LeftAlt", Key::LeftAlt},
    {"LeftSuper", Key::LeftSuper},
    {"RightShift", Key::RightShift},
    {"RightControl", Key::RightControl},
    {"RightAlt", Key::RightAlt},
    {"RightSuper", Key::RightSuper},
    {"Menu", Key::Menu},
});

const auto ATLAS_MOUSE_BUTTON_ENTRIES =
    std::to_array<std::pair<const char *, MouseButton>>({
        {"Left", MouseButton::Left},
        {"Right", MouseButton::Right},
        {"Middle", MouseButton::Middle},
        {"X1", MouseButton::Button4},
        {"X2", MouseButton::Button5},
        {"Button4", MouseButton::Button4},
        {"Button5", MouseButton::Button5},
        {"Button6", MouseButton::Button6},
        {"Button7", MouseButton::Button7},
        {"Button8", MouseButton::Button8},
        {"Last", MouseButton::Last},
    });

const std::array<MouseButton, 8> ATLAS_MOUSE_BUTTONS = {
    MouseButton::Left,    MouseButton::Right,   MouseButton::Middle,
    MouseButton::Button4, MouseButton::Button5, MouseButton::Button6,
    MouseButton::Button7, MouseButton::Button8};

ScriptHost *getHost(JSContext *ctx) {
    return static_cast<ScriptHost *>(JS_GetContextOpaque(ctx));
}

std::string normalizeToken(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '.') {
            continue;
        }
        normalized.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

std::string makeComponentLookupKey(int ownerId, const std::string &name) {
    return std::to_string(ownerId) + ":" + normalizeToken(name);
}

ResourceType toNativeResourceType(int type) {
    switch (type) {
    case 1:
        return ResourceType::Image;
    case 2:
        return ResourceType::SpecularMap;
    case 3:
        return ResourceType::Audio;
    case 4:
        return ResourceType::Font;
    case 5:
        return ResourceType::Model;
    default:
        return ResourceType::File;
    }
}

int toScriptResourceType(ResourceType type) {
    switch (type) {
    case ResourceType::Image:
        return 1;
    case ResourceType::SpecularMap:
        return 2;
    case ResourceType::Audio:
        return 3;
    case ResourceType::Font:
        return 4;
    case ResourceType::Model:
        return 5;
    case ResourceType::File:
    default:
        return 0;
    }
}

TextureType toNativeTextureType(int type) {
    switch (type) {
    case 1:
        return TextureType::Specular;
    case 2:
        return TextureType::Cubemap;
    case 3:
        return TextureType::Depth;
    case 4:
        return TextureType::DepthCube;
    case 5:
        return TextureType::Normal;
    case 6:
        return TextureType::Parallax;
    case 7:
        return TextureType::SSAONoise;
    case 8:
        return TextureType::SSAO;
    case 9:
        return TextureType::Metallic;
    case 10:
        return TextureType::Roughness;
    case 11:
        return TextureType::AO;
    case 12:
        return TextureType::Opacity;
    case 13:
        return TextureType::HDR;
    case 0:
    default:
        return TextureType::Color;
    }
}

int toScriptTextureType(TextureType type) {
    switch (type) {
    case TextureType::Specular:
        return 1;
    case TextureType::Cubemap:
        return 2;
    case TextureType::Depth:
        return 3;
    case TextureType::DepthCube:
        return 4;
    case TextureType::Normal:
        return 5;
    case TextureType::Parallax:
        return 6;
    case TextureType::SSAONoise:
        return 7;
    case TextureType::SSAO:
        return 8;
    case TextureType::Metallic:
        return 9;
    case TextureType::Roughness:
        return 10;
    case TextureType::AO:
        return 11;
    case TextureType::Opacity:
        return 12;
    case TextureType::HDR:
        return 13;
    case TextureType::Color:
    default:
        return 0;
    }
}

RenderTargetType toNativeRenderTargetType(int type) {
    switch (type) {
    case 1:
        return RenderTargetType::Multisampled;
    case 2:
        return RenderTargetType::Shadow;
    case 3:
        return RenderTargetType::CubeShadow;
    case 4:
        return RenderTargetType::GBuffer;
    case 5:
        return RenderTargetType::SSAO;
    case 6:
        return RenderTargetType::SSAOBlur;
    case 0:
    default:
        return RenderTargetType::Scene;
    }
}

int toScriptRenderTargetType(RenderTargetType type) {
    switch (type) {
    case RenderTargetType::Multisampled:
        return 1;
    case RenderTargetType::Shadow:
        return 2;
    case RenderTargetType::CubeShadow:
        return 3;
    case RenderTargetType::GBuffer:
        return 4;
    case RenderTargetType::SSAO:
        return 5;
    case RenderTargetType::SSAOBlur:
        return 6;
    case RenderTargetType::Scene:
    default:
        return 0;
    }
}

std::uint64_t makeInstanceKey(int ownerId, std::uint32_t index) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(ownerId))
            << 32) |
           static_cast<std::uint64_t>(index);
}

bool setProperty(JSContext *ctx, JSValue obj, const char *name, JSValue value) {
    if (JS_SetPropertyStr(ctx, obj, name, value) < 0) {
        JS_FreeValue(ctx, value);
        return false;
    }
    return true;
}

ScriptTextureState *resolveTexture(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value);

bool getInt64(JSContext *ctx, JSValueConst value, std::int64_t &out) {
    int64_t temp = 0;
    if (JS_ToInt64(ctx, &temp, value) < 0) {
        return false;
    }
    out = static_cast<std::int64_t>(temp);
    return true;
}

bool getUint32(JSContext *ctx, JSValueConst value, std::uint32_t &out) {
    uint32_t temp = 0;
    if (JS_ToUint32(ctx, &temp, value) < 0) {
        return false;
    }
    out = temp;
    return true;
}

bool getDouble(JSContext *ctx, JSValueConst value, double &out) {
    double temp = 0.0;
    if (JS_ToFloat64(ctx, &temp, value) < 0) {
        return false;
    }
    out = temp;
    return true;
}

bool readNumberProperty(JSContext *ctx, JSValueConst obj, const char *name,
                        double &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    bool ok = !JS_IsUndefined(value) && getDouble(ctx, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

bool readBoolProperty(JSContext *ctx, JSValueConst obj, const char *name,
                      bool &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    if (JS_IsUndefined(value)) {
        JS_FreeValue(ctx, value);
        return false;
    }
    out = JS_ToBool(ctx, value) == 1;
    JS_FreeValue(ctx, value);
    return true;
}

bool readIntProperty(JSContext *ctx, JSValueConst obj, const char *name,
                     std::int64_t &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    bool ok = !JS_IsUndefined(value) && getInt64(ctx, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

bool readStringProperty(JSContext *ctx, JSValueConst obj, const char *name,
                        std::string &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    if (JS_IsUndefined(value)) {
        JS_FreeValue(ctx, value);
        return false;
    }
    const char *str = JS_ToCString(ctx, value);
    if (str == nullptr) {
        JS_FreeValue(ctx, value);
        return false;
    }
    out = str;
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, value);
    return true;
}

bool ensureCurrentGeneration(JSContext *ctx, ScriptHost &host,
                             JSValueConst obj) {
    std::int64_t generation = 0;
    if (!readIntProperty(ctx, obj, ATLAS_GENERATION_PROP, generation)) {
        return true;
    }
    if (static_cast<std::uint64_t>(generation) == host.generation) {
        return true;
    }
    JS_ThrowReferenceError(ctx, "Stale Atlas script handle");
    return false;
}

GameObject *findObjectById(ScriptHost &host, int id) {
    auto atlasIt = atlas::gameObjects.find(id);
    if (atlasIt != atlas::gameObjects.end()) {
        return atlasIt->second;
    }

    if (host.context == nullptr) {
        return nullptr;
    }

    auto stateIt = host.objectStates.find(id);
    if (stateIt != host.objectStates.end()) {
        return stateIt->second.object;
    }

    return nullptr;
}

GameObject *findObjectByName(ScriptHost &host, const std::string &name) {
    if (host.context == nullptr) {
        return nullptr;
    }

    auto it = host.context->objectReferences.find(name);
    if (it != host.context->objectReferences.end()) {
        return it->second;
    }

    const std::string normalized = normalizeToken(name);
    it = host.context->objectReferences.find(normalized);
    if (it != host.context->objectReferences.end()) {
        return it->second;
    }

    return nullptr;
}

Camera *getSceneCamera(ScriptHost &host) {
    if (host.context == nullptr) {
        return nullptr;
    }
    if (host.context->camera != nullptr) {
        return host.context->camera.get();
    }
    if (host.context->window != nullptr) {
        return host.context->window->getCamera();
    }
    return nullptr;
}

Window *getWindow(ScriptHost &host) {
    if (host.context != nullptr && host.context->window != nullptr) {
        return host.context->window.get();
    }
    return Window::mainWindow;
}

Scene *getScene(ScriptHost &host) {
    if (host.context == nullptr) {
        return nullptr;
    }
    if (host.context->window != nullptr) {
        Scene *currentScene = host.context->window->getCurrentScene();
        if (currentScene != nullptr) {
            return currentScene;
        }
    }
    return host.context->scene.get();
}

void assignObjectName(Context &context, GameObject &object,
                      const std::string &name) {
    const int objectId = object.getId();
    auto oldIt = context.objectNames.find(objectId);
    if (oldIt != context.objectNames.end()) {
        const std::string oldName = oldIt->second;
        auto eraseIfMatches = [&](const std::string &key) {
            auto refIt = context.objectReferences.find(key);
            if (refIt != context.objectReferences.end() &&
                refIt->second == &object) {
                context.objectReferences.erase(refIt);
            }
        };
        if (!oldName.empty() && oldName != name) {
            eraseIfMatches(oldName);
            eraseIfMatches(normalizeToken(oldName));
        }
    }

    if (name.empty()) {
        context.objectNames.erase(objectId);
        context.objectSceneReferences.erase(objectId);
        object.name.clear();
        return;
    }

    auto ensureAvailable = [&](const std::string &key) {
        if (key.empty()) {
            return true;
        }
        auto refIt = context.objectReferences.find(key);
        return refIt == context.objectReferences.end() ||
               refIt->second == &object;
    };

    const std::string normalized = normalizeToken(name);
    if (!ensureAvailable(name) || !ensureAvailable(normalized)) {
        throw std::runtime_error("Duplicate object reference: " + name);
    }

    context.objectReferences[name] = &object;
    context.objectReferences[normalized] = &object;
    context.objectNames[objectId] = name;
    object.name = name;
}

void cachePrototype(JSContext *ctx, JSValueConst ns, const char *exportName,
                    JSValue &target) {
    if (!JS_IsUndefined(target)) {
        return;
    }

    JSValue ctor = JS_GetPropertyStr(ctx, ns, exportName);
    if (JS_IsException(ctor) || JS_IsUndefined(ctor)) {
        JS_FreeValue(ctx, ctor);
        return;
    }

    target = JS_GetPropertyStr(ctx, ctor, "prototype");
    JS_FreeValue(ctx, ctor);
}

bool ensureBuiltins(JSContext *ctx, ScriptHost &host) {
    if (JS_IsUndefined(host.atlasNamespace)) {
        host.atlasNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas");
        if (JS_IsException(host.atlasNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasUnitsNamespace)) {
        host.atlasUnitsNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/units");
        if (JS_IsException(host.atlasUnitsNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasInputNamespace)) {
        host.atlasInputNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/input");
        if (JS_IsException(host.atlasInputNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasGraphicsNamespace)) {
        host.atlasGraphicsNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/graphics");
        if (JS_IsException(host.atlasGraphicsNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasParticleNamespace)) {
        host.atlasParticleNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/particles");
        if (JS_IsException(host.atlasParticleNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasBezelNamespace)) {
        host.atlasBezelNamespace =
            runtime::scripting::importModuleNamespace(ctx, "bezel");
        if (JS_IsException(host.atlasBezelNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.auroraNamespace)) {
        host.auroraNamespace =
            runtime::scripting::importModuleNamespace(ctx, "aurora");
        if (JS_IsException(host.auroraNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.finewaveNamespace)) {
        host.finewaveNamespace =
            runtime::scripting::importModuleNamespace(ctx, "finewave");
        if (JS_IsException(host.finewaveNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.graphiteNamespace)) {
        host.graphiteNamespace =
            runtime::scripting::importModuleNamespace(ctx, "graphite");
        if (JS_IsException(host.graphiteNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.hydraNamespace)) {
        host.hydraNamespace =
            runtime::scripting::importModuleNamespace(ctx, "hydra");
        if (JS_IsException(host.hydraNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    cachePrototype(ctx, host.atlasNamespace, "Component",
                   host.componentPrototype);
    cachePrototype(ctx, host.atlasNamespace, "GameObject",
                   host.gameObjectPrototype);
    cachePrototype(ctx, host.atlasNamespace, "UIObject",
                   host.uiObjectPrototype);
    cachePrototype(ctx, host.atlasNamespace, "CoreObject",
                   host.coreObjectPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Model", host.modelPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Material",
                   host.materialPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Instance",
                   host.instancePrototype);
    cachePrototype(ctx, host.atlasNamespace, "CoreVertex",
                   host.coreVertexPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Resource",
                   host.resourcePrototype);
    cachePrototype(ctx, host.atlasNamespace, "Window", host.windowPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Monitor", host.monitorPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Gamepad", host.gamepadPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Joystick",
                   host.joystickPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Camera", host.cameraPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Scene", host.scenePrototype);
    cachePrototype(ctx, host.finewaveNamespace, "AudioEngine",
                   host.audioEnginePrototype);
    cachePrototype(ctx, host.finewaveNamespace, "AudioData",
                   host.audioDataPrototype);
    cachePrototype(ctx, host.finewaveNamespace, "AudioSource",
                   host.audioSourcePrototype);
    cachePrototype(ctx, host.finewaveNamespace, "AudioEffect",
                   host.audioEffectPrototype);
    cachePrototype(ctx, host.finewaveNamespace, "Reverb", host.reverbPrototype);
    cachePrototype(ctx, host.finewaveNamespace, "Echo", host.echoPrototype);
    cachePrototype(ctx, host.finewaveNamespace, "Distortion",
                   host.distortionPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Image", host.imagePrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Text", host.textPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "TextField",
                   host.textFieldPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Button", host.buttonPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Checkbox",
                   host.checkboxPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Column", host.columnPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Row", host.rowPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Stack", host.stackPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Font", host.fontPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "UIStyle",
                   host.uiStylePrototype);
    cachePrototype(ctx, host.graphiteNamespace, "UIStyleVariant",
                   host.uiStyleVariantPrototype);
    cachePrototype(ctx, host.graphiteNamespace, "Theme", host.themePrototype);
    cachePrototype(ctx, host.hydraNamespace, "WorleyNoise3D",
                   host.worleyNoisePrototype);
    cachePrototype(ctx, host.hydraNamespace, "Clouds", host.cloudsPrototype);
    cachePrototype(ctx, host.hydraNamespace, "Atmosphere",
                   host.atmospherePrototype);
    cachePrototype(ctx, host.hydraNamespace, "Fluid", host.fluidPrototype);
    cachePrototype(ctx, host.atlasInputNamespace, "Trigger",
                   host.triggerPrototype);
    cachePrototype(ctx, host.atlasInputNamespace, "AxisTrigger",
                   host.axisTriggerPrototype);
    cachePrototype(ctx, host.atlasInputNamespace, "InputAction",
                   host.inputActionPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Texture",
                   host.texturePrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Cubemap",
                   host.cubemapPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Skybox",
                   host.skyboxPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "RenderTarget",
                   host.renderTargetPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Light",
                   host.pointLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "DirectionalLight",
                   host.directionalLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "SpotLight",
                   host.spotLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "AreaLight",
                   host.areaLightPrototype);
    cachePrototype(ctx, host.atlasParticleNamespace, "ParticleEmitter",
                   host.particleEmitterPrototype);
    cachePrototype(ctx, host.auroraNamespace, "Terrain", host.terrainPrototype);
    cachePrototype(ctx, host.auroraNamespace, "Biome", host.biomePrototype);
    cachePrototype(ctx, host.auroraNamespace, "TerrainGenerator",
                   host.terrainGeneratorPrototype);
    cachePrototype(ctx, host.auroraNamespace, "HillGenerator",
                   host.hillGeneratorPrototype);
    cachePrototype(ctx, host.auroraNamespace, "MountainGenerator",
                   host.mountainGeneratorPrototype);
    cachePrototype(ctx, host.auroraNamespace, "PlainGenerator",
                   host.plainGeneratorPrototype);
    cachePrototype(ctx, host.auroraNamespace, "IslandGenerator",
                   host.islandGeneratorPrototype);
    cachePrototype(ctx, host.auroraNamespace, "CompoundGenerator",
                   host.compoundGeneratorPrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "Rigidbody",
                   host.rigidbodyPrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "Sensor",
                   host.sensorPrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "Vehicle",
                   host.vehiclePrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "FixedJoint",
                   host.fixedJointPrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "HingeJoint",
                   host.hingeJointPrototype);
    cachePrototype(ctx, host.atlasBezelNamespace, "SpringJoint",
                   host.springJointPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Position3d",
                   host.position3dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Position2d",
                   host.position2dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Color", host.colorPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Size2d",
                   host.size2dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Quaternion",
                   host.quaternionPrototype);
    return true;
}

JSValue newObjectFromPrototype(JSContext *ctx, JSValueConst prototype) {
    if (JS_IsUndefined(prototype)) {
        return JS_NewObject(ctx);
    }
    return JS_NewObjectProto(ctx, prototype);
}

JSValue makePosition3d(JSContext *ctx, ScriptHost &host,
                       const Position3d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position3dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    setProperty(ctx, result, "z", JS_NewFloat64(ctx, value.z));
    return result;
}

JSValue makePosition2d(JSContext *ctx, ScriptHost &host,
                       const Position2d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position2dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    return result;
}

JSValue makeSize2d(JSContext *ctx, ScriptHost &host, const Size2d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.size2dPrototype);
    setProperty(ctx, result, "width", JS_NewFloat64(ctx, value.width));
    setProperty(ctx, result, "height", JS_NewFloat64(ctx, value.height));
    return result;
}

JSValue makeColor(JSContext *ctx, ScriptHost &host, const Color &value) {
    JSValue result = newObjectFromPrototype(ctx, host.colorPrototype);
    setProperty(ctx, result, "r", JS_NewFloat64(ctx, value.r));
    setProperty(ctx, result, "g", JS_NewFloat64(ctx, value.g));
    setProperty(ctx, result, "b", JS_NewFloat64(ctx, value.b));
    setProperty(ctx, result, "a", JS_NewFloat64(ctx, value.a));
    return result;
}

JSValue makeResource(JSContext *ctx, ScriptHost &host,
                     const Resource &resource) {
    JSValue result = newObjectFromPrototype(ctx, host.resourcePrototype);
    setProperty(ctx, result, "type",
                JS_NewInt32(ctx, toScriptResourceType(resource.type)));
    setProperty(ctx, result, "path",
                JS_NewString(ctx, resource.path.string().c_str()));
    setProperty(ctx, result, "name", JS_NewString(ctx, resource.name.c_str()));
    return result;
}

bool parsePosition3d(JSContext *ctx, JSValueConst value, Position3d &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z)) {
        return false;
    }
    out = Position3d(x, y, z);
    return true;
}

bool parsePosition2d(JSContext *ctx, JSValueConst value, Position2d &out) {
    double x = 0.0;
    double y = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y)) {
        return false;
    }
    out = Position2d(x, y);
    return true;
}

JSValue makeRotation3d(JSContext *ctx, ScriptHost &host,
                       const Rotation3d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position3dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.pitch));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.yaw));
    setProperty(ctx, result, "z", JS_NewFloat64(ctx, value.roll));
    return result;
}

bool parseRotation3d(JSContext *ctx, JSValueConst value, Rotation3d &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z)) {
        return false;
    }
    out = Rotation3d(x, y, z);
    return true;
}

bool parseSize2d(JSContext *ctx, JSValueConst value, Size2d &out) {
    double width = 0.0;
    double height = 0.0;
    if (!readNumberProperty(ctx, value, "width", width)) {
        if (!readNumberProperty(ctx, value, "x", width)) {
            return false;
        }
    }
    if (!readNumberProperty(ctx, value, "height", height)) {
        if (!readNumberProperty(ctx, value, "y", height)) {
            return false;
        }
    }
    out = Size2d(width, height);
    return true;
}

bool parseColor(JSContext *ctx, JSValueConst value, Color &out) {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 1.0;
    if (!readNumberProperty(ctx, value, "r", r) ||
        !readNumberProperty(ctx, value, "g", g) ||
        !readNumberProperty(ctx, value, "b", b)) {
        return false;
    }
    readNumberProperty(ctx, value, "a", a);
    out = Color(r, g, b, a);
    return true;
}

bool parseEnvironmentValue(JSContext *ctx, ScriptHost &host, JSValueConst value,
                           Environment &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    JSValue prop = JS_GetPropertyStr(ctx, value, "fog");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && JS_IsObject(prop) &&
        !JS_IsNull(prop)) {
        JSValue field = JS_GetPropertyStr(ctx, prop, "color");
        if (!JS_IsException(field) && !JS_IsUndefined(field)) {
            parseColor(ctx, field, out.fog.color);
        }
        JS_FreeValue(ctx, field);

        double number = out.fog.intensity;
        readNumberProperty(ctx, prop, "intensity", number);
        out.fog.intensity = static_cast<float>(number);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "volumetricLighting");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && JS_IsObject(prop) &&
        !JS_IsNull(prop)) {
        readBoolProperty(ctx, prop, "enabled", out.volumetricLighting.enabled);
        double number = out.volumetricLighting.density;
        readNumberProperty(ctx, prop, "density", number);
        out.volumetricLighting.density = static_cast<float>(number);
        number = out.volumetricLighting.weight;
        readNumberProperty(ctx, prop, "weight", number);
        out.volumetricLighting.weight = static_cast<float>(number);
        number = out.volumetricLighting.decay;
        readNumberProperty(ctx, prop, "decay", number);
        out.volumetricLighting.decay = static_cast<float>(number);
        number = out.volumetricLighting.exposure;
        readNumberProperty(ctx, prop, "exposure", number);
        out.volumetricLighting.exposure = static_cast<float>(number);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "lightBloom");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && JS_IsObject(prop) &&
        !JS_IsNull(prop)) {
        double radius = out.lightBloom.radius;
        readNumberProperty(ctx, prop, "radius", radius);
        out.lightBloom.radius = static_cast<float>(radius);

        std::int64_t maxSamples = out.lightBloom.maxSamples;
        readIntProperty(ctx, prop, "maxSamples", maxSamples);
        out.lightBloom.maxSamples = static_cast<int>(maxSamples);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "rimLighting");
    if (JS_IsException(prop) || JS_IsUndefined(prop) || !JS_IsObject(prop) ||
        JS_IsNull(prop)) {
        JS_FreeValue(ctx, prop);
        prop = JS_GetPropertyStr(ctx, value, "rimLight");
    }
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && JS_IsObject(prop) &&
        !JS_IsNull(prop)) {
        JSValue field = JS_GetPropertyStr(ctx, prop, "color");
        if (!JS_IsException(field) && !JS_IsUndefined(field)) {
            parseColor(ctx, field, out.rimLight.color);
        }
        JS_FreeValue(ctx, field);

        double intensity = out.rimLight.intensity;
        readNumberProperty(ctx, prop, "intensity", intensity);
        out.rimLight.intensity = static_cast<float>(intensity);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "lookupTexture");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && !JS_IsNull(prop)) {
        auto *textureState = resolveTexture(ctx, host, prop);
        if (textureState != nullptr && textureState->texture) {
            out.lookupTexture = *textureState->texture;
        }
    }
    JS_FreeValue(ctx, prop);
    return true;
}

bool parseResource(JSContext *ctx, JSValueConst value, Resource &out) {
    std::string path;
    std::string name;
    std::int64_t type = 0;

    if (!readStringProperty(ctx, value, "path", path) ||
        !readStringProperty(ctx, value, "name", name)) {
        return false;
    }

    readIntProperty(ctx, value, "type", type);
    out.path = path;
    out.name = name;
    out.type = toNativeResourceType(static_cast<int>(type));
    return true;
}

std::shared_ptr<TerrainGenerator>
parseTerrainGeneratorValue(JSContext *ctx, ScriptHost &host,
                           JSValueConst value) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return nullptr;
    }

    std::string algorithm;
    if (!readStringProperty(ctx, value, "algorithm", algorithm)) {
        if (!readStringProperty(ctx, value, "type", algorithm)) {
            return nullptr;
        }
    }

    const std::string token = normalizeToken(algorithm);

    if (token == "hill" || token == "hills" || token == "perlin" ||
        token == "perlinnoise") {
        double scale = 0.01;
        double amplitude = 10.0;
        readNumberProperty(ctx, value, "scale", scale);
        readNumberProperty(ctx, value, "amplitude", amplitude);
        return std::make_shared<HillGenerator>(static_cast<float>(scale),
                                               static_cast<float>(amplitude));
    }

    if (token == "plain" || token == "plains") {
        double scale = 0.02;
        double amplitude = 2.0;
        readNumberProperty(ctx, value, "scale", scale);
        readNumberProperty(ctx, value, "amplitude", amplitude);
        return std::make_shared<PlainGenerator>(static_cast<float>(scale),
                                                static_cast<float>(amplitude));
    }

    if (token == "mountain" || token == "mountains" || token == "fractal" ||
        token == "fractalnoise" || token == "simplex" ||
        token == "simplexnoise" || token == "diamondsquare") {
        double scale = 10.0;
        double amplitude = 100.0;
        std::int64_t octaves = 5;
        double persistence = 0.5;
        readNumberProperty(ctx, value, "scale", scale);
        readNumberProperty(ctx, value, "amplitude", amplitude);
        readIntProperty(ctx, value, "octaves", octaves);
        if (!readNumberProperty(ctx, value, "persistence", persistence)) {
            readNumberProperty(ctx, value, "persistance", persistence);
        }
        return std::make_shared<MountainGenerator>(
            static_cast<float>(scale), static_cast<float>(amplitude),
            static_cast<int>(octaves), static_cast<float>(persistence));
    }

    if (token == "island" || token == "worley" || token == "worleynoise") {
        std::int64_t numFeatures = 10;
        double scale = 0.01;
        readIntProperty(ctx, value, "numFeatures", numFeatures);
        readNumberProperty(ctx, value, "scale", scale);
        return std::make_shared<IslandGenerator>(static_cast<int>(numFeatures),
                                                 static_cast<float>(scale));
    }

    if (token == "compound" || token == "combined" || token == "composite") {
        auto compound = std::make_shared<CompoundGenerator>();
        JSValue generators = JS_GetPropertyStr(ctx, value, "generators");
        if (!JS_IsException(generators) && JS_IsArray(generators)) {
            std::uint32_t length = 0;
            JSValue lengthValue = JS_GetPropertyStr(ctx, generators, "length");
            if (!JS_IsException(lengthValue) &&
                getUint32(ctx, lengthValue, length)) {
                for (std::uint32_t i = 0; i < length; ++i) {
                    JSValue childValue =
                        JS_GetPropertyUint32(ctx, generators, i);
                    auto child =
                        !JS_IsException(childValue)
                            ? parseTerrainGeneratorValue(ctx, host, childValue)
                            : nullptr;
                    JS_FreeValue(ctx, childValue);
                    if (!child) {
                        continue;
                    }
                    if (auto hill =
                            std::dynamic_pointer_cast<HillGenerator>(child)) {
                        compound->addGenerator(*hill);
                    } else if (auto plain =
                                   std::dynamic_pointer_cast<PlainGenerator>(
                                       child)) {
                        compound->addGenerator(*plain);
                    } else if (auto mountain =
                                   std::dynamic_pointer_cast<MountainGenerator>(
                                       child)) {
                        compound->addGenerator(*mountain);
                    } else if (auto island =
                                   std::dynamic_pointer_cast<IslandGenerator>(
                                       child)) {
                        compound->addGenerator(*island);
                    } else if (auto nested =
                                   std::dynamic_pointer_cast<CompoundGenerator>(
                                       child)) {
                        compound->addGenerator(*nested);
                    }
                }
            }
            JS_FreeValue(ctx, lengthValue);
        }
        JS_FreeValue(ctx, generators);
        return compound;
    }

    (void)host;
    return nullptr;
}

bool parseBiomeValue(JSContext *ctx, ScriptHost &host, JSValueConst value,
                     Biome &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    readStringProperty(ctx, value, "name", out.name);

    JSValue prop = JS_GetPropertyStr(ctx, value, "color");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && !JS_IsNull(prop)) {
        parseColor(ctx, prop, out.color);
    }
    JS_FreeValue(ctx, prop);

    bool useTexture = out.useTexture;
    readBoolProperty(ctx, value, "useTexture", useTexture);
    out.useTexture = useTexture;

    double number = out.minHeight;
    readNumberProperty(ctx, value, "minHeight", number);
    out.minHeight = static_cast<float>(number);
    number = out.maxHeight;
    readNumberProperty(ctx, value, "maxHeight", number);
    out.maxHeight = static_cast<float>(number);
    number = out.minMoisture;
    readNumberProperty(ctx, value, "minMoisture", number);
    out.minMoisture = static_cast<float>(number);
    number = out.maxMoisture;
    readNumberProperty(ctx, value, "maxMoisture", number);
    out.maxMoisture = static_cast<float>(number);
    number = out.minTemperature;
    readNumberProperty(ctx, value, "minTemperature", number);
    out.minTemperature = static_cast<float>(number);
    number = out.maxTemperature;
    readNumberProperty(ctx, value, "maxTemperature", number);
    out.maxTemperature = static_cast<float>(number);

    prop = JS_GetPropertyStr(ctx, value, "texture");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop) && !JS_IsNull(prop)) {
        auto *textureState = resolveTexture(ctx, host, prop);
        if (textureState != nullptr && textureState->texture) {
            out.texture = *textureState->texture;
            if (out.texture.texture != nullptr || out.texture.id != 0) {
                out.useTexture = true;
            }
        }
    }
    JS_FreeValue(ctx, prop);

    return true;
}

bool parseQuaternion(JSContext *ctx, JSValueConst value, glm::quat &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z) ||
        !readNumberProperty(ctx, value, "w", w)) {
        return false;
    }
    out =
        glm::normalize(glm::quat(static_cast<float>(w), static_cast<float>(x),
                                 static_cast<float>(y), static_cast<float>(z)));
    return true;
}

bool parseMaterial(JSContext *ctx, JSValueConst value, Material &out) {
    Color albedo = out.albedo;
    JSValue albedoValue = JS_GetPropertyStr(ctx, value, "albedo");
    if (!JS_IsException(albedoValue) && !JS_IsUndefined(albedoValue)) {
        parseColor(ctx, albedoValue, albedo);
    }
    JS_FreeValue(ctx, albedoValue);
    out.albedo = albedo;

    double metallic = out.metallic;
    double roughness = out.roughness;
    double ao = out.ao;
    double reflectivity = out.reflectivity;
    double emissiveIntensity = out.emissiveIntensity;
    double normalMapStrength = out.normalMapStrength;
    double transmittance = out.transmittance;
    double ior = out.ior;
    bool useNormalMap = out.useNormalMap;
    Color emissiveColor = out.emissiveColor;

    readNumberProperty(ctx, value, "metallic", metallic);
    readNumberProperty(ctx, value, "roughness", roughness);
    readNumberProperty(ctx, value, "ao", ao);
    readNumberProperty(ctx, value, "reflectivity", reflectivity);
    readNumberProperty(ctx, value, "emissiveIntensity", emissiveIntensity);
    readNumberProperty(ctx, value, "normalMapStrength", normalMapStrength);
    readNumberProperty(ctx, value, "transmittance", transmittance);
    readNumberProperty(ctx, value, "ior", ior);
    readBoolProperty(ctx, value, "useNormalMap", useNormalMap);

    JSValue emissiveValue = JS_GetPropertyStr(ctx, value, "emissiveColor");
    if (!JS_IsException(emissiveValue) && !JS_IsUndefined(emissiveValue)) {
        parseColor(ctx, emissiveValue, emissiveColor);
    }
    JS_FreeValue(ctx, emissiveValue);

    out.metallic = static_cast<float>(metallic);
    out.roughness = static_cast<float>(roughness);
    out.ao = static_cast<float>(ao);
    out.reflectivity = static_cast<float>(reflectivity);
    out.emissiveColor = emissiveColor;
    out.emissiveIntensity = static_cast<float>(emissiveIntensity);
    out.normalMapStrength = static_cast<float>(normalMapStrength);
    out.useNormalMap = useNormalMap;
    out.transmittance = static_cast<float>(transmittance);
    out.ior = static_cast<float>(ior);
    return true;
}

JSValue makeParticleSettings(JSContext *ctx, const ParticleSettings &settings) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "minLifetime",
                JS_NewFloat64(ctx, settings.minLifetime));
    setProperty(ctx, result, "maxLifetime",
                JS_NewFloat64(ctx, settings.maxLifetime));
    setProperty(ctx, result, "minSize", JS_NewFloat64(ctx, settings.minSize));
    setProperty(ctx, result, "maxSize", JS_NewFloat64(ctx, settings.maxSize));
    setProperty(ctx, result, "fadeSpeed",
                JS_NewFloat64(ctx, settings.fadeSpeed));
    setProperty(ctx, result, "gravity", JS_NewFloat64(ctx, settings.gravity));
    setProperty(ctx, result, "spread", JS_NewFloat64(ctx, settings.spread));
    setProperty(ctx, result, "speedVariation",
                JS_NewFloat64(ctx, settings.speedVariation));
    return result;
}

bool parseParticleSettings(JSContext *ctx, JSValueConst value,
                           ParticleSettings &out) {
    double minLifetime = out.minLifetime;
    double maxLifetime = out.maxLifetime;
    double minSize = out.minSize;
    double maxSize = out.maxSize;
    double fadeSpeed = out.fadeSpeed;
    double gravity = out.gravity;
    double spread = out.spread;
    double speedVariation = out.speedVariation;

    readNumberProperty(ctx, value, "minLifetime", minLifetime);
    readNumberProperty(ctx, value, "maxLifetime", maxLifetime);
    readNumberProperty(ctx, value, "minSize", minSize);
    readNumberProperty(ctx, value, "maxSize", maxSize);
    readNumberProperty(ctx, value, "fadeSpeed", fadeSpeed);
    readNumberProperty(ctx, value, "gravity", gravity);
    readNumberProperty(ctx, value, "spread", spread);
    readNumberProperty(ctx, value, "speedVariation", speedVariation);

    out.minLifetime = static_cast<float>(minLifetime);
    out.maxLifetime = static_cast<float>(maxLifetime);
    out.minSize = static_cast<float>(minSize);
    out.maxSize = static_cast<float>(maxSize);
    out.fadeSpeed = static_cast<float>(fadeSpeed);
    out.gravity = static_cast<float>(gravity);
    out.spread = static_cast<float>(spread);
    out.speedVariation = static_cast<float>(speedVariation);
    return true;
}

ScriptAudioPlayerState *findAudioPlayerState(ScriptHost &host,
                                             std::uint64_t audioPlayerId) {
    auto it = host.audioPlayers.find(audioPlayerId);
    if (it == host.audioPlayers.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptAudioDataState *findAudioDataState(ScriptHost &host,
                                         std::uint64_t audioDataId) {
    auto it = host.audioData.find(audioDataId);
    if (it == host.audioData.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptAudioSourceState *findAudioSourceState(ScriptHost &host,
                                             std::uint64_t audioSourceId) {
    auto it = host.audioSources.find(audioSourceId);
    if (it == host.audioSources.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptReverbState *findReverbState(ScriptHost &host, std::uint64_t reverbId) {
    auto it = host.reverbs.find(reverbId);
    if (it == host.reverbs.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptEchoState *findEchoState(ScriptHost &host, std::uint64_t echoId) {
    auto it = host.echoes.find(echoId);
    if (it == host.echoes.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptDistortionState *findDistortionState(ScriptHost &host,
                                           std::uint64_t distortionId) {
    auto it = host.distortions.find(distortionId);
    if (it == host.distortions.end()) {
        return nullptr;
    }
    return &it->second;
}

bool getArrayLength(JSContext *ctx, JSValueConst value, std::uint32_t &length) {
    JSValue lengthValue = JS_GetPropertyStr(ctx, value, "length");
    if (JS_IsException(lengthValue)) {
        return false;
    }
    const bool ok = getUint32(ctx, lengthValue, length);
    JS_FreeValue(ctx, lengthValue);
    return ok;
}

bool parseColorArray(JSContext *ctx, JSValueConst value,
                     std::array<Color, 6> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    if (!getArrayLength(ctx, value, length) || length != out.size()) {
        return false;
    }

    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item) || !parseColor(ctx, item, out[i])) {
            JS_FreeValue(ctx, item);
            return false;
        }
        JS_FreeValue(ctx, item);
    }

    return true;
}

bool parseResourceVector(JSContext *ctx, JSValueConst value,
                         std::vector<Resource> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    if (!getArrayLength(ctx, value, length)) {
        return false;
    }

    out.clear();
    out.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item)) {
            JS_FreeValue(ctx, item);
            return false;
        }
        Resource resource;
        const bool ok = parseResource(ctx, item, resource);
        JS_FreeValue(ctx, item);
        if (!ok) {
            return false;
        }
        out.push_back(resource);
    }

    return true;
}

JSValue makePlainPosition2d(JSContext *ctx, const Position2d &value) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    return result;
}

JSValue makeAxisPacketValue(JSContext *ctx, const AxisPacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "deltaX", JS_NewFloat64(ctx, packet.deltaX));
    setProperty(ctx, result, "deltaY", JS_NewFloat64(ctx, packet.deltaY));
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, packet.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, packet.y));
    setProperty(ctx, result, "valueX", JS_NewFloat64(ctx, packet.valueX));
    setProperty(ctx, result, "valueY", JS_NewFloat64(ctx, packet.valueY));
    setProperty(ctx, result, "inputDeltaX",
                JS_NewFloat64(ctx, packet.inputDeltaX));
    setProperty(ctx, result, "inputDeltaY",
                JS_NewFloat64(ctx, packet.inputDeltaY));
    setProperty(ctx, result, "hasValueInput",
                JS_NewBool(ctx, packet.hasValueInput));
    setProperty(ctx, result, "hasDeltaInput",
                JS_NewBool(ctx, packet.hasDeltaInput));
    return result;
}

JSValue makeMousePacketValue(JSContext *ctx, const MousePacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "xpos", JS_NewFloat64(ctx, packet.xpos));
    setProperty(ctx, result, "ypos", JS_NewFloat64(ctx, packet.ypos));
    setProperty(ctx, result, "xoffset", JS_NewFloat64(ctx, packet.xoffset));
    setProperty(ctx, result, "yoffset", JS_NewFloat64(ctx, packet.yoffset));
    setProperty(ctx, result, "constrainPitch",
                JS_NewBool(ctx, packet.constrainPitch));
    setProperty(ctx, result, "firstMouse", JS_NewBool(ctx, packet.firstMouse));
    return result;
}

JSValue makeMouseScrollPacketValue(JSContext *ctx,
                                   const MouseScrollPacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "xoffset", JS_NewFloat64(ctx, packet.xoffset));
    setProperty(ctx, result, "yoffset", JS_NewFloat64(ctx, packet.yoffset));
    return result;
}

JSValue makeTriggerValue(JSContext *ctx, ScriptHost &host,
                         const Trigger &value) {
    ensureBuiltins(ctx, host);
    JSValue result = newObjectFromPrototype(ctx, host.triggerPrototype);
    setProperty(ctx, result, "type",
                JS_NewInt32(ctx, static_cast<int>(value.type)));
    switch (value.type) {
    case TriggerType::MouseButton:
        setProperty(ctx, result, "mouseButton",
                    JS_NewInt32(ctx, static_cast<int>(value.mouseButton)));
        break;
    case TriggerType::Key:
        setProperty(ctx, result, "key",
                    JS_NewInt32(ctx, static_cast<int>(value.key)));
        break;
    case TriggerType::ControllerButton: {
        JSValue controllerButton = JS_NewObject(ctx);
        setProperty(ctx, controllerButton, "controllerID",
                    JS_NewInt32(ctx, value.controllerButton.controllerID));
        setProperty(ctx, controllerButton, "buttonIndex",
                    JS_NewInt32(ctx, value.controllerButton.buttonIndex));
        setProperty(ctx, result, "controllerButton", controllerButton);
        break;
    }
    }
    return result;
}

JSValue makeAxisTriggerValue(JSContext *ctx, ScriptHost &host,
                             const AxisTrigger &value) {
    ensureBuiltins(ctx, host);
    JSValue result = newObjectFromPrototype(ctx, host.axisTriggerPrototype);
    setProperty(ctx, result, "type",
                JS_NewInt32(ctx, static_cast<int>(value.type)));
    setProperty(ctx, result, "positiveX",
                makeTriggerValue(ctx, host, value.positiveX));
    setProperty(ctx, result, "negativeX",
                makeTriggerValue(ctx, host, value.negativeX));
    setProperty(ctx, result, "positiveY",
                makeTriggerValue(ctx, host, value.positiveY));
    setProperty(ctx, result, "negativeY",
                makeTriggerValue(ctx, host, value.negativeY));
    setProperty(ctx, result, "controllerId",
                JS_NewInt32(ctx, value.controllerID));
    setProperty(ctx, result, "controllerAxisSingle",
                JS_NewBool(ctx, value.controllerAxisSingle));
    setProperty(ctx, result, "axisIndex", JS_NewInt32(ctx, value.axisIndex));
    setProperty(ctx, result, "axisIndexY", JS_NewInt32(ctx, value.axisIndexY));
    setProperty(ctx, result, "isJoystick", JS_NewBool(ctx, value.isJoystick));
    return result;
}

JSValue makeInputActionValue(JSContext *ctx, ScriptHost &host,
                             const InputAction &value) {
    ensureBuiltins(ctx, host);
    JSValue result = newObjectFromPrototype(ctx, host.inputActionPrototype);
    setProperty(ctx, result, "name", JS_NewString(ctx, value.name.c_str()));
    setProperty(ctx, result, "isAxis", JS_NewBool(ctx, value.isAxis));
    setProperty(ctx, result, "isAxisSingle",
                JS_NewBool(ctx, value.isAxisSingle));
    setProperty(ctx, result, "normalized", JS_NewBool(ctx, value.normalize2D));
    setProperty(ctx, result, "invertY",
                JS_NewBool(ctx, value.invertControllerY));
    setProperty(ctx, result, "controllerDeadzone",
                JS_NewFloat64(ctx, value.controllerDeadzone));

    JSValue triggers = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < value.buttonTriggers.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, triggers, i,
            makeTriggerValue(ctx, host, value.buttonTriggers[i]));
    }
    setProperty(ctx, result, "triggers", triggers);

    JSValue axisTriggers = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < value.axisTriggers.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, axisTriggers, i,
            makeAxisTriggerValue(ctx, host, value.axisTriggers[i]));
    }
    setProperty(ctx, result, "axisTriggers", axisTriggers);
    return result;
}

JSValue makeVideoModeValue(JSContext *ctx, const VideoMode &value) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "width", JS_NewInt32(ctx, value.width));
    setProperty(ctx, result, "height", JS_NewInt32(ctx, value.height));
    setProperty(ctx, result, "refreshRate",
                JS_NewInt32(ctx, value.refreshRate));
    return result;
}

JSValue makeControllerIdValue(JSContext *ctx, const ControllerID &value) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "id", JS_NewInt32(ctx, value.id));
    setProperty(ctx, result, "name", JS_NewString(ctx, value.name.c_str()));
    setProperty(ctx, result, "isJoystick", JS_NewBool(ctx, value.isJoystick));
    return result;
}

bool parseTrigger(JSContext *ctx, JSValueConst value, Trigger &out) {
    std::int64_t type = 0;
    if (!readIntProperty(ctx, value, "type", type)) {
        return false;
    }

    switch (static_cast<TriggerType>(type)) {
    case TriggerType::MouseButton: {
        std::int64_t button = 0;
        if (!readIntProperty(ctx, value, "mouseButton", button)) {
            return false;
        }
        out = Trigger::fromMouseButton(static_cast<MouseButton>(button));
        return true;
    }
    case TriggerType::Key: {
        std::int64_t key = 0;
        if (!readIntProperty(ctx, value, "key", key)) {
            return false;
        }
        out = Trigger::fromKey(static_cast<Key>(key));
        return true;
    }
    case TriggerType::ControllerButton: {
        JSValue controllerButton =
            JS_GetPropertyStr(ctx, value, "controllerButton");
        if (JS_IsException(controllerButton) ||
            JS_IsUndefined(controllerButton)) {
            JS_FreeValue(ctx, controllerButton);
            return false;
        }
        std::int64_t controllerID = 0;
        std::int64_t buttonIndex = 0;
        const bool ok =
            readIntProperty(ctx, controllerButton, "controllerID",
                            controllerID) &&
            readIntProperty(ctx, controllerButton, "buttonIndex", buttonIndex);
        JS_FreeValue(ctx, controllerButton);
        if (!ok) {
            return false;
        }
        out = Trigger::fromControllerButton(static_cast<int>(controllerID),
                                            static_cast<int>(buttonIndex));
        return true;
    }
    }

    return false;
}

bool parseAxisTrigger(JSContext *ctx, JSValueConst value, AxisTrigger &out) {
    std::int64_t type = 0;
    if (!readIntProperty(ctx, value, "type", type)) {
        return false;
    }

    switch (static_cast<AxisTriggerType>(type)) {
    case AxisTriggerType::MouseAxis:
        out = AxisTrigger::mouse();
        break;
    case AxisTriggerType::KeyCustom: {
        Trigger positiveX;
        Trigger negativeX;
        Trigger positiveY;
        Trigger negativeY;

        JSValue prop = JS_GetPropertyStr(ctx, value, "positiveX");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, positiveX)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "negativeX");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, negativeX)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "positiveY");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, positiveY)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "negativeY");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, negativeY)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        out = AxisTrigger::custom(positiveX, negativeX, positiveY, negativeY);
        break;
    }
    case AxisTriggerType::ControllerAxis: {
        std::int64_t controllerId = CONTROLLER_UNDEFINED;
        std::int64_t axisIndex = -1;
        std::int64_t axisIndexY = -1;
        bool single = false;

        readIntProperty(ctx, value, "controllerId", controllerId);
        readIntProperty(ctx, value, "axisIndex", axisIndex);
        readIntProperty(ctx, value, "axisIndexY", axisIndexY);
        readBoolProperty(ctx, value, "controllerAxisSingle", single);

        if (axisIndex < 0) {
            return false;
        }

        out = AxisTrigger::controller(static_cast<int>(controllerId),
                                      static_cast<int>(axisIndex), single,
                                      static_cast<int>(axisIndexY));
        break;
    }
    }

    bool isJoystick = out.isJoystick;
    readBoolProperty(ctx, value, "isJoystick", isJoystick);
    out.isJoystick = isJoystick;
    return true;
}

bool parseInputAction(JSContext *ctx, JSValueConst value, InputAction &out) {
    std::string name;
    if (!readStringProperty(ctx, value, "name", name)) {
        return false;
    }

    bool isAxis = false;
    readBoolProperty(ctx, value, "isAxis", isAxis);
    bool isAxisSingle = false;
    readBoolProperty(ctx, value, "isAxisSingle", isAxisSingle);

    out = InputAction();
    out.name = name;
    out.isAxis = isAxis;
    out.isAxisSingle = isAxisSingle;

    if (isAxis) {
        JSValue axisTriggers = JS_GetPropertyStr(ctx, value, "axisTriggers");
        if (!JS_IsException(axisTriggers) && JS_IsArray(axisTriggers)) {
            std::uint32_t length = 0;
            if (!getArrayLength(ctx, axisTriggers, length)) {
                JS_FreeValue(ctx, axisTriggers);
                return false;
            }
            out.axisTriggers.clear();
            out.axisTriggers.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue axisTriggerValue =
                    JS_GetPropertyUint32(ctx, axisTriggers, i);
                AxisTrigger axisTrigger;
                const bool ok =
                    !JS_IsException(axisTriggerValue) &&
                    parseAxisTrigger(ctx, axisTriggerValue, axisTrigger);
                JS_FreeValue(ctx, axisTriggerValue);
                if (!ok) {
                    JS_FreeValue(ctx, axisTriggers);
                    return false;
                }
                out.axisTriggers.push_back(axisTrigger);
            }
        }
        JS_FreeValue(ctx, axisTriggers);
    } else {
        JSValue triggers = JS_GetPropertyStr(ctx, value, "triggers");
        if (!JS_IsException(triggers) && JS_IsArray(triggers)) {
            std::uint32_t length = 0;
            if (!getArrayLength(ctx, triggers, length)) {
                JS_FreeValue(ctx, triggers);
                return false;
            }
            out.buttonTriggers.clear();
            out.buttonTriggers.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue triggerValue = JS_GetPropertyUint32(ctx, triggers, i);
                Trigger trigger;
                const bool ok = !JS_IsException(triggerValue) &&
                                parseTrigger(ctx, triggerValue, trigger);
                JS_FreeValue(ctx, triggerValue);
                if (!ok) {
                    JS_FreeValue(ctx, triggers);
                    return false;
                }
                out.buttonTriggers.push_back(trigger);
            }
        }
        JS_FreeValue(ctx, triggers);
    }

    bool normalized = out.normalize2D;
    bool invertY = out.invertControllerY;
    double controllerDeadzone = out.controllerDeadzone;
    readBoolProperty(ctx, value, "normalized", normalized);
    readBoolProperty(ctx, value, "invertY", invertY);
    readNumberProperty(ctx, value, "controllerDeadzone", controllerDeadzone);
    out.normalize2D = normalized;
    out.invertControllerY = invertY;
    out.controllerDeadzone = static_cast<float>(controllerDeadzone);
    return true;
}

bool callObjectMethod(JSContext *ctx, JSValueConst object,
                      const char *methodName, int argc, JSValueConst *argv) {
    JSValue fn = JS_GetPropertyStr(ctx, object, methodName);
    if (JS_IsException(fn)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }
    if (JS_IsUndefined(fn) || !JS_IsFunction(ctx, fn)) {
        JS_FreeValue(ctx, fn);
        return false;
    }

    JSValue ret = JS_Call(ctx, fn, object, argc, argv);
    JS_FreeValue(ctx, fn);
    if (JS_IsException(ret)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }

    JS_FreeValue(ctx, ret);
    return true;
}

bool callObjectMethodEither(JSContext *ctx, JSValueConst object,
                            const char *primaryMethodName,
                            const char *secondaryMethodName, int argc,
                            JSValueConst *argv) {
    if (callObjectMethod(ctx, object, primaryMethodName, argc, argv)) {
        return true;
    }
    if (secondaryMethodName == nullptr) {
        return false;
    }
    return callObjectMethod(ctx, object, secondaryMethodName, argc, argv);
}

ScriptTextureState *findTextureState(ScriptHost &host,
                                     std::uint64_t textureId) {
    auto it = host.textures.find(textureId);
    if (it == host.textures.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptRigidbodyState *findRigidbodyState(ScriptHost &host,
                                         std::uint64_t rigidbodyId) {
    auto it = host.rigidbodies.find(rigidbodyId);
    if (it == host.rigidbodies.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptVehicleState *findVehicleState(ScriptHost &host,
                                     std::uint64_t vehicleId) {
    auto it = host.vehicles.find(vehicleId);
    if (it == host.vehicles.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptFixedJointState *findFixedJointState(ScriptHost &host,
                                           std::uint64_t jointId) {
    auto it = host.fixedJoints.find(jointId);
    if (it == host.fixedJoints.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptHingeJointState *findHingeJointState(ScriptHost &host,
                                           std::uint64_t jointId) {
    auto it = host.hingeJoints.find(jointId);
    if (it == host.hingeJoints.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptSpringJointState *findSpringJointState(ScriptHost &host,
                                             std::uint64_t jointId) {
    auto it = host.springJoints.find(jointId);
    if (it == host.springJoints.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptCubemapState *findCubemapState(ScriptHost &host,
                                     std::uint64_t cubemapId) {
    auto it = host.cubemaps.find(cubemapId);
    if (it == host.cubemaps.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptSkyboxState *findSkyboxState(ScriptHost &host, std::uint64_t skyboxId) {
    auto it = host.skyboxes.find(skyboxId);
    if (it == host.skyboxes.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptRenderTargetState *findRenderTargetState(ScriptHost &host,
                                               std::uint64_t renderTargetId) {
    auto it = host.renderTargets.find(renderTargetId);
    if (it == host.renderTargets.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptPointLightState *findPointLightState(ScriptHost &host,
                                           std::uint64_t lightId) {
    auto it = host.pointLights.find(lightId);
    if (it == host.pointLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptDirectionalLightState *findDirectionalLightState(ScriptHost &host,
                                                       std::uint64_t lightId) {
    auto it = host.directionalLights.find(lightId);
    if (it == host.directionalLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptSpotLightState *findSpotLightState(ScriptHost &host,
                                         std::uint64_t lightId) {
    auto it = host.spotLights.find(lightId);
    if (it == host.spotLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptAreaLightState *findAreaLightState(ScriptHost &host,
                                         std::uint64_t lightId) {
    auto it = host.areaLights.find(lightId);
    if (it == host.areaLights.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint64_t registerAudioDataState(ScriptHost &host,
                                     const std::shared_ptr<AudioData> &data) {
    if (!data) {
        return 0;
    }

    auto it = host.audioDataIds.find(data.get());
    if (it != host.audioDataIds.end()) {
        return it->second;
    }

    const std::uint64_t audioDataId = host.nextAudioDataId++;
    host.audioData[audioDataId] = {.data = data, .value = JS_UNDEFINED};
    host.audioDataIds[data.get()] = audioDataId;
    return audioDataId;
}

std::uint64_t
registerAudioSourceState(ScriptHost &host,
                         const std::shared_ptr<AudioSource> &ownedSource) {
    if (!ownedSource) {
        return 0;
    }

    auto it = host.audioSourceIds.find(ownedSource.get());
    if (it != host.audioSourceIds.end()) {
        return it->second;
    }

    const std::uint64_t audioSourceId = host.nextAudioSourceId++;
    host.audioSources[audioSourceId] = {.ownedSource = ownedSource,
                                        .source = ownedSource.get(),
                                        .value = JS_UNDEFINED};
    host.audioSourceIds[ownedSource.get()] = audioSourceId;
    return audioSourceId;
}

std::uint64_t registerAudioSourcePointer(ScriptHost &host,
                                         AudioSource *source) {
    if (source == nullptr) {
        return 0;
    }

    auto it = host.audioSourceIds.find(source);
    if (it != host.audioSourceIds.end()) {
        return it->second;
    }

    const std::uint64_t audioSourceId = host.nextAudioSourceId++;
    host.audioSources[audioSourceId] = {
        .ownedSource = nullptr, .source = source, .value = JS_UNDEFINED};
    host.audioSourceIds[source] = audioSourceId;
    return audioSourceId;
}

std::uint64_t registerReverbState(ScriptHost &host,
                                  const std::shared_ptr<Reverb> &effect) {
    if (!effect) {
        return 0;
    }

    auto it = host.reverbIds.find(effect.get());
    if (it != host.reverbIds.end()) {
        return it->second;
    }

    const std::uint64_t reverbId = host.nextReverbId++;
    host.reverbs[reverbId] = {.effect = effect, .value = JS_UNDEFINED};
    host.reverbIds[effect.get()] = reverbId;
    return reverbId;
}

std::uint64_t registerEchoState(ScriptHost &host,
                                const std::shared_ptr<Echo> &effect) {
    if (!effect) {
        return 0;
    }

    auto it = host.echoIds.find(effect.get());
    if (it != host.echoIds.end()) {
        return it->second;
    }

    const std::uint64_t echoId = host.nextEchoId++;
    host.echoes[echoId] = {.effect = effect, .value = JS_UNDEFINED};
    host.echoIds[effect.get()] = echoId;
    return echoId;
}

std::uint64_t
registerDistortionState(ScriptHost &host,
                        const std::shared_ptr<Distortion> &effect) {
    if (!effect) {
        return 0;
    }

    auto it = host.distortionIds.find(effect.get());
    if (it != host.distortionIds.end()) {
        return it->second;
    }

    const std::uint64_t distortionId = host.nextDistortionId++;
    host.distortions[distortionId] = {.effect = effect, .value = JS_UNDEFINED};
    host.distortionIds[effect.get()] = distortionId;
    return distortionId;
}

JSValue syncAudioEngineWrapper(JSContext *ctx, ScriptHost &host,
                               AudioEngine &engine) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.audioEngineValue)) {
        wrapper = JS_DupValue(ctx, host.audioEngineValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.audioEnginePrototype);
        host.audioEngineValue = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_AUDIO_ENGINE_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "deviceName",
                JS_NewString(ctx, engine.deviceName.c_str()));
    return wrapper;
}

JSValue syncAudioDataWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t audioDataId) {
    auto *state = findAudioDataState(host, audioDataId);
    if (state == nullptr || !state->data) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.audioDataPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_AUDIO_DATA_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(audioDataId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "isMono", JS_NewBool(ctx, state->data->isMono));
    setProperty(ctx, wrapper, "resource",
                makeResource(ctx, host, state->data->resource));
    return wrapper;
}

JSValue syncAudioSourceWrapper(JSContext *ctx, ScriptHost &host,
                               std::uint64_t audioSourceId) {
    auto *state = findAudioSourceState(host, audioSourceId);
    if (state == nullptr || state->source == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.audioSourcePrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_AUDIO_SOURCE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(audioSourceId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    return wrapper;
}

JSValue syncReverbWrapper(JSContext *ctx, ScriptHost &host,
                          std::uint64_t reverbId) {
    auto *state = findReverbState(host, reverbId);
    if (state == nullptr || !state->effect) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.reverbPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_REVERB_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(reverbId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    return wrapper;
}

JSValue syncEchoWrapper(JSContext *ctx, ScriptHost &host,
                        std::uint64_t echoId) {
    auto *state = findEchoState(host, echoId);
    if (state == nullptr || !state->effect) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.echoPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_ECHO_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(echoId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    return wrapper;
}

JSValue syncDistortionWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t distortionId) {
    auto *state = findDistortionState(host, distortionId);
    if (state == nullptr || !state->effect) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.distortionPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_DISTORTION_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(distortionId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    return wrapper;
}

std::uint64_t registerTextureState(ScriptHost &host, const Texture &texture) {
    const std::uint64_t textureId = host.nextTextureId++;
    host.textures[textureId] = {.texture = std::make_shared<Texture>(texture),
                                .value = JS_UNDEFINED};
    return textureId;
}

std::uint64_t registerCubemapState(ScriptHost &host, const Cubemap &cubemap) {
    const std::uint64_t cubemapId = host.nextCubemapId++;
    host.cubemaps[cubemapId] = {.cubemap = std::make_shared<Cubemap>(cubemap),
                                .value = JS_UNDEFINED};
    return cubemapId;
}

JSValue syncTextureWrapper(JSContext *ctx, ScriptHost &host,
                           std::uint64_t textureId) {
    auto *state = findTextureState(host, textureId);
    if (state == nullptr || !state->texture) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.texturePrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    const Resource resource = state->texture->resource;
    setProperty(ctx, wrapper, ATLAS_TEXTURE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(textureId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "type",
                JS_NewInt32(ctx, toScriptTextureType(state->texture->type)));
    setProperty(ctx, wrapper, "resource", makeResource(ctx, host, resource));
    setProperty(ctx, wrapper, "width",
                JS_NewInt32(ctx, state->texture->creationData.width));
    setProperty(ctx, wrapper, "height",
                JS_NewInt32(ctx, state->texture->creationData.height));
    setProperty(ctx, wrapper, "channels",
                JS_NewInt32(ctx, state->texture->creationData.channels));
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(state->texture->id)));
    setProperty(ctx, wrapper, "borderColor",
                makeColor(ctx, host, state->texture->borderColor));
    return wrapper;
}

JSValue syncCubemapWrapper(JSContext *ctx, ScriptHost &host,
                           std::uint64_t cubemapId) {
    auto *state = findCubemapState(host, cubemapId);
    if (state == nullptr || !state->cubemap) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.cubemapPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    JSValue resources = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < state->cubemap->resources.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, resources, i,
            makeResource(ctx, host, state->cubemap->resources[i]));
    }

    setProperty(ctx, wrapper, ATLAS_CUBEMAP_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(cubemapId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "resources", resources);
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(state->cubemap->id)));
    return wrapper;
}

JSValue syncSkyboxWrapper(JSContext *ctx, ScriptHost &host,
                          std::uint64_t skyboxId) {
    auto *state = findSkyboxState(host, skyboxId);
    if (state == nullptr || !state->skybox) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.skyboxPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_SKYBOX_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(skyboxId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    if (state->cubemapId != 0) {
        setProperty(ctx, wrapper, "cubemap",
                    syncCubemapWrapper(ctx, host, state->cubemapId));
    }
    return wrapper;
}

void syncRenderTargetTextureStates(ScriptHost &host,
                                   std::uint64_t renderTargetId) {
    auto *state = findRenderTargetState(host, renderTargetId);
    if (state == nullptr || !state->renderTarget) {
        return;
    }

    auto assignTexture = [&](std::uint64_t &slot, const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            slot = 0;
            return;
        }
        if (slot == 0) {
            slot = registerTextureState(host, texture);
            return;
        }
        auto *textureState = findTextureState(host, slot);
        if (textureState != nullptr && textureState->texture) {
            *textureState->texture = texture;
        }
    };

    std::vector<std::uint64_t> currentOutTextureIds = state->outTextureIds;
    state->outTextureIds.clear();
    std::size_t outTextureIndex = 0;
    auto pushTexture = [&](const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            return;
        }
        std::uint64_t slot = 0;
        if (outTextureIndex < currentOutTextureIds.size()) {
            slot = currentOutTextureIds[outTextureIndex];
        }
        assignTexture(slot, texture);
        if (slot != 0) {
            state->outTextureIds.push_back(slot);
        }
        outTextureIndex += 1;
    };

    switch (state->renderTarget->type) {
    case RenderTargetType::GBuffer:
        pushTexture(state->renderTarget->gPosition);
        pushTexture(state->renderTarget->gNormal);
        pushTexture(state->renderTarget->gAlbedoSpec);
        pushTexture(state->renderTarget->gMaterial);
        break;
    case RenderTargetType::Shadow:
    case RenderTargetType::CubeShadow:
        break;
    default:
        pushTexture(state->renderTarget->texture);
        if (state->renderTarget->brightTexture.id != 0 ||
            state->renderTarget->brightTexture.texture != nullptr) {
            pushTexture(state->renderTarget->brightTexture);
        }
        break;
    }

    if (state->renderTarget->type == RenderTargetType::Shadow ||
        state->renderTarget->type == RenderTargetType::CubeShadow) {
        assignTexture(state->depthTextureId, state->renderTarget->texture);
    } else if (state->renderTarget->depthTexture.id != 0 ||
               state->renderTarget->depthTexture.texture != nullptr) {
        assignTexture(state->depthTextureId, state->renderTarget->depthTexture);
    } else {
        state->depthTextureId = 0;
    }
}

JSValue syncRenderTargetWrapper(JSContext *ctx, ScriptHost &host,
                                std::uint64_t renderTargetId) {
    auto *state = findRenderTargetState(host, renderTargetId);
    if (state == nullptr || !state->renderTarget) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    syncRenderTargetTextureStates(host, renderTargetId);

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.renderTargetPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    JSValue outTextures = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < state->outTextureIds.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, outTextures, i,
            syncTextureWrapper(ctx, host, state->outTextureIds[i]));
    }

    setProperty(ctx, wrapper, ATLAS_RENDER_TARGET_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(renderTargetId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(
        ctx, wrapper, "type",
        JS_NewInt32(ctx, toScriptRenderTargetType(state->renderTarget->type)));
    setProperty(ctx, wrapper, "resolution",
                JS_NewInt32(ctx, state->renderTarget->getWidth()));
    setProperty(ctx, wrapper, "outTextures", outTextures);
    if (state->depthTextureId != 0) {
        setProperty(ctx, wrapper, "depthTexture",
                    syncTextureWrapper(ctx, host, state->depthTextureId));
    } else {
        setProperty(ctx, wrapper, "depthTexture", JS_NULL);
    }
    return wrapper;
}

void syncPointLightDebugObject(Light &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.debugObject->setColor(light.color);
}

void syncSpotLightDebugObject(Spotlight &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.setColor(light.color);
    light.updateDebugObjectRotation();
}

void syncAreaLightDebugObject(AreaLight &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.debugObject->lookAt(light.position + light.getNormal(), light.up);
    light.debugObject->material.albedo = light.color;
    light.debugObject->material.emissiveColor = light.color;
    light.debugObject->material.emissiveIntensity =
        std::clamp(light.intensity * 0.2f, 1.0f, 8.0f);
}

JSValue syncPointLightWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t lightId) {
    auto *state = findPointLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.pointLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_POINT_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    setProperty(ctx, wrapper, "distance",
                JS_NewFloat64(ctx, state->light->distance));
    return wrapper;
}

JSValue syncDirectionalLightWrapper(JSContext *ctx, ScriptHost &host,
                                    std::uint64_t lightId) {
    auto *state = findDirectionalLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.directionalLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_DIRECTIONAL_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "direction",
                makePosition3d(ctx, host, state->light->direction));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    return wrapper;
}

JSValue syncSpotLightWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t lightId) {
    auto *state = findSpotLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.spotLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    const double cutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(state->light->cutOff), -1.0, 1.0)));
    const double outerCutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(state->light->outerCutoff), -1.0, 1.0)));

    setProperty(ctx, wrapper, ATLAS_SPOT_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "direction",
                makePosition3d(ctx, host, state->light->direction));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "range", JS_NewFloat64(ctx, state->light->range));
    setProperty(ctx, wrapper, "cutOff", JS_NewFloat64(ctx, cutOff));
    setProperty(ctx, wrapper, "outerCutOff", JS_NewFloat64(ctx, outerCutOff));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    return wrapper;
}

JSValue syncAreaLightWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t lightId) {
    auto *state = findAreaLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.areaLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_AREA_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "right",
                makePosition3d(ctx, host, state->light->right));
    setProperty(ctx, wrapper, "up",
                makePosition3d(ctx, host, state->light->up));
    setProperty(ctx, wrapper, "size",
                makeSize2d(ctx, host, state->light->size));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    setProperty(ctx, wrapper, "range", JS_NewFloat64(ctx, state->light->range));
    setProperty(ctx, wrapper, "angle", JS_NewFloat64(ctx, state->light->angle));
    setProperty(ctx, wrapper, "castsBothSides",
                JS_NewBool(ctx, state->light->castsBothSides));
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, state->light->rotation));
    return wrapper;
}

ScriptWorleyNoiseState *findWorleyNoiseState(ScriptHost &host,
                                             std::uint64_t noiseId) {
    auto it = host.worleyNoise.find(noiseId);
    if (it == host.worleyNoise.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint64_t registerWorleyNoiseState(ScriptHost &host,
                                       std::shared_ptr<WorleyNoise3D> noise) {
    const std::uint64_t noiseId = host.nextWorleyNoiseId++;
    host.worleyNoise[noiseId] = {.noise = std::move(noise),
                                 .value = JS_UNDEFINED};
    return noiseId;
}

JSValue syncWorleyNoiseWrapper(JSContext *ctx, ScriptHost &host,
                               std::uint64_t noiseId) {
    auto *state = findWorleyNoiseState(host, noiseId);
    if (state == nullptr || !state->noise) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.worleyNoisePrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_WORLEY_NOISE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(noiseId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    return wrapper;
}

ScriptWorleyNoiseState *resolveWorleyNoise(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t noiseId = 0;
    if (!readIntProperty(ctx, value, ATLAS_WORLEY_NOISE_ID_PROP, noiseId)) {
        JS_ThrowTypeError(ctx, "Expected WorleyNoise3D handle");
        return nullptr;
    }

    auto *state =
        findWorleyNoiseState(host, static_cast<std::uint64_t>(noiseId));
    if (state == nullptr || !state->noise) {
        JS_ThrowReferenceError(ctx, "Unknown WorleyNoise3D id");
        return nullptr;
    }
    return state;
}

ScriptCloudsState *findCloudsState(ScriptHost &host, std::uint64_t cloudsId) {
    auto it = host.clouds.find(cloudsId);
    if (it == host.clouds.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint64_t registerCloudsState(ScriptHost &host,
                                  std::shared_ptr<Clouds> clouds) {
    if (!clouds) {
        return 0;
    }
    auto existing = host.cloudIds.find(clouds.get());
    if (existing != host.cloudIds.end()) {
        return existing->second;
    }

    const std::uint64_t cloudsId = host.nextCloudsId++;
    host.clouds[cloudsId] = {
        .ownedClouds = clouds, .clouds = clouds.get(), .value = JS_UNDEFINED};
    host.cloudIds[clouds.get()] = cloudsId;
    return cloudsId;
}

JSValue syncCloudsWrapper(JSContext *ctx, ScriptHost &host,
                          std::uint64_t cloudsId) {
    auto *state = findCloudsState(host, cloudsId);
    if (state == nullptr || state->clouds == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.cloudsPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_CLOUDS_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(cloudsId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->clouds->position));
    setProperty(ctx, wrapper, "size",
                makePosition3d(ctx, host, state->clouds->size));
    setProperty(ctx, wrapper, "scale",
                JS_NewFloat64(ctx, state->clouds->scale));
    setProperty(ctx, wrapper, "offset",
                makePosition3d(ctx, host, state->clouds->offset));
    setProperty(ctx, wrapper, "density",
                JS_NewFloat64(ctx, state->clouds->density));
    setProperty(ctx, wrapper, "densityMultiplier",
                JS_NewFloat64(ctx, state->clouds->densityMultiplier));
    setProperty(ctx, wrapper, "absorption",
                JS_NewFloat64(ctx, state->clouds->absorption));
    setProperty(ctx, wrapper, "scattering",
                JS_NewFloat64(ctx, state->clouds->scattering));
    setProperty(ctx, wrapper, "phase",
                JS_NewFloat64(ctx, state->clouds->phase));
    setProperty(ctx, wrapper, "clusterStrength",
                JS_NewFloat64(ctx, state->clouds->clusterStrength));
    setProperty(ctx, wrapper, "primaryStepCount",
                JS_NewInt32(ctx, state->clouds->primaryStepCount));
    setProperty(ctx, wrapper, "lightStepCount",
                JS_NewInt32(ctx, state->clouds->lightStepCount));
    setProperty(ctx, wrapper, "lightStepMultiplier",
                JS_NewFloat64(ctx, state->clouds->lightStepMultiplier));
    setProperty(ctx, wrapper, "minStepLength",
                JS_NewFloat64(ctx, state->clouds->minStepLength));
    setProperty(ctx, wrapper, "wind",
                makePosition3d(ctx, host, state->clouds->wind));
    return wrapper;
}

ScriptCloudsState *resolveClouds(JSContext *ctx, ScriptHost &host,
                                 JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t cloudsId = 0;
    if (!readIntProperty(ctx, value, ATLAS_CLOUDS_ID_PROP, cloudsId)) {
        JS_ThrowTypeError(ctx, "Expected Clouds handle");
        return nullptr;
    }

    auto *state = findCloudsState(host, static_cast<std::uint64_t>(cloudsId));
    if (state == nullptr || state->clouds == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Clouds id");
        return nullptr;
    }
    return state;
}

ScriptAtmosphereState *findAtmosphereState(ScriptHost &host,
                                           std::uint64_t atmosphereId) {
    auto it = host.atmospheres.find(atmosphereId);
    if (it == host.atmospheres.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint64_t registerAtmosphereState(ScriptHost &host,
                                      std::shared_ptr<Atmosphere> atmosphere) {
    if (!atmosphere) {
        return 0;
    }
    auto existing = host.atmosphereIds.find(atmosphere.get());
    if (existing != host.atmosphereIds.end()) {
        return existing->second;
    }

    const std::uint64_t atmosphereId = host.nextAtmosphereId++;
    host.atmospheres[atmosphereId] = {.ownedAtmosphere = atmosphere,
                                      .atmosphere = atmosphere.get(),
                                      .value = JS_UNDEFINED,
                                      .weatherDelegate = JS_UNDEFINED};
    host.atmosphereIds[atmosphere.get()] = atmosphereId;
    return atmosphereId;
}

JSValue syncAtmosphereWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t atmosphereId);

JSValue makeViewInformationValue(JSContext *ctx, ScriptHost &host,
                                 const ViewInformation &info) {
    JSValue value = JS_NewObject(ctx);
    setProperty(ctx, value, "position",
                makePosition3d(ctx, host, info.position));
    setProperty(ctx, value, "target", makePosition3d(ctx, host, info.target));
    setProperty(ctx, value, "time", JS_NewFloat64(ctx, info.time));
    setProperty(ctx, value, "deltaTime", JS_NewFloat64(ctx, info.deltaTime));
    return value;
}

bool parseWeatherState(JSContext *ctx, JSValueConst value, WeatherState &out) {
    if (JS_IsUndefined(value) || JS_IsNull(value)) {
        return false;
    }

    std::int64_t condition = static_cast<std::int64_t>(out.condition);
    if (readIntProperty(ctx, value, "condition", condition)) {
        out.condition = static_cast<WeatherCondition>(condition);
    }

    double intensity = out.intensity;
    if (readNumberProperty(ctx, value, "intensity", intensity)) {
        out.intensity = static_cast<float>(intensity);
    }

    JSValue wind = JS_GetPropertyStr(ctx, value, "wind");
    if (!JS_IsException(wind) && !JS_IsUndefined(wind) && !JS_IsNull(wind)) {
        parsePosition3d(ctx, wind, out.wind);
    }
    JS_FreeValue(ctx, wind);

    return true;
}

void bindAtmosphereWeatherDelegate(JSContext *ctx, ScriptHost &host,
                                   std::uint64_t atmosphereId) {
    auto *state = findAtmosphereState(host, atmosphereId);
    if (state == nullptr || state->atmosphere == nullptr) {
        return;
    }

    ScriptHost *hostPtr = &host;
    state->atmosphere->weatherDelegate = [ctx, hostPtr,
                                          atmosphereId](ViewInformation info) {
        if (hostPtr == nullptr) {
            return WeatherState();
        }
        auto *callbackState = findAtmosphereState(*hostPtr, atmosphereId);
        if (callbackState == nullptr ||
            JS_IsUndefined(callbackState->weatherDelegate)) {
            return WeatherState();
        }
        JSValue wrapper = syncAtmosphereWrapper(ctx, *hostPtr, atmosphereId);
        if (JS_IsException(wrapper) || JS_IsNull(wrapper)) {
            JS_FreeValue(ctx, wrapper);
            return WeatherState();
        }

        JSValue payload = makeViewInformationValue(ctx, *hostPtr, info);
        JSValue args[] = {payload};
        JSValue result = JS_Call(ctx, callbackState->weatherDelegate, wrapper,
                                 1, const_cast<JSValueConst *>(args));
        JS_FreeValue(ctx, payload);
        JS_FreeValue(ctx, wrapper);
        if (JS_IsException(result)) {
            runtime::scripting::dumpExecution(ctx);
            JS_FreeValue(ctx, result);
            return WeatherState();
        }

        WeatherState state;
        parseWeatherState(ctx, result, state);
        JS_FreeValue(ctx, result);
        return state;
    };
}

JSValue syncAtmosphereWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t atmosphereId) {
    auto *state = findAtmosphereState(host, atmosphereId);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.atmospherePrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_ATMOSPHERE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(atmosphereId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "timeOfDay",
                JS_NewFloat64(ctx, state->atmosphere->timeOfDay));
    setProperty(ctx, wrapper, "secondsPerHour",
                JS_NewFloat64(ctx, state->atmosphere->secondsPerHour));
    setProperty(ctx, wrapper, "wind",
                makePosition3d(ctx, host, state->atmosphere->wind));
    setProperty(ctx, wrapper, "sunColor",
                makeColor(ctx, host, state->atmosphere->sunColor));
    setProperty(ctx, wrapper, "moonColor",
                makeColor(ctx, host, state->atmosphere->moonColor));
    setProperty(ctx, wrapper, "sunSize",
                JS_NewFloat64(ctx, state->atmosphere->sunSize));
    setProperty(ctx, wrapper, "moonSize",
                JS_NewFloat64(ctx, state->atmosphere->moonSize));
    setProperty(ctx, wrapper, "sunTintStrength",
                JS_NewFloat64(ctx, state->atmosphere->sunTintStrength));
    setProperty(ctx, wrapper, "moonTintStrength",
                JS_NewFloat64(ctx, state->atmosphere->moonTintStrength));
    setProperty(ctx, wrapper, "starIntensity",
                JS_NewFloat64(ctx, state->atmosphere->starIntensity));
    setProperty(ctx, wrapper, "cycle",
                JS_NewBool(ctx, state->atmosphere->cycle));
    if (state->atmosphere->clouds) {
        setProperty(ctx, wrapper, "clouds",
                    syncCloudsWrapper(
                        ctx, host,
                        registerCloudsState(host, state->atmosphere->clouds)));
    } else {
        setProperty(ctx, wrapper, "clouds", JS_NULL);
    }
    if (!JS_IsUndefined(state->weatherDelegate)) {
        setProperty(ctx, wrapper, "weatherDelegate",
                    JS_DupValue(ctx, state->weatherDelegate));
    } else {
        setProperty(ctx, wrapper, "weatherDelegate", JS_NULL);
    }
    return wrapper;
}

ScriptAtmosphereState *resolveAtmosphere(JSContext *ctx, ScriptHost &host,
                                         JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t atmosphereId = 0;
    if (!readIntProperty(ctx, value, ATLAS_ATMOSPHERE_ID_PROP, atmosphereId)) {
        JS_ThrowTypeError(ctx, "Expected Atmosphere handle");
        return nullptr;
    }

    auto *state =
        findAtmosphereState(host, static_cast<std::uint64_t>(atmosphereId));
    if (state == nullptr || state->atmosphere == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atmosphere id");
        return nullptr;
    }
    return state;
}

ScriptFontState *findFontState(ScriptHost &host, std::uint64_t fontId) {
    auto it = host.fonts.find(fontId);
    if (it == host.fonts.end()) {
        return nullptr;
    }
    return &it->second;
}

Texture makeAtlasTexture(const Font &font) {
    Texture atlasTexture;
    atlasTexture.resource = font.resource;
    atlasTexture.texture = font.texture;
    atlasTexture.type = TextureType::Color;
    if (font.texture != nullptr) {
        atlasTexture.id = font.texture->textureID;
        atlasTexture.creationData.width = font.texture->width;
        atlasTexture.creationData.height = font.texture->height;
        atlasTexture.creationData.channels = 1;
    }
    return atlasTexture;
}

std::uint64_t registerFontState(ScriptHost &host, const Font &font) {
    const std::uint64_t fontId = host.nextFontId++;
    auto state = ScriptFontState{
        .font = std::make_shared<Font>(font),
        .value = JS_UNDEFINED,
        .textureId = 0,
    };
    if (state.font->texture != nullptr) {
        state.textureId =
            registerTextureState(host, makeAtlasTexture(*state.font));
    }
    host.fonts[fontId] = std::move(state);
    return fontId;
}

JSValue syncFontWrapper(JSContext *ctx, ScriptHost &host,
                        std::uint64_t fontId) {
    auto *state = findFontState(host, fontId);
    if (state == nullptr || !state->font) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    if (state->font->texture != nullptr) {
        if (state->textureId == 0) {
            state->textureId =
                registerTextureState(host, makeAtlasTexture(*state->font));
        } else if (auto *textureState =
                       findTextureState(host, state->textureId);
                   textureState != nullptr &&
                   textureState->texture != nullptr) {
            *textureState->texture = makeAtlasTexture(*state->font);
        }
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.fontPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_FONT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(fontId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "name",
                JS_NewString(ctx, state->font->name.c_str()));
    setProperty(ctx, wrapper, "size", JS_NewInt32(ctx, state->font->size));
    setProperty(ctx, wrapper, "resource",
                makeResource(ctx, host, state->font->resource));
    if (state->textureId != 0) {
        JSValue texture = syncTextureWrapper(ctx, host, state->textureId);
        setProperty(ctx, wrapper, "atlas", JS_DupValue(ctx, texture));
        setProperty(ctx, wrapper, "texture", texture);
    } else {
        setProperty(ctx, wrapper, "atlas", JS_NULL);
        setProperty(ctx, wrapper, "texture", JS_NULL);
    }
    return wrapper;
}

ScriptFontState *resolveFont(JSContext *ctx, ScriptHost &host,
                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t fontId = 0;
    if (!readIntProperty(ctx, value, ATLAS_FONT_ID_PROP, fontId)) {
        JS_ThrowTypeError(ctx, "Expected Graphite font handle");
        return nullptr;
    }

    auto *state = findFontState(host, static_cast<std::uint64_t>(fontId));
    if (state == nullptr || !state->font) {
        JS_ThrowReferenceError(ctx, "Unknown Graphite font id");
        return nullptr;
    }
    return state;
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host, GameObject &object);
GameObject *resolveObjectArg(JSContext *ctx, ScriptHost &host,
                             JSValueConst value);

JSValue makeUIStyleVariantValue(JSContext *ctx, ScriptHost &host,
                                const graphite::UIStyleVariant &variant) {
    JSValue value = newObjectFromPrototype(ctx, host.uiStyleVariantPrototype);
    if (variant.paddingValue.has_value()) {
        setProperty(ctx, value, "paddingValue",
                    makeSize2d(ctx, host, *variant.paddingValue));
    }
    if (variant.cornerRadiusValue.has_value()) {
        setProperty(ctx, value, "cornerRadiusValue",
                    JS_NewFloat64(ctx, *variant.cornerRadiusValue));
    }
    if (variant.borderWidthValue.has_value()) {
        setProperty(ctx, value, "borderWidthValue",
                    JS_NewFloat64(ctx, *variant.borderWidthValue));
    }
    if (variant.backgroundColorValue.has_value()) {
        setProperty(ctx, value, "backgroundColorValue",
                    makeColor(ctx, host, *variant.backgroundColorValue));
    }
    if (variant.borderColorValue.has_value()) {
        setProperty(ctx, value, "borderColorValue",
                    makeColor(ctx, host, *variant.borderColorValue));
    }
    if (variant.foregroundColorValue.has_value()) {
        setProperty(ctx, value, "foregroundColorValue",
                    makeColor(ctx, host, *variant.foregroundColorValue));
    }
    if (variant.tintColorValue.has_value()) {
        setProperty(ctx, value, "tintColorValue",
                    makeColor(ctx, host, *variant.tintColorValue));
    }
    if (variant.fontValue.has_value() && *variant.fontValue != nullptr) {
        setProperty(
            ctx, value, "fontValue",
            syncFontWrapper(ctx, host,
                            registerFontState(host, *(*variant.fontValue))));
    }
    if (variant.fontSizeValue.has_value()) {
        setProperty(ctx, value, "fontSizeValue",
                    JS_NewFloat64(ctx, *variant.fontSizeValue));
    }
    return value;
}

JSValue makeUIStyleValue(JSContext *ctx, ScriptHost &host,
                         const graphite::UIStyle &style) {
    JSValue value = newObjectFromPrototype(ctx, host.uiStylePrototype);
    setProperty(ctx, value, "__normal",
                makeUIStyleVariantValue(ctx, host, style.normal()));
    setProperty(ctx, value, "__hovered",
                makeUIStyleVariantValue(ctx, host, style.hovered()));
    setProperty(ctx, value, "__pressed",
                makeUIStyleVariantValue(ctx, host, style.pressed()));
    setProperty(ctx, value, "__disabled",
                makeUIStyleVariantValue(ctx, host, style.disabled()));
    setProperty(ctx, value, "__focused",
                makeUIStyleVariantValue(ctx, host, style.focused()));
    setProperty(ctx, value, "__checked",
                makeUIStyleVariantValue(ctx, host, style.checked()));
    return value;
}

JSValue makeThemeValue(JSContext *ctx, ScriptHost &host,
                       const graphite::Theme &theme) {
    JSValue value = newObjectFromPrototype(ctx, host.themePrototype);
    setProperty(ctx, value, "text", makeUIStyleValue(ctx, host, theme.text));
    setProperty(ctx, value, "image", makeUIStyleValue(ctx, host, theme.image));
    setProperty(ctx, value, "textField",
                makeUIStyleValue(ctx, host, theme.textField));
    setProperty(ctx, value, "button",
                makeUIStyleValue(ctx, host, theme.button));
    setProperty(ctx, value, "checkbox",
                makeUIStyleValue(ctx, host, theme.checkbox));
    setProperty(ctx, value, "row", makeUIStyleValue(ctx, host, theme.row));
    setProperty(ctx, value, "column",
                makeUIStyleValue(ctx, host, theme.column));
    setProperty(ctx, value, "stack", makeUIStyleValue(ctx, host, theme.stack));
    return value;
}

bool parseUIStyleVariant(JSContext *ctx, ScriptHost &host, JSValueConst value,
                         graphite::UIStyleVariant &out) {
    JSValue field = JS_GetPropertyStr(ctx, value, "paddingValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        Size2d padding;
        if (parseSize2d(ctx, field, padding)) {
            out.paddingValue = padding;
        }
    }
    JS_FreeValue(ctx, field);

    double number = 0.0;
    if (readNumberProperty(ctx, value, "cornerRadiusValue", number)) {
        out.cornerRadiusValue = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, value, "borderWidthValue", number)) {
        out.borderWidthValue = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, value, "fontSizeValue", number)) {
        out.fontSizeValue = static_cast<float>(number);
    }

    field = JS_GetPropertyStr(ctx, value, "backgroundColorValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        Color color;
        if (parseColor(ctx, field, color)) {
            out.backgroundColorValue = color;
        }
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "borderColorValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        Color color;
        if (parseColor(ctx, field, color)) {
            out.borderColorValue = color;
        }
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "foregroundColorValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        Color color;
        if (parseColor(ctx, field, color)) {
            out.foregroundColorValue = color;
        }
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "tintColorValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        Color color;
        if (parseColor(ctx, field, color)) {
            out.tintColorValue = color;
        }
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "fontValue");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        auto *fontState = resolveFont(ctx, host, field);
        if (fontState != nullptr && fontState->font) {
            out.fontValue = fontState->font.get();
        }
    }
    JS_FreeValue(ctx, field);

    return true;
}

bool parseUIStyle(JSContext *ctx, ScriptHost &host, JSValueConst value,
                  graphite::UIStyle &out) {
    JSValue field = JS_GetPropertyStr(ctx, value, "__normal");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.normal());
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "__hovered");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.hovered());
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "__pressed");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.pressed());
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "__disabled");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.disabled());
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "__focused");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.focused());
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "__checked");
    if (!JS_IsException(field) && !JS_IsUndefined(field) && !JS_IsNull(field)) {
        parseUIStyleVariant(ctx, host, field, out.checked());
    }
    JS_FreeValue(ctx, field);

    return true;
}

bool parseTheme(JSContext *ctx, ScriptHost &host, JSValueConst value,
                graphite::Theme &out) {
    JSValue field = JS_GetPropertyStr(ctx, value, "text");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.text);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "image");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.image);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "textField");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.textField);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "button");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.button);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "checkbox");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.checkbox);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "row");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.row);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "column");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.column);
    }
    JS_FreeValue(ctx, field);
    field = JS_GetPropertyStr(ctx, value, "stack");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseUIStyle(ctx, host, field, out.stack);
    }
    JS_FreeValue(ctx, field);
    return true;
}

bool callGraphiteCallback(JSContext *ctx, ScriptHost &host, GameObject &object,
                          const char *propName, int argc, JSValue *argv) {
    JSValue wrapper = syncObjectWrapper(ctx, host, object);
    if (JS_IsException(wrapper) || JS_IsNull(wrapper)) {
        for (int i = 0; i < argc; ++i) {
            JS_FreeValue(ctx, argv[i]);
        }
        JS_FreeValue(ctx, wrapper);
        return false;
    }
    JSValue callback = JS_GetPropertyStr(ctx, wrapper, propName);
    if (JS_IsException(callback) || !JS_IsFunction(ctx, callback)) {
        for (int i = 0; i < argc; ++i) {
            JS_FreeValue(ctx, argv[i]);
        }
        JS_FreeValue(ctx, callback);
        JS_FreeValue(ctx, wrapper);
        return false;
    }
    JSValue result =
        JS_Call(ctx, callback, wrapper, argc, const_cast<JSValueConst *>(argv));
    const bool ok = !JS_IsException(result);
    for (int i = 0; i < argc; ++i) {
        JS_FreeValue(ctx, argv[i]);
    }
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, callback);
    JS_FreeValue(ctx, wrapper);
    return ok;
}

JSValue makeBiomeValue(JSContext *ctx, ScriptHost &host, const Biome &biome) {
    ensureBuiltins(ctx, host);
    JSValue result = newObjectFromPrototype(ctx, host.biomePrototype);
    setProperty(ctx, result, "name", JS_NewString(ctx, biome.name.c_str()));
    setProperty(ctx, result, "color", makeColor(ctx, host, biome.color));
    setProperty(ctx, result, "useTexture", JS_NewBool(ctx, biome.useTexture));
    setProperty(ctx, result, "minHeight", JS_NewFloat64(ctx, biome.minHeight));
    setProperty(ctx, result, "maxHeight", JS_NewFloat64(ctx, biome.maxHeight));
    setProperty(ctx, result, "minMoisture",
                JS_NewFloat64(ctx, biome.minMoisture));
    setProperty(ctx, result, "maxMoisture",
                JS_NewFloat64(ctx, biome.maxMoisture));
    setProperty(ctx, result, "minTemperature",
                JS_NewFloat64(ctx, biome.minTemperature));
    setProperty(ctx, result, "maxTemperature",
                JS_NewFloat64(ctx, biome.maxTemperature));
    if (biome.texture.texture != nullptr || biome.texture.id != 0) {
        const std::uint64_t textureId =
            registerTextureState(host, biome.texture);
        setProperty(ctx, result, "texture",
                    syncTextureWrapper(ctx, host, textureId));
    } else {
        setProperty(ctx, result, "texture", JS_NULL);
    }
    return result;
}

JSValue
makeTerrainGeneratorValue(JSContext *ctx, ScriptHost &host,
                          const std::shared_ptr<TerrainGenerator> &generator) {
    ensureBuiltins(ctx, host);
    if (!generator) {
        return JS_NULL;
    }

    JSValueConst prototype = host.terrainGeneratorPrototype;
    if (auto hill = std::dynamic_pointer_cast<HillGenerator>(generator)) {
        JSValue result =
            newObjectFromPrototype(ctx, host.hillGeneratorPrototype);
        setProperty(ctx, result, "algorithm", JS_NewString(ctx, "hill"));
        setProperty(ctx, result, "type", JS_NewString(ctx, "hill"));
        setProperty(ctx, result, "scale", JS_NewFloat64(ctx, hill->getScale()));
        setProperty(ctx, result, "amplitude",
                    JS_NewFloat64(ctx, hill->getAmplitude()));
        return result;
    }
    if (auto mountain =
            std::dynamic_pointer_cast<MountainGenerator>(generator)) {
        JSValue result =
            newObjectFromPrototype(ctx, host.mountainGeneratorPrototype);
        setProperty(ctx, result, "algorithm", JS_NewString(ctx, "mountain"));
        setProperty(ctx, result, "type", JS_NewString(ctx, "mountain"));
        setProperty(ctx, result, "scale",
                    JS_NewFloat64(ctx, mountain->getScale()));
        setProperty(ctx, result, "amplitude",
                    JS_NewFloat64(ctx, mountain->getAmplitude()));
        setProperty(ctx, result, "octaves",
                    JS_NewInt32(ctx, mountain->getOctaves()));
        setProperty(ctx, result, "persistence",
                    JS_NewFloat64(ctx, mountain->getPersistence()));
        setProperty(ctx, result, "persistance",
                    JS_NewFloat64(ctx, mountain->getPersistence()));
        return result;
    }
    if (auto plain = std::dynamic_pointer_cast<PlainGenerator>(generator)) {
        JSValue result =
            newObjectFromPrototype(ctx, host.plainGeneratorPrototype);
        setProperty(ctx, result, "algorithm", JS_NewString(ctx, "plain"));
        setProperty(ctx, result, "type", JS_NewString(ctx, "plain"));
        setProperty(ctx, result, "scale",
                    JS_NewFloat64(ctx, plain->getScale()));
        setProperty(ctx, result, "amplitude",
                    JS_NewFloat64(ctx, plain->getAmplitude()));
        return result;
    }
    if (auto island = std::dynamic_pointer_cast<IslandGenerator>(generator)) {
        JSValue result =
            newObjectFromPrototype(ctx, host.islandGeneratorPrototype);
        setProperty(ctx, result, "algorithm", JS_NewString(ctx, "island"));
        setProperty(ctx, result, "type", JS_NewString(ctx, "island"));
        setProperty(ctx, result, "numFeatures",
                    JS_NewInt32(ctx, island->getNumFeatures()));
        setProperty(ctx, result, "scale",
                    JS_NewFloat64(ctx, island->getScale()));
        return result;
    }
    if (auto compound =
            std::dynamic_pointer_cast<CompoundGenerator>(generator)) {
        JSValue result =
            newObjectFromPrototype(ctx, host.compoundGeneratorPrototype);
        setProperty(ctx, result, "algorithm", JS_NewString(ctx, "compound"));
        setProperty(ctx, result, "type", JS_NewString(ctx, "compound"));
        JSValue children = JS_NewArray(ctx);
        const auto &generators = compound->getGenerators();
        for (std::uint32_t i = 0; i < generators.size(); ++i) {
            JS_SetPropertyUint32(
                ctx, children, i,
                makeTerrainGeneratorValue(ctx, host, generators[i]));
        }
        setProperty(ctx, result, "generators", children);
        return result;
    }

    return newObjectFromPrototype(ctx, prototype);
}

JSValue makeMonitorValue(JSContext *ctx, ScriptHost &host,
                         const Monitor &monitor) {
    ensureBuiltins(ctx, host);
    JSValue wrapper = newObjectFromPrototype(ctx, host.monitorPrototype);
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "monitorId", JS_NewInt32(ctx, monitor.monitorID));
    setProperty(ctx, wrapper, "primary", JS_NewBool(ctx, monitor.primary));
    return wrapper;
}

JSValue makeGamepadValue(JSContext *ctx, ScriptHost &host,
                         const Gamepad &gamepad) {
    ensureBuiltins(ctx, host);
    JSValue wrapper = newObjectFromPrototype(ctx, host.gamepadPrototype);
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "controllerId",
                JS_NewInt32(ctx, gamepad.controllerID));
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, gamepad.name.c_str()));
    setProperty(ctx, wrapper, "connected", JS_NewBool(ctx, gamepad.connected));
    return wrapper;
}

JSValue makeJoystickValue(JSContext *ctx, ScriptHost &host,
                          const Joystick &joystick) {
    ensureBuiltins(ctx, host);
    JSValue wrapper = newObjectFromPrototype(ctx, host.joystickPrototype);
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "joystickId",
                JS_NewInt32(ctx, joystick.joystickID));
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, joystick.name.c_str()));
    setProperty(ctx, wrapper, "connected", JS_NewBool(ctx, joystick.connected));
    return wrapper;
}

JSValue syncWindowWrapper(JSContext *ctx, ScriptHost &host, Window &window) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.windowValue)) {
        wrapper = JS_DupValue(ctx, host.windowValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.windowPrototype);
        host.windowValue = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_WINDOW_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "_title",
                JS_NewString(ctx, window.title.c_str()));
    setProperty(ctx, wrapper, "_width", JS_NewInt32(ctx, window.width));
    setProperty(ctx, wrapper, "_height", JS_NewInt32(ctx, window.height));
    setProperty(ctx, wrapper, "_currentFrame",
                JS_NewInt32(ctx, window.currentFrame));
    setProperty(ctx, wrapper, "_gravity", JS_NewFloat64(ctx, window.gravity));
    setProperty(ctx, wrapper, "_usesDeferred",
                JS_NewBool(ctx, window.usesDeferred));
    if (window.audioEngine == nullptr) {
        window.audioEngine = std::make_shared<AudioEngine>();
        window.audioEngine->initialize();
    }
    if (window.audioEngine != nullptr) {
        setProperty(ctx, wrapper, "audioEngine",
                    syncAudioEngineWrapper(ctx, host, *window.audioEngine));
    } else {
        setProperty(ctx, wrapper, "audioEngine", JS_NULL);
    }
    return wrapper;
}

JSValue syncSceneWrapper(JSContext *ctx, ScriptHost &host, Scene &scene) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.sceneValue)) {
        wrapper = JS_DupValue(ctx, host.sceneValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.scenePrototype);
        host.sceneValue = JS_DupValue(ctx, wrapper);
    }

    std::string name;
    if (host.context != nullptr) {
        name = host.context->currentSceneName;
    }

    setProperty(ctx, wrapper, ATLAS_SCENE_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, name.c_str()));
    setProperty(ctx, wrapper, "atmosphere",
                syncAtmosphereWrapper(
                    ctx, host,
                    registerAtmosphereState(
                        host, std::shared_ptr<Atmosphere>(
                                  &scene.atmosphere, [](Atmosphere *) {}))));
    return wrapper;
}

Texture createEmptyTexture(int width, int height, TextureType type,
                           Color borderColor) {
    opal::TextureFormat format = opal::TextureFormat::Rgba8;
    opal::TextureDataFormat dataFormat = opal::TextureDataFormat::Rgba;

    switch (type) {
    case TextureType::Depth:
    case TextureType::DepthCube:
        format = opal::TextureFormat::DepthComponent24;
        dataFormat = opal::TextureDataFormat::DepthComponent;
        break;
    case TextureType::HDR:
        format = opal::TextureFormat::Rgba16F;
        dataFormat = opal::TextureDataFormat::Rgba;
        break;
    case TextureType::AO:
    case TextureType::Opacity:
    case TextureType::SSAONoise:
    case TextureType::SSAO:
        format = opal::TextureFormat::Red8;
        dataFormat = opal::TextureDataFormat::Red;
        break;
    default:
        break;
    }

    return Texture::create(width, height, format, dataFormat, type, {},
                           borderColor);
}

Texture createSolidTexture(Color color, TextureType type, int width,
                           int height) {
    const int checkSize = std::max(width, height);
    Texture texture =
        Texture::createCheckerboard(width, height, checkSize, color, color);
    texture.type = type;
    return texture;
}

std::shared_ptr<Effect> parseEffectValue(JSContext *ctx, JSValueConst value) {
    std::string type;
    if (JS_IsString(value)) {
        const char *name = JS_ToCString(ctx, value);
        if (name == nullptr) {
            return nullptr;
        }
        type = name;
        JS_FreeCString(ctx, name);
    } else {
        if (!readStringProperty(ctx, value, "type", type)) {
            return nullptr;
        }
    }

    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "inversion" || normalizedType == "invert") {
        return Inversion::create();
    }
    if (normalizedType == "grayscale") {
        return Grayscale::create();
    }
    if (normalizedType == "sharpen") {
        return Sharpen::create();
    }
    if (normalizedType == "blur") {
        double magnitude = 16.0;
        if (!JS_IsString(value)) {
            readNumberProperty(ctx, value, "magnitude", magnitude);
        }
        return Blur::create(static_cast<float>(magnitude));
    }
    if (normalizedType == "edgedetection") {
        return EdgeDetection::create();
    }
    if (normalizedType == "colorcorrection") {
        ColorCorrectionParameters params;
        if (!JS_IsString(value)) {
            double exposure = params.exposure;
            double contrast = params.contrast;
            double saturation = params.saturation;
            double gamma = params.gamma;
            double temperature = params.temperature;
            double tint = params.tint;
            readNumberProperty(ctx, value, "exposure", exposure);
            readNumberProperty(ctx, value, "contrast", contrast);
            readNumberProperty(ctx, value, "saturation", saturation);
            readNumberProperty(ctx, value, "gamma", gamma);
            readNumberProperty(ctx, value, "temperature", temperature);
            readNumberProperty(ctx, value, "tint", tint);
            params.exposure = static_cast<float>(exposure);
            params.contrast = static_cast<float>(contrast);
            params.saturation = static_cast<float>(saturation);
            params.gamma = static_cast<float>(gamma);
            params.temperature = static_cast<float>(temperature);
            params.tint = static_cast<float>(tint);
        }
        return ColorCorrection::create(params);
    }
    if (normalizedType == "motionblur") {
        MotionBlurParameters params;
        if (!JS_IsString(value)) {
            std::int64_t size = params.size;
            JSValue sizeValue = JS_GetPropertyStr(ctx, value, "size");
            if (!JS_IsException(sizeValue)) {
                getInt64(ctx, sizeValue, size);
            }
            JS_FreeValue(ctx, sizeValue);
            double separation = params.separation;
            readNumberProperty(ctx, value, "separation", separation);
            params.size = static_cast<int>(size);
            params.separation = static_cast<float>(separation);
        }
        return MotionBlur::create(params);
    }
    if (normalizedType == "chromaticaberration") {
        ChromaticAberrationParameters params;
        if (!JS_IsString(value)) {
            double red = params.red;
            double green = params.green;
            double blue = params.blue;
            readNumberProperty(ctx, value, "red", red);
            readNumberProperty(ctx, value, "green", green);
            readNumberProperty(ctx, value, "blue", blue);
            params.red = static_cast<float>(red);
            params.green = static_cast<float>(green);
            params.blue = static_cast<float>(blue);
            JSValue direction = JS_GetPropertyStr(ctx, value, "direction");
            if (!JS_IsException(direction) && !JS_IsUndefined(direction)) {
                parsePosition2d(ctx, direction, params.direction);
            }
            JS_FreeValue(ctx, direction);
        }
        return ChromaticAberration::create(params);
    }
    if (normalizedType == "posterization") {
        PosterizationParameters params;
        if (!JS_IsString(value)) {
            double levels = params.levels;
            readNumberProperty(ctx, value, "levels", levels);
            params.levels = static_cast<float>(levels);
        }
        return Posterization::create(params);
    }
    if (normalizedType == "pixelation") {
        PixelationParameters params;
        if (!JS_IsString(value)) {
            std::int64_t pixelSize = params.pixelSize;
            JSValue pixelSizeValue = JS_GetPropertyStr(ctx, value, "pixelSize");
            if (!JS_IsException(pixelSizeValue)) {
                getInt64(ctx, pixelSizeValue, pixelSize);
            }
            JS_FreeValue(ctx, pixelSizeValue);
            params.pixelSize = static_cast<int>(pixelSize);
        }
        return Pixelation::create(params);
    }
    if (normalizedType == "dialation" || normalizedType == "dilation") {
        DilationParameters params;
        if (!JS_IsString(value)) {
            std::int64_t size = params.size;
            JSValue sizeValue = JS_GetPropertyStr(ctx, value, "size");
            if (!JS_IsException(sizeValue)) {
                getInt64(ctx, sizeValue, size);
            }
            JS_FreeValue(ctx, sizeValue);
            double separation = params.separation;
            readNumberProperty(ctx, value, "separation", separation);
            params.size = static_cast<int>(size);
            params.separation = static_cast<float>(separation);
        }
        return Dilation::create(params);
    }
    if (normalizedType == "filmgrain") {
        FilmGrainParameters params;
        if (!JS_IsString(value)) {
            double amount = params.amount;
            readNumberProperty(ctx, value, "amount", amount);
            params.amount = static_cast<float>(amount);
        }
        return FilmGrain::create(params);
    }

    return nullptr;
}

JSValue syncCameraWrapper(JSContext *ctx, ScriptHost &host, Camera &camera) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.cameraValue)) {
        wrapper = JS_DupValue(ctx, host.cameraValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.cameraPrototype);
        host.cameraValue = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_CAMERA_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, camera.position));
    setProperty(ctx, wrapper, "target",
                makePosition3d(ctx, host, camera.target));
    setProperty(ctx, wrapper, "fov", JS_NewFloat64(ctx, camera.fov));
    setProperty(ctx, wrapper, "nearClip", JS_NewFloat64(ctx, camera.nearClip));
    setProperty(ctx, wrapper, "farClip", JS_NewFloat64(ctx, camera.farClip));
    setProperty(ctx, wrapper, "orthographicSize",
                JS_NewFloat64(ctx, camera.orthographicSize));
    setProperty(ctx, wrapper, "movementSpeed",
                JS_NewFloat64(ctx, camera.movementSpeed));
    setProperty(ctx, wrapper, "mouseSensitivity",
                JS_NewFloat64(ctx, camera.mouseSensitivity));
    setProperty(ctx, wrapper, "controllerLookSensitivity",
                JS_NewFloat64(ctx, camera.controllerLookSensitivity));
    setProperty(ctx, wrapper, "lookSmoothness",
                JS_NewFloat64(ctx, camera.lookSmoothness));
    setProperty(ctx, wrapper, "useOrthographic",
                JS_NewBool(ctx, camera.useOrthographic));
    setProperty(ctx, wrapper, "focusDepth",
                JS_NewFloat64(ctx, camera.focusDepth));
    setProperty(ctx, wrapper, "focusRange",
                JS_NewFloat64(ctx, camera.focusRange));
    return wrapper;
}

bool applyCamera(JSContext *ctx, JSValueConst wrapper, Camera &camera) {
    Position3d position = camera.position;
    Point3d target = camera.target;
    double fov = camera.fov;
    double nearClip = camera.nearClip;
    double farClip = camera.farClip;
    double orthographicSize = camera.orthographicSize;
    double movementSpeed = camera.movementSpeed;
    double mouseSensitivity = camera.mouseSensitivity;
    double controllerLookSensitivity = camera.controllerLookSensitivity;
    double lookSmoothness = camera.lookSmoothness;
    bool useOrthographic = camera.useOrthographic;
    double focusDepth = camera.focusDepth;
    double focusRange = camera.focusRange;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "target");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, target);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "fov", fov);
    readNumberProperty(ctx, wrapper, "nearClip", nearClip);
    readNumberProperty(ctx, wrapper, "farClip", farClip);
    readNumberProperty(ctx, wrapper, "orthographicSize", orthographicSize);
    readNumberProperty(ctx, wrapper, "movementSpeed", movementSpeed);
    readNumberProperty(ctx, wrapper, "mouseSensitivity", mouseSensitivity);
    readNumberProperty(ctx, wrapper, "controllerLookSensitivity",
                       controllerLookSensitivity);
    readNumberProperty(ctx, wrapper, "lookSmoothness", lookSmoothness);
    readBoolProperty(ctx, wrapper, "useOrthographic", useOrthographic);
    readNumberProperty(ctx, wrapper, "focusDepth", focusDepth);
    readNumberProperty(ctx, wrapper, "focusRange", focusRange);

    camera.setPosition(position);
    if (target.x != position.x || target.y != position.y ||
        target.z != position.z) {
        camera.lookAt(target);
    } else {
        camera.target = target;
    }
    camera.fov = static_cast<float>(fov);
    camera.nearClip = static_cast<float>(nearClip);
    camera.farClip = static_cast<float>(farClip);
    camera.orthographicSize = static_cast<float>(orthographicSize);
    camera.movementSpeed = static_cast<float>(movementSpeed);
    camera.mouseSensitivity = static_cast<float>(mouseSensitivity);
    camera.controllerLookSensitivity =
        static_cast<float>(controllerLookSensitivity);
    camera.lookSmoothness = static_cast<float>(lookSmoothness);
    camera.useOrthographic = useOrthographic;
    camera.focusDepth = static_cast<float>(focusDepth);
    camera.focusRange = static_cast<float>(focusRange);

    return true;
}

bool parseCoreVertex(JSContext *ctx, JSValueConst value, CoreVertex &out) {
    JSValue prop = JS_GetPropertyStr(ctx, value, "position");
    if (JS_IsException(prop) || !parsePosition3d(ctx, prop, out.position)) {
        JS_FreeValue(ctx, prop);
        return false;
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "color");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parseColor(ctx, prop, out.color);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "textureCoord");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        double x = 0.0;
        double y = 0.0;
        if (readNumberProperty(ctx, prop, "x", x) &&
            readNumberProperty(ctx, prop, "y", y)) {
            out.textureCoordinate = {static_cast<float>(x),
                                     static_cast<float>(y)};
        }
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "normal");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.normal);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "tangent");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.tangent);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "bitangent");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.bitangent);
    }
    JS_FreeValue(ctx, prop);

    return true;
}

JSValue buildComponentsArray(JSContext *ctx, ScriptHost &host, int ownerId) {
    JSValue result = JS_NewArray(ctx);
    std::uint32_t writeIndex = 0;

    auto orderIt = host.componentOrder.find(ownerId);
    if (orderIt == host.componentOrder.end()) {
        return result;
    }

    for (std::uint64_t componentId : orderIt->second) {
        auto stateIt = host.componentStates.find(componentId);
        if (stateIt == host.componentStates.end()) {
            continue;
        }
        if (stateIt->second.ownerId != ownerId) {
            continue;
        }
        JS_SetPropertyUint32(ctx, result, writeIndex++,
                             JS_DupValue(ctx, stateIt->second.value));
    }

    return result;
}

void trackObjectState(ScriptHost &host, GameObject &object, bool attached) {
    auto &state = host.objectStates[object.getId()];
    state.object = &object;
    state.attachedToWindow = state.attachedToWindow || attached;
}

void attachObjectIfReady(ScriptHost &host, GameObject &object) {
    if (host.context == nullptr || host.context->window == nullptr) {
        return;
    }
    if (dynamic_cast<UIObject *>(&object) != nullptr) {
        return;
    }

    auto &state = host.objectStates[object.getId()];
    state.object = &object;
    if (state.attachedToWindow) {
        return;
    }

    if (auto *core = dynamic_cast<CoreObject *>(&object);
        core != nullptr && core->vertices.empty()) {
        return;
    }

    if (auto *terrain = dynamic_cast<Terrain *>(&object); terrain != nullptr) {
        if (terrain->createdWithMap) {
            if (terrain->heightmap.path.empty()) {
                return;
            }
        } else if (terrain->generator == nullptr) {
            return;
        }
    }
    if (auto *fluid = dynamic_cast<Fluid *>(&object); fluid != nullptr) {
        if (!fluid->isCreated()) {
            return;
        }
    }

    object.initialize();
    host.context->window->addObject(&object);
    state.attachedToWindow = true;
}

void syncObjectTextureStates(ScriptHost &host, CoreObject &object) {
    auto &state = host.objectStates[object.getId()];
    std::vector<std::uint64_t> currentTextureIds = state.textureIds;
    state.textureIds.clear();

    auto assignTexture = [&](std::uint64_t &slot, const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            slot = 0;
            return;
        }
        if (slot == 0) {
            slot = registerTextureState(host, texture);
            return;
        }
        auto *textureState = findTextureState(host, slot);
        if (textureState != nullptr && textureState->texture) {
            *textureState->texture = texture;
            return;
        }
        slot = registerTextureState(host, texture);
    };

    for (std::size_t i = 0; i < object.textures.size(); ++i) {
        std::uint64_t slot = 0;
        if (i < currentTextureIds.size()) {
            slot = currentTextureIds[i];
        }
        assignTexture(slot, object.textures[i]);
        if (slot != 0) {
            state.textureIds.push_back(slot);
        }
    }
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host, GameObject &object);

JSValue syncInstanceWrapper(JSContext *ctx, ScriptHost &host,
                            CoreObject &object, std::uint32_t index) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    if (index >= object.instances.size()) {
        return JS_NULL;
    }

    const std::uint64_t key = makeInstanceKey(object.getId(), index);
    JSValue wrapper = JS_UNDEFINED;
    auto cacheIt = host.instanceCache.find(key);
    if (cacheIt != host.instanceCache.end()) {
        wrapper = JS_DupValue(ctx, cacheIt->second);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.instancePrototype);
        host.instanceCache[key] = JS_DupValue(ctx, wrapper);
    }

    const Instance &instance = object.instances[index];
    setProperty(ctx, wrapper, ATLAS_INSTANCE_OWNER_ID_PROP,
                JS_NewInt32(ctx, object.getId()));
    setProperty(ctx, wrapper, ATLAS_INSTANCE_INDEX_PROP,
                JS_NewInt32(ctx, static_cast<int>(index)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, instance.position));
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, instance.rotation));
    setProperty(ctx, wrapper, "scale",
                makePosition3d(ctx, host, instance.scale));
    return wrapper;
}

JSValue makeMaterial(JSContext *ctx, ScriptHost &host,
                     const Material &material) {
    JSValue result = newObjectFromPrototype(ctx, host.materialPrototype);
    setProperty(ctx, result, "albedo", makeColor(ctx, host, material.albedo));
    setProperty(ctx, result, "metallic", JS_NewFloat64(ctx, material.metallic));
    setProperty(ctx, result, "roughness",
                JS_NewFloat64(ctx, material.roughness));
    setProperty(ctx, result, "ao", JS_NewFloat64(ctx, material.ao));
    setProperty(ctx, result, "reflectivity",
                JS_NewFloat64(ctx, material.reflectivity));
    setProperty(ctx, result, "emissiveColor",
                makeColor(ctx, host, material.emissiveColor));
    setProperty(ctx, result, "emissiveIntensity",
                JS_NewFloat64(ctx, material.emissiveIntensity));
    setProperty(ctx, result, "normalMapStrength",
                JS_NewFloat64(ctx, material.normalMapStrength));
    setProperty(ctx, result, "useNormalMap",
                JS_NewBool(ctx, material.useNormalMap));
    setProperty(ctx, result, "transmittance",
                JS_NewFloat64(ctx, material.transmittance));
    setProperty(ctx, result, "ior", JS_NewFloat64(ctx, material.ior));
    return result;
}

JSValue makeCoreVertex(JSContext *ctx, ScriptHost &host,
                       const CoreVertex &vertex) {
    JSValue result = newObjectFromPrototype(ctx, host.coreVertexPrototype);
    setProperty(ctx, result, "position",
                makePosition3d(ctx, host, vertex.position));
    setProperty(ctx, result, "color", makeColor(ctx, host, vertex.color));
    setProperty(ctx, result, "textureCoord",
                makePosition2d(ctx, host,
                               Position2d(vertex.textureCoordinate[0],
                                          vertex.textureCoordinate[1])));
    setProperty(ctx, result, "normal",
                makePosition3d(ctx, host, vertex.normal));
    setProperty(ctx, result, "tangent",
                makePosition3d(ctx, host, vertex.tangent));
    setProperty(ctx, result, "bitangent",
                makePosition3d(ctx, host, vertex.bitangent));
    return result;
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host,
                          GameObject &object) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    const int objectId = object.getId();
    JSValue wrapper = JS_UNDEFINED;
    auto cacheIt = host.objectCache.find(objectId);
    if (cacheIt != host.objectCache.end()) {
        wrapper = JS_DupValue(ctx, cacheIt->second);
    } else {
        JSValueConst prototype = host.gameObjectPrototype;
        if (dynamic_cast<Image *>(&object) != nullptr) {
            prototype = host.imagePrototype;
        } else if (dynamic_cast<Text *>(&object) != nullptr) {
            prototype = host.textPrototype;
        } else if (dynamic_cast<TextField *>(&object) != nullptr) {
            prototype = host.textFieldPrototype;
        } else if (dynamic_cast<Button *>(&object) != nullptr) {
            prototype = host.buttonPrototype;
        } else if (dynamic_cast<Checkbox *>(&object) != nullptr) {
            prototype = host.checkboxPrototype;
        } else if (dynamic_cast<Column *>(&object) != nullptr) {
            prototype = host.columnPrototype;
        } else if (dynamic_cast<Row *>(&object) != nullptr) {
            prototype = host.rowPrototype;
        } else if (dynamic_cast<Stack *>(&object) != nullptr) {
            prototype = host.stackPrototype;
        } else if (dynamic_cast<Fluid *>(&object) != nullptr) {
            prototype = host.fluidPrototype;
        } else if (dynamic_cast<UIObject *>(&object) != nullptr) {
            prototype = host.uiObjectPrototype;
        } else if (dynamic_cast<CoreObject *>(&object) != nullptr) {
            prototype = host.coreObjectPrototype;
        } else if (dynamic_cast<ParticleEmitter *>(&object) != nullptr) {
            prototype = host.particleEmitterPrototype;
        } else if (dynamic_cast<Model *>(&object) != nullptr) {
            prototype = host.modelPrototype;
        } else if (dynamic_cast<Terrain *>(&object) != nullptr) {
            prototype = host.terrainPrototype;
        }
        wrapper = newObjectFromPrototype(ctx, prototype);
        host.objectCache[objectId] = JS_DupValue(ctx, wrapper);
    }

    auto stateIt = host.objectStates.find(objectId);
    const bool attached = stateIt != host.objectStates.end()
                              ? stateIt->second.attachedToWindow
                              : true;
    trackObjectState(host, object, attached);

    setProperty(ctx, wrapper, ATLAS_OBJECT_ID_PROP, JS_NewInt32(ctx, objectId));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(
        ctx, wrapper, ATLAS_IS_CORE_OBJECT_PROP,
        JS_NewBool(ctx, dynamic_cast<CoreObject *>(&object) != nullptr));
    setProperty(ctx, wrapper, "id", JS_NewInt32(ctx, objectId));
    setProperty(ctx, wrapper, "components",
                buildComponentsArray(ctx, host, objectId));
    if (auto *uiObject = dynamic_cast<UIObject *>(&object);
        uiObject != nullptr) {
        const Position2d screenPosition = uiObject->getScreenPosition();
        setProperty(ctx, wrapper, "position",
                    makePosition3d(
                        ctx, host,
                        Position3d(screenPosition.x, screenPosition.y, 0.0f)));
    } else {
        setProperty(ctx, wrapper, "position",
                    makePosition3d(ctx, host, object.getPosition()));
    }
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, object.getRotation()));
    setProperty(ctx, wrapper, "scale",
                makePosition3d(ctx, host, object.getScale()));

    std::string name;
    if (host.context != nullptr) {
        auto nameIt = host.context->objectNames.find(objectId);
        if (nameIt != host.context->objectNames.end()) {
            name = nameIt->second;
        }
    }
    if (name.empty()) {
        name = object.name;
    }
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, name.c_str()));

    if (auto *emitter = dynamic_cast<ParticleEmitter *>(&object);
        emitter != nullptr) {
        setProperty(ctx, wrapper, ATLAS_PARTICLE_EMITTER_ID_PROP,
                    JS_NewInt32(ctx, objectId));
        setProperty(ctx, wrapper, "settings",
                    makeParticleSettings(ctx, emitter->settings));
        setProperty(ctx, wrapper, "position",
                    makePosition3d(ctx, host, emitter->getPosition()));
    }

    if (auto *core = dynamic_cast<CoreObject *>(&object); core != nullptr) {
        syncObjectTextureStates(host, *core);

        JSValue vertices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->vertices.size(); ++i) {
            JS_SetPropertyUint32(ctx, vertices, i,
                                 makeCoreVertex(ctx, host, core->vertices[i]));
        }

        JSValue indices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->indices.size(); ++i) {
            JS_SetPropertyUint32(
                ctx, indices, i,
                JS_NewInt32(ctx, static_cast<int>(core->indices[i])));
        }

        JSValue instances = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->instances.size(); ++i) {
            JS_SetPropertyUint32(ctx, instances, i,
                                 syncInstanceWrapper(ctx, host, *core, i));
        }

        JSValue textures = JS_NewArray(ctx);
        auto stateIt = host.objectStates.find(objectId);
        if (stateIt != host.objectStates.end()) {
            for (std::uint32_t i = 0; i < stateIt->second.textureIds.size();
                 ++i) {
                JS_SetPropertyUint32(
                    ctx, textures, i,
                    syncTextureWrapper(ctx, host,
                                       stateIt->second.textureIds[i]));
            }
        }

        setProperty(ctx, wrapper, "vertices", vertices);
        setProperty(ctx, wrapper, "indices", indices);
        setProperty(ctx, wrapper, "textures", textures);
        setProperty(ctx, wrapper, "material",
                    makeMaterial(ctx, host, core->material));
        setProperty(ctx, wrapper, "instances", instances);
        setProperty(ctx, wrapper, "castsShadows",
                    JS_NewBool(ctx, core->castsShadows));
    }

    if (auto *terrain = dynamic_cast<Terrain *>(&object); terrain != nullptr) {
        setProperty(ctx, wrapper, "heightmap",
                    makeResource(ctx, host, terrain->heightmap));
        if (terrain->moistureTexture.texture != nullptr ||
            terrain->moistureTexture.id != 0) {
            const std::uint64_t moistureId =
                registerTextureState(host, terrain->moistureTexture);
            setProperty(ctx, wrapper, "moistureTexture",
                        syncTextureWrapper(ctx, host, moistureId));
        } else {
            setProperty(ctx, wrapper, "moistureTexture", JS_NULL);
        }
        if (terrain->temperatureTexture.texture != nullptr ||
            terrain->temperatureTexture.id != 0) {
            const std::uint64_t temperatureId =
                registerTextureState(host, terrain->temperatureTexture);
            setProperty(ctx, wrapper, "temperatureTexture",
                        syncTextureWrapper(ctx, host, temperatureId));
        } else {
            setProperty(ctx, wrapper, "temperatureTexture", JS_NULL);
        }
        setProperty(ctx, wrapper, "generator",
                    makeTerrainGeneratorValue(ctx, host, terrain->generator));
        setProperty(ctx, wrapper, "createdWithMap",
                    JS_NewBool(ctx, terrain->createdWithMap));
        setProperty(ctx, wrapper, "width", JS_NewInt32(ctx, terrain->width));
        setProperty(ctx, wrapper, "length", JS_NewInt32(ctx, terrain->height));
        setProperty(ctx, wrapper, "height", JS_NewInt32(ctx, terrain->height));
        setProperty(ctx, wrapper, "resolution",
                    JS_NewInt32(ctx, static_cast<int>(terrain->resolution)));
        setProperty(ctx, wrapper, "maxPeak",
                    JS_NewFloat64(ctx, terrain->maxPeak));
        setProperty(ctx, wrapper, "seaLevel",
                    JS_NewFloat64(ctx, terrain->seaLevel));
        JSValue biomes = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < terrain->biomes.size(); ++i) {
            JS_SetPropertyUint32(ctx, biomes, i,
                                 makeBiomeValue(ctx, host, terrain->biomes[i]));
        }
        setProperty(ctx, wrapper, "biomes", biomes);
    }

    if (auto *image = dynamic_cast<Image *>(&object); image != nullptr) {
        if (image->texture.texture != nullptr || image->texture.id != 0) {
            const std::uint64_t textureId =
                registerTextureState(host, image->texture);
            setProperty(ctx, wrapper, "texture",
                        syncTextureWrapper(ctx, host, textureId));
        } else {
            setProperty(ctx, wrapper, "texture", JS_NULL);
        }
        setProperty(ctx, wrapper, "size", makeSize2d(ctx, host, image->size));
        setProperty(ctx, wrapper, "tint", makeColor(ctx, host, image->tint));
    } else if (auto *text = dynamic_cast<Text *>(&object); text != nullptr) {
        setProperty(ctx, wrapper, "content",
                    JS_NewString(ctx, text->content.c_str()));
        setProperty(
            ctx, wrapper, "font",
            syncFontWrapper(ctx, host, registerFontState(host, text->font)));
        setProperty(ctx, wrapper, "fontSize",
                    JS_NewFloat64(ctx, text->fontSize));
        setProperty(ctx, wrapper, "color", makeColor(ctx, host, text->color));
    } else if (auto *field = dynamic_cast<TextField *>(&object);
               field != nullptr) {
        setProperty(ctx, wrapper, "text",
                    JS_NewString(ctx, field->text.c_str()));
        setProperty(ctx, wrapper, "placeholder",
                    JS_NewString(ctx, field->placeholder.c_str()));
        setProperty(
            ctx, wrapper, "font",
            syncFontWrapper(ctx, host, registerFontState(host, field->font)));
        setProperty(ctx, wrapper, "fontSize",
                    JS_NewFloat64(ctx, field->fontSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, field->padding));
        setProperty(ctx, wrapper, "maximumWidth",
                    JS_NewFloat64(ctx, field->maximumWidth));
        setProperty(ctx, wrapper, "textColor",
                    makeColor(ctx, host, field->textColor));
        setProperty(ctx, wrapper, "placeholderColor",
                    makeColor(ctx, host, field->placeholderColor));
        setProperty(ctx, wrapper, "backgroundColor",
                    makeColor(ctx, host, field->backgroundColor));
        setProperty(ctx, wrapper, "borderColor",
                    makeColor(ctx, host, field->borderColor));
        setProperty(ctx, wrapper, "focusedBorderColor",
                    makeColor(ctx, host, field->focusedBorderColor));
        setProperty(ctx, wrapper, "cursorColor",
                    makeColor(ctx, host, field->cursorColor));
    } else if (auto *button = dynamic_cast<Button *>(&object);
               button != nullptr) {
        setProperty(ctx, wrapper, "label",
                    JS_NewString(ctx, button->label.c_str()));
        setProperty(
            ctx, wrapper, "font",
            syncFontWrapper(ctx, host, registerFontState(host, button->font)));
        setProperty(ctx, wrapper, "fontSize",
                    JS_NewFloat64(ctx, button->fontSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, button->padding));
        setProperty(ctx, wrapper, "minimumSize",
                    makeSize2d(ctx, host, button->minimumSize));
        setProperty(ctx, wrapper, "textColor",
                    makeColor(ctx, host, button->textColor));
        setProperty(ctx, wrapper, "backgroundColor",
                    makeColor(ctx, host, button->backgroundColor));
        setProperty(ctx, wrapper, "hoverBackgroundColor",
                    makeColor(ctx, host, button->hoverBackgroundColor));
        setProperty(ctx, wrapper, "pressedBackgroundColor",
                    makeColor(ctx, host, button->pressedBackgroundColor));
        setProperty(ctx, wrapper, "borderColor",
                    makeColor(ctx, host, button->borderColor));
        setProperty(ctx, wrapper, "hoverBorderColor",
                    makeColor(ctx, host, button->hoverBorderColor));
        setProperty(ctx, wrapper, "enabled", JS_NewBool(ctx, button->enabled));
    } else if (auto *checkbox = dynamic_cast<Checkbox *>(&object);
               checkbox != nullptr) {
        setProperty(ctx, wrapper, "label",
                    JS_NewString(ctx, checkbox->label.c_str()));
        setProperty(ctx, wrapper, "font",
                    syncFontWrapper(ctx, host,
                                    registerFontState(host, checkbox->font)));
        setProperty(ctx, wrapper, "fontSize",
                    JS_NewFloat64(ctx, checkbox->fontSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, checkbox->padding));
        setProperty(ctx, wrapper, "boxSize",
                    JS_NewFloat64(ctx, checkbox->boxSize));
        setProperty(ctx, wrapper, "spacing",
                    JS_NewFloat64(ctx, checkbox->spacing));
        setProperty(ctx, wrapper, "checked",
                    JS_NewBool(ctx, checkbox->checked));
        setProperty(ctx, wrapper, "enabled",
                    JS_NewBool(ctx, checkbox->enabled));
        setProperty(ctx, wrapper, "textColor",
                    makeColor(ctx, host, checkbox->textColor));
        setProperty(ctx, wrapper, "boxBackgroundColor",
                    makeColor(ctx, host, checkbox->boxBackgroundColor));
        setProperty(ctx, wrapper, "hoverBoxBackgroundColor",
                    makeColor(ctx, host, checkbox->hoverBoxBackgroundColor));
        setProperty(ctx, wrapper, "borderColor",
                    makeColor(ctx, host, checkbox->borderColor));
        setProperty(ctx, wrapper, "activeBorderColor",
                    makeColor(ctx, host, checkbox->activeBorderColor));
        setProperty(ctx, wrapper, "checkColor",
                    makeColor(ctx, host, checkbox->checkColor));
    } else if (auto *column = dynamic_cast<Column *>(&object);
               column != nullptr) {
        setProperty(ctx, wrapper, "spacing",
                    JS_NewFloat64(ctx, column->spacing));
        setProperty(ctx, wrapper, "maxSize",
                    makeSize2d(ctx, host, column->maxSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, column->padding));
        setProperty(ctx, wrapper, "style",
                    makeUIStyleValue(ctx, host, column->style()));
        JSValue children = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < column->children.size(); ++i) {
            if (column->children[i] == nullptr) {
                JS_SetPropertyUint32(ctx, children, i, JS_NULL);
            } else {
                JS_SetPropertyUint32(
                    ctx, children, i,
                    syncObjectWrapper(ctx, host, *column->children[i]));
            }
        }
        setProperty(ctx, wrapper, "children", children);
        setProperty(ctx, wrapper, "alignment",
                    JS_NewInt32(ctx, static_cast<int>(column->alignment)));
        setProperty(ctx, wrapper, "anchor",
                    JS_NewInt32(ctx, static_cast<int>(column->anchor)));
    } else if (auto *row = dynamic_cast<Row *>(&object); row != nullptr) {
        setProperty(ctx, wrapper, "spacing", JS_NewFloat64(ctx, row->spacing));
        setProperty(ctx, wrapper, "maxSize",
                    makeSize2d(ctx, host, row->maxSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, row->padding));
        setProperty(ctx, wrapper, "style",
                    makeUIStyleValue(ctx, host, row->style()));
        JSValue children = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < row->children.size(); ++i) {
            if (row->children[i] == nullptr) {
                JS_SetPropertyUint32(ctx, children, i, JS_NULL);
            } else {
                JS_SetPropertyUint32(
                    ctx, children, i,
                    syncObjectWrapper(ctx, host, *row->children[i]));
            }
        }
        setProperty(ctx, wrapper, "children", children);
        setProperty(ctx, wrapper, "alignment",
                    JS_NewInt32(ctx, static_cast<int>(row->alignment)));
        setProperty(ctx, wrapper, "anchor",
                    JS_NewInt32(ctx, static_cast<int>(row->anchor)));
    } else if (auto *stack = dynamic_cast<Stack *>(&object); stack != nullptr) {
        setProperty(ctx, wrapper, "maxSize",
                    makeSize2d(ctx, host, stack->maxSize));
        setProperty(ctx, wrapper, "padding",
                    makeSize2d(ctx, host, stack->padding));
        setProperty(ctx, wrapper, "style",
                    makeUIStyleValue(ctx, host, stack->style()));
        JSValue children = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < stack->children.size(); ++i) {
            if (stack->children[i] == nullptr) {
                JS_SetPropertyUint32(ctx, children, i, JS_NULL);
            } else {
                JS_SetPropertyUint32(
                    ctx, children, i,
                    syncObjectWrapper(ctx, host, *stack->children[i]));
            }
        }
        setProperty(ctx, wrapper, "children", children);
        setProperty(
            ctx, wrapper, "horizontalAlignment",
            JS_NewInt32(ctx, static_cast<int>(stack->horizontalAlignment)));
        setProperty(
            ctx, wrapper, "verticalAlignment",
            JS_NewInt32(ctx, static_cast<int>(stack->verticalAlignment)));
        setProperty(ctx, wrapper, "anchor",
                    JS_NewInt32(ctx, static_cast<int>(stack->anchor)));
    } else if (auto *fluid = dynamic_cast<Fluid *>(&object); fluid != nullptr) {
        setProperty(ctx, wrapper, "waveVelocity",
                    JS_NewFloat64(ctx, fluid->waveVelocity));
        if (fluid->normalTexture.texture != nullptr ||
            fluid->normalTexture.id != 0) {
            setProperty(ctx, wrapper, "normalTexture",
                        syncTextureWrapper(
                            ctx, host,
                            registerTextureState(host, fluid->normalTexture)));
        } else {
            setProperty(ctx, wrapper, "normalTexture", JS_NULL);
        }
        if (fluid->movementTexture.texture != nullptr ||
            fluid->movementTexture.id != 0) {
            setProperty(ctx, wrapper, "movementTexture",
                        syncTextureWrapper(ctx, host,
                                           registerTextureState(
                                               host, fluid->movementTexture)));
        } else {
            setProperty(ctx, wrapper, "movementTexture", JS_NULL);
        }
    }

    return wrapper;
}

bool parseGraphiteStyleProperty(JSContext *ctx, ScriptHost &host,
                                JSValueConst wrapper, const char *propName,
                                graphite::UIStyle &out) {
    JSValue value = JS_GetPropertyStr(ctx, wrapper, propName);
    if (JS_IsException(value) || JS_IsUndefined(value) || JS_IsNull(value)) {
        JS_FreeValue(ctx, value);
        return false;
    }
    const bool ok = parseUIStyle(ctx, host, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

bool parseUIChildren(JSContext *ctx, ScriptHost &host, JSValueConst value,
                     std::vector<UIObject *> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    if (!getArrayLength(ctx, value, length)) {
        return false;
    }

    out.clear();
    out.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue childValue = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(childValue) || JS_IsNull(childValue) ||
            JS_IsUndefined(childValue)) {
            JS_FreeValue(ctx, childValue);
            continue;
        }
        GameObject *child = resolveObjectArg(ctx, host, childValue);
        JS_FreeValue(ctx, childValue);
        if (child == nullptr) {
            continue;
        }
        auto *uiChild = dynamic_cast<UIObject *>(child);
        if (uiChild != nullptr) {
            out.push_back(uiChild);
        }
    }
    return true;
}

bool applyClouds(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                 Clouds &clouds) {
    (void)host;
    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parsePosition3d(ctx, value, clouds.position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "size");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parsePosition3d(ctx, value, clouds.size);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "offset");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parsePosition3d(ctx, value, clouds.offset);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "wind");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parsePosition3d(ctx, value, clouds.wind);
    }
    JS_FreeValue(ctx, value);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "scale", number)) {
        clouds.scale = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "density", number)) {
        clouds.density = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "densityMultiplier", number)) {
        clouds.densityMultiplier = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "absorption", number)) {
        clouds.absorption = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "scattering", number)) {
        clouds.scattering = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "phase", number)) {
        clouds.phase = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "clusterStrength", number)) {
        clouds.clusterStrength = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "lightStepMultiplier", number)) {
        clouds.lightStepMultiplier = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "minStepLength", number)) {
        clouds.minStepLength = static_cast<float>(number);
    }

    std::int64_t integer = 0;
    if (readIntProperty(ctx, wrapper, "primaryStepCount", integer)) {
        clouds.primaryStepCount = static_cast<int>(integer);
    }
    if (readIntProperty(ctx, wrapper, "lightStepCount", integer)) {
        clouds.lightStepCount = static_cast<int>(integer);
    }

    return true;
}

bool applyAtmosphere(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     Atmosphere &atmosphere) {
    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "timeOfDay", number)) {
        atmosphere.timeOfDay = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "secondsPerHour", number)) {
        atmosphere.secondsPerHour = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "sunSize", number)) {
        atmosphere.sunSize = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "moonSize", number)) {
        atmosphere.moonSize = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "sunTintStrength", number)) {
        atmosphere.sunTintStrength = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "moonTintStrength", number)) {
        atmosphere.moonTintStrength = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "starIntensity", number)) {
        atmosphere.starIntensity = static_cast<float>(number);
    }

    bool cycle = atmosphere.cycle;
    if (readBoolProperty(ctx, wrapper, "cycle", cycle)) {
        atmosphere.cycle = cycle;
    }

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "wind");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parsePosition3d(ctx, value, atmosphere.wind);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "sunColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, atmosphere.sunColor);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "moonColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, atmosphere.moonColor);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "clouds");
    if (!JS_IsException(value)) {
        if (JS_IsNull(value) || JS_IsUndefined(value)) {
            atmosphere.clouds = nullptr;
        } else {
            auto *cloudState = resolveClouds(ctx, host, value);
            if (cloudState != nullptr && cloudState->ownedClouds) {
                atmosphere.clouds = cloudState->ownedClouds;
                applyClouds(ctx, host, value, *atmosphere.clouds);
            }
        }
    }
    JS_FreeValue(ctx, value);

    auto *state = resolveAtmosphere(ctx, host, wrapper);
    if (state != nullptr) {
        const auto atmosphereIt = host.atmosphereIds.find(&atmosphere);
        value = JS_GetPropertyStr(ctx, wrapper, "weatherDelegate");
        if (!JS_IsException(value)) {
            if (JS_IsFunction(ctx, value)) {
                if (!JS_IsUndefined(state->weatherDelegate)) {
                    JS_FreeValue(ctx, state->weatherDelegate);
                }
                state->weatherDelegate = JS_DupValue(ctx, value);
                if (atmosphereIt != host.atmosphereIds.end()) {
                    bindAtmosphereWeatherDelegate(ctx, host,
                                                  atmosphereIt->second);
                }
            } else if (JS_IsNull(value) || JS_IsUndefined(value)) {
                if (!JS_IsUndefined(state->weatherDelegate)) {
                    JS_FreeValue(ctx, state->weatherDelegate);
                    state->weatherDelegate = JS_UNDEFINED;
                }
                atmosphere.weatherDelegate = [](ViewInformation) {
                    return WeatherState();
                };
            }
        }
        JS_FreeValue(ctx, value);
    }

    return true;
}

bool applyBaseObject(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     GameObject &object) {
    Position3d position = object.getPosition();
    Rotation3d rotation = object.getRotation();
    Scale3d scale = object.getScale();

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        if (auto *uiObject = dynamic_cast<UIObject *>(&object);
            uiObject != nullptr) {
            Position3d screenPosition;
            if (parsePosition3d(ctx, value, screenPosition)) {
                uiObject->setScreenPosition(
                    Position2d(screenPosition.x, screenPosition.y));
            }
        } else {
            parsePosition3d(ctx, value, position);
        }
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "scale");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, scale);
    }
    JS_FreeValue(ctx, value);

    if (dynamic_cast<UIObject *>(&object) == nullptr) {
        object.setPosition(position);
        object.setRotation(rotation);
        object.setScale(scale);
    }

    if (host.context != nullptr) {
        std::string name;
        if (readStringProperty(ctx, wrapper, "name", name)) {
            assignObjectName(*host.context, object, name);
        }
    }

    return true;
}

bool applyFluid(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                Fluid &fluid) {
    applyBaseObject(ctx, host, wrapper, fluid);

    double waveVelocity = fluid.waveVelocity;
    if (readNumberProperty(ctx, wrapper, "waveVelocity", waveVelocity)) {
        fluid.waveVelocity = static_cast<float>(waveVelocity);
    }

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "normalTexture");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *textureState = resolveTexture(ctx, host, value);
        if (textureState != nullptr && textureState->texture) {
            fluid.normalTexture = *textureState->texture;
        }
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "movementTexture");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *textureState = resolveTexture(ctx, host, value);
        if (textureState != nullptr && textureState->texture) {
            fluid.movementTexture = *textureState->texture;
        }
    }
    JS_FreeValue(ctx, value);

    return true;
}

bool applyImage(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                Image &image) {
    applyBaseObject(ctx, host, wrapper, image);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "texture");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *textureState = resolveTexture(ctx, host, value);
        if (textureState != nullptr && textureState->texture) {
            image.setTexture(*textureState->texture);
        }
    }
    JS_FreeValue(ctx, value);

    Size2d size = image.size;
    value = JS_GetPropertyStr(ctx, wrapper, "size");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, size);
    }
    JS_FreeValue(ctx, value);
    image.setSize(size);

    Color tint = image.tint;
    value = JS_GetPropertyStr(ctx, wrapper, "tint");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, tint);
    }
    JS_FreeValue(ctx, value);
    image.tint = tint;

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "__style", style)) {
        image.setStyle(style);
    }

    return true;
}

bool applyText(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
               Text &text) {
    applyBaseObject(ctx, host, wrapper, text);

    std::string content = text.content;
    readStringProperty(ctx, wrapper, "content", content);
    text.content = content;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "font");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *fontState = resolveFont(ctx, host, value);
        if (fontState != nullptr && fontState->font) {
            text.font = *fontState->font;
        }
    }
    JS_FreeValue(ctx, value);

    double fontSize = text.fontSize;
    readNumberProperty(ctx, wrapper, "fontSize", fontSize);
    text.fontSize = static_cast<float>(fontSize);

    Color color = text.color;
    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);
    text.color = color;

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "__style", style)) {
        text.setStyle(style);
    }

    return true;
}

bool applyTextField(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                    TextField &field) {
    applyBaseObject(ctx, host, wrapper, field);

    readStringProperty(ctx, wrapper, "text", field.text);
    readStringProperty(ctx, wrapper, "placeholder", field.placeholder);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "font");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *fontState = resolveFont(ctx, host, value);
        if (fontState != nullptr && fontState->font) {
            field.font = *fontState->font;
        }
    }
    JS_FreeValue(ctx, value);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "fontSize", number)) {
        field.fontSize = static_cast<float>(number);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, field.padding);
    }
    JS_FreeValue(ctx, value);
    if (readNumberProperty(ctx, wrapper, "maximumWidth", number)) {
        field.maximumWidth = static_cast<float>(number);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "textColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.textColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "placeholderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.placeholderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "backgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.backgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "borderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.borderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "focusedBorderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.focusedBorderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "cursorColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, field.cursorColor);
    }
    JS_FreeValue(ctx, value);

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "__style", style)) {
        field.setStyle(style);
    }

    return true;
}

bool applyButton(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                 Button &button) {
    applyBaseObject(ctx, host, wrapper, button);

    readStringProperty(ctx, wrapper, "label", button.label);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "font");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *fontState = resolveFont(ctx, host, value);
        if (fontState != nullptr && fontState->font) {
            button.font = *fontState->font;
        }
    }
    JS_FreeValue(ctx, value);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "fontSize", number)) {
        button.fontSize = static_cast<float>(number);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, button.padding);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "minimumSize");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, button.minimumSize);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "textColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.textColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "backgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.backgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "hoverBackgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.hoverBackgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "pressedBackgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.pressedBackgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "borderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.borderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "hoverBorderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, button.hoverBorderColor);
    }
    JS_FreeValue(ctx, value);
    bool enabled = button.enabled;
    if (readBoolProperty(ctx, wrapper, "enabled", enabled)) {
        button.enabled = enabled;
    }

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "__style", style)) {
        button.setStyle(style);
    }

    return true;
}

bool applyCheckbox(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                   Checkbox &checkbox) {
    applyBaseObject(ctx, host, wrapper, checkbox);

    readStringProperty(ctx, wrapper, "label", checkbox.label);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "font");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *fontState = resolveFont(ctx, host, value);
        if (fontState != nullptr && fontState->font) {
            checkbox.font = *fontState->font;
        }
    }
    JS_FreeValue(ctx, value);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "fontSize", number)) {
        checkbox.fontSize = static_cast<float>(number);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, checkbox.padding);
    }
    JS_FreeValue(ctx, value);
    if (readNumberProperty(ctx, wrapper, "boxSize", number)) {
        checkbox.boxSize = static_cast<float>(number);
    }
    if (readNumberProperty(ctx, wrapper, "spacing", number)) {
        checkbox.spacing = static_cast<float>(number);
    }
    bool checked = checkbox.checked;
    if (readBoolProperty(ctx, wrapper, "checked", checked)) {
        checkbox.checked = checked;
    }
    bool enabled = checkbox.enabled;
    if (readBoolProperty(ctx, wrapper, "enabled", enabled)) {
        checkbox.enabled = enabled;
    }
    value = JS_GetPropertyStr(ctx, wrapper, "textColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.textColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "boxBackgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.boxBackgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "hoverBoxBackgroundColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.hoverBoxBackgroundColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "borderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.borderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "activeBorderColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.activeBorderColor);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "checkColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseColor(ctx, value, checkbox.checkColor);
    }
    JS_FreeValue(ctx, value);

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "__style", style)) {
        checkbox.setStyle(style);
    }

    return true;
}

bool applyColumn(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                 Column &column) {
    applyBaseObject(ctx, host, wrapper, column);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "spacing", number)) {
        column.spacing = static_cast<float>(number);
    }
    JSValue value = JS_GetPropertyStr(ctx, wrapper, "maxSize");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, column.maxSize);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, column.padding);
    }
    JS_FreeValue(ctx, value);
    std::int64_t enumValue = 0;
    if (readIntProperty(ctx, wrapper, "alignment", enumValue)) {
        column.alignment = static_cast<ElementAlignment>(enumValue);
    }
    if (readIntProperty(ctx, wrapper, "anchor", enumValue)) {
        column.anchor = static_cast<LayoutAnchor>(enumValue);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "children");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        std::vector<UIObject *> children;
        if (parseUIChildren(ctx, host, value, children)) {
            column.setChildren(children);
        }
    }
    JS_FreeValue(ctx, value);

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "style", style)) {
        column.setStyle(style);
    }
    return true;
}

bool applyRow(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
              Row &row) {
    applyBaseObject(ctx, host, wrapper, row);

    double number = 0.0;
    if (readNumberProperty(ctx, wrapper, "spacing", number)) {
        row.spacing = static_cast<float>(number);
    }
    JSValue value = JS_GetPropertyStr(ctx, wrapper, "maxSize");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, row.maxSize);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, row.padding);
    }
    JS_FreeValue(ctx, value);
    std::int64_t enumValue = 0;
    if (readIntProperty(ctx, wrapper, "alignment", enumValue)) {
        row.alignment = static_cast<ElementAlignment>(enumValue);
    }
    if (readIntProperty(ctx, wrapper, "anchor", enumValue)) {
        row.anchor = static_cast<LayoutAnchor>(enumValue);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "children");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        std::vector<UIObject *> children;
        if (parseUIChildren(ctx, host, value, children)) {
            row.setChildren(children);
        }
    }
    JS_FreeValue(ctx, value);

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "style", style)) {
        row.setStyle(style);
    }
    return true;
}

bool applyStack(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                Stack &stack) {
    applyBaseObject(ctx, host, wrapper, stack);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "maxSize");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, stack.maxSize);
    }
    JS_FreeValue(ctx, value);
    value = JS_GetPropertyStr(ctx, wrapper, "padding");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        parseSize2d(ctx, value, stack.padding);
    }
    JS_FreeValue(ctx, value);
    std::int64_t enumValue = 0;
    if (readIntProperty(ctx, wrapper, "horizontalAlignment", enumValue)) {
        stack.horizontalAlignment = static_cast<ElementAlignment>(enumValue);
    }
    if (readIntProperty(ctx, wrapper, "verticalAlignment", enumValue)) {
        stack.verticalAlignment = static_cast<ElementAlignment>(enumValue);
    }
    if (readIntProperty(ctx, wrapper, "anchor", enumValue)) {
        stack.anchor = static_cast<LayoutAnchor>(enumValue);
    }
    value = JS_GetPropertyStr(ctx, wrapper, "children");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        std::vector<UIObject *> children;
        if (parseUIChildren(ctx, host, value, children)) {
            stack.setChildren(children);
        }
    }
    JS_FreeValue(ctx, value);

    graphite::UIStyle style;
    if (parseGraphiteStyleProperty(ctx, host, wrapper, "style", style)) {
        stack.setStyle(style);
    }
    return true;
}

bool applyCoreObject(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     CoreObject &object) {
    applyBaseObject(ctx, host, wrapper, object);

    bool geometryUpdated = false;
    JSValue vertices = JS_GetPropertyStr(ctx, wrapper, "vertices");
    if (!JS_IsException(vertices) && JS_IsArray(vertices)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, vertices, "length");
        getUint32(ctx, lengthValue, length);
        JS_FreeValue(ctx, lengthValue);

        std::vector<CoreVertex> parsedVertices;
        parsedVertices.reserve(length);
        for (std::uint32_t i = 0; i < length; ++i) {
            JSValue vertexValue = JS_GetPropertyUint32(ctx, vertices, i);
            if (JS_IsException(vertexValue)) {
                JS_FreeValue(ctx, vertices);
                return false;
            }
            CoreVertex vertex;
            if (parseCoreVertex(ctx, vertexValue, vertex)) {
                parsedVertices.push_back(vertex);
            }
            JS_FreeValue(ctx, vertexValue);
        }
        if (!parsedVertices.empty()) {
            object.attachVertices(parsedVertices);
            geometryUpdated = true;
        }
    }
    JS_FreeValue(ctx, vertices);

    JSValue indices = JS_GetPropertyStr(ctx, wrapper, "indices");
    if (!JS_IsException(indices) && JS_IsArray(indices)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, indices, "length");
        getUint32(ctx, lengthValue, length);
        JS_FreeValue(ctx, lengthValue);

        std::vector<Index> parsedIndices;
        parsedIndices.reserve(length);
        for (std::uint32_t i = 0; i < length; ++i) {
            JSValue indexValue = JS_GetPropertyUint32(ctx, indices, i);
            if (JS_IsException(indexValue)) {
                JS_FreeValue(ctx, indices);
                return false;
            }
            std::int64_t parsedIndex = 0;
            if (getInt64(ctx, indexValue, parsedIndex)) {
                parsedIndices.push_back(static_cast<Index>(parsedIndex));
            }
            JS_FreeValue(ctx, indexValue);
        }
        object.attachIndices(parsedIndices);
        geometryUpdated = true;
    }
    JS_FreeValue(ctx, indices);

    JSValue materialValue = JS_GetPropertyStr(ctx, wrapper, "material");
    if (!JS_IsException(materialValue) && !JS_IsUndefined(materialValue)) {
        Material material = object.material;
        parseMaterial(ctx, materialValue, material);
        object.material = material;
    }
    JS_FreeValue(ctx, materialValue);

    bool castsShadows = object.castsShadows;
    if (readBoolProperty(ctx, wrapper, "castsShadows", castsShadows)) {
        object.castsShadows = castsShadows;
    }

    if (geometryUpdated) {
        attachObjectIfReady(host, object);
    }

    return true;
}

bool applyTerrain(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                  Terrain &terrain) {
    applyBaseObject(ctx, host, wrapper, terrain);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "heightmap");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        Resource heightmap;
        if (parseResource(ctx, value, heightmap)) {
            terrain.heightmap = heightmap;
            terrain.createdWithMap = true;
        }
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "moistureTexture");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *textureState = resolveTexture(ctx, host, value);
        if (textureState != nullptr && textureState->texture) {
            terrain.moistureTexture = *textureState->texture;
        }
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "temperatureTexture");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto *textureState = resolveTexture(ctx, host, value);
        if (textureState != nullptr && textureState->texture) {
            terrain.temperatureTexture = *textureState->texture;
        }
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "generator");
    if (!JS_IsException(value) && !JS_IsUndefined(value) && !JS_IsNull(value)) {
        auto generator = parseTerrainGeneratorValue(ctx, host, value);
        if (generator) {
            terrain.generator = generator;
            terrain.createdWithMap = false;
        }
    }
    JS_FreeValue(ctx, value);

    bool createdWithMap = terrain.createdWithMap;
    readBoolProperty(ctx, wrapper, "createdWithMap", createdWithMap);
    terrain.createdWithMap = createdWithMap;

    std::int64_t integer = terrain.width;
    readIntProperty(ctx, wrapper, "width", integer);
    terrain.width = static_cast<int>(integer);
    integer = terrain.height;
    if (!readIntProperty(ctx, wrapper, "length", integer)) {
        readIntProperty(ctx, wrapper, "height", integer);
    }
    terrain.height = static_cast<int>(integer);
    integer = static_cast<int>(terrain.resolution);
    readIntProperty(ctx, wrapper, "resolution", integer);
    terrain.resolution = static_cast<unsigned int>(integer);

    double number = terrain.maxPeak;
    readNumberProperty(ctx, wrapper, "maxPeak", number);
    terrain.maxPeak = static_cast<float>(number);
    number = terrain.seaLevel;
    readNumberProperty(ctx, wrapper, "seaLevel", number);
    terrain.seaLevel = static_cast<float>(number);

    value = JS_GetPropertyStr(ctx, wrapper, "biomes");
    if (!JS_IsException(value) && JS_IsArray(value)) {
        std::uint32_t length = 0;
        if (getArrayLength(ctx, value, length)) {
            terrain.biomes.clear();
            terrain.biomes.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue biomeValue = JS_GetPropertyUint32(ctx, value, i);
                if (!JS_IsException(biomeValue)) {
                    Biome biome("", Color());
                    if (parseBiomeValue(ctx, host, biomeValue, biome)) {
                        terrain.biomes.push_back(biome);
                    }
                }
                JS_FreeValue(ctx, biomeValue);
            }
        }
    }
    JS_FreeValue(ctx, value);

    terrain.updateModelMatrix();
    return true;
}

bool applyPointLight(JSContext *ctx, JSValueConst wrapper, Light &light) {
    Position3d position = light.position;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;
    double distance = light.distance;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);
    readNumberProperty(ctx, wrapper, "distance", distance);

    light.position = position;
    light.setColor(color);
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    light.distance = static_cast<float>(distance);
    syncPointLightDebugObject(light);
    return true;
}

bool applyDirectionalLight(JSContext *ctx, JSValueConst wrapper,
                           DirectionalLight &light) {
    Magnitude3d direction = light.direction;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "direction");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, direction);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);

    light.direction = direction.normalized();
    light.color = color;
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    return true;
}

bool applySpotLight(JSContext *ctx, JSValueConst wrapper, Spotlight &light) {
    Position3d position = light.position;
    Magnitude3d direction = light.direction;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double range = light.range;
    double cutOff = glm::degrees(
        std::acos(std::clamp(static_cast<double>(light.cutOff), -1.0, 1.0)));
    double outerCutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(light.outerCutoff), -1.0, 1.0)));
    double intensity = light.intensity;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "direction");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, direction);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "range", range);
    readNumberProperty(ctx, wrapper, "cutOff", cutOff);
    readNumberProperty(ctx, wrapper, "outerCutOff", outerCutOff);
    readNumberProperty(ctx, wrapper, "intensity", intensity);

    light.position = position;
    light.direction = direction.normalized();
    light.setColor(color);
    light.shineColor = shineColor;
    light.range = static_cast<float>(range);
    light.cutOff =
        glm::cos(glm::radians(static_cast<double>(static_cast<float>(cutOff))));
    light.outerCutoff = glm::cos(
        glm::radians(static_cast<double>(static_cast<float>(outerCutOff))));
    light.intensity = static_cast<float>(intensity);
    syncSpotLightDebugObject(light);
    return true;
}

bool applyAreaLight(JSContext *ctx, JSValueConst wrapper, AreaLight &light) {
    Position3d position = light.position;
    Magnitude3d right = light.right;
    Magnitude3d up = light.up;
    Size2d size = light.size;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;
    double range = light.range;
    double angle = light.angle;
    bool castsBothSides = light.castsBothSides;
    Rotation3d rotation = light.rotation;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "right");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, right);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "up");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, up);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "size");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseSize2d(ctx, value, size);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);
    readNumberProperty(ctx, wrapper, "range", range);
    readNumberProperty(ctx, wrapper, "angle", angle);
    readBoolProperty(ctx, wrapper, "castsBothSides", castsBothSides);

    light.position = position;
    light.right = right.normalized();
    light.up = up.normalized();
    light.size = size;
    light.setColor(color);
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    light.range = static_cast<float>(range);
    light.angle = static_cast<float>(angle);
    light.castsBothSides = castsBothSides;
    light.rotation = rotation;
    syncAreaLightDebugObject(light);
    return true;
}

bool parseStringArray(JSContext *ctx, JSValueConst value,
                      std::vector<std::string> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    JSValue lengthValue = JS_GetPropertyStr(ctx, value, "length");
    if (JS_IsException(lengthValue) || !getUint32(ctx, lengthValue, length)) {
        JS_FreeValue(ctx, lengthValue);
        return false;
    }
    JS_FreeValue(ctx, lengthValue);

    out.clear();
    out.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item)) {
            return false;
        }
        const char *text = JS_ToCString(ctx, item);
        if (text == nullptr) {
            JS_FreeValue(ctx, item);
            return false;
        }
        out.emplace_back(text);
        JS_FreeCString(ctx, text);
        JS_FreeValue(ctx, item);
    }
    return true;
}

int toScriptQueryOperation(QueryOperation operation) {
    switch (operation) {
    case QueryOperation::RaycastAll:
        return 0;
    case QueryOperation::Raycast:
        return 1;
    case QueryOperation::RaycastWorld:
        return 2;
    case QueryOperation::RaycastWorldAll:
        return 3;
    case QueryOperation::RaycastTagged:
        return 4;
    case QueryOperation::RaycastTaggedAll:
        return 5;
    case QueryOperation::Movement:
        return 6;
    case QueryOperation::Overlap:
        return 7;
    case QueryOperation::MovementAll:
        return 8;
    default:
        return 1;
    }
}

MotionType parseMotionTypeValue(const std::string &value) {
    const std::string normalized = normalizeToken(value);
    if (normalized == "static") {
        return MotionType::Static;
    }
    if (normalized == "kinematic") {
        return MotionType::Kinematic;
    }
    return MotionType::Dynamic;
}

std::shared_ptr<bezel::Collider>
makeColliderFromScript(JSContext *ctx, JSValueConst value, GameObject *owner) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return nullptr;
    }

    double radius = 0.0;
    double height = 0.0;
    if (readNumberProperty(ctx, value, "radius", radius) &&
        readNumberProperty(ctx, value, "height", height)) {
        return std::make_shared<bezel::CapsuleCollider>(
            static_cast<float>(radius), static_cast<float>(height));
    }

    JSValue sizeValue = JS_GetPropertyStr(ctx, value, "size");
    if (!JS_IsException(sizeValue) && !JS_IsUndefined(sizeValue)) {
        Size3d size;
        if (parsePosition3d(ctx, sizeValue, size)) {
            JS_FreeValue(ctx, sizeValue);
            return std::make_shared<bezel::BoxCollider>(size / 2.0);
        }
    }
    JS_FreeValue(ctx, sizeValue);

    if (readNumberProperty(ctx, value, "radius", radius)) {
        return std::make_shared<bezel::SphereCollider>(
            static_cast<float>(radius));
    }

    auto *coreObject = dynamic_cast<CoreObject *>(owner);
    if (coreObject == nullptr) {
        return nullptr;
    }

    std::vector<Position3d> vertices;
    vertices.reserve(coreObject->getVertices().size());
    for (const auto &vertex : coreObject->getVertices()) {
        vertices.push_back(vertex.position);
    }
    return std::make_shared<bezel::MeshCollider>(vertices, coreObject->indices);
}

bool parseSpringValue(JSContext *ctx, JSValueConst value, Spring &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    readBoolProperty(ctx, value, "enabled", out.enabled);

    std::int64_t mode = static_cast<std::int64_t>(out.mode);
    readIntProperty(ctx, value, "mode", mode);
    out.mode = static_cast<SpringMode>(static_cast<int>(mode));

    double frequency = out.frequencyHz;
    readNumberProperty(ctx, value, "frequency", frequency);
    out.frequencyHz = static_cast<float>(frequency);

    double dampingRatio = out.dampingRatio;
    readNumberProperty(ctx, value, "dampingRatio", dampingRatio);
    out.dampingRatio = static_cast<float>(dampingRatio);

    double stiffness = out.stiffness;
    readNumberProperty(ctx, value, "stiffness", stiffness);
    out.stiffness = static_cast<float>(stiffness);

    double damping = out.damping;
    readNumberProperty(ctx, value, "damping", damping);
    out.damping = static_cast<float>(damping);
    return true;
}

bool parseAngleLimitsValue(JSContext *ctx, JSValueConst value,
                           AngleLimits &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    readBoolProperty(ctx, value, "enabled", out.enabled);

    double minAngle = out.minAngle;
    readNumberProperty(ctx, value, "minAngle", minAngle);
    out.minAngle = static_cast<float>(minAngle);

    double maxAngle = out.maxAngle;
    readNumberProperty(ctx, value, "maxAngle", maxAngle);
    out.maxAngle = static_cast<float>(maxAngle);
    return true;
}

bool parseMotorValue(JSContext *ctx, JSValueConst value, Motor &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    readBoolProperty(ctx, value, "enabled", out.enabled);

    double maxForce = out.maxForce;
    readNumberProperty(ctx, value, "maxForce", maxForce);
    out.maxForce = static_cast<float>(maxForce);

    double maxTorque = out.maxTorque;
    readNumberProperty(ctx, value, "maxTorque", maxTorque);
    out.maxTorque = static_cast<float>(maxTorque);
    return true;
}

bool parseVehicleWheelSettingsValue(JSContext *ctx, JSValueConst value,
                                    bezel::VehicleWheelSettings &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    JSValue field = JS_GetPropertyStr(ctx, value, "position");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.position);
    }
    JS_FreeValue(ctx, field);

    readBoolProperty(ctx, value, "enableSuspensionForcePoint",
                     out.enableSuspensionForcePoint);

    field = JS_GetPropertyStr(ctx, value, "suspensionForcePoint");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.suspensionForcePoint);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "suspensionDirection");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.suspensionDirection);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "steeringAxis");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.steeringAxis);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "wheelUp");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.wheelUp);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "wheelForward");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.wheelForward);
    }
    JS_FreeValue(ctx, field);

    double number = 0.0;
    number = out.suspensionMinLength;
    readNumberProperty(ctx, value, "suspensionMinLength", number);
    out.suspensionMinLength = static_cast<float>(number);
    number = out.suspensionMaxLength;
    readNumberProperty(ctx, value, "suspensionMaxLength", number);
    out.suspensionMaxLength = static_cast<float>(number);
    number = out.suspensionPreloadLength;
    readNumberProperty(ctx, value, "suspensionPreloadLength", number);
    out.suspensionPreloadLength = static_cast<float>(number);
    number = out.suspensionFrequencyHz;
    readNumberProperty(ctx, value, "suspensionFrequencyHz", number);
    out.suspensionFrequencyHz = static_cast<float>(number);
    number = out.suspensionDampingRatio;
    readNumberProperty(ctx, value, "suspensionDampingRatio", number);
    out.suspensionDampingRatio = static_cast<float>(number);
    number = out.radius;
    readNumberProperty(ctx, value, "radius", number);
    out.radius = static_cast<float>(number);
    number = out.width;
    readNumberProperty(ctx, value, "width", number);
    out.width = static_cast<float>(number);
    number = out.inertia;
    readNumberProperty(ctx, value, "inertia", number);
    out.inertia = static_cast<float>(number);
    number = out.angularDamping;
    readNumberProperty(ctx, value, "angularDamping", number);
    out.angularDamping = static_cast<float>(number);
    number = out.maxSteerAngleDeg;
    if (!readNumberProperty(ctx, value, "maxSteerAngleDeg", number)) {
        readNumberProperty(ctx, value, "maxSteerAngleDegrees", number);
    }
    out.maxSteerAngleDeg = static_cast<float>(number);
    number = out.maxBrakeTorque;
    readNumberProperty(ctx, value, "maxBrakeTorque", number);
    out.maxBrakeTorque = static_cast<float>(number);
    number = out.maxHandBrakeTorque;
    readNumberProperty(ctx, value, "maxHandBrakeTorque", number);
    out.maxHandBrakeTorque = static_cast<float>(number);
    return true;
}

bool parseVehicleDifferentialValue(JSContext *ctx, JSValueConst value,
                                   bezel::VehicleDifferential &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    std::int64_t intValue = out.leftWheel;
    readIntProperty(ctx, value, "leftWheel", intValue);
    out.leftWheel = static_cast<int>(intValue);
    intValue = out.rightWheel;
    readIntProperty(ctx, value, "rightWheel", intValue);
    out.rightWheel = static_cast<int>(intValue);

    double number = out.differentialRatio;
    readNumberProperty(ctx, value, "differentialRatio", number);
    out.differentialRatio = static_cast<float>(number);
    number = out.leftRightSplit;
    readNumberProperty(ctx, value, "leftRightSplit", number);
    out.leftRightSplit = static_cast<float>(number);
    number = out.limitedSlipRatio;
    readNumberProperty(ctx, value, "limitedSlipRatio", number);
    out.limitedSlipRatio = static_cast<float>(number);
    number = out.engineTorqueRatio;
    readNumberProperty(ctx, value, "engineTorqueRatio", number);
    out.engineTorqueRatio = static_cast<float>(number);
    return true;
}

bool parseVehicleEngineValue(JSContext *ctx, JSValueConst value,
                             bezel::VehicleEngine &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    double number = out.maxTorque;
    readNumberProperty(ctx, value, "maxTorque", number);
    out.maxTorque = static_cast<float>(number);
    number = out.minRPM;
    readNumberProperty(ctx, value, "minRPM", number);
    out.minRPM = static_cast<float>(number);
    number = out.maxRPM;
    readNumberProperty(ctx, value, "maxRPM", number);
    out.maxRPM = static_cast<float>(number);
    number = out.inertia;
    readNumberProperty(ctx, value, "inertia", number);
    out.inertia = static_cast<float>(number);
    number = out.angularDamping;
    readNumberProperty(ctx, value, "angularDamping", number);
    out.angularDamping = static_cast<float>(number);
    return true;
}

bool parseFloatVector(JSContext *ctx, JSValueConst value,
                      std::vector<float> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    JSValue lengthValue = JS_GetPropertyStr(ctx, value, "length");
    if (JS_IsException(lengthValue) || !getUint32(ctx, lengthValue, length)) {
        JS_FreeValue(ctx, lengthValue);
        return false;
    }
    JS_FreeValue(ctx, lengthValue);

    out.clear();
    out.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item)) {
            return false;
        }
        double number = 0.0;
        const bool ok = getDouble(ctx, item, number);
        JS_FreeValue(ctx, item);
        if (!ok) {
            return false;
        }
        out.push_back(static_cast<float>(number));
    }
    return true;
}

bool parseVehicleTransmissionValue(JSContext *ctx, JSValueConst value,
                                   bezel::VehicleTransmission &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    std::int64_t mode = static_cast<std::int64_t>(out.mode);
    readIntProperty(ctx, value, "mode", mode);
    out.mode =
        static_cast<bezel::VehicleTransmissionMode>(static_cast<int>(mode));

    JSValue field = JS_GetPropertyStr(ctx, value, "gearRatios");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseFloatVector(ctx, field, out.gearRatios);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "reverseGearRatios");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseFloatVector(ctx, field, out.reverseGearRatios);
    }
    JS_FreeValue(ctx, field);

    double number = out.switchTime;
    readNumberProperty(ctx, value, "switchTime", number);
    out.switchTime = static_cast<float>(number);
    number = out.clutchReleaseTime;
    readNumberProperty(ctx, value, "clutchReleaseTime", number);
    out.clutchReleaseTime = static_cast<float>(number);
    number = out.switchLatency;
    readNumberProperty(ctx, value, "switchLatency", number);
    out.switchLatency = static_cast<float>(number);
    number = out.shiftUpRPM;
    readNumberProperty(ctx, value, "shiftUpRPM", number);
    out.shiftUpRPM = static_cast<float>(number);
    number = out.shiftDownRPM;
    readNumberProperty(ctx, value, "shiftDownRPM", number);
    out.shiftDownRPM = static_cast<float>(number);
    number = out.clutchStrength;
    readNumberProperty(ctx, value, "clutchStrength", number);
    out.clutchStrength = static_cast<float>(number);
    return true;
}

bool parseVehicleControllerSettingsValue(
    JSContext *ctx, JSValueConst value, bezel::VehicleControllerSettings &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    JSValue field = JS_GetPropertyStr(ctx, value, "engine");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseVehicleEngineValue(ctx, field, out.engine);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "transmission");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseVehicleTransmissionValue(ctx, field, out.transmission);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "differentials");
    if (!JS_IsException(field) && JS_IsArray(field)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, field, "length");
        if (!JS_IsException(lengthValue) &&
            getUint32(ctx, lengthValue, length)) {
            out.differentials.clear();
            out.differentials.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue item = JS_GetPropertyUint32(ctx, field, i);
                if (!JS_IsException(item)) {
                    bezel::VehicleDifferential differential;
                    if (parseVehicleDifferentialValue(ctx, item,
                                                      differential)) {
                        out.differentials.push_back(differential);
                    }
                }
                JS_FreeValue(ctx, item);
            }
        }
        JS_FreeValue(ctx, lengthValue);
    }
    JS_FreeValue(ctx, field);

    double number = out.differentialLimitedSlipRatio;
    readNumberProperty(ctx, value, "differentialLimitedSlipRatio", number);
    out.differentialLimitedSlipRatio = static_cast<float>(number);
    return true;
}

bool parseVehicleSettingsValue(JSContext *ctx, JSValueConst value,
                               bezel::VehicleSettings &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    JSValue field = JS_GetPropertyStr(ctx, value, "up");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.up);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "forward");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parsePosition3d(ctx, field, out.forward);
    }
    JS_FreeValue(ctx, field);

    double number = out.maxPitchRollAngleDeg;
    readNumberProperty(ctx, value, "maxPitchRollAngleDeg", number);
    out.maxPitchRollAngleDeg = static_cast<float>(number);

    field = JS_GetPropertyStr(ctx, value, "wheels");
    if (!JS_IsException(field) && JS_IsArray(field)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, field, "length");
        if (!JS_IsException(lengthValue) &&
            getUint32(ctx, lengthValue, length)) {
            out.wheels.clear();
            out.wheels.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue item = JS_GetPropertyUint32(ctx, field, i);
                if (!JS_IsException(item)) {
                    bezel::VehicleWheelSettings wheel;
                    if (parseVehicleWheelSettingsValue(ctx, item, wheel)) {
                        out.wheels.push_back(wheel);
                    }
                }
                JS_FreeValue(ctx, item);
            }
        }
        JS_FreeValue(ctx, lengthValue);
    }
    JS_FreeValue(ctx, field);

    field = JS_GetPropertyStr(ctx, value, "controller");
    if (!JS_IsException(field) && !JS_IsUndefined(field)) {
        parseVehicleControllerSettingsValue(ctx, field, out.controller);
    }
    JS_FreeValue(ctx, field);

    number = out.maxSlopeAngleDeg;
    if (!readNumberProperty(ctx, value, "maxSlopeAngleDeg", number)) {
        readNumberProperty(ctx, value, "maxSlopAngleDeg", number);
    }
    out.maxSlopeAngleDeg = static_cast<float>(number);
    return true;
}

bool parseJointMember(JSContext *ctx, ScriptHost &host, JSValueConst value,
                      JointChild &out) {
    if (JS_IsNull(value) || JS_IsUndefined(value)) {
        out = WorldBody{};
        return true;
    }

    std::int64_t objectId = 0;
    if (readIntProperty(ctx, value, ATLAS_OBJECT_ID_PROP, objectId) ||
        readIntProperty(ctx, value, "id", objectId)) {
        GameObject *object = findObjectById(host, static_cast<int>(objectId));
        if (object != nullptr) {
            out = object;
            return true;
        }
        return false;
    }

    out = WorldBody{};
    return true;
}

bezel::RaycastResult
convertToTaggedRaycastResult(const bezel::RaycastResult &input,
                             const std::vector<std::string> &tags) {
    if (tags.empty()) {
        return input;
    }

    bezel::RaycastResult result;
    result.closestDistance = -1.0f;
    for (const auto &hit : input.hits) {
        if (hit.rigidbody == nullptr) {
            continue;
        }
        if (std::find_first_of(hit.rigidbody->tags.begin(),
                               hit.rigidbody->tags.end(), tags.begin(),
                               tags.end()) == hit.rigidbody->tags.end()) {
            continue;
        }
        result.hits.push_back(hit);
    }

    if (!result.hits.empty()) {
        result.hit = result.hits.front();
        result.closestDistance = result.hit.distance;
    }
    return result;
}

RaycastResult convertRaycastResult(const bezel::RaycastResult &input) {
    RaycastResult result;
    result.closestDistance = input.closestDistance;
    result.hits.reserve(input.hits.size());
    for (const auto &hit : input.hits) {
        RaycastHit converted;
        converted.position = hit.position;
        converted.normal = hit.normal;
        converted.distance = hit.distance;
        converted.rigidbody = hit.rigidbody;
        converted.didHit = hit.didHit;
        if (hit.rigidbody != nullptr) {
            auto it = atlas::gameObjects.find(
                static_cast<int>(hit.rigidbody->id.atlasId));
            if (it != atlas::gameObjects.end()) {
                converted.object = it->second;
            }
        }
        result.hits.push_back(converted);
    }
    if (!result.hits.empty()) {
        result.hit = result.hits.front();
    }
    return result;
}

OverlapResult convertOverlapResultToAtlas(const bezel::OverlapResult &input) {
    OverlapResult result;
    result.hitAny = input.hitAny;
    result.hits.reserve(input.hits.size());
    for (const auto &hit : input.hits) {
        OverlapHit converted;
        converted.contactPoint = hit.contactPoint;
        converted.penetrationAxis = hit.penetrationAxis;
        converted.penetrationDepth = hit.penetrationDepth;
        converted.rigidbody = hit.rigidbody;
        if (hit.rigidbody != nullptr) {
            auto it = atlas::gameObjects.find(
                static_cast<int>(hit.rigidbody->id.atlasId));
            if (it != atlas::gameObjects.end()) {
                converted.object = it->second;
            }
        }
        result.hits.push_back(converted);
    }
    return result;
}

SweepResult convertSweepResultToAtlas(const bezel::SweepResult &input,
                                      const Position3d &endPosition) {
    SweepResult result;
    result.hitAny = input.hitAny;
    result.endPosition = endPosition;
    result.hits.reserve(input.hits.size());
    for (const auto &hit : input.hits) {
        SweepHit converted;
        converted.position = hit.position;
        converted.normal = hit.normal;
        converted.distance = hit.distance;
        converted.percentage = hit.percentage;
        converted.rigidbody = hit.rigidbody;
        if (hit.rigidbody != nullptr) {
            auto it = atlas::gameObjects.find(
                static_cast<int>(hit.rigidbody->id.atlasId));
            if (it != atlas::gameObjects.end()) {
                converted.object = it->second;
            }
        }
        result.hits.push_back(converted);
    }
    if (input.hitAny) {
        result.closest = result.hits.empty() ? SweepHit{} : result.hits.front();
    }
    return result;
}

JSValue makeRaycastHitValue(JSContext *ctx, ScriptHost &host,
                            const RaycastHit &hit) {
    JSValue value = JS_NewObject(ctx);
    setProperty(ctx, value, "position",
                makePosition3d(ctx, host, hit.position));
    setProperty(ctx, value, "normal", makePosition3d(ctx, host, hit.normal));
    setProperty(ctx, value, "distance", JS_NewFloat64(ctx, hit.distance));
    setProperty(ctx, value, "object",
                hit.object != nullptr
                    ? syncObjectWrapper(ctx, host, *hit.object)
                    : JS_NULL);
    setProperty(ctx, value, "didHit", JS_NewBool(ctx, hit.didHit));
    return value;
}

JSValue makeRaycastResultValue(JSContext *ctx, ScriptHost &host,
                               const RaycastResult &result) {
    JSValue value = JS_NewObject(ctx);
    JSValue hits = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < result.hits.size(); ++i) {
        JS_SetPropertyUint32(ctx, hits, i,
                             makeRaycastHitValue(ctx, host, result.hits[i]));
    }
    setProperty(ctx, value, "hits", hits);
    setProperty(ctx, value, "hit",
                result.hits.empty()
                    ? JS_NULL
                    : makeRaycastHitValue(ctx, host, result.hit));
    setProperty(ctx, value, "closestDistance",
                JS_NewFloat64(ctx, result.closestDistance));
    return value;
}

JSValue makeOverlapHitValue(JSContext *ctx, ScriptHost &host,
                            const OverlapHit &hit) {
    JSValue value = JS_NewObject(ctx);
    setProperty(ctx, value, "contactPoint",
                makePosition3d(ctx, host, hit.contactPoint));
    JSValue axis = makePosition3d(ctx, host, hit.penetrationAxis);
    setProperty(ctx, value, "penerationAxis", JS_DupValue(ctx, axis));
    setProperty(ctx, value, "penetrationAxis", axis);
    setProperty(ctx, value, "penetrationDepth",
                JS_NewFloat64(ctx, hit.penetrationDepth));
    setProperty(ctx, value, "object",
                hit.object != nullptr
                    ? syncObjectWrapper(ctx, host, *hit.object)
                    : JS_NULL);
    return value;
}

JSValue makeOverlapResultValue(JSContext *ctx, ScriptHost &host,
                               const OverlapResult &result) {
    JSValue value = JS_NewObject(ctx);
    JSValue hits = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < result.hits.size(); ++i) {
        JS_SetPropertyUint32(ctx, hits, i,
                             makeOverlapHitValue(ctx, host, result.hits[i]));
    }
    setProperty(ctx, value, "hits", hits);
    setProperty(ctx, value, "hitAny", JS_NewBool(ctx, result.hitAny));
    return value;
}

JSValue makeSweepHitValue(JSContext *ctx, ScriptHost &host,
                          const SweepHit &hit) {
    JSValue value = JS_NewObject(ctx);
    setProperty(ctx, value, "position",
                makePosition3d(ctx, host, hit.position));
    setProperty(ctx, value, "normal", makePosition3d(ctx, host, hit.normal));
    setProperty(ctx, value, "distance", JS_NewFloat64(ctx, hit.distance));
    setProperty(ctx, value, "percentage", JS_NewFloat64(ctx, hit.percentage));
    setProperty(ctx, value, "object",
                hit.object != nullptr
                    ? syncObjectWrapper(ctx, host, *hit.object)
                    : JS_NULL);
    return value;
}

JSValue makeSweepResultValue(JSContext *ctx, ScriptHost &host,
                             const SweepResult &result) {
    JSValue value = JS_NewObject(ctx);
    JSValue hits = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < result.hits.size(); ++i) {
        JS_SetPropertyUint32(ctx, hits, i,
                             makeSweepHitValue(ctx, host, result.hits[i]));
    }
    setProperty(ctx, value, "hits", hits);
    setProperty(ctx, value, "closest",
                result.hitAny ? makeSweepHitValue(ctx, host, result.closest)
                              : JS_NULL);
    setProperty(ctx, value, "hitAny", JS_NewBool(ctx, result.hitAny));
    setProperty(ctx, value, "endPosition",
                makePosition3d(ctx, host, result.endPosition));
    return value;
}

JSValue makeQueryResultValue(JSContext *ctx, ScriptHost &host,
                             const QueryResult &result) {
    JSValue value = JS_NewObject(ctx);
    setProperty(ctx, value, "operation",
                JS_NewInt32(ctx, toScriptQueryOperation(result.operation)));

    switch (result.operation) {
    case QueryOperation::Raycast:
    case QueryOperation::RaycastAll:
    case QueryOperation::RaycastWorld:
    case QueryOperation::RaycastWorldAll:
    case QueryOperation::RaycastTagged:
    case QueryOperation::RaycastTaggedAll:
        setProperty(ctx, value, "raycastResult",
                    makeRaycastResultValue(ctx, host, result.raycastResult));
        break;
    case QueryOperation::Overlap:
        setProperty(ctx, value, "overlapResult",
                    makeOverlapResultValue(ctx, host, result.overlapResult));
        break;
    case QueryOperation::Movement:
    case QueryOperation::MovementAll:
        setProperty(ctx, value, "sweepResult",
                    makeSweepResultValue(ctx, host, result.sweepResult));
        break;
    default:
        break;
    }

    return value;
}

void dispatchQueryResultToObject(JSContext *ctx, ScriptHost &host,
                                 Rigidbody &component,
                                 const QueryResult &result) {
    if (component.object == nullptr) {
        return;
    }
    component.object->onQueryReceive(const_cast<QueryResult &>(result));
    auto objectValue = syncObjectWrapper(ctx, host, *component.object);
    JS_FreeValue(ctx, objectValue);
}

bool applyRigidbody(JSContext *ctx, JSValueConst wrapper,
                    Rigidbody &component) {
    std::string sendSignal = component.sendSignal;
    readStringProperty(ctx, wrapper, "sendSignal", sendSignal);
    component.sendSignal = sendSignal;

    bool isSensor = component.isSensor;
    readBoolProperty(ctx, wrapper, "isSensor", isSensor);
    component.isSensor = isSensor;

    if (component.body != nullptr) {
        component.body->sensorSignal = component.sendSignal;
        component.body->isSensor = component.isSensor;
        if (component.object != nullptr) {
            component.body->id.atlasId = component.object->getId();
        }
    }
    return true;
}

std::shared_ptr<bezel::Rigidbody> ensureBezelBody(Rigidbody &component) {
    if (!component.body) {
        component.body = std::make_shared<bezel::Rigidbody>();
    }
    if (component.object != nullptr) {
        component.body->id.atlasId = component.object->getId();
    }
    component.body->isSensor = component.isSensor;
    component.body->sensorSignal = component.sendSignal;
    return component.body;
}

bool applyVehicle(JSContext *ctx, JSValueConst wrapper, Vehicle &component) {
    JSValue settingsValue = JS_GetPropertyStr(ctx, wrapper, "settings");
    if (!JS_IsException(settingsValue) && !JS_IsUndefined(settingsValue)) {
        parseVehicleSettingsValue(ctx, settingsValue, component.settings);
    }
    JS_FreeValue(ctx, settingsValue);

    double forward = component.forward;
    readNumberProperty(ctx, wrapper, "forward", forward);
    component.forward = static_cast<float>(forward);

    double right = component.right;
    readNumberProperty(ctx, wrapper, "right", right);
    component.right = static_cast<float>(right);

    double brake = component.brake;
    readNumberProperty(ctx, wrapper, "brake", brake);
    component.brake = static_cast<float>(brake);

    double handBrake = component.handBrake;
    readNumberProperty(ctx, wrapper, "handBrake", handBrake);
    component.handBrake = static_cast<float>(handBrake);
    return true;
}

bool applyJointBase(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                    Joint &component) {
    JSValue parentValue = JS_GetPropertyStr(ctx, wrapper, "parent");
    if (!JS_IsException(parentValue) && !JS_IsUndefined(parentValue)) {
        parseJointMember(ctx, host, parentValue, component.parent);
    }
    JS_FreeValue(ctx, parentValue);

    JSValue childValue = JS_GetPropertyStr(ctx, wrapper, "child");
    if (!JS_IsException(childValue) && !JS_IsUndefined(childValue)) {
        parseJointMember(ctx, host, childValue, component.child);
    }
    JS_FreeValue(ctx, childValue);

    std::int64_t space = static_cast<std::int64_t>(component.space);
    readIntProperty(ctx, wrapper, "space", space);
    component.space = static_cast<Space>(static_cast<int>(space));

    JSValue anchorValue = JS_GetPropertyStr(ctx, wrapper, "anchor");
    if (!JS_IsException(anchorValue) && !JS_IsUndefined(anchorValue)) {
        parsePosition3d(ctx, anchorValue, component.anchor);
    }
    JS_FreeValue(ctx, anchorValue);

    double breakForce = component.breakForce;
    readNumberProperty(ctx, wrapper, "breakForce", breakForce);
    component.breakForce = static_cast<float>(breakForce);

    double breakTorque = component.breakTorque;
    readNumberProperty(ctx, wrapper, "breakTorque", breakTorque);
    component.breakTorque = static_cast<float>(breakTorque);
    return true;
}

bool applyFixedJoint(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     FixedJoint &component) {
    return applyJointBase(ctx, host, wrapper, component);
}

bool applyHingeJoint(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     HingeJoint &component) {
    applyJointBase(ctx, host, wrapper, component);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "axis1");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, component.axis1);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "axis2");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, component.axis2);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "angleLimits");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseAngleLimitsValue(ctx, value, component.limits);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "motor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseMotorValue(ctx, value, component.motor);
    }
    JS_FreeValue(ctx, value);
    return true;
}

bool applySpringJoint(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                      SpringJoint &component) {
    applyJointBase(ctx, host, wrapper, component);

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "anchorB");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, component.anchorB);
    }
    JS_FreeValue(ctx, value);

    double number = component.restLength;
    readNumberProperty(ctx, wrapper, "restLength", number);
    component.restLength = static_cast<float>(number);
    readBoolProperty(ctx, wrapper, "useLimits", component.useLimits);
    number = component.minLength;
    readNumberProperty(ctx, wrapper, "minLength", number);
    component.minLength = static_cast<float>(number);
    number = component.maxLength;
    readNumberProperty(ctx, wrapper, "maxLength", number);
    component.maxLength = static_cast<float>(number);

    value = JS_GetPropertyStr(ctx, wrapper, "spring");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseSpringValue(ctx, value, component.spring);
    }
    JS_FreeValue(ctx, value);
    return true;
}

class HostedRigidbodyComponent final : public Rigidbody {
  public:
    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::uint64_t scriptId = 0;

    void atAttach() override {
        Rigidbody::atAttach();
        syncFromWrapper();
    }

    void init() override {
        syncFromWrapper();
        Rigidbody::init();
    }

    void beforePhysics() override {
        syncFromWrapper();
        Rigidbody::beforePhysics();
    }

    void update(float dt) override { Rigidbody::update(dt); }

  private:
    void syncFromWrapper() {
        if (host == nullptr) {
            return;
        }
        auto *state = findRigidbodyState(*host, scriptId);
        if (state == nullptr || JS_IsUndefined(state->value)) {
            return;
        }
        applyRigidbody(ctx, state->value, *this);
    }
};

class HostedVehicleComponent final : public Component {
  public:
    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::uint64_t scriptId = 0;
    Vehicle vehicle;

    void atAttach() override {
        vehicle.object = object;
        syncFromWrapper();
        vehicle.atAttach();
    }

    void beforePhysics() override {
        vehicle.object = object;
        syncFromWrapper();
        vehicle.beforePhysics();
    }

  private:
    void syncFromWrapper() {
        if (host == nullptr) {
            return;
        }
        auto *state = findVehicleState(*host, scriptId);
        if (state == nullptr || JS_IsUndefined(state->value)) {
            return;
        }
        applyVehicle(ctx, state->value, vehicle);
    }
};

class HostedFixedJointComponent final : public Component {
  public:
    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::uint64_t scriptId = 0;
    FixedJoint joint;

    void atAttach() override {
        joint.object = object;
        syncFromWrapper();
    }

    void beforePhysics() override {
        joint.object = object;
        syncFromWrapper();
        joint.beforePhysics();
    }

    void breakJoint() { joint.breakJoint(); }

    FixedJoint *nativeComponent() { return &joint; }

  private:
    void syncFromWrapper() {
        if (host == nullptr) {
            return;
        }
        auto *state = findFixedJointState(*host, scriptId);
        if (state == nullptr || JS_IsUndefined(state->value)) {
            return;
        }
        applyFixedJoint(ctx, *host, state->value, joint);
    }
};

class HostedHingeJointComponent final : public Component {
  public:
    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::uint64_t scriptId = 0;
    HingeJoint joint;

    void atAttach() override {
        joint.object = object;
        syncFromWrapper();
    }

    void beforePhysics() override {
        joint.object = object;
        syncFromWrapper();
        joint.beforePhysics();
    }

    void breakJoint() { joint.breakJoint(); }

    HingeJoint *nativeComponent() { return &joint; }

  private:
    void syncFromWrapper() {
        if (host == nullptr) {
            return;
        }
        auto *state = findHingeJointState(*host, scriptId);
        if (state == nullptr || JS_IsUndefined(state->value)) {
            return;
        }
        applyHingeJoint(ctx, *host, state->value, joint);
    }
};

class HostedSpringJointComponent final : public Component {
  public:
    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::uint64_t scriptId = 0;
    SpringJoint joint;

    void atAttach() override {
        joint.object = object;
        syncFromWrapper();
    }

    void beforePhysics() override {
        joint.object = object;
        syncFromWrapper();
        joint.beforePhysics();
    }

    void breakJoint() { joint.breakJoint(); }

    SpringJoint *nativeComponent() { return &joint; }

  private:
    void syncFromWrapper() {
        if (host == nullptr) {
            return;
        }
        auto *state = findSpringJointState(*host, scriptId);
        if (state == nullptr || JS_IsUndefined(state->value)) {
            return;
        }
        applySpringJoint(ctx, *host, state->value, joint);
    }
};

std::uint64_t registerRigidbodyState(
    ScriptHost &host, const std::shared_ptr<Rigidbody> &component,
    JSContext *ctx, JSValueConst wrapper, bool attached = false) {
    const std::uint64_t id = host.nextRigidbodyId++;
    host.rigidbodies[id] = {.ownedComponent = component,
                            .component = component.get(),
                            .value = JS_DupValue(ctx, wrapper),
                            .attached = attached};
    if (auto *hosted =
            dynamic_cast<HostedRigidbodyComponent *>(component.get())) {
        hosted->scriptId = id;
    }
    return id;
}

std::uint64_t registerVehicleState(ScriptHost &host,
                                   const std::shared_ptr<Component> &component,
                                   Vehicle *nativeComponent, JSContext *ctx,
                                   JSValueConst wrapper,
                                   bool attached = false) {
    const std::uint64_t id = host.nextVehicleId++;
    host.vehicles[id] = {.ownedComponent = component,
                         .component = nativeComponent,
                         .value = JS_DupValue(ctx, wrapper),
                         .attached = attached};
    if (auto *hosted =
            dynamic_cast<HostedVehicleComponent *>(component.get())) {
        hosted->scriptId = id;
    }
    return id;
}

std::uint64_t
registerFixedJointState(ScriptHost &host,
                        const std::shared_ptr<Component> &component,
                        FixedJoint *nativeComponent, JSContext *ctx,
                        JSValueConst wrapper, bool attached = false) {
    const std::uint64_t id = host.nextFixedJointId++;
    host.fixedJoints[id] = {.ownedComponent = component,
                            .component = nativeComponent,
                            .value = JS_DupValue(ctx, wrapper),
                            .attached = attached};
    if (auto *hosted =
            dynamic_cast<HostedFixedJointComponent *>(component.get())) {
        hosted->scriptId = id;
    }
    return id;
}

std::uint64_t
registerHingeJointState(ScriptHost &host,
                        const std::shared_ptr<Component> &component,
                        HingeJoint *nativeComponent, JSContext *ctx,
                        JSValueConst wrapper, bool attached = false) {
    const std::uint64_t id = host.nextHingeJointId++;
    host.hingeJoints[id] = {.ownedComponent = component,
                            .component = nativeComponent,
                            .value = JS_DupValue(ctx, wrapper),
                            .attached = attached};
    if (auto *hosted =
            dynamic_cast<HostedHingeJointComponent *>(component.get())) {
        hosted->scriptId = id;
    }
    return id;
}

std::uint64_t
registerSpringJointState(ScriptHost &host,
                         const std::shared_ptr<Component> &component,
                         SpringJoint *nativeComponent, JSContext *ctx,
                         JSValueConst wrapper, bool attached = false) {
    const std::uint64_t id = host.nextSpringJointId++;
    host.springJoints[id] = {.ownedComponent = component,
                             .component = nativeComponent,
                             .value = JS_DupValue(ctx, wrapper),
                             .attached = attached};
    if (auto *hosted =
            dynamic_cast<HostedSpringJointComponent *>(component.get())) {
        hosted->scriptId = id;
    }
    return id;
}

JSValue syncRigidbodyWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t rigidbodyId) {
    auto *state = findRigidbodyState(host, rigidbodyId);
    if (state == nullptr || state->component == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper =
        JS_IsUndefined(state->value)
            ? newObjectFromPrototype(ctx, state->component->isSensor
                                              ? host.sensorPrototype
                                              : host.rigidbodyPrototype)
            : JS_DupValue(ctx, state->value);
    if (JS_IsUndefined(state->value)) {
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_RIGIDBODY_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(rigidbodyId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, ATLAS_NATIVE_COMPONENT_KIND_PROP,
                JS_NewString(ctx, "rigidbody"));
    setProperty(ctx, wrapper, "sendSignal",
                JS_NewString(ctx, state->component->sendSignal.c_str()));
    setProperty(ctx, wrapper, "isSensor",
                JS_NewBool(ctx, state->component->isSensor));
    return wrapper;
}

JSValue syncVehicleWrapper(JSContext *ctx, ScriptHost &host,
                           std::uint64_t vehicleId) {
    auto *state = findVehicleState(host, vehicleId);
    if (state == nullptr || state->component == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_IsUndefined(state->value)
                          ? newObjectFromPrototype(ctx, host.vehiclePrototype)
                          : JS_DupValue(ctx, state->value);
    if (JS_IsUndefined(state->value)) {
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_VEHICLE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(vehicleId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, ATLAS_NATIVE_COMPONENT_KIND_PROP,
                JS_NewString(ctx, "vehicle"));
    setProperty(ctx, wrapper, "forward",
                JS_NewFloat64(ctx, state->component->forward));
    setProperty(ctx, wrapper, "right",
                JS_NewFloat64(ctx, state->component->right));
    setProperty(ctx, wrapper, "brake",
                JS_NewFloat64(ctx, state->component->brake));
    setProperty(ctx, wrapper, "handBrake",
                JS_NewFloat64(ctx, state->component->handBrake));
    return wrapper;
}

template <typename JointState>
JSValue syncJointWrapperBase(JSContext *ctx, ScriptHost &host,
                             JointState &state, std::uint64_t jointId,
                             JSValueConst prototype, const char *idProp,
                             const char *kind) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_IsUndefined(state.value)
                          ? newObjectFromPrototype(ctx, prototype)
                          : JS_DupValue(ctx, state.value);
    if (JS_IsUndefined(state.value)) {
        state.value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, idProp,
                JS_NewInt64(ctx, static_cast<int64_t>(jointId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, ATLAS_NATIVE_COMPONENT_KIND_PROP,
                JS_NewString(ctx, kind));
    return wrapper;
}

JSValue syncFixedJointWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t jointId) {
    auto *state = findFixedJointState(host, jointId);
    if (state == nullptr || state->component == nullptr) {
        return JS_NULL;
    }
    JSValue wrapper = syncJointWrapperBase(
        ctx, host, *state, jointId, host.fixedJointPrototype,
        ATLAS_FIXED_JOINT_ID_PROP, "fixed-joint");
    setProperty(ctx, wrapper, "anchor",
                makePosition3d(ctx, host, state->component->anchor));
    setProperty(ctx, wrapper, "breakForce",
                JS_NewFloat64(ctx, state->component->breakForce));
    setProperty(ctx, wrapper, "breakTorque",
                JS_NewFloat64(ctx, state->component->breakTorque));
    setProperty(ctx, wrapper, "space",
                JS_NewInt32(ctx, static_cast<int>(state->component->space)));
    return wrapper;
}

JSValue syncHingeJointWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t jointId) {
    auto *state = findHingeJointState(host, jointId);
    if (state == nullptr || state->component == nullptr) {
        return JS_NULL;
    }
    JSValue wrapper = syncJointWrapperBase(
        ctx, host, *state, jointId, host.hingeJointPrototype,
        ATLAS_HINGE_JOINT_ID_PROP, "hinge-joint");
    setProperty(ctx, wrapper, "anchor",
                makePosition3d(ctx, host, state->component->anchor));
    setProperty(ctx, wrapper, "breakForce",
                JS_NewFloat64(ctx, state->component->breakForce));
    setProperty(ctx, wrapper, "breakTorque",
                JS_NewFloat64(ctx, state->component->breakTorque));
    setProperty(ctx, wrapper, "space",
                JS_NewInt32(ctx, static_cast<int>(state->component->space)));
    setProperty(ctx, wrapper, "axis1",
                makePosition3d(ctx, host, state->component->axis1));
    setProperty(ctx, wrapper, "axis2",
                makePosition3d(ctx, host, state->component->axis2));
    JSValue limits = JS_NewObject(ctx);
    setProperty(ctx, limits, "enabled",
                JS_NewBool(ctx, state->component->limits.enabled));
    setProperty(ctx, limits, "minAngle",
                JS_NewFloat64(ctx, state->component->limits.minAngle));
    setProperty(ctx, limits, "maxAngle",
                JS_NewFloat64(ctx, state->component->limits.maxAngle));
    setProperty(ctx, wrapper, "angleLimits", limits);
    JSValue motor = JS_NewObject(ctx);
    setProperty(ctx, motor, "enabled",
                JS_NewBool(ctx, state->component->motor.enabled));
    setProperty(ctx, motor, "maxForce",
                JS_NewFloat64(ctx, state->component->motor.maxForce));
    setProperty(ctx, motor, "maxTorque",
                JS_NewFloat64(ctx, state->component->motor.maxTorque));
    setProperty(ctx, wrapper, "motor", motor);
    return wrapper;
}

JSValue syncSpringJointWrapper(JSContext *ctx, ScriptHost &host,
                               std::uint64_t jointId) {
    auto *state = findSpringJointState(host, jointId);
    if (state == nullptr || state->component == nullptr) {
        return JS_NULL;
    }
    JSValue wrapper = syncJointWrapperBase(
        ctx, host, *state, jointId, host.springJointPrototype,
        ATLAS_SPRING_JOINT_ID_PROP, "spring-joint");
    setProperty(ctx, wrapper, "anchor",
                makePosition3d(ctx, host, state->component->anchor));
    setProperty(ctx, wrapper, "breakForce",
                JS_NewFloat64(ctx, state->component->breakForce));
    setProperty(ctx, wrapper, "breakTorque",
                JS_NewFloat64(ctx, state->component->breakTorque));
    setProperty(ctx, wrapper, "space",
                JS_NewInt32(ctx, static_cast<int>(state->component->space)));
    setProperty(ctx, wrapper, "anchorB",
                makePosition3d(ctx, host, state->component->anchorB));
    setProperty(ctx, wrapper, "restLength",
                JS_NewFloat64(ctx, state->component->restLength));
    setProperty(ctx, wrapper, "useLimits",
                JS_NewBool(ctx, state->component->useLimits));
    setProperty(ctx, wrapper, "minLength",
                JS_NewFloat64(ctx, state->component->minLength));
    setProperty(ctx, wrapper, "maxLength",
                JS_NewFloat64(ctx, state->component->maxLength));
    JSValue spring = JS_NewObject(ctx);
    setProperty(ctx, spring, "enabled",
                JS_NewBool(ctx, state->component->spring.enabled));
    setProperty(
        ctx, spring, "mode",
        JS_NewInt32(ctx, static_cast<int>(state->component->spring.mode)));
    setProperty(ctx, spring, "frequency",
                JS_NewFloat64(ctx, state->component->spring.frequencyHz));
    setProperty(ctx, spring, "dampingRatio",
                JS_NewFloat64(ctx, state->component->spring.dampingRatio));
    setProperty(ctx, spring, "stiffness",
                JS_NewFloat64(ctx, state->component->spring.stiffness));
    setProperty(ctx, spring, "damping",
                JS_NewFloat64(ctx, state->component->spring.damping));
    setProperty(ctx, wrapper, "spring", spring);
    return wrapper;
}

GameObject *resolveObjectArg(JSContext *ctx, ScriptHost &host,
                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t objectId = 0;
    if (!readIntProperty(ctx, value, ATLAS_OBJECT_ID_PROP, objectId) &&
        !readIntProperty(ctx, value, "id", objectId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas object handle");
        return nullptr;
    }

    GameObject *object = findObjectById(host, static_cast<int>(objectId));
    if (object == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                               static_cast<int>(objectId));
    }
    return object;
}

CoreObject *resolveCoreObjectArg(JSContext *ctx, ScriptHost &host,
                                 JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *core = dynamic_cast<CoreObject *>(object);
    if (core == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a CoreObject");
        return nullptr;
    }
    return core;
}

Model *resolveModelArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *model = dynamic_cast<Model *>(object);
    if (model == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a Model");
        return nullptr;
    }
    return model;
}

ParticleEmitter *resolveParticleEmitterArg(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *emitter = dynamic_cast<ParticleEmitter *>(object);
    if (emitter == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a ParticleEmitter");
        return nullptr;
    }
    return emitter;
}

Instance *resolveInstanceArg(JSContext *ctx, ScriptHost &host,
                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t ownerId = 0;
    std::int64_t index = 0;
    if (!readIntProperty(ctx, value, ATLAS_INSTANCE_OWNER_ID_PROP, ownerId) ||
        !readIntProperty(ctx, value, ATLAS_INSTANCE_INDEX_PROP, index)) {
        JS_ThrowTypeError(ctx, "Expected Atlas instance handle");
        return nullptr;
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(host, static_cast<int>(ownerId)));
    if (object == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                               static_cast<int>(ownerId));
        return nullptr;
    }

    if (index < 0 ||
        static_cast<std::size_t>(index) >= object->instances.size()) {
        JS_ThrowRangeError(ctx, "Invalid Atlas instance index");
        return nullptr;
    }

    return &object->instances[static_cast<std::size_t>(index)];
}

Camera *resolveCameraArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isCamera = false;
    if (!readBoolProperty(ctx, value, ATLAS_CAMERA_PROP, isCamera) ||
        !isCamera) {
        JS_ThrowTypeError(ctx, "Expected Atlas camera handle");
        return nullptr;
    }

    Camera *camera = getSceneCamera(host);
    if (camera == nullptr) {
        JS_ThrowReferenceError(ctx, "Atlas camera is unavailable");
    }
    return camera;
}

Window *resolveWindowArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isWindow = false;
    if (!readBoolProperty(ctx, value, ATLAS_WINDOW_PROP, isWindow) ||
        !isWindow) {
        JS_ThrowTypeError(ctx, "Expected Atlas window handle");
        return nullptr;
    }

    Window *window = getWindow(host);
    if (window == nullptr) {
        JS_ThrowReferenceError(ctx, "Atlas window is unavailable");
    }
    return window;
}

Scene *resolveSceneArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isScene = false;
    if (!readBoolProperty(ctx, value, ATLAS_SCENE_PROP, isScene) || !isScene) {
        JS_ThrowTypeError(ctx, "Expected Atlas scene handle");
        return nullptr;
    }

    Scene *scene = getScene(host);
    if (scene == nullptr) {
        JS_ThrowReferenceError(ctx, "Atlas scene is unavailable");
    }
    return scene;
}

AudioEngine *resolveAudioEngineArg(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isAudioEngine = false;
    if (!readBoolProperty(ctx, value, ATLAS_AUDIO_ENGINE_PROP, isAudioEngine) ||
        !isAudioEngine) {
        JS_ThrowTypeError(ctx, "Expected Finewave audio engine handle");
        return nullptr;
    }

    Window *window = getWindow(host);
    if (window == nullptr || window->audioEngine == nullptr) {
        JS_ThrowReferenceError(ctx, "Finewave audio engine is unavailable");
        return nullptr;
    }
    return window->audioEngine.get();
}

ScriptAudioDataState *resolveAudioData(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t audioDataId = 0;
    if (!readIntProperty(ctx, value, ATLAS_AUDIO_DATA_ID_PROP, audioDataId)) {
        JS_ThrowTypeError(ctx, "Expected Finewave audio data handle");
        return nullptr;
    }

    auto *state =
        findAudioDataState(host, static_cast<std::uint64_t>(audioDataId));
    if (state == nullptr || !state->data) {
        JS_ThrowReferenceError(ctx, "Unknown Finewave audio data id");
        return nullptr;
    }
    return state;
}

ScriptAudioSourceState *resolveAudioSource(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t audioSourceId = 0;
    if (!readIntProperty(ctx, value, ATLAS_AUDIO_SOURCE_ID_PROP,
                         audioSourceId)) {
        JS_ThrowTypeError(ctx, "Expected Finewave audio source handle");
        return nullptr;
    }

    auto *state =
        findAudioSourceState(host, static_cast<std::uint64_t>(audioSourceId));
    if (state == nullptr || state->source == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Finewave audio source id");
        return nullptr;
    }
    return state;
}

Reverb *resolveReverb(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t reverbId = 0;
    if (!readIntProperty(ctx, value, ATLAS_REVERB_ID_PROP, reverbId)) {
        JS_ThrowTypeError(ctx, "Expected Finewave reverb handle");
        return nullptr;
    }

    auto *state = findReverbState(host, static_cast<std::uint64_t>(reverbId));
    if (state == nullptr || !state->effect) {
        JS_ThrowReferenceError(ctx, "Unknown Finewave reverb id");
        return nullptr;
    }
    return state->effect.get();
}

Echo *resolveEcho(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t echoId = 0;
    if (!readIntProperty(ctx, value, ATLAS_ECHO_ID_PROP, echoId)) {
        JS_ThrowTypeError(ctx, "Expected Finewave echo handle");
        return nullptr;
    }

    auto *state = findEchoState(host, static_cast<std::uint64_t>(echoId));
    if (state == nullptr || !state->effect) {
        JS_ThrowReferenceError(ctx, "Unknown Finewave echo id");
        return nullptr;
    }
    return state->effect.get();
}

Distortion *resolveDistortion(JSContext *ctx, ScriptHost &host,
                              JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t distortionId = 0;
    if (!readIntProperty(ctx, value, ATLAS_DISTORTION_ID_PROP, distortionId)) {
        JS_ThrowTypeError(ctx, "Expected Finewave distortion handle");
        return nullptr;
    }

    auto *state =
        findDistortionState(host, static_cast<std::uint64_t>(distortionId));
    if (state == nullptr || !state->effect) {
        JS_ThrowReferenceError(ctx, "Unknown Finewave distortion id");
        return nullptr;
    }
    return state->effect.get();
}

ScriptTextureState *resolveTexture(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t textureId = 0;
    if (!readIntProperty(ctx, value, ATLAS_TEXTURE_ID_PROP, textureId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas texture handle");
        return nullptr;
    }

    auto *state = findTextureState(host, static_cast<std::uint64_t>(textureId));
    if (state == nullptr || !state->texture) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas texture id");
        return nullptr;
    }
    return state;
}

bool parseWindowConfiguration(JSContext *ctx, JSValueConst value,
                              const Window *window, WindowConfiguration &out) {
    out = WindowConfiguration{};
    if (window != nullptr) {
        out.title = window->title;
        out.width = window->width;
        out.height = window->height;
        out.renderScale = window->getRenderScale();
        out.mouseCaptured = false;
        out.multisampling = window->useMultisampling;
        out.ssaoScale = window->getSSAORenderScale();
        out.editorControls = window->areEditorControlsEnabled();
    }
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    readStringProperty(ctx, value, "title", out.title);

    std::int64_t intValue = out.width;
    if (readIntProperty(ctx, value, "width", intValue)) {
        out.width = static_cast<int>(intValue);
    }
    intValue = out.height;
    if (readIntProperty(ctx, value, "height", intValue)) {
        out.height = static_cast<int>(intValue);
    }
    double numberValue = out.renderScale;
    if (readNumberProperty(ctx, value, "renderScale", numberValue)) {
        out.renderScale = static_cast<float>(numberValue);
    }
    readBoolProperty(ctx, value, "mouseCaptured", out.mouseCaptured);
    intValue = out.posX;
    if (readIntProperty(ctx, value, "posX", intValue)) {
        out.posX = static_cast<int>(intValue);
    }
    intValue = out.posY;
    if (readIntProperty(ctx, value, "posY", intValue)) {
        out.posY = static_cast<int>(intValue);
    }
    readBoolProperty(ctx, value, "multisampling", out.multisampling);
    readBoolProperty(ctx, value, "editorControls", out.editorControls);
    readBoolProperty(ctx, value, "decorations", out.decorations);
    readBoolProperty(ctx, value, "resizable", out.resizable);
    readBoolProperty(ctx, value, "transparent", out.transparent);
    readBoolProperty(ctx, value, "alwaysOnTop", out.alwaysOnTop);
    numberValue = out.opacity;
    if (readNumberProperty(ctx, value, "opacity", numberValue)) {
        out.opacity = static_cast<float>(numberValue);
    }
    intValue = out.aspectRatioX;
    if (readIntProperty(ctx, value, "aspectRatioX", intValue)) {
        out.aspectRatioX = static_cast<int>(intValue);
    }
    intValue = out.aspectRatioY;
    if (readIntProperty(ctx, value, "aspectRatioY", intValue)) {
        out.aspectRatioY = static_cast<int>(intValue);
    }
    numberValue = out.ssaoScale;
    if (readNumberProperty(ctx, value, "ssaoScale", numberValue)) {
        out.ssaoScale = static_cast<float>(numberValue);
    }
    return true;
}

bool parseControllerIdValue(JSContext *ctx, JSValueConst value,
                            ControllerID &out) {
    if (!JS_IsObject(value) || JS_IsNull(value)) {
        return false;
    }

    std::int64_t id = out.id;
    if (!readIntProperty(ctx, value, "id", id)) {
        return false;
    }
    out.id = static_cast<int>(id);
    readStringProperty(ctx, value, "name", out.name);
    readBoolProperty(ctx, value, "isJoystick", out.isJoystick);
    return true;
}

bool findMonitorById(int monitorId, Monitor &out) {
    const auto monitors = Window::enumerateMonitors();
    for (const auto &monitor : monitors) {
        if (monitor.monitorID == monitorId) {
            out = monitor;
            return true;
        }
    }
    return false;
}

ScriptCubemapState *resolveCubemap(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t cubemapId = 0;
    if (!readIntProperty(ctx, value, ATLAS_CUBEMAP_ID_PROP, cubemapId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas cubemap handle");
        return nullptr;
    }

    auto *state = findCubemapState(host, static_cast<std::uint64_t>(cubemapId));
    if (state == nullptr || !state->cubemap) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas cubemap id");
        return nullptr;
    }
    return state;
}

ScriptSkyboxState *resolveSkybox(JSContext *ctx, ScriptHost &host,
                                 JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t skyboxId = 0;
    if (!readIntProperty(ctx, value, ATLAS_SKYBOX_ID_PROP, skyboxId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas skybox handle");
        return nullptr;
    }

    auto *state = findSkyboxState(host, static_cast<std::uint64_t>(skyboxId));
    if (state == nullptr || !state->skybox) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas skybox id");
        return nullptr;
    }
    return state;
}

ScriptRenderTargetState *resolveRenderTarget(JSContext *ctx, ScriptHost &host,
                                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t renderTargetId = 0;
    if (!readIntProperty(ctx, value, ATLAS_RENDER_TARGET_ID_PROP,
                         renderTargetId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas render target handle");
        return nullptr;
    }

    auto *state =
        findRenderTargetState(host, static_cast<std::uint64_t>(renderTargetId));
    if (state == nullptr || !state->renderTarget) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas render target id");
        return nullptr;
    }
    return state;
}

ScriptPointLightState *resolvePointLight(JSContext *ctx, ScriptHost &host,
                                         JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_POINT_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas point light handle");
        return nullptr;
    }

    auto *state =
        findPointLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas point light id");
        return nullptr;
    }
    return state;
}

ScriptDirectionalLightState *
resolveDirectionalLight(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_DIRECTIONAL_LIGHT_ID_PROP,
                         lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas directional light handle");
        return nullptr;
    }

    auto *state =
        findDirectionalLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas directional light id");
        return nullptr;
    }
    return state;
}

ScriptSpotLightState *resolveSpotLight(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_SPOT_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas spot light handle");
        return nullptr;
    }

    auto *state = findSpotLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas spot light id");
        return nullptr;
    }
    return state;
}

ScriptAreaLightState *resolveAreaLight(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_AREA_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas area light handle");
        return nullptr;
    }

    auto *state = findAreaLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas area light id");
        return nullptr;
    }
    return state;
}

ScriptRigidbodyState *resolveRigidbody(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t rigidbodyId = 0;
    if (!readIntProperty(ctx, value, ATLAS_RIGIDBODY_ID_PROP, rigidbodyId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas rigidbody handle");
        return nullptr;
    }

    auto *state =
        findRigidbodyState(host, static_cast<std::uint64_t>(rigidbodyId));
    if (state == nullptr || state->component == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas rigidbody id");
        return nullptr;
    }
    return state;
}

ScriptVehicleState *resolveVehicle(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t vehicleId = 0;
    if (!readIntProperty(ctx, value, ATLAS_VEHICLE_ID_PROP, vehicleId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas vehicle handle");
        return nullptr;
    }

    auto *state = findVehicleState(host, static_cast<std::uint64_t>(vehicleId));
    if (state == nullptr || state->component == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas vehicle id");
        return nullptr;
    }
    return state;
}

ScriptFixedJointState *resolveFixedJoint(JSContext *ctx, ScriptHost &host,
                                         JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t jointId = 0;
    if (!readIntProperty(ctx, value, ATLAS_FIXED_JOINT_ID_PROP, jointId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas fixed joint handle");
        return nullptr;
    }

    auto *state =
        findFixedJointState(host, static_cast<std::uint64_t>(jointId));
    if (state == nullptr || state->component == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas fixed joint id");
        return nullptr;
    }
    return state;
}

ScriptHingeJointState *resolveHingeJoint(JSContext *ctx, ScriptHost &host,
                                         JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t jointId = 0;
    if (!readIntProperty(ctx, value, ATLAS_HINGE_JOINT_ID_PROP, jointId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas hinge joint handle");
        return nullptr;
    }

    auto *state =
        findHingeJointState(host, static_cast<std::uint64_t>(jointId));
    if (state == nullptr || state->component == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas hinge joint id");
        return nullptr;
    }
    return state;
}

ScriptSpringJointState *resolveSpringJoint(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t jointId = 0;
    if (!readIntProperty(ctx, value, ATLAS_SPRING_JOINT_ID_PROP, jointId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas spring joint handle");
        return nullptr;
    }

    auto *state =
        findSpringJointState(host, static_cast<std::uint64_t>(jointId));
    if (state == nullptr || state->component == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas spring joint id");
        return nullptr;
    }
    return state;
}

JSValue jsGetObjectById(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_NULL;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object == nullptr) {
        return JS_NULL;
    }
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetCamera(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Camera *camera = getSceneCamera(*host);
    if (camera == nullptr) {
        return JS_NULL;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsGetWindow(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Window *window = getWindow(*host);
    if (window == nullptr) {
        return JS_NULL;
    }

    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetClearColor(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and color");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    Color color = Color::black();
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected color");
    }
    window->setClearColor(color);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowClose(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    window->close();
    return JS_UNDEFINED;
}

JSValue jsWindowSetFullscreen(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and enabled");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    const bool enabled = JS_ToBool(ctx, argv[1]) == 1;
    window->setFullscreen(enabled);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetFullscreenMonitor(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and monitor");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[1])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[1], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    window->setFullscreen(monitor);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetWindowed(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and configuration");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    WindowConfiguration config;
    if (!parseWindowConfiguration(ctx, argv[1], window, config)) {
        return JS_ThrowTypeError(ctx, "Expected WindowConfiguration");
    }
    window->setWindowed(config);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowEnumerateMonitors(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    (void)window;
    const auto monitors = Window::enumerateMonitors();
    JSValue result = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < monitors.size(); ++i) {
        JS_SetPropertyUint32(ctx, result, i,
                             makeMonitorValue(ctx, *host, monitors[i]));
    }
    return result;
}

JSValue jsMonitorQueryVideoModes(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    const auto modes = monitor.queryVideoModes();
    JSValue result = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < modes.size(); ++i) {
        JS_SetPropertyUint32(ctx, result, i, makeVideoModeValue(ctx, modes[i]));
    }
    return result;
}

JSValue jsMonitorGetCurrentVideoMode(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    return makeVideoModeValue(ctx, monitor.getCurrentVideoMode());
}

JSValue jsMonitorGetPhysicalSize(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    const auto [width, height] = monitor.getPhysicalSize();
    return makeSize2d(ctx, *host,
                      {static_cast<float>(width), static_cast<float>(height)});
}

JSValue jsMonitorGetPosition(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    const auto [x, y] = monitor.getPosition();
    return makePosition2d(ctx, *host,
                          {static_cast<float>(x), static_cast<float>(y)});
}

JSValue jsMonitorGetContentScale(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    const auto scales = monitor.getContentScale();
    return JS_NewFloat64(ctx, std::get<0>(scales));
}

JSValue jsMonitorGetName(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t monitorId = 0;
    if (!readIntProperty(ctx, argv[0], "monitorId", monitorId)) {
        return JS_ThrowTypeError(ctx, "Expected monitor");
    }
    Monitor monitor(0, 0, false);
    if (!findMonitorById(static_cast<int>(monitorId), monitor)) {
        return JS_ThrowReferenceError(ctx, "Unknown monitor");
    }
    return JS_NewString(ctx, monitor.getName().c_str());
}

JSValue jsWindowGetControllers(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    const auto controllers = window->getControllers();
    JSValue result = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < controllers.size(); ++i) {
        JS_SetPropertyUint32(ctx, result, i,
                             makeControllerIdValue(ctx, controllers[i]));
    }
    return result;
}

JSValue jsWindowGetController(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and controller id");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    ControllerID id{};
    if (!parseControllerIdValue(ctx, argv[1], id) || id.isJoystick) {
        return JS_NULL;
    }
    return makeGamepadValue(ctx, *host, window->getController(id));
}

JSValue jsWindowGetJoystick(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and controller id");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    ControllerID id{};
    if (!parseControllerIdValue(ctx, argv[1], id) || !id.isJoystick) {
        return JS_NULL;
    }
    return makeJoystickValue(ctx, *host, window->getJoystick(id));
}

JSValue jsWindowInstantiate(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and object");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    GameObject *object = resolveObjectArg(ctx, *host, argv[1]);
    if (window == nullptr || object == nullptr) {
        return JS_EXCEPTION;
    }
    (void)window;
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsWindowDestroy(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and object");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    GameObject *object = resolveObjectArg(ctx, *host, argv[1]);
    if (window == nullptr || object == nullptr) {
        return JS_EXCEPTION;
    }
    window->removeObject(object);
    auto stateIt = host->objectStates.find(object->getId());
    if (stateIt != host->objectStates.end()) {
        stateIt->second.attachedToWindow = false;
    }
    return JS_UNDEFINED;
}

JSValue jsWindowAddUIObject(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and object");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    GameObject *object = resolveObjectArg(ctx, *host, argv[1]);
    if (window == nullptr || object == nullptr) {
        return JS_EXCEPTION;
    }
    if (dynamic_cast<UIObject *>(object) == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected UIObject");
    }
    object->initialize();
    window->addUIObject(object);
    trackObjectState(*host, *object, true);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsWindowSetCamera(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and camera");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    Camera *camera = resolveCameraArg(ctx, *host, argv[1]);
    if (window == nullptr || camera == nullptr) {
        return JS_EXCEPTION;
    }
    window->setCamera(camera);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetScene(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and scene");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    Scene *scene = resolveSceneArg(ctx, *host, argv[1]);
    if (window == nullptr || scene == nullptr) {
        return JS_EXCEPTION;
    }
    window->setScene(scene);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowGetTime(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getTime());
}

JSValue jsWindowAddRenderTarget(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and render target");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    auto *state = resolveRenderTarget(ctx, *host, argv[1]);
    if (window == nullptr || state == nullptr || !state->renderTarget) {
        return JS_EXCEPTION;
    }
    window->addRenderTarget(state->renderTarget.get());
    state->attached = true;
    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[1], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsWindowGetSize(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return makeSize2d(ctx, *host, window->getSize());
}

JSValue jsWindowGetDeltaTime(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getDeltaTime());
}

JSValue jsWindowGetFramesPerSecond(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getFramesPerSecond());
}

JSValue jsWindowActivateDebug(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    window->activateDebug();
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowDeactivateDebug(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    window->deactivateDebug();
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetGravity(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and gravity");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    double gravity = window->gravity;
    if (!getDouble(ctx, argv[1], gravity)) {
        return JS_ThrowTypeError(ctx, "Expected gravity");
    }
    window->gravity = static_cast<float>(gravity);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowUseTracer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and enabled");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    window->useTracer(JS_ToBool(ctx, argv[1]) == 1);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowSetLogOutput(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 4) {
        return JS_ThrowTypeError(ctx, "Expected window and three booleans");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    window->setLogOutput(JS_ToBool(ctx, argv[1]) == 1,
                         JS_ToBool(ctx, argv[2]) == 1,
                         JS_ToBool(ctx, argv[3]) == 1);
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowGetRenderScale(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getRenderScale());
}

JSValue jsWindowUseMetalUpscaling(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and ratio");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    double ratio = window->getMetalUpscalingRatio();
    if (!getDouble(ctx, argv[1], ratio)) {
        return JS_ThrowTypeError(ctx, "Expected ratio");
    }
    window->useMetalUpscaling(static_cast<float>(ratio));
    return syncWindowWrapper(ctx, *host, *window);
}

JSValue jsWindowIsMetalUpscalingEnabled(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, window->isMetalUpscalingEnabled());
}

JSValue jsWindowGetMetalUpscalingRatio(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getMetalUpscalingRatio());
}

JSValue jsWindowGetSSAORenderScale(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected window");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, window->getSSAORenderScale());
}

JSValue jsWindowGetInputAction(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected window and action name");
    }
    Window *window = resolveWindowArg(ctx, *host, argv[0]);
    if (window == nullptr) {
        return JS_EXCEPTION;
    }
    const char *name = JS_ToCString(ctx, argv[1]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    auto action = window->getInputAction(name);
    JS_FreeCString(ctx, name);
    if (action == nullptr) {
        return JS_NULL;
    }
    return makeInputActionValue(ctx, *host, *action);
}

JSValue jsGamepadRumble(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected gamepad, strength, and duration");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t controllerId = CONTROLLER_UNDEFINED;
    if (!readIntProperty(ctx, argv[0], "controllerId", controllerId)) {
        return JS_ThrowTypeError(ctx, "Expected gamepad");
    }
    double strength = 0.0;
    double duration = 0.0;
    if (!getDouble(ctx, argv[1], strength) ||
        !getDouble(ctx, argv[2], duration)) {
        return JS_ThrowTypeError(ctx, "Expected strength and duration");
    }
    Window *window = getWindow(*host);
    if (window == nullptr) {
        return JS_NULL;
    }
    ControllerID id{static_cast<int>(controllerId), "", false};
    window->getController(id).rumble(static_cast<float>(strength),
                                     static_cast<float>(duration));
    return JS_UNDEFINED;
}

JSValue jsJoystickGetAxisCount(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected joystick");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t joystickId = 0;
    if (!readIntProperty(ctx, argv[0], "joystickId", joystickId)) {
        return JS_ThrowTypeError(ctx, "Expected joystick");
    }
    Window *window = getWindow(*host);
    if (window == nullptr) {
        return JS_NewInt32(ctx, 0);
    }
    ControllerID id{static_cast<int>(joystickId), "", true};
    return JS_NewInt32(ctx, window->getJoystick(id).getAxisCount());
}

JSValue jsJoystickGetButtonCount(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected joystick");
    }
    if (!ensureCurrentGeneration(ctx, *host, argv[0])) {
        return JS_EXCEPTION;
    }
    std::int64_t joystickId = 0;
    if (!readIntProperty(ctx, argv[0], "joystickId", joystickId)) {
        return JS_ThrowTypeError(ctx, "Expected joystick");
    }
    Window *window = getWindow(*host);
    if (window == nullptr) {
        return JS_NewInt32(ctx, 0);
    }
    ControllerID id{static_cast<int>(joystickId), "", true};
    return JS_NewInt32(ctx, window->getJoystick(id).getButtonCount());
}

JSValue jsUpdateCamera(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected camera");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    if (!applyCamera(ctx, argv[0], *camera)) {
        return JS_EXCEPTION;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsSetPositionKeepingOrientation(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected camera and position");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    camera->setPositionKeepingOrientation(position);
    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsLookAtCamera(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected camera and target");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Point3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    if (target.x != camera->position.x || target.y != camera->position.y ||
        target.z != camera->position.z) {
        camera->lookAt(target);
    } else {
        camera->target = target;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsMoveCameraTo(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected camera, position, and speed");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d targetPosition;
    if (!parsePosition3d(ctx, argv[1], targetPosition)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    double speed = 0.0;
    if (!getDouble(ctx, argv[2], speed)) {
        return JS_ThrowTypeError(ctx, "Expected speed");
    }

    const double dx = targetPosition.x - camera->position.x;
    const double dy = targetPosition.y - camera->position.y;
    const double dz = targetPosition.z - camera->position.z;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance == 0.0) {
        return syncCameraWrapper(ctx, *host, *camera);
    }

    if (speed <= 0.0 || distance <= speed) {
        camera->setPositionKeepingOrientation(targetPosition);
        return syncCameraWrapper(ctx, *host, *camera);
    }

    const double factor = speed / distance;
    camera->setPositionKeepingOrientation(Position3d(
        camera->position.x + dx * factor, camera->position.y + dy * factor,
        camera->position.z + dz * factor));
    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsGetCameraDirection(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Camera *camera = nullptr;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        camera = resolveCameraArg(ctx, *host, argv[0]);
        if (camera == nullptr) {
            return JS_EXCEPTION;
        }
    } else {
        camera = getSceneCamera(*host);
    }

    if (camera == nullptr) {
        return JS_NULL;
    }

    return makePosition3d(ctx, *host, camera->getFrontVector());
}

JSValue jsGetScene(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Scene *scene = getScene(*host);
    if (scene == nullptr) {
        return JS_NULL;
    }

    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneAmbientIntensity(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and intensity");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    double intensity = 0.0;
    if (!getDouble(ctx, argv[1], intensity)) {
        return JS_ThrowTypeError(ctx, "Expected intensity");
    }

    scene->setAmbientIntensity(static_cast<float>(intensity));
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneAutomaticAmbient(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and enabled flag");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setAutomaticAmbient(JS_ToBool(ctx, argv[1]) == 1);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneEnvironment(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and environment");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    Environment environment;
    if (!parseEnvironmentValue(ctx, *host, argv[1], environment)) {
        return JS_ThrowTypeError(ctx, "Expected environment");
    }

    scene->setEnvironment(environment);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneAmbientColor(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and color");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    Color color;
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected color");
    }

    scene->setAmbientColor(color);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneSkybox(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and skybox");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *skyboxState = resolveSkybox(ctx, *host, argv[1]);
    if (skyboxState == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setSkybox(skyboxState->skybox);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsUseAtmosphereSkybox(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and enabled flag");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setUseAtmosphereSkybox(JS_ToBool(ctx, argv[1]) == 1);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSceneAddDirectionalLight(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and directional light");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *state = resolveDirectionalLight(ctx, *host, argv[1]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    scene->addDirectionalLight(state->light);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSceneAddLight(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and light");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *state = resolvePointLight(ctx, *host, argv[1]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    const auto &lights = scene->getPointLights();
    if (std::find(lights.begin(), lights.end(), state->light) == lights.end()) {
        scene->addLight(state->light);
    }
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSceneAddSpotLight(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and spot light");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *state = resolveSpotLight(ctx, *host, argv[1]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    const auto &lights = scene->getSpotlights();
    if (std::find(lights.begin(), lights.end(), state->light) == lights.end()) {
        scene->addSpotlight(state->light);
    }
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSceneAddAreaLight(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and area light");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *state = resolveAreaLight(ctx, *host, argv[1]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    const auto &lights = scene->getAreaLights();
    if (std::find(lights.begin(), lights.end(), state->light) == lights.end()) {
        scene->addAreaLight(state->light);
    }
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsCreateTextureFromResource(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected resource and texture type");
    }

    Resource resource;
    if (JS_IsString(argv[0])) {
        const char *name = JS_ToCString(ctx, argv[0]);
        if (name == nullptr) {
            return JS_ThrowTypeError(ctx, "Expected resource");
        }
        resource = Workspace::get().getResource(name);
        JS_FreeCString(ctx, name);
    } else if (!parseResource(ctx, argv[0], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource or resource name");
    }

    if (resource.name.empty() && resource.path.empty()) {
        return JS_ThrowReferenceError(ctx, "Unknown resource");
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        return JS_ThrowTypeError(ctx, "Expected texture type");
    }

    const std::uint64_t textureId = registerTextureState(
        *host, Texture::fromResource(
                   resource, toNativeTextureType(static_cast<int>(type))));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateEmptyTexture(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected width, height, and texture type");
    }

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t type = 0;
    if (!getInt64(ctx, argv[0], width) || !getInt64(ctx, argv[1], height) ||
        !getInt64(ctx, argv[2], type)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected width, height, and texture type");
    }

    Color borderColor(0.0f, 0.0f, 0.0f, 0.0f);
    if (argc > 3 && !JS_IsUndefined(argv[3])) {
        if (!parseColor(ctx, argv[3], borderColor)) {
            return JS_ThrowTypeError(ctx, "Expected border color");
        }
    }

    const std::uint64_t textureId = registerTextureState(
        *host, createEmptyTexture(
                   static_cast<int>(width), static_cast<int>(height),
                   toNativeTextureType(static_cast<int>(type)), borderColor));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateColorTexture(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected color, texture type, width, and height");
    }

    Color color;
    std::int64_t type = 0;
    std::int64_t width = 0;
    std::int64_t height = 0;
    if (!parseColor(ctx, argv[0], color) || !getInt64(ctx, argv[1], type) ||
        !getInt64(ctx, argv[2], width) || !getInt64(ctx, argv[3], height)) {
        return JS_ThrowTypeError(
            ctx, "Expected color, texture type, width, and height");
    }

    const std::uint64_t textureId = registerTextureState(
        *host,
        createSolidTexture(color, toNativeTextureType(static_cast<int>(type)),
                           static_cast<int>(width), static_cast<int>(height)));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateCheckerboardTexture(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 6) {
        return JS_ThrowTypeError(
            ctx, "Expected texture, width, height, check size, and two colors");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t checkSize = 0;
    Color color1;
    Color color2;
    if (!getInt64(ctx, argv[1], width) || !getInt64(ctx, argv[2], height) ||
        !getInt64(ctx, argv[3], checkSize) ||
        !parseColor(ctx, argv[4], color1) ||
        !parseColor(ctx, argv[5], color2)) {
        return JS_ThrowTypeError(
            ctx, "Expected texture, width, height, check size, and two colors");
    }

    *state->texture = Texture::createCheckerboard(
        static_cast<int>(width), static_cast<int>(height),
        static_cast<int>(checkSize), color1, color2);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsCreateDoubleCheckerboardTexture(JSContext *ctx, JSValueConst,
                                          int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 8) {
        return JS_ThrowTypeError(ctx, "Expected texture, width, height, two "
                                      "check sizes, and three colors");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t checkSizeBig = 0;
    std::int64_t checkSizeSmall = 0;
    Color color1;
    Color color2;
    Color color3;
    if (!getInt64(ctx, argv[1], width) || !getInt64(ctx, argv[2], height) ||
        !getInt64(ctx, argv[3], checkSizeBig) ||
        !getInt64(ctx, argv[4], checkSizeSmall) ||
        !parseColor(ctx, argv[5], color1) ||
        !parseColor(ctx, argv[6], color2) ||
        !parseColor(ctx, argv[7], color3)) {
        return JS_ThrowTypeError(ctx, "Expected texture, width, height, two "
                                      "check sizes, and three colors");
    }

    *state->texture = Texture::createDoubleCheckerboard(
        static_cast<int>(width), static_cast<int>(height),
        static_cast<int>(checkSizeBig), static_cast<int>(checkSizeSmall),
        color1, color2, color3);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsDisplayTexture(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected texture");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->texture->display(*host->context->window);
    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsCreateCubemap(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource array");
    }

    std::vector<Resource> resources;
    if (!parseResourceVector(ctx, argv[0], resources) ||
        resources.size() != 6) {
        return JS_ThrowTypeError(ctx, "Expected six cubemap resources");
    }

    ResourceGroup group;
    group.groupName = "ScriptCubemap";
    group.resources = resources;
    const std::uint64_t cubemapId =
        registerCubemapState(*host, Cubemap::fromResourceGroup(group));
    return syncCubemapWrapper(ctx, *host, cubemapId);
}

JSValue jsCreateCubemapFromGroup(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    return jsCreateCubemap(ctx, JS_UNDEFINED, argc, argv);
}

JSValue jsGetCubemapAverageColor(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected cubemap");
    }

    auto *state = resolveCubemap(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    return makeColor(ctx, *host, state->cubemap->averageColor);
}

JSValue jsUpdateCubemapWithColors(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected cubemap and six colors");
    }

    auto *state = resolveCubemap(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::array<Color, 6> colors;
    if (!parseColorArray(ctx, argv[1], colors)) {
        return JS_ThrowTypeError(ctx, "Expected an array of six colors");
    }

    state->cubemap->updateWithColors(colors);
    std::int64_t cubemapId = 0;
    readIntProperty(ctx, argv[0], ATLAS_CUBEMAP_ID_PROP, cubemapId);
    return syncCubemapWrapper(ctx, *host,
                              static_cast<std::uint64_t>(cubemapId));
}

JSValue jsCreateSkybox(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected cubemap");
    }

    auto *cubemapState = resolveCubemap(ctx, *host, argv[0]);
    if (cubemapState == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t cubemapId = 0;
    readIntProperty(ctx, argv[0], ATLAS_CUBEMAP_ID_PROP, cubemapId);

    const std::uint64_t skyboxId = host->nextSkyboxId++;
    host->skyboxes[skyboxId] = {
        .skybox =
            Skybox::create(*cubemapState->cubemap, *host->context->window),
        .value = JS_UNDEFINED,
        .cubemapId = static_cast<std::uint64_t>(cubemapId)};
    return syncSkyboxWrapper(ctx, *host, skyboxId);
}

JSValue jsCreateRenderTarget(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    std::int64_t type = 0;
    std::int64_t resolution = 1024;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !getInt64(ctx, argv[0], type)) {
        return JS_ThrowTypeError(ctx, "Expected render target type");
    }
    if (argc > 1 && !JS_IsUndefined(argv[1]) &&
        !getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    const std::uint64_t renderTargetId = host->nextRenderTargetId++;
    host->renderTargets[renderTargetId] = {
        .renderTarget = std::make_shared<RenderTarget>(
            *host->context->window,
            toNativeRenderTargetType(static_cast<int>(type)),
            static_cast<int>(resolution)),
        .value = JS_UNDEFINED,
        .attached = false,
        .outTextureIds = {},
        .depthTextureId = 0};
    return syncRenderTargetWrapper(ctx, *host, renderTargetId);
}

JSValue jsAddRenderTargetToPassQueue(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected render target and pass type");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t passType = 0;
    if (!getInt64(ctx, argv[1], passType)) {
        return JS_ThrowTypeError(ctx, "Expected pass type");
    }

    switch (passType) {
    case 0:
        host->context->window->useDeferredRendering();
        break;
    case 1:
        host->context->window->usesDeferred = false;
        host->context->window->usePathTracing = false;
        break;
    case 2:
        host->context->window->enablePathTracing();
        break;
    default:
        return JS_ThrowRangeError(ctx, "Unknown render pass type");
    }

    if (!state->attached) {
        host->context->window->addRenderTarget(state->renderTarget.get());
        state->attached = true;
    }

    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsAddRenderTargetEffect(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected render target and effect");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    auto effect = parseEffectValue(ctx, argv[1]);
    if (effect == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected Atlas effect");
    }

    state->renderTarget->addEffect(effect);

    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsDisplayRenderTarget(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected render target");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->renderTarget->display(*host->context->window);
    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsCreatePointLight(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<Light>();
    applyPointLight(ctx, argv[0], *light);
    scene->addLight(light.get());

    const std::uint64_t lightId = host->nextPointLightId++;
    host->pointLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->pointLights.push_back(std::move(light));
    return syncPointLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdatePointLight(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected point light");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyPointLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCreatePointLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected point light");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyPointLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncPointLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCastPointLightShadows(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected point light and resolution");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyPointLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateDirectionalLight(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<DirectionalLight>();
    applyDirectionalLight(ctx, argv[0], *light);
    scene->addDirectionalLight(light.get());

    const std::uint64_t lightId = host->nextDirectionalLightId++;
    host->directionalLights[lightId] = {.light = light.get(),
                                        .value = JS_UNDEFINED};
    host->context->directionalLights.push_back(std::move(light));
    return syncDirectionalLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateDirectionalLight(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected directional light");
    }

    auto *state = resolveDirectionalLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyDirectionalLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_DIRECTIONAL_LIGHT_ID_PROP, lightId);
    return syncDirectionalLightWrapper(ctx, *host,
                                       static_cast<std::uint64_t>(lightId));
}

JSValue jsCastDirectionalLightShadows(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected directional light and resolution");
    }

    auto *state = resolveDirectionalLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyDirectionalLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_DIRECTIONAL_LIGHT_ID_PROP, lightId);
    return syncDirectionalLightWrapper(ctx, *host,
                                       static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<Spotlight>();
    applySpotLight(ctx, argv[0], *light);
    scene->addSpotlight(light.get());

    const std::uint64_t lightId = host->nextSpotLightId++;
    host->spotLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->spotlights.push_back(std::move(light));
    return syncSpotLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spot light");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateSpotLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spot light");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncSpotLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsLookAtSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected spot light and target");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    Position3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    state->light->lookAt(target);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCastSpotLightShadows(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected spot light and resolution");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applySpotLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<AreaLight>();
    applyAreaLight(ctx, argv[0], *light);
    scene->addAreaLight(light.get());

    const std::uint64_t lightId = host->nextAreaLightId++;
    host->areaLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->areaLights.push_back(std::move(light));
    return syncAreaLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsGetAreaLightNormal(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    return makePosition3d(ctx, *host, state->light->getNormal());
}

JSValue jsSetAreaLightRotation(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and rotation");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    state->light->setRotation(rotation);
    syncAreaLightDebugObject(*state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsRotateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and rotation");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    state->light->rotate(rotation);
    syncAreaLightDebugObject(*state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateAreaLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncAreaLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCastAreaLightShadows(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and resolution");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyAreaLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsGetObjectByName(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_NULL;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected object name");
    }

    GameObject *object = findObjectByName(*host, name);
    JS_FreeCString(ctx, name);
    if (object == nullptr) {
        return JS_NULL;
    }
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetComponentByName(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_NULL;
    }

    std::int64_t ownerId = 0;
    if (!getInt64(ctx, argv[0], ownerId)) {
        return JS_ThrowTypeError(ctx, "Expected owner id");
    }

    const char *name = JS_ToCString(ctx, argv[1]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected component name");
    }

    auto lookupIt = host->componentLookup.find(
        makeComponentLookupKey(static_cast<int>(ownerId), name));
    JS_FreeCString(ctx, name);
    if (lookupIt == host->componentLookup.end()) {
        return JS_NULL;
    }

    auto componentIt = host->componentStates.find(lookupIt->second);
    if (componentIt == host->componentStates.end()) {
        return JS_NULL;
    }

    return JS_DupValue(ctx, componentIt->second.value);
}

JSValue jsLoadResource(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected path and resource type");
    }

    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    const char *path = JS_ToCString(ctx, argv[0]);
    if (path == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected resource path");
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        JS_FreeCString(ctx, path);
        return JS_ThrowTypeError(ctx, "Expected resource type");
    }

    std::string name;
    if (argc > 2 && !JS_IsUndefined(argv[2])) {
        const char *providedName = JS_ToCString(ctx, argv[2]);
        if (providedName != nullptr) {
            name = providedName;
            JS_FreeCString(ctx, providedName);
        }
    }

    std::filesystem::path resourcePath(path);
    if (name.empty()) {
        name = resourcePath.stem().string();
        if (name.empty()) {
            name = resourcePath.filename().string();
        }
    }

    Resource resource = Workspace::get().createResource(
        resourcePath, name, toNativeResourceType(static_cast<int>(type)));
    JS_FreeCString(ctx, path);
    return makeResource(ctx, *host, resource);
}

JSValue jsGetResourceByName(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource name");
    }

    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected resource name");
    }

    Resource resource = Workspace::get().getResource(name);
    JS_FreeCString(ctx, name);

    if (resource.name.empty()) {
        return JS_NULL;
    }

    if (argc > 1 && !JS_IsUndefined(argv[1])) {
        std::int64_t expectedType = 0;
        if (getInt64(ctx, argv[1], expectedType) &&
            resource.type !=
                toNativeResourceType(static_cast<int>(expectedType))) {
            return JS_NULL;
        }
    }

    return makeResource(ctx, *host, resource);
}

JSValue jsGetAudioEngine(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Window *window = getWindow(*host);
    if (window == nullptr) {
        return JS_NULL;
    }
    if (window->audioEngine == nullptr) {
        window->audioEngine = std::make_shared<AudioEngine>();
        window->audioEngine->initialize();
    }
    if (window->audioEngine == nullptr) {
        return JS_NULL;
    }
    return syncAudioEngineWrapper(ctx, *host, *window->audioEngine);
}

JSValue jsAudioEngineSetListenerPosition(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio engine and position");
    }

    AudioEngine *engine = resolveAudioEngineArg(ctx, *host, argv[0]);
    if (engine == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    engine->setListenerPosition(position);
    return JS_UNDEFINED;
}

JSValue jsAudioEngineSetListenerOrientation(JSContext *ctx, JSValueConst,
                                            int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected audio engine, forward, and up");
    }

    AudioEngine *engine = resolveAudioEngineArg(ctx, *host, argv[0]);
    if (engine == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d forward;
    Position3d up;
    if (!parsePosition3d(ctx, argv[1], forward) ||
        !parsePosition3d(ctx, argv[2], up)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d values");
    }

    engine->setListenerOrientation(forward, up);
    return JS_UNDEFINED;
}

JSValue jsAudioEngineSetListenerVelocity(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio engine and velocity");
    }

    AudioEngine *engine = resolveAudioEngineArg(ctx, *host, argv[0]);
    if (engine == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    engine->setListenerVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsAudioEngineSetMasterVolume(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio engine and volume");
    }

    AudioEngine *engine = resolveAudioEngineArg(ctx, *host, argv[0]);
    if (engine == nullptr) {
        return JS_EXCEPTION;
    }

    double volume = 1.0;
    if (!getDouble(ctx, argv[1], volume)) {
        return JS_ThrowTypeError(ctx, "Expected volume");
    }

    engine->setMasterVolume(static_cast<float>(volume));
    return JS_UNDEFINED;
}

JSValue jsCreateAudioData(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource");
    }
    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    Resource resource;
    if (!parseResource(ctx, argv[0], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource");
    }

    auto data = AudioData::fromResource(resource);
    if (!data) {
        return JS_NULL;
    }

    const std::uint64_t audioDataId = registerAudioDataState(*host, data);
    return syncAudioDataWrapper(ctx, *host, audioDataId);
}

JSValue jsCreateAudioSource(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    (void)argc;
    (void)argv;
    if (host == nullptr) {
        return JS_ThrowInternalError(ctx, "Script host is unavailable");
    }
    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    auto source = std::make_shared<AudioSource>();
    const std::uint64_t audioSourceId = registerAudioSourceState(*host, source);
    return syncAudioSourceWrapper(ctx, *host, audioSourceId);
}

JSValue jsAudioSourceSetData(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and audio data");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    auto *dataState = resolveAudioData(ctx, *host, argv[1]);
    if (sourceState == nullptr || dataState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->setData(dataState->data);
    return JS_UNDEFINED;
}

JSValue jsAudioSourceFromFile(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and resource");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    Resource resource;
    if (!parseResource(ctx, argv[1], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource");
    }

    sourceState->source->fromFile(resource);
    return JS_UNDEFINED;
}

JSValue jsAudioSourcePlay(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->play();
    return JS_UNDEFINED;
}

JSValue jsAudioSourcePause(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->pause();
    return JS_UNDEFINED;
}

JSValue jsAudioSourceStop(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->stop();
    return JS_UNDEFINED;
}

JSValue jsAudioSourceSetLoop(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and loop flag");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->setLooping(JS_ToBool(ctx, argv[1]) == 1);
    return JS_UNDEFINED;
}

JSValue jsAudioSourceSetVolume(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and volume");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    double volume = 1.0;
    if (!getDouble(ctx, argv[1], volume)) {
        return JS_ThrowTypeError(ctx, "Expected volume");
    }

    sourceState->source->setVolume(static_cast<float>(volume));
    return JS_UNDEFINED;
}

JSValue jsAudioSourceSetPitch(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and pitch");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    double pitch = 1.0;
    if (!getDouble(ctx, argv[1], pitch)) {
        return JS_ThrowTypeError(ctx, "Expected pitch");
    }

    sourceState->source->setPitch(static_cast<float>(pitch));
    return JS_UNDEFINED;
}

JSValue jsAudioSourceSetPosition(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and position");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    sourceState->source->setPosition(position);
    return JS_UNDEFINED;
}

JSValue jsAudioSourceSetVelocity(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and velocity");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    sourceState->source->setVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsAudioSourceIsPlaying(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    return JS_NewBool(ctx, sourceState->source->isPlaying());
}

JSValue jsAudioSourcePlayFrom(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and position");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    double position = 0.0;
    if (!getDouble(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected position");
    }

    sourceState->source->playFrom(static_cast<float>(position));
    return JS_UNDEFINED;
}

JSValue jsAudioSourceDisableSpatialization(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->disableSpatialization();
    return JS_UNDEFINED;
}

JSValue jsAudioSourceApplyEffect(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio source and effect");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    if (!ensureCurrentGeneration(ctx, *host, argv[1])) {
        return JS_EXCEPTION;
    }

    std::int64_t effectId = 0;
    if (readIntProperty(ctx, argv[1], ATLAS_REVERB_ID_PROP, effectId)) {
        auto *state =
            findReverbState(*host, static_cast<std::uint64_t>(effectId));
        if (state == nullptr || !state->effect) {
            return JS_ThrowReferenceError(ctx, "Unknown Finewave reverb id");
        }
        sourceState->source->applyEffect(*state->effect);
        return JS_UNDEFINED;
    }
    if (readIntProperty(ctx, argv[1], ATLAS_ECHO_ID_PROP, effectId)) {
        auto *state =
            findEchoState(*host, static_cast<std::uint64_t>(effectId));
        if (state == nullptr || !state->effect) {
            return JS_ThrowReferenceError(ctx, "Unknown Finewave echo id");
        }
        sourceState->source->applyEffect(*state->effect);
        return JS_UNDEFINED;
    }
    if (readIntProperty(ctx, argv[1], ATLAS_DISTORTION_ID_PROP, effectId)) {
        auto *state =
            findDistortionState(*host, static_cast<std::uint64_t>(effectId));
        if (state == nullptr || !state->effect) {
            return JS_ThrowReferenceError(ctx,
                                          "Unknown Finewave distortion id");
        }
        sourceState->source->applyEffect(*state->effect);
        return JS_UNDEFINED;
    }

    return JS_ThrowTypeError(ctx, "Expected Finewave audio effect");
}

JSValue jsAudioSourceGetPosition(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    return makePosition3d(ctx, *host, sourceState->source->getPosition());
}

JSValue jsAudioSourceGetListenerPosition(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    return makePosition3d(ctx, *host,
                          sourceState->source->getListenerPosition());
}

JSValue jsAudioSourceUseSpatialization(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio source");
    }

    auto *sourceState = resolveAudioSource(ctx, *host, argv[0]);
    if (sourceState == nullptr) {
        return JS_EXCEPTION;
    }

    sourceState->source->useSpatialization();
    return JS_UNDEFINED;
}

JSValue jsCreateReverb(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || !ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    auto effect = std::make_shared<Reverb>();
    const std::uint64_t reverbId = registerReverbState(*host, effect);
    return syncReverbWrapper(ctx, *host, reverbId);
}

JSValue jsCreateEcho(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || !ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    auto effect = std::make_shared<Echo>();
    const std::uint64_t echoId = registerEchoState(*host, effect);
    return syncEchoWrapper(ctx, *host, echoId);
}

JSValue jsCreateDistortion(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || !ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    auto effect = std::make_shared<Distortion>();
    const std::uint64_t distortionId = registerDistortionState(*host, effect);
    return syncDistortionWrapper(ctx, *host, distortionId);
}

JSValue jsReverbSetRoomSize(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected reverb and size");
    }

    Reverb *effect = resolveReverb(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }

    effect->setRoomSize(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsReverbSetDamping(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected reverb and damping");
    }

    Reverb *effect = resolveReverb(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected damping");
    }

    effect->setDamping(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsReverbSetWetLevel(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected reverb and level");
    }

    Reverb *effect = resolveReverb(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected level");
    }

    effect->setWetLevel(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsReverbSetDryLevel(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected reverb and level");
    }

    Reverb *effect = resolveReverb(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected level");
    }

    effect->setDryLevel(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsReverbSetWidth(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected reverb and width");
    }

    Reverb *effect = resolveReverb(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected width");
    }

    effect->setWidth(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsEchoSetDelay(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected echo and delay");
    }

    Echo *effect = resolveEcho(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected delay");
    }

    effect->setDelay(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsEchoSetDecay(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected echo and decay");
    }

    Echo *effect = resolveEcho(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected decay");
    }

    effect->setDecay(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsEchoSetWetLevel(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected echo and level");
    }

    Echo *effect = resolveEcho(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected level");
    }

    effect->setWetLevel(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsEchoSetDryLevel(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected echo and level");
    }

    Echo *effect = resolveEcho(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected level");
    }

    effect->setDryLevel(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsDistortionSetEdge(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected distortion and edge");
    }

    Distortion *effect = resolveDistortion(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected edge");
    }

    effect->setEdge(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsDistortionSetGain(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected distortion and gain");
    }

    Distortion *effect = resolveDistortion(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected gain");
    }

    effect->setGain(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsDistortionSetLowpassCutoff(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected distortion and cutoff");
    }

    Distortion *effect = resolveDistortion(ctx, *host, argv[0]);
    if (effect == nullptr) {
        return JS_EXCEPTION;
    }

    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected cutoff");
    }

    effect->setLowpassCutoff(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsCreateAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player wrapper");
    }

    std::uint64_t audioPlayerId = host->nextAudioPlayerId++;
    auto component = std::make_shared<AudioPlayer>();
    component->init();

    host->audioPlayers[audioPlayerId] = {.component = component,
                                         .value = JS_DupValue(ctx, argv[0]),
                                         .attached = false};

    JSValue wrapper = JS_DupValue(ctx, argv[0]);
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(audioPlayerId)));
    setProperty(ctx, wrapper, ATLAS_AUDIO_PLAYER_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(audioPlayerId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host->generation)));
    setProperty(ctx, wrapper, ATLAS_NATIVE_COMPONENT_KIND_PROP,
                JS_NewString(ctx, "audio-player"));
    if (component->source != nullptr) {
        const std::uint64_t audioSourceId =
            registerAudioSourcePointer(*host, component->source.get());
        setProperty(ctx, wrapper, "source",
                    syncAudioSourceWrapper(ctx, *host, audioSourceId));
    } else {
        setProperty(ctx, wrapper, "source", JS_NULL);
    }
    JS_FreeValue(ctx, wrapper);

    return JS_UNDEFINED;
}

ScriptAudioPlayerState *resolveAudioPlayer(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    std::int64_t audioPlayerId = 0;
    if (!getInt64(ctx, value, audioPlayerId)) {
        JS_ThrowTypeError(ctx, "Expected audio player id");
        return nullptr;
    }

    ScriptAudioPlayerState *state =
        findAudioPlayerState(host, static_cast<std::uint64_t>(audioPlayerId));
    if (state == nullptr || !state->component) {
        JS_ThrowReferenceError(ctx, "Unknown audio player id");
        return nullptr;
    }

    return state;
}

void syncAudioPlayerSourceProperty(JSContext *ctx, ScriptHost &host,
                                   ScriptAudioPlayerState &state) {
    if (JS_IsUndefined(state.value)) {
        return;
    }

    JSValue wrapper = JS_DupValue(ctx, state.value);
    if (state.component && state.component->source != nullptr) {
        const std::uint64_t audioSourceId =
            registerAudioSourcePointer(host, state.component->source.get());
        setProperty(ctx, wrapper, "source",
                    syncAudioSourceWrapper(ctx, host, audioSourceId));
    } else {
        setProperty(ctx, wrapper, "source", JS_NULL);
    }
    JS_FreeValue(ctx, wrapper);
}

JSValue jsInitAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->init();
    syncAudioPlayerSourceProperty(ctx, *host, *state);
    return JS_UNDEFINED;
}

JSValue jsPlayAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->play();
    return JS_UNDEFINED;
}

JSValue jsPauseAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->pause();
    return JS_UNDEFINED;
}

JSValue jsStopAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->stop();
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerVolume(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and volume");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double volume = 1.0;
    if (!getDouble(ctx, argv[1], volume)) {
        return JS_ThrowTypeError(ctx, "Expected volume");
    }

    state->component->setVolume(static_cast<float>(volume));
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerLoop(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and loop flag");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->setLoop(JS_ToBool(ctx, argv[1]) == 1);
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerSource(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and resource");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    Resource resource;
    if (!parseResource(ctx, argv[1], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource");
    }

    state->component->setSource(resource);
    syncAudioPlayerSourceProperty(ctx, *host, *state);
    return JS_UNDEFINED;
}

JSValue jsUpdateAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected audio player id and delta time");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double deltaTime = 0.0;
    if (!getDouble(ctx, argv[1], deltaTime)) {
        return JS_ThrowTypeError(ctx, "Expected delta time");
    }

    state->component->update(static_cast<float>(deltaTime));
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerPosition(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and position");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    state->component->setPosition(position);
    return JS_UNDEFINED;
}

JSValue jsUseSpatialAudio(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected audio player id and enabled flag");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    if (JS_ToBool(ctx, argv[1]) == 1) {
        state->component->useSpatialization();
    } else {
        state->component->disableSpatialization();
    }

    return JS_UNDEFINED;
}

JSValue jsCreateRigidbody(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody wrapper");
    }

    auto component = std::make_shared<HostedRigidbodyComponent>();
    component->ctx = ctx;
    component->host = host;
    applyRigidbody(ctx, argv[0], *component);
    const std::uint64_t rigidbodyId =
        registerRigidbodyState(*host, component, ctx, argv[0], false);
    JSValue wrapper = syncRigidbodyWrapper(ctx, *host, rigidbodyId);
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue jsCloneRigidbody(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }

    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr || state->component == nullptr) {
        return JS_EXCEPTION;
    }

    auto component = std::make_shared<HostedRigidbodyComponent>();
    component->ctx = ctx;
    component->host = host;
    component->sendSignal = state->component->sendSignal;
    component->isSensor = state->component->isSensor;
    if (state->component->body != nullptr) {
        component->body =
            std::make_shared<bezel::Rigidbody>(*state->component->body);
        component->body->id.joltId = bezel::INVALID_JOLT_ID;
        component->body->id.atlasId = 0;
    }

    JSValue prototype = JS_GetPrototype(ctx, argv[0]);
    JSValue wrapper = JS_IsException(prototype)
                          ? JS_NewObject(ctx)
                          : JS_NewObjectProto(ctx, prototype);
    JS_FreeValue(ctx, prototype);

    const std::uint64_t rigidbodyId =
        registerRigidbodyState(*host, component, ctx, wrapper, false);
    JS_FreeValue(ctx, wrapper);
    return syncRigidbodyWrapper(ctx, *host, rigidbodyId);
}

JSValue jsInitRigidbody(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }

    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyRigidbody(ctx, argv[0], *state->component);
    state->component->init();
    std::int64_t rigidbodyId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RIGIDBODY_ID_PROP, rigidbodyId);
    return syncRigidbodyWrapper(ctx, *host,
                                static_cast<std::uint64_t>(rigidbodyId));
}

JSValue jsBeforePhysicsRigidbody(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }

    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyRigidbody(ctx, argv[0], *state->component);
    state->component->beforePhysics();
    std::int64_t rigidbodyId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RIGIDBODY_ID_PROP, rigidbodyId);
    return syncRigidbodyWrapper(ctx, *host,
                                static_cast<std::uint64_t>(rigidbodyId));
}

JSValue jsUpdateRigidbody(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and delta time");
    }

    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double deltaTime = 0.0;
    if (!getDouble(ctx, argv[1], deltaTime)) {
        return JS_ThrowTypeError(ctx, "Expected delta time");
    }

    state->component->update(static_cast<float>(deltaTime));
    std::int64_t rigidbodyId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RIGIDBODY_ID_PROP, rigidbodyId);
    return syncRigidbodyWrapper(ctx, *host,
                                static_cast<std::uint64_t>(rigidbodyId));
}

JSValue jsRigidbodyAddCollider(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and collider");
    }

    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double radius = 0.0;
    double height = 0.0;
    if (readNumberProperty(ctx, argv[1], "radius", radius) &&
        readNumberProperty(ctx, argv[1], "height", height)) {
        state->component->addCapsuleCollider(static_cast<float>(radius),
                                             static_cast<float>(height));
    } else {
        JSValue sizeValue = JS_GetPropertyStr(ctx, argv[1], "size");
        if (!JS_IsException(sizeValue) && !JS_IsUndefined(sizeValue)) {
            Size3d size;
            const bool ok = parsePosition3d(ctx, sizeValue, size);
            JS_FreeValue(ctx, sizeValue);
            if (!ok) {
                return JS_ThrowTypeError(ctx, "Expected collider size");
            }
            state->component->addBoxCollider(size);
        } else {
            JS_FreeValue(ctx, sizeValue);
            if (readNumberProperty(ctx, argv[1], "radius", radius)) {
                state->component->addSphereCollider(static_cast<float>(radius));
            } else {
                state->component->addMeshCollider();
            }
        }
    }

    return JS_UNDEFINED;
}

JSValue jsRigidbodySetFriction(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and friction");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected friction");
    }
    state->component->setFriction(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsRigidbodyApplyForce(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and force");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Force3d force;
    if (!parsePosition3d(ctx, argv[1], force)) {
        return JS_ThrowTypeError(ctx, "Expected force");
    }
    state->component->applyForce(force);
    return JS_UNDEFINED;
}

JSValue jsRigidbodyApplyForceAtPoint(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody, force, and point");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Force3d force;
    Position3d point;
    if (!parsePosition3d(ctx, argv[1], force) ||
        !parsePosition3d(ctx, argv[2], point)) {
        return JS_ThrowTypeError(ctx, "Expected force and point");
    }
    state->component->applyForceAtPoint(force, point);
    return JS_UNDEFINED;
}

JSValue jsRigidbodyApplyImpulse(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and impulse");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Impulse3d impulse;
    if (!parsePosition3d(ctx, argv[1], impulse)) {
        return JS_ThrowTypeError(ctx, "Expected impulse");
    }
    state->component->applyImpulse(impulse);
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetLinearVelocity(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Velocity3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->setLinearVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsRigidbodyAddLinearVelocity(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Velocity3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->addLinearVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetAngularVelocity(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Velocity3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->setAngularVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsRigidbodyAddAngularVelocity(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Velocity3d velocity;
    if (!parsePosition3d(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->addAngularVelocity(velocity);
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetMaxLinearVelocity(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->setMaxLinearVelocity(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetMaxAngularVelocity(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and velocity");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double value = 0.0;
    if (!getDouble(ctx, argv[1], value)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    state->component->setMaxAngularVelocity(static_cast<float>(value));
    return JS_UNDEFINED;
}

JSValue jsRigidbodyGetLinearVelocity(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    return makePosition3d(ctx, *host, state->component->getLinearVelocity());
}

JSValue jsRigidbodyGetAngularVelocity(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    return makePosition3d(ctx, *host, state->component->getAngularVelocity());
}

JSValue jsRigidbodyGetVelocity(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    return makePosition3d(ctx, *host, state->component->getVelocity());
}

JSValue jsRigidbodyHasTag(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and tag");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    const char *tag = JS_ToCString(ctx, argv[1]);
    if (tag == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected tag");
    }
    const bool result = state->component->hasTag(tag);
    JS_FreeCString(ctx, tag);
    return JS_NewBool(ctx, result);
}

JSValue jsRigidbodyAddTag(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and tag");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    const char *tag = JS_ToCString(ctx, argv[1]);
    if (tag == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected tag");
    }
    state->component->addTag(tag);
    JS_FreeCString(ctx, tag);
    return JS_UNDEFINED;
}

JSValue jsRigidbodyRemoveTag(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and tag");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    const char *tag = JS_ToCString(ctx, argv[1]);
    if (tag == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected tag");
    }
    state->component->removeTag(tag);
    JS_FreeCString(ctx, tag);
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetDamping(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and damping values");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double linear = 0.0;
    double angular = 0.0;
    if (!getDouble(ctx, argv[1], linear) || !getDouble(ctx, argv[2], angular)) {
        return JS_ThrowTypeError(ctx, "Expected damping values");
    }
    state->component->setDamping(static_cast<float>(linear),
                                 static_cast<float>(angular));
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetMass(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and mass");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double mass = 0.0;
    if (!getDouble(ctx, argv[1], mass)) {
        return JS_ThrowTypeError(ctx, "Expected mass");
    }
    state->component->setMass(static_cast<float>(mass));
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetRestitution(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and restitution");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double restitution = 0.0;
    if (!getDouble(ctx, argv[1], restitution)) {
        return JS_ThrowTypeError(ctx, "Expected restitution");
    }
    state->component->setRestitution(static_cast<float>(restitution));
    return JS_UNDEFINED;
}

JSValue jsRigidbodySetMotionType(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and motion type");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    const char *motionType = JS_ToCString(ctx, argv[1]);
    if (motionType == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected motion type");
    }
    state->component->setMotionType(parseMotionTypeValue(motionType));
    JS_FreeCString(ctx, motionType);
    return JS_UNDEFINED;
}

JSValue jsSensorSetSignal(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and signal");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    const char *signal = JS_ToCString(ctx, argv[1]);
    if (signal == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected signal");
    }
    state->component->sendSignal = signal;
    ensureBezelBody(*state->component)->sensorSignal = signal;
    JS_FreeCString(ctx, signal);
    return JS_UNDEFINED;
}

JSValue jsCreateVehicle(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected vehicle wrapper");
    }
    auto component = std::make_shared<HostedVehicleComponent>();
    component->ctx = ctx;
    component->host = host;
    applyVehicle(ctx, argv[0], component->vehicle);
    const std::uint64_t vehicleId = registerVehicleState(
        *host, component, &component->vehicle, ctx, argv[0], false);
    JSValue wrapper = syncVehicleWrapper(ctx, *host, vehicleId);
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue jsVehicleRequestRecreate(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected vehicle");
    }
    auto *state = resolveVehicle(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    applyVehicle(ctx, argv[0], *state->component);
    if (state->ownedComponent) {
        state->component->object = state->ownedComponent->object;
    }
    state->component->requestRecreate();
    std::int64_t vehicleId = 0;
    readIntProperty(ctx, argv[0], ATLAS_VEHICLE_ID_PROP, vehicleId);
    return syncVehicleWrapper(ctx, *host,
                              static_cast<std::uint64_t>(vehicleId));
}

JSValue jsVehicleBeforePhysics(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected vehicle");
    }
    auto *state = resolveVehicle(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    applyVehicle(ctx, argv[0], *state->component);
    if (state->ownedComponent) {
        state->component->object = state->ownedComponent->object;
    }
    state->component->beforePhysics();
    std::int64_t vehicleId = 0;
    readIntProperty(ctx, argv[0], ATLAS_VEHICLE_ID_PROP, vehicleId);
    return syncVehicleWrapper(ctx, *host,
                              static_cast<std::uint64_t>(vehicleId));
}

JSValue jsCreateFixedJoint(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected fixed joint wrapper");
    }
    auto component = std::make_shared<HostedFixedJointComponent>();
    component->ctx = ctx;
    component->host = host;
    applyFixedJoint(ctx, *host, argv[0], component->joint);
    const std::uint64_t jointId = registerFixedJointState(
        *host, component, &component->joint, ctx, argv[0], false);
    JSValue wrapper = syncFixedJointWrapper(ctx, *host, jointId);
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue jsFixedJointBeforePhysics(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected fixed joint");
    }
    auto *state = resolveFixedJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    applyFixedJoint(ctx, *host, argv[0], *state->component);
    if (state->ownedComponent) {
        state->component->object = state->ownedComponent->object;
    }
    state->component->beforePhysics();
    std::int64_t jointId = 0;
    readIntProperty(ctx, argv[0], ATLAS_FIXED_JOINT_ID_PROP, jointId);
    return syncFixedJointWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(jointId));
}

JSValue jsFixedJointBreak(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected fixed joint");
    }
    auto *state = resolveFixedJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    state->component->breakJoint();
    return JS_UNDEFINED;
}

JSValue jsCreateHingeJoint(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected hinge joint wrapper");
    }
    auto component = std::make_shared<HostedHingeJointComponent>();
    component->ctx = ctx;
    component->host = host;
    applyHingeJoint(ctx, *host, argv[0], component->joint);
    const std::uint64_t jointId = registerHingeJointState(
        *host, component, &component->joint, ctx, argv[0], false);
    JSValue wrapper = syncHingeJointWrapper(ctx, *host, jointId);
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue jsHingeJointBeforePhysics(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected hinge joint");
    }
    auto *state = resolveHingeJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    applyHingeJoint(ctx, *host, argv[0], *state->component);
    if (state->ownedComponent) {
        state->component->object = state->ownedComponent->object;
    }
    state->component->beforePhysics();
    std::int64_t jointId = 0;
    readIntProperty(ctx, argv[0], ATLAS_HINGE_JOINT_ID_PROP, jointId);
    return syncHingeJointWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(jointId));
}

JSValue jsHingeJointBreak(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected hinge joint");
    }
    auto *state = resolveHingeJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    state->component->breakJoint();
    return JS_UNDEFINED;
}

JSValue jsCreateSpringJoint(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spring joint wrapper");
    }
    auto component = std::make_shared<HostedSpringJointComponent>();
    component->ctx = ctx;
    component->host = host;
    applySpringJoint(ctx, *host, argv[0], component->joint);
    const std::uint64_t jointId = registerSpringJointState(
        *host, component, &component->joint, ctx, argv[0], false);
    JSValue wrapper = syncSpringJointWrapper(ctx, *host, jointId);
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue jsSpringJointBeforePhysics(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spring joint");
    }
    auto *state = resolveSpringJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    applySpringJoint(ctx, *host, argv[0], *state->component);
    if (state->ownedComponent) {
        state->component->object = state->ownedComponent->object;
    }
    state->component->beforePhysics();
    std::int64_t jointId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPRING_JOINT_ID_PROP, jointId);
    return syncSpringJointWrapper(ctx, *host,
                                  static_cast<std::uint64_t>(jointId));
}

JSValue jsSpringJointBreak(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spring joint");
    }
    auto *state = resolveSpringJoint(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    state->component->breakJoint();
    return JS_UNDEFINED;
}

JSValue runRigidbodyRaycastQuery(JSContext *ctx, ScriptHost &host,
                                 Rigidbody &component, QueryOperation operation,
                                 const bezel::RaycastResult &nativeResult) {
    QueryResult result;
    result.operation = operation;
    result.raycastResult = convertRaycastResult(nativeResult);
    dispatchQueryResultToObject(ctx, host, component, result);
    return makeQueryResultValue(ctx, host, result);
}

JSValue jsRigidbodyRaycast(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected rigidbody, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parsePosition3d(ctx, argv[1], direction) ||
        !getDouble(ctx, argv[2], maxDistance)) {
        return JS_ThrowTypeError(ctx, "Expected direction and distance");
    }
    auto body = ensureBezelBody(*state->component);
    return runRigidbodyRaycastQuery(
        ctx, *host, *state->component, QueryOperation::Raycast,
        body->raycast(direction, static_cast<float>(maxDistance),
                      host->context->window->physicsWorld));
}

JSValue jsRigidbodyRaycastAll(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected rigidbody, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parsePosition3d(ctx, argv[1], direction) ||
        !getDouble(ctx, argv[2], maxDistance)) {
        return JS_ThrowTypeError(ctx, "Expected direction and distance");
    }
    auto body = ensureBezelBody(*state->component);
    return runRigidbodyRaycastQuery(
        ctx, *host, *state->component, QueryOperation::RaycastAll,
        body->raycastAll(direction, static_cast<float>(maxDistance),
                         host->context->window->physicsWorld));
}

JSValue jsRigidbodyRaycastWorld(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, origin, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d origin;
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parsePosition3d(ctx, argv[1], origin) ||
        !parsePosition3d(ctx, argv[2], direction) ||
        !getDouble(ctx, argv[3], maxDistance)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected origin, direction, and distance");
    }
    QueryResult result;
    result.operation = QueryOperation::RaycastWorld;
    result.raycastResult =
        convertRaycastResult(host->context->window->physicsWorld->raycast(
            origin, direction, static_cast<float>(maxDistance)));
    dispatchQueryResultToObject(ctx, *host, *state->component, result);
    return makeQueryResultValue(ctx, *host, result);
}

JSValue jsRigidbodyRaycastWorldAll(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, origin, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d origin;
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parsePosition3d(ctx, argv[1], origin) ||
        !parsePosition3d(ctx, argv[2], direction) ||
        !getDouble(ctx, argv[3], maxDistance)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected origin, direction, and distance");
    }
    QueryResult result;
    result.operation = QueryOperation::RaycastWorldAll;
    result.raycastResult =
        convertRaycastResult(host->context->window->physicsWorld->raycastAll(
            origin, direction, static_cast<float>(maxDistance)));
    dispatchQueryResultToObject(ctx, *host, *state->component, result);
    return makeQueryResultValue(ctx, *host, result);
}

JSValue jsRigidbodyRaycastTagged(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, tags, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    std::vector<std::string> tags;
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parseStringArray(ctx, argv[1], tags) ||
        !parsePosition3d(ctx, argv[2], direction) ||
        !getDouble(ctx, argv[3], maxDistance)) {
        return JS_ThrowTypeError(ctx, "Expected tags, direction, and distance");
    }
    auto body = ensureBezelBody(*state->component);
    return runRigidbodyRaycastQuery(
        ctx, *host, *state->component, QueryOperation::RaycastTagged,
        convertToTaggedRaycastResult(
            body->raycast(direction, static_cast<float>(maxDistance),
                          host->context->window->physicsWorld),
            tags));
}

JSValue jsRigidbodyRaycastTaggedAll(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, tags, direction, and distance");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    std::vector<std::string> tags;
    Normal3d direction;
    double maxDistance = 0.0;
    if (!parseStringArray(ctx, argv[1], tags) ||
        !parsePosition3d(ctx, argv[2], direction) ||
        !getDouble(ctx, argv[3], maxDistance)) {
        return JS_ThrowTypeError(ctx, "Expected tags, direction, and distance");
    }
    auto body = ensureBezelBody(*state->component);
    return runRigidbodyRaycastQuery(
        ctx, *host, *state->component, QueryOperation::RaycastTaggedAll,
        convertToTaggedRaycastResult(
            body->raycastAll(direction, static_cast<float>(maxDistance),
                             host->context->window->physicsWorld),
            tags));
}

JSValue jsRigidbodyOverlap(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    auto body = ensureBezelBody(*state->component);
    QueryResult result;
    result.operation = QueryOperation::Overlap;
    if (body->collider != nullptr && state->component->object != nullptr) {
        result.overlapResult = convertOverlapResultToAtlas(
            body->overlap(host->context->window->physicsWorld, body->collider,
                          state->component->object->getPosition(),
                          state->component->object->getRotation()));
    }
    dispatchQueryResultToObject(ctx, *host, *state->component, result);
    return makeQueryResultValue(ctx, *host, result);
}

JSValue jsRigidbodyOverlapWithCollider(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and collider");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    auto collider =
        makeColliderFromScript(ctx, argv[1], state->component->object);
    if (collider == nullptr || state->component->object == nullptr) {
        QueryResult result;
        result.operation = QueryOperation::Overlap;
        return makeQueryResultValue(ctx, *host, result);
    }
    auto body = ensureBezelBody(*state->component);
    QueryResult result;
    result.operation = QueryOperation::Overlap;
    result.overlapResult = convertOverlapResultToAtlas(
        body->overlap(host->context->window->physicsWorld, collider,
                      state->component->object->getPosition(),
                      state->component->object->getRotation()));
    dispatchQueryResultToObject(ctx, *host, *state->component, result);
    return makeQueryResultValue(ctx, *host, result);
}

JSValue jsRigidbodyOverlapWithColliderWorld(JSContext *ctx, JSValueConst,
                                            int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected rigidbody, collider, and position");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d position;
    if (!parsePosition3d(ctx, argv[2], position)) {
        return JS_ThrowTypeError(ctx, "Expected position");
    }
    auto collider =
        makeColliderFromScript(ctx, argv[1], state->component->object);
    QueryResult result;
    result.operation = QueryOperation::Overlap;
    if (collider != nullptr) {
        auto body = ensureBezelBody(*state->component);
        result.overlapResult = convertOverlapResultToAtlas(body->overlap(
            host->context->window->physicsWorld, collider, position,
            state->component->object != nullptr
                ? state->component->object->getRotation()
                : Rotation3d{}));
    }
    dispatchQueryResultToObject(ctx, *host, *state->component, result);
    return makeQueryResultValue(ctx, *host, result);
}

JSValue runRigidbodySweepQuery(JSContext *ctx, ScriptHost &host,
                               Rigidbody &component, QueryOperation operation,
                               const bezel::SweepResult &nativeResult,
                               const Position3d &endPosition) {
    QueryResult result;
    result.operation = operation;
    result.sweepResult = convertSweepResultToAtlas(nativeResult, endPosition);
    dispatchQueryResultToObject(ctx, host, component, result);
    return makeQueryResultValue(ctx, host, result);
}

JSValue jsRigidbodyPredictMovementWithCollider(JSContext *ctx, JSValueConst,
                                               int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, end position, and collider");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected end position");
    }
    auto collider =
        makeColliderFromScript(ctx, argv[2], state->component->object);
    if (collider == nullptr || state->component->object == nullptr) {
        QueryResult result;
        result.operation = QueryOperation::Movement;
        result.sweepResult.endPosition = endPosition;
        return makeQueryResultValue(ctx, *host, result);
    }
    auto body = ensureBezelBody(*state->component);
    Position3d actualEnd = endPosition;
    const Position3d direction =
        endPosition - state->component->object->getPosition();
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::Movement,
        body->sweep(host->context->window->physicsWorld, collider, direction,
                    actualEnd),
        actualEnd);
}

JSValue jsRigidbodyPredictMovement(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and end position");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected end position");
    }
    auto body = ensureBezelBody(*state->component);
    if (body->collider == nullptr || state->component->object == nullptr) {
        QueryResult result;
        result.operation = QueryOperation::Movement;
        result.sweepResult.endPosition = endPosition;
        return makeQueryResultValue(ctx, *host, result);
    }
    Position3d actualEnd = endPosition;
    const Position3d direction =
        endPosition - state->component->object->getPosition();
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::Movement,
        body->sweep(host->context->window->physicsWorld, body->collider,
                    direction, actualEnd),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementWithColliderWorld(JSContext *ctx,
                                                    JSValueConst, int argc,
                                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx,
            "Expected rigidbody, start position, end position, and collider");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d startPosition;
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], startPosition) ||
        !parsePosition3d(ctx, argv[2], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected start and end positions");
    }
    auto collider =
        makeColliderFromScript(ctx, argv[3], state->component->object);
    QueryResult result;
    result.operation = QueryOperation::Movement;
    result.sweepResult.endPosition = endPosition;
    if (collider == nullptr) {
        return makeQueryResultValue(ctx, *host, result);
    }
    auto body = ensureBezelBody(*state->component);
    Position3d actualEnd = endPosition;
    const Position3d direction = endPosition - startPosition;
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::Movement,
        host->context->window->physicsWorld->sweep(
            host->context->window->physicsWorld, collider, startPosition,
            state->component->object != nullptr
                ? state->component->object->getRotation()
                : Rotation3d{},
            direction, actualEnd, body->id.joltId),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementWorld(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, start position, and end position");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d startPosition;
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], startPosition) ||
        !parsePosition3d(ctx, argv[2], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected start and end positions");
    }
    auto body = ensureBezelBody(*state->component);
    QueryResult result;
    result.operation = QueryOperation::Movement;
    result.sweepResult.endPosition = endPosition;
    if (body->collider == nullptr) {
        return makeQueryResultValue(ctx, *host, result);
    }
    Position3d actualEnd = endPosition;
    const Position3d direction = endPosition - startPosition;
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::Movement,
        host->context->window->physicsWorld->sweep(
            host->context->window->physicsWorld, body->collider, startPosition,
            state->component->object != nullptr
                ? state->component->object->getRotation()
                : Rotation3d{},
            direction, actualEnd, body->id.joltId),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementWithColliderAll(JSContext *ctx, JSValueConst,
                                                  int argc,
                                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, end position, and collider");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected end position");
    }
    auto collider =
        makeColliderFromScript(ctx, argv[2], state->component->object);
    if (collider == nullptr || state->component->object == nullptr) {
        QueryResult result;
        result.operation = QueryOperation::MovementAll;
        result.sweepResult.endPosition = endPosition;
        return makeQueryResultValue(ctx, *host, result);
    }
    auto body = ensureBezelBody(*state->component);
    Position3d actualEnd = endPosition;
    const Position3d direction =
        endPosition - state->component->object->getPosition();
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::MovementAll,
        body->sweepAll(host->context->window->physicsWorld, collider, direction,
                       actualEnd),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementAll(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected rigidbody and end position");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected end position");
    }
    auto body = ensureBezelBody(*state->component);
    QueryResult result;
    result.operation = QueryOperation::MovementAll;
    result.sweepResult.endPosition = endPosition;
    if (body->collider == nullptr || state->component->object == nullptr) {
        return makeQueryResultValue(ctx, *host, result);
    }
    Position3d actualEnd = endPosition;
    const Position3d direction =
        endPosition - state->component->object->getPosition();
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::MovementAll,
        body->sweepAll(host->context->window->physicsWorld, body->collider,
                       direction, actualEnd),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementWithColliderAllWorld(JSContext *ctx,
                                                       JSValueConst, int argc,
                                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx,
            "Expected rigidbody, start position, end position, and collider");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d startPosition;
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], startPosition) ||
        !parsePosition3d(ctx, argv[2], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected start and end positions");
    }
    auto collider =
        makeColliderFromScript(ctx, argv[3], state->component->object);
    QueryResult result;
    result.operation = QueryOperation::MovementAll;
    result.sweepResult.endPosition = endPosition;
    if (collider == nullptr) {
        return makeQueryResultValue(ctx, *host, result);
    }
    auto body = ensureBezelBody(*state->component);
    Position3d actualEnd = endPosition;
    const Position3d direction = endPosition - startPosition;
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::MovementAll,
        host->context->window->physicsWorld->sweepAll(
            host->context->window->physicsWorld, collider, startPosition,
            state->component->object != nullptr
                ? state->component->object->getRotation()
                : Rotation3d{},
            direction, actualEnd, body->id.joltId),
        actualEnd);
}

JSValue jsRigidbodyPredictMovementAllWorld(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected rigidbody, start position, and end position");
    }
    auto *state = resolveRigidbody(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    Position3d startPosition;
    Position3d endPosition;
    if (!parsePosition3d(ctx, argv[1], startPosition) ||
        !parsePosition3d(ctx, argv[2], endPosition)) {
        return JS_ThrowTypeError(ctx, "Expected start and end positions");
    }
    auto body = ensureBezelBody(*state->component);
    QueryResult result;
    result.operation = QueryOperation::MovementAll;
    result.sweepResult.endPosition = endPosition;
    if (body->collider == nullptr) {
        return makeQueryResultValue(ctx, *host, result);
    }
    Position3d actualEnd = endPosition;
    const Position3d direction = endPosition - startPosition;
    return runRigidbodySweepQuery(
        ctx, *host, *state->component, QueryOperation::MovementAll,
        host->context->window->physicsWorld->sweepAll(
            host->context->window->physicsWorld, body->collider, startPosition,
            state->component->object != nullptr
                ? state->component->object->getRotation()
                : Rotation3d{},
            direction, actualEnd, body->id.joltId),
        actualEnd);
}

JSValue jsGetInputConstants(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    JSValue result = JS_NewObject(ctx);
    JSValue keyObject = JS_NewObject(ctx);
    for (const auto &[name, key] : ATLAS_KEY_ENTRIES) {
        setProperty(ctx, keyObject, name,
                    JS_NewInt32(ctx, static_cast<int>(key)));
    }

    JSValue mouseButtonObject = JS_NewObject(ctx);
    for (const auto &[name, button] : ATLAS_MOUSE_BUTTON_ENTRIES) {
        setProperty(ctx, mouseButtonObject, name,
                    JS_NewInt32(ctx, static_cast<int>(button)));
    }

    JSValue triggerTypeObject = JS_NewObject(ctx);
    setProperty(ctx, triggerTypeObject, "MouseButton",
                JS_NewInt32(ctx, static_cast<int>(TriggerType::MouseButton)));
    setProperty(ctx, triggerTypeObject, "Key",
                JS_NewInt32(ctx, static_cast<int>(TriggerType::Key)));
    setProperty(
        ctx, triggerTypeObject, "ControllerButton",
        JS_NewInt32(ctx, static_cast<int>(TriggerType::ControllerButton)));

    JSValue axisTriggerTypeObject = JS_NewObject(ctx);
    setProperty(ctx, axisTriggerTypeObject, "MouseAxis",
                JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::MouseAxis)));
    setProperty(ctx, axisTriggerTypeObject, "KeyCustom",
                JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::KeyCustom)));
    setProperty(
        ctx, axisTriggerTypeObject, "ControllerAxis",
        JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::ControllerAxis)));

    setProperty(ctx, result, "Key", keyObject);
    setProperty(ctx, result, "MouseButton", mouseButtonObject);
    setProperty(ctx, result, "TriggerType", triggerTypeObject);
    setProperty(ctx, result, "AxisTriggerType", axisTriggerTypeObject);
    return result;
}

JSValue jsGetWindowConstants(JSContext *ctx, JSValueConst, int,
                             JSValueConst *) {
    JSValue result = JS_NewObject(ctx);

    JSValue controllerAxis = JS_NewObject(ctx);
    setProperty(ctx, controllerAxis, "LeftStick",
                JS_NewInt32(ctx, static_cast<int>(ControllerAxis::LeftStick)));
    setProperty(ctx, controllerAxis, "LeftStickX",
                JS_NewInt32(ctx, static_cast<int>(ControllerAxis::LeftStickX)));
    setProperty(ctx, controllerAxis, "LeftStickY",
                JS_NewInt32(ctx, static_cast<int>(ControllerAxis::LeftStickY)));
    setProperty(ctx, controllerAxis, "RightStick",
                JS_NewInt32(ctx, static_cast<int>(ControllerAxis::RightStick)));
    setProperty(
        ctx, controllerAxis, "RightStickX",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::RightStickX)));
    setProperty(
        ctx, controllerAxis, "RightStickY",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::RightStickY)));
    setProperty(ctx, controllerAxis, "Trigger",
                JS_NewInt32(ctx, static_cast<int>(ControllerAxis::Trigger)));
    setProperty(
        ctx, controllerAxis, "TriggerLeft",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::LeftTrigger)));
    setProperty(
        ctx, controllerAxis, "TriggerRight",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::RightTrigger)));
    setProperty(
        ctx, controllerAxis, "LeftTrigger",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::LeftTrigger)));
    setProperty(
        ctx, controllerAxis, "RightTrigger",
        JS_NewInt32(ctx, static_cast<int>(ControllerAxis::RightTrigger)));

    JSValue controllerButton = JS_NewObject(ctx);
    setProperty(ctx, controllerButton, "A",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::A)));
    setProperty(ctx, controllerButton, "B",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::B)));
    setProperty(ctx, controllerButton, "X",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::X)));
    setProperty(ctx, controllerButton, "Y",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::Y)));
    setProperty(
        ctx, controllerButton, "LeftBumper",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::LeftBumper)));
    setProperty(
        ctx, controllerButton, "RightBumper",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::RightBumper)));
    setProperty(ctx, controllerButton, "Back",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::Back)));
    setProperty(ctx, controllerButton, "Start",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::Start)));
    setProperty(ctx, controllerButton, "Guide",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::Guide)));
    setProperty(
        ctx, controllerButton, "LeftThumb",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::LeftThumb)));
    setProperty(
        ctx, controllerButton, "RightThumb",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::RightThumb)));
    setProperty(ctx, controllerButton, "DPadUp",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::DPadUp)));
    setProperty(
        ctx, controllerButton, "DPadRight",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::DPadRight)));
    setProperty(ctx, controllerButton, "DPadDown",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::DPadDown)));
    setProperty(ctx, controllerButton, "DPadLeft",
                JS_NewInt32(ctx, static_cast<int>(ControllerButton::DPadLeft)));
    setProperty(
        ctx, controllerButton, "ButtonCount",
        JS_NewInt32(ctx, static_cast<int>(ControllerButton::ButtonCount)));

    JSValue nintendoButton = JS_NewObject(ctx);
    setProperty(
        ctx, nintendoButton, "B",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::B)));
    setProperty(
        ctx, nintendoButton, "A",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::A)));
    setProperty(
        ctx, nintendoButton, "Y",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::Y)));
    setProperty(
        ctx, nintendoButton, "X",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::X)));
    setProperty(
        ctx, nintendoButton, "L",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::L)));
    setProperty(
        ctx, nintendoButton, "R",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::R)));
    setProperty(
        ctx, nintendoButton, "ZL",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::ZL)));
    setProperty(
        ctx, nintendoButton, "ZR",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::ZR)));
    setProperty(
        ctx, nintendoButton, "Minus",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::Minus)));
    setProperty(
        ctx, nintendoButton, "Plus",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::Plus)));
    setProperty(ctx, nintendoButton, "LeftStick",
                JS_NewInt32(ctx, static_cast<int>(
                                     NintendoControllerButton::LeftStick)));
    setProperty(ctx, nintendoButton, "RightStick",
                JS_NewInt32(ctx, static_cast<int>(
                                     NintendoControllerButton::RightStick)));
    setProperty(
        ctx, nintendoButton, "DPadUp",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::DPadUp)));
    setProperty(ctx, nintendoButton, "DPadRight",
                JS_NewInt32(ctx, static_cast<int>(
                                     NintendoControllerButton::DPadRight)));
    setProperty(
        ctx, nintendoButton, "DPadDown",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::DPadDown)));
    setProperty(
        ctx, nintendoButton, "DPadLeft",
        JS_NewInt32(ctx, static_cast<int>(NintendoControllerButton::DPadLeft)));
    setProperty(ctx, nintendoButton, "ButtonCount",
                JS_NewInt32(ctx, static_cast<int>(
                                     NintendoControllerButton::ButtonCount)));

    JSValue sonyButton = JS_NewObject(ctx);
    setProperty(
        ctx, sonyButton, "Cross",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Cross)));
    setProperty(
        ctx, sonyButton, "Circle",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Circle)));
    setProperty(
        ctx, sonyButton, "Square",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Square)));
    setProperty(
        ctx, sonyButton, "Triangle",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Triangle)));
    setProperty(ctx, sonyButton, "L1",
                JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::L1)));
    setProperty(ctx, sonyButton, "R1",
                JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::R1)));
    setProperty(ctx, sonyButton, "L2",
                JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::L2)));
    setProperty(ctx, sonyButton, "R2",
                JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::R2)));
    setProperty(
        ctx, sonyButton, "Share",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Share)));
    setProperty(
        ctx, sonyButton, "Options",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::Options)));
    setProperty(
        ctx, sonyButton, "LeftStick",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::LeftStick)));
    setProperty(
        ctx, sonyButton, "RightStick",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::RightStick)));
    setProperty(
        ctx, sonyButton, "DPadUp",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::DPadUp)));
    setProperty(
        ctx, sonyButton, "DPadRight",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::DPadRight)));
    setProperty(
        ctx, sonyButton, "DPadDown",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::DPadDown)));
    setProperty(
        ctx, sonyButton, "DPadLeft",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::DPadLeft)));
    setProperty(
        ctx, sonyButton, "ButtonCount",
        JS_NewInt32(ctx, static_cast<int>(SonyControllerButton::ButtonCount)));

    setProperty(ctx, result, "ControllerAxis", controllerAxis);
    setProperty(ctx, result, "ControllerButton", controllerButton);
    setProperty(ctx, result, "NintendoControllerButton", nintendoButton);
    setProperty(ctx, result, "SonyControllerButton", sonyButton);
    setProperty(ctx, result, "CONTROLLER_UNDEFINED",
                JS_NewInt32(ctx, CONTROLLER_UNDEFINED));
    return result;
}

JSValue jsRegisterInputAction(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected input action");
    }

    InputAction action;
    if (!parseInputAction(ctx, argv[0], action)) {
        return JS_ThrowTypeError(ctx, "Expected InputAction");
    }

    auto existing = host->context->window->getInputAction(action.name);
    if (existing != nullptr) {
        *existing = action;
    } else {
        host->context->window->addInputAction(
            std::make_shared<InputAction>(action));
    }

    return JS_DupValue(ctx, argv[0]);
}

JSValue jsResetInputActions(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->resetInputActions();
    }
    return JS_UNDEFINED;
}

JSValue jsIsKeyActive(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t key = 0;
    if (!getInt64(ctx, argv[0], key)) {
        return JS_ThrowTypeError(ctx, "Expected key");
    }
    return JS_NewBool(
        ctx, host->context->window->isKeyActive(static_cast<Key>(key)));
}

JSValue jsIsKeyPressed(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t key = 0;
    if (!getInt64(ctx, argv[0], key)) {
        return JS_ThrowTypeError(ctx, "Expected key");
    }
    return JS_NewBool(
        ctx, host->context->window->isKeyPressed(static_cast<Key>(key)));
}

JSValue jsIsMouseButtonActive(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t button = 0;
    if (!getInt64(ctx, argv[0], button)) {
        return JS_ThrowTypeError(ctx, "Expected mouse button");
    }
    return JS_NewBool(ctx, host->context->window->isMouseButtonActive(
                               static_cast<MouseButton>(button)));
}

JSValue jsIsMouseButtonPressed(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t button = 0;
    if (!getInt64(ctx, argv[0], button)) {
        return JS_ThrowTypeError(ctx, "Expected mouse button");
    }
    return JS_NewBool(ctx, host->context->window->isMouseButtonPressed(
                               static_cast<MouseButton>(button)));
}

JSValue jsGetTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_NewString(ctx, "");
    }
    return JS_NewString(ctx, host->context->window->getTextInput().c_str());
}

JSValue jsStartTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->startTextInput();
    }
    return JS_UNDEFINED;
}

JSValue jsStopTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->stopTextInput();
    }
    return JS_UNDEFINED;
}

JSValue jsIsTextInputActive(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_FALSE;
    }
    return JS_NewBool(ctx, host->context->window->isTextInputActive());
}

JSValue jsIsControllerButtonPressed(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_FALSE;
    }

    std::int64_t controllerID = 0;
    std::int64_t buttonIndex = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], buttonIndex)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected controller id and button index");
    }

    return JS_NewBool(ctx, host->context->window->isControllerButtonPressed(
                               static_cast<int>(controllerID),
                               static_cast<int>(buttonIndex)));
}

JSValue jsGetControllerAxisValue(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_NewFloat64(ctx, 0.0);
    }

    std::int64_t controllerID = 0;
    std::int64_t axisIndex = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], axisIndex)) {
        return JS_ThrowTypeError(ctx, "Expected controller id and axis index");
    }

    return JS_NewFloat64(
        ctx, host->context->window->getControllerAxisValue(
                 static_cast<int>(controllerID), static_cast<int>(axisIndex)));
}

JSValue jsGetControllerAxisPairValue(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return makePlainPosition2d(ctx, Position2d(0.0, 0.0));
    }

    std::int64_t controllerID = 0;
    std::int64_t axisIndexX = 0;
    std::int64_t axisIndexY = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], axisIndexX) ||
        !getInt64(ctx, argv[2], axisIndexY)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected controller id and axis indices");
    }

    const auto value = host->context->window->getControllerAxisPairValue(
        static_cast<int>(controllerID), static_cast<int>(axisIndexX),
        static_cast<int>(axisIndexY));
    return makePlainPosition2d(ctx, Position2d(value.first, value.second));
}

JSValue jsCaptureMouse(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->captureMouse();
    }
    return JS_UNDEFINED;
}

JSValue jsReleaseMouse(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->releaseMouse();
    }
    return JS_UNDEFINED;
}

JSValue jsGetMousePosition(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return makePlainPosition2d(ctx, Position2d(0.0, 0.0));
    }

    const auto [x, y] = host->context->window->getCursorPosition();
    return makePlainPosition2d(ctx, Position2d(x, y));
}

JSValue jsIsActionTriggered(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const bool result = host->context->window->isActionTriggered(name);
    JS_FreeCString(ctx, name);
    return JS_NewBool(ctx, result);
}

JSValue jsIsActionCurrentlyActive(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const bool result = host->context->window->isActionCurrentlyActive(name);
    JS_FreeCString(ctx, name);
    return JS_NewBool(ctx, result);
}

JSValue jsGetAxisActionValue(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return makeAxisPacketValue(ctx, {});
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const AxisPacket packet = host->context->window->getAxisActionValue(name);
    JS_FreeCString(ctx, name);
    return makeAxisPacketValue(ctx, packet);
}

JSValue jsRegisterInteractive(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1 || !JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected interactive object");
    }

    for (const JSValue &interactive : host->interactiveValues) {
        if (JS_IsStrictEqual(ctx, interactive, argv[0])) {
            return JS_DupValue(ctx, argv[0]);
        }
    }

    host->interactiveValues.push_back(JS_DupValue(ctx, argv[0]));
    return JS_DupValue(ctx, argv[0]);
}

class HostedScriptComponent final : public Component {
  public:
    HostedScriptComponent(JSContext *context, ScriptHost *scriptHost,
                          JSValueConst value, std::string componentName)
        : ctx(context), host(scriptHost), name(std::move(componentName)),
          instance(JS_DupValue(context, value)) {}

    ~HostedScriptComponent() override {
        if (ctx != nullptr && !JS_IsUndefined(instance)) {
            JS_FreeValue(ctx, instance);
        }
    }

    void atAttach() override {
        if (host == nullptr || object == nullptr) {
            return;
        }
        runtime::scripting::registerComponentInstance(
            ctx, *host, this, object->getId(), name, instance);
        call("atAttach", 0, nullptr);
    }

    void init() override {
        if (!initialized) {
            initialized = true;
            call("init", 0, nullptr);
        }
    }

    void beforePhysics() override { call("beforePhysics", 0, nullptr); }

    void update(float deltaTime) override {
        JSValue delta = JS_NewFloat64(ctx, deltaTime);
        JSValueConst args[] = {delta};
        call("update", 1, args);
        JS_FreeValue(ctx, delta);
    }

    void onCollisionEnter(GameObject *other) override {
        JSValue args[] = {
            other != nullptr ? syncObjectWrapper(ctx, *host, *other) : JS_NULL};
        call("onCollisionEnter", 1, args);
        JS_FreeValue(ctx, args[0]);
    }

    void onCollisionStay(GameObject *other) override {
        JSValue args[] = {
            other != nullptr ? syncObjectWrapper(ctx, *host, *other) : JS_NULL};
        call("onCollisionStay", 1, args);
        JS_FreeValue(ctx, args[0]);
    }

    void onCollisionExit(GameObject *other) override {
        JSValue args[] = {
            other != nullptr ? syncObjectWrapper(ctx, *host, *other) : JS_NULL};
        call("onCollisionExit", 1, args);
        JS_FreeValue(ctx, args[0]);
    }

    void onSignalRecieve(const std::string &signal,
                         GameObject *sender) override {
        JSValue args[] = {JS_NewString(ctx, signal.c_str()),
                          sender != nullptr
                              ? syncObjectWrapper(ctx, *host, *sender)
                              : JS_NULL};
        call("onSignalRecieve", 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
    }

    void onSignalEnd(const std::string &signal, GameObject *sender) override {
        JSValue args[] = {JS_NewString(ctx, signal.c_str()),
                          sender != nullptr
                              ? syncObjectWrapper(ctx, *host, *sender)
                              : JS_NULL};
        call("onSignalEnd", 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
    }

    void onQueryReceive(QueryResult &result) override {
        JSValue args[] = {makeQueryResultValue(ctx, *host, result),
                          object != nullptr
                              ? syncObjectWrapper(ctx, *host, *object)
                              : JS_NULL};
        callAlias("onQueryRecieve", "onQueryReceive", 2, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
    }

  private:
    bool call(const char *method, int argc, JSValueConst *argv) {
        return callObjectMethod(ctx, instance, method, argc, argv);
    }

    bool callAlias(const char *primary, const char *secondary, int argc,
                   JSValueConst *argv) {
        return callObjectMethodEither(ctx, instance, primary, secondary, argc,
                                      argv);
    }

    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::string name;
    JSValue instance = JS_UNDEFINED;
    bool initialized = false;
};

JSValue jsAddComponent(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object id and component");
    }

    std::int64_t ownerId = 0;
    if (!getInt64(ctx, argv[0], ownerId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(ownerId));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                                      static_cast<int>(ownerId));
    }

    std::string nativeKind;
    if (readStringProperty(ctx, argv[1], ATLAS_NATIVE_COMPONENT_KIND_PROP,
                           nativeKind) &&
        nativeKind == "audio-player") {
        std::int64_t audioPlayerId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_AUDIO_PLAYER_ID_PROP,
                             audioPlayerId)) {
            return JS_ThrowReferenceError(ctx,
                                          "AudioPlayer wrapper is invalid");
        }

        auto *audioState = findAudioPlayerState(
            *host, static_cast<std::uint64_t>(audioPlayerId));
        if (audioState == nullptr || !audioState->component) {
            return JS_ThrowReferenceError(ctx, "Unknown audio player id");
        }

        if (audioState->attached) {
            return JS_ThrowTypeError(
                ctx, "AudioPlayer is already attached to an object");
        }

        object->addComponent(audioState->component);
        runtime::scripting::registerComponentInstance(
            ctx, *host, audioState->component.get(), static_cast<int>(ownerId),
            "AudioPlayer", argv[1]);
        audioState->attached = true;

        auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
        if (objectIt != host->objectCache.end()) {
            syncObjectWrapper(ctx, *host, *object);
        }

        return JS_DupValue(ctx, argv[1]);
    }

    if (nativeKind == "rigidbody") {
        std::int64_t rigidbodyId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_RIGIDBODY_ID_PROP,
                             rigidbodyId)) {
            return JS_ThrowReferenceError(ctx, "Rigidbody wrapper is invalid");
        }
        auto *rigidbodyState =
            findRigidbodyState(*host, static_cast<std::uint64_t>(rigidbodyId));
        if (rigidbodyState == nullptr || rigidbodyState->component == nullptr) {
            return JS_ThrowReferenceError(ctx, "Unknown rigidbody id");
        }
        if (rigidbodyState->attached) {
            return JS_ThrowTypeError(
                ctx, "Rigidbody is already attached to an object");
        }

        applyRigidbody(ctx, argv[1], *rigidbodyState->component);
        object->addComponent(rigidbodyState->ownedComponent);

        std::string componentName = "Rigidbody";
        JSValue ctor = JS_GetPropertyStr(ctx, argv[1], "constructor");
        if (!JS_IsException(ctor) && !JS_IsUndefined(ctor)) {
            readStringProperty(ctx, ctor, "name", componentName);
        }
        JS_FreeValue(ctx, ctor);

        const std::uint64_t componentId =
            runtime::scripting::registerComponentInstance(
                ctx, *host, rigidbodyState->component,
                static_cast<int>(ownerId), componentName, argv[1]);
        if (componentName == "Sensor") {
            host->componentLookup[makeComponentLookupKey(
                static_cast<int>(ownerId), "Rigidbody")] = componentId;
        }
        rigidbodyState->attached = true;

        auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
        if (objectIt != host->objectCache.end()) {
            syncObjectWrapper(ctx, *host, *object);
        }

        return JS_DupValue(ctx, argv[1]);
    }

    if (nativeKind == "vehicle") {
        std::int64_t vehicleId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_VEHICLE_ID_PROP, vehicleId)) {
            return JS_ThrowReferenceError(ctx, "Vehicle wrapper is invalid");
        }
        auto *vehicleState =
            findVehicleState(*host, static_cast<std::uint64_t>(vehicleId));
        if (vehicleState == nullptr || vehicleState->component == nullptr) {
            return JS_ThrowReferenceError(ctx, "Unknown vehicle id");
        }
        if (vehicleState->attached) {
            return JS_ThrowTypeError(
                ctx, "Vehicle is already attached to an object");
        }

        applyVehicle(ctx, argv[1], *vehicleState->component);
        object->addComponent(vehicleState->ownedComponent);
        runtime::scripting::registerComponentInstance(
            ctx, *host, vehicleState->ownedComponent.get(),
            static_cast<int>(ownerId), "Vehicle", argv[1]);
        vehicleState->attached = true;

        auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
        if (objectIt != host->objectCache.end()) {
            syncObjectWrapper(ctx, *host, *object);
        }

        return JS_DupValue(ctx, argv[1]);
    }

    if (nativeKind == "fixed-joint") {
        std::int64_t jointId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_FIXED_JOINT_ID_PROP,
                             jointId)) {
            return JS_ThrowReferenceError(ctx, "FixedJoint wrapper is invalid");
        }
        auto *jointState =
            findFixedJointState(*host, static_cast<std::uint64_t>(jointId));
        if (jointState == nullptr || jointState->component == nullptr) {
            return JS_ThrowReferenceError(ctx, "Unknown fixed joint id");
        }
        if (jointState->attached) {
            return JS_ThrowTypeError(
                ctx, "FixedJoint is already attached to an object");
        }

        applyFixedJoint(ctx, *host, argv[1], *jointState->component);
        object->addComponent(jointState->ownedComponent);
        const std::uint64_t componentId =
            runtime::scripting::registerComponentInstance(
                ctx, *host, jointState->ownedComponent.get(),
                static_cast<int>(ownerId), "FixedJoint", argv[1]);
        host->componentLookup[makeComponentLookupKey(static_cast<int>(ownerId),
                                                     "Joint")] = componentId;
        jointState->attached = true;
        return JS_DupValue(ctx, argv[1]);
    }

    if (nativeKind == "hinge-joint") {
        std::int64_t jointId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_HINGE_JOINT_ID_PROP,
                             jointId)) {
            return JS_ThrowReferenceError(ctx, "HingeJoint wrapper is invalid");
        }
        auto *jointState =
            findHingeJointState(*host, static_cast<std::uint64_t>(jointId));
        if (jointState == nullptr || jointState->component == nullptr) {
            return JS_ThrowReferenceError(ctx, "Unknown hinge joint id");
        }
        if (jointState->attached) {
            return JS_ThrowTypeError(
                ctx, "HingeJoint is already attached to an object");
        }

        applyHingeJoint(ctx, *host, argv[1], *jointState->component);
        object->addComponent(jointState->ownedComponent);
        const std::uint64_t componentId =
            runtime::scripting::registerComponentInstance(
                ctx, *host, jointState->ownedComponent.get(),
                static_cast<int>(ownerId), "HingeJoint", argv[1]);
        host->componentLookup[makeComponentLookupKey(static_cast<int>(ownerId),
                                                     "Joint")] = componentId;
        jointState->attached = true;
        return JS_DupValue(ctx, argv[1]);
    }

    if (nativeKind == "spring-joint") {
        std::int64_t jointId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_SPRING_JOINT_ID_PROP,
                             jointId)) {
            return JS_ThrowReferenceError(ctx,
                                          "SpringJoint wrapper is invalid");
        }
        auto *jointState =
            findSpringJointState(*host, static_cast<std::uint64_t>(jointId));
        if (jointState == nullptr || jointState->component == nullptr) {
            return JS_ThrowReferenceError(ctx, "Unknown spring joint id");
        }
        if (jointState->attached) {
            return JS_ThrowTypeError(
                ctx, "SpringJoint is already attached to an object");
        }

        applySpringJoint(ctx, *host, argv[1], *jointState->component);
        object->addComponent(jointState->ownedComponent);
        const std::uint64_t componentId =
            runtime::scripting::registerComponentInstance(
                ctx, *host, jointState->ownedComponent.get(),
                static_cast<int>(ownerId), "SpringJoint", argv[1]);
        host->componentLookup[makeComponentLookupKey(static_cast<int>(ownerId),
                                                     "Joint")] = componentId;
        jointState->attached = true;
        return JS_DupValue(ctx, argv[1]);
    }

    std::string componentName = "Component";
    JSValue ctor = JS_GetPropertyStr(ctx, argv[1], "constructor");
    if (!JS_IsException(ctor) && !JS_IsUndefined(ctor)) {
        readStringProperty(ctx, ctor, "name", componentName);
    }
    JS_FreeValue(ctx, ctor);

    auto component = std::make_shared<HostedScriptComponent>(ctx, host, argv[1],
                                                             componentName);
    object->addComponent(component);

    auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
    if (objectIt != host->objectCache.end()) {
        syncObjectWrapper(ctx, *host, *object);
    }

    return JS_DupValue(ctx, argv[1]);
}

std::shared_ptr<CoreObject> ownCoreObject(ScriptHost &host,
                                          std::shared_ptr<CoreObject> object,
                                          bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(object);
    }
    host.objectStates[object->getId()] = {.object = object.get(),
                                          .attachedToWindow = attachedToWindow,
                                          .textureIds = {}};
    return object;
}

std::shared_ptr<Terrain> ownTerrain(ScriptHost &host,
                                    std::shared_ptr<Terrain> terrain,
                                    bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(terrain);
    }
    host.objectStates[terrain->getId()] = {.object = terrain.get(),
                                           .attachedToWindow = attachedToWindow,
                                           .textureIds = {}};
    return terrain;
}

std::shared_ptr<Model> ownModel(ScriptHost &host, std::shared_ptr<Model> model,
                                bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(model);
    }
    host.objectStates[model->getId()] = {.object = model.get(),
                                         .attachedToWindow = attachedToWindow,
                                         .textureIds = {}};
    return model;
}

std::shared_ptr<Fluid> ownFluid(ScriptHost &host, std::shared_ptr<Fluid> fluid,
                                bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(fluid);
    }
    host.objectStates[fluid->getId()] = {.object = fluid.get(),
                                         .attachedToWindow = attachedToWindow,
                                         .textureIds = {}};
    return fluid;
}

std::shared_ptr<ParticleEmitter>
ownParticleEmitter(ScriptHost &host, std::shared_ptr<ParticleEmitter> emitter,
                   bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(emitter);
    }
    host.objectStates[emitter->getId()] = {.object = emitter.get(),
                                           .attachedToWindow = attachedToWindow,
                                           .textureIds = {}};
    return emitter;
}

template <typename TObject>
std::shared_ptr<TObject> ownUIObject(ScriptHost &host,
                                     std::shared_ptr<TObject> object,
                                     bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(object);
    }
    host.objectStates[object->getId()] = {.object = object.get(),
                                          .attachedToWindow = attachedToWindow,
                                          .textureIds = {}};
    return object;
}

void bindTextFieldCallback(JSContext *ctx, ScriptHost *host, TextField &field) {
    field.setOnChange([ctx, host, &field](const TextFieldChangeEvent &event) {
        if (host == nullptr) {
            return;
        }
        JSValue payload = JS_NewObject(ctx);
        setProperty(ctx, payload, "text",
                    JS_NewString(ctx, event.text.c_str()));
        setProperty(ctx, payload, "cursorPosition",
                    JS_NewInt64(ctx, static_cast<int64_t>(event.cursorIndex)));
        setProperty(ctx, payload, "focused", JS_NewBool(ctx, event.focused));
        JSValue args[] = {payload};
        callGraphiteCallback(ctx, *host, field, GRAPHITE_ON_CHANGE_PROP, 1,
                             args);
    });
}

void bindButtonCallback(JSContext *ctx, ScriptHost *host, Button &button) {
    button.setOnClick([ctx, host, &button](const ButtonClickEvent &event) {
        if (host == nullptr) {
            return;
        }
        JSValue payload = JS_NewObject(ctx);
        setProperty(ctx, payload, "label",
                    JS_NewString(ctx, event.label.c_str()));
        JSValue args[] = {payload};
        callGraphiteCallback(ctx, *host, button, GRAPHITE_ON_CLICK_PROP, 1,
                             args);
    });
}

void bindCheckboxCallback(JSContext *ctx, ScriptHost *host,
                          Checkbox &checkbox) {
    checkbox.setOnToggle([ctx, host,
                          &checkbox](const CheckboxToggleEvent &event) {
        if (host == nullptr) {
            return;
        }
        JSValue payload = JS_NewObject(ctx);
        setProperty(ctx, payload, "label",
                    JS_NewString(ctx, event.label.c_str()));
        setProperty(ctx, payload, "checked", JS_NewBool(ctx, event.checked));
        JSValue args[] = {payload};
        callGraphiteCallback(ctx, *host, checkbox, GRAPHITE_ON_TOGGLE_PROP, 1,
                             args);
    });
}

JSValue jsGraphiteGetUISize(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected UI object");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *uiObject =
        object != nullptr ? dynamic_cast<UIObject *>(object) : nullptr;
    if (uiObject == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected UI object");
    }
    return makeSize2d(ctx, *host, uiObject->getSize());
}

JSValue jsGraphiteGetUIScreenPosition(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected UI object");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *uiObject =
        object != nullptr ? dynamic_cast<UIObject *>(object) : nullptr;
    if (uiObject == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected UI object");
    }
    return makePosition2d(ctx, *host, uiObject->getScreenPosition());
}

JSValue jsGraphiteSetUIScreenPosition(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected UI object and position");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *uiObject =
        object != nullptr ? dynamic_cast<UIObject *>(object) : nullptr;
    if (uiObject == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected UI object");
    }
    Position2d position;
    if (!parsePosition2d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position2d");
    }
    uiObject->setScreenPosition(position);
    return syncObjectWrapper(ctx, *host, *uiObject);
}

JSValue jsGraphiteSetUIObjectStyle(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected UI object and style");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    graphite::UIStyle style;
    if (!parseUIStyle(ctx, *host, argv[1], style)) {
        return JS_ThrowTypeError(ctx, "Expected UIStyle");
    }

    if (auto *image = dynamic_cast<Image *>(object); image != nullptr) {
        image->setStyle(style);
    } else if (auto *text = dynamic_cast<Text *>(object); text != nullptr) {
        text->setStyle(style);
    } else if (auto *field = dynamic_cast<TextField *>(object);
               field != nullptr) {
        field->setStyle(style);
    } else if (auto *button = dynamic_cast<Button *>(object);
               button != nullptr) {
        button->setStyle(style);
    } else if (auto *checkbox = dynamic_cast<Checkbox *>(object);
               checkbox != nullptr) {
        checkbox->setStyle(style);
    } else if (auto *column = dynamic_cast<Column *>(object);
               column != nullptr) {
        column->setStyle(style);
    } else if (auto *row = dynamic_cast<Row *>(object); row != nullptr) {
        row->setStyle(style);
    } else if (auto *stack = dynamic_cast<Stack *>(object); stack != nullptr) {
        stack->setStyle(style);
    } else {
        return JS_ThrowTypeError(ctx, "Expected styled Graphite object");
    }

    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateImage(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Image>());
    applyImage(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateText(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Text>());
    applyText(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateTextField(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<TextField>());
    bindTextFieldCallback(ctx, host, *object);
    applyTextField(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateButton(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Button>());
    bindButtonCallback(ctx, host, *object);
    applyButton(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateCheckbox(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Checkbox>());
    bindCheckboxCallback(ctx, host, *object);
    applyCheckbox(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateColumn(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Column>());
    applyColumn(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateRow(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Row>());
    applyRow(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateStack(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownUIObject(*host, std::make_shared<Stack>());
    applyStack(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGraphiteCreateFont(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource");
    }
    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    Resource resource;
    if (!parseResource(ctx, argv[0], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource");
    }
    std::string name =
        !resource.name.empty() ? resource.name : resource.path.stem().string();
    Font font = Font::fromResource(name, resource, 48);
    return syncFontWrapper(ctx, *host, registerFontState(*host, font));
}

JSValue jsGraphiteGetFont(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected font name");
    }
    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected font name");
    }
    Font font = Font::getFont(name);
    JS_FreeCString(ctx, name);
    return syncFontWrapper(ctx, *host, registerFontState(*host, font));
}

JSValue jsGraphiteChangeFontSize(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected font and size");
    }
    auto *state = resolveFont(ctx, *host, argv[0]);
    if (state == nullptr || !state->font) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    state->font->changeSize(static_cast<int>(size));
    return syncFontWrapper(
        ctx, *host,
        static_cast<std::uint64_t>(
            [](ScriptHost &h, Font *fontPtr) -> std::uint64_t {
                for (const auto &[id, state] : h.fonts) {
                    if (state.font.get() == fontPtr) {
                        return id;
                    }
                }
                return 0;
            }(*host, state->font.get())));
}

JSValue jsGraphiteGetTheme(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || !ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }
    return makeThemeValue(ctx, *host, graphite::Theme::current());
}

JSValue jsGraphiteSetTheme(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected theme");
    }
    graphite::Theme theme;
    if (!parseTheme(ctx, *host, argv[0], theme)) {
        return JS_ThrowTypeError(ctx, "Expected theme");
    }
    graphite::Theme::set(theme);
    return makeThemeValue(ctx, *host, graphite::Theme::current());
}

JSValue jsGraphiteResetTheme(JSContext *ctx, JSValueConst, int,
                             JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || !ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }
    graphite::Theme::reset();
    return makeThemeValue(ctx, *host, graphite::Theme::current());
}

JSValue jsGraphiteTextFieldFocus(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *field =
        object != nullptr ? dynamic_cast<TextField *>(object) : nullptr;
    if (field == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    field->focus();
    return syncObjectWrapper(ctx, *host, *field);
}

JSValue jsGraphiteTextFieldBlur(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *field =
        object != nullptr ? dynamic_cast<TextField *>(object) : nullptr;
    if (field == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    field->blur();
    return syncObjectWrapper(ctx, *host, *field);
}

JSValue jsGraphiteTextFieldIsFocused(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *field =
        object != nullptr ? dynamic_cast<TextField *>(object) : nullptr;
    if (field == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    return JS_NewBool(ctx, field->isFocused());
}

JSValue jsGraphiteTextFieldGetCursorIndex(JSContext *ctx, JSValueConst,
                                          int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *field =
        object != nullptr ? dynamic_cast<TextField *>(object) : nullptr;
    if (field == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected text field");
    }
    return JS_NewInt64(ctx, static_cast<int64_t>(field->getCursorIndex()));
}

JSValue jsGraphiteButtonIsHovered(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected button");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *button = object != nullptr ? dynamic_cast<Button *>(object) : nullptr;
    if (button == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected button");
    }
    return JS_NewBool(ctx, button->isHovered());
}

JSValue jsGraphiteCheckboxToggle(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected checkbox");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *checkbox =
        object != nullptr ? dynamic_cast<Checkbox *>(object) : nullptr;
    if (checkbox == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected checkbox");
    }
    checkbox->toggle();
    return syncObjectWrapper(ctx, *host, *checkbox);
}

JSValue jsGraphiteCheckboxIsHovered(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected checkbox");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *checkbox =
        object != nullptr ? dynamic_cast<Checkbox *>(object) : nullptr;
    if (checkbox == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected checkbox");
    }
    return JS_NewBool(ctx, checkbox->isHovered());
}

JSValue jsHydraCreateWorleyNoise(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected frequency and divisions");
    }
    std::int64_t frequency = 0;
    std::int64_t divisions = 0;
    if (!getInt64(ctx, argv[0], frequency) ||
        !getInt64(ctx, argv[1], divisions)) {
        return JS_ThrowTypeError(ctx, "Expected integer parameters");
    }
    return syncWorleyNoiseWrapper(
        ctx, *host,
        registerWorleyNoiseState(*host, std::make_shared<WorleyNoise3D>(
                                            static_cast<int>(frequency),
                                            static_cast<int>(divisions))));
}

JSValue jsHydraWorleyGetValue(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 4) {
        return JS_ThrowTypeError(ctx, "Expected noise and coordinates");
    }
    auto *state = resolveWorleyNoise(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    double x = 0.0, y = 0.0, z = 0.0;
    if (!getDouble(ctx, argv[1], x) || !getDouble(ctx, argv[2], y) ||
        !getDouble(ctx, argv[3], z)) {
        return JS_ThrowTypeError(ctx, "Expected coordinates");
    }
    return JS_NewFloat64(ctx, state->noise->getValue(static_cast<float>(x),
                                                     static_cast<float>(y),
                                                     static_cast<float>(z)));
}

JSValue jsHydraWorleyGet3dTexture(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected noise and size");
    }
    auto *state = resolveWorleyNoise(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    return JS_NewInt64(ctx, static_cast<int64_t>(state->noise->get3dTexture(
                                static_cast<int>(size))));
}

JSValue jsHydraWorleyGetDetailTexture(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected noise and size");
    }
    auto *state = resolveWorleyNoise(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    return JS_NewInt64(ctx, static_cast<int64_t>(state->noise->getDetailTexture(
                                static_cast<int>(size))));
}

JSValue jsHydraWorleyGetAllChannelsTexture(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected noise and size");
    }
    auto *state = resolveWorleyNoise(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    return JS_NewInt64(
        ctx, static_cast<int64_t>(state->noise->get3dTextureAtAllChannels(
                 static_cast<int>(size))));
}

JSValue jsHydraCreateClouds(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected frequency and divisions");
    }
    std::int64_t frequency = 0;
    std::int64_t divisions = 0;
    if (!getInt64(ctx, argv[0], frequency) ||
        !getInt64(ctx, argv[1], divisions)) {
        return JS_ThrowTypeError(ctx, "Expected integer parameters");
    }
    return syncCloudsWrapper(
        ctx, *host,
        registerCloudsState(
            *host, std::make_shared<Clouds>(static_cast<int>(frequency),
                                            static_cast<int>(divisions))));
}

JSValue jsHydraUpdateClouds(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected clouds");
    }
    auto *state = resolveClouds(ctx, *host, argv[0]);
    if (state == nullptr || state->clouds == nullptr) {
        return JS_EXCEPTION;
    }
    applyClouds(ctx, *host, argv[0], *state->clouds);
    return syncCloudsWrapper(ctx, *host, host->cloudIds[state->clouds]);
}

JSValue jsHydraCloudsGetTexture(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected clouds and size");
    }
    auto *state = resolveClouds(ctx, *host, argv[0]);
    if (state == nullptr || state->clouds == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    return JS_NewInt64(ctx, static_cast<int64_t>(state->clouds->getCloudTexture(
                                static_cast<int>(size))));
}

JSValue jsHydraCreateAtmosphere(JSContext *ctx, JSValueConst, int,
                                JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    return syncAtmosphereWrapper(
        ctx, *host,
        registerAtmosphereState(*host, std::make_shared<Atmosphere>()));
}

JSValue jsHydraUpdateAtmosphere(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereEnable(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    state->atmosphere->enable();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereDisable(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    state->atmosphere->disable();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereIsEnabled(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, state->atmosphere->isEnabled());
}

JSValue jsHydraAtmosphereEnableWeather(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    state->atmosphere->enableWeather();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereDisableWeather(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    state->atmosphere->disableWeather();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereGetNormalizedTime(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, state->atmosphere->getNormalizedTime());
}

JSValue jsHydraAtmosphereGetSunAngle(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return makePosition3d(ctx, *host, state->atmosphere->getSunAngle());
}

JSValue jsHydraAtmosphereGetMoonAngle(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return makePosition3d(ctx, *host, state->atmosphere->getMoonAngle());
}

JSValue jsHydraAtmosphereGetLightIntensity(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewFloat64(ctx, state->atmosphere->getLightIntensity());
}

JSValue jsHydraAtmosphereGetLightColor(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return makeColor(ctx, *host, state->atmosphere->getLightColor());
}

JSValue jsHydraAtmosphereGetSkyboxColors(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    JSValue result = JS_NewArray(ctx);
    const auto colors = state->atmosphere->getSkyboxColors();
    for (std::uint32_t i = 0; i < colors.size(); ++i) {
        JS_SetPropertyUint32(ctx, result, i, makeColor(ctx, *host, colors[i]));
    }
    return result;
}

JSValue jsHydraAtmosphereCreateSkyCubemap(JSContext *ctx, JSValueConst,
                                          int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere and size");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t size = 0;
    if (!getInt64(ctx, argv[1], size)) {
        return JS_ThrowTypeError(ctx, "Expected size");
    }
    return syncCubemapWrapper(
        ctx, *host,
        registerCubemapState(*host, state->atmosphere->createSkyCubemap(
                                        static_cast<int>(size))));
}

JSValue jsHydraAtmosphereUpdateSkyCubemap(JSContext *ctx, JSValueConst,
                                          int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere and cubemap");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    auto *cubemapState = resolveCubemap(ctx, *host, argv[1]);
    if (state == nullptr || state->atmosphere == nullptr ||
        cubemapState == nullptr || !cubemapState->cubemap) {
        return JS_EXCEPTION;
    }
    state->atmosphere->updateSkyCubemap(*cubemapState->cubemap);
    std::int64_t cubemapId = 0;
    if (!readIntProperty(ctx, argv[1], ATLAS_CUBEMAP_ID_PROP, cubemapId)) {
        return JS_EXCEPTION;
    }
    return syncCubemapWrapper(ctx, *host,
                              static_cast<std::uint64_t>(cubemapId));
}

JSValue jsHydraAtmosphereCastShadows(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere and resolution");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }
    state->atmosphere->castShadowsFromSunlight(static_cast<int>(resolution));
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereUseGlobalLight(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    applyAtmosphere(ctx, *host, argv[0], *state->atmosphere);
    state->atmosphere->useGlobalLight();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereIsDaytime(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, state->atmosphere->isDaytime());
}

JSValue jsHydraAtmosphereSetTime(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 4) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere and time");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    double hours = 0.0;
    double minutes = 0.0;
    double seconds = 0.0;
    if (!getDouble(ctx, argv[1], hours) || !getDouble(ctx, argv[2], minutes) ||
        !getDouble(ctx, argv[3], seconds)) {
        return JS_ThrowTypeError(ctx, "Expected numeric time values");
    }
    state->atmosphere->timeOfDay =
        static_cast<float>(hours + (minutes / 60.0) + (seconds / 3600.0));
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereAddClouds(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected atmosphere, frequency, divisions");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    std::int64_t frequency = 0;
    std::int64_t divisions = 0;
    if (!getInt64(ctx, argv[1], frequency) ||
        !getInt64(ctx, argv[2], divisions)) {
        return JS_ThrowTypeError(ctx, "Expected integer parameters");
    }
    state->atmosphere->addClouds(static_cast<int>(frequency),
                                 static_cast<int>(divisions));
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraAtmosphereResetRuntimeState(JSContext *ctx, JSValueConst,
                                           int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected atmosphere");
    }
    auto *state = resolveAtmosphere(ctx, *host, argv[0]);
    if (state == nullptr || state->atmosphere == nullptr) {
        return JS_EXCEPTION;
    }
    state->atmosphere->resetRuntimeState();
    return syncAtmosphereWrapper(ctx, *host,
                                 host->atmosphereIds[state->atmosphere]);
}

JSValue jsHydraCreateFluid(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }
    auto object = ownFluid(*host, std::make_shared<Fluid>());
    applyFluid(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsHydraFluidCreate(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected fluid, extent, and color");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *fluid = object != nullptr ? dynamic_cast<Fluid *>(object) : nullptr;
    if (fluid == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected fluid");
    }
    Size2d extent;
    Color color;
    if (!parseSize2d(ctx, argv[1], extent) ||
        !parseColor(ctx, argv[2], color)) {
        return JS_ThrowTypeError(ctx, "Expected extent and color");
    }
    fluid->create(extent, color);
    attachObjectIfReady(*host, *fluid);
    return syncObjectWrapper(ctx, *host, *fluid);
}

JSValue jsHydraFluidSetExtent(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected fluid and extent");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *fluid = object != nullptr ? dynamic_cast<Fluid *>(object) : nullptr;
    if (fluid == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected fluid");
    }
    Size2d extent;
    if (!parseSize2d(ctx, argv[1], extent)) {
        return JS_ThrowTypeError(ctx, "Expected extent");
    }
    fluid->setExtent(extent);
    return syncObjectWrapper(ctx, *host, *fluid);
}

JSValue jsHydraFluidSetWaveVelocity(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected fluid and velocity");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *fluid = object != nullptr ? dynamic_cast<Fluid *>(object) : nullptr;
    if (fluid == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected fluid");
    }
    double velocity = 0.0;
    if (!getDouble(ctx, argv[1], velocity)) {
        return JS_ThrowTypeError(ctx, "Expected velocity");
    }
    fluid->setWaveVelocity(static_cast<float>(velocity));
    return syncObjectWrapper(ctx, *host, *fluid);
}

JSValue jsHydraFluidSetWaterColor(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected fluid and color");
    }
    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    auto *fluid = object != nullptr ? dynamic_cast<Fluid *>(object) : nullptr;
    if (fluid == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected fluid");
    }
    Color color;
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected color");
    }
    fluid->setWaterColor(color);
    return syncObjectWrapper(ctx, *host, *fluid);
}

JSValue jsCreateCoreObject(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto object = ownCoreObject(*host, std::make_shared<CoreObject>());
    applyCoreObject(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreateTerrain(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto terrain = ownTerrain(*host, std::make_shared<Terrain>());
    applyTerrain(ctx, *host, argv[0], *terrain);
    return syncObjectWrapper(ctx, *host, *terrain);
}

JSValue jsCreateModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected model resource path");
    }

    const char *path = JS_ToCString(ctx, argv[0]);
    if (path == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected model resource path");
    }

    std::filesystem::path resourcePath(path);
    JS_FreeCString(ctx, path);

    std::string name = resourcePath.stem().string();
    if (name.empty()) {
        name = resourcePath.filename().string();
    }

    Resource resource = Workspace::get().createResource(resourcePath, name,
                                                        ResourceType::Model);
    auto object = ownModel(*host, std::make_shared<Model>());
    object->fromResource(resource);
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetModelObjects(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected model");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    JSValue result = JS_NewArray(ctx);
    const auto &objects = model->getObjects();
    for (std::uint32_t i = 0; i < objects.size(); ++i) {
        const auto &object = objects[i];
        if (object == nullptr) {
            JS_SetPropertyUint32(ctx, result, i, JS_NULL);
            continue;
        }

        auto &state = host->objectStates[object->getId()];
        state.object = object.get();
        state.attachedToWindow = true;

        JS_SetPropertyUint32(ctx, result, i,
                             syncObjectWrapper(ctx, *host, *object));
    }

    return result;
}

JSValue jsMoveModel(JSContext *ctx, JSValueConst, int argc,
                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and position");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    model->move(position);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelPosition(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and position");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    model->setPosition(position);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelRotation(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and rotation");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    model->setRotation(rotation);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsLookAtModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and target");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    Position3d up(0.0, 1.0, 0.0);
    if (argc > 2) {
        parsePosition3d(ctx, argv[2], up);
    }

    model->lookAt(target, up);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsRotateModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and rotation");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    model->rotate(rotation);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelScale(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and scale");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Scale3d scale;
    if (!parsePosition3d(ctx, argv[1], scale)) {
        return JS_ThrowTypeError(ctx, "Expected Scale3d");
    }

    model->setScale(scale);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsScaleModelBy(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and scale");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Scale3d scale;
    if (!parsePosition3d(ctx, argv[1], scale)) {
        return JS_ThrowTypeError(ctx, "Expected Scale3d");
    }

    Scale3d currentScale = model->getScale();
    currentScale.x *= scale.x;
    currentScale.y *= scale.y;
    currentScale.z *= scale.z;
    model->setScale(currentScale);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsCreateParticleEmitter(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and max particles");
    }

    std::int64_t maxParticles = 0;
    if (!getInt64(ctx, argv[1], maxParticles)) {
        return JS_ThrowTypeError(ctx, "Expected max particles");
    }

    auto emitter =
        ownParticleEmitter(*host, std::make_shared<ParticleEmitter>(
                                      static_cast<unsigned int>(maxParticles)));

    JSValue objectValue = JS_DupValue(ctx, argv[0]);
    setProperty(ctx, objectValue, ATLAS_PARTICLE_EMITTER_ID_PROP,
                JS_NewInt32(ctx, emitter->getId()));
    setProperty(ctx, objectValue, "id", JS_NewInt32(ctx, emitter->getId()));
    setProperty(ctx, objectValue, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host->generation)));
    JS_FreeValue(ctx, objectValue);

    attachObjectIfReady(*host, *emitter);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsAttachParticleEmitterTexture(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and texture");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    auto *textureState = resolveTexture(ctx, *host, argv[1]);
    if (textureState == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->attachTexture(*textureState->texture);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterColor(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and color");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Color color;
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected Color");
    }

    emitter->setColor(color);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterUseTexture(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and enabled flag");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    if (JS_ToBool(ctx, argv[1]) == 1) {
        emitter->enableTexture();
    } else {
        emitter->disableTexture();
    }

    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterPosition(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and position");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->setPosition(position);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsMoveParticleEmitter(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and position");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->move(position);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsGetParticleEmitterPosition(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    return makePosition3d(ctx, *host, emitter->getPosition());
}

JSValue jsSetParticleEmitterEmissionType(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and emission type");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        return JS_ThrowTypeError(ctx, "Expected emission type");
    }

    emitter->setEmissionType(type == 1 ? ParticleEmissionType::Ambient
                                       : ParticleEmissionType::Fountain);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterDirection(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and direction");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d direction;
    if (!parsePosition3d(ctx, argv[1], direction)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->setDirection(direction);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSpawnRadius(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and spawn radius");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    double radius = 0.0;
    if (!getDouble(ctx, argv[1], radius)) {
        return JS_ThrowTypeError(ctx, "Expected spawn radius");
    }

    emitter->setSpawnRadius(static_cast<float>(radius));
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSpawnRate(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and spawn rate");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    double rate = 0.0;
    if (!getDouble(ctx, argv[1], rate)) {
        return JS_ThrowTypeError(ctx, "Expected spawn rate");
    }

    emitter->setSpawnRate(static_cast<float>(rate));
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSettings(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and settings");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    ParticleSettings settings = emitter->settings;
    parseParticleSettings(ctx, argv[1], settings);
    emitter->setParticleSettings(settings);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitOnce(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->emitOnce();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitContinuous(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->emitContinuously();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterStartEmission(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->startEmission();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterStopEmission(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->stopEmission();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitBurst(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and count");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t count = 0;
    if (!getInt64(ctx, argv[1], count)) {
        return JS_ThrowTypeError(ctx, "Expected burst count");
    }

    emitter->emitBurst(static_cast<int>(count));
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsUpdateObject(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    if (auto *image = dynamic_cast<Image *>(object); image != nullptr) {
        applyImage(ctx, *host, argv[0], *image);
    } else if (auto *text = dynamic_cast<Text *>(object); text != nullptr) {
        applyText(ctx, *host, argv[0], *text);
    } else if (auto *field = dynamic_cast<TextField *>(object);
               field != nullptr) {
        applyTextField(ctx, *host, argv[0], *field);
    } else if (auto *button = dynamic_cast<Button *>(object);
               button != nullptr) {
        applyButton(ctx, *host, argv[0], *button);
    } else if (auto *checkbox = dynamic_cast<Checkbox *>(object);
               checkbox != nullptr) {
        applyCheckbox(ctx, *host, argv[0], *checkbox);
    } else if (auto *column = dynamic_cast<Column *>(object);
               column != nullptr) {
        applyColumn(ctx, *host, argv[0], *column);
    } else if (auto *row = dynamic_cast<Row *>(object); row != nullptr) {
        applyRow(ctx, *host, argv[0], *row);
    } else if (auto *stack = dynamic_cast<Stack *>(object); stack != nullptr) {
        applyStack(ctx, *host, argv[0], *stack);
    } else if (auto *fluid = dynamic_cast<Fluid *>(object); fluid != nullptr) {
        applyFluid(ctx, *host, argv[0], *fluid);
        attachObjectIfReady(*host, *fluid);
    } else if (auto *core = dynamic_cast<CoreObject *>(object);
               core != nullptr) {
        if (!applyCoreObject(ctx, *host, argv[0], *core)) {
            return JS_EXCEPTION;
        }
    } else if (auto *terrain = dynamic_cast<Terrain *>(object);
               terrain != nullptr) {
        if (!applyTerrain(ctx, *host, argv[0], *terrain)) {
            return JS_EXCEPTION;
        }
        attachObjectIfReady(*host, *terrain);
    } else {
        applyBaseObject(ctx, *host, argv[0], *object);
        attachObjectIfReady(*host, *object);
    }

    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitiveBox(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size3d size;
    if (!parsePosition3d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size3d");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createBox(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitivePlane(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size2d size;
    if (!parseSize2d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size2d");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createPlane(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitivePyramid(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size3d size;
    if (!parsePosition3d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size3d");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createPyramid(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitiveSphere(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected radius, sectorCount, and stackCount");
    }

    double radius = 0.0;
    std::int64_t sectorCount = 0;
    std::int64_t stackCount = 0;
    if (!getDouble(ctx, argv[0], radius) ||
        !getInt64(ctx, argv[1], sectorCount) ||
        !getInt64(ctx, argv[2], stackCount)) {
        return JS_ThrowTypeError(
            ctx, "Expected radius, sectorCount, and stackCount");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createSphere(
                                 radius, static_cast<unsigned int>(sectorCount),
                                 static_cast<unsigned int>(stackCount))));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCloneCoreObject(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected CoreObject");
    }

    CoreObject *object = resolveCoreObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    auto clone =
        ownCoreObject(*host, std::make_shared<CoreObject>(object->clone()));
    attachObjectIfReady(*host, *clone);
    return syncObjectWrapper(ctx, *host, *clone);
}

JSValue jsMakeEmissive(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected id, color, intensity");
    }

    std::int64_t objectId = 0;
    double intensity = 0.0;
    if (!getInt64(ctx, argv[0], objectId) ||
        !getDouble(ctx, argv[2], intensity)) {
        return JS_ThrowTypeError(ctx, "Expected id, color, intensity");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    Color color = Color::white();
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected Color");
    }

    object->makeEmissive(host->context->scene.get(), color,
                         static_cast<float>(intensity));
    attachObjectIfReady(*host, *object);
    return JS_UNDEFINED;
}

JSValue jsAttachTexture(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object and texture");
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    auto *textureState = resolveTexture(ctx, *host, argv[1]);
    if (textureState == nullptr) {
        return JS_EXCEPTION;
    }

    object->attachTexture(*textureState->texture);
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsAuroraPerlinNoise(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    double x = 0.0;
    double y = 0.0;
    std::int64_t seed = 0;
    if (argc < 3 || !getInt64(ctx, argv[0], seed) ||
        !getDouble(ctx, argv[1], x) || !getDouble(ctx, argv[2], y)) {
        return JS_ThrowTypeError(ctx, "Expected seed, x, and y");
    }
    PerlinNoise noise(static_cast<unsigned int>(seed));
    return JS_NewFloat64(
        ctx, noise.noise(static_cast<float>(x), static_cast<float>(y)));
}

JSValue jsAuroraSimplexNoise(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    double x = 0.0;
    double y = 0.0;
    if (argc < 2 || !getDouble(ctx, argv[0], x) ||
        !getDouble(ctx, argv[1], y)) {
        return JS_ThrowTypeError(ctx, "Expected x and y");
    }
    return JS_NewFloat64(
        ctx, SimplexNoise::noise(static_cast<float>(x), static_cast<float>(y)));
}

JSValue jsAuroraWorleyNoise(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    std::int64_t numPoints = 0;
    std::int64_t seed = 0;
    double x = 0.0;
    double y = 0.0;
    if (argc < 4 || !getInt64(ctx, argv[0], numPoints) ||
        !getInt64(ctx, argv[1], seed) || !getDouble(ctx, argv[2], x) ||
        !getDouble(ctx, argv[3], y)) {
        return JS_ThrowTypeError(ctx, "Expected numPoints, seed, x, and y");
    }
    WorleyNoise noise(static_cast<int>(numPoints),
                      static_cast<unsigned int>(seed));
    return JS_NewFloat64(
        ctx, noise.noise(static_cast<float>(x), static_cast<float>(y)));
}

JSValue jsAuroraFractalNoise(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    std::int64_t octaves = 0;
    double persistence = 0.0;
    double x = 0.0;
    double y = 0.0;
    if (argc < 4 || !getInt64(ctx, argv[0], octaves) ||
        !getDouble(ctx, argv[1], persistence) || !getDouble(ctx, argv[2], x) ||
        !getDouble(ctx, argv[3], y)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected octaves, persistence, x, and y");
    }
    FractalNoise noise(static_cast<int>(octaves),
                       static_cast<float>(persistence));
    return JS_NewFloat64(
        ctx, noise.noise(static_cast<float>(x), static_cast<float>(y)));
}

JSValue jsShowObject(JSContext *ctx, JSValueConst, int argc,
                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object != nullptr) {
        object->show();
    }
    return JS_UNDEFINED;
}

JSValue jsHideObject(JSContext *ctx, JSValueConst, int argc,
                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object != nullptr) {
        object->hide();
    }
    return JS_UNDEFINED;
}

JSValue jsEnableDeferred(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }
    object->useDeferredRendering = true;
    return JS_UNDEFINED;
}

JSValue jsDisableDeferred(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }
    object->disableDeferredRendering();
    return JS_UNDEFINED;
}

JSValue jsCreateInstance(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    object->createInstance();
    return syncInstanceWrapper(
        ctx, *host, *object,
        static_cast<std::uint32_t>(object->instances.size() - 1));
}

JSValue jsCommitInstance(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected instance");
    }

    Instance *instance = resolveInstanceArg(ctx, *host, argv[0]);
    if (instance == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position = instance->position;
    Rotation3d rotation = instance->rotation;
    Scale3d scale = instance->scale;

    JSValue value = JS_GetPropertyStr(ctx, argv[0], "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, argv[0], "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, argv[0], "scale");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, scale);
    }
    JS_FreeValue(ctx, value);

    instance->setPosition(position);
    instance->setRotation(rotation);
    instance->setScale(scale);

    return JS_DupValue(ctx, argv[0]);
}

JSValue jsLookAtObject(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object and target");
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    Position3d up(0.0, 1.0, 0.0);
    if (argc > 2) {
        parsePosition3d(ctx, argv[2], up);
    }

    object->lookAt(target, up);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsSetRotationQuaternion(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object id and quaternion");
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    glm::quat quat;
    if (!parseQuaternion(ctx, argv[1], quat)) {
        return JS_ThrowTypeError(ctx, "Expected Quaternion");
    }

    object->setRotationQuat(quat);
    return syncObjectWrapper(ctx, *host, *object);
}

} // namespace

void runtime::scripting::dumpExecution(JSContext *ctx) {
    JSValue exceptionVal = JS_GetException(ctx);
    const char *exceptionStr = JS_ToCString(ctx, exceptionVal);
    if (exceptionStr) {
        std::cout << BOLD << RED << "Script execution failed: " << RESET
                  << YELLOW << exceptionStr << RESET << std::endl;
        JS_FreeCString(ctx, exceptionStr);
    }

    JSValue stack = JS_GetPropertyStr(ctx, exceptionVal, "stack");
    if (!JS_IsUndefined(stack)) {
        const char *stackStr = JS_ToCString(ctx, stack);
        if (stackStr) {
            std::cerr << stackStr << "\n";
            JS_FreeCString(ctx, stackStr);
        }
    }

    JS_FreeValue(ctx, stack);
    JS_FreeValue(ctx, exceptionVal);
}

bool runtime::scripting::checkNotException(JSContext *ctx, JSValueConst value,
                                           const char *what) {
    if (JS_IsException(value)) {
        std::cout << BOLD << RED << "Error during " << what << ": " << RESET;
        dumpExecution(ctx);
        return false;
    }
    return true;
}

void runtime::scripting::clearSceneBindings(JSContext *ctx, ScriptHost &host) {
    if (!JS_IsUndefined(host.windowValue)) {
        JS_FreeValue(ctx, host.windowValue);
        host.windowValue = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.cameraValue)) {
        JS_FreeValue(ctx, host.cameraValue);
        host.cameraValue = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.sceneValue)) {
        JS_FreeValue(ctx, host.sceneValue);
        host.sceneValue = JS_UNDEFINED;
    }

    for (auto &[_, value] : host.objectCache) {
        JS_FreeValue(ctx, value);
    }
    host.objectCache.clear();

    for (auto &[_, value] : host.instanceCache) {
        JS_FreeValue(ctx, value);
    }
    host.instanceCache.clear();

    for (auto &[_, state] : host.componentStates) {
        JS_FreeValue(ctx, state.value);
    }
    host.componentStates.clear();

    for (auto &[_, state] : host.audioPlayers) {
        JS_FreeValue(ctx, state.value);
    }
    host.audioPlayers.clear();

    for (auto &[_, state] : host.rigidbodies) {
        JS_FreeValue(ctx, state.value);
    }
    host.rigidbodies.clear();

    for (auto &[_, state] : host.vehicles) {
        JS_FreeValue(ctx, state.value);
    }
    host.vehicles.clear();

    for (auto &[_, state] : host.fixedJoints) {
        JS_FreeValue(ctx, state.value);
    }
    host.fixedJoints.clear();

    for (auto &[_, state] : host.hingeJoints) {
        JS_FreeValue(ctx, state.value);
    }
    host.hingeJoints.clear();

    for (auto &[_, state] : host.springJoints) {
        JS_FreeValue(ctx, state.value);
    }
    host.springJoints.clear();

    for (auto &[_, state] : host.textures) {
        JS_FreeValue(ctx, state.value);
    }
    host.textures.clear();

    for (auto &[_, state] : host.cubemaps) {
        JS_FreeValue(ctx, state.value);
    }
    host.cubemaps.clear();

    for (auto &[_, state] : host.skyboxes) {
        JS_FreeValue(ctx, state.value);
    }
    host.skyboxes.clear();

    for (auto &[_, state] : host.renderTargets) {
        JS_FreeValue(ctx, state.value);
    }
    host.renderTargets.clear();

    for (auto &[_, state] : host.pointLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.pointLights.clear();

    for (auto &[_, state] : host.directionalLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.directionalLights.clear();

    for (auto &[_, state] : host.spotLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.spotLights.clear();

    for (auto &[_, state] : host.areaLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.areaLights.clear();
    for (auto &[_, state] : host.audioData) {
        JS_FreeValue(ctx, state.value);
    }
    host.audioData.clear();
    host.audioDataIds.clear();
    for (auto &[_, state] : host.audioSources) {
        JS_FreeValue(ctx, state.value);
    }
    host.audioSources.clear();
    host.audioSourceIds.clear();
    for (auto &[_, state] : host.reverbs) {
        JS_FreeValue(ctx, state.value);
    }
    host.reverbs.clear();
    host.reverbIds.clear();
    for (auto &[_, state] : host.echoes) {
        JS_FreeValue(ctx, state.value);
    }
    host.echoes.clear();
    host.echoIds.clear();
    for (auto &[_, state] : host.distortions) {
        JS_FreeValue(ctx, state.value);
    }
    host.distortions.clear();
    host.distortionIds.clear();
    for (auto &[_, state] : host.fonts) {
        JS_FreeValue(ctx, state.value);
    }
    host.fonts.clear();
    for (auto &[_, state] : host.worleyNoise) {
        JS_FreeValue(ctx, state.value);
    }
    host.worleyNoise.clear();
    for (auto &[_, state] : host.clouds) {
        JS_FreeValue(ctx, state.value);
    }
    host.clouds.clear();
    host.cloudIds.clear();
    for (auto &[_, state] : host.atmospheres) {
        JS_FreeValue(ctx, state.value);
        if (!JS_IsUndefined(state.weatherDelegate)) {
            JS_FreeValue(ctx, state.weatherDelegate);
        }
    }
    host.atmospheres.clear();
    host.atmosphereIds.clear();

    if (!JS_IsUndefined(host.atlasParticleNamespace)) {
        JS_FreeValue(ctx, host.atlasParticleNamespace);
        host.atlasParticleNamespace = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.atlasBezelNamespace)) {
        JS_FreeValue(ctx, host.atlasBezelNamespace);
        host.atlasBezelNamespace = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.auroraNamespace)) {
        JS_FreeValue(ctx, host.auroraNamespace);
        host.auroraNamespace = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.finewaveNamespace)) {
        JS_FreeValue(ctx, host.finewaveNamespace);
        host.finewaveNamespace = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.graphiteNamespace)) {
        JS_FreeValue(ctx, host.graphiteNamespace);
        host.graphiteNamespace = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.hydraNamespace)) {
        JS_FreeValue(ctx, host.hydraNamespace);
        host.hydraNamespace = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.audioEngineValue)) {
        JS_FreeValue(ctx, host.audioEngineValue);
        host.audioEngineValue = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.particleEmitterPrototype)) {
        JS_FreeValue(ctx, host.particleEmitterPrototype);
        host.particleEmitterPrototype = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.terrainPrototype)) {
        JS_FreeValue(ctx, host.terrainPrototype);
        host.terrainPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.biomePrototype)) {
        JS_FreeValue(ctx, host.biomePrototype);
        host.biomePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.terrainGeneratorPrototype)) {
        JS_FreeValue(ctx, host.terrainGeneratorPrototype);
        host.terrainGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.hillGeneratorPrototype)) {
        JS_FreeValue(ctx, host.hillGeneratorPrototype);
        host.hillGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.mountainGeneratorPrototype)) {
        JS_FreeValue(ctx, host.mountainGeneratorPrototype);
        host.mountainGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.plainGeneratorPrototype)) {
        JS_FreeValue(ctx, host.plainGeneratorPrototype);
        host.plainGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.islandGeneratorPrototype)) {
        JS_FreeValue(ctx, host.islandGeneratorPrototype);
        host.islandGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.compoundGeneratorPrototype)) {
        JS_FreeValue(ctx, host.compoundGeneratorPrototype);
        host.compoundGeneratorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.audioEnginePrototype)) {
        JS_FreeValue(ctx, host.audioEnginePrototype);
        host.audioEnginePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.audioDataPrototype)) {
        JS_FreeValue(ctx, host.audioDataPrototype);
        host.audioDataPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.audioSourcePrototype)) {
        JS_FreeValue(ctx, host.audioSourcePrototype);
        host.audioSourcePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.audioEffectPrototype)) {
        JS_FreeValue(ctx, host.audioEffectPrototype);
        host.audioEffectPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.reverbPrototype)) {
        JS_FreeValue(ctx, host.reverbPrototype);
        host.reverbPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.echoPrototype)) {
        JS_FreeValue(ctx, host.echoPrototype);
        host.echoPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.distortionPrototype)) {
        JS_FreeValue(ctx, host.distortionPrototype);
        host.distortionPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.imagePrototype)) {
        JS_FreeValue(ctx, host.imagePrototype);
        host.imagePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.textPrototype)) {
        JS_FreeValue(ctx, host.textPrototype);
        host.textPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.textFieldPrototype)) {
        JS_FreeValue(ctx, host.textFieldPrototype);
        host.textFieldPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.buttonPrototype)) {
        JS_FreeValue(ctx, host.buttonPrototype);
        host.buttonPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.checkboxPrototype)) {
        JS_FreeValue(ctx, host.checkboxPrototype);
        host.checkboxPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.columnPrototype)) {
        JS_FreeValue(ctx, host.columnPrototype);
        host.columnPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.rowPrototype)) {
        JS_FreeValue(ctx, host.rowPrototype);
        host.rowPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.stackPrototype)) {
        JS_FreeValue(ctx, host.stackPrototype);
        host.stackPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.fontPrototype)) {
        JS_FreeValue(ctx, host.fontPrototype);
        host.fontPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.uiStylePrototype)) {
        JS_FreeValue(ctx, host.uiStylePrototype);
        host.uiStylePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.uiStyleVariantPrototype)) {
        JS_FreeValue(ctx, host.uiStyleVariantPrototype);
        host.uiStyleVariantPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.themePrototype)) {
        JS_FreeValue(ctx, host.themePrototype);
        host.themePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.uiObjectPrototype)) {
        JS_FreeValue(ctx, host.uiObjectPrototype);
        host.uiObjectPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.worleyNoisePrototype)) {
        JS_FreeValue(ctx, host.worleyNoisePrototype);
        host.worleyNoisePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.cloudsPrototype)) {
        JS_FreeValue(ctx, host.cloudsPrototype);
        host.cloudsPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.atmospherePrototype)) {
        JS_FreeValue(ctx, host.atmospherePrototype);
        host.atmospherePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.fluidPrototype)) {
        JS_FreeValue(ctx, host.fluidPrototype);
        host.fluidPrototype = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.rigidbodyPrototype)) {
        JS_FreeValue(ctx, host.rigidbodyPrototype);
        host.rigidbodyPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.sensorPrototype)) {
        JS_FreeValue(ctx, host.sensorPrototype);
        host.sensorPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.vehiclePrototype)) {
        JS_FreeValue(ctx, host.vehiclePrototype);
        host.vehiclePrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.fixedJointPrototype)) {
        JS_FreeValue(ctx, host.fixedJointPrototype);
        host.fixedJointPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.hingeJointPrototype)) {
        JS_FreeValue(ctx, host.hingeJointPrototype);
        host.hingeJointPrototype = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.springJointPrototype)) {
        JS_FreeValue(ctx, host.springJointPrototype);
        host.springJointPrototype = JS_UNDEFINED;
    }

    for (JSValue &interactive : host.interactiveValues) {
        JS_FreeValue(ctx, interactive);
    }
    host.interactiveValues.clear();

    host.componentIds.clear();
    host.componentOrder.clear();
    host.componentLookup.clear();
    host.interactiveKeyStates.clear();
    host.interactiveFirstMouse = true;
    host.objectStates.clear();
    host.nextComponentId = 1;
    host.nextAudioPlayerId = 1;
    host.nextAudioDataId = 1;
    host.nextAudioSourceId = 1;
    host.nextReverbId = 1;
    host.nextEchoId = 1;
    host.nextDistortionId = 1;
    host.nextFontId = 1;
    host.nextRigidbodyId = 1;
    host.nextVehicleId = 1;
    host.nextFixedJointId = 1;
    host.nextHingeJointId = 1;
    host.nextSpringJointId = 1;
    host.nextTextureId = 1;
    host.nextCubemapId = 1;
    host.nextSkyboxId = 1;
    host.nextRenderTargetId = 1;
    host.nextPointLightId = 1;
    host.nextDirectionalLightId = 1;
    host.nextSpotLightId = 1;
    host.nextAreaLightId = 1;
    host.nextWorleyNoiseId = 1;
    host.nextCloudsId = 1;
    host.nextAtmosphereId = 1;
    host.generation += 1;
}

void runtime::scripting::dispatchInteractiveFrame(JSContext *ctx,
                                                  ScriptHost &host,
                                                  Window &window,
                                                  float deltaTime) {
    if (host.interactiveValues.empty()) {
        host.interactiveKeyStates.clear();
        return;
    }

    auto dispatch = [&](const char *methodName, int argc, JSValue *args) {
        JSValueConst *constArgs = args;
        for (JSValue &interactive : host.interactiveValues) {
            callObjectMethod(ctx, interactive, methodName, argc, constArgs);
        }
        for (int i = 0; i < argc; ++i) {
            JS_FreeValue(ctx, args[i]);
        }
    };

    for (const auto &[_, key] : ATLAS_KEY_ENTRIES) {
        const int keyCode = static_cast<int>(key);
        const bool active = window.isKeyActive(key);
        const bool previous = host.interactiveKeyStates[keyCode];
        if (active && !previous) {
            JSValue args[] = {JS_NewInt32(ctx, keyCode),
                              JS_NewFloat64(ctx, deltaTime)};
            dispatch("onKeyPress", 2, args);
        } else if (!active && previous) {
            JSValue args[] = {JS_NewInt32(ctx, keyCode),
                              JS_NewFloat64(ctx, deltaTime)};
            dispatch("onKeyRelease", 2, args);
        }
        host.interactiveKeyStates[keyCode] = active;
    }

    for (MouseButton button : ATLAS_MOUSE_BUTTONS) {
        if (!window.isMouseButtonPressed(button)) {
            continue;
        }
        JSValue args[] = {JS_NewInt32(ctx, static_cast<int>(button)),
                          JS_NewFloat64(ctx, deltaTime)};
        dispatch("onMouseButtonPress", 2, args);
    }

    JSValue frameArgs[] = {JS_NewFloat64(ctx, deltaTime)};
    dispatch("onEachFrame", 1, frameArgs);
}

void runtime::scripting::dispatchInteractiveMouseMove(JSContext *ctx,
                                                      ScriptHost &host,
                                                      Window &window,
                                                      const MousePacket &packet,
                                                      float deltaTime) {
    if (host.interactiveValues.empty()) {
        return;
    }

    JSValue args[] = {makeMousePacketValue(ctx, packet),
                      JS_NewFloat64(ctx, deltaTime)};
    JSValueConst *constArgs = args;
    for (JSValue &interactive : host.interactiveValues) {
        callObjectMethod(ctx, interactive, "onMouseMove", 2, constArgs);
    }
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);

    (void)window;
    host.interactiveFirstMouse = false;
}

void runtime::scripting::dispatchInteractiveMouseScroll(
    JSContext *ctx, ScriptHost &host, const MouseScrollPacket &packet,
    float deltaTime) {
    if (host.interactiveValues.empty()) {
        return;
    }

    JSValue args[] = {makeMouseScrollPacketValue(ctx, packet),
                      JS_NewFloat64(ctx, deltaTime)};
    JSValueConst *constArgs = args;
    for (JSValue &interactive : host.interactiveValues) {
        callObjectMethod(ctx, interactive, "onMouseScroll", 2, constArgs);
    }
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
}

std::uint64_t runtime::scripting::registerComponentInstance(
    JSContext *ctx, ScriptHost &host, Component *component, int ownerId,
    const std::string &name, JSValueConst value) {
    if (component == nullptr) {
        return 0;
    }

    std::uint64_t componentId = 0;
    auto existing = host.componentIds.find(component);
    if (existing != host.componentIds.end()) {
        componentId = existing->second;
        auto stateIt = host.componentStates.find(componentId);
        if (stateIt != host.componentStates.end()) {
            JS_FreeValue(ctx, stateIt->second.value);
        }
    } else {
        componentId = host.nextComponentId++;
        host.componentIds[component] = componentId;
        host.componentOrder[ownerId].push_back(componentId);
    }

    host.componentLookup[makeComponentLookupKey(ownerId, name)] = componentId;
    host.componentStates[componentId] = {.component = component,
                                         .ownerId = ownerId,
                                         .name = name,
                                         .value = JS_DupValue(ctx, value)};

    JSValue objectValue = JS_DupValue(ctx, value);
    setProperty(ctx, objectValue, ATLAS_COMPONENT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(componentId)));
    setProperty(ctx, objectValue, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, objectValue, "parentId", JS_NewInt32(ctx, ownerId));
    JS_FreeValue(ctx, objectValue);

    return componentId;
}

void runtime::scripting::registerNativeRigidbody(
    JSContext *ctx, ScriptHost &host, int ownerId,
    const std::shared_ptr<Rigidbody> &component) {
    if (ctx == nullptr || !component) {
        return;
    }

    const std::uint64_t rigidbodyId =
        registerRigidbodyState(host, component, ctx, JS_UNDEFINED, true);
    JSValue wrapper = syncRigidbodyWrapper(ctx, host, rigidbodyId);
    const std::string name = component->isSensor ? "Sensor" : "Rigidbody";
    const std::uint64_t componentId = registerComponentInstance(
        ctx, host, component.get(), ownerId, name, wrapper);
    if (component->isSensor) {
        host.componentLookup[makeComponentLookupKey(ownerId, "Rigidbody")] =
            componentId;
    }
    JS_FreeValue(ctx, wrapper);
}

void runtime::scripting::registerNativeVehicle(
    JSContext *ctx, ScriptHost &host, int ownerId,
    const std::shared_ptr<Vehicle> &component) {
    if (ctx == nullptr || !component) {
        return;
    }

    const std::uint64_t vehicleId = registerVehicleState(
        host, component, component.get(), ctx, JS_UNDEFINED, true);
    JSValue wrapper = syncVehicleWrapper(ctx, host, vehicleId);
    registerComponentInstance(ctx, host, component.get(), ownerId, "Vehicle",
                              wrapper);
    JS_FreeValue(ctx, wrapper);
}

void runtime::scripting::registerNativeFixedJoint(
    JSContext *ctx, ScriptHost &host, int ownerId,
    const std::shared_ptr<FixedJoint> &component) {
    if (ctx == nullptr || !component) {
        return;
    }

    const std::uint64_t jointId = registerFixedJointState(
        host, component, component.get(), ctx, JS_UNDEFINED, true);
    JSValue wrapper = syncFixedJointWrapper(ctx, host, jointId);
    const std::uint64_t componentId = registerComponentInstance(
        ctx, host, component.get(), ownerId, "FixedJoint", wrapper);
    host.componentLookup[makeComponentLookupKey(ownerId, "Joint")] =
        componentId;
    JS_FreeValue(ctx, wrapper);
}

void runtime::scripting::registerNativeHingeJoint(
    JSContext *ctx, ScriptHost &host, int ownerId,
    const std::shared_ptr<HingeJoint> &component) {
    if (ctx == nullptr || !component) {
        return;
    }

    const std::uint64_t jointId = registerHingeJointState(
        host, component, component.get(), ctx, JS_UNDEFINED, true);
    JSValue wrapper = syncHingeJointWrapper(ctx, host, jointId);
    const std::uint64_t componentId = registerComponentInstance(
        ctx, host, component.get(), ownerId, "HingeJoint", wrapper);
    host.componentLookup[makeComponentLookupKey(ownerId, "Joint")] =
        componentId;
    JS_FreeValue(ctx, wrapper);
}

void runtime::scripting::registerNativeSpringJoint(
    JSContext *ctx, ScriptHost &host, int ownerId,
    const std::shared_ptr<SpringJoint> &component) {
    if (ctx == nullptr || !component) {
        return;
    }

    const std::uint64_t jointId = registerSpringJointState(
        host, component, component.get(), ctx, JS_UNDEFINED, true);
    JSValue wrapper = syncSpringJointWrapper(ctx, host, jointId);
    const std::uint64_t componentId = registerComponentInstance(
        ctx, host, component.get(), ownerId, "SpringJoint", wrapper);
    host.componentLookup[makeComponentLookupKey(ownerId, "Joint")] =
        componentId;
    JS_FreeValue(ctx, wrapper);
}

void runtime::scripting::installGlobals(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "print",
                      JS_NewCFunction(ctx, jsPrint, "print", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetScene",
                      JS_NewCFunction(ctx, jsGetScene, "__atlasGetScene", 0));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindow",
                      JS_NewCFunction(ctx, jsGetWindow, "__atlasGetWindow", 0));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneAmbientIntensity",
                      JS_NewCFunction(ctx, jsSetSceneAmbientIntensity,
                                      "__atlasSetSceneAmbientIntensity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneAutomaticAmbient",
                      JS_NewCFunction(ctx, jsSetSceneAutomaticAmbient,
                                      "__atlasSetSceneAutomaticAmbient", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneEnvironment",
                      JS_NewCFunction(ctx, jsSetSceneEnvironment,
                                      "__atlasSetSceneEnvironment", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneAmbientColor",
                      JS_NewCFunction(ctx, jsSetSceneAmbientColor,
                                      "__atlasSetSceneAmbientColor", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetSceneSkybox",
        JS_NewCFunction(ctx, jsSetSceneSkybox, "__atlasSetSceneSkybox", 2));
    JS_SetPropertyStr(ctx, global, "__atlasUseAtmosphereSkybox",
                      JS_NewCFunction(ctx, jsUseAtmosphereSkybox,
                                      "__atlasUseAtmosphereSkybox", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSceneAddDirectionalLight",
                      JS_NewCFunction(ctx, jsSceneAddDirectionalLight,
                                      "__atlasSceneAddDirectionalLight", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSceneAddLight",
        JS_NewCFunction(ctx, jsSceneAddLight, "__atlasSceneAddLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSceneAddSpotLight",
                      JS_NewCFunction(ctx, jsSceneAddSpotLight,
                                      "__atlasSceneAddSpotLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSceneAddAreaLight",
                      JS_NewCFunction(ctx, jsSceneAddAreaLight,
                                      "__atlasSceneAddAreaLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetCamera",
                      JS_NewCFunction(ctx, jsGetCamera, "__atlasGetCamera", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateCamera",
        JS_NewCFunction(ctx, jsUpdateCamera, "__atlasUpdateCamera", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetPositionKeepingOrientation",
                      JS_NewCFunction(ctx, jsSetPositionKeepingOrientation,
                                      "__atlasSetPositionKeepingOrientation",
                                      2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtCamera",
        JS_NewCFunction(ctx, jsLookAtCamera, "__atlasLookAtCamera", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasMoveCameraTo",
        JS_NewCFunction(ctx, jsMoveCameraTo, "__atlasMoveCameraTo", 3));
    JS_SetPropertyStr(ctx, global, "__atlasGetCameraDirection",
                      JS_NewCFunction(ctx, jsGetCameraDirection,
                                      "__atlasGetCameraDirection", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetWindowClearColor",
                      JS_NewCFunction(ctx, jsWindowSetClearColor,
                                      "__atlasSetWindowClearColor", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCloseWindow",
        JS_NewCFunction(ctx, jsWindowClose, "__atlasCloseWindow", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetWindowFullscreen",
                      JS_NewCFunction(ctx, jsWindowSetFullscreen,
                                      "__atlasSetWindowFullscreen", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetWindowFullscreenMonitor",
                      JS_NewCFunction(ctx, jsWindowSetFullscreenMonitor,
                                      "__atlasSetWindowFullscreenMonitor", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetWindowed",
        JS_NewCFunction(ctx, jsWindowSetWindowed, "__atlasSetWindowed", 2));
    JS_SetPropertyStr(ctx, global, "__atlasEnumerateMonitors",
                      JS_NewCFunction(ctx, jsWindowEnumerateMonitors,
                                      "__atlasEnumerateMonitors", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMonitorQueryVideoModes",
                      JS_NewCFunction(ctx, jsMonitorQueryVideoModes,
                                      "__atlasMonitorQueryVideoModes", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMonitorGetCurrentVideoMode",
                      JS_NewCFunction(ctx, jsMonitorGetCurrentVideoMode,
                                      "__atlasMonitorGetCurrentVideoMode", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMonitorGetPhysicalSize",
                      JS_NewCFunction(ctx, jsMonitorGetPhysicalSize,
                                      "__atlasMonitorGetPhysicalSize", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMonitorGetPosition",
                      JS_NewCFunction(ctx, jsMonitorGetPosition,
                                      "__atlasMonitorGetPosition", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMonitorGetContentScale",
                      JS_NewCFunction(ctx, jsMonitorGetContentScale,
                                      "__atlasMonitorGetContentScale", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasMonitorGetName",
        JS_NewCFunction(ctx, jsMonitorGetName, "__atlasMonitorGetName", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetControllers",
                      JS_NewCFunction(ctx, jsWindowGetControllers,
                                      "__atlasGetControllers", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetController",
        JS_NewCFunction(ctx, jsWindowGetController, "__atlasGetController", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetJoystick",
        JS_NewCFunction(ctx, jsWindowGetJoystick, "__atlasGetJoystick", 2));
    JS_SetPropertyStr(ctx, global, "__atlasInstantiateObject",
                      JS_NewCFunction(ctx, jsWindowInstantiate,
                                      "__atlasInstantiateObject", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasDestroyObject",
        JS_NewCFunction(ctx, jsWindowDestroy, "__atlasDestroyObject", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasAddUIObject",
        JS_NewCFunction(ctx, jsWindowAddUIObject, "__atlasAddUIObject", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetWindowCamera",
        JS_NewCFunction(ctx, jsWindowSetCamera, "__atlasSetWindowCamera", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetWindowScene",
        JS_NewCFunction(ctx, jsWindowSetScene, "__atlasSetWindowScene", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetWindowTime",
        JS_NewCFunction(ctx, jsWindowGetTime, "__atlasGetWindowTime", 1));
    JS_SetPropertyStr(ctx, global, "__atlasAddWindowRenderTarget",
                      JS_NewCFunction(ctx, jsWindowAddRenderTarget,
                                      "__atlasAddWindowRenderTarget", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetWindowSize",
        JS_NewCFunction(ctx, jsWindowGetSize, "__atlasGetWindowSize", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowDeltaTime",
                      JS_NewCFunction(ctx, jsWindowGetDeltaTime,
                                      "__atlasGetWindowDeltaTime", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowFramesPerSecond",
                      JS_NewCFunction(ctx, jsWindowGetFramesPerSecond,
                                      "__atlasGetWindowFramesPerSecond", 1));
    JS_SetPropertyStr(ctx, global, "__atlasActivateWindowDebug",
                      JS_NewCFunction(ctx, jsWindowActivateDebug,
                                      "__atlasActivateWindowDebug", 1));
    JS_SetPropertyStr(ctx, global, "__atlasDeactivateWindowDebug",
                      JS_NewCFunction(ctx, jsWindowDeactivateDebug,
                                      "__atlasDeactivateWindowDebug", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetWindowGravity",
        JS_NewCFunction(ctx, jsWindowSetGravity, "__atlasSetWindowGravity", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasUseWindowTracer",
        JS_NewCFunction(ctx, jsWindowUseTracer, "__atlasUseWindowTracer", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetWindowLogOutput",
                      JS_NewCFunction(ctx, jsWindowSetLogOutput,
                                      "__atlasSetWindowLogOutput", 4));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowRenderScale",
                      JS_NewCFunction(ctx, jsWindowGetRenderScale,
                                      "__atlasGetWindowRenderScale", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUseWindowMetalUpscaling",
                      JS_NewCFunction(ctx, jsWindowUseMetalUpscaling,
                                      "__atlasUseWindowMetalUpscaling", 2));
    JS_SetPropertyStr(ctx, global, "__atlasIsWindowMetalUpscalingEnabled",
                      JS_NewCFunction(ctx, jsWindowIsMetalUpscalingEnabled,
                                      "__atlasIsWindowMetalUpscalingEnabled",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowMetalUpscalingRatio",
                      JS_NewCFunction(ctx, jsWindowGetMetalUpscalingRatio,
                                      "__atlasGetWindowMetalUpscalingRatio",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowSSAORenderScale",
                      JS_NewCFunction(ctx, jsWindowGetSSAORenderScale,
                                      "__atlasGetWindowSSAORenderScale", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowInputAction",
                      JS_NewCFunction(ctx, jsWindowGetInputAction,
                                      "__atlasGetWindowInputAction", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGamepadRumble",
        JS_NewCFunction(ctx, jsGamepadRumble, "__atlasGamepadRumble", 3));
    JS_SetPropertyStr(ctx, global, "__atlasJoystickGetAxisCount",
                      JS_NewCFunction(ctx, jsJoystickGetAxisCount,
                                      "__atlasJoystickGetAxisCount", 1));
    JS_SetPropertyStr(ctx, global, "__atlasJoystickGetButtonCount",
                      JS_NewCFunction(ctx, jsJoystickGetButtonCount,
                                      "__atlasJoystickGetButtonCount", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetInputConstants",
                      JS_NewCFunction(ctx, jsGetInputConstants,
                                      "__atlasGetInputConstants", 0));
    JS_SetPropertyStr(ctx, global, "__atlasGetWindowConstants",
                      JS_NewCFunction(ctx, jsGetWindowConstants,
                                      "__atlasGetWindowConstants", 0));
    JS_SetPropertyStr(
        ctx, global, "__auroraPerlinNoise",
        JS_NewCFunction(ctx, jsAuroraPerlinNoise, "__auroraPerlinNoise", 3));
    JS_SetPropertyStr(
        ctx, global, "__auroraSimplexNoise",
        JS_NewCFunction(ctx, jsAuroraSimplexNoise, "__auroraSimplexNoise", 2));
    JS_SetPropertyStr(
        ctx, global, "__auroraWorleyNoise",
        JS_NewCFunction(ctx, jsAuroraWorleyNoise, "__auroraWorleyNoise", 4));
    JS_SetPropertyStr(
        ctx, global, "__auroraFractalNoise",
        JS_NewCFunction(ctx, jsAuroraFractalNoise, "__auroraFractalNoise", 4));
    JS_SetPropertyStr(ctx, global, "__atlasRegisterInputAction",
                      JS_NewCFunction(ctx, jsRegisterInputAction,
                                      "__atlasRegisterInputAction", 1));
    JS_SetPropertyStr(ctx, global, "__atlasResetInputActions",
                      JS_NewCFunction(ctx, jsResetInputActions,
                                      "__atlasResetInputActions", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasIsKeyActive",
        JS_NewCFunction(ctx, jsIsKeyActive, "__atlasIsKeyActive", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasIsKeyPressed",
        JS_NewCFunction(ctx, jsIsKeyPressed, "__atlasIsKeyPressed", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsMouseButtonActive",
                      JS_NewCFunction(ctx, jsIsMouseButtonActive,
                                      "__atlasIsMouseButtonActive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsMouseButtonPressed",
                      JS_NewCFunction(ctx, jsIsMouseButtonPressed,
                                      "__atlasIsMouseButtonPressed", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetTextInput",
        JS_NewCFunction(ctx, jsGetTextInput, "__atlasGetTextInput", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasStartTextInput",
        JS_NewCFunction(ctx, jsStartTextInput, "__atlasStartTextInput", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasStopTextInput",
        JS_NewCFunction(ctx, jsStopTextInput, "__atlasStopTextInput", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsTextInputActive",
                      JS_NewCFunction(ctx, jsIsTextInputActive,
                                      "__atlasIsTextInputActive", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsControllerButtonPressed",
                      JS_NewCFunction(ctx, jsIsControllerButtonPressed,
                                      "__atlasIsControllerButtonPressed", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetControllerAxisValue",
                      JS_NewCFunction(ctx, jsGetControllerAxisValue,
                                      "__atlasGetControllerAxisValue", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetControllerAxisPairValue",
                      JS_NewCFunction(ctx, jsGetControllerAxisPairValue,
                                      "__atlasGetControllerAxisPairValue", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasCaptureMouse",
        JS_NewCFunction(ctx, jsCaptureMouse, "__atlasCaptureMouse", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasReleaseMouse",
        JS_NewCFunction(ctx, jsReleaseMouse, "__atlasReleaseMouse", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetMousePosition",
        JS_NewCFunction(ctx, jsGetMousePosition, "__atlasGetMousePosition", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsActionTriggered",
                      JS_NewCFunction(ctx, jsIsActionTriggered,
                                      "__atlasIsActionTriggered", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsActionCurrentlyActive",
                      JS_NewCFunction(ctx, jsIsActionCurrentlyActive,
                                      "__atlasIsActionCurrentlyActive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetAxisActionValue",
                      JS_NewCFunction(ctx, jsGetAxisActionValue,
                                      "__atlasGetAxisActionValue", 1));
    JS_SetPropertyStr(ctx, global, "__atlasRegisterInteractive",
                      JS_NewCFunction(ctx, jsRegisterInteractive,
                                      "__atlasRegisterInteractive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateTextureFromResource",
                      JS_NewCFunction(ctx, jsCreateTextureFromResource,
                                      "__atlasCreateTextureFromResource", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateEmptyTexture",
                      JS_NewCFunction(ctx, jsCreateEmptyTexture,
                                      "__atlasCreateEmptyTexture", 4));
    JS_SetPropertyStr(ctx, global, "__atlasCreateColorTexture",
                      JS_NewCFunction(ctx, jsCreateColorTexture,
                                      "__atlasCreateColorTexture", 4));
    JS_SetPropertyStr(ctx, global, "__atlasCreateCheckerboardTexture",
                      JS_NewCFunction(ctx, jsCreateCheckerboardTexture,
                                      "__atlasCreateCheckerboardTexture", 6));
    JS_SetPropertyStr(ctx, global, "__atlasCreateDoubleCheckerboardTexture",
                      JS_NewCFunction(ctx, jsCreateDoubleCheckerboardTexture,
                                      "__atlasCreateDoubleCheckerboardTexture",
                                      8));
    JS_SetPropertyStr(
        ctx, global, "__atlasDisplayTexture",
        JS_NewCFunction(ctx, jsDisplayTexture, "__atlasDisplayTexture", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateCubemap",
        JS_NewCFunction(ctx, jsCreateCubemap, "__atlasCreateCubemap", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateCubemapFromGroup",
                      JS_NewCFunction(ctx, jsCreateCubemapFromGroup,
                                      "__atlasCreateCubemapFromGroup", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetCubemapAverageColor",
                      JS_NewCFunction(ctx, jsGetCubemapAverageColor,
                                      "__atlasGetCubemapAverageColor", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateCubemapWithColors",
                      JS_NewCFunction(ctx, jsUpdateCubemapWithColors,
                                      "__atlasUpdateCubemapWithColors", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateSkybox",
        JS_NewCFunction(ctx, jsCreateSkybox, "__atlasCreateSkybox", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateRenderTarget",
                      JS_NewCFunction(ctx, jsCreateRenderTarget,
                                      "__atlasCreateRenderTarget", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAddRenderTargetEffect",
                      JS_NewCFunction(ctx, jsAddRenderTargetEffect,
                                      "__atlasAddRenderTargetEffect", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAddRenderTargetToPassQueue",
                      JS_NewCFunction(ctx, jsAddRenderTargetToPassQueue,
                                      "__atlasAddRenderTargetToPassQueue", 2));
    JS_SetPropertyStr(ctx, global, "__atlasDisplayRenderTarget",
                      JS_NewCFunction(ctx, jsDisplayRenderTarget,
                                      "__atlasDisplayRenderTarget", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreatePointLight",
        JS_NewCFunction(ctx, jsCreatePointLight, "__atlasCreatePointLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdatePointLight",
        JS_NewCFunction(ctx, jsUpdatePointLight, "__atlasUpdatePointLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePointLightDebugObject",
                      JS_NewCFunction(ctx, jsCreatePointLightDebugObject,
                                      "__atlasCreatePointLightDebugObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastPointLightShadows",
                      JS_NewCFunction(ctx, jsCastPointLightShadows,
                                      "__atlasCastPointLightShadows", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateDirectionalLight",
                      JS_NewCFunction(ctx, jsCreateDirectionalLight,
                                      "__atlasCreateDirectionalLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateDirectionalLight",
                      JS_NewCFunction(ctx, jsUpdateDirectionalLight,
                                      "__atlasUpdateDirectionalLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastDirectionalLightShadows",
                      JS_NewCFunction(ctx, jsCastDirectionalLightShadows,
                                      "__atlasCastDirectionalLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateSpotLight",
        JS_NewCFunction(ctx, jsCreateSpotLight, "__atlasCreateSpotLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateSpotLight",
        JS_NewCFunction(ctx, jsUpdateSpotLight, "__atlasUpdateSpotLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSpotLightDebugObject",
                      JS_NewCFunction(ctx, jsCreateSpotLightDebugObject,
                                      "__atlasCreateSpotLightDebugObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtSpotLight",
        JS_NewCFunction(ctx, jsLookAtSpotLight, "__atlasLookAtSpotLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCastSpotLightShadows",
                      JS_NewCFunction(ctx, jsCastSpotLightShadows,
                                      "__atlasCastSpotLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateAreaLight",
        JS_NewCFunction(ctx, jsCreateAreaLight, "__atlasCreateAreaLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateAreaLight",
        JS_NewCFunction(ctx, jsUpdateAreaLight, "__atlasUpdateAreaLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetAreaLightNormal",
                      JS_NewCFunction(ctx, jsGetAreaLightNormal,
                                      "__atlasGetAreaLightNormal", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetAreaLightRotation",
                      JS_NewCFunction(ctx, jsSetAreaLightRotation,
                                      "__atlasSetAreaLightRotation", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRotateAreaLight",
        JS_NewCFunction(ctx, jsRotateAreaLight, "__atlasRotateAreaLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateAreaLightDebugObject",
                      JS_NewCFunction(ctx, jsCreateAreaLightDebugObject,
                                      "__atlasCreateAreaLightDebugObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastAreaLightShadows",
                      JS_NewCFunction(ctx, jsCastAreaLightShadows,
                                      "__atlasCastAreaLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetObjectById",
        JS_NewCFunction(ctx, jsGetObjectById, "__atlasGetObjectById", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetObjectByName",
        JS_NewCFunction(ctx, jsGetObjectByName, "__atlasGetObjectByName", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetComponentByName",
                      JS_NewCFunction(ctx, jsGetComponentByName,
                                      "__atlasGetComponentByName", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLoadResource",
        JS_NewCFunction(ctx, jsLoadResource, "__atlasLoadResource", 3));
    JS_SetPropertyStr(ctx, global, "__atlasGetResourceByName",
                      JS_NewCFunction(ctx, jsGetResourceByName,
                                      "__atlasGetResourceByName", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasAddComponent",
        JS_NewCFunction(ctx, jsAddComponent, "__atlasAddComponent", 2));
    JS_SetPropertyStr(
        ctx, global, "__graphiteGetUISize",
        JS_NewCFunction(ctx, jsGraphiteGetUISize, "__graphiteGetUISize", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteGetUIScreenPosition",
                      JS_NewCFunction(ctx, jsGraphiteGetUIScreenPosition,
                                      "__graphiteGetUIScreenPosition", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteSetUIScreenPosition",
                      JS_NewCFunction(ctx, jsGraphiteSetUIScreenPosition,
                                      "__graphiteSetUIScreenPosition", 2));
    JS_SetPropertyStr(ctx, global, "__graphiteSetUIObjectStyle",
                      JS_NewCFunction(ctx, jsGraphiteSetUIObjectStyle,
                                      "__graphiteSetUIObjectStyle", 2));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateImage",
                      JS_NewCFunction(ctx, jsGraphiteCreateImage,
                                      "__graphiteCreateImage", 1));
    JS_SetPropertyStr(
        ctx, global, "__graphiteCreateText",
        JS_NewCFunction(ctx, jsGraphiteCreateText, "__graphiteCreateText", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateTextField",
                      JS_NewCFunction(ctx, jsGraphiteCreateTextField,
                                      "__graphiteCreateTextField", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateButton",
                      JS_NewCFunction(ctx, jsGraphiteCreateButton,
                                      "__graphiteCreateButton", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateCheckbox",
                      JS_NewCFunction(ctx, jsGraphiteCreateCheckbox,
                                      "__graphiteCreateCheckbox", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateColumn",
                      JS_NewCFunction(ctx, jsGraphiteCreateColumn,
                                      "__graphiteCreateColumn", 1));
    JS_SetPropertyStr(
        ctx, global, "__graphiteCreateRow",
        JS_NewCFunction(ctx, jsGraphiteCreateRow, "__graphiteCreateRow", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCreateStack",
                      JS_NewCFunction(ctx, jsGraphiteCreateStack,
                                      "__graphiteCreateStack", 1));
    JS_SetPropertyStr(
        ctx, global, "__graphiteCreateFont",
        JS_NewCFunction(ctx, jsGraphiteCreateFont, "__graphiteCreateFont", 1));
    JS_SetPropertyStr(
        ctx, global, "__graphiteGetFont",
        JS_NewCFunction(ctx, jsGraphiteGetFont, "__graphiteGetFont", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteChangeFontSize",
                      JS_NewCFunction(ctx, jsGraphiteChangeFontSize,
                                      "__graphiteChangeFontSize", 2));
    JS_SetPropertyStr(
        ctx, global, "__graphiteGetTheme",
        JS_NewCFunction(ctx, jsGraphiteGetTheme, "__graphiteGetTheme", 0));
    JS_SetPropertyStr(
        ctx, global, "__graphiteSetTheme",
        JS_NewCFunction(ctx, jsGraphiteSetTheme, "__graphiteSetTheme", 1));
    JS_SetPropertyStr(
        ctx, global, "__graphiteResetTheme",
        JS_NewCFunction(ctx, jsGraphiteResetTheme, "__graphiteResetTheme", 0));
    JS_SetPropertyStr(ctx, global, "__graphiteTextFieldFocus",
                      JS_NewCFunction(ctx, jsGraphiteTextFieldFocus,
                                      "__graphiteTextFieldFocus", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteTextFieldBlur",
                      JS_NewCFunction(ctx, jsGraphiteTextFieldBlur,
                                      "__graphiteTextFieldBlur", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteTextFieldIsFocused",
                      JS_NewCFunction(ctx, jsGraphiteTextFieldIsFocused,
                                      "__graphiteTextFieldIsFocused", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteTextFieldGetCursorIndex",
                      JS_NewCFunction(ctx, jsGraphiteTextFieldGetCursorIndex,
                                      "__graphiteTextFieldGetCursorIndex", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteButtonIsHovered",
                      JS_NewCFunction(ctx, jsGraphiteButtonIsHovered,
                                      "__graphiteButtonIsHovered", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCheckboxToggle",
                      JS_NewCFunction(ctx, jsGraphiteCheckboxToggle,
                                      "__graphiteCheckboxToggle", 1));
    JS_SetPropertyStr(ctx, global, "__graphiteCheckboxIsHovered",
                      JS_NewCFunction(ctx, jsGraphiteCheckboxIsHovered,
                                      "__graphiteCheckboxIsHovered", 1));
    JS_SetPropertyStr(ctx, global, "__hydraCreateWorleyNoise",
                      JS_NewCFunction(ctx, jsHydraCreateWorleyNoise,
                                      "__hydraCreateWorleyNoise", 2));
    JS_SetPropertyStr(ctx, global, "__hydraWorleyGetValue",
                      JS_NewCFunction(ctx, jsHydraWorleyGetValue,
                                      "__hydraWorleyGetValue", 4));
    JS_SetPropertyStr(ctx, global, "__hydraWorleyGet3dTexture",
                      JS_NewCFunction(ctx, jsHydraWorleyGet3dTexture,
                                      "__hydraWorleyGet3dTexture", 2));
    JS_SetPropertyStr(ctx, global, "__hydraWorleyGetDetailTexture",
                      JS_NewCFunction(ctx, jsHydraWorleyGetDetailTexture,
                                      "__hydraWorleyGetDetailTexture", 2));
    JS_SetPropertyStr(ctx, global, "__hydraWorleyGetAllChannelsTexture",
                      JS_NewCFunction(ctx, jsHydraWorleyGetAllChannelsTexture,
                                      "__hydraWorleyGetAllChannelsTexture", 2));
    JS_SetPropertyStr(
        ctx, global, "__hydraCreateClouds",
        JS_NewCFunction(ctx, jsHydraCreateClouds, "__hydraCreateClouds", 2));
    JS_SetPropertyStr(
        ctx, global, "__hydraUpdateClouds",
        JS_NewCFunction(ctx, jsHydraUpdateClouds, "__hydraUpdateClouds", 1));
    JS_SetPropertyStr(ctx, global, "__hydraCloudsGetTexture",
                      JS_NewCFunction(ctx, jsHydraCloudsGetTexture,
                                      "__hydraCloudsGetTexture", 2));
    JS_SetPropertyStr(ctx, global, "__hydraCreateAtmosphere",
                      JS_NewCFunction(ctx, jsHydraCreateAtmosphere,
                                      "__hydraCreateAtmosphere", 0));
    JS_SetPropertyStr(ctx, global, "__hydraUpdateAtmosphere",
                      JS_NewCFunction(ctx, jsHydraUpdateAtmosphere,
                                      "__hydraUpdateAtmosphere", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereEnable",
                      JS_NewCFunction(ctx, jsHydraAtmosphereEnable,
                                      "__hydraAtmosphereEnable", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereDisable",
                      JS_NewCFunction(ctx, jsHydraAtmosphereDisable,
                                      "__hydraAtmosphereDisable", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereIsEnabled",
                      JS_NewCFunction(ctx, jsHydraAtmosphereIsEnabled,
                                      "__hydraAtmosphereIsEnabled", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereEnableWeather",
                      JS_NewCFunction(ctx, jsHydraAtmosphereEnableWeather,
                                      "__hydraAtmosphereEnableWeather", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereDisableWeather",
                      JS_NewCFunction(ctx, jsHydraAtmosphereDisableWeather,
                                      "__hydraAtmosphereDisableWeather", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetNormalizedTime",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetNormalizedTime,
                                      "__hydraAtmosphereGetNormalizedTime", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetSunAngle",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetSunAngle,
                                      "__hydraAtmosphereGetSunAngle", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetMoonAngle",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetMoonAngle,
                                      "__hydraAtmosphereGetMoonAngle", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetLightIntensity",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetLightIntensity,
                                      "__hydraAtmosphereGetLightIntensity", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetLightColor",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetLightColor,
                                      "__hydraAtmosphereGetLightColor", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereGetSkyboxColors",
                      JS_NewCFunction(ctx, jsHydraAtmosphereGetSkyboxColors,
                                      "__hydraAtmosphereGetSkyboxColors", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereCreateSkyCubemap",
                      JS_NewCFunction(ctx, jsHydraAtmosphereCreateSkyCubemap,
                                      "__hydraAtmosphereCreateSkyCubemap", 2));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereUpdateSkyCubemap",
                      JS_NewCFunction(ctx, jsHydraAtmosphereUpdateSkyCubemap,
                                      "__hydraAtmosphereUpdateSkyCubemap", 2));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereCastShadows",
                      JS_NewCFunction(ctx, jsHydraAtmosphereCastShadows,
                                      "__hydraAtmosphereCastShadows", 2));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereUseGlobalLight",
                      JS_NewCFunction(ctx, jsHydraAtmosphereUseGlobalLight,
                                      "__hydraAtmosphereUseGlobalLight", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereIsDaytime",
                      JS_NewCFunction(ctx, jsHydraAtmosphereIsDaytime,
                                      "__hydraAtmosphereIsDaytime", 1));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereSetTime",
                      JS_NewCFunction(ctx, jsHydraAtmosphereSetTime,
                                      "__hydraAtmosphereSetTime", 4));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereAddClouds",
                      JS_NewCFunction(ctx, jsHydraAtmosphereAddClouds,
                                      "__hydraAtmosphereAddClouds", 3));
    JS_SetPropertyStr(ctx, global, "__hydraAtmosphereResetRuntimeState",
                      JS_NewCFunction(ctx, jsHydraAtmosphereResetRuntimeState,
                                      "__hydraAtmosphereResetRuntimeState", 1));
    JS_SetPropertyStr(
        ctx, global, "__hydraCreateFluid",
        JS_NewCFunction(ctx, jsHydraCreateFluid, "__hydraCreateFluid", 1));
    JS_SetPropertyStr(
        ctx, global, "__hydraFluidCreate",
        JS_NewCFunction(ctx, jsHydraFluidCreate, "__hydraFluidCreate", 3));
    JS_SetPropertyStr(ctx, global, "__hydraFluidSetExtent",
                      JS_NewCFunction(ctx, jsHydraFluidSetExtent,
                                      "__hydraFluidSetExtent", 2));
    JS_SetPropertyStr(ctx, global, "__hydraFluidSetWaveVelocity",
                      JS_NewCFunction(ctx, jsHydraFluidSetWaveVelocity,
                                      "__hydraFluidSetWaveVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__hydraFluidSetWaterColor",
                      JS_NewCFunction(ctx, jsHydraFluidSetWaterColor,
                                      "__hydraFluidSetWaterColor", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveGetAudioEngine",
        JS_NewCFunction(ctx, jsGetAudioEngine, "__finewaveGetAudioEngine", 0));
    JS_SetPropertyStr(
        ctx, global, "__finewaveAudioEngineSetListenerPosition",
        JS_NewCFunction(ctx, jsAudioEngineSetListenerPosition,
                        "__finewaveAudioEngineSetListenerPosition", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveAudioEngineSetListenerOrientation",
        JS_NewCFunction(ctx, jsAudioEngineSetListenerOrientation,
                        "__finewaveAudioEngineSetListenerOrientation", 3));
    JS_SetPropertyStr(
        ctx, global, "__finewaveAudioEngineSetListenerVelocity",
        JS_NewCFunction(ctx, jsAudioEngineSetListenerVelocity,
                        "__finewaveAudioEngineSetListenerVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioEngineSetMasterVolume",
                      JS_NewCFunction(ctx, jsAudioEngineSetMasterVolume,
                                      "__finewaveAudioEngineSetMasterVolume",
                                      2));
    JS_SetPropertyStr(ctx, global, "__finewaveCreateAudioData",
                      JS_NewCFunction(ctx, jsCreateAudioData,
                                      "__finewaveCreateAudioData", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveCreateAudioSource",
                      JS_NewCFunction(ctx, jsCreateAudioSource,
                                      "__finewaveCreateAudioSource", 0));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetData",
                      JS_NewCFunction(ctx, jsAudioSourceSetData,
                                      "__finewaveAudioSourceSetData", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceFromFile",
                      JS_NewCFunction(ctx, jsAudioSourceFromFile,
                                      "__finewaveAudioSourceFromFile", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourcePlay",
                      JS_NewCFunction(ctx, jsAudioSourcePlay,
                                      "__finewaveAudioSourcePlay", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourcePause",
                      JS_NewCFunction(ctx, jsAudioSourcePause,
                                      "__finewaveAudioSourcePause", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceStop",
                      JS_NewCFunction(ctx, jsAudioSourceStop,
                                      "__finewaveAudioSourceStop", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetLoop",
                      JS_NewCFunction(ctx, jsAudioSourceSetLoop,
                                      "__finewaveAudioSourceSetLoop", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetVolume",
                      JS_NewCFunction(ctx, jsAudioSourceSetVolume,
                                      "__finewaveAudioSourceSetVolume", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetPitch",
                      JS_NewCFunction(ctx, jsAudioSourceSetPitch,
                                      "__finewaveAudioSourceSetPitch", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetPosition",
                      JS_NewCFunction(ctx, jsAudioSourceSetPosition,
                                      "__finewaveAudioSourceSetPosition", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceSetVelocity",
                      JS_NewCFunction(ctx, jsAudioSourceSetVelocity,
                                      "__finewaveAudioSourceSetVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceIsPlaying",
                      JS_NewCFunction(ctx, jsAudioSourceIsPlaying,
                                      "__finewaveAudioSourceIsPlaying", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourcePlayFrom",
                      JS_NewCFunction(ctx, jsAudioSourcePlayFrom,
                                      "__finewaveAudioSourcePlayFrom", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveAudioSourceDisableSpatialization",
        JS_NewCFunction(ctx, jsAudioSourceDisableSpatialization,
                        "__finewaveAudioSourceDisableSpatialization", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceApplyEffect",
                      JS_NewCFunction(ctx, jsAudioSourceApplyEffect,
                                      "__finewaveAudioSourceApplyEffect", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceGetPosition",
                      JS_NewCFunction(ctx, jsAudioSourceGetPosition,
                                      "__finewaveAudioSourceGetPosition", 1));
    JS_SetPropertyStr(
        ctx, global, "__finewaveAudioSourceGetListenerPosition",
        JS_NewCFunction(ctx, jsAudioSourceGetListenerPosition,
                        "__finewaveAudioSourceGetListenerPosition", 1));
    JS_SetPropertyStr(ctx, global, "__finewaveAudioSourceUseSpatialization",
                      JS_NewCFunction(ctx, jsAudioSourceUseSpatialization,
                                      "__finewaveAudioSourceUseSpatialization",
                                      1));
    JS_SetPropertyStr(
        ctx, global, "__finewaveCreateReverb",
        JS_NewCFunction(ctx, jsCreateReverb, "__finewaveCreateReverb", 0));
    JS_SetPropertyStr(
        ctx, global, "__finewaveCreateEcho",
        JS_NewCFunction(ctx, jsCreateEcho, "__finewaveCreateEcho", 0));
    JS_SetPropertyStr(ctx, global, "__finewaveCreateDistortion",
                      JS_NewCFunction(ctx, jsCreateDistortion,
                                      "__finewaveCreateDistortion", 0));
    JS_SetPropertyStr(ctx, global, "__finewaveReverbSetRoomSize",
                      JS_NewCFunction(ctx, jsReverbSetRoomSize,
                                      "__finewaveReverbSetRoomSize", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveReverbSetDamping",
                      JS_NewCFunction(ctx, jsReverbSetDamping,
                                      "__finewaveReverbSetDamping", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveReverbSetWetLevel",
                      JS_NewCFunction(ctx, jsReverbSetWetLevel,
                                      "__finewaveReverbSetWetLevel", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveReverbSetDryLevel",
                      JS_NewCFunction(ctx, jsReverbSetDryLevel,
                                      "__finewaveReverbSetDryLevel", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveReverbSetWidth",
        JS_NewCFunction(ctx, jsReverbSetWidth, "__finewaveReverbSetWidth", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveEchoSetDelay",
        JS_NewCFunction(ctx, jsEchoSetDelay, "__finewaveEchoSetDelay", 2));
    JS_SetPropertyStr(
        ctx, global, "__finewaveEchoSetDecay",
        JS_NewCFunction(ctx, jsEchoSetDecay, "__finewaveEchoSetDecay", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveEchoSetWetLevel",
                      JS_NewCFunction(ctx, jsEchoSetWetLevel,
                                      "__finewaveEchoSetWetLevel", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveEchoSetDryLevel",
                      JS_NewCFunction(ctx, jsEchoSetDryLevel,
                                      "__finewaveEchoSetDryLevel", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveDistortionSetEdge",
                      JS_NewCFunction(ctx, jsDistortionSetEdge,
                                      "__finewaveDistortionSetEdge", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveDistortionSetGain",
                      JS_NewCFunction(ctx, jsDistortionSetGain,
                                      "__finewaveDistortionSetGain", 2));
    JS_SetPropertyStr(ctx, global, "__finewaveDistortionSetLowpassCutoff",
                      JS_NewCFunction(ctx, jsDistortionSetLowpassCutoff,
                                      "__finewaveDistortionSetLowpassCutoff",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateAudioPlayer",
                      JS_NewCFunction(ctx, jsCreateAudioPlayer,
                                      "__atlasCreateAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasInitAudioPlayer",
        JS_NewCFunction(ctx, jsInitAudioPlayer, "__atlasInitAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasPlayAudioPlayer",
        JS_NewCFunction(ctx, jsPlayAudioPlayer, "__atlasPlayAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasPauseAudioPlayer",
        JS_NewCFunction(ctx, jsPauseAudioPlayer, "__atlasPauseAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasStopAudioPlayer",
        JS_NewCFunction(ctx, jsStopAudioPlayer, "__atlasStopAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerVolume",
                      JS_NewCFunction(ctx, jsSetAudioPlayerVolume,
                                      "__atlasSetAudioPlayerVolume", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerLoop",
                      JS_NewCFunction(ctx, jsSetAudioPlayerLoop,
                                      "__atlasSetAudioPlayerLoop", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerSource",
                      JS_NewCFunction(ctx, jsSetAudioPlayerSource,
                                      "__atlasSetAudioPlayerSource", 2));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateAudioPlayer",
                      JS_NewCFunction(ctx, jsUpdateAudioPlayer,
                                      "__atlasUpdateAudioPlayer", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerPosition",
                      JS_NewCFunction(ctx, jsSetAudioPlayerPosition,
                                      "__atlasSetAudioPlayerPosition", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasUseSpatialAudio",
        JS_NewCFunction(ctx, jsUseSpatialAudio, "__atlasUseSpatialAudio", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateRigidbody",
        JS_NewCFunction(ctx, jsCreateRigidbody, "__atlasCreateRigidbody", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCloneRigidbody",
        JS_NewCFunction(ctx, jsCloneRigidbody, "__atlasCloneRigidbody", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasInitRigidbody",
        JS_NewCFunction(ctx, jsInitRigidbody, "__atlasInitRigidbody", 1));
    JS_SetPropertyStr(ctx, global, "__atlasBeforePhysicsRigidbody",
                      JS_NewCFunction(ctx, jsBeforePhysicsRigidbody,
                                      "__atlasBeforePhysicsRigidbody", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateRigidbody",
        JS_NewCFunction(ctx, jsUpdateRigidbody, "__atlasUpdateRigidbody", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyAddCollider",
                      JS_NewCFunction(ctx, jsRigidbodyAddCollider,
                                      "__atlasRigidbodyAddCollider", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetFriction",
                      JS_NewCFunction(ctx, jsRigidbodySetFriction,
                                      "__atlasRigidbodySetFriction", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyApplyForce",
                      JS_NewCFunction(ctx, jsRigidbodyApplyForce,
                                      "__atlasRigidbodyApplyForce", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyApplyForceAtPoint",
                      JS_NewCFunction(ctx, jsRigidbodyApplyForceAtPoint,
                                      "__atlasRigidbodyApplyForceAtPoint", 3));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyApplyImpulse",
                      JS_NewCFunction(ctx, jsRigidbodyApplyImpulse,
                                      "__atlasRigidbodyApplyImpulse", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetLinearVelocity",
                      JS_NewCFunction(ctx, jsRigidbodySetLinearVelocity,
                                      "__atlasRigidbodySetLinearVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyAddLinearVelocity",
                      JS_NewCFunction(ctx, jsRigidbodyAddLinearVelocity,
                                      "__atlasRigidbodyAddLinearVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetAngularVelocity",
                      JS_NewCFunction(ctx, jsRigidbodySetAngularVelocity,
                                      "__atlasRigidbodySetAngularVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyAddAngularVelocity",
                      JS_NewCFunction(ctx, jsRigidbodyAddAngularVelocity,
                                      "__atlasRigidbodyAddAngularVelocity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetMaxLinearVelocity",
                      JS_NewCFunction(ctx, jsRigidbodySetMaxLinearVelocity,
                                      "__atlasRigidbodySetMaxLinearVelocity",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetMaxAngularVelocity",
                      JS_NewCFunction(ctx, jsRigidbodySetMaxAngularVelocity,
                                      "__atlasRigidbodySetMaxAngularVelocity",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyGetLinearVelocity",
                      JS_NewCFunction(ctx, jsRigidbodyGetLinearVelocity,
                                      "__atlasRigidbodyGetLinearVelocity", 1));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyGetAngularVelocity",
                      JS_NewCFunction(ctx, jsRigidbodyGetAngularVelocity,
                                      "__atlasRigidbodyGetAngularVelocity", 1));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyGetVelocity",
                      JS_NewCFunction(ctx, jsRigidbodyGetVelocity,
                                      "__atlasRigidbodyGetVelocity", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyRaycast",
        JS_NewCFunction(ctx, jsRigidbodyRaycast, "__atlasRigidbodyRaycast", 3));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRaycastAll",
                      JS_NewCFunction(ctx, jsRigidbodyRaycastAll,
                                      "__atlasRigidbodyRaycastAll", 3));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRaycastWorld",
                      JS_NewCFunction(ctx, jsRigidbodyRaycastWorld,
                                      "__atlasRigidbodyRaycastWorld", 4));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRaycastWorldAll",
                      JS_NewCFunction(ctx, jsRigidbodyRaycastWorldAll,
                                      "__atlasRigidbodyRaycastWorldAll", 4));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRaycastTagged",
                      JS_NewCFunction(ctx, jsRigidbodyRaycastTagged,
                                      "__atlasRigidbodyRaycastTagged", 4));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRaycastTaggedAll",
                      JS_NewCFunction(ctx, jsRigidbodyRaycastTaggedAll,
                                      "__atlasRigidbodyRaycastTaggedAll", 4));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyOverlap",
        JS_NewCFunction(ctx, jsRigidbodyOverlap, "__atlasRigidbodyOverlap", 1));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyOverlapWithCollider",
                      JS_NewCFunction(ctx, jsRigidbodyOverlapWithCollider,
                                      "__atlasRigidbodyOverlapWithCollider",
                                      2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyOverlapWithColliderWorld",
        JS_NewCFunction(ctx, jsRigidbodyOverlapWithColliderWorld,
                        "__atlasRigidbodyOverlapWithColliderWorld", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyPredictMovementWithCollider",
        JS_NewCFunction(ctx, jsRigidbodyPredictMovementWithCollider,
                        "__atlasRigidbodyPredictMovementWithCollider", 3));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyPredictMovement",
                      JS_NewCFunction(ctx, jsRigidbodyPredictMovement,
                                      "__atlasRigidbodyPredictMovement", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyPredictMovementWithColliderWorld",
        JS_NewCFunction(ctx, jsRigidbodyPredictMovementWithColliderWorld,
                        "__atlasRigidbodyPredictMovementWithColliderWorld", 4));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyPredictMovementWorld",
                      JS_NewCFunction(ctx, jsRigidbodyPredictMovementWorld,
                                      "__atlasRigidbodyPredictMovementWorld",
                                      3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyPredictMovementWithColliderAll",
        JS_NewCFunction(ctx, jsRigidbodyPredictMovementWithColliderAll,
                        "__atlasRigidbodyPredictMovementWithColliderAll", 3));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyPredictMovementAll",
                      JS_NewCFunction(ctx, jsRigidbodyPredictMovementAll,
                                      "__atlasRigidbodyPredictMovementAll", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyPredictMovementWithColliderAllWorld",
        JS_NewCFunction(ctx, jsRigidbodyPredictMovementWithColliderAllWorld,
                        "__atlasRigidbodyPredictMovementWithColliderAllWorld",
                        4));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyPredictMovementAllWorld",
                      JS_NewCFunction(ctx, jsRigidbodyPredictMovementAllWorld,
                                      "__atlasRigidbodyPredictMovementAllWorld",
                                      3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyHasTag",
        JS_NewCFunction(ctx, jsRigidbodyHasTag, "__atlasRigidbodyHasTag", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodyAddTag",
        JS_NewCFunction(ctx, jsRigidbodyAddTag, "__atlasRigidbodyAddTag", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodyRemoveTag",
                      JS_NewCFunction(ctx, jsRigidbodyRemoveTag,
                                      "__atlasRigidbodyRemoveTag", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetDamping",
                      JS_NewCFunction(ctx, jsRigidbodySetDamping,
                                      "__atlasRigidbodySetDamping", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRigidbodySetMass",
        JS_NewCFunction(ctx, jsRigidbodySetMass, "__atlasRigidbodySetMass", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetRestitution",
                      JS_NewCFunction(ctx, jsRigidbodySetRestitution,
                                      "__atlasRigidbodySetRestitution", 2));
    JS_SetPropertyStr(ctx, global, "__atlasRigidbodySetMotionType",
                      JS_NewCFunction(ctx, jsRigidbodySetMotionType,
                                      "__atlasRigidbodySetMotionType", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSensorSetSignal",
        JS_NewCFunction(ctx, jsSensorSetSignal, "__atlasSensorSetSignal", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateVehicle",
        JS_NewCFunction(ctx, jsCreateVehicle, "__atlasCreateVehicle", 1));
    JS_SetPropertyStr(ctx, global, "__atlasVehicleRequestRecreate",
                      JS_NewCFunction(ctx, jsVehicleRequestRecreate,
                                      "__atlasVehicleRequestRecreate", 1));
    JS_SetPropertyStr(ctx, global, "__atlasVehicleBeforePhysics",
                      JS_NewCFunction(ctx, jsVehicleBeforePhysics,
                                      "__atlasVehicleBeforePhysics", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateFixedJoint",
        JS_NewCFunction(ctx, jsCreateFixedJoint, "__atlasCreateFixedJoint", 1));
    JS_SetPropertyStr(ctx, global, "__atlasFixedJointBeforePhysics",
                      JS_NewCFunction(ctx, jsFixedJointBeforePhysics,
                                      "__atlasFixedJointBeforePhysics", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasFixedJointBreak",
        JS_NewCFunction(ctx, jsFixedJointBreak, "__atlasFixedJointBreak", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateHingeJoint",
        JS_NewCFunction(ctx, jsCreateHingeJoint, "__atlasCreateHingeJoint", 1));
    JS_SetPropertyStr(ctx, global, "__atlasHingeJointBeforePhysics",
                      JS_NewCFunction(ctx, jsHingeJointBeforePhysics,
                                      "__atlasHingeJointBeforePhysics", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHingeJointBreak",
        JS_NewCFunction(ctx, jsHingeJointBreak, "__atlasHingeJointBreak", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSpringJoint",
                      JS_NewCFunction(ctx, jsCreateSpringJoint,
                                      "__atlasCreateSpringJoint", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSpringJointBeforePhysics",
                      JS_NewCFunction(ctx, jsSpringJointBeforePhysics,
                                      "__atlasSpringJointBeforePhysics", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasSpringJointBreak",
        JS_NewCFunction(ctx, jsSpringJointBreak, "__atlasSpringJointBreak", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateCoreObject",
        JS_NewCFunction(ctx, jsCreateCoreObject, "__atlasCreateCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__auroraCreateTerrain",
        JS_NewCFunction(ctx, jsCreateTerrain, "__auroraCreateTerrain", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateModel",
        JS_NewCFunction(ctx, jsCreateModel, "__atlasCreateModel", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetModelObjects",
        JS_NewCFunction(ctx, jsGetModelObjects, "__atlasGetModelObjects", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMoveModel",
                      JS_NewCFunction(ctx, jsMoveModel, "__atlasMoveModel", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelPosition",
        JS_NewCFunction(ctx, jsSetModelPosition, "__atlasSetModelPosition", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelRotation",
        JS_NewCFunction(ctx, jsSetModelRotation, "__atlasSetModelRotation", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtModel",
        JS_NewCFunction(ctx, jsLookAtModel, "__atlasLookAtModel", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRotateModel",
        JS_NewCFunction(ctx, jsRotateModel, "__atlasRotateModel", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelScale",
        JS_NewCFunction(ctx, jsSetModelScale, "__atlasSetModelScale", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasScaleModelBy",
        JS_NewCFunction(ctx, jsScaleModelBy, "__atlasScaleModelBy", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowModel",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowModel", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideModel",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideModel", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateParticleEmitter",
                      JS_NewCFunction(ctx, jsCreateParticleEmitter,
                                      "__atlasCreateParticleEmitter", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAttachParticleEmitterTexture",
                      JS_NewCFunction(ctx, jsAttachParticleEmitterTexture,
                                      "__atlasAttachParticleEmitterTexture",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterColor",
                      JS_NewCFunction(ctx, jsSetParticleEmitterColor,
                                      "__atlasSetParticleEmitterColor", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterUseTexture",
                      JS_NewCFunction(ctx, jsSetParticleEmitterUseTexture,
                                      "__atlasSetParticleEmitterUseTexture",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterPosition",
                      JS_NewCFunction(ctx, jsSetParticleEmitterPosition,
                                      "__atlasSetParticleEmitterPosition", 2));
    JS_SetPropertyStr(ctx, global, "__atlasMoveParticleEmitter",
                      JS_NewCFunction(ctx, jsMoveParticleEmitter,
                                      "__atlasMoveParticleEmitter", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetParticleEmitterPosition",
                      JS_NewCFunction(ctx, jsGetParticleEmitterPosition,
                                      "__atlasGetParticleEmitterPosition", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterEmissionType",
                      JS_NewCFunction(ctx, jsSetParticleEmitterEmissionType,
                                      "__atlasSetParticleEmitterEmissionType",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterDirection",
                      JS_NewCFunction(ctx, jsSetParticleEmitterDirection,
                                      "__atlasSetParticleEmitterDirection", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSpawnRadius",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSpawnRadius,
                                      "__atlasSetParticleEmitterSpawnRadius",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSpawnRate",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSpawnRate,
                                      "__atlasSetParticleEmitterSpawnRate", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSettings",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSettings,
                                      "__atlasSetParticleEmitterSettings", 2));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitOnce",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitOnce,
                                      "__atlasParticleEmitterEmitOnce", 1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitContinuous",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitContinuous,
                                      "__atlasParticleEmitterEmitContinuous",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterStartEmission",
                      JS_NewCFunction(ctx, jsParticleEmitterStartEmission,
                                      "__atlasParticleEmitterStartEmission",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterStopEmission",
                      JS_NewCFunction(ctx, jsParticleEmitterStopEmission,
                                      "__atlasParticleEmitterStopEmission", 1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitBurst",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitBurst,
                                      "__atlasParticleEmitterEmitBurst", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateObject",
        JS_NewCFunction(ctx, jsUpdateObject, "__atlasUpdateObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateCoreObject",
        JS_NewCFunction(ctx, jsUpdateObject, "__atlasUpdateCoreObject", 2));
    JS_SetPropertyStr(
        ctx, global, "__auroraUpdateTerrain",
        JS_NewCFunction(ctx, jsUpdateObject, "__auroraUpdateTerrain", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateBox",
        JS_NewCFunction(ctx, jsCreatePrimitiveBox, "__atlasCreateBox", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreatePlane",
        JS_NewCFunction(ctx, jsCreatePrimitivePlane, "__atlasCreatePlane", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePyramid",
                      JS_NewCFunction(ctx, jsCreatePrimitivePyramid,
                                      "__atlasCreatePyramid", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSphere",
                      JS_NewCFunction(ctx, jsCreatePrimitiveSphere,
                                      "__atlasCreateSphere", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasCloneCoreObject",
        JS_NewCFunction(ctx, jsCloneCoreObject, "__atlasCloneCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasMakeEmissive",
        JS_NewCFunction(ctx, jsMakeEmissive, "__atlasMakeEmissive", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasAttachTexture",
        JS_NewCFunction(ctx, jsAttachTexture, "__atlasAttachTexture", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowObject",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideObject",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowCoreObject",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideCoreObject",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowParticleEmitter",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowParticleEmitter", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideParticleEmitter",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideParticleEmitter", 1));
    JS_SetPropertyStr(ctx, global, "__atlasEnableDeferredRendering",
                      JS_NewCFunction(ctx, jsEnableDeferred,
                                      "__atlasEnableDeferredRendering", 1));
    JS_SetPropertyStr(ctx, global, "__atlasDisableDeferredRendering",
                      JS_NewCFunction(ctx, jsDisableDeferred,
                                      "__atlasDisableDeferredRendering", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateInstance",
        JS_NewCFunction(ctx, jsCreateInstance, "__atlasCreateInstance", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCommitInstance",
        JS_NewCFunction(ctx, jsCommitInstance, "__atlasCommitInstance", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtObject",
        JS_NewCFunction(ctx, jsLookAtObject, "__atlasLookAtObject", 3));
    JS_SetPropertyStr(ctx, global, "__atlasSetRotationQuaternion",
                      JS_NewCFunction(ctx, jsSetRotationQuaternion,
                                      "__atlasSetRotationQuaternion", 2));
    JS_FreeValue(ctx, global);
}

char *runtime::scripting::normalizeModuleName(JSContext *ctx,
                                              const char *baseName,
                                              const char *name, void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);
    const std::string base = baseName == nullptr ? "" : baseName;
    const std::string module = name == nullptr ? "" : name;

    if (host->modules.contains(module)) {
        return js_strdup(ctx, module.c_str());
    }

    if (!module.empty() && module[0] == '.') {
        auto slash = base.rfind('/');
        std::string dir =
            (slash == std::string::npos) ? "" : base.substr(0, slash + 1);
        std::string resolved = dir + module;

        while (true) {
            auto pos = resolved.find("/./");
            if (pos == std::string::npos) {
                break;
            }
            resolved.replace(pos, 3, "/");
        }

        while (true) {
            auto pos = resolved.find("../");
            if (pos == std::string::npos) {
                break;
            }
            auto prev = resolved.rfind('/', pos > 1 ? pos - 2 : 0);
            if (prev == std::string::npos) {
                break;
            }
            auto next = resolved.find('/', pos + 2);
            resolved.erase(
                prev + 1,
                (next == std::string::npos ? resolved.size() : next + 1) -
                    (prev + 1));
        }

        if (host->modules.contains(resolved)) {
            return js_strdup(ctx, resolved.c_str());
        }
    }

    JS_ThrowReferenceError(ctx, "Could not resolve module '%s' from '%s'",
                           module.c_str(),
                           base.empty() ? "<root>" : base.c_str());
    return nullptr;
}

JSModuleDef *runtime::scripting::loadModule(JSContext *ctx,
                                            const char *module_name,
                                            void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);

    auto it = host->modules.find(module_name);
    if (it == host->modules.end()) {
        JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
        return nullptr;
    }

    const std::string &source = it->second;

    JSValue func_val = JS_Eval(ctx, source.c_str(), source.size(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func_val)) {
        return nullptr;
    }

    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val));
    JS_FreeValue(ctx, func_val);
    return m;
}

bool runtime::scripting::evalModule(JSContext *ctx, const std::string &name,
                                    const std::string &src) {
    JSValue compiled = JS_Eval(ctx, src.c_str(), src.length(), name.c_str(),
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!checkNotException(ctx, compiled, "compile module")) {
        return false;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (!checkNotException(ctx, result, "execute module")) {
        return false;
    }

    JS_FreeValue(ctx, result);
    return true;
}

JSValue
runtime::scripting::importModuleNamespace(JSContext *ctx,
                                          const std::string &module_name) {
    std::string src = "import * as ns from '" + module_name +
                      "';\n"
                      "globalThis.__atlas_tmp_ns = ns;\n";

    JSValue compiled = JS_Eval(ctx, src.c_str(), src.size(), "<import_ns>",
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) {
        return JS_EXCEPTION;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) {
        return JS_EXCEPTION;
    }
    JS_FreeValue(ctx, result);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ns = JS_GetPropertyStr(ctx, global, "__atlas_tmp_ns");
    JSAtom atom = JS_NewAtom(ctx, "__atlas_tmp_ns");
    JS_DeleteProperty(ctx, global, atom, 0);
    JS_FreeAtom(ctx, atom);
    JS_FreeValue(ctx, global);
    return ns;
}

ScriptInstance::~ScriptInstance() {
    if (ctx && !JS_IsUndefined(instance)) {
        JS_FreeValue(ctx, instance);
    }
}

bool ScriptInstance::callMethod(const char *method_name, int argc,
                                JSValueConst *argv) {
    return callObjectMethod(ctx, instance, method_name, argc, argv);
}

ScriptInstance *runtime::scripting::createScriptInstance(
    JSContext *ctx, const std::string &entryModuleName,
    const std::string &scriptPath, const std::string &className) {
    JSValue ns =
        runtime::scripting::importModuleNamespace(ctx, entryModuleName);
    if (JS_IsException(ns)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    JSValue script_exports = JS_UNDEFINED;
    if (scriptPath.empty()) {
        script_exports = ns;
    } else {
        JSValue atlas_scripts = JS_GetPropertyStr(ctx, ns, "default");
        JS_FreeValue(ctx, ns);
        if (JS_IsException(atlas_scripts)) {
            runtime::scripting::dumpExecution(ctx);
            return nullptr;
        }

        script_exports =
            JS_GetPropertyStr(ctx, atlas_scripts, scriptPath.c_str());
        JS_FreeValue(ctx, atlas_scripts);
        if (JS_IsException(script_exports)) {
            runtime::scripting::dumpExecution(ctx);
            return nullptr;
        }

        if (JS_IsUndefined(script_exports)) {
            std::cerr << "Script exports not found for path: " << scriptPath
                      << "\n";
            JS_FreeValue(ctx, script_exports);
            return nullptr;
        }
    }

    JSValue ctor = JS_GetPropertyStr(ctx, script_exports, className.c_str());
    JS_FreeValue(ctx, script_exports);
    if (JS_IsException(ctor)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    if (!JS_IsFunction(ctx, ctor)) {
        std::cerr << "Export '" << className
                  << "' is not a constructor/function\n";
        JS_FreeValue(ctx, ctor);
        return nullptr;
    }

    JSValue obj = JS_CallConstructor(ctx, ctor, 0, nullptr);
    JS_FreeValue(ctx, ctor);
    if (JS_IsException(obj)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    auto *inst = new ScriptInstance{};
    inst->ctx = ctx;
    inst->instance = obj;
    return inst;
}

JSValue runtime::scripting::jsPrint(JSContext *ctx,
                                    [[maybe_unused]] JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            std::cout << str;
            JS_FreeCString(ctx, str);
        } else {
            std::cout << "<non-string value>";
        }
        if (i < argc - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    return JS_UNDEFINED;
}
