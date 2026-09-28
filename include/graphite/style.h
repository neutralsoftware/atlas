#ifndef GRAPHITE_STYLE_H
#define GRAPHITE_STYLE_H

#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct Font;

namespace graphite {

/**
 * @brief Enumerates the interaction states a UI style can resolve against.
 */
enum class UIStyleState {
    /** @brief Default resting state. */
    Normal,
    /** @brief State used while the cursor hovers the element. */
    Hovered,
    /** @brief State used while the element is actively pressed. */
    Pressed,
    /** @brief State used while the element owns keyboard focus. */
    Focused,
    /** @brief State used while interaction is disabled. */
    Disabled,
    /** @brief State used while a toggle-like element is checked. */
    Checked,
};

/**
 * @brief Captures the current interaction flags for a UI element.
 */
struct UIStyleStateSnapshot {
    /** @brief Whether the element is currently hovered. */
    bool hovered = false;
    /** @brief Whether the element is currently pressed. */
    bool pressed = false;
    /** @brief Whether the element is currently focused. */
    bool focused = false;
    /** @brief Whether the element is currently disabled. */
    bool disabled = false;
    /** @brief Whether the element is currently checked. */
    bool checked = false;
};

/**
 * @brief Partial style definition for a single UI state.
 */
struct UIStyleVariant {
    /** @brief Optional padding override. */
    std::optional<Size2d> paddingValue;
    /** @brief Optional corner radius override. */
    std::optional<float> cornerRadiusValue;
    /** @brief Optional border width override. */
    std::optional<float> borderWidthValue;
    /** @brief Optional background color override. */
    std::optional<Color> backgroundColorValue;
    /** @brief Optional border color override. */
    std::optional<Color> borderColorValue;
    /** @brief Optional foreground color override. */
    std::optional<Color> foregroundColorValue;
    /** @brief Optional tint color override. */
    std::optional<Color> tintColorValue;
    /** @brief Optional font override. */
    std::optional<const Font *> fontValue;
    /** @brief Optional font size override. */
    std::optional<float> fontSizeValue;

    /** @brief Sets the horizontal and vertical padding for the variant. */
    UIStyleVariant &padding(Size2d value);
    /** @brief Sets the horizontal and vertical padding components. */
    UIStyleVariant &padding(float horizontal, float vertical);
    /** @brief Sets the corner radius used for rounded boxes. */
    UIStyleVariant &cornerRadius(float value);
    /** @brief Sets the background color for the variant. */
    UIStyleVariant &background(const Color &value);
    /** @brief Sets the border width and color for the variant. */
    UIStyleVariant &border(float width, const Color &color);
    /** @brief Sets the foreground color, typically used for text. */
    UIStyleVariant &foreground(const Color &value);
    /** @brief Sets the tint color, typically used for images. */
    UIStyleVariant &tint(const Color &value);
    /** @brief Sets the font to use when this variant resolves. */
    UIStyleVariant &font(const Font &value);
    /** @brief Sets the font size override for this variant. */
    UIStyleVariant &fontSize(float value);
};

/**
 * @brief Fully resolved style values ready to be used during rendering.
 */
struct UIResolvedStyle {
    /** @brief Final padding applied to the element. */
    Size2d padding{.width = 0.0f, .height = 0.0f};
    /** @brief Final corner radius. */
    float cornerRadius = 0.0f;
    /** @brief Final border width. */
    float borderWidth = 0.0f;
    /** @brief Final background color. */
    Color backgroundColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    /** @brief Final border color. */
    Color borderColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    /** @brief Final foreground color. */
    Color foregroundColor = Color::white();
    /** @brief Final tint color. */
    Color tintColor = Color::white();
    /** @brief Final font selection, if any. */
    const Font *font = nullptr;
    /** @brief Final font size override. */
    float fontSize = 0.0f;
};

/**
 * @brief Collection of style variants for a single UI element.
 *
 * \subsection graphite-style-example Example
 * ```cpp
 * graphite::UIStyle style;
 * style.normal()
 *     .padding(16.0f, 12.0f)
 *     .background(Color(0.12f, 0.14f, 0.18f, 0.96f))
 *     .foreground(Color::white())
 *     .cornerRadius(12.0f);
 *
 * style.hovered()
 *     .background(Color(0.18f, 0.2f, 0.25f, 0.98f))
 *     .border(1.0f, Color(1.0f, 0.55f, 0.14f, 1.0f));
 * ```
 */
class UIStyle {
  public:
    /** @brief Returns the base variant applied to every state. */
    UIStyleVariant &normal();
    /** @brief Returns the variant used while the element is hovered. */
    UIStyleVariant &hovered();
    /** @brief Returns the variant used while the element is pressed. */
    UIStyleVariant &pressed();
    /** @brief Returns the variant used while the element is focused. */
    UIStyleVariant &focused();
    /** @brief Returns the variant used while the element is disabled. */
    UIStyleVariant &disabled();
    /** @brief Returns the variant used while the element is checked. */
    UIStyleVariant &checked();
    /** @brief Returns the variant associated with the requested state. */
    UIStyleVariant &variant(UIStyleState state);

    /** @brief Returns the base variant applied to every state. */
    const UIStyleVariant &normal() const;
    /** @brief Returns the variant used while the element is hovered. */
    const UIStyleVariant &hovered() const;
    /** @brief Returns the variant used while the element is pressed. */
    const UIStyleVariant &pressed() const;
    /** @brief Returns the variant used while the element is focused. */
    const UIStyleVariant &focused() const;
    /** @brief Returns the variant used while the element is disabled. */
    const UIStyleVariant &disabled() const;
    /** @brief Returns the variant used while the element is checked. */
    const UIStyleVariant &checked() const;
    /** @brief Returns the variant associated with the requested state. */
    const UIStyleVariant &variant(UIStyleState state) const;

  private:
    UIStyleVariant normalVariant;
    UIStyleVariant hoveredVariant;
    UIStyleVariant pressedVariant;
    UIStyleVariant focusedVariant;
    UIStyleVariant disabledVariant;
    UIStyleVariant checkedVariant;
};

/**
 * @brief Global theme container used as the default style source for Graphite.
 *
 * \subsection graphite-theme-example Example
 * ```cpp
 * graphite::Theme theme;
 * theme.button.normal()
 *     .padding(18.0f, 12.0f)
 *     .background(Color(0.16f, 0.17f, 0.22f, 0.96f))
 *     .foreground(Color::white())
 *     .cornerRadius(10.0f);
 * theme.button.hovered().background(Color(0.22f, 0.24f, 0.3f, 0.98f));
 *
 * graphite::Theme::set(theme);
 * ```
 */
class Theme {
  public:
    /** @brief Default style applied to `Text` elements. */
    UIStyle text;
    /** @brief Default style applied to `Image` elements. */
    UIStyle image;
    /** @brief Default style applied to `TextField` elements. */
    UIStyle textField;
    /** @brief Default style applied to `Button` elements. */
    UIStyle button;
    /** @brief Default style applied to `Checkbox` elements. */
    UIStyle checkbox;
    /** @brief Default style applied to `Row` layouts. */
    UIStyle row;
    /** @brief Default style applied to `Column` layouts. */
    UIStyle column;
    /** @brief Default style applied to `Stack` layouts. */
    UIStyle stack;

    /** @brief Returns the active global theme. */
    static Theme &current();
    /** @brief Replaces the active global theme. */
    static void set(const Theme &theme);
    /** @brief Restores the default built-in theme. */
    static void reset();
};

/**
 * @brief Cached GPU resources used to draw styled rectangular UI surfaces.
 */
struct BoxRendererData {
    std::shared_ptr<opal::DrawingState> vao = nullptr;
    std::shared_ptr<opal::Buffer> vertexBuffer = nullptr;
    std::size_t vertexBufferCapacity = 0;
    glm::mat4 projection{1.0f};
    ShaderProgram shader;
};

/**
 * @brief Merges fallback, theme, and override styles into a single resolved
 * result for the supplied interaction state.
 */
UIResolvedStyle resolveStyle(const UIStyle &fallbackStyle,
                             const UIStyle *themeStyle,
                             const UIStyle *overrideStyle = nullptr,
                             const UIStyleStateSnapshot &state = {});

/** @brief Computes the scale factor required to render a font at a target size.
 */
float resolveTextScale(const Font &font, float fontSize = 0.0f);
/** @brief Measures the advance width of a single character. */
float measureCharacterWidth(const Font &font, char ch, float fontSize = 0.0f);
/** @brief Measures the width of a full string using the specified font. */
float measureTextWidth(const Font &font, const std::string &text,
                       float fontSize = 0.0f);
/** @brief Measures the width of a substring within the given index range. */
float measureTextWidth(const Font &font, const std::string &text,
                       std::size_t start, std::size_t end,
                       float fontSize = 0.0f);
/** @brief Returns the font ascent for the resolved size. */
float getFontAscent(const Font &font, float fontSize = 0.0f);
/** @brief Returns the font descent for the resolved size. */
float getFontDescent(const Font &font, float fontSize = 0.0f);
/** @brief Returns the line height for the resolved size. */
float getLineHeight(const Font &font, float fontSize = 0.0f);
/** @brief Filters unsupported glyphs out of a string for a specific font. */
std::string sanitizeText(const Font &font, const std::string &input);

/** @brief Allocates or refreshes renderer state for a styled box. */
void initializeBoxRenderer(BoxRendererData &renderer, std::uint32_t objectId);
/** @brief Draws a rectangle using the resolved style and optional texture. */
void renderStyledBox(BoxRendererData &renderer, std::uint32_t objectId,
                     const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
                     Position2d position, Size2d size,
                     const UIResolvedStyle &style, std::uint32_t textureId = 0);

} // namespace graphite

#endif // GRAPHITE_STYLE_H
