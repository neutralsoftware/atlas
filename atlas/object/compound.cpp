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
#include <cmath>
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

    bool canCastShadows() const override { return false; }
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

void CompoundObject::addObject(GameObject *obj, bool childInitialized) {
    if (obj == nullptr || obj == this || containsObject(obj)) {
        return;
    }
    objects.push_back(obj);
    if (obj->renderLateForward) {
        lateForwardObjects.push_back(obj);
    }
    if (childInitialized) {
        initializedObjects.insert(obj);
    } else if (initialized) {
        obj->initialize();
        initializedObjects.insert(obj);
    }
    syncLateRenderableRegistration();
}

void CompoundObject::removeObject(GameObject *obj) {
    if (obj == nullptr) {
        return;
    }
    objects.erase(std::remove(objects.begin(), objects.end(), obj),
                  objects.end());
    lateForwardObjects.erase(
        std::remove(lateForwardObjects.begin(), lateForwardObjects.end(), obj),
        lateForwardObjects.end());
    initializedObjects.erase(obj);
    syncLateRenderableRegistration();
}

bool CompoundObject::containsObject(const GameObject *obj) const {
    return obj != nullptr && std::ranges::find(objects, obj) != objects.end();
}

void CompoundObject::syncLateRenderableRegistration() {
    if (!initialized || Window::mainWindow == nullptr) {
        return;
    }
    if (!lateForwardObjects.empty() && !lateRenderableRegistered) {
        if (!lateRenderableProxy) {
            lateRenderableProxy =
                std::make_unique<LateCompoundRenderable>(*this);
        }
        Window::mainWindow->addLateForwardObject(lateRenderableProxy.get());
        lateRenderableRegistered = true;
    } else if (lateForwardObjects.empty() && lateRenderableRegistered) {
        Window::mainWindow->removeObjectFromRendering(
            lateRenderableProxy.get());
        lateRenderableRegistered = false;
    }
}

void CompoundObject::initialize() {
    if (initialized) {
        return;
    }
    init();
    for (auto &component : components) {
        component->init();
    }
    for (auto *obj : objects) {
        if (obj != nullptr && !initializedObjects.contains(obj)) {
            obj->initialize();
            initializedObjects.insert(obj);
        }
    }
    initialized = true;
    syncLateRenderableRegistration();
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
        if (obj == nullptr || obj->renderLateForward) {
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
        if (obj != nullptr) {
            obj->setViewMatrix(view);
        }
    }
}

void CompoundObject::setProjectionMatrix(const glm::mat4 &projection) {
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->setProjectionMatrix(projection);
        }
    }
}

bool CompoundObject::canUseDeferredRendering() {
    for (const auto &obj : objects) {
        if (obj == nullptr || obj->renderLateForward) {
            continue;
        }
        if (!obj->canUseDeferredRendering()) {
            for (auto &forwardObject : objects) {
                if (forwardObject == nullptr ||
                    forwardObject->renderLateForward) {
                    continue;
                }
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
        if (obj == nullptr || obj->renderLateForward) {
            continue;
        }
        if (CoreObject *coreObj = dynamic_cast<CoreObject *>(obj);
            coreObj != nullptr) {
            coreObj->useDeferredRendering = true;
        }
    }
    return true;
}

std::optional<std::shared_ptr<opal::Pipeline>> CompoundObject::getPipeline() {
    for (auto *obj : objects) {
        if (obj == nullptr || obj->renderLateForward) {
            continue;
        }
        auto shader = obj->getPipeline();
        if (shader.has_value()) {
            return shader;
        }
    }
    return getLateShaderPipelineInternal();
}

void CompoundObject::setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) {
    for (auto &obj : objects) {
        if (obj != nullptr && !obj->renderLateForward) {
            obj->setPipeline(pipeline);
        }
    }
}

Position3d CompoundObject::getPosition() const { return position; }

Rotation3d CompoundObject::getRotation() const { return rotation; }

Size3d CompoundObject::getScale() const { return scale; }

void CompoundObject::update(Window &window) {
    updateObjects(window);
    for (auto *obj : objects) {
        if (obj != nullptr) {
            obj->update(window);
        }
    }
}

void CompoundObject::beforePhysics() {
    GameObject::beforePhysics();
    for (auto *obj : objects) {
        if (obj != nullptr) {
            obj->beforePhysics();
        }
    }
}

bool CompoundObject::canCastShadows() const {
    return std::ranges::any_of(objects, [](const auto *obj) {
        return obj != nullptr && obj->canCastShadows();
    });
}

void CompoundObject::setPosition(const Position3d &newPosition) {
    Position3d delta = newPosition - position;
    this->position = newPosition;
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->move(delta);
        }
    }
}

void CompoundObject::move(const Position3d &deltaPosition) {
    this->position += deltaPosition;
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->move(deltaPosition);
        }
    }
}

void CompoundObject::setRotation(const Rotation3d &newRotation) {
    const glm::quat oldQuaternion = glm::normalize(rotation.toGlmQuat());
    const glm::quat newQuaternion = glm::normalize(newRotation.toGlmQuat());
    const glm::quat delta = newQuaternion * glm::inverse(oldQuaternion);
    rotation = newRotation;
    for (auto *obj : objects) {
        if (obj == nullptr) {
            continue;
        }
        const glm::vec3 offset = obj->getPosition().toGlm() - position.toGlm();
        obj->setPosition(
            Position3d::fromGlm(position.toGlm() + delta * offset));
        const glm::quat childRotation =
            glm::normalize(obj->getRotation().toGlmQuat());
        obj->setRotation(
            Rotation3d::fromGlmQuat(glm::normalize(delta * childRotation)));
    }
}

void CompoundObject::lookAt(const Position3d &target, const Normal3d &up) {
    glm::vec3 forward = target.toGlm() - position.toGlm();
    if (glm::length(forward) < 0.000001f) {
        return;
    }
    forward = glm::normalize(forward);
    glm::vec3 upVector = up.toGlm();
    if (glm::length(upVector) < 0.000001f ||
        std::abs(glm::dot(glm::normalize(upVector), forward)) > 0.9999f) {
        upVector = std::abs(forward.y) < 0.9999f ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                 : glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 right = glm::normalize(glm::cross(forward, upVector));
    glm::vec3 realUp = glm::cross(right, forward);
    glm::mat3 matrix;
    matrix[0] = right;
    matrix[1] = realUp;
    matrix[2] = -forward;
    setRotation(
        Rotation3d::fromGlmQuat(glm::normalize(glm::quat_cast(matrix))));
}

void CompoundObject::rotate(const Rotation3d &deltaRotation) {
    setRotation(rotation + deltaRotation);
}

void CompoundObject::setScale(const Scale3d &newScale) {
    const auto factor = [](double next, double previous) {
        return std::abs(previous) < 0.000001 ? next : next / previous;
    };
    const glm::vec3 scaleFactor(factor(newScale.x, scale.x),
                                factor(newScale.y, scale.y),
                                factor(newScale.z, scale.z));
    const glm::quat orientation = glm::normalize(rotation.toGlmQuat());
    const glm::quat inverseOrientation = glm::inverse(orientation);
    scale = newScale;
    for (auto *obj : objects) {
        if (obj == nullptr) {
            continue;
        }
        glm::vec3 offset = obj->getPosition().toGlm() - position.toGlm();
        offset = orientation * ((inverseOrientation * offset) * scaleFactor);
        obj->setPosition(Position3d::fromGlm(position.toGlm() + offset));
        const Size3d childScale = obj->getScale();
        obj->setScale({childScale.x * scaleFactor.x,
                       childScale.y * scaleFactor.y,
                       childScale.z * scaleFactor.z});
    }
}

void CompoundObject::hide() {
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->hide();
        }
    }
}

void CompoundObject::show() {
    for (auto &obj : objects) {
        if (obj != nullptr) {
            obj->show();
        }
    }
}

std::vector<CoreVertex> CompoundObject::getVertices() const {
    std::vector<CoreVertex> allVertices;
    for (const auto &obj : objects) {
        if (obj == nullptr) {
            continue;
        }
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
