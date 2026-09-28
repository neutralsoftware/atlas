//
// epa.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: EPA algorithm implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/native/body.h"
#include "bezel/native/shape.h"
#include <cstddef>
#include <utility>
#include <vector>
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>

glm::vec3 bezel::barycentricCoordinates(glm::vec3 &s1, glm::vec3 &s2,
                                        glm::vec3 &s3, const glm::vec3 &pt) {
    s1 = s1 - pt;
    s2 = s2 - pt;
    s3 = s3 - pt;

    glm::vec3 normal = glm::cross(s2 - s1, s3 - s1);
    glm::vec3 p0 = normal * glm::dot(normal, s1) / glm::length2(normal);

    int idx = 0;
    float area_max = 0;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        glm::vec2 a = glm::vec2(s1[j], s1[k]);
        glm::vec2 b = glm::vec2(s2[j], s2[k]);
        glm::vec2 c = glm::vec2(s3[j], s3[k]);
        glm::vec2 ab = b - a;
        glm::vec2 ac = c - a;

        float area = ab.x * ac.y - ab.y * ac.x;
        if (area * area > area_max * area_max) {
            idx = i;
            area_max = area;
        }
    }

    int x = (idx + 1) % 3;
    int y = (idx + 2) % 3;
    glm::vec2 s[3];
    s[0] = glm::vec2(s1[x], s1[y]);
    s[1] = glm::vec2(s2[x], s2[y]);
    s[2] = glm::vec2(s3[x], s3[y]);
    glm::vec2 p(p0[x], p0[y]);

    glm::vec3 areas;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        glm::vec2 a = p;
        glm::vec2 b = s[j];
        glm::vec2 c = s[k];
        glm::vec2 ab = b - a;
        glm::vec2 ac = c - a;

        areas[i] = ab.x * ac.y - ab.y * ac.x;
    }

    glm::vec3 lambdas = areas / area_max;
    if (glm::any(glm::isnan(lambdas))) {
        return glm::vec3(0.0f);
    }
    return lambdas;
}

glm::vec3 bezel::normalDirection(const Triangle &tri,
                                 const std::vector<Point> &points) {
    const glm::vec3 &a = points[tri.a].xyz;
    const glm::vec3 &b = points[tri.b].xyz;
    const glm::vec3 &c = points[tri.c].xyz;

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 normal = glm::cross(ab, ac);
    normal = glm::normalize(normal);
    return normal;
}

float bezel::signedDistanceToTriangle(const Triangle &tri, const glm::vec3 &pt,
                                      const std::vector<Point> &points) {
    const glm::vec3 normal = bezel::normalDirection(tri, points);
    const glm::vec3 &a = points[tri.a].xyz;
    float dist = glm::dot(normal, pt - a);
    return dist;
}

int bezel::closestTriangle(const std::vector<Triangle> &triangles,
                           const std::vector<Point> &points) {
    float minDistSqr = 1e10f;

    int idx = -1;
    for (size_t i = 0; i < triangles.size(); i++) {
        const Triangle &tri = triangles[i];

        float dist =
            bezel::signedDistanceToTriangle(tri, glm::vec3(0.0f), points);
        float distSqr = dist * dist;
        if (distSqr < minDistSqr) {
            minDistSqr = distSqr;
            idx = i;
        }
    }

    return idx;
}

bool bezel::hasPoint(const glm::vec3 &w, const std::vector<Triangle> &triangles,
                     const std::vector<Point> &points) {
    const float epsilon = 1e-6f;
    glm::vec3 delta;

    for (size_t i = 0; i < triangles.size(); i++) {
        const Triangle &tri = triangles[i];

        delta = w - points[tri.a].xyz;
        if (glm::length2(delta) < epsilon) {
            return true;
        }

        delta = w - points[tri.b].xyz;
        if (glm::length2(delta) < epsilon) {
            return true;
        }

        delta = w - points[tri.c].xyz;
        if (glm::length2(delta) < epsilon) {
            return true;
        }
    }
    return false;
}

int bezel::removeTrianglesFacingPoint(const glm::vec3 &pt,
                                      std::vector<Triangle> &triangles,
                                      const std::vector<Point> &points) {
    int numRemoved = 0;
    for (size_t i = 0; i < triangles.size(); i++) {
        const Triangle &tri = triangles[i];

        float dist = bezel::signedDistanceToTriangle(tri, pt, points);
        if (dist > 0.0f) {
            triangles.erase(triangles.begin() + i);
            i--;
            numRemoved++;
        }
    }
    return numRemoved;
}

void bezel::findDanglingEdges(std::vector<Edge> &danglingEdges,
                              const std::vector<Triangle> &triangles) {
    danglingEdges.clear();

    for (size_t i = 0; i < triangles.size(); i++) {
        const Triangle &tri = triangles[i];

        Edge edges[3];
        edges[0].a = tri.a;
        edges[0].b = tri.b;

        edges[1].a = tri.b;
        edges[1].b = tri.c;

        edges[2].a = tri.c;
        edges[2].b = tri.a;

        int counts[3];
        counts[0] = 0;
        counts[1] = 0;
        counts[2] = 0;

        for (size_t j = 0; j < triangles.size(); j++) {
            if (i == j) {
                continue;
            }

            const Triangle &tri2 = triangles[j];

            Edge edges2[3];
            edges2[0].a = tri2.a;
            edges2[0].b = tri2.b;

            edges2[1].a = tri2.b;
            edges2[1].b = tri2.c;

            edges2[2].a = tri2.c;
            edges2[2].b = tri2.a;

            for (int k = 0; k < 3; k++) {
                if (edges[k] == edges2[0]) {
                    counts[k]++;
                }

                if (edges[k] == edges2[1]) {
                    counts[k]++;
                }

                if (edges[k] == edges2[2]) {
                    counts[k]++;
                }
            }
        }

        for (int k = 0; k < 3; k++) {
            if (counts[k] == 0) {
                danglingEdges.push_back(edges[k]);
            }
        }
    }
}

float bezel::epaExpand(const std::shared_ptr<Body> bodyA,
                       const std::shared_ptr<Body> bodyB, const float bias,
                       const std::array<Point, 4> simplex, glm::vec3 &ptOnA,
                       glm::vec3 &ptOnB, glm::vec3 &normalOut) {
    std::vector<Point> points;
    std::vector<Triangle> triangles;
    std::vector<Edge> danglingEdges;

    glm::vec3 center = glm::vec3(0.0f);
    for (int i = 0; i < 4; i++) {
        points.push_back(simplex[i]);
        center += simplex[i].xyz;
    }
    center /= 4.0f;

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        int k = (i + 2) % 4;
        Triangle tri;
        tri.a = i;
        tri.b = j;
        tri.c = k;

        int unusedPt = (i + 3) % 4;
        float dist =
            bezel::signedDistanceToTriangle(tri, points[unusedPt].xyz, points);

        if (dist > 0.0f) {
            std::swap(tri.a, tri.b);
        }

        triangles.push_back(tri);
    }

    while (true) {
        const int idx = closestTriangle(triangles, points);
        glm::vec3 normal = bezel::normalDirection(triangles[idx], points);

        const Point newPt = bezel::support(bodyA, bodyB, normal, bias);

        if (bezel::hasPoint(newPt.xyz, triangles, points)) {
            break;
        }

        float dist =
            signedDistanceToTriangle(triangles[idx], newPt.xyz, points);
        if (dist <= 0.0f) {
            break;
        }

        const int newIdx = points.size();
        points.push_back(newPt);

        int numRemoved =
            bezel::removeTrianglesFacingPoint(newPt.xyz, triangles, points);
        if (numRemoved == 0) {
            break;
        }

        danglingEdges.clear();
        bezel::findDanglingEdges(danglingEdges, triangles);
        if (danglingEdges.size() == 0) {
            break;
        }

        for (size_t i = 0; i < danglingEdges.size(); i++) {
            const Edge &edge = danglingEdges[i];
            Triangle tri;
            tri.a = newIdx;
            tri.b = edge.a;
            tri.c = edge.b;

            float dist = bezel::signedDistanceToTriangle(tri, center, points);
            if (dist > 0.0f) {
                std::swap(tri.b, tri.c);
            }

            triangles.push_back(tri);
        }
    }

    const int idx = closestTriangle(triangles, points);
    const Triangle &tri = triangles[idx];
    glm::vec3 ptA_w = points[tri.a].xyz;
    glm::vec3 ptB_w = points[tri.b].xyz;
    glm::vec3 ptC_w = points[tri.c].xyz;
    glm::vec3 lambdas =
        bezel::barycentricCoordinates(ptA_w, ptB_w, ptC_w, glm::vec3(0.0f));

    glm::vec3 ptA_a = points[tri.a].ptA;
    glm::vec3 ptB_a = points[tri.b].ptA;
    glm::vec3 ptC_a = points[tri.c].ptA;
    ptOnA = lambdas.x * ptA_a + lambdas.y * ptB_a + lambdas.z * ptC_a;

    glm::vec3 ptA_b = points[tri.a].ptB;
    glm::vec3 ptB_b = points[tri.b].ptB;
    glm::vec3 ptC_b = points[tri.c].ptB;
    ptOnB = lambdas.x * ptA_b + lambdas.y * ptB_b + lambdas.z * ptC_b;

    glm::vec3 delta = ptOnB - ptOnA;
    float depth = glm::length(delta);

    glm::vec3 normal(0.0f);
    if (depth > 1e-6f) {
        normal = delta / depth;
    } else {
        normal = bezel::normalDirection(tri, points);
        float signedDist =
            bezel::signedDistanceToTriangle(tri, glm::vec3(0.0f), points);
        depth = std::abs(signedDist);
    }

    glm::vec3 centerDelta =
        bodyB->getCenterOfMassWorldSpace() - bodyA->getCenterOfMassWorldSpace();
    if (glm::length2(centerDelta) > 1e-12f &&
        glm::dot(normal, centerDelta) < 0.0f) {
        normal = -normal;
    }

    normalOut = normal;
    ptOnB = ptOnA + normal * depth;
    return depth;
}