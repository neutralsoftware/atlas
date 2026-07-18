/*
 scene.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Scene functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_SCENE_H
#define ATLAS_SCENE_H

#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "bezel/bezel.h"
#include "hydra/atmosphere.h"
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

class Window;
struct AreaLight;

/**
 * @brief Parameters controlling exponential fog accumulation in the scene.
 */
struct Fog {
    /**
     * @brief Tint applied to the fog as fragments recede into the distance.
     */
    Color color = Color::white();
    /**
     * @brief Density of the fog; larger values cause quicker fade-out.
     */
    float intensity = 0.0f;
};

/**
 * @brief Settings used when simulating volumetric light shafts.
 */
struct VolumetricLighting {
    bool enabled = false;
    /**
     * @brief Overall density of the participating medium.
     */
    float density = 0.3f;
    /**
     * @brief Strength of each iterative sample along the ray march.
     */
    float weight = 0.01;
    /**
     * @brief Damping factor applied per step to soften distant contributions.
     */
    float decay = 0.95;
    /**
     * @brief Exposure applied after integrating scattered light.
     */
    float exposure = 0.6f;
};

/**
 * @brief Configuration values for bloom post-processing.
 */
struct LightBloom {
    /**
     * @brief Radius of the blur kernel applied to bright fragments.
     */
    float radius = 0.008f;
    /**
     * @brief Maximum number of blur passes performed.
     */
    int maxSamples = 5;
};

/**
 * @brief Settings for rim lighting, accentuating silhouettes opposite the
 * camera.
 */
struct RimLight {
    /**
     * @brief Strength of the rim contribution.
     */
    float intensity = 0.0f;
    /**
     * @brief Color applied to the rim highlight.
     */
    Color color = Color::white();
};

/**
 * @brief Aggregates configurable environmental effects such as fog and bloom.
 */
class Environment {
  public:
    /**
     * @brief Fog parameters that softly blend distant geometry with the sky.
     */
    Fog fog;
    /**
     * @brief Controls for volumetric light scattering.
     */
    VolumetricLighting volumetricLighting;
    /**
     * @brief Configures bloom radii and blur passes.
     */
    LightBloom lightBloom;
    /**
     * @brief Rim lighting intensity and tint.
     */
    RimLight rimLight;
    /**
     * @brief Optional 3D lookup texture for color grading.
     */
    Texture lookupTexture;
};

/**
 * @brief Abstract class that represents a 3D scene. It contains all lights and
 * objects that are going to be rendered. It also provides methods for updating
 * the scene and handling input events.
 * \subsection scene-example Example
 * ```cpp
 * class MyScene : public Scene {
 *  public:
 *    void initialize(Window &window) override {
 *      // Initialize scene objects and lights here
 *    }
 *
 *    void update(Window &window) override {
 *      // Update scene logic here
 *    }
 *   void onMouseMove(Window &window, Movement2d movement) override {
 *      // Handle mouse movement here
 *    }
 * };
 *
 * ```
 *
 */
class Scene {
  public:
    virtual ~Scene() = default;
    /**
     * @brief Function that initializes the scene. This method is called every
     * frame.
     *
     * @param window The window in which the scene is going to be runned.
     */
    virtual void update([[maybe_unused]] Window &window) {};
    /**
     * @brief Function that initializes the scene. This method is called once by
     * the \ref Window class.
     *
     * @param window The window in which the scene is going to be runned.
     */
    virtual void initialize(Window &window) = 0;
    /**
     * @brief Function that handles mouse movement events.
     *
     * @param window  The window in which the scene is being rendered.
     * @param movement The movement delta.
     */
    virtual void onMouseMove([[maybe_unused]] Window &window,
                             [[maybe_unused]] Movement2d movement) {}
    /**
     * @brief Function that handles mouse scroll events.
     *
     * @param window The window in which the scene is being rendered.
     * @param offset The scroll offset.
     */
    virtual void onMouseScroll([[maybe_unused]] Window &window,
                               [[maybe_unused]] Movement2d offset) {}

    /**
     * @brief Sets the intensity of the ambient light in the scene.
     *
     * @param intensity The desired ambient light intensity. This value is
     * divided by 4 internally.
     */
    void setAmbientIntensity(float intensity) {
        this->ambientLight.intensity = intensity / 4;
        if (automaticAmbient) {
            automaticAmbient = false;
        }
    }

    /**
     * @brief Enables or disables automatic ambient coloring derived from the
     * scene's skybox.
     */
    void setAutomaticAmbient(bool enabled) {
        automaticAmbient = enabled;
        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
        }
    }

    /**
     * @brief Returns whether automatic ambient sampling is enabled.
     */
    bool isAutomaticAmbientEnabled() const { return automaticAmbient; }

    /**
     * @brief Gets the ambient color computed from the skybox when automatic
     * ambient is active.
     */
    Color getAutomaticAmbientColor() const { return automaticAmbientColor; }

    /**
     * @brief Gets the intensity derived from the skybox when automatic ambient
     * is active.
     */
    float getAutomaticAmbientIntensity() const {
        return automaticAmbientIntensity;
    }

    /**
     * @brief Returns the manually configured ambient light color.
     */
    Color getAmbientColor() const { return ambientLight.color; }
    void setAmbientColor(const Color &color) {
        ambientLight.color = color;
        if (automaticAmbient) {
            automaticAmbient = false;
        }
    }

    /**
     * @brief Returns the manually configured ambient intensity.
     */
    float getAmbientIntensity() const { return ambientLight.intensity; }

    /**
     * @brief Function that adds a directional light to the scene. \warning The
     * light object must be valid during the entire scene lifetime.
     *
     * @param light The directional light to add.
     */
    void addDirectionalLight(DirectionalLight *light) {
        directionalLights.push_back(light);
    }

    /**
     * @brief Function that adds a point light to the scene. \warning The light
     * object must be valid during the entire scene lifetime.
     *
     * @param light The point light to add.
     */

    void addLight(Light *light) { pointLights.push_back(light); }

    /**
     * @brief Function that adds a spotlight to the scene. \warning The light
     * object must be valid during the entire scene lifetime.
     *
     * @param light The spotlight to add.
     */
    void addSpotlight(Spotlight *light) { spotlights.push_back(light); }
    /**
     * @brief Adds an area light to the scene. \warning The light pointer must
     * remain valid for the entire scene lifetime.
     */
    void addAreaLight(AreaLight *light) { areaLights.push_back(light); }

    const std::vector<DirectionalLight *> &getDirectionalLights() const {
        return directionalLights;
    }

    const std::vector<Light *> &getPointLights() const { return pointLights; }

    const std::vector<Spotlight *> &getSpotlights() const { return spotlights; }

    const std::vector<AreaLight *> &getAreaLights() const { return areaLights; }

    void clearLights() {
        directionalLights.clear();
        pointLights.clear();
        spotlights.clear();
        areaLights.clear();
    }

    /**
     * @brief Set the Skybox object
     *
     * @param newSkybox The new skybox to set.
     */
    void setSkybox(std::shared_ptr<Skybox> newSkybox) {
        userSkybox = std::move(newSkybox);
        if (!useAtmosphereSkybox) {
            skybox = userSkybox;
            if (automaticAmbient) {
                updateAutomaticAmbientFromSkybox();
            }
        }
    }

    /**
     * @brief Enables or disables using the atmosphere-generated skybox as the
     * scene skybox.
     *
     * When enabled, the renderer is expected to populate `atmosphereSkybox`,
     * and the scene will expose it via `getSkybox()`.
     *
     * When disabled, the scene will use the user-provided skybox set through
     * `setSkybox()`.
     */
    void setUseAtmosphereSkybox(bool enabled) {
        useAtmosphereSkybox = enabled;
        skybox = useAtmosphereSkybox ? atmosphereSkybox : userSkybox;
        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
        }
    }

    /**
     * @brief Returns whether the atmosphere-generated skybox is currently used.
     */
    bool isUsingAtmosphereSkybox() const { return useAtmosphereSkybox; }

    /**
     * @brief Sets the internally stored atmosphere-generated skybox.
     *
     * This is intended to be called by the renderer/engine once it has
     * generated or updated the sky cubemap from `atmosphere`.
     */
    void setAtmosphereSkybox(std::shared_ptr<Skybox> generatedSkybox) {
        atmosphereSkybox = std::move(generatedSkybox);
        if (useAtmosphereSkybox) {
            skybox = atmosphereSkybox;
            if (automaticAmbient) {
                updateAutomaticAmbientFromSkybox();
            }
        }
    }

    /**
     * @brief Returns the internally stored atmosphere-generated skybox.
     */
    std::shared_ptr<Skybox> getAtmosphereSkybox() const {
        return atmosphereSkybox;
    }

    /**
     * @brief Overrides the environmental rendering configuration for the
     * scene.
     */
    void setEnvironment(Environment newEnv) { environment = std::move(newEnv); }

    /**
     * @brief Internal update hook used by the renderer to advance scene-wide
     * effects.
     */
    void updateScene(float dt);

    /**
     * @brief Returns the currently active skybox instance.
     */
    std::shared_ptr<Skybox> getSkybox() const { return skybox; }

    /**
     * @brief Atmosphere settings used for procedural sky and weather.
     */
    Atmosphere atmosphere;

  private:
    Environment environment;
    std::vector<DirectionalLight *> directionalLights;
    std::vector<Light *> pointLights;
    std::vector<Spotlight *> spotlights;
    std::vector<AreaLight *> areaLights;
    std::shared_ptr<Skybox> skybox = nullptr;

    std::shared_ptr<Skybox> userSkybox = nullptr;

    std::shared_ptr<Skybox> atmosphereSkybox = nullptr;

    bool useAtmosphereSkybox = false;
    AmbientLight ambientLight = {
        .color = {.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f},
        .intensity = 0.5f / 4};
    bool automaticAmbient = false;
    Color automaticAmbientColor = Color::white();
    float automaticAmbientIntensity = ambientLight.intensity;

    void updateAutomaticAmbientFromSkybox() {
        if (skybox != nullptr && skybox->cubemap.hasAverageColor) {
            automaticAmbientColor = skybox->cubemap.averageColor;
            double luminance = (0.2126 * automaticAmbientColor.r) +
                               (0.7152 * automaticAmbientColor.g) +
                               (0.0722 * automaticAmbientColor.b);
            automaticAmbientIntensity =
                static_cast<float>(std::clamp(luminance, 0.0, 1.0));
        } else {
            automaticAmbientColor = ambientLight.color;
            automaticAmbientIntensity = ambientLight.intensity;
        }
    }

    friend class CoreObject;
    friend class Window;
    friend class Terrain;
    friend class RenderTarget;
    friend struct Fluid;
    friend class Context;
};

#endif // ATLAS_SCENE_H
