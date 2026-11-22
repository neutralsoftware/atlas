/*
 atmosphere.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Daylight and atmospheric effects
 Copyright (c) 2025 Max Van den Eynde
*/

#ifndef HYDRA_ATMOSPHERE_H
#define HYDRA_ATMOSPHERE_H

#include "atlas/camera.h"
#include "atlas/light.h"
#include "atlas/particle.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief Utility that procedurally generates 3D Worley noise textures for
 * cloud rendering.
 */
class WorleyNoise3D {
  public:
    /**
     * @brief Creates a Worley noise generator.
     *
     * @param frequency Number of feature points per unit space.
     * @param numberOfDivisions Octave divisions contributing to turbulence.
     */
    WorleyNoise3D(int frequency, int numberOfDivisions);
    /**
     * @brief Samples the noise field at a position.
     */
    float getValue(float x, float y, float z) const;

    /**
     * @brief Creates or retrieves a 3D noise texture with RGB packed channels.
     */
    Id get3dTexture(int res) const;
    /**
     * @brief Returns a detail layer texture for modulating primary noise.
     */
    Id getDetailTexture(int res) const;
    /**
     * @brief Creates a monochrome texture replicated across every channel.
     */
    Id get3dTextureAtAllChannels(int res) const;

  private:
    int frequency;
    int numberOfDivisions;
    std::vector<glm::vec3> featurePoints;

    void generateFeaturePoints();
    float getWorleyNoise(float x, float y, float z, int octave) const;
    std::vector<float> getClosestDistances(float x, float y, float z,
                                           int count) const;
    glm::ivec3 getGridCell(float x, float y, float z) const;
    int getCellIndex(int cx, int cy, int cz) const;

    Id createTexture3d(const std::vector<float> &data, int res) const;
};

/**
 * @brief High-level volumetric cloud configuration using Worley noise.
 */
class Clouds {
  public:
    /**
     * @brief Constructs a cloud generator with Worley noise presets.
     */
    Clouds(int frequency, int divisions) : worleyNoise(frequency, divisions) {};

    /**
     * @brief Builds a 3D procedural cloud texture at the requested resolution.
     */
    Id getCloudTexture(int res) const;

    /**
     * @brief Center of the cloud volume in world space.
     */
    Position3d position = {0.0f, 5.0f, 0.0f};
    /**
     * @brief Dimensions of the cloud volume.
     */
    Size3d size = {10.0f, 3.0f, 10.0f};
    /**
     * @brief Global scale factor applied during raymarching.
     */
    float scale = 1.5f;
    /**
     * @brief Texture offset applied to sample positions.
     */
    Position3d offset = {0.0f, 0.0f, 0.0f};
    /**
     * @brief Base density threshold for cloud appearance.
     */
    float density = 0.45f;
    /**
     * @brief Multiplier applied to density when shaping cloud clusters.
     */
    float densityMultiplier = 1.5f;
    /**
     * @brief Governs how much light is absorbed when traveling through clouds.
     */
    float absorption = 1.1f;
    /**
     * @brief Controls forward/backward scattering intensity.
     */
    float scattering = 0.85f;
    /**
     * @brief Phase function parameter used in lighting calculations.
     */
    float phase = 0.55f;
    /**
     * @brief Strength of high-frequency clusters inside the volume.
     */
    float clusterStrength = 0.5f;
    /**
     * @brief Number of primary raymarching steps per pixel.
     */
    int primaryStepCount = 12;
    /**
     * @brief Number of steps taken when computing lighting contributions.
     */
    int lightStepCount = 6;
    /**
     * @brief Scales the distance between light steps.
     */
    float lightStepMultiplier = 1.6f;
    /**
     * @brief Minimum step length to avoid undersampling.
     */
    float minStepLength = 0.05f;
    /**
     * @brief Wind vector influencing cloud movement.
     */
    Magnitude3d wind = {0.02f, 0.0f, 0.01f};

  private:
    WorleyNoise3D worleyNoise = WorleyNoise3D(4, 6);

    mutable Id cachedTextureId = 0;
    mutable int cachedResolution = 0;
};

/**
 * @brief Enumerates supported weather presets.
 */
enum class WeatherCondition { Clear, Rain, Snow, Storm };

/**
 * @brief Summary of the current atmospheric weather state.
 */
struct WeatherState {
    /**
     * @brief Dominant weather pattern.
     */
    WeatherCondition condition = WeatherCondition::Clear;
    /**
     * @brief Strength of the active weather effect (0-1).
     */
    float intensity = 0.0f;
    /**
     * @brief Resulting wind vector affecting particles and clouds.
     */
    Magnitude3d wind = {0.0f, 0.0f, 0.0f};
};

/**
 * @brief Callback type that returns weather based on view-dependent data.
 */
typedef std::function<WeatherState(ViewInformation)> WeatherDelegate;

/**
 * @brief Manages day-night cycle, sky colors, lights, and volumetric weather.
 *
 * \subsection atmosphere-example Example
 * ```cpp
 * // Create atmosphere with day-night cycle
 * Atmosphere atmosphere;
 * atmosphere.timeOfDay = 14.5f; // 2:30 PM
 * atmosphere.secondsPerHour = 120.0f; // Fast time: 2 minutes per hour
 * atmosphere.enable();
 *
 * // Configure volumetric clouds
 * Clouds clouds(4, 6); // Worley frequency and divisions
 * clouds.position = Position3d(0.0f, 100.0f, 0.0f);
 * clouds.size = Size3d(500.0f, 80.0f, 500.0f);
 * clouds.density = 0.5f;
 * clouds.absorption = 1.2f;
 * clouds.scattering = 0.9f;
 * clouds.wind = Magnitude3d(0.05f, 0.0f, 0.02f);
 * clouds.primaryStepCount = 16; // Quality vs performance
 *
 * // Set up dynamic weather
 * atmosphere.enableWeather();
 * atmosphere.weatherDelegate = [](ViewInformation viewInfo) {
 *     WeatherState weather;
 *     weather.condition = WeatherCondition::Rain;
 *     weather.intensity = 0.7f;
 *     weather.wind = Magnitude3d(0.1f, -0.5f, 0.05f);
 *     return weather;
 * };
 *
 * // Update each frame
 * atmosphere.update(deltaTime);
 * ```
 *
 */
class Atmosphere {
  public:
    ~Atmosphere();

    /**
     * @brief Current time of day expressed in hours [0, 24).
     */
    float timeOfDay = 12.0f;
    /**
     * @brief Number of simulated seconds per in-game hour.
     */
    float secondsPerHour = 3600.0f;

    /**
     * @brief Global wind vector affecting particles and clouds.
     */
    Magnitude3d wind = {0.0f, 0.0f, 0.0f};

    /**
     * @brief User-provided callback to derive weather conditions per frame.
     */
    WeatherDelegate weatherDelegate = [](ViewInformation) {
        return WeatherState();
    };

    /**
     * @brief Advances celestial bodies and weather simulation.
     */
    void update(float dt);
    /**
     * @brief Activates atmospheric rendering.
     */
    void enable() { enabled = true; }
    /**
     * @brief Deactivates atmospheric rendering.
     */
    void disable() { enabled = false; }
    /**
     * @brief Returns whether the atmosphere currently renders elements.
     */
    bool isEnabled() const { return enabled; }

    /**
     * @brief Enables weather effects such as precipitation.
     */
    void enableWeather() { weatherEnabled = true; }
    /**
     * @brief Disables weather particle systems.
     */
    void disableWeather() { weatherEnabled = false; }

    /**
     * @brief Time of day expressed as a normalized value [0, 1).
     */
    float getNormalizedTime() const;

    /**
     * @brief Direction vector pointing towards the sun.
     */
    Magnitude3d getSunAngle() const;
    /**
     * @brief Direction vector pointing towards the moon.
     */
    Magnitude3d getMoonAngle() const;

    /**
     * @brief Computes global lighting intensity based on time of day.
     */
    float getLightIntensity() const;
    /**
     * @brief Returns the current tint applied to global lighting.
     */
    Color getLightColor() const;

    /**
     * @brief Optional volumetric cloud configuration attached to the system.
     */
    std::shared_ptr<Clouds> clouds = nullptr;

    /**
     * @brief Retrieves the current gradient colors for procedural skyboxes.
     */
    std::array<Color, 6> getSkyboxColors() const;
    /**
     * @brief Builds a cubemap representing the sky at the current time.
     */
    Cubemap createSkyCubemap(int size = 256) const;
    /**
     * @brief Updates an existing cubemap in-place with new atmospheric data.
     */
    void updateSkyCubemap(Cubemap &cubemap) const;

    /**
     * @brief Generates shadow maps based on current sun direction.
     */
    void castShadowsFromSunlight(int res) const;
    /**
     * @brief Configures global scene lighting using the atmosphere settings.
     */
    void useGlobalLight();

    /**
     * @brief Color applied to sunlight during daytime.
     */
    Color sunColor = Color(1.0, 0.95, 0.8, 1.0);
    /**
     * @brief Color applied to moonlight during nighttime.
     */
    Color moonColor = Color(0.5, 0.5, 0.8, 1.0);

    /**
     * @brief Apparent size of the sun disk in the sky.
     */
    float sunSize = 1.0f;
    /**
     * @brief Apparent size of the moon disk in the sky.
     */
    float moonSize = 1.0f;

    /**
     * @brief Strength of the warm tint applied near horizons.
     */
    float sunTintStrength = 0.3f;
    /**
     * @brief Strength of the cold tint used for moonlit nights.
     */
    float moonTintStrength = 0.8f;
    /**
     * @brief Brightness multiplier for stars rendered in the skybox.
     */
    float starIntensity = 3.0f;

    /**
     * @brief Quickly assess whether the sun is above the horizon.
     */
    bool isDaytime() const {
        Magnitude3d sunDir = getSunAngle();
        return sunDir.y > 0.0f;
    }

    /**
     * @brief Overrides the current time of day.
     */
    void setTime(float hours) {
        if (hours >= 0 && hours < 24)
            timeOfDay = hours;
    }

    /**
     * @brief Instantiates cloud settings with default Worley parameters.
     */
    void addClouds(int frequency = 4, int divisions = 6) {
        if (!clouds) {
            clouds = std::make_shared<Clouds>(Clouds(frequency, divisions));
            clouds->wind = this->wind;
        }
    }

    /**
     * @brief Toggles automatic day-night cycling when true.
     */
    bool cycle = false;

    void resetRuntimeState();

  private:
    WeatherState lastWeather;
    bool enabled = false;
    bool weatherEnabled = false;
    mutable float lastSkyboxUpdateTime = -1.0f;
    mutable bool skyboxCacheValid = false;
    mutable std::array<Color, 6> lastSkyboxColors = {};

    std::shared_ptr<DirectionalLight> mainLight = nullptr;
    std::shared_ptr<ParticleEmitter> rainEmitter = nullptr;
    std::shared_ptr<ParticleEmitter> snowEmitter = nullptr;
};

#endif // HYDRA_ATMOSPHERE_H
