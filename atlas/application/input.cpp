//
// input.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Controller and input handling implementation
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/input.h"
#include "atlas/texture.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"

AxisTrigger Gamepad::getAxisTrigger(const ControllerAxis &axis) const {
    switch (axis) {
    case ControllerAxis::LeftStick:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_LEFT_X, false,
                                       CONTROLLER_AXIS_LEFT_Y);
    case ControllerAxis::LeftStickX:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_LEFT_X, true);
    case ControllerAxis::LeftStickY:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_LEFT_Y, true);
    case ControllerAxis::RightStick:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_RIGHT_X, false,
                                       CONTROLLER_AXIS_RIGHT_Y);
    case ControllerAxis::RightStickX:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_RIGHT_X, true);
    case ControllerAxis::RightStickY:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_RIGHT_Y, true);
    case ControllerAxis::Trigger:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_LEFT_TRIGGER, false,
                                       CONTROLLER_AXIS_RIGHT_TRIGGER);
    case ControllerAxis::LeftTrigger:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_LEFT_TRIGGER, true);
    case ControllerAxis::RightTrigger:
        return AxisTrigger::controller(this->controllerID,
                                       CONTROLLER_AXIS_RIGHT_TRIGGER, true);
    default:
        return AxisTrigger{};
    }
}

AxisTrigger Gamepad::getGlobalAxisTrigger(const ControllerAxis &axis) {
    switch (axis) {
    case ControllerAxis::LeftStick:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_LEFT_X, false,
                                       CONTROLLER_AXIS_LEFT_Y);
    case ControllerAxis::LeftStickX:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_LEFT_X, true);
    case ControllerAxis::LeftStickY:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_LEFT_Y, true);
    case ControllerAxis::RightStick:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_RIGHT_X, false,
                                       CONTROLLER_AXIS_RIGHT_Y);
    case ControllerAxis::RightStickX:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_RIGHT_X, true);
    case ControllerAxis::RightStickY:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_RIGHT_Y, true);
    case ControllerAxis::Trigger:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_LEFT_TRIGGER, false,
                                       CONTROLLER_AXIS_RIGHT_TRIGGER);
    case ControllerAxis::LeftTrigger:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_LEFT_TRIGGER, true);
    case ControllerAxis::RightTrigger:
        return AxisTrigger::controller(CONTROLLER_UNDEFINED,
                                       CONTROLLER_AXIS_RIGHT_TRIGGER, true);
    default:
        return AxisTrigger{};
    }
}

Trigger Gamepad::getButtonTrigger(int buttonIndex) const {
    if (buttonIndex < 0 || buttonIndex >= (int)ControllerButton::ButtonCount) {
        atlas_error("Button is out of bounds");
        return Trigger{};
    }

    return Trigger::fromControllerButton(this->controllerID, buttonIndex);
}

Trigger Gamepad::getGlobalButtonTrigger(int buttonIndex) {
    if (buttonIndex < 0 || buttonIndex >= (int)ControllerButton::ButtonCount) {
        atlas_error("Button is out of bounds");
        return Trigger{};
    }

    return Trigger::fromControllerButton(CONTROLLER_UNDEFINED, buttonIndex);
}

int Joystick::getAxisCount() const {
    SDL_Joystick *joystick = SDL_OpenJoystick(this->joystickID);
    if (joystick == nullptr) {
        return 0;
    }
    const int count = SDL_GetNumJoystickAxes(joystick);
    SDL_CloseJoystick(joystick);
    return count;
}

int Joystick::getButtonCount() const {
    SDL_Joystick *joystick = SDL_OpenJoystick(this->joystickID);
    if (joystick == nullptr) {
        return 0;
    }
    const int count = SDL_GetNumJoystickButtons(joystick);
    SDL_CloseJoystick(joystick);
    return count;
}

Trigger Joystick::getButtonTrigger(int buttonIndex) const {
    if (buttonIndex < 0 || buttonIndex >= this->getButtonCount()) {
        atlas_error("Button is out of bounds");
        return Trigger{};
    }
    return Trigger::fromControllerButton(this->joystickID, buttonIndex);
}

AxisTrigger Joystick::getSingleAxisTrigger(int axisIndex) const {
    if (axisIndex < 0 || axisIndex >= this->getAxisCount()) {
        atlas_error("Axis is out of bounds");
        return AxisTrigger{};
    }
    return AxisTrigger::controller(this->joystickID, axisIndex, true);
}

AxisTrigger Joystick::getDualAxisTrigger(int axisIndexX, int axisIndexY) const {
    if (axisIndexX < 0 || axisIndexX >= this->getAxisCount() ||
        axisIndexY < 0 || axisIndexY >= this->getAxisCount()) {
        atlas_error("Axis is out of bounds");
        return AxisTrigger{};
    }
    return AxisTrigger::controller(this->joystickID, axisIndexX, false,
                                   axisIndexY);
}

void Gamepad::rumble(float strength, float duration) const {
    SDL_Gamepad *gamepad = SDL_OpenGamepad(this->controllerID);
    if (!gamepad) {
        atlas_error("Failed to open gamepad for rumble");
        return;
    }
    SDL_RumbleGamepad(gamepad, (Uint16)(strength * 65535),
                      (Uint16)(strength * 65535), (Uint32)(duration * 1000));
}
