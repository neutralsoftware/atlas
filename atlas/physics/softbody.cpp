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

void recreateSoftbody(Softbody *softbody) {
    if (!ensureSoftbodyAndWorld(softbody) || !softbody->isCreated()) {
        return;
    }

    auto world = Window::mainWindow->physicsWorld;
    softbody->body->destroy(world);
    softbody->body->position = softbody->object->getPosition();
    softbody->body->rotation = softbody->object->getRotation();
    softbody->body->rotationQuat =
        glm::normalize(softbody->object->getRotation().toGlmQuat());
    softbody->body->create(world);
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
        throw std::runtime_error("Softbody initialization failed: "
                                 "Softbody requires a CoreObject.");
        return;
    }

    if (coreObject->vertices.empty() || coreObject->indices.empty()) {
        throw std::runtime_error("Softbody initialization failed: "
                                 "CoreObject has no mesh geometry.");
        return;
    }

    body->id.atlasId = object->getId();
    body->position = object->getPosition();
    body->rotation = object->getRotation();
    body->rotationQuat = glm::normalize(object->getRotation().toGlmQuat());
    body->isSensor = isSensor;
    body->sensorSignal = sendSignal;

    body->setObject(coreObject);
}

void Softbody::update(float dt) {
    (void)dt;

    if (!createdSoftbody) {
        body->createMesh();
        body->create(Window::mainWindow->physicsWorld);
        createdSoftbody = true;
    }

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

    const bool sensorChanged = body->isSensor != isSensor;
    body->isSensor = isSensor;
    body->sensorSignal = sendSignal;
    if (sensorChanged) {
        recreateSoftbody(this);
    }
}

void Softbody::setMass(float mass) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    mass = std::max(0.0f, mass);
    if (body->mass == mass) {
        return;
    }
    body->mass = mass;
    recreateSoftbody(this);
}

void Softbody::setStiffness(float stiffness) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    stiffness = std::clamp(stiffness, 0.0f, 1.0f);
    if (body->material.stiffness == stiffness) {
        return;
    }
    body->material.stiffness = stiffness;
    recreateSoftbody(this);
}

void Softbody::setVolumeStiffness(float stiffness) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    stiffness = std::clamp(stiffness, 0.0f, 1.0f);
    if (body->material.volumeStiffness == stiffness) {
        return;
    }
    body->material.volumeStiffness = stiffness;
    recreateSoftbody(this);
}

void Softbody::setDamping(float damping) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    damping = std::max(0.0f, damping);
    if (body->material.damping == damping) {
        return;
    }
    body->material.damping = damping;
    recreateSoftbody(this);
}

void Softbody::setFriction(float friction) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    friction = std::max(0.0f, friction);
    if (body->material.friction == friction) {
        return;
    }
    body->material.friction = friction;
    recreateSoftbody(this);
}

void Softbody::setRestitution(float restitution) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    restitution = std::clamp(restitution, 0.0f, 1.0f);
    if (body->material.restitution == restitution) {
        return;
    }
    body->material.restitution = restitution;
    recreateSoftbody(this);
}

void Softbody::setGravityFactor(float gravityFactor) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    if (body->gravityFactor == gravityFactor) {
        return;
    }
    body->gravityFactor = gravityFactor;
    recreateSoftbody(this);
}

void Softbody::setSolverIterations(uint32_t solverIterations) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    solverIterations = std::max(1u, solverIterations);
    if (body->solverIterations == solverIterations) {
        return;
    }
    body->solverIterations = solverIterations;
    recreateSoftbody(this);
}

void Softbody::setAllowSleeping(bool allowSleeping) {
    if (!body) {
        body = std::make_shared<bezel::Softbody>();
    }

    if (body->allowSleeping == allowSleeping) {
        return;
    }
    body->allowSleeping = allowSleeping;
    recreateSoftbody(this);
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
