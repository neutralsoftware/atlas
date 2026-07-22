//
// command_buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The Command Buffer implementation for drawing commands
// Copyright (c) 2025 maxvdec
//

#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <opal/opal.h>
#include <stdexcept>
#include <string>
#include <utility>
#ifdef METAL
#include "metal_state.h"
#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>
#endif
#endif

namespace opal {

CommandBuffer::~CommandBuffer() {
#ifdef METAL
    metal::releaseCommandBufferState(this);
#endif
}

#ifdef METAL
namespace {

constexpr NS::UInteger kVertexStreamBufferIndex = 24;
constexpr NS::UInteger kInstanceStreamBufferIndex = 25;
constexpr float kIdentityInstanceMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

template <typename T> inline T alignUp(T value, T alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

std::vector<std::shared_ptr<Texture>>
collectColorAttachments(const std::shared_ptr<Framebuffer> &framebuffer) {
    std::vector<std::shared_ptr<Texture>> colors;
    if (framebuffer == nullptr) {
        return colors;
    }
    const int drawLimit = framebuffer->getDrawBufferCount();
    int colorIndex = 0;
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.type == Attachment::Type::Color &&
            attachment.texture != nullptr) {
            if (drawLimit >= 0 && colorIndex >= drawLimit) {
                break;
            }
            colors.push_back(attachment.texture);
            colorIndex++;
        }
    }
    return colors;
}

std::shared_ptr<Texture>
collectDepthAttachment(const std::shared_ptr<Framebuffer> &framebuffer) {
    if (framebuffer == nullptr) {
        return nullptr;
    }
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.texture == nullptr) {
            continue;
        }
        if (attachment.type == Attachment::Type::Depth ||
            attachment.type == Attachment::Type::DepthStencil) {
            return attachment.texture;
        }
    }
    return nullptr;
}

std::shared_ptr<Texture>
collectStencilAttachment(const std::shared_ptr<Framebuffer> &framebuffer) {
    if (framebuffer == nullptr) {
        return nullptr;
    }
    for (const auto &attachment : framebuffer->attachments) {
        if (attachment.texture == nullptr) {
            continue;
        }
        if (attachment.type == Attachment::Type::Stencil ||
            attachment.type == Attachment::Type::DepthStencil) {
            return attachment.texture;
        }
    }
    return nullptr;
}

void ensureDefaultAuxiliaryTextures(Device *device, int width, int height) {
    if (device == nullptr || width <= 0 || height <= 0) {
        return;
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.brightTexture == nullptr ||
        deviceState.brightTexture->width != width ||
        deviceState.brightTexture->height != height) {
        deviceState.brightTexture =
            Texture::create(TextureType::Texture2D, TextureFormat::Rgba16F,
                            width, height, TextureDataFormat::Rgba, nullptr, 1);
    }

    if (deviceState.depthTexture == nullptr ||
        deviceState.depthTexture->width != width ||
        deviceState.depthTexture->height != height) {
        deviceState.depthTexture = Texture::create(
            TextureType::Texture2D, TextureFormat::Depth32F, width, height,
            TextureDataFormat::DepthComponent, nullptr, 1);
    }
}

void updateLayerDrawableSize(Device *device, int width, int height) {
    if (device == nullptr || width <= 0 || height <= 0) {
        return;
    }
    auto &deviceState = metal::deviceState(device);
    if (deviceState.context == nullptr) {
        return;
    }
    auto &contextState = metal::contextState(deviceState.context);
    if (contextState.layer == nullptr) {
        return;
    }
    contextState.layer->setDrawableSize(
        CGSizeMake(static_cast<double>(width), static_cast<double>(height)));
}

#ifdef __APPLE__
using CocoaObj = void *;

inline CocoaObj sendObjCId(CocoaObj object, const char *selector) {
    return reinterpret_cast<CocoaObj (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

inline double sendObjCDouble(CocoaObj object, const char *selector) {
    return reinterpret_cast<double (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

struct CocoaPoint {
    double x;
    double y;
};

struct CocoaSize {
    double width;
    double height;
};

struct CocoaRect {
    CocoaPoint origin;
    CocoaSize size;
};

inline CocoaRect sendObjCRect(CocoaObj object, const char *selector) {
    return reinterpret_cast<CocoaRect (*)(CocoaObj, SEL)>(objc_msgSend)(
        object, sel_registerName(selector));
}

void queryMetalDrawableSizeFromView(void *view, int fallbackWidth,
                                    int fallbackHeight, int *width,
                                    int *height) {
    int resultWidth = std::max(1, fallbackWidth);
    int resultHeight = std::max(1, fallbackHeight);
    if (view != nullptr) {
        CocoaObj targetView = view;
        CocoaRect bounds = sendObjCRect(targetView, "bounds");
        double scale = 1.0;
        CocoaObj hostWindow = sendObjCId(targetView, "window");
        if (hostWindow != nullptr) {
            const double backingScale =
                sendObjCDouble(hostWindow, "backingScaleFactor");
            if (backingScale > 0.0) {
                scale = backingScale;
            }
        }

        resultWidth =
            std::max(1, static_cast<int>(std::lround(
                            std::max(1.0, bounds.size.width) * scale)));
        resultHeight =
            std::max(1, static_cast<int>(std::lround(
                            std::max(1.0, bounds.size.height) * scale)));
    }

    if (width != nullptr) {
        *width = resultWidth;
    }
    if (height != nullptr) {
        *height = resultHeight;
    }
}

void updateMetalLayerFrameFromView(Device *device, void *view) {
    if (device == nullptr || view == nullptr) {
        return;
    }
    auto &deviceState = metal::deviceState(device);
    if (deviceState.context == nullptr) {
        return;
    }
    auto &contextState = metal::contextState(deviceState.context);
    if (contextState.layer == nullptr) {
        return;
    }

    CocoaObj targetView = view;
    CocoaRect bounds = sendObjCRect(targetView, "bounds");
    reinterpret_cast<void (*)(CocoaObj, SEL, CocoaRect)>(objc_msgSend)(
        reinterpret_cast<CocoaObj>(contextState.layer),
        sel_registerName("setFrame:"), bounds);

    double scale = 1.0;
    CocoaObj hostWindow = sendObjCId(targetView, "window");
    if (hostWindow != nullptr) {
        const double backingScale =
            sendObjCDouble(hostWindow, "backingScaleFactor");
        if (backingScale > 0.0) {
            scale = backingScale;
        }
    }
    reinterpret_cast<void (*)(CocoaObj, SEL, double)>(objc_msgSend)(
        reinterpret_cast<CocoaObj>(contextState.layer),
        sel_registerName("setContentsScale:"), scale);
}
#endif

void configureColorAttachmentForClear(MTL::RenderPassDescriptor *pass,
                                      uint32_t colorCount,
                                      const float clearColor[4],
                                      bool clearRequested) {
    if (pass == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < colorCount; ++i) {
        auto *attachment = pass->colorAttachments()->object(i);
        if (attachment == nullptr || attachment->texture() == nullptr) {
            continue;
        }
        if (clearRequested) {
            attachment->setLoadAction(MTL::LoadActionClear);
            attachment->setClearColor(MTL::ClearColor::Make(
                clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
        } else {
            attachment->setLoadAction(MTL::LoadActionLoad);
        }
        attachment->setStoreAction(MTL::StoreActionStore);
    }
}

void configureDepthAttachmentForClear(MTL::RenderPassDescriptor *pass,
                                      float clearDepth, bool clearRequested) {
    if (pass == nullptr) {
        return;
    }
    auto *depthAttachment = pass->depthAttachment();
    if (depthAttachment != nullptr && depthAttachment->texture() != nullptr) {
        depthAttachment->setLoadAction(clearRequested ? MTL::LoadActionClear
                                                      : MTL::LoadActionLoad);
        depthAttachment->setStoreAction(MTL::StoreActionStore);
        depthAttachment->setClearDepth(static_cast<double>(clearDepth));
    }
    auto *stencilAttachment = pass->stencilAttachment();
    if (stencilAttachment != nullptr &&
        stencilAttachment->texture() != nullptr) {
        stencilAttachment->setLoadAction(clearRequested ? MTL::LoadActionClear
                                                        : MTL::LoadActionLoad);
        stencilAttachment->setStoreAction(MTL::StoreActionStore);
        stencilAttachment->setClearStencil(0);
    }
}

uint32_t requiredColorOutputs(const std::shared_ptr<Pipeline> &pipeline) {
    if (pipeline == nullptr || pipeline->shaderProgram == nullptr) {
        return 1;
    }
    auto &programState = metal::programState(pipeline->shaderProgram.get());
    return programState.fragmentColorOutputs > 0
               ? programState.fragmentColorOutputs
               : 1;
}

std::shared_ptr<Texture> fallbackTexture2D() {
    if (Device::globalInstance == nullptr) {
        return nullptr;
    }
    static std::shared_ptr<Texture> texture = nullptr;
    if (texture == nullptr) {
        const unsigned char black[4] = {0, 0, 0, 255};
        texture = Texture::create(TextureType::Texture2D, TextureFormat::Rgba8,
                                  1, 1, TextureDataFormat::Rgba, black, 1);
        texture->setFilterMode(TextureFilterMode::Nearest,
                               TextureFilterMode::Nearest);
        texture->setWrapMode(TextureAxis::S, TextureWrapMode::ClampToEdge);
        texture->setWrapMode(TextureAxis::T, TextureWrapMode::ClampToEdge);
    }
    return texture;
}

std::shared_ptr<Texture> fallbackTexture2DArray() {
    if (Device::globalInstance == nullptr) {
        return nullptr;
    }
    static std::shared_ptr<Texture> texture = nullptr;
    if (texture == nullptr) {
        const unsigned char black[4] = {0, 0, 0, 255};
        texture =
            Texture::create(TextureType::Texture2DArray, TextureFormat::Rgba8,
                            1, 1, TextureDataFormat::Rgba, black, 1);
        texture->setFilterMode(TextureFilterMode::Nearest,
                               TextureFilterMode::Nearest);
        texture->setWrapMode(TextureAxis::S, TextureWrapMode::ClampToEdge);
        texture->setWrapMode(TextureAxis::T, TextureWrapMode::ClampToEdge);
    }
    return texture;
}

std::shared_ptr<Texture> fallbackTextureCube() {
    if (Device::globalInstance == nullptr) {
        return nullptr;
    }
    static std::shared_ptr<Texture> texture = nullptr;
    if (texture == nullptr) {
        const unsigned char black[4] = {0, 0, 0, 255};
        texture =
            Texture::create(TextureType::TextureCubeMap, TextureFormat::Rgba8,
                            1, 1, TextureDataFormat::Rgba, nullptr, 1);
        texture->setFilterMode(TextureFilterMode::Nearest,
                               TextureFilterMode::Nearest);
        texture->setWrapMode(TextureAxis::S, TextureWrapMode::ClampToEdge);
        texture->setWrapMode(TextureAxis::T, TextureWrapMode::ClampToEdge);
        texture->setWrapMode(TextureAxis::R, TextureWrapMode::ClampToEdge);
        for (int face = 0; face < 6; ++face) {
            texture->updateFace(face, black, 1, 1, TextureDataFormat::Rgba);
        }
    }
    return texture;
}

std::shared_ptr<Texture> fallbackTexture3D() {
    if (Device::globalInstance == nullptr) {
        return nullptr;
    }
    static std::shared_ptr<Texture> texture = nullptr;
    if (texture == nullptr) {
        const unsigned char black[4] = {0, 0, 0, 255};
        texture = Texture::create3D(TextureFormat::Rgba8, 1, 1, 1,
                                    TextureDataFormat::Rgba, black);
        texture->setParameters3D(
            TextureWrapMode::ClampToEdge, TextureWrapMode::ClampToEdge,
            TextureWrapMode::ClampToEdge, TextureFilterMode::Nearest,
            TextureFilterMode::Nearest);
    }
    return texture;
}

std::shared_ptr<Texture> fallbackTextureForType(TextureType type) {
    switch (type) {
    case TextureType::TextureCubeMap:
        return fallbackTextureCube();
    case TextureType::Texture3D:
        return fallbackTexture3D();
    case TextureType::Texture2DArray:
        return fallbackTexture2DArray();
    case TextureType::Texture2D:
    default:
        return fallbackTexture2D();
    }
}

#ifdef __APPLE__
bool encodeMetalFXSpatialUpscale(
    CommandBuffer *commandBuffer, Device *device,
    const std::shared_ptr<Texture> &sourceTexture) {
    if (commandBuffer == nullptr || device == nullptr ||
        sourceTexture == nullptr) {
        return false;
    }

    auto &state = metal::commandBufferState(commandBuffer);
    auto &deviceState = metal::deviceState(device);
    auto &sourceState = metal::textureState(sourceTexture.get());
    if (deviceState.device == nullptr || deviceState.queue == nullptr ||
        sourceState.texture == nullptr) {
        return false;
    }

    MTL::Texture *destinationTexture = nullptr;
    if (state.passDescriptor != nullptr) {
        auto *colorAttachment =
            state.passDescriptor->colorAttachments()->object(0);
        if (colorAttachment != nullptr) {
            destinationTexture = colorAttachment->texture();
        }
    }
    if (destinationTexture == nullptr && state.drawable != nullptr) {
        destinationTexture = state.drawable->texture();
    }

    if (destinationTexture == nullptr ||
        sourceState.texture == destinationTexture) {
        return false;
    }
    if (sourceState.texture->sampleCount() != 1 ||
        destinationTexture->sampleCount() != 1) {
        return false;
    }

    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.commandBuffer == nullptr) {
        return false;
    }

    Class descriptorClass =
        reinterpret_cast<Class>(objc_getClass("MTLFXSpatialScalerDescriptor"));
    if (descriptorClass == nullptr) {
        return false;
    }

    auto sendIdNoArgs = [](id obj, SEL selector) -> id {
        return ((id (*)(id, SEL))objc_msgSend)(obj, selector);
    };
    auto sendVoidUInteger = [](id obj, SEL selector, NS::UInteger value) {
        ((void (*)(id, SEL, NS::UInteger))objc_msgSend)(obj, selector, value);
    };
    auto sendVoidPixelFormat = [](id obj, SEL selector,
                                  MTL::PixelFormat value) {
        ((void (*)(id, SEL, MTL::PixelFormat))objc_msgSend)(obj, selector,
                                                            value);
    };
    auto sendVoidId = [](id obj, SEL selector, id value) {
        ((void (*)(id, SEL, id))objc_msgSend)(obj, selector, value);
    };

    id descriptor = sendIdNoArgs(reinterpret_cast<id>(descriptorClass),
                                 sel_registerName("alloc"));
    descriptor = sendIdNoArgs(descriptor, sel_registerName("init"));
    if (descriptor == nullptr) {
        return false;
    }

    sendVoidUInteger(descriptor, sel_registerName("setInputWidth:"),
                     sourceState.texture->width());
    sendVoidUInteger(descriptor, sel_registerName("setInputHeight:"),
                     sourceState.texture->height());
    sendVoidUInteger(descriptor, sel_registerName("setOutputWidth:"),
                     destinationTexture->width());
    sendVoidUInteger(descriptor, sel_registerName("setOutputHeight:"),
                     destinationTexture->height());
    sendVoidPixelFormat(descriptor, sel_registerName("setColorTextureFormat:"),
                        sourceState.texture->pixelFormat());
    sendVoidPixelFormat(descriptor, sel_registerName("setOutputTextureFormat:"),
                        destinationTexture->pixelFormat());

    id scaler = ((id (*)(id, SEL, id))objc_msgSend)(
        descriptor, sel_registerName("newSpatialScalerWithDevice:"),
        reinterpret_cast<id>(deviceState.device));
    ((void (*)(id, SEL))objc_msgSend)(descriptor, sel_registerName("release"));

    if (scaler == nullptr) {
        return false;
    }

    sendVoidId(scaler, sel_registerName("setColorTexture:"),
               reinterpret_cast<id>(sourceState.texture));
    sendVoidId(scaler, sel_registerName("setOutputTexture:"),
               reinterpret_cast<id>(destinationTexture));
    sendVoidId(scaler, sel_registerName("encodeToCommandBuffer:"),
               reinterpret_cast<id>(state.commandBuffer));

    ((void (*)(id, SEL))objc_msgSend)(scaler, sel_registerName("release"));
    state.hasDraw = true;
    return true;
}
#endif

MTL::RenderPipelineState *
getRenderPipelineState(Device *device,
                       const std::shared_ptr<Pipeline> &pipeline,
                       const std::array<MTL::PixelFormat, 8> &colorFormats,
                       uint32_t colorCount, MTL::PixelFormat depthFormat,
                       MTL::PixelFormat stencilFormat, uint32_t sampleCount) {
    if (device == nullptr || pipeline == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return nullptr;
    }

    auto &deviceState = metal::deviceState(device);
    auto &pipelineState = metal::pipelineState(pipeline.get());
    auto &programState = metal::programState(pipeline->shaderProgram.get());
    if (deviceState.device == nullptr ||
        programState.vertexFunction == nullptr ||
        programState.fragmentFunction == nullptr) {
        return nullptr;
    }

    std::string key = metal::makePipelineKey(
        colorFormats, colorCount, depthFormat, stencilFormat, sampleCount);
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.blendingEnabled));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendSrc));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendDst));
    key += "|" + std::to_string(static_cast<int>(pipelineState.blendOp));
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.depthTestEnabled));
    key +=
        "|" + std::to_string(static_cast<int>(pipelineState.depthWriteEnabled));
    key += "|" + std::to_string(static_cast<int>(pipelineState.depthCompare));
    auto cacheIt = pipelineState.renderPipelineCache.find(key);
    if (cacheIt != pipelineState.renderPipelineCache.end()) {
        return cacheIt->second;
    }

    MTL::RenderPipelineDescriptor *descriptor =
        MTL::RenderPipelineDescriptor::alloc()->init();
    descriptor->setVertexFunction(programState.vertexFunction);
    descriptor->setFragmentFunction(programState.fragmentFunction);
    descriptor->setSampleCount(std::max<uint32_t>(1, sampleCount));
    if (pipelineState.vertexDescriptor != nullptr) {
        descriptor->setVertexDescriptor(pipelineState.vertexDescriptor);
    }

    for (uint32_t i = 0; i < colorCount && i < colorFormats.size(); ++i) {
        auto *attachment = descriptor->colorAttachments()->object(i);
        if (attachment == nullptr) {
            continue;
        }
        attachment->setPixelFormat(colorFormats[i]);
        attachment->setWriteMask(MTL::ColorWriteMaskAll);
        attachment->setBlendingEnabled(pipelineState.blendingEnabled);
        if (pipelineState.blendingEnabled) {
            attachment->setRgbBlendOperation(pipelineState.blendOp);
            attachment->setAlphaBlendOperation(pipelineState.blendOp);
            attachment->setSourceRGBBlendFactor(pipelineState.blendSrc);
            attachment->setDestinationRGBBlendFactor(pipelineState.blendDst);
            attachment->setSourceAlphaBlendFactor(pipelineState.blendSrc);
            attachment->setDestinationAlphaBlendFactor(pipelineState.blendDst);
        }
    }

    if (depthFormat != MTL::PixelFormatInvalid) {
        descriptor->setDepthAttachmentPixelFormat(depthFormat);
    }
    if (stencilFormat != MTL::PixelFormatInvalid) {
        descriptor->setStencilAttachmentPixelFormat(stencilFormat);
    }

    NS::Error *error = nullptr;
    MTL::RenderPipelineState *created =
        deviceState.device->newRenderPipelineState(descriptor, &error);
    descriptor->release();
    if (created == nullptr) {
        std::string message = "Failed to create Metal render pipeline";
        if (error != nullptr && error->localizedDescription() != nullptr) {
            message += ": ";
            message += error->localizedDescription()->utf8String();
        }
        throw std::runtime_error(message);
    }

    pipelineState.renderPipelineCache[key] = created;
    return created;
}

void uploadUniformBuffers(const std::shared_ptr<Pipeline> &pipeline,
                          MTL::RenderCommandEncoder *encoder,
                          MTL::Device *device) {
    if (pipeline == nullptr || encoder == nullptr || device == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return;
    }

    auto &programState = metal::programState(pipeline->shaderProgram.get());
    auto &pipelineState = metal::pipelineState(pipeline.get());

    for (const auto &binding : programState.bindings) {
        auto uploadStage = [&](metal::MetalProgramStage stage) {
            uint32_t key = metal::stageBindingKey(binding.index, stage);
            auto shaderBufferIt = pipelineState.shaderBuffers.find(key);
            if (shaderBufferIt != pipelineState.shaderBuffers.end() &&
                shaderBufferIt->second != nullptr) {
                auto &bufferState =
                    metal::bufferState(shaderBufferIt->second.get());
                if (bufferState.buffer == nullptr) {
                    throw std::runtime_error(
                        "Metal shader buffer is not initialized");
                }
                if (stage == metal::MetalProgramStage::Fragment) {
                    encoder->setFragmentBuffer(bufferState.buffer, 0,
                                               binding.index);
                } else {
                    encoder->setVertexBuffer(bufferState.buffer, 0,
                                             binding.index);
                }
                return;
            }
            auto &bytes = pipelineState.uniformData[key];
            size_t requiredSize = 0;
            auto bindingSizeIt = programState.bindingSize.find(key);
            if (bindingSizeIt != programState.bindingSize.end()) {
                requiredSize = bindingSizeIt->second;
            }
            if (bytes.size() < requiredSize) {
                bytes.resize(requiredSize, 0);
            }
            if (bytes.empty()) {
                return;
            }

            if (bytes.size() <= 4096) {
                if (stage == metal::MetalProgramStage::Fragment) {
                    encoder->setFragmentBytes(bytes.data(), bytes.size(),
                                              binding.index);
                } else {
                    encoder->setVertexBytes(bytes.data(), bytes.size(),
                                            binding.index);
                }
                return;
            }

            MTL::Buffer *inlineBuffer =
                device->newBuffer(bytes.data(),
                                  static_cast<NS::UInteger>(alignUp(
                                      bytes.size(), static_cast<size_t>(16))),
                                  MTL::ResourceStorageModeShared);
            if (inlineBuffer == nullptr) {
                throw std::runtime_error(
                    "Failed to allocate Metal uniform buffer");
            }
            if (stage == metal::MetalProgramStage::Fragment) {
                encoder->setFragmentBuffer(inlineBuffer, 0, binding.index);
            } else {
                encoder->setVertexBuffer(inlineBuffer, 0, binding.index);
            }
            inlineBuffer->release();
        };

        if (binding.vertexStage) {
            uploadStage(metal::MetalProgramStage::Vertex);
        }
        if (binding.fragmentStage) {
            uploadStage(metal::MetalProgramStage::Fragment);
        }
    }
}

void bindTextures(CommandBuffer *commandBuffer,
                  const std::shared_ptr<Pipeline> &pipeline,
                  MTL::RenderCommandEncoder *encoder, MTL::Device *device) {
    if (commandBuffer == nullptr || pipeline == nullptr || encoder == nullptr ||
        device == nullptr) {
        return;
    }

    auto &commandState = metal::commandBufferState(commandBuffer);
    auto &pipelineState = metal::pipelineState(pipeline.get());
    std::array<MTL::Texture *, 32> desiredTextures{};
    std::array<MTL::SamplerState *, 32> desiredSamplers{};
    desiredTextures.fill(nullptr);
    desiredSamplers.fill(nullptr);

    for (const auto &pair : pipelineState.texturesByUnit) {
        int unit = pair.first;
        const auto &texture = pair.second;
        if (unit < 0 || unit >= static_cast<int>(desiredTextures.size()) ||
            texture == nullptr) {
            continue;
        }
        auto &textureState = metal::textureState(texture.get());
        if (textureState.texture == nullptr) {
            continue;
        }
        if (textureState.sampler == nullptr) {
            metal::rebuildTextureSampler(texture.get(), device);
        }
        desiredTextures[static_cast<size_t>(unit)] = textureState.texture;
        desiredSamplers[static_cast<size_t>(unit)] = textureState.sampler;
    }

    if (pipeline->shaderProgram != nullptr) {
        auto &programState = metal::programState(pipeline->shaderProgram.get());
        for (const auto &binding : programState.textureTypesByBinding) {
            int unit = binding.first;
            if (unit < 0 || unit >= static_cast<int>(desiredTextures.size())) {
                continue;
            }
            size_t unitIndex = static_cast<size_t>(unit);
            if (desiredTextures[unitIndex] != nullptr) {
                continue;
            }
            auto fallback = fallbackTextureForType(binding.second);
            if (fallback == nullptr) {
                continue;
            }
            auto &fallbackState = metal::textureState(fallback.get());
            if (fallbackState.texture == nullptr) {
                continue;
            }
            if (fallbackState.sampler == nullptr) {
                metal::rebuildTextureSampler(fallback.get(), device);
            }
            desiredTextures[unitIndex] = fallbackState.texture;
            desiredSamplers[unitIndex] = fallbackState.sampler;
        }
    }

    if (!commandState.textureBindingsInitialized) {
        commandState.boundVertexTextures.fill(nullptr);
        commandState.boundFragmentTextures.fill(nullptr);
        commandState.boundVertexSamplers.fill(nullptr);
        commandState.boundFragmentSamplers.fill(nullptr);
        commandState.textureBindingsInitialized = true;
    }

    constexpr size_t kMaxRenderTextureUnits = 16;
    const size_t renderUnitCount =
        std::min(desiredTextures.size(), kMaxRenderTextureUnits);

    for (size_t unit = 0; unit < renderUnitCount; ++unit) {
        MTL::Texture *desiredTexture = desiredTextures[unit];
        MTL::SamplerState *desiredSampler = desiredSamplers[unit];

        if (commandState.boundVertexTextures[unit] != desiredTexture) {
            encoder->setVertexTexture(desiredTexture,
                                      static_cast<NS::UInteger>(unit));
            commandState.boundVertexTextures[unit] = desiredTexture;
        }
        if (commandState.boundFragmentTextures[unit] != desiredTexture) {
            encoder->setFragmentTexture(desiredTexture,
                                        static_cast<NS::UInteger>(unit));
            commandState.boundFragmentTextures[unit] = desiredTexture;
        }
        if (commandState.boundVertexSamplers[unit] != desiredSampler) {
            encoder->setVertexSamplerState(desiredSampler,
                                           static_cast<NS::UInteger>(unit));
            commandState.boundVertexSamplers[unit] = desiredSampler;
        }
        if (commandState.boundFragmentSamplers[unit] != desiredSampler) {
            encoder->setFragmentSamplerState(desiredSampler,
                                             static_cast<NS::UInteger>(unit));
            commandState.boundFragmentSamplers[unit] = desiredSampler;
        }
    }
}

MTL::ComputePipelineState *
getComputePipelineState(Device *device,
                        const std::shared_ptr<Pipeline> &pipeline) {
    if (device == nullptr || pipeline == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return nullptr;
    }

    auto &deviceState = metal::deviceState(device);
    auto &programState = metal::programState(pipeline->shaderProgram.get());
    auto &pipelineState = metal::pipelineState(pipeline.get());
    if (deviceState.device == nullptr ||
        programState.computeFunction == nullptr) {
        return nullptr;
    }

    if (pipelineState.computePipelineState != nullptr) {
        return pipelineState.computePipelineState;
    }

    NS::Error *error = nullptr;
    pipelineState.computePipelineState =
        deviceState.device->newComputePipelineState(
            programState.computeFunction, &error);
    if (pipelineState.computePipelineState == nullptr) {
        std::string message = "Failed to create Metal compute pipeline";
        if (error != nullptr && error->localizedDescription() != nullptr) {
            message += ": ";
            message += error->localizedDescription()->utf8String();
        }
        throw std::runtime_error(message);
    }

    return pipelineState.computePipelineState;
}

void uploadComputeUniformBuffers(const std::shared_ptr<Pipeline> &pipeline,
                                 MTL::ComputeCommandEncoder *encoder,
                                 MTL::Device *device) {
    if (pipeline == nullptr || encoder == nullptr || device == nullptr ||
        pipeline->shaderProgram == nullptr) {
        return;
    }

    auto &programState = metal::programState(pipeline->shaderProgram.get());
    auto &pipelineState = metal::pipelineState(pipeline.get());

    for (const auto &binding : programState.bindings) {
        if (!binding.computeStage) {
            continue;
        }

        uint32_t key = metal::stageBindingKey(
            binding.index, metal::MetalProgramStage::Compute);
        auto shaderBufferIt = pipelineState.shaderBuffers.find(key);
        if (shaderBufferIt != pipelineState.shaderBuffers.end() &&
            shaderBufferIt->second != nullptr) {
            auto &bufferState =
                metal::bufferState(shaderBufferIt->second.get());
            if (bufferState.buffer == nullptr) {
                throw std::runtime_error(
                    "Metal shader buffer is not initialized");
            }
            encoder->setBuffer(bufferState.buffer, 0, binding.index);
            continue;
        }
        auto &bytes = pipelineState.uniformData[key];

        size_t requiredSize = 0;
        auto bindingSizeIt = programState.bindingSize.find(key);
        if (bindingSizeIt != programState.bindingSize.end()) {
            requiredSize = bindingSizeIt->second;
        }
        if (bytes.size() < requiredSize) {
            bytes.resize(requiredSize, 0);
        }
        if (bytes.empty()) {
            continue;
        }

        if (bytes.size() <= 4096) {
            encoder->setBytes(bytes.data(), bytes.size(), binding.index);
            continue;
        }

        MTL::Buffer *inlineBuffer =
            device->newBuffer(bytes.data(),
                              static_cast<NS::UInteger>(alignUp(
                                  bytes.size(), static_cast<size_t>(16))),
                              MTL::ResourceStorageModeShared);
        if (inlineBuffer == nullptr) {
            throw std::runtime_error(
                "Failed to allocate Metal compute uniform buffer");
        }
        encoder->setBuffer(inlineBuffer, 0, binding.index);
        inlineBuffer->release();
    }
}

void bindComputeTextures(const std::shared_ptr<Pipeline> &pipeline,
                         MTL::ComputeCommandEncoder *encoder,
                         MTL::Device *device) {
    if (pipeline == nullptr || encoder == nullptr || device == nullptr) {
        return;
    }

    auto &pipelineState = metal::pipelineState(pipeline.get());
    if (pipelineState.textureArgumentBuffer != nullptr) {
        encoder->setBuffer(pipelineState.textureArgumentBuffer, 0,
                           pipelineState.textureArgumentBufferIndex);
        for (const auto &texture : pipelineState.textureArgumentTextures) {
            if (texture == nullptr) {
                continue;
            }
            auto &textureState = metal::textureState(texture.get());
            encoder->useResource(textureState.texture,
                                 MTL::ResourceUsageRead);
        }
    }
    std::array<MTL::Texture *, 64> desiredTextures{};
    std::array<MTL::SamplerState *, 16> desiredSamplers{};
    desiredTextures.fill(nullptr);
    desiredSamplers.fill(nullptr);

    for (const auto &pair : pipelineState.texturesByUnit) {
        int unit = pair.first;
        const auto &texture = pair.second;
        if (unit < 0 || unit >= static_cast<int>(desiredTextures.size()) ||
            texture == nullptr) {
            continue;
        }
        auto &textureState = metal::textureState(texture.get());
        if (textureState.texture == nullptr) {
            continue;
        }
        desiredTextures[static_cast<size_t>(unit)] = textureState.texture;
        if (unit < static_cast<int>(desiredSamplers.size())) {
            if (textureState.sampler == nullptr) {
                metal::rebuildTextureSampler(texture.get(), device);
            }
            desiredSamplers[static_cast<size_t>(unit)] = textureState.sampler;
        }
    }

    if (pipeline->shaderProgram != nullptr) {
        auto &programState = metal::programState(pipeline->shaderProgram.get());
        for (const auto &binding : programState.textureTypesByBinding) {
            int unit = binding.first;
            if (unit < 0 || unit >= static_cast<int>(desiredTextures.size())) {
                continue;
            }
            size_t unitIndex = static_cast<size_t>(unit);
            if (desiredTextures[unitIndex] != nullptr) {
                continue;
            }
            auto fallback = fallbackTextureForType(binding.second);
            if (fallback == nullptr) {
                continue;
            }
            auto &fallbackState = metal::textureState(fallback.get());
            if (fallbackState.texture == nullptr) {
                continue;
            }
            desiredTextures[unitIndex] = fallbackState.texture;
            if (unitIndex < desiredSamplers.size()) {
                if (fallbackState.sampler == nullptr) {
                    metal::rebuildTextureSampler(fallback.get(), device);
                }
                desiredSamplers[unitIndex] = fallbackState.sampler;
            }
        }
    }

    for (size_t unit = 0; unit < desiredTextures.size(); ++unit) {
        encoder->setTexture(desiredTextures[unit],
                            static_cast<NS::UInteger>(unit));
    }
    for (size_t unit = 0; unit < desiredSamplers.size(); ++unit) {
        encoder->setSamplerState(desiredSamplers[unit],
                                 static_cast<NS::UInteger>(unit));
    }
}

void ensureRenderEncoder(CommandBuffer *commandBuffer, Device *device,
                         const std::shared_ptr<Framebuffer> &framebuffer,
                         const std::shared_ptr<Pipeline> &boundPipeline,
                         const float clearColorValue[4],
                         float clearDepthValue) {
    if (commandBuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(commandBuffer);
    if (device == nullptr || framebuffer == nullptr ||
        boundPipeline == nullptr) {
        return;
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.queue == nullptr || deviceState.device == nullptr) {
        throw std::runtime_error("Metal device queue is not initialized");
    }
    if (state.passDescriptor == nullptr) {
        return;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }

    uint32_t requiredColors = requiredColorOutputs(boundPipeline);
    const int drawLimit = framebuffer->getDrawBufferCount();
    if (drawLimit >= 0) {
        requiredColors =
            std::min(requiredColors, static_cast<uint32_t>(drawLimit));
    }
    auto framebufferColors = collectColorAttachments(framebuffer);
    uint32_t actualColorCount = static_cast<uint32_t>(framebufferColors.size());

    int fbWidth = framebuffer->width;
    int fbHeight = framebuffer->height;
    if (framebuffer->isDefaultFramebuffer) {
        fbWidth = std::max(1, deviceState.drawableWidth);
        fbHeight = std::max(1, deviceState.drawableHeight);
        ensureDefaultAuxiliaryTextures(device, fbWidth, fbHeight);
    }

    uint32_t targetColorCount = actualColorCount;
    if (framebuffer->isDefaultFramebuffer) {
        targetColorCount = std::max(requiredColors, targetColorCount);
        targetColorCount = std::max<uint32_t>(1, targetColorCount);
    }
    targetColorCount = std::min<uint32_t>(8, targetColorCount);

    for (uint32_t i = 0; i < 8; ++i) {
        auto *attachment = state.passDescriptor->colorAttachments()->object(i);
        if (attachment == nullptr) {
            continue;
        }
        MTL::Texture *targetTexture = nullptr;
        MTL::StoreAction storeAction = MTL::StoreActionDontCare;

        if (i < targetColorCount) {
            if (!framebuffer->isDefaultFramebuffer) {
                if (i < framebufferColors.size() &&
                    framebufferColors[i] != nullptr) {
                    auto &textureState =
                        metal::textureState(framebufferColors[i].get());
                    targetTexture = textureState.texture;
                    storeAction = MTL::StoreActionStore;
                }
            } else if (i == 0 && state.drawable != nullptr) {
                targetTexture = state.drawable->texture();
                storeAction = MTL::StoreActionStore;
            } else if (deviceState.brightTexture != nullptr) {
                auto &brightState =
                    metal::textureState(deviceState.brightTexture.get());
                targetTexture = brightState.texture;
            }
        }

        attachment->setTexture(targetTexture);
        attachment->setResolveTexture(nullptr);
        attachment->setStoreAction(storeAction);
        if (targetTexture == nullptr) {
            attachment->setLoadAction(MTL::LoadActionDontCare);
        }
    }

    if (state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, targetColorCount,
                                         clearColorValue,
                                         state.clearColorPending);
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         state.clearDepthPending);
    }

    std::array<MTL::PixelFormat, 8> colorFormats{};
    colorFormats.fill(MTL::PixelFormatInvalid);
    uint32_t colorCount = 0;
    uint32_t sampleCount = 1;

    for (uint32_t i = 0; i < targetColorCount; ++i) {
        auto *attachment = state.passDescriptor->colorAttachments()->object(i);
        if (attachment == nullptr || attachment->texture() == nullptr) {
            continue;
        }
        colorFormats[i] = attachment->texture()->pixelFormat();
        colorCount = std::max(colorCount, i + 1);
        sampleCount = std::max<uint32_t>(
            sampleCount,
            static_cast<uint32_t>(attachment->texture()->sampleCount()));
    }

    MTL::PixelFormat depthFormat = MTL::PixelFormatInvalid;
    MTL::PixelFormat stencilFormat = MTL::PixelFormatInvalid;
    if (state.passDescriptor->depthAttachment() != nullptr &&
        state.passDescriptor->depthAttachment()->texture() != nullptr) {
        depthFormat =
            state.passDescriptor->depthAttachment()->texture()->pixelFormat();
        sampleCount = std::max<uint32_t>(
            sampleCount,
            static_cast<uint32_t>(state.passDescriptor->depthAttachment()
                                      ->texture()
                                      ->sampleCount()));
    }
    if (state.passDescriptor->stencilAttachment() != nullptr &&
        state.passDescriptor->stencilAttachment()->texture() != nullptr) {
        stencilFormat =
            state.passDescriptor->stencilAttachment()->texture()->pixelFormat();
    }

    if (state.encoder == nullptr) {
        state.encoder =
            state.commandBuffer->renderCommandEncoder(state.passDescriptor);
        if (state.encoder == nullptr) {
            throw std::runtime_error(
                "Failed to create Metal render command encoder");
        }

        state.clearColorPending = false;
        state.clearDepthPending = false;
    }

    MTL::RenderPipelineState *renderPipelineState =
        getRenderPipelineState(device, boundPipeline, colorFormats, colorCount,
                               depthFormat, stencilFormat, sampleCount);
    if (renderPipelineState == nullptr) {
        throw std::runtime_error("Metal render pipeline state creation failed");
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->setRenderPipelineState(renderPipelineState);
    if (pipelineState.depthStencilState != nullptr) {
        state.encoder->setDepthStencilState(pipelineState.depthStencilState);
    }
    state.encoder->setCullMode(pipelineState.cullMode);
    state.encoder->setFrontFacingWinding(pipelineState.frontFace);
    state.encoder->setTriangleFillMode(pipelineState.fillMode);
    if (pipelineState.polygonOffsetEnabled) {
        state.encoder->setDepthBias(pipelineState.polygonOffsetUnits,
                                    pipelineState.polygonOffsetFactor, 0.0f);
    } else {
        state.encoder->setDepthBias(0.0f, 0.0f, 0.0f);
    }

    int viewportX = std::max(0, pipelineState.viewportX);
    int viewportY = std::max(0, pipelineState.viewportY);
    int viewportW =
        pipelineState.viewportWidth > 0 ? pipelineState.viewportWidth : fbWidth;
    int viewportH = pipelineState.viewportHeight > 0
                        ? pipelineState.viewportHeight
                        : fbHeight;
    viewportW =
        std::max(1, std::min(viewportW, std::max(1, fbWidth - viewportX)));
    viewportH =
        std::max(1, std::min(viewportH, std::max(1, fbHeight - viewportY)));

    MTL::Viewport viewport{static_cast<double>(viewportX),
                           static_cast<double>(viewportY),
                           static_cast<double>(viewportW),
                           static_cast<double>(viewportH),
                           0.0,
                           1.0};
    state.encoder->setViewport(viewport);

    MTL::ScissorRect scissor{static_cast<NS::UInteger>(viewportX),
                             static_cast<NS::UInteger>(viewportY),
                             static_cast<NS::UInteger>(std::max(1, viewportW)),
                             static_cast<NS::UInteger>(std::max(1, viewportH))};
    state.encoder->setScissorRect(scissor);

    uploadUniformBuffers(boundPipeline, state.encoder, deviceState.device);
    bindTextures(commandBuffer, boundPipeline, state.encoder,
                 deviceState.device);
}

} // namespace
#endif

std::shared_ptr<CommandBuffer> Device::acquireCommandBuffer() {
    auto commandBuffer = std::make_shared<CommandBuffer>();
    commandBuffer->device = this;

#ifdef VULKAN
    commandBuffer->commandBuffers.resize(CommandBuffer::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = Device::globalInstance->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = CommandBuffer::MAX_FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(this->logicalDevice, &allocInfo,
                                 commandBuffer->commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
#endif
    return commandBuffer;
}

void CommandBuffer::start() {
    boundPipeline = nullptr;
    boundDrawingState = nullptr;
    renderPass = nullptr;
    framebuffer = nullptr;
#ifdef VULKAN
    hasStarted = false;
    imageAcquired = false;
    commandBufferBegan = false;
    this->createSyncObjects();
    vkWaitForFences(device->logicalDevice, 1, &inFlightFences[currentFrame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(device->logicalDevice, 1, &inFlightFences[currentFrame]);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    for (size_t i = 0; i < state.inFlightCommandBuffers.size();) {
        auto *buffer = state.inFlightCommandBuffers[i];
        if (buffer->status() < MTL::CommandBufferStatusCompleted) {
            ++i;
            continue;
        }
        if (buffer->status() == MTL::CommandBufferStatusError) {
            auto *error = buffer->error();
            const char *description =
                error != nullptr && error->localizedDescription() != nullptr
                    ? error->localizedDescription()->utf8String()
                    : "Unknown Metal command buffer error";
            atlas_error(std::string("Metal GPU command failed: ") +
                        description);
        }
        buffer->release();
        state.inFlightCommandBuffers.erase(
            state.inFlightCommandBuffers.begin() + i);
        state.inFlightResources.erase(state.inFlightResources.begin() + i);
    }
    if (state.inFlightCommandBuffers.size() >= 3) {
        auto *oldest = state.inFlightCommandBuffers.front();
        oldest->waitUntilCompleted();
        if (oldest->status() == MTL::CommandBufferStatusError) {
            auto *error = oldest->error();
            const char *description =
                error != nullptr && error->localizedDescription() != nullptr
                    ? error->localizedDescription()->utf8String()
                    : "Unknown Metal command buffer error";
            atlas_error(std::string("Metal GPU command failed: ") +
                        description);
        }
        oldest->release();
        state.inFlightCommandBuffers.erase(
            state.inFlightCommandBuffers.begin());
        state.inFlightResources.erase(state.inFlightResources.begin());
    }
    if (state.autoreleasePool != nullptr) {
        state.autoreleasePool->release();
    }
    state.autoreleasePool = NS::AutoreleasePool::alloc()->init();
    state.commandBuffer = nullptr;
    state.encoder = nullptr;
    state.computeEncoder = nullptr;
    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
    }
    state.passDescriptor = nullptr;
    state.drawable = nullptr;
    state.boundVertexTextures.fill(nullptr);
    state.boundFragmentTextures.fill(nullptr);
    state.boundVertexSamplers.fill(nullptr);
    state.boundFragmentSamplers.fill(nullptr);
    state.textureBindingsInitialized = false;
    state.needsPresent = false;
    state.hasDraw = false;
    state.clearColorPending = false;
    state.clearDepthPending = false;
#endif
}

void Device::submitCommandBuffer(
    [[maybe_unused]] const std::shared_ptr<CommandBuffer> &commandBuffer) {}

void CommandBuffer::beginPass(std::shared_ptr<RenderPass> newRenderPass) {
    if (newRenderPass == nullptr) {
        throw std::runtime_error(
            "Cannot begin a command buffer pass without a render pass");
    }
    if (newRenderPass->framebuffer == nullptr) {
        throw std::runtime_error(
            "Render pass must have a framebuffer before beginPass");
    }

    renderPass = std::move(newRenderPass);
    framebuffer = renderPass->framebuffer;

#ifdef OPENGL
    framebuffer->bind();
#elif defined(VULKAN)
    if (framebuffer->isDefaultFramebuffer && device != nullptr) {
        framebuffer->width = static_cast<int>(device->swapChainExtent.width);
        framebuffer->height = static_cast<int>(device->swapChainExtent.height);
    }
#elif defined(METAL)
    if (device == nullptr) {
        throw std::runtime_error("Metal command buffer has no device");
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.device == nullptr || deviceState.queue == nullptr) {
        throw std::runtime_error("Metal device queue is not initialized");
    }

    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }

    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }

    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
        state.passDescriptor = nullptr;
    }
    state.passDescriptor =
        MTL::RenderPassDescriptor::renderPassDescriptor()->copy();
    state.hasDraw = false;

    if (framebuffer->isDefaultFramebuffer) {
        if (deviceState.context == nullptr) {
            throw std::runtime_error("Metal device context is missing");
        }
        int fbWidth = 0;
        int fbHeight = 0;
        atlasGetWindowSizeInPixels(deviceState.context->getWindow(), &fbWidth,
                                   &fbHeight);
#ifdef __APPLE__
        queryMetalDrawableSizeFromView(
            deviceState.context->getMetalTargetView(), fbWidth, fbHeight,
            &fbWidth, &fbHeight);
        updateMetalLayerFrameFromView(
            device, deviceState.context->getMetalTargetView());
#endif
        fbWidth = std::max(1, fbWidth);
        fbHeight = std::max(1, fbHeight);

        updateLayerDrawableSize(device, fbWidth, fbHeight);
        ensureDefaultAuxiliaryTextures(device, fbWidth, fbHeight);

        auto &contextState = metal::contextState(deviceState.context);
        deviceState.drawable = contextState.layer->nextDrawable();
        state.drawable = deviceState.drawable;
        if (deviceState.drawable == nullptr) {
            state.passDescriptor->release();
            state.passDescriptor = nullptr;
            state.commandBuffer = nullptr;
            state.needsPresent = false;
            return;
        }
        deviceState.drawableWidth = fbWidth;
        deviceState.drawableHeight = fbHeight;
        framebuffer->width = fbWidth;
        framebuffer->height = fbHeight;

        auto *color0 = state.passDescriptor->colorAttachments()->object(0);
        color0->setTexture(deviceState.drawable->texture());
        color0->setStoreAction(MTL::StoreActionStore);

        if (deviceState.depthTexture != nullptr) {
            auto &depthState =
                metal::textureState(deviceState.depthTexture.get());
            auto *depthAttachment = state.passDescriptor->depthAttachment();
            depthAttachment->setTexture(depthState.texture);
            depthAttachment->setStoreAction(MTL::StoreActionStore);
            if (depthState.format == TextureFormat::Depth24Stencil8) {
                auto *stencilAttachment =
                    state.passDescriptor->stencilAttachment();
                stencilAttachment->setTexture(depthState.texture);
                stencilAttachment->setStoreAction(MTL::StoreActionStore);
            }
        }

        state.needsPresent = true;
    } else {
        std::vector<std::shared_ptr<Texture>> colors =
            collectColorAttachments(framebuffer);
        for (size_t i = 0; i < colors.size() && i < 8; ++i) {
            auto *attachment =
                state.passDescriptor->colorAttachments()->object(i);
            auto &textureState = metal::textureState(colors[i].get());
            attachment->setTexture(textureState.texture);
            attachment->setStoreAction(MTL::StoreActionStore);
        }

        auto depthTexture = collectDepthAttachment(framebuffer);
        if (depthTexture != nullptr) {
            auto *depthAttachment = state.passDescriptor->depthAttachment();
            auto &depthState = metal::textureState(depthTexture.get());
            depthAttachment->setTexture(depthState.texture);
            depthAttachment->setStoreAction(MTL::StoreActionStore);
        }

        auto stencilTexture = collectStencilAttachment(framebuffer);
        if (stencilTexture != nullptr) {
            auto *stencilAttachment = state.passDescriptor->stencilAttachment();
            auto &stencilState = metal::textureState(stencilTexture.get());
            stencilAttachment->setTexture(stencilState.texture);
            stencilAttachment->setStoreAction(MTL::StoreActionStore);
        }
    }

    configureColorAttachmentForClear(state.passDescriptor, 8, clearColorValue,
                                     state.clearColorPending);
    configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                     state.clearDepthPending);

    state.clearColorPending = false;
    state.clearDepthPending = false;
#endif
}

void CommandBuffer::beginSampled(
    [[maybe_unused]] const std::shared_ptr<Framebuffer> &readFramebuffer,
    [[maybe_unused]] const std::shared_ptr<Framebuffer> &writeFramebuffer) {
    if (writeFramebuffer != nullptr) {
        writeFramebuffer->bindForDraw();
    }
    if (readFramebuffer != nullptr) {
        readFramebuffer->bindForRead();
    }
}

void CommandBuffer::endPass() {
#ifdef VULKAN
    if (hasStarted) {
        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        hasStarted = false;

        if (framebuffer != nullptr && !framebuffer->isDefaultFramebuffer) {
            for (auto &attachment : framebuffer->attachments) {
                if (attachment.texture != nullptr) {
                    if (attachment.type == opal::Attachment::Type::Color) {
                        attachment.texture->currentLayout =
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    } else {
                        attachment.texture->currentLayout =
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }
                }
            }
        }
    }
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }
    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
        state.passDescriptor = nullptr;
    }
    state.textureBindingsInitialized = false;
    state.hasDraw = false;
#endif
}

int CommandBuffer::getAndResetDrawCallCount() {
    int count = drawCallCount;
    drawCallCount = 0;
    return count;
}

void CommandBuffer::commit() {
#ifdef VULKAN
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
            std::cout << "commit(): Skipping frame - no render pass!"
                      << std::endl;
            return;
        }

        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;

        beginCommandBufferIfNeeded();
        this->record(imageIndex);
        hasStarted = true;
    }

    if (!imageAcquired) {
        return;
    }

    if (hasStarted) {
        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        hasStarted = false;
    }

    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {device->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(device->presentQueue, &presentInfo);

    imageAcquired = false;
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    if (state.commandBuffer == nullptr) {
        if (state.passDescriptor != nullptr) {
            state.passDescriptor->release();
            state.passDescriptor = nullptr;
        }
        if (state.autoreleasePool != nullptr) {
            state.autoreleasePool->release();
            state.autoreleasePool = nullptr;
        }
        state.textureBindingsInitialized = false;
        return;
    }

    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }

    if (state.passDescriptor != nullptr) {
        state.passDescriptor->release();
        state.passDescriptor = nullptr;
    }

    if (state.needsPresent && state.drawable != nullptr) {
        state.commandBuffer->presentDrawable(state.drawable);
    }

    state.commandBuffer->retain();
    state.inFlightCommandBuffers.push_back(state.commandBuffer);
    state.inFlightResources.push_back(std::move(state.pendingResources));
    state.pendingResources.clear();
    state.commandBuffer->commit();
    state.commandBuffer = nullptr;
    state.passDescriptor = nullptr;
    state.drawable = nullptr;
    state.textureBindingsInitialized = false;
    state.needsPresent = false;
    state.hasDraw = false;
    if (state.autoreleasePool != nullptr) {
        state.autoreleasePool->release();
        state.autoreleasePool = nullptr;
    }
#endif
}

void CommandBuffer::waitForSubmittedWork() {
#ifdef METAL
    auto &state = metal::commandBufferState(this);
    for (auto *submitted : state.inFlightCommandBuffers) {
        submitted->waitUntilCompleted();
        if (submitted->status() == MTL::CommandBufferStatusError) {
            auto *error = submitted->error();
            const char *description =
                error != nullptr && error->localizedDescription() != nullptr
                    ? error->localizedDescription()->utf8String()
                    : "Unknown Metal command buffer error";
            atlas_error(std::string("Metal GPU command failed: ") +
                        description);
        }
        submitted->release();
    }
    state.inFlightCommandBuffers.clear();
    state.inFlightResources.clear();
#endif
}

void CommandBuffer::bindPipeline(const std::shared_ptr<Pipeline> &pipeline) {
#ifdef METAL
    metal::pipelineState(pipeline.get()).suppressTextureReset = true;
#endif
    pipeline->bind();
#ifdef VULKAN
#endif
    boundPipeline = pipeline;
#ifdef VULKAN

    std::shared_ptr<CoreRenderPass> coreRenderPass = nullptr;

    for (auto &corePipeline : RenderPass::cachedRenderPasses) {
        bool renderPassIsDefault =
            renderPass->framebuffer->isDefaultFramebuffer;
        bool cachedIsDefault =
            corePipeline->opalFramebuffer->isDefaultFramebuffer;

        bool framebufferMatch = false;
        if (renderPassIsDefault && cachedIsDefault) {
            framebufferMatch = true;
        } else if (!renderPassIsDefault && !cachedIsDefault) {
            framebufferMatch = (corePipeline->opalFramebuffer->attachments ==
                                renderPass->framebuffer->attachments);
        }

        if (framebufferMatch && *corePipeline->opalPipeline == pipeline) {
            renderPass->currentRenderPass = corePipeline;
            coreRenderPass = corePipeline;
            break;
        }
    }

    if (coreRenderPass == nullptr) {
        if (hasStarted && renderPass->currentRenderPass != nullptr &&
            renderPass->currentRenderPass->renderPass != VK_NULL_HANDLE) {
            coreRenderPass = CoreRenderPass::createWithExistingRenderPass(
                pipeline, renderPass->framebuffer,
                renderPass->currentRenderPass->renderPass);
        } else {
            coreRenderPass =
                CoreRenderPass::create(pipeline, renderPass->framebuffer);
        }
        renderPass->currentRenderPass = coreRenderPass;
    }

    if (framebuffer->isDefaultFramebuffer) {
        if (device->swapchainDirty || framebuffer->vkFramebuffers.size() == 0) {
            framebuffer->vkFramebuffers.resize(
                device->swapChainImages.imageViews.size());
            if (device->swapChainBrightTextures.size() !=
                device->swapChainImages.imageViews.size()) {
                device->createSwapChainBrightTextures();
            }
            for (size_t i = 0; i < device->swapChainImages.imageViews.size();
                 i++) {
                auto &brightTextures = device->swapChainBrightTextures;
                if (brightTextures.size() <= i ||
                    brightTextures[i] == nullptr ||
                    brightTextures[i]->vkImageView == VK_NULL_HANDLE) {
                    throw std::runtime_error(
                        "Swapchain bright attachments are not initialized");
                }

                std::vector<VkImageView> attachments = {
                    device->swapChainImages.imageViews[i],
                    brightTextures[i]->vkImageView};

                if (device->swapChainDepthTexture != nullptr &&
                    device->swapChainDepthTexture->vkImageView !=
                        VK_NULL_HANDLE) {
                    attachments.push_back(
                        device->swapChainDepthTexture->vkImageView);
                }

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType =
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = coreRenderPass->renderPass;
                framebufferInfo.attachmentCount =
                    static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = device->swapChainExtent.width;
                framebufferInfo.height = device->swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(
                        device->logicalDevice, &framebufferInfo, nullptr,
                        &framebuffer->vkFramebuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }

            device->swapchainDirty = false;
        }
    } else {
        if (framebuffer->vkFramebuffers.empty()) {
            framebuffer->createVulkanFramebuffers(coreRenderPass);
        }
    }
#endif
}

void CommandBuffer::unbindPipeline() { boundPipeline = nullptr; }

void CommandBuffer::bindDrawingState(
    std::shared_ptr<DrawingState> drawingState) {
    boundDrawingState = std::move(drawingState);
}

void CommandBuffer::unbindDrawingState() { boundDrawingState = nullptr; }

auto CommandBuffer::draw(uint vertexCount, uint instanceCount, uint firstVertex,
                         [[maybe_unused]] uint firstInstance, int objectId)
    -> void {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    glDrawArraysInstanced(GL_TRIANGLES, firstVertex, vertexCount,
                          instanceCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#elif defined(VULKAN)
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, instanceCount,
              firstVertex, firstInstance);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->instanceBuffer != nullptr) {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    } else {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawPrimitives(pipelineState.primitiveType,
                                  static_cast<NS::UInteger>(firstVertex),
                                  static_cast<NS::UInteger>(vertexCount),
                                  static_cast<NS::UInteger>(instanceCount),
                                  static_cast<NS::UInteger>(firstInstance));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = (int)device->frameCount;
        info.type = DrawCallType::Draw;
        info.send();
    }

    drawCallCount++;
}

void CommandBuffer::drawIndexed(uint indexCount, uint instanceCount,
                                uint firstIndex,
                                [[maybe_unused]] int vertexOffset,
                                [[maybe_unused]] uint firstInstance,
                                int objectId) {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT,
                            (void *)(uintptr_t)(firstIndex * sizeof(uint)),
                            instanceCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#elif defined(VULKAN)
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        vkAcquireNextImageKHR(device->logicalDevice, device->swapChain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);
        imageAcquired = true;
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    if (boundDrawingState != nullptr) {
        bindVertexBuffersIfNeeded();
        if (boundDrawingState->indexBuffer != nullptr) {
            vkCmdBindIndexBuffer(commandBuffers[currentFrame],
                                 boundDrawingState->indexBuffer->vkBuffer, 0,
                                 VK_INDEX_TYPE_UINT32);
        }
    }
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, instanceCount,
                     firstIndex, vertexOffset, firstInstance);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }
    if (boundDrawingState == nullptr ||
        boundDrawingState->indexBuffer == nullptr) {
        return;
    }

    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }

    if (boundDrawingState->instanceBuffer != nullptr) {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    } else {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    }

    auto &indexState = metal::bufferState(boundDrawingState->indexBuffer.get());
    if (indexState.buffer == nullptr) {
        return;
    }

    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawIndexedPrimitives(
        pipelineState.primitiveType, static_cast<NS::UInteger>(indexCount),
        MTL::IndexTypeUInt32, indexState.buffer,
        static_cast<NS::UInteger>(firstIndex * sizeof(uint)),
        static_cast<NS::UInteger>(instanceCount), vertexOffset,
        static_cast<NS::UInteger>(firstInstance));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = (int)device->frameCount;
        info.type = DrawCallType::Indexed;
        info.send();
    }

    drawCallCount++;
}

void CommandBuffer::drawPatches(uint vertexCount, uint firstVertex,
                                int objectId) {
#ifdef OPENGL
    if (boundDrawingState != nullptr) {
        boundDrawingState->bind();
    }
    if (boundPipeline != nullptr) {
        glPatchParameteri(GL_PATCH_VERTICES, boundPipeline->getPatchVertices());
    }
    glDrawArrays(GL_PATCHES, firstVertex, vertexCount);
    if (boundDrawingState != nullptr) {
        boundDrawingState->unbind();
    }
#elif defined(VULKAN)
    if (renderPass == nullptr || renderPass->currentRenderPass == nullptr) {
        return;
    }
    if (!imageAcquired && framebuffer != nullptr &&
        framebuffer->isDefaultFramebuffer) {
        VkResult result = vkAcquireNextImageKHR(
            device->logicalDevice, device->swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
            &imageIndex);
        imageAcquired = true;
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            device->remakeSwapChain(device->context);
            if (framebuffer->isDefaultFramebuffer) {
                framebuffer->vkFramebuffers.resize(
                    device->swapChainImages.imageViews.size());
                if (device->swapChainBrightTextures.size() !=
                    device->swapChainImages.imageViews.size()) {
                    device->createSwapChainBrightTextures();
                }
                for (size_t i = 0;
                     i < device->swapChainImages.imageViews.size(); i++) {
                    auto &brightTextures = device->swapChainBrightTextures;
                    if (brightTextures.size() <= i ||
                        brightTextures[i] == nullptr ||
                        brightTextures[i]->vkImageView == VK_NULL_HANDLE) {
                        throw std::runtime_error(
                            "Swapchain bright attachments are not initialized");
                    }

                    std::vector<VkImageView> attachments = {
                        device->swapChainImages.imageViews[i],
                        brightTextures[i]->vkImageView};

                    if (device->swapChainDepthTexture != nullptr &&
                        device->swapChainDepthTexture->vkImageView !=
                            VK_NULL_HANDLE) {
                        attachments.push_back(
                            device->swapChainDepthTexture->vkImageView);
                    }

                    VkFramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.sType =
                        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferInfo.renderPass =
                        renderPass->currentRenderPass->renderPass;
                    framebufferInfo.attachmentCount =
                        static_cast<uint32_t>(attachments.size());
                    framebufferInfo.pAttachments = attachments.data();
                    framebufferInfo.width = device->swapChainExtent.width;
                    framebufferInfo.height = device->swapChainExtent.height;
                    framebufferInfo.layers = 1;

                    if (vkCreateFramebuffer(
                            device->logicalDevice, &framebufferInfo, nullptr,
                            &framebuffer->vkFramebuffers[i]) != VK_SUCCESS) {
                        throw std::runtime_error(
                            "failed to create framebuffer!");
                    }
                }
            } else {
                device->swapchainDirty = true;
            }
        }
    }
    beginCommandBufferIfNeeded();
    if (!hasStarted) {
        this->record(imageIndex);
        hasStarted = true;
    }
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      renderPass->currentRenderPass->pipeline);
    if (boundPipeline != nullptr) {
        boundPipeline->bindDescriptorSets(commandBuffers[currentFrame]);
    }
    bindVertexBuffersIfNeeded();
    if (boundPipeline != nullptr) {
        VkViewport viewport = boundPipeline->vkViewport;
        if (viewport.width != 0.0f) {
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        } else if (framebuffer != nullptr) {
            VkViewport defaultViewport{};
            defaultViewport.x = 0.0f;
            defaultViewport.y = 0.0f;
            defaultViewport.width = static_cast<float>(framebuffer->width);
            defaultViewport.height = static_cast<float>(framebuffer->height);
            defaultViewport.minDepth = 0.0f;
            defaultViewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1,
                             &defaultViewport);
        }
        boundPipeline->flushPushConstants(commandBuffers[currentFrame]);
    }
    vkCmdDraw(commandBuffers[currentFrame], vertexCount, 1, firstVertex, 0);
#elif defined(METAL)
    if (boundPipeline == nullptr || framebuffer == nullptr) {
        return;
    }
    auto &state = metal::commandBufferState(this);
    ensureRenderEncoder(this, device, framebuffer, boundPipeline,
                        clearColorValue, clearDepthValue);
    if (state.encoder == nullptr) {
        return;
    }

    if (boundDrawingState != nullptr &&
        boundDrawingState->vertexBuffer != nullptr) {
        auto &vertexState =
            metal::bufferState(boundDrawingState->vertexBuffer.get());
        if (vertexState.buffer != nullptr) {
            state.encoder->setVertexBuffer(vertexState.buffer, 0,
                                           kVertexStreamBufferIndex);
        }
    }
    if (boundDrawingState == nullptr ||
        boundDrawingState->instanceBuffer == nullptr) {
        state.encoder->setVertexBytes(
            kIdentityInstanceMatrix,
            static_cast<NS::UInteger>(sizeof(kIdentityInstanceMatrix)),
            kInstanceStreamBufferIndex);
    } else {
        auto &instanceState =
            metal::bufferState(boundDrawingState->instanceBuffer.get());
        if (instanceState.buffer != nullptr) {
            state.encoder->setVertexBuffer(instanceState.buffer, 0,
                                           kInstanceStreamBufferIndex);
        }
    }
    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    state.encoder->drawPrimitives(pipelineState.primitiveType,
                                  static_cast<NS::UInteger>(firstVertex),
                                  static_cast<NS::UInteger>(vertexCount));
    state.hasDraw = true;
#endif

    if (TracerServices::getInstance().isOk()) {
        DrawCallInfo info;
        info.callerObject = std::to_string(objectId);
        info.frameNumber = device->frameCount;
        info.type = DrawCallType::Patch;
        info.send();
    }

    drawCallCount++;
}

void CommandBuffer::dispatch(uint threadCountX, uint threadCountY,
                             uint threadCountZ) {
#ifdef OPENGL
    (void)threadCountX;
    (void)threadCountY;
    (void)threadCountZ;
    throw std::runtime_error("Compute dispatch is not supported on OpenGL");
#elif defined(VULKAN)
    (void)threadCountX;
    (void)threadCountY;
    (void)threadCountZ;
    throw std::runtime_error("Compute dispatch is not implemented for Vulkan");
#elif defined(METAL)
    if (boundPipeline == nullptr || boundPipeline->shaderProgram == nullptr) {
        return;
    }
    if (!boundPipeline->shaderProgram->isComputeProgram()) {
        throw std::runtime_error("Dispatch requires a compute shader program");
    }
    if (device == nullptr) {
        throw std::runtime_error("Metal command buffer has no device");
    }

    auto &deviceState = metal::deviceState(device);
    if (deviceState.device == nullptr || deviceState.queue == nullptr) {
        throw std::runtime_error("Metal device queue is not initialized");
    }

    auto &state = metal::commandBufferState(this);
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
        state.textureBindingsInitialized = false;
    }
    if (state.commandBuffer == nullptr) {
        state.commandBuffer = deviceState.queue->commandBuffer();
    }
    if (state.computeEncoder == nullptr) {
        state.computeEncoder = state.commandBuffer->computeCommandEncoder();
        if (state.computeEncoder == nullptr) {
            throw std::runtime_error(
                "Failed to create Metal compute command encoder");
        }
    }

    MTL::ComputePipelineState *computeState =
        getComputePipelineState(device, boundPipeline);
    if (computeState == nullptr) {
        throw std::runtime_error(
            "Metal compute pipeline state creation failed");
    }
    state.computeEncoder->setComputePipelineState(computeState);
    uploadComputeUniformBuffers(boundPipeline, state.computeEncoder,
                                deviceState.device);
    bindComputeTextures(boundPipeline, state.computeEncoder,
                        deviceState.device);
    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    for (const auto &[binding, accelerationStructure] :
         pipelineState.primitiveAccelerationStructures) {
        if (accelerationStructure == nullptr ||
            accelerationStructure->blas == nullptr ||
            !accelerationStructure->isBuilt) {
            throw std::runtime_error(
                "Metal primitive acceleration structure is unavailable");
        }
        state.computeEncoder->setAccelerationStructure(
            accelerationStructure->blas, binding);
    }
    for (const auto &[binding, accelerationStructure] :
         pipelineState.instanceAccelerationStructures) {
        if (accelerationStructure == nullptr ||
            accelerationStructure->tlas == nullptr ||
            !accelerationStructure->isBuilt) {
            throw std::runtime_error(
                "Metal instance acceleration structure is unavailable");
        }
        state.computeEncoder->setAccelerationStructure(
            accelerationStructure->tlas, binding);
    }

    NS::UInteger tgX =
        static_cast<NS::UInteger>(boundPipeline->getComputeThreadgroupSizeX());
    NS::UInteger tgY =
        static_cast<NS::UInteger>(boundPipeline->getComputeThreadgroupSizeY());
    NS::UInteger tgZ =
        static_cast<NS::UInteger>(boundPipeline->getComputeThreadgroupSizeZ());
    tgX = std::max<NS::UInteger>(1, tgX);
    tgY = std::max<NS::UInteger>(1, tgY);
    tgZ = std::max<NS::UInteger>(1, tgZ);

    NS::UInteger maxThreads = std::max<NS::UInteger>(
        1, computeState->maxTotalThreadsPerThreadgroup());
    while (tgX * tgY * tgZ > maxThreads) {
        if (tgZ > 1) {
            tgZ = std::max<NS::UInteger>(1, tgZ / 2);
            continue;
        }
        if (tgY > 1) {
            tgY = std::max<NS::UInteger>(1, tgY / 2);
            continue;
        }
        if (tgX > 1) {
            tgX = std::max<NS::UInteger>(1, tgX / 2);
            continue;
        }
        break;
    }

    NS::UInteger countX = std::max<NS::UInteger>(1, threadCountX);
    NS::UInteger countY = std::max<NS::UInteger>(1, threadCountY);
    NS::UInteger countZ = std::max<NS::UInteger>(1, threadCountZ);
    MTL::Size threadsPerGroup = MTL::Size(tgX, tgY, tgZ);
    MTL::Size threadgroups =
        MTL::Size((countX + tgX - 1) / tgX, (countY + tgY - 1) / tgY,
                  (countZ + tgZ - 1) / tgZ);
    state.computeEncoder->dispatchThreadgroups(threadgroups, threadsPerGroup);
    state.hasDraw = true;
#endif
}

void CommandBuffer::computeBarrier() {
#ifdef METAL
    auto &state = metal::commandBufferState(this);
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->memoryBarrier(MTL::BarrierScope(
            MTL::BarrierScopeBuffers | MTL::BarrierScopeTextures));
    }
#endif
}

void CommandBuffer::generateMipmaps(const std::shared_ptr<Texture> &texture) {
    if (texture == nullptr) {
        return;
    }
#ifdef OPENGL
    texture->generateMipmaps(0);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    auto &textureState = metal::textureState(texture.get());
    if (state.commandBuffer == nullptr || textureState.texture == nullptr ||
        textureState.texture->mipmapLevelCount() <= 1) {
        return;
    }
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
    }
    if (state.computeEncoder != nullptr) {
        state.computeEncoder->endEncoding();
        state.computeEncoder = nullptr;
    }
    auto *blitEncoder = state.commandBuffer->blitCommandEncoder();
    blitEncoder->generateMipmaps(textureState.texture);
    blitEncoder->endEncoding();
#endif
}

bool CommandBuffer::performSpatialUpscale(
    const std::shared_ptr<Texture> &sourceTexture) {
#ifdef METAL
#ifdef __APPLE__
    return encodeMetalFXSpatialUpscale(this, device, sourceTexture);
#else
    (void)sourceTexture;
    return false;
#endif
#else
    (void)sourceTexture;
    return false;
#endif
}

#ifdef VULKAN
void CommandBuffer::bindVertexBuffersIfNeeded() {
    if (boundDrawingState == nullptr ||
        boundDrawingState->vertexBuffer == nullptr) {
        return;
    }

    Pipeline *activePipeline = nullptr;
    if (renderPass != nullptr && renderPass->currentRenderPass != nullptr &&
        renderPass->currentRenderPass->opalPipeline != nullptr) {
        activePipeline = renderPass->currentRenderPass->opalPipeline.get();
    }

    std::array<VkBuffer, 2> buffers = {
        boundDrawingState->vertexBuffer->vkBuffer, VK_NULL_HANDLE};
    std::array<VkDeviceSize, 2> offsets = {0, 0};
    uint32_t bindingCount = 1;

    bool needsInstanceBinding =
        activePipeline != nullptr && activePipeline->hasInstanceAttributes;

    VkBuffer instanceBufferHandle = VK_NULL_HANDLE;
    if (boundDrawingState->instanceBuffer != nullptr) {
        instanceBufferHandle = boundDrawingState->instanceBuffer->vkBuffer;
    }

    if (needsInstanceBinding && instanceBufferHandle == VK_NULL_HANDLE &&
        device != nullptr) {
        auto fallback = device->getDefaultInstanceBuffer();
        if (fallback != nullptr) {
            instanceBufferHandle = fallback->vkBuffer;
        }
    }

    if (needsInstanceBinding || instanceBufferHandle != VK_NULL_HANDLE) {
        buffers[1] = instanceBufferHandle;
        bindingCount = 2;
    }

    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, bindingCount,
                           buffers.data(), offsets.data());
}
#endif

void CommandBuffer::clearColor(float r, float g, float b, float a) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearColorPending = true;
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, 8,
                                         clearColorValue, true);
    }
#endif
}

void CommandBuffer::clearDepth(float depth) {
#ifdef OPENGL
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearDepthPending = true;
#endif
    this->clearDepthValue = depth;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         true);
    }
#endif
}

void CommandBuffer::clear(float r, float g, float b, float a, float depth) {
#ifdef OPENGL
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#elif defined(METAL)
    auto &state = metal::commandBufferState(this);
    state.clearColorPending = true;
    state.clearDepthPending = true;
#endif
    this->clearColorValue[0] = r;
    this->clearColorValue[1] = g;
    this->clearColorValue[2] = b;
    this->clearColorValue[3] = a;
    this->clearDepthValue = depth;

#ifdef METAL
    if (state.passDescriptor != nullptr && state.encoder == nullptr) {
        configureColorAttachmentForClear(state.passDescriptor, 8,
                                         clearColorValue, true);
        configureDepthAttachmentForClear(state.passDescriptor, clearDepthValue,
                                         true);
    }
#endif
}

#ifdef METAL
void CommandBuffer::bindPrimitiveAccelerationStructure(
    const std::shared_ptr<PrimitiveAccelerationStructure> &as,
    uint32_t binding) {
    if (boundPipeline == nullptr) {
        throw std::runtime_error(
            "Cannot bind an acceleration structure without a pipeline");
    }
    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    pipelineState.instanceAccelerationStructures.erase(binding);
    pipelineState.primitiveAccelerationStructures[binding] = as;
}

void CommandBuffer::bindInstanceAccelerationStructure(
    const std::shared_ptr<InstanceAccelerationStructure> &as,
    uint32_t binding) {
    if (boundPipeline == nullptr) {
        throw std::runtime_error(
            "Cannot bind an acceleration structure without a pipeline");
    }
    auto &pipelineState = metal::pipelineState(boundPipeline.get());
    pipelineState.primitiveAccelerationStructures.erase(binding);
    pipelineState.instanceAccelerationStructures[binding] = as;
}
#endif

} // namespace opal
