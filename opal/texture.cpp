//
// texture.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Texture support for Opal
// Copyright (c) 2025 maxvdec
//

#include "atlas/tracer/data.h"
#include "opal/opal.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sys/types.h>
#ifdef METAL
#include "metal_state.h"
#endif

namespace opal {

Texture::~Texture() {
#ifdef METAL
    metal::releaseTextureState(this);
#endif
}

namespace {

#ifdef OPENGL
constexpr GLenum glInternalFormatTable[] = {GL_RGBA8,
                                            GL_SRGB8_ALPHA8,
                                            GL_RGB8,
                                            GL_SRGB8,
                                            GL_RGBA16F,
                                            GL_RGB16F,
                                            GL_DEPTH24_STENCIL8,
                                            GL_DEPTH_COMPONENT24,
                                            GL_DEPTH_COMPONENT32F,
                                            GL_R8,
                                            GL_R16F};

constexpr GLenum glDataFormatTable[] = {
    GL_RGBA, GL_RGB, GL_RED, GL_BGR, GL_BGRA, GL_DEPTH_COMPONENT,
};

constexpr GLenum glTextureTypeTable[] = {GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP,
                                         GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY,
                                         GL_TEXTURE_2D_MULTISAMPLE};

constexpr GLenum glWrapModeTable[] = {GL_REPEAT, GL_MIRRORED_REPEAT,
                                      GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER};

constexpr GLenum glFilterModeTable[] = {
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR};

inline GLenum getGLInternalFormat(TextureFormat format) {
    return glInternalFormatTable[static_cast<int>(format)];
}

inline GLenum getGLDataFormat(TextureDataFormat format) {
    return glDataFormatTable[static_cast<int>(format)];
}

inline GLenum getGLTextureType(TextureType type) {
    return glTextureTypeTable[static_cast<int>(type)];
}

inline GLenum getGLWrapMode(TextureWrapMode mode) {
    return glWrapModeTable[static_cast<int>(mode)];
}

inline GLenum getGLFilterMode(TextureFilterMode mode) {
    return glFilterModeTable[static_cast<int>(mode)];
}

#endif

inline float calculateTextureSizeMb(TextureFormat format, int width, int height,
                                    int depth = 1) {
    size_t bytesPerPixel = 0;

    switch (format) {
    case TextureFormat::Rgba8:
        bytesPerPixel = 4;
        break;
    case TextureFormat::Rgb8:
        bytesPerPixel = 3;
        break;
    case TextureFormat::Rgba16F:
        bytesPerPixel = 8;
        break;
    case TextureFormat::Rgb16F:
        bytesPerPixel = 6;
        break;
    case TextureFormat::Depth24Stencil8:
        bytesPerPixel = 4;
        break;
    case TextureFormat::DepthComponent24:
        bytesPerPixel = 3;
        break;
    case TextureFormat::Depth32F:
        bytesPerPixel = 4;
        break;
    case TextureFormat::Red8:
        bytesPerPixel = 1;
        break;
    case TextureFormat::Red16F:
        bytesPerPixel = 2;
        break;
    default:
        bytesPerPixel = 4;
        break;
    }

    size_t totalBytes =
        static_cast<size_t>(width) * height * depth * bytesPerPixel;
    return totalBytes / (1024.0f * 1024.0f);
}

#ifdef METAL
struct MetalUploadBuffer {
    std::vector<uint8_t> converted;
    const void *bytes = nullptr;
    NS::UInteger bytesPerRow = 0;
    NS::UInteger bytesPerImage = 0;
};

inline size_t metalDataChannels(TextureDataFormat format) {
    switch (format) {
    case TextureDataFormat::Rgba:
        return 4;
    case TextureDataFormat::Rgb:
        return 3;
    case TextureDataFormat::Red:
        return 1;
    case TextureDataFormat::DepthComponent:
        return 1;
    default:
        return 4;
    }
}

inline size_t metalDestinationChannels(TextureFormat format) {
    switch (format) {
    case TextureFormat::Red8:
    case TextureFormat::Red16F:
    case TextureFormat::DepthComponent24:
    case TextureFormat::Depth32F:
        return 1;
    case TextureFormat::Depth24Stencil8:
        return 2;
    default:
        return 4;
    }
}

inline size_t metalSourceBytesPerChannel(TextureFormat textureFormat,
                                         TextureDataFormat dataFormat) {
    if (dataFormat == TextureDataFormat::DepthComponent) {
        return 4;
    }
    switch (textureFormat) {
    case TextureFormat::Rgba16F:
    case TextureFormat::Rgb16F:
    case TextureFormat::Red16F:
        return 4;
    default:
        return 1;
    }
}

inline size_t metalDestinationBytesPerChannel(TextureFormat textureFormat) {
    switch (textureFormat) {
    case TextureFormat::Rgba16F:
    case TextureFormat::Rgb16F:
    case TextureFormat::Red16F:
        return 2;
    case TextureFormat::DepthComponent24:
    case TextureFormat::Depth32F:
        return 4;
    default:
        return 1;
    }
}

inline uint16_t metalFloatToHalf(float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));

    uint32_t sign = (bits >> 16) & 0x8000u;
    uint32_t mantissa = bits & 0x007fffffu;
    int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xffu) - 127 + 15;

    if (exponent <= 0) {
        if (exponent < -10) {
            return static_cast<uint16_t>(sign);
        }
        mantissa = (mantissa | 0x00800000u) >> (1 - exponent);
        if ((mantissa & 0x00001000u) != 0u) {
            mantissa += 0x00002000u;
        }
        return static_cast<uint16_t>(sign | (mantissa >> 13));
    }

    if (exponent >= 31) {
        if (mantissa == 0u) {
            return static_cast<uint16_t>(sign | 0x7c00u);
        }
        mantissa >>= 13;
        return static_cast<uint16_t>(sign | 0x7c00u | mantissa |
                                     (mantissa == 0u));
    }

    if ((mantissa & 0x00001000u) != 0u) {
        mantissa += 0x00002000u;
        if ((mantissa & 0x00800000u) != 0u) {
            mantissa = 0u;
            exponent += 1;
            if (exponent >= 31) {
                return static_cast<uint16_t>(sign | 0x7c00u);
            }
        }
    }

    return static_cast<uint16_t>(
        sign | (static_cast<uint32_t>(exponent) << 10) | (mantissa >> 13));
}

inline float metalReadChannel(const uint8_t *src, size_t bytesPerChannel) {
    if (bytesPerChannel == 1) {
        return static_cast<float>(src[0]) / 255.0f;
    }
    if (bytesPerChannel == 4) {
        float value = 0.0f;
        std::memcpy(&value, src, sizeof(float));
        return value;
    }
    return 0.0f;
}

inline void metalWriteChannel(uint8_t *dst, size_t bytesPerChannel,
                              float value) {
    if (bytesPerChannel == 1) {
        float clamped = std::clamp(value, 0.0f, 1.0f);
        dst[0] = static_cast<uint8_t>(std::lroundf(clamped * 255.0f));
        return;
    }
    if (bytesPerChannel == 2) {
        uint16_t half = metalFloatToHalf(value);
        std::memcpy(dst, &half, sizeof(uint16_t));
        return;
    }
    if (bytesPerChannel == 4) {
        std::memcpy(dst, &value, sizeof(float));
    }
}

MetalUploadBuffer prepareMetalUpload(const void *data, int width, int height,
                                     int depth, TextureFormat textureFormat,
                                     TextureDataFormat dataFormat) {
    MetalUploadBuffer upload{};
    if (data == nullptr || width <= 0 || height <= 0 || depth <= 0) {
        return upload;
    }

    size_t srcChannels = metalDataChannels(dataFormat);
    size_t dstChannels = metalDestinationChannels(textureFormat);
    size_t srcBytesPerChannel =
        metalSourceBytesPerChannel(textureFormat, dataFormat);
    size_t dstBytesPerChannel = metalDestinationBytesPerChannel(textureFormat);
    size_t texelCount = static_cast<size_t>(width) *
                        static_cast<size_t>(height) *
                        static_cast<size_t>(depth);

    size_t srcBytesPerTexel = srcChannels * srcBytesPerChannel;
    size_t dstBytesPerTexel = dstChannels * dstBytesPerChannel;
    size_t srcRowBytes = static_cast<size_t>(width) * srcBytesPerTexel;
    size_t dstRowBytes = static_cast<size_t>(width) * dstBytesPerTexel;

    if (srcChannels == dstChannels &&
        srcBytesPerChannel == dstBytesPerChannel) {
        upload.bytes = data;
        upload.bytesPerRow = static_cast<NS::UInteger>(srcRowBytes);
        upload.bytesPerImage = static_cast<NS::UInteger>(
            srcRowBytes * static_cast<size_t>(height));
        return upload;
    }

    upload.converted.resize(texelCount * dstBytesPerTexel);
    const uint8_t *srcBytes = static_cast<const uint8_t *>(data);
    uint8_t *dstBytes = upload.converted.data();

    for (size_t texel = 0; texel < texelCount; ++texel) {
        const uint8_t *srcTexel = srcBytes + (texel * srcBytesPerTexel);
        uint8_t *dstTexel = dstBytes + (texel * dstBytesPerTexel);

        for (size_t channel = 0; channel < dstChannels; ++channel) {
            float value = 0.0f;
            if (channel < srcChannels) {
                value =
                    metalReadChannel(srcTexel + (channel * srcBytesPerChannel),
                                     srcBytesPerChannel);
            } else if (channel == 3) {
                value = 1.0f;
            }
            metalWriteChannel(dstTexel + (channel * dstBytesPerChannel),
                              dstBytesPerChannel, value);
        }
    }

    upload.bytes = upload.converted.data();
    upload.bytesPerRow = static_cast<NS::UInteger>(dstRowBytes);
    upload.bytesPerImage =
        static_cast<NS::UInteger>(dstRowBytes * static_cast<size_t>(height));
    return upload;
}
#endif

} // namespace

std::shared_ptr<Texture> Texture::create(TextureType type, TextureFormat format,
                                         int width, int height,
                                         TextureDataFormat dataFormat,
                                         const void *data, uint mipLevels) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = type;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    const GLenum textureType = getGLTextureType(type);
    const GLenum glFormat = getGLInternalFormat(format);
    const GLenum glDataFmt = getGLDataFormat(dataFormat);

    texture->glType = textureType;
    texture->glFormat = glFormat;

    // For single-channel textures, save and set unpack alignment to 1
    GLint previousAlignment = 4;
    bool needsAlignmentFix = (dataFormat == TextureDataFormat::Red);
    if (needsAlignmentFix) {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glGenTextures(1, &texture->textureID);
    glBindTexture(textureType, texture->textureID);

    if (textureType == GL_TEXTURE_2D && width > 0 && height > 0) {
        GLenum dataType = GL_UNSIGNED_BYTE;
        if (format == TextureFormat::Rgba16F ||
            format == TextureFormat::Rgb16F ||
            format == TextureFormat::Red16F) {
            dataType = GL_FLOAT;
        }
        glTexImage2D(textureType, 0, glFormat, width, height, 0, glDataFmt,
                     dataType, data);
        if (mipLevels > 1 && data != nullptr) {
            glGenerateMipmap(textureType);
        }
    } else if (textureType == GL_TEXTURE_CUBE_MAP && width > 0 && height > 0) {
        for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glFormat, width,
                         height, 0, glDataFmt, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    // Restore previous alignment
    if (needsAlignmentFix) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
    }

    return texture;
#elif defined(VULKAN)
    return Texture::createVulkan(type, format, width, height, dataFormat, data,
                                 mipLevels);
#elif defined(METAL)
    if (Device::globalInstance == nullptr) {
        throw std::runtime_error("Cannot create Metal texture without device");
    }

    auto &deviceState = metal::deviceState(Device::globalInstance);
    if (deviceState.device == nullptr) {
        throw std::runtime_error("Metal device is not initialized");
    }

    auto texture = std::make_shared<Texture>();
    texture->type = type;
    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->samples = (type == TextureType::Texture2DMultisample)
                           ? static_cast<int>(mipLevels)
                           : 1;

    auto &state = metal::textureState(texture.get());
    state.type = type;
    state.format = format;
    state.dataFormat = dataFormat;
    state.width = width;
    state.height = height;
    state.depth = 1;
    state.samples = texture->samples;

    MTL::TextureDescriptor *descriptor =
        MTL::TextureDescriptor::alloc()->init();
    descriptor->setTextureType(metal::textureTypeToMetal(type));
    descriptor->setPixelFormat(metal::textureFormatToPixelFormat(format));
    descriptor->setWidth(static_cast<NS::UInteger>(std::max(width, 1)));
    descriptor->setHeight(static_cast<NS::UInteger>(std::max(height, 1)));
    descriptor->setDepth(1);
    descriptor->setMipmapLevelCount(
        static_cast<NS::UInteger>(std::max<uint>(1, mipLevels)));
    descriptor->setUsage(metal::textureUsageFor(type, format));
    descriptor->setStorageMode(MTL::StorageModeShared);

    if (type == TextureType::TextureCubeMap) {
        descriptor->setTextureType(MTL::TextureTypeCube);
        descriptor->setArrayLength(1);
    } else if (type == TextureType::Texture2DArray) {
        descriptor->setTextureType(MTL::TextureType2DArray);
        descriptor->setArrayLength(1);
    } else if (type == TextureType::Texture2DMultisample) {
        descriptor->setTextureType(MTL::TextureType2DMultisample);
        descriptor->setSampleCount(
            static_cast<NS::UInteger>(std::max(1, texture->samples)));
        descriptor->setMipmapLevelCount(1);
    }

    state.texture = deviceState.device->newTexture(descriptor);
    descriptor->release();
    if (state.texture == nullptr) {
        throw std::runtime_error("Failed to create Metal texture");
    }

    metal::rebuildTextureSampler(texture.get(), deviceState.device);
    state.handle = metal::registerTextureHandle(texture);
    texture->textureID = state.handle;

    if (data != nullptr && width > 0 && height > 0 &&
        type != TextureType::Texture2DMultisample &&
        type != TextureType::TextureCubeMap) {
        texture->updateData(data, width, height, dataFormat);
    }

    return texture;
#else
    return nullptr;
#endif

    ResourceEventInfo info;
    info.callerObject = "-1";
    info.resourceType = DebugResourceType::Texture;
    info.operation = DebugResourceOperation::Created;
    info.frameNumber = Device::globalInstance->frameCount;
    info.sizeMb = calculateTextureSizeMb(format, width, height);
    info.send();
}

void Texture::updateFace(int faceIndex, const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    if (this->width == width && this->height == height) {
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, 0, 0,
                        width, height, glDataFmt, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, glFormat,
                     width, height, 0, glDataFmt, GL_UNSIGNED_BYTE, data);
    }
#elif defined(VULKAN)
    if (data == nullptr || width <= 0 || height <= 0 ||
        vkImage == VK_NULL_HANDLE) {
        return;
    }

    size_t bytesPerPixel = 4;
    if (dataFormat == TextureDataFormat::Rgb) {
        bytesPerPixel = 3;
    } else if (dataFormat == TextureDataFormat::Red) {
        bytesPerPixel = 1;
    }

    size_t pixelCount = static_cast<size_t>(width) * height;
    VkDeviceSize imageSize = pixelCount * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Buffer::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingBufferMemory);

    void *mappedData;
    vkMapMemory(Device::globalDevice, stagingBufferMemory, 0, imageSize, 0,
                &mappedData);

    if (bytesPerPixel == 3) {
        const auto *src = static_cast<const uint8_t *>(data);
        auto *dst = static_cast<uint8_t *>(mappedData);
        for (size_t i = 0; i < pixelCount; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }
    } else if (bytesPerPixel == 1) {
        const auto *src = static_cast<const uint8_t *>(data);
        auto *dst = static_cast<uint8_t *>(mappedData);
        for (size_t i = 0; i < pixelCount; ++i) {
            dst[i * 4 + 0] = src[i];
            dst[i * 4 + 1] = src[i];
            dst[i * 4 + 2] = src[i];
            dst[i * 4 + 3] = 255;
        }
    } else {
        memcpy(mappedData, data, pixelCount * bytesPerPixel);
    }

    vkUnmapMemory(Device::globalDevice, stagingBufferMemory);

    VkFormat vkFormat = opalTextureFormatToVulkanFormat(format);

    if (currentLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        Framebuffer::transitionImageLayout(vkImage, vkFormat, currentLayout,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           6);
        currentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Device::globalInstance->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Device::globalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = static_cast<uint32_t>(faceIndex);
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {.x = 0, .y = 0, .z = 0};
    region.imageExtent = {.width = static_cast<uint32_t>(width),
                          .height = static_cast<uint32_t>(height),
                          .depth = 1};

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(Device::globalInstance->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(Device::globalInstance->graphicsQueue);

    vkFreeCommandBuffers(Device::globalDevice,
                         Device::globalInstance->commandPool, 1,
                         &commandBuffer);

    if (faceIndex == 5) {
        Framebuffer::transitionImageLayout(
            vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);
        currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    vkDestroyBuffer(Device::globalDevice, stagingBuffer, nullptr);
    vkFreeMemory(Device::globalDevice, stagingBufferMemory, nullptr);
#elif defined(METAL)
    auto &state = metal::textureState(this);
    if (state.texture == nullptr || data == nullptr || width <= 0 ||
        height <= 0 || type != TextureType::TextureCubeMap) {
        return;
    }

    MetalUploadBuffer upload =
        prepareMetalUpload(data, width, height, 1, format, dataFormat);
    if (upload.bytes == nullptr || upload.bytesPerRow == 0) {
        return;
    }

    MTL::Region region =
        MTL::Region::Make2D(0, 0, static_cast<NS::UInteger>(width),
                            static_cast<NS::UInteger>(height));
    state.texture->replaceRegion(
        region, 0, static_cast<NS::UInteger>(faceIndex), upload.bytes,
        upload.bytesPerRow, upload.bytesPerImage);
#endif

    ResourceEventInfo info;
    info.callerObject = "-1";
    info.resourceType = DebugResourceType::Texture;
    info.operation = DebugResourceOperation::Created;
    info.frameNumber = Device::globalInstance->frameCount;
    info.sizeMb = calculateTextureSizeMb(format, width, height);
    info.send();
}

void Texture::updateData3D(const void *data, int width, int height, int depth,
                           TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0, glDataFmt,
                 GL_UNSIGNED_BYTE, data);
#elif defined(VULKAN)
    (void)data;
    (void)width;
    (void)height;
    (void)depth;
    (void)dataFormat;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    if (state.texture == nullptr || data == nullptr || width <= 0 ||
        height <= 0 || depth <= 0) {
        return;
    }

    MetalUploadBuffer upload =
        prepareMetalUpload(data, width, height, depth, format, dataFormat);
    if (upload.bytes == nullptr || upload.bytesPerRow == 0 ||
        upload.bytesPerImage == 0) {
        return;
    }

    MTL::Region region = MTL::Region::Make3D(
        0, 0, 0, static_cast<NS::UInteger>(width),
        static_cast<NS::UInteger>(height), static_cast<NS::UInteger>(depth));
    state.texture->replaceRegion(region, 0, 0, upload.bytes, upload.bytesPerRow,
                                 upload.bytesPerImage);
    state.depth = depth;
#endif
}

#ifdef METAL
MTL::Texture *Texture::getMetalTexture() const {
    return metal::textureState(const_cast<Texture *>(this)).texture;
}
#endif

void Texture::updateData(const void *data, int width, int height,
                         TextureDataFormat dataFormat) {
#ifdef OPENGL
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    GLenum dataType = GL_UNSIGNED_BYTE;
    if (format == TextureFormat::Rgba16F || format == TextureFormat::Rgb16F ||
        format == TextureFormat::Red16F) {
        dataType = GL_FLOAT;
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (this->width == width && this->height == height) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, glDataFmt,
                        dataType, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glDataFmt,
                     dataType, data);
        this->width = width;
        this->height = height;
    }
#elif defined(VULKAN)
    (void)data;
    (void)width;
    (void)height;
    (void)dataFormat;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    if (state.texture == nullptr || data == nullptr || width <= 0 ||
        height <= 0) {
        return;
    }

    MetalUploadBuffer upload =
        prepareMetalUpload(data, width, height, 1, format, dataFormat);
    if (upload.bytes == nullptr || upload.bytesPerRow == 0) {
        return;
    }

    MTL::Region region =
        MTL::Region::Make2D(0, 0, static_cast<NS::UInteger>(width),
                            static_cast<NS::UInteger>(height));
    state.texture->replaceRegion(region, 0, upload.bytes, upload.bytesPerRow);
    this->width = width;
    this->height = height;
    state.width = width;
    state.height = height;
#endif
}

void Texture::changeFormat(TextureFormat newFormat) {
#ifdef OPENGL
    this->format = newFormat;
    this->glFormat = getGLInternalFormat(newFormat);
#elif defined(VULKAN)
    this->format = newFormat;
#elif defined(METAL)
    this->format = newFormat;
    metal::textureState(this).format = newFormat;
#endif
}

void Texture::readData(void *buffer, TextureDataFormat dataFormat) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLenum glDataFormat;
    GLenum glDataType = GL_UNSIGNED_BYTE;
    switch (dataFormat) {
    case TextureDataFormat::Rgba:
        glDataFormat = GL_RGBA;
        break;
    case TextureDataFormat::Rgb:
        glDataFormat = GL_RGB;
        break;
    case TextureDataFormat::Red:
        glDataFormat = GL_RED;
        break;
    case TextureDataFormat::DepthComponent:
        glDataFormat = GL_DEPTH_COMPONENT;
        glDataType = GL_FLOAT;
        break;
    default:
        glDataFormat = GL_RGBA;
        break;
    }
    glGetTexImage(this->glType, 0, glDataFormat, glDataType, buffer);
#elif defined(VULKAN)
    (void)buffer;
    (void)dataFormat;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    if (state.texture == nullptr || buffer == nullptr || width <= 0 ||
        height <= 0) {
        return;
    }

    size_t bytesPerPixel = 4;
    if (dataFormat == TextureDataFormat::Rgb) {
        bytesPerPixel = 3;
    } else if (dataFormat == TextureDataFormat::Red) {
        bytesPerPixel = 1;
    } else if (dataFormat == TextureDataFormat::DepthComponent) {
        bytesPerPixel = 4;
    }

    MTL::Region region =
        MTL::Region::Make2D(0, 0, static_cast<NS::UInteger>(width),
                            static_cast<NS::UInteger>(height));
    size_t rowBytes = static_cast<size_t>(width) * bytesPerPixel;
    NS::UInteger bytesPerRow = static_cast<NS::UInteger>(rowBytes);
    state.texture->getBytes(buffer, bytesPerRow, region, 0);
#endif
}

void Texture::generateMipmaps([[maybe_unused]] uint levels) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#elif defined(VULKAN)
#elif defined(METAL)
    if (Device::globalInstance == nullptr) {
        return;
    }
    auto &deviceState = metal::deviceState(Device::globalInstance);
    auto &state = metal::textureState(this);
    if (deviceState.queue == nullptr || state.texture == nullptr) {
        return;
    }
    MTL::CommandBuffer *commandBuffer = deviceState.queue->commandBuffer();
    MTL::BlitCommandEncoder *blit = commandBuffer->blitCommandEncoder();
    blit->generateMipmaps(state.texture);
    blit->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
#endif
}

void Texture::automaticallyGenerateMipmaps() {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glGenerateMipmap(this->glType);
#elif defined(VULKAN)
#elif defined(METAL)
    generateMipmaps(0);
#endif
}

void Texture::setWrapMode(TextureAxis axis, TextureWrapMode mode) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    const GLenum glMode = getGLWrapMode(mode);
    static constexpr GLenum axisTable[] = {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
                                           GL_TEXTURE_WRAP_R};
    glTexParameteri(this->glType, axisTable[static_cast<int>(axis)], glMode);
#elif defined(VULKAN)
    (void)axis;
    (void)mode;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    if (axis == TextureAxis::S) {
        state.wrapS = mode;
    } else if (axis == TextureAxis::T) {
        state.wrapT = mode;
    } else {
        state.wrapR = mode;
    }
    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        metal::rebuildTextureSampler(this, deviceState.device);
    }
#endif
}

void Texture::changeBorderColor(const glm::vec4 &borderColor) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    GLfloat color[4] = {borderColor.r, borderColor.g, borderColor.b,
                        borderColor.a};
    glTexParameterfv(this->glType, GL_TEXTURE_BORDER_COLOR, color);
#elif defined(VULKAN)
    (void)borderColor;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    state.borderColor = borderColor;
    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        metal::rebuildTextureSampler(this, deviceState.device);
    }
#endif
}

void Texture::setFilterMode(TextureFilterMode minFilter,
                            TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#elif defined(VULKAN)
    (void)minFilter;
    (void)magFilter;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    state.minFilter = minFilter;
    state.magFilter = magFilter;
    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        metal::rebuildTextureSampler(this, deviceState.device);
    }
#endif
}

void Texture::setParameters(TextureWrapMode wrapS, TextureWrapMode wrapT,
                            TextureFilterMode minFilter,
                            TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_WRAP_S, getGLWrapMode(wrapS));
    glTexParameteri(glType, GL_TEXTURE_WRAP_T, getGLWrapMode(wrapT));
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#elif defined(VULKAN)
    (void)wrapS;
    (void)wrapT;
    (void)minFilter;
    (void)magFilter;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    state.wrapS = wrapS;
    state.wrapT = wrapT;
    state.minFilter = minFilter;
    state.magFilter = magFilter;
    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        metal::rebuildTextureSampler(this, deviceState.device);
    }
#endif
}

void Texture::setParameters3D(TextureWrapMode wrapS, TextureWrapMode wrapT,
                              TextureWrapMode wrapR,
                              TextureFilterMode minFilter,
                              TextureFilterMode magFilter) {
#ifdef OPENGL
    glBindTexture(this->glType, textureID);
    glTexParameteri(glType, GL_TEXTURE_WRAP_S, getGLWrapMode(wrapS));
    glTexParameteri(glType, GL_TEXTURE_WRAP_T, getGLWrapMode(wrapT));
    glTexParameteri(glType, GL_TEXTURE_WRAP_R, getGLWrapMode(wrapR));
    glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, getGLFilterMode(minFilter));
    glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, getGLFilterMode(magFilter));
#elif defined(VULKAN)
    (void)wrapS;
    (void)wrapT;
    (void)wrapR;
    (void)minFilter;
    (void)magFilter;
#elif defined(METAL)
    auto &state = metal::textureState(this);
    state.wrapS = wrapS;
    state.wrapT = wrapT;
    state.wrapR = wrapR;
    state.minFilter = minFilter;
    state.magFilter = magFilter;
    if (Device::globalInstance != nullptr) {
        auto &deviceState = metal::deviceState(Device::globalInstance);
        metal::rebuildTextureSampler(this, deviceState.device);
    }
#endif
}

void Pipeline::bindTexture(const std::string &name,
                           const std::shared_ptr<Texture> &texture,
                           [[maybe_unused]] int unit, int callerId) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(texture->glType, texture->textureID);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
#elif defined(VULKAN)
    if (!shaderProgram || texture == nullptr) {
        return;
    }
    const UniformBindingInfo *info = shaderProgram->findUniform(name);
    if (info == nullptr || !info->isSampler) {
        return;
    }

    ensureDescriptorResources();
    if (descriptorSets.size() <= info->set ||
        descriptorSets[info->set] == VK_NULL_HANDLE) {
        return;
    }

    VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (texture->currentLayout != desiredLayout &&
        texture->currentLayout != VK_IMAGE_LAYOUT_GENERAL) {
        VkFormat vkFormat = opalTextureFormatToVulkanFormat(texture->format);
        bool isDepth = (texture->format == TextureFormat::Depth24Stencil8 ||
                        texture->format == TextureFormat::DepthComponent24 ||
                        texture->format == TextureFormat::Depth32F);

        if (!isDepth || texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            uint32_t layerCount =
                (texture->type == TextureType::TextureCubeMap) ? 6 : 1;
            Framebuffer::transitionImageLayout(texture->vkImage, vkFormat,
                                               texture->currentLayout,
                                               desiredLayout, layerCount);
            texture->currentLayout = desiredLayout;
        }
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture->vkSampler;
    imageInfo.imageView = texture->vkImageView;
    imageInfo.imageLayout =
        (texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : texture->currentLayout;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[info->set];
    write.dstBinding = info->binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(Device::globalDevice, 1, &write, 0, nullptr);
#elif defined(METAL)
    auto &state = metal::pipelineState(this);
    int resolvedUnit = unit;
    if (shaderProgram != nullptr) {
        auto &programState = metal::programState(shaderProgram.get());
        auto bindingIt = programState.textureBindings.find(name);
        if (bindingIt != programState.textureBindings.end()) {
            resolvedUnit = bindingIt->second;
        }
    }
    if (texture == nullptr) {
        state.texturesByUnit.erase(resolvedUnit);
        return;
    }
    state.texturesByUnit[resolvedUnit] = texture;
#endif

    if (texture == nullptr) {
        return;
    }

    ResourceEventInfo resourceInfo;
    resourceInfo.callerObject = std::to_string(callerId);
    resourceInfo.resourceType = DebugResourceType::Texture;
    resourceInfo.operation = DebugResourceOperation::Loaded;
    resourceInfo.frameNumber = Device::globalInstance->frameCount;
    resourceInfo.sizeMb = calculateTextureSizeMb(
        texture->format, texture->width, texture->height);
    resourceInfo.send();
}

#ifdef METAL
void Pipeline::bindTextureArray(
    const std::vector<std::shared_ptr<Texture>> &textures,
    uint32_t bufferIndex) {
    if (shaderProgram == nullptr || Device::globalInstance == nullptr) {
        return;
    }
    auto &state = metal::pipelineState(this);
    if (state.textureArgumentBuffer != nullptr &&
        state.textureArgumentBufferIndex == bufferIndex &&
        state.textureArgumentTextures == textures) {
        return;
    }
    auto &programState = metal::programState(shaderProgram.get());
    if (programState.computeFunction == nullptr) {
        return;
    }
    if (state.textureArgumentBuffer != nullptr) {
        state.textureArgumentBuffer->release();
        state.textureArgumentBuffer = nullptr;
    }
    if (state.textureArgumentEncoder != nullptr) {
        state.textureArgumentEncoder->release();
    }
    state.textureArgumentEncoder =
        programState.computeFunction->newArgumentEncoder(bufferIndex);
    if (state.textureArgumentEncoder == nullptr) {
        return;
    }
    auto &deviceState = metal::deviceState(Device::globalInstance);
    state.textureArgumentBuffer = deviceState.device->newBuffer(
        state.textureArgumentEncoder->encodedLength(),
        MTL::ResourceStorageModeShared);
    state.textureArgumentEncoder->setArgumentBuffer(
        state.textureArgumentBuffer, 0);
    for (size_t i = 0; i < textures.size(); ++i) {
        if (textures[i] == nullptr) {
            continue;
        }
        auto &textureState = metal::textureState(textures[i].get());
        state.textureArgumentEncoder->setTexture(textureState.texture, i);
    }
    state.textureArgumentBufferIndex = bufferIndex;
    state.textureArgumentTextures = textures;
}
#endif

void Pipeline::bindTexture2D(const std::string &name, uint textureId, int unit,
                             int callerId) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
    (void)callerId;
#elif defined(VULKAN)
    auto texture = Texture::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#elif defined(METAL)
    auto texture = metal::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#endif
}

void Pipeline::bindTexture3D(const std::string &name, uint textureId, int unit,
                             int callerId) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
    (void)callerId;

#elif defined(VULKAN)
    auto texture = Texture::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#elif defined(METAL)
    auto texture = metal::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#endif
}

void Pipeline::bindTextureCubemap(const std::string &name, uint textureId,
                                  int unit, int callerId) {
#ifdef OPENGL
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    int location = glGetUniformLocation(shaderProgram->programID, name.c_str());
    glUniform1i(location, unit);
    (void)callerId;
#elif defined(VULKAN)
    auto texture = Texture::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#elif defined(METAL)
    auto texture = metal::getTextureFromHandle(textureId);
    bindTexture(name, texture, unit, callerId);
#endif
}

std::shared_ptr<Texture> Texture::createMultisampled(TextureFormat format,
                                                     int width, int height,
                                                     int samples) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture2DMultisample;
    texture->format = format;
    texture->width = width;
    uint glFormat = getGLInternalFormat(format);
    texture->glFormat = glFormat;

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture->textureID);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, glFormat, width,
                            height, GL_TRUE);

    return texture;
#elif defined(VULKAN)
    return Texture::createMultisampledVulkan(format, width, height, samples);
#elif defined(METAL)
    return Texture::create(TextureType::Texture2DMultisample, format, width,
                           height, TextureDataFormat::Rgba, nullptr,
                           static_cast<uint>(samples));
#else
    return nullptr;
#endif
}

std::shared_ptr<Texture> Texture::createDepthCubemap(TextureFormat format,
                                                     int resolution) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::TextureCubeMap;
    texture->format = format;
    texture->width = resolution;
    texture->height = resolution;

    const GLenum glFormat = getGLInternalFormat(format);
    texture->glType = GL_TEXTURE_CUBE_MAP;
    texture->glFormat = glFormat;

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture->textureID);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glFormat,
                     resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                     nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texture;
#elif defined(VULKAN)
    return Texture::createDepthCubemapVulkan(format, resolution);
#elif defined(METAL)
    return Texture::create(TextureType::TextureCubeMap, format, resolution,
                           resolution, TextureDataFormat::DepthComponent,
                           nullptr, 1);
#else
    return nullptr;
#endif
}

std::shared_ptr<Texture> Texture::create3D(TextureFormat format, int width,
                                           int height, int depth,
                                           TextureDataFormat dataFormat,
                                           const void *data) {
#ifdef OPENGL
    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture3D;
    texture->format = format;
    texture->width = width;
    texture->height = height;

    const GLenum glFormat = getGLInternalFormat(format);
    const GLenum glDataFmt = getGLDataFormat(dataFormat);
    texture->glType = GL_TEXTURE_3D;
    texture->glFormat = glFormat;

    // Determine data type based on format
    GLenum dataType = GL_UNSIGNED_BYTE;
    if (format == TextureFormat::Rgba16F || format == TextureFormat::Rgb16F ||
        format == TextureFormat::Red16F) {
        dataType = GL_FLOAT;
    }

    // Save and restore previous state
    GLint previousAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &previousTexture);

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_3D, texture->textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D(GL_TEXTURE_3D, 0, glFormat, width, height, depth, 0, glDataFmt,
                 dataType, data);

    // Restore previous state
    glBindTexture(GL_TEXTURE_3D, previousTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

    return texture;
#elif defined(VULKAN)
    return Texture::create3DVulkan(format, width, height, depth, dataFormat,
                                   data);
#elif defined(METAL)
    if (Device::globalInstance == nullptr) {
        throw std::runtime_error(
            "Cannot create Metal 3D texture without device");
    }
    auto &deviceState = metal::deviceState(Device::globalInstance);
    if (deviceState.device == nullptr) {
        throw std::runtime_error("Metal device is not initialized");
    }

    auto texture = std::make_shared<Texture>();
    texture->type = TextureType::Texture3D;
    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->samples = 1;

    auto &state = metal::textureState(texture.get());
    state.type = TextureType::Texture3D;
    state.format = format;
    state.dataFormat = dataFormat;
    state.width = width;
    state.height = height;
    state.depth = depth;
    state.samples = 1;

    MTL::TextureDescriptor *descriptor =
        MTL::TextureDescriptor::alloc()->init();
    descriptor->setTextureType(MTL::TextureType3D);
    descriptor->setPixelFormat(metal::textureFormatToPixelFormat(format));
    descriptor->setWidth(static_cast<NS::UInteger>(std::max(width, 1)));
    descriptor->setHeight(static_cast<NS::UInteger>(std::max(height, 1)));
    descriptor->setDepth(static_cast<NS::UInteger>(std::max(depth, 1)));
    descriptor->setMipmapLevelCount(1);
    descriptor->setUsage(
        metal::textureUsageFor(TextureType::Texture3D, format));
    descriptor->setStorageMode(MTL::StorageModeShared);

    state.texture = deviceState.device->newTexture(descriptor);
    descriptor->release();
    if (state.texture == nullptr) {
        throw std::runtime_error("Failed to create Metal 3D texture");
    }

    metal::rebuildTextureSampler(texture.get(), deviceState.device);
    state.handle = metal::registerTextureHandle(texture);
    texture->textureID = state.handle;

    if (data != nullptr) {
        texture->updateData3D(data, width, height, depth, dataFormat);
    }

    return texture;
#else
    return nullptr;
#endif
}

#ifdef VULKAN
uint32_t Texture::nextTextureHandle = 1;
std::unordered_map<uint32_t, std::weak_ptr<Texture>>
    Texture::textureHandleRegistry;

uint32_t
Texture::registerTextureHandle(const std::shared_ptr<Texture> &texture) {
    if (!texture) {
        return 0;
    }

    uint32_t handle = nextTextureHandle++;
    if (handle == 0) {
        handle = nextTextureHandle++;
    }

    textureHandleRegistry[handle] = texture;
    return handle;
}

std::shared_ptr<Texture> Texture::getTextureFromHandle(uint32_t handle) {
    if (handle == 0) {
        return nullptr;
    }

    auto it = textureHandleRegistry.find(handle);
    if (it == textureHandleRegistry.end()) {
        return nullptr;
    }

    auto texture = it->second.lock();
    if (!texture) {
        textureHandleRegistry.erase(it);
    }
    return texture;
}
#endif

} // namespace opal
