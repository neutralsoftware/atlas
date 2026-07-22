/*
 illuminate.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Global illumination and light management for Atlas
 Copyright (c) 2026 Max Van den Eynde
*/

#ifndef PHOTON_ILLUMINATE_H
#define PHOTON_ILLUMINATE_H

#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstdint>

class CoreObject;
class Window;

namespace photon {

/**
 * @brief Spatial layout and atlas packing configuration for DDGI probes.
 */
struct ProbeSpace {
    /** @brief World-space origin of probe index (0,0,0). */
    Position3d originWorldSpace;
    /** @brief Distance between neighboring probes on each axis. */
    Position3d spacing;
    /** @brief Probe counts along X/Y/Z axes. */
    Vector3 probeCount;
    /** @brief Debug tint used by probe visualization tools. */
    Color debugColor;

    /** @brief Border texels used around each probe tile to reduce seams. */
    int textureBorderSize = 1;
    /** @brief Inner texel resolution per probe tile (without border). */
    int probeResolution = 6;
    /** @brief Number of probe tiles packed per atlas row. */
    int probesPerRow = 16;

    /** @brief Returns the total number of probes in the grid. */
    int totalProbes() const {
        return static_cast<int>(probeCount.x * probeCount.y * probeCount.z);
    }

    /** @brief Returns per-tile resolution including border texels. */
    int tileResolution() const {
        return probeResolution + (2 * textureBorderSize);
    }

    /** @brief Returns the number of rows required by the probe atlas. */
    int atlasRows() const {
        return (totalProbes() + probesPerRow - 1) / probesPerRow;
    }

    /** @brief Returns the packed atlas width in pixels. */
    int atlasWidth() const { return probesPerRow * tileResolution(); }

    /** @brief Returns the packed atlas height in pixels. */
    int atlasHeight() const { return atlasRows() * tileResolution(); }
};

/**
 * @brief Real-time path tracing helper that builds scene acceleration
 * structures and writes a progressive output texture.
 */
class PathTracing {
  public:
#ifdef METAL
    /** @brief Runs one path tracing pass into the active output texture. */
    bool render(const std::shared_ptr<opal::CommandBuffer> &commandBuffer,
                const std::shared_ptr<opal::Texture> &output,
                const std::shared_ptr<opal::Texture> &brightOutput);
    /** @brief Rebuilds BLAS/TLAS data for the current scene geometry. */
    bool buildAccelerationStructure(
        const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    /** @brief Uploads light lists used by path tracing shaders. */
    bool createLightBuffers();
    /** @brief Initializes compute pipelines and internal buffers. */
    void init();
    /** @brief Resizes path tracing output and history textures. */
    void resizeOutput(int width, int height);
    const std::string &getLastError() const { return lastError; }

    /** @brief Current frame output texture. */
    std::shared_ptr<Texture> pathTracingTexturePrev;

    /** @brief Rays traced per pixel each dispatch. */
    int raysPerPixel = 1;
    /** @brief Maximum bounce count for indirect transport. */
    int maxBounces = 6;
    /** @brief Scalar multiplier for indirect lighting contribution. */
    float indirectStrength = 1.0f;
    /** @brief Whether normal maps are evaluated during shading. */
    bool sampleNormalMaps = true;
    /** @brief Strength multiplier applied to sampled normal maps. */
    float normalMapStrength = 1.0f;

  private:
    std::shared_ptr<opal::Buffer> pointLights;
    std::shared_ptr<opal::Buffer> spotLights;
    std::shared_ptr<opal::Buffer> areaLights;

    std::shared_ptr<opal::Buffer> globalVertices;
    std::shared_ptr<opal::Buffer> globalIndices;
    std::shared_ptr<opal::Buffer> meshInfo;
    std::shared_ptr<opal::Buffer> materialBuffer;
    std::shared_ptr<opal::Buffer> instanceDataBuffer;
    std::shared_ptr<opal::Buffer> blasPrimitiveOffsets;
    std::vector<std::shared_ptr<opal::Texture>> materialTextures;
    std::vector<std::shared_ptr<opal::Texture>> materialTextureBindings;
    std::shared_ptr<opal::InstanceAccelerationStructure> sceneTLAS;
    std::shared_ptr<opal::Pipeline> pathTracingPipeline;
    std::shared_ptr<ShaderProgram> computePathTracer;
    std::array<std::shared_ptr<Texture>, 4> pathTracingAovTextures;
    std::shared_ptr<Texture> pathTracingHistoryGuide;
    std::unordered_map<int,
                       std::shared_ptr<opal::PrimitiveAccelerationStructure>>
        objectBLAS;
    std::vector<uint32_t> cachedBLASPrimitiveOffsets;
    std::vector<CoreObject *> cachedObjects;
    std::vector<CoreObject *> cachedSceneObjects;
    std::vector<glm::mat4> cachedInstanceTransforms;
    std::vector<uint64_t> cachedObjectStateHashes;
    std::vector<uint64_t> cachedSceneObjectStateHashes;
    uint64_t cachedLightHash = 0;

    int frameIndex = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    int interactiveFramesRemaining = 0;
    bool interactive = false;
    bool accelerationBuildFailed = false;
    std::string lastError;
    glm::mat4 cachedInvViewProj = glm::mat4(1.0f);
    glm::mat4 previousViewProj = glm::mat4(1.0f);
    glm::vec3 cachedDirectionalLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 cachedDirectionalLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float cachedDirectionalLightIntensity = -1.0f;
    glm::vec3 cachedAmbientColor = glm::vec3(-1.0f);
    float cachedAmbientIntensity = -1.0f;
    int cachedDirectionalLightCount = -1;
    uint64_t cachedSkyboxTextureId = 0;

    friend class ::Window;
#endif
};

class GlobalIllumination {
  public:
#ifdef METAL
    /** @brief Executes DDGI ray tracing and irradiance atlas writeback. */
    void render(const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    /** @brief Recomputes probe-space bounds based on scene extents. */
    void updateProbeLayout();
    /** @brief Initializes DDGI shaders, pipelines, and textures. */
    void init();

    /** @brief Current irradiance probe atlas sampled by shading passes. */
    std::shared_ptr<Texture> irradianceMap;
    /** @brief Previous irradiance atlas for temporal blending. */
    std::shared_ptr<Texture> irradianceMapPrev;
    std::shared_ptr<Texture> distanceMap;
    std::shared_ptr<Texture> distanceMapPrev;
    /** @brief Compute program that writes probe irradiance tiles to the atlas.
     */
    std::shared_ptr<ShaderProgram> giWriteShader;
    /** @brief Compute program that traces rays from probes into the scene. */
    std::shared_ptr<ShaderProgram> giRaytracingShader;
    /** @brief Pipeline used to write irradiance probe atlas data. */
    std::shared_ptr<opal::Pipeline> giPipeline;
    /** @brief Pipeline used for DDGI probe ray tracing. */
    std::shared_ptr<opal::Pipeline> giRaytracingPipeline;

    /** @brief GPU buffer storing per-ray radiance before atlas resolve. */
    std::shared_ptr<opal::Buffer> probeRadianceBuffer;
    /** @brief Byte capacity currently allocated for probeRadianceBuffer. */
    int probeRadianceCapacity = 0;

    std::shared_ptr<opal::Buffer> triangleBuffer;
    std::shared_ptr<opal::Buffer> materialBuffer;
    std::shared_ptr<opal::PrimitiveAccelerationStructure> sceneBLAS;
    std::shared_ptr<opal::InstanceAccelerationStructure> sceneTLAS;
    bool accelerationStructureDirty = false;

    /** @brief Active probe-space definition used for DDGI dispatch. */
    std::shared_ptr<ProbeSpace> probeSpace;

    /** @brief Source framebuffer used for atlas history copies. */
    std::shared_ptr<opal::Framebuffer> copySrcFramebuffer;
    /** @brief Destination framebuffer used for atlas history copies. */
    std::shared_ptr<opal::Framebuffer> copyDstFramebuffer;

    /**
     * @brief GPU-packed material description consumed by DDGI compute shaders.
     */
    struct DDGIMaterial {
        /** @brief Material identifier used by triangles. */
        int materialID;
        /** @brief Albedo texture slot index, or -1 if unused. */
        int albedoTextureIndex;
        /** @brief Normal texture slot index, or -1 if unused. */
        int normalTextureIndex;
        /** @brief Metallic texture slot index, or -1 if unused. */
        int metallicTextureIndex;

        /** @brief Roughness texture slot index, or -1 if unused. */
        int roughnessTextureIndex;
        /** @brief Ambient occlusion texture slot index, or -1 if unused. */
        int aoTextureIndex;
        /** @brief Scalar metallic factor. */
        float metallic;
        /** @brief Scalar roughness factor. */
        float roughness;
        /** @brief Scalar ambient occlusion factor. */
        float ao;
        /** @brief Emissive intensity multiplier. */
        float emissiveIntensity;

        /** @brief Base albedo color. */
        glm::vec3 albedo;
        float _pad0;

        /** @brief Emissive color. */
        glm::vec3 emissiveColor;
        float _pad1;
    };

    /**
     * @brief GPU-packed triangle payload consumed by DDGI ray tracing.
     */
    struct DDGITriangle {
        /** @brief Position of the first vertex in world space. */
        glm::vec4 v0;
        /** @brief Position of the second vertex in world space. */
        glm::vec4 v1;
        /** @brief Position of the third vertex in world space. */
        glm::vec4 v2;

        /** @brief Normal at the first vertex. */
        glm::vec4 n0;
        /** @brief Normal at the second vertex. */
        glm::vec4 n1;
        /** @brief Normal at the third vertex. */
        glm::vec4 n2;

        glm::vec4 uv0;
        glm::vec4 uv1;
        glm::vec4 uv2;

        glm::vec4 t0;
        glm::vec4 t1;
        glm::vec4 t2;

        glm::vec4 b0;
        glm::vec4 b1;
        glm::vec4 b2;

        /** @brief Material identifier referenced by this triangle. */
        int materialID;

        int pad[3];
    };

    /** @brief Flattened triangle data uploaded for DDGI tracing. */
    std::vector<DDGITriangle> triangles;
    /** @brief Material table uploaded for DDGI tracing. */
    std::vector<DDGIMaterial> materials;
    /** @brief Texture table referenced by DDGI materials. */
    std::vector<std::shared_ptr<opal::Texture>> materialTextures;

    /** @brief Baseline spacing scalar used when fitting probe grids. */
    float probeSpacing = 0.9f;

    /** @brief Rays traced per probe update. */
    int raysPerProbe = 64;
    /** @brief Maximum ray distance for DDGI probe tracing. */
    float maxRayDistance = 20.f;
    /** @brief Surface normal bias used to reduce self-intersections. */
    float normalBias = 0.05;
    /** @brief Temporal blending factor for probe history accumulation. */
    float hysteresis = 0.85;
    /** @brief Whether DDGI tracing evaluates normal maps. */
    bool sampleNormalMaps = false;
    /** @brief Strength applied to sampled normal maps when enabled. */
    float normalMapStrength = 1.0f;
    /** @brief Updates one probe group every N frames for staggered updates. */
    int probeUpdateStride = 4;
    /** @brief Monotonic frame counter used by DDGI dispatch logic. */
    int frameIndex = 0;
    /** @brief Cached hash to detect when probe layout must be rebuilt. */
    uint64_t cachedLayoutSignature = 0;
    /** @brief Indicates whether cachedLayoutSignature contains valid data. */
    bool hasCachedLayoutSignature = false;
#endif
};

} // namespace photon

#endif // PHOTON_ILLUMINATE_H
