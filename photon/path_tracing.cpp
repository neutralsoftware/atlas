/*
 path_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Path Tracing implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/window.h"
#include "photon/illuminate.h"
#include "opal/opal.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <sys/types.h>

#ifdef METAL

namespace {
constexpr int kPathTracerMaterialTextureUnitStart = 12;
constexpr int kPathTracerMaxMaterialTextures = 48;
constexpr int kPathTracerSkyboxTextureUnit = 60;

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
    for (int face = 0; face < 6; ++face) {
        texture->updateFace(face, faceColors[face], 1, 1,
                            opal::TextureDataFormat::Rgba);
    }
    return texture;
}

bool mat4ApproximatelyEqual(const glm::mat4 &a, const glm::mat4 &b,
                            float epsilon) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (std::fabs(a[c][r] - b[c][r]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

int registerMaterialTextureSlot(
    const Texture &texture,
    std::vector<std::shared_ptr<opal::Texture>> &materialTextures,
    std::unordered_map<uint64_t, int> &slotByTexture) {
    if (texture.texture == nullptr) {
        return -1;
    }
    uint64_t key = static_cast<uint64_t>(
        reinterpret_cast<uintptr_t>(texture.texture.get()));
    auto it = slotByTexture.find(key);
    if (it != slotByTexture.end()) {
        return it->second;
    }
    if (materialTextures.size() >=
        static_cast<size_t>(kPathTracerMaxMaterialTextures)) {
        return -1;
    }
    int slot = static_cast<int>(materialTextures.size());
    materialTextures.push_back(texture.texture);
    slotByTexture[key] = slot;
    return slot;
}

int findTextureSlotForType(
    const std::vector<Texture> &textures, TextureType type,
    std::vector<std::shared_ptr<opal::Texture>> &materialTextures,
    std::unordered_map<uint64_t, int> &slotByTexture) {
    for (const auto &texture : textures) {
        if (texture.type == type) {
            return registerMaterialTextureSlot(texture, materialTextures,
                                               slotByTexture);
        }
    }
    return -1;
}

void collectPathTracingObject(Renderable *renderable,
                              std::unordered_set<CoreObject *> &seen,
                              std::vector<CoreObject *> &objects) {
    if (renderable == nullptr) {
        return;
    }
    if (auto *object = dynamic_cast<CoreObject *>(renderable)) {
        if (seen.insert(object).second) {
            objects.push_back(object);
        }
        return;
    }
    if (auto *compound = dynamic_cast<CompoundObject *>(renderable)) {
        for (auto *child : compound->objects) {
            collectPathTracingObject(child, seen, objects);
        }
        return;
    }
    if (auto *model = dynamic_cast<Model *>(renderable)) {
        const auto &meshes = static_cast<const Model *>(model)->getObjects();
        for (const auto &mesh : meshes) {
            CoreObject *object = mesh.get();
            if (object == nullptr) {
                continue;
            }
            bool hasAnyTexture = !object->textures.empty();
            if (!hasAnyTexture) {
                object->material = model->material;
            }
            object->material.useNormalMap = model->material.useNormalMap;
            object->material.normalMapStrength =
                model->material.normalMapStrength;
            object->useDeferredRendering = model->useDeferredRendering;
            if (seen.insert(object).second) {
                objects.push_back(object);
            }
        }
    }
}

void collectPathTracingObjectsFromQueue(
    const std::vector<Renderable *> &renderables,
    std::unordered_set<CoreObject *> &seen,
    std::vector<CoreObject *> &objects) {
    for (auto *renderable : renderables) {
        collectPathTracingObject(renderable, seen, objects);
    }
}

} // namespace

void photon::PathTracing::init() {
    materialTextures.clear();
    objectBLAS.clear();
    cachedObjects.clear();

    ComputeShader pathTracerShader =
        ComputeShader::fromDefaultShader(AtlasComputeShader::PathTracer);
    pathTracerShader.compile();
    computePathTracer = std::make_shared<ShaderProgram>();
    computePathTracer->computeShader = pathTracerShader;
    computePathTracer->compile();

    pathTracingPipeline = opal::Pipeline::create();
    pathTracingPipeline->setShaderProgram(computePathTracer->shader);
    pathTracingPipeline->setComputeThreadgroupSize(8, 8, 1);
    pathTracingPipeline->build();

    outputWidth = std::max(1, Window::mainWindow->viewportWidth);
    outputHeight = std::max(1, Window::mainWindow->viewportHeight);

    pathTracingTexture = std::make_shared<Texture>(
        Texture::create(outputWidth, outputHeight, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    pathTracingTexturePrev = std::make_shared<Texture>(
        Texture::create(outputWidth, outputHeight, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    copySrcFramebuffer = std::make_shared<opal::Framebuffer>();
    copyDstFramebuffer = std::make_shared<opal::Framebuffer>();
}

void photon::PathTracing::resizeOutput(int width, int height) {
    const int newWidth = std::max(1, width);
    const int newHeight = std::max(1, height);
    if (newWidth == outputWidth && newHeight == outputHeight &&
        pathTracingTexture != nullptr && pathTracingTexturePrev != nullptr) {
        return;
    }

    outputWidth = newWidth;
    outputHeight = newHeight;
    pathTracingTexture = std::make_shared<Texture>(
        Texture::create(outputWidth, outputHeight, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));
    pathTracingTexturePrev = std::make_shared<Texture>(
        Texture::create(outputWidth, outputHeight, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));
    frameIndex = 0;
}

void photon::PathTracing::buildAccelerationStructure(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    struct MaterialData {
        float albedo[4];
        float metallic;
        float roughness;
        float ao;
        float emissiveIntensity;
        float emissiveColor[3];
        float _pad0;
        int albedoTextureIndex;
        int normalTextureIndex;
        int metallicTextureIndex;
        int roughnessTextureIndex;
        int aoTextureIndex;
        int opacityTextureIndex;
        int _pad1[2];

        float transmittance;
        float ior;
        float _pad2[2];
    };

    struct MeshData {
        uint vertexOffset;
        uint indexOffset;
        uint _pad0;
        uint _pad1;
    };

    struct VertexData {
        float normal[3];
        float uv[2];
        float tangent[3];
        float bitangent[3];
    };

    static_assert(sizeof(VertexData) == 44);

    std::vector<CoreObject *> pathTracingObjects;
    std::unordered_set<CoreObject *> seenPathObjects;
    if (Window::mainWindow != nullptr) {
        collectPathTracingObjectsFromQueue(Window::mainWindow->renderables,
                                           seenPathObjects, pathTracingObjects);
        collectPathTracingObjectsFromQueue(Window::mainWindow->firstRenderables,
                                           seenPathObjects, pathTracingObjects);
    }
    std::vector<CoreObject *> traceableObjects;
    traceableObjects.reserve(pathTracingObjects.size());
    for (auto *object : pathTracingObjects) {
        if (object == nullptr) {
            continue;
        }
        if (!object->canUseDeferredRendering()) {
            continue;
        }
        if (object->vertices.size() < 3 || object->indices.size() < 3) {
            continue;
        }
        traceableObjects.push_back(object);
    }
    std::vector<MaterialData> materialData;

    std::vector<VertexData> allVertices;
    std::vector<uint32_t> allIndices;
    std::vector<MeshData> meshData;

    bool needsRebuild =
        objectBLAS.empty() || cachedObjects.size() != traceableObjects.size();
    if (!needsRebuild) {
        for (size_t i = 0; i < traceableObjects.size(); ++i) {
            if (cachedObjects[i] != traceableObjects[i]) {
                needsRebuild = true;
                break;
            }
        }
    }

    int objectID = 0;
    if (needsRebuild) {
        objectBLAS.clear();
        materialTextures.clear();
        cachedObjects = traceableObjects;
        std::unordered_map<uint64_t, int> textureSlots;

        for (auto *object : traceableObjects) {
            const auto &objectVertices = object->vertices;
            const auto &objectIndices = object->indices;

            std::vector<opal::PrimitiveVertex> vertices;
            vertices.reserve(objectVertices.size());
            std::vector<uint32_t> indices;
            indices.reserve(objectIndices.size());

            int vertexOffset = allVertices.size();
            int indexOffset = allIndices.size();

            for (const auto &v : objectVertices) {
                opal::PrimitiveVertex pv{};
                pv.position[0] = v.position.x;
                pv.position[1] = v.position.y;
                pv.position[2] = v.position.z;
                pv.normal[0] = v.normal.x;
                pv.normal[1] = v.normal.y;
                pv.normal[2] = v.normal.z;
                pv.uv[0] = v.textureCoordinate[0];
                pv.uv[1] = v.textureCoordinate[1];
                pv.tangent[0] = v.tangent.x;
                pv.tangent[1] = v.tangent.y;
                pv.tangent[2] = v.tangent.z;
                pv.bitangent[0] = v.bitangent.x;
                pv.bitangent[1] = v.bitangent.y;
                pv.bitangent[2] = v.bitangent.z;
                vertices.push_back(pv);

                VertexData vd{};
                vd.normal[0] = v.normal.x;
                vd.normal[1] = v.normal.y;
                vd.normal[2] = v.normal.z;
                vd.uv[0] = v.textureCoordinate[0];
                vd.uv[1] = v.textureCoordinate[1];
                vd.tangent[0] = v.tangent.x;
                vd.tangent[1] = v.tangent.y;
                vd.tangent[2] = v.tangent.z;
                vd.bitangent[0] = v.bitangent.x;
                vd.bitangent[1] = v.bitangent.y;
                vd.bitangent[2] = v.bitangent.z;
                allVertices.push_back(vd);
            }

            indices = objectIndices;
            for (auto &index : indices) {
                allIndices.push_back(vertexOffset + index);
            }

            auto blas =
                opal::PrimitiveAccelerationStructure::create(vertices, indices);
            objectBLAS[objectID] = blas;

            MaterialData data;
            data.albedo[0] = object->material.albedo.r;
            data.albedo[1] = object->material.albedo.g;
            data.albedo[2] = object->material.albedo.b;
            data.albedo[3] = object->material.albedo.a;
            data.metallic = object->material.metallic;
            data.roughness = object->material.roughness;
            data.ao = object->material.ao;
            data.emissiveIntensity = object->material.emissiveIntensity;
            data.emissiveColor[0] = object->material.emissiveColor.r;
            data.emissiveColor[1] = object->material.emissiveColor.g;
            data.emissiveColor[2] = object->material.emissiveColor.b;
            data.ior = object->material.ior;
            data.transmittance = object->material.transmittance;
            const bool useNormalMap =
                object->material.useNormalMap && sampleNormalMaps;
            const float normalStrength = std::max(
                0.0f, object->material.normalMapStrength * normalMapStrength);
            data._pad0 = normalStrength;
            data.albedoTextureIndex =
                findTextureSlotForType(object->textures, TextureType::Color,
                                       materialTextures, textureSlots);
            int normalTextureIndex = -1;
            if (useNormalMap && normalStrength > 0.0f) {
                normalTextureIndex = findTextureSlotForType(
                    object->textures, TextureType::Normal, materialTextures,
                    textureSlots);
                if (normalTextureIndex < 0) {
                    normalTextureIndex = findTextureSlotForType(
                        object->textures, TextureType::Parallax,
                        materialTextures, textureSlots);
                }
            }
            data.normalTextureIndex = normalTextureIndex;
            data.metallicTextureIndex =
                findTextureSlotForType(object->textures, TextureType::Metallic,
                                       materialTextures, textureSlots);
            data.roughnessTextureIndex =
                findTextureSlotForType(object->textures, TextureType::Roughness,
                                       materialTextures, textureSlots);
            data.aoTextureIndex =
                findTextureSlotForType(object->textures, TextureType::AO,
                                       materialTextures, textureSlots);
            data.opacityTextureIndex =
                findTextureSlotForType(object->textures, TextureType::Opacity,
                                       materialTextures, textureSlots);
            data._pad1[0] = useNormalMap ? 1 : 0;
            data._pad1[1] = 0;
            materialData.push_back(data);

            MeshData mdata;
            mdata.vertexOffset = vertexOffset;
            mdata.indexOffset = indexOffset;
            meshData.push_back(mdata);

            objectID++;
        }

        for (const auto &[_, blas] : objectBLAS) {
            if (blas != nullptr) {
                commandBuffer->buildPrimitiveAccelerationStructure(blas);
            }
        }

        materialBuffer = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            materialData.size() * sizeof(MaterialData), materialData.data());

        globalVertices = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            allVertices.size() * sizeof(VertexData), allVertices.data());

        globalIndices = opal::Buffer::create(
            opal::BufferUsage::ShaderRead, allIndices.size() * sizeof(uint32_t),
            allIndices.data());

        meshInfo = opal::Buffer::create(opal::BufferUsage::ShaderRead,
                                        meshData.size() * sizeof(MeshData),
                                        meshData.data());
        frameIndex = 0;
    }

    std::vector<opal::AccelerationStructureInstance> instances;

    struct InstanceData {
        float model[16];
        float normalCol0[4];
        float normalCol1[4];
        float normalCol2[4];
    };

    std::vector<InstanceData> instanceData;

    for (size_t objectIndex = 0; objectIndex < traceableObjects.size();
         ++objectIndex) {
        auto *object = traceableObjects[objectIndex];
        auto it = objectBLAS.find(static_cast<int>(objectIndex));
        if (it == objectBLAS.end()) {
            continue;
        }
        auto blas = it->second;
        if (blas == nullptr || !blas->isBuilt) {
            continue;
        }

        opal::AccelerationStructureInstance instance{};
        instance.blas = blas;
        instance.transform = object->model;
        instance.instanceId = static_cast<uint>(objectIndex);
        instance.mask = 0xFF;
        instance.cullDisable = false;
        instances.push_back(instance);

        InstanceData d{};
        const glm::mat4 &m = object->model;
        memcpy(d.model, &m[0][0], sizeof(float) * 16);
        glm::mat3 linear = glm::mat3(m);
        glm::mat3 normalMatrix(1.0f);
        float det = glm::determinant(linear);
        if (std::fabs(det) > 0.000001f) {
            normalMatrix = glm::transpose(glm::inverse(linear));
        }
        d.normalCol0[0] = normalMatrix[0][0];
        d.normalCol0[1] = normalMatrix[0][1];
        d.normalCol0[2] = normalMatrix[0][2];
        d.normalCol0[3] = 0.0f;
        d.normalCol1[0] = normalMatrix[1][0];
        d.normalCol1[1] = normalMatrix[1][1];
        d.normalCol1[2] = normalMatrix[1][2];
        d.normalCol1[3] = 0.0f;
        d.normalCol2[0] = normalMatrix[2][0];
        d.normalCol2[1] = normalMatrix[2][1];
        d.normalCol2[2] = normalMatrix[2][2];
        d.normalCol2[3] = 0.0f;
        instanceData.push_back(d);
    }

    instanceDataBuffer = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        instanceData.size() * sizeof(InstanceData), instanceData.data());

    sceneTLAS = opal::InstanceAccelerationStructure::create(instances);
    commandBuffer->buildInstanceAccelerationStructure(sceneTLAS);
}

void photon::PathTracing::createLightBuffers() {
    struct PointLightData {
        float position[3];
        float intensity;
        float color[3];
        float range;
    };

    struct SpotLightData {
        float position[3];
        float intensity;

        float direction[3];
        float innerCos;

        float color[3];
        float outerCos;

        float range;
        float _pad0[3];
    };

    struct AreaLightData {
        float position[3];
        float intensity;

        float right[3];
        float halfWidth;

        float up[3];
        float halfHeight;

        float color[3];
        float twoSided;
    };

    static_assert(sizeof(PointLightData) == 32);
    static_assert(sizeof(SpotLightData) == 64);
    static_assert(sizeof(AreaLightData) == 64);

    std::vector<PointLightData> pointLightData;
    std::vector<SpotLightData> spotLightData;
    std::vector<AreaLightData> areaLightData;

    Scene *scene = (Window::mainWindow != nullptr)
                       ? Window::mainWindow->getCurrentScene()
                       : nullptr;

    if (scene != nullptr) {
        for (auto *light : scene->getPointLights()) {
            if (light == nullptr) {
                continue;
            }
            PointLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.range = light->distance;
            pointLightData.push_back(data);
        }

        for (auto *light : scene->getSpotlights()) {
            if (light == nullptr) {
                continue;
            }
            SpotLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.direction[0] = light->direction.x;
            data.direction[1] = light->direction.y;
            data.direction[2] = light->direction.z;
            data.innerCos = light->cutOff;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.outerCos = light->outerCutoff;
            data.range = light->range;
            spotLightData.push_back(data);
        }

        for (auto *light : scene->getAreaLights()) {
            if (light == nullptr) {
                continue;
            }
            AreaLightData data{};
            data.position[0] = light->position.x;
            data.position[1] = light->position.y;
            data.position[2] = light->position.z;
            data.intensity = light->intensity;
            data.right[0] = light->right.x;
            data.right[1] = light->right.y;
            data.right[2] = light->right.z;
            data.halfWidth = light->size.width * 0.5f;
            data.up[0] = light->up.x;
            data.up[1] = light->up.y;
            data.up[2] = light->up.z;
            data.halfHeight = light->size.height * 0.5f;
            data.color[0] = light->color.r;
            data.color[1] = light->color.g;
            data.color[2] = light->color.b;
            data.twoSided = light->castsBothSides ? 1.0f : 0.0f;
            areaLightData.push_back(data);
        }
    }

    if (pointLightData.empty()) {
        pointLightData.push_back(PointLightData{});
    }
    this->pointLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        pointLightData.size() * sizeof(PointLightData), pointLightData.data());

    if (spotLightData.empty()) {
        spotLightData.push_back(SpotLightData{});
    }
    this->spotLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        spotLightData.size() * sizeof(SpotLightData), spotLightData.data());

    if (areaLightData.empty()) {
        areaLightData.push_back(AreaLightData{});
    }
    this->areaLights = opal::Buffer::create(
        opal::BufferUsage::ShaderRead,
        areaLightData.size() * sizeof(AreaLightData), areaLightData.data());
}

void photon::PathTracing::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {

    // Copy textures
    copySrcFramebuffer->attachTexture(pathTracingTexture->texture, 0);
    copyDstFramebuffer->attachTexture(pathTracingTexturePrev->texture, 0);
    auto copy = opal::ResolveAction::createForColorAttachment(
        copySrcFramebuffer, copyDstFramebuffer, 0);
    commandBuffer->performResolve(copy);

    auto view = Window::mainWindow->getCamera()->calculateViewMatrix();
    auto proj = Window::mainWindow->calculateProjectionMatrix();

    auto invViewProj = glm::inverse(proj * view);

    if (frameIndex == 0) {
        cachedInvViewProj = invViewProj;
    } else if (!mat4ApproximatelyEqual(cachedInvViewProj, invViewProj,
                                       0.00001f)) {
        frameIndex = 0;
        cachedInvViewProj = invViewProj;
    }

    pathTracingPipeline->setUniformMat4f("cam.invViewProj", invViewProj);
    pathTracingPipeline->setUniform3f(
        "cam.camPos", Window::mainWindow->getCamera()->position.x,
        Window::mainWindow->getCamera()->position.y,
        Window::mainWindow->getCamera()->position.z);

    int directionalLightCount = 0;
    int pointLightCount = 0;
    int spotLightCount = 0;
    int areaLightCount = 0;
    glm::vec3 directionalLightDirection(0.0f, -1.0f, 0.0f);
    glm::vec3 directionalLightColor(1.0f, 1.0f, 1.0f);
    float directionalLightIntensity = 0.0f;
    float ambientIntensity = 0.0f;

    Scene *scene = (Window::mainWindow != nullptr)
                       ? Window::mainWindow->getCurrentScene()
                       : nullptr;

    if (scene != nullptr) {
        ambientIntensity = scene->isAutomaticAmbientEnabled()
                               ? scene->getAutomaticAmbientIntensity()
                               : scene->getAmbientIntensity();
        const auto &directionalLights = scene->getDirectionalLights();
        for (auto *light : directionalLights) {
            if (light == nullptr) {
                continue;
            }
            glm::vec3 sceneDirection = light->direction.toGlm();
            if (glm::length(sceneDirection) > 0.0001f) {
                directionalLightDirection = glm::normalize(sceneDirection);
            }
            directionalLightColor =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            directionalLightIntensity = light->intensity;
            directionalLightCount = 1;
            break;
        }

        if (directionalLightCount == 0 && scene->atmosphere.isEnabled()) {
            glm::vec3 sceneDirection = scene->atmosphere.getSunAngle().toGlm();
            if (glm::length(sceneDirection) > 0.0001f) {
                directionalLightDirection = -glm::normalize(sceneDirection);
            }
            Color atmosphereLightColor = scene->atmosphere.getLightColor();
            directionalLightColor =
                glm::vec3(atmosphereLightColor.r, atmosphereLightColor.g,
                          atmosphereLightColor.b);
            directionalLightIntensity =
                scene->atmosphere.getLightIntensity() * 1.2f;
            directionalLightCount = 1;
        }

        for (auto *light : scene->getPointLights()) {
            if (light != nullptr) {
                pointLightCount++;
            }
        }
        for (auto *light : scene->getSpotlights()) {
            if (light != nullptr) {
                spotLightCount++;
            }
        }
        for (auto *light : scene->getAreaLights()) {
            if (light != nullptr) {
                areaLightCount++;
            }
        }
    }

    pathTracingPipeline->setUniform1i("sceneData.numDirectionalLights",
                                      directionalLightCount);
    pathTracingPipeline->setUniform3f(
        "dirLight.direction", directionalLightDirection.x,
        directionalLightDirection.y, directionalLightDirection.z);
    pathTracingPipeline->setUniform3f("dirLight.color", directionalLightColor.x,
                                      directionalLightColor.y,
                                      directionalLightColor.z);
    pathTracingPipeline->setUniform1f("dirLight.intensity",
                                      directionalLightIntensity);
    pathTracingPipeline->setUniform1f("sceneData.ambientIntensity",
                                      ambientIntensity);
    pathTracingPipeline->setUniform1i("sceneData.numPointLights",
                                      pointLightCount);
    pathTracingPipeline->setUniform1i("sceneData.numSpotLights",
                                      spotLightCount);
    pathTracingPipeline->setUniform1i("sceneData.numAreaLights",
                                      areaLightCount);
    pathTracingPipeline->setUniform1i("sceneData.frameIndex", this->frameIndex);
    pathTracingPipeline->setUniform1i("sceneData.raysPerPixel",
                                      this->raysPerPixel);
    pathTracingPipeline->setUniform1i("sceneData.maxBounces", this->maxBounces);
    pathTracingPipeline->setUniform1f("sceneData.indirectStrength",
                                      this->indirectStrength);

    this->buildAccelerationStructure(commandBuffer);
    this->createLightBuffers();
    commandBuffer->bindPipeline(this->pathTracingPipeline);
    pathTracingPipeline->bindTexture("outTex", pathTracingTexture->texture, 0);
    pathTracingPipeline->bindTexture("prevTex", pathTracingTexturePrev->texture,
                                     1);

    static std::shared_ptr<opal::Texture> fallbackSkyboxTexture = nullptr;
    if (fallbackSkyboxTexture == nullptr) {
        fallbackSkyboxTexture = createFallbackSkyboxTexture();
    }
    auto skyboxTextureId = fallbackSkyboxTexture->textureID;
    if (scene != nullptr) {
        auto skybox = scene->getSkybox();
        if (skybox != nullptr && skybox->cubemap.id != 0) {
            skyboxTextureId = skybox->cubemap.id;
        }
    }
    pathTracingPipeline->bindTextureCubemap("skybox", skyboxTextureId,
                                            kPathTracerSkyboxTextureUnit);
    bool lightChanged =
        cachedDirectionalLightCount != directionalLightCount ||
        glm::length(cachedDirectionalLightDirection -
                    directionalLightDirection) > 0.0001f ||
        glm::length(cachedDirectionalLightColor - directionalLightColor) >
            0.0001f ||
        std::fabs(cachedDirectionalLightIntensity - directionalLightIntensity) >
            0.0001f;
    bool skyChanged = cachedSkyboxTextureId != skyboxTextureId;
    if (lightChanged || skyChanged) {
        frameIndex = 0;
    }
    cachedDirectionalLightCount = directionalLightCount;
    cachedDirectionalLightDirection = directionalLightDirection;
    cachedDirectionalLightColor = directionalLightColor;
    cachedDirectionalLightIntensity = directionalLightIntensity;
    cachedSkyboxTextureId = skyboxTextureId;

    commandBuffer->bindInstanceAccelerationStructure(this->sceneTLAS, 0);

    pathTracingPipeline->bindBuffer("materials", materialBuffer, 2);
    pathTracingPipeline->bindBuffer("meshData", meshInfo, 3);
    pathTracingPipeline->bindBuffer("vertices", globalVertices, 4);
    pathTracingPipeline->bindBuffer("indices", globalIndices, 5);
    pathTracingPipeline->bindBuffer("instanceData", instanceDataBuffer, 6);
    pathTracingPipeline->bindBuffer("pointLights", pointLights, 9);
    pathTracingPipeline->bindBuffer("spotLights", spotLights, 10);
    pathTracingPipeline->bindBuffer("areaLights", areaLights, 11);
    pathTracingPipeline->setUniform1i(
        "sceneData.materialTextureCount",
        std::min<int>(static_cast<int>(materialTextures.size()),
                      kPathTracerMaxMaterialTextures));

    for (int i = 0; i < kPathTracerMaxMaterialTextures; ++i) {
        std::shared_ptr<opal::Texture> texture = nullptr;
        if (i < static_cast<int>(materialTextures.size())) {
            texture = materialTextures[static_cast<size_t>(i)];
        }
        pathTracingPipeline->bindTexture(
            "materialTexture" + std::to_string(i), texture,
            kPathTracerMaterialTextureUnitStart + i);
    }

    commandBuffer->dispatch(outputWidth, outputHeight, 1);

    commandBuffer->computeBarrier();

    frameIndex++;
}

#endif
