//
// softbody.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Softbody physics implementation for Bezel
// SPDX-License-Identifier: MPL-2.0
//

#include "atlas/units.h"
#include "atlas/object.h"
#include "bezel/bezel.h"

#include <geogram/basic/common.h>
#include "geogram/mesh/mesh.h"

#include <floattetwild/FloatTetwild.h>
#include <floattetwild/Parameters.h>
#include <floattetwild/MeshIO.hpp>

#include <Eigen/Core>

#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
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

    std::ranges::sort(vertices.begin(), vertices.end());

    return {.a = vertices[0], .b = vertices[1], .c = vertices[2]};
}

struct EdgeKey {
    uint32_t a;
    uint32_t b;

    bool operator==(const EdgeKey &other) const {
        return a == other.a && b == other.b;
    }
};

struct EdgeKeyHash {
    std::size_t operator()(const EdgeKey &edge) const {
        return std::hash<uint32_t>{}(edge.a) ^
               (std::hash<uint32_t>{}(edge.b) << 1);
    }
};

EdgeKey makeEdgeKey(uint32_t a, uint32_t b) {
    if (a > b) {
        std::swap(a, b);
    }

    return {.a = a, .b = b};
}

void initializeGeogram() {
    static std::once_flag initializationFlag;

    std::call_once(initializationFlag, [] { GEO::initialize(); });
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

float complianceFromStiffness(float stiffness) {
    stiffness = std::clamp(stiffness, 0.0f, 1.0f);

    if (stiffness >= 0.9999f) {
        return 0.0f;
    }

    constexpr float maxCompliance = 1.0e-4f;

    const float x = 1.0f - stiffness;

    return maxCompliance * (x * x * x);
}

struct ClosestTrianglePoint {
    glm::vec3 point;
    glm::vec3 weights;
};

ClosestTrianglePoint closestPointOnTriangle(const glm::vec3 &p,
                                            const glm::vec3 &a,
                                            const glm::vec3 &b,
                                            const glm::vec3 &c) {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = p - a;

    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);

    if (d1 <= 0.0f && d2 <= 0.0f) {
        return {.point = a, .weights = {1.0f, 0.0f, 0.0f}};
    }

    const glm::vec3 bp = p - b;

    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);

    if (d3 >= 0.0f && d4 <= d3) {
        return {.point = b, .weights = {0.0f, 1.0f, 0.0f}};
    }

    const float vc = (d1 * d4) - (d3 * d2);

    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);

        return {.point = a + v * ab, .weights = {1.0f - v, v, 0.0f}};
    }

    const glm::vec3 cp = p - c;

    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);

    if (d6 >= 0.0f && d5 <= d6) {
        return {.point = c, .weights = {0.0f, 0.0f, 1.0f}};
    }

    const float vb = (d5 * d2) - (d1 * d6);

    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);

        return {.point = a + w * ac, .weights = {1.0f - w, 0.0f, w}};
    }

    const float va = (d3 * d6) - (d5 * d4);

    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const glm::vec3 bc = c - b;

        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

        return {.point = b + w * bc, .weights = {0.0f, 1.0f - w, w}};
    }

    const float denominator = 1.0f / (va + vb + vc);

    const float v = vb * denominator;
    const float w = vc * denominator;
    const float u = 1.0f - v - w;

    return {
        .point = u * a + v * b + w * c,
        .weights = {u, v, w},
    };
}

struct TetrahedralCacheHeader {
    uint64_t magic = 0x41544c4153544554ULL;
    uint64_t key = 0;
    uint64_t vertices = 0;
    uint64_t tetrahedra = 0;
};

uint64_t tetrahedralCacheKey(const CoreObject &object) {
    uint64_t hash = 1469598103934665603ULL;
    auto append = [&hash](const auto &value) {
        const auto *bytes = reinterpret_cast<const unsigned char *>(&value);
        for (size_t index = 0; index < sizeof(value); ++index) {
            hash ^= bytes[index];
            hash *= 1099511628211ULL;
        }
    };
    const uint32_t version = 1;
    append(version);
    append(object.scale.x);
    append(object.scale.y);
    append(object.scale.z);
    const uint64_t vertexCount = object.vertices.size();
    const uint64_t indexCount = object.indices.size();
    append(vertexCount);
    append(indexCount);
    for (const auto &vertex : object.vertices) {
        append(vertex.position.x);
        append(vertex.position.y);
        append(vertex.position.z);
    }
    for (const auto &index : object.indices)
        append(index);
    return hash;
}

bool loadTetrahedralCache(const std::filesystem::path &path, uint64_t key,
                          SoftbodyMesh &mesh) {
    std::ifstream input(path, std::ios::binary);
    TetrahedralCacheHeader header;
    input.read(reinterpret_cast<char *>(&header), sizeof(header));
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (!input || error || header.magic != 0x41544c4153544554ULL ||
        header.key != key || header.vertices == 0 || header.tetrahedra == 0 ||
        header.vertices > 10000000 || header.tetrahedra > 20000000 ||
        size != sizeof(header) + header.vertices * 3 * sizeof(float) +
                    header.tetrahedra * 4 * sizeof(uint32_t))
        return false;
    SoftbodyMesh cached;
    cached.vertices.resize(static_cast<size_t>(header.vertices));
    cached.tetrahedra.resize(static_cast<size_t>(header.tetrahedra));
    for (auto &vertex : cached.vertices) {
        float position[3];
        input.read(reinterpret_cast<char *>(position), sizeof(position));
        if (!input || !std::isfinite(position[0]) ||
            !std::isfinite(position[1]) || !std::isfinite(position[2]))
            return false;
        vertex.position = {position[0], position[1], position[2]};
    }
    for (auto &tet : cached.tetrahedra) {
        uint32_t indices[4];
        input.read(reinterpret_cast<char *>(indices), sizeof(indices));
        if (!input || std::any_of(std::begin(indices), std::end(indices),
                                 [&](uint32_t index) { return index >= header.vertices; }))
            return false;
        tet = {indices[0], indices[1], indices[2], indices[3]};
    }
    cached.surface = extractSurfaceTriangles(cached.tetrahedra);
    if (cached.surface.empty())
        return false;
    mesh = std::move(cached);
    return true;
}

void storeTetrahedralCache(const std::filesystem::path &path, uint64_t key,
                           const SoftbodyMesh &mesh) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path temporary = path.string() + "." +
                                             std::to_string(nonce) + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output)
        return;
    const TetrahedralCacheHeader header{0x41544c4153544554ULL, key,
                                       mesh.vertices.size(), mesh.tetrahedra.size()};
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
    for (const auto &vertex : mesh.vertices) {
        const float position[3]{vertex.position.x, vertex.position.y, vertex.position.z};
        output.write(reinterpret_cast<const char *>(position), sizeof(position));
    }
    for (const auto &tet : mesh.tetrahedra) {
        const uint32_t indices[4]{tet.a, tet.b, tet.c, tet.d};
        output.write(reinterpret_cast<const char *>(indices), sizeof(indices));
    }
    output.close();
    if (output)
        std::filesystem::rename(temporary, path, error);
    if (!output || error)
        std::filesystem::remove(temporary, error);
}

} // namespace

void Softbody::createMesh() {

    const auto tetrahedralizationStarted = std::chrono::steady_clock::now();
    const auto reportProgress = [&](int percent, const char *phase) {
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - tetrahedralizationStarted)
                .count();

        std::cout << "[softbody] " << percent << "% " << phase << " ("
                  << (elapsed / 1000.0) << "s)" << std::endl;
    };

    if (!object) {
        throw std::runtime_error(
            "Cannot create softbody mesh from null CoreObject");
        return;
    }

    if (object->vertices.empty()) {
        throw std::runtime_error(
            "Cannot create softbody mesh: CoreObject has no vertices");
        return;
    }

    if (object->indices.empty()) {
        throw std::runtime_error(
            "Cannot create softbody mesh: CoreObject has no indices");
        return;
    }

    if (object->indices.size() % 3 != 0) {
        throw std::runtime_error(
            "Cannot create softbody mesh: index count is not divisible by 3");
        return;
    }

    const uint64_t cacheKey = tetrahedralCacheKey(*object);
    if (isMeshCreated && generatedMeshKey == cacheKey)
        return;
    isMeshCreated = false;
    generatedMeshKey = cacheKey;
    std::error_code cacheError;
    const std::filesystem::path cacheDirectory = meshCacheDirectory.empty()
        ? std::filesystem::temp_directory_path(cacheError) / "atlas-tetrahedra-v1"
        : std::filesystem::path(meshCacheDirectory);
    const std::filesystem::path cachePath = cacheDirectory /
        (std::to_string(cacheKey) + ".tetra");
    meshCacheFile = cacheError ? std::string() : cachePath.string();
    if (!cacheError && loadTetrahedralCache(cachePath, cacheKey, mesh)) {
        reportProgress(100, "loaded cached tetrahedra");
        isMeshCreated = true;
        return;
    }
    initializeGeogram();
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
            throw std::runtime_error(
                "Cannot create softbody mesh: invalid triangle index");
            return;
        }

        GEO::index_t facet = surfaceMesh.facets.create_polygon(3);

        surfaceMesh.facets.set_vertex(facet, 0, static_cast<GEO::index_t>(a));
        surfaceMesh.facets.set_vertex(facet, 1, static_cast<GEO::index_t>(b));
        surfaceMesh.facets.set_vertex(facet, 2, static_cast<GEO::index_t>(c));
    }

    surfaceMesh.facets.connect();

    reportProgress(10, "surface mesh ready; tetrahedralizing");

    floatTetWild::Parameters params;

    params.ideal_edge_length_rel = 0.25;
    params.max_its = 8;
    params.stop_energy = 20;

    Eigen::MatrixXd verticesOut;
    Eigen::MatrixXi tetrahedraOut;

    int status = floatTetWild::tetrahedralization(surfaceMesh, params,
                                                  verticesOut, tetrahedraOut);

    reportProgress(90, "tetrahedralization complete; building render surface");

    if (status != 0) {
        throw std::runtime_error(
            "Softbody tetrahedralization failed with status: " +
            std::to_string(status));

        return;
    }

    if (verticesOut.cols() != 3 || tetrahedraOut.cols() != 4) {
        throw std::runtime_error(
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
            throw std::runtime_error(
                "Softbody tetrahedralization produced invalid "
                "tetrahedron indices");

            mesh.vertices.clear();
            mesh.tetrahedra.clear();
            mesh.surface.clear();

            return;
        }

        mesh.tetrahedra.push_back(tet);
    }

    mesh.surface = extractSurfaceTriangles(mesh.tetrahedra);
    if (!cacheError)
        storeTetrahedralCache(cachePath, cacheKey, mesh);

    reportProgress(100, "softbody mesh ready");

    std::cout << ("Created softbody mesh:" +
                  std::to_string(mesh.vertices.size()) + " vertices, " +
                  std::to_string(mesh.tetrahedra.size()) + " tetrahedra, " +
                  std::to_string(mesh.surface.size()) + " surface triangles");

    isMeshCreated = true;
}

void Softbody::setObject(CoreObject *object) {
    if (!object) {
        throw std::runtime_error("Cannot set null CoreObject for softbody");
        return;
    }

    this->object = object;
}

void Softbody::create(const std::shared_ptr<PhysicsWorld> &world) {
    if (!world || !world->initialized) {
        throw std::runtime_error(
            "Cannot create softbody: invalid or uninitialized world");
        return;
    }

    if (!object) {
        throw std::runtime_error(
            "Cannot create softbody: no CoreObject attached");
        return;
    }

    if (mesh.vertices.empty() || mesh.tetrahedra.empty() ||
        mesh.surface.empty()) {
        throw std::runtime_error("Cannot create softbody: mesh is empty");
        return;
    }

    if (id.joltId != INVALID_JOLT_ID) {
        throw std::runtime_error(
            "Cannot create softbody: already created in world");
        return;
    }

    JPH::Ref<JPH::SoftBodySharedSettings> settings =
        new JPH::SoftBodySharedSettings();

    settings->mVertices.reserve(mesh.vertices.size());

    const float vertexMass = mass / static_cast<float>(mesh.vertices.size());

    const float inverseMass = (vertexMass > 0.0f) ? (1.0f / vertexMass) : 0.0f;

    for (const SoftbodyVertex &vertex : mesh.vertices) {
        settings->mVertices.emplace_back(
            JPH::Float3(static_cast<float>(vertex.position.x),
                        static_cast<float>(vertex.position.y),
                        static_cast<float>(vertex.position.z)),
            JPH::Float3(0.0f, 0.0f, 0.0f), inverseMass);
    }

    settings->mFaces.reserve(mesh.surface.size());

    for (const SoftbodyTriangle &triangle : mesh.surface) {
        settings->mFaces.emplace_back(static_cast<uint32_t>(triangle.a),
                                      static_cast<uint32_t>(triangle.b),
                                      static_cast<uint32_t>(triangle.c));
    }

    const float edgeCompliance = complianceFromStiffness(material.stiffness);

    const float volumeCompliance =
        complianceFromStiffness(material.volumeStiffness);

    std::unordered_set<EdgeKey, EdgeKeyHash> edges;

    edges.reserve(mesh.tetrahedra.size() * 6);

    auto addEdge = [&](uint32_t a, uint32_t b) {
        EdgeKey edge = makeEdgeKey(a, b);

        if (!edges.insert(edge).second) {
            return;
        }

        settings->mEdgeConstraints.emplace_back(edge.a, edge.b, edgeCompliance);
    };

    settings->mVolumeConstraints.reserve(mesh.tetrahedra.size());

    for (const SoftbodyTetrahedron &tet : mesh.tetrahedra) {

        addEdge(tet.a, tet.b);
        addEdge(tet.a, tet.c);
        addEdge(tet.a, tet.d);

        addEdge(tet.b, tet.c);
        addEdge(tet.b, tet.d);

        addEdge(tet.c, tet.d);

        settings->mVolumeConstraints.emplace_back(tet.a, tet.b, tet.c, tet.d,
                                                  volumeCompliance);
    }

    settings->CalculateEdgeLengths();
    settings->CalculateVolumeConstraintVolumes();
    settings->Optimize();

    JPH::RVec3 joltPosition(position.x, position.y, position.z);

    JPH::Quat joltRotation(rotationQuat.x, rotationQuat.y, rotationQuat.z,
                           rotationQuat.w);

    const JPH::ObjectLayer layer =
        isSensor ? bezel::jolt::layers::SENSOR : bezel::jolt::layers::MOVING;

    JPH::SoftBodyCreationSettings creation(settings, joltPosition, joltRotation,
                                           layer);

    creation.mNumIterations = solverIterations;
    creation.mLinearDamping = material.damping;
    creation.mMaxLinearVelocity = 50.0f;
    creation.mFriction = material.friction;
    creation.mRestitution = material.restitution;
    creation.mGravityFactor = gravityFactor;
    creation.mAllowSleeping = allowSleeping;
    creation.mMakeRotationIdentity = true;
    creation.mFacesDoubleSided = true;
    creation.mVertexRadius = 0.01f;

    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    JPH::BodyID bodyId = bodyInterface.CreateAndAddSoftBody(
        creation, JPH::EActivation::Activate);

    if (bodyId.IsInvalid()) {
        throw std::runtime_error("Jolt failed to create softbody");
        return;
    }

    id.joltId = bodyId.GetIndexAndSequenceNumber();

    createRenderBindings();
    object->setScale({1.0f, 1.0f, 1.0f});
    updateVertices(world);

    std::cout << ("Created Jolt softbody with " +
                  std::to_string(mesh.vertices.size()) + " vertices and " +
                  std::to_string(mesh.tetrahedra.size()) + " tetrahedra");
}

void Softbody::createRenderBindings() {
    renderBindings.clear();

    if (!object) {
        return;
    }

    renderBindings.reserve(object->vertices.size());

    for (const CoreVertex &vertex : object->vertices) {
        glm::vec3 p((vertex.position.x * object->scale.x),
                    (vertex.position.y * object->scale.y),
                    (vertex.position.z * object->scale.z));

        float bestDistanceSq = std::numeric_limits<float>::max();

        SoftbodyRenderBinding best{};

        bool found = false;

        for (const SoftbodyTriangle &triangle : mesh.surface) {
            const SoftbodyVertex &va = mesh.vertices[triangle.a];
            const SoftbodyVertex &vb = mesh.vertices[triangle.b];
            const SoftbodyVertex &vc = mesh.vertices[triangle.c];

            glm::vec3 a(va.position.x, va.position.y, va.position.z);
            glm::vec3 b(vb.position.x, vb.position.y, vb.position.z);
            glm::vec3 c(vc.position.x, vc.position.y, vc.position.z);

            ClosestTrianglePoint closest = closestPointOnTriangle(p, a, b, c);

            const glm::vec3 delta = closest.point - p;

            const float distanceSq = glm::dot(delta, delta);

            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;

                best = {
                    .a = triangle.a,
                    .b = triangle.b,
                    .c = triangle.c,
                    .weights = closest.weights,
                };

                found = true;
            }
        }

        if (!found) {
            throw std::runtime_error(
                "Failed to bind Atlas vertex to softbody surface");

            renderBindings.clear();
            return;
        }

        renderBindings.push_back(best);
    }
}

void Softbody::updateVertices(const std::shared_ptr<PhysicsWorld> &world) {
    if (!world || !world->initialized || id.joltId == INVALID_JOLT_ID) {
        throw std::runtime_error(
            "Cannot update softbody vertices: invalid world or body ID");
        return;
    }

    if (renderBindings.size() != object->vertices.size()) {
        throw std::runtime_error(
            "Cannot update softbody vertices: render bindings not created");
        return;
    }

    const JPH::BodyID bodyId(id.joltId);
    JPH::BodyLockRead lock(world->physicsSystem.GetBodyLockInterface(), bodyId);

    if (!lock.Succeeded()) {
        throw std::runtime_error("Failed to lock softbody for reading");
        return;
    }

    const JPH::Body &body = lock.GetBody();

    if (!body.IsSoftBody()) {
        throw std::runtime_error("Body is not a softbody");
        return;
    }

    const auto *motion = static_cast<const JPH::SoftBodyMotionProperties *>(
        body.GetMotionProperties());

    const auto &vertices = motion->GetVertices();

    for (std::size_t i = 0; i < object->vertices.size(); ++i) {
        const SoftbodyRenderBinding &binding = renderBindings[i];

        if (binding.a >= vertices.size() || binding.b >= vertices.size() ||
            binding.c >= vertices.size()) {
            continue;
        }

        const JPH::Vec3 a = vertices[binding.a].mPosition;
        const JPH::Vec3 b = vertices[binding.b].mPosition;
        const JPH::Vec3 c = vertices[binding.c].mPosition;

        JPH::Vec3 position = a * binding.weights.x + b * binding.weights.y +
                             c * binding.weights.z;

        object->vertices[i].position = {position.GetX(), position.GetY(),
                                        position.GetZ()};
    }

    const JPH::RVec3 comPosition = body.GetCenterOfMassPosition();

    const JPH::Quat comRotation = body.GetRotation();

    object->setPosition(
        {comPosition.GetX(), comPosition.GetY(), comPosition.GetZ()});

    object->setRotationQuat(glm::quat(comRotation.GetW(), comRotation.GetX(),
                                      comRotation.GetY(), comRotation.GetZ()));

    object->updateVertices();
}

void Softbody::destroy(const std::shared_ptr<PhysicsWorld> &world) {
    if (!world || id.joltId == INVALID_JOLT_ID) {
        return;
    }

    JPH::BodyID bodyId(id.joltId);

    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    if (bodyInterface.IsAdded(bodyId)) {
        bodyInterface.RemoveBody(bodyId);
    }

    bodyInterface.DestroyBody(bodyId);

    id.joltId = INVALID_JOLT_ID;

    renderBindings.clear();
}

Velocity3d
Softbody::getLinearVelocity(const std::shared_ptr<PhysicsWorld> &world) const {
    if (!world || id.joltId == INVALID_JOLT_ID) {
        return {0.0f, 0.0f, 0.0f};
    }

    JPH::Vec3 velocity =
        world->physicsSystem.GetBodyInterface().GetLinearVelocity(
            JPH::BodyID(id.joltId));

    return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
}

Velocity3d
Softbody::getVelocity(const std::shared_ptr<PhysicsWorld> &world) const {
    return getLinearVelocity(world);
}

} // namespace bezel

#endif
