#include "graphite/image.h"
#include "graphite/style.h"
#include <algorithm>

namespace {

std::uint32_t getTextureId(const Texture &texture) {
    if (texture.id != 0) {
        return texture.id;
    }
    if (texture.texture != nullptr) {
        return texture.texture->textureID;
    }
    return 0;
}

graphite::UIStyle makeFallbackStyle(const Image &image) {
    graphite::UIStyle style;
    style.normal().tint(image.tint);
    return style;
}

} // namespace

Size2d Image::getSize() const {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().image,
        usesLocalStyle ? &localStyle : nullptr);
    const float contentWidth =
        size.width > 0.0f ? size.width
                          : static_cast<float>(texture.creationData.width);
    const float contentHeight =
        size.height > 0.0f ? size.height
                           : static_cast<float>(texture.creationData.height);
    return {.width = contentWidth + (style.padding.width * 2.0f),
            .height = contentHeight + (style.padding.height * 2.0f)};
}

void Image::setTexture(Texture newTexture) { texture = std::move(newTexture); }

void Image::setSize(Size2d newSize) { size = newSize; }

void Image::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
}

void Image::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                   bool updatePipeline) {
    (void)updatePipeline;

    if (boxRenderer.shader.shader == nullptr || boxRenderer.vao == nullptr ||
        boxRenderer.vertexBuffer == nullptr) {
        initialize();
    }

    for (auto &component : components) {
        component->update(dt);
    }

    if (commandBuffer == nullptr) {
        return;
    }

    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeFallbackStyle(*this), &graphite::Theme::current().image,
        usesLocalStyle ? &localStyle : nullptr);
    const float contentWidth =
        size.width > 0.0f ? size.width
                          : static_cast<float>(texture.creationData.width);
    const float contentHeight =
        size.height > 0.0f ? size.height
                           : static_cast<float>(texture.creationData.height);
    if (contentWidth <= 0.0f || contentHeight <= 0.0f) {
        return;
    }

    const std::uint32_t textureId = getTextureId(texture);
    const Position2d framePosition = position;
    const Size2d frameSize{.width = contentWidth + (style.padding.width * 2.0f),
                           .height =
                               contentHeight + (style.padding.height * 2.0f)};
    if ((style.backgroundColor.a > 0.0f) ||
        (style.borderWidth > 0.0f && style.borderColor.a > 0.0f)) {
        graphite::renderStyledBox(boxRenderer, id, commandBuffer, framePosition,
                                  frameSize, style);
    }

    graphite::UIResolvedStyle imageStyle;
    imageStyle.tintColor = style.tintColor;
    imageStyle.cornerRadius =
        std::max(0.0f, style.cornerRadius - style.borderWidth);
    if (textureId != 0) {
        graphite::renderStyledBox(
            boxRenderer, id, commandBuffer,
            {.x = framePosition.x + style.padding.width,
             .y = framePosition.y + style.padding.height},
            {.width = contentWidth, .height = contentHeight}, imageStyle,
            textureId);
    }
}
