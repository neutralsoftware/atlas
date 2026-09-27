#pragma once

#include <metal_stdlib>
using namespace metal;

constant uint PHOTON_FEATURE_DIRECT_LIGHTING = 1u << 0;
constant uint PHOTON_FEATURE_SHADOWS = 1u << 1;
constant uint PHOTON_FEATURE_ENVIRONMENT_LIGHTING = 1u << 2;
constant uint PHOTON_FEATURE_EMISSIVE_LIGHTING = 1u << 3;
constant uint PHOTON_FEATURE_INDIRECT_LIGHTING = 1u << 4;
constant uint PHOTON_FEATURE_TRANSMISSION = 1u << 5;
constant uint PHOTON_FEATURE_DISPERSION = 1u << 6;
constant uint PHOTON_FEATURE_IRIDESCENCE = 1u << 7;
constant uint PHOTON_FEATURE_CAUSTICS = 1u << 8;
constant uint PHOTON_FEATURE_VOLUMES = 1u << 9;
constant uint PHOTON_FEATURE_SUBSURFACE = 1u << 10;
constant uint PHOTON_FEATURE_NORMAL_MAPS = 1u << 11;
constant uint PHOTON_FEATURE_MATERIAL_TEXTURES = 1u << 12;
constant uint PHOTON_FEATURE_ALPHA_TRANSPARENCY = 1u << 13;

struct CameraUniforms {
    float4x4 invViewProj;
    float4x4 prevViewProj;
    float3 camPos;
    float _pad0;
};

struct Material {
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
    packed_float3 emissiveColor;
    float _pad0;
    int albedoTextureIndex;
    int normalTextureIndex;
    int metallicTextureIndex;
    int roughnessTextureIndex;
    int aoTextureIndex;
    int opacityTextureIndex;
    float abbeNumber;
    float useNormalMap;

    float transmittance;
    float ior;
    float reflectivity;
    float _pad2;
    packed_float2 textureScale;
    packed_float2 textureOffset;

    packed_float3 attenuationColor;
    float attenuationDistance;

    // Volume properties
    int isVolume;
    float volumeDensity;
    packed_float3 volumeAbsorptionColor;
    float volumeAbsorptionStrength;
    packed_float3 volumeScatteringColor;
    float volumeScatteringStrength;
    float volumeAnisotropy;
    packed_float3 volumeEmissionColor;
    float volumeEmissionStrength;
    float _pad3;

    // Iridescence properties
    float iridescenceFactor;
    float iridescenceIor;
    float iridescenceThickness;  // in nanometers
    float iridescenceAbbeNumber; // optional, for spectral dispersion

    float subsurfaceWeight;
    float subsurfaceScale;
    float subsurfaceAnisotropy;
    float _pad4;
    packed_float3 subsurfaceColor;
    float _pad5;
    packed_float3 subsurfaceRadius;
    float _pad6;
};

static_assert(sizeof(Material) == 256);
static_assert(__builtin_offsetof(Material, emissiveColor) == 32);
static_assert(__builtin_offsetof(Material, albedoTextureIndex) == 48);
static_assert(__builtin_offsetof(Material, transmittance) == 80);
static_assert(__builtin_offsetof(Material, textureScale) == 96);
static_assert(__builtin_offsetof(Material, isVolume) == 128);
static_assert(__builtin_offsetof(Material, volumeAbsorptionColor) == 136);
static_assert(__builtin_offsetof(Material, volumeEmissionStrength) == 184);
static_assert(__builtin_offsetof(Material, iridescenceFactor) == 192);
static_assert(__builtin_offsetof(Material, iridescenceIor) == 196);
static_assert(__builtin_offsetof(Material, iridescenceThickness) == 200);
static_assert(__builtin_offsetof(Material, iridescenceAbbeNumber) == 204);
static_assert(__builtin_offsetof(Material, subsurfaceWeight) == 208);
static_assert(__builtin_offsetof(Material, subsurfaceColor) == 224);
static_assert(__builtin_offsetof(Material, subsurfaceRadius) == 240);

struct VertexData {
    packed_float3 position;
    packed_float3 normal;
    packed_float2 uv;
    packed_float3 tangent;
    packed_float3 bitangent;
};

struct InstanceData {
    float4x4 model;
    float4 normalCol0;
    float4 normalCol1;
    float4 normalCol2;
};

struct DirectionalLightData {
    float3 direction;
    float intensity;
    float3 color;
    float _pad0;
};

struct PointLight {
    packed_float3 position;
    float intensity;
    packed_float3 color;
    float range;
};

struct SpotLight {
    packed_float3 position;
    float intensity;

    packed_float3 direction;
    float innerCos;
    packed_float3 color;
    float outerCos;

    float range;
    float _pad0[3];
};

struct AreaLight {
    packed_float3 position;
    float intensity;

    packed_float3 right;
    float halfWidth;

    packed_float3 up;
    float halfHeight;

    packed_float3 color;
    float twoSided;
    float emissionCos;
    float _pad[3];
};

static_assert(sizeof(AreaLight) == 80);

struct EmissiveTriangle {
    float4 p0;
    float4 p1;
    float4 p2;
    float4 normal;
    packed_float3 emission;
    float area;
    float cdf;
    float selectionPdf;
    float2 _pad;
};

static_assert(sizeof(EmissiveTriangle) == 96);

struct DirectLightSample {
    uint type;
    uint index;
    float2 uv;
};

struct DirectReservoir {
    DirectLightSample sample;
    float target;
    float weightSum;
    float sampleCount;
    float age;
};

struct SceneData {
    uint numDirectionalLights;
    uint numPointLights;
    uint numSpotLights;
    uint numAreaLights;

    uint frameIndex;
    uint raysPerPixel;
    uint maxBounces;
    float indirectStrength;
    float ambientIntensity;
    uint materialTextureCount;
    uint atmosphereEnabled;
    float atmosphereSunSize;
    float3 atmosphereSunDirection;
    float atmosphereSunIntensity;
    float3 atmosphereSunColor;
    uint pixelStride;
    float3 ambientColor;
    uint environmentEnabled;
    uint accumulationFrameLimit;
    float fireflyClamp;
    uint numEmissiveTriangles;
    float bloomThreshold;
    uint causticsEnabled;
    uint atmosphereSkyEnabled;
    uint featureFlags;
    float4 cloudSettings;
    float4 cloudLighting;
    uint cloudsEnabled;
};

static_assert(sizeof(SceneData) == 208);
static_assert(__builtin_offsetof(SceneData, atmosphereSunDirection) == 48);
static_assert(__builtin_offsetof(SceneData, atmosphereSunIntensity) == 64);
static_assert(__builtin_offsetof(SceneData, atmosphereSunColor) == 80);
static_assert(__builtin_offsetof(SceneData, pixelStride) == 96);
static_assert(__builtin_offsetof(SceneData, ambientColor) == 112);
static_assert(__builtin_offsetof(SceneData, accumulationFrameLimit) == 132);
static_assert(__builtin_offsetof(SceneData, numEmissiveTriangles) == 140);
static_assert(__builtin_offsetof(SceneData, bloomThreshold) == 144);
static_assert(__builtin_offsetof(SceneData, causticsEnabled) == 148);
static_assert(__builtin_offsetof(SceneData, atmosphereSkyEnabled) == 152);
static_assert(__builtin_offsetof(SceneData, featureFlags) == 156);
static_assert(__builtin_offsetof(SceneData, cloudSettings) == 160);
static_assert(__builtin_offsetof(SceneData, cloudLighting) == 176);
static_assert(__builtin_offsetof(SceneData, cloudsEnabled) == 192);

bool photonFeatureEnabled(constant SceneData &sceneData, uint feature) {
    return (sceneData.featureFlags & feature) != 0;
}

struct CausticPhoton {
    float4 positionWavelength;
    float4 normalPower;
    float4 incomingObject;
};

struct CausticSettings {
    float4 bounds;
    float radius;
    uint seed;
    float launchDistance;
};

static_assert(sizeof(CausticPhoton) == 48);
static_assert(sizeof(CausticSettings) == 32);

constant uint CAUSTIC_PHOTON_COUNT = 1u << 19;
constant uint CAUSTIC_BUCKET_COUNT = 1u << 17;
constant uint CAUSTIC_BUCKET_SAMPLES = 32;
