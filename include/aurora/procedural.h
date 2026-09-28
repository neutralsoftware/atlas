//
// procedural.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Procedural Terrain Generation
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef AURORA_PROCEDURAL_H
#define AURORA_PROCEDURAL_H

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

/**
 * @brief Classic gradient-noise implementation used for soft terrain shapes.
 *
 * \subsection perlinnoise-example Example
 * ```cpp
 * // Generate Perlin noise for terrain heightmap
 * PerlinNoise perlin(12345); // Seed for reproducible results
 * float height = perlin.noise(10.5f, 20.3f);
 * // Apply to terrain mesh vertex
 * Position3d vertex(x, height * 50.0f, z); // Scale height by 50 units
 * ```
 */
struct PerlinNoise {
  private:
    static std::vector<int> p;

    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y);

    static std::vector<int> permutation;

  public:
    /**
     * @brief Builds a noise generator with an optional deterministic seed.
     *
     * @param seed Seed value used to shuffle gradients. Defaults to random.
     */
    PerlinNoise(unsigned int seed = 0);

    /**
     * @brief Produces a coherent noise value for the provided coordinates.
     *
     * @param x X coordinate to sample.
     * @param y Y coordinate to sample.
     * @return Smoothed noise value in the range [-1, 1].
     */
    float noise(float x, float y) const;
};

/**
 * @brief 2D Simplex noise helper that delivers sharper ridges than Perlin.
 */
class SimplexNoise {
  private:
    static const int grad3[12][3];
    static int fastfloor(float x);
    static float dot(const int *g, float x, float y);

  public:
    /**
     * @brief Computes Simplex noise for the supplied coordinates.
     *
     * @param xin X coordinate to sample.
     * @param yin Y coordinate to sample.
     * @return Noise value roughly in the range [-1, 1].
     */
    static float noise(float xin, float yin);
};

/**
 * @brief Worley (cellular) noise generator used for island and crater shapes.
 */
class WorleyNoise {
  private:
    int numPoints;
    std::vector<std::pair<float, float>> featurePoints;

  public:
    /**
     * @brief Creates a Worley noise generator with a desired feature count.
     *
     * @param numPoints Number of feature points to scatter.
     * @param seed Seed controlling point distribution.
     */
    WorleyNoise(int numPoints, unsigned int seed = 0);

    /**
     * @brief Samples the distance to the closest feature point.
     *
     * @param x X coordinate to sample.
     * @param y Y coordinate to sample.
     * @return Normalized cellular noise value.
     */
    float noise(float x, float y) const;
};

/**
 * @brief Fractal sums of Perlin noise used for more complex landscapes.
 *
 * \subsection fractalnoise-example Example
 * ```cpp
 * // Create fractal noise for mountainous terrain
 * FractalNoise fractal(6, 0.5f); // 6 octaves, 0.5 persistence
 * for (int z = 0; z < gridSize; z++) {
 *     for (int x = 0; x < gridSize; x++) {
 *         float height = fractal.noise(x * 0.01f, z * 0.01f);
 *         heightmap[z][x] = height * 100.0f; // Scale to world units
 *     }
 * }
 * ```
 */
class FractalNoise {
  private:
    PerlinNoise base;
    int octaves;
    float persistence;

  public:
    /**
     * @brief Configures the octave count and persistence of the fractal.
     *
     * @param o Number of octaves to accumulate.
     * @param p Persistence factor applied per octave.
     */
    FractalNoise(int o, float p);

    /**
     * @brief Samples compounded Perlin noise with the configured settings.
     *
     * @param x X coordinate to sample.
     * @param y Y coordinate to sample.
     * @return Fractal noise value.
     */
    float noise(float x, float y) const;
};

/**
 * @brief Convenience namespace bundling the available noise algorithms.
 */
class Noise {
  public:
    /**
     * @brief Samples Perlin noise using the global seed when enabled.
     */
    static float perlin(float x, float y);
    /**
     * @brief Samples Simplex noise using the global seed when enabled.
     */
    static float simplex(float x, float y);
    /**
     * @brief Samples Worley noise using the global seed when enabled.
     */
    static float worley(float x, float y);
    /**
     * @brief Samples a fractal sum of Perlin noise with runtime parameters.
     *
     * @param x X coordinate to sample.
     * @param y Y coordinate to sample.
     * @param octaves Number of octaves to accumulate.
     * @param persistence Persistence factor per octave.
     */
    static float fractal(float x, float y, int octaves, float persistence);

    /** @brief Global seed value used by noise helpers when enabled. */
    static float seed;
    /** @brief Indicates whether the global seed has already been initialized.
     */
    static bool initializedSeed;
    /** @brief When true, helper functions use the shared `seed` value. */
    static bool useSeed;
};

class Terrain;
/**
 * @brief Abstract interface that translates 2D coordinates into heights.
 */
class TerrainGenerator {
  public:
    virtual ~TerrainGenerator() = default;

    /**
     * @brief Evaluates the height of the terrain at the requested position.
     *
     * @param x X coordinate in world or heightmap space.
     * @param y Y coordinate in world or heightmap space.
     * @return Height value to assign to the terrain mesh.
     */
    virtual float generateHeight(float x, float y) = 0;

    /**
     * @brief Optional hook invoked when the generator is attached to a terrain.
     */
    virtual void applyTo(Terrain &) const {};
};

/**
 * @brief Low-frequency noise generator used for rolling hills and plains.
 *
 * \subsection terraingenerator-example Example
 * ```cpp
 * // Use individual generators
 * HillGenerator hills(0.02f, 15.0f);
 * MountainGenerator mountains(8.0f, 120.0f, 6, 0.6f);
 *
 * // Or combine multiple generators
 * CompoundGenerator combined;
 * combined.addGenerator(PlainGenerator(0.03f, 3.0f));
 * combined.addGenerator(MountainGenerator(10.0f, 80.0f));
 * combined.addGenerator(HillGenerator(0.01f, 20.0f));
 *
 * // Apply to terrain
 * Terrain terrain;
 * combined.applyTo(terrain);
 *
 * // Or sample directly
 * float heightValue = combined.generateHeight(100.0f, 50.0f);
 * ```
 */
class HillGenerator : public TerrainGenerator {
  private:
    float scale;
    float amplitude;

  public:
    /**
     * @brief Configures the hill generator parameters.
     *
     * @param scale Noise scale controlling hill size.
     * @param amplitude Maximum height contribution.
     */
    HillGenerator(float scale = 0.01f, float amplitude = 10.0f)
        : scale(scale), amplitude(amplitude) {}

    float getScale() const { return scale; }
    float getAmplitude() const { return amplitude; }

    /**
     * @brief Samples Perlin noise and remaps it to a gentle hill profile.
     */
    float generateHeight(float x, float y) override {
        float noise = Noise::perlin(x / scale, y / scale);
        return (noise + 1.0f) * 0.5f * amplitude / 10.0f;
    }
};

/**
 * @brief Fractal noise generator that yields rugged mountainous features.
 */
class MountainGenerator : public TerrainGenerator {
  private:
    float scale;
    float amplitude;
    int octaves;
    float persistence;

  public:
    MountainGenerator(float scale = 10.f, float amplitude = 100.0f,
                      int octaves = 5, float persistence = 0.5f)
        : scale(scale), amplitude(amplitude), octaves(octaves),
          persistence(persistence) {}

    float getScale() const { return scale; }
    float getAmplitude() const { return amplitude; }
    int getOctaves() const { return octaves; }
    float getPersistence() const { return persistence; }

    /**
     * @brief Produces jagged mountain heights using fractal noise.
     */
    float generateHeight(float x, float y) override {
        float noise =
            Noise::fractal(x * scale, y * scale, octaves, persistence);
        float height = noise;
        return height * amplitude;
    }
};

/**
 * @brief Gentle noise generator producing subtle undulating plains.
 */
class PlainGenerator : public TerrainGenerator {
  private:
    float amplitude;
    float scale;

  public:
    PlainGenerator(float scale = 0.02f, float amplitude = 2.0f)
        : amplitude(amplitude), scale(scale) {}

    float getScale() const { return scale; }
    float getAmplitude() const { return amplitude; }

    /**
     * @brief Returns low-amplitude Perlin noise suited for flat regions.
     */
    float generateHeight(float x, float y) override {
        float noise = Noise::perlin(x * scale, y * scale);
        return (noise + 1.0f) * 0.5f * amplitude / 2.0f;
    }
};

/**
 * @brief Worley-noise-based generator that mimics coastline islands.
 */
class IslandGenerator : public TerrainGenerator {
  private:
    int numFeatures;
    float scale;

  public:
    IslandGenerator(int numFeatures = 10, float scale = 0.01f)
        : numFeatures(numFeatures), scale(scale) {}

    int getNumFeatures() const { return numFeatures; }
    float getScale() const { return scale; }

    /**
     * @brief Produces island-style plateaus using cellular noise.
     */
    float generateHeight(float x, float y) override {
        WorleyNoise worley(numFeatures);
        float noise = worley.noise(x * scale, y * scale);
        return std::clamp(noise, 0.0f, 1.0f);
    }
};

/**
 * @brief Aggregates multiple terrain generators and sums their contributions.
 */
class CompoundGenerator : public TerrainGenerator {
  private:
    std::vector<std::shared_ptr<TerrainGenerator>> generators;

  public:
    /**
     * @brief Adds another generator to the compound stack.
     *
     * @tparam T Type of generator to store.
     * @param gen Instance that will be copied into the stack.
     */
    template <typename T>
        requires std::is_base_of<TerrainGenerator, T>::value
    void addGenerator(T gen) {
        generators.push_back(std::make_shared<T>(std::move(gen)));
    }

    const std::vector<std::shared_ptr<TerrainGenerator>> &
    getGenerators() const {
        return generators;
    }

    /**
     * @brief Sums the height contributions of all registered generators.
     */
    float generateHeight(float x, float y) override {
        float height = 0.0f;
        for (auto &g : generators) {
            height += g->generateHeight(x, y);
        }
        return height;
    }
};

#endif // AURORA_PROCEDURAL_H
