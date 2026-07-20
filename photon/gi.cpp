/*
 gi.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: DDGI Implementation for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include "photon/illuminate.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#ifdef METAL
namespace {
constexpr int kDdgiMaterialTextureUnitStart = 10;
constexpr int kDdgiMaxMaterialTextures = 48;
constexpr int kDdgiSkyboxTextureUnit = 60;
constexpr int kDdgiPreviousIrradianceTextureUnit = 61;

std::shared_ptr<opal::Texture> createDdgiFallbackSkyboxTexture() {
    constexpr unsigned char value[4] = {0, 0, 0, 255};
    const unsigned char *faces[6] = {value, value, value, value, value, value};
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
        texture->updateFace(face, faces[face], 1, 1,
                            opal::TextureDataFormat::Rgba);
    }
    return texture;
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
        static_cast<size_t>(kDdgiMaxMaterialTextures)) {
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

glm::vec3 normalizeOr(const glm::vec3 &v, const glm::vec3 &fallback) {
    float len2 = glm::dot(v, v);
    if (len2 > 1e-10f) {
        return v * glm::inversesqrt(len2);
    }
    return fallback;
}

uint64_t hashCombineU64(uint64_t seed, uint64_t v) {
    seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    return seed;
}

uint64_t hashFloat(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    return static_cast<uint64_t>(bits);
}

uint64_t computeDdgiLayoutSignature(const std::vector<CoreObject *> &objects,
                                    float spacing, int probeResolution,
                                    int borderSize) {
    uint64_t signature = 1469598103934665603ULL;
    signature = hashCombineU64(signature, hashFloat(spacing));
    signature =
        hashCombineU64(signature, static_cast<uint64_t>(probeResolution));
    signature = hashCombineU64(signature, static_cast<uint64_t>(borderSize));
    for (auto *object : objects) {
        if (object == nullptr || !object->canUseDeferredRendering()) {
            continue;
        }
        signature = hashCombineU64(
            signature,
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(object)));
        signature = hashCombineU64(
            signature, static_cast<uint64_t>(object->vertices.size()));
        signature = hashCombineU64(
            signature, static_cast<uint64_t>(object->indices.size()));
        Position3d pos = object->getPosition();
        Rotation3d rot = object->getRotation();
        Size3d scl = object->getScale();
        glm::quat rotQuat = rot.toGlmQuat();
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(pos.x)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(pos.y)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(pos.z)));
        signature = hashCombineU64(signature, hashFloat(rotQuat.x));
        signature = hashCombineU64(signature, hashFloat(rotQuat.y));
        signature = hashCombineU64(signature, hashFloat(rotQuat.z));
        signature = hashCombineU64(signature, hashFloat(rotQuat.w));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scl.x)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scl.y)));
        signature =
            hashCombineU64(signature, hashFloat(static_cast<float>(scl.z)));
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                signature = hashCombineU64(
                    signature, hashFloat(object->model[column][row]));
            }
        }
        signature = hashCombineU64(
            signature, static_cast<uint64_t>(object->textures.size()));
        signature = hashCombineU64(
            signature,
            static_cast<uint64_t>(object->material.useNormalMap ? 1 : 0));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.albedo.r)));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.albedo.g)));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.albedo.b)));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.albedo.a)));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.metallic)));
        signature = hashCombineU64(signature, hashFloat(static_cast<float>(
                                                  object->material.roughness)));
        signature = hashCombineU64(
            signature, hashFloat(static_cast<float>(object->material.ao)));
        signature = hashCombineU64(
            signature,
            hashFloat(static_cast<float>(object->material.emissiveColor.r)));
        signature = hashCombineU64(
            signature,
            hashFloat(static_cast<float>(object->material.emissiveColor.g)));
        signature = hashCombineU64(
            signature,
            hashFloat(static_cast<float>(object->material.emissiveColor.b)));
        signature = hashCombineU64(
            signature,
            hashFloat(static_cast<float>(object->material.emissiveIntensity)));
        signature = hashCombineU64(
            signature,
            hashFloat(static_cast<float>(object->material.normalMapStrength)));
        for (const auto &texture : object->textures) {
            signature =
                hashCombineU64(signature, static_cast<uint64_t>(texture.type));
            uint64_t textureKey =
                texture.texture != nullptr
                    ? static_cast<uint64_t>(
                          reinterpret_cast<uintptr_t>(texture.texture.get()))
                    : static_cast<uint64_t>(texture.id);
            signature = hashCombineU64(signature, textureKey);
        }
    }
    return signature;
}

void collectDdgiObject(Renderable *renderable,
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
            collectDdgiObject(child, seen, objects);
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

void collectDdgiObjectsFromQueue(const std::vector<Renderable *> &renderables,
                                 std::unordered_set<CoreObject *> &seen,
                                 std::vector<CoreObject *> &objects) {
    for (auto *renderable : renderables) {
        collectDdgiObject(renderable, seen, objects);
    }
}

} // namespace

void photon::GlobalIllumination::init() {
    ComputeShader ddgiRaytracing =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI);
    ddgiRaytracing.compile();
    giRaytracingShader = std::make_shared<ShaderProgram>();
    giRaytracingShader->computeShader = ddgiRaytracing;
    giRaytracingShader->compile();

    giRaytracingPipeline = opal::Pipeline::create();
    giRaytracingPipeline->setShaderProgram(giRaytracingShader->shader);
    giRaytracingPipeline->setComputeThreadgroupSize(64, 1, 1);
    giRaytracingPipeline->build();

    ComputeShader ddgiWrite =
        ComputeShader::fromDefaultShader(AtlasComputeShader::DDGI_WRITE);
    ddgiWrite.compile();
    giWriteShader = std::make_shared<ShaderProgram>();
    giWriteShader->computeShader = ddgiWrite;
    giWriteShader->compile();

    irradianceMap = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    irradianceMapPrev = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    distanceMap = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    distanceMapPrev = std::make_shared<Texture>(
        Texture::create(512, 512, opal::TextureFormat::Rgba16F,
                        opal::TextureDataFormat::Rgba, TextureType::Color));

    giPipeline = opal::Pipeline::create();
    giPipeline->setShaderProgram(giWriteShader->shader);
    giPipeline->setComputeThreadgroupSize(8, 8, 1);
    giPipeline->build();

    probeSpace = std::make_shared<ProbeSpace>();
    probeSpace->debugColor = Color(1.0f, 1.0f, 1.0f, 0.0f);
    frameIndex = 0;

    copySrcFramebuffer = opal::Framebuffer::create();
    copyDstFramebuffer = opal::Framebuffer::create();
}

void photon::GlobalIllumination::updateProbeLayout() {
    if (probeSpace == nullptr || Window::mainWindow == nullptr) {
        return;
    }

    Position3d previousOrigin = probeSpace->originWorldSpace;
    Position3d previousSpacing = probeSpace->spacing;
    Vector3 previousProbeCount = probeSpace->probeCount;
    int previousProbesPerRow = probeSpace->probesPerRow;
    int previousProbeResolution = probeSpace->probeResolution;
    int previousBorder = probeSpace->textureBorderSize;
    float spacing = std::max(0.001f, probeSpacing);
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    bool hasGeometry = false;

    auto includePoint = [&](const glm::vec3 &p) {
        boundsMin = glm::min(boundsMin, p);
        boundsMax = glm::max(boundsMax, p);
        hasGeometry = true;
    };

    std::vector<CoreObject *> ddgiObjects;
    std::unordered_set<CoreObject *> seenDdgiObjects;
    if (Window::mainWindow != nullptr) {
        collectDdgiObjectsFromQueue(Window::mainWindow->renderables,
                                    seenDdgiObjects, ddgiObjects);
        collectDdgiObjectsFromQueue(Window::mainWindow->firstRenderables,
                                    seenDdgiObjects, ddgiObjects);
    }
    const uint64_t layoutSignature = computeDdgiLayoutSignature(
        ddgiObjects, std::max(0.001f, probeSpacing),
        probeSpace->probeResolution, probeSpace->textureBorderSize);
    if (hasCachedLayoutSignature && cachedLayoutSignature == layoutSignature &&
        probeRadianceBuffer != nullptr && irradianceMap != nullptr &&
        irradianceMapPrev != nullptr && distanceMap != nullptr &&
        distanceMapPrev != nullptr && triangleBuffer != nullptr &&
        materialBuffer != nullptr && sceneBLAS != nullptr) {
        return;
    }
    cachedLayoutSignature = layoutSignature;
    hasCachedLayoutSignature = true;

    triangles.clear();
    materials.clear();
    materialTextures.clear();
    materials.reserve(ddgiObjects.size());
    std::unordered_map<uint64_t, int> textureSlots;

    for (auto *object : ddgiObjects) {
        if (object == nullptr || !object->canUseDeferredRendering()) {
            continue;
        }
        findTextureSlotForType(object->textures, TextureType::Color,
                               materialTextures, textureSlots);
    }

    for (auto *object : ddgiObjects) {
        if (object == nullptr || !object->canUseDeferredRendering()) {
            continue;
        }

        const auto &vertices = object->getVertices();
        if (vertices.size() < 3) {
            continue;
        }

        const auto &indices = object->indices;
        const bool useIndexBuffer = indices.size() >= 3;
        glm::mat4 model = object->model;
        glm::mat3 linearMatrix = glm::mat3(model);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        DDGIMaterial baseMaterial{};
        baseMaterial.albedo = object->material.albedo.toGlm();
        baseMaterial.ao = object->material.ao;
        baseMaterial.metallic = object->material.metallic;
        baseMaterial.roughness = object->material.roughness;
        baseMaterial.emissiveColor = object->material.emissiveColor.toGlm();
        baseMaterial.emissiveIntensity = object->material.emissiveIntensity;
        const bool useNormalMap =
            object->material.useNormalMap && sampleNormalMaps;
        const float normalStrength = std::max(
            0.0f, object->material.normalMapStrength * normalMapStrength);
        baseMaterial._pad0 = normalStrength;
        baseMaterial._pad1 = useNormalMap ? 1.0f : 0.0f;
        baseMaterial.albedoTextureIndex =
            findTextureSlotForType(object->textures, TextureType::Color,
                                   materialTextures, textureSlots);
        int normalTextureIndex = -1;
        if (useNormalMap && normalStrength > 0.0f) {
            normalTextureIndex =
                findTextureSlotForType(object->textures, TextureType::Normal,
                                       materialTextures, textureSlots);
            if (normalTextureIndex < 0) {
                normalTextureIndex = findTextureSlotForType(
                    object->textures, TextureType::Parallax, materialTextures,
                    textureSlots);
            }
        }
        baseMaterial.normalTextureIndex = normalTextureIndex;
        const int pbrPackTextureIndex = findTextureSlotForType(
            object->textures, TextureType::PBRPack, materialTextures,
            textureSlots);
        baseMaterial.metallicTextureIndex =
            pbrPackTextureIndex >= 0
                ? pbrPackTextureIndex
                : findTextureSlotForType(
                      object->textures, TextureType::Metallic,
                      materialTextures, textureSlots);
        baseMaterial.roughnessTextureIndex =
            pbrPackTextureIndex >= 0
                ? pbrPackTextureIndex
                : findTextureSlotForType(
                      object->textures, TextureType::Roughness,
                      materialTextures, textureSlots);
        baseMaterial.aoTextureIndex =
            pbrPackTextureIndex >= 0
                ? pbrPackTextureIndex
                : findTextureSlotForType(object->textures, TextureType::AO,
                                         materialTextures, textureSlots);
        bool materialBound = false;
        int materialID = -1;

        auto appendTriangle = [&](const CoreVertex &v0, const CoreVertex &v1,
                                  const CoreVertex &v2) {
            if (!materialBound) {
                materialID = static_cast<int>(materials.size());
                baseMaterial.materialID = materialID;
                materials.push_back(baseMaterial);
                materialBound = true;
            }

            DDGITriangle tri{};
            tri.v0 = model * glm::vec4(v0.position.toGlm(), 1.0f);
            tri.v1 = model * glm::vec4(v1.position.toGlm(), 1.0f);
            tri.v2 = model * glm::vec4(v2.position.toGlm(), 1.0f);
            glm::vec3 n0 = normalizeOr(normalMatrix * v0.normal.toGlm(),
                                       glm::vec3(0, 1, 0));
            glm::vec3 n1 = normalizeOr(normalMatrix * v1.normal.toGlm(),
                                       glm::vec3(0, 1, 0));
            glm::vec3 n2 = normalizeOr(normalMatrix * v2.normal.toGlm(),
                                       glm::vec3(0, 1, 0));
            tri.n0 = glm::vec4(n0, 0.0f);
            tri.n1 = glm::vec4(n1, 0.0f);
            tri.n2 = glm::vec4(n2, 0.0f);
            tri.uv0 = glm::vec4(v0.textureCoordinate[0],
                                v0.textureCoordinate[1], 0.0f, 0.0f);
            tri.uv1 = glm::vec4(v1.textureCoordinate[0],
                                v1.textureCoordinate[1], 0.0f, 0.0f);
            tri.uv2 = glm::vec4(v2.textureCoordinate[0],
                                v2.textureCoordinate[1], 0.0f, 0.0f);

            auto fillTangentFrame = [&](const CoreVertex &v, const glm::vec3 &n,
                                        glm::vec4 &tOut, glm::vec4 &bOut) {
                glm::vec3 t = normalizeOr(linearMatrix * v.tangent.toGlm(),
                                          glm::vec3(1, 0, 0));
                glm::vec3 b = normalizeOr(linearMatrix * v.bitangent.toGlm(),
                                          glm::vec3(0, 0, 1));
                if (glm::dot(glm::cross(t, b), glm::cross(t, b)) <= 1e-10f) {
                    glm::vec3 up = std::abs(n.y) < 0.999f ? glm::vec3(0, 1, 0)
                                                          : glm::vec3(1, 0, 0);
                    t = normalizeOr(glm::cross(up, n), glm::vec3(1, 0, 0));
                    b = normalizeOr(glm::cross(n, t), glm::vec3(0, 0, 1));
                } else {
                    t = normalizeOr(t - n * glm::dot(n, t), glm::vec3(1, 0, 0));
                    b = normalizeOr(b - n * glm::dot(n, b), glm::vec3(0, 0, 1));
                }
                tOut = glm::vec4(t, 0.0f);
                bOut = glm::vec4(b, 0.0f);
            };
            fillTangentFrame(v0, n0, tri.t0, tri.b0);
            fillTangentFrame(v1, n1, tri.t1, tri.b1);
            fillTangentFrame(v2, n2, tri.t2, tri.b2);
            tri.materialID = materialID;

            triangles.push_back(tri);
            includePoint(glm::vec3(tri.v0));
            includePoint(glm::vec3(tri.v1));
            includePoint(glm::vec3(tri.v2));
        };

        if (useIndexBuffer) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                const size_t i0 = static_cast<size_t>(indices[i]);
                const size_t i1 = static_cast<size_t>(indices[i + 1]);
                const size_t i2 = static_cast<size_t>(indices[i + 2]);
                if (i0 >= vertices.size() || i1 >= vertices.size() ||
                    i2 >= vertices.size()) {
                    continue;
                }
                appendTriangle(vertices[i0], vertices[i1], vertices[i2]);
            }
        } else {
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                appendTriangle(vertices[i], vertices[i + 1], vertices[i + 2]);
            }
        }
    }

    if (hasGeometry) {
        glm::vec3 rawExtent = glm::max(boundsMax - boundsMin, glm::vec3(0.0f));
        spacing = std::max(spacing, std::max(rawExtent.x, std::max(rawExtent.y,
                                                                  rawExtent.z)) /
                                        15.0f);
    }

    std::vector<opal::PrimitiveVertex> accelerationVertices;
    std::vector<uint32_t> accelerationIndices;
    accelerationVertices.reserve(triangles.size() * 3);
    accelerationIndices.reserve(triangles.size() * 3);
    for (const auto &triangle : triangles) {
        const glm::vec4 positions[3] = {triangle.v0, triangle.v1, triangle.v2};
        for (const auto &position : positions) {
            opal::PrimitiveVertex vertex{};
            vertex.position[0] = position.x;
            vertex.position[1] = position.y;
            vertex.position[2] = position.z;
            accelerationIndices.push_back(
                static_cast<uint32_t>(accelerationVertices.size()));
            accelerationVertices.push_back(vertex);
        }
    }
    if (!accelerationVertices.empty()) {
        sceneBLAS = opal::PrimitiveAccelerationStructure::create(
            accelerationVertices, accelerationIndices);
        sceneTLAS.reset();
        accelerationStructureDirty = true;
        triangleBuffer = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            triangles.size() * sizeof(DDGITriangle), triangles.data());
        materialBuffer = opal::Buffer::create(
            opal::BufferUsage::ShaderRead,
            materials.size() * sizeof(DDGIMaterial), materials.data());
    } else {
        sceneBLAS.reset();
        sceneTLAS.reset();
        triangleBuffer.reset();
        materialBuffer.reset();
        accelerationStructureDirty = false;
    }

    float layoutPad = spacing * 0.25f;
    Position3d minWs = hasGeometry ? Position3d(boundsMin.x - layoutPad,
                                                boundsMin.y - layoutPad,
                                                boundsMin.z - layoutPad)
                                   : Position3d(-spacing, -spacing, -spacing);
    Position3d maxWs = hasGeometry ? Position3d(boundsMax.x + layoutPad,
                                                boundsMax.y + layoutPad,
                                                boundsMax.z + layoutPad)
                                   : Position3d(spacing, spacing, spacing);

    auto snapDown = [&](float v) { return std::floor(v / spacing) * spacing; };
    auto snapUp = [&](float v) { return std::ceil(v / spacing) * spacing; };
    minWs.x = snapDown(minWs.x);
    minWs.y = snapDown(minWs.y);
    minWs.z = snapDown(minWs.z);
    maxWs.x = snapUp(maxWs.x);
    maxWs.y = snapUp(maxWs.y);
    maxWs.z = snapUp(maxWs.z);

    Position3d extent = maxWs - minWs;
    extent.x = std::max(0.0f, extent.x);
    extent.y = std::max(0.0f, extent.y);
    extent.z = std::max(0.0f, extent.z);

    auto countAxis = [&](float axisExtent) {
        if (!std::isfinite(axisExtent) || axisExtent <= 0.0f) {
            return 1;
        }
        int n = static_cast<int>(std::floor(axisExtent / spacing)) + 1;
        return std::max(1, n);
    };

    int Nx = std::clamp(countAxis(extent.x), 1, 16);
    int Ny = std::clamp(countAxis(extent.y), 1, 16);
    int Nz = std::clamp(countAxis(extent.z), 1, 16);
    int totalProbeCount = std::max(1, Nx * Ny * Nz);
    int probesPerRow = std::clamp(
        static_cast<int>(std::ceil(std::sqrt((float)totalProbeCount))), 1, 64);

    probeSpace->originWorldSpace = minWs;
    probeSpace->spacing = Position3d(
        Nx > 1 ? extent.x / static_cast<float>(Nx - 1) : spacing,
        Ny > 1 ? extent.y / static_cast<float>(Ny - 1) : spacing,
        Nz > 1 ? extent.z / static_cast<float>(Nz - 1) : spacing);
    probeSpace->probeCount = Vector3((float)Nx, (float)Ny, (float)Nz);
    probeSpace->probesPerRow = probesPerRow;

    float layoutEpsilon = std::max(1e-3f, spacing * 0.25f);
    auto nearlyEqual = [&](float a, float b) {
        return std::fabs(a - b) <= layoutEpsilon;
    };
    bool layoutChanged =
        !nearlyEqual(previousOrigin.x, probeSpace->originWorldSpace.x) ||
        !nearlyEqual(previousOrigin.y, probeSpace->originWorldSpace.y) ||
        !nearlyEqual(previousOrigin.z, probeSpace->originWorldSpace.z) ||
        !nearlyEqual(previousSpacing.x, probeSpace->spacing.x) ||
        !nearlyEqual(previousSpacing.y, probeSpace->spacing.y) ||
        !nearlyEqual(previousSpacing.z, probeSpace->spacing.z) ||
        !nearlyEqual(previousProbeCount.x, probeSpace->probeCount.x) ||
        !nearlyEqual(previousProbeCount.y, probeSpace->probeCount.y) ||
        !nearlyEqual(previousProbeCount.z, probeSpace->probeCount.z) ||
        previousProbesPerRow != probeSpace->probesPerRow ||
        previousProbeResolution != probeSpace->probeResolution ||
        previousBorder != probeSpace->textureBorderSize;

    const int atlasW = std::max(1, probeSpace->atlasWidth());
    const int atlasH = std::max(1, probeSpace->atlasHeight());
    bool needCreate = !irradianceMap || !irradianceMapPrev || !distanceMap ||
                      !distanceMapPrev;
    bool sizeChanged =
        !needCreate && (irradianceMap->creationData.width != atlasW ||
                        irradianceMap->creationData.height != atlasH);
    bool resetHistory = layoutChanged || needCreate || sizeChanged;

    if (needCreate || sizeChanged) {
        irradianceMap = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
        irradianceMapPrev = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
        distanceMap = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
        distanceMapPrev = std::make_shared<Texture>(
            Texture::create(atlasW, atlasH, opal::TextureFormat::Rgba16F,
                            opal::TextureDataFormat::Rgba, TextureType::Color));
    }

    int effectiveRaysPerProbe = std::max(1, raysPerProbe);
    if (probeRadianceBuffer == nullptr ||
        probeRadianceCapacity != totalProbeCount * effectiveRaysPerProbe) {
        int totalElements = totalProbeCount * effectiveRaysPerProbe;
        probeRadianceBuffer =
            opal::Buffer::create(opal::BufferUsage::ShaderReadWrite,
                                 sizeof(glm::vec4) * totalElements);
        probeRadianceCapacity = totalElements;
        resetHistory = true;
    }

    if (resetHistory) {
        frameIndex = 0;
    }
}

void photon::GlobalIllumination::render(
    const std::shared_ptr<opal::CommandBuffer> &commandBuffer) {
    if (commandBuffer == nullptr || giPipeline == nullptr ||
        giRaytracingPipeline == nullptr || probeSpace == nullptr ||
        probeRadianceBuffer == nullptr || irradianceMap == nullptr ||
        irradianceMapPrev == nullptr || irradianceMap->texture == nullptr ||
        irradianceMapPrev->texture == nullptr ||
        distanceMap == nullptr || distanceMapPrev == nullptr ||
        distanceMap->texture == nullptr || distanceMapPrev->texture == nullptr ||
        triangleBuffer == nullptr || materialBuffer == nullptr ||
        sceneBLAS == nullptr ||
        copySrcFramebuffer == nullptr || copyDstFramebuffer == nullptr) {
        return;
    }

    if (accelerationStructureDirty) {
        commandBuffer->buildPrimitiveAccelerationStructure(sceneBLAS);
        opal::AccelerationStructureInstance instance{};
        instance.blas = sceneBLAS;
        instance.transform = glm::mat4(1.0f);
        instance.instanceId = 0;
        instance.mask = 0xFF;
        instance.cullDisable = true;
        sceneTLAS = opal::InstanceAccelerationStructure::create({instance});
        commandBuffer->buildInstanceAccelerationStructure(sceneTLAS);
        accelerationStructureDirty = false;
    }
    if (sceneTLAS == nullptr || !sceneTLAS->isBuilt) {
        return;
    }

    const uint totalProbes =
        static_cast<uint>(std::max(1, this->probeSpace->totalProbes()));
    const uint requestedRays =
        static_cast<uint>(std::max(1, this->raysPerProbe));
    const uint effectiveRays = std::max(1u, requestedRays);
    uint updateStride = static_cast<uint>(std::max(1, this->probeUpdateStride));
    uint updateOffset =
        (updateStride > 1u)
            ? static_cast<uint>(std::max(0, frameIndex)) % updateStride
            : 0u;
    if (totalProbes > 0u) {
        updateOffset %= totalProbes;
    }
    uint activeProbeCount =
        (totalProbes > updateOffset)
            ? ((totalProbes - updateOffset + updateStride - 1u) / updateStride)
            : 0u;
    activeProbeCount = std::max(1u, activeProbeCount);
    const uint totalRays = activeProbeCount * effectiveRays;

    if (irradianceMap->id == 0) {
        irradianceMap->id = irradianceMap->texture->textureID;
    }

    // Copy the old map
    copySrcFramebuffer->attachTexture(irradianceMap->texture, 0);
    copyDstFramebuffer->attachTexture(irradianceMapPrev->texture, 0);
    auto copy = opal::ResolveAction::createForColorAttachment(
        copySrcFramebuffer, copyDstFramebuffer, 0);
    commandBuffer->performResolve(copy);
    copySrcFramebuffer->attachTexture(distanceMap->texture, 0);
    copyDstFramebuffer->attachTexture(distanceMapPrev->texture, 0);
    copy = opal::ResolveAction::createForColorAttachment(
        copySrcFramebuffer, copyDstFramebuffer, 0);
    commandBuffer->performResolve(copy);

    // Perform Ray Tracing
    giRaytracingPipeline->bindShaderReadWriteBuffer("probeRadianceOut",
                                                    probeRadianceBuffer);
    Scene *scene = (Window::mainWindow != nullptr)
                       ? Window::mainWindow->currentScene
                       : nullptr;
    std::vector<GPUDirectionalLight> directionalLights;
    std::vector<GPUPointLight> pointLights;
    std::vector<GPUSpotLight> spotLights;
    std::vector<GPUAreaLight> areaLights;

    if (scene != nullptr) {
        constexpr size_t maxDirectionalGI = 2;
        constexpr size_t maxPointGI = 4;
        constexpr size_t maxSpotGI = 2;
        constexpr size_t maxAreaGI = 2;
        const auto &sceneDirectionalLights = scene->getDirectionalLights();
        directionalLights.reserve(
            std::min<size_t>(sceneDirectionalLights.size(), maxDirectionalGI));
        for (auto *light : sceneDirectionalLights) {
            if (light == nullptr) {
                continue;
            }
            GPUDirectionalLight gpu{};
            gpu.direction = light->direction.toGlm();
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.intensity = light->intensity;
            directionalLights.push_back(gpu);
            if (directionalLights.size() >= maxDirectionalGI) {
                break;
            }
        }
        if (directionalLights.empty() && scene->atmosphere.isEnabled()) {
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
            directionalLights.push_back(gpu);
        }

        const auto &scenePointLights = scene->getPointLights();
        pointLights.reserve(
            std::min<size_t>(scenePointLights.size(), maxPointGI));
        for (auto *light : scenePointLights) {
            if (light == nullptr) {
                continue;
            }
            PointLightConstants plc = light->calculateConstants();
            GPUPointLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.intensity = light->intensity;
            gpu.constant = plc.constant;
            gpu.linear = plc.linear;
            gpu.quadratic = plc.quadratic;
            gpu.radius = plc.radius;
            pointLights.push_back(gpu);
            if (pointLights.size() >= maxPointGI) {
                break;
            }
        }

        const auto &sceneSpotLights = scene->getSpotlights();
        spotLights.reserve(std::min<size_t>(sceneSpotLights.size(), maxSpotGI));
        for (auto *light : sceneSpotLights) {
            if (light == nullptr) {
                continue;
            }
            GPUSpotLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.direction = light->direction.toGlm();
            gpu.cutOff = light->cutOff;
            gpu.outerCutOff = light->outerCutoff;
            gpu.intensity = light->intensity;
            gpu.range = light->range;
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            spotLights.push_back(gpu);
            if (spotLights.size() >= maxSpotGI) {
                break;
            }
        }

        const auto &sceneAreaLights = scene->getAreaLights();
        areaLights.reserve(std::min<size_t>(sceneAreaLights.size(), maxAreaGI));
        for (auto *light : sceneAreaLights) {
            if (light == nullptr) {
                continue;
            }
            GPUAreaLight gpu{};
            gpu.position = light->position.toGlm();
            gpu.right = light->right.toGlm();
            gpu.up = light->up.toGlm();
            gpu.size =
                glm::vec2((float)light->size.width, (float)light->size.height);
            gpu.diffuse =
                glm::vec3(light->color.r, light->color.g, light->color.b);
            gpu.specular = glm::vec3(light->shineColor.r, light->shineColor.g,
                                     light->shineColor.b);
            gpu.angle = light->angle;
            gpu.castsBothSides = light->castsBothSides ? 1 : 0;
            gpu.intensity = light->intensity;
            gpu.range = light->range;
            areaLights.push_back(gpu);
            if (areaLights.size() >= maxAreaGI) {
                break;
            }
        }
    }

    GPUDirectionalLight fallbackDirectional{};
    GPUPointLight fallbackPoint{};
    GPUSpotLight fallbackSpot{};
    GPUAreaLight fallbackArea{};
    giRaytracingPipeline->bindBufferData(
        "directionalLights",
        directionalLights.empty()
            ? static_cast<const void *>(&fallbackDirectional)
            : static_cast<const void *>(directionalLights.data()),
        directionalLights.empty()
            ? sizeof(GPUDirectionalLight)
            : directionalLights.size() * sizeof(GPUDirectionalLight));
    giRaytracingPipeline->bindBufferData(
        "pointLights",
        pointLights.empty() ? static_cast<const void *>(&fallbackPoint)
                            : static_cast<const void *>(pointLights.data()),
        pointLights.empty() ? sizeof(GPUPointLight)
                            : pointLights.size() * sizeof(GPUPointLight));
    giRaytracingPipeline->bindBufferData(
        "spotLights",
        spotLights.empty() ? static_cast<const void *>(&fallbackSpot)
                           : static_cast<const void *>(spotLights.data()),
        spotLights.empty() ? sizeof(GPUSpotLight)
                           : spotLights.size() * sizeof(GPUSpotLight));
    giRaytracingPipeline->bindBufferData(
        "areaLights",
        areaLights.empty() ? static_cast<const void *>(&fallbackArea)
                           : static_cast<const void *>(areaLights.data()),
        areaLights.empty() ? sizeof(GPUAreaLight)
                           : areaLights.size() * sizeof(GPUAreaLight));

    giRaytracingPipeline->bindBuffer("tris", triangleBuffer, 1);
    giRaytracingPipeline->bindBuffer("materials", materialBuffer, 2);

    struct GPUSceneCounts {
        uint32_t triCount;
        uint32_t materialCount;
        uint32_t directionalLightCount;
        uint32_t pointLightCount;
        uint32_t spotLightCount;
        uint32_t areaLightCount;
        uint32_t textureCount;
        uint32_t _pad0;
    };
    GPUSceneCounts sceneCounts{};
    sceneCounts.triCount = static_cast<uint32_t>(triangles.size());
    sceneCounts.materialCount = static_cast<uint32_t>(materials.size());
    sceneCounts.directionalLightCount =
        static_cast<uint32_t>(directionalLights.size());
    sceneCounts.pointLightCount = static_cast<uint32_t>(pointLights.size());
    sceneCounts.spotLightCount = static_cast<uint32_t>(spotLights.size());
    sceneCounts.areaLightCount = static_cast<uint32_t>(areaLights.size());
    sceneCounts.textureCount = static_cast<uint32_t>(std::min<int>(
        static_cast<int>(materialTextures.size()), kDdgiMaxMaterialTextures));
    giRaytracingPipeline->bindBufferData("sc", &sceneCounts,
                                         sizeof(sceneCounts));

    for (int i = 0; i < kDdgiMaxMaterialTextures; ++i) {
        std::shared_ptr<opal::Texture> texture = nullptr;
        if (i < static_cast<int>(materialTextures.size())) {
            texture = materialTextures[static_cast<size_t>(i)];
        }
        giRaytracingPipeline->bindTexture("materialTexture" + std::to_string(i),
                                          texture,
                                          kDdgiMaterialTextureUnitStart + i);
    }

    static std::shared_ptr<opal::Texture> fallbackSkybox = nullptr;
    if (fallbackSkybox == nullptr) {
        fallbackSkybox = createDdgiFallbackSkyboxTexture();
    }
    std::shared_ptr<opal::Texture> skyboxTexture = fallbackSkybox;
    int useSkybox = 0;
    glm::vec3 skyColor(0.12f, 0.14f, 0.18f);
    if (scene != nullptr) {
        auto skybox = scene->getSkybox();
        if (skybox != nullptr && skybox->cubemap.texture != nullptr) {
            skyboxTexture = skybox->cubemap.texture;
            useSkybox = 1;
        } else if (scene->atmosphere.isEnabled()) {
            Color atmosphereColor = scene->atmosphere.getLightColor();
            skyColor = glm::vec3(atmosphereColor.r, atmosphereColor.g,
                                 atmosphereColor.b) *
                       std::max(scene->atmosphere.getLightIntensity(), 0.0f);
        }
    }
    giRaytracingPipeline->bindTexture("skybox", skyboxTexture,
                                      kDdgiSkyboxTextureUnit);
    giRaytracingPipeline->bindTexture("previousIrradiance",
                                      irradianceMapPrev->texture,
                                      kDdgiPreviousIrradianceTextureUnit);

    giRaytracingPipeline->setUniform3f(
        "ps.origin", probeSpace->originWorldSpace.x,
        probeSpace->originWorldSpace.y, probeSpace->originWorldSpace.z);
    giRaytracingPipeline->setUniform3f("ps.spacing", probeSpace->spacing.x,
                                       probeSpace->spacing.y,
                                       probeSpace->spacing.z);
    giRaytracingPipeline->setUniform3f(
        "ps.probeCount", probeSpace->probeCount.x, probeSpace->probeCount.y,
        probeSpace->probeCount.z);
    giRaytracingPipeline->setUniform4f(
        "ps.debugColor", probeSpace->debugColor.r, probeSpace->debugColor.g,
        probeSpace->debugColor.b, probeSpace->debugColor.a);
    giRaytracingPipeline->setUniform4f(
        "ps.atlasParams", static_cast<float>(probeSpace->textureBorderSize),
        static_cast<float>(probeSpace->probeResolution),
        static_cast<float>(probeSpace->probesPerRow),
        static_cast<float>(probeSpace->totalProbes()));

    giRaytracingPipeline->setUniform1i("rt.raysPerProbe",
                                       static_cast<int>(effectiveRays));
    giRaytracingPipeline->setUniform1f("rt.maxRayDistance",
                                       this->maxRayDistance);
    giRaytracingPipeline->setUniform1f("rt.normalBias", this->normalBias);
    giRaytracingPipeline->setUniform1f("rt.hysteresis", this->hysteresis);
    int ddgiFrameIndex = std::max(0, frameIndex);
    giRaytracingPipeline->setUniform1i("rt.frameIndex", ddgiFrameIndex);
    giRaytracingPipeline->setUniform1i("rt.probeUpdateOffset",
                                       static_cast<int>(updateOffset));
    giRaytracingPipeline->setUniform1i("rt.probeUpdateStride",
                                       static_cast<int>(updateStride));
    giRaytracingPipeline->setUniform1i("rt.probeUpdateCount",
                                       static_cast<int>(activeProbeCount));
    giRaytracingPipeline->setUniform3f("rt.skyColor", skyColor.x, skyColor.y,
                                       skyColor.z);
    giRaytracingPipeline->setUniform1i("rt.useSkybox", useSkybox);

    commandBuffer->bindPipeline(giRaytracingPipeline);
    commandBuffer->bindInstanceAccelerationStructure(sceneTLAS, 10);
    commandBuffer->dispatch(totalRays, 1, 1);

    commandBuffer->computeBarrier();

    // Write to irradiance texture
    giPipeline->bindTexture("outTexture", irradianceMap->texture, 0);
    giPipeline->bindTexture("prevTexture", irradianceMapPrev->texture, 1);
    giPipeline->bindTexture("outDistance", distanceMap->texture, 2);
    giPipeline->bindTexture("prevDistance", distanceMapPrev->texture, 3);

    giPipeline->bindBuffer("probeRadiance", this->probeRadianceBuffer);

    giPipeline->setUniform3f("ps.origin", probeSpace->originWorldSpace.x,
                             probeSpace->originWorldSpace.y,
                             probeSpace->originWorldSpace.z);
    giPipeline->setUniform3f("ps.spacing", probeSpace->spacing.x,
                             probeSpace->spacing.y, probeSpace->spacing.z);
    giPipeline->setUniform3f("ps.probeCount", probeSpace->probeCount.x,
                             probeSpace->probeCount.y,
                             probeSpace->probeCount.z);
    giPipeline->setUniform4f("ps.debugColor", probeSpace->debugColor.r,
                             probeSpace->debugColor.g, probeSpace->debugColor.b,
                             probeSpace->debugColor.a);
    giPipeline->setUniform4f("ps.atlasParams",
                             static_cast<float>(probeSpace->textureBorderSize),
                             static_cast<float>(probeSpace->probeResolution),
                             static_cast<float>(probeSpace->probesPerRow),
                             static_cast<float>(probeSpace->totalProbes()));

    giPipeline->setUniform1i("rt.raysPerProbe",
                             static_cast<int>(effectiveRays));
    giPipeline->setUniform1f("rt.maxRayDistance", this->maxRayDistance);
    giPipeline->setUniform1f("rt.normalBias", this->normalBias);
    giPipeline->setUniform1f("rt.hysteresis", this->hysteresis);
    giPipeline->setUniform1i("rt.frameIndex", ddgiFrameIndex);
    giPipeline->setUniform1i("rt.probeUpdateOffset",
                             static_cast<int>(updateOffset));
    giPipeline->setUniform1i("rt.probeUpdateStride",
                             static_cast<int>(updateStride));
    giPipeline->setUniform1i("rt.probeUpdateCount",
                             static_cast<int>(activeProbeCount));

    commandBuffer->bindPipeline(giPipeline);
    const int dispatchWidth = std::max(1, irradianceMap->creationData.width);
    const int dispatchHeight = std::max(1, irradianceMap->creationData.height);
    commandBuffer->dispatch(static_cast<uint>(dispatchWidth),
                            static_cast<uint>(dispatchHeight), 1);
    commandBuffer->computeBarrier();

    frameIndex = std::min(frameIndex + 1, 1 << 30);
}

#endif
