//
// text.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Text rendering implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "graphite/text.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include "ft2build.h" // IWYU pragma: keep
#include <algorithm>
#include <iostream>
#include FT_FREETYPE_H
#include <vector>

namespace {

graphite::UIStyle makeFallbackStyle(const Text &text) {
    graphite::UIStyle style;
    style.normal().foreground(text.color).font(text.font);
    if (text.fontSize > 0.0f) {
        style.normal().fontSize(text.fontSize);
    }
    return style;
}

} // namespace

Font Font::fromResource(const std::string &fontName, const Resource &resource,
                        int fontSize) {
    atlas_log("Loading font: " + fontName +
              " (size: " + std::to_string(fontSize) + ")");
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        atlas_error("Could not initialize FreeType Library");
        return Font();
    }

    FT_Face face;
    if (FT_New_Face(ft, resource.path.c_str(), 0, &face)) {
        atlas_error("Failed to load font: " + std::string(resource.path));
        return Font();
    }

    int dpi = 96;
    FT_Set_Char_Size(face, 0, fontSize * 64, dpi, dpi);

    Font font;

    const int atlasWidth = 1024;
    const int atlasHeight = 1024;
    std::vector<unsigned char> atlasData(atlasWidth * atlasHeight, 0);

    int currentX = 0;
    int currentY = 0;
    int rowHeight = 0;
    const int padding = 1;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            atlas_warning("Failed to load Glyph: " +
                          std::to_string(static_cast<int>(c)));
            std::cerr << "Failed to load Glyph: " << c << std::endl;
            continue;
        }

        int width = face->glyph->bitmap.width;
        int height = face->glyph->bitmap.rows;

        if (currentX + width + padding > atlasWidth) {
            currentX = 0;
            currentY += rowHeight + padding;
            rowHeight = 0;
        }

        if (currentY + height + padding > atlasHeight) {
            atlas_warning("Font atlas full for font: " + fontName);
            std::cerr << "Font atlas full for font: " << fontName << std::endl;
            break;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                atlasData[((currentY + y) * atlasWidth) + (currentX + x)] =
                    face->glyph->bitmap.buffer[(y * width) + x];
            }
        }

        Character character = {
            .size = Size2d(width, height),
            .bearing =
                Position2d(face->glyph->bitmap_left, face->glyph->bitmap_top),
            .advance = static_cast<unsigned int>(face->glyph->advance.x),
            // UV Min (Top-Left)
            .uvMin = Position2d(static_cast<float>(currentX) / atlasWidth,
                                static_cast<float>(currentY) / atlasHeight),
            // UV Max (Bottom-Right)
            .uvMax = Position2d(
                static_cast<float>(currentX + width) / atlasWidth,
                static_cast<float>(currentY + height) / atlasHeight)};

        font.atlas.insert(std::pair<char, Character>(c, character));

        currentX += width + padding;
        rowHeight = std::max(rowHeight, height);
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Red8, atlasWidth,
        atlasHeight, opal::TextureDataFormat::Red, atlasData.data(), 1);

    opalTexture->setParameters(
        opal::TextureWrapMode::ClampToEdge, opal::TextureWrapMode::ClampToEdge,
        opal::TextureFilterMode::Linear, opal::TextureFilterMode::Linear);

    font.texture = opalTexture;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    font.name = fontName;
    font.size = fontSize;
    font.resource = resource;

    Font::fonts.push_back(font);
    return font;
}

Font &Font::getFont(const std::string &fontName) {
    for (auto &font : fonts) {
        if (font.name == fontName) {
            return font;
        }
    }
    atlas_error("Font not found: " + fontName);
    return fonts[0]; // Return a default font or handle this case appropriately
}

void Font::changeSize(int newSize) {
    if (newSize == size) {
        return;
    }
    Font newFont = Font::fromResource(name, resource, newSize);
    atlas = newFont.atlas;
    texture = newFont.texture;
    size = newSize;
}

std::vector<Font> Font::fonts = {};

void Text::initialize() {
    for (auto &component : components) {
        component->init();
    }
    Window *window = Window::mainWindow;
    Size2d framebufferSize = window->getSize();
    int fbWidth = static_cast<int>(framebufferSize.width);
    int fbHeight = static_cast<int>(framebufferSize.height);

#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    vertexBufferCapacity = sizeof(float) * 6 * 4; // one glyph quad
    vertexBuffer = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                                        vertexBufferCapacity, nullptr,
                                        opal::MemoryUsageType::CPUToGPU, id);
    vao = opal::DrawingState::create(vertexBuffer);
    vao->setBuffers(vertexBuffer, nullptr);

    opal::VertexAttribute textAttribute{
        .name = "textVertex",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 4,
        .stride = static_cast<uint>(4 * sizeof(float)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    std::vector<opal::VertexAttributeBinding> bindings = {
        {textAttribute, vertexBuffer}};
    vao->configureAttributes(bindings);

    shader = ShaderProgram::fromDefaultShaders(AtlasVertexShader::Text,
                                               AtlasFragmentShader::Text);
}

void Text::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                  bool updatePipeline) {
    (void)updatePipeline;
    if (shader.shader == nullptr || vao == nullptr || vertexBuffer == nullptr) {
        initialize();
    }
    for (auto &component : components) {
        component->update(dt);
    }
    if (commandBuffer == nullptr) {
        atlas_error("Text::render requires a valid command buffer");
        return;
    }

    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().text,
        usesLocalStyle ? &localStyle : nullptr);
    const Font &resolvedFont = style.font != nullptr ? *style.font : font;
    if ((style.backgroundColor.a > 0.0f) ||
        (style.borderWidth > 0.0f && style.borderColor.a > 0.0f)) {
        graphite::renderStyledBox(backgroundRenderer, id, commandBuffer, position,
                                  getSize(), style);
    }

    static std::shared_ptr<opal::Pipeline> textPipeline = nullptr;
    Size2d framebufferSize = Window::mainWindow->getSize();
    int fbWidth = static_cast<int>(framebufferSize.width);
    int fbHeight = static_cast<int>(framebufferSize.height);

    if (textPipeline == nullptr) {
        textPipeline = opal::Pipeline::create();

        opal::VertexAttribute textAttribute{
            .name = "vertex",
            .type = opal::VertexAttributeType::Float,
            .offset = 0,
            .location = 0,
            .normalized = false,
            .size = 4,
            .stride = static_cast<uint>(4 * sizeof(float)),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0};
        std::vector<opal::VertexAttribute> textAttributes = {textAttribute};
        opal::VertexBinding textBinding{
            .stride = static_cast<uint>(4 * sizeof(float)),
            .inputRate = opal::VertexBindingInputRate::Vertex};
        textPipeline->setVertexAttributes(textAttributes, textBinding);
        textPipeline->setShaderProgram(shader.shader);
#ifdef VULKAN
        textPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        textPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        textPipeline->setCullMode(opal::CullMode::None);
        textPipeline->enableDepthTest(false);
        textPipeline->enableDepthWrite(false);
        textPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        textPipeline->enableBlending(true);
        textPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                   opal::BlendFunc::OneMinusSrcAlpha);
        textPipeline->build();
    } else {
#ifdef VULKAN
        textPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        textPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        textPipeline->setShaderProgram(shader.shader);
    }

    textPipeline->enableBlending(true);
    textPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                               opal::BlendFunc::OneMinusSrcAlpha);
    textPipeline->enableDepthTest(false);

    commandBuffer->bindPipeline(textPipeline);

    framebufferSize = Window::mainWindow->getSize();
    fbWidth = static_cast<int>(framebufferSize.width);
    fbHeight = static_cast<int>(framebufferSize.height);
#if defined(VULKAN) || defined(METAL)
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                            static_cast<float>(fbHeight), 0.0f);
#else
    projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                            static_cast<float>(fbHeight));
#endif

    textPipeline->setUniform3f("textColor", style.foregroundColor.r,
                               style.foregroundColor.g,
                               style.foregroundColor.b);
    textPipeline->setUniformMat4f("projection", projection);

    if (resolvedFont.texture) {
        textPipeline->bindTexture2D("text", resolvedFont.texture->textureID, 0,
                                    id);
    }

    commandBuffer->bindDrawingState(vao);

    float scale = graphite::resolveTextScale(resolvedFont, style.fontSize);

    float maxBearingY = 0.0f;
    for (const char &ch : content) {
        const auto it = resolvedFont.atlas.find(ch);
        if (it == resolvedFont.atlas.end()) {
            continue;
        }
        maxBearingY = std::max(it->second.bearing.y, maxBearingY);
    }

    float x = position.x + style.padding.width;
    float y = position.y + style.padding.height + (maxBearingY * scale);

    std::string::const_iterator c;
    const size_t glyphCount = content.size();
    const size_t bytesPerGlyph = sizeof(float) * 6 * 4;
    const size_t requiredBytes = glyphCount * bytesPerGlyph;
    if (requiredBytes > vertexBufferCapacity) {
        vertexBufferCapacity = requiredBytes;
        vertexBuffer = opal::Buffer::create(
            opal::BufferUsage::VertexBuffer, vertexBufferCapacity, nullptr,
            opal::MemoryUsageType::CPUToGPU, id);
        vao->setBuffers(vertexBuffer, nullptr);
    }

    size_t glyphIndex = 0;
    for (c = content.begin(); c != content.end(); c++, ++glyphIndex) {
        const auto it = resolvedFont.atlas.find(*c);
        if (it == resolvedFont.atlas.end()) {
            continue;
        }
        Character ch = it->second;

        float xpos = x + (ch.bearing.x * scale);
        float ypos = y - (ch.bearing.y * scale);

        float w = ch.size.width * scale;
        float h = ch.size.height * scale;

        float u0 = ch.uvMin.x;
        float v0 = ch.uvMin.y;
        float u1 = ch.uvMax.x;
        float v1 = ch.uvMax.y;

        float vertices[6][4] = {
            {xpos, ypos, u0, v0},         {xpos, ypos + h, u0, v1},
            {xpos + w, ypos + h, u1, v1},

            {xpos, ypos, u0, v0},         {xpos + w, ypos + h, u1, v1},
            {xpos + w, ypos, u1, v0}};

        vertexBuffer->bind();
        const size_t offset = glyphIndex * bytesPerGlyph;
        vertexBuffer->updateData(offset, sizeof(vertices), vertices);
        vertexBuffer->unbind();
        commandBuffer->draw(
            6, 1, static_cast<uint>(offset / (4 * sizeof(float))), 0, id);

        x += (ch.advance >> 6) * scale;
    }

    commandBuffer->unbindDrawingState();
    textPipeline->enableBlending(false);
    textPipeline->enableDepthTest(true);
    textPipeline->bind();

    if (TracerServices::getInstance().isOk()) {
        DebugObjectPacket debugPacket{};
        debugPacket.drawCallsForObject = 1;
        debugPacket.frameCount =
            Window::mainWindow != nullptr && Window::mainWindow->device != nullptr
                ? Window::mainWindow->device->frameCount
                : 0;
        debugPacket.triangleCount = static_cast<unsigned int>(glyphCount) * 2;
        debugPacket.vertexBufferSizeMb =
            static_cast<float>(requiredBytes) / (1024.0f * 1024.0f);
        debugPacket.indexBufferSizeMb = 0.0f;
        debugPacket.textureCount = (resolvedFont.texture ? 1 : 0);
        debugPacket.materialCount = 0;
        debugPacket.objectType = DebugObjectType::Other;
        debugPacket.objectId = this->id;
        debugPacket.send();
    }
}
