/*
 body.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Body calculations
 Copyright (c) 2025 maxvdec
*/

#include "bezel/native/body.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "bezel/native/bounds.h"
#include "bezel/native/shape.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>

int Contact::compareTo(const Contact other) const {
    if (this->timeOfImpact < other.timeOfImpact) {
        return -1;
    } else if (this->timeOfImpact > other.timeOfImpact) {
        return 1;
    } else {
        return 0;
    }
}

glm::vec3 Body::getCenterOfMassWorldSpace() const {
    return modelSpaceToWorldSpace(this->shape->getCenterOfMass());
}

glm::vec3 Body::getCenterOfMassModelSpace() const {
    return this->shape->getCenterOfMass();
}

glm::vec3 Body::worldSpaceToModelSpace(const glm::vec3 &point) const {
    glm::vec3 translated = point - position.toGlm();
    glm::quat invOrientation = glm::conjugate(orientation);
    return invOrientation * translated;
}

glm::vec3 Body::modelSpaceToWorldSpace(const glm::vec3 &point) const {
    glm::vec3 rotated = orientation * point;
    return rotated + position.toGlm();
}

void Body::update(Window &window) {
    if (thisShared == nullptr) {
        for (const auto &body : window.getAllBodies()) {
            if (body.get() == this) {
                thisShared = body;
                break;
            }
        }
    }

    float dt = window.getDeltaTime();
    dt = std::min(dt, 0.0333f);

    if (isSleeping && invMass == 0.0f) {
        return;
    }

    std::vector<std::shared_ptr<Body>> bodies = window.getAllBodies();

    std::vector<CollisionPair> pairs;
    bezel::broadPhase(bodies, pairs, dt);

    if (isSleeping && invMass > 0.0f) {
        for (const auto &pair : pairs) {
            auto bodyA = bodies[pair.a];
            auto bodyB = bodies[pair.b];
            if ((bodyA.get() == this || bodyB.get() == this)) {
                auto other = (bodyA.get() == this) ? bodyB : bodyA;
                if (glm::length(other->linearVelocity) >
                        SLEEP_LINEAR_THRESHOLD ||
                    glm::length(other->angularVelocity) >
                        SLEEP_ANGULAR_THRESHOLD) {
                    isSleeping = false;
                    sleepTimer = 0.0f;
                    break;
                }
            }
        }
    }

    if (isSleeping) {
        return;
    }

    int numContacts = 0;
    const int maxContacts = bodies.size() * bodies.size();
    std::vector<Contact> contacts(maxContacts);

    for (size_t i = 0; i < pairs.size(); i++) {
        CollisionPair pair = pairs[i];
        std::shared_ptr<Body> bodyA = bodies[pair.a];
        std::shared_ptr<Body> bodyB = bodies[pair.b];

        Contact contact;
        if (intersects(bodyA, bodyB, contact, dt)) {
            std::cout << "Contact between bodies at t=" << contact.timeOfImpact
                      << "s, separation=" << contact.separationDistance
                      << "m\n";
            glm::vec3 velA = bodyA->linearVelocity;
            glm::vec3 velB = bodyB->linearVelocity;
            glm::vec3 relVel = velA - velB;
            float normalVel = glm::dot(relVel, contact.normal);

            bool isPenetrating = contact.separationDistance <= 0.0f;
            bool isApproaching = normalVel < -0.01f;
            bool isNearContact =
                contact.separationDistance < 0.05f; // Within 5cm

            if (isPenetrating || isApproaching || isNearContact) {
                std::cout << "Adding contact: penetration=" << isPenetrating
                          << ", approaching=" << isApproaching
                          << ", near=" << isNearContact << "\n";
                contacts[numContacts] = contact;
                numContacts++;
            }
        }
    }

    if (numContacts >= 1) {
        std::sort(contacts.begin(), contacts.begin() + numContacts,
                  [](const Contact &a, const Contact &b) {
                      return a.timeOfImpact < b.timeOfImpact;
                  });
    }

    bool isResting = false;
    int restingContactCount = 0;

    if (invMass > 0.0f) {
        for (int i = 0; i < numContacts; i++) {
            Contact &contact = contacts[i];
            if (contact.bodyA.get() == this || contact.bodyB.get() == this) {
                glm::vec3 normal = contact.normal;

                bool isSupporting = false;
                const float verticalThreshold = 0.8f;

                if (contact.bodyA.get() == this &&
                    normal.y < -verticalThreshold) {
                    isSupporting = true;
                } else if (contact.bodyB.get() == this &&
                           normal.y > verticalThreshold) {
                    isSupporting = true;
                }

                if (isSupporting) {
                    float speed = glm::length(this->linearVelocity);
                    float angularSpeed = glm::length(this->angularVelocity);

                    bool isStable = contact.separationDistance > -0.02f;
                    bool isSlowEnough = speed < SLEEP_LINEAR_THRESHOLD &&
                                        angularSpeed < SLEEP_ANGULAR_THRESHOLD;

                    if (isStable && isSlowEnough) {
                        isResting = true;
                        restingContactCount++;

                        if (contact.separationDistance < -0.005f) {
                            glm::vec3 correction =
                                -normal * contact.separationDistance * 0.8f;
                            if (contact.bodyA.get() == this) {
                                this->position =
                                    this->position +
                                    Position3d::fromGlm(correction);
                            } else {
                                this->position =
                                    this->position -
                                    Position3d::fromGlm(correction);
                            }
                        }

                        this->linearVelocity *= 0.5f;
                        this->angularVelocity *= 0.5f;

                        if (glm::length2(this->linearVelocity) < 1e-4f) {
                            this->linearVelocity = glm::vec3(0.0f);
                        }
                        if (glm::length2(this->angularVelocity) < 1e-4f) {
                            this->angularVelocity = glm::vec3(0.0f);
                        }
                    }
                }
            }
        }
    }

    if (isResting && restingContactCount > 0) {
        sleepTimer += dt;
        if (sleepTimer > SLEEP_TIME_THRESHOLD) {
            isSleeping = true;
            linearVelocity = glm::vec3(0.0f);
            angularVelocity = glm::vec3(0.0f);
            return;
        }
    } else {
        sleepTimer = 0.0f;
    }

    if (!isResting && invMass > 0.0f) {
        float mass = 1.0 / invMass;
        glm::vec3 impulseGravity =
            glm::vec3(0.0f, -window.gravity, 0.0f) * mass * dt;
        applyLinearImpulse(impulseGravity);
    }

    float accumulatedTime = 0.0f;
    for (int i = 0; i < numContacts; i++) {
        Contact &contact = contacts[i];
        float dtContact = contact.timeOfImpact - accumulatedTime;

        updatePhysics(dtContact);
        resolveContact(contact);
        accumulatedTime += dtContact;
    }

    const float remainingTime = dt - accumulatedTime;
    if (remainingTime > 0.0f) {
        updatePhysics(remainingTime);
    }
}

void Body::applyLinearImpulse(const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }
    linearVelocity += impulse * invMass;
}

void Body::applyAngularImpulse(const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }
    angularVelocity += getInverseInertiaTensorWorldSpace() * impulse;

    const float maxAngularSpeed = 30.0f;
    if (glm::length(angularVelocity) > maxAngularSpeed) {
        angularVelocity = glm::normalize(angularVelocity) * maxAngularSpeed;
    }
}

void Body::applyImpulse(const glm::vec3 &point, const glm::vec3 &impulse) {
    if (invMass == 0.0f) {
        return; // Infinite mass, do nothing
    }

    applyLinearImpulse(impulse);

    glm::vec3 position = getCenterOfMassWorldSpace();
    glm::vec3 r = point - position;
    glm::vec3 angularImpulse = glm::cross(r, impulse);
    applyAngularImpulse(angularImpulse);
}

void Body::resolveContact(Contact &contact) {
    std::shared_ptr<Body> bodyA = contact.bodyA;
    std::shared_ptr<Body> bodyB = contact.bodyB;

    glm::vec3 n = contact.normal;

    float normalLength = glm::length(n);
    if (normalLength < 1e-6f || !std::isfinite(n.x) || !std::isfinite(n.y) ||
        !std::isfinite(n.z)) {
        return;
    }
    n /= normalLength;

    glm::vec3 pointA = contact.pointA.worldSpacePoint;
    glm::vec3 pointB = contact.pointB.worldSpacePoint;

    if (!std::isfinite(pointA.x) || !std::isfinite(pointA.y) ||
        !std::isfinite(pointA.z) || !std::isfinite(pointB.x) ||
        !std::isfinite(pointB.y) || !std::isfinite(pointB.z)) {
        return;
    }

    const float invMassA = bodyA->invMass;
    const float invMassB = bodyB->invMass;
    const float totalInvMass = invMassA + invMassB;

    if (totalInvMass < 1e-8f) {
        return;
    }

    bool isBoxBox = (bodyA->shape->getType() == Shape::ShapeType::Box &&
                     bodyB->shape->getType() == Shape::ShapeType::Box);

    if (isBoxBox) {
        glm::vec3 ra = pointA - bodyA->getCenterOfMassWorldSpace();
        glm::vec3 rb = pointB - bodyB->getCenterOfMassWorldSpace();

        glm::vec3 velA = bodyA->linearVelocity;
        glm::vec3 velB = bodyB->linearVelocity;

        if (invMassA > 0.0f) {
            velA += glm::cross(bodyA->angularVelocity, ra);
        }
        if (invMassB > 0.0f) {
            velB += glm::cross(bodyB->angularVelocity, rb);
        }

        glm::vec3 relativeVel = velA - velB;
        float normalVel = glm::dot(relativeVel, n);

        if (normalVel >= 0.0f) {
            return;
        }

        float restitution = 0.0f;
        float impulseJ = -(1.0f + restitution) * normalVel / totalInvMass;

        glm::vec3 impulse = impulseJ * n;

        if (invMassA > 0.0f) {
            bodyA->linearVelocity += impulse * invMassA;
        }
        if (invMassB > 0.0f) {
            bodyB->linearVelocity -= impulse * invMassB;
        }

        glm::vec3 tangent = relativeVel - normalVel * n;
        float tangentLength = glm::length(tangent);

        if (tangentLength > 1e-6f) {
            tangent /= tangentLength;

            float frictionCoeff = std::sqrt(bodyA->friction * bodyB->friction);
            float maxFriction = frictionCoeff * impulseJ;
            float jt = -glm::dot(relativeVel, tangent) / totalInvMass;
            jt = glm::clamp(jt, -maxFriction, maxFriction);
            glm::vec3 frictionImpulse = jt * tangent;

            if (invMassA > 0.0f) {
                bodyA->linearVelocity -= frictionImpulse * invMassA;
            }
            if (invMassB > 0.0f) {
                bodyB->linearVelocity += frictionImpulse * invMassB;
            }
        }

        return;
    }

    const float elasticityA = bodyA->elasticity;
    const float elasticityB = bodyB->elasticity;
    const float elasticity = elasticityA * elasticityB;

    glm::mat3 invWorldInertiaA = bodyA->getInverseInertiaTensorWorldSpace();
    glm::mat3 invWorldInertiaB = bodyB->getInverseInertiaTensorWorldSpace();

    glm::vec3 ra = pointA - bodyA->getCenterOfMassWorldSpace();
    glm::vec3 rb = pointB - bodyB->getCenterOfMassWorldSpace();

    glm::vec3 velA = bodyA->linearVelocity;
    glm::vec3 velB = bodyB->linearVelocity;

    if (invMassA > 0.0f) {
        velA += glm::cross(bodyA->angularVelocity, ra);
    }
    if (invMassB > 0.0f) {
        velB += glm::cross(bodyB->angularVelocity, rb);
    }

    glm::vec3 relativeVel = velA - velB;
    float normalVel = glm::dot(relativeVel, n);

    float penetrationDepth = -contact.separationDistance;
    float baumgarte = 0.0f;

    if (penetrationDepth > 0.0f) {
        const float slop = 0.0005f;
        const float maxCorrection = 0.2f;

        float correctionDepth = std::max(0.0f, penetrationDepth - slop);
        correctionDepth = std::min(correctionDepth, maxCorrection);

        float baumgarteCoeff = 0.4f;
        baumgarte = baumgarteCoeff * correctionDepth;
    }

    const float minRelativeVel = -0.0001f;
    if (normalVel >= minRelativeVel && penetrationDepth <= 0.001f) {
        return;
    }

    glm::vec3 angularJA = glm::vec3(0.0f);
    glm::vec3 angularJB = glm::vec3(0.0f);

    if (invMassA > 0.0f) {
        angularJA = glm::cross((invWorldInertiaA * glm::cross(ra, n)), ra);
    }
    if (invMassB > 0.0f) {
        angularJB = glm::cross((invWorldInertiaB * glm::cross(rb, n)), rb);
    }

    const float angularFactor = glm::dot(angularJA + angularJB, n);
    const float denominator = invMassA + invMassB + angularFactor;

    if (!std::isfinite(denominator) || std::abs(denominator) < 1e-8f) {
        return;
    }

    float impulseJ = -(1.0f + elasticity) * normalVel / denominator;

    if (penetrationDepth > 0.0f) {
        impulseJ += baumgarte / denominator;
    }

    const float maxImpulse = 1000.0f;
    impulseJ = glm::clamp(impulseJ, 0.0f, maxImpulse);

    if (!std::isfinite(impulseJ) || impulseJ < 1e-8f) {
        return;
    }

    glm::vec3 normalImpulse = impulseJ * n;

    bodyA->applyImpulse(pointA, normalImpulse);
    bodyB->applyImpulse(pointB, -normalImpulse);

    glm::vec3 newVelA = bodyA->linearVelocity;
    glm::vec3 newVelB = bodyB->linearVelocity;

    if (invMassA > 0.0f) {
        newVelA += glm::cross(bodyA->angularVelocity, ra);
    }
    if (invMassB > 0.0f) {
        newVelB += glm::cross(bodyB->angularVelocity, rb);
    }

    glm::vec3 newRelativeVel = newVelA - newVelB;
    glm::vec3 tangentialVel = newRelativeVel - glm::dot(newRelativeVel, n) * n;
    float tangentialSpeed = glm::length(tangentialVel);

    if (tangentialSpeed > 1e-6f) {
        glm::vec3 frictionDir = tangentialVel / tangentialSpeed;

        float frictionA = bodyA->friction;
        float frictionB = bodyB->friction;
        float combinedFriction = std::sqrt(frictionA * frictionB);

        glm::vec3 frictionAngularJA = glm::vec3(0.0f);
        glm::vec3 frictionAngularJB = glm::vec3(0.0f);

        if (invMassA > 0.0f) {
            frictionAngularJA = glm::cross(
                (invWorldInertiaA * glm::cross(ra, frictionDir)), ra);
        }
        if (invMassB > 0.0f) {
            frictionAngularJB = glm::cross(
                (invWorldInertiaB * glm::cross(rb, frictionDir)), rb);
        }

        float frictionAngularFactor =
            glm::dot(frictionAngularJA + frictionAngularJB, frictionDir);
        float frictionDenominator = invMassA + invMassB + frictionAngularFactor;

        if (std::abs(frictionDenominator) > 1e-8f) {
            float frictionImpulseJ = -tangentialSpeed / frictionDenominator;
            float maxFrictionImpulse = combinedFriction * impulseJ;

            frictionImpulseJ = glm::clamp(frictionImpulseJ, -maxFrictionImpulse,
                                          maxFrictionImpulse);

            glm::vec3 frictionImpulse = frictionImpulseJ * frictionDir;

            if (std::isfinite(frictionImpulse.x) &&
                std::isfinite(frictionImpulse.y) &&
                std::isfinite(frictionImpulse.z)) {
                bodyA->applyImpulse(pointA, frictionImpulse);
                bodyB->applyImpulse(pointB, -frictionImpulse);
            }
        }
    }

    if (penetrationDepth > 0.005f) {
        float correctionAmount = penetrationDepth * 0.5f;
        glm::vec3 correction = n * correctionAmount;

        if (invMassA > 0.0f && invMassB > 0.0f) {
            float ratioA = invMassA / totalInvMass;
            float ratioB = invMassB / totalInvMass;
            bodyA->position =
                bodyA->position - Position3d::fromGlm(correction * ratioA);
            bodyB->position =
                bodyB->position + Position3d::fromGlm(correction * ratioB);
        } else if (invMassA > 0.0f) {
            bodyA->position = bodyA->position - Position3d::fromGlm(correction);
        } else if (invMassB > 0.0f) {
            bodyB->position = bodyB->position + Position3d::fromGlm(correction);
        }
    }
}
glm::mat3 Body::getInverseInertiaTensorBodySpace() const {
    glm::mat3 inertiaTensor = shape->getInertiaTensor();
    glm::mat3 invInertiaTensor = glm::inverse(inertiaTensor) * invMass;
    return invInertiaTensor;
}

glm::mat3 Body::getInverseInertiaTensorWorldSpace() const {
    if (invMass == 0.0f) {
        return glm::mat3(0.0f);
    }

    glm::mat3 inertiaTensor = shape->getInertiaTensor();

    glm::mat3 invInertiaTensor = glm::inverse(inertiaTensor) * invMass;

    glm::mat3 rotationMatrix = glm::mat3_cast(orientation);
    glm::mat3 result =
        rotationMatrix * invInertiaTensor * glm::transpose(rotationMatrix);

    return result;
}

void Body::updatePhysics(double dt) {
    if (invMass == 0.0f) {
        return;
    }

    glm::vec3 pos = this->position.toGlm() + linearVelocity * (float)dt;
    this->position = Position3d::fromGlm(pos);

    if (glm::length2(angularVelocity) > 1e-12f) {
        glm::mat3 orientation = glm::mat3_cast(this->orientation);

        glm::mat3 inertiaTensor = orientation *
                                  this->shape->getInertiaTensor() *
                                  glm::transpose(orientation);

        float det = glm::determinant(inertiaTensor);
        if (std::abs(det) > 1e-12f) {
            glm::vec3 L = inertiaTensor * angularVelocity;
            glm::vec3 torque = glm::cross(angularVelocity, L);
            glm::vec3 alpha = glm::inverse(inertiaTensor) * torque;

            angularVelocity += alpha * (float)dt;

            const float maxAngularSpeed = 30.0f;
            if (glm::length2(angularVelocity) >
                maxAngularSpeed * maxAngularSpeed) {
                angularVelocity =
                    glm::normalize(angularVelocity) * maxAngularSpeed;
            }
        }

        glm::vec3 dAngle = angularVelocity * (float)dt;
        float angle = glm::length(dAngle);

        if (angle > 1e-8f) {
            glm::vec3 axis = dAngle / angle;
            glm::quat dq = glm::angleAxis(angle, axis);

            if (glm::length2(glm::vec4(dq.x, dq.y, dq.z, dq.w)) > 1e-12f &&
                glm::length2(glm::vec4(this->orientation.x, this->orientation.y,
                                       this->orientation.z,
                                       this->orientation.w)) > 1e-12f) {

                this->orientation = glm::normalize(dq * this->orientation);

                if (!std::isfinite(this->orientation.x) ||
                    !std::isfinite(this->orientation.y) ||
                    !std::isfinite(this->orientation.z) ||
                    !std::isfinite(this->orientation.w)) {
                    this->orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    angularVelocity = glm::vec3(0.0f);
                }
            }
        }
    }

    glm::vec3 finalPos = this->position.toGlm();
    if (!std::isfinite(finalPos.x) || !std::isfinite(finalPos.y) ||
        !std::isfinite(finalPos.z)) {
        std::cerr << "ERROR: Invalid position detected! Resetting to origin."
                  << std::endl;
        this->position = Position3d::fromGlm(glm::vec3(0.0f, 5.0f, 0.0f));
        this->linearVelocity = glm::vec3(0.0f);
        this->angularVelocity = glm::vec3(0.0f);
    }
}

Point bezel::support(const std::shared_ptr<Body> bodyA,
                     const std::shared_ptr<Body> bodyB, glm::vec3 dir,
                     float bias) {
    dir = glm::normalize(dir);

    Point point;

    point.ptA = bodyA->shape->support(dir, bodyA->position.toGlm(),
                                      bodyA->orientation, bias);

    dir *= -1.0f;

    point.ptB = bodyB->shape->support(dir, bodyB->position.toGlm(),
                                      bodyB->orientation, bias);

    point.xyz = point.ptA - point.ptB;
    return point;
}