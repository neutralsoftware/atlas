//
// softbody.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Softbody physics implementation for Bezel
// SPDX-License-Identifier: MIT
//

#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/object.h"
#include "bezel/bezel.h"

#include "geogram/mesh/mesh.h"

#include <floattetwild/FloatTetwild.h>
#include <floattetwild/Parameters.h>
#include <floattetwild/MeshIO.hpp>

#include <Eigen/Core>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#ifndef BEZEL_NATIVE

namespace bezel {

namespace {

struct FaceKey {
    uint32_t a;
    uint32_t b;
    uint32_t c;

    bool operator==(const FaceKey &other) const {
        return a == other.a && b == other.b && c == other.c;
    }
};

struct FaceKeyHash {
    std::size_t operator()(const FaceKey &face) const {
        std::size_t h1 = std::hash<uint32_t>{}(face.a);
        std::size_t h2 = std::hash<uint32_t>{}(face.b);
        std::size_t h3 = std::hash<uint32_t>{}(face.c);

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct FaceEntry {
    uint32_t count = 0;

    SoftbodyTriangle triangle;
};

FaceKey makeFaceKey(uint32_t a, uint32_t b, uint32_t c) {
    std::array<uint32_t, 3> vertices = {a, b, c};

    std::sort(vertices.begin(), vertices.end());

    return {.a = vertices[0], .b = vertices[1], .c = vertices[2]};
}

void addFace(std::unordered_map<FaceKey, FaceEntry, FaceKeyHash> &faces,
             uint32_t a, uint32_t b, uint32_t c) {
    FaceKey key = makeFaceKey(a, b, c);

    FaceEntry &entry = faces[key];

    if (entry.count == 0) {
        entry.triangle = {.a = a, .b = b, .c = c};
    }

    ++entry.count;
}

std::vector<SoftbodyTriangle>
extractSurfaceTriangles(const std::vector<SoftbodyTetrahedron> &tetrahedra) {
    std::unordered_map<FaceKey, FaceEntry, FaceKeyHash> faces;

    faces.reserve(tetrahedra.size() * 4);

    for (const SoftbodyTetrahedron &tet : tetrahedra) {
        addFace(faces, tet.a, tet.b, tet.c);
        addFace(faces, tet.a, tet.d, tet.b);
        addFace(faces, tet.a, tet.c, tet.d);
        addFace(faces, tet.b, tet.d, tet.c);
    }

    std::vector<SoftbodyTriangle> surface;

    surface.reserve(faces.size());

    for (const auto &[key, entry] : faces) {
        if (entry.count != 1) {
            continue;
        }

        surface.push_back(entry.triangle);
    }

    return surface;
}

} // namespace

void Softbody::createMesh(CoreObject *object) {
    if (!object) {
        atlas_error("Cannot create softbody mesh from null CoreObject");
        return;
    }

    if (object->vertices.empty()) {
        atlas_error("Cannot create softbody mesh: CoreObject has no vertices");
        return;
    }

    if (object->indices.empty()) {
        atlas_error("Cannot create softbody mesh: CoreObject has no indices");
        return;
    }

    if (object->indices.size() % 3 != 0) {
        atlas_error(
            "Cannot create softbody mesh: index count is not divisible by 3");
        return;
    }

    this->object = object;

    mesh.vertices.clear();
    mesh.tetrahedra.clear();
    mesh.surface.clear();

    GEO::Mesh surfaceMesh;

    const std::size_t vertexCount = object->vertices.size();
    const std::size_t faceCount = object->indices.size() / 3;

    surfaceMesh.vertices.create_vertices(
        static_cast<GEO::index_t>(vertexCount));

    for (std::size_t i = 0; i < vertexCount; ++i) {
        const CoreVertex &vertex = object->vertices[i];

        double *position =
            surfaceMesh.vertices.point_ptr(static_cast<GEO::index_t>(i));

        position[0] = static_cast<double>(vertex.position.x * object->scale.x);

        position[1] = static_cast<double>(vertex.position.y * object->scale.y);

        position[2] = static_cast<double>(vertex.position.z * object->scale.z);
    }

    for (std::size_t i = 0; i < faceCount; ++i) {
        uint32_t a = static_cast<uint32_t>(object->indices[(i * 3) + 0]);

        uint32_t b = static_cast<uint32_t>(object->indices[(i * 3) + 1]);

        uint32_t c = static_cast<uint32_t>(object->indices[(i * 3) + 2]);

        if (a >= vertexCount || b >= vertexCount || c >= vertexCount) {
            atlas_error("Cannot create softbody mesh: invalid triangle index");
            return;
        }

        GEO::index_t facet = surfaceMesh.facets.create_polygon(3);

        surfaceMesh.facets.set_vertex(facet, 0, static_cast<GEO::index_t>(a));

        surfaceMesh.facets.set_vertex(facet, 1, static_cast<GEO::index_t>(b));

        surfaceMesh.facets.set_vertex(facet, 2, static_cast<GEO::index_t>(c));
    }

    surfaceMesh.facets.connect();

    floatTetWild::Parameters params;

    params.ideal_edge_length_rel = 0.1;

    Eigen::MatrixXd verticesOut;
    Eigen::MatrixXi tetrahedraOut;

    int status = floatTetWild::tetrahedralization(surfaceMesh, params,
                                                  verticesOut, tetrahedraOut);

    if (status != 0) {
        atlas_error("Softbody tetrahedralization failed with status: " +
                    std::to_string(status));

        return;
    }

    if (verticesOut.cols() != 3 || tetrahedraOut.cols() != 4) {
        atlas_error(
            "Softbody tetrahedralization returned invalid mesh dimensions");

        return;
    }

    mesh.vertices.reserve(static_cast<std::size_t>(verticesOut.rows()));

    for (Eigen::Index i = 0; i < verticesOut.rows(); ++i) {
        SoftbodyVertex vertex;

        vertex.position = {static_cast<float>(verticesOut(i, 0)),
                           static_cast<float>(verticesOut(i, 1)),
                           static_cast<float>(verticesOut(i, 2))};

        mesh.vertices.push_back(vertex);
    }

    mesh.tetrahedra.reserve(static_cast<std::size_t>(tetrahedraOut.rows()));

    for (Eigen::Index i = 0; i < tetrahedraOut.rows(); ++i) {
        SoftbodyTetrahedron tet;

        tet.a = static_cast<uint32_t>(tetrahedraOut(i, 0));

        tet.b = static_cast<uint32_t>(tetrahedraOut(i, 1));

        tet.c = static_cast<uint32_t>(tetrahedraOut(i, 2));

        tet.d = static_cast<uint32_t>(tetrahedraOut(i, 3));

        if (tet.a >= mesh.vertices.size() || tet.b >= mesh.vertices.size() ||
            tet.c >= mesh.vertices.size() || tet.d >= mesh.vertices.size()) {
            atlas_error("Softbody tetrahedralization produced invalid "
                        "tetrahedron indices");

            mesh.vertices.clear();
            mesh.tetrahedra.clear();
            mesh.surface.clear();

            return;
        }

        mesh.tetrahedra.push_back(tet);
    }

    mesh.surface = extractSurfaceTriangles(mesh.tetrahedra);

    atlas_log("Created softbody mesh:" + std::to_string(mesh.vertices.size()) +
              " vertices, " + std::to_string(mesh.tetrahedra.size()) +
              " tetrahedra, " + std::to_string(mesh.surface.size()) +
              " surface triangles");
}

} // namespace bezel

#endif