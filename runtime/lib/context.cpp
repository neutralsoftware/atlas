//
// context.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Context settings for the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/audio.h"
#include "atlas/effect.h"
#include "atlas/input.h"
#include "atlas/object.h"
#include "atlas/particle.h"
#include "atlas/physics.h"
#include "atlas/runtime/scripting.h"
#include "atlas/texture.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "aurora/procedural.h"
#include "aurora/terrain.h"
#include "atlas/runtime/atlasScripts.h"
#include "hydra/fluid.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <json.hpp>
#include <string>
#include <toml.hpp>
#include <unordered_set>
#include <variant>
#include <vector>

namespace {

#define JSON_READ_BOOL(node, key, target)                                      \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_boolean()) {  \
            (target) = _atlas_json_it->get<bool>();                            \
        }                                                                      \
    } while (0)

#define JSON_READ_FLOAT(node, key, target)                                     \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_number()) {   \
            (target) = _atlas_json_it->get<float>();                           \
        }                                                                      \
    } while (0)

#define JSON_READ_INT(node, key, target)                                       \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_number()) {   \
            (target) = _atlas_json_it->get<int>();                             \
        }                                                                      \
    } while (0)

#define JSON_READ_STRING(node, key, target)                                    \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_string()) {   \
            (target) = _atlas_json_it->get<std::string>();                     \
        }                                                                      \
    } while (0)

struct JsonDefinition {
    json data;
    std::string baseDir;
};

struct MaterialDefinition {
    Material material;
    std::vector<Texture> textures;
};

struct PendingComponent {
    GameObject *object = nullptr;
    std::string objectType;
    std::string baseDir;
    json data;
};

struct RuntimeEnvironmentDefinition {
    Environment environment;
    Atmosphere atmosphere;
    bool automaticAmbient = false;
    bool useAtmosphereSkybox = false;
    bool useGlobalLight = false;
    bool atmosphereCastsShadows = false;
    int atmosphereShadowResolution = 4096;
};

class RuntimeScriptComponent final : public Component {
  public:
    Context *host = nullptr;
    std::string source;
    std::string className;
    std::string entryModuleName;
    json variables;
    std::string traitedType;
    bool isTrait = false;
    std::unique_ptr<ScriptInstance> instance;
    bool initialized = false;

    void atAttach() override {
        if (!ensureInstance()) {
            throw std::runtime_error("Failed to create script instance: " +
                                     className);
        }
        instance->callMethod("atAttach", 0, nullptr);
    }

    void init() override {
        if (!ensureInstance() || initialized) {
            return;
        }
        initialized = true;
        instance->callMethod("init", 0, nullptr);
    }

    void update(float deltaTime) override {
        if (!ensureInstance()) {
            return;
        }

        JSValue delta = JS_NewFloat64(host->context, deltaTime);
        JSValueConst args[] = {delta};
        instance->callMethod("update", 1, args);
        JS_FreeValue(host->context, delta);
    }

    void beforePhysics() override {
        if (!ensureInstance()) {
            return;
        }
        instance->callMethod("beforePhysics", 0, nullptr);
    }

  private:
    bool ensureInstance() {
        if (instance != nullptr) {
            return true;
        }
        if (host == nullptr || host->context == nullptr || className.empty()) {
            return false;
        }

        const std::string moduleName = entryModuleName.empty()
                                           ? host->scriptBundleModuleName
                                           : entryModuleName;
        instance.reset(runtime::scripting::createScriptInstance(
            host->context, moduleName, source, className));
        if (instance == nullptr) {
            return false;
        }

        if (!variables.is_null()) {
            const std::string serialized = variables.dump();
            JSValue parsed =
                JS_ParseJSON(host->context, serialized.c_str(),
                             serialized.size(), "<atlas:variables>");
            if (JS_IsException(parsed)) {
                runtime::scripting::dumpExecution(host->context);
                JS_FreeValue(host->context, parsed);
            } else {
                JS_SetPropertyStr(host->context, instance->instance,
                                  "variables", parsed);
            }
        }

        runtime::scripting::registerComponentInstance(
            host->context, host->scriptHost, this, object->getId(), className,
            instance->instance);

        return true;
    }
};

constexpr const char *RUNTIME_SCRIPT_BUNDLE_PATH = "dist/scripts.js";
constexpr const char *RUNTIME_FILE_MODULE_PREFIX = "__atlas_file__/";

std::string serializableObjectName(const Context &context, GameObject &object);
std::string serializableObjectReference(const Context &context,
                                        GameObject &object);

std::string normalizeScriptPath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string packScriptSource(const AtlasPackedScriptSource &source) {
    std::string result;
    for (std::size_t index = 0; index < source.count; ++index) {
        result += source.parts[index];
    }
    return result;
}

std::string readTextFile(const std::string &path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::string inferScriptClassName(const std::string &path) {
    std::string stem = std::filesystem::path(path).stem().string();
    if (!stem.empty()) {
        stem[0] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(stem[0])));
    }
    return stem;
}

void registerBuiltInScriptModules(Context &context) {
    for (std::size_t index = 0; index < ATLAS_RUNTIME_SCRIPT_MODULE_COUNT;
         ++index) {
        const auto &module = ATLAS_RUNTIME_SCRIPT_MODULES[index];
        context.scriptHost.modules[module.name] =
            packScriptSource(module.source);
    }
}

const json *findField(const json &node,
                      std::initializer_list<const char *> keys) {
    if (!node.is_object()) {
        return nullptr;
    }
    for (const char *key : keys) {
        auto it = node.find(key);
        if (it != node.end()) {
            return &(*it);
        }
    }
    return nullptr;
}

bool tryReadStringAny(const json &node,
                      std::initializer_list<const char *> keys,
                      std::string &target) {
    if (const json *field = findField(node, keys);
        field != nullptr && field->is_string()) {
        target = field->get<std::string>();
        return true;
    }
    return false;
}

bool tryReadBoolAny(const json &node, std::initializer_list<const char *> keys,
                    bool &target) {
    if (const json *field = findField(node, keys);
        field != nullptr && field->is_boolean()) {
        target = field->get<bool>();
        return true;
    }
    return false;
}

bool tryReadFloatAny(const json &node, std::initializer_list<const char *> keys,
                     float &target) {
    if (const json *field = findField(node, keys);
        field != nullptr && field->is_number()) {
        target = field->get<float>();
        return true;
    }
    return false;
}

bool tryReadIntAny(const json &node, std::initializer_list<const char *> keys,
                   int &target) {
    if (const json *field = findField(node, keys);
        field != nullptr && field->is_number()) {
        target = field->get<int>();
        return true;
    }
    return false;
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

std::string resolveRuntimePath(const std::string &baseDir,
                               const std::string &path) {
    const std::filesystem::path candidate(path);
    if (candidate.is_absolute()) {
        return candidate.lexically_normal().string();
    }
    return (std::filesystem::path(baseDir) / candidate)
        .lexically_normal()
        .string();
}

std::string stripJsonComments(const std::string &text) {
    std::string result;
    result.reserve(text.size());

    bool inString = false;
    bool escaping = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        const char next = i + 1 < text.size() ? text[i + 1] : '\0';

        if (inLineComment) {
            if (ch == '\n') {
                inLineComment = false;
                result.push_back(ch);
            }
            continue;
        }

        if (inBlockComment) {
            if (ch == '\n') {
                result.push_back(ch);
                continue;
            }
            if (ch == '*' && next == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }

        if (inString) {
            result.push_back(ch);
            if (escaping) {
                escaping = false;
            } else if (ch == '\\') {
                escaping = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
            result.push_back(ch);
            continue;
        }

        if (ch == '/' && next == '/') {
            inLineComment = true;
            ++i;
            continue;
        }

        if (ch == '/' && next == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }

        result.push_back(ch);
    }

    return result;
}

std::string stripTrailingJsonCommas(const std::string &text) {
    std::string result;
    result.reserve(text.size());

    bool inString = false;
    bool escaping = false;

    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];

        if (inString) {
            result.push_back(ch);
            if (escaping) {
                escaping = false;
            } else if (ch == '\\') {
                escaping = true;
            } else if (ch == '"') {
                inString = false;
            }
            continue;
        }

        if (ch == '"') {
            inString = true;
            result.push_back(ch);
            continue;
        }

        if (ch == ',') {
            size_t nextIndex = i + 1;
            while (nextIndex < text.size() &&
                   std::isspace(static_cast<unsigned char>(text[nextIndex]))) {
                ++nextIndex;
            }

            if (nextIndex < text.size() &&
                (text[nextIndex] == '}' || text[nextIndex] == ']')) {
                continue;
            }
        }

        result.push_back(ch);
    }

    return result;
}

json loadJsonFile(const std::string &path) {
    const std::string text = readTextFile(path);
    const std::string sanitized =
        stripTrailingJsonCommas(stripJsonComments(text));
    return json::parse(sanitized);
}

JsonDefinition loadJsonDefinition(const json &value,
                                  const std::string &baseDir) {
    if (value.is_string()) {
        const std::string relativePath = value.get<std::string>();
        if (relativePath.empty()) {
            throw std::runtime_error("Expected a non-empty JSON definition "
                                     "path");
        }
        const std::string resolvedPath =
            resolveRuntimePath(baseDir, relativePath);
        return {
            .data = loadJsonFile(resolvedPath),
            .baseDir =
                std::filesystem::path(resolvedPath).parent_path().string(),
        };
    }
    return {.data = value, .baseDir = baseDir};
}

bool isEmptyStringValue(const json &value) {
    return value.is_string() && value.get<std::string>().empty();
}

bool tryReadVec3(const json &node, const char *key, Position3d &target) {
    auto it = node.find(key);
    if (it == node.end() || !it->is_array() || it->size() != 3) {
        return false;
    }
    target = Position3d((*it)[0].get<float>(), (*it)[1].get<float>(),
                        (*it)[2].get<float>());
    return true;
}

bool tryReadVec3Any(const json &node, std::initializer_list<const char *> keys,
                    Position3d &target) {
    for (const char *key : keys) {
        if (tryReadVec3(node, key, target)) {
            return true;
        }
    }
    return false;
}

bool tryReadVec2(const json &node, const char *key, Position2d &target) {
    auto it = node.find(key);
    if (it == node.end() || !it->is_array() || it->size() != 2) {
        return false;
    }
    target = Position2d{(*it)[0].get<float>(), (*it)[1].get<float>()};
    return true;
}

bool tryReadVec2Any(const json &node, std::initializer_list<const char *> keys,
                    Position2d &target) {
    for (const char *key : keys) {
        if (tryReadVec2(node, key, target)) {
            return true;
        }
    }
    return false;
}

Color parseColor(const json &value) {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    if (value.is_array() && (value.size() == 3 || value.size() == 4)) {
        r = value[0].get<float>();
        g = value[1].get<float>();
        b = value[2].get<float>();
        if (value.size() == 4) {
            a = value[3].get<float>();
        }
    } else if (value.is_object()) {
        JSON_READ_FLOAT(value, "r", r);
        JSON_READ_FLOAT(value, "g", g);
        JSON_READ_FLOAT(value, "b", b);
        JSON_READ_FLOAT(value, "a", a);
    } else {
        throw std::runtime_error("Invalid color value");
    }

    const float maxComponent = std::max(std::max(r, g), std::max(b, a));
    if (maxComponent > 1.0f) {
        r /= 255.0f;
        g /= 255.0f;
        b /= 255.0f;
        a /= 255.0f;
    }

    return Color{.r = r, .g = g, .b = b, .a = a};
}

bool tryReadColor(const json &node, const char *key, Color &target) {
    auto it = node.find(key);
    if (it == node.end()) {
        return false;
    }
    target = parseColor(*it);
    return true;
}

bool tryReadColorAny(const json &node, std::initializer_list<const char *> keys,
                     Color &target) {
    for (const char *key : keys) {
        if (tryReadColor(node, key, target)) {
            return true;
        }
    }
    return false;
}

std::string makeRuntimeResourceName(const std::string &prefix,
                                    const std::string &resolvedPath) {
    return prefix + ":" +
           std::filesystem::path(resolvedPath).lexically_normal().string();
}

ResourceType resourceTypeForTextureType(TextureType type) {
    if (type == TextureType::Specular) {
        return ResourceType::SpecularMap;
    }
    return ResourceType::Image;
}

TextureWrappingMode parseTextureWrappingMode(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "repeat") {
        return TextureWrappingMode::Repeat;
    }
    if (token == "mirroredrepeat" || token == "mirror") {
        return TextureWrappingMode::MirroredRepeat;
    }
    if (token == "clamptoedge" || token == "edge") {
        return TextureWrappingMode::ClampToEdge;
    }
    if (token == "clamptoborder" || token == "border") {
        return TextureWrappingMode::ClampToBorder;
    }
    throw std::runtime_error("Unknown texture wrapping mode: " + value);
}

TextureFilteringMode parseTextureFilteringMode(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "nearest") {
        return TextureFilteringMode::Nearest;
    }
    if (token == "linear") {
        return TextureFilteringMode::Linear;
    }
    throw std::runtime_error("Unknown texture filtering mode: " + value);
}

TextureType parseTextureTypeString(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "color" || token == "albedo" || token == "diffuse") {
        return TextureType::Color;
    }
    if (token == "specular") {
        return TextureType::Specular;
    }
    if (token == "cubemap") {
        return TextureType::Cubemap;
    }
    if (token == "normal" || token == "normalmap") {
        return TextureType::Normal;
    }
    if (token == "metallic" || token == "metalness") {
        return TextureType::Metallic;
    }
    if (token == "roughness") {
        return TextureType::Roughness;
    }
    if (token == "ao" || token == "ambientocclusion") {
        return TextureType::AO;
    }
    if (token == "opacity" || token == "alpha") {
        return TextureType::Opacity;
    }
    if (token == "depth") {
        return TextureType::Depth;
    }
    if (token == "hdr") {
        return TextureType::HDR;
    }
    throw std::runtime_error("Unknown texture type: " + value);
}

Resource createRuntimeResource(const std::string &baseDir,
                               const std::string &path, ResourceType type,
                               const std::string &prefix) {
    const std::string resolvedPath = resolveRuntimePath(baseDir, path);
    return Workspace::get().createResource(
        resolvedPath, makeRuntimeResourceName(prefix, resolvedPath), type);
}

Texture loadTextureDefinition(const json &value, const std::string &baseDir,
                              TextureType defaultType, bool allowOverride) {
    std::string path;
    TextureType type = defaultType;
    TextureParameters params;
    Color borderColor = {0.0f, 0.0f, 0.0f, 0.0f};

    if (value.is_string()) {
        path = value.get<std::string>();
    } else if (value.is_object()) {
        tryReadStringAny(value, {"path", "source"}, path);

        if (allowOverride) {
            std::string typeName;
            if (tryReadStringAny(value, {"textureType", "type"}, typeName)) {
                type = parseTextureTypeString(typeName);
            }
        }

        std::string wrapping;
        if (tryReadStringAny(value, {"wrappingModeS", "wrapS"}, wrapping)) {
            params.wrappingModeS = parseTextureWrappingMode(wrapping);
        }
        if (tryReadStringAny(value, {"wrappingModeT", "wrapT"}, wrapping)) {
            params.wrappingModeT = parseTextureWrappingMode(wrapping);
        }

        std::string filtering;
        if (tryReadStringAny(value, {"minifyingFilter", "minFilter"},
                             filtering)) {
            params.minifyingFilter = parseTextureFilteringMode(filtering);
        }
        if (tryReadStringAny(value, {"magnifyingFilter", "magFilter"},
                             filtering)) {
            params.magnifyingFilter = parseTextureFilteringMode(filtering);
        }

        tryReadColorAny(value, {"borderColor"}, borderColor);
    } else {
        throw std::runtime_error("Invalid texture definition");
    }

    if (path.empty()) {
        throw std::runtime_error("Texture definition is missing a source path");
    }

    if (type == TextureType::Cubemap) {
        throw std::runtime_error("Cubemap textures are not supported in this "
                                 "material slot");
    }

    Resource resource = createRuntimeResource(
        baseDir, path, resourceTypeForTextureType(type), "runtime-texture");
    return Texture::fromResource(resource, type, params, borderColor);
}

MaterialDefinition loadMaterialDefinition(const json &value,
                                          const std::string &baseDir) {
    JsonDefinition definition = loadJsonDefinition(value, baseDir);
    const json *materialNode = findField(definition.data, {"material"});
    const json &materialData =
        materialNode != nullptr && materialNode->is_object() ? *materialNode
                                                             : definition.data;

    if (!materialData.is_object()) {
        throw std::runtime_error("Material definition must be an object");
    }

    MaterialDefinition loaded;

    tryReadColorAny(materialData, {"albedo", "color", "baseColor"},
                    loaded.material.albedo);
    tryReadColorAny(materialData,
                    {"emissiveColor", "emissive", "emissionColor"},
                    loaded.material.emissiveColor);
    tryReadFloatAny(materialData, {"metallic"}, loaded.material.metallic);
    tryReadFloatAny(materialData, {"roughness"}, loaded.material.roughness);
    tryReadFloatAny(materialData, {"ao", "ambientOcclusion"},
                    loaded.material.ao);
    tryReadFloatAny(materialData, {"reflectivity"},
                    loaded.material.reflectivity);
    tryReadFloatAny(materialData, {"emissiveIntensity", "emissionIntensity"},
                    loaded.material.emissiveIntensity);
    tryReadFloatAny(materialData, {"normalMapStrength"},
                    loaded.material.normalMapStrength);
    tryReadBoolAny(materialData, {"useNormalMap"},
                   loaded.material.useNormalMap);
    tryReadFloatAny(materialData, {"transmittance"},
                    loaded.material.transmittance);
    tryReadFloatAny(materialData, {"ior"}, loaded.material.ior);

    auto appendTexture = [&](std::initializer_list<const char *> keys,
                             TextureType type, bool allowTypeOverride) {
        if (const json *field = findField(materialData, keys);
            field != nullptr && !isEmptyStringValue(*field)) {
            loaded.textures.push_back(loadTextureDefinition(
                *field, definition.baseDir, type, allowTypeOverride));
        }
    };

    appendTexture(
        {"texture", "albedoTexture", "colorTexture", "diffuseTexture"},
        TextureType::Color, false);
    appendTexture({"specularTexture", "specularMap"}, TextureType::Specular,
                  false);
    appendTexture({"normalTexture", "normalMap"}, TextureType::Normal, false);
    appendTexture({"metallicTexture", "metalnessTexture"},
                  TextureType::Metallic, false);
    appendTexture({"roughnessTexture"}, TextureType::Roughness, false);
    appendTexture({"aoTexture", "ambientOcclusionTexture"}, TextureType::AO,
                  false);
    appendTexture({"opacityTexture", "alphaTexture"}, TextureType::Opacity,
                  false);

    if (const json *texturesField = findField(materialData, {"textures"});
        texturesField != nullptr && texturesField->is_array()) {
        for (const auto &textureData : *texturesField) {
            loaded.textures.push_back(loadTextureDefinition(
                textureData, definition.baseDir, TextureType::Color, true));
        }
    }

    return loaded;
}

WeatherCondition parseWeatherCondition(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "clear") {
        return WeatherCondition::Clear;
    }
    if (token == "rain") {
        return WeatherCondition::Rain;
    }
    if (token == "snow") {
        return WeatherCondition::Snow;
    }
    if (token == "storm" || token == "thunderstorm") {
        return WeatherCondition::Storm;
    }
    throw std::runtime_error("Unknown weather condition: " + value);
}

RuntimeEnvironmentDefinition
loadEnvironmentDefinition(const json &sceneData, const std::string &baseDir) {
    RuntimeEnvironmentDefinition loaded;

    const json *environmentNode = findField(sceneData, {"environment"});
    if (environmentNode == nullptr) {
        return loaded;
    }
    if (!environmentNode->is_object()) {
        throw std::runtime_error("Environment definition must be an object");
    }

    const json &environmentData = *environmentNode;

    if (const json *fogNode = findField(environmentData, {"fog"});
        fogNode != nullptr) {
        if (!fogNode->is_object()) {
            throw std::runtime_error("Environment fog must be an object");
        }
        tryReadColorAny(*fogNode, {"color"}, loaded.environment.fog.color);
        tryReadFloatAny(*fogNode, {"intensity"},
                        loaded.environment.fog.intensity);
    }

    if (const json *volumetricNode =
            findField(environmentData, {"volumetricLighting", "volumetric"});
        volumetricNode != nullptr) {
        if (!volumetricNode->is_object()) {
            throw std::runtime_error(
                "Environment volumetric lighting must be an object");
        }
        tryReadBoolAny(*volumetricNode, {"enabled"},
                       loaded.environment.volumetricLighting.enabled);
        tryReadFloatAny(*volumetricNode, {"density"},
                        loaded.environment.volumetricLighting.density);
        tryReadFloatAny(*volumetricNode, {"weight"},
                        loaded.environment.volumetricLighting.weight);
        tryReadFloatAny(*volumetricNode, {"decay"},
                        loaded.environment.volumetricLighting.decay);
        tryReadFloatAny(*volumetricNode, {"exposure"},
                        loaded.environment.volumetricLighting.exposure);
    }

    if (const json *bloomNode =
            findField(environmentData, {"lightBloom", "bloom"});
        bloomNode != nullptr) {
        if (!bloomNode->is_object()) {
            throw std::runtime_error("Environment light bloom must be an "
                                     "object");
        }
        tryReadFloatAny(*bloomNode, {"radius"},
                        loaded.environment.lightBloom.radius);
        tryReadIntAny(*bloomNode, {"maxSamples"},
                      loaded.environment.lightBloom.maxSamples);
    }

    if (const json *rimNode = findField(environmentData, {"rimLight"});
        rimNode != nullptr) {
        if (!rimNode->is_object()) {
            throw std::runtime_error("Environment rim light must be an object");
        }
        tryReadFloatAny(*rimNode, {"intensity"},
                        loaded.environment.rimLight.intensity);
        tryReadColorAny(*rimNode, {"color"}, loaded.environment.rimLight.color);
    }

    if (const json *lookupNode =
            findField(environmentData, {"lookupTexture", "lutTexture", "lut"});
        lookupNode != nullptr && !lookupNode->is_null() &&
        !isEmptyStringValue(*lookupNode)) {
        loaded.environment.lookupTexture = loadTextureDefinition(
            *lookupNode, baseDir, TextureType::Color, false);
    }

    tryReadBoolAny(environmentData, {"automaticAmbient"},
                   loaded.automaticAmbient);
    tryReadBoolAny(environmentData,
                   {"atmosphereSky", "useAtmosphereSkybox", "proceduralSky",
                    "proceduralSkybox"},
                   loaded.useAtmosphereSkybox);

    const json *atmosphereNode = findField(environmentData, {"atmosphere"});
    if (atmosphereNode != nullptr && !atmosphereNode->is_object()) {
        throw std::runtime_error("Environment atmosphere must be an object");
    }
    const json &atmosphereData =
        atmosphereNode != nullptr ? *atmosphereNode : environmentData;

    bool hasAtmosphereConfiguration = false;
    bool explicitAtmosphereEnabled = false;
    bool atmosphereEnabled = false;

    if (tryReadBoolAny(atmosphereData, {"enabled"}, atmosphereEnabled)) {
        explicitAtmosphereEnabled = true;
        hasAtmosphereConfiguration = true;
    }

    if (tryReadFloatAny(atmosphereData, {"timeOfDay", "time"},
                        loaded.atmosphere.timeOfDay)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"secondsPerHour"},
                        loaded.atmosphere.secondsPerHour)) {
        hasAtmosphereConfiguration = true;
    }

    Position3d vector;
    if (tryReadVec3Any(atmosphereData, {"wind"}, vector)) {
        loaded.atmosphere.wind = vector;
        hasAtmosphereConfiguration = true;
    }

    if (tryReadColorAny(atmosphereData, {"sunColor"},
                        loaded.atmosphere.sunColor)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadColorAny(atmosphereData, {"moonColor"},
                        loaded.atmosphere.moonColor)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"sunSize"},
                        loaded.atmosphere.sunSize)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"moonSize"},
                        loaded.atmosphere.moonSize)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"sunTintStrength"},
                        loaded.atmosphere.sunTintStrength)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"moonTintStrength"},
                        loaded.atmosphere.moonTintStrength)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadFloatAny(atmosphereData, {"starIntensity"},
                        loaded.atmosphere.starIntensity)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadBoolAny(atmosphereData, {"cycle"}, loaded.atmosphere.cycle)) {
        hasAtmosphereConfiguration = true;
    }

    if (const json *globalLightNode =
            findField(atmosphereData, {"globalLight", "sunLight"});
        globalLightNode != nullptr) {
        hasAtmosphereConfiguration = true;
        if (globalLightNode->is_boolean()) {
            loaded.useGlobalLight = globalLightNode->get<bool>();
        } else if (globalLightNode->is_object()) {
            loaded.useGlobalLight = true;
            tryReadBoolAny(*globalLightNode, {"enabled"},
                           loaded.useGlobalLight);
            tryReadBoolAny(*globalLightNode, {"castsShadows", "castShadows"},
                           loaded.atmosphereCastsShadows);
            tryReadIntAny(*globalLightNode, {"shadowResolution"},
                          loaded.atmosphereShadowResolution);
        } else {
            throw std::runtime_error(
                "Atmosphere globalLight must be a boolean or object");
        }
    }

    if (tryReadBoolAny(atmosphereData,
                       {"useGlobalLight", "globalDirectionalLight"},
                       loaded.useGlobalLight)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadBoolAny(atmosphereData,
                       {"castsShadows", "castShadows", "sunShadows"},
                       loaded.atmosphereCastsShadows)) {
        hasAtmosphereConfiguration = true;
    }
    if (tryReadIntAny(atmosphereData, {"shadowResolution"},
                      loaded.atmosphereShadowResolution)) {
        hasAtmosphereConfiguration = true;
    }

    if (const json *cloudsNode = findField(atmosphereData, {"clouds"});
        cloudsNode != nullptr) {
        hasAtmosphereConfiguration = true;
        bool enableClouds = true;

        if (cloudsNode->is_boolean()) {
            enableClouds = cloudsNode->get<bool>();
        } else if (cloudsNode->is_object()) {
            tryReadBoolAny(*cloudsNode, {"enabled"}, enableClouds);
        } else {
            throw std::runtime_error("Atmosphere clouds must be a boolean or "
                                     "object");
        }

        if (enableClouds) {
            int frequency = 4;
            int divisions = 6;

            if (cloudsNode->is_object()) {
                tryReadIntAny(*cloudsNode, {"frequency"}, frequency);
                tryReadIntAny(*cloudsNode, {"divisions", "numberOfDivisions"},
                              divisions);
            }

            loaded.atmosphere.addClouds(frequency, divisions);

            if (loaded.atmosphere.clouds != nullptr &&
                cloudsNode->is_object()) {
                auto &clouds = *loaded.atmosphere.clouds;
                if (tryReadVec3Any(*cloudsNode, {"position"}, vector)) {
                    clouds.position = vector;
                }
                if (tryReadVec3Any(*cloudsNode, {"size"}, vector)) {
                    clouds.size = vector;
                }
                if (tryReadVec3Any(*cloudsNode, {"offset"}, vector)) {
                    clouds.offset = vector;
                }
                if (tryReadVec3Any(*cloudsNode, {"wind"}, vector)) {
                    clouds.wind = vector;
                }

                tryReadFloatAny(*cloudsNode, {"scale"}, clouds.scale);
                tryReadFloatAny(*cloudsNode, {"density"}, clouds.density);
                tryReadFloatAny(*cloudsNode, {"densityMultiplier"},
                                clouds.densityMultiplier);
                tryReadFloatAny(*cloudsNode, {"absorption"}, clouds.absorption);
                tryReadFloatAny(*cloudsNode, {"scattering"}, clouds.scattering);
                tryReadFloatAny(*cloudsNode, {"phase"}, clouds.phase);
                tryReadFloatAny(*cloudsNode, {"clusterStrength"},
                                clouds.clusterStrength);
                tryReadIntAny(*cloudsNode, {"primaryStepCount"},
                              clouds.primaryStepCount);
                tryReadIntAny(*cloudsNode, {"lightStepCount"},
                              clouds.lightStepCount);
                tryReadFloatAny(*cloudsNode, {"lightStepMultiplier"},
                                clouds.lightStepMultiplier);
                tryReadFloatAny(*cloudsNode, {"minStepLength"},
                                clouds.minStepLength);
            }
        }
    }

    if (const json *weatherNode = findField(atmosphereData, {"weather"});
        weatherNode != nullptr) {
        hasAtmosphereConfiguration = true;
        bool enableWeather = true;
        WeatherState state;
        state.wind = loaded.atmosphere.wind;

        if (weatherNode->is_boolean()) {
            enableWeather = weatherNode->get<bool>();
        } else if (weatherNode->is_object()) {
            tryReadBoolAny(*weatherNode, {"enabled"}, enableWeather);

            std::string condition;
            if (tryReadStringAny(*weatherNode, {"condition", "type"},
                                 condition)) {
                state.condition = parseWeatherCondition(condition);
            }
            tryReadFloatAny(*weatherNode, {"intensity"}, state.intensity);
            if (tryReadVec3Any(*weatherNode, {"wind"}, vector)) {
                state.wind = vector;
            }
        } else {
            throw std::runtime_error("Atmosphere weather must be a boolean or "
                                     "object");
        }

        if (enableWeather) {
            loaded.atmosphere.weatherDelegate = [state](ViewInformation) {
                return state;
            };
            loaded.atmosphere.enableWeather();
        }
    }

    const bool shouldEnableAtmosphere =
        loaded.useAtmosphereSkybox || loaded.useGlobalLight ||
        (explicitAtmosphereEnabled ? atmosphereEnabled
                                   : hasAtmosphereConfiguration);

    if (shouldEnableAtmosphere) {
        loaded.atmosphere.enable();
    } else {
        loaded.atmosphere.disable();
    }

    return loaded;
}

Normal3d normalizeVector(const Position3d &vector, const Normal3d &fallback) {
    if (std::fabs(vector.x) < 1.0e-6f && std::fabs(vector.y) < 1.0e-6f &&
        std::fabs(vector.z) < 1.0e-6f) {
        return fallback;
    }
    return Position3d{vector.x, vector.y, vector.z}.normalized();
}

std::shared_ptr<TerrainGenerator>
parseTerrainGenerator(const json &value, const std::string &baseDir) {
    JsonDefinition definition = loadJsonDefinition(value, baseDir);
    if (!definition.data.is_object()) {
        throw std::runtime_error("Terrain generator definition must be an "
                                 "object");
    }

    std::string algorithm;
    tryReadStringAny(definition.data, {"algorithm", "type", "generator"},
                     algorithm);
    if (algorithm.empty()) {
        throw std::runtime_error("Terrain generator is missing an algorithm");
    }

    const std::string token = normalizeToken(algorithm);
    const json *settingsNode = findField(definition.data, {"settings"});
    const json &settings = settingsNode != nullptr && settingsNode->is_object()
                               ? *settingsNode
                               : definition.data;

    if (token == "hill" || token == "hills" || token == "perlin" ||
        token == "perlinnoise") {
        float scale = 0.01f;
        float amplitude = 10.0f;
        tryReadFloatAny(settings, {"scale"}, scale);
        tryReadFloatAny(settings, {"amplitude", "height"}, amplitude);
        return std::make_shared<HillGenerator>(scale, amplitude);
    }

    if (token == "plain" || token == "plains") {
        float scale = 0.02f;
        float amplitude = 2.0f;
        tryReadFloatAny(settings, {"scale"}, scale);
        tryReadFloatAny(settings, {"amplitude", "height"}, amplitude);
        return std::make_shared<PlainGenerator>(scale, amplitude);
    }

    if (token == "mountain" || token == "mountains" || token == "fractal" ||
        token == "fractalnoise" || token == "simplex" ||
        token == "simplexnoise" || token == "diamondsquare") {
        float scale = 10.0f;
        float amplitude = 100.0f;
        int octaves = 5;
        float persistence = 0.5f;
        tryReadFloatAny(settings, {"scale"}, scale);
        tryReadFloatAny(settings, {"amplitude", "height"}, amplitude);
        tryReadIntAny(settings, {"octaves"}, octaves);
        tryReadFloatAny(settings, {"persistence"}, persistence);
        return std::make_shared<MountainGenerator>(scale, amplitude, octaves,
                                                   persistence);
    }

    if (token == "island" || token == "worley" || token == "worleynoise") {
        int numFeatures = 10;
        float scale = 0.01f;
        tryReadIntAny(settings, {"numFeatures", "features"}, numFeatures);
        tryReadFloatAny(settings, {"scale"}, scale);
        return std::make_shared<IslandGenerator>(numFeatures, scale);
    }

    if (token == "compound" || token == "combined" || token == "composite") {
        auto compound = std::make_shared<CompoundGenerator>();
        const json *generatorsNode =
            findField(settings, {"generators", "items"});
        if (generatorsNode == nullptr || !generatorsNode->is_array()) {
            throw std::runtime_error("Compound terrain generator requires a "
                                     "generators array");
        }

        for (const auto &childData : *generatorsNode) {
            auto child = parseTerrainGenerator(childData, definition.baseDir);
            if (auto hill = std::dynamic_pointer_cast<HillGenerator>(child)) {
                compound->addGenerator(*hill);
            } else if (auto plain =
                           std::dynamic_pointer_cast<PlainGenerator>(child)) {
                compound->addGenerator(*plain);
            } else if (auto mountain =
                           std::dynamic_pointer_cast<MountainGenerator>(
                               child)) {
                compound->addGenerator(*mountain);
            } else if (auto island =
                           std::dynamic_pointer_cast<IslandGenerator>(child)) {
                compound->addGenerator(*island);
            } else if (auto nestedCompound =
                           std::dynamic_pointer_cast<CompoundGenerator>(
                               child)) {
                compound->addGenerator(*nestedCompound);
            }
        }

        return compound;
    }

    throw std::runtime_error("Unknown terrain generator algorithm: " +
                             algorithm);
}

void appendBiomeList(Terrain &terrain, const json &value,
                     const std::string &baseDir) {
    JsonDefinition definition = loadJsonDefinition(value, baseDir);

    const json *biomesNode = nullptr;
    if (definition.data.is_array()) {
        biomesNode = &definition.data;
    } else if (definition.data.is_object()) {
        biomesNode = findField(definition.data, {"biomes"});
    }

    if (biomesNode == nullptr || !biomesNode->is_array()) {
        throw std::runtime_error(
            "Biome definition must provide a biomes array");
    }

    for (const auto &biomeData : *biomesNode) {
        if (!biomeData.is_object()) {
            throw std::runtime_error("Biome entry must be an object");
        }

        Biome biome("", Color());
        tryReadStringAny(biomeData, {"name"}, biome.name);
        tryReadColorAny(biomeData, {"color"}, biome.color);
        bool explicitUseTexture = biome.useTexture;
        bool hasExplicitUseTexture =
            tryReadBoolAny(biomeData, {"useTexture"}, explicitUseTexture);
        tryReadFloatAny(biomeData, {"minHeight"}, biome.minHeight);
        tryReadFloatAny(biomeData, {"maxHeight"}, biome.maxHeight);
        tryReadFloatAny(biomeData, {"minMoisture"}, biome.minMoisture);
        tryReadFloatAny(biomeData, {"maxMoisture"}, biome.maxMoisture);
        tryReadFloatAny(biomeData, {"minTemperature"}, biome.minTemperature);
        tryReadFloatAny(biomeData, {"maxTemperature"}, biome.maxTemperature);

        const json *textureField = findField(biomeData, {"texture"});
        if (textureField != nullptr && !isEmptyStringValue(*textureField)) {
            biome.attachTexture(loadTextureDefinition(
                *textureField, definition.baseDir, TextureType::Color, false));
        }

        if (hasExplicitUseTexture) {
            biome.useTexture = explicitUseTexture;
        }

        terrain.addBiome(biome);
    }
}

std::optional<std::string>
findCubemapFace(const std::string &directory,
                std::initializer_list<const char *> aliases) {
    const std::filesystem::path dirPath(directory);
    if (!std::filesystem::exists(dirPath) ||
        !std::filesystem::is_directory(dirPath)) {
        return std::nullopt;
    }

    std::vector<std::string> normalizedAliases;
    normalizedAliases.reserve(aliases.size());
    for (const char *alias : aliases) {
        normalizedAliases.push_back(normalizeToken(alias));
    }

    for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::string stem = normalizeToken(entry.path().stem().string());
        const std::string filename =
            normalizeToken(entry.path().filename().string());

        for (const auto &alias : normalizedAliases) {
            if (stem == alias || filename == alias ||
                stem.rfind(alias, 0) == 0) {
                return entry.path().string();
            }
        }
    }

    return std::nullopt;
}

Cubemap loadCubemapFromPaths(const std::array<std::string, 6> &paths,
                             const std::string &baseDir) {
    std::vector<Resource> resources;
    resources.reserve(paths.size());
    for (const auto &path : paths) {
        resources.push_back(createRuntimeResource(
            baseDir, path, ResourceType::Image, "runtime-cubemap-face"));
    }

    ResourceGroup group = Workspace::get().createResourceGroup(
        "runtime-cubemap:" + makeRuntimeResourceName("group", paths[0]),
        resources);
    return Cubemap::fromResourceGroup(group);
}

Cubemap loadCubemapDefinition(const json &value, const std::string &baseDir) {
    if (value.is_string()) {
        const std::string rawPath = value.get<std::string>();
        if (rawPath.empty()) {
            throw std::runtime_error("Skybox cubemap path cannot be empty");
        }

        const std::string resolvedPath = resolveRuntimePath(baseDir, rawPath);
        const std::filesystem::path path(resolvedPath);

        if (std::filesystem::is_directory(path)) {
            auto right = findCubemapFace(resolvedPath,
                                         {"px", "posx", "positivex", "right"});
            auto left = findCubemapFace(resolvedPath,
                                        {"nx", "negx", "negativex", "left"});
            auto top = findCubemapFace(
                resolvedPath, {"py", "posy", "positivey", "top", "up"});
            auto bottom = findCubemapFace(
                resolvedPath, {"ny", "negy", "negativey", "bottom", "down"});
            auto front = findCubemapFace(resolvedPath,
                                         {"pz", "posz", "positivez", "front"});
            auto back = findCubemapFace(resolvedPath,
                                        {"nz", "negz", "negativez", "back"});

            if (!right || !left || !top || !bottom || !front || !back) {
                throw std::runtime_error("Cubemap directory is missing one or "
                                         "more faces: " +
                                         resolvedPath);
            }

            return loadCubemapFromPaths(
                {*right, *left, *top, *bottom, *front, *back}, "");
        }

        const std::string extension = normalizeToken(path.extension().string());
        if (extension == "json" || extension == "acubemap") {
            return loadCubemapDefinition(
                loadJsonFile(resolvedPath),
                std::filesystem::path(resolvedPath).parent_path().string());
        }

        throw std::runtime_error("Cubemap paths must point to a directory or a "
                                 "JSON cubemap definition: " +
                                 resolvedPath);
    }

    if (value.is_array() && value.size() == 6) {
        if (value[0].is_string()) {
            std::array<std::string, 6> facePaths;
            for (size_t i = 0; i < facePaths.size(); ++i) {
                facePaths[i] = value[i].get<std::string>();
            }
            return loadCubemapFromPaths(facePaths, baseDir);
        }

        std::array<Color, 6> colors;
        for (size_t i = 0; i < colors.size(); ++i) {
            colors[i] = parseColor(value[i]);
        }
        return Cubemap::fromColors(colors, 1024);
    }

    if (value.is_object()) {
        if (const json *colorsField = findField(value, {"colors"});
            colorsField != nullptr && colorsField->is_array() &&
            colorsField->size() == 6) {
            std::array<Color, 6> colors;
            for (size_t i = 0; i < colors.size(); ++i) {
                colors[i] = parseColor((*colorsField)[i]);
            }
            int size = 1024;
            tryReadIntAny(value, {"size", "resolution"}, size);
            return Cubemap::fromColors(colors, size);
        }

        auto readFacePath = [&](std::initializer_list<const char *> keys)
            -> std::optional<std::string> {
            if (const json *field = findField(value, keys);
                field != nullptr && field->is_string()) {
                return field->get<std::string>();
            }
            return std::nullopt;
        };

        auto right =
            readFacePath({"px", "posx", "positiveX", "right", "rightFace"});
        auto left =
            readFacePath({"nx", "negx", "negativeX", "left", "leftFace"});
        auto top = readFacePath({"py", "posy", "positiveY", "top", "up"});
        auto bottom =
            readFacePath({"ny", "negy", "negativeY", "bottom", "down"});
        auto front =
            readFacePath({"pz", "posz", "positiveZ", "front", "frontFace"});
        auto back =
            readFacePath({"nz", "negz", "negativeZ", "back", "backFace"});

        if (right && left && top && bottom && front && back) {
            return loadCubemapFromPaths(
                {*right, *left, *top, *bottom, *front, *back}, baseDir);
        }

        if (const json *faces = findField(value, {"faces", "cubemap"});
            faces != nullptr && faces != &value) {
            return loadCubemapDefinition(*faces, baseDir);
        }
    }

    throw std::runtime_error("Invalid cubemap definition");
}

void registerObjectReference(Context &context, const std::string &reference,
                             GameObject *object) {
    if (reference.empty() || object == nullptr) {
        return;
    }

    auto registerSingle = [&](const std::string &key) {
        if (key.empty()) {
            return;
        }
        auto it = context.objectReferences.find(key);
        if (it != context.objectReferences.end() && it->second != object) {
            throw std::runtime_error("Duplicate object reference: " + key);
        }
        context.objectReferences[key] = object;
    };

    registerSingle(reference);
    registerSingle(normalizeToken(reference));
}

void registerGameObject(Context &context, GameObject &object,
                        const json &objectData, const std::string &objectType,
                        size_t generatedIndex) {
    std::string name;
    tryReadStringAny(objectData, {"name"}, name);
    if (name.empty()) {
        name = objectType + "_" + std::to_string(generatedIndex);
    }

    object.name = name;
    registerObjectReference(context, name, &object);
    context.objectNames[object.getId()] = name;
    context.objectSceneReferences[object.getId()] = name;
    context.objectSceneTypes[object.getId()] = objectType;
    if (objectType == "solid") {
        std::string solidType;
        tryReadStringAny(objectData, {"solid_type", "solidType"}, solidType);
        context.objectSceneSolidTypes[object.getId()] =
            normalizeToken(solidType);
    }

    if (const json *idField = findField(objectData, {"id"});
        idField != nullptr) {
        if (idField->is_string()) {
            std::string sceneReference = idField->get<std::string>();
            context.objectSceneReferences[object.getId()] = sceneReference;
            registerObjectReference(context, sceneReference, &object);
        } else if (idField->is_number_integer()) {
            std::string sceneReference = std::to_string(idField->get<int>());
            context.objectSceneReferences[object.getId()] = sceneReference;
            registerObjectReference(context, sceneReference, &object);
        }
    }

    if (const json *parentField = findField(objectData, {"parent"});
        parentField != nullptr) {
        if (parentField->is_string()) {
            context.objectParentReferences[object.getId()] =
                parentField->get<std::string>();
        } else if (parentField->is_number_integer()) {
            context.objectParentReferences[object.getId()] =
                std::to_string(parentField->get<int>());
        }
    }

    registerObjectReference(context, std::to_string(object.getId()), &object);
}

void applyTransform(GameObject &object, const json &objectData) {
    Position3d position;
    if (tryReadVec3Any(objectData, {"position"}, position)) {
        object.setPosition(position);
    }

    Position3d rotationValues;
    if (tryReadVec3Any(objectData, {"rotation"}, rotationValues)) {
        object.setRotation(
            Rotation3d{rotationValues.x, rotationValues.y, rotationValues.z});
    }

    Scale3d scale;
    if (tryReadVec3Any(objectData, {"scale"}, scale)) {
        object.setScale(scale);
    }

    Position3d target;
    if (tryReadVec3Any(objectData, {"target", "lookAt"}, target)) {
        object.lookAt(target, {0.0f, 1.0f, 0.0f});
    }
}

json vec3ToJson(const Position3d &value) {
    return json::array({value.x, value.y, value.z});
}

json rotationToJson(const Rotation3d &value) {
    return json::array({value.pitch, value.yaw, value.roll});
}

json colorToJson(const Color &value) {
    return json::array({value.r, value.g, value.b, value.a});
}

json sizeToJson(const Size2d &value) {
    return json::array({value.width, value.height});
}

Magnitude3d editorForwardDirection(GameObject &object) {
    glm::vec3 direction =
        object.getRotation().toGlmQuat() * glm::vec3(0.0f, -1.0f, 0.0f);
    if (glm::length(direction) < 0.000001f) {
        direction = glm::vec3(0.0f, -1.0f, 0.0f);
    }
    return Magnitude3d::fromGlm(glm::normalize(direction));
}

bool isEditorLightObject(const Context &context, GameObject &object) {
    const int id = static_cast<int>(object.getId());
    return context.editorPointLights.contains(id) ||
           context.editorSpotlights.contains(id) ||
           context.editorAreaLights.contains(id) ||
           context.editorDirectionalLights.contains(id) ||
           context.editorLightSourceData.contains(id);
}

void syncEditorLightObject(Context &context, GameObject &object) {
    const int id = static_cast<int>(object.getId());
    if (auto it = context.editorPointLights.find(id);
        it != context.editorPointLights.end() && it->second != nullptr) {
        it->second->position = object.getPosition();
    }
    if (auto it = context.editorSpotlights.find(id);
        it != context.editorSpotlights.end() && it->second != nullptr) {
        it->second->position = object.getPosition();
        it->second->direction = editorForwardDirection(object);
        it->second->updateDebugObjectRotation();
    }
    if (auto it = context.editorAreaLights.find(id);
        it != context.editorAreaLights.end() && it->second != nullptr) {
        it->second->position = object.getPosition();
        it->second->setRotation(object.getRotation());
    }
    if (auto it = context.editorDirectionalLights.find(id);
        it != context.editorDirectionalLights.end() && it->second != nullptr) {
        it->second->direction = editorForwardDirection(object);
    }
}

json serializeEditorLightObject(Context &context, GameObject &object) {
    syncEditorLightObject(context, object);

    const int id = static_cast<int>(object.getId());
    json node = json::object();
    if (auto source = context.editorLightSourceData.find(id);
        source != context.editorLightSourceData.end() &&
        source->second.is_object()) {
        node = source->second;
    }

    const std::string name = serializableObjectName(context, object);
    if (!name.empty()) {
        node["name"] = name;
    }
    const std::string reference = serializableObjectReference(context, object);
    node["id"] = reference.empty() ? std::to_string(id) : reference;

    if (auto it = context.editorPointLights.find(id);
        it != context.editorPointLights.end() && it->second != nullptr) {
        Light &light = *it->second;
        node["type"] = "pointLight";
        node["position"] = vec3ToJson(light.position);
        node["color"] = colorToJson(light.color);
        node["shineColor"] = colorToJson(light.shineColor);
        node["intensity"] = light.intensity;
        node["distance"] = light.distance;
        return node;
    }

    if (auto it = context.editorSpotlights.find(id);
        it != context.editorSpotlights.end() && it->second != nullptr) {
        Spotlight &light = *it->second;
        node["type"] = "spotLight";
        node["position"] = vec3ToJson(light.position);
        node["direction"] = vec3ToJson(light.direction);
        node["color"] = colorToJson(light.color);
        node["shineColor"] = colorToJson(light.shineColor);
        node["intensity"] = light.intensity;
        node["range"] = light.range;
        node["cutoff"] =
            glm::degrees(std::acos(std::clamp(light.cutOff, -1.0f, 1.0f)));
        node["outerCutoff"] =
            glm::degrees(std::acos(std::clamp(light.outerCutoff, -1.0f, 1.0f)));
        return node;
    }

    if (auto it = context.editorAreaLights.find(id);
        it != context.editorAreaLights.end() && it->second != nullptr) {
        AreaLight &light = *it->second;
        node["type"] = "areaLight";
        node["position"] = vec3ToJson(light.position);
        node["right"] = vec3ToJson(light.right);
        node["up"] = vec3ToJson(light.up);
        node["size"] = sizeToJson(light.size);
        node["color"] = colorToJson(light.color);
        node["shineColor"] = colorToJson(light.shineColor);
        node["intensity"] = light.intensity;
        node["range"] = light.range;
        node["angle"] = light.angle;
        node["castsBothSides"] = light.castsBothSides;
        return node;
    }

    if (auto it = context.editorDirectionalLights.find(id);
        it != context.editorDirectionalLights.end() && it->second != nullptr) {
        DirectionalLight &light = *it->second;
        node["type"] = "directionalLight";
        node["position"] = vec3ToJson(object.getPosition());
        node["direction"] = vec3ToJson(light.direction);
        node["color"] = colorToJson(light.color);
        node["shineColor"] = colorToJson(light.shineColor);
        node["intensity"] = light.intensity;
        return node;
    }

    node["type"] = "ambientLight";
    node["position"] = vec3ToJson(object.getPosition());
    if (!node.contains("color")) {
        node["color"] = colorToJson(Color::white());
    }
    if (!node.contains("intensity")) {
        node["intensity"] = 0.5f;
    }
    return node;
}

std::string serializableObjectName(const Context &context, GameObject &object) {
    if (!object.name.empty()) {
        return object.name;
    }
    auto it = context.objectNames.find(static_cast<int>(object.getId()));
    if (it != context.objectNames.end()) {
        return it->second;
    }
    return {};
}

std::string serializableObjectReference(const Context &context,
                                        GameObject &object) {
    auto it =
        context.objectSceneReferences.find(static_cast<int>(object.getId()));
    if (it != context.objectSceneReferences.end()) {
        return it->second;
    }
    return {};
}

bool objectNodeMatches(const json &node, const std::string &name,
                       const std::string &reference) {
    if (!node.is_object()) {
        return false;
    }
    if (!name.empty() || !reference.empty()) {
        if (const json *nameField = findField(node, {"name"});
            nameField != nullptr && nameField->is_string()) {
            const std::string nodeName = nameField->get<std::string>();
            if ((!name.empty() && nodeName == name) ||
                (!reference.empty() && nodeName == reference)) {
                return true;
            }
        }
    }
    if (!reference.empty()) {
        if (const json *idField = findField(node, {"id"}); idField != nullptr) {
            if (idField->is_string() &&
                idField->get<std::string>() == reference) {
                return true;
            }
            if (idField->is_number_integer() &&
                std::to_string(idField->get<int>()) == reference) {
                return true;
            }
        }
    }
    return false;
}

void writeObjectTransform(json &node, const Context &context,
                          GameObject &object) {
    const std::string name = serializableObjectName(context, object);
    if (!name.empty()) {
        node["name"] = name;
    }
    node["position"] = vec3ToJson(object.getPosition());
    node["rotation"] = rotationToJson(object.getRotation());
    node["scale"] = vec3ToJson(object.getScale());
    auto parentIt =
        context.objectParents.find(static_cast<int>(object.getId()));
    if (parentIt != context.objectParents.end()) {
        auto parentName = context.objectNames.find(parentIt->second);
        node["parent"] = parentName != context.objectNames.end()
                             ? parentName->second
                             : std::to_string(parentIt->second);
    } else {
        node.erase("parent");
    }
}

bool updateObjectNode(json &node, const Context &context, GameObject &object) {
    const std::string name = serializableObjectName(context, object);
    const std::string reference = serializableObjectReference(context, object);
    if (objectNodeMatches(node, name, reference)) {
        writeObjectTransform(node, context, object);
        return true;
    }

    if (const json *children = findField(node, {"objects"});
        children != nullptr && children->is_array()) {
        json &mutableChildren = node["objects"];
        for (auto &child : mutableChildren) {
            if (updateObjectNode(child, context, object)) {
                return true;
            }
        }
    }

    return false;
}

bool removeObjectNode(json &nodes, const std::string &name,
                      const std::string &reference) {
    if (!nodes.is_array()) {
        return false;
    }

    bool removed = false;
    for (auto it = nodes.begin(); it != nodes.end();) {
        if (objectNodeMatches(*it, name, reference)) {
            it = nodes.erase(it);
            removed = true;
            continue;
        }

        if (it->is_object()) {
            if (const json *children = findField(*it, {"objects"});
                children != nullptr && children->is_array()) {
                removed = removeObjectNode((*it)["objects"], name, reference) ||
                          removed;
            }
        }

        ++it;
    }
    return removed;
}

json serializeNewObject(const Context &context, GameObject &object) {
    json node = json::object();
    const std::string name = serializableObjectName(context, object);
    if (!name.empty()) {
        node["name"] = name;
    }
    node["id"] = static_cast<int>(object.getId());
    const int id = static_cast<int>(object.getId());
    auto typeIt = context.objectSceneTypes.find(id);
    const std::string type =
        typeIt != context.objectSceneTypes.end() ? typeIt->second : "solid";
    node["type"] = type;
    if (type == "solid") {
        auto solidIt = context.objectSceneSolidTypes.find(id);
        node["solid_type"] = solidIt != context.objectSceneSolidTypes.end()
                                 ? solidIt->second
                                 : "cube";
    }
    writeObjectTransform(node, context, object);
    return node;
}

CoreObject createCapsulePrimitive(float radius, float height, Color color) {
    constexpr unsigned int sectorCount = 32;
    constexpr unsigned int hemisphereSegments = 8;
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
    const float halfHeight = std::max(0.0f, height * 0.5f);
    const float pi = static_cast<float>(std::numbers::pi);

    auto appendRing = [&](float y, float ringRadius, float centerY,
                          float vCoord) {
        for (unsigned int j = 0; j <= sectorCount; ++j) {
            float sector = (static_cast<float>(j) / sectorCount) * pi * 2.0f;
            float x = ringRadius * std::cos(sector);
            float z = ringRadius * std::sin(sector);
            glm::vec3 normal(x, y - centerY, z);
            if (glm::length(normal) < 0.000001f) {
                normal = glm::vec3(0.0f, y >= 0.0f ? 1.0f : -1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }
            glm::vec3 tangent(-std::sin(sector), 0.0f, std::cos(sector));
            if (glm::length(tangent) < 0.000001f) {
                tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                tangent = glm::normalize(tangent);
            }
            glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

            CoreVertex vertex;
            vertex.position = Position3d(x, y, z);
            vertex.color = color;
            vertex.textureCoordinate = {
                static_cast<float>(j) / sectorCount,
                vCoord,
            };
            vertex.normal = Normal3d::fromGlm(normal);
            vertex.tangent = Normal3d::fromGlm(tangent);
            vertex.bitangent = Normal3d::fromGlm(bitangent);
            vertices.push_back(vertex);
        }
    };

    for (unsigned int i = 0; i <= hemisphereSegments; ++i) {
        float t = static_cast<float>(i) / hemisphereSegments;
        float angle = (pi * 0.5f) * (1.0f - t);
        appendRing(halfHeight + radius * std::sin(angle),
                   radius * std::cos(angle), halfHeight, t * 0.5f);
    }
    for (unsigned int i = 1; i <= hemisphereSegments; ++i) {
        float t = static_cast<float>(i) / hemisphereSegments;
        float angle = -(pi * 0.5f) * t;
        appendRing(-halfHeight + radius * std::sin(angle),
                   radius * std::cos(angle), -halfHeight, 0.5f + t * 0.5f);
    }

    const unsigned int ringCount = hemisphereSegments * 2 + 1;
    for (unsigned int i = 0; i < ringCount - 1; ++i) {
        unsigned int k1 = i * (sectorCount + 1);
        unsigned int k2 = k1 + sectorCount + 1;
        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);
            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    CoreObject capsule;
    capsule.attachVertices(vertices);
    capsule.attachIndices(indices);
    capsule.material.albedo = color;
    capsule.initialize();
    return capsule;
}

void resolveObjectParentReferences(Context &context) {
    for (const auto &[childId, parentReference] :
         context.objectParentReferences) {
        auto parentIt = context.objectReferences.find(parentReference);
        if (parentIt == context.objectReferences.end()) {
            parentIt =
                context.objectReferences.find(normalizeToken(parentReference));
        }
        if (parentIt == context.objectReferences.end() ||
            parentIt->second == nullptr) {
            continue;
        }

        GameObject *child = nullptr;
        for (const auto &renderable : context.objects) {
            if (renderable == nullptr) {
                continue;
            }
            auto *object = dynamic_cast<GameObject *>(renderable.get());
            if (object != nullptr &&
                static_cast<int>(object->getId()) == childId) {
                child = object;
                break;
            }
        }
        if (child == nullptr || child == parentIt->second) {
            continue;
        }

        context.objectParents[childId] =
            static_cast<int>(parentIt->second->getId());
        if (auto *compound = dynamic_cast<CompoundObject *>(parentIt->second);
            compound != nullptr &&
            std::ranges::find(compound->objects, child) ==
                compound->objects.end()) {
            compound->addObject(child);
        }
    }
}

void applyMaterial(GameObject &object, const MaterialDefinition &material) {
    if (auto *coreObject = dynamic_cast<CoreObject *>(&object);
        coreObject != nullptr) {
        coreObject->material = material.material;
        for (const auto &texture : material.textures) {
            coreObject->attachTexture(texture);
        }
        return;
    }

    if (auto *model = dynamic_cast<Model *>(&object); model != nullptr) {
        model->material = material.material;
        for (auto &mesh : model->getObjects()) {
            if (mesh != nullptr) {
                mesh->material = material.material;
            }
        }
        for (const auto &texture : material.textures) {
            model->attachTexture(texture);
        }
    }
}

std::shared_ptr<CoreObject> createEditorLightProxy(const std::string &type,
                                                   const Color &color,
                                                   const Position3d &position) {
    auto object = std::make_shared<CoreObject>();
    const std::string normalized = normalizeToken(type);
    if (normalized == "pointlight") {
        *object = createSphere(0.1f, 24, 12, color);
    } else if (normalized == "arealight") {
        *object = createPlane({0.55f, 0.55f}, color);
    } else {
        *object = createPyramid({0.35f, 0.35f, 0.35f}, color);
    }
    object->setPosition(position);
    object->material.albedo = color;
    object->material.emissiveColor = color;
    object->material.emissiveIntensity = 1.5f;
    object->castsShadows = false;
    object->editorOnly = true;
    return object;
}

int registerEditorLightObject(Context &context,
                              const std::shared_ptr<CoreObject> &object,
                              const json &sourceData,
                              const std::string &objectType) {
    if (context.window == nullptr || object == nullptr) {
        return -1;
    }
    registerGameObject(context, *object, sourceData, objectType,
                       context.objects.size());
    const int id = static_cast<int>(object->getId());
    context.editorLightSourceData[id] =
        sourceData.is_object() ? sourceData : json::object();
    context.objects.push_back(object);
    context.window->addObject(object.get());
    return id;
}

void collectPendingComponents(GameObject &object, const json &objectData,
                              const std::string &baseDir,
                              std::vector<PendingComponent> &rigidbodies,
                              std::vector<PendingComponent> &standard,
                              std::vector<PendingComponent> &joints) {
    const json *componentsField = findField(objectData, {"components"});
    if (componentsField == nullptr) {
        return;
    }

    JsonDefinition definition = loadJsonDefinition(*componentsField, baseDir);
    std::vector<json> componentEntries;

    if (definition.data.is_array()) {
        componentEntries.assign(definition.data.begin(), definition.data.end());
    } else if (definition.data.is_object()) {
        if (const json *arrayField = findField(definition.data, {"components"});
            arrayField != nullptr && arrayField->is_array()) {
            componentEntries.assign(arrayField->begin(), arrayField->end());
        } else if (definition.data.contains("type")) {
            componentEntries.push_back(definition.data);
        }
    }

    if (componentEntries.empty()) {
        return;
    }

    std::string objectType;
    tryReadStringAny(objectData, {"type"}, objectType);
    objectType = normalizeToken(objectType);

    for (const auto &componentData : componentEntries) {
        if (!componentData.is_object()) {
            continue;
        }

        std::string type;
        tryReadStringAny(componentData, {"type"}, type);
        const std::string normalizedType = normalizeToken(type);
        if (normalizedType.empty()) {
            continue;
        }

        PendingComponent pending{
            .object = &object,
            .objectType = objectType,
            .baseDir = definition.baseDir,
            .data = componentData,
        };

        if (normalizedType == "rigidbody") {
            rigidbodies.push_back(std::move(pending));
        } else if (normalizedType == "joint" ||
                   normalizedType == "fixedjoint" ||
                   normalizedType == "hingejoint" ||
                   normalizedType == "springjoint") {
            joints.push_back(std::move(pending));
        } else {
            standard.push_back(std::move(pending));
        }
    }
}

MotionType parseMotionType(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "static") {
        return MotionType::Static;
    }
    if (token == "dynamic") {
        return MotionType::Dynamic;
    }
    if (token == "kinematic") {
        return MotionType::Kinematic;
    }
    throw std::runtime_error("Unknown motion type: " + value);
}

Space parseSpace(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "local") {
        return Space::Local;
    }
    if (token == "world" || token == "global") {
        return Space::Global;
    }
    throw std::runtime_error("Unknown joint space: " + value);
}

SpringMode parseSpringMode(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "frequencyanddamping") {
        return SpringMode::FrequencyAndDamping;
    }
    if (token == "stiffnessanddamping") {
        return SpringMode::StiffnessAndDamping;
    }
    throw std::runtime_error("Unknown spring mode: " + value);
}

std::variant<GameObject *, WorldBody>
parseJointEndpoint(Context &context, const json &value, GameObject *fallback) {
    if (value.is_null()) {
        if (fallback != nullptr) {
            return fallback;
        }
        throw std::runtime_error("Joint endpoint is missing");
    }

    if (value.is_string()) {
        const std::string reference = value.get<std::string>();
        const std::string token = normalizeToken(reference);
        if (token == "world") {
            return WorldBody{};
        }

        auto it = context.objectReferences.find(reference);
        if (it != context.objectReferences.end()) {
            return it->second;
        }

        it = context.objectReferences.find(token);
        if (it != context.objectReferences.end()) {
            return it->second;
        }

        throw std::runtime_error("Unknown joint object reference: " +
                                 reference);
    }

    if (value.is_number_integer()) {
        const std::string reference = std::to_string(value.get<int>());
        auto it = context.objectReferences.find(reference);
        if (it != context.objectReferences.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown joint object reference: " +
                                 reference);
    }

    if (value.is_object()) {
        if (const json *field =
                findField(value, {"name", "id", "ref", "object"});
            field != nullptr) {
            return parseJointEndpoint(context, *field, fallback);
        }
    }

    if (fallback != nullptr) {
        return fallback;
    }

    throw std::runtime_error("Invalid joint endpoint definition");
}

void configureJointBase(Joint &joint, Context &context, GameObject &owner,
                        const json &componentData) {
    if (const json *parentField = findField(componentData, {"parent"});
        parentField != nullptr) {
        auto endpoint = parseJointEndpoint(context, *parentField, nullptr);
        if (std::holds_alternative<GameObject *>(endpoint)) {
            joint.parent = std::get<GameObject *>(endpoint);
        } else {
            joint.parent = WorldBody{};
        }
    } else {
        joint.parent = WorldBody{};
    }

    if (const json *childField = findField(componentData, {"child"});
        childField != nullptr) {
        auto endpoint = parseJointEndpoint(context, *childField, &owner);
        if (std::holds_alternative<GameObject *>(endpoint)) {
            joint.child = std::get<GameObject *>(endpoint);
        } else {
            joint.child = WorldBody{};
        }
    } else {
        joint.child = &owner;
    }

    std::string space;
    if (tryReadStringAny(componentData, {"space"}, space)) {
        joint.space = parseSpace(space);
    }

    tryReadVec3Any(componentData, {"anchor"}, joint.anchor);
    tryReadFloatAny(componentData, {"breakForce"}, joint.breakForce);
    tryReadFloatAny(componentData, {"breakTorque"}, joint.breakTorque);
}

void configureRigidbodyCollider(const std::shared_ptr<Rigidbody> &rigidbody,
                                GameObject &object, const json &colliderData) {
    if (!colliderData.is_object()) {
        throw std::runtime_error("Collider definition must be an object");
    }

    std::string colliderType;
    tryReadStringAny(colliderData, {"type"}, colliderType);
    const std::string token = normalizeToken(colliderType);

    if (token == "box") {
        Position3d size{1.0f, 1.0f, 1.0f};
        tryReadVec3Any(colliderData, {"size", "extents", "dimensions"}, size);
        rigidbody->addBoxCollider(size);
        return;
    }

    if (token == "sphere") {
        float radius = 0.5f;
        tryReadFloatAny(colliderData, {"radius"}, radius);
        rigidbody->addSphereCollider(radius);
        return;
    }

    if (token == "capsule") {
        float radius = 0.5f;
        float height = 1.0f;
        tryReadFloatAny(colliderData, {"radius"}, radius);
        tryReadFloatAny(colliderData, {"height"}, height);
        rigidbody->addCapsuleCollider(radius, height);
        return;
    }

    if (token == "mesh") {
        if (auto *coreObject = dynamic_cast<CoreObject *>(&object);
            coreObject != nullptr) {
            rigidbody->addMeshCollider();
            return;
        }

        if (auto *model = dynamic_cast<Model *>(&object); model != nullptr) {
            if (rigidbody->body == nullptr) {
                rigidbody->body = std::make_shared<bezel::Rigidbody>();
                rigidbody->body->id.atlasId = object.getId();
            }

            std::vector<Position3d> vertices;
            std::vector<uint32_t> indices;
            uint32_t vertexOffset = 0;

            for (const auto &mesh : model->getObjects()) {
                if (mesh == nullptr) {
                    continue;
                }
                for (const auto &vertex : mesh->vertices) {
                    vertices.push_back(vertex.position);
                }
                for (const auto &index : mesh->indices) {
                    indices.push_back(vertexOffset +
                                      static_cast<uint32_t>(index));
                }
                vertexOffset += static_cast<uint32_t>(mesh->vertices.size());
            }

            rigidbody->body->setCollider(
                std::make_shared<bezel::MeshCollider>(vertices, indices));
            return;
        }

        throw std::runtime_error("Mesh colliders are only supported on solid "
                                 "and model runtime objects");
    }

    throw std::runtime_error("Unknown collider type: " + colliderType);
}

float normalizePercentage(float value) {
    return value > 1.0f ? value / 100.0f : value;
}

void configureVehicleSettings(bezel::VehicleSettings &settings,
                              const json &componentData) {
    const json *settingsNode = findField(componentData, {"settings"});
    const json &data = settingsNode != nullptr && settingsNode->is_object()
                           ? *settingsNode
                           : componentData;

    Position3d vector;
    if (tryReadVec3Any(data, {"up"}, vector)) {
        settings.up = normalizeVector(vector, {0.0f, 1.0f, 0.0f});
    }
    if (tryReadVec3Any(data, {"forward"}, vector)) {
        settings.forward = normalizeVector(vector, {0.0f, 0.0f, 1.0f});
    }
    tryReadFloatAny(data, {"maxPitchRollAngleDeg"},
                    settings.maxPitchRollAngleDeg);
    tryReadFloatAny(data, {"maxSlopeAngleDeg"}, settings.maxSlopeAngleDeg);

    if (const json *wheelsNode = findField(data, {"wheels"});
        wheelsNode != nullptr && wheelsNode->is_array()) {
        settings.wheels.clear();
        settings.wheels.reserve(wheelsNode->size());

        for (const auto &wheelData : *wheelsNode) {
            if (!wheelData.is_object()) {
                throw std::runtime_error("Vehicle wheel definition must be an "
                                         "object");
            }

            bezel::VehicleWheelSettings wheel;

            if (tryReadVec3Any(wheelData, {"position"}, vector)) {
                wheel.position = vector;
            }
            tryReadBoolAny(wheelData, {"enableSuspensionForcePoint"},
                           wheel.enableSuspensionForcePoint);
            if (tryReadVec3Any(wheelData, {"suspensionForcePoint"}, vector)) {
                wheel.suspensionForcePoint = vector;
            }
            if (tryReadVec3Any(wheelData, {"suspensionDirection"}, vector)) {
                wheel.suspensionDirection =
                    normalizeVector(vector, {0.0f, -1.0f, 0.0f});
            }
            if (tryReadVec3Any(wheelData, {"steeringAxis"}, vector)) {
                wheel.steeringAxis =
                    normalizeVector(vector, {0.0f, 1.0f, 0.0f});
            }
            if (tryReadVec3Any(wheelData, {"wheelUp"}, vector)) {
                wheel.wheelUp = normalizeVector(vector, {0.0f, 1.0f, 0.0f});
            }
            if (tryReadVec3Any(wheelData, {"wheelForward"}, vector)) {
                wheel.wheelForward =
                    normalizeVector(vector, {0.0f, 0.0f, 1.0f});
            }

            tryReadFloatAny(wheelData, {"suspensionMinLength"},
                            wheel.suspensionMinLength);
            tryReadFloatAny(wheelData, {"suspensionMaxLength"},
                            wheel.suspensionMaxLength);
            tryReadFloatAny(wheelData, {"suspensionPreloadLength"},
                            wheel.suspensionPreloadLength);
            tryReadFloatAny(wheelData, {"suspensionFrequencyHz"},
                            wheel.suspensionFrequencyHz);
            tryReadFloatAny(wheelData, {"suspensionDampingRatio"},
                            wheel.suspensionDampingRatio);
            tryReadFloatAny(wheelData, {"radius"}, wheel.radius);
            tryReadFloatAny(wheelData, {"width"}, wheel.width);
            tryReadFloatAny(wheelData, {"inertia"}, wheel.inertia);
            tryReadFloatAny(wheelData, {"angularDamping"},
                            wheel.angularDamping);
            tryReadFloatAny(wheelData, {"maxSteerAngleDeg"},
                            wheel.maxSteerAngleDeg);
            tryReadFloatAny(wheelData, {"maxBrakeTorque", "maxBreakTorque"},
                            wheel.maxBrakeTorque);
            tryReadFloatAny(wheelData,
                            {"maxHandBrakeTorque", "maxHandBreakTorque"},
                            wheel.maxHandBrakeTorque);

            settings.wheels.push_back(wheel);
        }
    }

    if (const json *controllerNode = findField(data, {"controller"});
        controllerNode != nullptr && controllerNode->is_object()) {
        const json &controllerData = *controllerNode;

        if (const json *engineNode = findField(controllerData, {"engine"});
            engineNode != nullptr && engineNode->is_object()) {
            tryReadFloatAny(*engineNode, {"maxTorque"},
                            settings.controller.engine.maxTorque);
            tryReadFloatAny(*engineNode, {"minRPM"},
                            settings.controller.engine.minRPM);
            tryReadFloatAny(*engineNode, {"maxRPM"},
                            settings.controller.engine.maxRPM);
            tryReadFloatAny(*engineNode, {"inertia"},
                            settings.controller.engine.inertia);
            tryReadFloatAny(*engineNode, {"angularDamping"},
                            settings.controller.engine.angularDamping);
        }

        if (const json *transmissionNode =
                findField(controllerData, {"transmission"});
            transmissionNode != nullptr && transmissionNode->is_object()) {
            std::string transmissionType;
            if (tryReadStringAny(*transmissionNode, {"type", "mode"},
                                 transmissionType)) {
                const std::string token = normalizeToken(transmissionType);
                settings.controller.transmission.mode =
                    token == "manual" ? bezel::VehicleTransmissionMode::Manual
                                      : bezel::VehicleTransmissionMode::Auto;
            }

            if (const json *gearRatios =
                    findField(*transmissionNode, {"gearRatios"});
                gearRatios != nullptr && gearRatios->is_array()) {
                settings.controller.transmission.gearRatios.clear();
                for (const auto &ratio : *gearRatios) {
                    if (ratio.is_number()) {
                        settings.controller.transmission.gearRatios.push_back(
                            ratio.get<float>());
                    }
                }
            }

            if (const json *reverseRatios =
                    findField(*transmissionNode, {"reverseGearRatios"});
                reverseRatios != nullptr && reverseRatios->is_array()) {
                settings.controller.transmission.reverseGearRatios.clear();
                for (const auto &ratio : *reverseRatios) {
                    if (ratio.is_number()) {
                        settings.controller.transmission.reverseGearRatios
                            .push_back(ratio.get<float>());
                    }
                }
            } else {
                float reverseRatio = 0.0f;
                if (tryReadFloatAny(*transmissionNode, {"reverseGearRatio"},
                                    reverseRatio)) {
                    settings.controller.transmission.reverseGearRatios = {
                        reverseRatio};
                }
            }

            tryReadFloatAny(*transmissionNode, {"switchTime"},
                            settings.controller.transmission.switchTime);
            tryReadFloatAny(*transmissionNode, {"clutchReleaseTime"},
                            settings.controller.transmission.clutchReleaseTime);
            tryReadFloatAny(*transmissionNode, {"switchLatency"},
                            settings.controller.transmission.switchLatency);
            tryReadFloatAny(*transmissionNode, {"shiftUpRPM"},
                            settings.controller.transmission.shiftUpRPM);
            tryReadFloatAny(*transmissionNode, {"shiftDownRPM"},
                            settings.controller.transmission.shiftDownRPM);
            tryReadFloatAny(*transmissionNode, {"clutchStrength"},
                            settings.controller.transmission.clutchStrength);
        }

        if (const json *differentialsNode =
                findField(controllerData, {"differentials"});
            differentialsNode != nullptr && differentialsNode->is_array()) {
            settings.controller.differentials.clear();

            for (const auto &differentialData : *differentialsNode) {
                if (!differentialData.is_object()) {
                    throw std::runtime_error(
                        "Vehicle differential definition must be an object");
                }

                bezel::VehicleDifferential differential;
                tryReadIntAny(differentialData, {"leftWheel"},
                              differential.leftWheel);
                tryReadIntAny(differentialData, {"rightWheel"},
                              differential.rightWheel);
                tryReadFloatAny(differentialData, {"differentialRatio"},
                                differential.differentialRatio);
                tryReadFloatAny(differentialData, {"leftRightSplit"},
                                differential.leftRightSplit);
                tryReadFloatAny(differentialData, {"limitedSlipRatio"},
                                differential.limitedSlipRatio);
                tryReadFloatAny(differentialData, {"engineTorqueRatio"},
                                differential.engineTorqueRatio);

                differential.leftRightSplit =
                    normalizePercentage(differential.leftRightSplit);
                differential.engineTorqueRatio =
                    normalizePercentage(differential.engineTorqueRatio);

                settings.controller.differentials.push_back(differential);
            }
        }

        tryReadFloatAny(controllerData, {"differentialLimitedSlipRatio"},
                        settings.controller.differentialLimitedSlipRatio);
    }
}

void attachComponent(Context &context, const PendingComponent &pending) {
    if (pending.object == nullptr || !pending.data.is_object()) {
        return;
    }

    std::string type;
    tryReadStringAny(pending.data, {"type"}, type);
    const std::string token = normalizeToken(type);

    if (token == "script" || token == "traitscript") {
        auto component = std::make_shared<RuntimeScriptComponent>();
        component->host = &context;
        component->isTrait = token == "traitscript";

        tryReadStringAny(pending.data, {"name", "class", "className"},
                         component->className);

        std::string source;
        if (tryReadStringAny(pending.data, {"source"}, source) &&
            !source.empty()) {
            const std::string resolvedSource =
                resolveRuntimePath(pending.baseDir, source);
            std::string extension =
                std::filesystem::path(resolvedSource).extension().string();
            std::transform(extension.begin(), extension.end(),
                           extension.begin(), [](unsigned char value) {
                               return static_cast<char>(std::tolower(value));
                           });

            if (extension == ".js" || extension == ".mjs") {
                component->entryModuleName =
                    context.registerScriptModule(resolvedSource);
                component->source.clear();
            } else {
                component->entryModuleName = context.scriptBundleModuleName;
                component->source = context.toProjectScriptPath(resolvedSource);
            }

            if (component->className.empty()) {
                component->className = inferScriptClassName(resolvedSource);
            }
        } else if (!component->className.empty()) {
            if (const auto it =
                    context.scriptRegistry.find(component->className);
                it != context.scriptRegistry.end()) {
                component->entryModuleName = context.scriptBundleModuleName;
                component->source = it->second;
            }
        }

        if (const json *variables = findField(pending.data, {"variables"});
            variables != nullptr) {
            component->variables = *variables;
        }

        tryReadStringAny(pending.data, {"traitedType"}, component->traitedType);

        if (component->isTrait && !component->traitedType.empty() &&
            normalizeToken(component->traitedType) != pending.objectType) {
            throw std::runtime_error("Trait script is incompatible with object "
                                     "type: " +
                                     component->traitedType);
        }

        if (component->className.empty()) {
            throw std::runtime_error(
                "Script component is missing a class name");
        }

        if (component->entryModuleName.empty()) {
            throw std::runtime_error(
                "Script component could not resolve class: " +
                component->className);
        }

        if (component->entryModuleName == context.scriptBundleModuleName &&
            !context.scriptHost.modules.contains(
                context.scriptBundleModuleName)) {
            throw std::runtime_error(
                "Compiled script bundle not found: " +
                resolveRuntimePath(context.projectDir,
                                   RUNTIME_SCRIPT_BUNDLE_PATH));
        }

        pending.object->addComponent(component);
        return;
    }

    if (token == "rigidbody") {
        auto rigidbody = std::make_shared<Rigidbody>();
        pending.object->addComponent(rigidbody);

        tryReadStringAny(pending.data, {"sendSignal", "signal"},
                         rigidbody->sendSignal);
        tryReadBoolAny(pending.data, {"isSensor"}, rigidbody->isSensor);

        if (const json *collider = findField(pending.data, {"collider"});
            collider != nullptr) {
            configureRigidbodyCollider(rigidbody, *pending.object, *collider);
        }

        float friction = 0.5f;
        if (tryReadFloatAny(pending.data, {"friction"}, friction)) {
            rigidbody->setFriction(friction);
        }

        if (const json *tags = findField(pending.data, {"tags"});
            tags != nullptr && tags->is_array()) {
            for (const auto &tag : *tags) {
                if (tag.is_string()) {
                    rigidbody->addTag(tag.get<std::string>());
                }
            }
        }

        if (const json *damping = findField(pending.data, {"damping"});
            damping != nullptr && damping->is_object()) {
            float linear = 0.0f;
            float angular = 0.0f;
            tryReadFloatAny(*damping, {"linear"}, linear);
            tryReadFloatAny(*damping, {"angular"}, angular);
            rigidbody->setDamping(linear, angular);
        }

        float mass = 0.0f;
        if (tryReadFloatAny(pending.data, {"mass"}, mass)) {
            rigidbody->setMass(mass);
        }

        float restitution = 0.0f;
        if (tryReadFloatAny(pending.data, {"restitution", "restituition"},
                            restitution)) {
            rigidbody->setRestitution(restitution);
        }

        std::string motionType;
        if (tryReadStringAny(pending.data, {"motionType"}, motionType)) {
            rigidbody->setMotionType(parseMotionType(motionType));
        }

        runtime::scripting::registerNativeRigidbody(
            context.context, context.scriptHost, pending.object->getId(),
            rigidbody);

        return;
    }

    if (token == "audioplayer") {
        auto component = std::make_shared<AudioPlayer>();
        pending.object->addComponent(component);

        std::string source;
        if (tryReadStringAny(pending.data, {"source"}, source) &&
            !source.empty()) {
            component->setSource(createRuntimeResource(
                pending.baseDir, source, ResourceType::Audio, "runtime-audio"));
        }

        bool spatialization = false;
        if (tryReadBoolAny(pending.data, {"useSpatialization"},
                           spatialization)) {
            if (spatialization) {
                component->useSpatialization();
            } else {
                component->disableSpatialization();
            }
        }

        Position3d position;
        if (tryReadVec3Any(pending.data, {"position"}, position)) {
            component->setPosition(position);
        }

        bool autoPlay = false;
        if (tryReadBoolAny(pending.data,
                           {"autoplay", "autoPlay", "playOnStart"}, autoPlay) &&
            autoPlay) {
            component->play();
        }

        return;
    }

    if (token == "joint" || token == "fixedjoint") {
        auto component = std::make_shared<FixedJoint>();
        pending.object->addComponent(component);
        configureJointBase(*component, context, *pending.object, pending.data);
        runtime::scripting::registerNativeFixedJoint(
            context.context, context.scriptHost, pending.object->getId(),
            component);
        return;
    }

    if (token == "hingejoint") {
        auto component = std::make_shared<HingeJoint>();
        pending.object->addComponent(component);
        configureJointBase(*component, context, *pending.object, pending.data);

        Position3d axis;
        if (tryReadVec3Any(pending.data, {"axis1"}, axis)) {
            component->axis1 = normalizeVector(axis, {0.0f, 1.0f, 0.0f});
        }
        if (tryReadVec3Any(pending.data, {"axis2"}, axis)) {
            component->axis2 = normalizeVector(axis, {0.0f, 1.0f, 0.0f});
        }

        if (const json *limits = findField(pending.data, {"limits"});
            limits != nullptr && limits->is_object()) {
            tryReadBoolAny(*limits, {"isEnabled", "enabled"},
                           component->limits.enabled);
            tryReadFloatAny(*limits, {"minAngle"}, component->limits.minAngle);
            tryReadFloatAny(*limits, {"maxAngle"}, component->limits.maxAngle);
        }

        if (const json *motor = findField(pending.data, {"motor"});
            motor != nullptr && motor->is_object()) {
            tryReadBoolAny(*motor, {"isEnabled", "enabled"},
                           component->motor.enabled);
            tryReadFloatAny(*motor, {"maxForce"}, component->motor.maxForce);
            tryReadFloatAny(*motor, {"maxTorque"}, component->motor.maxTorque);
        }

        runtime::scripting::registerNativeHingeJoint(
            context.context, context.scriptHost, pending.object->getId(),
            component);

        return;
    }

    if (token == "springjoint") {
        auto component = std::make_shared<SpringJoint>();
        pending.object->addComponent(component);
        configureJointBase(*component, context, *pending.object, pending.data);

        tryReadVec3Any(pending.data, {"anchorB"}, component->anchorB);
        tryReadFloatAny(pending.data, {"restLength"}, component->restLength);
        tryReadBoolAny(pending.data, {"useLimits"}, component->useLimits);
        tryReadFloatAny(pending.data, {"minLength"}, component->minLength);
        tryReadFloatAny(pending.data, {"maxLength"}, component->maxLength);

        if (const json *spring = findField(pending.data, {"spring"});
            spring != nullptr && spring->is_object()) {
            tryReadBoolAny(*spring, {"enabled", "isEnabled"},
                           component->spring.enabled);
            std::string mode;
            if (tryReadStringAny(*spring, {"mode"}, mode)) {
                component->spring.mode = parseSpringMode(mode);
            }
            tryReadFloatAny(*spring, {"frequencyHz"},
                            component->spring.frequencyHz);
            tryReadFloatAny(*spring, {"dampingRatio"},
                            component->spring.dampingRatio);
            tryReadFloatAny(*spring, {"stiffness"},
                            component->spring.stiffness);
            tryReadFloatAny(*spring, {"damping"}, component->spring.damping);
        }

        runtime::scripting::registerNativeSpringJoint(
            context.context, context.scriptHost, pending.object->getId(),
            component);

        return;
    }

    if (token == "vehicle") {
        auto component = std::make_shared<Vehicle>();
        pending.object->addComponent(component);
        configureVehicleSettings(component->settings, pending.data);
        runtime::scripting::registerNativeVehicle(
            context.context, context.scriptHost, pending.object->getId(),
            component);
        return;
    }

    throw std::runtime_error("Unknown component type: " + type);
}

Key parseKeyString(const std::string &value) {
    SDL_Scancode scancode = SDL_GetScancodeFromName(value.c_str());
    if (scancode != SDL_SCANCODE_UNKNOWN) {
        return static_cast<Key>(scancode);
    }

    std::string spaced = value;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    std::replace(spaced.begin(), spaced.end(), '-', ' ');
    scancode = SDL_GetScancodeFromName(spaced.c_str());
    if (scancode != SDL_SCANCODE_UNKNOWN) {
        return static_cast<Key>(scancode);
    }

    const std::string token = normalizeToken(value);
    if (token.size() == 1 &&
        std::isalpha(static_cast<unsigned char>(token[0]))) {
        std::string letter(1, static_cast<char>(std::toupper(
                                  static_cast<unsigned char>(token[0]))));
        scancode = SDL_GetScancodeFromName(letter.c_str());
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            return static_cast<Key>(scancode);
        }
    }

    if (token.size() == 1 &&
        std::isdigit(static_cast<unsigned char>(token[0]))) {
        std::string digit(1, token[0]);
        scancode = SDL_GetScancodeFromName(digit.c_str());
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            return static_cast<Key>(scancode);
        }
    }

    if (token == "space") {
        return Key::Space;
    }
    if (token == "leftshift" || token == "lshift") {
        return Key::LeftShift;
    }
    if (token == "rightshift" || token == "rshift") {
        return Key::RightShift;
    }
    if (token == "leftcontrol" || token == "leftctrl" || token == "lctrl") {
        return Key::LeftControl;
    }
    if (token == "rightcontrol" || token == "rightctrl" || token == "rctrl") {
        return Key::RightControl;
    }
    if (token == "leftalt" || token == "lalt") {
        return Key::LeftAlt;
    }
    if (token == "rightalt" || token == "ralt") {
        return Key::RightAlt;
    }
    if (token == "leftsuper" || token == "lsuper" || token == "leftgui") {
        return Key::LeftSuper;
    }
    if (token == "rightsuper" || token == "rsuper" || token == "rightgui") {
        return Key::RightSuper;
    }
    if (token == "return") {
        return Key::Enter;
    }

    throw std::runtime_error("Unknown key trigger: " + value);
}

MouseButton parseMouseButtonString(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "mouseleft" || token == "leftmouse" || token == "button1" ||
        token == "mouse1") {
        return MouseButton::Left;
    }
    if (token == "mouseright" || token == "rightmouse" || token == "button2" ||
        token == "mouse2") {
        return MouseButton::Right;
    }
    if (token == "mousemiddle" || token == "middlemouse" ||
        token == "button3" || token == "mouse3") {
        return MouseButton::Middle;
    }
    if (token == "button4" || token == "mouse4") {
        return MouseButton::Button4;
    }
    if (token == "button5" || token == "mouse5") {
        return MouseButton::Button5;
    }
    if (token == "button6" || token == "mouse6") {
        return MouseButton::Button6;
    }
    if (token == "button7" || token == "mouse7") {
        return MouseButton::Button7;
    }
    if (token == "button8" || token == "mouse8") {
        return MouseButton::Button8;
    }
    throw std::runtime_error("Unknown mouse trigger: " + value);
}

Trigger parseTrigger(const json &triggerData) {
    if (triggerData.is_string()) {
        const std::string raw = triggerData.get<std::string>();
        const std::string token = normalizeToken(raw);
        if (token.rfind("mouse", 0) == 0 || token.rfind("button", 0) == 0) {
            return Trigger::fromMouseButton(parseMouseButtonString(raw));
        }
        return Trigger::fromKey(parseKeyString(raw));
    }

    if (!triggerData.is_object()) {
        throw std::runtime_error("Invalid trigger definition");
    }

    std::string type;
    JSON_READ_STRING(triggerData, "type", type);
    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "mouse") {
        std::string buttonName;
        JSON_READ_STRING(triggerData, "button", buttonName);
        if (buttonName.empty()) {
            JSON_READ_STRING(triggerData, "name", buttonName);
        }
        return Trigger::fromMouseButton(parseMouseButtonString(buttonName));
    }

    if (normalizedType == "controller") {
        int controllerId = -1;
        int buttonIndex = -1;
        JSON_READ_INT(triggerData, "id", controllerId);
        JSON_READ_INT(triggerData, "button", buttonIndex);
        JSON_READ_INT(triggerData, "buttonIndex", buttonIndex);
        if (buttonIndex < 0) {
            throw std::runtime_error("Controller trigger is missing button");
        }
        return Trigger::fromControllerButton(controllerId, buttonIndex);
    }

    std::string keyName;
    JSON_READ_STRING(triggerData, "key", keyName);
    if (keyName.empty()) {
        JSON_READ_STRING(triggerData, "name", keyName);
    }
    if (keyName.empty()) {
        throw std::runtime_error("Key trigger is missing key name");
    }
    return Trigger::fromKey(parseKeyString(keyName));
}

AxisTrigger parseAxisTrigger(const json &triggerData) {
    if (triggerData.is_string()) {
        const std::string type = normalizeToken(triggerData.get<std::string>());
        if (type == "mouse") {
            return AxisTrigger::mouse();
        }
        throw std::runtime_error("Unknown axis trigger: " +
                                 triggerData.get<std::string>());
    }

    if (!triggerData.is_object()) {
        throw std::runtime_error("Invalid axis trigger definition");
    }

    std::string type;
    JSON_READ_STRING(triggerData, "type", type);
    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "mouse") {
        return AxisTrigger::mouse();
    }

    if (normalizedType == "controller") {
        int controllerId = -1;
        int axisIndex = -1;
        int axisIndexY = -1;
        JSON_READ_INT(triggerData, "id", controllerId);
        JSON_READ_INT(triggerData, "index", axisIndex);
        JSON_READ_INT(triggerData, "indexY", axisIndexY);
        if (axisIndex < 0 && triggerData.contains("indexes") &&
            triggerData["indexes"].is_array() &&
            triggerData["indexes"].size() == 2) {
            axisIndex = triggerData["indexes"][0].get<int>();
            axisIndexY = triggerData["indexes"][1].get<int>();
        }
        if (axisIndex < 0) {
            throw std::runtime_error(
                "Controller axis trigger is missing index");
        }
        return AxisTrigger::controller(controllerId, axisIndex, axisIndexY < 0,
                                       axisIndexY);
    }

    if (normalizedType == "custom") {
        if (triggerData.contains("triggers") &&
            triggerData["triggers"].is_array()) {
            const auto &triggers = triggerData["triggers"];
            if (triggers.size() == 2) {
                return AxisTrigger::custom(parseTrigger(triggers[0]),
                                           parseTrigger(triggers[1]), {}, {});
            }
            if (triggers.size() == 4) {
                return AxisTrigger::custom(
                    parseTrigger(triggers[0]), parseTrigger(triggers[1]),
                    parseTrigger(triggers[2]), parseTrigger(triggers[3]));
            }
        }

        Trigger positiveX{};
        Trigger negativeX{};
        Trigger positiveY{};
        Trigger negativeY{};
        bool hasPositive = false;
        bool hasNegative = false;

        auto it = triggerData.find("positiveX");
        if (it != triggerData.end()) {
            positiveX = parseTrigger(*it);
            hasPositive = true;
        }
        it = triggerData.find("negativeX");
        if (it != triggerData.end()) {
            negativeX = parseTrigger(*it);
            hasNegative = true;
        }
        it = triggerData.find("positiveY");
        if (it != triggerData.end()) {
            positiveY = parseTrigger(*it);
        }
        it = triggerData.find("negativeY");
        if (it != triggerData.end()) {
            negativeY = parseTrigger(*it);
        }
        it = triggerData.find("positive");
        if (!hasPositive && it != triggerData.end()) {
            positiveX = parseTrigger(*it);
            hasPositive = true;
        }
        it = triggerData.find("negative");
        if (!hasNegative && it != triggerData.end()) {
            negativeX = parseTrigger(*it);
            hasNegative = true;
        }

        if (!hasPositive || !hasNegative) {
            throw std::runtime_error("Custom axis trigger is incomplete");
        }

        return AxisTrigger::custom(positiveX, negativeX, positiveY, negativeY);
    }

    throw std::runtime_error("Unknown axis trigger type: " + type);
}

std::shared_ptr<InputAction> parseInputAction(const json &actionData) {
    if (!actionData.is_object()) {
        throw std::runtime_error("Input action entry must be an object");
    }

    std::string name;
    JSON_READ_STRING(actionData, "name", name);
    if (name.empty()) {
        throw std::runtime_error("Input action is missing a name");
    }

    std::shared_ptr<InputAction> action;

    if (actionData.contains("triggerAxes") &&
        actionData["triggerAxes"].is_array()) {
        std::vector<AxisTrigger> triggers;
        for (const auto &triggerData : actionData["triggerAxes"]) {
            triggers.push_back(parseAxisTrigger(triggerData));
        }

        action = InputAction::createAxisInputAction(name, triggers);

        JSON_READ_BOOL(actionData, "singleAxis", action->isAxisSingle);
        JSON_READ_BOOL(actionData, "normalize2D", action->normalize2D);
        JSON_READ_BOOL(actionData, "invertControllerY",
                       action->invertControllerY);
        JSON_READ_BOOL(actionData, "clampAxis", action->clampAxis);
        JSON_READ_FLOAT(actionData, "controllerDeadzone",
                        action->controllerDeadzone);
        JSON_READ_FLOAT(actionData, "axisScaleX", action->axisScaleX);
        JSON_READ_FLOAT(actionData, "axisScaleY", action->axisScaleY);

        auto it = actionData.find("axisScale");
        if (it != actionData.end()) {
            if (it->is_number()) {
                action->axisScaleX = it->get<float>();
                action->axisScaleY = it->get<float>();
            } else if (it->is_array() && it->size() == 2) {
                action->axisScaleX = (*it)[0].get<float>();
                action->axisScaleY = (*it)[1].get<float>();
            }
        }

        it = actionData.find("axisClamp");
        if (it != actionData.end() && it->is_array() && it->size() == 2) {
            action->axisClampMin = (*it)[0].get<float>();
            action->axisClampMax = (*it)[1].get<float>();
            action->clampAxis = true;
        }
    } else if (actionData.contains("triggerButtons") &&
               actionData["triggerButtons"].is_array()) {
        std::vector<Trigger> triggers;
        for (const auto &triggerData : actionData["triggerButtons"]) {
            triggers.push_back(parseTrigger(triggerData));
        }
        action = InputAction::createButtonInputAction(name, triggers);
    } else {
        throw std::runtime_error("Input action '" + name +
                                 "' has no triggerButtons or triggerAxes");
    }

    return action;
}

void loadInputActionsFromJson(Window &window, const json &inputData,
                              const std::string &baseDir) {
    if (inputData.is_string()) {
        loadInputActionsFromJson(window,
                                 loadJsonFile(resolveRuntimePath(
                                     baseDir, inputData.get<std::string>())),
                                 baseDir);
        return;
    }

    const json *actions = nullptr;
    if (inputData.is_array()) {
        actions = &inputData;
    } else if (inputData.is_object()) {
        auto it = inputData.find("actions");
        if (it != inputData.end() && it->is_array()) {
            actions = &(*it);
        } else {
            it = inputData.find("inputActions");
            if (it != inputData.end() && it->is_array()) {
                actions = &(*it);
            }
        }
    }

    if (actions == nullptr) {
        throw std::runtime_error("Invalid input actions payload");
    }

    for (const auto &actionData : *actions) {
        auto action = parseInputAction(actionData);
        if (window.getInputAction(action->name) == nullptr) {
            window.addInputAction(action);
        }
    }
}

std::shared_ptr<Effect> parseEffect(const json &effectData) {
    if (!effectData.is_object()) {
        throw std::runtime_error(
            "Render target effect entry must be an object");
    }

    std::string type;
    JSON_READ_STRING(effectData, "type", type);
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
        float magnitude = 16.0f;
        JSON_READ_FLOAT(effectData, "magnitude", magnitude);
        return Blur::create(magnitude);
    }
    if (normalizedType == "edgedetection") {
        return EdgeDetection::create();
    }
    if (normalizedType == "colorcorrection") {
        ColorCorrectionParameters params;
        JSON_READ_FLOAT(effectData, "exposure", params.exposure);
        JSON_READ_FLOAT(effectData, "contrast", params.contrast);
        JSON_READ_FLOAT(effectData, "saturation", params.saturation);
        JSON_READ_FLOAT(effectData, "gamma", params.gamma);
        JSON_READ_FLOAT(effectData, "temperature", params.temperature);
        JSON_READ_FLOAT(effectData, "tint", params.tint);
        return ColorCorrection::create(params);
    }
    if (normalizedType == "motionblur") {
        MotionBlurParameters params;
        JSON_READ_INT(effectData, "size", params.size);
        JSON_READ_FLOAT(effectData, "separation", params.separation);
        return MotionBlur::create(params);
    }
    if (normalizedType == "chromaticaberration") {
        ChromaticAberrationParameters params;
        JSON_READ_FLOAT(effectData, "red", params.red);
        JSON_READ_FLOAT(effectData, "green", params.green);
        JSON_READ_FLOAT(effectData, "blue", params.blue);
        Position2d direction;
        if (tryReadVec2(effectData, "direction", direction)) {
            params.direction = direction;
        }
        return ChromaticAberration::create(params);
    }
    if (normalizedType == "posterization") {
        PosterizationParameters params;
        JSON_READ_FLOAT(effectData, "levels", params.levels);
        return Posterization::create(params);
    }
    if (normalizedType == "pixelation") {
        PixelationParameters params;
        JSON_READ_INT(effectData, "pixelSize", params.pixelSize);
        return Pixelation::create(params);
    }
    if (normalizedType == "dilation") {
        DilationParameters params;
        JSON_READ_INT(effectData, "size", params.size);
        JSON_READ_FLOAT(effectData, "separation", params.separation);
        return Dilation::create(params);
    }
    if (normalizedType == "filmgrain") {
        FilmGrainParameters params;
        JSON_READ_FLOAT(effectData, "amount", params.amount);
        return FilmGrain::create(params);
    }

    throw std::runtime_error("Unknown render target effect type: " + type);
}

std::shared_ptr<Renderable>
createRenderable(Context &context, const json &objectData,
                 const std::string &baseDir,
                 std::vector<PendingComponent> &rigidbodies,
                 std::vector<PendingComponent> &standard,
                 std::vector<PendingComponent> &joints) {
    if (!objectData.is_object()) {
        return nullptr;
    }

    std::string type;
    tryReadStringAny(objectData, {"type"}, type);
    if (type.empty()) {
        return nullptr;
    }

    const std::string normalizedType = normalizeToken(type);
    const size_t generatedIndex = context.objects.size();

    if (normalizedType == "solid") {
        std::string solidType;
        tryReadStringAny(objectData, {"solid_type", "solidType"}, solidType);
        const std::string normalizedSolidType = normalizeToken(solidType);
        if (normalizedSolidType.empty()) {
            return nullptr;
        }

        Color color = Color::white();
        tryReadColorAny(objectData, {"color"}, color);

        auto object = std::make_shared<CoreObject>();

        if (normalizedSolidType == "cube" || normalizedSolidType == "box") {
            Size3d size{1.0f, 1.0f, 1.0f};
            tryReadVec3Any(objectData, {"size", "dimensions"}, size);
            *object = createBox(size, color);
        } else if (normalizedSolidType == "plane") {
            Position2d size{1.0f, 1.0f};
            tryReadVec2Any(objectData, {"size", "dimensions"}, size);
            *object = createPlane({size.x, size.y}, color);
        } else if (normalizedSolidType == "pyramid") {
            Size3d size{1.0f, 1.0f, 1.0f};
            tryReadVec3Any(objectData, {"size", "dimensions"}, size);
            *object = createPyramid(size, color);
        } else if (normalizedSolidType == "sphere") {
            float radius = 0.5f;
            int sectorCount = 36;
            int stackCount = 18;
            tryReadFloatAny(objectData, {"radius"}, radius);
            tryReadIntAny(objectData, {"sectorCount"}, sectorCount);
            tryReadIntAny(objectData, {"stackCount"}, stackCount);
            *object =
                createSphere(radius, static_cast<unsigned int>(sectorCount),
                             static_cast<unsigned int>(stackCount), color);
        } else if (normalizedSolidType == "capsule") {
            float radius = 0.35f;
            float height = 1.0f;
            tryReadFloatAny(objectData, {"radius"}, radius);
            tryReadFloatAny(objectData, {"height"}, height);
            *object = createCapsulePrimitive(radius, height, color);
        } else {
            throw std::runtime_error("Unknown solid type: " + solidType);
        }

        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);

        if (const json *materialField = findField(objectData, {"material"});
            materialField != nullptr && !isEmptyStringValue(*materialField)) {
            applyMaterial(*object,
                          loadMaterialDefinition(*materialField, baseDir));
        }

        applyTransform(*object, objectData);
        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "camera") {
        auto object = std::make_shared<CoreObject>();
        *object = createPyramid({0.65f, 0.45f, 0.65f},
                                Color{0.25f, 0.55f, 1.0f, 1.0f});
        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);
        applyTransform(*object, objectData);
        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "compound") {
        auto object = std::make_shared<CompoundObject>();
        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);

        if (const json *childrenNode = findField(objectData, {"objects"});
            childrenNode != nullptr && childrenNode->is_array()) {
            for (const auto &childData : *childrenNode) {
                auto child = createRenderable(context, childData, baseDir,
                                              rigidbodies, standard, joints);
                if (child == nullptr) {
                    continue;
                }
                auto childObject = std::dynamic_pointer_cast<GameObject>(child);
                if (childObject == nullptr) {
                    throw std::runtime_error(
                        "Compound objects can only contain GameObject-derived "
                        "children");
                }
                object->addObject(childObject.get());
                context.objectParents[static_cast<int>(childObject->getId())] =
                    static_cast<int>(object->getId());
            }
        }

        applyTransform(*object, objectData);
        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "model") {
        std::string source;
        tryReadStringAny(objectData, {"source"}, source);
        if (source.empty()) {
            return nullptr;
        }

        auto object = std::make_shared<Model>();
        object->fromResource(createRuntimeResource(
            baseDir, source, ResourceType::Model, "runtime-model"));

        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);

        if (const json *materialField = findField(objectData, {"material"});
            materialField != nullptr && !isEmptyStringValue(*materialField)) {
            applyMaterial(*object,
                          loadMaterialDefinition(*materialField, baseDir));
        }

        applyTransform(*object, objectData);
        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "particleemitter") {
        int maxParticles = 100;
        tryReadIntAny(objectData, {"maxParticles"}, maxParticles);

        auto object = std::make_shared<ParticleEmitter>(
            static_cast<unsigned int>(maxParticles));
        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);

        if (const json *textureField = findField(objectData, {"texture"});
            textureField != nullptr && !isEmptyStringValue(*textureField)) {
            object->attachTexture(loadTextureDefinition(
                *textureField, baseDir, TextureType::Color, false));
        }

        Color color = Color::white();
        if (tryReadColorAny(objectData, {"color"}, color)) {
            object->setColor(color);
        }

        bool useTexture = false;
        if (tryReadBoolAny(objectData, {"useTexture"}, useTexture)) {
            if (useTexture) {
                object->enableTexture();
            } else {
                object->disableTexture();
            }
        }

        applyTransform(*object, objectData);

        Position3d direction;
        if (tryReadVec3Any(objectData, {"direction"}, direction)) {
            object->setDirection(direction);
        }

        float spawnRadius = 0.1f;
        if (tryReadFloatAny(objectData, {"spawnRadius"}, spawnRadius)) {
            object->setSpawnRadius(spawnRadius);
        }

        float spawnRate = 10.0f;
        if (tryReadFloatAny(objectData, {"spawnRate"}, spawnRate)) {
            object->setSpawnRate(spawnRate);
        }

        std::string emissionType;
        if (tryReadStringAny(objectData, {"emissionType"}, emissionType)) {
            const std::string emissionToken = normalizeToken(emissionType);
            if (emissionToken == "ambient") {
                object->setEmissionType(ParticleEmissionType::Ambient);
            } else {
                object->setEmissionType(ParticleEmissionType::Fountain);
            }
        }

        bool emitOnce = false;
        if (tryReadBoolAny(objectData, {"emitOnce"}, emitOnce) && emitOnce) {
            object->emitOnce();
        }

        if (const json *settingsField = findField(objectData, {"settings"});
            settingsField != nullptr && settingsField->is_object()) {
            ParticleSettings settings;
            tryReadFloatAny(*settingsField, {"minLifetime"},
                            settings.minLifetime);
            tryReadFloatAny(*settingsField, {"maxLifetime"},
                            settings.maxLifetime);
            tryReadFloatAny(*settingsField, {"minSize"}, settings.minSize);
            tryReadFloatAny(*settingsField, {"maxSize"}, settings.maxSize);
            tryReadFloatAny(*settingsField, {"fadeSpeed"}, settings.fadeSpeed);
            tryReadFloatAny(*settingsField, {"gravity"}, settings.gravity);
            tryReadFloatAny(*settingsField, {"spread"}, settings.spread);
            tryReadFloatAny(*settingsField, {"speedVariation"},
                            settings.speedVariation);
            object->setParticleSettings(settings);
        }

        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "terrain") {
        auto object = std::make_shared<Terrain>();
        registerGameObject(context, *object, objectData, normalizedType,
                           generatedIndex);
        context.objects.push_back(object);

        if (const json *heightmapField = findField(objectData, {"heightmap"});
            heightmapField != nullptr && !isEmptyStringValue(*heightmapField)) {
            std::string heightmapPath = heightmapField->get<std::string>();
            object->heightmap =
                createRuntimeResource(baseDir, heightmapPath,
                                      ResourceType::Image, "runtime-heightmap");
            object->createdWithMap = true;
        }

        if (const json *moistureField =
                findField(objectData, {"moistureTexture"});
            moistureField != nullptr && !isEmptyStringValue(*moistureField)) {
            object->moistureTexture = loadTextureDefinition(
                *moistureField, baseDir, TextureType::Color, false);
        }

        if (const json *temperatureField =
                findField(objectData, {"temperatureTexture"});
            temperatureField != nullptr &&
            !isEmptyStringValue(*temperatureField)) {
            object->temperatureTexture = loadTextureDefinition(
                *temperatureField, baseDir, TextureType::Color, false);
        }

        if (const json *generatorField = findField(objectData, {"generator"});
            generatorField != nullptr && !generatorField->is_null() &&
            !isEmptyStringValue(*generatorField)) {
            object->generator = parseTerrainGenerator(*generatorField, baseDir);
        }

        tryReadIntAny(objectData, {"width"}, object->width);
        tryReadIntAny(objectData, {"height"}, object->height);

        int resolution = static_cast<int>(object->resolution);
        if (tryReadIntAny(objectData, {"resolution"}, resolution)) {
            object->resolution = static_cast<unsigned int>(resolution);
        }

        tryReadFloatAny(objectData, {"maxPeak"}, object->maxPeak);
        tryReadFloatAny(objectData, {"minPeak", "seaLevel"}, object->seaLevel);

        if (const json *biomesField = findField(objectData, {"biomes"});
            biomesField != nullptr && !biomesField->is_null() &&
            !isEmptyStringValue(*biomesField)) {
            appendBiomeList(*object, *biomesField, baseDir);
        }

        applyTransform(*object, objectData);
        collectPendingComponents(*object, objectData, baseDir, rigidbodies,
                                 standard, joints);
        return object;
    }

    if (normalizedType == "skybox") {
        const json *cubemapField = findField(objectData, {"cubemap"});
        if (cubemapField == nullptr) {
            return nullptr;
        }

        auto skybox = std::make_shared<Skybox>();
        skybox->cubemap = loadCubemapDefinition(*cubemapField, baseDir);
        context.objects.push_back(skybox);
        return skybox;
    }

    throw std::runtime_error("Unknown scene object type: " + type);
}

} // namespace

static std::shared_ptr<Context>
makeContextWithWindowOptions(std::string projectFile, void *metalView,
                             CoreWindowReference sdlInputWindow) {
    auto context = std::make_shared<Context>();

    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);
    context->editorRuntime = metalView != nullptr;

    int resWidth = 1280;
    int resHeight = 720;
    bool mouseCaptured = false;
    bool multisampling = false;
    float ssaoScale = 0.4f;
    bool editorControls = false;

    if (auto *windowTable = configTable["window"].as_table()) {
        if (auto *dimensions = (*windowTable)["dimensions"].as_array()) {
            if (dimensions->size() == 2 && (*dimensions)[0].is_integer() &&
                (*dimensions)[1].is_integer()) {
                resWidth = (*dimensions)[0].as_integer()->get();
                resHeight = (*dimensions)[1].as_integer()->get();
            }
        }
        mouseCaptured = (*windowTable)["mouse_capture"].value_or(false);
        multisampling = (*windowTable)["multisampling"].value_or(false);
        ssaoScale = (*windowTable)["ssaoScale"].value_or(0.4f);
    }
    if (auto *editorTable = configTable["editor"].as_table()) {
        editorControls = (*editorTable)["controls"].value_or(false);
    }
    Logger::getInstance().setConsoleFilter(false, true, true);

    context->window = std::make_unique<Window>(WindowConfiguration{
        .title = "Atlas Runtime",
        .width = resWidth,
        .height = resHeight,
        .renderScale = 1.f,
        .mouseCaptured = mouseCaptured,
        .multisampling = multisampling,
        .ssaoScale = ssaoScale,
        .metalTargetView = metalView,
        .sdlInputWindow = sdlInputWindow,
        .editorControls = editorControls,
    });

    context->projectFile =
        std::filesystem::absolute(std::move(projectFile)).string();
    context->projectDir =
        std::filesystem::path(context->projectFile).parent_path().string();
    context->sceneDir = context->projectDir;
    context->scene = std::make_shared<RuntimeScene>();
    context->scene->context = context;

    return context;
}

std::shared_ptr<Context> runtime::makeContext(std::string projectFile) {
    return makeContextWithWindowOptions(std::move(projectFile), nullptr,
                                        nullptr);
}

std::shared_ptr<Context>
runtime::makeContextForMetalView(std::string projectFile, void *metalView,
                                 CoreWindowReference sdlInputWindow) {
#ifdef METAL
    if (metalView == nullptr) {
        throw std::runtime_error("Metal view pointer cannot be null");
    }
    return makeContextWithWindowOptions(std::move(projectFile), metalView,
                                        sdlInputWindow);
#else
    (void)projectFile;
    (void)metalView;
    (void)sdlInputWindow;
    throw std::runtime_error(
        "makeContextForMetalView is only available with the Metal backend");
#endif
}

std::shared_ptr<Context> runtime::makeContextForMetalViewNonBlocking(
    std::string projectFile, void *metalView,
    CoreWindowReference sdlInputWindow) {
    auto context = runtime::makeContextForMetalView(std::move(projectFile),
                                                    metalView, sdlInputWindow);
    context->loadProject();
    return context;
}

void runtime::runProjectInMetalView(std::string projectFile, void *metalView,
                                    CoreWindowReference sdlInputWindow) {
    auto context = runtime::makeContextForMetalViewNonBlocking(
        std::move(projectFile), metalView, sdlInputWindow);
    while (context->stepFrame()) {
    }
    context->end();
}

void Context::initializeScripting() {
    if (runtime == nullptr) {
        runtime = JS_NewRuntime();
        if (runtime == nullptr) {
            throw std::runtime_error("Failed to create QuickJS runtime");
        }
    }

    if (context == nullptr) {
        context = JS_NewContext(runtime);
        if (context == nullptr) {
            throw std::runtime_error("Failed to create QuickJS context");
        }
        JS_SetContextOpaque(context, &scriptHost);
        runtime::scripting::installGlobals(context);
        JS_SetModuleLoaderFunc(runtime, runtime::scripting::normalizeModuleName,
                               runtime::scripting::loadModule, &scriptHost);
    }

    scriptHost.context = this;
    JS_SetContextOpaque(context, &scriptHost);
    runtime::scripting::clearSceneBindings(context, scriptHost);
    scriptHost.modules.clear();
    loadedScriptModules.clear();
    registerBuiltInScriptModules(*this);

    const std::string bundlePath =
        resolveRuntimePath(projectDir, RUNTIME_SCRIPT_BUNDLE_PATH);
    if (std::filesystem::exists(bundlePath)) {
        scriptHost.modules[scriptBundleModuleName] = readTextFile(bundlePath);
    }
}

std::string Context::toProjectScriptPath(const std::string &path) const {
    std::filesystem::path absolutePath = std::filesystem::path(path);
    if (!absolutePath.is_absolute()) {
        return normalizeScriptPath(path);
    }

    std::filesystem::path projectPath = std::filesystem::absolute(projectDir);
    std::error_code error;
    std::filesystem::path relative =
        std::filesystem::relative(absolutePath, projectPath, error);
    if (!error && !relative.empty()) {
        return normalizeScriptPath(relative.string());
    }

    return normalizeScriptPath(absolutePath.string());
}

std::string Context::registerScriptModule(const std::string &modulePath) {
    const std::string absolutePath =
        std::filesystem::absolute(modulePath).string();
    if (const auto it = loadedScriptModules.find(absolutePath);
        it != loadedScriptModules.end()) {
        return it->second;
    }

    const std::string moduleName = std::string(RUNTIME_FILE_MODULE_PREFIX) +
                                   toProjectScriptPath(absolutePath);
    scriptHost.modules[moduleName] = readTextFile(absolutePath);
    loadedScriptModules[absolutePath] = moduleName;
    return moduleName;
}

void Context::runWindowed() {
    window->setScene(scene.get());
    window->run();
}

bool Context::stepFrame() {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    if (scene == nullptr) {
        throw std::runtime_error("Scene is not initialized");
    }
    for (const auto &renderable : objects) {
        auto *object = dynamic_cast<GameObject *>(
            renderable != nullptr ? renderable.get() : nullptr);
        if (object != nullptr && isEditorLightObject(*this, *object)) {
            syncEditorLightObject(*this, *object);
        }
    }
    window->setScene(scene.get());
    return window->stepFrame();
}

bool Context::resize(int width, int height, float scale) {
    if (window == nullptr) {
        throw std::runtime_error("Window is not initialized");
    }
    window->resize(width, height, scale);
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

int Context::selectedObjectId() const {
    if (window == nullptr || window->getSelectedEditorObject() == nullptr) {
        return -1;
    }
    return static_cast<int>(window->getSelectedEditorObject()->getId());
}

std::string Context::selectedObjectName() const {
    int id = selectedObjectId();
    if (id < 0) {
        return {};
    }
    auto it = objectNames.find(id);
    if (it != objectNames.end()) {
        return it->second;
    }
    if (window != nullptr && window->getSelectedEditorObject() != nullptr &&
        !window->getSelectedEditorObject()->name.empty()) {
        return window->getSelectedEditorObject()->name;
    }
    return std::to_string(id);
}

namespace {
GameObject *findContextObject(const Context &context, int id) {
    for (const auto &renderable : context.objects) {
        if (renderable == nullptr) {
            continue;
        }
        auto *object = dynamic_cast<GameObject *>(renderable.get());
        if (object != nullptr && static_cast<int>(object->getId()) == id) {
            return object;
        }
    }
    return nullptr;
}

std::string editorObjectName(const Context &context, GameObject &object) {
    if (!object.name.empty()) {
        return object.name;
    }
    auto it = context.objectNames.find(static_cast<int>(object.getId()));
    if (it != context.objectNames.end()) {
        return it->second;
    }
    return std::to_string(object.getId());
}

std::string editorObjectType(const Context &context, GameObject &object) {
    const int id = static_cast<int>(object.getId());
    auto sceneType = context.objectSceneTypes.find(id);
    if (sceneType != context.objectSceneTypes.end()) {
        if (sceneType->second == "solid") {
            auto solidType = context.objectSceneSolidTypes.find(id);
            if (solidType != context.objectSceneSolidTypes.end() &&
                !solidType->second.empty()) {
                return solidType->second;
            }
        }
        return sceneType->second;
    }
    if (dynamic_cast<CompoundObject *>(&object) != nullptr) {
        return "compound";
    }
    if (dynamic_cast<Model *>(&object) != nullptr) {
        return "model";
    }
    if (dynamic_cast<Terrain *>(&object) != nullptr) {
        return "terrain";
    }
    if (dynamic_cast<ParticleEmitter *>(&object) != nullptr) {
        return "particleEmitter";
    }
    if (dynamic_cast<Fluid *>(&object) != nullptr) {
        return "fluid";
    }
    if (dynamic_cast<UIObject *>(&object) != nullptr) {
        return "uiObject";
    }
    if (dynamic_cast<CoreObject *>(&object) != nullptr) {
        return "solid";
    }
    return "gameObject";
}

json editorObjectJson(const Context &context, GameObject &object,
                      const std::unordered_map<int, std::vector<int>> &children,
                      std::unordered_set<int> &visiting) {
    const int id = static_cast<int>(object.getId());
    json node = json::object();
    node["id"] = id;
    node["viewportId"] = id;
    node["name"] = editorObjectName(context, object);
    node["type"] = editorObjectType(context, object);
    node["position"] = vec3ToJson(object.getPosition());
    node["rotation"] = rotationToJson(object.getRotation());
    node["scale"] = vec3ToJson(object.getScale());

    if (visiting.contains(id)) {
        return node;
    }
    visiting.insert(id);

    auto childrenIt = children.find(id);
    if (childrenIt != children.end() && !childrenIt->second.empty()) {
        node["children"] = json::array();
        for (int childId : childrenIt->second) {
            GameObject *child = findContextObject(context, childId);
            if (child != nullptr) {
                node["children"].push_back(
                    editorObjectJson(context, *child, children, visiting));
            }
        }
    }

    visiting.erase(id);
    return node;
}

std::string uniqueEditorObjectName(const Context &context,
                                   const std::string &baseName) {
    std::string base = baseName.empty() ? "Object" : baseName;
    auto exists = [&](const std::string &name) {
        const std::string normalized = normalizeToken(name);
        for (const auto &[id, existingName] : context.objectNames) {
            (void)id;
            if (existingName == name ||
                normalizeToken(existingName) == normalized) {
                return true;
            }
        }
        if (context.objectReferences.contains(name) ||
            context.objectReferences.contains(normalized)) {
            return true;
        }
        return false;
    };

    if (!exists(base)) {
        return base;
    }

    for (int index = 2; index < 100000; ++index) {
        std::string candidate = base + " " + std::to_string(index);
        if (!exists(candidate)) {
            return candidate;
        }
    }
    return base + " " + std::to_string(context.objects.size() + 1);
}
} // namespace

std::string Context::sceneObjectsJson() const {
    json snapshot = json::object();
    snapshot["name"] =
        currentSceneName.empty()
            ? std::filesystem::path(currentSceneFile).stem().string()
            : currentSceneName;
    snapshot["selectedId"] = selectedObjectId();
    snapshot["objects"] = json::array();

    std::unordered_map<int, std::vector<int>> children;
    for (const auto &[childId, parentId] : objectParents) {
        children[parentId].push_back(childId);
    }

    std::unordered_set<int> visiting;
    for (const auto &renderable : objects) {
        if (renderable == nullptr) {
            continue;
        }
        auto *object = dynamic_cast<GameObject *>(renderable.get());
        if (object == nullptr ||
            objectParents.contains(static_cast<int>(object->getId()))) {
            continue;
        }
        snapshot["objects"].push_back(
            editorObjectJson(*this, *object, children, visiting));
    }

    return snapshot.dump();
}

bool Context::selectObject(int id, bool focusCamera) {
    if (window == nullptr) {
        return false;
    }
    GameObject *object = findContextObject(*this, id);
    if (object == nullptr) {
        return false;
    }
    window->selectEditorObject(object, focusCamera);
    return true;
}

bool Context::renameObject(int id, const std::string &name) {
    if (name.empty()) {
        return false;
    }
    GameObject *object = findContextObject(*this, id);
    if (object == nullptr) {
        return false;
    }

    const std::string normalized = normalizeToken(name);
    for (const auto &[key, referenced] : objectReferences) {
        if ((key == name || key == normalized) && referenced != object) {
            return false;
        }
    }

    object->name = name;
    objectNames[id] = name;
    registerObjectReference(*this, name, object);
    registerObjectReference(*this, std::to_string(id), object);
    return true;
}

bool Context::setObjectParent(int childId, int parentId) {
    GameObject *child = findContextObject(*this, childId);
    if (child == nullptr) {
        return false;
    }

    GameObject *parent = nullptr;
    if (parentId >= 0) {
        parent = findContextObject(*this, parentId);
        if (parent == nullptr || parent == child) {
            return false;
        }

        int cursor = parentId;
        while (cursor >= 0) {
            if (cursor == childId) {
                return false;
            }
            auto parentIt = objectParents.find(cursor);
            if (parentIt == objectParents.end()) {
                break;
            }
            cursor = parentIt->second;
        }
    }

    if (parentId < 0) {
        objectParents.erase(childId);
        objectParentReferences.erase(childId);
        if (window != nullptr) {
            window->setEditorObjectParent(child, nullptr);
        }
        return true;
    }

    objectParents[childId] = parentId;
    objectParentReferences[childId] = std::to_string(parentId);
    if (window != nullptr) {
        window->setEditorObjectParent(child, parent);
    }

    return true;
}

bool Context::deleteObject(int id) {
    GameObject *object = findContextObject(*this, id);
    if (object == nullptr) {
        return false;
    }

    std::vector<int> childrenToDelete;
    for (const auto &[childId, parentId] : objectParents) {
        if (parentId == id) {
            childrenToDelete.push_back(childId);
        }
    }
    for (int childId : childrenToDelete) {
        deleteObject(childId);
    }

    const std::string name = serializableObjectName(*this, *object);
    const std::string reference = serializableObjectReference(*this, *object);
    deletedObjectReferences.push_back({name, reference});

    setObjectParent(id, -1);
    for (auto it = objectParents.begin(); it != objectParents.end();) {
        if (it->second == id) {
            objectParentReferences.erase(it->first);
            it = objectParents.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = objectReferences.begin(); it != objectReferences.end();) {
        if (it->second == object) {
            it = objectReferences.erase(it);
        } else {
            ++it;
        }
    }

    if (auto it = editorPointLights.find(id); it != editorPointLights.end()) {
        Light *light = it->second;
        if (scene != nullptr) {
            scene->pointLights.erase(std::remove(scene->pointLights.begin(),
                                                 scene->pointLights.end(),
                                                 light),
                                     scene->pointLights.end());
        }
        pointLights.erase(std::remove_if(pointLights.begin(), pointLights.end(),
                                         [&](const auto &entry) {
                                             return entry != nullptr &&
                                                    entry.get() == light;
                                         }),
                          pointLights.end());
        editorPointLights.erase(it);
    }
    if (auto it = editorSpotlights.find(id); it != editorSpotlights.end()) {
        Spotlight *light = it->second;
        if (scene != nullptr) {
            scene->spotlights.erase(std::remove(scene->spotlights.begin(),
                                                scene->spotlights.end(), light),
                                    scene->spotlights.end());
        }
        spotlights.erase(std::remove_if(spotlights.begin(), spotlights.end(),
                                        [&](const auto &entry) {
                                            return entry != nullptr &&
                                                   entry.get() == light;
                                        }),
                         spotlights.end());
        editorSpotlights.erase(it);
    }
    if (auto it = editorAreaLights.find(id); it != editorAreaLights.end()) {
        AreaLight *light = it->second;
        if (scene != nullptr) {
            scene->areaLights.erase(std::remove(scene->areaLights.begin(),
                                                scene->areaLights.end(), light),
                                    scene->areaLights.end());
        }
        areaLights.erase(std::remove_if(areaLights.begin(), areaLights.end(),
                                        [&](const auto &entry) {
                                            return entry != nullptr &&
                                                   entry.get() == light;
                                        }),
                         areaLights.end());
        editorAreaLights.erase(it);
    }
    if (auto it = editorDirectionalLights.find(id);
        it != editorDirectionalLights.end()) {
        DirectionalLight *light = it->second;
        if (scene != nullptr) {
            scene->directionalLights.erase(
                std::remove(scene->directionalLights.begin(),
                            scene->directionalLights.end(), light),
                scene->directionalLights.end());
        }
        directionalLights.erase(
            std::remove_if(directionalLights.begin(), directionalLights.end(),
                           [&](const auto &entry) {
                               return entry != nullptr && entry.get() == light;
                           }),
            directionalLights.end());
        editorDirectionalLights.erase(it);
    }
    editorLightSourceData.erase(id);
    objectNames.erase(id);
    objectSceneReferences.erase(id);
    objectSceneTypes.erase(id);
    objectSceneSolidTypes.erase(id);
    objectParentReferences.erase(id);

    for (const auto &renderable : objects) {
        auto *compound = dynamic_cast<CompoundObject *>(renderable.get());
        if (compound == nullptr) {
            continue;
        }
        auto &compoundObjects = compound->objects;
        compoundObjects.erase(
            std::remove(compoundObjects.begin(), compoundObjects.end(), object),
            compoundObjects.end());
    }

    if (window != nullptr) {
        window->removeObject(object);
    }

    objects.erase(std::remove_if(objects.begin(), objects.end(),
                                 [&](const auto &renderable) {
                                     return renderable != nullptr &&
                                            renderable.get() == object;
                                 }),
                  objects.end());
    return true;
}

int Context::createObject(const std::string &type, const std::string &name) {
    if (window == nullptr) {
        return -1;
    }

    const std::string normalized = normalizeToken(type);
    std::shared_ptr<GameObject> object;
    std::string sceneType = "solid";
    std::string solidType = normalized.empty() ? "cube" : normalized;
    std::string fallbackName = solidType;

    Position3d position = Position3d::zero();
    if (window->getSelectedEditorObject() != nullptr) {
        position = window->getSelectedEditorObject()->getPosition();
        position.x += 1.25f;
    } else if (window->getCamera() != nullptr) {
        position = window->getCamera()->target;
    }

    if (solidType == "cube" || solidType == "box") {
        solidType = "cube";
        auto core = std::make_shared<CoreObject>();
        *core = createBox({1.0f, 1.0f, 1.0f}, Color::white());
        object = core;
    } else if (solidType == "sphere") {
        auto core = std::make_shared<CoreObject>();
        *core = createSphere(0.5f, 36, 18, Color::white());
        object = core;
    } else if (solidType == "plane") {
        auto core = std::make_shared<CoreObject>();
        *core = createPlane({1.0f, 1.0f}, Color::white());
        object = core;
    } else if (solidType == "pyramid") {
        auto core = std::make_shared<CoreObject>();
        *core = createPyramid({1.0f, 1.0f, 1.0f}, Color::white());
        object = core;
    } else if (solidType == "capsule") {
        auto core = std::make_shared<CoreObject>();
        *core = createCapsulePrimitive(0.35f, 1.0f, Color::white());
        object = core;
    } else if (solidType == "group" || solidType == "empty" ||
               solidType == "emptygameobject") {
        sceneType = "compound";
        fallbackName = "Group";
        object = std::make_shared<CompoundObject>();
        solidType.clear();
    } else if (solidType == "camera") {
        sceneType = "camera";
        fallbackName = "Camera";
        auto core = std::make_shared<CoreObject>();
        *core = createPyramid({0.65f, 0.45f, 0.65f},
                              Color{0.25f, 0.55f, 1.0f, 1.0f});
        object = core;
        solidType.clear();
    } else if (solidType == "particleemitter" || solidType == "particles" ||
               solidType == "particlegenerator") {
        sceneType = "particleEmitter";
        fallbackName = "Particle Emitter";
        auto emitter = std::make_shared<ParticleEmitter>(100);
        emitter->setPosition(position);
        emitter->setSpawnRate(10.0f);
        object = emitter;
        solidType.clear();
    } else if (solidType == "terrain" || solidType == "landscape") {
        sceneType = "terrain";
        fallbackName = "Terrain";
        auto terrain = std::make_shared<Terrain>();
        terrain->width = 32;
        terrain->height = 32;
        terrain->resolution = 64;
        terrain->maxPeak = 1.0f;
        terrain->seaLevel = 0.0f;
        object = terrain;
        solidType.clear();
    } else if (solidType == "pointlight" || solidType == "light") {
        auto light = std::make_unique<Light>(position, Color::white(), 50.0f,
                                             Color::white(), 1.0f);
        light->createDebugObject();
        int id = registerEditorLightObject(*this, light->debugObject,
                                           json::object(), "pointLight");
        if (id < 0) {
            return -1;
        }
        const std::string displayName =
            uniqueEditorObjectName(*this, name.empty() ? "Point Light" : name);
        light->debugObject->name = displayName;
        objectNames[id] = displayName;
        objectSceneReferences[id] = std::to_string(id);
        registerObjectReference(*this, displayName, light->debugObject.get());
        editorPointLights[id] = light.get();
        scene->addLight(light.get());
        pointLights.push_back(std::move(light));
        window->selectEditorObject(light->debugObject.get(), true);
        return id;
    } else if (solidType == "spotlight" || solidType == "spot") {
        auto light = std::make_unique<Spotlight>(position, Position3d::down(),
                                                 Color::white(), 35.0f, 40.0f,
                                                 Color::white(), 1.0f, 50.0f);
        light->createDebugObject();
        int id = registerEditorLightObject(*this, light->debugObject,
                                           json::object(), "spotLight");
        if (id < 0) {
            return -1;
        }
        const std::string displayName =
            uniqueEditorObjectName(*this, name.empty() ? "Spot Light" : name);
        light->debugObject->name = displayName;
        objectNames[id] = displayName;
        objectSceneReferences[id] = std::to_string(id);
        registerObjectReference(*this, displayName, light->debugObject.get());
        editorSpotlights[id] = light.get();
        scene->addSpotlight(light.get());
        spotlights.push_back(std::move(light));
        window->selectEditorObject(light->debugObject.get(), true);
        return id;
    } else if (solidType == "directionallight" || solidType == "directional" ||
               solidType == "sun") {
        auto light = std::make_unique<DirectionalLight>(
            Position3d::down(), Color::white(), Color::white(), 1.0f);
        auto proxy = createEditorLightProxy("directionalLight", Color::white(),
                                            position);
        proxy->lookAt(position + Position3d::down(), Position3d::up());
        int id = registerEditorLightObject(*this, proxy, json::object(),
                                           "directionalLight");
        if (id < 0) {
            return -1;
        }
        const std::string displayName = uniqueEditorObjectName(
            *this, name.empty() ? "Directional Light" : name);
        proxy->name = displayName;
        objectNames[id] = displayName;
        objectSceneReferences[id] = std::to_string(id);
        registerObjectReference(*this, displayName, proxy.get());
        editorDirectionalLights[id] = light.get();
        scene->addDirectionalLight(light.get());
        directionalLights.push_back(std::move(light));
        window->selectEditorObject(proxy.get(), true);
        return id;
    } else if (solidType == "arealight" || solidType == "area") {
        auto light = std::make_unique<AreaLight>();
        light->position = position;
        light->createDebugObject();
        int id = registerEditorLightObject(*this, light->debugObject,
                                           json::object(), "areaLight");
        if (id < 0) {
            return -1;
        }
        const std::string displayName =
            uniqueEditorObjectName(*this, name.empty() ? "Area Light" : name);
        light->debugObject->name = displayName;
        objectNames[id] = displayName;
        objectSceneReferences[id] = std::to_string(id);
        registerObjectReference(*this, displayName, light->debugObject.get());
        editorAreaLights[id] = light.get();
        scene->addAreaLight(light.get());
        areaLights.push_back(std::move(light));
        window->selectEditorObject(light->debugObject.get(), true);
        return id;
    } else if (solidType == "ambientlight" || solidType == "ambient") {
        scene->setAmbientColor(Color::white());
        scene->setAmbientIntensity(2.0f);
        auto proxy =
            createEditorLightProxy("ambientLight", Color::white(), position);
        int id = registerEditorLightObject(*this, proxy, json::object(),
                                           "ambientLight");
        if (id < 0) {
            return -1;
        }
        const std::string displayName = uniqueEditorObjectName(
            *this, name.empty() ? "Ambient Light" : name);
        proxy->name = displayName;
        objectNames[id] = displayName;
        objectSceneReferences[id] = std::to_string(id);
        registerObjectReference(*this, displayName, proxy.get());
        window->selectEditorObject(proxy.get(), true);
        return id;
    } else {
        return -1;
    }

    object->setPosition(position);

    const int id = static_cast<int>(object->getId());
    const std::string displayName =
        uniqueEditorObjectName(*this, name.empty() ? fallbackName : name);
    object->name = displayName;
    objectNames[id] = displayName;
    objectSceneReferences[id] = std::to_string(id);
    objectSceneTypes[id] = sceneType;
    if (!solidType.empty()) {
        objectSceneSolidTypes[id] = solidType;
    }
    registerObjectReference(*this, displayName, object.get());
    registerObjectReference(*this, std::to_string(id), object.get());

    objects.push_back(object);
    window->addObject(object.get());
    window->selectEditorObject(object.get(), true);
    return id;
}

bool Context::saveCurrentScene() {
    if (currentSceneFile.empty()) {
        return false;
    }

    json sceneData = loadJsonFile(currentSceneFile);
    if (!sceneData.is_object()) {
        return false;
    }
    if (!sceneData.contains("objects") || !sceneData["objects"].is_array()) {
        sceneData["objects"] = json::array();
    }
    if (!sceneData.contains("lights") || !sceneData["lights"].is_array()) {
        sceneData["lights"] = json::array();
    }

    for (const auto &[name, reference] : deletedObjectReferences) {
        removeObjectNode(sceneData["objects"], name, reference);
    }

    json serializedLights = json::array();
    for (const auto &renderable : objects) {
        if (renderable == nullptr) {
            continue;
        }

        auto *object = dynamic_cast<GameObject *>(renderable.get());
        if (object == nullptr) {
            continue;
        }

        if (isEditorLightObject(*this, *object)) {
            serializedLights.push_back(
                serializeEditorLightObject(*this, *object));
            continue;
        }

        bool updated = false;
        for (auto &objectNode : sceneData["objects"]) {
            if (updateObjectNode(objectNode, *this, *object)) {
                updated = true;
                break;
            }
        }

        if (!updated) {
            sceneData["objects"].push_back(serializeNewObject(*this, *object));
        }
    }
    sceneData["lights"] = serializedLights;

    std::ofstream output(currentSceneFile, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << sceneData.dump(4) << '\n';
    bool good = output.good();
    if (good) {
        deletedObjectReferences.clear();
    }
    return good;
}

void Context::end() {
    if (window == nullptr) {
        return;
    }
    window->close();
    window->endRunLoop();
}

void Context::loadProject() {
    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);

    std::string defaultRenderer = "normal";
    bool globalIllumination = false;
    std::string mainScene = "main.ascene";
    std::vector<std::string> assetDirectories;
    bool useUpscaling = false;

    if (auto *renderer = configTable["renderer"].as_table()) {
        defaultRenderer = (*renderer)["default"].value_or("normal");
        globalIllumination = (*renderer)["global_illumination"].value_or(false);
        useUpscaling = (*renderer)["use_upscaling"].value_or(false);
    }

    if (auto *gameTable = configTable["game"].as_table()) {
        mainScene = (*gameTable)["main_scene"].value_or("main.ascene");

        assetDirectories.clear();
        if (auto *assets = (*gameTable)["assets"].as_array()) {
            for (const auto &assetDir : *assets) {
                if (assetDir.is_string()) {
                    assetDirectories.push_back(assetDir.as_string()->get());
                }
            }
        }
    }

    scriptRegistry.clear();
    if (auto *scriptsTable = configTable["scripts"].as_table()) {
        for (const auto &[name, value] : *scriptsTable) {
            if (!value.is_string()) {
                continue;
            }
            scriptRegistry[std::string(name.str())] = toProjectScriptPath(
                resolveRuntimePath(projectDir, value.as_string()->get()));
        }
    }

    config.renderer = defaultRenderer;
    config.globalIllumination = globalIllumination;
    config.mainScene = mainScene;
    config.assetDirectories = assetDirectories;
    config.useUpscaling = useUpscaling;

    Workspace::get().setRootPath(projectDir);
    initializeScripting();
}

void RuntimeScene::update(Window &window) {
    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving) {
        if (context != nullptr && context->context != nullptr) {
            runtime::scripting::dispatchInteractiveFrame(
                context->context, context->scriptHost, window,
                window.getDeltaTime());
        }
        return;
    }

    if (context->cameraActions.size() >= 3) {
        context->camera->updateWithActions(window, context->cameraActions[0],
                                           context->cameraActions[1],
                                           context->cameraActions[2]);
    } else {
        context->camera->update(window);
    }

    if (context->context != nullptr) {
        runtime::scripting::dispatchInteractiveFrame(
            context->context, context->scriptHost, window,
            window.getDeltaTime());
    }
}

void RuntimeScene::onMouseMove(Window &window, Movement2d movement) {
    if (context != nullptr && context->context != nullptr) {
        const auto [x, y] = window.getCursorPosition();
        MousePacket packet;
        packet.xpos = static_cast<float>(x);
        packet.ypos = static_cast<float>(y);
        packet.xoffset = movement.x;
        packet.yoffset = movement.y;
        packet.constrainPitch = true;
        packet.firstMouse = context->scriptHost.interactiveFirstMouse;
        runtime::scripting::dispatchInteractiveMouseMove(
            context->context, context->scriptHost, window, packet,
            window.getDeltaTime());
    }

    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving || context->cameraActions.size() >= 3) {
        return;
    }
    context->camera->updateLook(window, movement);
}

void RuntimeScene::onMouseScroll(Window &window, Movement2d offset) {
    if (context != nullptr && context->context != nullptr) {
        MouseScrollPacket packet{offset.x, offset.y};
        runtime::scripting::dispatchInteractiveMouseScroll(
            context->context, context->scriptHost, packet,
            window.getDeltaTime());
    }

    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving || context->cameraActions.size() >= 3) {
        return;
    }
    context->camera->updateZoom(window, offset);
}

void Context::loadMainScene(Window &window) {
    RUNTIME_LOG("Loading main scene: " + config.mainScene);
    const std::string resolvedScenePath =
        resolveRuntimePath(projectDir, config.mainScene);
    json sceneData = loadJsonFile(resolvedScenePath);
    currentSceneFile = resolvedScenePath;
    sceneDir = std::filesystem::path(resolvedScenePath).parent_path().string();
    currentSceneName = std::filesystem::path(resolvedScenePath).stem().string();
    auto sceneNameIt = sceneData.find("name");
    if (sceneNameIt != sceneData.end() && sceneNameIt->is_string()) {
        currentSceneName = sceneNameIt->get<std::string>();
    }
    loadScene(window, sceneData);
}

void Context::loadScene(Window &window, const json &sceneData) {
    auto sceneNameIt = sceneData.find("name");
    if (sceneNameIt != sceneData.end() && sceneNameIt->is_string()) {
        currentSceneName = sceneNameIt->get<std::string>();
    }

    scene->atmosphere.resetRuntimeState();
    scene->setUseAtmosphereSkybox(false);
    scene->setAtmosphereSkybox(nullptr);
    scene->setSkybox(nullptr);
    scene->setAutomaticAmbient(false);
    scene->setEnvironment(Environment());
    scene->atmosphere = Atmosphere();
    scene->clearLights();
    scene->setAmbientColor(Color::white());
    scene->setAmbientIntensity(0.5f);

    for (const auto &pointLight : pointLights) {
        if (pointLight != nullptr && pointLight->debugObject != nullptr) {
            window.removeObject(pointLight->debugObject.get());
        }
    }
    for (const auto &spotlight : spotlights) {
        if (spotlight != nullptr && spotlight->debugObject != nullptr) {
            window.removeObject(spotlight->debugObject.get());
        }
    }
    for (const auto &areaLight : areaLights) {
        if (areaLight != nullptr && areaLight->debugObject != nullptr) {
            window.removeObject(areaLight->debugObject.get());
        }
    }
    for (const auto &object : objects) {
        if (object != nullptr) {
            window.removeObject(object.get());
        }
    }

    objects.clear();
    objectReferences.clear();
    objectNames.clear();
    objectSceneReferences.clear();
    objectSceneTypes.clear();
    objectSceneSolidTypes.clear();
    objectParentReferences.clear();
    objectParents.clear();
    editorPointLights.clear();
    editorSpotlights.clear();
    editorAreaLights.clear();
    editorDirectionalLights.clear();
    editorLightSourceData.clear();
    deletedObjectReferences.clear();
    renderTargets.clear();
    directionalLights.clear();
    pointLights.clear();
    spotlights.clear();
    areaLights.clear();
    cameraActions.clear();
    cameraAutomaticMoving = false;
    camera = std::make_unique<Camera>();
    window.resetInputActions();
    if (context != nullptr) {
        runtime::scripting::clearSceneBindings(context, scriptHost);
    } else {
        scriptHost.generation += 1;
    }

    const std::string baseDir = sceneDir.empty() ? projectDir : sceneDir;
    RuntimeEnvironmentDefinition environmentDefinition =
        loadEnvironmentDefinition(sceneData, baseDir);
    scene->setEnvironment(std::move(environmentDefinition.environment));
    scene->atmosphere = environmentDefinition.atmosphere;
    scene->setUseAtmosphereSkybox(environmentDefinition.useAtmosphereSkybox);
    scene->setAutomaticAmbient(environmentDefinition.automaticAmbient);

    if (sceneData.contains("inputActions")) {
        loadInputActionsFromJson(window, sceneData["inputActions"], baseDir);
    } else if (sceneData.contains("input_actions")) {
        loadInputActionsFromJson(window, sceneData["input_actions"], baseDir);
    }

    if (sceneData.contains("targets") && sceneData["targets"].is_array()) {
        for (const auto &targetData : sceneData["targets"]) {
            if (!targetData.is_object()) {
                continue;
            }

            std::string type;
            JSON_READ_STRING(targetData, "type", type);
            if (type.empty()) {
                continue;
            }

            std::unique_ptr<RenderTarget> target;
            const std::string normalizedType = normalizeToken(type);
            if (normalizedType == "multisampled") {
                target = std::make_unique<RenderTarget>(
                    window, RenderTargetType::Multisampled);
            } else if (normalizedType == "scene") {
                target = std::make_unique<RenderTarget>(
                    window, RenderTargetType::Scene);
            } else {
                throw std::runtime_error("Unknown render target type: " + type);
            }

            if (targetData.contains("effects") &&
                targetData["effects"].is_array()) {
                for (const auto &effectData : targetData["effects"]) {
                    try {
                        target->addEffect(parseEffect(effectData));
                    } catch (const std::exception &error) {
                        RUNTIME_LOG(
                            "Skipping incomplete render target effect: " +
                            std::string(error.what()));
                    }
                }
            }

            bool render = false;
            bool display = false;
            JSON_READ_BOOL(targetData, "render", render);
            JSON_READ_BOOL(targetData, "display", display);

            if (render) {
                window.addRenderTarget(target.get());
            }
            if (display) {
                target->display(window);
            }

            std::string name;
            JSON_READ_STRING(targetData, "name", name);
            if (name.empty()) {
                continue;
            }
            renderTargets[name] = std::move(target);
        }
    }

    if (sceneData.contains("camera") && sceneData["camera"].is_object()) {
        const auto &cameraData = sceneData["camera"];

        tryReadVec3(cameraData, "position", camera->position);

        Position3d target;
        if (tryReadVec3(cameraData, "target", target)) {
            camera->lookAt(target);
        }

        JSON_READ_FLOAT(cameraData, "fov", camera->fov);
        JSON_READ_FLOAT(cameraData, "nearClip", camera->nearClip);
        JSON_READ_FLOAT(cameraData, "farClip", camera->farClip);
        JSON_READ_FLOAT(cameraData, "orthoSize", camera->orthographicSize);
        JSON_READ_FLOAT(cameraData, "movementSpeed", camera->movementSpeed);
        JSON_READ_FLOAT(cameraData, "mouseSensitivity",
                        camera->mouseSensitivity);
        JSON_READ_FLOAT(cameraData, "controllerLookSensitivity",
                        camera->controllerLookSensitivity);
        JSON_READ_FLOAT(cameraData, "lookSmoothness", camera->lookSmoothness);
        JSON_READ_BOOL(cameraData, "orthographic", camera->useOrthographic);
        JSON_READ_FLOAT(cameraData, "focusDepth", camera->focusDepth);
        JSON_READ_FLOAT(cameraData, "focusRange", camera->focusRange);
        JSON_READ_BOOL(cameraData, "automaticMoving", cameraAutomaticMoving);

        if (cameraData.contains("inputActions")) {
            loadInputActionsFromJson(window, cameraData["inputActions"],
                                     baseDir);
        }

        if (cameraData.contains("actions") &&
            cameraData["actions"].is_array()) {
            for (const auto &actionValue : cameraData["actions"]) {
                if (actionValue.is_string()) {
                    cameraActions.push_back(actionValue.get<std::string>());
                }
            }
        }
    }

    window.setCamera(camera.get());

    if (sceneData.contains("lights") && sceneData["lights"].is_array()) {
        for (const auto &lightData : sceneData["lights"]) {
            if (!lightData.is_object()) {
                continue;
            }

            std::string type;
            JSON_READ_STRING(lightData, "type", type);
            if (type.empty()) {
                continue;
            }
            const std::string normalizedType = normalizeToken(type);

            if (normalizedType == "ambient" ||
                normalizedType == "ambientlight") {
                Color ambientColor = Color::white();
                float intensity = 0.5f;
                tryReadColor(lightData, "color", ambientColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                scene->setAmbientColor(ambientColor);
                scene->setAmbientIntensity(intensity * 4.0f);
                if (editorRuntime) {
                    Position3d position = Position3d::zero();
                    tryReadVec3(lightData, "position", position);
                    auto object = createEditorLightProxy(
                        "ambientLight", ambientColor, position);
                    int id = registerEditorLightObject(*this, object, lightData,
                                                       "ambientLight");
                    (void)id;
                }
                continue;
            }

            if (normalizedType == "directional" ||
                normalizedType == "directionallight") {
                Position3d direction = Position3d::down();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                int shadowResolution = 4096;
                bool castsShadows = false;

                tryReadVec3(lightData, "direction", direction);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);

                auto light = std::make_unique<DirectionalLight>(
                    direction.normalized(), color, shineColor, intensity);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (editorRuntime) {
                    Position3d position = Position3d::zero();
                    tryReadVec3(lightData, "position", position);
                    auto object = createEditorLightProxy("directionalLight",
                                                         color, position);
                    object->lookAt(position + direction.normalized(),
                                   Position3d::up());
                    int id = registerEditorLightObject(*this, object, lightData,
                                                       "directionalLight");
                    if (id >= 0) {
                        editorDirectionalLights[id] = light.get();
                    }
                }
                scene->addDirectionalLight(light.get());
                directionalLights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "point" || normalizedType == "pointlight") {
                Position3d position = Position3d::zero();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                float distance = 50.0f;
                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;

                tryReadVec3(lightData, "position", position);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_FLOAT(lightData, "distance", distance);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                auto light = std::make_unique<Light>(position, color, distance,
                                                     shineColor, intensity);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (editorRuntime) {
                    light->createDebugObject();
                    if (light->debugObject != nullptr) {
                        int id = registerEditorLightObject(
                            *this, light->debugObject, lightData, "pointLight");
                        if (id >= 0) {
                            editorPointLights[id] = light.get();
                            if (addDebugObject) {
                                editorLightSourceData[id]["addDebugObject"] =
                                    true;
                            }
                        }
                    }
                }
                scene->addLight(light.get());
                pointLights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "spot" || normalizedType == "spotlight") {
                Position3d position = Position3d::zero();
                Position3d direction = Position3d::down();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                float range = 50.0f;
                float cutOff = 35.0f;
                float outerCutoff = 40.0f;
                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;

                tryReadVec3(lightData, "position", position);
                tryReadVec3(lightData, "direction", direction);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_FLOAT(lightData, "range", range);
                JSON_READ_FLOAT(lightData, "cutoff", cutOff);
                JSON_READ_FLOAT(lightData, "outerCutoff", outerCutoff);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                auto light = std::make_unique<Spotlight>(
                    position, direction.normalized(), color, cutOff,
                    outerCutoff, shineColor, intensity, range);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (editorRuntime) {
                    light->createDebugObject();
                    if (light->debugObject != nullptr) {
                        int id = registerEditorLightObject(
                            *this, light->debugObject, lightData, "spotLight");
                        if (id >= 0) {
                            editorSpotlights[id] = light.get();
                            if (addDebugObject) {
                                editorLightSourceData[id]["addDebugObject"] =
                                    true;
                            }
                        }
                    }
                }
                scene->addSpotlight(light.get());
                spotlights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "area" || normalizedType == "arealight") {
                auto light = std::make_unique<AreaLight>();
                light->right = light->right.normalized();
                light->up = light->up.normalized();

                tryReadVec3(lightData, "position", light->position);
                tryReadVec3(lightData, "right", light->right);
                tryReadVec3(lightData, "up", light->up);
                light->right = light->right.normalized();
                light->up = light->up.normalized();

                Position2d size;
                if (tryReadVec2(lightData, "size", size)) {
                    light->size = Size2d{size.x, size.y};
                }

                tryReadColor(lightData, "color", light->color);
                tryReadColor(lightData, "shineColor", light->shineColor);
                JSON_READ_FLOAT(lightData, "intensity", light->intensity);
                JSON_READ_FLOAT(lightData, "range", light->range);
                JSON_READ_FLOAT(lightData, "angle", light->angle);
                JSON_READ_BOOL(lightData, "castsBothSides",
                               light->castsBothSides);

                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (editorRuntime) {
                    light->createDebugObject();
                    if (light->debugObject != nullptr) {
                        int id = registerEditorLightObject(
                            *this, light->debugObject, lightData, "areaLight");
                        if (id >= 0) {
                            editorAreaLights[id] = light.get();
                            if (addDebugObject) {
                                editorLightSourceData[id]["addDebugObject"] =
                                    true;
                            }
                        }
                    }
                }
                scene->addAreaLight(light.get());
                areaLights.push_back(std::move(light));
                continue;
            }

            throw std::runtime_error("Unknown light type: " + type);
        }
    }

    if (environmentDefinition.useGlobalLight) {
        scene->atmosphere.useGlobalLight();
        if (environmentDefinition.atmosphereCastsShadows) {
            scene->atmosphere.castShadowsFromSunlight(
                environmentDefinition.atmosphereShadowResolution);
        }
    }

    std::vector<PendingComponent> rigidbodyComponents;
    std::vector<PendingComponent> standardComponents;
    std::vector<PendingComponent> jointComponents;
    std::vector<std::shared_ptr<Renderable>> topLevelRenderables;

    if (sceneData.contains("objects") && sceneData["objects"].is_array()) {
        for (const auto &objectData : sceneData["objects"]) {
            topLevelRenderables.push_back(createRenderable(
                *this, objectData, baseDir, rigidbodyComponents,
                standardComponents, jointComponents));
        }
    }

    resolveObjectParentReferences(*this);
    for (const auto &[childId, parentId] : objectParents) {
        GameObject *child = findContextObject(*this, childId);
        GameObject *parent = findContextObject(*this, parentId);
        if (auto *compound = dynamic_cast<CompoundObject *>(parent);
            compound != nullptr &&
            std::ranges::find(compound->objects, child) !=
                compound->objects.end()) {
            continue;
        }
        window.setEditorObjectParent(child, parent);
    }

    for (const auto &pending : rigidbodyComponents) {
        try {
            attachComponent(*this, pending);
        } catch (const std::exception &error) {
            RUNTIME_LOG("Skipping incomplete rigidbody component: " +
                        std::string(error.what()));
        }
    }
    for (const auto &pending : standardComponents) {
        try {
            attachComponent(*this, pending);
        } catch (const std::exception &error) {
            RUNTIME_LOG("Skipping incomplete component: " +
                        std::string(error.what()));
        }
    }
    for (const auto &pending : jointComponents) {
        try {
            attachComponent(*this, pending);
        } catch (const std::exception &error) {
            RUNTIME_LOG("Skipping incomplete joint component: " +
                        std::string(error.what()));
        }
    }

    for (const auto &renderable : topLevelRenderables) {
        if (renderable == nullptr) {
            continue;
        }

        if (auto skybox = std::dynamic_pointer_cast<Skybox>(renderable);
            skybox != nullptr) {
            scene->setSkybox(skybox);
            continue;
        }

        if (auto object = std::dynamic_pointer_cast<GameObject>(renderable);
            object != nullptr) {
            auto parentIt =
                objectParents.find(static_cast<int>(object->getId()));
            if (parentIt != objectParents.end()) {
                if (auto *parentObject =
                        findContextObject(*this, parentIt->second);
                    dynamic_cast<CompoundObject *>(parentObject) != nullptr) {
                    continue;
                }
            }
        }

        window.addObject(renderable.get());
    }
}
