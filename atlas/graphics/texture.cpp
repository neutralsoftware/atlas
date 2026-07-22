/*
 texture.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture importing functions and definitions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "opal/opal.h"
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    glm::vec3 cubemapDirectionFromFace(int faceIndex, float u, float v) {
        switch (faceIndex) {
        case 0:
            return glm::normalize(glm::vec3(1.0f, -v, -u));
        case 1:
            return glm::normalize(glm::vec3(-1.0f, -v, u));
        case 2:
            return glm::normalize(glm::vec3(u, 1.0f, v));
        case 3:
            return glm::normalize(glm::vec3(u, -1.0f, -v));
        case 4:
            return glm::normalize(glm::vec3(u, -v, 1.0f));
        case 5:
            return glm::normalize(glm::vec3(-u, -v, -1.0f));
        default:
            return glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    struct CubemapWeightCache {
        int size = 0;
        std::array<std::vector<float>, 6> weights;
    };

    const CubemapWeightCache& getCubemapWeightCache(int size) {
        static std::unordered_map<int, CubemapWeightCache> cache;
        auto it = cache.find(size);
        if (it != cache.end()) {
            return it->second;
        }

        CubemapWeightCache weightCache;
        weightCache.size = size;
        const size_t pixelCount = static_cast<size_t>(size) * size;
        constexpr int horizonOrder[4] = {0, 4, 1, 5};

        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            std::vector<float>& faceWeights = weightCache.weights[faceIndex];
            faceWeights.resize(pixelCount * 6);

            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    float u = ((static_cast<float>(x) + 0.5f) /
                            static_cast<float>(size) * 2.0f) -
                        1.0f;
                    float v = ((static_cast<float>(y) + 0.5f) /
                            static_cast<float>(size) * 2.0f) -
                        1.0f;

                    glm::vec3 direction = cubemapDirectionFromFace(faceIndex, u, v);
                    float up = glm::clamp(direction.y, -1.0f, 1.0f);
                    float upPositive = glm::clamp(up, 0.0f, 1.0f);
                    float upNegative = glm::clamp(-up, 0.0f, 1.0f);
                    float horizonBase =
                        glm::clamp(1.0f - std::fabs(up), 0.0f, 1.0f);

                    float topFactor = std::pow(upPositive, 0.85f);
                    float bottomFactor = std::pow(upNegative, 0.85f);
                    float horizonFactor = std::pow(horizonBase, 0.65f);

                    float normalizationSum =
                        topFactor + bottomFactor + horizonFactor;
                    if (normalizationSum <= 1e-6f) {
                        horizonFactor = 1.0f;
                        topFactor = 0.0f;
                        bottomFactor = 0.0f;
                        normalizationSum = 1.0f;
                    }

                    topFactor /= normalizationSum;
                    bottomFactor /= normalizationSum;
                    horizonFactor /= normalizationSum;

                    float angle = std::atan2(direction.z, direction.x);
                    if (angle < 0.0f) {
                        angle += glm::two_pi<float>();
                    }
                    float scaled = angle / glm::two_pi<float>() * 4.0f;
                    float sectorFloat = std::floor(scaled);
                    int sector = static_cast<int>(sectorFloat) & 3;
                    float sectorT = scaled - sectorFloat;
                    float horizonInterp = glm::smoothstep(0.0f, 1.0f, sectorT);

                    float horizonWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                    horizonWeights[sector] = (1.0f - horizonInterp) * horizonFactor;
                    horizonWeights[(sector + 1) & 3] +=
                        horizonInterp * horizonFactor;

                    float colorWeights[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
                    colorWeights[2] = topFactor;
                    colorWeights[3] = bottomFactor;
                    for (int h = 0; h < 4; ++h) {
                        colorWeights[horizonOrder[h]] += horizonWeights[h];
                    }

                    float totalWeight = 0.0f;
                    for (float weight : colorWeights) {
                        totalWeight += weight;
                    }
                    float invTotal =
                        totalWeight > 1e-6f ? 1.0f / totalWeight : 0.0f;

                    size_t pixelWeightIndex =
                        (static_cast<size_t>(y) * static_cast<size_t>(size) +
                            static_cast<size_t>(x)) *
                        6;
                    float* dest = faceWeights.data() + pixelWeightIndex;
                    for (size_t neighbor = 0; neighbor < 6; ++neighbor) {
                        dest[neighbor] = colorWeights[neighbor] * invTotal;
                    }
                }
            }
        }

        auto inserted = cache.emplace(size, std::move(weightCache));
        return inserted.first->second;
    }

    void cubemapFillFaceData(const std::array<Color, 6>& colors, int faceIndex,
                             int size, std::vector<unsigned char>& faceData,
                             glm::dvec3& accumulatedColor) {
        const auto& weightCache = getCubemapWeightCache(size);
        const std::vector<float>& faceWeights = weightCache.weights[faceIndex];
        const size_t pixelCount =
            static_cast<size_t>(size) * static_cast<size_t>(size);

        if (faceData.size() != pixelCount * 4) {
            faceData.resize(pixelCount * 4);
        }

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const size_t pixelIndex = (static_cast<size_t>(y) * size) + x;
                const float* weights = faceWeights.data() + (pixelIndex * 6);

                glm::vec3 finalColor(0.0f);
                float finalAlpha = 0.0f;

                for (size_t neighbor = 0; neighbor < 6; ++neighbor) {
                    float weight = weights[neighbor];
                    if (weight <= 0.0f) {
                        continue;
                    }
                    const Color& sample = colors[neighbor];
                    finalColor.r += static_cast<float>(sample.r) * weight;
                    finalColor.g += static_cast<float>(sample.g) * weight;
                    finalColor.b += static_cast<float>(sample.b) * weight;
                    finalAlpha += static_cast<float>(sample.a) * weight;
                }

                finalColor =
                    glm::clamp(finalColor, glm::vec3(0.0f), glm::vec3(1.0f));
                finalAlpha = glm::clamp(finalAlpha, 0.0f, 1.0f);

                size_t byteIndex = pixelIndex * 4;
                faceData[byteIndex + 0] =
                    static_cast<unsigned char>(finalColor.r * 255.0f);
                faceData[byteIndex + 1] =
                    static_cast<unsigned char>(finalColor.g * 255.0f);
                faceData[byteIndex + 2] =
                    static_cast<unsigned char>(finalColor.b * 255.0f);
                faceData[byteIndex + 3] =
                    static_cast<unsigned char>(finalAlpha * 255.0f);

                accumulatedColor.x += finalColor.r;
                accumulatedColor.y += finalColor.g;
                accumulatedColor.z += finalColor.b;
            }
        }
    }
} // namespace

Texture Texture::fromResourceName(const std::string& resourceName,
                                  TextureType type, TextureParameters params,
                                  Color borderColor) {
    Resource resource = Workspace::get().getResource(resourceName);
    return fromResource(resource, type, params, borderColor);
}

Texture Texture::fromResource(const Resource& resource, TextureType type,
                              TextureParameters params, Color borderColor) {
    if (resource.type != ResourceType::Image &&
        resource.type != ResourceType::SpecularMap) {
        atlas_error("Resource is not an image: " + resource.name);
    }

    atlas_log("Loading texture: " + resource.name);

    int width, height, channelsInFile;
#ifdef OPENGL
    stbi_set_flip_vertically_on_load(true);
#else
    stbi_set_flip_vertically_on_load(false);
#endif

    TextureCreationData creationData;
    std::shared_ptr<opal::Texture> opalTexture;

    if (type == TextureType::HDR) {
        int channels = 0;
        float* data = stbi_loadf(resource.path.string().c_str(), &width,
                                 &height, &channels, 0);
        if (!data) {
            atlas_error("Failed to load HDR image: " + resource.path.string());
        }

        creationData = TextureCreationData{.width = width, .height = height, .channels = channels};

        opal::TextureFormat internalFormat = opal::TextureFormat::Rgb16F;
        opal::TextureDataFormat dataFormat = opal::TextureDataFormat::Rgb;
        if (channels == 1) {
            internalFormat = opal::TextureFormat::Red16F;
            dataFormat = opal::TextureDataFormat::Red;
        }
        else if (channels == 4) {
            internalFormat = opal::TextureFormat::Rgba16F;
            dataFormat = opal::TextureDataFormat::Rgba;
        }

        const uint mipLevels = 1u + static_cast<uint>(std::floor(std::log2(
                                           std::max(width, height))));
        opalTexture =
            opal::Texture::create(opal::TextureType::Texture2D, internalFormat,
                                  width, height, dataFormat, data, mipLevels);
        stbi_image_free(data);
    }
    else {
        const int requestedChannels =
            (type == TextureType::Color) ? STBI_rgb_alpha : 0;
        unsigned char* data = stbi_load(resource.path.string().c_str(), &width,
                                        &height, &channelsInFile,
                                        requestedChannels);
        if (!data) {
            atlas_error("Failed to load image: " + resource.path.string());
        }

        int channels = requestedChannels > 0 ? requestedChannels : channelsInFile;
        creationData = TextureCreationData{
            .width = width, .height = height, .channels = channels};

        opal::TextureFormat internalFormat;
        const bool useSrgb = (type == TextureType::Color);
        if (channels == 4) {
            internalFormat = useSrgb ? opal::TextureFormat::sRgba8
                                     : opal::TextureFormat::Rgba8;
        } else if (channels == 3) {
            internalFormat = useSrgb ? opal::TextureFormat::sRgb8
                                     : opal::TextureFormat::Rgb8;
        } else {
            internalFormat = opal::TextureFormat::Red8;
        }

        opal::TextureDataFormat dataFormat;
        if (channels == 4) {
            dataFormat = opal::TextureDataFormat::Rgba;
        } else if (channels == 3) {
            dataFormat = opal::TextureDataFormat::Rgb;
        } else {
            dataFormat = opal::TextureDataFormat::Red;
        }

        const uint mipLevels = 1u + static_cast<uint>(std::floor(std::log2(
                                           std::max(width, height))));
        opalTexture =
            opal::Texture::create(opal::TextureType::Texture2D, internalFormat,
                                  width, height, dataFormat, data, mipLevels);
        stbi_image_free(data);
    }

    auto toOpalWrap = [](TextureWrappingMode m) -> opal::TextureWrapMode
    {
        switch (m) {
        case TextureWrappingMode::Repeat:
            return opal::TextureWrapMode::Repeat;
        case TextureWrappingMode::MirroredRepeat:
            return opal::TextureWrapMode::MirroredRepeat;
        case TextureWrappingMode::ClampToEdge:
            return opal::TextureWrapMode::ClampToEdge;
        case TextureWrappingMode::ClampToBorder:
            return opal::TextureWrapMode::ClampToBorder;
        default:
            return opal::TextureWrapMode::Repeat;
        }
    };
    auto toOpalFilter = [](TextureFilteringMode m) -> opal::TextureFilterMode
    {
        return m == TextureFilteringMode::Nearest
                   ? opal::TextureFilterMode::Nearest
                   : opal::TextureFilterMode::Linear;
    };
    const opal::TextureFilterMode minFilter =
        params.minifyingFilter == TextureFilteringMode::Nearest
            ? opal::TextureFilterMode::NearestMipmapNearest
            : opal::TextureFilterMode::LinearMipmapLinear;
    opalTexture->setParameters(toOpalWrap(params.wrappingModeS),
                               toOpalWrap(params.wrappingModeT), minFilter,
                               toOpalFilter(params.magnifyingFilter));

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        opalTexture->changeBorderColor(borderColor.toGlm());
    }

    opalTexture->automaticallyGenerateMipmaps();

    Texture texture{
        .resource = resource, .creationData = creationData, .id = opalTexture->textureID,
        .texture = opalTexture, .type = type, .borderColor = borderColor
    };
    return texture;
}

void Texture::applyWrappingMode(TextureWrappingMode mode,
                                opal::TextureAxis axis,
                                const std::shared_ptr<opal::Texture>& texture) {
    switch (mode) {
    case TextureWrappingMode::Repeat:
        texture->setWrapMode(axis, opal::TextureWrapMode::Repeat);
        break;
    case TextureWrappingMode::MirroredRepeat:
        texture->setWrapMode(axis, opal::TextureWrapMode::MirroredRepeat);
        break;
    case TextureWrappingMode::ClampToEdge:
        texture->setWrapMode(axis, opal::TextureWrapMode::ClampToEdge);
        break;
    case TextureWrappingMode::ClampToBorder:
        texture->setWrapMode(axis, opal::TextureWrapMode::ClampToBorder);
        break;
    default:
        throw std::runtime_error("Unknown wrapping mode");
    }
}

void Texture::applyFilteringModes(TextureFilteringMode minMode,
                                  TextureFilteringMode magMode,
                                  const std::shared_ptr<opal::Texture>& texture) {
    opal::TextureFilterMode opalMinMode;
    opal::TextureFilterMode opalMagMode;

    switch (minMode) {
    case TextureFilteringMode::Nearest:
        opalMinMode = opal::TextureFilterMode::Nearest;
        break;
    case TextureFilteringMode::Linear:
        opalMinMode = opal::TextureFilterMode::Linear;
        break;
    default:
        throw std::runtime_error("Unknown minifying filter mode");
    }

    switch (magMode) {
    case TextureFilteringMode::Nearest:
        opalMagMode = opal::TextureFilterMode::Nearest;
        break;
    case TextureFilteringMode::Linear:
        opalMagMode = opal::TextureFilterMode::Linear;
        break;
    default:
        throw std::runtime_error("Unknown magnifying filter mode");
    }

    texture->setFilterMode(opalMinMode, opalMagMode);
}

void Texture::display(Window& window, float zindex) {
    if (texture == nullptr) {
        throw std::runtime_error("Texture is not initialized");
    }
    if (id == 0) {
        id = texture->textureID;
    }
    if (id == 0) {
        throw std::runtime_error("Texture is not initialized");
    }

    if (object == nullptr) {
        CoreObject obj;
        std::vector<CoreVertex> vertices = {
#ifdef METAL
            {{1.0f, 1.0f, zindex}, Color::white(), {1.0f, 0.0f}},
            {{1.0f, -1.0f, zindex}, Color::white(), {1.0f, 1.0f}},
            {{-1.0f, -1.0f, zindex}, Color::white(), {0.0f, 1.0f}},
            {{-1.0f, 1.0f, zindex}, Color::white(), {0.0f, 0.0f}}
#else
            {{1.0f, 1.0f, zindex}, Color::white(), {1.0f, 1.0f}},
            {{1.0f, -1.0f, zindex}, Color::white(), {1.0f, 0.0f}},
            {{-1.0f, -1.0f, zindex}, Color::white(), {0.0f, 0.0f}},
            {{-1.0f, 1.0f, zindex}, Color::white(), {0.0f, 1.0f}}
#endif
        };

        VertexShader vertexShader =
            VertexShader::fromDefaultShader(AtlasVertexShader::Fullscreen);
        FragmentShader fragmentShader =
            FragmentShader::fromDefaultShader(AtlasFragmentShader::Fullscreen);
        vertexShader.fromDefaultShaderType = std::nullopt;
        fragmentShader.fromDefaultShaderType = std::nullopt;
        obj.createAndAttachProgram(vertexShader, fragmentShader);

#ifdef METAL
        std::vector<Index> indices = {0, 3, 1, 1, 3, 2};
#else
        std::vector<Index> indices = {0, 1, 3, 1, 2, 3};
#endif
        obj.attachTexture(*this);
        obj.attachVertices(vertices);
        obj.attachIndices(indices);
        obj.renderOnlyTexture();
        obj.show();
        obj.initialize();
        object = std::make_shared<CoreObject>(obj);
        window.addPreferencedObject(object.get());
    } else {
        object->show();
    }
}

Cubemap Cubemap::fromResourceGroup(ResourceGroup& group) {
    if (group.resources.size() != 6) {
        atlas_error("Cubemap requires exactly 6 resources");
        throw std::runtime_error("Cubemap requires exactly 6 resources");
    }

    atlas_log("Creating cubemap from resource group: " + group.groupName);

    int width = 0, height = 0, channels = 0;
    glm::dvec3 accumulatedColor(0.0);
    unsigned long long accumulatedPixels = 0;

    stbi_set_flip_vertically_on_load(false);
    unsigned char* firstData =
        stbi_load(group.resources[0].path.string().c_str(), &width, &height,
                  &channels, 0);
    if (!firstData) {
        atlas_error("Failed to load image: " +
            group.resources[0].path.string());
        throw std::runtime_error("Failed to load image: " +
            group.resources[0].path.string());
    }

    opal::TextureFormat format;
    if (channels == 3)
        format = channels == 4
                     ? opal::TextureFormat::Rgba8
                     : opal::TextureFormat::Rgb8;
    else
        format = channels == 4
                     ? opal::TextureFormat::Rgba8
                     : opal::TextureFormat::Red8;
    opal::TextureDataFormat dataFormat;
    if (channels == 3)
        dataFormat = channels == 4
                         ? opal::TextureDataFormat::Rgba
                         : opal::TextureDataFormat::Rgb;
    else
        dataFormat = channels == 4
                         ? opal::TextureDataFormat::Rgba
                         : opal::TextureDataFormat::Red;

    auto opalTexture = opal::Texture::create(opal::TextureType::TextureCubeMap,
                                             format, width, height);

    {
        unsigned long long facePixelCount =
            static_cast<unsigned long long>(width) *
            static_cast<unsigned long long>(height);
        if (facePixelCount > 0 && channels >= 1) {
            glm::dvec3 faceSum(0.0);
            if (channels >= 3) {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    unsigned char* p = firstData + (pixel * channels);
                    faceSum.x += p[0];
                    faceSum.y += p[1];
                    faceSum.z += p[2];
                }
            }
            else {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    faceSum += glm::dvec3(firstData[pixel]);
                }
            }
            accumulatedColor += faceSum;
            accumulatedPixels += facePixelCount;
        }
        opalTexture->updateFace(0, firstData, width, height, dataFormat);
        stbi_image_free(firstData);
    }

    for (size_t i = 1; i < 6; i++) {
        Resource& resource = group.resources[i];
        if (resource.type != ResourceType::Image) {
            throw std::runtime_error("Resource is not an image: " +
                resource.name);
        }

        int w, h, c;
        unsigned char* data =
            stbi_load(resource.path.string().c_str(), &w, &h, &c, 0);
        if (!data) {
            throw std::runtime_error("Failed to load image: " +
                resource.path.string());
        }
        if (w != width || h != height || c != channels) {
            stbi_image_free(data);
            throw std::runtime_error("All cubemap images must have the same "
                "dimensions and channels");
        }

        unsigned long long facePixelCount = static_cast<unsigned long long>(w) *
            static_cast<unsigned long long>(h);
        if (facePixelCount > 0) {
            glm::dvec3 faceSum(0.0);
            if (c >= 3) {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    unsigned char* p = data + (pixel * c);
                    faceSum.x += p[0];
                    faceSum.y += p[1];
                    faceSum.z += p[2];
                }
            }
            else if (c == 1) {
                for (unsigned long long pixel = 0; pixel < facePixelCount;
                     ++pixel) {
                    faceSum += glm::dvec3(data[pixel]);
                }
            }
            accumulatedColor += faceSum;
            accumulatedPixels += facePixelCount;
        }

        opalTexture->updateFace(static_cast<int>(i), data, w, h, dataFormat);
        stbi_image_free(data);
    }

    opalTexture->setParameters3D(
        opal::TextureWrapMode::ClampToEdge, opal::TextureWrapMode::ClampToEdge,
        opal::TextureWrapMode::ClampToEdge, opal::TextureFilterMode::Linear,
        opal::TextureFilterMode::Linear);

    TextureCreationData creationData{.width = width, .height = height, .channels = channels};
    Cubemap cubemap;
    cubemap.creationData = creationData;
    cubemap.id = opalTexture->textureID;
    cubemap.texture = opalTexture;
    std::array<Resource, 6> resources;
    std::ranges::copy(group.resources,
                      resources.begin());
    cubemap.resources = resources;
    if (accumulatedPixels > 0) {
        glm::dvec3 normalized =
            accumulatedColor / (static_cast<double>(accumulatedPixels) * 255.0);
        cubemap.averageColor = {
            .r = static_cast<float>(normalized.x),
            .g = static_cast<float>(normalized.y),
            .b = static_cast<float>(normalized.z),
            .a = 1.0f
        };
        cubemap.hasAverageColor = true;
    }
    return cubemap;
}

Cubemap Cubemap::fromColors(const std::array<Color, 6>& colors, int size) {
    if (size <= 0) {
        throw std::runtime_error("Cubemap size must be positive");
    }

    auto opalTexture =
        opal::Texture::create(opal::TextureType::TextureCubeMap,
                              opal::TextureFormat::Rgba8, size, size);

    glm::dvec3 accumulatedColor(0.0);
    unsigned long long accumulatedPixels = 0;
    std::vector<unsigned char> faceData;
    for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
        cubemapFillFaceData(colors, faceIndex, size, faceData,
                            accumulatedColor);

        opalTexture->updateFace(faceIndex, faceData.data(), size, size,
                                opal::TextureDataFormat::Rgba);

        accumulatedPixels += static_cast<unsigned long long>(size) *
            static_cast<unsigned long long>(size);
    }

    opalTexture->setParameters3D(
        opal::TextureWrapMode::ClampToEdge, opal::TextureWrapMode::ClampToEdge,
        opal::TextureWrapMode::ClampToEdge, opal::TextureFilterMode::Linear,
        opal::TextureFilterMode::Linear);

    TextureCreationData creationData{.width = size, .height = size, .channels = 4};
    Cubemap cubemap;
    cubemap.creationData = creationData;
    cubemap.id = opalTexture->textureID;
    cubemap.texture = opalTexture;

    if (accumulatedPixels > 0) {
        glm::dvec3 normalized =
            accumulatedColor / static_cast<double>(accumulatedPixels);
        cubemap.averageColor = {
            .r = static_cast<float>(normalized.x),
            .g = static_cast<float>(normalized.y),
            .b = static_cast<float>(normalized.z),
            .a = 1.0f
        };
        cubemap.hasAverageColor = true;
    }

    return cubemap;
}

void Cubemap::updateWithColors(const std::array<Color, 6>& colors) {
    if (id == 0 || texture == nullptr) {
        throw std::runtime_error("Cubemap is not initialized");
    }
    if (creationData.width <= 0 || creationData.height <= 0) {
        throw std::runtime_error("Cubemap has invalid dimensions for update");
    }

    int size = creationData.width;

    glm::dvec3 accumulatedColor(0.0);
    unsigned long long accumulatedPixels = 0;
    std::vector<unsigned char> faceData;

    for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
        cubemapFillFaceData(colors, faceIndex, size, faceData,
                            accumulatedColor);

        texture->updateFace(faceIndex, faceData.data(), size, size,
                            opal::TextureDataFormat::Rgba);

        accumulatedPixels += static_cast<unsigned long long>(size) *
            static_cast<unsigned long long>(size);
    }

    if (accumulatedPixels > 0) {
        glm::dvec3 normalized =
            accumulatedColor / static_cast<double>(accumulatedPixels);
        averageColor = {
            .r = static_cast<float>(normalized.x),
            .g = static_cast<float>(normalized.y),
            .b = static_cast<float>(normalized.z),
            .a = 1.0f
        };
        hasAverageColor = true;
    }
    else {
        hasAverageColor = false;
    }
}

void Skybox::display(Window& window) {
    CoreObject obj;

    std::vector<CoreVertex> vertices = {
        // Positions (x, y, z)
        CoreVertex({-1.0f, 1.0f, -1.0f}), // 0
        CoreVertex({-1.0f, -1.0f, -1.0f}), // 1
        CoreVertex({1.0f, -1.0f, -1.0f}), // 2
        CoreVertex({1.0f, 1.0f, -1.0f}), // 3
        CoreVertex({-1.0f, 1.0f, 1.0f}), // 4
        CoreVertex({-1.0f, -1.0f, 1.0f}), // 5
        CoreVertex({1.0f, -1.0f, 1.0f}), // 6
        CoreVertex({1.0f, 1.0f, 1.0f}) // 7
    };

    std::vector<Index> indices = {
        // Back face
        0, 1, 2, 2, 3, 0,
        // Front face
        4, 7, 6, 6, 5, 4,
        // Left face
        4, 5, 1, 1, 0, 4,
        // Right face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        1, 5, 6, 6, 2, 1,
        // Top face
        4, 0, 3, 3, 7, 4
    };

    obj.attachVertices(vertices);
    obj.attachIndices(indices);

    VertexShader vertexShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Skybox);
    FragmentShader fragmentShader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Skybox);

    obj.createAndAttachProgram(vertexShader, fragmentShader);

    obj.initialize();

    this->object = std::make_shared<CoreObject>(obj);

    window.addPreludeObject(this);
}

void Skybox::hide() const {
    if (object != nullptr) {
        this->object->hide();
    }
}

void Skybox::show() const {
    if (object != nullptr) {
        this->object->show();
    }
}

void Skybox::setViewMatrix(const glm::mat4& newView) {
    this->view = glm::mat4(glm::mat3(newView));
    if (object != nullptr) {
        object->setViewMatrix(this->view);
    }
}

void Skybox::setProjectionMatrix(const glm::mat4& newProjection) {
    this->projection = newProjection;
    if (object != nullptr) {
        object->setProjectionMatrix(this->projection);
    }
}

void Skybox::render(float, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool) {
    if (!object || !object->isVisible) {
        return;
    }
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "Skybox::render requires a valid command buffer");
    }

    CoreObject* obj = this->object.get();

    auto pipelineDesc = opal::Pipeline::create();

    if (Window::mainWindow) {
        Window& window = *Window::mainWindow;
        int viewportWidth = window.viewportWidth;
        int viewportHeight = window.viewportHeight;
        if (viewportWidth == 0 || viewportHeight == 0) {
            Size2d size = window.getSize();
            viewportWidth = static_cast<int>(size.width);
            viewportHeight = static_cast<int>(size.height);
        }

        pipelineDesc->setViewport(window.viewportX, window.viewportY,
                                  viewportWidth, viewportHeight);
        pipelineDesc->setPrimitiveStyle(window.primitiveStyle);
        pipelineDesc->setRasterizerMode(window.rasterizerMode);
        pipelineDesc->setFrontFace(window.frontFace);
        pipelineDesc->enableBlending(window.useBlending);
        pipelineDesc->setBlendFunc(window.srcBlend, window.dstBlend);
        pipelineDesc->enableMultisampling(window.useMultisampling);
    }

    pipelineDesc->setDepthCompareOp(opal::CompareOp::LessEqual);
    pipelineDesc->enableDepthWrite(false);
    pipelineDesc->setCullMode(opal::CullMode::None);
    pipelineDesc->enableDepthTest(true);

    auto pipeline = obj->shaderProgram.requestPipeline(pipelineDesc);
    obj->setPipeline(pipeline);

    pipeline->bind();

    pipeline->setUniformMat4f("view", view);
    pipeline->setUniformMat4f("projection", projection);
    pipeline->bindTextureCubemap("skybox", cubemap.id, 0, obj->id);

    if (!Window::mainWindow || !Window::mainWindow->getCurrentScene()) {
        throw std::runtime_error(
            "Skybox rendering requires a valid main window and scene");
    }
    if (Window::mainWindow->getCurrentScene()->atmosphere.isEnabled()) {
        Magnitude3d sunDirection =
            Window::mainWindow->getCurrentScene()->atmosphere.getSunAngle();
        Magnitude3d moonDirection =
            Window::mainWindow->getCurrentScene()->atmosphere.getMoonAngle();
        pipeline->setUniform3f("sunDirection", sunDirection.x, sunDirection.y,
                               sunDirection.z);
        pipeline->setUniform3f("moonDirection", moonDirection.x,
                               moonDirection.y, moonDirection.z);

        Color sunColor =
            Window::mainWindow->getCurrentScene()->atmosphere.sunColor;
        pipeline->setUniform4f("sunColor", sunColor.r, sunColor.g, sunColor.b,
                               sunColor.a);
        Color moonColor =
            Window::mainWindow->getCurrentScene()->atmosphere.moonColor;
        pipeline->setUniform4f("moonColor", moonColor.r, moonColor.g,
                               moonColor.b, moonColor.a);

        pipeline->setUniform1f(
            "sunTintStrength",
            Window::mainWindow->getCurrentScene()->atmosphere.sunTintStrength);
        pipeline->setUniform1f(
            "moonTintStrength",
            Window::mainWindow->getCurrentScene()->atmosphere.moonTintStrength);
        pipeline->setUniform1f(
            "sunSizeMultiplier",
            Window::mainWindow->getCurrentScene()->atmosphere.sunSize);
        pipeline->setUniform1f(
            "moonSizeMultiplier",
            Window::mainWindow->getCurrentScene()->atmosphere.moonSize);
        pipeline->setUniform1f(
            "starDensity",
            Window::mainWindow->getCurrentScene()->atmosphere.starIntensity);
        pipeline->setUniform1i("hasDayNight", 1);
    }
    else {
        pipeline->setUniform1i("hasDayNight", 0);
    }

    commandBuffer->bindDrawingState(obj->vao);
    commandBuffer->bindPipeline(pipeline); // Required for Vulkan
    commandBuffer->drawIndexed(static_cast<unsigned int>(obj->indices.size()),
                               1, 0, 0, 0, obj->id);
    commandBuffer->unbindDrawingState();

    if (TracerServices::getInstance().isOk()) {
        DebugObjectPacket debugPacket;
        debugPacket.drawCallsForObject = 1;
        debugPacket.frameCount =
            Window::mainWindow != nullptr && Window::mainWindow->device != nullptr
                ? Window::mainWindow->device->frameCount
                : 0;
        debugPacket.triangleCount =
            static_cast<unsigned int>(obj->indices.size()) / 3;
        debugPacket.vertexBufferSizeMb =
            static_cast<float>(sizeof(CoreVertex) * obj->vertices.size()) /
            (1024.0f * 1024.0f);
        debugPacket.indexBufferSizeMb =
            static_cast<float>(sizeof(Index) * obj->indices.size()) /
            (1024.0f * 1024.0f);
        debugPacket.textureCount = 1;
        debugPacket.materialCount = 0;
        debugPacket.objectType = DebugObjectType::Terrain;
        debugPacket.objectId = obj->id;
        debugPacket.send();
    }
}
