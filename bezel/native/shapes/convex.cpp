/*
 convex.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions for convex shapes
 Copyright (c) 2025 maxvdec
*/

#include "bezel/native/bounds.h"
#include "bezel/native/shape.h"
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

Bounds Convex::getBounds(const glm::vec3 &pos,
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

float Convex::fastestLinearSpeed(const glm::vec3 &angularVelocity,
                                 const glm::vec3 &dir) const {
    float maxSpeed = 0.0f;
    for (const auto &v : vertices) {
        glm::vec3 r = v - centerOfMass;
        glm::vec3 linearVel = glm::cross(angularVelocity, r);
        float speed = glm::dot(linearVel, dir);
        if (speed > maxSpeed) {
            maxSpeed = speed;
        }
    }

    return maxSpeed;
}

void Convex::build(const std::vector<glm::vec3> points) {
    this->vertices = points;

    std::vector<glm::vec3> hullPoints;
    std::vector<Triangle> triangles;
    bezel::buildConvexHull(points, hullPoints, triangles);

    bounds.clear();
    bounds.expand(hullPoints, hullPoints.size());

    centerOfMass = bezel::calculateCenterOfMass(hullPoints, triangles);
    inertiaTensor =
        bezel::calculateInertiaTensor(hullPoints, triangles, centerOfMass);
}

glm::vec3 Convex::support(const glm::vec3 &dir, const glm::vec3 &pos,
                          const glm::quat &orientation, float bias) const {
    glm::vec3 maxPt = orientation * vertices[0] + pos;
    float maxDist = glm::dot(maxPt, dir);
    for (size_t i = 1; i < vertices.size(); i++) {
        glm::vec3 pt = orientation * vertices[i] + pos;
        float dist = glm::dot(pt, dir);
        if (dist > maxDist) {
            maxDist = dist;
            maxPt = pt;
        }
    }

    glm::vec3 norm = dir;
    norm = glm::normalize(norm);
    norm *= bias;

    return maxPt + norm;
}