/*
 shader.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader utilities and functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/core/default_shaders.h"
#include "atlas/object.h"
#include "atlas/tracer/log.h"
#include "opal/opal.h"
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

std::map<std::pair<AtlasVertexShader, AtlasFragmentShader>, ShaderProgram>
    ShaderProgram::shaderCache = {};

std::map<AtlasVertexShader, VertexShader> VertexShader::vertexShaderCache = {};

std::map<AtlasFragmentShader, FragmentShader>
    FragmentShader::fragmentShaderCache = {};

std::map<AtlasComputeShader, ComputeShader> ComputeShader::computeShaderCache =
    {};

std::map<AtlasComputeShader, ShaderProgram> ShaderProgram::computeShaderCache =
    {};

VertexShader VertexShader::fromDefaultShader(AtlasVertexShader shader) {
    if (VertexShader::vertexShaderCache.contains(shader)) {
        return VertexShader::vertexShaderCache[shader];
    }
    VertexShader vertexShader;
    switch (shader) {
    case AtlasVertexShader::Debug: {
        vertexShader = VertexShader::fromSource(DEBUG_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Color: {
        vertexShader = VertexShader::fromSource(COLOR_VERT);
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Main: {
        vertexShader = VertexShader::fromSource(MAIN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Lighting,  ShaderCapability::Textures,
            ShaderCapability::Shadows,   ShaderCapability::EnvironmentMapping,
            ShaderCapability::IBL,       ShaderCapability::Material,
            ShaderCapability::Instances, ShaderCapability::Environment};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Texture: {
        vertexShader = VertexShader::fromSource(TEXTURE_VERT);
        vertexShader.desiredAttributes = {0, 1, 2};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Fullscreen: {
        vertexShader = VertexShader::fromSource(FULLSCREEN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Skybox: {
        vertexShader = VertexShader::fromSource(SKYBOX_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Depth: {
        vertexShader = VertexShader::fromSource(DEPTH_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Particle: {
        vertexShader = VertexShader::fromSource(PARTICLE_VERT);
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Text: {
        vertexShader = VertexShader::fromSource(TEXT_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::PointLightShadow: {
        vertexShader = VertexShader::fromSource(POINT_DEPTH_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::PointLightShadowNoGeom: {
#ifdef VULKAN
        vertexShader = VertexShader::fromSource(POINT_DEPTH_NOGEOM_VERT);
#else
        vertexShader = VertexShader::fromSource(POINT_DEPTH_VERT);
#endif
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Deferred: {
        vertexShader = VertexShader::fromSource(DEFERRED_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Textures, ShaderCapability::Deferred,
            ShaderCapability::Material, ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Light: {
        vertexShader = VertexShader::fromSource(LIGHT_VERT);
        vertexShader.desiredAttributes = {0, 2};
        vertexShader.capabilities = {
            ShaderCapability::Shadows, ShaderCapability::Lighting,
            ShaderCapability::EnvironmentMapping,
            ShaderCapability::LightDeferred, ShaderCapability::Environment};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Terrain: {
        vertexShader = VertexShader::fromSource(TERRAIN_VERT);
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Volumetric: {
        vertexShader = VertexShader::fromSource(VOLUMETRIC_VERT);
        vertexShader.desiredAttributes = {0, 2};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Fluid: {
        vertexShader = VertexShader::fromSource(FLUID_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {ShaderCapability::Fluid,
                                     ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default vertex shader");
    }

    return vertexShader;
}

VertexShader VertexShader::fromSource(const char *source) {
    VertexShader shader;
    shader.source = source;
    shader.shaderId = 0;
    shader.desiredAttributes = {};
    return shader;
}

void VertexShader::compile() {
    if (shaderId != 0) {
        if (shader != nullptr) {
            return;
        }
        shaderId = 0;
    }

    if (source == nullptr) {
        throw std::runtime_error("Vertex shader source is null");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Vertex);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Vertex shader compilation failed: " +
                    std::string(infoLog));
        throw std::runtime_error(
            std::string("Vertex shader compilation failed: ") + infoLog);
    }

    atlas_log("Vertex shader compiled successfully");
    this->shaderId = shader->shaderID;
}

ComputeShader ComputeShader::fromDefaultShader(AtlasComputeShader shader) {
    if (ComputeShader::computeShaderCache.contains(shader)) {
        return ComputeShader::computeShaderCache[shader];
    }

    ComputeShader computeShader;
    switch (shader) {
    case AtlasComputeShader::DDGI: {
#ifdef METAL
        computeShader = ComputeShader::fromSource(DDGI);
        computeShader.fromDefaultShaderType = shader;
        ComputeShader::computeShaderCache[shader] = computeShader;
        break;
#else
        throw std::runtime_error(
            "AtlasComputeShader::DDGI is only supported on Metal");
#endif
    }
    case AtlasComputeShader::DDGI_WRITE: {
#ifdef METAL
        computeShader = ComputeShader::fromSource(DDGI_WRITE);
        computeShader.fromDefaultShaderType = shader;
        ComputeShader::computeShaderCache[shader] = computeShader;
        break;
#else
        throw std::runtime_error(
            "AtlasComputeShader::DDGI_WRITE is only supported on Metal");
#endif
    }
    case AtlasComputeShader::PathTracer: {
#ifdef METAL
        computeShader = ComputeShader::fromSource(PATH);
        computeShader.fromDefaultShaderType = shader;
        ComputeShader::computeShaderCache[shader] = computeShader;
        break;
#else
        throw std::runtime_error(
            "AtlasComputeShader::PathTracer is only supported on Metal");
#endif
    }
    case AtlasComputeShader::PathDenoiser: {
#ifdef METAL
        computeShader = ComputeShader::fromSource(PATH_DENOISE);
        computeShader.fromDefaultShaderType = shader;
        ComputeShader::computeShaderCache[shader] = computeShader;
        break;
#else
        throw std::runtime_error(
            "AtlasComputeShader::PathDenoiser is only supported on Metal");
#endif
    }
    default:
        throw std::runtime_error("Unknown default compute shader");
    }

    return computeShader;
}

ComputeShader ComputeShader::fromSource(const char *source) {
    ComputeShader shader;
    shader.source = source;
    shader.shaderId = 0;
    shader.desiredAttributes = {};
    return shader;
}

void ComputeShader::compile() {
    if (shaderId != 0) {
        if (shader != nullptr) {
            return;
        }
        shaderId = 0;
    }

    if (source == nullptr) {
        throw std::runtime_error("Compute shader source is null");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Compute);
    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Compute shader compilation failed: " +
                    std::string(infoLog));
        throw std::runtime_error(
            std::string("Compute shader compilation failed: ") + infoLog);
    }

    atlas_log("Compute shader compiled successfully");
    this->shaderId = shader->shaderID;
}

FragmentShader FragmentShader::fromDefaultShader(AtlasFragmentShader shader) {
    if (FragmentShader::fragmentShaderCache.contains(shader)) {
        return FragmentShader::fragmentShaderCache[shader];
    }
    FragmentShader fragmentShader;
    switch (shader) {
    case AtlasFragmentShader::Debug: {
        fragmentShader = FragmentShader::fromSource(DEBUG_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Color: {
        fragmentShader = FragmentShader::fromSource(COLOR_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Main: {
        fragmentShader = FragmentShader::fromSource(MAIN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::GaussianBlur: {
        fragmentShader = FragmentShader::fromSource(GAUSSIAN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Texture: {
        fragmentShader = FragmentShader::fromSource(TEXTURE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Fullscreen: {
        fragmentShader = FragmentShader::fromSource(FULLSCREEN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Skybox: {
        fragmentShader = FragmentShader::fromSource(SKYBOX_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Empty: {
        fragmentShader = FragmentShader::fromSource(EMPTY_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Particle: {
        fragmentShader = FragmentShader::fromSource(PARTICLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Text: {
        fragmentShader = FragmentShader::fromSource(TEXT_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::PointLightShadow: {
        fragmentShader = FragmentShader::fromSource(POINT_DEPTH_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::PointLightShadowNoGeom: {
#ifdef OPENGL
        fragmentShader = FragmentShader::fromSource(EMPTY_FRAG);
#elif defined(VULKAN) || defined(METAL)
        fragmentShader = FragmentShader::fromSource(POINT_DEPTH_FRAG);
#endif
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Deferred: {
        fragmentShader = FragmentShader::fromSource(DEFERRED_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Light: {
        fragmentShader = FragmentShader::fromSource(LIGHT_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAO: {
        fragmentShader = FragmentShader::fromSource(SSAO_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAOBlur: {
        fragmentShader = FragmentShader::fromSource(SSAO_BLUR_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Terrain: {
        fragmentShader = FragmentShader::fromSource(TERRAIN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Volumetric: {
        fragmentShader = FragmentShader::fromSource(VOLUMETRIC_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Downsample: {
        fragmentShader = FragmentShader::fromSource(DOWNSAMPLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Upsample: {
        fragmentShader = FragmentShader::fromSource(UPSAMPLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Fluid: {
        fragmentShader = FragmentShader::fromSource(FLUID_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSR: {
        fragmentShader = FragmentShader::fromSource(SSR_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default fragment shader");
    }
    return fragmentShader;
}

FragmentShader FragmentShader::fromSource(const char *source) {
    FragmentShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void FragmentShader::compile() {
    if (shaderId != 0) {
        if (shader != nullptr) {
            return;
        }
        shaderId = 0;
    }

    if (source == nullptr) {
        throw std::runtime_error("Fragment shader source is null");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Fragment);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Fragment shader compilation failed: " +
                    std::string(infoLog));
        throw std::runtime_error(
            std::string("Fragment shader compilation failed: ") + infoLog);
    }

    atlas_log("Fragment shader compiled successfully");
    this->shaderId = shader->shaderID;
}

GeometryShader GeometryShader::fromDefaultShader(AtlasGeometryShader shader) {
    switch (shader) {
    case AtlasGeometryShader::PointLightShadow:
        return GeometryShader::fromSource(POINT_DEPTH_GEOM);
    default:
        throw std::runtime_error("Unknown default geometry shader");
    }
}

GeometryShader GeometryShader::fromSource(const char *source) {
    GeometryShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void GeometryShader::compile() {
    if (shaderId != 0) {
        if (shader != nullptr) {
            return;
        }
        shaderId = 0;
    }

    if (source == nullptr) {
        throw std::runtime_error("Geometry shader source is null");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Geometry);

    shader->compile();
    bool success = shader->getShaderStatus();

    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Geometry shader compilation failed: " +
                    std::string(infoLog));
        throw std::runtime_error(
            std::string("Geometry shader compilation failed: ") + infoLog);
    }

    atlas_log("Geometry shader compiled successfully");
    this->shaderId = shader->shaderID;
}

TessellationShader
TessellationShader::fromDefaultShader(AtlasTessellationShader shader) {
    switch (shader) {
    case AtlasTessellationShader::TerrainControl:
        return TessellationShader::fromSource(TERRAIN_CONTROL_TESC,
                                              TessellationShaderType::Control);
    case AtlasTessellationShader::TerrainEvaluation:
        return TessellationShader::fromSource(
            TERRAIN_EVAL_TESE, TessellationShaderType::Evaluation);
    default:
        throw std::runtime_error("Unknown default tessellation shader");
    }
}

TessellationShader TessellationShader::fromSource(const char *source,
                                                  TessellationShaderType type) {
    TessellationShader shader;
    shader.source = source;
    shader.type = type;
    shader.shaderId = 0;
    return shader;
}

void TessellationShader::compile() {
    if (shaderId != 0) {
        if (shader != nullptr) {
            return;
        }
        shaderId = 0;
    }

    if (source == nullptr) {
        throw std::runtime_error("Tessellation shader source is null");
    }

    if (shaderId != 0) {
        return;
    }

    opal::ShaderType shaderType;
    switch (type) {
    case TessellationShaderType::Control:
        shaderType = opal::ShaderType::TessellationControl;
        break;
    case TessellationShaderType::Evaluation:
        shaderType = opal::ShaderType::TessellationEvaluation;
        break;
    case TessellationShaderType::Primitive:
        throw std::runtime_error("Primitive tessellation shader not supported");
    default:
        throw std::runtime_error("Unknown tessellation shader type");
    }

    shader = opal::Shader::createFromSource(source, shaderType);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        throw std::runtime_error(
            std::string("Tessellation shader compilation failed: ") + infoLog);
    }

    this->shaderId = shader->shaderID;
}

void ShaderProgram::compile() {
    if (programId != 0) {
        if (shader != nullptr) {
            return;
        }
        programId = 0;
    }

    bool hasComputeShader = computeShader.source != nullptr ||
                            computeShader.shader != nullptr ||
                            computeShader.shaderId != 0;
    bool hasGraphicsShader =
        vertexShader.source != nullptr || vertexShader.shader != nullptr ||
        vertexShader.shaderId != 0 || fragmentShader.source != nullptr ||
        fragmentShader.shader != nullptr || fragmentShader.shaderId != 0;

    if (hasComputeShader && hasGraphicsShader) {
        throw std::runtime_error(
            "Cannot mix compute shaders with vertex/fragment shaders in one "
            "ShaderProgram");
    }
    if (hasComputeShader) {
        isComputeProgram = true;
    }

    if (isComputeProgram) {
        if (computeShader.shaderId == 0) {
            atlas_error("Compute shader not compiled");
            throw std::runtime_error("Compute shader not compiled");
        }
        if (computeShader.shader == nullptr) {
            atlas_error("Compute shader object is null");
            throw std::runtime_error("Compute shader object is null");
        }

        if (computeShader.fromDefaultShaderType.has_value()) {
            auto key = computeShader.fromDefaultShaderType.value();
            if (ShaderProgram::computeShaderCache.contains(key)) {
                *this = ShaderProgram::computeShaderCache[key];
                return;
            }
        }

        atlas_log("Linking compute shader program");
        desiredAttributes = computeShader.desiredAttributes;
        capabilities = computeShader.capabilities;

        this->shader = opal::ShaderProgram::create();
        this->shader->attachShader(computeShader.shader);
        this->shader->link();
        this->programId = this->shader->programID;

        bool success = this->shader->getProgramStatus();
        if (!success) {
            char infoLog[512];
            this->shader->getProgramLog(infoLog, sizeof(infoLog));
            atlas_error("Compute shader program linking failed: " +
                        std::string(infoLog));
            throw std::runtime_error(
                std::string("Compute shader program linking failed: ") +
                infoLog);
        }

        atlas_log("Compute shader program linked successfully");

        if (computeShader.fromDefaultShaderType.has_value()) {
            ShaderProgram::computeShaderCache
                [computeShader.fromDefaultShaderType.value()] = *this;
        }
        return;
    }

    isComputeProgram = false;

    if (vertexShader.shaderId == 0) {
        atlas_error("Vertex shader not compiled");
        throw std::runtime_error("Vertex shader not compiled");
    }
    if (vertexShader.shader == nullptr) {
        atlas_error("Vertex shader object is null");
        throw std::runtime_error("Vertex shader object is null");
    }

    if (fragmentShader.shaderId == 0) {
        atlas_error("Fragment shader not compiled");
        throw std::runtime_error("Fragment shader not compiled");
    }
    if (fragmentShader.shader == nullptr) {
        atlas_error("Fragment shader object is null");
        throw std::runtime_error("Fragment shader object is null");
    }

    if (fragmentShader.fromDefaultShaderType.has_value() &&
        vertexShader.fromDefaultShaderType.has_value()) {
        auto key = std::make_pair(vertexShader.fromDefaultShaderType.value(),
                                  fragmentShader.fromDefaultShaderType.value());
        if (ShaderProgram::shaderCache.contains(key)) {
            *this = ShaderProgram::shaderCache[key];
            return;
        }
    }

    atlas_log("Linking shader program");
    desiredAttributes = vertexShader.desiredAttributes;
    capabilities = vertexShader.capabilities;

    this->shader = opal::ShaderProgram::create();

    this->shader->attachShader(vertexShader.shader);
    this->shader->attachShader(fragmentShader.shader);
    if (geometryShader.shaderId != 0 && geometryShader.shader != nullptr) {
        this->shader->attachShader(geometryShader.shader);
    }
    for (const auto &tessShader : tessellationShaders) {
        if (tessShader.shaderId != 0 && tessShader.shader != nullptr) {
            this->shader->attachShader(tessShader.shader);
        }
    }
    this->shader->link();
    this->programId = this->shader->programID;

    bool success = this->shader->getProgramStatus();
    if (!success) {
        char infoLog[512];
        this->shader->getProgramLog(infoLog, sizeof(infoLog));
        atlas_error("Shader program linking failed: " + std::string(infoLog));
        throw std::runtime_error(
            std::string("Shader program linking failed: ") + infoLog);
    }

    atlas_log("Shader program linked successfully");

    if (fragmentShader.fromDefaultShaderType.has_value() &&
        vertexShader.fromDefaultShaderType.has_value()) {
        auto key = std::make_pair(vertexShader.fromDefaultShaderType.value(),
                                  fragmentShader.fromDefaultShaderType.value());
        ShaderProgram::shaderCache[key] = *this;
    }
}

ShaderProgram ShaderProgram::defaultProgram() {
    static ShaderProgram program;
    static bool initialized = false;

    if (!initialized) {
        program.vertexShader = VertexShader::fromDefaultShader(
            AtlasVertexShader::DEFAULT_VERT_SHADER);
        program.fragmentShader = FragmentShader::fromDefaultShader(
            AtlasFragmentShader::DEFAULT_FRAG_SHADER);
        program.desiredAttributes = program.vertexShader.desiredAttributes;
        program.vertexShader.compile();
        program.fragmentShader.compile();
        program.compile();
        initialized = true;
    }

    return program;
}

void ShaderProgram::setUniform4f(const std::string &name, float v0, float v1,
                                 float v2, float v3) const {
    if (currentPipeline) {
        currentPipeline->setUniform4f(name, v0, v1, v2, v3);
    }
}

void ShaderProgram::setUniform3f(const std::string &name, float v0, float v1,
                                 float v2) const {
    if (currentPipeline) {
        currentPipeline->setUniform3f(name, v0, v1, v2);
    }
}

void ShaderProgram::setUniform2f(const std::string &name, float v0,
                                 float v1) const {
    if (currentPipeline) {
        currentPipeline->setUniform2f(name, v0, v1);
    }
}

void ShaderProgram::setUniform1f(const std::string &name, float v0) const {
    if (currentPipeline) {
        currentPipeline->setUniform1f(name, v0);
    }
}

void ShaderProgram::setUniformMat4f(const std::string &name,
                                    const glm::mat4 &matrix) const {
    if (currentPipeline) {
        currentPipeline->setUniformMat4f(name, matrix);
    }
}

void ShaderProgram::setUniform1i(const std::string &name, int v0) const {
    if (currentPipeline) {
        currentPipeline->setUniform1i(name, v0);
    }
}

void ShaderProgram::setUniformBool(const std::string &name, bool value) const {
    if (currentPipeline) {
        currentPipeline->setUniformBool(name, value);
    }
}

ShaderProgram ShaderProgram::fromDefaultShaders(
    AtlasVertexShader vShader, AtlasFragmentShader fShader,
    GeometryShader gShader, std::vector<TessellationShader> tShaders) {
    ShaderProgram program;
    program.vertexShader = VertexShader::fromDefaultShader(vShader);
    program.fragmentShader = FragmentShader::fromDefaultShader(fShader);
    program.computeShader = ComputeShader();
    program.geometryShader = std::move(gShader);
    program.tessellationShaders = std::move(tShaders);
    program.programId = 0;
    program.isComputeProgram = false;
    program.desiredAttributes = program.vertexShader.desiredAttributes;

    program.vertexShader.compile();
    program.fragmentShader.compile();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (program.vertexShader.shaderId == 0 ||
        program.fragmentShader.shaderId == 0) {
        throw std::runtime_error("Failed to compile default shaders");
    }

    program.compile();
    return program;
}

ShaderProgram
ShaderProgram::fromDefaultComputeShader(AtlasComputeShader cShader) {
    ShaderProgram program;
    program.computeShader = ComputeShader::fromDefaultShader(cShader);
    program.programId = 0;
    program.isComputeProgram = true;
    program.desiredAttributes = {};

    program.computeShader.compile();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (program.computeShader.shaderId == 0) {
        throw std::runtime_error("Failed to compile default compute shader");
    }

    program.compile();
    return program;
}

ShaderProgram ShaderProgram::fromComputeShader(ComputeShader cShader) {
    ShaderProgram program;
    program.computeShader = std::move(cShader);
    program.programId = 0;
    program.isComputeProgram = true;
    program.desiredAttributes = {};

    program.computeShader.compile();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (program.computeShader.shaderId == 0) {
        throw std::runtime_error("Failed to compile compute shader");
    }

    program.compile();
    return program;
}

std::shared_ptr<opal::Pipeline> ShaderProgram::requestPipeline(
    std::shared_ptr<opal::Pipeline> unbuiltPipeline) {
    unbuiltPipeline->setShaderProgram(this->shader);
    if (isComputeProgram) {
        for (auto &existingPipeline : pipelines) {
            if (*existingPipeline == unbuiltPipeline) {
                currentPipeline = existingPipeline;
                return existingPipeline;
            }
        }

        unbuiltPipeline->build();
        pipelines.push_back(unbuiltPipeline);
        currentPipeline = unbuiltPipeline;
        return unbuiltPipeline;
    }

    std::vector<LayoutDescriptor> layoutDescriptors =
        CoreVertex::getLayoutDescriptors();

    std::vector<uint32_t> activeLocations = this->desiredAttributes;
    if (activeLocations.empty()) {
        for (const auto &attr : layoutDescriptors) {
            activeLocations.push_back(attr.layoutPos);
        }
    }

    std::vector<opal::VertexAttribute> vertexAttributes;
    opal::VertexBinding vertexBinding;

    vertexAttributes.reserve(layoutDescriptors.size());
    for (const auto &attr : layoutDescriptors) {
        vertexAttributes.push_back(opal::VertexAttribute{
            .name = attr.name,
            .type = attr.type,
            .offset = static_cast<uint>(attr.offset),
            .location = static_cast<uint>(attr.layoutPos),
            .normalized = attr.normalized,
            .size = static_cast<uint>(attr.size),
            .stride = static_cast<uint>(attr.stride),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0});
    }

    std::size_t vec4Size = sizeof(glm::vec4);
    for (unsigned int i = 0; i < 4; ++i) {
        vertexAttributes.push_back(opal::VertexAttribute{
            .name = "instanceModel" + std::to_string(i),
            .type = opal::VertexAttributeType::Float,
            .offset = static_cast<uint>(i * vec4Size),
            .location = static_cast<uint>(6 + i),
            .normalized = false,
            .size = 4,
            .stride = static_cast<uint>(sizeof(glm::mat4)),
            .inputRate = opal::VertexBindingInputRate::Instance,
            .divisor = 1});
    }

    vertexBinding = opal::VertexBinding{(uint)layoutDescriptors[0].stride,
                                        opal::VertexBindingInputRate::Vertex};

    unbuiltPipeline->setVertexAttributes(vertexAttributes, vertexBinding);

    for (auto &existingPipeline : pipelines) {
        if (*existingPipeline == unbuiltPipeline) {
            currentPipeline = existingPipeline;
            return existingPipeline;
        }
    }

    unbuiltPipeline->build();

    pipelines.push_back(unbuiltPipeline);
    currentPipeline = unbuiltPipeline;

    return unbuiltPipeline;
}
