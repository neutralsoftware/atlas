/*
 convexHull.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions for the convex hull algorithm
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
#include <utility>

int bezel::findFurthestPointInDirection(const std::vector<glm::vec3> &points,
                                        const glm::vec3 &dir) {
    int maxIdx = 0;
    float maxDist = glm::dot(points[0], dir);
    for (size_t i = 1; i < points.size(); i++) {
        const glm::vec3 &v = points[i];
        float dist = glm::dot(v, dir);
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return maxIdx;
}

float bezel::distanceFromLine(const glm::vec3 &a, const glm::vec3 &b,
                              const glm::vec3 &pt) {
    glm::vec3 ab = b - a;
    ab = glm::normalize(ab);

    glm::vec3 ray = pt - a;
    glm::vec3 proj = glm::dot(ray, ab) * ab;
    glm::vec3 perp = ray - proj;
    return glm::length(perp);
}

glm::vec3 bezel::findFurthestPointFromLine(const std::vector<glm::vec3> &points,
                                           const glm::vec3 &a,
                                           const glm::vec3 &b) {
    int maxIdx = 0;
    float maxDist = distanceFromLine(a, b, points[0]);
    for (size_t i = 1; i < points.size(); i++) {
        float dist = distanceFromLine(a, b, points[i]);
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return points[maxIdx];
}

float bezel::distanceFromTriangle(const glm::vec3 &a, const glm::vec3 &b,
                                  const glm::vec3 &c, const glm::vec3 &pt) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 normal = glm::cross(ab, ac);
    normal = glm::normalize(normal);

    glm::vec3 ray = pt - a;
    float dist = glm::dot(ray, normal);
    return dist;
}

glm::vec3
bezel::findFurthestPointFromTriangle(const std::vector<glm::vec3> &points,
                                     const glm::vec3 &a, const glm::vec3 &b,
                                     const glm::vec3 &c) {
    int maxIdx = 0;
    float maxDist = std::abs(distanceFromTriangle(a, b, c, points[0]));
    for (size_t i = 1; i < points.size(); i++) {
        float dist = std::abs(distanceFromTriangle(a, b, c, points[i]));
        if (dist > maxDist) {
            maxDist = dist;
            maxIdx = i;
        }
    }
    return points[maxIdx];
}

void bezel::computeTetrahedron(std::vector<glm::vec3> vert,
                               std::vector<glm::vec3> &hullPts,
                               std::vector<Triangle> &hullTris) {
    hullPts.clear();
    hullTris.clear();

    glm::vec3 points[4];

    int idx = findFurthestPointInDirection(vert, glm::vec3(1, 0, 0));
    points[0] = vert[idx];
    idx = findFurthestPointInDirection(vert, points[0] * -1.0f);
    points[1] = vert[idx];
    points[2] = findFurthestPointFromLine(vert, points[0], points[1]);
    points[3] =
        findFurthestPointFromTriangle(vert, points[0], points[1], points[2]);

    float dist =
        distanceFromTriangle(points[0], points[1], points[2], points[3]);
    if (dist > 0.0f) {
        std::swap(points[0], points[1]);
    }

    hullPts.push_back(points[0]);
    hullPts.push_back(points[1]);
    hullPts.push_back(points[2]);
    hullPts.push_back(points[3]);

    Triangle t1 = {0, 1, 2};
    Triangle t2 = {0, 2, 3};
    Triangle t3 = {2, 1, 3};
    Triangle t4 = {1, 0, 3};

    hullTris.push_back(t1);
    hullTris.push_back(t2);
    hullTris.push_back(t3);
    hullTris.push_back(t4);
}

void bezel::expandConvexHull(std::vector<glm::vec3> &hullPts,
                             std::vector<Triangle> &hullTris,
                             const std::vector<glm::vec3> &vertices) {
    std::vector<glm::vec3> externalVerts = vertices;
    removeInternalPoints(hullPts, hullTris, externalVerts);

    while (externalVerts.size() > 0) {
        int ptIdx =
            findFurthestPointInDirection(externalVerts, externalVerts[0]);
        glm::vec3 pt = externalVerts[ptIdx];
        externalVerts.erase(externalVerts.begin() + ptIdx);
        addPoint(hullPts, hullTris, pt);
        removeInternalPoints(hullPts, hullTris, externalVerts);
    }

    removeUnreferencedVertices(hullPts, hullTris);
}

void bezel::removeInternalPoints(std::vector<glm::vec3> &hullPts,
                                 std::vector<Triangle> &hullTris,
                                 std::vector<glm::vec3> &checkPts) {
    for (size_t i = 0; i < checkPts.size(); i++) {
        const glm::vec3 &pt = checkPts[i];

        bool isExternal = false;
        for (const auto &tri : hullTris) {
            const glm::vec3 &a = hullPts[tri.a];
            const glm::vec3 &b = hullPts[tri.b];
            const glm::vec3 &c = hullPts[tri.c];

            float dist = distanceFromTriangle(a, b, c, pt);
            if (dist > 0.0f) {
                isExternal = true;
                break;
            }
        }

        if (!isExternal) {
            checkPts.erase(checkPts.begin() + i);
            i--;
        }
    }

    for (size_t i = 0; i < checkPts.size(); i++) {
        const glm::vec3 &pt = checkPts[i];

        bool isTooClose = false;
        for (size_t j = 0; j < hullPts.size(); j++) {
            glm::vec3 hullPt = hullPts[j];
            glm::vec3 ray = hullPt - pt;
            if (glm::length2(ray) < 1e-6f) {
                isTooClose = true;
                break;
            }
        }

        if (isTooClose) {
            checkPts.erase(checkPts.begin() + i);
            i--;
        }
    }
}

bool bezel::isEdgeUnique(const std::vector<Triangle> &triangles,
                         const std::vector<int> &facingTris,
                         const int ignoreTri, const Edge &edge) {
    for (auto &tri : facingTris) {
        if (tri == ignoreTri) {
            continue;
        }
        const Triangle &t = triangles[tri];
        Edge e1 = {t.a, t.b};
        Edge e2 = {t.b, t.c};
        Edge e3 = {t.c, t.a};

        for (auto &e : {e1, e2, e3}) {
            if (e == edge) {
                return false;
            }
        }
    }

    return true;
}

void bezel::addPoint(std::vector<glm::vec3> &hullPts,
                     std::vector<Triangle> &hullTris, const glm::vec3 &pt) {
    std::vector<int> facingTris;
    for (size_t i = 0; i < hullTris.size(); i++) {
        const Triangle &tri = hullTris[i];
        const glm::vec3 &a = hullPts[tri.a];
        const glm::vec3 &b = hullPts[tri.b];
        const glm::vec3 &c = hullPts[tri.c];

        float dist = distanceFromTriangle(a, b, c, pt);
        if (dist > 0.0f) {
            facingTris.push_back(i);
        }
    }

    std::vector<Edge> uniqueEdges;
    for (size_t i = 0; i < facingTris.size(); i++) {
        const Triangle &tri = hullTris[facingTris[i]];
        Edge e1 = {tri.a, tri.b};
        Edge e2 = {tri.b, tri.c};
        Edge e3 = {tri.c, tri.a};

        for (auto &e : {e1, e2, e3}) {
            if (isEdgeUnique(hullTris, facingTris, facingTris[i], e)) {
                uniqueEdges.push_back(e);
            }
        }
    }

    for (size_t i = 0; i < uniqueEdges.size(); i++) {
        hullTris.erase(hullTris.begin() + facingTris[i]);
    }

    hullPts.push_back(pt);
    const int newPtIdx = hullPts.size() - 1;

    for (auto &e : uniqueEdges) {
        Triangle newTri = {e.a, e.b, newPtIdx};
        hullTris.push_back(newTri);
    }
}

void bezel::removeUnreferencedVertices(std::vector<glm::vec3> &hullPts,
                                       std::vector<Triangle> &hullTris) {
    for (int i = 0; i < (int)hullPts.size(); i++) {
        bool isUsed = false;
        for (const auto &tri : hullTris) {
            if (tri.a == i || tri.b == i || tri.c == i) {
                isUsed = true;
                break;
            }
        }

        if (isUsed) {
            continue;
        }

        for (int j = 0; j < (int)hullTris.size(); j++) {
            Triangle &tri = hullTris[j];
            if (tri.a > i) {
                tri.a--;
            }
            if (tri.b > i) {
                tri.b--;
            }
            if (tri.c > i) {
                tri.c--;
            }
        }

        hullPts.erase(hullPts.begin() + i);
        i--;
    }
}

void bezel::buildConvexHull(std::vector<glm::vec3> vertices,
                            std::vector<glm::vec3> &hullPts,
                            std::vector<Triangle> &hullTris) {
    if (vertices.size() < 4) {
        return;
    }

    computeTetrahedron(vertices, hullPts, hullTris);
    expandConvexHull(hullPts, hullTris, vertices);
}

bool bezel::isExternal(const std::vector<glm::vec3> &pts,
                       const std::vector<Triangle> &tris, const glm::vec3 &pt) {
    bool isExternal = false;
    for (const auto &tri : tris) {
        const glm::vec3 &a = pts[tri.a];
        const glm::vec3 &b = pts[tri.b];
        const glm::vec3 &c = pts[tri.c];

        float dist = distanceFromTriangle(a, b, c, pt);
        if (dist > 0.0f) {
            isExternal = true;
            break;
        }
    }
    return isExternal;
}

glm::vec3 bezel::calculateCenterOfMass(const std::vector<glm::vec3> &pts,
                                       const std::vector<Triangle> &tris) {
    const int sampleCount = 100;

    Bounds bounds;
    bounds.expand(pts, pts.size());

    glm::vec3 centerOfMass(0.0f);
    const float dx = bounds.widthX() / sampleCount;
    const float dy = bounds.widthY() / sampleCount;
    const float dz = bounds.widthZ() / sampleCount;

    int numSamples = 0;
    for (float x = bounds.mins.x; x < bounds.maxs.x; x += dx) {
        for (float y = bounds.mins.y; y < bounds.maxs.y; y += dy) {
            for (float z = bounds.mins.z; z < bounds.maxs.z; z += dz) {
                glm::vec3 pt(x, y, z);
                if (isExternal(pts, tris, pt)) {
                    continue;
                }

                centerOfMass += pt;
                numSamples++;
            }
        }
    }

    centerOfMass /= (float)numSamples;
    return centerOfMass;
}

glm::mat3 bezel::calculateInertiaTensor(const std::vector<glm::vec3> &pts,
                                        const std::vector<Triangle> &tris,
                                        const glm::vec3 &centerOfMass) {
    const int sampleCount = 100;

    Bounds bounds;
    bounds.expand(pts, pts.size());

    glm::mat3 tensor(0.0f);

    const float dx = bounds.widthX() / sampleCount;
    const float dy = bounds.widthY() / sampleCount;
    const float dz = bounds.widthZ() / sampleCount;

    int numSamples = 0;
    for (float x = bounds.mins.x; x < bounds.maxs.x; x += dx) {
        for (float y = bounds.mins.y; y < bounds.maxs.y; y += dy) {
            for (float z = bounds.mins.z; z < bounds.maxs.z; z += dz) {
                glm::vec3 pt(x, y, z);
                if (isExternal(pts, tris, pt)) {
                    continue;
                }

                pt -= centerOfMass;

                tensor[0][0] += pt.y * pt.y + pt.z * pt.z;
                tensor[1][1] += pt.z * pt.z + pt.x * pt.x;
                tensor[2][2] += pt.x * pt.x + pt.y * pt.y;

                tensor[0][1] += -1.0 * pt.x * pt.y;
                tensor[0][2] += -1.0 * pt.x * pt.z;
                tensor[1][2] += -1.0 * pt.y * pt.z;

                tensor[1][0] = -1.0f * pt.x * pt.y;
                tensor[2][0] = -1.0f * pt.x * pt.z;
                tensor[2][1] = -1.0f * pt.y * pt.z;

                numSamples++;
            }
        }
    }

    tensor /= (float)numSamples;
    return tensor;
}
