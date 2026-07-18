//
// compound.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Compound Object implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

class CompoundObject::LateCompoundRenderable : public Renderable {
  public:
    explicit LateCompoundRenderable(CompoundObject &owner) : parent(owner) {}

    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline) override {
        parent.renderLate(dt, commandBuffer, updatePipeline);
    }

    void initialize() override {}

    void update(Window &window) override { parent.updateLate(window); }

    void setViewMatrix(const glm::mat4 &view) override {
        parent.setLateViewMatrix(view);
    }

    void setProjectionMatrix(const glm::mat4 &projection) override {
        parent.setLateProjectionMatrix(projection);
    }

    std::optional<std::shared_ptr<opal::Pipeline>> getPipeline() override {
        return parent.getLateShaderPipelineInternal();
    }

    void setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) override {
        parent.setLatePipeline(pipeline);
    }

    bool canCastShadows() const override { return parent.lateCanCastShadows(); }
    bool canUseDeferredRendering() override { return false; }

  private:
    CompoundObject &parent;
};

Renderable *CompoundObject::getLateRenderable() {
    if (lateForwardObjects.empty()) {
        return nullptr;
    }
    if (!lateRenderableProxy) {
        lateRenderableProxy = std::make_unique<LateCompoundRenderable>(*this);
    }
    return lateRenderableProxy.get();
}

void CompoundObject::initialize() {
    init();
    for (auto &component : components) {
        component->init();
    }

    if (!lateForwardObjects.empty() && !lateRenderableRegistered) {
        if (!lateRenderableProxy) {
            lateRenderableProxy =
                std::make_unique<LateCompoundRenderable>(*this);
        }
        if (Window::mainWindow != nullptr) {
            Window::mainWindow->addLateForwardObject(lateRenderableProxy.get());
            lateRenderableRegistered = true;
        }
    }
}

void CompoundObject::render(float dt,
                            std::shared_ptr<opal::CommandBuffer> commandBuffer,
                            bool updatePipeline) {
    for (auto &component : components) {
        component->update(dt);
    }
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "CompoundObject::render requires a valid command buffer");
    }
    for (auto &obj : objects) {
        if (obj != nullptr && obj->renderLateForward) {
            continue;
        }
        obj->render(dt, commandBuffer, updatePipeline);
    }
}

void CompoundObject::renderLate(
    float dt, const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
    bool updatePipeline) {
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "CompoundObject::renderLate requires a valid command buffer");
    }
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->render(dt, commandBuffer, updatePipeline);
    }
}

void CompoundObject::setViewMatrix(const glm::mat4 &view) {
    for (auto &obj : objects) {
        obj->setViewMatrix(view);
    }
}

void CompoundObject::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &obj : objects) {
        obj->setProjectionMatrix(projection);
    }
}

bool CompoundObject::canUseDeferredRendering() {
    for (const auto &obj : objects) {
        if (!obj->canUseDeferredRendering()) {
            for (auto &forwardObject : objects) {
                if (CoreObject *coreObj =
                        dynamic_cast<CoreObject *>(forwardObject);
                    coreObj != nullptr) {
                    coreObj->useDeferredRendering = false;
                }
            }
            return false;
        }
    }
    for (auto &obj : objects) {
        if (CoreObject *coreObj = dynamic_cast<CoreObject *>(obj);
            coreObj != nullptr) {
            coreObj->useDeferredRendering = true;
        }
    }
    return true;
}

std::optional<std::shared_ptr<opal::Pipeline>> CompoundObject::getPipeline() {
    if (!objects.empty()) {
        auto shader = objects[0]->getPipeline();
        if (shader.has_value()) {
            return shader;
        }
    }
    return getLateShaderPipelineInternal();
}

void CompoundObject::setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) {
    for (auto &obj : objects) {
        obj->setPipeline(pipeline);
    }
}

Position3d CompoundObject::getPosition() const {
    return position;
}

Size3d CompoundObject::getScale() const {
    if (objects.empty()) {
        if (!lateForwardObjects.empty() && lateForwardObjects[0] != nullptr) {
            return lateForwardObjects[0]->getScale();
        }
        return Size3d{1.0, 1.0, 1.0};
    }
    return objects[0]->getScale();
}

void CompoundObject::update(Window &window) {
    updateObjects(window);
    changedPosition = false;
}

bool CompoundObject::canCastShadows() const {
    return std::ranges::any_of(
        objects, [](const auto &obj) { return obj->canCastShadows(); });
}

void CompoundObject::setPosition(const Position3d &newPosition) {
    Position3d delta = newPosition - position;
    this->position = newPosition;
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->move(delta);
        }
    }
    changedPosition = false;
}

void CompoundObject::move(const Position3d &deltaPosition) {
    this->position += deltaPosition;
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->move(deltaPosition);
        }
    }
    changedPosition = false;
}

void CompoundObject::setRotation(const Rotation3d &newRotation) {
    for (auto &obj : objects) {
        obj->setRotation(newRotation);
    }
}

void CompoundObject::lookAt(const Position3d &target, const Normal3d &up) {
    for (auto &obj : objects) {
        obj->lookAt(target, up);
    }
}

void CompoundObject::rotate(const Rotation3d &deltaRotation) {
    for (auto &obj : objects) {
        obj->rotate(deltaRotation);
    }
}

void CompoundObject::setScale(const Scale3d &newScale) {
    for (auto &obj : objects) {
        obj->setScale(newScale);
    }
}

void CompoundObject::hide() {
    for (auto &obj : objects) {
        obj->hide();
    }
}

void CompoundObject::show() {
    for (auto &obj : objects) {
        obj->show();
    }
}

std::vector<CoreVertex> CompoundObject::getVertices() const {
    std::vector<CoreVertex> allVertices;
    for (const auto &obj : objects) {
        std::vector<CoreVertex> objVertices = obj->getVertices();
        allVertices.insert(allVertices.end(), objVertices.begin(),
                           objVertices.end());
    }
    return allVertices;
}

void CompoundObject::updateLate(Window &window) { (void)window; }

void CompoundObject::setLateViewMatrix(const glm::mat4 &view) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setViewMatrix(view);
    }
}

void CompoundObject::setLateProjectionMatrix(const glm::mat4 &projection) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setProjectionMatrix(projection);
    }
}

std::optional<std::shared_ptr<opal::Pipeline>>
CompoundObject::getLateShaderPipelineInternal() {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        auto program = obj->getPipeline();
        if (program.has_value()) {
            return program;
        }
    }
    return std::nullopt;
}

void CompoundObject::setLatePipeline(std::shared_ptr<opal::Pipeline> pipeline) {
    for (auto *obj : lateForwardObjects) {
        if (obj == nullptr) {
            continue;
        }
        obj->setPipeline(pipeline);
    }
}

bool CompoundObject::lateCanCastShadows() const {
    return std::ranges::any_of(lateForwardObjects, [](const auto *obj) {
        return obj != nullptr && obj->canCastShadows();
    });
}

Window *Component::getWindow() { return Window::mainWindow; }

void UIView::setViewMatrix(const glm::mat4 &view) {
    for (auto &obj : children) {
        obj->setViewMatrix(view);
    }
}

void UIView::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &obj : children) {
        obj->setProjectionMatrix(projection);
    }
}

void UIView::render(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline) {
    for (auto &component : components) {
        component->update(dt);
    }
    if (commandBuffer == nullptr) {
        throw std::runtime_error(
            "UIView::render requires a valid command buffer");
    }
    for (auto &obj : children) {
        obj->render(dt, commandBuffer, updatePipeline);
    }
}
