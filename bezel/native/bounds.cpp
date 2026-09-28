/*
 bounds.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Bound declarations
 Copyright (c) 2025 maxvdec
*/

#include "bezel/native/body.h"
#include "bezel/native/bounds.h"
#include "bezel/native/shape.h"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <vector>

Bounds::Bounds() { clear(); }

Bounds &Bounds::operator=(const Bounds &rhs) {
    if (this != &rhs) {
        mins = rhs.mins;
        maxs = rhs.maxs;
    }
    return *this;
}

void Bounds::clear() {
    mins = glm::vec3(std::numeric_limits<float>::max());
    maxs = glm::vec3(std::numeric_limits<float>::lowest());
}

bool Bounds::doesIntersect(const Bounds &other) const {
    if (maxs.x < other.mins.x || maxs.y < other.mins.y ||
        maxs.z < other.mins.z) {
        return false;
    }
    if (mins.x > other.maxs.x || mins.y > other.maxs.y ||
        mins.z > other.maxs.z) {
        return false;
    }
    return true;
}

void Bounds::expand(std::vector<glm::vec3> pts, const int number) {
    for (int i = 0; i < number; ++i) {
        expand(pts[i]);
    }
}

void Bounds::expand(const glm::vec3 rhs) {
    mins.x = std::min(mins.x, rhs.x);
    mins.y = std::min(mins.y, rhs.y);
    mins.z = std::min(mins.z, rhs.z);

    maxs.x = std::max(maxs.x, rhs.x);
    maxs.y = std::max(maxs.y, rhs.y);
    maxs.z = std::max(maxs.z, rhs.z);
}

void Bounds::expand(const Bounds &rhs) {
    expand(rhs.mins);
    expand(rhs.maxs);
}

int bezel::compareSAP(void *a, void *b) {
    PseudoBody *pa = (PseudoBody *)a;
    PseudoBody *pb = (PseudoBody *)b;
    if (pa->value < pb->value) {
        return -1;
    }
    return 1;
}

void bezel::sortBodiesForBounds(std::vector<std::shared_ptr<Body>> bodies,
                                std::vector<PseudoBody> &sortedArray,
                                float dt) {
    glm::vec3 axis = glm::vec3(1.0f, 1.0f, 1.0f);
    axis = glm::normalize(axis);

    for (size_t i = 0; i < bodies.size(); i++) {
        const std::shared_ptr<Body> body = bodies[i];
        Bounds bounds =
            body->shape->getBounds(body->position.toGlm(), body->orientation);

        bounds.expand(bounds.mins + body->linearVelocity * dt);
        bounds.expand(bounds.maxs + body->linearVelocity * dt);

        const float epsilon = 0.01f;
        bounds.expand(bounds.mins - glm::vec3(1, 1, 1) * epsilon);
        bounds.expand(bounds.maxs + glm::vec3(1, 1, 1) * epsilon);

        sortedArray[2 * i].id = i;
        sortedArray[2 * i].value = glm::dot(axis, bounds.mins);
        sortedArray[2 * i].ismin = true;

        sortedArray[2 * i + 1].id = i;
        sortedArray[2 * i + 1].value = glm::dot(axis, bounds.maxs);
        sortedArray[2 * i + 1].ismin = false;
    }

    std::sort(sortedArray.begin(), sortedArray.end(),
              [](const PseudoBody &a, const PseudoBody &b) {
                  return a.value < b.value;
              });
}

void bezel::buildPairs(std::vector<CollisionPair> &pairs,
                       std::vector<PseudoBody> &bodies) {
    pairs.clear();

    for (size_t i = 0; i < bodies.size(); i++) {
        PseudoBody &a = bodies[i];
        if (!a.ismin) {
            continue;
        }

        CollisionPair pair;
        pair.a = a.id;

        for (size_t j = i + 1; j < bodies.size(); j++) {
            PseudoBody &b = bodies[j];
            if (b.id == a.id) {
                break;
            }

            if (!b.ismin) {
                continue;
            }
            pair.b = b.id;
            pairs.push_back(pair);
        }
    }
}

void bezel::sweepAndPrune1d(std::vector<std::shared_ptr<Body>> bodies,
                            std::vector<CollisionPair> &pairs, float dt) {
    std::vector<PseudoBody> sortedArray(bodies.size() * 2);
    sortBodiesForBounds(bodies, sortedArray, dt);
    buildPairs(pairs, sortedArray);
}

void bezel::broadPhase(std::vector<std::shared_ptr<Body>> bodies,
                       std::vector<CollisionPair> &pairs, float dt) {
    pairs.clear();

    sweepAndPrune1d(bodies, pairs, dt);
}
