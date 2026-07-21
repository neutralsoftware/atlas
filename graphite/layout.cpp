//
// layout.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Layout definitions and calculations
// Copyright (c) 2026 Max Van den Eynde
//

#include "graphite/layout.h"
#include "atlas/component.h"

namespace {

graphite::UIStyle makeLayoutStyle(Size2d padding) {
    graphite::UIStyle style;
    style.normal().padding(padding);
    return style;
}

LayoutAnchor getLayoutAnchorForChild(const UIObject *child) {
    if (const auto *column = dynamic_cast<const Column *>(child)) {
        return column->anchor;
    }
    if (const auto *row = dynamic_cast<const Row *>(child)) {
        return row->anchor;
    }
    if (const auto *stack = dynamic_cast<const Stack *>(child)) {
        return stack->anchor;
    }
    return LayoutAnchor::TopLeft;
}

Position2d getPositionForTopLeft(Position2d topLeft, Size2d size,
                                 LayoutAnchor anchor) {
    Position2d position = topLeft;

    switch (anchor) {
    case LayoutAnchor::TopLeft:
        break;
    case LayoutAnchor::TopCenter:
        position.x += size.width / 2.0f;
        break;
    case LayoutAnchor::TopRight:
        position.x += size.width;
        break;
    case LayoutAnchor::CenterLeft:
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::Center:
        position.x += size.width / 2.0f;
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::CenterRight:
        position.x += size.width;
        position.y += size.height / 2.0f;
        break;
    case LayoutAnchor::BottomLeft:
        position.y += size.height;
        break;
    case LayoutAnchor::BottomCenter:
        position.x += size.width / 2.0f;
        position.y += size.height;
        break;
    case LayoutAnchor::BottomRight:
        position.x += size.width;
        position.y += size.height;
        break;
    }

    return position;
}

void setChildTopLeft(UIObject *child, const Position2d &topLeft,
                     const Size2d &childSize) {
    if (child == nullptr) {
        return;
    }

    child->setScreenPosition(getPositionForTopLeft(
        topLeft, childSize, getLayoutAnchorForChild(child)));
}

} // namespace

void Column::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
    for (auto *child : children) {
        if (child != nullptr) {
            child->initialize();
        }
    }
}

void Column::addChild(UIObject *child) {
    children.push_back(child);
    recalculatePositions();
}

void Column::setChildren(const std::vector<UIObject *> &newChildren) {
    children = newChildren;
    recalculatePositions();
}

void Column::setViewMatrix(const glm::mat4 &view) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setViewMatrix(view);
    }
}

void Column::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setProjectionMatrix(projection);
    }
}

void Column::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    if (boxRenderer.shader.shader == nullptr || boxRenderer.vao == nullptr ||
        boxRenderer.vertexBuffer == nullptr) {
        initialize();
    }
    for (auto &component : components) {
        component->update(dt);
    }
    recalculatePositions();
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().column,
        usesLocalStyle ? &localStyle : nullptr);
    const Size2d layoutSize = getSize();
    const Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);
    if ((style.backgroundColor.a > 0.0f) ||
        (style.borderWidth > 0.0f && style.borderColor.a > 0.0f)) {
        graphite::renderStyledBox(boxRenderer, id, commandBuffer, topLeft,
                                  layoutSize, style);
    }
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Column::recalculatePositions() {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().column,
        usesLocalStyle ? &localStyle : nullptr);
    Size2d layoutSize = getSize();
    Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);

    float currentY = topLeft.y + style.padding.height;

    float contentWidth = layoutSize.width - (style.padding.width * 2.0f);

    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        Size2d childSize = child->getSize();
        float childX = topLeft.x + style.padding.width;

        switch (alignment) {
        case ElementAlignment::Top:
            childX = topLeft.x + style.padding.width;
            break;

        case ElementAlignment::Center:
            childX = topLeft.x + style.padding.width +
                     ((contentWidth - childSize.width) / 2.0f);
            break;

        case ElementAlignment::Bottom:
            childX = topLeft.x + layoutSize.width - style.padding.width -
                     childSize.width;
            break;
        }

        setChildTopLeft(child, {.x = childX, .y = currentY}, childSize);

        currentY += childSize.height + spacing;
    }
}

void Row::addChild(UIObject *child) {
    children.push_back(child);
    recalculatePositions();
}

void Row::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
    for (auto *child : children) {
        if (child != nullptr) {
            child->initialize();
        }
    }
}

void Row::setChildren(const std::vector<UIObject *> &newChildren) {
    children = newChildren;
    recalculatePositions();
}

void Row::setViewMatrix(const glm::mat4 &view) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setViewMatrix(view);
    }
}

void Row::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setProjectionMatrix(projection);
    }
}

void Row::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                 bool updatePipeline) {
    if (boxRenderer.shader.shader == nullptr || boxRenderer.vao == nullptr ||
        boxRenderer.vertexBuffer == nullptr) {
        initialize();
    }
    for (auto &component : components) {
        component->update(dt);
    }
    recalculatePositions();
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().row,
        usesLocalStyle ? &localStyle : nullptr);
    const Size2d layoutSize = getSize();
    const Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);
    if ((style.backgroundColor.a > 0.0f) ||
        (style.borderWidth > 0.0f && style.borderColor.a > 0.0f)) {
        graphite::renderStyledBox(boxRenderer, id, commandBuffer, topLeft,
                                  layoutSize, style);
    }
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Row::recalculatePositions() {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().row,
        usesLocalStyle ? &localStyle : nullptr);
    Size2d layoutSize = getSize();
    Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);

    float currentX = topLeft.x + style.padding.width;

    float contentHeight = layoutSize.height - (style.padding.height * 2.0f);

    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        Size2d childSize = child->getSize();
        float childY = topLeft.y + style.padding.height;

        switch (alignment) {
        case ElementAlignment::Top:
            childY = topLeft.y + style.padding.height;
            break;

        case ElementAlignment::Center:
            childY = topLeft.y + style.padding.height +
                     ((contentHeight - childSize.height) / 2.0f);
            break;

        case ElementAlignment::Bottom:
            childY = topLeft.y + layoutSize.height - style.padding.height -
                     childSize.height;
            break;
        }

        setChildTopLeft(child, {.x = currentX, .y = childY}, childSize);

        currentX += childSize.width + spacing;
    }
}

void Stack::addChild(UIObject *child) {
    children.push_back(child);
    recalculatePositions();
}

void Stack::initialize() {
    for (auto &component : components) {
        component->init();
    }
    graphite::initializeBoxRenderer(boxRenderer, id);
    for (auto *child : children) {
        if (child != nullptr) {
            child->initialize();
        }
    }
}

void Stack::setChildren(const std::vector<UIObject *> &newChildren) {
    children = newChildren;
    recalculatePositions();
}

void Stack::setViewMatrix(const glm::mat4 &view) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setViewMatrix(view);
    }
}

void Stack::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setProjectionMatrix(projection);
    }
}

void Stack::render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                   bool updatePipeline) {
    if (boxRenderer.shader.shader == nullptr || boxRenderer.vao == nullptr ||
        boxRenderer.vertexBuffer == nullptr) {
        initialize();
    }
    for (auto &component : components) {
        component->update(dt);
    }
    recalculatePositions();
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().stack,
        usesLocalStyle ? &localStyle : nullptr);
    const Size2d layoutSize = getSize();
    const Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);
    if ((style.backgroundColor.a > 0.0f) ||
        (style.borderWidth > 0.0f && style.borderColor.a > 0.0f)) {
        graphite::renderStyledBox(boxRenderer, id, commandBuffer, topLeft,
                                  layoutSize, style);
    }
    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }
        child->render(dt, commandBuffer, updatePipeline);
    }
}

void Stack::recalculatePositions() {
    const graphite::UIResolvedStyle style = graphite::resolveStyle(
        makeLayoutStyle(padding), &graphite::Theme::current().stack,
        usesLocalStyle ? &localStyle : nullptr);
    Size2d layoutSize = getSize();
    Position2d topLeft =
        graphite::getAnchoredTopLeft(position, layoutSize, anchor);

    float contentWidth = layoutSize.width - (style.padding.width * 2.0f);
    float contentHeight = layoutSize.height - (style.padding.height * 2.0f);

    for (auto &child : children) {
        if (child == nullptr) {
            continue;
        }

        Size2d childSize = child->getSize();
        float childX = topLeft.x + style.padding.width;
        float childY = topLeft.y + style.padding.height;

        switch (horizontalAlignment) {
        case ElementAlignment::Top:
            childX = topLeft.x + style.padding.width;
            break;
        case ElementAlignment::Center:
            childX = topLeft.x + style.padding.width +
                     ((contentWidth - childSize.width) / 2.0f);
            break;
        case ElementAlignment::Bottom:
            childX = topLeft.x + layoutSize.width - style.padding.width -
                     childSize.width;
            break;
        }

        switch (verticalAlignment) {
        case ElementAlignment::Top:
            childY = topLeft.y + style.padding.height;
            break;
        case ElementAlignment::Center:
            childY = topLeft.y + style.padding.height +
                     ((contentHeight - childSize.height) / 2.0f);
            break;
        case ElementAlignment::Bottom:
            childY = topLeft.y + layoutSize.height - style.padding.height -
                     childSize.height;
            break;
        }

        setChildTopLeft(child, {.x = childX, .y = childY}, childSize);
    }
}

Position2d graphite::getAnchoredTopLeft(Position2d position, Size2d size,
                                        LayoutAnchor anchor) {
    Position2d topLeft = position;

    switch (anchor) {
    case LayoutAnchor::TopLeft:
        break;
    case LayoutAnchor::TopCenter:
        topLeft.x -= size.width / 2.0f;
        break;
    case LayoutAnchor::TopRight:
        topLeft.x -= size.width;
        break;
    case LayoutAnchor::CenterLeft:
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::Center:
        topLeft.x -= size.width / 2.0f;
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::CenterRight:
        topLeft.x -= size.width;
        topLeft.y -= size.height / 2.0f;
        break;
    case LayoutAnchor::BottomLeft:
        topLeft.y -= size.height;
        break;
    case LayoutAnchor::BottomCenter:
        topLeft.x -= size.width / 2.0f;
        topLeft.y -= size.height;
        break;
    case LayoutAnchor::BottomRight:
        topLeft.x -= size.width;
        topLeft.y -= size.height;
        break;
    }

    return topLeft;
}
