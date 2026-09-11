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
    float _pad1[2];

    float transmittance;
    float ior;
    float reflectivity;
    float _pad2;
    packed_float2 textureScale;
    packed_float2 textureOffset;
};

static_assert(sizeof(Material) == 112);
static_assert(__builtin_offsetof(Material, emissiveColor) == 32);
static_assert(__builtin_offsetof(Material, albedoTextureIndex) == 48);
static_assert(__builtin_offsetof(Material, transmittance) == 80);
static_assert(__builtin_offsetof(Material, textureScale) == 96);

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
};

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
};

static_assert(sizeof(SceneData) == 160);
static_assert(__builtin_offsetof(SceneData, atmosphereSunDirection) == 48);
static_assert(__builtin_offsetof(SceneData, atmosphereSunIntensity) == 64);
static_assert(__builtin_offsetof(SceneData, atmosphereSunColor) == 80);
static_assert(__builtin_offsetof(SceneData, pixelStride) == 96);
static_assert(__builtin_offsetof(SceneData, ambientColor) == 112);
static_assert(__builtin_offsetof(SceneData, accumulationFrameLimit) == 132);
static_assert(__builtin_offsetof(SceneData, numEmissiveTriangles) == 140);
static_assert(__builtin_offsetof(SceneData, bloomThreshold) == 144);

