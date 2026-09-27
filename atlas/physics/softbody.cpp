//
// softbody.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Softbody implementation for Atlas
// SPDX-License-Identifier: MIT
//

#include "atlas/object.h"
#include "atlas/physics.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include "bezel/bezel.h"

namespace {

bool ensureSoftbodyAndWorld(Softbody *softbody) {
    return softbody && softbody->object && softbody->body &&
           Window::mainWindow && Window::mainWindow->physicsWorld;
}

} // namespace

void Softbody::atAttach() {
    if (!object) {
        atlas_warning("Softbody attached without a valid GameObject.");
        return;
    }

    auto *coreObject = dynamic_cast<CoreObject *>(object);

    if (!coreObject) {
        atlas_warning("Softbody can only be attached to CoreObject instances.");
        return;
    }

    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    body->id.atlasId = object->getId();
}

void Softbody::init() {
    if (!body) {
        atlas_warning("Softbody initialization failed: missing Bezel body.");
        return;
    }

    if (!object) {
        atlas_warning("Softbody initialization failed: missing GameObject.");
        return;
    }

    if (!Window::mainWindow) {
        atlas_warning("Softbody initialization failed: missing main window.");
        return;
    }

    if (!Window::mainWindow->physicsWorld) {
        atlas_warning("Softbody initialization failed: missing physics world.");
        return;
    }

    auto *coreObject = dynamic_cast<CoreObject *>(object);

    if (!coreObject) {
        atlas_error("Softbody initialization failed: "
                    "Softbody requires a CoreObject.");
        return;
    }

    if (coreObject->vertices.empty() || coreObject->indices.empty()) {
        atlas_error("Softbody initialization failed: "
                    "CoreObject has no mesh geometry.");
        return;
    }

    body->id.atlasId = object->getId();
    body->position = object->getPosition();
    body->rotation = object->getRotation();
    body->rotationQuat = glm::normalize(object->getRotation().toGlmQuat());
    body->isSensor = isSensor;
    body->sensorSignal = sendSignal;

    body->createMesh(coreObject);

    body->create(Window::mainWindow->physicsWorld);
}

void Softbody::update(float dt) {
    (void)dt;

    if (!ensureSoftbodyAndWorld(this)) {
        return;
    }

    if (!isCreated()) {
        return;
    }

    body->updateVertices(Window::mainWindow->physicsWorld);
}

void Softbody::beforePhysics() {
    if (!ensureSoftbodyAndWorld(this)) {
        return;
    }

    if (!isCreated()) {
        return;
    }

    body->isSensor = isSensor;
    body->sensorSignal = sendSignal;
}

void Softbody::setMass(float mass) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    body->mass = std::max(0.0f, mass);
}

void Softbody::setStiffness(float stiffness) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    body->material.stiffness = std::clamp(stiffness, 0.0f, 1.0f);
}

void Softbody::setVolumeStiffness(float stiffness) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    body->material.volumeStiffness = std::clamp(stiffness, 0.0f, 1.0f);
}

void Softbody::setAllowSleeping(bool allowSleeping) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    body->allowSleeping = allowSleeping;
}

Velocity3d Softbody::getLinearVelocity() {
    if (!ensureSoftbodyAndWorld(this)) {
        return {};
    }

    return body->getLinearVelocity(Window::mainWindow->physicsWorld);
}

Velocity3d Softbody::getVelocity() {
    if (!ensureSoftbodyAndWorld(this)) {
        return {};
    }

    return body->getVelocity(Window::mainWindow->physicsWorld);
}

bool Softbody::isCreated() const {
    return body && body->id.joltId != bezel::INVALID_JOLT_ID;
}

Softbody::~Softbody() {
    if (!body) {
        return;
    }

    if (!Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    if (body->id.joltId == bezel::INVALID_JOLT_ID) {
        return;
    }

    body->destroy(Window::mainWindow->physicsWorld);
}

std::shared_ptr<Component> Softbody::clone() const {
    auto cloned = std::make_shared<Softbody>();

    cloned->sendSignal = sendSignal;

    cloned->isSensor = isSensor;

    cloned->body = std::make_shared<bezel::Softbody>();

    if (body) {
        cloned->body->mass = body->mass;
        cloned->body->material = body->material;
        cloned->body->solverIterations = body->solverIterations;
        cloned->body->gravityFactor = body->gravityFactor;
        cloned->body->allowSleeping = body->allowSleeping;
        cloned->body->tags = body->tags;
        cloned->body->isSensor = body->isSensor;
        cloned->body->sensorSignal = body->sensorSignal;
    }

    return cloned;
}