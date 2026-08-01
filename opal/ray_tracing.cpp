/*
 ray_tracing.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Ray tracing APIs for using with the renderer
 Copyright (c) 2026 Max Van den Eynde
*/

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <opal/opal.h>
#ifdef METAL

#include "Metal/Metal.hpp"
#include "metal_state.h"

opal::PrimitiveAccelerationStructure::~PrimitiveAccelerationStructure() {
    if (blasDescriptor != nullptr) {
        blasDescriptor->release();
    }
    if (blas != nullptr) {
        blas->release();
    }
}

std::shared_ptr<opal::PrimitiveAccelerationStructure>
opal::PrimitiveAccelerationStructure::create(
    const std::vector<PrimitiveVertex> &vertices,
    const std::vector<uint32_t> &indices) {
    std::vector<float> positions;
    positions.reserve(vertices.size() * 3);
    for (const auto &vertex : vertices) {
        positions.push_back(vertex.position[0]);
        positions.push_back(vertex.position[1]);
        positions.push_back(vertex.position[2]);
    }
    return create(positions, indices);
}

std::shared_ptr<opal::PrimitiveAccelerationStructure>
opal::PrimitiveAccelerationStructure::create(
    const std::vector<float> &positions, const std::vector<uint32_t> &indices) {
    return create(std::vector<std::vector<float>>{positions},
                  std::vector<std::vector<uint32_t>>{indices});
}

std::shared_ptr<opal::PrimitiveAccelerationStructure>
opal::PrimitiveAccelerationStructure::create(
    const std::vector<std::vector<float>> &positions,
    const std::vector<std::vector<uint32_t>> &indices) {
    if (positions.empty() || positions.size() != indices.size()) {
        return nullptr;
    }
    auto blas = std::make_shared<PrimitiveAccelerationStructure>();

    auto &deviceState = metal::deviceState(Device::globalInstance);

    std::vector<MTL::AccelerationStructureGeometryDescriptor *> descriptors;
    descriptors.reserve(positions.size());
    blas->vertexBuffers.reserve(positions.size());
    blas->indexBuffers.reserve(indices.size());
    for (size_t geometryIndex = 0; geometryIndex < positions.size();
         ++geometryIndex) {
        const auto &geometryPositions = positions[geometryIndex];
        const auto &geometryIndices = indices[geometryIndex];
        if (geometryPositions.size() < 9 || geometryPositions.size() % 3 != 0 ||
            geometryIndices.size() < 3 || geometryIndices.size() % 3 != 0) {
            return nullptr;
        }

        auto vertexBuffer = std::shared_ptr<MTL::Buffer>(
            deviceState.device->newBuffer(geometryPositions.data(),
                                          geometryPositions.size() *
                                              sizeof(float),
                                          MTL::ResourceStorageModeShared),
            [](MTL::Buffer *buffer) {
                if (buffer != nullptr) {
                    buffer->release();
                }
            });
        auto indexBuffer = std::shared_ptr<MTL::Buffer>(
            deviceState.device->newBuffer(geometryIndices.data(),
                                          geometryIndices.size() *
                                              sizeof(uint32_t),
                                          MTL::ResourceStorageModeShared),
            [](MTL::Buffer *buffer) {
                if (buffer != nullptr) {
                    buffer->release();
                }
            });
        if (vertexBuffer == nullptr || indexBuffer == nullptr) {
            return nullptr;
        }

        auto *triangleDescriptor =
            MTL::AccelerationStructureTriangleGeometryDescriptor::descriptor();
        triangleDescriptor->setVertexBuffer(vertexBuffer.get());
        triangleDescriptor->setVertexStride(sizeof(float) * 3);
        triangleDescriptor->setVertexFormat(MTL::AttributeFormatFloat3);
        triangleDescriptor->setVertexBufferOffset(0);
        triangleDescriptor->setIndexBuffer(indexBuffer.get());
        triangleDescriptor->setIndexType(MTL::IndexType::IndexTypeUInt32);
        triangleDescriptor->setTriangleCount(geometryIndices.size() / 3);
        triangleDescriptor->setOpaque(true);
        descriptors.push_back(triangleDescriptor);
        blas->vertexBuffers.push_back(std::move(vertexBuffer));
        blas->indexBuffers.push_back(std::move(indexBuffer));
    }

    auto *blasDesc =
        MTL::PrimitiveAccelerationStructureDescriptor::descriptor()->retain();
    NS::Array *geoms =
        NS::Array::array((NS::Object **)descriptors.data(), descriptors.size());
    blasDesc->setGeometryDescriptors(geoms);
    blasDesc->setUsage(MTL::AccelerationStructureUsagePreferFastIntersection);

    MTL::AccelerationStructureSizes sizes =
        deviceState.device->accelerationStructureSizes(blasDesc);

    try {
        blas->scratch = Buffer::create(BufferUsage::GeneralPurpose,
                                       sizes.buildScratchBufferSize);
    } catch (const std::runtime_error &) {
        blasDesc->release();
        return nullptr;
    }

    MTL::AccelerationStructure *blasPtr =
        deviceState.device->newAccelerationStructure(
            sizes.accelerationStructureSize);

    if (blasPtr == nullptr) {
        blasDesc->release();
        return nullptr;
    }

    blas->blas = blasPtr;
    blas->blasDescriptor = blasDesc;

    return blas;
}

void opal::CommandBuffer::buildPrimitiveAccelerationStructure(
    const std::shared_ptr<PrimitiveAccelerationStructure> &blas) {
    auto &deviceState = metal::deviceState(Device::globalInstance);
    auto &scratchBuffer = metal::bufferState(blas->scratch.get());
    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }
    auto *cs = state.commandBuffer;

    auto *asEnc = cs->accelerationStructureCommandEncoder();
    asEnc->buildAccelerationStructure(blas->blas, blas->blasDescriptor,
                                      scratchBuffer.buffer, 0);
    asEnc->endEncoding();

    blas->isBuilt = true;
    state.pendingResources.emplace_back(blas->scratch);
    for (const auto &vertexBuffer : blas->vertexBuffers) {
        state.pendingResources.emplace_back(vertexBuffer);
    }
    for (const auto &indexBuffer : blas->indexBuffers) {
        state.pendingResources.emplace_back(indexBuffer);
    }
    blas->scratch.reset();
    blas->vertexBuffers.clear();
    blas->indexBuffers.clear();
}

std::shared_ptr<opal::InstanceAccelerationStructure>
opal::CommandBuffer::buildAccelerationStructures(
    const std::vector<std::shared_ptr<PrimitiveAccelerationStructure>> &blases,
    const std::vector<AccelerationStructureInstance> &instances) {
    if (blases.empty() || instances.empty()) {
        return nullptr;
    }

    auto &deviceState = metal::deviceState(Device::globalInstance);
    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }

    auto *asEnc = state.commandBuffer->accelerationStructureCommandEncoder();
    for (const auto &blas : blases) {
        if (blas == nullptr || blas->scratch == nullptr ||
            blas->vertexBuffers.empty() || blas->indexBuffers.empty()) {
            asEnc->endEncoding();
            return nullptr;
        }
        auto &scratchBuffer = metal::bufferState(blas->scratch.get());
        asEnc->buildAccelerationStructure(blas->blas, blas->blasDescriptor,
                                          scratchBuffer.buffer, 0);
        blas->isBuilt = true;
        state.pendingResources.emplace_back(blas->scratch);
        for (const auto &vertexBuffer : blas->vertexBuffers) {
            state.pendingResources.emplace_back(vertexBuffer);
        }
        for (const auto &indexBuffer : blas->indexBuffers) {
            state.pendingResources.emplace_back(indexBuffer);
        }
        blas->scratch.reset();
        blas->vertexBuffers.clear();
        blas->indexBuffers.clear();
    }

    std::shared_ptr<InstanceAccelerationStructure> tlas;
    try {
        tlas = InstanceAccelerationStructure::create(instances);
    } catch (...) {
        asEnc->endEncoding();
        throw;
    }
    if (tlas == nullptr) {
        asEnc->endEncoding();
        return nullptr;
    }

    auto &scratchState = metal::bufferState(tlas->scratch.get());
    asEnc->buildAccelerationStructure(tlas->tlas, tlas->tlasDescriptor,
                                      scratchState.buffer, 0);
    asEnc->endEncoding();

    tlas->isBuilt = true;
    state.pendingResources.emplace_back(tlas->scratch);
    state.pendingResources.emplace_back(tlas->instanceBuffer);
    tlas->scratch.reset();
    tlas->instanceBuffer.reset();
    return tlas;
}

static inline void opal::writeMetalTransform3x4(const glm::mat4 &M,
                                                float out[12]) {
    out[0] = M[0][0];
    out[1] = M[0][1];
    out[2] = M[0][2];
    out[3] = M[1][0];
    out[4] = M[1][1];
    out[5] = M[1][2];
    out[6] = M[2][0];
    out[7] = M[2][1];
    out[8] = M[2][2];
    out[9] = M[3][0];
    out[10] = M[3][1];
    out[11] = M[3][2];
}

opal::InstanceAccelerationStructure::~InstanceAccelerationStructure() {
    if (tlasDescriptor != nullptr) {
        tlasDescriptor->release();
    }
    if (tlas != nullptr) {
        tlas->release();
    }
}

std::shared_ptr<opal::InstanceAccelerationStructure>
opal::InstanceAccelerationStructure::create(
    const std::vector<opal::AccelerationStructureInstance> &instances) {
    if (instances.empty()) {
        return nullptr;
    }
    auto tlas = std::make_shared<opal::InstanceAccelerationStructure>();
    tlas->instances = instances;

    tlas->blasRefs.reserve(instances.size());
    tlas->blasPtrs.reserve(instances.size());

    std::vector<MTL::AccelerationStructureInstanceDescriptor> descs;
    descs.resize(instances.size());

    for (size_t i = 0; i < instances.size(); ++i) {
        const auto &inst = instances[i];

        if (!inst.blas || !inst.blas->isBuilt)
            throw std::runtime_error("All BLAS must be built before TLAS.");

        tlas->blasRefs.push_back(inst.blas);
        tlas->blasPtrs.push_back(inst.blas->blas);

        auto &d = descs[i];
        memset(&d, 0, sizeof(d));

        float t[12];
        writeMetalTransform3x4(inst.transform, t);

        memcpy(&d.transformationMatrix, t, sizeof(float) * 12);

        d.accelerationStructureIndex = (uint32_t)i;
        d.mask = inst.mask ? inst.mask : 0xFF;
        d.options = MTL::AccelerationStructureInstanceOptionOpaque;
        if (inst.cullDisable) {
            d.options |=
                MTL::AccelerationStructureInstanceOptionDisableTriangleCulling;
        }
        d.intersectionFunctionTableOffset = 0;
    }

    tlas->instanceBuffer =
        Buffer::create(BufferUsage::GeneralPurpose,
                       descs.size() * sizeof(descs[0]), descs.data());
    if (tlas->instanceBuffer == nullptr) {
        return nullptr;
    }

    tlas->tlasDescriptor =
        MTL::InstanceAccelerationStructureDescriptor::descriptor()->retain();
    tlas->tlasDescriptor->setUsage(
        MTL::AccelerationStructureUsagePreferFastIntersection);
    tlas->tlasDescriptor->setInstanceDescriptorType(
        MTL::AccelerationStructureInstanceDescriptorTypeDefault);
    auto &ib = metal::bufferState(tlas->instanceBuffer.get());

    tlas->tlasDescriptor->setInstanceDescriptorBuffer(ib.buffer);
    tlas->tlasDescriptor->setInstanceDescriptorStride(
        sizeof(MTL::AccelerationStructureInstanceDescriptor));
    tlas->tlasDescriptor->setInstanceCount((NS::UInteger)descs.size());

    NS::Array *instancedAS =
        NS::Array::array((NS::Object **)tlas->blasPtrs.data(),
                         (NS::UInteger)tlas->blasPtrs.size());
    tlas->tlasDescriptor->setInstancedAccelerationStructures(instancedAS);

    metal::DeviceState &deviceState =
        metal::deviceState(Device::globalInstance);

    MTL::AccelerationStructureSizes sizes =
        deviceState.device->accelerationStructureSizes(tlas->tlasDescriptor);

    tlas->scratch = Buffer::create(BufferUsage::GeneralPurpose,
                                   sizes.buildScratchBufferSize);

    MTL::AccelerationStructure *tlasPtr =
        deviceState.device->newAccelerationStructure(
            sizes.accelerationStructureSize);

    if (tlasPtr == nullptr) {
        return nullptr;
    }

    tlas->tlas = tlasPtr;

    return tlas;
}

void opal::CommandBuffer::buildInstanceAccelerationStructure(
    const std::shared_ptr<InstanceAccelerationStructure> &tlas) {
    auto &deviceState = metal::deviceState(Device::globalInstance);
    auto &state = metal::commandBufferState(this);

    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }

    auto *cs = state.commandBuffer;
    auto &scratchState = metal::bufferState(tlas->scratch.get());

    auto *asEnc = cs->accelerationStructureCommandEncoder();
    asEnc->buildAccelerationStructure(tlas->tlas, tlas->tlasDescriptor,
                                      scratchState.buffer, 0);
    asEnc->endEncoding();

    tlas->isBuilt = true;
    state.pendingResources.emplace_back(tlas->scratch);
    state.pendingResources.emplace_back(tlas->instanceBuffer);
    tlas->scratch.reset();
    tlas->instanceBuffer.reset();
}

#endif
