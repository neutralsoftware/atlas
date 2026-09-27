//
// cloth_dynamics.cpp
// As part of the Atlas project
// Copyright (c) 2026 Neutral Software. All rights reserved.
// atlasengine.org | github.com/neutralsoftware
// --------------------------------------------------
// Description: Cloth Dynamics bridge between Bezel and Atlas
// SPDX-License-Identifier: MIT
//

#include "atlas/object.h"
#include "atlas/physics.h"
#include "atlas/window.h"

void Cloth::atAttach() {
    CoreObject *coreObject = dynamic_cast<CoreObject *>(object);

    if (!coreObject) {
        throw std::runtime_error("Cloth can only be attached to a CoreObject");
    }

    body = std::make_shared<bezel::Cloth>();

    body->id.atlasId = coreObject->getId();
}

void Cloth::init() {
    if (!body || !object || Window::mainWindow == nullptr ||
        Window::mainWindow->physicsWorld == nullptr) {
        return;
    }

    CoreObject *coreObject = dynamic_cast<CoreObject *>(object);

    if (!coreObject) {
        throw std::runtime_error("Cloth requires a CoreObject");
    }

    body->position = coreObject->getPosition();
    body->rotation = coreObject->getRotation();
    body->rotationQuat = coreObject->getRotation().toGlmQuat();
    body->mass = mass;
    body->material.stretchCompliance = stretchCompliance;
    body->material.shearCompliance = shearCompliance;
    body->material.bendCompliance = bendCompliance;
    body->material.damping = damping;
    body->material.friction = friction;
    body->material.restitution = restitution;
    body->gravityFactor = gravityFactor;
    body->solverIterations = solverIterations;
    body->allowSleeping = allowSleeping;
    body->doubleSided = doubleSided;
    body->vertexRadius = vertexRadius;
    body->bendType = bendType;
    body->setAnchors(anchors);
    body->setObject(coreObject);
    body->createMesh();
    body->create(Window::mainWindow->physicsWorld);

    createdCloth = true;
}

void Cloth::update([[maybe_unused]] float dt) {
    if (!createdCloth || !body || Window::mainWindow == nullptr ||
        Window::mainWindow->physicsWorld == nullptr) {
        return;
    }

    body->updateVertices(Window::mainWindow->physicsWorld);
}

void Cloth::beforePhysics() {
    if (!recreateRequested) {
        return;
    }

    recreate();
}

void Cloth::requestRecreate() {
    if (createdCloth) {
        recreateRequested = true;
    }
}

void Cloth::recreate() {
    if (!body || Window::mainWindow == nullptr ||
        Window::mainWindow->physicsWorld == nullptr) {
        return;
    }

    auto world = Window::mainWindow->physicsWorld;

    body->destroy(world);

    createdCloth = false;

    CoreObject *coreObject = dynamic_cast<CoreObject *>(object);

    if (!coreObject) {
        return;
    }

    body->mass = mass;
    body->material.stretchCompliance = stretchCompliance;
    body->material.shearCompliance = shearCompliance;
    body->material.bendCompliance = bendCompliance;
    body->material.damping = damping;
    body->material.friction = friction;
    body->material.restitution = restitution;
    body->solverIterations = solverIterations;
    body->gravityFactor = gravityFactor;
    body->allowSleeping = allowSleeping;
    body->doubleSided = doubleSided;
    body->vertexRadius = vertexRadius;
    body->bendType = bendType;

    body->setAnchors(anchors);

    body->setObject(coreObject);

    body->createMesh();
    body->create(world);

    createdCloth = true;
    recreateRequested = false;
}

Cloth::~Cloth() {
    if (!body || Window::mainWindow == nullptr ||
        Window::mainWindow->physicsWorld == nullptr) {
        return;
    }

    body->destroy(Window::mainWindow->physicsWorld);
}

std::shared_ptr<Component> Cloth::clone() const {
    auto result = std::make_shared<Cloth>();

    result->mass = mass;
    result->stretchCompliance = stretchCompliance;
    result->shearCompliance = shearCompliance;
    result->bendCompliance = bendCompliance;
    result->damping = damping;
    result->friction = friction;
    result->restitution = restitution;
    result->gravityFactor = gravityFactor;
    result->solverIterations = solverIterations;
    result->allowSleeping = allowSleeping;
    result->doubleSided = doubleSided;
    result->vertexRadius = vertexRadius;
    result->bendType = bendType;
    result->anchors = anchors;

    return result;
}

void Cloth::setMass(float value) {
    mass = value;
    requestRecreate();
}

void Cloth::setStretchCompliance(float value) {
    stretchCompliance = value;
    requestRecreate();
}

void Cloth::setShearCompliance(float value) {
    shearCompliance = value;
    requestRecreate();
}

void Cloth::setBendCompliance(float value) {
    bendCompliance = value;
    requestRecreate();
}

void Cloth::setDamping(float value) {
    damping = value;
    requestRecreate();
}

void Cloth::setFriction(float value) {
    friction = value;
    requestRecreate();
}

void Cloth::setRestitution(float value) {
    restitution = value;
    requestRecreate();
}

void Cloth::setGravityFactor(float value) {
    gravityFactor = value;
    requestRecreate();
}

void Cloth::setSolverIterations(uint32_t value) {

    solverIterations = value;
    requestRecreate();
}

void Cloth::setAllowSleeping(bool value) {
    allowSleeping = value;
    requestRecreate();
}

void Cloth::setDoubleSided(bool value) {
    doubleSided = value;
    requestRecreate();
}

void Cloth::setVertexRadius(float value) {
    vertexRadius = value;
    requestRecreate();
}

void Cloth::setBendType(ClothBendType value) {
    bendType = value;
    requestRecreate();
}

void Cloth::addAnchor(AnchorPoint anchor) {
    if (std::find(anchors.begin(), anchors.end(), anchor) == anchors.end()) {
        anchors.push_back(anchor);
    }

    if (body) {
        body->setAnchors(anchors);
    }

    requestRecreate();
}

void Cloth::setAnchors(const std::vector<AnchorPoint> &newAnchors) {
    anchors = newAnchors;

    if (body) {
        body->setAnchors(anchors);
    }

    requestRecreate();
}

void Cloth::removeAnchor(AnchorPoint anchor) {
    std::erase(anchors, anchor);

    if (body) {
        body->setAnchors(anchors);
    }

    requestRecreate();
}

void Cloth::clearAnchors() {
    anchors.clear();

    if (body) {
        body->clearAnchors();
    }

    requestRecreate();
}

void Cloth::pinVertex(uint32_t index) {
    if (!body) {
        return;
    }

    body->pinVertex(index);

    requestRecreate();
}

void Cloth::unpinVertex(uint32_t index) {
    if (!body) {
        return;
    }

    body->unpinVertex(index);

    requestRecreate();
}

bool Cloth::isCreated() const { return body && body->isCreated(); }