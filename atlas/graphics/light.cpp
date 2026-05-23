/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting helper functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <algorithm>
#include <tuple>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Light::createDebugObject() {
    CoreObject sphere = createSphere(0.05f, 36, 18, this->color);
    sphere.setPosition(this->position);
    sphere.material.albedo = this->color;
    sphere.material.emissiveColor = this->color;
    sphere.material.emissiveIntensity =
        std::clamp(this->intensity * 0.2f, 1.0f, 8.0f);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    sphere.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(sphere);
    this->debugObject->castsShadows = false;
    this->debugObject->editorOnly = true;
}

void Light::setColor(Color newColor) {
    this->color = newColor;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(newColor);
    }
}

void Light::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}

PointLightConstants Light::calculateConstants() const {
    struct Entry {
        float distance, constant, linear, quadratic;
    };
    static const Entry table[] = {
        {.distance = 7, .constant = 1.0f, .linear = 0.7f, .quadratic = 1.8f},
        {.distance = 13, .constant = 1.0f, .linear = 0.35f, .quadratic = 0.44f},
        {.distance = 20, .constant = 1.0f, .linear = 0.22f, .quadratic = 0.20f},
        {.distance = 32, .constant = 1.0f, .linear = 0.14f, .quadratic = 0.07f},
        {.distance = 50,
         .constant = 1.0f,
         .linear = 0.09f,
         .quadratic = 0.032f},
        {.distance = 65,
         .constant = 1.0f,
         .linear = 0.07f,
         .quadratic = 0.017f},
        {.distance = 100,
         .constant = 1.0f,
         .linear = 0.045f,
         .quadratic = 0.0075f},
        {.distance = 160,
         .constant = 1.0f,
         .linear = 0.027f,
         .quadratic = 0.0028f},
        {.distance = 200,
         .constant = 1.0f,
         .linear = 0.022f,
         .quadratic = 0.0019f},
        {.distance = 325,
         .constant = 1.0f,
         .linear = 0.014f,
         .quadratic = 0.0007f},
        {.distance = 600,
         .constant = 1.0f,
         .linear = 0.007f,
         .quadratic = 0.0002f},
        {.distance = 3250,
         .constant = 1.0f,
         .linear = 0.0014f,
         .quadratic = 0.000007f},
    };

    const int n = sizeof(table) / sizeof(table[0]);

    if (distance <= table[0].distance) {
        return {.distance = distance,
                .constant = table[0].constant,
                .linear = table[0].linear,
                .quadratic = table[0].quadratic,
                .radius = 0.0f};
    }
    if (distance >= table[n - 1].distance) {
        return {.distance = distance,
                .constant = table[n - 1].constant,
                .linear = table[n - 1].linear,
                .quadratic = table[n - 1].quadratic,
                .radius = 0.0f};
    }

    for (int i = 0; i < n - 1; i++) {
        if (distance >= table[i].distance &&
            distance <= table[i + 1].distance) {
            float t = (distance - table[i].distance) /
                      (table[i + 1].distance - table[i].distance);
            float constant = table[i].constant +
                             (t * (table[i + 1].constant - table[i].constant));
            float linear =
                table[i].linear + (t * (table[i + 1].linear - table[i].linear));
            float quadratic =
                table[i].quadratic +
                (t * (table[i + 1].quadratic - table[i].quadratic));
            float radius =
                (-linear + sqrt((linear * linear) -
                                ((constant - (256.0f / 5.0f) * distance) * 4 *
                                 quadratic))) /
                (2 * quadratic);
            return {.distance = distance,
                    .constant = constant,
                    .linear = linear,
                    .quadratic = quadratic,
                    .radius = radius};
        }
    }

    return {.distance = distance,
            .constant = 1.0f,
            .linear = 0.0f,
            .quadratic = 0.0f,
            .radius = 0.0f};
}

void Spotlight::createDebugObject() {
    CoreObject pyramid = createPyramid({0.1f, 0.1f, 0.1f}, this->color);
    pyramid.setPosition(this->position);
    pyramid.lookAt(this->position + this->direction);
    pyramid.material.albedo = this->color;
    pyramid.material.emissiveColor = this->color;
    pyramid.material.emissiveIntensity =
        std::clamp(this->intensity * 0.2f, 1.0f, 8.0f);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    pyramid.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(pyramid);
    this->debugObject->castsShadows = false;
    this->debugObject->editorOnly = true;
}

void Spotlight::setColor(Color newColor) {
    this->color = newColor;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(newColor);
    }
}

void Spotlight::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}

void Spotlight::updateDebugObjectRotation() const {
    if (this->debugObject != nullptr) {
        this->debugObject->setPosition(this->position);
        this->debugObject->lookAt(this->position + this->direction);
    }
}

void Spotlight::lookAt(const Position3d &target) {
    Magnitude3d newDirection = {target.x - this->position.x,
                                target.y - this->position.y,
                                target.z - this->position.z};

    this->direction = newDirection.normalized();

    updateDebugObjectRotation();
}

void Spotlight::castShadows(Window &window, int resolution) {
    atlas_log("Enabling shadow casting for spotlight (resolution: " +
              std::to_string(resolution) + ")");
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::Shadow, resolution);
    }
    this->doesCastShadows = true;
}

void DirectionalLight::castShadows(Window &window, int resolution) {
    atlas_log("Enabling shadow casting for directional light (resolution: " +
              std::to_string(resolution) + ")");
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::Shadow, resolution);
    }
    this->doesCastShadows = true;
}

ShadowParams DirectionalLight::calculateLightSpaceMatrix(
    const std::vector<Renderable *> &renderable) const {
    if (renderable.empty()) {
        glm::mat4 identity = glm::mat4(1.0f);
        return {.lightView = identity,
                .lightProjection = identity,
                .bias = 0.0f,
                .farPlane = 0.0f};
    }

    std::vector<glm::vec3> worldPoints;
    worldPoints.reserve(4096);
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());

    for (auto *obj : renderable) {
        if (obj == nullptr || !obj->canCastShadows())
            continue;

        if (const auto *modelObj = dynamic_cast<const Model *>(obj)) {
            const auto &modelObjects = modelObj->getObjects();
            for (const auto &modelMesh : modelObjects) {
                if (modelMesh == nullptr || !modelMesh->canCastShadows()) {
                    continue;
                }
                const auto &modelVertices = modelMesh->getVertices();
                if (modelVertices.empty()) {
                    continue;
                }
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix,
                                             modelMesh->getPosition().toGlm());
                modelMatrix *= glm::mat4_cast(
                    glm::normalize(modelMesh->getRotation().toGlmQuat()));
                modelMatrix =
                    glm::scale(modelMatrix, modelMesh->getScale().toGlm());

                for (const auto &vertex : modelVertices) {
                    glm::vec3 worldPos = glm::vec3(
                        modelMatrix * glm::vec4(vertex.position.toGlm(), 1.0f));
                    worldPoints.push_back(worldPos);
                    worldMin = glm::min(worldMin, worldPos);
                    worldMax = glm::max(worldMax, worldPos);
                }
            }
            continue;
        }

        const auto &vertices = obj->getVertices();
        if (vertices.empty())
            continue;

        if (const auto *coreObj = dynamic_cast<const CoreObject *>(obj)) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, coreObj->getPosition().toGlm());
            model *= glm::mat4_cast(
                glm::normalize(coreObj->getRotation().toGlmQuat()));
            model = glm::scale(model, coreObj->getScale().toGlm());
            for (const auto &vertex : vertices) {
                glm::vec3 worldPos =
                    glm::vec3(model * glm::vec4(vertex.position.toGlm(), 1.0f));
                worldPoints.push_back(worldPos);
                worldMin = glm::min(worldMin, worldPos);
                worldMax = glm::max(worldMax, worldPos);
            }
        } else {
            glm::vec3 pos = obj->getPosition().toGlm();
            glm::vec3 scale = obj->getScale().toGlm();
            for (const auto &vertex : vertices) {
                glm::vec3 worldPos = pos + (vertex.position.toGlm() * scale);
                worldPoints.push_back(worldPos);
                worldMin = glm::min(worldMin, worldPos);
                worldMax = glm::max(worldMax, worldPos);
            }
        }
    }

    if (worldPoints.empty()) {
        glm::mat4 identity = glm::mat4(1.0f);
        return {.lightView = identity,
                .lightProjection = identity,
                .bias = 0.0f,
                .farPlane = 0.0f};
    }

    glm::vec3 center = (worldMin + worldMax) * 0.5f;
    glm::vec3 extent = worldMax - worldMin;

    glm::vec3 lightDir = direction.toGlm();
    if (glm::dot(lightDir, lightDir) < 1e-8f) {
        lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
    }
    lightDir = glm::normalize(lightDir);

    float sceneRadius = std::max(1.0f, glm::length(extent) * 0.5f);
    float lightDistance = std::max(10.0f, sceneRadius + 5.0f);
    glm::vec3 lightPos = center - lightDir * lightDistance;

    glm::vec3 upAxis(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(lightDir, upAxis)) > 0.95f) {
        upAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if (glm::abs(glm::dot(lightDir, upAxis)) > 0.95f) {
        upAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::mat4 lightView = glm::lookAt(lightPos, center, upAxis);

    glm::vec3 lightSpaceMin(std::numeric_limits<float>::max());
    glm::vec3 lightSpaceMax(std::numeric_limits<float>::lowest());

    for (const auto &worldPos : worldPoints) {
        glm::vec3 lightSpacePos =
            glm::vec3(lightView * glm::vec4(worldPos, 1.0f));
        lightSpaceMin = glm::min(lightSpaceMin, lightSpacePos);
        lightSpaceMax = glm::max(lightSpaceMax, lightSpacePos);
    }

    float xyMargin = std::max(0.5f, sceneRadius * 0.08f);
    float zMargin = std::max(2.0f, sceneRadius * 0.1f);
    float left = lightSpaceMin.x - xyMargin;
    float right = lightSpaceMax.x + xyMargin;
    float bottom = lightSpaceMin.y - xyMargin;
    float top = lightSpaceMax.y + xyMargin;
    float near_plane = std::max(0.01f, -lightSpaceMax.z - zMargin);
    float far_plane = std::max(near_plane + 1.0f, -lightSpaceMin.z + zMargin);

    glm::mat4 lightProjection =
        glm::ortho(left, right, bottom, top, near_plane, far_plane);

    float bias =
        std::clamp(0.000008f * glm::length(extent), 0.00002f, 0.00025f);

    return {.lightView = lightView,
            .lightProjection = lightProjection,
            .bias = bias,
            .farPlane = 0.0f};
}

std::tuple<glm::mat4, glm::mat4> Spotlight::calculateLightSpaceMatrix() const {
    float near_plane = 0.1f, far_plane = 100.f;
    glm::mat4 lightProjection =
        glm::perspective(outerCutoff * 2.0f, 1.0f, near_plane, far_plane);
    glm::vec3 lightDir = glm::normalize(direction.toGlm());
    glm::vec3 lightPos = position.toGlm();
    glm::mat4 lightView =
        glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    return {lightView, lightProjection};
}

void Light::castShadows(Window &window, int resolution) {
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::CubeShadow, resolution);
    }
    this->doesCastShadows = true;
}

std::vector<glm::mat4> Light::calculateShadowTransforms() const {
    float aspect = (float)shadowRenderTarget->texture.creationData.width /
                   (float)shadowRenderTarget->texture.creationData.height;
    float near = 0.1f;
    float far = this->distance;
    glm::mat4 shadowProj =
        glm::perspective(glm::radians(90.0f), aspect, near, far);
    std::vector<glm::mat4> shadowTransforms;
    glm::vec3 lightPos = this->position.toGlm();
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0),
                                 glm::vec3(0.0, 0.0, 1.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0),
                                 glm::vec3(0.0, 0.0, -1.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    return shadowTransforms;
}

void AreaLight::castShadows(Window &window, int resolution) {
    atlas_log("Enabling shadow casting for area light (resolution: " +
              std::to_string(resolution) + ")");
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::Shadow, resolution);
    }
    this->doesCastShadows = true;
}

ShadowParams AreaLight::calculateLightSpaceMatrix() const {
    glm::vec3 rightAxis = glm::normalize(this->right.toGlm());
    glm::vec3 upAxis = glm::normalize(this->up.toGlm());
    glm::vec3 normal = glm::cross(rightAxis, upAxis);
    if (glm::length(normal) < 1e-6f) {
        normal = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        normal = glm::normalize(normal);
    }

    float clampedRange = std::max(1.0f, this->range);
    glm::vec3 lightPos = this->position.toGlm();
    float farPlane = clampedRange;
    if (this->castsBothSides) {
        lightPos -= normal * (clampedRange * 0.5f);
        farPlane = clampedRange * 2.0f;
    }

    glm::vec3 viewUp = upAxis;
    if (glm::abs(glm::dot(viewUp, normal)) > 0.98f) {
        viewUp = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(viewUp, normal)) > 0.98f) {
            viewUp = glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }

    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + normal, viewUp);
    float projectionPadding = std::max(0.35f, clampedRange * 0.28f);
    float halfWidth = std::max(
        0.45f, static_cast<float>(this->size.width) * 0.5f + projectionPadding);
    float halfHeight =
        std::max(0.45f, static_cast<float>(this->size.height) * 0.5f +
                            projectionPadding);
    float nearPlane = std::max(0.08f, clampedRange * 0.03f);
    farPlane = std::max(nearPlane + 0.75f, farPlane);

    glm::mat4 lightProjection = glm::ortho(-halfWidth, halfWidth, -halfHeight,
                                           halfHeight, nearPlane, farPlane);

    ShadowParams params;
    params.lightView = lightView;
    params.lightProjection = lightProjection;
    params.bias = 0.0025f;
    params.farPlane = 0.0f;
    return params;
}

void AreaLight::createDebugObject() {
    double w = this->size.width * 0.5;
    double h = this->size.height * 0.5;
    Color emissiveColor = this->color * 2.5f;
    emissiveColor.a = this->color.a;

    std::vector<CoreVertex> vertices = {
        {{-w, -h, 0.0},
         emissiveColor,
         {0.0, 0.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{w, -h, 0.0},
         emissiveColor,
         {1.0, 0.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{w, h, 0.0},
         emissiveColor,
         {1.0, 1.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{-w, h, 0.0},
         emissiveColor,
         {0.0, 1.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
    };

    std::vector<Index> indices = {
        0, 1, 2, 2, 3, 0, 0, 3, 2, 2, 1, 0,
    };

    CoreObject plane;
    plane.attachVertices(vertices);
    plane.attachIndices(indices);
    plane.material.albedo = this->color;
    plane.material.emissiveColor = this->color;
    plane.material.emissiveIntensity =
        std::clamp(this->intensity * 0.2f, 1.0f, 8.0f);

    glm::vec3 desiredRight = glm::normalize(this->right.toGlm());
    glm::vec3 desiredUp = glm::normalize(this->up.toGlm());
    glm::vec3 desiredNormal =
        glm::normalize(glm::cross(desiredRight, desiredUp));

    plane.setPosition(this->position);

    glm::vec3 center = this->position.toGlm();
    plane.lookAt(Position3d::fromGlm(center + desiredNormal),
                 Position3d::fromGlm(desiredUp));

    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);
    plane.createAndAttachProgram(vShader, shader);
    if (Window::mainWindow->usesDeferred) {
        plane.useDeferredRendering = false;
    }

    this->debugObject = std::make_shared<CoreObject>(plane);
    this->debugObject->castsShadows = false;
    this->debugObject->editorOnly = true;
}

void AreaLight::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}
