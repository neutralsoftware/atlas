/*
 shape.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shape functions
 Copyright (c) 2025 maxvdec
*/

#include "bezel/native/shape.h"
#include "bezel/native/bounds.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

Sphere::Sphere(float radius) : radius(radius) {
    this->centerOfMass = {0.0f, 0.0f, 0.0f};
}

glm::mat3 Sphere::getInertiaTensor() const {
    float coeff = 2.0f * radius * radius / 5.0f;
    return glm::mat3(coeff, 0.0f, 0.0f, 0.0f, coeff, 0.0f, 0.0f, 0.0f, coeff);
}

bool bezel::raySphere(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                      const glm::vec3 &sphereCenter, const float sphereRadius,
                      float &t1, float &t2) {
    glm::vec3 m = sphereCenter - rayOrigin;
    float a = glm::dot(rayDirection, rayDirection);
    float b = glm::dot(m, rayDirection);
    float c = glm::dot(m, m) - sphereRadius * sphereRadius;

    float discriminant = b * b - a * c;
    float invA = 1.0f / a;

    if (discriminant < 0) {
        return false;
    }

    float deltaRoot = std::sqrt(discriminant);
    t1 = invA * (b - deltaRoot);
    t2 = invA * (b + deltaRoot);

    return true;
}

Bounds Sphere::getBounds(const glm::vec3 &pos, const glm::quat &) const {
    Bounds bounds;
    glm::vec3 radiusVec(radius, radius, radius);
    bounds.expand(pos - radiusVec);
    bounds.expand(pos + radiusVec);
    return bounds;
}

Bounds Sphere::getBounds() const {
    Bounds bounds;
    glm::vec3 radiusVec(radius, radius, radius);
    bounds.expand(-radiusVec);
    bounds.expand(radiusVec);
    return bounds;
}

glm::vec3 Sphere::support(const glm::vec3 &dir, const glm::vec3 &pos,
                          const glm::quat &, float bias) const {

    return pos + dir * (radius + bias);
}

Box::Box(std::vector<glm::vec3> points) { build(points); }

void Box::build(const std::vector<glm::vec3> points) {
    if (points.size() < 1) {
        return;
    }

    bounds = Bounds();
    bounds.expand(points, points.size());

    vertices = {
        glm::vec3(bounds.mins.x, bounds.mins.y, bounds.mins.z),
        glm::vec3(bounds.maxs.x, bounds.mins.y, bounds.mins.z),
        glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.mins.z),
        glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.mins.z),
        glm::vec3(bounds.mins.x, bounds.mins.y, bounds.maxs.z),
        glm::vec3(bounds.maxs.x, bounds.mins.y, bounds.maxs.z),
        glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.maxs.z),
        glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.maxs.z),
    };

    centerOfMass = (bounds.mins + bounds.maxs) * 0.5f;
}

glm::mat3 Box::getInertiaTensor() const {
    const float dx = bounds.maxs.x - bounds.mins.x;
    const float dy = bounds.maxs.y - bounds.mins.y;
    const float dz = bounds.maxs.z - bounds.mins.z;

    const float mass = 1.0f;

    glm::mat3 tensor(0.0f);
    tensor[0][0] = mass * (dy * dy + dz * dz) / 12.0f;
    tensor[1][1] = mass * (dx * dx + dz * dz) / 12.0f;
    tensor[2][2] = mass * (dx * dx + dy * dy) / 12.0f;

    glm::vec3 cm;
    cm.x = (bounds.maxs.x + bounds.mins.x) * 0.5f;
    cm.y = (bounds.maxs.y + bounds.mins.y) * 0.5f;
    cm.z = (bounds.maxs.z + bounds.mins.z) * 0.5f;

    const glm::vec3 R = glm::vec3(0.0f) - cm;
    const float R2 = glm::dot(R, R);

    glm::mat3 patTensor(0.0f);
    patTensor[0] = glm::vec3(R2 - R.x * R.x, -R.x * R.y, -R.x * R.z);
    patTensor[1] = glm::vec3(-R.y * R.x, R2 - R.y * R.y, -R.y * R.z);
    patTensor[2] = glm::vec3(-R.z * R.x, -R.z * R.y, R2 - R.z * R.z);

    tensor += mass * patTensor;

    return tensor;
}

glm::vec3 Box::support(const glm::vec3 &dir, const glm::vec3 &pos,
                       const glm::quat &orientation, float bias) const {
    glm::vec3 maxPt = glm::rotate(orientation, vertices[0]) + pos;

    float maxDist = glm::dot(maxPt, dir);

    for (auto &v : vertices) {
        glm::vec3 pt = (orientation * v) + pos;
        float dist = glm::dot(pt, dir);

        if (dist > maxDist) {
            maxDist = dist;
            maxPt = pt;
        }
    }

    if (bias != 0.0f) {
        glm::vec3 norm = glm::normalize(dir);
        maxPt += norm * bias;
    }

    return maxPt;
}

Bounds Box::getBounds(const glm::vec3 &pos,
                      const glm::quat &orientation) const {
    glm::vec3 corners[8];
    corners[0] = glm::vec3(bounds.mins.x, bounds.mins.y, bounds.mins.z);
    corners[1] = glm::vec3(bounds.mins.x, bounds.mins.y, bounds.maxs.z);
    corners[2] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.mins.z);
    corners[3] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.maxs.z);
    corners[4] = glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.maxs.z);
    corners[5] = glm::vec3(bounds.maxs.x, bounds.maxs.y, bounds.mins.z);
    corners[6] = glm::vec3(bounds.maxs.x, bounds.mins.y, bounds.maxs.z);
    corners[7] = glm::vec3(bounds.mins.x, bounds.maxs.y, bounds.maxs.z);

    Bounds bounds;
    for (int i = 0; i < 8; i++) {
        corners[i] = orientation * corners[i] + pos;
        bounds.expand(corners[i]);
    }

    return bounds;
}

Bounds Box::getBounds() const { return bounds; }

float Box::fastestLinearSpeed(const glm::vec3 &angularVelocity,
                              const glm::vec3 &dir) const {
    if (glm::length2(dir) < 1e-12f) {
        return 0.0f;
    }

    glm::vec3 dirNorm = glm::normalize(dir);
    float maxSpeed = 0.0f;
    for (const auto &v : vertices) {
        glm::vec3 r = v - centerOfMass;
        glm::vec3 linearVel = glm::cross(angularVelocity, r);
        float speed = std::abs(glm::dot(linearVel, dirNorm));
        if (speed > maxSpeed) {
            maxSpeed = speed;
        }
    }

    return maxSpeed;
}
