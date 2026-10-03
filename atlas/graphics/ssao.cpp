//
// ssao.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: SSAO Implementation for the Window Class
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/window.h"
#include "atlas/tracer/log.h"
#include <algorithm>
#include <cstddef>
#include <glm/geometric.hpp>
#include <glm/gtc/random.hpp>
#include <random>
#include <utility>
#include <vector>
#include "opal/opal.h"

void Window::setupSSAO() {
    this->ssaoKernelSize = std::max(this->ssaoKernelSize, 16);
    atlas_log("Setting up SSAO (kernel size: " +
              std::to_string(this->ssaoKernelSize) + ")");
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    static std::default_random_engine generator([] {
        std::random_device rd;
        return std::default_random_engine(rd());
    }());
    this->ssaoKernel.clear();
    this->ssaoKernel.reserve(this->ssaoKernelSize);
    for (int i = 0; i < this->ssaoKernelSize; ++i) {
        glm::vec3 sample((randomFloats(generator) * 2.0) - 1.0,
                         (randomFloats(generator) * 2.0) - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = static_cast<float>(i) /
                      static_cast<float>(std::max(1, this->ssaoKernelSize));
        scale = 0.1f + (0.9f * (scale * scale));
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    this->ssaoNoise.clear();
    this->ssaoNoise.reserve(16);
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise((randomFloats(generator) * 2.0) - 1.0,
                        (randomFloats(generator) * 2.0) - 1.0, 0.0f);
        this->ssaoNoise.push_back(noise);
    }

    noiseTexture.texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Rgb16F, 4, 4,
        opal::TextureDataFormat::Rgb, this->ssaoNoise.data(), 1);
    noiseTexture.texture->setFilterMode(opal::TextureFilterMode::Nearest,
                                        opal::TextureFilterMode::Nearest);
    noiseTexture.texture->setWrapMode(opal::TextureAxis::S,
                                      opal::TextureWrapMode::Repeat);
    noiseTexture.texture->setWrapMode(opal::TextureAxis::T,
                                      opal::TextureWrapMode::Repeat);
    noiseTexture.id = noiseTexture.texture->textureID;
    noiseTexture.creationData.width = 4;
    noiseTexture.creationData.height = 4;
    noiseTexture.type = TextureType::SSAONoise;

    this->ssaoProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSAO);
    this->ssaoBlurProgram = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Light, AtlasFragmentShader::SSAOBlur);

    this->ssaoBuffer = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::SSAO));
    this->ssaoBlurBuffer = std::make_shared<RenderTarget>(
        RenderTarget(*this, RenderTargetType::SSAOBlur));
    this->ssaoMapsDirty = true;
}

void Window::renderSSAO(std::shared_ptr<opal::CommandBuffer> commandBuffer) {
    if (this->ssaoBuffer == nullptr || this->ssaoBlurBuffer == nullptr) {
        return;
    }

    bool ownsCommandBuffer = false;
    auto ssaoCommandBuffer = std::move(commandBuffer);
    if (ssaoCommandBuffer == nullptr) {
        ssaoCommandBuffer = Window::mainWindow->device->acquireCommandBuffer();
        ssaoCommandBuffer->start();
        ownsCommandBuffer = true;
    }
    ssaoCommandBuffer->clearColor(1.0f, 1.0f, 1.0f, 1.0f);
    static std::shared_ptr<opal::DrawingState> ssaoState = nullptr;
    static std::shared_ptr<opal::Buffer> renderSSAOBuffer = nullptr;
    if (ssaoState == nullptr) {
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
        renderSSAOBuffer =
            opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                 sizeof(quadVertices), quadVertices);
        ssaoState = opal::DrawingState::create(renderSSAOBuffer);
        ssaoState->setBuffers(renderSSAOBuffer, nullptr);

        opal::VertexAttribute positionAttr{
            .name = "ssaoPosition",
            .type = opal::VertexAttributeType::Float,
            .offset = static_cast<unsigned int>(offsetof(CoreVertex, position)),
            .location = 0,
            .normalized = false,
            .size = 3,
            .stride = static_cast<unsigned int>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};
        opal::VertexAttribute uvAttr{
            .name = "ssaoUV",
            .type = opal::VertexAttributeType::Float,
            .offset =
                static_cast<unsigned int>(offsetof(CoreVertex, textureCoordinate)),
            .location = 2,
            .normalized = false,
            .size = 2,
            .stride = static_cast<unsigned int>(sizeof(CoreVertex)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};

        std::vector<opal::VertexAttributeBinding> bindings = {
            {.attribute = positionAttr, .sourceBuffer = renderSSAOBuffer},
            {.attribute = uvAttr, .sourceBuffer = renderSSAOBuffer}};
        ssaoState->configureAttributes(bindings);
    }

    static std::shared_ptr<opal::Pipeline> ssaoPipeline = nullptr;
    if (ssaoPipeline == nullptr) {
        ssaoPipeline = opal::Pipeline::create();
    }
    ssaoPipeline = this->ssaoProgram.requestPipeline(ssaoPipeline);
    ssaoPipeline->setCullMode(opal::CullMode::None);
    ssaoPipeline->enableDepthTest(false);
    ssaoPipeline->enableDepthWrite(false);
    ssaoPipeline->enableBlending(false);
    ssaoPipeline->setViewport(0, 0, this->ssaoBuffer->getWidth(),
                              this->ssaoBuffer->getHeight());
    auto ssaoRenderPass = opal::RenderPass::create();
    ssaoRenderPass->setFramebuffer(this->ssaoBuffer->getFramebuffer());
    ssaoCommandBuffer->beginPass(ssaoRenderPass);
    ssaoPipeline->bind();
    ssaoCommandBuffer->bindPipeline(ssaoPipeline);

    ssaoPipeline->bindTexture2D("gPosition", this->gBuffer->gPosition.id, 0);
    ssaoPipeline->bindTexture2D("gNormal", this->gBuffer->gNormal.id, 1);
    ssaoPipeline->bindTexture2D("texNoise", this->noiseTexture.id, 2);
    for (size_t i = 0; i < this->ssaoKernel.size(); ++i) {
        ssaoPipeline->setUniform3f("samples[" + std::to_string(i) + "]",
                                   ssaoKernel[i].x, ssaoKernel[i].y,
                                   ssaoKernel[i].z);
    }
    ssaoPipeline->setUniform1i("kernelSize",
                               static_cast<int>(this->ssaoKernel.size()));
    ssaoPipeline->setUniformMat4f("projection",
                                  this->calculateProjectionMatrix());
    ssaoPipeline->setUniformMat4f(
        "inverseProjection", glm::inverse(this->calculateProjectionMatrix()));
    ssaoPipeline->setUniformMat4f("view", getCamera()->calculateViewMatrix());
    glm::vec2 screenSize(this->ssaoBuffer->getWidth(),
                         this->ssaoBuffer->getHeight());
    glm::vec2 noiseSize(4.0f, 4.0f);
    ssaoPipeline->setUniform2f("noiseScale", screenSize.x / noiseSize.x,
                               screenSize.y / noiseSize.y);
    ssaoCommandBuffer->bindDrawingState(ssaoState);
    ssaoCommandBuffer->draw(6, 1, 0, 0);
    ssaoCommandBuffer->unbindDrawingState();
    ssaoCommandBuffer->endPass();

    ssaoCommandBuffer->clearColor(1.0f, 1.0f, 1.0f, 1.0f);

    static std::shared_ptr<opal::Pipeline> ssaoBlurPipeline = nullptr;
    if (ssaoBlurPipeline == nullptr) {
        ssaoBlurPipeline = opal::Pipeline::create();
    }
    ssaoBlurPipeline = this->ssaoBlurProgram.requestPipeline(ssaoBlurPipeline);
    ssaoBlurPipeline->setCullMode(opal::CullMode::None);
    ssaoBlurPipeline->enableDepthTest(false);
    ssaoBlurPipeline->enableDepthWrite(false);
    ssaoBlurPipeline->enableBlending(false);
    ssaoBlurPipeline->setViewport(0, 0, this->ssaoBlurBuffer->getWidth(),
                                  this->ssaoBlurBuffer->getHeight());
    auto ssaoBlurRenderPass = opal::RenderPass::create();
    ssaoBlurRenderPass->setFramebuffer(this->ssaoBlurBuffer->getFramebuffer());
    ssaoCommandBuffer->beginPass(ssaoBlurRenderPass);
    ssaoBlurPipeline->bind();
    ssaoCommandBuffer->bindPipeline(ssaoBlurPipeline);

    ssaoBlurPipeline->bindTexture2D("inSSAO", this->ssaoBuffer->texture.id, 0);
    ssaoCommandBuffer->bindDrawingState(ssaoState);
    ssaoCommandBuffer->draw(6, 1, 0, 0);
    ssaoCommandBuffer->unbindDrawingState();
    ssaoCommandBuffer->endPass();
    if (ownsCommandBuffer) {
        ssaoCommandBuffer->commit();
    }

}
