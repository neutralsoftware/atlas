//
// model.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Model object implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <any>
#include <assimp/material.h>
#include <assimp/scene.h>
#include "assimp/matrix4x4.h"
#include "atlas/animation.h"
#include "atlas/core/workers.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultIOSystem.h>
#include <assimp/version.h>
#include <json.hpp>
#include <assimp/GltfMaterial.h>
#include <assimp/ProgressHandler.hpp>
#include <assimp/postprocess.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "stb/stb_image.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/epsilon.hpp>

namespace {
class ModelImportProgressHandler : public Assimp::ProgressHandler {
  public:
    explicit ModelImportProgressHandler(
        std::function<void(float, const std::string &)> callback)
        : callback(std::move(callback)) {}

    bool Update(float percentage = -1.0f) override {
        if (callback && std::isfinite(percentage) && percentage >= 0.0f)
            callback(std::clamp(percentage, 0.0f, 1.0f) * 0.85f,
                     "Reading and optimizing model");
        return true;
    }

  private:
    std::function<void(float, const std::string &)> callback;
};

class ModelImportIO : public Assimp::IOSystem {
  public:
    bool Exists(const char *path) const override { return files.Exists(path); }
    char getOsSeparator() const override { return files.getOsSeparator(); }
    Assimp::IOStream *Open(const char *path, const char *mode = "rb") override {
        auto *stream = files.Open(path, mode);
        if (stream != nullptr && mode[0] == 'r') {
            std::error_code error;
            auto absolute = std::filesystem::absolute(path, error);
            if (!error)
                dependencies.insert(absolute.lexically_normal().string());
        }
        return stream;
    }
    void Close(Assimp::IOStream *stream) override { files.Close(stream); }
    bool ComparePaths(const char *one, const char *two) const override {
        return files.ComparePaths(one, two);
    }
    std::unordered_set<std::string> dependencies;

  private:
    Assimp::DefaultIOSystem files;
};

nlohmann::json modelFileStamp(const std::string &path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error)
        return {};
    const auto time = std::filesystem::last_write_time(path, error);
    if (error)
        return {};
    return {{"path", path},
            {"size", size},
            {"time", time.time_since_epoch().count()}};
}

std::filesystem::path modelSceneCachePath(const Resource &resource,
                                          unsigned int flags) {
    std::error_code error;
    auto directory = std::filesystem::temp_directory_path(error);
    if (error)
        return {};
    const std::string key = std::filesystem::absolute(resource.path, error)
                                .lexically_normal()
                                .string() +
                            "|" + std::to_string(flags) + "|" +
                            std::to_string(aiGetVersionMajor()) + "." +
                            std::to_string(aiGetVersionMinor()) + "." +
                            std::to_string(aiGetVersionPatch());
    if (error)
        return {};
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char value : key) {
        hash ^= value;
        hash *= 1099511628211ULL;
    }
    return directory / "atlas-model-scenes-v1" /
           (std::to_string(hash) + ".assbin");
}

bool isModelSceneCacheValid(const std::filesystem::path &path) {
    if (path.empty())
        return false;
    try {
        std::ifstream input(path.string() + ".json");
        if (!input)
            return false;
        const auto metadata = nlohmann::json::parse(input);
        if (metadata.value("version", 0) != 1 ||
            metadata.at("scene") != modelFileStamp(path.string()))
            return false;
        const auto &dependencies = metadata.at("dependencies");
        if (!dependencies.is_array() || dependencies.empty())
            return false;
        for (const auto &dependency : dependencies) {
            if (dependency !=
                modelFileStamp(dependency.at("path").get<std::string>()))
                return false;
        }
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

void storeModelSceneCache(const aiScene *scene,
                          const std::filesystem::path &path,
                          const ModelImportIO &io) {
    if (path.empty() || io.dependencies.empty())
        return;
    std::filesystem::path temporary;
    try {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error)
            return;
        const auto nonce =
            std::chrono::steady_clock::now().time_since_epoch().count();
        temporary = path.string() + "." + std::to_string(nonce) + ".tmp";
        Assimp::Exporter exporter;
        if (exporter.Export(scene, "assbin", temporary.string()) !=
            AI_SUCCESS) {
            std::filesystem::remove(temporary, error);
            return;
        }
        nlohmann::json dependencies = nlohmann::json::array();
        for (const auto &dependency : io.dependencies) {
            auto stamp = modelFileStamp(dependency);
            if (stamp.is_null()) {
                std::filesystem::remove(temporary, error);
                return;
            }
            dependencies.push_back(std::move(stamp));
        }
        std::filesystem::rename(temporary, path, error);
        if (error) {
            std::filesystem::remove(path, error);
            if (!error)
                std::filesystem::rename(temporary, path, error);
        }
        if (error) {
            std::filesystem::remove(temporary, error);
            return;
        }
        nlohmann::json metadata{{"version", 1},
                                {"scene", modelFileStamp(path.string())},
                                {"dependencies", std::move(dependencies)}};
        std::ofstream output(path.string() + ".json", std::ios::trunc);
        output << metadata.dump();
    } catch (const std::exception &) {
        std::error_code error;
        if (!temporary.empty())
            std::filesystem::remove(temporary, error);
    }
}

glm::mat4 assimpToGlmMatrix(const aiMatrix4x4 &matrix) {
    return glm::transpose(glm::make_mat4(&matrix.a1));
}

float saturate(float value) { return std::clamp(value, 0.0f, 1.0f); }

float luminance(const aiColor3D &color) {
    return (color.r * 0.2126f) + (color.g * 0.7152f) + (color.b * 0.0722f);
}

bool hasVisibleColor(const aiColor3D &color) {
    return color.r > 1e-4f || color.g > 1e-4f || color.b > 1e-4f;
}

float roughnessFromShininess(float shininess, float strength) {
    const float effectiveShininess =
        std::max(0.0f, shininess) * std::max(0.0f, strength);
    return saturate(std::sqrt(2.0f / (effectiveShininess + 2.0f)));
}

struct GltfTextureCoordinates {
    unsigned int channel = 0;
    glm::vec2 scale{1.0f};
    glm::vec2 offset{0.0f};
    float cosine = 1.0f;
    float sine = 0.0f;

    glm::vec2 at(const aiMesh &mesh, unsigned int vertex) const {
        if (channel >= AI_MAX_NUMBER_OF_TEXTURECOORDS ||
            mesh.mTextureCoords[channel] == nullptr)
            return offset;

        const auto &source = mesh.mTextureCoords[channel][vertex];
        if (!std::isfinite(source.x) || !std::isfinite(source.y))
            return offset;

        const glm::vec2 uv = glm::vec2(source.x, source.y) * scale;
        return offset + glm::vec2(cosine * uv.x - sine * uv.y,
                                  sine * uv.x + cosine * uv.y);
    }
};

void updateModelTangents(std::vector<CoreVertex> &vertices,
                         const std::vector<unsigned int> &indices) {
    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0f));
    const auto normalUv = [](const CoreVertex &vertex) {
        return glm::vec2(vertex.textureCoordinate[0], vertex.textureCoordinate[1]) +
               glm::vec2(vertex.normalAndPbrUvOffset);
    };

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const unsigned int a = indices[i];
        const unsigned int b = indices[i + 1];
        const unsigned int c = indices[i + 2];
        const glm::vec3 edgeU =
            vertices[b].position.toGlm() - vertices[a].position.toGlm();
        const glm::vec3 edgeV =
            vertices[c].position.toGlm() - vertices[a].position.toGlm();
        const glm::vec2 uvU = normalUv(vertices[b]) - normalUv(vertices[a]);
        const glm::vec2 uvV = normalUv(vertices[c]) - normalUv(vertices[a]);
        const float determinant = uvU.x * uvV.y - uvU.y * uvV.x;
        if (std::abs(determinant) <= 1e-8f)
            continue;

        const glm::vec3 tangent = (edgeU * uvV.y - edgeV * uvU.y) / determinant;
        const glm::vec3 bitangent = (edgeV * uvU.x - edgeU * uvV.x) / determinant;
        for (unsigned int index : {a, b, c}) {
            tangents[index] += tangent;
            bitangents[index] += bitangent;
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        auto &vertex = vertices[i];
        const glm::vec3 normal = vertex.normal.toGlm();
        const glm::vec3 tangent =
            tangents[i] - normal * glm::dot(normal, tangents[i]);
        if (!(glm::dot(tangent, tangent) > 1e-12f))
            continue;

        const glm::vec3 unitTangent = glm::normalize(tangent);
        const glm::vec3 bitangent = glm::cross(normal, unitTangent);
        const float sign =
            glm::dot(bitangent, bitangents[i]) < 0.0f ? -1.0f : 1.0f;
        vertex.tangent = Normal3d::fromGlm(unitTangent);
        vertex.bitangent = Normal3d::fromGlm(bitangent * sign);
    }
}

GltfTextureCoordinates gltfTextureCoordinates(
    const aiMaterial &material, aiTextureType textureType) {
    GltfTextureCoordinates coordinates;
    material.Get(AI_MATKEY_UVWSRC(textureType, 0), coordinates.channel);
    aiUVTransform transform;
    if (material.Get(AI_MATKEY_UVTRANSFORM(textureType, 0), transform) ==
        AI_SUCCESS) {
        coordinates.scale = {transform.mScaling.x, transform.mScaling.y};
        coordinates.cosine = std::cos(transform.mRotation);
        coordinates.sine = std::sin(transform.mRotation);
        const float cosine = coordinates.cosine;
        const float sine = coordinates.sine;
        coordinates.offset = {
            transform.mTranslation.x -
                0.5f * coordinates.scale.x * (1.0f - cosine + sine),
            transform.mTranslation.y + 1.0f - coordinates.scale.y +
                0.5f * coordinates.scale.y * (sine + cosine - 1.0f)};
    }
    return coordinates;
}

struct ModelTextureJob {
    std::string cacheKey;
    std::string fullPath;
    std::string filename;
    std::string aoPath;
    TextureType textureType = TextureType::Color;
    ResourceType resourceType = ResourceType::Image;
    int maximumDimension = 0;
    const aiTexture *source = nullptr;
    const aiTexture *ambientOcclusion = nullptr;
};

struct DecodedModelTexture {
    std::vector<unsigned char> pixels;
    std::vector<unsigned char> ao;
    int width = 0;
    int height = 0;
};

struct DecodedTextureCacheHeader {
    uint64_t magic = 0x41544C4153544558ULL;
    uint32_t version = 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t pixelBytes = 0;
    uint64_t aoBytes = 0;
};

uint64_t modelTextureCacheKey(const ModelTextureJob &job) {
    uint64_t hash = 1469598103934665603ULL;
    auto append = [&hash](const void *data, size_t size) {
        const auto *bytes = static_cast<const unsigned char *>(data);
        for (size_t index = 0; index < size; ++index) {
            hash ^= bytes[index];
            hash *= 1099511628211ULL;
        }
    };
    auto appendPath = [&](const std::string &path) {
        append(path.data(), path.size());
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (!error) {
            append(&size, sizeof(size));
        }
        const auto timestamp = std::filesystem::last_write_time(path, error);
        if (!error) {
            const auto count = timestamp.time_since_epoch().count();
            append(&count, sizeof(count));
        }
    };
    appendPath(job.fullPath);
    appendPath(job.aoPath);
    for (const auto *texture : {job.source, job.ambientOcclusion}) {
        if (texture == nullptr || texture->pcData == nullptr)
            continue;
        if (texture->mHeight == 0) {
            append(texture->pcData, texture->mWidth);
        } else {
            append(&texture->mWidth, sizeof(texture->mWidth));
            append(&texture->mHeight, sizeof(texture->mHeight));
            append(texture->pcData, static_cast<size_t>(texture->mWidth) *
                                        texture->mHeight * sizeof(aiTexel));
        }
    }
    append(&job.maximumDimension, sizeof(job.maximumDimension));
    append(&job.textureType, sizeof(job.textureType));
    return hash;
}

std::filesystem::path modelTextureCachePath(const ModelTextureJob &job) {
    std::error_code error;
    const auto temporaryDirectory = std::filesystem::temp_directory_path(error);
    if (error) {
        return {};
    }
    return temporaryDirectory / "atlas-model-cache-v1" /
           (std::to_string(modelTextureCacheKey(job)) + ".rgba");
}

std::optional<DecodedModelTexture>
loadDecodedTextureCache(const ModelTextureJob &job,
                        const std::filesystem::path &path) {
    if (path.empty()) {
        return std::nullopt;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    DecodedTextureCacheHeader header;
    input.read(reinterpret_cast<char *>(&header), sizeof(header));
    const uint64_t expectedPixels = static_cast<uint64_t>(header.width) *
                                    static_cast<uint64_t>(header.height) * 4;
    const uint64_t expectedAo = static_cast<uint64_t>(header.width) *
                                static_cast<uint64_t>(header.height);
    std::error_code cacheError;
    const auto cacheBytes = std::filesystem::file_size(path, cacheError);
    if (cacheError || cacheBytes < sizeof(header) ||
        header.pixelBytes > cacheBytes - sizeof(header) ||
        header.aoBytes != cacheBytes - sizeof(header) - header.pixelBytes)
        return std::nullopt;
    if (!input || header.magic != 0x41544C4153544558ULL ||
        header.version != 1 || header.width == 0 || header.height == 0 ||
        header.width > static_cast<uint32_t>(job.maximumDimension > 0
                                                 ? job.maximumDimension
                                                 : 16384) ||
        header.height > static_cast<uint32_t>(job.maximumDimension > 0
                                                  ? job.maximumDimension
                                                  : 16384) ||
        header.pixelBytes != expectedPixels ||
        (header.aoBytes != 0 && header.aoBytes != expectedAo)) {
        return std::nullopt;
    }
    DecodedModelTexture decoded;
    decoded.width = static_cast<int>(header.width);
    decoded.height = static_cast<int>(header.height);
    decoded.pixels.resize(header.pixelBytes);
    decoded.ao.resize(header.aoBytes);
    input.read(reinterpret_cast<char *>(decoded.pixels.data()),
               static_cast<std::streamsize>(decoded.pixels.size()));
    if (!decoded.ao.empty()) {
        input.read(reinterpret_cast<char *>(decoded.ao.data()),
                   static_cast<std::streamsize>(decoded.ao.size()));
    }
    if (!input) {
        return std::nullopt;
    }
    return decoded;
}

void storeDecodedTextureCache(const std::filesystem::path &path,
                              const DecodedModelTexture &decoded) {
    if (decoded.pixels.empty() || path.empty()) {
        return;
    }
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return;
    }
    static std::atomic_uint64_t nextTemporaryId{0};
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const auto id = nextTemporaryId.fetch_add(1, std::memory_order_relaxed);
    auto temporary = path;
    temporary += "." + std::to_string(stamp) + "." + std::to_string(id) + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        return;
    }
    DecodedTextureCacheHeader header;
    header.width = static_cast<uint32_t>(decoded.width);
    header.height = static_cast<uint32_t>(decoded.height);
    header.pixelBytes = decoded.pixels.size();
    header.aoBytes = decoded.ao.size();
    output.write(reinterpret_cast<const char *>(&header), sizeof(header));
    output.write(reinterpret_cast<const char *>(decoded.pixels.data()),
                 static_cast<std::streamsize>(decoded.pixels.size()));
    if (!decoded.ao.empty()) {
        output.write(reinterpret_cast<const char *>(decoded.ao.data()),
                     static_cast<std::streamsize>(decoded.ao.size()));
    }
    output.close();
    if (output)
        std::filesystem::rename(temporary, path, error);
    if (!output || error)
        std::filesystem::remove(temporary, error);
}

std::vector<unsigned char> resizeModelTexture(const unsigned char *source,
                                              int sourceWidth, int sourceHeight,
                                              int channels, int targetWidth,
                                              int targetHeight) {
    std::vector<unsigned char> resized(static_cast<size_t>(targetWidth) *
                                       targetHeight * channels);
    const bool halfResolution =
        sourceWidth == targetWidth * 2 && sourceHeight == targetHeight * 2;
    for (int y = 0; y < targetHeight; ++y) {
        const int sourceY =
            std::min(sourceHeight - 1,
                     static_cast<int>((static_cast<int64_t>(y) * sourceHeight) /
                                      targetHeight));
        for (int x = 0; x < targetWidth; ++x) {
            const int sourceX = std::min(
                sourceWidth - 1,
                static_cast<int>((static_cast<int64_t>(x) * sourceWidth) /
                                 targetWidth));
            const size_t sourceOffset =
                (static_cast<size_t>(sourceY) * sourceWidth + sourceX) *
                channels;
            const size_t targetOffset =
                (static_cast<size_t>(y) * targetWidth + x) * channels;
            if (halfResolution) {
                const size_t rightOffset = sourceOffset + channels;
                const size_t lowerOffset =
                    sourceOffset + static_cast<size_t>(sourceWidth) * channels;
                const size_t lowerRightOffset = lowerOffset + channels;
                for (int channel = 0; channel < channels; ++channel) {
                    const unsigned int total =
                        source[sourceOffset + channel] +
                        source[rightOffset + channel] +
                        source[lowerOffset + channel] +
                        source[lowerRightOffset + channel];
                    resized[targetOffset + channel] =
                        static_cast<unsigned char>((total + 2) / 4);
                }
            } else {
                std::memcpy(resized.data() + targetOffset,
                            source + sourceOffset,
                            static_cast<size_t>(channels));
            }
        }
    }
    return resized;
}

DecodedModelTexture decodeModelTexture(const ModelTextureJob &job) {
    const auto cachePath = modelTextureCachePath(job);
    if (auto cached = loadDecodedTextureCache(job, cachePath)) {
        return std::move(*cached);
    }
    stbi_set_flip_vertically_on_load_thread(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> source(
        nullptr, stbi_image_free);
    const unsigned char *sourcePixels = nullptr;
    std::vector<unsigned char> rawSource;
    if (job.source != nullptr && job.source->mHeight > 0) {
        width = static_cast<int>(job.source->mWidth);
        height = static_cast<int>(job.source->mHeight);
        rawSource.resize(static_cast<size_t>(width) * height * 4);
        for (size_t i = 0; i < rawSource.size() / 4; ++i) {
            const auto &pixel = job.source->pcData[i];
            rawSource[i * 4 + 0] = pixel.r;
            rawSource[i * 4 + 1] = pixel.g;
            rawSource[i * 4 + 2] = pixel.b;
            rawSource[i * 4 + 3] = pixel.a;
        }
        sourcePixels = rawSource.data();
    } else {
        source.reset(job.source == nullptr
                         ? stbi_load(job.fullPath.c_str(), &width, &height,
                                     &channels, STBI_rgb_alpha)
                         : stbi_load_from_memory(
                               reinterpret_cast<const unsigned char *>(job.source->pcData),
                               static_cast<int>(job.source->mWidth), &width,
                               &height, &channels, STBI_rgb_alpha));
        sourcePixels = source.get();
    }
    if (sourcePixels == nullptr || width <= 0 || height <= 0) {
        return {};
    }

    const float scale =
        job.maximumDimension > 0
            ? std::min(1.0f, static_cast<float>(job.maximumDimension) /
                                 static_cast<float>(std::max(width, height)))
            : 1.0f;
    const int targetWidth = std::max(1, static_cast<int>(width * scale));
    const int targetHeight = std::max(1, static_cast<int>(height * scale));

    DecodedModelTexture decoded;
    decoded.width = targetWidth;
    decoded.height = targetHeight;
    if (targetWidth == width && targetHeight == height) {
        const size_t byteCount = static_cast<size_t>(width) * height * 4;
        decoded.pixels.assign(sourcePixels, sourcePixels + byteCount);
    } else {
        decoded.pixels = resizeModelTexture(sourcePixels, width, height, 4,
                                            targetWidth, targetHeight);
    }

    if (!job.aoPath.empty()) {
        int aoWidth = 0;
        int aoHeight = 0;
        int aoChannels = 0;
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> aoSource(
            nullptr, stbi_image_free);
        const unsigned char *aoPixels = nullptr;
        std::vector<unsigned char> rawAo;
        const auto *embedded = job.ambientOcclusion;
        if (embedded != nullptr && embedded->mHeight > 0) {
            aoWidth = static_cast<int>(embedded->mWidth);
            aoHeight = static_cast<int>(embedded->mHeight);
            rawAo.resize(static_cast<size_t>(aoWidth) * aoHeight);
            for (size_t i = 0; i < rawAo.size(); ++i)
                rawAo[i] = embedded->pcData[i].b;
            aoPixels = rawAo.data();
        } else {
            aoSource.reset(
                embedded == nullptr
                    ? stbi_load(job.aoPath.c_str(), &aoWidth, &aoHeight,
                                &aoChannels, STBI_grey)
                    : stbi_load_from_memory(
                          reinterpret_cast<const unsigned char *>(embedded->pcData),
                          static_cast<int>(embedded->mWidth), &aoWidth, &aoHeight,
                          &aoChannels, STBI_grey));
            aoPixels = aoSource.get();
        }
        if (aoPixels != nullptr && aoWidth > 0 && aoHeight > 0) {
            if (aoWidth == targetWidth && aoHeight == targetHeight) {
                const size_t byteCount =
                    static_cast<size_t>(aoWidth) * aoHeight;
                decoded.ao.assign(aoPixels, aoPixels + byteCount);
            } else {
                decoded.ao = resizeModelTexture(aoPixels, aoWidth, aoHeight, 1,
                                                targetWidth, targetHeight);
            }
        }
    }
    storeDecodedTextureCache(cachePath, decoded);
    return decoded;
}

Texture uploadModelTexture(const ModelTextureJob &job,
                           DecodedModelTexture decoded) {
    if (decoded.pixels.empty() || decoded.width <= 0 || decoded.height <= 0) {
        throw std::runtime_error("Failed to decode model texture");
    }
    if (job.textureType == TextureType::PBRPack) {
        const size_t pixelCount =
            static_cast<size_t>(decoded.width) * decoded.height;
        for (size_t pixel = 0; pixel < pixelCount; ++pixel) {
            decoded.pixels[pixel * 4] =
                decoded.ao.empty() ? 255 : decoded.ao[pixel];
        }
    }

    Resource resource = Workspace::get().createResource(
        job.fullPath, job.filename, job.resourceType);
    const opal::TextureFormat format = job.textureType == TextureType::Color
                                           ? opal::TextureFormat::sRgba8
                                           : opal::TextureFormat::Rgba8;
    const unsigned int mipLevels =
        1u + static_cast<unsigned int>(std::floor(
                 std::log2(std::max(decoded.width, decoded.height))));
    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, format, decoded.width, decoded.height,
        opal::TextureDataFormat::Rgba, decoded.pixels.data(), mipLevels);
    opalTexture->setParameters(opal::TextureWrapMode::Repeat,
                               opal::TextureWrapMode::Repeat,
                               opal::TextureFilterMode::LinearMipmapLinear,
                               opal::TextureFilterMode::Linear);
    opalTexture->automaticallyGenerateMipmaps();
    return Texture{.resource = resource,
                   .creationData = {decoded.width, decoded.height, 4},
                   .id = opalTexture->textureID,
                   .texture = opalTexture,
                   .type = job.textureType};
}

void importMaterialProperties(aiMaterial *material, CoreObject &object) {
    aiColor4D baseColor;
    if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
        object.material.albedo = {baseColor.r, baseColor.g, baseColor.b,
                                  baseColor.a};
    } else {
        aiColor3D diffuseColor;
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) ==
            AI_SUCCESS) {
            object.material.albedo.r = diffuseColor.r;
            object.material.albedo.g = diffuseColor.g;
            object.material.albedo.b = diffuseColor.b;
        }
    }
    float opacity = 1.0f;
    if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        opacity = saturate(opacity);
        if (std::abs(object.material.albedo.a - opacity) > 1e-5f) {
            object.material.albedo.a =
                saturate(object.material.albedo.a * opacity);
        }
    } else {
        float transparency = 0.0f;
        if (material->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency) ==
            AI_SUCCESS) {
            object.material.albedo.a =
                saturate(object.material.albedo.a * (1.0f - transparency));
        }
    }

    float metallic = 0.0f;
    if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        object.material.metallic = saturate(metallic);
    }

    float roughness = 0.0f;
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        object.material.roughness = saturate(roughness);
    } else {
        float glossiness = 0.0f;
        if (material->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness) ==
            AI_SUCCESS) {
            object.material.roughness = saturate(1.0f - glossiness);
        } else {
            float shininess = 0.0f;
            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                float shininessStrength = 1.0f;
                material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
                object.material.roughness =
                    roughnessFromShininess(shininess, shininessStrength);
            }
        }
    }

    aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
    const bool hasEmissiveColor =
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS;
    if (hasEmissiveColor) {
        object.material.emissiveColor = {emissiveColor.r, emissiveColor.g,
                                         emissiveColor.b, 1.0f};
    }

    float emissiveIntensity = 0.0f;
    if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) ==
        AI_SUCCESS) {
        object.material.emissiveIntensity = std::max(0.0f, emissiveIntensity);
    } else if (hasEmissiveColor && hasVisibleColor(emissiveColor)) {
        object.material.emissiveIntensity = 1.0f;
    }

    float bumpScaling = 1.0f;
    if (material->Get(AI_MATKEY_BUMPSCALING, bumpScaling) == AI_SUCCESS) {
        object.material.normalMapStrength = std::max(0.0f, bumpScaling);
    }

    float reflectivity = 0.0f;
    if (material->Get(AI_MATKEY_REFLECTIVITY, reflectivity) == AI_SUCCESS) {
        object.material.reflectivity = saturate(reflectivity);
    } else {
        aiColor3D specularColor(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) ==
            AI_SUCCESS) {
            object.material.reflectivity = saturate(luminance(specularColor));
        }
    }
}

glm::mat4 convertMatrix(const aiMatrix4x4 &m) {
    glm::mat4 result{1.0f};

    result[0] = glm::vec4(m.a1, m.b1, m.c1, m.d1);
    result[1] = glm::vec4(m.a2, m.b2, m.c2, m.d2);
    result[2] = glm::vec4(m.a3, m.b3, m.c3, m.d3);
    result[3] = glm::vec4(m.a4, m.b4, m.c4, m.d4);

    return result;
}

struct SkeletonImportResult {
    Skeleton skeleton;

    std::unordered_map<std::string, atlas::animation::NodeIndex> nodeByName;
};

void importAnimationNode(const aiNode *source,
                         atlas::animation::NodeIndex parent,
                         SkeletonImportResult &result) {
    atlas::animation::NodeIndex index =
        static_cast<atlas::animation::NodeIndex>(result.skeleton.nodes.size());

    result.skeleton.nodes.push_back({
        .name = source->mName.C_Str(),
        .parent = parent,
        .localTransform = convertMatrix(source->mTransformation),
    });
    result.nodeByName.emplace(source->mName.C_Str(), index);

    for (unsigned int i = 0; i < source->mNumChildren; i++) {
        importAnimationNode(source->mChildren[i], index, result);
    }
}

SkeletonImportResult importSkeleton(const aiScene *scene) {
    SkeletonImportResult result;

    if (scene->mRootNode) {
        importAnimationNode(scene->mRootNode,
                            atlas::animation::INVALID_NODE_INDEX, result);
    }
    return result;
}

AnimationClip importClip(const aiAnimation *source,
                              const std::unordered_map<std::string, atlas::animation::NodeIndex> &nodes,
                              float fallbackTicksPerSecond = 1.0f) {
    AnimationClip clip;
    clip.name = source->mName.C_Str();

    const double ticksPerSecond = source->mTicksPerSecond > 0.0
                                      ? source->mTicksPerSecond
                                      : fallbackTicksPerSecond;

    if (ticksPerSecond <= 0.0) {
        atlas_error("Invalid ticks per second for animation: " + clip.name);
        return clip;
    }

    const double secondsPerTick = 1.0 / ticksPerSecond;

    clip.duration = static_cast<float>(source->mDuration * secondsPerTick);

    for (unsigned int i = 0; i < source->mNumChannels; i++) {
        const aiNodeAnim *src = source->mChannels[i];

        auto it = nodes.find(src->mNodeName.C_Str());
        if (it == nodes.end()) {
            atlas_warning("Animation channel for unknown node: " +
                          std::string(src->mNodeName.C_Str()));
            continue;
        }

        AnimationChannel channel;
        channel.targetNode = it->second;

        for (unsigned int j = 0; j < src->mNumPositionKeys; j++) {
            const auto &key = src->mPositionKeys[j];

            channel.positionKeyframes.push_back({
                .time = static_cast<float>(key.mTime * secondsPerTick),
                .value = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z),
                .step = key.mInterpolation == aiAnimInterpolation_Step,
            });
        }

        for (unsigned int j = 0; j < src->mNumRotationKeys; j++) {
            const auto &key = src->mRotationKeys[j];

            channel.rotationKeyframes.push_back({
                .time = static_cast<float>(key.mTime * secondsPerTick),
                .value = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y,
                                   key.mValue.z),
                .step = key.mInterpolation == aiAnimInterpolation_Step,
            });
        }

        for (unsigned int j = 0; j < src->mNumScalingKeys; j++) {
            const auto &key = src->mScalingKeys[j];

            channel.scaleKeyframes.push_back({
                .time = static_cast<float>(key.mTime * secondsPerTick),
                .value = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z),
                .step = key.mInterpolation == aiAnimInterpolation_Step,
            });
        }

        clip.channels.push_back(std::move(channel));
    }

    return clip;
}

struct ImportedSkin {
    Skin skin;
    std::vector<VertexSkinData> vertices;
};

ImportedSkin importSkin(const aiMesh *mesh,
                        const std::unordered_map<std::string, atlas::animation::NodeIndex> &nodes) {
    ImportedSkin result;
    result.vertices.resize(mesh->mNumVertices);

    using Influence = std::pair<uint32_t, float>;
    std::vector<std::vector<Influence>> influences(mesh->mNumVertices);

    for (unsigned b = 0; b < mesh->mNumBones; b++) {
        const aiBone *bone = mesh->mBones[b];

        auto it = nodes.find(bone->mName.C_Str());

        if (it == nodes.end()) {
            atlas_warning("Bone for unknown node: " +
                          std::string(bone->mName.C_Str()));
            continue;
        }

        uint32_t jointIndex = static_cast<uint32_t>(result.skin.joints.size());

        result.skin.joints.push_back({
            .nodeIndex = it->second,
            .inverseBindMatrix = convertMatrix(bone->mOffsetMatrix),
        });

        for (unsigned w = 0; w < bone->mNumWeights; ++w) {
            const auto &weight = bone->mWeights[w];

            if (weight.mVertexId >= mesh->mNumVertices) {
                atlas_warning("Bone weight for invalid vertex index: " +
                              std::to_string(weight.mVertexId));
                continue;
            }

            if (weight.mWeight > 0.0f) {
                influences[weight.mVertexId].emplace_back(jointIndex,
                                                          weight.mWeight);
            }
        }
    }

    for (size_t v = 0; v < influences.size(); ++v) {
        auto &list = influences[v];

        std::ranges::sort(list, [](const auto &a, const auto &b) {
            return a.second > b.second;
        });

        auto &vertex = result.vertices[v];

        float total = 0.0f;
        const size_t count = std::min<size_t>(4, list.size());

        for (size_t i = 0; i < count; ++i) {
            vertex.joints[i] = list[i].first;
            vertex.weights[i] = list[i].second;
            total += list[i].second;
        }

        if (total > 0.0f)
            vertex.weights /= total;
    }

    return result;
}

struct ModelMeshInput {
    const aiMesh *mesh;
    glm::mat4 transform;
    atlas::animation::NodeIndex nodeIndex;
};

struct PreparedModelMesh {
    Skin skin;
    glm::mat4 inverseMeshTransform{1.0f};
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
};

void collectModelMeshes(const aiNode &node, const aiScene &scene,
                        const glm::mat4 &parentTransform,
                        atlas::animation::NodeIndex &nextNode,
                        std::vector<ModelMeshInput> &meshes) {
    const glm::mat4 transform =
        parentTransform * assimpToGlmMatrix(node.mTransformation);
    const auto nodeIndex = nextNode++;
    for (unsigned int i = 0; i < node.mNumMeshes; ++i)
        meshes.push_back({scene.mMeshes[node.mMeshes[i]], transform, nodeIndex});
    for (unsigned int i = 0; i < node.mNumChildren; ++i)
        collectModelMeshes(*node.mChildren[i], scene, transform, nextNode, meshes);
}

std::optional<PreparedModelMesh> prepareModelMesh(
    const ModelMeshInput &input, const aiScene &scene,
    const std::unordered_map<std::string, atlas::animation::NodeIndex> *nodes,
    bool usesGltfTextureCoordinates, bool parallelVertices = false) {
    const auto *mesh = input.mesh;
    const auto &transform = input.transform;
    ImportedSkin importedSkin;
    if (nodes) {
        importedSkin = importSkin(mesh, *nodes);
        const auto rigidJoint = static_cast<unsigned int>(importedSkin.skin.joints.size());
        bool usesRigidJoint = false;
        for (auto &vertex : importedSkin.vertices) {
            if (vertex.weights.x == 0.0f) {
                vertex.joints.x = rigidJoint;
                vertex.weights.x = 1.0f;
                usesRigidJoint = true;
            }
        }
        if (usesRigidJoint)
            importedSkin.skin.joints.push_back({input.nodeIndex, glm::mat4(1.0f)});
        if (importedSkin.skin.joints.size() > ModelMesh::MaxJoints)
            throw std::runtime_error("Model mesh exceeds the 256 joint palette limit");
    }
    PreparedModelMesh prepared;
    prepared.skin = std::move(importedSkin.skin);
    const auto &vertexSkin = importedSkin.vertices;
    auto &vertices = prepared.vertices;
    auto &indices = prepared.indices;
    const glm::mat3 linearTransform(transform);
    const float determinant = glm::determinant(linearTransform);
    if (!prepared.skin.joints.empty()) {
        if (determinant == 0.0f || !std::isfinite(determinant))
            throw std::runtime_error("Animated mesh has a singular bind transform");
        prepared.inverseMeshTransform = glm::inverse(transform);
    }
    const bool reflected = determinant < 0.0f;
    const glm::mat3 normalTransform =
        determinant != 0.0f && std::isfinite(determinant)
            ? glm::transpose(glm::inverse(linearTransform))
            : glm::mat3(0.0f);

    indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const auto &face = mesh->mFaces[i];
        if (face.mNumIndices != 3)
            continue;
        const unsigned int a = face.mIndices[0];
        const unsigned int b = face.mIndices[1];
        const unsigned int c = face.mIndices[2];
        if (a >= mesh->mNumVertices || b >= mesh->mNumVertices ||
            c >= mesh->mNumVertices)
            continue;
        indices.push_back(a);
        indices.push_back(reflected ? c : b);
        indices.push_back(reflected ? b : c);
    }
    if (indices.empty())
        return std::nullopt;

    GltfTextureCoordinates colorCoordinates;
    GltfTextureCoordinates normalCoordinates;
    GltfTextureCoordinates pbrCoordinates;
    GltfTextureCoordinates aoCoordinates;
    bool needsNormalTangents = false;
    if (usesGltfTextureCoordinates &&
        mesh->mMaterialIndex < scene.mNumMaterials) {
        const auto &material = *scene.mMaterials[mesh->mMaterialIndex];
        const auto colorType = material.GetTextureCount(aiTextureType_BASE_COLOR)
                                   ? aiTextureType_BASE_COLOR
                                   : aiTextureType_DIFFUSE;
        colorCoordinates = gltfTextureCoordinates(material, colorType);
        normalCoordinates =
            gltfTextureCoordinates(material, aiTextureType_NORMALS);
        needsNormalTangents =
            material.GetTextureCount(aiTextureType_NORMALS) > 0 &&
            (normalCoordinates.channel != 0 || normalCoordinates.sine != 0.0f ||
             normalCoordinates.scale.x * normalCoordinates.cosine < 0.0f ||
             normalCoordinates.scale.y * normalCoordinates.cosine < 0.0f);
        pbrCoordinates = gltfTextureCoordinates(
            material, aiTextureType_GLTF_METALLIC_ROUGHNESS);
        aoCoordinates = gltfTextureCoordinates(material, aiTextureType_LIGHTMAP);
    }

    vertices.resize(mesh->mNumVertices);
    const auto convertVertex = [&](size_t i) {
        glm::vec4 pos =
            transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                  mesh->mVertices[i].z, 1.0f);

        CoreVertex vertex;
        if (!vertexSkin.empty()) {
            vertex.joints = vertexSkin[i].joints;
            vertex.weights = vertexSkin[i].weights;
        }
        vertex.position = Position3d::fromGlm(glm::vec3(pos));

        // ---------- Normals ----------
        if (mesh->mNormals) {
            glm::vec3 normal = normalTransform * glm::vec3(mesh->mNormals[i].x,
                                                           mesh->mNormals[i].y,
                                                           mesh->mNormals[i].z);
            if (glm::length(normal) < 1e-6f) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }
            vertex.normal = Normal3d::fromGlm(normal);
        } else {
            vertex.normal = Normal3d::fromGlm(glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // ---------- Tangents ----------
        glm::vec3 normal = vertex.normal.toGlm();
        glm::vec3 tangent(1.0f, 0.0f, 0.0f);
        if (mesh->mTangents) {
            tangent = linearTransform * glm::vec3(mesh->mTangents[i].x,
                                                  mesh->mTangents[i].y,
                                                  mesh->mTangents[i].z);
        }
        tangent = tangent - normal * glm::dot(normal, tangent);
        if (glm::length(tangent) < 1e-6f) {
            glm::vec3 up = std::abs(normal.y) < 0.999f
                               ? glm::vec3(0.0f, 1.0f, 0.0f)
                               : glm::vec3(1.0f, 0.0f, 0.0f);
            tangent = glm::cross(up, normal);
        }
        tangent = glm::normalize(tangent);

        glm::vec3 bitangent(0.0f, 0.0f, 1.0f);
        if (mesh->mBitangents) {
            bitangent = linearTransform * glm::vec3(mesh->mBitangents[i].x,
                                                    mesh->mBitangents[i].y,
                                                    mesh->mBitangents[i].z);
        } else {
            bitangent = glm::cross(normal, tangent);
        }
        bitangent = bitangent - normal * glm::dot(normal, bitangent);
        if (glm::length(bitangent) < 1e-6f ||
            glm::length(glm::cross(tangent, bitangent)) < 1e-6f) {
            bitangent = glm::cross(normal, tangent);
        }
        const glm::vec3 orthogonalBitangent = glm::cross(normal, tangent);
        bitangent = glm::dot(orthogonalBitangent, bitangent) < 0.0f
                        ? -orthogonalBitangent
                        : orthogonalBitangent;

        vertex.tangent = Normal3d::fromGlm(tangent);
        vertex.bitangent = Normal3d::fromGlm(bitangent);

        // ---------- Texture Coordinates ----------
        if (usesGltfTextureCoordinates) {
            const glm::vec2 colorUv = colorCoordinates.at(*mesh, i);
            vertex.textureCoordinate = {colorUv.x, colorUv.y};
            vertex.normalAndPbrUvOffset = {
                normalCoordinates.at(*mesh, i) - colorUv,
                pbrCoordinates.at(*mesh, i) - colorUv};
            vertex.aoUvOffset = aoCoordinates.at(*mesh, i) - colorUv;
        } else if (mesh->mTextureCoords[0]) {
            glm::vec2 uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                                     mesh->mTextureCoords[0][i].y);
            if (glm::all(glm::isnan(uv)) || glm::any(glm::isinf(uv)))
                uv = glm::vec2(0.0f, 0.0f);
            vertex.textureCoordinate = TextureCoordinate{uv.x, uv.y};
        } else {
            vertex.textureCoordinate = TextureCoordinate{0.0f, 0.0f};
        }

        // ---------- Vertex Color ----------
        if (mesh->mColors[0]) {
            glm::vec4 col =
                glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                          mesh->mColors[0][i].b, mesh->mColors[0][i].a);
            if (glm::all(glm::isnan(col)) || glm::any(glm::isinf(col)))
                col = glm::vec4(1.0f);
            vertex.color = Color{col.r, col.g, col.b, col.a};
        } else {
            vertex.color = Color{1.0f, 1.0f, 1.0f, 1.0f};
        }

        vertices[i] = vertex;
    };
    if (parallelVertices) {
        atlas::detail::Workers::shared().parallelFor(vertices.size(), 8192,
                                                    convertVertex);
    } else {
        for (size_t i = 0; i < vertices.size(); ++i)
            convertVertex(i);
    }

    if (needsNormalTangents)
        updateModelTangents(vertices, indices);

    return prepared;
}

} // namespace

void Model::fromResource(const Resource &resource, bool loadAnimations) {
    loadModel(resource, {}, loadAnimations);
}

void Model::fromResource(
    const Resource &resource,
    const std::function<void(float, const std::string &)> &progress,
    bool loadAnimations) {
    loadModel(resource, progress, loadAnimations);
}

void Model::fromResourceWithEmbeddedAnimations(
    const Resource &resource,
    const std::function<void(float, const std::string &)> &progress) {
    loadModel(resource, progress, true, true);
}

void Model::loadModel(
    const Resource &resource,
    const std::function<void(float, const std::string &)> &progress,
    bool loadAnimations, bool embeddedAnimationsOnly) {
    Assimp::Importer importer;
    if (resource.type != ResourceType::Model) {
        atlas_warning("Resource is not a model: " + resource.name);
        return;
    }

    std::error_code fileError;
    if (!std::filesystem::is_regular_file(resource.path, fileError)) {
        throw std::runtime_error("Model file not found: " +
                                 resource.path.string());
    }

    atlas_log("Loading model: " + resource.name);

    struct ImportStatus {
        std::mutex mutex;
        float value = 0.0f;
        std::string text = "Opening model";
    };
    auto status = std::make_shared<ImportStatus>();
    const auto reportProgress = [status](float value, const std::string &text) {
        std::scoped_lock lock(status->mutex);
        status->value = std::max(status->value, value);
        status->text = text;
    };
    if (progress) {
        progress(0.0f, "Opening model");
        importer.SetProgressHandler(
            new ModelImportProgressHandler(reportProgress));
    }

    unsigned int importFlags =
        aiProcess_Triangulate | aiProcess_CalcTangentSpace |
        aiProcess_SortByPType | aiProcess_GenSmoothNormals |
        aiProcess_ValidateDataStructure | aiProcess_PopulateArmatureData;
    std::string extension = resource.path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return std::tolower(value); });
    usesGltfTextureCoordinates = extension == ".gltf" || extension == ".glb";
    if (usesGltfTextureCoordinates) {
        importFlags |= aiProcess_FlipUVs;
    } else {
        importFlags |=
            aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality;
    }

    const auto sceneCachePath = modelSceneCachePath(resource, importFlags);
    const auto readScene = [&]() -> const aiScene * {
        const aiScene *scene = nullptr;
        if (isModelSceneCacheValid(sceneCachePath)) {
            reportProgress(0.0f, "Loading cached model");
            scene = importer.ReadFile(sceneCachePath.string(), 0);
        }
        if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
            scene->mRootNode == nullptr) {
            auto *io = new ModelImportIO();
            importer.SetIOHandler(io);
            scene = importer.ReadFile(resource.path.string(), importFlags);
            if (scene != nullptr &&
                !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) &&
                scene->mRootNode != nullptr) {
                reportProgress(0.85f, "Caching imported model");
                storeModelSceneCache(scene, sceneCachePath, *io);
            }
        }
        return scene;
    };
    const aiScene *scene = nullptr;
    if (progress) {
        auto pendingScene = std::async(std::launch::async, readScene);
        while (pendingScene.wait_for(std::chrono::milliseconds(33)) !=
               std::future_status::ready) {
            float value;
            std::string text;
            {
                std::lock_guard<std::mutex> lock(status->mutex);
                value = status->value;
                text = status->text;
            }
            progress(value, text);
        }
        scene = pendingScene.get();
    } else {
        scene = readScene();
    }

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        atlas_error("Assimp error: " + std::string(importer.GetErrorString()));
        throw std::runtime_error("Assimp error: " +
                                 std::string(importer.GetErrorString()));
        return;
    }
    directory = resource.path.parent_path().string();
    if (progress)
        progress(0.88f, "Loading meshes and materials");
    // Texture cache to avoid loading the same texture multiple times
    std::unordered_map<std::string, Texture> textureCache;

    preloadMaterialTextures(scene, textureCache, progress);

    loadAnimations = loadAnimations &&
                     (!embeddedAnimationsOnly || scene->HasAnimations());
    const auto imported = loadAnimations ? importSkeleton(scene) : SkeletonImportResult{};
    objects.clear();
    animator.setSkeleton(imported.skeleton);
    if (loadAnimations) {
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            auto clip = importClip(scene->mAnimations[i], imported.nodeByName);
            if (clip.name.empty())
                clip.name = "Animation " + std::to_string(i + 1);
            const std::string baseName = clip.name;
            for (unsigned int suffix = 2; animator.findClip(clip.name); ++suffix)
                clip.name = baseName + " (" + std::to_string(suffix) + ")";
            animator.addClip(std::move(clip));
        }
    }

    std::vector<ModelMeshInput> meshes;
    atlas::animation::NodeIndex nextNode = 0;
    collectModelMeshes(*scene->mRootNode, *scene, glm::mat4(1.0f), nextNode,
                       meshes);
    const auto reportMeshes = [&](size_t completed) {
        if (progress) {
            const float fraction = static_cast<float>(completed) / meshes.size();
            progress(0.96f + 0.03f * fraction, "Converting model meshes");
        }
    };
    const auto *nodes = loadAnimations ? &imported.nodeByName : nullptr;
    const bool usesGltf = usesGltfTextureCoordinates;
    const auto consumeMesh = [&](size_t index,
                                 std::optional<PreparedModelMesh> prepared) {
        if (prepared) {
            auto object = std::make_shared<ModelMesh>();
            object->vertices = std::move(prepared->vertices);
            object->indices = std::move(prepared->indices);
            object->skin = std::move(prepared->skin);
            object->inverseMeshTransform = prepared->inverseMeshTransform;
            finishMesh(*object, *meshes[index].mesh, *scene, textureCache);
            object->updateSkin(animator.getGlobalPose());
            objects.push_back(std::move(object));
        } else {
            atlas_warning("Skipping mesh without valid triangles: " +
                          std::string(meshes[index].mesh->mName.C_Str()));
        }
        reportMeshes(index + 1);
    };
    if (meshes.size() == 1) {
        consumeMesh(0, prepareModelMesh(meshes.front(), *scene, nodes,
                                        usesGltf, true));
    } else {
        auto &workers = atlas::detail::Workers::shared();
        workers.process(
            meshes.size(), workers.size(),
            [&](size_t index) {
                return prepareModelMesh(meshes[index], *scene, nodes, usesGltf);
            },
            [&](size_t index, std::future<std::optional<PreparedModelMesh>> ready) {
                consumeMesh(index, ready.get());
            },
            progress ? std::function<void(size_t)>(reportMeshes) : nullptr);
    }

    if (progress)
        progress(1.0f, "Finishing import");

    atlas_log("Model loaded successfully: " + resource.name + " (" +
              std::to_string(objects.size()) + " objects, " +
              std::to_string(scene->mNumMeshes) + " meshes)");

    // std::cout << "Created model from resource: " << resource.name << " with "
    //           << objects.size() << " objects." << std::endl;
    // std::cout << "Model path: " << resource.path << std::endl;
    // std::cout << "Model directory: " << directory << std::endl;
    // std::cout << "First model object has "
    //           << (objects.size() > 0 ? objects[0]->getVertices().size() : 0)
    //           << " vertices." << std::endl;

    // std::cout << "Model loaded successfully." << std::endl;
    // std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    // std::cout << "Unique textures loaded: " << textureCache.size() <<
    // std::endl;

    // size_t totalVertices = 0;
    // size_t totalTriangles = 0;
    // for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //     aiMesh *mesh = scene->mMeshes[i];
    //     totalVertices += mesh->mNumVertices;
    //     totalTriangles +=
    //         mesh->mNumFaces; // each face is a triangle (after Triangulate)
    // }

    // std::cout << "Total Vertices: " << totalVertices << std::endl;
    // std::cout << "Total Triangles: " << totalTriangles << std::endl;
}

void Model::preloadMaterialTextures(
    const aiScene *scene,
    std::unordered_map<std::string, Texture> &textureCache,
    const std::function<void(float, const std::string &)> &progress) {
    std::vector<ModelTextureJob> jobs;
    std::unordered_set<std::string> scheduled;

    auto queueTextures = [&](aiMaterial *material, aiTextureType sourceType,
                             const std::string &typeName,
                             TextureType textureType) {
        for (unsigned int index = 0;
             index < material->GetTextureCount(sourceType); ++index) {
            aiString path;
            if (material->GetTexture(sourceType, index, &path) != AI_SUCCESS) {
                continue;
            }
            const std::string filename = path.C_Str();
            const std::string fullPath = directory + "/" + filename;
            std::string aoPath;
            aiString aoTexturePath;
            std::string cacheKey = fullPath + "|" + typeName;
            if (textureType == TextureType::PBRPack &&
                !usesGltfTextureCoordinates &&
                material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0,
                                     &aoTexturePath) == AI_SUCCESS) {
                aoPath = directory + "/" + aoTexturePath.C_Str();
                cacheKey += "|" + std::string(aoTexturePath.C_Str());
            }
            if (!scheduled.insert(cacheKey).second)
                continue;

            const auto *ao = aoPath.empty()
                                 ? nullptr
                                 : scene->GetEmbeddedTexture(aoTexturePath.C_Str());
            jobs.push_back(
                ModelTextureJob{.cacheKey = std::move(cacheKey),
                                .fullPath = fullPath,
                                .filename = filename,
                                .aoPath = std::move(aoPath),
                                .textureType = textureType,
                                .resourceType = typeName == "texture_specular"
                                                    ? ResourceType::SpecularMap
                                                    : ResourceType::Image,
                                .source = scene->GetEmbeddedTexture(filename.c_str()),
                                .ambientOcclusion = ao});
        }
    };

    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials;
         ++materialIndex) {
        aiMaterial *material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
            queueTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse",
                          TextureType::Color);
        } else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            queueTextures(material, aiTextureType_DIFFUSE, "texture_diffuse",
                          TextureType::Color);
        } else {
            queueTextures(material, aiTextureType_AMBIENT, "texture_diffuse",
                          TextureType::Color);
        }

        queueTextures(material, aiTextureType_SPECULAR, "texture_specular",
                      TextureType::Specular);
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            queueTextures(material, aiTextureType_NORMALS, "texture_normal",
                          TextureType::Normal);
        } else if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) {
            queueTextures(material, aiTextureType_HEIGHT, "texture_normal",
                          TextureType::Normal);
        } else {
            queueTextures(material, aiTextureType_DISPLACEMENT,
                          "texture_normal", TextureType::Normal);
        }

        if (material->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) >
            0) {
            queueTextures(material, aiTextureType_GLTF_METALLIC_ROUGHNESS,
                          "texture_pbr_pack", TextureType::PBRPack);
        } else {
            queueTextures(material, aiTextureType_METALNESS, "texture_metallic",
                          TextureType::Metallic);
            queueTextures(material, aiTextureType_DIFFUSE_ROUGHNESS,
                          "texture_roughness", TextureType::Roughness);
            if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) >
                0) {
                queueTextures(material, aiTextureType_AMBIENT_OCCLUSION,
                              "texture_ao", TextureType::AO);
            } else {
                queueTextures(material, aiTextureType_LIGHTMAP, "texture_ao",
                              TextureType::AO);
            }
        }
        queueTextures(material, aiTextureType_OPACITY, "texture_opacity",
                      TextureType::Opacity);
    }

    if (jobs.size() >= 48) {
        for (auto &job : jobs)
            job.maximumDimension = 2048;
    }
    const auto reportTextures = [&](size_t completed) {
        if (progress)
            progress(0.88f + 0.08f * static_cast<float>(completed) / jobs.size(),
                     "Loading model textures");
    };
    auto &workers = atlas::detail::Workers::shared();
    workers.process(
        jobs.size(), std::min(workers.size(), size_t{6}),
        [&](size_t index) { return decodeModelTexture(jobs[index]); },
        [&](size_t index, std::future<DecodedModelTexture> ready) {
            const auto &job = jobs[index];
            try {
                textureCache[job.cacheKey] = uploadModelTexture(job, ready.get());
            } catch (const std::exception &error) {
                atlas_warning("Failed to preload texture '" + job.filename +
                              "': " + error.what());
            }
            reportTextures(index + 1);
        },
        progress ? std::function<void(size_t)>(reportTextures) : nullptr);
}

void Model::finishMesh(
    ModelMesh &object, const aiMesh &mesh, const aiScene &scene,
    std::unordered_map<std::string, Texture> &textureCache) {
    std::vector<Texture> textures;
    if (mesh.mMaterialIndex < scene.mNumMaterials) {
        aiMaterial *material = scene.mMaterials[mesh.mMaterialIndex];
        importMaterialProperties(material, object);

        auto diffuseMaps =
            loadMaterialTextures(material, std::any(aiTextureType_BASE_COLOR),
                                 "texture_diffuse", textureCache);
        if (diffuseMaps.empty()) {
            auto legacyDiffuseMaps =
                loadMaterialTextures(material, std::any(aiTextureType_DIFFUSE),
                                     "texture_diffuse", textureCache);
            diffuseMaps.insert(diffuseMaps.end(), legacyDiffuseMaps.begin(),
                               legacyDiffuseMaps.end());
        }
        if (diffuseMaps.empty()) {
            auto ambientMaps =
                loadMaterialTextures(material, std::any(aiTextureType_AMBIENT),
                                     "texture_diffuse", textureCache);
            diffuseMaps.insert(diffuseMaps.end(), ambientMaps.begin(),
                               ambientMaps.end());
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps =
            loadMaterialTextures(material, std::any(aiTextureType_SPECULAR),
                                 "texture_specular", textureCache);
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());

        auto normalMaps =
            loadMaterialTextures(material, std::any(aiTextureType_NORMALS),
                                 "texture_normal", textureCache);
        if (normalMaps.empty()) {
            auto heightAsNormalMaps =
                loadMaterialTextures(material, std::any(aiTextureType_HEIGHT),
                                     "texture_normal", textureCache);
            normalMaps.insert(normalMaps.end(), heightAsNormalMaps.begin(),
                              heightAsNormalMaps.end());
        }
        if (normalMaps.empty()) {
            auto displacementAsNormalMaps = loadMaterialTextures(
                material, std::any(aiTextureType_DISPLACEMENT),
                "texture_normal", textureCache);
            normalMaps.insert(normalMaps.end(),
                              displacementAsNormalMaps.begin(),
                              displacementAsNormalMaps.end());
        }
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        auto pbrPackMaps = loadMaterialTextures(
            material, std::any(aiTextureType_GLTF_METALLIC_ROUGHNESS),
            "texture_pbr_pack", textureCache);
        textures.insert(textures.end(), pbrPackMaps.begin(), pbrPackMaps.end());

        if (pbrPackMaps.empty()) {
            auto metallicMaps = loadMaterialTextures(
                material, std::any(aiTextureType_METALNESS), "texture_metallic",
                textureCache);
            textures.insert(textures.end(), metallicMaps.begin(),
                            metallicMaps.end());

            auto roughnessMaps = loadMaterialTextures(
                material, std::any(aiTextureType_DIFFUSE_ROUGHNESS),
                "texture_roughness", textureCache);
            textures.insert(textures.end(), roughnessMaps.begin(),
                            roughnessMaps.end());
        }

        if (usesGltfTextureCoordinates || pbrPackMaps.empty()) {
            auto aoMaps = loadMaterialTextures(
                material, std::any(aiTextureType_AMBIENT_OCCLUSION),
                "texture_ao", textureCache);
            if (aoMaps.empty()) {
                auto lightmapMaps = loadMaterialTextures(
                    material, std::any(aiTextureType_LIGHTMAP), "texture_ao",
                    textureCache);
                aoMaps.insert(aoMaps.end(), lightmapMaps.begin(),
                              lightmapMaps.end());
            }
            textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        }

        auto opacityMaps =
            loadMaterialTextures(material, std::any(aiTextureType_OPACITY),
                                 "texture_opacity", textureCache);
        aiString alphaMode;
        if (opacityMaps.empty() && !diffuseMaps.empty() &&
            material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS &&
            std::string(alphaMode.C_Str()) != "OPAQUE") {
            Texture opacityMap = diffuseMaps.front();
            opacityMap.type = TextureType::Opacity;
            opacityMaps.push_back(std::move(opacityMap));
        }
        textures.insert(textures.end(), opacityMaps.begin(), opacityMaps.end());
    }

    if (!textures.empty()) {
        for (const auto &texture : textures) {
            object.attachTexture(texture);
        }
    }

    object.name = mesh.mName.length > 0 ? mesh.mName.C_Str() : "Mesh";

    ResourceEventInfo info;
    info.resourceType = DebugResourceType::Mesh;
    info.callerObject = std::to_string(object.getId());
    info.operation = DebugResourceOperation::Created;
    info.frameNumber =
        Window::mainWindow != nullptr && Window::mainWindow->device != nullptr
            ? Window::mainWindow->device->frameCount
            : 0;
    info.sizeMb = static_cast<float>((object.vertices.size() * sizeof(CoreVertex)) +
                                     (object.indices.size() * sizeof(unsigned int))) /
                  (1024.0f * 1024.0f);

    info.send();
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat, std::any type, const std::string &typeName,
    std::unordered_map<std::string, Texture> &textureCache) {
    aiMaterial *material = mat;
    aiTextureType textureType = std::any_cast<aiTextureType>(type);
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++) {
        aiString str;
        material->GetTexture(textureType, i, &str);
        std::string filename = std::string(str.C_Str());
        std::string fullPath = directory + "/" + filename;
        std::string cacheKey = fullPath + "|" + typeName;
        if (typeName == "texture_pbr_pack" && !usesGltfTextureCoordinates) {
            aiString aoPath;
            if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0,
                                     &aoPath) == AI_SUCCESS)
                cacheKey += "|" + std::string(aoPath.C_Str());
        }

        // Check if texture is already cached
        auto cacheIt = textureCache.find(cacheKey);
        if (cacheIt != textureCache.end()) {
            textures.push_back(cacheIt->second);
            continue;
        }

        // std::cout << "Loading texture: " << str.C_Str() << " of type "
        //           << typeName << std::endl;

        ResourceType resType = ResourceType::Image;
        if (typeName == "texture_specular") {
            resType = ResourceType::SpecularMap;
        }
        Resource resource =
            Workspace::get().createResource(fullPath, filename, resType);
        TextureType texType = TextureType::Color;
        if (typeName == "texture_specular") {
            texType = TextureType::Specular;
        } else if (typeName == "texture_normal") {
            texType = TextureType::Normal;
        } else if (typeName == "texture_height") {
            texType = TextureType::Parallax;
        } else if (typeName == "texture_metallic") {
            texType = TextureType::Metallic;
        } else if (typeName == "texture_roughness") {
            texType = TextureType::Roughness;
        } else if (typeName == "texture_ao") {
            texType = TextureType::AO;
        } else if (typeName == "texture_opacity") {
            texType = TextureType::Opacity;
        } else if (typeName == "texture_pbr_pack") {
            texType = TextureType::PBRPack;
        }

        try {
            Texture loadedTexture;
            if (texType == TextureType::PBRPack) {
                int width = 0;
                int height = 0;
                int channels = 0;
                std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
                    stbi_load(fullPath.c_str(), &width, &height, &channels,
                              STBI_rgb_alpha),
                    stbi_image_free);
                if (data == nullptr)
                    throw std::runtime_error("Failed to load PBR pack image");

                const size_t pixelCount = static_cast<size_t>(width) * height;
                for (size_t pixel = 0; pixel < pixelCount; pixel++)
                    data.get()[pixel * 4] = 255;

                aiString aoPath;
                if (!usesGltfTextureCoordinates &&
                    material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0,
                                         &aoPath) == AI_SUCCESS) {
                    int aoWidth = 0;
                    int aoHeight = 0;
                    int aoChannels = 0;
                    const std::string fullAoPath =
                        directory + "/" + std::string(aoPath.C_Str());
                    std::unique_ptr<unsigned char, decltype(&stbi_image_free)>
                        aoData(stbi_load(fullAoPath.c_str(), &aoWidth,
                                         &aoHeight, &aoChannels, STBI_grey),
                               stbi_image_free);
                    if (aoData != nullptr && aoWidth > 0 && aoHeight > 0) {
                        for (int y = 0; y < height; y++) {
                            const int aoY = y * aoHeight / height;
                            for (int x = 0; x < width; x++) {
                                const int aoX = x * aoWidth / width;
                                data.get()[(static_cast<size_t>(y) * width +
                                            x) *
                                           4] =
                                    aoData.get()[static_cast<size_t>(aoY) *
                                                     aoWidth +
                                                 aoX];
                            }
                        }
                    }
                }

                const unsigned int mipLevels =
                    1u + static_cast<unsigned int>(
                             std::floor(std::log2(std::max(width, height))));
                auto opalTexture = opal::Texture::create(
                    opal::TextureType::Texture2D, opal::TextureFormat::Rgba8,
                    width, height, opal::TextureDataFormat::Rgba, data.get(),
                    mipLevels);
                opalTexture->setParameters(
                    opal::TextureWrapMode::Repeat,
                    opal::TextureWrapMode::Repeat,
                    opal::TextureFilterMode::LinearMipmapLinear,
                    opal::TextureFilterMode::Linear);
                opalTexture->automaticallyGenerateMipmaps();
                loadedTexture = Texture{.resource = resource,
                                        .creationData = {width, height, 4},
                                        .id = opalTexture->textureID,
                                        .texture = opalTexture,
                                        .type = texType};
            } else {
                loadedTexture = Texture::fromResource(resource, texType);
            }
            textureCache[cacheKey] = loadedTexture;
            textures.push_back(loadedTexture);
        } catch (const std::exception &ex) {
            atlas_warning("Failed to load texture '" + filename +
                          "': " + ex.what());
            std::cerr << "Failed to load texture '" << filename
                      << "': " << ex.what() << std::endl;
        }
    }
    return textures;
}

bool Model::importAnimationClip(const Resource &resource,
                                std::string_view sourceName,
                                std::string_view name) {
    if (resource.type != ResourceType::Model || animator.getSkeleton().nodes.empty())
        return false;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(resource.path.string(), 0);
    if (!scene)
        return false;

    std::unordered_map<std::string, atlas::animation::NodeIndex> nodes;
    const auto &skeletonNodes = animator.getSkeleton().nodes;
    for (atlas::animation::NodeIndex i = 0; i < skeletonNodes.size(); ++i)
        nodes.emplace(skeletonNodes[i].name, i);

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        const auto *source = scene->mAnimations[i];
        const std::string candidate = source->mName.length
            ? source->mName.C_Str() : "Animation " + std::to_string(i + 1);
        if (candidate != sourceName && !(sourceName.empty() && scene->mNumAnimations == 1))
            continue;
        auto clip = importClip(source, nodes);
        if (clip.channels.empty())
            return false;
        clip.name = name.empty() ? candidate : std::string(name);
        return animator.addClip(std::move(clip));
    }
    return false;
}

void Model::update(Window &window) {
    for (auto &component : components) {
        component->update(window.getDeltaTime());
    }
    animator.update(window.getDeltaTime());
    for (auto &obj : objects) {
        if (obj == nullptr) {
            continue;
        }
        if (auto *mesh = dynamic_cast<ModelMesh *>(obj.get()))
            mesh->updateSkin(animator.getGlobalPose());
        bool hasAnyTexture = !obj->textures.empty();
        if (!hasAnyTexture) {
            obj->material = material;
        }
        obj->material.useNormalMap = material.useNormalMap;
        obj->material.normalMapStrength = material.normalMapStrength;
        obj->material.textureScale = material.textureScale;
        obj->material.textureOffset = material.textureOffset;
        obj->useDeferredRendering = useDeferredRendering;
        obj->update(window);
    }
}
