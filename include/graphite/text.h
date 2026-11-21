//
// text.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Text rendering definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_TEXT_H
#define ATLAS_TEXT_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "graphite/style.h"
#include <algorithm>
#include <map>
#include "opal/opal.h"
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Structure that represents a single character in the font atlas.
 *
 */
struct Character {
    /**
     * @brief The size of the glyph.
     *
     */
    Size2d size;
    /**
     * @brief The offset from the baseline to the top-left of the glyph.
     *
     */
    Position2d bearing;
    /**
     * @brief The advance width of the glyph.
     *
     */
    unsigned int advance;
    /**
     * @brief The top-left UV coordinate in the atlas.
     *
     */
    Position2d uvMin;
    /**
     * @brief The bottom-right UV coordinate in the atlas.
     *
     */
    Position2d uvMax;
};

/**
 * @brief A map that associates characters with their corresponding glyph
 * information.
 *
 */
typedef std::map<char, Character> FontAtlas;

/**
 * @brief Structure that represents a font. It is created from a Resource and
 * then bound to the Text component.
 * \subsection font-example Example
 * ```cpp
 * // Load a font from a resource
 * Font myFont = Font::fromResource("MyFont",
 * Workspace::get().getResource("MyFontResource"), 48);
 * // Retrieve the font later by name
 * Font &retrievedFont = Font::getFont("MyFont");
 * // Change the size of the font
 * retrievedFont.changeSize(72);
 * ```
 */
struct Font {
    /**
     * @brief The name of the font.
     *
     */
    std::string name;
    /**
     * @brief The font atlas that contains the glyphs for this font.
     *
     */
    FontAtlas atlas;
    /**
     * @brief The size of the font.
     *
     */
    int size;
    /**
     * @brief The resource associated with the font.
     *
     */
    Resource resource;
    /**
     * @brief The texture atlas containing all glyphs.
     *
     */
    std::shared_ptr<opal::Texture> texture;

    /**
     * @brief Creates a font from a resource.
     *
     * @param fontName The name to associate with the font.
     * @param resource The resource from which to create the font.
     * @param fontSize The size of the font.
     * @return (Font) The created font instance.
     */
    static Font fromResource(const std::string &fontName,
                             const Resource &resource, int fontSize);
    /**
     * @brief Gets the font associated with the given name.
     *
     * @param fontName The name of the font to retrieve.
     * @return (Font&) The requested font.
     */
    static Font &getFont(const std::string &fontName);

    /**
     * @brief Changes the size of the font. \warning This will regenerate the
     * font. Use it when performance is not a concern.
     *
     * @param newSize The new size for the font.
     */
    void changeSize(int newSize);

  private:
    static std::vector<Font> fonts;
};

/**
 * @brief Represents a text object in the game world. It allows rendering of
 * text with a specific font, position, and color.
 * \subsection text-example Example
 * ```cpp
 * // Create a text object
 * Text myText("Hello, Atlas!", Font::getFont("MyFont"), {100, 200},
 * Color::white());
 * // Set the position of the text
 * myText.position = {150, 250};
 * // Set the color of the text
 * myText.color = Color(0.0f, 1.0f, 0.0f, 1.0f); // Green color
 * // Add the text object to the scene
 * scene.addObject(&myText);
 * ```
 */
class Text : public UIObject {
  public:
    /**
     * @brief The content of the text to render.
     *
     */
    std::string content;
    /**
     * @brief The font used to render the text.
     *
     */
    Font font;
    /**
     * @brief The position of the text in 2D space.
     *
     */
    Position2d position;
    float fontSize = 0.0f;
    /**
     * @brief The color of the text.
     *
     */
    Color color = Color::white();
    /**
     * @brief Function that constructs a new Text object.
     *
     */
    Text() {};
    /**
     * @brief Function that constructs a new Text object with the given
     * parameters.
     *
     * @param text The text content to display.
     * @param font The font with which to render the text.
     * @param position The position of the text.
     * @param color The color of the text.
     */
    Text(std::string text, Font font, const Color &color = Color::white(),
         Position2d position = {.x = 0, .y = 0})
        : content(std::move(text)), font(std::move(font)), position(position),
          color(color) {}

    /**
     * @brief Prepares vertex buffers, shader state, and fonts for runtime use.
     */
    void initialize() override;

    /**
     * @brief Renders the text glyphs to the screen honoring kerning and font
     * metrics.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().foreground(color).font(font);
        if (fontSize > 0.0f) {
            fallbackStyle.normal().fontSize(fontSize);
        }
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().text,
            usesLocalStyle ? &localStyle : nullptr);
        const Font &resolvedFont = style.font != nullptr ? *style.font : font;
        return Size2d{
            .width = graphite::measureTextWidth(resolvedFont, content,
                                                style.fontSize) +
                     (style.padding.width * 2.0f),
            .height = graphite::getLineHeight(resolvedFont, style.fontSize) +
                      (style.padding.height * 2.0f),
        };
    }

    Position2d getScreenPosition() const override { return position; }

    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    Text &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        return *this;
    }

    Text &setFontSize(float newFontSize) {
        fontSize = std::max(newFontSize, 0.0f);
        return *this;
    }

  private:
    std::shared_ptr<opal::DrawingState> vao = nullptr;
    std::shared_ptr<opal::Buffer> vertexBuffer = nullptr;
    size_t vertexBufferCapacity = sizeof(float) * 6 * 4; // capacity in bytes
    glm::mat4 projection;
    ShaderProgram shader;
    graphite::BoxRendererData backgroundRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

#endif // ATLAS_TEXT_H
