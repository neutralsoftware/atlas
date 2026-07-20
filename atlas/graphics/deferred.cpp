//
// deferred.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Deferred rendering pipeline implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/light.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <cstddef>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
std::shared_ptr<opal::Texture> createFallbackSSAOTexture() {
    const unsigned char white = 255;
    auto texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Red8, 1, 1,
        opal::TextureDataFormat::Red, &white, 1);
    texture->setFilterMode(opal::TextureFilterMode::Nearest,
                           opal::TextureFilterMode::Nearest);
    texture->setWrapMode(opal::TextureAxis::S,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::T,
                         opal::TextureWrapMode::ClampToEdge);
    return texture;
}

std::shared_ptr<opal::Texture> createFallbackIrradianceTexture() {
    const unsigned char black[4] = {0, 0, 0, 255};
    auto texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Rgba8, 1, 1,
        opal::TextureDataFormat::Rgba, black, 1);
    texture->setFilterMode(opal::TextureFilterMode::Nearest,
                           opal::TextureFilterMode::Nearest);
    texture->setWrapMode(opal::TextureAxis::S,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::T,
                         opal::TextureWrapMode::ClampToEdge);
    return texture;
}

std::shared_ptr<opal::Texture> createFallbackSkyboxTexture() {
    constexpr unsigned char horizon[4] = {0, 0, 0, 255};
    constexpr unsigned char zenith[4] = {0, 0, 0, 255};
    constexpr unsigned char nadir[4] = {0, 0, 0, 255};
    const unsigned char *faceColors[6] = {horizon, horizon, zenith,
                                          nadir,   horizon, horizon};
    auto texture = opal::Texture::create(
        opal::TextureType::TextureCubeMap, opal::TextureFormat::Rgba8, 1, 1,
        opal::TextureDataFormat::Rgba, nullptr, 1);
    texture->setFilterMode(opal::TextureFilterMode::Linear,
                           opal::TextureFilterMode::Linear);
    texture->setWrapMode(opal::TextureAxis::S,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::T,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::R,
                         opal::TextureWrapMode::ClampToEdge);
    for (int face = 0; face < 6; face++) {
        texture->updateFace(face, faceColors[face], 1, 1,
                            opal::TextureDataFormat::Rgba);
    }
    return texture;
}

std::shared_ptr<opal::Texture> createFallbackShadowCubemapTexture() {
    const unsigned char white[4] = {255, 255, 255, 255};
    auto texture = opal::Texture::create(
        opal::TextureType::TextureCubeMap, opal::TextureFormat::Rgba8, 1, 1,
        opal::TextureDataFormat::Rgba, nullptr, 1);
    texture->setFilterMode(opal::TextureFilterMode::Linear,
                           opal::TextureFilterMode::Linear);
    texture->setWrapMode(opal::TextureAxis::S,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::T,
                         opal::TextureWrapMode::ClampToEdge);
    texture->setWrapMode(opal::TextureAxis::R,
                         opal::TextureWrapMode::ClampToEdge);
    for (int face = 0; face < 6; face++) {
        texture->updateFace(face, white, 1, 1, opal::TextureDataFormat::Rgba);
    }
    return texture;
}

uint64_t hashCombineU64(uint64_t seed, uint64_t value) {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    return seed;
}

uint64_t hashFloat(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    return static_cast<uint64_t>(bits);
}

uint64_t
computeDeferredGeometrySignature(const std::vector<Renderable *> &renderables) {
    uint64_t signature = 1469598103934665603ULL;
    for (auto *renderable : renderables) {
        if (renderable == nullptr) {
            continue;
        }
        signature = hashCombineU64(
            signature,
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(renderable)));
        Position3d position = renderable->getPosition();
        Size3d scale = renderable->getScale();
        signature = hashCombineU64(signature,
                                   hashFloat(static_cast<float>(position.x)));
        signature = hashCombineU64(signature,
                                   hashFloat(static_cast<float>(position.y)));
        signature = hashCombineU64(signature,
                                   hashFloat(static_cast<float>(position.z)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scale.x)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scale.y)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scale.z)));
        if (auto *object = dynamic_cast<CoreObject *>(renderable)) {
            Rotation3d rotation = object->getRotation();
            glm::quat rotationQuat = rotation.toGlmQuat();
            signature = hashCombineU64(signature, hashFloat(rotationQuat.x));
            signature = hashCombineU64(signature, hashFloat(rotationQuat.y));
            signature = hashCombineU64(signature, hashFloat(rotationQuat.z));
            signature = hashCombineU64(signature, hashFloat(rotationQuat.w));
            signature = hashCombineU64(
                signature, static_cast<uint64_t>(object->vertices.size()));
            signature = hashCombineU64(
                signature, static_cast<uint64_t>(object->indices.size()));
        }
    }
    return signature;
}

std::vector<GPUDirectionalLight>
buildGPUDirectionalLights(const std::vector<DirectionalLight *> &lights,
                          int maxCount) {
    std::vector<GPUDirectionalLight> result;
    int count = std::min(static_cast<int>(lights.size()), maxCount);
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        DirectionalLight *light = lights.at(i);
        GPUDirectionalLight gpu{};
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
        Light *light = lights.at(i);
        PointLightConstants plc = light->calculateConstants();
        GPUPointLight gpu{};
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
        Spotlight *light = lights.at(i);
        GPUSpotLight gpu{};
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
        AreaLight *light = lights.at(i);
        GPUAreaLight gpu{};
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
} // anonymous namespace

#ifdef METAL
void Window::enableGlobalIllumination() {
    usesGlobalIllumination = true;
    ddgiSystem = std::make_shared<photon::GlobalIllumination>();
    ddgiSystem->sampleNormalMaps = false;
    ddgiSystem->init();
}
#endif

void Window::deferredRendering(
    RenderTarget *target, std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (target == nullptr) {
        return;
    }
    if (commandBuffer == nullptr) {
        commandBuffer = this->activeCommandBuffer;
    }
    if (commandBuffer == nullptr) {
        return;
    }
    auto outputFramebuffer = target->getFramebuffer();
    if (target->type == RenderTargetType::Multisampled &&
        target->getResolveFramebuffer() != nullptr) {
        outputFramebuffer = target->getResolveFramebuffer();
    }
    if (outputFramebuffer == nullptr) {
        return;
    }

    const int targetWidth = std::max(1, target->getWidth());
    const int targetHeight = std::max(1, target->getHeight());
    bool recreateDeferredTargets = false;
    if (this->gBuffer == nullptr || this->gBuffer->getWidth() != targetWidth ||
        this->gBuffer->getHeight() != targetHeight) {
        recreateDeferredTargets = true;
    }
    if (this->volumetricBuffer == nullptr ||
        this->volumetricBuffer->getWidth() != targetWidth ||
        this->volumetricBuffer->getHeight() != targetHeight) {
        recreateDeferredTargets = true;
    }
    if (this->ssrFramebuffer == nullptr ||
        this->ssrFramebuffer->getWidth() != targetWidth ||
        this->ssrFramebuffer->getHeight() != targetHeight) {
        recreateDeferredTargets = true;
    }
    if (recreateDeferredTargets) {
        this->gBuffer = std::make_shared<RenderTarget>(
            RenderTarget(*this, RenderTargetType::GBuffer));
        this->volumetricBuffer = std::make_shared<RenderTarget>(
            RenderTarget(*this, RenderTargetType::Scene));
        this->ssrFramebuffer = std::make_shared<RenderTarget>(
            RenderTarget(*this, RenderTargetType::Scene));
        this->ssaoBuffer = std::make_shared<RenderTarget>(
            RenderTarget(*this, RenderTargetType::SSAO));
        this->ssaoBlurBuffer = std::make_shared<RenderTarget>(
            RenderTarget(*this, RenderTargetType::SSAOBlur));
        this->ssaoMapsDirty = true;
    }

    struct DeferredPipelineCacheEntry {
        std::shared_ptr<opal::Pipeline> pipeline;
        int width = 0;
        int height = 0;
        opal::RasterizerMode rasterizerMode = opal::RasterizerMode::Fill;
    };
    static std::unordered_map<Renderable *, ShaderProgram> deferredPrograms;
    static std::unordered_map<Renderable *, DeferredPipelineCacheEntry>
        deferredPipelines;

    auto gBufferRenderPass = opal::RenderPass::create();
    gBufferRenderPass->setFramebuffer(this->gBuffer->getFramebuffer());
    this->gBuffer->getFramebuffer()->setDrawBuffers(4);
    commandBuffer->beginPass(gBufferRenderPass);

    this->gBuffer->bind();
    this->gBuffer->getFramebuffer()->setViewport(
        0, 0, this->gBuffer->getWidth(), this->gBuffer->getHeight());
    commandBuffer->clear(0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    std::unordered_set<Renderable *> activeDeferredRenderables;
    std::vector<Renderable *> orderedDeferredRenderables;
    activeDeferredRenderables.reserve(this->renderables.size() +
                                      this->firstRenderables.size());
    orderedDeferredRenderables.reserve(this->renderables.size() +
                                       this->firstRenderables.size());

    auto collectDeferredRenderable = [&](Renderable *obj) {
        if (obj == nullptr) {
            return;
        }
        if (auto *model = dynamic_cast<Model *>(obj)) {
            const auto &meshes =
                static_cast<const Model *>(model)->getObjects();
            for (const auto &mesh : meshes) {
                CoreObject *meshRenderable = mesh.get();
                if (meshRenderable == nullptr) {
                    continue;
                }
                bool hasAnyTexture = !meshRenderable->textures.empty();
                if (!hasAnyTexture) {
                    meshRenderable->material = model->material;
                }
                meshRenderable->material.useNormalMap =
                    model->material.useNormalMap;
                meshRenderable->material.normalMapStrength =
                    model->material.normalMapStrength;
                meshRenderable->material.textureScale =
                    model->material.textureScale;
                meshRenderable->material.textureOffset =
                    model->material.textureOffset;
                meshRenderable->useDeferredRendering =
                    model->useDeferredRendering;
                if (!meshRenderable->canUseDeferredRendering()) {
                    continue;
                }
                if (activeDeferredRenderables.insert(meshRenderable).second) {
                    orderedDeferredRenderables.push_back(meshRenderable);
                }
            }
            return;
        }
        if (!obj->canUseDeferredRendering()) {
            return;
        }
        if (activeDeferredRenderables.insert(obj).second) {
            orderedDeferredRenderables.push_back(obj);
        }
    };

    for (auto *obj : this->firstRenderables) {
        collectDeferredRenderable(obj);
    }
    for (auto *obj : this->renderables) {
        collectDeferredRenderable(obj);
    }

    static uint64_t lastDeferredGeometrySignature = 0;
    static bool hasDeferredGeometrySignature = false;
    const uint64_t deferredGeometrySignature =
        computeDeferredGeometrySignature(orderedDeferredRenderables);
    if (!hasDeferredGeometrySignature ||
        deferredGeometrySignature != lastDeferredGeometrySignature) {
        this->ssaoMapsDirty = true;
        this->ssaoUpdateCooldown = 0.0f;
        lastDeferredGeometrySignature = deferredGeometrySignature;
        hasDeferredGeometrySignature = true;
    }

    for (auto it = deferredPrograms.begin(); it != deferredPrograms.end();) {
        if (!activeDeferredRenderables.contains(it->first)) {
            it = deferredPrograms.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = deferredPipelines.begin(); it != deferredPipelines.end();) {
        if (!activeDeferredRenderables.contains(it->first)) {
            it = deferredPipelines.erase(it);
        } else {
            ++it;
        }
    }

    auto renderDeferredRenderable = [&](Renderable *obj) {
        if (obj == nullptr || !obj->canUseDeferredRendering()) {
            return;
        }

        auto programIt = deferredPrograms.find(obj);
        if (programIt == deferredPrograms.end()) {
            ShaderProgram renderableProgram = this->deferredProgram;
            renderableProgram.pipelines.clear();
            renderableProgram.currentPipeline = nullptr;
            programIt =
                deferredPrograms.emplace(obj, std::move(renderableProgram))
                    .first;
        }

        auto &pipelineEntry = deferredPipelines[obj];
        const int gbufferWidth = this->gBuffer->getWidth();
        const int gbufferHeight = this->gBuffer->getHeight();
        if (pipelineEntry.pipeline == nullptr ||
            pipelineEntry.width != gbufferWidth ||
            pipelineEntry.height != gbufferHeight ||
            pipelineEntry.rasterizerMode != this->rasterizerMode) {
            auto deferredPipeline = opal::Pipeline::create();
            deferredPipeline->setViewport(0, 0, gbufferWidth, gbufferHeight);
            deferredPipeline->setCullMode(this->cullMode);
            deferredPipeline->setRasterizerMode(this->rasterizerMode);
            deferredPipeline->setFrontFace(this->deferredFrontFace);
            deferredPipeline->enableDepthTest(true);
            deferredPipeline->setDepthCompareOp(opal::CompareOp::Less);
            deferredPipeline->enableDepthWrite(true);
            pipelineEntry.pipeline =
                programIt->second.requestPipeline(deferredPipeline);
            pipelineEntry.width = gbufferWidth;
            pipelineEntry.height = gbufferHeight;
            pipelineEntry.rasterizerMode = this->rasterizerMode;
        }

        obj->setViewMatrix(this->camera->calculateViewMatrix());
        obj->setProjectionMatrix(calculateProjectionMatrix());
        obj->setPipeline(pipelineEntry.pipeline);
        if (auto *coreObject = dynamic_cast<CoreObject *>(obj)) {
            ShaderProgram originalProgram = coreObject->shaderProgram;
            coreObject->shaderProgram = programIt->second;
            coreObject->render(getDeltaTime(), commandBuffer, false);
            coreObject->shaderProgram = originalProgram;
        } else {
            obj->render(getDeltaTime(), commandBuffer, false);
        }
    };

    for (auto *obj : orderedDeferredRenderables) {
        renderDeferredRenderable(obj);
    }

    this->gBuffer->resolve();

    commandBuffer->endPass();
    this->gBuffer->unbind();

#ifdef METAL
    if (usesGlobalIllumination) {
        if (this->ddgiSystem == nullptr) {
            this->ddgiSystem = std::make_shared<photon::GlobalIllumination>();
            ddgiSystem->init();
        }

        static int ddgiLayoutCountdown = 0;
        static int ddgiRenderCountdown = 0;
        const int ddgiLayoutInterval = 4;
        const int ddgiRenderInterval = 1;

        bool needImmediateLayout = ddgiSystem->probeRadianceBuffer == nullptr ||
                                   ddgiSystem->probeSpace == nullptr;
        if (needImmediateLayout || ddgiLayoutCountdown <= 0) {
            ddgiSystem->updateProbeLayout();
            ddgiLayoutCountdown = ddgiLayoutInterval - 1;
        } else {
            ddgiLayoutCountdown--;
        }

        if (ddgiRenderCountdown <= 0) {
            ddgiSystem->render(commandBuffer);
            ddgiRenderCountdown = ddgiRenderInterval - 1;
        } else {
            ddgiRenderCountdown--;
        }
    }
#endif

    this->ssaoMapsDirty = true;
    this->ssaoUpdateCooldown = 0.0f;
    this->renderSSAO(commandBuffer);

    auto targetRenderPass = opal::RenderPass::create();
    targetRenderPass->setFramebuffer(outputFramebuffer);
    outputFramebuffer->setDrawBuffers(2);
    commandBuffer->beginPass(targetRenderPass);

    outputFramebuffer->bind();
    outputFramebuffer->setViewport(0, 0, target->getWidth(),
                                   target->getHeight());
    commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 1.0f);

    static std::shared_ptr<opal::DrawingState> quadState = nullptr;
    static std::shared_ptr<opal::Buffer> quadBuffer = nullptr;
    if (quadState == nullptr) {
        CoreVertex quadVertices[] = {
#ifdef METAL
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}},
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}}
#else
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{-1.0f, -1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}}
#endif
        };

        quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        quadState = opal::DrawingState::create(quadBuffer);
        quadState->setBuffers(quadBuffer, nullptr);

        opal::VertexAttribute positionAttr{
            .name = "deferredPosition",
            .type = opal::VertexAttributeType::Float,
            .offset = static_cast<uint>(offsetof(CoreVertex, position)),
            .location = 0,
            .normalized = false,
            .size = 3,
            .stride = static_cast<uint>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};
        opal::VertexAttribute uvAttr{
            .name = "deferredUV",
            .type = opal::VertexAttributeType::Float,
            .offset =
                static_cast<uint>(offsetof(CoreVertex, textureCoordinate)),
            .location = 2,
            .normalized = false,
            .size = 2,
            .stride = static_cast<uint>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};

        std::vector<opal::VertexAttributeBinding> bindings = {
            {.attribute = positionAttr, .sourceBuffer = quadBuffer},
            {.attribute = uvAttr, .sourceBuffer = quadBuffer}};
        quadState->configureAttributes(bindings);
    }

    static std::shared_ptr<opal::Pipeline> lightPipeline = nullptr;
    if (lightPipeline == nullptr) {
        lightPipeline = opal::Pipeline::create();
    }
    lightPipeline = this->lightProgram.requestPipeline(lightPipeline);
    lightPipeline->setCullMode(opal::CullMode::None);
    lightPipeline->enableDepthTest(false);
    lightPipeline->enableDepthWrite(false);
    lightPipeline->enableBlending(false);
    lightPipeline->setViewport(0, 0, target->getWidth(), target->getHeight());
    lightPipeline->bind();

    lightPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id, 0);
    lightPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
    lightPipeline->bindTexture2D("gAlbedoSpec", this->gBuffer->gAlbedoSpec.id,
                                 2);
    lightPipeline->bindTexture2D("gMaterial", this->gBuffer->gMaterial.id, 3);

    int boundTextures = 4;

    static std::shared_ptr<opal::Texture> fallbackSSAOTexture = nullptr;
    if (fallbackSSAOTexture == nullptr) {
        fallbackSSAOTexture = createFallbackSSAOTexture();
    }
    static std::shared_ptr<opal::Texture> fallbackShadowCubemapTexture =
        nullptr;
    if (fallbackShadowCubemapTexture == nullptr) {
        fallbackShadowCubemapTexture = createFallbackShadowCubemapTexture();
    }
    static std::shared_ptr<opal::Texture> fallbackIrradianceTexture = nullptr;
    if (fallbackIrradianceTexture == nullptr) {
        fallbackIrradianceTexture = createFallbackIrradianceTexture();
    }
    if (this->ssaoBlurBuffer != nullptr &&
        this->ssaoBlurBuffer->texture.id != 0) {
        lightPipeline->bindTexture2D("ssao", this->ssaoBlurBuffer->texture.id,
                                     4);
    } else {
        lightPipeline->bindTexture2D("ssao", fallbackSSAOTexture->textureID, 4);
    }
    boundTextures++;
    for (int i = 0; i < 5; i++) {
        lightPipeline->bindTexture2D("texture" + std::to_string(i + 1),
                                     fallbackSSAOTexture->textureID,
                                     boundTextures + i);
        lightPipeline->bindTextureCubemap(
            "cubeMap" + std::to_string(i + 1),
            fallbackShadowCubemapTexture->textureID, 10 + i);
    }

#ifdef METAL
    if (usesGlobalIllumination && ddgiSystem != nullptr &&
        ddgiSystem->probeSpace != nullptr &&
        ddgiSystem->irradianceMap != nullptr &&
        ddgiSystem->irradianceMap->texture != nullptr) {
        lightPipeline->setUniform3f("ps.origin",
                                    ddgiSystem->probeSpace->originWorldSpace.x,
                                    ddgiSystem->probeSpace->originWorldSpace.y,
                                    ddgiSystem->probeSpace->originWorldSpace.z);
        lightPipeline->setUniform3f("ps.spacing",
                                    ddgiSystem->probeSpace->spacing.x,
                                    ddgiSystem->probeSpace->spacing.y,
                                    ddgiSystem->probeSpace->spacing.z);
        lightPipeline->setUniform3f("ps.probeCount",
                                    ddgiSystem->probeSpace->probeCount.x,
                                    ddgiSystem->probeSpace->probeCount.y,
                                    ddgiSystem->probeSpace->probeCount.z);
        lightPipeline->setUniform4f("ps.debugColor",
                                    ddgiSystem->probeSpace->debugColor.r,
                                    ddgiSystem->probeSpace->debugColor.g,
                                    ddgiSystem->probeSpace->debugColor.b,
                                    ddgiSystem->probeSpace->debugColor.a);
        lightPipeline->setUniform4f(
            "ps.atlasParams", (float)ddgiSystem->probeSpace->textureBorderSize,
            (float)ddgiSystem->probeSpace->probeResolution,
            (float)ddgiSystem->probeSpace->probesPerRow,
            (float)ddgiSystem->probeSpace->totalProbes());

        std::shared_ptr<opal::Texture> ddgiIrradianceTexture =
            ddgiSystem->irradianceMap->texture;
        if (ddgiSystem->irradianceMapPrev != nullptr &&
            ddgiSystem->irradianceMapPrev->texture != nullptr &&
            ddgiSystem->frameIndex <= 1) {
            ddgiIrradianceTexture = ddgiSystem->irradianceMapPrev->texture;
        }
        lightPipeline->bindTexture("irradianceMap", ddgiIrradianceTexture, 16);
    } else {
        lightPipeline->setUniform3f("ps.origin", 0.0f, 0.0f, 0.0f);
        lightPipeline->setUniform3f("ps.spacing", 1.0f, 1.0f, 1.0f);
        lightPipeline->setUniform3f("ps.probeCount", 0.0f, 0.0f, 0.0f);
        lightPipeline->setUniform4f("ps.debugColor", 0.0f, 0.0f, 0.0f, 0.0f);
        lightPipeline->setUniform4f("ps.atlasParams", 0.0f, 0.0f, 0.0f, 0.0f);
        lightPipeline->bindTexture2D("irradianceMap",
                                     fallbackIrradianceTexture->textureID, 16);
    }
#endif

    int boundCubemaps = 0;

    Scene *scene = this->currentScene;

    lightPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                getCamera()->position.y,
                                getCamera()->position.z);

    // Set ambient light
    Color ambientColor = scene->getAmbientColor();
    float ambientIntensity = scene->getAmbientIntensity();
    if (scene->isAutomaticAmbientEnabled()) {
        ambientColor = scene->getAutomaticAmbientColor();
        ambientIntensity = scene->getAutomaticAmbientIntensity();
    }
    lightPipeline->setUniform4f("ambientLight.color", ambientColor.r,
                                ambientColor.g, ambientColor.b, 1.0f);
    lightPipeline->setUniform1f("ambientLight.intensity", ambientIntensity);

    // Set camera position
    lightPipeline->setUniform3f("cameraPosition", getCamera()->position.x,
                                getCamera()->position.y,
                                getCamera()->position.z);

    // Send directional lights using buffer binding
    int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
    std::vector<GPUDirectionalLight> gpuDirLights;
    if (dirLightCount > 0) {
        gpuDirLights =
            buildGPUDirectionalLights(scene->directionalLights, dirLightCount);
    } else if (scene->atmosphere.isEnabled()) {
        GPUDirectionalLight gpu{};
        glm::vec3 sunDir = scene->atmosphere.getSunAngle().toGlm();
        if (glm::length(sunDir) > 0.0001f) {
            gpu.direction = -glm::normalize(sunDir);
        } else {
            gpu.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        Color skyLight = scene->atmosphere.getLightColor();
        gpu.diffuse = glm::vec3(skyLight.r, skyLight.g, skyLight.b);
        gpu.specular = gpu.diffuse;
        gpu.intensity = scene->atmosphere.getLightIntensity() * 1.2f;
        gpuDirLights.push_back(gpu);
        dirLightCount = 1;
    }
    lightPipeline->setUniform1i("directionalLightCount", dirLightCount);

    if (!gpuDirLights.empty()) {
        lightPipeline->bindBuffer("DirectionalLights", gpuDirLights);
    }

    // Send point lights using buffer binding
    int pointLightCount = std::min((int)scene->pointLights.size(), 256);
    lightPipeline->setUniform1i("pointLightCount", pointLightCount);

    if (pointLightCount > 0) {
        auto gpuPointLights =
            buildGPUPointLights(scene->pointLights, pointLightCount);
        lightPipeline->bindBuffer("PointLights", gpuPointLights);
    }

    // Send spotlights using buffer binding
    int spotlightCount = std::min((int)scene->spotlights.size(), 256);
    lightPipeline->setUniform1i("spotlightCount", spotlightCount);

    if (spotlightCount > 0) {
        auto gpuSpotLights =
            buildGPUSpotLights(scene->spotlights, spotlightCount);
        lightPipeline->bindBuffer("SpotLights", gpuSpotLights);
    }

    // Send area lights using buffer binding
    int areaLightCount = std::min((int)scene->areaLights.size(), 256);
    lightPipeline->setUniform1i("areaLightCount", areaLightCount);

    if (areaLightCount > 0) {
        auto gpuAreaLights =
            buildGPUAreaLights(scene->areaLights, areaLightCount);
        lightPipeline->bindBuffer("AreaLights", gpuAreaLights);
    }

    for (int i = 0; i < 5; i++) {
        std::string uniformName = "cubeMap" + std::to_string(i + 1);
        lightPipeline->setUniform1i(uniformName, i + 10);
    }

    int shadow2DSamplerIndex = 0;
    int boundParameters = 0;
#ifdef METAL
    std::vector<GPUShadowParams> gpuShadowParams;
    gpuShadowParams.reserve(10);
#endif

    // Cycle though directional lights
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

        if (shadow2DSamplerIndex >= 5) {
            break;
        }
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        lightPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
        lightPipeline->setUniform1i(baseName + ".textureIndex",
                                    shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
#ifdef METAL
        GPUShadowParams gpuShadow{};
        gpuShadow.lightView = shadowParams.lightView;
        gpuShadow.lightProjection = shadowParams.lightProjection;
        gpuShadow.bias = shadowParams.bias;
        gpuShadow.textureIndex = shadow2DSamplerIndex;
        gpuShadow.farPlane = 0.0f;
        gpuShadow._pad1 = 0.0f;
        gpuShadow.lightPos = glm::vec3(0.0f);
        gpuShadow.lightType = 0;
        gpuShadowParams.push_back(gpuShadow);
#else
        lightPipeline->setUniformMat4f(baseName + ".lightView",
                                       shadowParams.lightView);
        lightPipeline->setUniformMat4f(baseName + ".lightProjection",
                                       shadowParams.lightProjection);
        lightPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        lightPipeline->setUniform1i(baseName + ".lightType", 0);
#endif

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    // Cycle though spotlights
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

        if (shadow2DSamplerIndex >= 5) {
            break;
        }
        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        lightPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
        lightPipeline->setUniform1i(baseName + ".textureIndex",
                                    shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
#ifdef METAL
        GPUShadowParams gpuShadow{};
        gpuShadow.lightView = shadowParams.lightView;
        gpuShadow.lightProjection = shadowParams.lightProjection;
        gpuShadow.bias = shadowParams.bias;
        gpuShadow.textureIndex = shadow2DSamplerIndex;
        gpuShadow.farPlane = 0.0f;
        gpuShadow._pad1 = 0.0f;
        gpuShadow.lightPos = glm::vec3(0.0f);
        gpuShadow.lightType = 1;
        gpuShadowParams.push_back(gpuShadow);
#else
        lightPipeline->setUniformMat4f(baseName + ".lightView",
                                       shadowParams.lightView);
        lightPipeline->setUniformMat4f(baseName + ".lightProjection",
                                       shadowParams.lightProjection);
        lightPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        lightPipeline->setUniform1i(baseName + ".lightType", 1);
#endif

        boundParameters++;
        shadow2DSamplerIndex++;
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
        if (shadow2DSamplerIndex >= 5) {
            break;
        }

        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        lightPipeline->bindTexture2D(
            "texture" + std::to_string(shadow2DSamplerIndex + 1),
            light->shadowRenderTarget->texture.id, boundTextures);
        lightPipeline->setUniform1i(baseName + ".textureIndex",
                                    shadow2DSamplerIndex);
        ShadowParams shadowParams = light->lastShadowParams;
#ifdef METAL
        GPUShadowParams gpuShadow{};
        gpuShadow.lightView = shadowParams.lightView;
        gpuShadow.lightProjection = shadowParams.lightProjection;
        gpuShadow.bias = shadowParams.bias;
        gpuShadow.textureIndex = shadow2DSamplerIndex;
        gpuShadow.farPlane = 0.0f;
        gpuShadow._pad1 = 0.0f;
        gpuShadow.lightPos = glm::vec3(0.0f);
        gpuShadow.lightType = 2;
        gpuShadowParams.push_back(gpuShadow);
#else
        lightPipeline->setUniformMat4f(baseName + ".lightView",
                                       shadowParams.lightView);
        lightPipeline->setUniformMat4f(baseName + ".lightProjection",
                                       shadowParams.lightProjection);
        lightPipeline->setUniform1f(baseName + ".bias", shadowParams.bias);
        lightPipeline->setUniform1i(baseName + ".lightType", 2);
#endif

        boundParameters++;
        shadow2DSamplerIndex++;
        boundTextures++;
    }

    for (auto *light : scene->pointLights) {
        if (!light->doesCastShadows) {
            continue;
        }
        if (light->shadowRenderTarget == nullptr) {
            continue;
        }
        if (boundCubemaps >= 5) {
            break;
        }

        std::string baseName =
            "shadowParams[" + std::to_string(boundParameters) + "]";
        lightPipeline->bindTextureCubemap(
            "cubeMap" + std::to_string(boundCubemaps + 1),
            light->shadowRenderTarget->texture.id, 10 + boundCubemaps);
        lightPipeline->setUniform1i(baseName + ".textureIndex", boundCubemaps);
#ifdef METAL
        GPUShadowParams gpuShadow{};
        gpuShadow.lightView = glm::mat4(1.0f);
        gpuShadow.lightProjection = glm::mat4(1.0f);
        gpuShadow.bias = 0.0f;
        gpuShadow.textureIndex = boundCubemaps;
        gpuShadow.farPlane = light->distance;
        gpuShadow._pad1 = 0.0f;
        gpuShadow.lightPos = glm::vec3(static_cast<float>(light->position.x),
                                       static_cast<float>(light->position.y),
                                       static_cast<float>(light->position.z));
        gpuShadow.lightType = 3;
        gpuShadowParams.push_back(gpuShadow);
#else
        lightPipeline->setUniform1f(baseName + ".farPlane", light->distance);
        lightPipeline->setUniform3f(baseName + ".lightPos", light->position.x,
                                    light->position.y, light->position.z);
        lightPipeline->setUniform1i(baseName + ".lightType", 3);
#endif

        boundParameters++;
        boundCubemaps++;
    }

    lightPipeline->setUniform1i("shadowParamCount", boundParameters);
#ifdef METAL
    if (!gpuShadowParams.empty()) {
        lightPipeline->bindBuffer("ShadowParams", gpuShadowParams);
    }
#endif

    // Set texture units array using pipeline
    for (int i = 0; i < boundTextures && i < 16; i++) {
        std::string uniformName = "textures[" + std::to_string(i) + "]";
        lightPipeline->setUniform1i(uniformName, i);
    }

    // Bind skybox
    static std::shared_ptr<opal::Texture> fallbackSkyboxTexture = nullptr;
    if (fallbackSkyboxTexture == nullptr) {
        fallbackSkyboxTexture = createFallbackSkyboxTexture();
    }
    if (scene->skybox != nullptr && scene->skybox->cubemap.id != 0) {
        lightPipeline->bindTextureCubemap("skybox", scene->skybox->cubemap.id,
                                          boundTextures);
    } else {
        lightPipeline->bindTextureCubemap(
            "skybox", fallbackSkyboxTexture->textureID, boundTextures);
    }
    lightPipeline->setUniformBool(
        "useIBL", scene->skybox != nullptr && scene->skybox->cubemap.id != 0);
    boundTextures++;

    lightPipeline->setUniform1f(
        "environment.rimLightIntensity",
        Window::mainWindow->currentScene->environment.rimLight.intensity);
    lightPipeline->setUniform3f(
        "environment.rimLightColor",
        Window::mainWindow->currentScene->environment.rimLight.color.r,
        Window::mainWindow->currentScene->environment.rimLight.color.g,
        Window::mainWindow->currentScene->environment.rimLight.color.b);

    commandBuffer->bindDrawingState(quadState);
    commandBuffer->bindPipeline(lightPipeline);
    commandBuffer->draw(6, 1, 0, 0);
    commandBuffer->unbindDrawingState();

    bool targetPassActive = true;
    bool hasVolumetricTexture = false;
    bool hasSSRTexture = false;
    const auto &volumetricSettings = scene->environment.volumetricLighting;
    bool useVolumetric = dirLightCount > 0 && volumetricSettings.enabled &&
                         volumetricSettings.density > 0.0f &&
                         volumetricSettings.weight > 0.0f &&
                         volumetricSettings.exposure > 0.0f;

    if (useVolumetric) {
        if (!volumetricBuffer) {
            volumetricBuffer =
                std::make_shared<RenderTarget>(*this, RenderTargetType::Scene);
        }

        if (targetPassActive) {
            commandBuffer->endPass();
            targetPassActive = false;
        }
        auto volumetricRenderPass = opal::RenderPass::create();
        volumetricRenderPass->setFramebuffer(
            volumetricBuffer->getFramebuffer());
        volumetricBuffer->getFramebuffer()->setDrawBuffers(1);
        commandBuffer->beginPass(volumetricRenderPass);

        volumetricBuffer->bind();
        volumetricBuffer->getFramebuffer()->setViewport(
            0, 0, volumetricBuffer->getWidth(), volumetricBuffer->getHeight());
        commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 0.0f);

        static std::shared_ptr<opal::Pipeline> volumetricPipeline = nullptr;
        if (volumetricPipeline == nullptr) {
            volumetricPipeline = opal::Pipeline::create();
        }
        volumetricPipeline =
            this->volumetricProgram.requestPipeline(volumetricPipeline);
        volumetricPipeline->setCullMode(opal::CullMode::None);
        volumetricPipeline->enableDepthTest(false);
        volumetricPipeline->enableDepthWrite(false);
        volumetricPipeline->enableBlending(false);
        volumetricPipeline->bind();

        DirectionalLight *dirLight = scene->directionalLights.at(0);

        volumetricPipeline->bindTexture2D("sceneTexture", target->texture.id,
                                          0);

        volumetricPipeline->setUniform1f("density", volumetricSettings.density);
        volumetricPipeline->setUniform1f("weight", volumetricSettings.weight);
        volumetricPipeline->setUniform1f("decay", volumetricSettings.decay);
        volumetricPipeline->setUniform1f("exposure",
                                         volumetricSettings.exposure);
        volumetricPipeline->setUniform3f("directionalLight.color",
                                         dirLight->color.r, dirLight->color.g,
                                         dirLight->color.b);
        glm::vec3 lightPos = -dirLight->direction.toGlm() * 1000.f;
        glm::vec4 clipSpace = calculateProjectionMatrix() *
                              camera->calculateViewMatrix() *
                              glm::vec4(lightPos, 1.0f);

        if (std::abs(clipSpace.w) > 1e-6f) {
            glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
            glm::vec2 sunUV = (glm::vec2(ndc.x, ndc.y) + 1.0f) * 0.5f;
            if (std::isfinite(sunUV.x) && std::isfinite(sunUV.y) &&
                target->texture.id != 0) {
                volumetricPipeline->setUniform2f("sunPos", sunUV.x, sunUV.y);
                commandBuffer->bindDrawingState(quadState);
                commandBuffer->bindPipeline(volumetricPipeline);
                commandBuffer->draw(6, 1, 0, 0);
                commandBuffer->unbindDrawingState();
                hasVolumetricTexture = true;
            }
        }
        commandBuffer->endPass();
    }

    if (this->ssrFramebuffer != nullptr && useSSR) {
        if (targetPassActive) {
            commandBuffer->endPass();
            targetPassActive = false;
        }
        auto ssrRenderPass = opal::RenderPass::create();
        ssrRenderPass->setFramebuffer(ssrFramebuffer->getFramebuffer());
        ssrFramebuffer->getFramebuffer()->setDrawBuffers(1);
        commandBuffer->beginPass(ssrRenderPass);

        ssrFramebuffer->bind();
        ssrFramebuffer->getFramebuffer()->setViewport(
            0, 0, ssrFramebuffer->getWidth(), ssrFramebuffer->getHeight());
        commandBuffer->clearColor(0.0f, 0.0f, 0.0f, 0.0f);

        static std::shared_ptr<opal::Pipeline> ssrPipeline = nullptr;
        if (ssrPipeline == nullptr) {
            ssrPipeline = opal::Pipeline::create();
        }
        ssrPipeline = this->ssrProgram.requestPipeline(ssrPipeline);
        ssrPipeline->setCullMode(opal::CullMode::None);
        ssrPipeline->enableDepthTest(false);
        ssrPipeline->enableDepthWrite(false);
        ssrPipeline->enableBlending(false);
        ssrPipeline->bind();

        ssrPipeline->bindTexture2D("gPosition", gBuffer->gPosition.id, 0);
        ssrPipeline->bindTexture2D("gNormal", gBuffer->gNormal.id, 1);
        ssrPipeline->bindTexture2D("gAlbedoSpec", gBuffer->gAlbedoSpec.id, 2);
        ssrPipeline->bindTexture2D("gMaterial", gBuffer->gMaterial.id, 3);
        ssrPipeline->bindTexture2D("sceneColor", target->texture.id, 4);
        ssrPipeline->bindTexture2D("gDepth", gBuffer->depthTexture.id, 5);
        if (scene->skybox != nullptr && scene->skybox->cubemap.id != 0) {
            ssrPipeline->bindTextureCubemap("skybox", scene->skybox->cubemap.id,
                                            6);
        } else {
            ssrPipeline->bindTextureCubemap(
                "skybox", fallbackSkyboxTexture->textureID, 6);
        }

        const glm::mat4 projectionMatrix =
            Window::mainWindow->calculateProjectionMatrix();
        const glm::mat4 viewMatrix =
            Window::mainWindow->getCamera()->calculateViewMatrix();
        ssrPipeline->setUniformMat4f("projection", projectionMatrix);
        ssrPipeline->setUniformMat4f("view", viewMatrix);
        ssrPipeline->setUniformMat4f("inverseView", glm::inverse(viewMatrix));
        ssrPipeline->setUniformMat4f("inverseProjection",
                                     glm::inverse(projectionMatrix));
        ssrPipeline->setUniform3f("cameraPosition", camera->position.x,
                                  camera->position.y, camera->position.z);
        ssrPipeline->setUniform1f("maxDistance", 40.0f);
        ssrPipeline->setUniform1f("resolution", 0.5f);
        ssrPipeline->setUniform1i("steps", 64);
        ssrPipeline->setUniform1f("thickness", 1.0f);
        ssrPipeline->setUniform1f("maxRoughness", 0.5f);

        commandBuffer->bindDrawingState(quadState);
        commandBuffer->bindPipeline(ssrPipeline);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();
        commandBuffer->endPass();
        hasSSRTexture = true;
    }

    target->volumetricLightTexture = Texture();
    target->ssrTexture = Texture();
    if (hasVolumetricTexture && volumetricBuffer != nullptr) {
        target->volumetricLightTexture = volumetricBuffer->texture;
    }
    if (hasSSRTexture && ssrFramebuffer != nullptr) {
        target->ssrTexture = ssrFramebuffer->texture;
    }
    target->gPosition = gBuffer->gPosition;

    if (targetPassActive) {
        commandBuffer->endPass();
    }
}
