//
// colliders.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Collider creation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/bezel.h"

bezel::MeshCollider::MeshCollider(const std::vector<Position3d> &vertices,
                                  const std::vector<uint32_t> &indices)
    : vertices(vertices), indices(indices) {}

JPH::RefConst<JPH::Shape> bezel::BoxCollider::getJoltShape() const {
    return new JPH::BoxShape(JPH::Vec3(static_cast<float>(halfExtents.x),
                                       static_cast<float>(halfExtents.y),
                                       static_cast<float>(halfExtents.z)));
}

JPH::RefConst<JPH::Shape> bezel::CapsuleCollider::getJoltShape() const {
    const float safeRadius = std::isfinite(radius)
        ? std::clamp(radius, 0.001f, 1000000.0f) : 0.5f;
    const float safeHeight = std::isfinite(height)
        ? std::clamp(height, 0.001f, 1000000.0f) : 1.0f;
    return new JPH::CapsuleShape(safeHeight / 2, safeRadius);
}

JPH::RefConst<JPH::Shape> bezel::SphereCollider::getJoltShape() const {
    const float safeRadius = std::isfinite(radius)
        ? std::clamp(radius, 0.001f, 1000000.0f) : 0.5f;
    return new JPH::SphereShape(safeRadius);
}

JPH::RefConst<JPH::Shape> bezel::MeshCollider::getJoltShape() const {
    JPH::VertexList v;
    v.reserve(vertices.size());
    for (const auto &vert : vertices)
        v.emplace_back((float)vert.x, (float)vert.y, (float)vert.z);

    JPH::IndexedTriangleList tris;
    tris.reserve(indices.size() / 3);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        auto i0 = (uint32_t)indices[i + 0];
        auto i1 = (uint32_t)indices[i + 1];
        auto i2 = (uint32_t)indices[i + 2];

        tris.emplace_back(i0, i1, i2, 0);
    }

    JPH::MeshShapeSettings settings(v, tris);

    JPH::ShapeSettings::ShapeResult result = settings.Create();
    if (result.HasError())
        return nullptr;

    return result.Get();
}
