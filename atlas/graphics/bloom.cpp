/*
 bloom.cpp
 As part of the Atlas project
 Created by Maxims Enterprise in 2024
 --------------------------------------------------
 Description:
 Copyright (c) 2025 Max Van den Eynde
*/

#include "atlas/core/shader.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <atlas/texture.h>
#include <algorithm>
#include <climits>
#include <cstddef>
#include <utility>
#include <vector>
#include "opal/opal.h"

void BloomRenderTarget::init(int width, int height, int chainLength) {
    if (initialized) {
        return;
    }
    chainLength = std::max(1, chainLength);
    if (width <= 0 || height <= 0) {
        return;
    }
    if (width > INT_MAX || height > INT_MAX) {
        atlas_error("Texture dimensions exceed maximum allowed");
        return;
    }

    atlas_log("Initializing bloom system (chain length: " +
        std::to_string(chainLength) + ", resolution: " +
        std::to_string(width) + "x" + std::to_string(height) + ")");
    initialized = true;

    this->framebuffer = opal::Framebuffer::create();
    this->elements.clear();

    glm::vec2 mipSize((float)width, (float)height);
    glm::ivec2 mipIntSize(width, height);

    this->srcViewportSize = mipIntSize;
    this->srcViewportSizef = mipSize;

    for (int i = 0; i < chainLength; i++) {
        BloomElement element;
        mipSize.x = std::max(1.0f, mipSize.x * 0.5f);
        mipSize.y = std::max(1.0f, mipSize.y * 0.5f);
        mipIntSize.x = std::max(1, mipIntSize.x / 2);
        mipIntSize.y = std::max(1, mipIntSize.y / 2);
        element.size = mipSize;
        element.intSize = mipIntSize;

        auto opalTexture = opal::Texture::create(
            opal::TextureType::Texture2D, opal::TextureFormat::Rgb16F,
            element.intSize.x, element.intSize.y);
        opalTexture->setFilterMode(opal::TextureFilterMode::Linear,
                                   opal::TextureFilterMode::Linear);
        opalTexture->setWrapMode(opal::TextureAxis::S,
                                 opal::TextureWrapMode::ClampToEdge);
        opalTexture->setWrapMode(opal::TextureAxis::T,
                                 opal::TextureWrapMode::ClampToEdge);

        element.textureId = opalTexture->textureID;
        element.texture = opalTexture;

        this->elements.push_back(element);
    }

    if (elements.empty()) {
        this->framebuffer = nullptr;
        initialized = false;
        return;
    }

    this->framebuffer->attachTexture(elements.at(0).texture, 0);
    this->framebuffer->setDrawBuffers(1);

    this->framebuffer->unbind();

    downsampleProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Downsample);
    upsampleProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::Upsample);

    if (quadState == nullptr) {
        CoreVertex quadVertices[] = {
#if defined(METAL) || defined(VULKAN)
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{-1.0f, -1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}},
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}}
#else
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{-1.0f, -1.0f, 0.0f}, Color::white(), {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}, Color::white(), {0.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, Color::white(), {1.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f}, Color::white(), {1.0f, 1.0f}}
#endif
        };

        quadBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                          sizeof(quadVertices), quadVertices);
        quadState = opal::DrawingState::create(quadBuffer);
        quadState->setBuffers(quadBuffer, nullptr);

        opal::VertexAttribute positionAttr{
            .name = "bloomPosition",
            .type = opal::VertexAttributeType::Float,
            .offset = static_cast<unsigned int>(offsetof(CoreVertex, position)),
            .location = 0,
            .normalized = false,
            .size = 3,
            .stride = static_cast<unsigned int>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0
        };
        opal::VertexAttribute uvAttr{
            .name = "bloomUV",
            .type = opal::VertexAttributeType::Float,
            .offset =
            static_cast<unsigned int>(offsetof(CoreVertex, textureCoordinate)),
            .location = 2,
            .normalized = false,
            .size = 2,
            .stride = static_cast<unsigned int>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0
        };

        std::vector<opal::VertexAttributeBinding> bindings = {
            {.attribute = positionAttr, .sourceBuffer = quadBuffer}, {.attribute = uvAttr, .sourceBuffer = quadBuffer}
        };
        quadState->configureAttributes(bindings);
    }
}

void BloomRenderTarget::destroy() {
    for (auto& element : elements) {
        element.textureId = 0;
        element.texture = nullptr;
    }
    elements.clear();
    this->framebuffer = nullptr;
    this->srcViewportSize = glm::ivec2(0);
    this->srcViewportSizef = glm::vec2(0.0f);
    initialized = false;
}

const std::vector<BloomElement>& BloomRenderTarget::getElements() const {
    return elements;
}

void BloomRenderTarget::renderBloomTexture(
    unsigned int srcTexture, float filterRadius,
    std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (srcTexture == 0 || !initialized || framebuffer == nullptr ||
        elements.empty()) {
        return;
    }

    auto bloomCommandBuffer = std::move(commandBuffer);
    bool ownsCommandBuffer = false;
    if (bloomCommandBuffer == nullptr && Window::mainWindow != nullptr &&
        Window::mainWindow->device != nullptr) {
        bloomCommandBuffer = Window::mainWindow->device->acquireCommandBuffer();
        bloomCommandBuffer->start();
        ownsCommandBuffer = true;
    }
    if (bloomCommandBuffer == nullptr) {
        return;
    }

    this->bindForWriting();

    this->renderDownsamples(srcTexture, bloomCommandBuffer);
    this->renderUpsamples(filterRadius, bloomCommandBuffer);

    this->framebuffer->unbind();
    if (ownsCommandBuffer) {
        bloomCommandBuffer->commit();
    }

    if (Window::mainWindow != nullptr &&
        Window::mainWindow->getDevice() != nullptr) {
        Window::mainWindow->getDevice()->getDefaultFramebuffer()->setViewport(
            0, 0, srcViewportSize.x, srcViewportSize.y);
    }
}

unsigned int BloomRenderTarget::getBloomTexture() {
    if (elements.empty()) {
        return 0;
    }
    return elements.at(0).textureId;
}

void BloomRenderTarget::renderDownsamples(
    unsigned int srcTexture,
    const std::shared_ptr<opal::CommandBuffer>& commandBuffer) {
    if (commandBuffer == nullptr) {
        return;
    }

    static std::shared_ptr<opal::Pipeline> downsamplePipeline = nullptr;
    if (downsamplePipeline == nullptr) {
        downsamplePipeline = opal::Pipeline::create();
    }
    downsamplePipeline = downsampleProgram.requestPipeline(downsamplePipeline);
    downsamplePipeline->setCullMode(opal::CullMode::None);
    downsamplePipeline->enableDepthTest(false);
    downsamplePipeline->enableDepthWrite(false);
    downsamplePipeline->enableBlending(false);
    downsamplePipeline->bind();

    downsamplePipeline->setUniform2f("srcResolution", srcViewportSizef.x,
                                     srcViewportSizef.y);
    downsamplePipeline->bindTexture2D("srcTexture", srcTexture, 0);
    this->framebuffer->setDrawBuffers(1);

    for (const auto& element : elements) {
        this->framebuffer->width = element.intSize.x;
        this->framebuffer->height = element.intSize.y;
        this->framebuffer->setViewport(0, 0, element.intSize.x,
                                       element.intSize.y);
        this->framebuffer->attachTexture(element.texture, 0);

        auto renderPass = opal::RenderPass::create();
        renderPass->setFramebuffer(this->framebuffer);
        commandBuffer->beginPass(renderPass);
        commandBuffer->bindPipeline(downsamplePipeline);
        commandBuffer->bindDrawingState(quadState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();
        commandBuffer->endPass();

        downsamplePipeline->setUniform2f("srcResolution", element.size.x,
                                         element.size.y);
        downsamplePipeline->bindTexture2D("srcTexture", element.textureId, 0);
    }
}

void BloomRenderTarget::renderUpsamples(
    float filterRadius,
    const std::shared_ptr<opal::CommandBuffer>& commandBuffer) {
    if (commandBuffer == nullptr) {
        return;
    }

    static std::shared_ptr<opal::Pipeline> upsamplePipeline = nullptr;
    if (upsamplePipeline == nullptr) {
        upsamplePipeline = opal::Pipeline::create();
    }
    upsamplePipeline = upsampleProgram.requestPipeline(upsamplePipeline);
    upsamplePipeline->setCullMode(opal::CullMode::None);
    upsamplePipeline->enableDepthTest(false);
    upsamplePipeline->enableDepthWrite(false);
    upsamplePipeline->enableBlending(true);
    upsamplePipeline->setBlendFunc(opal::BlendFunc::One, opal::BlendFunc::One);
    upsamplePipeline->setBlendEquation(opal::BlendEquation::Add);
    upsamplePipeline->bind();

    upsamplePipeline->setUniform1f("filterRadius", filterRadius);
    this->framebuffer->setDrawBuffers(1);

    for (int i = elements.size() - 1; i > 0; i--) {
        const BloomElement& element = elements.at(i);
        const BloomElement& nextElement = elements.at(i - 1);

        upsamplePipeline->bindTexture2D("srcTexture", element.textureId, 0);
        upsamplePipeline->setUniform2f("srcResolution", element.size.x,
                                       element.size.y);

        this->framebuffer->width = nextElement.intSize.x;
        this->framebuffer->height = nextElement.intSize.y;
        this->framebuffer->setViewport(0, 0, nextElement.intSize.x,
                                       nextElement.intSize.y);
        this->framebuffer->attachTexture(nextElement.texture, 0);

        auto renderPass = opal::RenderPass::create();
        renderPass->setFramebuffer(this->framebuffer);
        commandBuffer->beginPass(renderPass);
        commandBuffer->bindPipeline(upsamplePipeline);
        commandBuffer->bindDrawingState(quadState);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->unbindDrawingState();
        commandBuffer->endPass();
    }

    upsamplePipeline->setBlendFunc(opal::BlendFunc::One,
                                   opal::BlendFunc::OneMinusSrcAlpha);
    upsamplePipeline->enableBlending(false);
    upsamplePipeline->bind();
}

void BloomRenderTarget::bindForWriting() { this->framebuffer->bind(); }
