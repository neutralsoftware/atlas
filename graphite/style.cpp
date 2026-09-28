#include "graphite/style.h"
#include "atlas/window.h"
#include "graphite/text.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace graphite {

namespace {

constexpr float baseTextScale = 2.0f;

struct BoxVertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float a;
    float u;
    float v;
};

void applyVariant(UIResolvedStyle &style, const UIStyleVariant &variant) {
    if (variant.paddingValue.has_value()) {
        style.padding = *variant.paddingValue;
    }
    if (variant.cornerRadiusValue.has_value()) {
        style.cornerRadius = std::max(0.0f, *variant.cornerRadiusValue);
    }
    if (variant.borderWidthValue.has_value()) {
        style.borderWidth = std::max(0.0f, *variant.borderWidthValue);
    }
    if (variant.backgroundColorValue.has_value()) {
        style.backgroundColor = *variant.backgroundColorValue;
    }
    if (variant.borderColorValue.has_value()) {
        style.borderColor = *variant.borderColorValue;
    }
    if (variant.foregroundColorValue.has_value()) {
        style.foregroundColor = *variant.foregroundColorValue;
    }
    if (variant.tintColorValue.has_value()) {
        style.tintColor = *variant.tintColorValue;
    }
    if (variant.fontValue.has_value()) {
        style.font = *variant.fontValue;
    }
    if (variant.fontSizeValue.has_value()) {
        style.fontSize = std::max(0.0f, *variant.fontSizeValue);
    }
}

std::array<UIStyleState, 5> getStateOrder() {
    return {UIStyleState::Checked, UIStyleState::Focused, UIStyleState::Hovered,
            UIStyleState::Pressed, UIStyleState::Disabled};
}

bool hasState(UIStyleState state, const UIStyleStateSnapshot &snapshot) {
    switch (state) {
    case UIStyleState::Hovered:
        return snapshot.hovered;
    case UIStyleState::Pressed:
        return snapshot.pressed;
    case UIStyleState::Focused:
        return snapshot.focused;
    case UIStyleState::Disabled:
        return snapshot.disabled;
    case UIStyleState::Checked:
        return snapshot.checked;
    case UIStyleState::Normal:
        return true;
    }
    return false;
}

std::vector<opal::VertexAttributeBinding>
makeBoxBindings(const std::shared_ptr<opal::Buffer> &vertexBuffer) {
    opal::VertexAttribute positionAttribute{
        .name = "aPos",
        .type = opal::VertexAttributeType::Float,
        .offset = 0,
        .location = 0,
        .normalized = false,
        .size = 3,
        .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute colorAttribute{
        .name = "aColor",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<std::uint32_t>(offsetof(BoxVertex, r)),
        .location = 1,
        .normalized = false,
        .size = 4,
        .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};
    opal::VertexAttribute texCoordAttribute{
        .name = "aTexCoord",
        .type = opal::VertexAttributeType::Float,
        .offset = static_cast<std::uint32_t>(offsetof(BoxVertex, u)),
        .location = 2,
        .normalized = false,
        .size = 2,
        .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
        .inputRate = opal::VertexBindingInputRate::Vertex,
        .divisor = 0};

    return {{positionAttribute, vertexBuffer},
            {colorAttribute, vertexBuffer},
            {texCoordAttribute, vertexBuffer}};
}

int getCornerSegments(float radius) {
    return std::max(4,
                    static_cast<int>(std::ceil(std::max(radius, 0.0f) / 4.0f)));
}

std::vector<glm::vec2> buildRoundedOutline(float width, float height,
                                           float radius, int segments) {
    const float clampedRadius =
        std::clamp(radius, 0.0f, std::min(width, height) * 0.5f);
    if (clampedRadius <= 0.0f) {
        return {{0.0f, 0.0f}, {width, 0.0f}, {width, height}, {0.0f, height}};
    }

    const int arcSegments = std::max(4, segments);
    const float halfPi = 1.57079632679f;
    const float pi = 3.14159265359f;

    std::vector<glm::vec2> points;
    points.reserve(static_cast<std::size_t>(arcSegments * 4) + 8);

    auto appendArc = [&](float centerX, float centerY, float startAngle,
                         float endAngle, bool includeStart,
                         bool includeEnd = true) {
        int startIndex = includeStart ? 0 : 1;
        int endIndex = includeEnd ? arcSegments : arcSegments - 1;
        for (int i = startIndex; i <= endIndex; ++i) {
            const float t =
                static_cast<float>(i) / static_cast<float>(arcSegments);
            const float angle = startAngle + ((endAngle - startAngle) * t);
            points.push_back({centerX + (std::cos(angle) * clampedRadius),
                              centerY + (std::sin(angle) * clampedRadius)});
        }
    };

    points.push_back({clampedRadius, 0.0f});
    points.push_back({width - clampedRadius, 0.0f});
    appendArc(width - clampedRadius, clampedRadius, -halfPi, 0.0f, false);
    points.push_back({width, height - clampedRadius});
    appendArc(width - clampedRadius, height - clampedRadius, 0.0f, halfPi,
              false);
    points.push_back({clampedRadius, height});
    appendArc(clampedRadius, height - clampedRadius, halfPi, pi, false);
    points.push_back({0.0f, clampedRadius});
    appendArc(clampedRadius, clampedRadius, pi, halfPi * 3.0f, false, false);

    return points;
}

void appendRoundedShape(std::vector<BoxVertex> &vertices, float left, float top,
                        float right, float bottom, float radius,
                        const Color &color) {
    const float width = std::abs(right - left);
    const float height = std::abs(bottom - top);
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    const float yDirection = bottom >= top ? 1.0f : -1.0f;
    const std::vector<glm::vec2> outline =
        buildRoundedOutline(width, height, radius, getCornerSegments(radius));
    if (outline.size() < 3) {
        return;
    }

    const float centerLocalX = width * 0.5f;
    const float centerLocalY = height * 0.5f;
    const float centerX = left + centerLocalX;
    const float centerY = top + (centerLocalY * yDirection);

    for (std::size_t i = 0; i < outline.size(); ++i) {
        const glm::vec2 current = outline[i];
        const glm::vec2 next = outline[(i + 1) % outline.size()];

        vertices.push_back({centerX, centerY, 0.0f, color.r, color.g, color.b,
                            color.a, 0.5f, 0.5f});
        vertices.push_back({left + current.x, top + (current.y * yDirection),
                            0.0f, color.r, color.g, color.b, color.a,
                            current.x / width, current.y / height});
        vertices.push_back({left + next.x, top + (next.y * yDirection), 0.0f,
                            color.r, color.g, color.b, color.a, next.x / width,
                            next.y / height});
    }
}

void appendRoundedBorder(std::vector<BoxVertex> &vertices, float left,
                         float top, float right, float bottom, float radius,
                         float inset, const Color &color) {
    const float width = std::abs(right - left);
    const float height = std::abs(bottom - top);
    if (width <= 0.0f || height <= 0.0f || inset <= 0.0f) {
        return;
    }

    const float clampedInset =
        std::clamp(inset, 0.0f, std::min(width, height) * 0.5f);
    const float innerWidth = width - (clampedInset * 2.0f);
    const float innerHeight = height - (clampedInset * 2.0f);
    if (innerWidth <= 0.0f || innerHeight <= 0.0f) {
        appendRoundedShape(vertices, left, top, right, bottom, radius, color);
        return;
    }

    const int segments = getCornerSegments(radius);
    const std::vector<glm::vec2> outerOutline =
        buildRoundedOutline(width, height, radius, segments);
    const std::vector<glm::vec2> innerOutline =
        buildRoundedOutline(innerWidth, innerHeight,
                            std::max(0.0f, radius - clampedInset), segments);
    if (outerOutline.size() != innerOutline.size() || outerOutline.size() < 3) {
        appendRoundedShape(vertices, left, top, right, bottom, radius, color);
        return;
    }

    const float yDirection = bottom >= top ? 1.0f : -1.0f;
    const auto makeVertex = [&](float x, float y, float u, float v) {
        return BoxVertex{x, y, 0.0f, color.r, color.g, color.b, color.a, u, v};
    };

    for (std::size_t i = 0; i < outerOutline.size(); ++i) {
        const std::size_t nextIndex = (i + 1) % outerOutline.size();
        const glm::vec2 outerCurrent = outerOutline[i];
        const glm::vec2 outerNext = outerOutline[nextIndex];
        const glm::vec2 innerCurrent = innerOutline[i];
        const glm::vec2 innerNext = innerOutline[nextIndex];

        const float outerCurrentX = left + outerCurrent.x;
        const float outerCurrentY = top + (outerCurrent.y * yDirection);
        const float outerNextX = left + outerNext.x;
        const float outerNextY = top + (outerNext.y * yDirection);
        const float innerCurrentX = left + clampedInset + innerCurrent.x;
        const float innerCurrentY =
            top + ((clampedInset + innerCurrent.y) * yDirection);
        const float innerNextX = left + clampedInset + innerNext.x;
        const float innerNextY =
            top + ((clampedInset + innerNext.y) * yDirection);

        vertices.push_back(
            makeVertex(outerCurrentX, outerCurrentY, 0.0f, 0.0f));
        vertices.push_back(makeVertex(outerNextX, outerNextY, 1.0f, 0.0f));
        vertices.push_back(makeVertex(innerNextX, innerNextY, 1.0f, 1.0f));

        vertices.push_back(
            makeVertex(outerCurrentX, outerCurrentY, 0.0f, 0.0f));
        vertices.push_back(makeVertex(innerNextX, innerNextY, 1.0f, 1.0f));
        vertices.push_back(
            makeVertex(innerCurrentX, innerCurrentY, 0.0f, 1.0f));
    }
}

std::shared_ptr<opal::Pipeline> getBoxPipeline(const ShaderProgram &shader,
                                               int fbWidth, int fbHeight) {
    static std::shared_ptr<opal::Pipeline> boxPipeline = nullptr;
    if (boxPipeline == nullptr) {
        boxPipeline = opal::Pipeline::create();
        std::vector<opal::VertexAttribute> attributes = {
            {.name = "aPos",
             .type = opal::VertexAttributeType::Float,
             .offset = 0,
             .location = 0,
             .normalized = false,
             .size = 3,
             .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aColor",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<std::uint32_t>(offsetof(BoxVertex, r)),
             .location = 1,
             .normalized = false,
             .size = 4,
             .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0},
            {.name = "aTexCoord",
             .type = opal::VertexAttributeType::Float,
             .offset = static_cast<std::uint32_t>(offsetof(BoxVertex, u)),
             .location = 2,
             .normalized = false,
             .size = 2,
             .stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex,
             .divisor = 0}};
        boxPipeline->setVertexAttributes(
            attributes,
            {.stride = static_cast<std::uint32_t>(sizeof(BoxVertex)),
             .inputRate = opal::VertexBindingInputRate::Vertex});
        boxPipeline->setShaderProgram(shader.shader);
#ifdef VULKAN
        boxPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
        boxPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
        boxPipeline->setCullMode(opal::CullMode::None);
        boxPipeline->enableDepthTest(false);
        boxPipeline->enableDepthWrite(false);
        boxPipeline->setPrimitiveStyle(opal::PrimitiveStyle::Triangles);
        boxPipeline->enableBlending(true);
        boxPipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                                  opal::BlendFunc::OneMinusSrcAlpha);
        boxPipeline->build();
        return boxPipeline;
    }

#ifdef VULKAN
    boxPipeline->setViewport(0, fbHeight, fbWidth, -fbHeight);
#else
    boxPipeline->setViewport(0, 0, fbWidth, fbHeight);
#endif
    boxPipeline->setShaderProgram(shader.shader);
    return boxPipeline;
}

void ensureVertexCapacity(BoxRendererData &renderer, std::size_t requiredBytes,
                          std::uint32_t objectId) {
    if (requiredBytes <= renderer.vertexBufferCapacity &&
        renderer.vertexBuffer != nullptr && renderer.vao != nullptr) {
        return;
    }

    renderer.vertexBufferCapacity = std::max(
        requiredBytes, sizeof(BoxVertex) * static_cast<std::size_t>(192));
    renderer.vertexBuffer = opal::Buffer::create(
        opal::BufferUsage::VertexBuffer, renderer.vertexBufferCapacity, nullptr,
        opal::MemoryUsageType::CPUToGPU, objectId);
    if (renderer.vao == nullptr) {
        renderer.vao = opal::DrawingState::create(renderer.vertexBuffer);
    }
    renderer.vao->setBuffers(renderer.vertexBuffer, nullptr);
    renderer.vao->configureAttributes(makeBoxBindings(renderer.vertexBuffer));
}

void drawVertices(BoxRendererData &renderer, std::uint32_t objectId,
                  const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
                  const std::vector<BoxVertex> &vertices,
                  const std::shared_ptr<opal::Pipeline> &pipeline,
                  std::uint32_t textureId) {
    if (vertices.empty()) {
        return;
    }

    const std::size_t requiredBytes = vertices.size() * sizeof(BoxVertex);
    ensureVertexCapacity(renderer, requiredBytes, objectId);

    renderer.vertexBuffer->bind();
    renderer.vertexBuffer->updateData(0, requiredBytes, vertices.data());
    renderer.vertexBuffer->unbind();

    commandBuffer->bindPipeline(pipeline);
    pipeline->setUniformMat4f("model", glm::mat4(1.0f));
    pipeline->setUniformMat4f("view", glm::mat4(1.0f));
    pipeline->setUniformMat4f("projection", renderer.projection);
    pipeline->setUniform1i("useTexture", textureId != 0 ? 1 : 0);
    pipeline->setUniform1i("onlyTexture", 0);
    pipeline->setUniform1i("textureCount", textureId != 0 ? 1 : 0);
    if (textureId != 0) {
        pipeline->bindTexture2D("texture1", textureId, 0, objectId);
    }

    commandBuffer->bindDrawingState(renderer.vao);
    commandBuffer->draw(static_cast<std::uint32_t>(vertices.size()), 1, 0, 0,
                        objectId);
    commandBuffer->unbindDrawingState();
}

} // namespace

UIStyleVariant &UIStyleVariant::padding(Size2d value) {
    paddingValue = Size2d{.width = std::max(value.width, 0.0f),
                          .height = std::max(value.height, 0.0f)};
    return *this;
}

UIStyleVariant &UIStyleVariant::padding(float horizontal, float vertical) {
    return padding({.width = horizontal, .height = vertical});
}

UIStyleVariant &UIStyleVariant::cornerRadius(float value) {
    cornerRadiusValue = std::max(value, 0.0f);
    return *this;
}

UIStyleVariant &UIStyleVariant::background(const Color &value) {
    backgroundColorValue = value;
    return *this;
}

UIStyleVariant &UIStyleVariant::border(float width, const Color &color) {
    borderWidthValue = std::max(width, 0.0f);
    borderColorValue = color;
    return *this;
}

UIStyleVariant &UIStyleVariant::foreground(const Color &value) {
    foregroundColorValue = value;
    return *this;
}

UIStyleVariant &UIStyleVariant::tint(const Color &value) {
    tintColorValue = value;
    return *this;
}

UIStyleVariant &UIStyleVariant::font(const Font &value) {
    fontValue = &value;
    return *this;
}

UIStyleVariant &UIStyleVariant::fontSize(float value) {
    fontSizeValue = std::max(value, 0.0f);
    return *this;
}

UIStyleVariant &UIStyle::normal() { return normalVariant; }
UIStyleVariant &UIStyle::hovered() { return hoveredVariant; }
UIStyleVariant &UIStyle::pressed() { return pressedVariant; }
UIStyleVariant &UIStyle::focused() { return focusedVariant; }
UIStyleVariant &UIStyle::disabled() { return disabledVariant; }
UIStyleVariant &UIStyle::checked() { return checkedVariant; }

UIStyleVariant &UIStyle::variant(UIStyleState state) {
    switch (state) {
    case UIStyleState::Hovered:
        return hoveredVariant;
    case UIStyleState::Pressed:
        return pressedVariant;
    case UIStyleState::Focused:
        return focusedVariant;
    case UIStyleState::Disabled:
        return disabledVariant;
    case UIStyleState::Checked:
        return checkedVariant;
    case UIStyleState::Normal:
        return normalVariant;
    }
    return normalVariant;
}

const UIStyleVariant &UIStyle::normal() const { return normalVariant; }
const UIStyleVariant &UIStyle::hovered() const { return hoveredVariant; }
const UIStyleVariant &UIStyle::pressed() const { return pressedVariant; }
const UIStyleVariant &UIStyle::focused() const { return focusedVariant; }
const UIStyleVariant &UIStyle::disabled() const { return disabledVariant; }
const UIStyleVariant &UIStyle::checked() const { return checkedVariant; }

const UIStyleVariant &UIStyle::variant(UIStyleState state) const {
    switch (state) {
    case UIStyleState::Hovered:
        return hoveredVariant;
    case UIStyleState::Pressed:
        return pressedVariant;
    case UIStyleState::Focused:
        return focusedVariant;
    case UIStyleState::Disabled:
        return disabledVariant;
    case UIStyleState::Checked:
        return checkedVariant;
    case UIStyleState::Normal:
        return normalVariant;
    }
    return normalVariant;
}

Theme &Theme::current() {
    static Theme theme;
    return theme;
}

void Theme::set(const Theme &theme) { Theme::current() = theme; }

void Theme::reset() { Theme::current() = Theme(); }

UIResolvedStyle resolveStyle(const UIStyle &fallbackStyle,
                             const UIStyle *themeStyle,
                             const UIStyle *overrideStyle,
                             const UIStyleStateSnapshot &state) {
    UIResolvedStyle resolved;
    applyVariant(resolved, fallbackStyle.normal());
    if (themeStyle != nullptr) {
        applyVariant(resolved, themeStyle->normal());
    }
    if (overrideStyle != nullptr) {
        applyVariant(resolved, overrideStyle->normal());
    }

    for (UIStyleState styleState : getStateOrder()) {
        if (!hasState(styleState, state)) {
            continue;
        }
        applyVariant(resolved, fallbackStyle.variant(styleState));
        if (themeStyle != nullptr) {
            applyVariant(resolved, themeStyle->variant(styleState));
        }
        if (overrideStyle != nullptr) {
            applyVariant(resolved, overrideStyle->variant(styleState));
        }
    }

    return resolved;
}

float resolveTextScale(const Font &font, float fontSize) {
    if (font.size <= 0 || fontSize <= 0.0f) {
        return baseTextScale;
    }
    return baseTextScale *
           (fontSize / static_cast<float>(std::max(font.size, 1)));
}

float measureCharacterWidth(const Font &font, char ch, float fontSize) {
    const auto it = font.atlas.find(ch);
    if (it == font.atlas.end()) {
        return 0.0f;
    }
    return static_cast<float>(it->second.advance >> 6) *
           resolveTextScale(font, fontSize);
}

float measureTextWidth(const Font &font, const std::string &text,
                       float fontSize) {
    return measureTextWidth(font, text, 0, text.size(), fontSize);
}

float measureTextWidth(const Font &font, const std::string &text,
                       std::size_t start, std::size_t end, float fontSize) {
    float width = 0.0f;
    const std::size_t clampedEnd = std::min(end, text.size());
    for (std::size_t index = start; index < clampedEnd; ++index) {
        width += measureCharacterWidth(font, text[index], fontSize);
    }
    return width;
}

float getFontAscent(const Font &font, float fontSize) {
    float ascent = 0.0f;
    const float scale = resolveTextScale(font, fontSize);
    for (const auto &entry : font.atlas) {
        ascent = std::max(ascent, entry.second.bearing.y * scale);
    }
    return ascent;
}

float getFontDescent(const Font &font, float fontSize) {
    float descent = 0.0f;
    const float scale = resolveTextScale(font, fontSize);
    for (const auto &entry : font.atlas) {
        descent = std::max(descent,
                           (entry.second.size.height - entry.second.bearing.y) *
                               scale);
    }
    return descent;
}

float getLineHeight(const Font &font, float fontSize) {
    float height =
        getFontAscent(font, fontSize) + getFontDescent(font, fontSize);
    if (height <= 0.0f) {
        height = static_cast<float>(std::max(font.size, 1)) *
                 resolveTextScale(font, fontSize);
    }
    return height;
}

std::string sanitizeText(const Font &font, const std::string &input) {
    std::string result;
    result.reserve(input.size());
    for (unsigned char raw : input) {
        const char ch = static_cast<char>(raw);
        if (ch == '\n' || ch == '\r' || ch == '\t' || raw < 32) {
            continue;
        }
        if (font.atlas.contains(ch)) {
            result.push_back(ch);
        }
    }
    return result;
}

void initializeBoxRenderer(BoxRendererData &renderer, std::uint32_t objectId) {
    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

#if defined(VULKAN) || defined(METAL)
    renderer.projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                                     static_cast<float>(fbHeight), 0.0f);
#else
    renderer.projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                                     static_cast<float>(fbHeight));
#endif

    ensureVertexCapacity(
        renderer, sizeof(BoxVertex) * static_cast<std::size_t>(192), objectId);
    renderer.shader = ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Texture, AtlasFragmentShader::Texture);
}

void renderStyledBox(BoxRendererData &renderer, std::uint32_t objectId,
                     const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
                     Position2d position, Size2d size,
                     const UIResolvedStyle &style, std::uint32_t textureId) {
    if (commandBuffer == nullptr || size.width <= 0.0f || size.height <= 0.0f) {
        return;
    }
    if (renderer.shader.shader == nullptr || renderer.vao == nullptr ||
        renderer.vertexBuffer == nullptr) {
        initializeBoxRenderer(renderer, objectId);
    }

    Size2d framebufferSize = Window::mainWindow->getSize();
    const int fbWidth = static_cast<int>(framebufferSize.width);
    const int fbHeight = static_cast<int>(framebufferSize.height);

#if defined(VULKAN) || defined(METAL)
    renderer.projection = glm::ortho(0.0f, static_cast<float>(fbWidth),
                                     static_cast<float>(fbHeight), 0.0f);
    const float left = position.x;
    const float top = position.y;
    const float right = left + size.width;
    const float bottom = top + size.height;
#else
    renderer.projection = glm::ortho(0.0f, static_cast<float>(fbWidth), 0.0f,
                                     static_cast<float>(fbHeight));
    const float left = position.x;
    const float top = static_cast<float>(fbHeight) - position.y;
    const float right = left + size.width;
    const float bottom = top - size.height;
#endif

    const auto pipeline = getBoxPipeline(renderer.shader, fbWidth, fbHeight);
    pipeline->enableBlending(true);
    pipeline->setBlendFunc(opal::BlendFunc::SrcAlpha,
                           opal::BlendFunc::OneMinusSrcAlpha);
    pipeline->enableDepthTest(false);
    pipeline->enableDepthWrite(false);

    const float inset = std::clamp(style.borderWidth, 0.0f,
                                   std::min(size.width, size.height) * 0.5f);
    const float yInsetDirection = bottom >= top ? 1.0f : -1.0f;
    const float innerLeft = left + inset;
    const float innerRight = right - inset;
    const float innerTop = top + (inset * yInsetDirection);
    const float innerBottom = bottom - (inset * yInsetDirection);
    const float innerRadius = std::max(0.0f, style.cornerRadius - inset);

    std::vector<BoxVertex> colorVertices;
    if (inset > 0.0f && style.borderColor.a > 0.0f) {
        appendRoundedBorder(colorVertices, left, top, right, bottom,
                            style.cornerRadius, inset, style.borderColor);
    }

    const bool hasInnerArea = std::abs(innerRight - innerLeft) > 0.0f &&
                              std::abs(innerBottom - innerTop) > 0.0f;

    if (style.backgroundColor.a > 0.0f && hasInnerArea) {
        appendRoundedShape(colorVertices, inset > 0.0f ? innerLeft : left,
                           inset > 0.0f ? innerTop : top,
                           inset > 0.0f ? innerRight : right,
                           inset > 0.0f ? innerBottom : bottom,
                           inset > 0.0f ? innerRadius : style.cornerRadius,
                           style.backgroundColor);
    }

    drawVertices(renderer, objectId, commandBuffer, colorVertices, pipeline, 0);

    if (textureId != 0 && hasInnerArea) {
        std::vector<BoxVertex> textureVertices;
        appendRoundedShape(
            textureVertices, inset > 0.0f ? innerLeft : left,
            inset > 0.0f ? innerTop : top, inset > 0.0f ? innerRight : right,
            inset > 0.0f ? innerBottom : bottom,
            inset > 0.0f ? innerRadius : style.cornerRadius, style.tintColor);
        drawVertices(renderer, objectId, commandBuffer, textureVertices,
                     pipeline, textureId);
    }

    pipeline->enableBlending(false);
    pipeline->enableDepthTest(true);
    pipeline->enableDepthWrite(true);
    pipeline->bind();
}

} // namespace graphite
