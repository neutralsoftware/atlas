//
// atlas_joints.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Joint implementation for Atlas
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "atlas/window.h"
#include "atlas/physics.h"
#include "bezel/bezel.h"
#include "atlas/tracer/log.h"
#include <memory>
#include <numbers>
#include <variant>

void FixedJoint::beforePhysics() {
    if (!joint) {
        auto nextJoint = std::make_shared<bezel::FixedJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error(
                    "FixedJoint parent GameObject has no Rigidbody component.");
                return;
            }
            nextJoint->parent = parentObject->rigidbody->body.get();
        } else {
            nextJoint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "FixedJoint child GameObject has no Rigidbody component.");
                return;
            }
            nextJoint->child = childObject->rigidbody->body.get();
        } else {
            nextJoint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "FixedJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            nextJoint->space = bezel::Space::Global;
            break;
        case Space::Local:
            nextJoint->space = bezel::Space::Local;
            break;
        }
        nextJoint->anchor = anchor;
        nextJoint->breakForce = breakForce;
        nextJoint->breakTorque = breakTorque;
        nextJoint->create(Window::mainWindow->physicsWorld);
        joint = std::move(nextJoint);
    }
}

void FixedJoint::breakJoint() {
    if (joint) {
        joint->breakJoint();
        joint.reset();
    }
}

void HingeJoint::beforePhysics() {
    if (!joint) {
        auto nextJoint = std::make_shared<bezel::HingeJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error(
                    "HingeJoint parent GameObject has no Rigidbody component.");
                return;
            }
            nextJoint->parent = parentObject->rigidbody->body.get();
        } else {
            nextJoint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "HingeJoint child GameObject has no Rigidbody component.");
                return;
            }
            nextJoint->child = childObject->rigidbody->body.get();
        } else {
            nextJoint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "HingeJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            nextJoint->space = bezel::Space::Global;
            break;
        case Space::Local:
            nextJoint->space = bezel::Space::Local;
            break;
        }
        nextJoint->anchor = anchor;
        nextJoint->breakForce = breakForce;
        nextJoint->breakTorque = breakTorque;

        nextJoint->axis1 = axis1;
        nextJoint->axis2 = axis2;
        nextJoint->limits.enabled = limits.enabled;
        nextJoint->limits.minAngle =
            limits.minAngle * (std::numbers::pi_v<float> / 180.0f);
        nextJoint->limits.maxAngle =
            limits.maxAngle * (std::numbers::pi_v<float> / 180.0f);
        nextJoint->motor.enabled = motor.enabled;
        nextJoint->motor.maxForce = motor.maxForce;
        nextJoint->motor.maxTorque = motor.maxTorque;
        nextJoint->create(Window::mainWindow->physicsWorld);
        joint = std::move(nextJoint);
    }
}

void HingeJoint::breakJoint() {
    if (joint) {
        joint->breakJoint();
        joint.reset();
    }
}

void SpringJoint::beforePhysics() {
    if (!joint) {
        auto nextJoint = std::make_shared<bezel::SpringJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error("SpringJoint parent GameObject has no Rigidbody "
                            "component.");
                return;
            }
            nextJoint->parent = parentObject->rigidbody->body.get();
        } else {
            nextJoint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "SpringJoint child GameObject has no Rigidbody component.");
                return;
            }
            nextJoint->child = childObject->rigidbody->body.get();
        } else {
            nextJoint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "SpringJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            nextJoint->space = bezel::Space::Global;
            break;
        case Space::Local:
            nextJoint->space = bezel::Space::Local;
            break;
        }
        nextJoint->anchor = anchor;
        nextJoint->breakForce = breakForce;
        nextJoint->breakTorque = breakTorque;

        nextJoint->restLength = restLength;
        nextJoint->useLimits = useLimits;
        nextJoint->minLength = minLength;
        nextJoint->maxLength = maxLength;
        nextJoint->spring.damping = spring.damping;
        nextJoint->spring.enabled = spring.enabled;
        nextJoint->spring.mode = static_cast<bezel::SpringMode>(spring.mode);
        nextJoint->spring.frequencyHz = spring.frequencyHz;
        nextJoint->spring.dampingRatio = spring.dampingRatio;
        nextJoint->spring.stiffness = spring.stiffness;
        nextJoint->spring.damping = spring.damping;
        nextJoint->create(Window::mainWindow->physicsWorld);
        joint = std::move(nextJoint);
    }
}

void SpringJoint::breakJoint() {
    if (joint) {
        joint->breakJoint();
        joint.reset();
    }
}
