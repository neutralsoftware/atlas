//
// particle.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Particle defintions and implementations
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_PARTICLE_H
#define ATLAS_PARTICLE_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "opal/opal.h"
#include <optional>

/**
 * @brief Type that describes how particles are emitted.
 *
 */
enum class ParticleEmissionType {
    /**
     * @brief Emission type for fountain-like particle effects.
     *
     */
    Fountain,
    /**
     * @brief Emission type for snow-like particle effects.
     *
     */
    Ambient
};

/**
 * @brief Structure representing settings for particle behavior and appearance.
 *
 */
struct ParticleSettings {
    /**
     * @brief The minimum lifetime of a particle in seconds.
     *
     */
    float minLifetime = 1.0f;
    /**
     * @brief The maximum lifetime of a particle in seconds.
     *
     */
    float maxLifetime = 3.0f;
    /**
     * @brief The minimum size of a particle.
     *
     */
    float minSize = 0.02f;
    /**
     * @brief The maximum size of a particle.
     *
     */
    float maxSize = 0.01f;
    /**
     * @brief The speed at which a particle fades out.
     *
     */
    float fadeSpeed = 0.5f;
    /**
     * @brief The gravitational force applied to particles.
     *
     */
    float gravity = -9.81f;
    /**
     * @brief The spread of particles from the emitter.
     *
     */
    float spread = 1.0f;
    /**
     * @brief The speed variation of particles. Is how much the speed is
     * randomized.
     *
     */
    float speedVariation = 1.0f;
};

/**
 * @brief Structure representing a single particle in a particle system.
 *
 */
struct Particle {
    /**
     * @brief The position of the particle in 3D space.
     *
     */
    Position3d position;
    /**
     * @brief The velocity of the particle in 3D space.
     *
     */
    Magnitude3d velocity;
    /**
     * @brief The color that the particle will have.
     *
     */
    Color color;
    /**
     * @brief The current life of the particle in seconds.
     *
     */
    float life;
    /**
     * @brief The maximum life of the particle in seconds.
     *
     */
    float maxLife;
    /**
     * @brief The scale the particle will have.
     *
     */
    float size;
    /**
     * @brief Whether the particle is active or not.
     *
     */
    bool active;
};

/**
 * @brief Class representing a particle emitter in the game. It emits and
 * manages particles.
 *
 * \subsection emitter-example Example
 * ```cpp
 * // Create a particle emitter with a maximum of 200 particles
 * ParticleEmitter emitter(200);
 * // Set the position of the emitter
 * emitter.setPosition({0.0f, 0.0f, 0.0f});
 * // Set the emission type to Fountain
 * emitter.setEmissionType(ParticleEmissionType::Fountain);
 * // Set the direction of particle emission
 * emitter.setDirection({0.0f, 1.0f, 0.0f});
 * // Set the spawn radius of particles
 * emitter.setSpawnRadius(0.5f);
 * // Set the spawn rate to 20 particles per second
 * emitter.setSpawnRate(20.0f);
 * // Define particle settings
 * ParticleSettings settings;
 * settings.minLifetime = 1.0f;
 * settings.maxLifetime = 3.0f;
 * settings.minSize = 0.05f;
 * settings.maxSize = 0.1f;
 * settings.fadeSpeed = 0.5f;
 * settings.gravity = -9.81f;
 * settings.spread = 1.0f;
 * settings.speedVariation = 0.5f;
 * emitter.setParticleSettings(settings);
 * // Attach a texture to the particles
 * Texture particleTexture("path/to/particle_texture.png");
 * emitter.attachTexture(particleTexture);
 * emitter.enableTexture();
 * // Add the emitter to the scene
 * scene.addObject(&emitter);
 * ```
 *
 */
class ParticleEmitter : public GameObject {
  public:
    /**
     * @brief Allocates buffers and shader state necessary for particle
     * simulation.
     */
    void initialize() override;
    /**
     * @brief Renders all active particles with billboarding logic.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Updates particle lifetimes, spawns new particles, and applies
     * forces.
     */
    void update(Window &window) override;
    /**
     * @brief Updates the projection matrix used to align particles to the
     * camera frustum.
     */
    void setProjectionMatrix(const glm::mat4 &newProjection) override;
    /**
     * @brief Stores the view matrix so particles can face the camera.
     */
    void setViewMatrix(const glm::mat4 &newView) override;

    /**
     * @brief Constructs a new ParticleEmitter object.
     *
     * @param maxParticles The maximum number of particles the emitter can
     * handle.
     */
    ParticleEmitter(unsigned int maxParticles = 100);

    /**
     * @brief Binds a sprite texture that will be used for each particle.
     */
    void attachTexture(const Texture &tex) override;
    /**
     * @brief Sets a tint color that modulates the particle sprite.
     */
    void setColor(const Color &newColor) override;
    /**
     * @brief Function that enables the use of a texture for the particles.
     *
     */
    void enableTexture() { useTexture = true; };
    /**
     * @brief Function that disables the use of a texture for the particles.
     *
     */
    void disableTexture() { useTexture = false; };

    void setPosition(const Position3d &newPosition) override;
    /**
     * @brief Moves the emitter by a relative offset.
     */
    void move(const Position3d &deltaPosition) override;
    void setRotation(const Rotation3d &newRotation) override;
    void rotate(const Rotation3d &deltaRotation) override;
    /**
     * @brief Returns the emitter world-space origin.
     */
    Position3d getPosition() const override { return position; };
    Rotation3d getRotation() const override { return rotation; };
    /**
     * @brief Particle emitters do not cast scene shadows.
     */
    bool canCastShadows() const override { return false; };

    bool canUseDeferredRendering() override { return false; }

    /**
     * @brief Function that sets the type of particle emission.
     *
     * @param type The type of particle emission.
     */
    void setEmissionType(ParticleEmissionType type);
    /**
     * @brief The direction in which particles are emitted.
     *
     * @param dir The direction vector.
     */
    void setDirection(const Magnitude3d &dir);
    /**
     * @brief The radius around the emitter from which particles are spawned.
     *
     * @param radius The spawn radius.
     */
    void setSpawnRadius(float radius);
    /**
     * @brief How much particles are spawned each second.
     *
     * @param particlesPerSecond The number of particles to spawn per second.
     */
    void setSpawnRate(float particlesPerSecond);
    /**
     * @brief Sets the settings for the particles.
     *
     * @param particleSettings The particle settings to apply.
     */
    void setParticleSettings(const ParticleSettings &particleSettings);

    /**
     * @brief Emits particles once.
     *
     */
    void emitOnce();
    /**
     * @brief Emits particles continuously.
     *
     */
    void emitContinuously();
    /**
     * @brief Starts emitting particles.
     *
     */
    void startEmission();
    /**
     * @brief Stops emitting particles.
     *
     */
    void stopEmission();
    /**
     * @brief Emits a burst of particles.
     *
     * @param count The number of particles to emit in the burst.
     */
    void emitBurst(int count);

    /**
     * @brief Sets the spawn rate for the particles.
     *
     * @param rate The spawn rate in particles per second.
     */
    void setSpawnRate(int rate) { setSpawnRate(static_cast<float>(rate)); }

    /**
     * @brief The settings used for particle behavior and appearance.
     *
     */
    ParticleSettings settings;

  private:
    /** @brief Particle pool reused across emissions. */
    std::vector<Particle> particles;
    /** @brief Maximum capacity of the particle pool. */
    unsigned int maxParticles;
    /** @brief Number of currently active particles in the pool. */
    unsigned int activeParticleCount = 0;

    /** @brief Active emission pattern. */
    ParticleEmissionType emissionType = ParticleEmissionType::Fountain;
    /** @brief Primary direction used when spawning particles. */
    Magnitude3d direction = {0.0, 1.0, 0.0};
    /** @brief Radius around the emitter origin where particles can spawn. */
    float spawnRadius = 0.1f;
    /** @brief Continuous spawn rate in particles per second. */
    float spawnRate = 10.0f;

    float timeSinceLastEmission = 0.0f;
    bool isEmitting = true;
    bool doesEmitOnce = false;
    bool hasEmittedOnce = false;
    int burstCount = 0;

    /** @brief Drawing state used to render particle billboards. */
    std::shared_ptr<opal::DrawingState> vao = nullptr;
    std::shared_ptr<opal::Buffer> quadBuffer = nullptr;
    std::shared_ptr<opal::Buffer> instanceBuffer = nullptr;
    std::shared_ptr<opal::Buffer> indexBuffer = nullptr;
    ShaderProgram program;
    Texture texture;
    Color color = Color::white();
    bool useTexture = false;

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);

    Position3d position = {0.0, 0.0, 0.0};
    Rotation3d rotation = {0.0, 0.0, 0.0};
    std::optional<Position3d> firstCameraPosition = std::nullopt;

    void spawnParticle();
    void updateParticle(Particle &p, float deltaTime);
    Magnitude3d generateRandomVelocity();
    Position3d generateSpawnPosition();
    int findInactiveParticle();
    void activateParticle(int index);
};

#endif // ATLAS_PARTICLE_H
