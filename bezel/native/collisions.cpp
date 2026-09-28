//
// collisions.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Collision detection functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/native/body.h"
#include "bezel/native/shape.h"
#include <algorithm>
#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>

bool bezel::collisions::sphereToSphereStatic(
    const Sphere *sphereA, const Sphere *sphereB, const glm::vec3 &posA,
    const glm::vec3 &posB, glm::vec3 &pointOnA, glm::vec3 &pointOnB) {

    const glm::vec3 ab = posB - posA;
    glm::vec3 norm = glm::normalize(ab);

    pointOnA = posA + norm * sphereA->radius;
    pointOnB = posB - norm * sphereB->radius;

    const float radiusAB = sphereA->radius + sphereB->radius;
    const float lengthSquare = glm::length2(ab);
    if (lengthSquare <= (radiusAB * radiusAB)) {
        return true;
    }

    return false;
}

bool bezel::sphereToSphereDynamic(const Sphere *sphereA, const Sphere *sphereB,
                                  const glm::vec3 &posA, const glm::vec3 &posB,
                                  const glm::vec3 &velA, const glm::vec3 &velB,
                                  const float dt, glm::vec3 &pointOnA,
                                  glm::vec3 &pointOnB, float &toi) {
    glm::vec3 relativeVelocity = velA - velB;

    const glm::vec3 startPointA = posA;
    const glm::vec3 endPointA = posA + relativeVelocity * dt;
    glm::vec3 rayDir = endPointA - startPointA;

    float t0 = 0;
    float t1 = 0;
    if (glm::length2(rayDir) < 1e-8f) {
        glm::vec3 ab = posB - posA;
        float radius = sphereA->radius + sphereB->radius + 0.001f;
        if (glm::length2(ab) > radius * radius) {
            return false;
        }
    } else if (!raySphere(posA, rayDir, posB, sphereA->radius + sphereB->radius,
                          t0, t1)) {
        return false;
    }

    t0 *= dt;
    t1 *= dt;

    if (t1 < 0.0f) {
        return false;
    }

    toi = (t0 < 0.0f) ? 0.0f : t0;

    if (toi > dt) {
        return false;
    }

    glm::vec3 newPosA = posA + velA * toi;
    glm::vec3 newPosB = posB + velB * toi;
    glm::vec3 ab = newPosB - newPosA;
    ab = glm::normalize(ab);

    pointOnA = newPosA + ab * sphereA->radius;
    pointOnB = newPosB - ab * sphereB->radius;
    return true;
}

bool bezel::collisions::conservativeAdvance(std::shared_ptr<Body> bodyA,
                                            std::shared_ptr<Body> bodyB,
                                            float dt, Contact &contact) {
    contact.bodyA = bodyA;
    contact.bodyB = bodyB;

    float toi = 0.0f;

    int numIters = 0;

    while (dt > 0.0f) {
        bool didIntersect = Body::intersectsStatic(bodyA, bodyB, contact);
        if (didIntersect) {
            contact.timeOfImpact = toi;
            bodyA->updatePhysics(-toi);
            bodyB->updatePhysics(-toi);
            return true;
        }

        numIters++;
        if (numIters > 10) {
            break;
        }

        glm::vec3 ab =
            contact.pointB.worldSpacePoint - contact.pointA.worldSpacePoint;
        ab = glm::normalize(ab);

        glm::vec3 relativeVel = bodyA->linearVelocity - bodyB->linearVelocity;
        float orthoSpeed = glm::dot(relativeVel, ab);

        float angularSpeedA =
            bodyA->shape->fastestLinearSpeed(bodyA->angularVelocity, ab);
        float angularSpeedB =
            bodyB->shape->fastestLinearSpeed(bodyB->angularVelocity, -ab);
        orthoSpeed += angularSpeedA + angularSpeedB;
        if (orthoSpeed <= 0.0f) {
            break;
        }

        float timeToGo = contact.separationDistance / orthoSpeed;
        if (timeToGo > dt) {
            break;
        }

        dt -= timeToGo;
        toi += timeToGo;

        bodyA->updatePhysics(timeToGo);
        bodyB->updatePhysics(timeToGo);
    }

    bodyA->updatePhysics(-toi);
    bodyB->updatePhysics(-toi);
    return false;
}

bool Body::intersects(const std::shared_ptr<Body> &body,
                      const std::shared_ptr<Body> &other, Contact &contact,
                      float dt) {
    if (body->shape == nullptr || other->shape == nullptr) {
        return false;
    }
    contact.bodyA = body;
    contact.bodyB = other;

    if (body->shape->getType() == Shape::ShapeType::Sphere &&
        other->shape->getType() == Shape::ShapeType::Sphere) {
        const Sphere *sphereA = dynamic_cast<const Sphere *>(body->shape.get());
        const Sphere *sphereB =
            dynamic_cast<const Sphere *>(other->shape.get());

        glm::vec3 posA = body->position.toGlm();
        glm::vec3 posB = other->position.toGlm();

        glm::vec3 velA = body->linearVelocity;
        glm::vec3 velB = other->linearVelocity;

        if (bezel::sphereToSphereDynamic(
                sphereA, sphereB, posA, posB, velA, velB, dt,
                contact.pointA.worldSpacePoint, contact.pointB.worldSpacePoint,
                contact.timeOfImpact)) {
            body->updatePhysics(contact.timeOfImpact);
            other->updatePhysics(contact.timeOfImpact);

            contact.pointA.modelSpacePoint =
                body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
            contact.pointB.modelSpacePoint =
                other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

            glm::vec3 normalVec =
                other->position.toGlm() - body->position.toGlm();
            if (glm::length2(normalVec) < 1e-8f) {
                contact.normal = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                contact.normal = glm::normalize(normalVec);
            }

            glm::vec3 centerDelta = body->getCenterOfMassWorldSpace() -
                                    other->getCenterOfMassWorldSpace();
            if (glm::dot(contact.normal, centerDelta) < 0.0f) {
                contact.normal = -contact.normal;
            }

            body->updatePhysics(-contact.timeOfImpact);
            other->updatePhysics(-contact.timeOfImpact);

            glm::vec3 ab =
                contact.pointB.worldSpacePoint - contact.pointA.worldSpacePoint;
            float r = glm::length(ab) - (sphereA->radius + sphereB->radius);
            contact.separationDistance = r;
            return true;
        }
    } else {
        bool result =
            bezel::collisions::conservativeAdvance(body, other, dt, contact);
        return result;
    }

    return false;
}

bool Body::intersectsStatic(const std::shared_ptr<Body> &body,
                            const std::shared_ptr<Body> &other,
                            Contact &contact) {
    contact.bodyA = body;
    contact.bodyB = other;
    contact.timeOfImpact = 0.0f;

    if (body->shape->getType() == Shape::ShapeType::Sphere &&
        other->shape->getType() == Shape::ShapeType::Sphere) {
        auto sphereA = dynamic_cast<Sphere *>(body->shape.get());
        auto sphereB = dynamic_cast<Sphere *>(other->shape.get());
        glm::vec3 posA = body->position.toGlm();
        glm::vec3 posB = other->position.toGlm();

        if (bezel::collisions::sphereToSphereStatic(
                sphereA, sphereB, posA, posB, contact.pointA.worldSpacePoint,
                contact.pointB.worldSpacePoint)) {
            contact.normal = glm::normalize(posB - posA);

            glm::vec3 centerDelta = body->getCenterOfMassWorldSpace() -
                                    other->getCenterOfMassWorldSpace();
            if (glm::dot(contact.normal, centerDelta) < 0.0f) {
                contact.normal = -contact.normal;
            }
            contact.pointA.modelSpacePoint =
                body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
            contact.pointB.modelSpacePoint =
                other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);
            glm::vec3 ab =
                contact.pointB.worldSpacePoint - contact.pointA.worldSpacePoint;
            float r = glm::length(ab) - (sphereA->radius + sphereB->radius);
            contact.separationDistance = r;
            return true;
        }
        return false;
    }

    glm::vec3 ptOnA;
    glm::vec3 ptOnB;
    const float bias = 0.001f;

    bool isBoxInvolved = (body->shape->getType() == Shape::ShapeType::Box ||
                          other->shape->getType() == Shape::ShapeType::Box);

    glm::vec3 gjkNormal(0.0f);
    float penetrationDepth = 0.0f;

    if (bezel::gjkIntersection(body, other, bias, ptOnA, ptOnB, gjkNormal,
                               penetrationDepth)) {
        glm::vec3 normal = gjkNormal;
        if (glm::length2(normal) < 1e-12f) {
            glm::vec3 centerDelta = other->getCenterOfMassWorldSpace() -
                                    body->getCenterOfMassWorldSpace();
            if (glm::length2(centerDelta) < 1e-12f) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(centerDelta);
            }
        } else {
            normal = glm::normalize(normal);
        }

        // if (isBoxInvolved) {
        //     float absX = std::abs(normal.x);
        //     float absY = std::abs(normal.y);
        //     float absZ = std::abs(normal.z);

        //     const float snapThreshold = 0.95f;

        //     if (absY > snapThreshold && absY > absX && absY > absZ) {
        //         normal = glm::vec3(0.0f, normal.y > 0 ? 1.0f : -1.0f, 0.0f);
        //     } else if (absX > snapThreshold && absX > absY && absX > absZ) {
        //         normal = glm::vec3(normal.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
        //     } else if (absZ > snapThreshold && absZ > absX && absZ > absY) {
        //         normal = glm::vec3(0.0f, 0.0f, normal.z > 0 ? 1.0f : -1.0f);
        //     }
        // }

        float depthOnNormal = std::abs(glm::dot(ptOnB - ptOnA, normal));
        penetrationDepth = std::max(penetrationDepth, depthOnNormal);

        contact.normal = normal;
        contact.pointA.worldSpacePoint = ptOnA;
        contact.pointB.worldSpacePoint = ptOnB;
        contact.pointA.modelSpacePoint =
            body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
        contact.pointB.modelSpacePoint =
            other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

        contact.separationDistance = -penetrationDepth;

        glm::vec3 centerDelta = body->getCenterOfMassWorldSpace() -
                                other->getCenterOfMassWorldSpace();
        if (glm::dot(contact.normal, centerDelta) < 0.0f) {
            contact.normal = -contact.normal;
        }

        return true;
    }

    bezel::gjkClosestPoints(body, other, ptOnA, ptOnB);
    contact.pointA.worldSpacePoint = ptOnA;
    contact.pointB.worldSpacePoint = ptOnB;
    contact.pointA.modelSpacePoint =
        body->worldSpaceToModelSpace(contact.pointA.worldSpacePoint);
    contact.pointB.modelSpacePoint =
        other->worldSpaceToModelSpace(contact.pointB.worldSpacePoint);

    glm::vec3 ab = ptOnB - ptOnA;
    float distance = glm::length(ab);

    if (distance > 1e-8f) {
        glm::vec3 normal = ab / distance;

        if (isBoxInvolved) {
            float absX = std::abs(normal.x);
            float absY = std::abs(normal.y);
            float absZ = std::abs(normal.z);
            const float snapThreshold = 0.95f;

            if (absY > snapThreshold && absY > absX && absY > absZ) {
                normal = glm::vec3(0.0f, normal.y > 0 ? 1.0f : -1.0f, 0.0f);
            } else if (absX > snapThreshold && absX > absY && absX > absZ) {
                normal = glm::vec3(normal.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
            } else if (absZ > snapThreshold && absZ > absX && absZ > absY) {
                normal = glm::vec3(0.0f, 0.0f, normal.z > 0 ? 1.0f : -1.0f);
            }
        }

        contact.normal = normal;
    } else {
        glm::vec3 centerA = body->getCenterOfMassWorldSpace();
        glm::vec3 centerB = other->getCenterOfMassWorldSpace();
        glm::vec3 dir = centerB - centerA;
        contact.normal = glm::length(dir) > 1e-8f ? glm::normalize(dir)
                                                  : glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 centerDelta =
        body->getCenterOfMassWorldSpace() - other->getCenterOfMassWorldSpace();
    if (glm::dot(contact.normal, centerDelta) < 0.0f) {
        contact.normal = -contact.normal;
    }

    contact.separationDistance = distance;
    return false;
}