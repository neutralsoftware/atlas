//
// layout.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Layout structures for Graphite UI
// Copyright (c) 2026 Max Van den Eynde
//

#ifndef GRAPHITE_LAYOUT_H
#define GRAPHITE_LAYOUT_H

#include "atlas/units.h"
#include "graphite/style.h"
#include <atlas/component.h>
#include <algorithm>
#include <vector>

/**
 * @brief Alignment options used when positioning children inside layouts.
 */
enum class ElementAlignment {
    /** @brief Align children to the start edge of the available axis. */
    Top,
    /** @brief Center children along the available axis. */
    Center,
    /** @brief Align children to the end edge of the available axis. */
    Bottom,
};

/**
 * @brief Anchor positions used to interpret a layout container's screen
 * position.
 */
enum class LayoutAnchor {
    /** @brief Interpret position as the container's top-left corner. */
    TopLeft,
    /** @brief Interpret position as the container's top edge center. */
    TopCenter,
    /** @brief Interpret position as the container's top-right corner. */
    TopRight,
    /** @brief Interpret position as the container's center-left edge. */
    CenterLeft,
    /** @brief Interpret position as the container's geometric center. */
    Center,
    /** @brief Interpret position as the container's center-right edge. */
    CenterRight,
    /** @brief Interpret position as the container's bottom-left corner. */
    BottomLeft,
    /** @brief Interpret position as the container's bottom edge center. */
    BottomCenter,
    /** @brief Interpret position as the container's bottom-right corner. */
    BottomRight,
};

namespace graphite {
/**
 * @brief Converts an anchored position into a top-left origin.
 *
 * @param position Anchor position provided by the caller.
 * @param size Size of the element being anchored.
 * @param anchor Anchor mode describing how position should be interpreted.
 * @return (Position2d) Top-left position to use for rendering.
 */
Position2d getAnchoredTopLeft(Position2d position, Size2d size,
                              LayoutAnchor anchor);
} // namespace graphite

/**
 * @brief Vertical layout that stacks children from top to bottom.
 *
 * \subsection graphite-column-example Example
 * ```cpp
 * Text title("Settings", Font::getFont("Inter"), Color::white());
 * Button save(Font::getFont("Inter"), "Save");
 *
 * Column panel({&title, &save}, 12.0f, {.width = 16.0f, .height = 16.0f},
 *              {.x = 24.0f, .y = 24.0f});
 * panel.alignment = ElementAlignment::Center;
 * panel.style().normal().background(Color(0.1f, 0.11f, 0.15f, 0.94f));
 * ```
 */
class Column : public UIObject {
  public:
    /** @brief Constructs an empty column at the given position. */
    Column(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }
    /** @brief Constructs a column with children, spacing, padding, and position. */
    Column(const std::vector<UIObject *> &children, float spacing = 0.0f,
           Size2d padding = {.width = 0.0f, .height = 0.0f},
           Position2d pos = {.x = 0, .y = 0})
        : spacing(spacing), padding(padding), children(children),
          position(pos) {
        recalculatePositions();
    }
    void initialize() override;
    /** @brief Renders the column background and all children. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override;

    /** @brief Spacing inserted between consecutive children. */
    float spacing = 0.0f;
    /** @brief Minimum size enforced for the layout container. */
    Size2d maxSize{.width = 0.0f, .height = 0.0f};
    /** @brief Padding applied around the children. */
    Size2d padding{.width = 0.0f, .height = 0.0f};
    /** @brief Child elements owned externally by the caller. */
    std::vector<UIObject *> children;
    /** @brief Screen-space anchor position of the layout container. */
    Position2d position;
    /** @brief Horizontal alignment applied to each child within the column. */
    ElementAlignment alignment = ElementAlignment::Top;
    /** @brief Anchor mode used to interpret position. */
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    /** @brief Propagates the view matrix to all children. */
    void setViewMatrix(const glm::mat4 &view) override;
    /** @brief Propagates the projection matrix to all children. */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /** @brief Appends a child to the layout. */
    void addChild(UIObject *child);
    /** @brief Replaces the child list for the layout. */
    void setChildren(const std::vector<UIObject *> &newChildren);

    /** @brief Computes the layout size including padding and child spacing. */
    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().column,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float width = 0.0f;
        float totalHeight = effectivePadding.height * 2.0f;
        bool hasChild = false;
        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            width = std::max(width, childSize.width);
            totalHeight += childSize.height;
            if (hasChild) {
                totalHeight += spacing;
            }
            hasChild = true;
        }
        return Size2d{
            .width =
                std::max(width + (effectivePadding.width * 2.0f), maxSize.width),
            .height = std::max(totalHeight, maxSize.height),
        };
    }

    /** @brief Returns the layout screen position. */
    Position2d getScreenPosition() const override { return position; }
    /** @brief Sets the layout screen position and recomputes child placement. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    /** @brief Returns a mutable local style override for this layout. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the local style override for this layout. */
    Column &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

/**
 * @brief Horizontal layout that arranges children from left to right.
 *
 * \subsection graphite-row-example Example
 * ```cpp
 * Checkbox music(Font::getFont("Inter"), "Music", true);
 * Checkbox sfx(Font::getFont("Inter"), "SFX", true);
 *
 * Row toggles({&music, &sfx}, 16.0f, {.width = 12.0f, .height = 12.0f},
 *             {.x = 24.0f, .y = 96.0f});
 * toggles.alignment = ElementAlignment::Center;
 * ```
 */
class Row : public UIObject {
  public:
    /** @brief Constructs an empty row at the given position. */
    Row(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }

    /** @brief Constructs a row with children, spacing, padding, and position. */
    Row(const std::vector<UIObject *> &children, float spacing = 0.0f,
        Size2d padding = {.width = 0.0f, .height = 0.0f},
        Position2d pos = {.x = 0, .y = 0})
        : spacing(spacing), padding(padding), children(children),
          position(pos) {
        recalculatePositions();
    }

    void initialize() override;

    /** @brief Renders the row background and all children. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override;

    /** @brief Spacing inserted between consecutive children. */
    float spacing = 0.0f;
    /** @brief Minimum size enforced for the layout container. */
    Size2d maxSize{.width = 0.0f, .height = 0.0f};
    /** @brief Padding applied around the children. */
    Size2d padding{.width = 0.0f, .height = 0.0f};
    /** @brief Child elements owned externally by the caller. */
    std::vector<UIObject *> children;
    /** @brief Screen-space anchor position of the layout container. */
    Position2d position;
    /** @brief Vertical alignment applied to each child within the row. */
    ElementAlignment alignment = ElementAlignment::Center;
    /** @brief Anchor mode used to interpret position. */
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    /** @brief Propagates the view matrix to all children. */
    void setViewMatrix(const glm::mat4 &view) override;
    /** @brief Propagates the projection matrix to all children. */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /** @brief Appends a child to the layout. */
    void addChild(UIObject *child);
    /** @brief Replaces the child list for the layout. */
    void setChildren(const std::vector<UIObject *> &newChildren);

    /** @brief Computes the layout size including padding and child spacing. */
    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().row,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float totalWidth = effectivePadding.width * 2.0f;
        float maxHeight = 0.0f;
        bool hasChild = false;

        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            if (hasChild) {
                totalWidth += spacing;
            }
            totalWidth += childSize.width;
            maxHeight = std::max(maxHeight, childSize.height);
            hasChild = true;
        }

        return Size2d{
            .width = std::max(totalWidth, maxSize.width),
            .height = std::max(maxHeight + (effectivePadding.height * 2.0f),
                               maxSize.height),
        };
    }

    /** @brief Returns the layout screen position. */
    Position2d getScreenPosition() const override { return position; }

    /** @brief Sets the layout screen position and recomputes child placement. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    /** @brief Returns a mutable local style override for this layout. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the local style override for this layout. */
    Row &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

/**
 * @brief Overlay layout that positions children within the same container.
 *
 * \subsection graphite-stack-example Example
 * ```cpp
 * Image background(texture, {.width = 320.0f, .height = 180.0f});
 * Text caption("Atlas", Font::getFont("Inter"), Color::white(),
 *              {.x = 20.0f, .y = 20.0f});
 *
 * Stack hero({&background, &caption}, {.width = 12.0f, .height = 12.0f},
 *            {.x = 24.0f, .y = 160.0f});
 * hero.horizontalAlignment = ElementAlignment::Center;
 * hero.verticalAlignment = ElementAlignment::Center;
 * ```
 */
class Stack : public UIObject {
  public:
    /** @brief Constructs an empty stack at the given position. */
    Stack(Position2d pos = {.x = 0, .y = 0}) : position(pos) {
        recalculatePositions();
    }

    /** @brief Constructs a stack with children, padding, and position. */
    Stack(const std::vector<UIObject *> &children,
          Size2d padding = {.width = 0.0f, .height = 0.0f},
          Position2d pos = {.x = 0, .y = 0})
        : padding(padding), children(children), position(pos) {
        recalculatePositions();
    }

    void initialize() override;

    /** @brief Renders the stack background and all children. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override;

    /** @brief Minimum size enforced for the layout container. */
    Size2d maxSize{.width = 0.0f, .height = 0.0f};
    /** @brief Padding applied around the overlaid children. */
    Size2d padding{.width = 0.0f, .height = 0.0f};
    /** @brief Child elements owned externally by the caller. */
    std::vector<UIObject *> children;
    /** @brief Screen-space anchor position of the layout container. */
    Position2d position;
    /** @brief Horizontal alignment for child placement within the stack. */
    ElementAlignment horizontalAlignment = ElementAlignment::Top;
    /** @brief Vertical alignment for child placement within the stack. */
    ElementAlignment verticalAlignment = ElementAlignment::Top;
    /** @brief Anchor mode used to interpret position. */
    LayoutAnchor anchor = LayoutAnchor::TopLeft;

    /** @brief Propagates the view matrix to all children. */
    void setViewMatrix(const glm::mat4 &view) override;
    /** @brief Propagates the projection matrix to all children. */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /** @brief Appends a child to the layout. */
    void addChild(UIObject *child);
    /** @brief Replaces the child list for the layout. */
    void setChildren(const std::vector<UIObject *> &newChildren);

    /** @brief Computes the layout size from the largest child and padding. */
    Size2d getSize() const override {
        graphite::UIStyle fallbackStyle;
        fallbackStyle.normal().padding(padding);
        const graphite::UIResolvedStyle style = graphite::resolveStyle(
            fallbackStyle, &graphite::Theme::current().stack,
            usesLocalStyle ? &localStyle : nullptr);
        const Size2d effectivePadding = style.padding;
        float width = 0.0f;
        float height = 0.0f;

        for (const auto *child : children) {
            if (child == nullptr) {
                continue;
            }
            Size2d childSize = child->getSize();
            width = std::max(width, childSize.width);
            height = std::max(height, childSize.height);
        }

        return Size2d{
            .width =
                std::max(width + (effectivePadding.width * 2.0f), maxSize.width),
            .height = std::max(height + (effectivePadding.height * 2.0f),
                               maxSize.height),
        };
    }

    /** @brief Returns the layout screen position. */
    Position2d getScreenPosition() const override { return position; }

    /** @brief Sets the layout screen position and recomputes child placement. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
        recalculatePositions();
    }

    /** @brief Returns a mutable local style override for this layout. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the local style override for this layout. */
    Stack &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        recalculatePositions();
        return *this;
    }

  private:
    void recalculatePositions();
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

#endif // GRAPHITE_LAYOUT_H
