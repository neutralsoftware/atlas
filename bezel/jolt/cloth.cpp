//
// cloth.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Cloth simulation interfaces and functions
// SPDX-License-Identifier: MIT
//

#include "atlas/object.h"
#include "bezel/bezel.h"
#include "bezel/jolt/world.h"
#include "glm/ext/vector_float2.hpp"
#include <algorithm>

#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>

namespace {
glm::vec2 getAnchorTarget(bezel::AnchorPoint point) {
    using bezel::AnchorPoint;

    switch (point) {
    case AnchorPoint::TopLeft:
        return {0.0f, 1.0f};

    case AnchorPoint::TopCenter:
        return {0.5f, 1.0f};

    case AnchorPoint::TopRight:
        return {1.0f, 1.0f};

    case AnchorPoint::CenterLeft:
        return {0.0f, 0.5f};

    case AnchorPoint::Center:
        return {0.5f, 0.5f};

    case AnchorPoint::CenterRight:
        return {1.0f, 0.5f};

    case AnchorPoint::BottomLeft:
        return {0.0f, 0.0f};

    case AnchorPoint::BottomCenter:
        return {0.5f, 0.0f};

    case AnchorPoint::BottomRight:
        return {1.0f, 0.0f};
    }

    return {0.5f, 0.5f};
}

JPH::SoftBodySharedSettings::EBendType
toJoltBendType(bezel::ClothBendType type) {
    switch (type) {
    case bezel::ClothBendType::None:
        return JPH::SoftBodySharedSettings::EBendType::None;

    case bezel::ClothBendType::Distance:
        return JPH::SoftBodySharedSettings::EBendType::Distance;

    case bezel::ClothBendType::Dihedral:
        return JPH::SoftBodySharedSettings::EBendType::Dihedral;
    }

    return JPH::SoftBodySharedSettings::EBendType::Dihedral;
}

void recalculateClothNormals(CoreObject *object) {
    std::vector<glm::vec3> normals(object->vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < object->indices.size(); i += 3) {
        const Index ia = object->indices[i + 0];
        const Index ib = object->indices[i + 1];
        const Index ic = object->indices[i + 2];

        const glm::vec3 a = object->vertices[ia].position.toGlm();
        const glm::vec3 b = object->vertices[ib].position.toGlm();
        const glm::vec3 c = object->vertices[ic].position.toGlm();

        const glm::vec3 crossProduct = glm::cross(b - a, c - a);

        const float lengthSquared = glm::dot(crossProduct, crossProduct);

        if (lengthSquared <= 1e-12f) {
            continue;
        }

        const glm::vec3 normal = glm::normalize(crossProduct);

        normals[ia] += normal;
        normals[ib] += normal;
        normals[ic] += normal;
    }

    for (size_t i = 0; i < object->vertices.size(); ++i) {
        if (glm::dot(normals[i], normals[i]) > 1e-12f) {

            normals[i] = glm::normalize(normals[i]);
        }

        object->vertices[i].normal = {
            normals[i].x,
            normals[i].y,
            normals[i].z,
        };
    }
}
} // namespace

void bezel::Cloth::setAnchors(const std::vector<AnchorPoint> &newAnchors) {
    anchors = newAnchors;
}

void bezel::Cloth::addAnchor(AnchorPoint anchor) {
    if (std::find(anchors.begin(), anchors.end(), anchor) == anchors.end()) {
        anchors.push_back(anchor);
    }
}

void bezel::Cloth::removeAnchor(AnchorPoint anchor) {
    std::erase(anchors, anchor);
}

void bezel::Cloth::clearAnchors() { anchors.clear(); }

void bezel::Cloth::pinVertex(uint32_t vertex) {
    if (std::find(manuallyPinnedVertices.begin(), manuallyPinnedVertices.end(),
                  vertex) == manuallyPinnedVertices.end()) {
        manuallyPinnedVertices.push_back(vertex);
    }
}

void bezel::Cloth::unpinVertex(uint32_t vertex) {
    std::erase(manuallyPinnedVertices, vertex);
}

void bezel::Cloth::clearPinnedVertices() { manuallyPinnedVertices.clear(); }

void bezel::Cloth::setObject(CoreObject *newObject) {
    object = newObject;
    isMeshCreated = false;
}

void bezel::Cloth::resolveAnchors() {
    resolvedPinnedVertices.clear();

    if (object == nullptr || object->vertices.empty()) {
        return;
    }

    float minU = std::numeric_limits<float>::max();
    float minV = std::numeric_limits<float>::max();

    float maxU = std::numeric_limits<float>::lowest();
    float maxV = std::numeric_limits<float>::lowest();

    for (const CoreVertex &vertex : object->vertices) {
        minU = std::min(minU, vertex.textureCoordinate[0]);

        maxU = std::max(maxU, vertex.textureCoordinate[0]);

        minV = std::min(minV, vertex.textureCoordinate[1]);

        maxV = std::max(maxV, vertex.textureCoordinate[1]);
    }

    const float rangeU = maxU - minU;
    const float rangeV = maxV - minV;

    if ((!anchors.empty()) && (rangeU < 1e-6f || rangeV < 1e-6f)) {
        throw std::runtime_error(
            "Cloth AnchorPoint requires usable UV coordinates");
    }

    for (AnchorPoint anchor : anchors) {
        glm::vec2 target = getAnchorTarget(anchor);

        float closestDistance = std::numeric_limits<float>::max();

        uint32_t closestIndex = 0;

        for (uint32_t i = 0; i < object->vertices.size(); ++i) {

            const auto &uv = object->vertices[i].textureCoordinate;

            glm::vec2 normalizedUV{
                (uv[0] - minU) / rangeU,
                (uv[1] - minV) / rangeV,
            };

            const glm::vec2 delta = normalizedUV - target;

            const float distance = glm::dot(delta, delta);

            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = i;
            }
        }

        const Position3d anchorPosition =
            object->vertices[closestIndex].position;

        constexpr double epsilon = 1e-6;

        for (uint32_t i = 0; i < object->vertices.size(); ++i) {

            const Position3d delta =
                object->vertices[i].position - anchorPosition;

            const double distanceSquared =
                (delta.x * delta.x) + (delta.y * delta.y) + (delta.z * delta.z);

            if (distanceSquared <= epsilon * epsilon) {

                resolvedPinnedVertices.push_back(i);
            }
        }
    }

    for (uint32_t index : manuallyPinnedVertices) {
        if (index >= object->vertices.size()) {
            throw std::runtime_error("Cloth pinned vertex index out of bounds");
        }

        resolvedPinnedVertices.push_back(index);
    }

    std::ranges::sort(resolvedPinnedVertices.begin(),
                      resolvedPinnedVertices.end());

    resolvedPinnedVertices.erase(std::unique(resolvedPinnedVertices.begin(),
                                             resolvedPinnedVertices.end()),
                                 resolvedPinnedVertices.end());
}

void bezel::Cloth::createMesh() {
    if (object == nullptr) {
        throw std::runtime_error("Cannot create cloth without a CoreObject");
    }

    if (object->vertices.empty()) {
        throw std::runtime_error("Cannot create cloth from empty mesh");
    }

    if (object->indices.empty() || object->indices.size() % 3 != 0) {
        throw std::runtime_error("Cloth requires indexed triangle geometry");
    }

    resolveAnchors();

    mesh.vertices.clear();
    mesh.surface.clear();

    mesh.vertices.reserve(object->vertices.size());

    mesh.surface.reserve(object->indices.size() / 3);

    const size_t movableVertexCount =
        object->vertices.size() - resolvedPinnedVertices.size();

    if (mass <= 0.0f) {
        throw std::runtime_error("Cloth mass must be greater than zero");
    }

    const float inverseMass =
        movableVertexCount > 0 ? static_cast<float>(movableVertexCount) / mass
                               : 0.0f;

    auto isPinned = [&](uint32_t index) {
        return std::binary_search(resolvedPinnedVertices.begin(),
                                  resolvedPinnedVertices.end(), index);
    };

    for (uint32_t i = 0; i < object->vertices.size(); ++i) {
        SoftbodyVertex vertex;

        vertex.position = object->vertices[i].position;
        vertex.inverseMass = isPinned(i) ? 0.0f : inverseMass;

        mesh.vertices.push_back(vertex);
    }

    for (size_t i = 0; i < object->indices.size(); i += 3) {
        const uint32_t a = object->indices[i + 0];
        const uint32_t b = object->indices[i + 1];
        const uint32_t c = object->indices[i + 2];

        if (a >= mesh.vertices.size() || b >= mesh.vertices.size() ||
            c >= mesh.vertices.size()) {

            throw std::runtime_error("Cloth mesh contains invalid index");
        }

        mesh.surface.push_back({
            .a = a,
            .b = b,
            .c = c,
        });
    }

    isMeshCreated = true;
}

void bezel::Cloth::create(const std::shared_ptr<PhysicsWorld> &world) {
    if (!world) {
        throw std::runtime_error("Cannot create cloth without PhysicsWorld");
    }

    if (!isMeshCreated) {
        createMesh();
    }

    if (mesh.vertices.empty()) {
        throw std::runtime_error("Cannot create cloth from empty mesh");
    }

    JPH::Ref<JPH::SoftBodySharedSettings> settings =
        new JPH::SoftBodySharedSettings();

    settings->mVertices.reserve(mesh.vertices.size());

    settings->mFaces.reserve(mesh.surface.size());

    for (const SoftbodyVertex &vertex : mesh.vertices) {
        settings->mVertices.emplace_back(
            JPH::Float3(static_cast<float>(vertex.position.x),
                        static_cast<float>(vertex.position.y),
                        static_cast<float>(vertex.position.z)),
            JPH::Float3(0.0f, 0.0f, 0.0f), vertex.inverseMass);
    }

    for (const SoftbodyTriangle &triangle : mesh.surface) {
        settings->AddFace(JPH::SoftBodySharedSettings::Face(
            triangle.a, triangle.b, triangle.c, 0));
    }

    JPH::SoftBodySharedSettings::VertexAttributes attributes;
    attributes.mCompliance = material.stretchCompliance;
    attributes.mShearCompliance = material.shearCompliance;
    attributes.mBendCompliance = material.bendCompliance;

    settings->CreateConstraints(&attributes, 1, toJoltBendType(bendType));
    settings->Optimize();

    JPH::SoftBodyCreationSettings creationSettings(
        settings, JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z,
                  rotationQuat.w),
        jolt::layers::MOVING);

    creationSettings.mNumIterations = solverIterations;
    creationSettings.mLinearDamping = material.damping;
    creationSettings.mFriction = material.friction;
    creationSettings.mRestitution = material.restitution;
    creationSettings.mGravityFactor = gravityFactor;
    creationSettings.mAllowSleeping = allowSleeping;
    creationSettings.mFacesDoubleSided = doubleSided;
    creationSettings.mVertexRadius = vertexRadius;
    creationSettings.mUserData = id.atlasId;

    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    const JPH::BodyID bodyId = bodyInterface.CreateAndAddSoftBody(
        creationSettings, JPH::EActivation::Activate);

    if (bodyId.IsInvalid()) {
        throw std::runtime_error("Failed to create Jolt cloth body");
    }

    id.joltId = bodyId.GetIndexAndSequenceNumber();
}

void bezel::Cloth::destroy(const std::shared_ptr<PhysicsWorld> &world) {
    if (!world || id.joltId == INVALID_JOLT_ID) {
        return;
    }

    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    JPH::BodyID bodyId(id.joltId);

    bodyInterface.RemoveBody(bodyId);
    bodyInterface.DestroyBody(bodyId);

    id.joltId = INVALID_JOLT_ID;
}

bool bezel::Cloth::isCreated() const { return id.joltId != INVALID_JOLT_ID; }

void bezel::Cloth::updateVertices(
    const std::shared_ptr<PhysicsWorld> &world) const {
    if (!world || !object || id.joltId == INVALID_JOLT_ID) {
        return;
    }

    JPH::BodyID bodyId(id.joltId);

    JPH::BodyLockRead lock(world->physicsSystem.GetBodyLockInterface(), bodyId);

    if (!lock.Succeeded()) {
        return;
    }

    const JPH::Body &joltBody = lock.GetBody();

    if (!joltBody.IsSoftBody()) {
        return;
    }

    const auto *motion = static_cast<const JPH::SoftBodyMotionProperties *>(
        joltBody.GetMotionProperties());

    const auto &vertices = motion->GetVertices();

    if (vertices.size() != object->vertices.size()) {

        throw std::runtime_error(
            "Cloth simulation/render vertex count mismatch");
    }

    for (size_t i = 0; i < vertices.size(); ++i) {

        const JPH::Vec3 &position = vertices[i].mPosition;

        object->vertices[i].position = {
            position.GetX(),
            position.GetY(),
            position.GetZ(),
        };
    }

    const JPH::RVec3 bodyPosition = joltBody.GetCenterOfMassPosition();

    object->setPosition({
        bodyPosition.GetX(),
        bodyPosition.GetY(),
        bodyPosition.GetZ(),
    });

    const JPH::Quat bodyRotation = joltBody.GetRotation();

    object->setRotationQuat(glm::quat(bodyRotation.GetW(), bodyRotation.GetX(),
                                      bodyRotation.GetY(),
                                      bodyRotation.GetZ()));

    recalculateClothNormals(object);
    object->updateVertices();
}