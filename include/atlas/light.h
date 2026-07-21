/*
 light.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Light definition and concepts
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_LIGHT_H
#define ATLAS_LIGHT_H

#include "atlas/camera.h"
#include "atlas/core/renderable.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <tuple>
#include <vector>

class Window;

// ============================================================================
// GPU Buffer Structures - must match shader buffer layouts exactly
// These are used with Pipeline::bindBuffer() for efficient GPU transfer
// ============================================================================

/**
 * @brief GPU-compatible directional light structure for shader buffers.
 * Matches the GLSL DirectionalLight struct in the shaders.
 */
struct alignas(16) GPUDirectionalLight {
    glm::vec3 direction;
    float _pad1;
    glm::vec3 diffuse;
    float _pad2;
    glm::vec3 specular;
    float intensity;
};

/**
 * @brief GPU-compatible point light structure for shader buffers.
 * Matches the GLSL PointLight struct in the shaders.
 */
struct alignas(16) GPUPointLight {
    glm::vec3 position;
    float _pad1;
    glm::vec3 diffuse;
    float _pad2;
    glm::vec3 specular;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

/**
 * @brief GPU-compatible spotlight structure for shader buffers.
 * Matches the GLSL SpotLight struct in the shaders.
 */
struct alignas(16) GPUSpotLight {
    glm::vec3 position;
    float _pad1;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    glm::vec3 diffuse;
    float _pad5;
    glm::vec3 specular;
    float _pad6;
};

/**
 * @brief GPU-compatible area light structure for shader buffers.
 * Matches the GLSL AreaLight struct in the shaders.
 */
struct alignas(16) GPUAreaLight {
    glm::vec3 position;
    float _pad1;
    glm::vec3 right;
    float _pad2;
    glm::vec3 up;
    float _pad3;
    glm::vec2 size;
    float _pad4;
    float _pad5;
    glm::vec3 diffuse;
    float _pad6;
    glm::vec3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

/**
 * @brief GPU-compatible shadow parameters structure for shader buffers.
 * Matches the GLSL ShadowParameters struct in the shaders.
 */
struct alignas(16) GPUShadowParams {
    glm::mat4 lightView;
    glm::mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    int lightIndex;
    glm::vec3 lightPos;
    int lightType;
};

// ============================================================================
// Original Light Structures
// ============================================================================

/**
 * @brief Structure representing ambient light in a scene. This is the most
 * straightforward type of light.
 *
 */
struct AmbientLight {
    /**
     * @brief The color of the ambient light. This will be reflected into all
     * objects that allow so, to offer a cohesive ambient.
     *
     */
    Color color;
    /**
     * @brief The intensity with which the ambient light is applied.
     *
     */
    float intensity;
};

/**
 * @brief Mathematical constants for a point light.
 *
 */
struct PointLightConstants {
    /**
     * @brief The distance the light reaches.
     *
     */
    float distance;
    /**
     * @brief The constant attenuation factor, which determines how much the
     * light intensity decreases over distance.
     *
     */
    float constant;
    /**
     * @brief The linear attenuation factor.
     *
     */
    float linear;
    /**
     * @brief The quadratic attenuation factor.
     *
     */
    float quadratic;
    /**
     * @brief Radius of influence derived from attenuation constants.
     */
    float radius;
};

/**
 * @brief Parameters that are submitted to a shadow shader to calculate shadows.
 *
 */
struct ShadowParams {
    /**
     * @brief The view matrix from the light's perspective.
     *
     */
    glm::mat4 lightView;
    /**
     * @brief The projection matrix from the light's perspective.
     *
     */
    glm::mat4 lightProjection;
    /**
     * @brief Constant bias to prevent shadow acne. It decreases when the
     * resolution increases.
     *
     */
    float bias;
    /**
     * @brief Far plane distance used for point light shadow mapping.
     * This should match the far plane used when rendering the point light's
     * shadow map.
     */
    float farPlane;
};

/**
 * @brief Structure representing a point light in a scene. A point light emits
 * light in all directions from a single point in space.
 *
 * \subsection example-light Example
 * ```cpp
 * // Create a point light at position (10, 10, 10) with white color and a
 * distance of 50 units Light pointLight({10.0f, 10.0f, 10.0f},
 * Color::white(), 50.0f);
 * // Set the light color to a soft yellow
 * pointLight.setColor(Color(1.0f, 0.9f, 0.7f));
 * // Enable shadow casting for the light
 * pointLight.castShadows(window);
 * // Add a debug object to visualize the light in the scene
 * pointLight.createDebugObject();
 * pointLight.addDebugObject(window);
 * // Add the light to the scene
 * this->addPointLight(&pointLight);
 * ```
 */
struct Light {
    /**
     * @brief The position of the light in 3D space.
     *
     */
    Position3d position = {0.0f, 0.0f, 0.0f};

    /**
     * @brief The color of the light.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the light will use for specular highlights.
     *
     */
    Color shineColor = Color::white();
    /**
     * @brief Overall brightness multiplier applied to this light.
     */
    float intensity = 1.0f;

    /**
     * @brief Function that constructs a new Light object.
     *
     * @param pos The position of the light in 3D space.
     * @param color The color for the light.
     * @param distance The distance the light reaches.
     * @param shineColor The color that the light will use for specular
     * highlights.
     */
    Light(const Position3d &pos = {0.0f, 0.0f, 0.0f},
          const Color &color = Color::white(), float distance = 50.f,
          const Color &shineColor = Color::white(), float intensity = 1.0f)
        : position(pos), color(color), shineColor(shineColor),
          intensity(intensity), distance(distance) {}

    /**
     * @brief Function that sets the color of the light.
     *
     * @param newColor The new color for the light.
     */
    void setColor(Color newColor);

    /**
     * @brief Function that creates a debug obejct to visualize the light in the
     * scene.
     *
     */
    void createDebugObject();
    /**
     * @brief Function that adds the debug object to the window.
     *
     * @param window The window in which to add the debug object.
     */
    void addDebugObject(Window &window);

    /**
     * @brief The debug object that visualizes the light in the scene.
     *
     */
    std::shared_ptr<CoreObject> debugObject = nullptr;

    /**
     * @brief Function that calculates the constants for the point light.
     *
     * @return (PointLightConstants) The calculated constants.
     */
    PointLightConstants calculateConstants() const;

    /**
     * @brief Distance to which the light reaches.
     *
     */
    float distance = 50.f;

    /**
     * @brief Function that casts shadows from the light.
     *
     * @param window The window in which to cast shadows.
     * @param resolution The resolution from which to build the shadow map.
     */
    void castShadows(Window &window, int resolution = 2048);

    /**
     * @brief The render target that holds the shadow map.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;
    /**
     * @brief Cached shadow matrices and bias used for the current shadow map.
     */
    ShadowParams lastShadowParams;

  private:
    bool doesCastShadows = false;

    std::vector<glm::mat4> calculateShadowTransforms() const;

    friend class Window;
    friend class CoreObject;
};

/**
 * @brief Structure representing a directional light in a scene. A directional
 * light emits light in a specific direction, simulating sunlight.
 *
 * \subsection example-directional-light Example
 * ```cpp
 * // Create a directional light pointing downwards with white color
 * DirectionalLight dirLight({0.0f, -1.0f, 0.0f}, Color::white());
 * // Set the light color to a warm yellow
 * dirLight.setColor(Color(1.0f, 0.95f, 0.8f));
 * // Enable shadow casting for the light
 * dirLight.castShadows(window);
 * // Add the light to the scene
 * this->addDirectionalLight(&dirLight);
 * ```
 *
 */
class DirectionalLight {
  public:
    /**
     * @brief The direction in which the light is pointing. This should be a
     * normalized vector.
     *
     */
    Magnitude3d direction;

    /**
     * @brief The color of the light.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the light will use for specular highlights.
     *
     */
    Color shineColor = Color::white();
    /**
     * @brief Overall brightness multiplier applied to this directional light.
     */
    float intensity = 1.0f;

    /**
     * @brief Function that constructs a new DirectionalLight object.
     *
     * @param dir The direction in which the light is pointing.
     * @param color The color for the light.
     * @param shineColor The color that the light will use for specular
     * highlights.
     */
    DirectionalLight(const Magnitude3d &dir = {0.0f, -1.0f, 0.0f},
                     const Color &color = Color::white(),
                     const Color &shineColor = Color::white(),
                     float intensity = 1.0f)
        : direction(dir.normalized()), color(color), shineColor(shineColor),
          intensity(intensity) {}

    /**
     * @brief Function that sets the color of the light.
     *
     * @param color The new color for the light.
     */
    void setColor(Color color) { this->color = color; }

    /**
     * @brief Object that holds the render target for shadow mapping.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;
    /**
     * @brief Cached shadow parameters used when the shadow map was last
     * rendered. Keeping this in sync with the shadow map avoids sampling
     * mismatches when matrices change between updates.
     */
    ShadowParams lastShadowParams;

    /**
     * @brief Function that enables casting shadows from the light.
     *
     * @param window  The window in which to cast shadows.
     * @param resolution The resolution to use for the shadow map.
     */
    void castShadows(Window &window, int resolution = 2048);

  private:
    bool doesCastShadows = false;

    ShadowParams calculateLightSpaceMatrix(
        const std::vector<Renderable *> &renderable) const;

    friend class Window;
    friend class CoreObject;
    friend class Terrain;

  private:
    std::vector<glm::vec4>
    getCameraFrustumCornersWorldSpace(Camera *camera, Window *window) const;
};

/**
 * @brief Structure representing a spotlight in a scene. A spotlight emits light
 * in a specific direction with a cone angle.
 * \subsection example-spotlight Example
 * ```cpp
 * // Create a spotlight at position (0, 10, 0) pointing downwards with white
 * color Spotlight spotLight({0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
 * Color::white(), 30.0f, 35.0f);
 * // Set the light color to a cool blue
 * spotLight.setColor(Color(0.7f, 0.8f, 1.0f));
 * // Enable shadow casting for the spotlight
 * spotLight.castShadows(window);
 * // Make the spotlight look at the origin
 * spotLight.lookAt({0.0f, 0.0f, 0.0f});
 * // Add a debug object to visualize the spotlight in the scene
 * spotLight.createDebugObject();
 * spotLight.addDebugObject(window);
 * // Add the spotlight to the scene
 * this->addSpotlight(&spotLight);
 * ```
 */
struct Spotlight {
    /**
     * @brief The position of the spotlight in 3D space.
     *
     */
    Position3d position = {0.0f, 0.0f, 0.0f};
    /**
     * @brief The direction in which the spotlight is pointing.
     *
     */
    Magnitude3d direction = {0.0f, -1.0f, 0.0f};

    /**
     * @brief The color of the spotlight.
     *
     */
    Color color = Color::white();
    /**
     * @brief The color that the spotlight will use for specular highlights.
     *
     */
    Color shineColor = Color::white();
    /**
     * @brief Overall brightness multiplier applied to this spotlight.
     */
    float intensity = 1.0f;
    /**
     * @brief Maximum influence distance of the spotlight.
     */
    float range = 50.0f;

    /**
     * @brief Function that constructs a new Spotlight object.
     *
     * @param pos The position of the spotlight in 3D space.
     * @param dir The direction in which the spotlight is pointing.
     * @param color The color for the spotlight.
     * @param angle The inner cone angle of the spotlight in degrees.
     * @param outerAngle The outer cone angle of the spotlight in degrees.
     * @param shineColor The color that the spotlight will use for specular
     * highlights.
     */
    Spotlight(const Position3d &pos = {0.0f, 0.0f, 0.0f},
              Magnitude3d dir = {0.0f, -1.0f, 0.0f},
              const Color &color = Color::white(), const float angle = 35.f,
              const float outerAngle = 40.f,
              const Color &shineColor = Color::white(), float intensity = 1.0f,
              float range = 50.0f)
        : position(pos), direction(dir), color(color), shineColor(shineColor),
          intensity(intensity), range(range),
          cutOff(glm::cos(glm::radians(static_cast<double>(angle)))),
          outerCutoff(glm::cos(glm::radians(static_cast<double>(outerAngle)))) {
    }

    /**
     * @brief Function that sets the color of the spotlight.
     *
     * @param newColor The new color for the spotlight.
     */
    void setColor(Color newColor);

    /**
     * @brief Function that creates a debug object to visualize the spotlight in
     * the scene.
     *
     */
    void createDebugObject();
    /**
     * @brief Function that adds the debug object to the window.
     *
     * @param window The window in which to add the debug object.
     */
    void addDebugObject(Window &window);
    /**
     * @brief Function that updates the rotation of the debug object.
     *
     */
    void updateDebugObjectRotation() const;
    /**
     * @brief Function that makes the spotlight look at a target position.
     *
     * @param target The position to look at.
     */
    void lookAt(const Position3d &target);

    /**
     * @brief The debug object that visualizes the spotlight in the scene.
     *
     */
    std::shared_ptr<CoreObject> debugObject = nullptr;

    /**
     * @brief The inner cone angle of the spotlight in degrees.
     *
     */
    float cutOff;
    /**
     * @brief The outer cone angle of the spotlight in degrees.
     *
     */
    float outerCutoff;

    /**
     * @brief The render target to which the spotlight casts shadows.
     *
     */
    RenderTarget *shadowRenderTarget = nullptr;
    /**
     * @brief Cached shadow parameters used when the shadow map was last
     * rendered. Keeping this in sync with the shadow map avoids sampling
     * mismatches when matrices change between updates.
     */
    ShadowParams lastShadowParams;

    /**
     * @brief Function that enables casting shadows from the spotlight.
     *
     * @param window The window in which to cast shadows.
     * @param resolution The resolution to use for the shadow map.
     */
    void castShadows(Window &window, int resolution = 2048);

  private:
    bool doesCastShadows = true;

    std::tuple<glm::mat4, glm::mat4> calculateLightSpaceMatrix() const;

    friend class Window;
    friend class CoreObject;
};

/**
 * @brief Rectangular area light with controllable emission angle and two-sided
 * emission. The rectangle is defined by its center (position) and two oriented
 * axes (right, up) and size (width, height). The plane normal is
 * normalize(cross(right, up)).
 */
struct AreaLight {
    /**
     * @brief Center position of the rectangle.
     */
    Position3d position = {0.0, 0.0, 0.0};
    /**
     * @brief Oriented axis for width direction (normalized).
     */
    Magnitude3d right = {1.0, 0.0, 0.0};
    /**
     * @brief Oriented axis for height direction (normalized).
     */
    Magnitude3d up = {0.0, 1.0, 0.0};
    /**
     * @brief Width and height of the rectangle.
     */
    Size2d size = {1.0, 1.0};

    /**
     * @brief Diffuse/emissive color of the light.
     */
    Color color = Color::white();
    /**
     * @brief Specular highlight color for the light.
     */
    Color shineColor = Color::white();
    /**
     * @brief Overall brightness multiplier applied to the area light.
     */
    float intensity = 1.0f;
    /**
     * @brief Maximum distance at which this area light contributes.
     */
    float range = 50.0f;

    /**
     * @brief Emission cone half-angle in degrees around the plane normal.
     * For example, 90 means a hemisphere emission relative to the plane normal.
     */
    float angle = 90.0f;

    /**
     * @brief If true, the light emits on both sides of the rectangle plane.
     */
    bool castsBothSides = false;

    /**
     * @brief Rotation tracking for the area light. Changing this and calling
     * setRotation/rotate will update right/up consistently.
     *
     * Rotation order matches CoreObject (roll Z, then pitch X, then yaw Y).
     */
    Rotation3d rotation = {0.0, 0.0, 0.0};

    /**
     * @brief Compute the plane normal (normalize(cross(right, up))).
     */
    Magnitude3d getNormal() const {
        glm::vec3 u = glm::normalize(right.toGlm());
        glm::vec3 v = glm::normalize(up.toGlm());
        glm::vec3 n = glm::normalize(glm::cross(u, v));
        return Magnitude3d::fromGlm(n);
    }

    /**
     * @brief Convenience to set diffuse color.
     */
    void setColor(Color c) { color = c; }

    /**
     * @brief Sets absolute rotation (in degrees) and updates right/up
     * accordingly. Rotation is applied to a canonical frame (right=+X, up=+Y,
     * normal=+Z) in the order: roll(Z), pitch(X), yaw(Y).
     */
    void setRotation(const Rotation3d &r) {
        rotation = r;
        updateAxesFromRotation();
    }

    /**
     * @brief Applies a delta rotation (in degrees) and updates right/up.
     */
    void rotate(const Rotation3d &delta) {
        rotation = rotation + delta;
        updateAxesFromRotation();
    }

    /**
     * @brief Debug helpers for visualizing the rectangle in the scene.
     */
    void createDebugObject();
    void addDebugObject(Window &window);
    void castShadows(Window &window, int resolution = 2048);

    /**
     * @brief Optional debug object visualizing the area light.
     */
    std::shared_ptr<CoreObject> debugObject = nullptr;
    /**
     * @brief Optional shadow render target generated for this area light.
     */
    RenderTarget *shadowRenderTarget = nullptr;
    /**
     * @brief Cached shadow matrices and bias for the current area-light map.
     */
    ShadowParams lastShadowParams;

  private:
    bool doesCastShadows = false;
    ShadowParams calculateLightSpaceMatrix() const;
    /**
     * @brief Recompute right/up from the current rotation to keep a coherent
     * frame.
     */
    void updateAxesFromRotation() {
        // Canonical basis
        glm::vec3 baseRight(1.0f, 0.0f, 0.0f);
        glm::vec3 baseUp(0.0f, 1.0f, 0.0f);
        // Construct rotation matrix (roll Z, then pitch X, then yaw Y)
        glm::mat4 m(1.0f);
        m = glm::rotate(m, glm::radians(static_cast<float>(rotation.roll)),
                        glm::vec3(0, 0, 1));
        m = glm::rotate(m, glm::radians(static_cast<float>(rotation.pitch)),
                        glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(static_cast<float>(rotation.yaw)),
                        glm::vec3(0, 1, 0));

        glm::vec3 R = glm::normalize(glm::vec3(m * glm::vec4(baseRight, 0.0f)));
        glm::vec3 U = glm::normalize(glm::vec3(m * glm::vec4(baseUp, 0.0f)));
        // Orthonormalize to avoid drift
        glm::vec3 N = glm::normalize(glm::cross(R, U));
        R = glm::normalize(glm::cross(U, N));
        U = glm::normalize(glm::cross(N, R));

        right = Magnitude3d::fromGlm(R);
        up = Magnitude3d::fromGlm(U);
    }

    friend class Window;
    friend class CoreObject;
};

#endif // ATLAS_LIGHT_H
