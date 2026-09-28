
#include "bezel/native/shape.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

glm::vec2 bezel::projectOn1D(const glm::vec3 &s1, const glm::vec3 &s2) {
    glm::vec3 ab = s2 - s1;
    glm::vec3 ao = -s1;

    float ab_length_sq = glm::dot(ab, ab);
    if (ab_length_sq < 1e-8f) {
        return glm::vec2(1.0f, 0.0f);
    }

    float t = glm::dot(ao, ab) / ab_length_sq;

    if (t <= 0.0f) {
        return glm::vec2(1.0f, 0.0f);
    }
    if (t >= 1.0f) {
        return glm::vec2(0.0f, 1.0f);
    }

    return glm::vec2(1.0f - t, t);
}

int bezel::compareSigns(float a, float b) {
    if (a < 0 && b < 0)
        return 1;
    if (a > 0 && b > 0)
        return 1;
    return 0;
}

glm::vec3 bezel::projectOn2D(const glm::vec3 &s1, const glm::vec3 &s2,
                             const glm::vec3 &s3) {
    glm::vec3 v0 = s2 - s1;
    glm::vec3 v1 = s3 - s1;
    glm::vec3 v2 = -s1;

    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);

    float denom = dot00 * dot11 - dot01 * dot01;

    if (std::abs(denom) < 1e-8f) {
        float d1 = glm::length2(s1);
        float d2 = glm::length2(s2);
        float d3 = glm::length2(s3);

        if (d1 <= d2 && d1 <= d3)
            return glm::vec3(1.0f, 0.0f, 0.0f);
        if (d2 <= d3)
            return glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }

    float invDenom = 1.0f / denom;
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    float w = 1.0f - u - v;

    const float epsilon = -1e-6f;
    if (w >= epsilon && u >= epsilon && v >= epsilon) {
        return glm::vec3(w, u, v);
    }

    float minDist = FLT_MAX;
    glm::vec3 bestLambdas(1.0f, 0.0f, 0.0f);

    float d1 = glm::length2(s1);
    if (d1 < minDist) {
        minDist = d1;
        bestLambdas = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    float d2 = glm::length2(s2);
    if (d2 < minDist) {
        minDist = d2;
        bestLambdas = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    float d3 = glm::length2(s3);
    if (d3 < minDist) {
        minDist = d3;
        bestLambdas = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec2 edge12 = projectOn1D(s1, s2);
    glm::vec3 pt12 = s1 * edge12[0] + s2 * edge12[1];
    float d12 = glm::length2(pt12);
    if (d12 < minDist) {
        minDist = d12;
        bestLambdas = glm::vec3(edge12[0], edge12[1], 0.0f);
    }

    glm::vec2 edge23 = projectOn1D(s2, s3);
    glm::vec3 pt23 = s2 * edge23[0] + s3 * edge23[1];
    float d23 = glm::length2(pt23);
    if (d23 < minDist) {
        minDist = d23;
        bestLambdas = glm::vec3(0.0f, edge23[0], edge23[1]);
    }

    glm::vec2 edge31 = projectOn1D(s3, s1);
    glm::vec3 pt31 = s3 * edge31[0] + s1 * edge31[1];
    float d31 = glm::length2(pt31);
    if (d31 < minDist) {
        minDist = d31;
        bestLambdas = glm::vec3(edge31[1], 0.0f, edge31[0]);
    }

    return bestLambdas;
}

glm::vec4 bezel::projectOn3D(const glm::vec3 &s1, const glm::vec3 &s2,
                             const glm::vec3 &s3, const glm::vec3 &s4) {
    glm::mat4 M;
    M[0] = glm::vec4(s1.x, s2.x, s3.x, s4.x);
    M[1] = glm::vec4(s1.y, s2.y, s3.y, s4.y);
    M[2] = glm::vec4(s1.z, s2.z, s3.z, s4.z);
    M[3] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    glm::vec4 C4;
    C4[0] = bezel::takeCofactor(M, 3, 0);
    C4[1] = bezel::takeCofactor(M, 3, 1);
    C4[2] = bezel::takeCofactor(M, 3, 2);
    C4[3] = bezel::takeCofactor(M, 3, 3);

    const float detM = C4[0] + C4[1] + C4[2] + C4[3];

    if (std::abs(detM) > 1e-8f) {
        if (compareSigns(detM, C4[0]) && compareSigns(detM, C4[1]) &&
            compareSigns(detM, C4[2]) && compareSigns(detM, C4[3])) {
            glm::vec4 lambdas = C4 / detM;
            return lambdas;
        }
    }

    float minDist = FLT_MAX;
    glm::vec4 bestLambdas(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 points[4] = {s1, s2, s3, s4};

    int faceIndices[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};

    for (int face = 0; face < 4; face++) {
        int i0 = faceIndices[face][0];
        int i1 = faceIndices[face][1];
        int i2 = faceIndices[face][2];

        glm::vec3 faceLambdas = projectOn2D(points[i0], points[i1], points[i2]);
        glm::vec3 pt = points[i0] * faceLambdas[0] +
                       points[i1] * faceLambdas[1] +
                       points[i2] * faceLambdas[2];

        float dist = glm::length2(pt);
        if (dist < minDist) {
            minDist = dist;
            bestLambdas = glm::vec4(0.0f);
            bestLambdas[i0] = faceLambdas[0];
            bestLambdas[i1] = faceLambdas[1];
            bestLambdas[i2] = faceLambdas[2];
        }
    }

    return bestLambdas;
}
