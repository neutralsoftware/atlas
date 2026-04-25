/*
 core_object.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object implementation and logic
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <algorithm>
#include <glad/glad.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

std::vector<opal::VertexAttributeBinding>
makeInstanceAttributeBindings(const std::shared_ptr<opal::Buffer> &buffer) {
    std::vector<opal::VertexAttributeBinding> bindings;
    bindings.reserve(4);
    std::size_t vec4Size = sizeof(glm::vec4);
    for (unsigned int i = 0; i < 4; ++i) {
        opal::VertexAttribute attribute{
            .name = "instanceModel" + std::to_string(i),
            .type = opal::VertexAttributeType::Float,
            .offset = static_cast<uint>(i * vec4Size),
            .location = static_cast<uint>(6 + i),
            .normalized = false,
            .size = 4,
            .stride = static_cast<uint>(sizeof(glm::mat4)),
            .inputRate = opal::VertexBindingInputRate::Instance,
            .divisor = 1};
        bindings.push_back({attribute, buffer});
    }
    return bindings;
}

std::vector<GPUDirectionalLight>
buildGPUDirectionalLights(const std::vector<DirectionalLight *> &lights,
                          int maxCount) {
    std::vector<GPUDirectionalLight> result;
    int count = std::min(static_cast<int>(lights.size()), maxCount);
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        DirectionalLight *light = lights[i];
        GPUDirectionalLight gpu;
        gpu.direction = glm::vec3(light->direction.x, light->direction.y,
                                  light->direction.z);
        gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
        gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                 light->shineColor.b);
        gpu._pad1 = 0.0f;
        gpu._pad2 = 0.0f;
        gpu.intensity = light->intensity;
        result.push_back(gpu);
    }
    return result;
}

std::vector<GPUPointLight>
buildGPUPointLights(const std::vector<Light *> &lights, int maxCount) {
    std::vector<GPUPointLight> result;
    int count = std::min(static_cast<int>(lights.size()), maxCount);
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        Light *light = lights[i];
        PointLightConstants plc = light->calculateConstants();
        GPUPointLight gpu;
        gpu.position =
            glm::vec3(light->position.x, light->position.y, light->position.z);
        gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
        gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                 light->shineColor.b);
        gpu.intensity = light->intensity;
        gpu.constant = plc.constant;
        gpu.linear = plc.linear;
        gpu.quadratic = plc.quadratic;
        gpu.radius = plc.radius;
        gpu._pad1 = 0.0f;
        gpu._pad2 = 0.0f;
        gpu._pad3 = 0.0f;
        result.push_back(gpu);
    }
    return result;
}

std::vector<GPUSpotLight>
buildGPUSpotLights(const std::vector<Spotlight *> &lights, int maxCount) {
    std::vector<GPUSpotLight> result;
    int count = std::min(static_cast<int>(lights.size()), maxCount);
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        Spotlight *light = lights[i];
        GPUSpotLight gpu;
        gpu.position =
            glm::vec3(light->position.x, light->position.y, light->position.z);
        gpu.direction = glm::vec3(light->direction.x, light->direction.y,
                                  light->direction.z);
        gpu.cutOff = light->cutOff;
        gpu.outerCutOff = light->outerCutoff;
        gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
        gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                 light->shineColor.b);
        gpu._pad1 = 0.0f;
        gpu.intensity = light->intensity;
        gpu.range = light->range;
        gpu._pad4 = 0.0f;
        gpu._pad5 = 0.0f;
        gpu._pad6 = 0.0f;
        result.push_back(gpu);
    }
    return result;
}

std::vector<GPUAreaLight>
buildGPUAreaLights(const std::vector<AreaLight *> &lights, int maxCount) {
    std::vector<GPUAreaLight> result;
    int count = std::min(static_cast<int>(lights.size()), maxCount);
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        AreaLight *light = lights[i];
        GPUAreaLight gpu;
        gpu.position =
            glm::vec3(light->position.x, light->position.y, light->position.z);
        gpu.right = glm::vec3(light->right.x, light->right.y, light->right.z);
        gpu.up = glm::vec3(light->up.x, light->up.y, light->up.z);
        gpu.size = glm::vec2(light->size.width, light->size.height);
        gpu.diffuse = glm::vec3(light->color.r, light->color.g, light->color.b);
        gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                 light->shineColor.b);
        gpu.angle = light->angle;
        gpu.castsBothSides = light->castsBothSides ? 1 : 0;
        gpu._pad1 = 0.0f;
        gpu._pad2 = 0.0f;
        gpu._pad3 = 0.0f;
        gpu._pad4 = 0.0f;
        gpu._pad5 = 0.0f;
        gpu._pad6 = 0.0f;
        gpu.intensity = light->intensity;
        gpu.range = light->range;
        gpu._pad9 = 0.0f;
        result.push_back(gpu);
    }
    return result;
}

} // namespace

std::vector<LayoutDescriptor> CoreVertex::getLayoutDescriptors() {
    return {{"position", 0, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, position)},
            {"color", 1, 4, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, color)},
            {"textureCoordinates", 2, 2, opal::VertexAttributeType::Float,
             false, sizeof(CoreVertex),
             offsetof(CoreVertex, textureCoordinate)},
            {"normal", 3, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, normal)},
            {"tangent", 4, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, tangent)},
            {"bitangent", 5, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, bitangent)}};
}

CoreObject::CoreObject() : vao(nullptr), vbo(nullptr), ebo(nullptr) {
    shaderProgram = ShaderProgram::defaultProgram();
}

void CoreObject::attachProgram(const ShaderProgram &program) {
    shaderProgram = program;
    if (shaderProgram.programId == 0) {
        shaderProgram.compile();
    }
    this->refreshPipeline();
}

void CoreObject::createAndAttachProgram(VertexShader &vertexShader,
                                        FragmentShader &fragmentShader) {
    if (vertexShader.shaderId == 0) {
        vertexShader.compile();
    }

    if (fragmentShader.shaderId == 0) {
        fragmentShader.compile();
    }

    shaderProgram = ShaderProgram(vertexShader, fragmentShader);
    shaderProgram.compile();
    this->refreshPipeline();
}

void CoreObject::renderColorWithTexture() {
    useColor = true;
    useTexture = true;
}

void CoreObject::renderOnlyColor() {
    useColor = true;
    useTexture = false;
}

void CoreObject::renderOnlyTexture() {
    useColor = false;
    useTexture = true;
}

void CoreObject::attachTexture(const Texture &tex) {
    textures.push_back(tex);
    useTexture = true;
    useColor = false;
}

void CoreObject::setColor(const Color &color) {
    for (auto &vertex : vertices) {
        vertex.color = color;
    }
    useColor = true;
    useTexture = false;
    material.albedo = color;
}

void CoreObject::attachVertices(const std::vector<CoreVertex> &newVertices) {
    if (newVertices.empty()) {
        throw std::runtime_error("Cannot attach empty vertex array");
    }

    vertices = newVertices;
}

void CoreObject::attachIndices(const std::vector<Index> &newIndices) {
    indices = newIndices;
}

void CoreObject::setPosition(const Position3d &newPosition) {
    Position3d oldPosition = position;
    Position3d delta = newPosition - oldPosition;
    position = newPosition;

    if (!instances.empty()) {
        for (auto &instance : instances) {
            instance.position += delta;
            instance.updateModelMatrix();
        }
    }

    if (light != nullptr) {
        light->position += delta;
    }

    updateModelMatrix();
}

void CoreObject::setRotation(const Rotation3d &newRotation) {
    const glm::quat oldQuat = rotationQuat;
    rotation = newRotation;
    rotationQuat = glm::normalize(rotation.toGlmQuat());

    if (!instances.empty()) {
        const glm::quat deltaQuat = rotationQuat * glm::inverse(oldQuat);
        for (auto &instance : instances) {
            glm::quat instQuat = glm::normalize(instance.rotation.toGlmQuat());
            instQuat = glm::normalize(deltaQuat * instQuat);
            instance.rotation = Rotation3d::fromGlmQuat(instQuat);
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::setRotationQuat(const glm::quat &quat) {
    const glm::quat oldQuat = rotationQuat;
    rotationQuat = glm::normalize(quat);
    rotation = Rotation3d::fromGlmQuat(rotationQuat);

    if (!instances.empty()) {
        const glm::quat deltaQuat = rotationQuat * glm::inverse(oldQuat);
        for (auto &instance : instances) {
            glm::quat instQuat = glm::normalize(instance.rotation.toGlmQuat());
            instQuat = glm::normalize(deltaQuat * instQuat);
            instance.rotation = Rotation3d::fromGlmQuat(instQuat);
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::setScale(const Scale3d &newScale) {
    Scale3d oldScale = scale;
    scale = newScale;

    if (!instances.empty()) {
        const auto computeFactor = [](double newValue, double oldValue) {
            const double epsilon = std::numeric_limits<double>::epsilon();
            if (std::abs(oldValue) <= epsilon) {
                return newValue;
            }
            return newValue / oldValue;
        };

        double factorX = computeFactor(newScale.x, oldScale.x);
        double factorY = computeFactor(newScale.y, oldScale.y);
        double factorZ = computeFactor(newScale.z, oldScale.z);

        for (auto &instance : instances) {
            instance.scale = {instance.scale.x * factorX,
                              instance.scale.y * factorY,
                              instance.scale.z * factorZ};
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::move(const Position3d &delta) {
    setPosition(position + delta);
}

void CoreObject::rotate(const Rotation3d &delta) {
    setRotation(rotation + delta);
}

void CoreObject::lookAt(const Position3d &target, const Normal3d &up) {
    glm::vec3 pos = position.toGlm();
    glm::vec3 targetPos = target.toGlm();
    glm::vec3 upVec = up.toGlm();

    glm::vec3 forward = glm::normalize(targetPos - pos);

    glm::vec3 right = glm::normalize(glm::cross(forward, upVec));

    glm::vec3 realUp = glm::cross(right, forward);

    glm::mat3 rotMatrix;
    rotMatrix[0] = right;    // X-axis
    rotMatrix[1] = realUp;   // Y-axis
    rotMatrix[2] = -forward; // Z-axis (negative because OpenGL looks down -Z)

    float pitch, yaw, roll;

    pitch = glm::degrees(asin(glm::clamp(rotMatrix[2][1], -1.0f, 1.0f)));

    if (abs(cos(glm::radians(pitch))) > 0.00001f) {
        yaw = glm::degrees(atan2(-rotMatrix[2][0], rotMatrix[2][2]));
        roll = glm::degrees(atan2(-rotMatrix[0][1], rotMatrix[1][1]));
    } else {
        yaw = glm::degrees(atan2(rotMatrix[1][0], rotMatrix[0][0]));
        roll = 0.0f;
    }

    rotation = Rotation3d{pitch, yaw, roll};
    rotationQuat = glm::normalize(rotation.toGlmQuat());
    updateModelMatrix();
}

void CoreObject::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4_cast(rotationQuat);

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    model = translation_matrix * rotation_matrix * scale_matrix;
}

void CoreObject::initialize() {
    for (auto &component : components) {
        component->init();
    }
    if (vertices.empty()) {
        throw std::runtime_error("No vertices attached to the object");
    }

    if (vao == nullptr) {
        vao = opal::DrawingState::create(nullptr);
    }

    vbo = opal::Buffer::create(
        opal::BufferUsage::VertexBuffer, vertices.size() * sizeof(CoreVertex),
        vertices.data(), opal::MemoryUsageType::CPUToGPU, id);

    if (!indices.empty()) {
        ebo = opal::Buffer::create(
            opal::BufferUsage::IndexArray, indices.size() * sizeof(Index),
            indices.data(), opal::MemoryUsageType::CPUToGPU, id);
    }

    vao->setBuffers(vbo, ebo);

    if (this->pipeline == nullptr) {
        this->pipeline = opal::Pipeline::create();
    }

    std::vector<LayoutDescriptor> layoutDescriptors =
        CoreVertex::getLayoutDescriptors();

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

    vertexBinding = opal::VertexBinding{(uint)layoutDescriptors[0].stride,
                                        opal::VertexBindingInputRate::Vertex};

    std::vector<opal::VertexAttributeBinding> attributeBindings;
    attributeBindings.reserve(vertexAttributes.size());
    for (const auto &attribute : vertexAttributes) {
        attributeBindings.push_back({attribute, vbo});
    }
    vao->configureAttributes(attributeBindings);

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

    if (!instances.empty()) {
        std::vector<glm::mat4> modelMatrices;
        for (auto &instance : instances) {
            instance.updateModelMatrix();
            modelMatrices.push_back(instance.getModelMatrix());
        }

        instanceVBO = opal::Buffer::create(opal::BufferUsage::GeneralPurpose,
                                           instances.size() * sizeof(glm::mat4),
                                           modelMatrices.data(),
                                           opal::MemoryUsageType::CPUToGPU, id);
        auto instanceBindings = makeInstanceAttributeBindings(instanceVBO);
        vao->configureAttributes(instanceBindings);
    }

    this->pipeline->setVertexAttributes(vertexAttributes, vertexBinding);

    vao->unbind();
}

std::optional<std::shared_ptr<opal::Pipeline>> CoreObject::getPipeline() {
    if (this->pipeline == nullptr) {
        return std::nullopt;
    }
    return this->pipeline;
}

void CoreObject::setPipeline(std::shared_ptr<opal::Pipeline> &newPipeline) {
    this->pipeline = newPipeline;
}

void CoreObject::refreshPipeline() {
    if (Window::mainWindow == nullptr) {
        return;
    }

    auto unbuiltPipeline = opal::Pipeline::create();

    Window &window = *Window::mainWindow;

    int viewportWidth = window.viewportWidth;
    int viewportHeight = window.viewportHeight;
    if (viewportWidth == 0 || viewportHeight == 0) {
        Size2d size = window.getSize();
        viewportWidth = static_cast<int>(size.width);
        viewportHeight = static_cast<int>(size.height);
    }

    unbuiltPipeline->setViewport(window.viewportX, window.viewportY,
                                 viewportWidth, viewportHeight);
    unbuiltPipeline->setPrimitiveStyle(window.primitiveStyle);
    unbuiltPipeline->setLineWidth(window.lineWidth);
    unbuiltPipeline->setRasterizerMode(window.rasterizerMode);
    unbuiltPipeline->setCullMode(window.cullMode);
    unbuiltPipeline->setFrontFace(window.frontFace);
    unbuiltPipeline->enableDepthTest(window.useDepth);
    unbuiltPipeline->setDepthCompareOp(window.depthCompareOp);
    unbuiltPipeline->enableBlending(window.useBlending);
    unbuiltPipeline->setBlendFunc(window.srcBlend, window.dstBlend);
    unbuiltPipeline->enableMultisampling(window.useMultisampling);

    this->pipeline = this->shaderProgram.requestPipeline(unbuiltPipeline);
}

void CoreObject::render(float dt,
                        std::shared_ptr<opal::CommandBuffer> commandBuffer,
                        bool updatePipeline) {
    for (auto &component : components) {
        component->update(dt);
    }
    if (!isVisible) {
        return;
    }
    if (shaderProgram.programId == 0) {
        atlas_error("Shader program not compiled.");
        return;
    }

    if (TracerServices::getInstance().isOk()) {
        DebugObjectPacket debugPacket{};
        debugPacket.drawCallsForObject = 1;
        debugPacket.frameCount = Window::mainWindow->device->frameCount;
        debugPacket.triangleCount = static_cast<uint32_t>(
            indices.empty() ? vertices.size() / 3 : indices.size() / 3);
        debugPacket.vertexBufferSizeMb =
            static_cast<float>(sizeof(CoreVertex) * vertices.size()) /
            (1024.0f * 1024.0f);
        debugPacket.indexBufferSizeMb =
            static_cast<float>(sizeof(Index) * indices.size()) /
            (1024.0f * 1024.0f);
        debugPacket.textureCount = static_cast<uint32_t>(textures.size());
        debugPacket.materialCount = 1;
        debugPacket.objectType = DebugObjectType::StaticMesh;
        debugPacket.objectId = this->id;
        debugPacket.send();
    }

    if (updatePipeline || this->pipeline == nullptr) {
        this->refreshPipeline();
    }

    if (this->pipeline != nullptr) {
        this->pipeline->bind();
    } else {
        throw std::runtime_error(
            "Pipeline not created - call refreshPipeline() first");
    }

    this->pipeline->setUniform1i("isInstanced", 0);
    this->pipeline->setUniformBool("isInstanced", false);
    this->pipeline->setUniformMat4f("model", model);
    this->pipeline->setUniformMat4f("view", view);
    this->pipeline->setUniformMat4f("projection", projection);

    this->pipeline->setUniform1i("useColor", useColor ? 1 : 0);
    this->pipeline->setUniform1i("useTexture", useTexture ? 1 : 0);

    int boundTextures = 0;
    int boundCubemaps = 0;

    const bool shaderSupportsTextures =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Textures) !=
        shaderProgram.capabilities.end();

    if (shaderSupportsTextures) {
        this->pipeline->setUniform1i("textureCount", 0);
        this->pipeline->setUniform1i("cubeMapCount", 0);
    }

    if (!textures.empty() && useTexture && shaderSupportsTextures) {
        int count = std::min((int)textures.size(), 10);
        this->pipeline->setUniform1i("textureCount", count);

        for (int i = 0; i < count; i++) {
            std::string uniformName = "texture" + std::to_string(i + 1) + "";
            if (textures[i].texture != nullptr) {
                this->pipeline->bindTexture(uniformName, textures[i].texture, i,
                                            id);
            } else {
                this->pipeline->bindTexture2D(uniformName, textures[i].id, i,
                                              id);
            }
            boundTextures++;
        }

        this->pipeline->setUniform1i("cubeMapCount", 5);
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1) + "";
            this->pipeline->setUniform1i(uniformName, i + 10);
        }

        for (int i = 0; i < count; i++) {
            std::string uniformName = "textureTypes[" + std::to_string(i) + "]";
            this->pipeline->setUniform1i(uniformName,
                                         static_cast<int>(textures[i].type));
        }
    }
    if (shaderSupportsTextures) {
        this->pipeline->setUniform1f("normalMapStrength",
                                     material.normalMapStrength);
        this->pipeline->setUniform1i("useNormalMap",
                                     material.useNormalMap ? 1 : 0);
        if (Window::mainWindow != nullptr &&
            Window::mainWindow->getCamera() != nullptr) {
            this->pipeline->setUniform3f(
                "cameraPosition", Window::mainWindow->getCamera()->position.x,
                Window::mainWindow->getCamera()->position.y,
                Window::mainWindow->getCamera()->position.z);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Material) !=
        shaderProgram.capabilities.end()) {
        this->pipeline->setUniform3f("material.albedo", material.albedo.r,
                                     material.albedo.g, material.albedo.b);
        this->pipeline->setUniform1f("material.metallic", material.metallic);
        this->pipeline->setUniform1f("material.roughness", material.roughness);
        this->pipeline->setUniform1f("material.ao", material.ao);
        this->pipeline->setUniform1f("material.reflectivity",
                                     material.reflectivity);

        this->pipeline->setUniform3f("albedo", material.albedo.r,
                                     material.albedo.g, material.albedo.b);
        this->pipeline->setUniform1f("metallic", material.metallic);
        this->pipeline->setUniform1f("roughness", material.roughness);
        this->pipeline->setUniform1f("ao", material.ao);
        this->pipeline->setUniform1f("reflectivity", material.reflectivity);
    }

    const bool shaderSupportsIbl =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::IBL) != shaderProgram.capabilities.end();

    bool hasHdrEnvironment = std::any_of(
        textures.begin(), textures.end(), [](const Texture &texture) {
            return texture.type == TextureType::HDR;
        });

    const bool useIbl = shaderSupportsIbl && hasHdrEnvironment;
    this->pipeline->setUniformBool("useIBL", useIbl);

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Lighting) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();

        // Set ambient light
        Color ambientColor = scene->getAmbientColor();
        float ambientIntensity = scene->getAmbientIntensity();
        if (!useIbl && scene->isAutomaticAmbientEnabled()) {
            ambientColor = scene->getAutomaticAmbientColor();
            ambientIntensity = scene->getAutomaticAmbientIntensity();
        }
        this->pipeline->setUniform4f("ambientLight.color", ambientColor.r,
                                     ambientColor.g, ambientColor.b, 1.0f);
        this->pipeline->setUniform1f("ambientLight.intensity",
                                     ambientIntensity);

        this->pipeline->setUniform3f(
            "cameraPosition", window->getCamera()->position.x,
            window->getCamera()->position.y, window->getCamera()->position.z);

        int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
        this->pipeline->setUniform1i("directionalLightCount", dirLightCount);

        if (dirLightCount > 0) {
            auto gpuDirLights = buildGPUDirectionalLights(
                scene->directionalLights, dirLightCount);
            this->pipeline->bindBuffer("DirectionalLights", gpuDirLights);
        }

        int pointLightCount = std::min((int)scene->pointLights.size(), 256);
        this->pipeline->setUniform1i("pointLightCount", pointLightCount);

        if (pointLightCount > 0) {
            auto gpuPointLights =
                buildGPUPointLights(scene->pointLights, pointLightCount);
            this->pipeline->bindBuffer("PointLights", gpuPointLights);
        }

        int spotlightCount = std::min((int)scene->spotlights.size(), 256);
        this->pipeline->setUniform1i("spotlightCount", spotlightCount);

        if (spotlightCount > 0) {
            auto gpuSpotLights =
                buildGPUSpotLights(scene->spotlights, spotlightCount);
            this->pipeline->bindBuffer("SpotLights", gpuSpotLights);
        }

        int areaLightCount = std::min((int)scene->areaLights.size(), 256);
        this->pipeline->setUniform1i("areaLightCount", areaLightCount);

        if (areaLightCount > 0) {
            auto gpuAreaLights =
                buildGPUAreaLights(scene->areaLights, areaLightCount);
            this->pipeline->bindBuffer("AreaLights", gpuAreaLights);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::LightDeferred) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        RenderTarget *gBuffer = window->gBuffer.get();
        this->pipeline->bindTexture2D("gPosition", gBuffer->gPosition.id,
                                      boundTextures, id);
        boundTextures++;

        this->pipeline->bindTexture2D("gNormal", gBuffer->gNormal.id,
                                      boundTextures, id);
        boundTextures++;

        this->pipeline->bindTexture2D("gAlbedoSpec", gBuffer->gAlbedoSpec.id,
                                      boundTextures, id);
        boundTextures++;

        this->pipeline->bindTexture2D("gMaterial", gBuffer->gMaterial.id,
                                      boundTextures, id);
        boundTextures++;
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Shadows) !=
        shaderProgram.capabilities.end()) {
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1);
            this->pipeline->setUniform1i(uniformName, i + 10);
        }
        Scene *scene = Window::mainWindow->currentScene;

        int boundParameters = 0;

        for (auto *light : scene->directionalLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (light->shadowRenderTarget == nullptr) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            this->pipeline->bindTexture2D(baseName + ".textureIndex",
                                          light->shadowRenderTarget->texture.id,
                                          boundTextures, id);
            this->pipeline->setUniform1i(baseName + ".textureIndex",
                                         boundTextures);
            ShadowParams shadowParams = light->lastShadowParams;
            this->pipeline->setUniformMat4f(baseName + ".lightView",
                                            shadowParams.lightView);
            this->pipeline->setUniformMat4f(baseName + ".lightProjection",
                                            shadowParams.lightProjection);
#ifdef METAL
            this->pipeline->setUniform1f(baseName + ".bias0",
                                         shadowParams.bias);
#else
            this->pipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
#endif
            this->pipeline->setUniform1i(baseName + ".lightType", 0);

            boundParameters++;
            boundTextures++;
        }

        for (auto *light : scene->spotlights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (light->shadowRenderTarget == nullptr) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            this->pipeline->bindTexture2D(baseName + ".textureIndex",
                                          light->shadowRenderTarget->texture.id,
                                          boundTextures, id);
            this->pipeline->setUniform1i(baseName + ".textureIndex",
                                         boundTextures);
            ShadowParams shadowParams = light->lastShadowParams;
            this->pipeline->setUniformMat4f(baseName + ".lightView",
                                            shadowParams.lightView);
            this->pipeline->setUniformMat4f(baseName + ".lightProjection",
                                            shadowParams.lightProjection);
#ifdef METAL
            this->pipeline->setUniform1f(baseName + ".bias0",
                                         shadowParams.bias);
#else
            this->pipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
#endif
            this->pipeline->setUniform1i(baseName + ".lightType", 1);

            boundParameters++;
            boundTextures++;
        }

        for (auto *light : scene->areaLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (light->shadowRenderTarget == nullptr) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            this->pipeline->bindTexture2D(baseName + ".textureIndex",
                                          light->shadowRenderTarget->texture.id,
                                          boundTextures, id);
            this->pipeline->setUniform1i(baseName + ".textureIndex",
                                         boundTextures);
            ShadowParams shadowParams = light->lastShadowParams;
            this->pipeline->setUniformMat4f(baseName + ".lightView",
                                            shadowParams.lightView);
            this->pipeline->setUniformMat4f(baseName + ".lightProjection",
                                            shadowParams.lightProjection);
#ifdef METAL
            this->pipeline->setUniform1f(baseName + ".bias0",
                                         shadowParams.bias);
#else
            this->pipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
#endif
            this->pipeline->setUniform1i(baseName + ".lightType", 2);

            boundParameters++;
            boundTextures++;
        }

        for (auto *light : scene->pointLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (light->shadowRenderTarget == nullptr) {
                continue;
            }
            if (boundTextures + 6 >= 16) {
                break;
            }

            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            this->pipeline->bindTextureCubemap(
                baseName + ".textureIndex",
                light->shadowRenderTarget->texture.id, 10 + boundCubemaps, id);
            this->pipeline->setUniform1i(baseName + ".textureIndex",
                                         boundCubemaps);
            this->pipeline->setUniform1f(baseName + ".farPlane",
                                         light->distance);
            this->pipeline->setUniform3f(baseName + ".lightPos",
                                         light->position.x, light->position.y,
                                         light->position.z);
            this->pipeline->setUniform1i(baseName + ".lightType", 3);

            boundParameters++;
            boundCubemaps++;
            boundTextures += 6;
        }

        this->pipeline->setUniform1i("shadowParamCount", boundParameters);

        for (int i = 0; i < boundTextures && i < 16; i++) {
            std::string uniformName = "textures[" + std::to_string(i) + "]";
            this->pipeline->setUniform1i(uniformName, i);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::EnvironmentMapping) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        if (scene->skybox != nullptr) {
            this->pipeline->bindTextureCubemap(
                "skybox", scene->skybox->cubemap.id, boundTextures, id);
            boundTextures++;
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Environment) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        this->pipeline->setUniform1f("environment.rimLightIntensity",
                                     scene->environment.rimLight.intensity);
        this->pipeline->setUniform3f("environment.rimLightColor",
                                     scene->environment.rimLight.color.r,
                                     scene->environment.rimLight.color.g,
                                     scene->environment.rimLight.color.b);
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Instances) !=
            shaderProgram.capabilities.end() &&
        !instances.empty()) {
        if (this->instances != this->savedInstances) {
            updateInstances();
            this->savedInstances = this->instances;
        }
        this->pipeline->setUniform1i("isInstanced", 1);
        this->pipeline->setUniformBool("isInstanced", true);

        if (!indices.empty()) {
            commandBuffer->bindDrawingState(vao);
            commandBuffer->bindPipeline(this->pipeline);
            commandBuffer->drawIndexed(indices.size(), instances.size(), 0, 0,
                                       0, id);
            commandBuffer->unbindDrawingState();
            return;
        }
        commandBuffer->bindDrawingState(vao);
        commandBuffer->bindPipeline(this->pipeline);
        commandBuffer->draw(vertices.size(), instances.size(), 0, 0, id);
        commandBuffer->unbindDrawingState();
        return;
    }

    this->pipeline->setUniform1i("isInstanced", 0);
    this->pipeline->setUniformBool("isInstanced", false);
    if (!indices.empty()) {
        commandBuffer->bindDrawingState(vao);
        commandBuffer->bindPipeline(this->pipeline);
        commandBuffer->drawIndexed(indices.size(), 1, 0, 0, 0, id);
        commandBuffer->unbindDrawingState();

        return;
    }
    commandBuffer->bindDrawingState(vao);
    commandBuffer->bindPipeline(this->pipeline);
    commandBuffer->draw(vertices.size(), 1, 0, 0, id);
    commandBuffer->unbindDrawingState();
}

void CoreObject::setViewMatrix(const glm::mat4 &view) {
    this->view = view;
    for (auto &component : components) {
        component->setViewMatrix(view);
    }
}

void CoreObject::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
    for (auto &component : components) {
        component->setProjectionMatrix(projection);
    }
}

CoreObject CoreObject::clone() const {
    CoreObject newObject = *this;

    newObject.vao = nullptr;
    newObject.vbo = nullptr;
    newObject.ebo = nullptr;
    newObject.instanceVBO = nullptr;
    newObject.shaderProgram = this->shaderProgram;
    newObject.pipeline = this->pipeline;

    newObject.initialize();

    return newObject;
}

void CoreObject::updateVertices() {
    if (vbo == nullptr || vertices.empty()) {
        throw std::runtime_error("Cannot update vertices: VBO not "
                                 "initialized or empty vertex list");
    }

    vbo->bind();
    vbo->updateData(0, vertices.size() * sizeof(CoreVertex), vertices.data());
    vbo->unbind();
}

void CoreObject::update(Window &) {
    if (!hasPhysics)
        return;

    DebugTimer physicsTimer("Physics Update");

    updateModelMatrix();

    uint64_t physicsTime = physicsTimer.stop();
    TimingEventPacket physicsEvent{};
    physicsEvent.name = "Physics Update";
    physicsEvent.durationMs = static_cast<float>(physicsTime) / 1'000'000.0f;
    physicsEvent.subsystem = TimingEventSubsystem::Physics;
    physicsEvent.frameNumber = Window::mainWindow->device->frameCount;
    physicsEvent.send();
}

void CoreObject::makeEmissive(Scene *scene, Color emissionColor,
                              float intensity) {
    this->initialize();
    if (light != nullptr) {
        throw std::runtime_error("Object is already emissive");
    }
    light = std::make_shared<Light>();
    light->color = emissionColor;
    light->shineColor = emissionColor;
    light->intensity = intensity;
    light->position = this->position;
    light->distance = 10.0f;
    light->doesCastShadows = false;
    this->useDeferredRendering = false;

    this->material.albedo = emissionColor;
    this->material.emissiveColor = emissionColor;
    this->material.emissiveIntensity = intensity;

    for (auto &vertex : vertices) {
        vertex.color = emissionColor * intensity;
    }
    updateVertices();
    useColor = true;
    useTexture = false;

    this->renderOnlyColor();
    this->attachProgram(ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Color, AtlasFragmentShader::Color));

    scene->addLight(light.get());
}

void CoreObject::updateInstances() {
    if (instances.empty()) {
        return;
    }

    if (this->instanceVBO == nullptr) {
        instanceVBO =
            opal::Buffer::create(opal::BufferUsage::GeneralPurpose,
                                 instances.size() * sizeof(glm::mat4), nullptr,
                                 opal::MemoryUsageType::CPUToGPU, id);
        auto instanceBindings = makeInstanceAttributeBindings(instanceVBO);
        vao->configureAttributes(instanceBindings);
    }

    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(instances.size());

    for (auto &instance : instances) {
        instance.updateModelMatrix();
        modelMatrices.push_back(instance.getModelMatrix());
    }

    instanceVBO->bind(id);
    instanceVBO->updateData(0, instances.size() * sizeof(glm::mat4),
                            modelMatrices.data());
    instanceVBO->unbind(id);
}

void Instance::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.roll),
                                  glm::vec3(0, 0, 1));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.pitch),
                                  glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.yaw),
                                  glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    this->model = translation_matrix * rotation_matrix * scale_matrix;
}

void Instance::move(const Position3d &deltaPosition) {
    setPosition(position + deltaPosition);
}

void Instance::setPosition(const Position3d &newPosition) {
    position = newPosition;
    updateModelMatrix();
}

void Instance::setRotation(const Rotation3d &newRotation) {
    rotation = newRotation;
    updateModelMatrix();
}

void Instance::rotate(const Rotation3d &deltaRotation) {
    setRotation(rotation + deltaRotation);
}

void Instance::setScale(const Scale3d &newScale) {
    scale = newScale;
    updateModelMatrix();
}

void Instance::scaleBy(const Scale3d &deltaScale) {
    setScale(Scale3d(scale.x * deltaScale.x, scale.y * deltaScale.y,
                     scale.z * deltaScale.z));
}
