//
// gjk.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: GJK algorithm implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/native/body.h"
#include "bezel/native/shape.h"
#include <array>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

bool bezel::simplexSignedVolumes(const std::vector<Point> &simplex,
                                 glm::vec3 &newDir, glm::vec4 &lambdasOut) {
    const float epsilonf = 1e-6f;
    lambdasOut = glm::vec4(0.0f);

    bool doesIntersect = false;
    switch (simplex.size()) {
    default:
    case 2: {
        glm::vec2 lambdas = bezel::projectOn1D(simplex[0].xyz, simplex[1].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 2; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
    } break;
    case 3: {
        glm::vec3 lambdas =
            bezel::projectOn2D(simplex[0].xyz, simplex[1].xyz, simplex[2].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 3; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
        lambdasOut[2] = lambdas[2];
    } break;
    case 4: {
        glm::vec4 lambdas = bezel::projectOn3D(simplex[0].xyz, simplex[1].xyz,
                                               simplex[2].xyz, simplex[3].xyz);
        glm::vec3 v(0.0f);
        for (int i = 0; i < 4; i++) {
            v += simplex[i].xyz * lambdas[i];
        }
        newDir = -v;
        doesIntersect = glm::length2(newDir) < epsilonf;
        lambdasOut[0] = lambdas[0];
        lambdasOut[1] = lambdas[1];
        lambdasOut[2] = lambdas[2];
        lambdasOut[3] = lambdas[3];
    } break;
    }

    return doesIntersect;
}

bool bezel::hasPoint(const std::array<Point, 4> &simplex, const Point &p) {
    const float precision = 1e-6f;

    for (const auto &sp : simplex) {
        glm::vec3 delta = sp.xyz - p.xyz;
        if (glm::length2(delta) < precision * precision) {
            return true;
        }
    }
    return false;
}

void bezel::sortValids(std::array<Point, 4> &simplex, glm::vec4 &lambdas) {
    const float epsilon = 1e-8f;

    bool valids[4];
    for (int i = 0; i < 4; i++) {
        valids[i] = (std::abs(lambdas[i]) > epsilon);
    }

    glm::vec4 validLambdas(0.0f);
    int validCount = 0;
    std::array<Point, 4> validPoints;

    for (int i = 0; i < 4; i++) {
        if (valids[i]) {
            validPoints[validCount] = simplex[i];
            validLambdas[validCount] = lambdas[i];
            validCount++;
        }
    }

    for (int i = 0; i < 4; i++) {
        simplex[i] = Point{};
        lambdas[i] = 0.0f;
    }

    for (int i = 0; i < validCount; i++) {
        simplex[i] = validPoints[i];
        lambdas[i] = validLambdas[i];
    }
}

int bezel::numValids(const glm::vec4 &lambdas) {
    const float epsilon = 1e-8f;
    int num = 0;
    for (int i = 0; i < 4; i++) {
        if (std::abs(lambdas[i]) > epsilon) {
            num++;
        }
    }
    return num;
}

bool bezel::gjkIntersection(const std::shared_ptr<Body> bodyA,
                            const std::shared_ptr<Body> bodyB, const float bias,
                            glm::vec3 &ptOnA, glm::vec3 &ptOnB,
                            glm::vec3 &normalOut, float &penetrationDepthOut) {

    const glm::vec3 origin(0.0);
    int numPts = 1;
    std::array<Point, 4> simplex;
    simplex[0] =
        bezel::support(bodyA, bodyB, glm::vec3(1.0f, 0.0f, 0.0f), 0.0f);

    bool doesContainOrigin = false;
    glm::vec3 newDir = -simplex[0].xyz;

    int maxIterations = 64;
    for (int iter = 0; iter < maxIterations && !doesContainOrigin; iter++) {
        Point newPt = bezel::support(bodyA, bodyB, newDir, 0.0f);

        if (hasPoint(simplex, newPt)) {
            break;
        }

        simplex[numPts] = newPt;
        numPts++;

        float dotdot = glm::dot(newDir, newPt.xyz - origin);
        if (dotdot < 0.0f) {
            break;
        }

        glm::vec4 lambdas;
        doesContainOrigin = bezel::simplexSignedVolumes(
            std::vector<Point>(simplex.begin(), simplex.begin() + numPts),
            newDir, lambdas);

        if (doesContainOrigin) {
            break;
        }

        bezel::sortValids(simplex, lambdas);
        numPts = bezel::numValids(lambdas);

        if (numPts == 0) {
            break;
        }
    }

    if (!doesContainOrigin) {
        return false;
    }

    while (numPts < 4) {
        glm::vec3 searchDir;

        if (numPts == 1) {
            searchDir = -simplex[0].xyz;
        } else if (numPts == 2) {
            glm::vec3 ab = simplex[1].xyz - simplex[0].xyz;
            glm::vec3 temp = (std::abs(ab.x) < 0.9f) ? glm::vec3(1, 0, 0)
                                                     : glm::vec3(0, 1, 0);
            searchDir = glm::normalize(glm::cross(ab, temp));
        } else if (numPts == 3) {
            glm::vec3 ab = simplex[1].xyz - simplex[0].xyz;
            glm::vec3 ac = simplex[2].xyz - simplex[0].xyz;
            searchDir = glm::normalize(glm::cross(ab, ac));
        }

        Point newPt = bezel::support(bodyA, bodyB, searchDir, 0.0f);

        if (hasPoint(simplex, newPt)) {
            newPt = bezel::support(bodyA, bodyB, -searchDir, 0.0f);
            if (hasPoint(simplex, newPt)) {
                break;
            }
        }

        simplex[numPts] = newPt;
        numPts++;
    }

    if (numPts == 4) {
        glm::vec3 epaNormal(0.0f);
        float penetrationDepth = bezel::epaExpand(bodyA, bodyB, bias, simplex,
                                                  ptOnA, ptOnB, epaNormal);

        if (glm::length2(epaNormal) > 1e-12f) {
            normalOut = glm::normalize(epaNormal);
        } else {
            normalOut = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        penetrationDepthOut = penetrationDepth;
        return true;
    }

    ptOnA = simplex[0].ptA;
    ptOnB = simplex[0].ptB;
    glm::vec3 delta = ptOnB - ptOnA;
    float depth = glm::length(delta);
    if (depth > 1e-6f) {
        normalOut = delta / depth;
        penetrationDepthOut = depth;
    } else {
        glm::vec3 centerDelta = bodyB->getCenterOfMassWorldSpace() -
                                bodyA->getCenterOfMassWorldSpace();
        if (glm::length2(centerDelta) < 1e-12f) {
            normalOut = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            normalOut = glm::normalize(centerDelta);
        }
        penetrationDepthOut = 0.0f;
    }
    return true;
}

void bezel::gjkClosestPoints(const std::shared_ptr<Body> bodyA,
                             const std::shared_ptr<Body> bodyB,
                             glm::vec3 &ptOnA, glm::vec3 &ptOnB) {
    float closestDist = 1e10f;
    const float bias = 0.0f;

    int numPts = 1;
    std::array<Point, 4> simplex;
    simplex[0] =
        bezel::support(bodyA, bodyB, glm::vec3(1.0f, 0.0f, 0.0f), bias);

    glm::vec4 lambdas = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 newDir = -simplex[0].xyz;

    while (numPts < 4) {
        Point newPt = bezel::support(bodyA, bodyB, newDir, bias);

        if (hasPoint(simplex, newPt)) {
            break;
        }

        simplex[numPts] = newPt;
        numPts++;

        bezel::simplexSignedVolumes(
            std::vector<Point>(simplex.begin(), simplex.begin() + numPts),
            newDir, lambdas);
        bezel::sortValids(simplex, lambdas);
        numPts = bezel::numValids(lambdas);

        float dist = glm::length2(newDir);
        if (dist >= closestDist) {
            break;
        }
        closestDist = dist;
    }

    ptOnA = glm::vec3(0.0f);
    ptOnB = glm::vec3(0.0f);
    for (int i = 0; i < numPts; i++) {
        ptOnA += simplex[i].ptA * lambdas[i];
        ptOnB += simplex[i].ptB * lambdas[i];
    }
}