/*
 camera.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Camera function implementations
 Copyright (c) 2025 maxvdec
*/

#include "atlas/camera.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/input.h"
#include <algorithm>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <string>

namespace {
constexpr float CAMERA_CONTROLLER_DEADZONE = 0.2f;

float applyControllerDeadzone(float value) {
    return std::abs(value) < CAMERA_CONTROLLER_DEADZONE ? 0.0f : value;
}

glm::vec2 sampleControllerAxisPair(Window &window, int axisIndexX,
                                   int axisIndexY, bool invertY) {
    glm::vec2 selected(0.0f, 0.0f);
    float bestMagnitude = -1.0f;

    for (const auto &controller : window.getControllers()) {
        auto pair = window.getControllerAxisPairValue(controller.id, axisIndexX,
                                                      axisIndexY);
        float x = applyControllerDeadzone(pair.first);
        float y = applyControllerDeadzone(pair.second);
        if (invertY) {
            y = -y;
        }

        float magnitude = glm::length(glm::vec2(x, y));
        if (magnitude > bestMagnitude) {
            selected = {x, y};
            bestMagnitude = magnitude;
        }
    }

    return selected;
}
} // namespace

glm::mat4 Camera::calculateViewMatrix() const {
    glm::dvec3 camPos(position.x, position.y, position.z);
    glm::dvec3 camTarget(target.x, target.y, target.z);
    glm::dvec3 direction = glm::normalize(camTarget - camPos);
    glm::dvec3 upVector =
        std::abs(glm::dot(direction, glm::dvec3(0.0, 1.0, 0.0))) > 0.999
            ? glm::dvec3(0.0, 0.0, 1.0)
            : glm::dvec3(0.0, 1.0, 0.0);

    return glm::mat4(glm::lookAt(camPos, camTarget, upVector));
}

void Camera::move(const Position3d &delta) {
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
}

void Camera::setPosition(const Position3d &newPosition) {
    position = newPosition;
}

void Camera::lookAt(const Point3d &newTarget) {
    glm::vec3 delta(newTarget.x - position.x, newTarget.y - position.y,
                    newTarget.z - position.z);
    if (!std::isfinite(delta.x) || !std::isfinite(delta.y) ||
        !std::isfinite(delta.z) || glm::length(delta) < 0.000001f) {
        return;
    }
    target = newTarget;
    glm::vec3 dir = glm::normalize(delta);
    pitch = glm::degrees(asin(dir.y));
    yaw = glm::degrees(atan2(dir.z, dir.x));
    targetPitch = pitch;
    targetYaw = yaw;
}

void Camera::setPositionKeepingOrientation(const Position3d &newPos) {
    glm::dvec3 oldPos(position.x, position.y, position.z);
    glm::dvec3 oldTgt(target.x, target.y, target.z);
    glm::dvec3 forward = glm::normalize(oldTgt - oldPos);

    position = newPos;
    target = {position.x + forward.x, position.y + forward.y,
              position.z + forward.z};
}

void Camera::moveTo(Direction3d direction, float speed) {
    glm::vec3 camPos = glm::vec3(position.x, position.y, position.z);
    glm::vec3 camFront =
        glm::normalize(glm::vec3(target.x, target.y, target.z) - camPos);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f); // Assuming Y-up coordinate system

    switch (direction) {
    case Direction3d::Forward:
        camPos += speed * camFront;
        break;
    case Direction3d::Backward:
        camPos -= speed * camFront;
        break;
    case Direction3d::Left:
        camPos -= glm::normalize(glm::cross(camFront, upVector)) * speed;
        break;
    case Direction3d::Right:
        camPos += glm::normalize(glm::cross(camFront, upVector)) * speed;
        break;
    case Direction3d::Up:
        camPos += upVector * speed;
        break;
    case Direction3d::Down:
        camPos -= upVector * speed;
        break;
    }

    position = {camPos.x, camPos.y, camPos.z};
    target = {camPos.x + camFront.x, camPos.y + camFront.y,
              camPos.z + camFront.z};
}

void Camera::update(Window &window) {
    glm::vec3 camPos = glm::vec3(position.x, position.y, position.z);
    glm::vec3 camFront =
        glm::normalize(glm::vec3(target.x, target.y, target.z) - camPos);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f); // Assuming Y-up coordinate system

    float deltaTime = window.getDeltaTime();
    glm::vec2 controllerMove = sampleControllerAxisPair(
        window, CONTROLLER_AXIS_LEFT_X, CONTROLLER_AXIS_LEFT_Y, true);
    glm::vec2 controllerLook = sampleControllerAxisPair(
        window, CONTROLLER_AXIS_RIGHT_X, CONTROLLER_AXIS_RIGHT_Y, true);

    float cameraSpeed = movementSpeed * deltaTime;

    targetYaw += controllerLook.x * controllerLookSensitivity * deltaTime;
    targetPitch += controllerLook.y * controllerLookSensitivity * deltaTime;

    targetPitch = std::min(targetPitch, 89.0f);
    targetPitch = std::max(targetPitch, -89.0f);

    yaw += (targetYaw - yaw) * lookSmoothness;
    pitch += (targetPitch - pitch) * lookSmoothness;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(front);

    if (window.isKeyActive(Key::W) || window.isKeyActive(Key::Up)) {
        camPos += cameraSpeed * camFront;
    }
    if (window.isKeyActive(Key::S) || window.isKeyActive(Key::Down)) {
        camPos -= cameraSpeed * camFront;
    }
    if (window.isKeyActive(Key::A) || window.isKeyActive(Key::Left)) {
        camPos -= glm::normalize(glm::cross(camFront, upVector)) * cameraSpeed;
    }
    if (window.isKeyActive(Key::D) || window.isKeyActive(Key::Right)) {
        camPos += glm::normalize(glm::cross(camFront, upVector)) * cameraSpeed;
    }
    if (window.isKeyActive(Key::Space)) {
        camPos.y += cameraSpeed;
    }
    if (window.isKeyActive(Key::LeftShift)) {
        camPos.y -= cameraSpeed;
    }

    glm::vec2 moveVector = controllerMove;
    if (glm::length(moveVector) > 1.0f) {
        moveVector = glm::normalize(moveVector);
    }
    camPos += moveVector.y * cameraSpeed * camFront;
    camPos += moveVector.x * cameraSpeed *
              glm::normalize(glm::cross(camFront, upVector));

    position = {camPos.x, camPos.y, camPos.z};
    target = {camPos.x + camFront.x, camPos.y + camFront.y,
              camPos.z + camFront.z};
}

void Camera::updateWithActions(Window &window, const std::string &moveAxis,
                               const std::string &lookAction,
                               const std::string &upAndDownAction) {
    AxisPacket moveInput = moveAxis.empty()
                               ? AxisPacket{}
                               : window.getAxisActionValue(moveAxis);
    AxisPacket lookInput = lookAction.empty()
                               ? AxisPacket{}
                               : window.getAxisActionValue(lookAction);
    AxisPacket upDownInput =
        upAndDownAction.empty()
            ? AxisPacket{}
            : window.getAxisActionValue(upAndDownAction);

    float deltaTime = window.getDeltaTime();
    if (!lookAction.empty()) {
        float xoffset = lookInput.inputDeltaX * mouseSensitivity;
        float yoffset = lookInput.inputDeltaY * mouseSensitivity;

        if (lookInput.hasValueInput) {
            glm::vec2 lookVector(lookInput.valueX, lookInput.valueY);
            if (glm::length(lookVector) > 1.0f) {
                lookVector = glm::normalize(lookVector);
            }
            xoffset +=
                lookVector.x * controllerLookSensitivity * deltaTime;
            yoffset +=
                lookVector.y * controllerLookSensitivity * deltaTime;
        }

        targetYaw += xoffset;
        targetPitch += yoffset;

        targetPitch = std::min(targetPitch, 89.0f);
        targetPitch = std::max(targetPitch, -89.0f);

        yaw += (targetYaw - yaw) * lookSmoothness;
        pitch += (targetPitch - pitch) * lookSmoothness;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        target = {position.x + front.x, position.y + front.y,
                  position.z + front.z};
    }

    glm::vec3 camPos = glm::vec3(position.x, position.y, position.z);
    glm::vec3 camFront =
        glm::normalize(glm::vec3(target.x, target.y, target.z) - camPos);
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);
    float cameraSpeed = movementSpeed * deltaTime;
    glm::vec2 moveVector = moveInput.hasValueInput
                               ? glm::vec2(moveInput.valueX, moveInput.valueY)
                               : glm::vec2(moveInput.x, moveInput.y);
    if (glm::length(moveVector) > 1.0f) {
        moveVector = glm::normalize(moveVector);
    }
    float verticalInput =
        upDownInput.hasValueInput ? upDownInput.valueY : upDownInput.y;
    camPos += moveVector.y * cameraSpeed * camFront;
    camPos += moveVector.x * cameraSpeed *
              glm::normalize(glm::cross(camFront, upVector));
    camPos += verticalInput * cameraSpeed * upVector;

    position = {camPos.x, camPos.y, camPos.z};
    target = {camPos.x + camFront.x, camPos.y + camFront.y,
              camPos.z + camFront.z};
}

void Camera::updateLook(Window &, Movement2d movement) {
    float xoffset = movement.x * mouseSensitivity;
    float yoffset = movement.y * mouseSensitivity;

    targetYaw += xoffset;
    targetPitch += yoffset;

    targetPitch = std::min(targetPitch, 89.0f);
    targetPitch = std::max(targetPitch, -89.0f);

    yaw += (targetYaw - yaw) * lookSmoothness;
    pitch += (targetPitch - pitch) * lookSmoothness;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    target = {position.x + front.x, position.y + front.y, position.z + front.z};
}

void Camera::updateZoom(Window &, Movement2d offset) {
    if (!useOrthographic) {
        fov -= offset.y;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 90.0f)
            fov = 90.0f;
    } else {
        orthographicSize -= offset.y * 0.1f;
        if (orthographicSize < 1.0f)
            orthographicSize = 1.0f;
        if (orthographicSize > 20.0f)
            orthographicSize = 20.0f;
    }
}
