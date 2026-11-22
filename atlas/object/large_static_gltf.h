#pragma once

#include "atlas/core/workers.h"
#include "atlas/object.h"
#include <json.hpp>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace atlas::detail {

struct StaticGltfRange {
    uint32_t first;
    uint32_t count;
    glm::vec3 minimum{std::numeric_limits<float>::infinity()};
    glm::vec3 maximum{-std::numeric_limits<float>::infinity()};
};

struct StaticGltfMesh {
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
    std::vector<StaticGltfRange> ranges;
    Material material;
    std::string name;
};

struct StaticGltfStream {
    size_t offset;
    size_t count;
};

inline StaticGltfStream staticGltfStream(const nlohmann::json &asset,
                                         size_t index, const char *type,
                                         int componentType, size_t elementSize,
                                         size_t bufferSize) {
    const auto &accessor = asset.at("accessors").at(index);
    const auto &view = asset.at("bufferViews").at(accessor.at("bufferView").get<size_t>());
    const size_t start = view.value("byteOffset", size_t{0});
    const size_t length = view.at("byteLength").get<size_t>();
    const size_t offset = accessor.value("byteOffset", size_t{0});
    const size_t count = accessor.at("count").get<size_t>();
    if (accessor.at("type") != type || accessor.at("componentType") != componentType ||
        accessor.contains("sparse") || accessor.value("normalized", false) ||
        view.at("buffer") != 0 || view.contains("extensions") ||
        view.value("byteStride", elementSize) != elementSize ||
        start > bufferSize || length > bufferSize - start ||
        offset > length || count == 0 || count > (length - offset) / elementSize ||
        count > UINT32_MAX) {
        throw std::invalid_argument("Unsupported static glTF accessor");
    }
    return {start + offset, count};
}

inline glm::vec3 staticGltfPosition(const char *positions, size_t index) {
    std::array<float, 3> p;
    std::memcpy(p.data(), positions + index * sizeof(p), sizeof(p));
    return {p[0], p[1], p[2]};
}

inline void groupStaticGltfTriangles(StaticGltfMesh &mesh, const char *positions,
                                     const char *source, size_t indexCount,
                                     const glm::vec3 &minimum, const glm::vec3 &maximum) {
    constexpr size_t cells = 8;
    constexpr size_t bucketCount = cells * cells * cells;
    constexpr size_t trianglesPerJob = 32768;
    using BucketCounts = std::array<uint32_t, bucketCount>;
    const size_t triangleCount = indexCount / 3;
    const size_t jobCount = (triangleCount + trianglesPerJob - 1) / trianglesPerJob;
    auto &workers = Workers::shared();
    auto triangleBuckets = std::make_unique_for_overwrite<uint16_t[]>(triangleCount);
    std::vector<BucketCounts> offsets(jobCount);
    const glm::vec3 scale = float(cells) / glm::max(maximum - minimum, glm::vec3(1e-6f));

    const auto readTriangle = [source](size_t index) {
        std::array<Index, 3> triangle;
        std::memcpy(triangle.data(), source + index * sizeof(triangle), sizeof(triangle));
        return triangle;
    };
    workers.parallelFor(jobCount, 1, [&](size_t job) {
        const size_t begin = job * trianglesPerJob;
        const size_t end = std::min(begin + trianglesPerJob, triangleCount);
        for (size_t i = begin; i < end; ++i) {
            const auto triangle = readTriangle(i);
            if (std::max({triangle[0], triangle[1], triangle[2]}) >= mesh.vertices.size())
                throw std::invalid_argument("Index outside static glTF primitive");
            const auto centroid =
                staticGltfPosition(positions, triangle[0]) * (1.0f / 3.0f) +
                staticGltfPosition(positions, triangle[1]) * (1.0f / 3.0f) +
                staticGltfPosition(positions, triangle[2]) * (1.0f / 3.0f);
            const glm::uvec3 cell(glm::clamp((centroid - minimum) * scale,
                                            glm::vec3(0), glm::vec3(cells - 1)));
            const size_t bucket = (cell.z * cells + cell.y) * cells + cell.x;
            triangleBuckets[i] = static_cast<uint16_t>(bucket);
            ++offsets[job][bucket];
        }
    });

    uint32_t cursor = 0;
    for (size_t bucket = 0; bucket < bucketCount; ++bucket) {
        const uint32_t start = cursor;
        for (auto &job : offsets) {
            const auto count = job[bucket];
            job[bucket] = cursor;
            cursor += count * 3;
        }
        if (cursor != start)
            mesh.ranges.push_back({start, cursor - start});
    }

    mesh.indices.resize(indexCount);
    workers.parallelFor(jobCount, 1, [&](size_t job) {
        const size_t begin = job * trianglesPerJob;
        const size_t end = std::min(begin + trianglesPerJob, triangleCount);
        auto &next = offsets[job];
        for (size_t i = begin; i < end; ++i) {
            const uint16_t bucket = triangleBuckets[i];
            const auto triangle = readTriangle(i);
            std::memcpy(mesh.indices.data() + next[bucket], triangle.data(), sizeof(triangle));
            next[bucket] += 3;
        }
    });
    workers.parallelFor(mesh.ranges.size(), 1, [&](size_t i) {
        auto &range = mesh.ranges[i];
        for (size_t index = range.first; index < size_t(range.first) + range.count; ++index) {
            const auto position = staticGltfPosition(positions, mesh.indices[index]);
            range.minimum = glm::min(range.minimum, position);
            range.maximum = glm::max(range.maximum, position);
        }
    });
}

inline std::optional<std::vector<StaticGltfMesh>>
loadLargeStaticGltf(const std::filesystem::path &path,
                    const std::function<void(float, const std::string &)> &progress) {
    if (std::endian::native != std::endian::little || path.extension() != ".gltf")
        return std::nullopt;

    using Json = nlohmann::json;
    try {
        std::ifstream metadata(path, std::ios::binary);
        const auto asset = Json::parse(metadata);
        const auto empty = Json::array();
        if (asset.at("asset").at("version") != "2.0" ||
            asset.at("buffers").size() != 1 || asset.at("meshes").size() != 1 ||
            asset.at("scenes").size() != 1 || asset.value("scene", 0) != 0 ||
            !asset.value("images", empty).empty() || !asset.value("textures", empty).empty() ||
            !asset.value("skins", empty).empty() || !asset.value("animations", empty).empty())
            return std::nullopt;
        for (const char *key : {"extensionsUsed", "extensionsRequired"}) {
            for (const auto &extension : asset.value(key, empty)) {
                if (extension != "KHR_lights_punctual")
                    return std::nullopt;
            }
        }

        const auto &buffer = asset.at("buffers").front();
        const size_t size = buffer.at("byteLength").get<size_t>();
        const std::string uri = buffer.at("uri").get<std::string>();
        const auto name = std::filesystem::path(std::u8string(uri.begin(), uri.end()));
        if (size < 128u * 1024 * 1024 || name.empty() || name != name.filename() ||
            name == "." || name == ".." || uri.find_first_of("%:#?\\") != std::string::npos)
            return std::nullopt;
        const auto binary = path.parent_path() / name;
        std::error_code error;
        if (std::filesystem::file_size(binary, error) != size || error)
            return std::nullopt;

        const auto &nodes = asset.at("nodes");
        const auto &roots = asset.at("scenes").front().at("nodes");
        size_t meshNodes = 0;
        std::string meshName = "Mesh";
        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto &node = nodes[i];
            if (!node.contains("mesh"))
                continue;
            if (node.at("mesh") != 0 || node.contains("matrix") ||
                node.contains("translation") || node.contains("rotation") ||
                node.contains("scale") || node.contains("skin") || node.contains("weights") ||
                std::find(roots.begin(), roots.end(), i) == roots.end())
                return std::nullopt;
            meshName = node.value("name", meshName);
            ++meshNodes;
        }
        if (meshNodes != 1)
            return std::nullopt;

        struct Primitive {
            StaticGltfStream positions, normals, uvs, indices;
            glm::vec3 minimum, maximum;
            Material material;
        };
        std::vector<Primitive> primitives;
        for (const auto &primitive : asset.at("meshes").front().at("primitives")) {
            const auto &attrs = primitive.at("attributes");
            if (primitive.value("mode", 4) != 4 || primitive.contains("targets") ||
                primitive.contains("extensions") || attrs.size() != 3)
                return std::nullopt;
            const size_t pos = attrs.at("POSITION").get<size_t>();
            const auto &accessor = asset.at("accessors").at(pos);
            const auto low = accessor.at("min").get<std::array<float, 3>>();
            const auto high = accessor.at("max").get<std::array<float, 3>>();
            Primitive layout{
                staticGltfStream(asset, pos, "VEC3", 5126, 12, size),
                staticGltfStream(asset, attrs.at("NORMAL").get<size_t>(), "VEC3", 5126, 12, size),
                staticGltfStream(asset, attrs.at("TEXCOORD_0").get<size_t>(), "VEC2", 5126, 8, size),
                staticGltfStream(asset, primitive.at("indices").get<size_t>(), "SCALAR", 5125, 4, size),
                {low[0], low[1], low[2]}, {high[0], high[1], high[2]}, {}};
            if (layout.normals.count != layout.positions.count ||
                layout.uvs.count != layout.positions.count || layout.indices.count % 3 != 0)
                return std::nullopt;
            for (size_t axis = 0; axis < 3; ++axis) {
                if (!std::isfinite(low[axis]) || !std::isfinite(high[axis] - low[axis]) ||
                    low[axis] > high[axis])
                    return std::nullopt;
            }
            const auto &material = asset.at("materials").at(primitive.at("material").get<size_t>());
            if (material.value("alphaMode", std::string("OPAQUE")) != "OPAQUE" ||
                material.contains("extensions") || material.contains("normalTexture") ||
                material.contains("occlusionTexture") || material.contains("emissiveTexture"))
                return std::nullopt;
            const auto pbr = material.value("pbrMetallicRoughness", Json::object());
            if (pbr.contains("baseColorTexture") || pbr.contains("metallicRoughnessTexture"))
                return std::nullopt;
            const auto color = pbr.value("baseColorFactor", std::array<float, 4>{1, 1, 1, 1});
            if (color[3] != 1.0f)
                return std::nullopt;
            layout.material.albedo = {color[0], color[1], color[2], color[3]};
            layout.material.metallic = pbr.value("metallicFactor", 1.0f);
            layout.material.roughness = pbr.value("roughnessFactor", 1.0f);
            const auto emission = material.value("emissiveFactor", std::array<float, 3>{});
            layout.material.emissiveColor = {emission[0], emission[1], emission[2], 1.0f};
            if (std::max({emission[0], emission[1], emission[2]}) > 0.0f)
                layout.material.emissiveIntensity = 1.0f;
            primitives.push_back(layout);
        }
        if (primitives.empty())
            return std::nullopt;

        if (progress)
            progress(0.05f, "Reading large static glTF");
        auto bytes = std::make_unique_for_overwrite<char[]>(size);
        auto &workers = Workers::shared();
        constexpr size_t readSize = 32u * 1024 * 1024;
        workers.parallelFor((size + readSize - 1) / readSize, 1, [&](size_t chunk) {
            const size_t offset = chunk * readSize;
            std::ifstream stream(binary, std::ios::binary);
            stream.seekg(static_cast<std::streamoff>(offset));
            if (!stream.read(bytes.get() + offset, std::min(readSize, size - offset)))
                throw std::invalid_argument("Unable to read static glTF buffer");
        });

        std::vector<StaticGltfMesh> meshes;
        for (const auto &layout : primitives) {
            if (progress)
                progress(0.25f + 0.7f * static_cast<float>(meshes.size()) / primitives.size(),
                         "Preparing large static glTF geometry");
            StaticGltfMesh mesh;
            mesh.name = primitives.size() == 1 ? meshName
                : meshName + "-" + std::to_string(meshes.size());
            mesh.material = layout.material;
            mesh.vertices.resize(layout.positions.count);
            const auto *positions = bytes.get() + layout.positions.offset;
            const auto *normals = bytes.get() + layout.normals.offset;
            const auto *uvs = bytes.get() + layout.uvs.offset;
            workers.parallelFor(layout.positions.count, 16384, [&](size_t vertex) {
                const auto p = staticGltfPosition(positions, vertex);
                if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                    throw std::invalid_argument("Non-finite static glTF position");
                auto n = staticGltfPosition(normals, vertex);
                const float magnitude = glm::dot(n, n);
                n = std::isfinite(magnitude) && magnitude > 1e-12f
                        ? n / std::sqrt(magnitude) : glm::vec3(0, 1, 0);
                const auto tangent = glm::normalize(glm::cross(
                    std::abs(n.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), n));
                const auto bitangent = glm::cross(n, tangent);
                std::array<float, 2> uv;
                std::memcpy(uv.data(), uvs + vertex * sizeof(uv), sizeof(uv));
                mesh.vertices[vertex] = CoreVertex(
                    {p.x, p.y, p.z}, {1, 1, 1, 1}, {uv[0], uv[1]},
                    {n.x, n.y, n.z}, {tangent.x, tangent.y, tangent.z},
                    {bitangent.x, bitangent.y, bitangent.z});
            });
            groupStaticGltfTriangles(mesh, positions, bytes.get() + layout.indices.offset,
                                     layout.indices.count, layout.minimum, layout.maximum);
            meshes.push_back(std::move(mesh));
        }
        return meshes;
    } catch (const Json::exception &) {
        return std::nullopt;
    } catch (const std::invalid_argument &) {
        return std::nullopt;
    }
}

}
