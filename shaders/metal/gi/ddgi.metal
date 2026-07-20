#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

struct ProbeSpace {
    float3 origin;
    float _pad0;

    float3 spacing;
    float _pad1;

    float3 probeCount;
    float _pad2;

    float4 debugColor;

    float4 atlasParams;
};

struct RaytracingSettings {
    uint raysPerProbe;
    float maxRayDistance;
    float normalBias;
    float hysteresis;

    uint frameIndex;
    uint probeUpdateOffset;
    uint probeUpdateStride;
    uint probeUpdateCount;
    float3 skyColor;
    uint useSkybox;
};

struct Material {
    int materialID;
    int albedoTextureIndex;
    int normalTextureIndex;
    int metallicTextureIndex;
    int roughnessTextureIndex;
    int aoTextureIndex;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;

    packed_float3 albedo;
    float _pad0;

    packed_float3 emissiveColor;
    float _pad1;
};

struct Triangle {
    float4 v0;
    float4 v1;
    float4 v2;
    float4 n0;
    float4 n1;
    float4 n2;

    float4 uv0;
    float4 uv1;
    float4 uv2;

    float4 t0;
    float4 t1;
    float4 t2;

    float4 b0;
    float4 b1;
    float4 b2;

    int materialID;
    int padding[3];
};

struct SceneCounts {
    uint triCount;
    uint materialCount;
    uint directionalLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint areaLightCount;
    uint textureCount;
    uint _pad0;
};

struct DirectionalLight {
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct PointLight {
    packed_float3 position;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct SpotLight {
    packed_float3 position;
    float _pad1;
    packed_float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    packed_float3 diffuse;
    float _pad5;
    packed_float3 specular;
    float _pad6;
};

struct AreaLight {
    packed_float3 position;
    float _pad1;
    packed_float3 right;
    float _pad2;
    packed_float3 up;
    float _pad3;
    float2 size;
    float _pad4;
    float _pad5;
    packed_float3 diffuse;
    float _pad6;
    packed_float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

struct Hit {
    float t;
    float3 n;
    float2 uv;
    float3 tangent;
    float3 bitangent;
    int materialID;
    uint triIndex;
    uint hit;
    uint _pad0;
};

static inline bool rayTriangleMT(float3 ro, float3 rd, float3 v0, float3 v1,
                                 float3 v2, thread float &t, thread float &u,
                                 thread float &v) {
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 p = cross(rd, e2);
    float det = dot(e1, p);

    if (fabs(det) < 1e-8f)
        return false;
    float invDet = 1.0f / det;

    float3 s = ro - v0;
    u = dot(s, p) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    float3 q = cross(s, e1);
    v = dot(rd, q) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = dot(e2, q) * invDet;
    return t > 0.0f;
}

static inline Hit traceScene(float3 ro, float3 rd, device const Triangle *tris,
                             instance_acceleration_structure sceneAS,
                             float maxDistance) {
    Hit best;
    best.t = INFINITY;
    best.hit = 0u;
    best.materialID = -1;
    best.n = float3(0.0f, 1.0f, 0.0f);
    best.uv = float2(0.0f);
    best.tangent = float3(1.0f, 0.0f, 0.0f);
    best.bitangent = float3(0.0f, 0.0f, 1.0f);
    best.triIndex = 0u;
    best._pad0 = 0u;

    intersector<triangle_data, instancing> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.set_triangle_cull_mode(triangle_cull_mode::none);
    ray query;
    query.origin = ro;
    query.direction = rd;
    query.min_distance = 0.0001f;
    query.max_distance = max(maxDistance, query.min_distance);
    auto intersection = isect.intersect(query, sceneAS, 0xFF);
    if (intersection.type != intersection_type::none) {
        uint i = intersection.primitive_id;
        float2 bary = intersection.triangle_barycentric_coord;
        float u = bary.x;
        float v = bary.y;
        float w = 1.0f - u - v;
        best.t = intersection.distance;
        best.n = normalize(tris[i].n0.xyz * w + tris[i].n1.xyz * u +
                           tris[i].n2.xyz * v);
        best.uv = tris[i].uv0.xy * w + tris[i].uv1.xy * u + tris[i].uv2.xy * v;
        float3 tRaw = tris[i].t0.xyz * w + tris[i].t1.xyz * u + tris[i].t2.xyz * v;
        float tLen2 = dot(tRaw, tRaw);
        best.tangent = (tLen2 > 1e-10f) ? tRaw * rsqrt(tLen2)
                                         : float3(1.0f, 0.0f, 0.0f);
        float3 bRaw = tris[i].b0.xyz * w + tris[i].b1.xyz * u + tris[i].b2.xyz * v;
        float bLen2 = dot(bRaw, bRaw);
        best.bitangent = (bLen2 > 1e-10f) ? bRaw * rsqrt(bLen2)
                                           : float3(0.0f, 0.0f, 1.0f);
        best.materialID = tris[i].materialID;
        best.triIndex = i;
        best.hit = 1u;
    }

    return best;
}

static inline uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

static inline float rand01(thread uint &state) {
    state = wangHash(state);
    return (float)(state & 0x00FFFFFFu) / 16777216.0f;
}

static inline uint3 probeCoordFromIndex(uint idx, uint3 counts) {
    uint xy = counts.x * counts.y;
    uint z = idx / xy;
    uint rem = idx - z * xy;
    uint y = rem / counts.x;
    uint x = rem - y * counts.x;
    return uint3(x, y, z);
}

static inline float3 safeNormalize(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10f) {
        return v * rsqrt(len2);
    }
    return fallback;
}

constexpr sampler materialTexSampler(coord::normalized, address::repeat,
                                     filter::linear, mip_filter::linear);

static inline float4 sampleMaterialTexture(
    int textureIndex, float2 uv, texture2d<float> materialTexture0,
    texture2d<float> materialTexture1, texture2d<float> materialTexture2,
    texture2d<float> materialTexture3, texture2d<float> materialTexture4,
    texture2d<float> materialTexture5, texture2d<float> materialTexture6,
    texture2d<float> materialTexture7, texture2d<float> materialTexture8,
    texture2d<float> materialTexture9, texture2d<float> materialTexture10,
    texture2d<float> materialTexture11, texture2d<float> materialTexture12,
    texture2d<float> materialTexture13, texture2d<float> materialTexture14,
    texture2d<float> materialTexture15, texture2d<float> materialTexture16,
    texture2d<float> materialTexture17, texture2d<float> materialTexture18,
    texture2d<float> materialTexture19, texture2d<float> materialTexture20,
    texture2d<float> materialTexture21, texture2d<float> materialTexture22,
    texture2d<float> materialTexture23, texture2d<float> materialTexture24,
    texture2d<float> materialTexture25, texture2d<float> materialTexture26,
    texture2d<float> materialTexture27, texture2d<float> materialTexture28,
    texture2d<float> materialTexture29, texture2d<float> materialTexture30,
    texture2d<float> materialTexture31, texture2d<float> materialTexture32,
    texture2d<float> materialTexture33, texture2d<float> materialTexture34,
    texture2d<float> materialTexture35, texture2d<float> materialTexture36,
    texture2d<float> materialTexture37, texture2d<float> materialTexture38,
    texture2d<float> materialTexture39, texture2d<float> materialTexture40,
    texture2d<float> materialTexture41, texture2d<float> materialTexture42,
    texture2d<float> materialTexture43, texture2d<float> materialTexture44,
    texture2d<float> materialTexture45, texture2d<float> materialTexture46,
    texture2d<float> materialTexture47) {
    switch (textureIndex) {
    case 0:
        return materialTexture0.sample(materialTexSampler, uv);
    case 1:
        return materialTexture1.sample(materialTexSampler, uv);
    case 2:
        return materialTexture2.sample(materialTexSampler, uv);
    case 3:
        return materialTexture3.sample(materialTexSampler, uv);
    case 4:
        return materialTexture4.sample(materialTexSampler, uv);
    case 5:
        return materialTexture5.sample(materialTexSampler, uv);
    case 6:
        return materialTexture6.sample(materialTexSampler, uv);
    case 7:
        return materialTexture7.sample(materialTexSampler, uv);
    case 8:
        return materialTexture8.sample(materialTexSampler, uv);
    case 9:
        return materialTexture9.sample(materialTexSampler, uv);
    case 10:
        return materialTexture10.sample(materialTexSampler, uv);
    case 11:
        return materialTexture11.sample(materialTexSampler, uv);
    case 12:
        return materialTexture12.sample(materialTexSampler, uv);
    case 13:
        return materialTexture13.sample(materialTexSampler, uv);
    case 14:
        return materialTexture14.sample(materialTexSampler, uv);
    case 15:
        return materialTexture15.sample(materialTexSampler, uv);
    case 16:
        return materialTexture16.sample(materialTexSampler, uv);
    case 17:
        return materialTexture17.sample(materialTexSampler, uv);
    case 18:
        return materialTexture18.sample(materialTexSampler, uv);
    case 19:
        return materialTexture19.sample(materialTexSampler, uv);
    case 20:
        return materialTexture20.sample(materialTexSampler, uv);
    case 21:
        return materialTexture21.sample(materialTexSampler, uv);
    case 22:
        return materialTexture22.sample(materialTexSampler, uv);
    case 23:
        return materialTexture23.sample(materialTexSampler, uv);
    case 24:
        return materialTexture24.sample(materialTexSampler, uv);
    case 25:
        return materialTexture25.sample(materialTexSampler, uv);
    case 26:
        return materialTexture26.sample(materialTexSampler, uv);
    case 27:
        return materialTexture27.sample(materialTexSampler, uv);
    case 28:
        return materialTexture28.sample(materialTexSampler, uv);
    case 29:
        return materialTexture29.sample(materialTexSampler, uv);
    case 30:
        return materialTexture30.sample(materialTexSampler, uv);
    case 31:
        return materialTexture31.sample(materialTexSampler, uv);
    case 32:
        return materialTexture32.sample(materialTexSampler, uv);
    case 33:
        return materialTexture33.sample(materialTexSampler, uv);
    case 34:
        return materialTexture34.sample(materialTexSampler, uv);
    case 35:
        return materialTexture35.sample(materialTexSampler, uv);
    case 36:
        return materialTexture36.sample(materialTexSampler, uv);
    case 37:
        return materialTexture37.sample(materialTexSampler, uv);
    case 38:
        return materialTexture38.sample(materialTexSampler, uv);
    case 39:
        return materialTexture39.sample(materialTexSampler, uv);
    case 40:
        return materialTexture40.sample(materialTexSampler, uv);
    case 41:
        return materialTexture41.sample(materialTexSampler, uv);
    case 42:
        return materialTexture42.sample(materialTexSampler, uv);
    case 43:
        return materialTexture43.sample(materialTexSampler, uv);
    case 44:
        return materialTexture44.sample(materialTexSampler, uv);
    case 45:
        return materialTexture45.sample(materialTexSampler, uv);
    case 46:
        return materialTexture46.sample(materialTexSampler, uv);
    case 47:
        return materialTexture47.sample(materialTexSampler, uv);
    default:
        break;
    }
    return float4(0.0f);
}

static inline void resolveMaterialParameters(
    device const Material *materials, int materialID, uint materialCount,
    float2 uv, uint textureCount, texture2d<float> materialTexture0,
    texture2d<float> materialTexture1, texture2d<float> materialTexture2,
    texture2d<float> materialTexture3, texture2d<float> materialTexture4,
    texture2d<float> materialTexture5, texture2d<float> materialTexture6,
    texture2d<float> materialTexture7, texture2d<float> materialTexture8,
    texture2d<float> materialTexture9, texture2d<float> materialTexture10,
    texture2d<float> materialTexture11, texture2d<float> materialTexture12,
    texture2d<float> materialTexture13, texture2d<float> materialTexture14,
    texture2d<float> materialTexture15, texture2d<float> materialTexture16,
    texture2d<float> materialTexture17, texture2d<float> materialTexture18,
    texture2d<float> materialTexture19, texture2d<float> materialTexture20,
    texture2d<float> materialTexture21, texture2d<float> materialTexture22,
    texture2d<float> materialTexture23, texture2d<float> materialTexture24,
    texture2d<float> materialTexture25, texture2d<float> materialTexture26,
    texture2d<float> materialTexture27, texture2d<float> materialTexture28,
    texture2d<float> materialTexture29, texture2d<float> materialTexture30,
    texture2d<float> materialTexture31, texture2d<float> materialTexture32,
    texture2d<float> materialTexture33, texture2d<float> materialTexture34,
    texture2d<float> materialTexture35, texture2d<float> materialTexture36,
    texture2d<float> materialTexture37, texture2d<float> materialTexture38,
    texture2d<float> materialTexture39, texture2d<float> materialTexture40,
    texture2d<float> materialTexture41, texture2d<float> materialTexture42,
    texture2d<float> materialTexture43, texture2d<float> materialTexture44,
    texture2d<float> materialTexture45, texture2d<float> materialTexture46,
    texture2d<float> materialTexture47, thread float3 &albedo,
    thread float &metallic, thread float &roughness, thread float &ao,
    thread float3 &emissive, thread int &normalTextureIndex,
    thread float &normalStrength) {
    albedo = float3(0.7f);
    metallic = 0.0f;
    roughness = 0.6f;
    ao = 1.0f;
    emissive = float3(0.0f);
    normalTextureIndex = -1;
    normalStrength = 0.0f;

    if (materialID < 0 || uint(materialID) >= materialCount) {
        return;
    }

    const Material mat = materials[materialID];
    albedo = clamp(float3(mat.albedo), float3(0.0f), float3(1.0f));
    metallic = clamp(mat.metallic, 0.0f, 1.0f);
    roughness = clamp(mat.roughness, 0.05f, 1.0f);
    ao = clamp(mat.ao, 0.0f, 1.0f);
    emissive = float3(mat.emissiveColor) * max(mat.emissiveIntensity, 0.0f);
    normalStrength = max(mat._pad0, 0.0f);
    bool useNormalMap = mat._pad1 > 0.5f && normalStrength > 0.0f;
    normalTextureIndex = useNormalMap ? mat.normalTextureIndex : -1;

    if (mat.albedoTextureIndex >= 0 && uint(mat.albedoTextureIndex) < textureCount) {
        albedo = clamp(sampleMaterialTexture(
                           mat.albedoTextureIndex, uv, materialTexture0,
                           materialTexture1, materialTexture2, materialTexture3,
                           materialTexture4, materialTexture5, materialTexture6,
                           materialTexture7, materialTexture8, materialTexture9,
                           materialTexture10, materialTexture11,
                           materialTexture12, materialTexture13,
                           materialTexture14, materialTexture15,
                           materialTexture16, materialTexture17,
                           materialTexture18, materialTexture19,
                           materialTexture20, materialTexture21,
                           materialTexture22, materialTexture23,
                           materialTexture24, materialTexture25,
                           materialTexture26, materialTexture27,
                           materialTexture28, materialTexture29,
                           materialTexture30, materialTexture31,
                           materialTexture32, materialTexture33,
                           materialTexture34, materialTexture35,
                           materialTexture36, materialTexture37,
                           materialTexture38, materialTexture39,
                           materialTexture40, materialTexture41,
                           materialTexture42, materialTexture43,
                           materialTexture44, materialTexture45,
                           materialTexture46, materialTexture47)
                           .xyz,
                       float3(0.0f), float3(1.0f));
    }
    if (mat.metallicTextureIndex >= 0 &&
        uint(mat.metallicTextureIndex) < textureCount) {
        float4 metallicSample = sampleMaterialTexture(
            mat.metallicTextureIndex, uv, materialTexture0, materialTexture1,
            materialTexture2, materialTexture3, materialTexture4,
            materialTexture5, materialTexture6, materialTexture7,
            materialTexture8, materialTexture9, materialTexture10,
            materialTexture11, materialTexture12, materialTexture13,
            materialTexture14, materialTexture15, materialTexture16,
            materialTexture17, materialTexture18, materialTexture19,
            materialTexture20, materialTexture21, materialTexture22,
            materialTexture23, materialTexture24, materialTexture25,
            materialTexture26, materialTexture27, materialTexture28,
            materialTexture29, materialTexture30, materialTexture31,
            materialTexture32, materialTexture33, materialTexture34,
            materialTexture35, materialTexture36, materialTexture37,
            materialTexture38, materialTexture39, materialTexture40,
            materialTexture41, materialTexture42, materialTexture43,
            materialTexture44, materialTexture45, materialTexture46,
            materialTexture47);
        float metallicValue = metallicSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            metallicValue = metallicSample.z;
        }
        metallic *= clamp(metallicValue, 0.0f, 1.0f);
    }
    if (mat.roughnessTextureIndex >= 0 &&
        uint(mat.roughnessTextureIndex) < textureCount) {
        float4 roughnessSample = sampleMaterialTexture(
            mat.roughnessTextureIndex, uv, materialTexture0, materialTexture1,
            materialTexture2, materialTexture3, materialTexture4,
            materialTexture5, materialTexture6, materialTexture7,
            materialTexture8, materialTexture9, materialTexture10,
            materialTexture11, materialTexture12, materialTexture13,
            materialTexture14, materialTexture15, materialTexture16,
            materialTexture17, materialTexture18, materialTexture19,
            materialTexture20, materialTexture21, materialTexture22,
            materialTexture23, materialTexture24, materialTexture25,
            materialTexture26, materialTexture27, materialTexture28,
            materialTexture29, materialTexture30, materialTexture31,
            materialTexture32, materialTexture33, materialTexture34,
            materialTexture35, materialTexture36, materialTexture37,
            materialTexture38, materialTexture39, materialTexture40,
            materialTexture41, materialTexture42, materialTexture43,
            materialTexture44, materialTexture45, materialTexture46,
            materialTexture47);
        float roughnessValue = roughnessSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            roughnessValue = roughnessSample.y;
        }
        roughness *= clamp(roughnessValue, 0.0f, 1.0f);
    }
    if (mat.aoTextureIndex >= 0 && uint(mat.aoTextureIndex) < textureCount) {
        ao *= clamp(sampleMaterialTexture(
                        mat.aoTextureIndex, uv, materialTexture0,
                        materialTexture1, materialTexture2, materialTexture3,
                        materialTexture4, materialTexture5, materialTexture6,
                        materialTexture7, materialTexture8, materialTexture9,
                        materialTexture10, materialTexture11,
                        materialTexture12, materialTexture13,
                        materialTexture14, materialTexture15,
                        materialTexture16, materialTexture17,
                        materialTexture18, materialTexture19,
                        materialTexture20, materialTexture21,
                        materialTexture22, materialTexture23,
                        materialTexture24, materialTexture25,
                        materialTexture26, materialTexture27,
                        materialTexture28, materialTexture29,
                        materialTexture30, materialTexture31,
                        materialTexture32, materialTexture33,
                        materialTexture34, materialTexture35,
                        materialTexture36, materialTexture37,
                        materialTexture38, materialTexture39,
                        materialTexture40, materialTexture41,
                        materialTexture42, materialTexture43,
                        materialTexture44, materialTexture45,
                        materialTexture46, materialTexture47)
                        .x,
                    0.0f, 1.0f);
    }

    emissive = clamp(emissive, float3(0.0f), float3(8.0f));
    roughness = clamp(roughness, 0.05f, 1.0f);
    metallic = clamp(metallic, 0.0f, 1.0f);
    ao = clamp(ao, 0.0f, 1.0f);
}

static inline float3 resolveNormal(
    int normalTextureIndex, float normalStrength, float2 uv, float3 n, float3 tangent,
    float3 bitangent, uint textureCount, texture2d<float> materialTexture0,
    texture2d<float> materialTexture1, texture2d<float> materialTexture2,
    texture2d<float> materialTexture3, texture2d<float> materialTexture4,
    texture2d<float> materialTexture5, texture2d<float> materialTexture6,
    texture2d<float> materialTexture7, texture2d<float> materialTexture8,
    texture2d<float> materialTexture9, texture2d<float> materialTexture10,
    texture2d<float> materialTexture11, texture2d<float> materialTexture12,
    texture2d<float> materialTexture13, texture2d<float> materialTexture14,
    texture2d<float> materialTexture15, texture2d<float> materialTexture16,
    texture2d<float> materialTexture17, texture2d<float> materialTexture18,
    texture2d<float> materialTexture19, texture2d<float> materialTexture20,
    texture2d<float> materialTexture21, texture2d<float> materialTexture22,
    texture2d<float> materialTexture23, texture2d<float> materialTexture24,
    texture2d<float> materialTexture25, texture2d<float> materialTexture26,
    texture2d<float> materialTexture27, texture2d<float> materialTexture28,
    texture2d<float> materialTexture29, texture2d<float> materialTexture30,
    texture2d<float> materialTexture31, texture2d<float> materialTexture32,
    texture2d<float> materialTexture33, texture2d<float> materialTexture34,
    texture2d<float> materialTexture35, texture2d<float> materialTexture36,
    texture2d<float> materialTexture37, texture2d<float> materialTexture38,
    texture2d<float> materialTexture39, texture2d<float> materialTexture40,
    texture2d<float> materialTexture41, texture2d<float> materialTexture42,
    texture2d<float> materialTexture43, texture2d<float> materialTexture44,
    texture2d<float> materialTexture45, texture2d<float> materialTexture46,
    texture2d<float> materialTexture47) {
    float3 N = safeNormalize(n, float3(0.0f, 1.0f, 0.0f));
    float3 T = safeNormalize(tangent - N * dot(N, tangent), float3(1.0f, 0.0f, 0.0f));
    float3 B = safeNormalize(bitangent - N * dot(N, bitangent), cross(N, T));
    if (dot(cross(T, B), cross(T, B)) <= 1e-10f) {
        float3 up = (fabs(N.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f)
                                         : float3(1.0f, 0.0f, 0.0f);
        T = safeNormalize(cross(up, N), float3(1.0f, 0.0f, 0.0f));
        B = safeNormalize(cross(N, T), float3(0.0f, 0.0f, 1.0f));
    }
    if (normalTextureIndex >= 0 && uint(normalTextureIndex) < textureCount &&
        normalStrength > 0.0f) {
        float3 texN = sampleMaterialTexture(
                          normalTextureIndex, uv, materialTexture0,
                          materialTexture1, materialTexture2, materialTexture3,
                          materialTexture4, materialTexture5, materialTexture6,
                          materialTexture7, materialTexture8, materialTexture9,
                          materialTexture10, materialTexture11,
                          materialTexture12, materialTexture13,
                          materialTexture14, materialTexture15,
                          materialTexture16, materialTexture17,
                          materialTexture18, materialTexture19,
                          materialTexture20, materialTexture21,
                          materialTexture22, materialTexture23,
                          materialTexture24, materialTexture25,
                          materialTexture26, materialTexture27,
                          materialTexture28, materialTexture29,
                          materialTexture30, materialTexture31,
                          materialTexture32, materialTexture33,
                          materialTexture34, materialTexture35,
                          materialTexture36, materialTexture37,
                          materialTexture38, materialTexture39,
                          materialTexture40, materialTexture41,
                          materialTexture42, materialTexture43,
                          materialTexture44, materialTexture45,
                          materialTexture46, materialTexture47)
                          .xyz;
        texN = texN * 2.0f - 1.0f;
        texN.xy *= normalStrength;
        texN = safeNormalize(texN, float3(0.0f, 0.0f, 1.0f));
        N = safeNormalize(float3x3(T, B, N) * texN, N);
    }
    return N;
}

static inline float2 octEncode(float3 n) {
    n /= max(fabs(n.x) + fabs(n.y) + fabs(n.z), 1e-6f);
    float2 e = n.xy;
    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(e.x >= 0.0f ? 1.0f : -1.0f, e.y >= 0.0f ? 1.0f : -1.0f);
        e = (1.0f - fabs(e.yx)) * signNotZero;
    }
    return e;
}

constexpr sampler ddgiLinearSampler(coord::normalized, address::clamp_to_edge,
                                    filter::linear);

static inline float3 samplePreviousIrradiance(texture2d<float> previous,
                                              constant ProbeSpace &ps,
                                              float3 posWS, float3 direction) {
    uint3 counts = uint3(ps.probeCount);
    if (counts.x == 0u || counts.y == 0u || counts.z == 0u) {
        return float3(0.0f);
    }
    float3 grid = clamp((posWS - ps.origin) / max(ps.spacing, float3(1e-4f)),
                        float3(0.0f), float3(counts - 1u));
    uint3 coord = uint3(round(grid));
    uint probeIndex = coord.x + counts.x * (coord.y + counts.y * coord.z);
    uint border = uint(ps.atlasParams.x);
    uint innerRes = uint(ps.atlasParams.y);
    uint probesPerRow = uint(ps.atlasParams.z);
    uint tileRes = innerRes + 2u * border;
    uint2 tile = uint2(probeIndex % probesPerRow, probeIndex / probesPerRow);
    float2 inner = octEncode(safeNormalize(direction, float3(0.0f, 1.0f, 0.0f))) *
                       0.5f +
                   0.5f;
    float2 pixel = float2(tile * tileRes + border) +
                   inner * float(max(innerRes, 1u) - 1u) + 0.5f;
    float3 history = previous.sample(
                                 ddgiLinearSampler,
                                 pixel / float2(previous.get_width(),
                                                previous.get_height()))
                         .xyz;
    return all(isfinite(history)) ? max(history, float3(0.0f))
                                  : float3(0.0f);
}

static inline float3 sampleSky(float3 d, texturecube<float> skybox,
                               float3 skyColor, uint useSkybox) {
    if (useSkybox != 0u) {
        return max(skybox.sample(ddgiLinearSampler, d).xyz, float3(0.0f));
    }
    float t = clamp(d.y * 0.5f + 0.5f, 0.0f, 1.0f);
    return mix(skyColor * 0.08f, skyColor, t);
}

static inline float shadowVisibility(float3 ro, float3 rd, float maxT,
                                     device const Triangle *tris,
                                     instance_acceleration_structure sceneAS) {
    if (maxT <= 1e-4f) {
        return 1.0f;
    }
    Hit h = traceScene(ro, rd, tris, sceneAS, maxT);
    if (!h.hit) {
        return 1.0f;
    }
    float minOccluderDistance = max(0.01f, maxT * 0.0025f);
    if (h.t <= minOccluderDistance) {
        return 1.0f;
    }
    return (h.t >= maxT) ? 1.0f : 0.0f;
}

static inline float giNdotL(float3 n, float3 l) {
    return max(dot(n, l), 0.0f);
}

static inline float3 evaluateDirectLights(
    float3 posWS, float3 normalWS, float bias, float maxDistance,
    device const Triangle *tris, instance_acceleration_structure sceneAS,
    device const DirectionalLight *directionalLights, uint directionalLightCount,
    device const PointLight *pointLights, uint pointLightCount,
    device const SpotLight *spotLights, uint spotLightCount,
    device const AreaLight *areaLights, uint areaLightCount) {
    float3 n = safeNormalize(normalWS, float3(0.0f, 1.0f, 0.0f));
    float3 sum = float3(0.0f);

    for (uint i = 0; i < directionalLightCount; i++) {
        float3 L = safeNormalize(-directionalLights[i].direction,
                                 float3(0.0f, 1.0f, 0.0f));
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float visibility = shadowVisibility(posWS + n * bias, L, maxDistance,
                                            tris, sceneAS);
        sum += directionalLights[i].diffuse *
               max(0.0f, directionalLights[i].intensity) * ndl * visibility;
    }

    for (uint i = 0; i < pointLightCount; i++) {
        float3 toLight = pointLights[i].position - posWS;
        float dist = length(toLight);
        float radius = max(pointLights[i].radius, 0.001f);
        if (dist <= 1e-4f || dist >= radius)
            continue;

        float3 L = toLight / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float attenuation =
            1.0f / max(pointLights[i].constant0 + pointLights[i].linear * dist +
                           pointLights[i].quadratic * dist * dist,
                       1e-4f);
        float fade = 1.0f - smoothstep(radius * 0.9f, radius, dist);
        float visibility = shadowVisibility(posWS + n * bias, L, dist - bias,
                                            tris, sceneAS);
        sum += pointLights[i].diffuse * max(0.0f, pointLights[i].intensity) *
               attenuation * fade * ndl * visibility;
    }

    for (uint i = 0; i < spotLightCount; i++) {
        float3 toLight = spotLights[i].position - posWS;
        float dist = length(toLight);
        float range = max(spotLights[i].range, 0.001f);
        if (dist <= 1e-4f || dist >= range)
            continue;

        float3 L = toLight / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float3 spotDir = safeNormalize(spotLights[i].direction,
                                       float3(0.0f, -1.0f, 0.0f));
        float theta = dot(L, -spotDir);
        float epsilon = max(spotLights[i].cutOff - spotLights[i].outerCutOff,
                            1e-4f);
        float cone = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0f,
                           1.0f);
        if (cone <= 0.0f)
            continue;

        float attenuation =
            1.0f / ((1.0f + (dist / range)) + ((dist * dist) / (range * range)));
        float fade = 1.0f - smoothstep(range * 0.9f, range, dist);
        float visibility = shadowVisibility(posWS + n * bias, L, dist - bias,
                                            tris, sceneAS);
        sum += spotLights[i].diffuse * max(0.0f, spotLights[i].intensity) * cone *
               attenuation * fade * ndl * visibility;
    }

    for (uint i = 0; i < areaLightCount; i++) {
        float3 center = areaLights[i].position;
        float3 right = safeNormalize(areaLights[i].right, float3(1.0f, 0.0f, 0.0f));
        float3 up = safeNormalize(areaLights[i].up, float3(0.0f, 1.0f, 0.0f));
        float2 halfSize = areaLights[i].size * 0.5f;

        float3 toPoint = posWS - center;
        float s = clamp(dot(toPoint, right), -halfSize.x, halfSize.x);
        float t = clamp(dot(toPoint, up), -halfSize.y, halfSize.y);
        float3 closest = center + right * s + up * t;

        float3 Lvec = closest - posWS;
        float dist = length(Lvec);
        float range = max(areaLights[i].range, 0.001f);
        if (dist <= 1e-4f || dist >= range)
            continue;

        float3 L = Lvec / dist;
        float ndl = giNdotL(n, L);
        if (ndl <= 0.0f)
            continue;

        float3 lightNormal =
            safeNormalize(cross(right, up), float3(0.0f, -1.0f, 0.0f));
        float nl = dot(lightNormal, -L);
        float facing = (areaLights[i].castsBothSides != 0) ? abs(nl) : max(nl, 0.0f);
        float cutoff = cos(areaLights[i].angle * 0.01745329251f);
        if (facing < cutoff || facing <= 0.0f)
            continue;

        float attenuation =
            1.0f / ((1.0f + (dist / range)) + ((dist * dist) / (range * range)));
        float fade = 1.0f - smoothstep(range * 0.9f, range, dist);
        float visibility = shadowVisibility(posWS + n * bias, L, dist - bias,
                                            tris, sceneAS);
        sum += areaLights[i].diffuse * max(0.0f, areaLights[i].intensity) *
               facing * attenuation * fade * ndl * visibility;
    }

    return sum;
}

static inline void buildBasis(float3 n, thread float3 &t, thread float3 &b) {
    float3 up = (fabs(n.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f)
                                     : float3(1.0f, 0.0f, 0.0f);
    t = safeNormalize(cross(up, n), float3(1.0f, 0.0f, 0.0f));
    b = safeNormalize(cross(n, t), float3(0.0f, 0.0f, 1.0f));
}

static inline float3 sampleCosineHemisphere(thread uint &state) {
    float u1 = rand01(state);
    float u2 = rand01(state);
    float r = sqrt(max(0.0f, u1));
    float phi = 6.28318530718f * u2;
    float x = r * cos(phi);
    float z = r * sin(phi);
    float y = sqrt(max(0.0f, 1.0f - u1));
    return float3(x, y, z);
}

static inline float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    const float GOLDEN_RATIO = 1.6180339887498949f;
    const float TAU = 6.28318530718f;
    float i = float(index) + 0.5f;
    float phi = TAU * fract(i / GOLDEN_RATIO);
    float cosTheta = 1.0f - (2.0f * i) / float(count);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    uint rotSeed = wangHash(frameIndex * 1471u + 5743u);
    float rotAngle = float(rotSeed & 0xFFFFu) / 65536.0f * TAU;
    phi += rotAngle;
    return float3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));
}

kernel void main0(device float4 *probeRadianceOut [[buffer(0)]],
                  device const Triangle *tris [[buffer(1)]],
                  device const Material *materials [[buffer(2)]],
                  constant SceneCounts &sc [[buffer(3)]],
                  constant ProbeSpace &ps [[buffer(4)]],
                  constant RaytracingSettings &rt [[buffer(5)]],
                  device const DirectionalLight *directionalLights [[buffer(6)]],
                  device const PointLight *pointLights [[buffer(7)]],
                  device const SpotLight *spotLights [[buffer(8)]],
                  device const AreaLight *areaLights [[buffer(9)]],
                  instance_acceleration_structure sceneAS [[buffer(10)]],
                  texture2d<float> materialTexture0 [[texture(10)]],
                  texture2d<float> materialTexture1 [[texture(11)]],
                  texture2d<float> materialTexture2 [[texture(12)]],
                  texture2d<float> materialTexture3 [[texture(13)]],
                  texture2d<float> materialTexture4 [[texture(14)]],
                  texture2d<float> materialTexture5 [[texture(15)]],
                  texture2d<float> materialTexture6 [[texture(16)]],
                  texture2d<float> materialTexture7 [[texture(17)]],
                  texture2d<float> materialTexture8 [[texture(18)]],
                  texture2d<float> materialTexture9 [[texture(19)]],
                  texture2d<float> materialTexture10 [[texture(20)]],
                  texture2d<float> materialTexture11 [[texture(21)]],
                  texture2d<float> materialTexture12 [[texture(22)]],
                  texture2d<float> materialTexture13 [[texture(23)]],
                  texture2d<float> materialTexture14 [[texture(24)]],
                  texture2d<float> materialTexture15 [[texture(25)]],
                  texture2d<float> materialTexture16 [[texture(26)]],
                  texture2d<float> materialTexture17 [[texture(27)]],
                  texture2d<float> materialTexture18 [[texture(28)]],
                  texture2d<float> materialTexture19 [[texture(29)]],
                  texture2d<float> materialTexture20 [[texture(30)]],
                  texture2d<float> materialTexture21 [[texture(31)]],
                  texture2d<float> materialTexture22 [[texture(32)]],
                  texture2d<float> materialTexture23 [[texture(33)]],
                  texture2d<float> materialTexture24 [[texture(34)]],
                  texture2d<float> materialTexture25 [[texture(35)]],
                  texture2d<float> materialTexture26 [[texture(36)]],
                  texture2d<float> materialTexture27 [[texture(37)]],
                  texture2d<float> materialTexture28 [[texture(38)]],
                  texture2d<float> materialTexture29 [[texture(39)]],
                  texture2d<float> materialTexture30 [[texture(40)]],
                  texture2d<float> materialTexture31 [[texture(41)]],
                  texture2d<float> materialTexture32 [[texture(42)]],
                  texture2d<float> materialTexture33 [[texture(43)]],
                  texture2d<float> materialTexture34 [[texture(44)]],
                  texture2d<float> materialTexture35 [[texture(45)]],
                  texture2d<float> materialTexture36 [[texture(46)]],
                  texture2d<float> materialTexture37 [[texture(47)]],
                  texture2d<float> materialTexture38 [[texture(48)]],
                  texture2d<float> materialTexture39 [[texture(49)]],
                  texture2d<float> materialTexture40 [[texture(50)]],
                  texture2d<float> materialTexture41 [[texture(51)]],
                  texture2d<float> materialTexture42 [[texture(52)]],
                  texture2d<float> materialTexture43 [[texture(53)]],
                  texture2d<float> materialTexture44 [[texture(54)]],
                  texture2d<float> materialTexture45 [[texture(55)]],
                  texture2d<float> materialTexture46 [[texture(56)]],
                  texture2d<float> materialTexture47 [[texture(57)]],
                  texturecube<float> skybox [[texture(60)]],
                  texture2d<float> previousIrradiance [[texture(61)]],
                  uint tid [[thread_position_in_grid]]) {
    const float PI = 3.14159265359f;
    uint totalProbes = (uint)ps.atlasParams.w;
    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    uint updateStride = max(rt.probeUpdateStride, 1u);
    uint updateOffset =
        (updateStride > 1u) ? rt.probeUpdateOffset % updateStride : 0u;
    if (totalProbes > 0u) {
        updateOffset %= totalProbes;
    }
    uint activeProbeCount = rt.probeUpdateCount;
    if (activeProbeCount == 0u && totalProbes > updateOffset) {
        activeProbeCount =
            (totalProbes - updateOffset + updateStride - 1u) / updateStride;
    }
    activeProbeCount = max(activeProbeCount, 1u);
    uint totalRays = activeProbeCount * raysPerProbe;

    if (tid >= totalRays)
        return;

    uint localProbeIndex = tid / raysPerProbe;
    uint rayIndex = tid - localProbeIndex * raysPerProbe;
    uint probeIndex = updateOffset + localProbeIndex * updateStride;
    if (probeIndex >= totalProbes) {
        return;
    }

    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);
    uint3 pc = probeCoordFromIndex(probeIndex, counts);
    float3 probePos = ps.origin + float3(pc) * ps.spacing;

    float3 rayDir = sphericalFibonacci(rayIndex, raysPerProbe, rt.frameIndex);

    float maxDistance = max(rt.maxRayDistance, 0.001f);
    float bias = max(rt.normalBias, 0.001f);

    float3 ro = probePos + rayDir * bias;
    Hit h;
    float selfHitThreshold = bias * 4.0f;
    for (uint escapeStep = 0u; escapeStep < 1u; escapeStep++) {
        h = traceScene(ro, rayDir, tris, sceneAS, maxDistance);
        if (h.hit == 0u || h.t >= selfHitThreshold) {
            break;
        }
        ro += rayDir * (h.t + selfHitThreshold);
    }
    if (h.hit != 0u && h.t < selfHitThreshold) {
        h.hit = 0u;
        h.t = INFINITY;
    }

    float3 radiance = float3(0.0f);
    if (h.hit == 0u || h.t > maxDistance) {
        radiance = sampleSky(rayDir, skybox, rt.skyColor, rt.useSkybox);
    } else {
        float3 hitPos = ro + rayDir * h.t;
        float3 albedo;
        float metallic;
        float roughness;
        float ao;
        float3 emissive;
        int normalTextureIndex;
        float normalStrength;
        resolveMaterialParameters(
            materials, h.materialID, sc.materialCount, h.uv, sc.textureCount,
            materialTexture0,
            materialTexture1, materialTexture2, materialTexture3,
            materialTexture4, materialTexture5, materialTexture6,
            materialTexture7, materialTexture8, materialTexture9,
            materialTexture10, materialTexture11, materialTexture12,
            materialTexture13, materialTexture14, materialTexture15,
            materialTexture16, materialTexture17, materialTexture18,
            materialTexture19, materialTexture20, materialTexture21,
            materialTexture22, materialTexture23, materialTexture24,
            materialTexture25, materialTexture26, materialTexture27,
            materialTexture28, materialTexture29, materialTexture30,
            materialTexture31, materialTexture32, materialTexture33,
            materialTexture34, materialTexture35, materialTexture36,
            materialTexture37, materialTexture38, materialTexture39,
            materialTexture40, materialTexture41, materialTexture42,
            materialTexture43, materialTexture44, materialTexture45,
            materialTexture46, materialTexture47, albedo, metallic, roughness,
            ao, emissive,
            normalTextureIndex, normalStrength);
        float3 hitNormal = resolveNormal(
            normalTextureIndex, normalStrength, h.uv, h.n, h.tangent, h.bitangent,
            sc.textureCount, materialTexture0, materialTexture1,
            materialTexture2, materialTexture3, materialTexture4,
            materialTexture5, materialTexture6, materialTexture7,
            materialTexture8, materialTexture9, materialTexture10,
            materialTexture11, materialTexture12, materialTexture13,
            materialTexture14, materialTexture15, materialTexture16,
            materialTexture17, materialTexture18, materialTexture19,
            materialTexture20, materialTexture21, materialTexture22,
            materialTexture23, materialTexture24, materialTexture25,
            materialTexture26, materialTexture27, materialTexture28,
            materialTexture29, materialTexture30, materialTexture31,
            materialTexture32, materialTexture33, materialTexture34,
            materialTexture35, materialTexture36, materialTexture37,
            materialTexture38, materialTexture39, materialTexture40,
            materialTexture41, materialTexture42, materialTexture43,
            materialTexture44, materialTexture45, materialTexture46,
            materialTexture47);

        if (dot(hitNormal, -rayDir) < 0.0f) {
            hitNormal = -hitNormal;
        }

        float3 direct = evaluateDirectLights(
            hitPos, hitNormal, bias, maxDistance, tris, sceneAS,
            directionalLights, sc.directionalLightCount, pointLights,
            sc.pointLightCount, spotLights, sc.spotLightCount, areaLights,
            sc.areaLightCount);

        float diffuseWeight = 1.0f - metallic;
        float3 diffuseResponse = albedo * diffuseWeight * max(ao, 0.05f) / PI;
        float3 previousBounce =
            rt.frameIndex >= max(rt.probeUpdateStride, 1u)
                ? samplePreviousIrradiance(previousIrradiance, ps,
                                           hitPos + hitNormal * bias, hitNormal)
                : float3(0.0f);
        float3 indirect = previousBounce * diffuseResponse * 0.35f;
        radiance = direct * diffuseResponse + indirect + emissive;
        radiance = clamp(radiance, float3(0.0f), float3(16.0f));
    }

    uint outIndex = probeIndex * raysPerProbe + rayIndex;
    probeRadianceOut[outIndex] = float4(radiance, h.hit != 0u ? h.t : -1.0f);
}
