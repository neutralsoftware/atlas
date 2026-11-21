//
// rigidbody.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Rigidbody Atlas implementation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "atlas/physics.h"
#include "atlas/object.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "bezel/bezel.h"
#include <vector>
#include <iostream>

namespace {

OverlapResult convertOverlapResult(const bezel::OverlapResult &in) {
    OverlapResult out;
    out.hitAny = in.hitAny;
    out.hits.reserve(in.hits.size());

    for (const auto &hit : in.hits) {
        OverlapHit h;
        h.contactPoint = hit.contactPoint;
        h.penetrationAxis = hit.penetrationAxis;
        h.penetrationDepth = hit.penetrationDepth;
        h.rigidbody = hit.rigidbody;

        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            }
        }
        h.object = hitObject;
        out.hits.push_back(h);
    }

    return out;
}

SweepResult convertSweepResult(const bezel::SweepResult &in,
                               const Position3d &endPosition) {
    SweepResult out;
    out.hitAny = in.hitAny;
    out.endPosition = endPosition;
    out.hits.reserve(in.hits.size());

    auto convertHit = [](const bezel::SweepHit &hit) {
        SweepHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.percentage = hit.percentage;
        h.rigidbody = hit.rigidbody;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            }
        }
        h.object = hitObject;
        return h;
    };

    for (const auto &hit : in.hits) {
        out.hits.push_back(convertHit(hit));
    }

    if (in.hitAny) {
        out.closest = convertHit(in.closest);
    }

    return out;
}

bool ensureBodyAndWorld(Rigidbody *rb) {
    return rb && rb->object && rb->body && Window::mainWindow &&
           Window::mainWindow->physicsWorld;
}

} // namespace

void Rigidbody::atAttach() {
    if (!object) {
        atlas_warning("Rigidbody attached without a valid GameObject.");
        return;
    }

    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }

    if (object->rigidbody == nullptr || object->rigidbody == this) {
        object->rigidbody = this;
    } else {
        atlas_warning("GameObject already has a Rigidbody component assigned.");
    }

    body->id.atlasId = object->getId();
}

void Rigidbody::init() {
    if (body && Window::mainWindow && Window::mainWindow->physicsWorld &&
        object) {
        if (!body->collider) {
            atlas_warning(
                "Rigidbody initialization skipped: missing collider.");
            return;
        }

        body->id.atlasId = object->getId();
        body->position = object->getPosition();
        body->rotation = object->getRotation();
        body->isSensor = isSensor;
        body->sensorSignal = sendSignal;
        body->create(Window::mainWindow->physicsWorld);
    } else {
        if (!body) {
            atlas_warning("Rigidbody initialization failed: missing body.");
        } else if (!object) {
            atlas_warning("Rigidbody initialization failed: missing object.");
        } else if (!Window::mainWindow) {
            atlas_warning(
                "Rigidbody initialization failed: missing main window.");
        } else if (!Window::mainWindow->physicsWorld) {
            atlas_warning(
                "Rigidbody initialization failed: missing physics world.");
        } else {
            atlas_warning("Rigidbody initialization failed: missing body or "
                          "physics world.");
        }
    }
}

void Rigidbody::addCapsuleCollider(float radius, float height) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }

    body->setCollider(std::make_shared<bezel::CapsuleCollider>(radius, height));
}

void Rigidbody::addBoxCollider(const Position3d &extents) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->setCollider(std::make_shared<bezel::BoxCollider>(extents / 2.0));
}

void Rigidbody::addSphereCollider(float radius) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
        std::cout
            << "Created NEW bezel::Rigidbody in addSphereCollider, atlasId: "
            << body->id.atlasId << std::endl;
    }
    body->setCollider(std::make_shared<bezel::SphereCollider>(radius));
}

void Rigidbody::addMeshCollider(const Position3d &offset) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }

    if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
        std::vector<Position3d> vertices;
        vertices.reserve(coreObject->getVertices().size());
        for (const auto &vert : coreObject->getVertices()) {
            vertices.emplace_back(vert.position);
        }
        body->setCollider(std::make_shared<bezel::MeshCollider>(
            vertices, coreObject->indices));
        body->collider->offset = offset;
        body->position = object->getPosition();
        body->rotation = object->getRotation();
        body->create(Window::mainWindow->physicsWorld);
    } else {
        atlas_warning(
            "MeshCollider can only be added to CoreObject instances.");
    }
}

void Rigidbody::beforePhysics() {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    if (body->motionType == MotionType::Dynamic) {
        body->applyProperties(Window::mainWindow->physicsWorld);
        return;
    }

    body->setPosition(object->getPosition(), Window::mainWindow->physicsWorld);
    body->setRotation(object->getRotation(), Window::mainWindow->physicsWorld);

    body->applyProperties(Window::mainWindow->physicsWorld);
}

void Rigidbody::update(float dt) {
    (void)dt;
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    body->refresh(Window::mainWindow->physicsWorld);

    if (body->motionType == MotionType::Dynamic) {
        object->setPosition(body->position);
        if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
            coreObject->setRotationQuat(body->rotationQuat);
        } else {
            object->setRotation(body->rotation);
        }
    }
}

std::shared_ptr<Component> Rigidbody::clone() const {
    auto cloned = std::make_shared<Rigidbody>();
    cloned->sendSignal = sendSignal;
    cloned->isSensor = isSensor;

    if (body) {
        cloned->body = std::make_shared<bezel::Rigidbody>(*body);
        cloned->body->id.joltId = bezel::INVALID_JOLT_ID;
        cloned->body->id.atlasId = 0;
    }

    return cloned;
}

void Rigidbody::setFriction(float friction) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->friction = friction;
}

void Rigidbody::setMass(float mass) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->mass = mass;
}

void Rigidbody::setRestitution(float restitution) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->restitution = restitution;
}

void Rigidbody::setMotionType(MotionType motionType) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->motionType = motionType;
}

void Rigidbody::applyForce(const Position3d &force) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->force = force;
}

void Rigidbody::applyForceAtPoint(const Position3d &force,
                                  const Position3d &point) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->force = force;
    body->forcePoint = point;
}

void Rigidbody::applyImpulse(const Position3d &impulse) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->impulse = impulse;
}

void Rigidbody::setLinearVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->linearVelocity = velocity;
}

void Rigidbody::setAngularVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->angularVelocity = velocity;
}

void Rigidbody::addLinearVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->linearVelocity = velocity;
    body->addLinearVelocity = true;
}

void Rigidbody::addAngularVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->angularVelocity = velocity;
    body->addAngularVelocity = true;
}

void Rigidbody::setDamping(float linearDamping, float angularDamping) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->linearDamping = linearDamping;
    body->angularDamping = angularDamping;
}

bool Rigidbody::hasTag(const std::string &tag) const {
    if (!body) {
        return false;
    }
    return std::find(body->tags.begin(), body->tags.end(), tag) !=
           body->tags.end();
}

void Rigidbody::addTag(const std::string &tag) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    if (!hasTag(tag)) {
        body->tags.push_back(tag);
    }
}

void Rigidbody::removeTag(const std::string &tag) const {
    if (!body) {
        return;
    }
    body->tags.erase(std::remove(body->tags.begin(), body->tags.end(), tag),
                     body->tags.end());
}

void Rigidbody::raycast(const Position3d &direction, float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Raycast;
    bezel::RaycastResult raycastResult =
        body->raycast(direction, maxDistance, Window::mainWindow->physicsWorld);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::raycastAll(const Position3d &direction, float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::RaycastAll;
    bezel::RaycastResult raycastResult = body->raycastAll(
        direction, maxDistance, Window::mainWindow->physicsWorld);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::raycastWorld(const Position3d &origin,
                             const Position3d &direction, float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::RaycastWorld;
    bezel::RaycastResult raycastResult =
        Window::mainWindow->physicsWorld->raycast(origin, direction,
                                                  maxDistance);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::raycastWorldAll(const Position3d &origin,
                                const Position3d &direction,
                                float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::RaycastWorldAll;
    bezel::RaycastResult raycastResult =
        Window::mainWindow->physicsWorld->raycastAll(origin, direction,
                                                     maxDistance);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::raycastTagged(const std::vector<std::string> &tags,
                              const Position3d &direction, float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::RaycastTagged;
    bezel::RaycastResult raycastResult =
        body->raycast(direction, maxDistance, Window::mainWindow->physicsWorld);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        if (hit.rigidbody->tags.end() ==
            std::find_first_of(hit.rigidbody->tags.begin(),
                               hit.rigidbody->tags.end(), tags.begin(),
                               tags.end())) {

            for (size_t i = 0; i < hit.rigidbody->tags.size(); ++i) {
                std::cout << "\"" << hit.rigidbody->tags[i] << "\"";
                if (i < hit.rigidbody->tags.size() - 1)
                    std::cout << ", ";
            }
            std::cout << "], required: [";
            for (size_t i = 0; i < tags.size(); ++i) {
                std::cout << "\"" << tags[i] << "\"";
                if (i < tags.size() - 1)
                    std::cout << ", ";
            }
            std::cout << "], skipping." << std::endl;
            continue;
        }
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::raycastTaggedAll(const std::vector<std::string> &tags,
                                 const Position3d &direction,
                                 float maxDistance) {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::RaycastTaggedAll;
    bezel::RaycastResult raycastResult = body->raycastAll(
        direction, maxDistance, Window::mainWindow->physicsWorld);

    result.raycastResult.closestDistance = raycastResult.closestDistance;
    result.raycastResult.hits.reserve(raycastResult.hits.size());
    for (const auto &hit : raycastResult.hits) {
        RaycastHit h;
        h.position = hit.position;
        h.normal = hit.normal;
        h.distance = hit.distance;
        h.rigidbody = hit.rigidbody;
        if (hit.rigidbody->tags.end() ==
            std::find_first_of(hit.rigidbody->tags.begin(),
                               hit.rigidbody->tags.end(), tags.begin(),
                               tags.end())) {
            continue;
        }
        h.didHit = hit.didHit;
        GameObject *hitObject = nullptr;
        if (hit.rigidbody) {
            auto it = atlas::gameObjects.find((int)hit.rigidbody->id.atlasId);
            if (it != atlas::gameObjects.end()) {
                hitObject = it->second;
            } else {
                atlas_error("Rigidbody hit by raycast does not have an "
                            "associated GameObject.");
                continue;
            }
        }
        h.object = hitObject;
        result.raycastResult.hits.push_back(h);
    }
    result.raycastResult.hit = result.raycastResult.hits.empty()
                                   ? RaycastHit{}
                                   : result.raycastResult.hits[0];

    if (object) {
        object->onQueryReceive(result);
    } else {
        atlas_warning("Rigidbody raycast result has no associated GameObject.");
    }
}

void Rigidbody::overlapCapsule(float radius, float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    overlapCapsuleWorld(object->getPosition(), radius, height);
}

void Rigidbody::overlapBox(const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    overlapBoxWorld(object->getPosition(), extents);
}

void Rigidbody::overlapSphere(float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    overlapSphereWorld(object->getPosition(), radius);
}

void Rigidbody::overlap() {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    if (!body->collider) {
        atlas_warning("Rigidbody overlap() called with no collider set.");
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Overlap;

    const Position3d pos = object->getPosition();
    const Rotation3d rot = object->getRotation();

    const bezel::OverlapResult overlapResult = body->overlap(
        Window::mainWindow->physicsWorld, body->collider, pos, rot);

    result.overlapResult = convertOverlapResult(overlapResult);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::overlapCapsuleWorld(const Position3d &position, float radius,
                                    float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Overlap;

    auto collider = std::make_shared<bezel::CapsuleCollider>(radius, height);
    const Rotation3d rot = object ? object->getRotation() : Rotation3d{};
    const bezel::OverlapResult overlapResult = body->overlap(
        Window::mainWindow->physicsWorld, collider, position, rot);

    result.overlapResult = convertOverlapResult(overlapResult);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::overlapBoxWorld(const Position3d &position,
                                const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Overlap;

    auto collider = std::make_shared<bezel::BoxCollider>(extents / 2.0);
    const Rotation3d rot = object ? object->getRotation() : Rotation3d{};
    const bezel::OverlapResult overlapResult = body->overlap(
        Window::mainWindow->physicsWorld, collider, position, rot);

    result.overlapResult = convertOverlapResult(overlapResult);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::overlapSphereWorld(const Position3d &position, float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Overlap;

    auto collider = std::make_shared<bezel::SphereCollider>(radius);
    const Rotation3d rot = object ? object->getRotation() : Rotation3d{};
    const bezel::OverlapResult overlapResult = body->overlap(
        Window::mainWindow->physicsWorld, collider, position, rot);

    result.overlapResult = convertOverlapResult(overlapResult);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementCapsule(const Position3d &endPosition,
                                       float radius, float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementCapsuleWorld(object->getPosition(), endPosition, radius,
                                height);
}

void Rigidbody::predictMovementBox(const Position3d &endPosition,
                                   const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementBoxWorld(object->getPosition(), endPosition, extents);
}

void Rigidbody::predictMovementSphere(const Position3d &endPosition,
                                      float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementSphereWorld(object->getPosition(), endPosition, radius);
}

void Rigidbody::predictMovement(const Position3d &endPosition) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    if (!body->collider) {
        atlas_warning(
            "Rigidbody predictMovement() called with no collider set.");
        return;
    }

    const Position3d startPosition = object->getPosition();
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    QueryResult result;
    result.operation = QueryOperation::Movement;

    const bezel::SweepResult sweepResult = body->sweep(
        Window::mainWindow->physicsWorld, body->collider, direction, actualEnd);
    result.sweepResult = convertSweepResult(sweepResult, actualEnd);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementCapsuleAll(const Position3d &endPosition,
                                          float radius, float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementCapsuleWorldAll(object->getPosition(), endPosition, radius,
                                   height);
}

void Rigidbody::predictMovementBoxAll(const Position3d &endPosition,
                                      const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementBoxWorldAll(object->getPosition(), endPosition, extents);
}

void Rigidbody::predictMovementSphereAll(const Position3d &endPosition,
                                         float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }
    predictMovementSphereWorldAll(object->getPosition(), endPosition, radius);
}

void Rigidbody::predictMovementAll(const Position3d &endPosition) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    if (!body->collider) {
        atlas_warning(
            "Rigidbody predictMovementAll() called with no collider set.");
        return;
    }

    const Position3d startPosition = object->getPosition();
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    QueryResult result;
    result.operation = QueryOperation::MovementAll;

    const bezel::SweepResult sweepResult = body->sweepAll(
        Window::mainWindow->physicsWorld, body->collider, direction, actualEnd);
    result.sweepResult = convertSweepResult(sweepResult, actualEnd);

    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementCapsuleWorld(const Position3d &startPosition,
                                            const Position3d &endPosition,
                                            float radius, float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Movement;

    auto collider = std::make_shared<bezel::CapsuleCollider>(radius, height);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweep(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementBoxWorld(const Position3d &startPosition,
                                        const Position3d &endPosition,
                                        const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Movement;

    auto collider = std::make_shared<bezel::BoxCollider>(extents / 2.0);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweep(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementSphereWorld(const Position3d &startPosition,
                                           const Position3d &endPosition,
                                           float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::Movement;

    auto collider = std::make_shared<bezel::SphereCollider>(radius);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweep(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementCapsuleWorldAll(const Position3d &startPosition,
                                               const Position3d &endPosition,
                                               float radius, float height) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::MovementAll;

    auto collider = std::make_shared<bezel::CapsuleCollider>(radius, height);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweepAll(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementBoxWorldAll(const Position3d &startPosition,
                                           const Position3d &endPosition,
                                           const Position3d &extents) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::MovementAll;

    auto collider = std::make_shared<bezel::BoxCollider>(extents / 2.0);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweepAll(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

void Rigidbody::predictMovementSphereWorldAll(const Position3d &startPosition,
                                              const Position3d &endPosition,
                                              float radius) {
    if (!ensureBodyAndWorld(this)) {
        return;
    }

    QueryResult result;
    result.operation = QueryOperation::MovementAll;

    auto collider = std::make_shared<bezel::SphereCollider>(radius);
    const Position3d direction = endPosition - startPosition;
    Position3d actualEnd = endPosition;

    const bezel::SweepResult sweepResult =
        Window::mainWindow->physicsWorld->sweepAll(
            Window::mainWindow->physicsWorld, collider, startPosition,
            object ? object->getRotation() : Rotation3d{}, direction, actualEnd,
            body ? body->id.joltId : bezel::INVALID_JOLT_ID);

    result.sweepResult = convertSweepResult(sweepResult, actualEnd);
    if (object) {
        object->onQueryReceive(result);
    }
}

Velocity3d Rigidbody::getAngularVelocity() {
    if (!ensureBodyAndWorld(this)) {
        return Velocity3d{};
    }
    return body->getAngularVelocity(Window::mainWindow->physicsWorld);
}

Velocity3d Rigidbody::getVelocity() {
    if (!ensureBodyAndWorld(this)) {
        return Velocity3d{};
    }
    return body->getVelocity(Window::mainWindow->physicsWorld);
}

Velocity3d Rigidbody::getLinearVelocity() {
    if (!ensureBodyAndWorld(this)) {
        return Velocity3d{};
    }
    return body->getLinearVelocity(Window::mainWindow->physicsWorld);
}

void Rigidbody::setMaxLinearVelocity(float maxLinearVelocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->setMaximumLinearVelocity(maxLinearVelocity);
}

void Rigidbody::setMaxAngularVelocity(float maxAngularVelocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
        if (object) {
            body->id.atlasId = object->getId();
        }
    }
    body->setMaximumAngularVelocity(maxAngularVelocity);
}
