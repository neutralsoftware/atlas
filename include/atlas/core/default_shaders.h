// This file contains packed shader source code.
// Metal shaders packed as source
#ifndef ATLAS_GENERATED_SHADERS_H
#define ATLAS_GENERATED_SHADERS_H

#include <cstddef>

namespace opal {
const char *packedShaderSource(const char *const *parts, std::size_t count);
}

struct AtlasPackedShaderSource {
    const char *const *parts;
    std::size_t count;

    operator const char *() const {
        return opal::packedShaderSource(parts, count);
    }
};

static const char* const COLOR_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float4 vertexColor [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    float3 color = in.vertexColor.xyz / (in.vertexColor.xyz + float3(1.0));
    out.FragColor = float4(color, in.vertexColor.w);
    if (length(color) > 1.0)
    {
        out.BrightColor = float4(color, in.vertexColor.w);
    }
    return out;
}

)",
};
static const AtlasPackedShaderSource COLOR_FRAG = {COLOR_FRAG_PARTS, 1};

static const char* const COLOR_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float4 vertexColor [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    float4x4 mvp;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        mvp = (_12.projection * _12.view) * instanceModel;
    }
    else
    {
        mvp = (_12.projection * _12.view) * _12.model;
    }
    out.gl_Position = mvp * float4(in.aPos, 1.0);
    out.vertexColor = in.aColor;
    return out;
}
)",
};
static const AtlasPackedShaderSource COLOR_VERT = {COLOR_VERT_PARTS, 1};

static const char* const DDGI_PARTS[] = {
R"(#include <metal_stdlib>
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
        return materialTexture0.sample)",
R"((materialTexSampler, uv);
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
                           materialTexture42)",
R"(, materialTexture43,
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
                          materialTexture10, m)",
R"(aterialTexture11,
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
        float3 closest = center + right * s + up *)",
R"( t;

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
    for (uint escapeStep = 0u)",
R"(; escapeStep < 1u; escapeStep++) {
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
)",
};
static const AtlasPackedShaderSource DDGI = {DDGI_PARTS, 6};

static const char* const DDGI_WRITE_PARTS[] = {
R"(#include <metal_stdlib>
using namespace metal;

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
};

static inline uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

static inline float3 octDecode(float2 e) {
    float3 n = float3(e.x, e.y, 1.0f - fabs(e.x) - fabs(e.y));
    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(n.x >= 0.0f ? 1.0f : -1.0f, n.y >= 0.0f ? 1.0f : -1.0f);
        float2 folded = (1.0f - fabs(n.yx)) * signNotZero;
        n.x = folded.x;
        n.y = folded.y;
    }
    return normalize(n);
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

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]],
                  texture2d<float, access::read> prevTexture [[texture(1)]],
                  texture2d<float, access::write> outDistance [[texture(2)]],
                  texture2d<float, access::read> prevDistance [[texture(3)]],
                  device float4 *probeRadiance [[buffer(0)]],
                  constant ProbeSpace &ps [[buffer(1)]],
                  constant RaytracingSettings &rt [[buffer(2)]],
                  uint2 gid [[thread_position_in_grid]]) {
    const float FOUR_PI = 12.566370614359172f;

    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    uint totalProbes = (uint)ps.atlasParams.w;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height())
        return;

    uint tileRes = innerRes + 2u * border;
    if (tileRes == 0u || probesPerRow == 0u || totalProbes == 0u ||
        innerRes == 0u) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;
    uint probeIndex = tileX + tileY * probesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint updateStride = max(rt.probeUpdateStride, 1u);
    uint updateOffset =
        (updateStride > 1u) ? (rt.probeUpdateOffset % updateStride) : 0u;
    bool probeIsActive =
        (updateStride <= 1u) ||
        ((probeIndex >= updateOffset) &&
         (((probeIndex - updateOffset) % updateStride) == 0u));
    if (!probeIsActive) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    int innerX = clamp(int(localX) - int(border), 0, int(innerRes) - 1);
    int innerY = clamp(int(localY) - int(border), 0, int(innerRes) - 1);

    float2 e = (float2(float(innerX), float(innerY)) + 0.5f) / float(innerRes) * 2.0f - 1.0f;
    float3 texelDir = octDecode(e);

    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    uint baseOffset = probeIndex * raysPerProbe;
    uint rayStep = (raysPerProbe >= 128u) ? 2u : 1u;
    uint sampledRayCount = 0u;

    float3 sum = float3(0.0f);
    float weightSum = 0.0f;
    float distanceSum = 0.0f;
    float distanceSquaredSum = 0.0f;
    float nearHitCount = 0.0f;
    float spacingScale =
        max(max(ps.spacing.x, max(ps.spacing.y, ps.spacing.z)), 1e-4f);
    float nearHitThreshold =
        max(max(rt.normalBias * 2.0f, spacingScale * 0.1f), 0.002f);

    for (uint r = 0; r < raysPerProbe; r += rayStep) {
        sampledRayCount++;
        float4 raySample = probeRadiance[baseOffset + r];
        float hitDistance = raySample.w;
        if (hitDistance > 0.0f && hitDistance < nearHitThreshold) {
            nearHitCount += 1.0f;
        }

        float3 rayDir = sphericalFibonacci(r, raysPerProbe, rt.frameIndex);
        float w = max(0.0f, dot(texelDir, rayDir));
        if (w > 1e-6f) {
            float3 rad = raySample.xyz;
            if (all(isfinite(rad))) {
                sum += rad * w;
                weightSum += w;
                float distance = hitDistance > 0.0f
                                     ? min(hitDistance, rt.maxRayDistance)
                                     : rt.maxRayDistance;
                distanceSum += distance * w;
                distanceSquaredSum += distance * distance * w;
            }
        }
    }

    float3 irradiance = float3(0.0f);
    float2 distanceMoments = float2(rt.maxRayDistance,
                                    rt.maxRayDistance * rt.maxRayDistance);
    float invRayCount = 1.0f / float(max(sampledRayCount, 1u));
    if (weightSum > 1e-6f) {
        irradiance = sum * (FOUR_PI * invRayCount);
        distanceMoments = float2(distanceSum, distanceSquaredSum) / weightSum;
    }

    if (!all(isfinite(irradiance))) {
        irradiance = float3(0.0f);
    }

    float4 prev = prevTexture.read(gid);
    float4 previousDistance = prevDistance.read(gid);
    float3 prevValue = all(isfinite(prev.xyz)) ? prev.xyz : float3(0.0f);
    float prevValidity = isfinite(prev.w) ? clamp(prev.w, 0.0f, 1.0f) : 1.0f;

    float nearFraction = nearHitCount * invRayCount;
    float nearPenalty = smoothstep(0.82f, 0.995f, nearFraction);
    float probeValidity = 1.0f - nearPenalty;
    probeValidity = clamp(probeValidity, 0.0f, 1.0f);

    float h = clamp(rt.hysteresis, 0.0f, 0.995f);
    bool firstProbeUpdate = rt.frameIndex < updateStride;
    float3 blended = firstProbeUpdate ? irradiance : mix(irradiance, prevValue, h);
    float2 blendedDistance =
        firstProbeUpdate
            ? distanceMoments
            : mix(distanceMoments, previousDistance.xy, h);
    float blendedValidity =
        firstProbeUpdate
            ? probeValidity
            : mix(probeValidity, prevValidity, h);

    outTexture.write(float4(max(blended, float3(0.0f)), blendedValidity), gid);
    outDistance.write(float4(max(blendedDistance, float2(0.0f)),
                             blendedValidity, 0.0f), gid);
}
)",
};
static const AtlasPackedShaderSource DDGI_WRITE = {DDGI_WRITE_PARTS, 1};

static const char* const DEBUG_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 FragColor [[color(0)]];
};

fragment main0_out main0()
{
    main0_out out = {};
    out.FragColor = float4(1.0, 0.0, 1.0, 1.0);
    return out;
}

)",
};
static const AtlasPackedShaderSource DEBUG_FRAG = {DEBUG_FRAG_PARTS, 1};

static const char* const DEBUG_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.aPos, 1.0);
    return out;
}

)",
};
static const AtlasPackedShaderSource DEBUG_VERT = {DEBUG_VERT_PARTS, 1};

static const char* const DEFERRED_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    int4 textureTypes[16];
    int textureCount;
    uint useTexture;
    uint useColor;
    float3 cameraPosition;
    float normalMapStrength;
    uint useNormalMap;
    float2 textureScale;
    float2 textureOffset;
};

struct MaterialPush
{
    packed_float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct main0_out
{
    float4 gPosition [[color(0)]];
    float4 gNormal [[color(1)]];
    float4 gAlbedoSpec [[color(2)]];
    float4 gMaterial [[color(3)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn1)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
};

static inline __attribute__((always_inline))
float4 sampleTextureAt(thread const int& textureIndex, thread const float2& uv, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    if (textureIndex == 0)
    {
        return texture1.sample(texture1Smplr, uv);
    }
    else
    {
        if (textureIndex == 1)
        {
            return texture2.sample(texture2Smplr, uv);
        }
        else
        {
            if (textureIndex == 2)
            {
                return texture3.sample(texture3Smplr, uv);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return texture4.sample(texture4Smplr, uv);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return texture5.sample(texture5Smplr, uv);
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return texture6.sample(texture6Smplr, uv);
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return texture7.sample(texture7Smplr, uv);
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return texture8.sample(texture8Smplr, uv);
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return texture9.sample(texture9Smplr, uv);
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return texture10.sample(texture10Smplr, uv);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline))
float2 parallaxMapping(thread const float2& texCoords, thread const float3& viewDir, constant UBO& _46, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 v = fast::normalize(viewDir);
    float numLayers = mix(32.0, 8.0, abs(dot(float3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float2 P = (v.xy / float2(fast::max(v.z, 0.0500000007450580596923828125))) * 0.039999999105930328369140625;
    float2 deltaTexCoords = P / float2(numLayers);
    float2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == 6)
        {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == (-1))
    {
        return currentTexCoords;
    }
    int param = textureIndex;
    float2 param_1 = currentTexCoords;
    float currentDepthMapValue = 1.0 - sampleTextureAt(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
    int maxIterations = int(numLayers) + 1;
    int iteration = 0;
    while (currentLayerDepth < currentDepthMapValue && iteration < maxIterations)
    {
        currentTexCoords -= deltaTexCoords;
        int param_2 = textureIndex;
        float2 param_3 = currentTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(param_2, param_3, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
        currentLayerDepth += layerDepth;
        iteration++;
    }
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    int param_4 = textureIndex;
    float2 param_5 = prevTexCoords;
    float beforeDepth = 1.0 - sampleTextureAt(param_4, param_5, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x - (currentLayerDepth - layerDepth);
    float denom = afterDepth - beforeDepth;
    float weight = abs(denom) > 9.9999997473787516355514526367188e-05 ? fast::clamp(afterDepth / denom, 0.0, 1.0) : 0.0;
    currentTexCoords = (prevTexCoords * weight) + (currentTexCoords * (1.0 - weight));
    return currentTexCoords;
}

static inline __attribute__((always_inline))
float4 enableTextures(thread const int& type, constant UBO& _46, texture2d<float> texture1, sampler texture1Smplr, thread float2& texCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float4 color = float4(0.0);
    int count = 0;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == type)
        {
            if (i == 0)
            {
                color += texture1.sample(texture1Smplr, texCoord);
            }
            else
            {
                if (i == 1)
                {
                    color += texture2.sample(texture2Smplr, texCoord);
                }
                else
                {
                    if (i == 2)
                    {
           )",
R"(             color += texture3.sample(texture3Smplr, texCoord);
                    }
                    else
                    {
                        if (i == 3)
                        {
                            color += texture4.sample(texture4Smplr, texCoord);
                        }
                        else
                        {
                            if (i == 4)
                            {
                                color += texture5.sample(texture5Smplr, texCoord);
                            }
                            else
                            {
                                if (i == 5)
                                {
                                    color += texture6.sample(texture6Smplr, texCoord);
                                }
                                else
                                {
                                    if (i == 6)
                                    {
                                        color += texture7.sample(texture7Smplr, texCoord);
                                    }
                                    else
                                    {
                                        if (i == 7)
                                        {
                                            color += texture8.sample(texture8Smplr, texCoord);
                                        }
                                        else
                                        {
                                            if (i == 8)
                                            {
                                                color += texture9.sample(texture9Smplr, texCoord);
                                            }
                                            else
                                            {
                                                if (i == 9)
                                                {
                                                    color += texture10.sample(texture10Smplr, texCoord);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            count++;
        }
    }
    if (count > 0)
    {
        color /= float4(float(count));
    }
    if (count == 0)
    {
        return float4(-1.0);
    }
    return color;
}

fragment main0_out main0(main0_in in [[stage_in]], constant UBO& _46 [[buffer(0)]], constant MaterialPush& material [[buffer(1)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float3x3 TBN = {};
    TBN[0] = in.TBN_0;
    TBN[1] = in.TBN_1;
    TBN[2] = in.TBN_2;
    float2 texCoord = in.TexCoord * _46.textureScale + _46.textureOffset;
    bool hasParallaxMap = false;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == 6)
        {
            hasParallaxMap = true;
            break;
        }
    }
    if (hasParallaxMap)
    {
        float3 tangentViewDir = fast::normalize(transpose(TBN) * (_46.cameraPosition - in.FragPos));
        float2 param = texCoord;
        float3 param_1 = tangentViewDir;
        texCoord = parallaxMapping(param, param_1, _46, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    }
    int param_2 = 0;
    float4 sampledColor = enableTextures(param_2, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool hasColorTexture = any(sampledColor != float4(-1.0));
    float4 baseColor = float4(material.albedo[0], material.albedo[1], material.albedo[2], 1.0);
    int param_3 = 0;
    float4 albedoTex = enableTextures(param_3, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(albedoTex != float4(-1.0)))
    {
        baseColor *= albedoTex;
    }
    int param_3a = 12;
    float4 opacityTex = enableTextures(param_3a, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(opacityTex != float4(-1.0)))
    {
        if (opacityTex.x < 0.100000001490116119384765625)
        {
            discard_fragment();
        }
    }
    else
    {
        if ((baseColor.w < 0.100000001490116119384765625) && (material.albedo[3] < 0.999000012874603271484375))
        {
            discard_fragment();
        }
    }
    int param_4 = 5;
    float4 normTexture = enableTextures(param_4, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool _527 = normTexture.x != (-1.0);
    bool _534;
    if (_527)
    {
        _534 = normTexture.y != (-1.0);
    }
    else
    {
        _534 = _527;
    }
    bool _540;
    if (_534)
    {
        _540 = normTexture.z != (-1.0);
    }
    else
    {
        _540 = _534;
    }
    float normalStrength = fast::max(_46.normalMapStrength, 0.0f);
    bool useNormalMap = (_46.useNormalMap != 0u) && (normalStrength > 0.0f);
    float3 normal;
    if (_540 && useNormalMap)
    {
        float3 tangentNormal = (normTexture.xyz * 2.0) - float3(1.0);
        tangentNormal.xy *= normalStrength;
        tangentNormal = fast::normalize(tangentNormal);
        normal = fast::normalize(TBN * tangentNormal);
    }
    else
    {
        normal = fast::normalize(in.Normal);
    }
    float3 albedoColor = baseColor.xyz;
    int param_pbr_pack = 14;
    float4 pbrPackTex = enableTextures(param_pbr_pack, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool hasPbrPack = any(pbrPackTex != float4(-1.0));
    float metallicValue = material.metallic;
    int param_5 = 9;
    float4 metallicTex = enableTextures(param_5, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        metallicValue *= pbrPackTex.z;
    }
    else if (any(metallicTex != float4(-1.0)))
    {
        metallicValue *= metallicTex.x;
    }
    float roughnessValue = mater)",
R"(ial.roughness;
    int param_6 = 10;
    float4 roughnessTex = enableTextures(param_6, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        roughnessValue *= pbrPackTex.y;
    }
    else if (any(roughnessTex != float4(-1.0)))
    {
        roughnessValue *= roughnessTex.x;
    }
    float aoValue = material.ao;
    int param_7 = 11;
    float4 aoTex = enableTextures(param_7, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        aoValue *= pbrPackTex.x;
    }
    else if (any(aoTex != float4(-1.0)))
    {
        aoValue *= aoTex.x;
    }
    metallicValue = fast::clamp(metallicValue, 0.0, 1.0);
    roughnessValue = fast::clamp(roughnessValue, 0.0, 1.0);
    aoValue = fast::clamp(aoValue, 0.0, 1.0);
    float nonlinearDepth = gl_FragCoord.z;
    out.gPosition = float4(in.FragPos, nonlinearDepth);
    float3 n = fast::normalize(normal);
    bool _639 = !all(n == n);
    bool _646;
    if (!_639)
    {
        _646 = length(n) < 9.9999997473787516355514526367188e-05;
    }
    else
    {
        _646 = _639;
    }
    if (_646)
    {
        n = fast::normalize(in.Normal);
    }
    out.gNormal = float4(n, 1.0);
    float3 a = fast::clamp(albedoColor, float3(0.0), float3(1.0));
    if (!all(a == a))
    {
        a = float3(0.0);
    }
    out.gAlbedoSpec = float4(a, aoValue);
    out.gMaterial = float4(metallicValue, roughnessValue, aoValue, fast::clamp(material.reflectivity, 0.0, 1.0));
    return out;
}
)",
};
static const AtlasPackedShaderSource DEFERRED_FRAG = {DEFERRED_FRAG_PARTS, 3};

static const char* const DEFERRED_VERT_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Returns the determinant of a 2x2 matrix.
static inline __attribute__((always_inline))
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Returns the determinant of a 3x3 matrix.
static inline __attribute__((always_inline))
float spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) + c1 * spvDet2x2(a2, a3, b2, b3);
}

// Returns the inverse of a matrix, by using the algorithm of calculating the classical
// adjoint and dividing by the determinant. The contents of the matrix are changed.
static inline __attribute__((always_inline))
float4x4 spvInverse4x4(float4x4 m)
{
    float4x4 adj;	// The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the matrix.
    adj[0][0] =  spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][2] =  spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);

    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][1] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
    adj[1][3] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);

    adj[2][0] =  spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][2] =  spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);

    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][1] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
    adj[3][3] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);

    // Calculate the determinant as a combination of the cofactors of the first row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float4 outColor [[user(locn0)]];
    float2 TexCoord [[user(locn1)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float2 aTexCoord [[attribute(2)]];
    float3 aNormal [[attribute(3)]];
    float3 aTangent [[attribute(4)]];
    float3 aBitangent [[attribute(5)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]])
{
    main0_out out = {};
    float3x3 TBN = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    float4x4 finalModel;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        finalModel = instanceModel;
    }
    else
    {
        finalModel = _12.model;
    }
    float4 worldPos = finalModel * float4(in.aPos, 1.0);
    out.FragPos = worldPos.xyz;
    out.gl_Position = (_12.projection * _12.view) * worldPos;
    out.TexCoord = in.aTexCoord;
    out.outColor = in.aColor;
    float4x4 _81 = transpose(spvInverse4x4(finalModel));
    float3x3 normalMatrix = float3x3(_81[0].xyz, _81[1].xyz, _81[2].xyz);
    out.Normal = fast::normalize(normalMatrix * in.aNormal);
    float3 T = fast::normalize(normalMatrix * in.aTangent);
    float3 B = fast::normalize(normalMatrix * in.aBitangent);
    float3 N = fast::normalize(normalMatrix * in.aNormal);
    TBN = float3x3(float3(T), float3(B), float3(N));
    out.TBN_0 = TBN[0];
    out.TBN_1 = TBN[1];
    out.TBN_2 = TBN[2];
    return out;
}
)",
};
static const AtlasPackedShaderSource DEFERRED_VERT = {DEFERRED_VERT_PARTS, 1};

static const char* const DEPTH_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float4 gl_Position [[position, invariant]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        float4x4 _36 = _12.projection * _12.view;
        float4x4 _40 = _36 * instanceModel;
        float4 _49 = float4(in.aPos, 1.0);
        float4 _50 = _40 * _49;
        out.gl_Position = _50;
    }
    else
    {
        float4x4 _58 = _12.projection * _12.view;
        float4x4 _61 = _58 * _12.model;
        float4 _66 = float4(in.aPos, 1.0);
        float4 _67 = _61 * _66;
        out.gl_Position = _67;
    }
    return out;
}
)",
};
static const AtlasPackedShaderSource DEPTH_VERT = {DEPTH_VERT_PARTS, 1};

static const char* const DOWNSAMPLE_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float2 srcResolution;
};

struct main0_out
{
    float4 downsample [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _13 [[buffer(0)]], texture2d<float> srcTexture [[texture(0)]], sampler srcTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 srcTexelSize =
        float2(1.0) / float2(srcTexture.get_width(), srcTexture.get_height());
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    float2 texCoord = in.TexCoord;
    float3 a = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 b = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y + (2.0 * y))).xyz;
    float3 c = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 d = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y)).xyz;
    float3 e = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y)).xyz;
    float3 f = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y)).xyz;
    float3 g = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 h = srcTexture.sample(srcTextureSmplr, float2(texCoord.x, texCoord.y - (2.0 * y))).xyz;
    float3 i = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 j = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y + y)).xyz;
    float3 k = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y + y)).xyz;
    float3 l = srcTexture.sample(srcTextureSmplr, float2(texCoord.x - x, texCoord.y - y)).xyz;
    float3 m = srcTexture.sample(srcTextureSmplr, float2(texCoord.x + x, texCoord.y - y)).xyz;
    float3 downsampleColor = e * 0.125;
    downsampleColor += ((((a + c) + g) + i) * 0.03125);
    downsampleColor += ((((b + d) + f) + h) * 0.0625);
    downsampleColor += ((((j + k) + l) + m) * 0.125);
    out.downsample = float4(downsampleColor, 1.0);
    return out;
}
)",
};
static const AtlasPackedShaderSource DOWNSAMPLE_FRAG = {DOWNSAMPLE_FRAG_PARTS, 1};

static const char* const EMPTY_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

fragment void main0()
{
}

)",
};
static const AtlasPackedShaderSource EMPTY_FRAG = {EMPTY_FRAG_PARTS, 1};

static const char* const FLUID_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4 waterColor;
    packed_float3 cameraPos;
    float time;
    float4x4 projection;
    float4x4 view;
    float4x4 invProjection;
    float4x4 invView;
    float3 lightDirection;
    float3 lightColor;
};

struct Parameters
{
    float refractionStrength;
    float reflectionStrength;
    float depthFade;
    int hasNormalTexture;
    int hasMovementTexture;
    float3 windForce;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float3 WorldPos [[user(locn1)]];
    float3 WorldNormal [[user(locn2)]];
    float3 WorldTangent [[user(locn3)]];
    float3 WorldBitangent [[user(locn4)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _19 [[buffer(0)]], constant Parameters& _68 [[buffer(1)]], texture2d<float> movementTexture [[texture(0)]], texture2d<float> normalTexture [[texture(1)]], texture2d<float> reflectionTexture [[texture(2)]], texture2d<float> refractionTexture [[texture(3)]], texture2d<float> sceneTexture [[texture(4)]], sampler movementTextureSmplr [[sampler(0)]], sampler normalTextureSmplr [[sampler(1)]], sampler reflectionTextureSmplr [[sampler(2)]], sampler refractionTextureSmplr [[sampler(3)]], sampler sceneTextureSmplr [[sampler(4)]])
{
    main0_out out = {};
    float3 normal = fast::normalize(in.WorldNormal);
    float3 viewDir = fast::normalize(float3(_19.cameraPos) - in.WorldPos);
    float4 clipSpace = (_19.projection * _19.view) * float4(in.WorldPos, 1.0);
    float3 ndc = clipSpace.xyz / float3(clipSpace.w);
    float2 screenUV = (ndc.xy * 0.5) + float2(0.5);
    float windStrength = length(_68.windForce);
    float2 _78;
    if (windStrength > 0.001000000047497451305389404296875)
    {
        _78 = fast::normalize(_68.windForce.xy);
    }
    else
    {
        _78 = float2(1.0, 0.0);
    }
    float2 windDir = _78;
    float waveSpeed = 0.1500000059604644775390625 + (windStrength * 0.300000011920928955078125);
    float waveAmplitude = 0.00999999977648258209228515625 + (windStrength * 0.0199999995529651641845703125);
    float waveFrequency = 30.0 + (windStrength * 10.0);
    float2 waveOffset;
    waveOffset.x = sin(((in.TexCoord.x * windDir.x) + (_19.time * waveSpeed)) * waveFrequency);
    waveOffset.y = cos(((in.TexCoord.y * windDir.y) - (_19.time * waveSpeed)) * waveFrequency);
    waveOffset *= waveAmplitude;
    float2 flowOffset = float2(0.0);
    if (_68.hasMovementTexture == 1)
    {
        float2 windUV = (_68.windForce.xy * _19.time) * 0.0500000007450580596923828125;
        float2 movementUV = (in.TexCoord * 2.0) + windUV;
        float2 movementSample = movementTexture.sample(movementTextureSmplr, movementUV).xy;
        movementSample = (movementSample * 2.0) - float2(1.0);
        flowOffset = (movementSample * windStrength) * 0.1500000059604644775390625;
        waveOffset += (flowOffset * 0.5);
    }
    if (_68.hasNormalTexture == 1)
    {
        float3 T = fast::normalize(in.WorldTangent);
        float3 B = fast::normalize(in.WorldBitangent);
        float3 N = fast::normalize(in.WorldNormal);
        float3x3 TBN = float3x3(float3(T), float3(B), float3(N));
        float normalSpeed = 0.02999999932944774627685546875 + (windStrength * 0.0500000007450580596923828125);
        float2 normalUV1 = ((in.TexCoord * 5.0) + (waveOffset * 10.0)) + ((_68.windForce.xy * _19.time) * normalSpeed);
        float2 normalUV2 = ((in.TexCoord * 3.0) - (waveOffset * 8.0)) - (((_68.windForce.xy * _19.time) * normalSpeed) * 0.800000011920928955078125);
        float3 normalMap1 = normalTexture.sample(normalTextureSmplr, normalUV1).xyz;
        float3 normalMap2 = normalTexture.sample(normalTextureSmplr, normalUV2).xyz;
        normalMap1 = (normalMap1 * 2.0) - float3(1.0);
        normalMap2 = (normalMap2 * 2.0) - float3(1.0);
        float3 blendedNormal = fast::normalize(normalMap1 + normalMap2);
        float3 worldSpaceNormal = fast::normalize(TBN * blendedNormal);
        float normalStrength = 0.5 + (windStrength * 0.300000011920928955078125);
        normal = fast::normalize(mix(N, worldSpaceNormal, float3(normalStrength)));
    }
    float2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;
    reflectionUV += (waveOffset * 0.300000011920928955078125);
    float2 refractionUV = screenUV - (waveOffset * 0.20000000298023223876953125);
    bool _324 = reflectionUV.x >= 0.0;
    bool _330;
    if (_324)
    {
        _330 = reflectionUV.x <= 1.0;
    }
    else
    {
        _330 = _324;
    }
    bool _336;
    if (_330)
    {
        _336 = reflectionUV.y >= 0.0;
    }
    else
    {
        _336 = _330;
    }
    bool _342;
    if (_336)
    {
        _342 = reflectionUV.y <= 1.0;
    }
    else
    {
        _342 = _336;
    }
    bool reflectionInBounds = _342;
    bool _346 = refractionUV.x >= 0.0;
    bool _352;
    if (_346)
    {
        _352 = refractionUV.x <= 1.0;
    }
    else
    {
        _352 = _346;
    }
    bool _358;
    if (_352)
    {
        _358 = refractionUV.y >= 0.0;
    }
    else
    {
        _358 = _352;
    }
    bool _364;
    if (_358)
    {
        _364 = refractionUV.y <= 1.0;
    }
    else
    {
        _364 = _358;
    }
    bool refractionInBounds = _364;
    reflectionUV = fast::clamp(reflectionUV, float2(0.0500000007450580596923828125), float2(0.949999988079071044921875));
    refractionUV = fast::clamp(refractionUV, float2(0.0500000007450580596923828125), float2(0.949999988079071044921875));
    float2 reflectionEdgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), reflectionUV) * smoothstep(float2(1.0), float2(0.85000002384185791015625), reflectionUV);
    float reflectionFadeFactor = reflectionEdgeFade.x * reflectionEdgeFade.y;
    float2 refractionEdgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), refractionUV) * smoothstep(float2(1.0), float2(0.85000002384185791015625), refractionUV);
    float refractionFadeFactor = refractionEdgeFade.x * refractionEdgeFade.y;
    if (!reflectionInBounds)
    {
        reflectionFadeFactor *= 0.300000011920928955078125;
    }
    if (!refractionInBounds)
    {
        refractionFadeFactor *= 0.300000011920928955078125;
    }
    float4 reflectionSample = reflectionTexture.sample(reflectionTextureSmplr, reflectionUV);
    float4 refractionSample = refractionTexture.sample(refractionTextureSmplr, refractionUV);
    float4 sceneFallback = sceneTexture.sample(sceneTextureSmplr, screenUV);
    bool validReflection = length(reflectionSample.xyz) > 0.00999999977648258209228515625;
    bool validRefraction = length(refractionSample.xyz) > 0.00999999977648258209228515625;
    if (!validReflection)
    {
        reflectionSample = sceneFallback;
    }
    if (!validRefraction)
    {
        refractionSample = sceneFallback;
    }
    float fresnel = powr(1.0 - fast::clamp(dot(normal, viewDir), 0.0, 1.0), 3.0);
    fresnel = mix(0.0199999995529651641845703125, 1.0, fresnel);
    fresnel *= _68.reflectionStrength;
    float3 combined = mix(refractionSample.xyz, reflectionSample.xyz, float3(fresnel));
    float waterTint = fast::clamp(_68.depthFade * 0.300000011920928955078125, 0.0, 0.5);
    combined = mix(combined, _19.waterColor.xyz, float3(waterTint));
    float foamAmount = 0.0;
    if (_68.hasMovementTexture == 1)
    {
        float foamFactor = smoothstep(0.699999988079071044921875, 1.0, length(flowOffset) * windStrength);
        foamAmount = foamFactor * 0.20000000298023223876953125;
        combined = mix(combined, float3(1.0), float3(foamAmount));
    }
    float3 lightDir = fast::normalize(-_19.lightDirection);
    float diffuse = fast::max(dot(normal, lightDir), 0.0);
    diffuse *= 0.300000011920928955078125;
    float3 halfDir = fast::normalize(lightDir + viewDir);
    float specAngle = fast::max(dot(normal, halfDir), 0.0);
    float specular = powr(specAngle, 128.0);
    specular *= ((fresnel * 0.5) + 0.5);
    float waveVariation = (sin((in.TexCoord.x * 50.0) + (_19.time * 2.0)) * 0.5) + 0.5;
    waveVariation *= ((cos((in.TexCoord.y * 5)",
R"(0.0) - (_19.time * 1.5)) * 0.5) + 0.5);
    specular *= (0.699999988079071044921875 + (waveVariation * 0.300000011920928955078125));
    float3 lighting = _19.lightColor * (diffuse + (specular * 2.0));
    combined += lighting;
    float3 specularHighlight = (_19.lightColor * specular) * 2.5;
    specularHighlight += float3(foamAmount * 2.0);
    float alpha = mix(0.699999988079071044921875, 0.949999988079071044921875, fresnel);
    out.FragColor = float4(combined, alpha);
    float luminance = fast::max(fast::max(combined.x, combined.y), combined.z);
    float3 bloomColor = float3(0.0);
    if (luminance > 1.0)
    {
        bloomColor = combined - float3(1.0);
    }
    bloomColor += specularHighlight;
    out.BrightColor = float4(bloomColor, alpha);
    return out;
}

)",
};
static const AtlasPackedShaderSource FLUID_FRAG = {FLUID_FRAG_PARTS, 2};

static const char* const FLUID_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float3 WorldPos [[user(locn1)]];
    float3 WorldNormal [[user(locn2)]];
    float3 WorldTangent [[user(locn3)]];
    float3 WorldBitangent [[user(locn4)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(1)]];
    float3 aNormal [[attribute(2)]];
    float3 aTangent [[attribute(3)]];
    float3 aBitangent [[attribute(4)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = ((_19.projection * _19.view) * _19.model) * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    out.WorldPos = float3((_19.model * float4(in.aPos, 1.0)).xyz);
    out.WorldNormal = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aNormal);
    out.WorldTangent = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aTangent);
    out.WorldBitangent = fast::normalize(float3x3(_19.model[0].xyz, _19.model[1].xyz, _19.model[2].xyz) * in.aBitangent);
    return out;
}

)",
};
static const AtlasPackedShaderSource FLUID_VERT = {FLUID_VERT_PARTS, 1};

static const char* const FULLSCREEN_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template<typename T, size_t Num>
struct spvUnsafeArray
{
    T elements[Num ? Num : 1];
    
    thread T& operator [] (size_t pos) thread
    {
        return elements[pos];
    }
    constexpr const thread T& operator [] (size_t pos) const thread
    {
        return elements[pos];
    }
    
    device T& operator [] (size_t pos) device
    {
        return elements[pos];
    }
    constexpr const device T& operator [] (size_t pos) const device
    {
        return elements[pos];
    }
    
    constexpr const constant T& operator [] (size_t pos) const constant
    {
        return elements[pos];
    }
    
    threadgroup T& operator [] (size_t pos) threadgroup
    {
        return elements[pos];
    }
    constexpr const threadgroup T& operator [] (size_t pos) const threadgroup
    {
        return elements[pos];
    }
};

// Implementation of the GLSL mod() function, which is slightly different than Metal fmod()
template<typename Tx, typename Ty>
inline Tx mod(Tx x, Ty y)
{
    return x - y * floor(x / y);
}

struct ColorCorrection
{
    float exposure;
    float contrast;
    float saturation;
    float gamma;
    float temperature;
    float tint;
};

struct PushConstants
{
    int hasBrightTexture;
    int hasDepthTexture;
    int hasVolumetricLightTexture;
    int hasPositionTexture;
    int hasLUTTexture;
    int hasSSRTexture;
    int TextureType;
    float lutSize;
    int EffectCount;
};

struct EffectBuffer
{
    int Effects[1];
};

struct EffectFloat1Buffer
{
    float EffectFloat1[1];
};

struct EffectFloat2Buffer
{
    float EffectFloat2[1];
};

struct EffectFloat3Buffer
{
    float EffectFloat3[1];
};

struct EffectFloat4Buffer
{
    float EffectFloat4[1];
};

struct EffectFloat5Buffer
{
    float EffectFloat5[1];
};

struct Uniforms
{
    float4x4 invProjectionMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 invViewMatrix;
    float4x4 lastViewMatrix;
    float nearPlane;
    float farPlane;
    float focusDepth;
    float focusRange;
    int maxMipLevel;
    float deltaTime;
    float time;
};

struct EffectFloat6Buffer
{
    float EffectFloat6[1];
};

struct Clouds
{
    float3 cloudPosition;
    float3 cloudSize;
    packed_float3 cameraPosition;
    float cloudScale;
    packed_float3 cloudOffset;
    float cloudDensityThreshold;
    float cloudDensityMultiplier;
    float cloudAbsorption;
    float cloudScattering;
    float cloudPhaseG;
    float cloudClusterStrength;
    int cloudPrimarySteps;
    int cloudLightSteps;
    float cloudLightStepMultiplier;
    float cloudMinStepLength;
    float3 sunDirection;
    packed_float3 sunColor;
    float sunIntensity;
    packed_float3 cloudAmbientColor;
    int hasClouds;
};

struct Environment
{
    packed_float3 fogColor;
    float fogIntensity;
};

constant spvUnsafeArray<float2, 9> _165 = spvUnsafeArray<float2, 9>({ float2(-0.0033333334140479564666748046875, 0.0033333334140479564666748046875), float2(0.0, 0.0033333334140479564666748046875), float2(0.0033333334140479564666748046875), float2(-0.0033333334140479564666748046875, 0.0), float2(0.0), float2(0.0033333334140479564666748046875, 0.0), float2(-0.0033333334140479564666748046875), float2(0.0, -0.0033333334140479564666748046875), float2(0.0033333334140479564666748046875, -0.0033333334140479564666748046875) });
constant spvUnsafeArray<float, 9> _171 = spvUnsafeArray<float, 9>({ -1.0, -1.0, -1.0, -1.0, 9.0, -1.0, -1.0, -1.0, -1.0 });
constant spvUnsafeArray<float, 9> _311 = spvUnsafeArray<float, 9>({ 1.0, 1.0, 1.0, 1.0, -8.0, 1.0, 1.0, 1.0, 1.0 });

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline))
float4 sampleColor(thread const float2& uv, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr)
{
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 7)
        {
            float redOffset = _394.EffectFloat1[i];
            float greenOffset = _403.EffectFloat2[i];
            float blueOffset = _411.EffectFloat3[i];
            float2 focusPoint = float2(_419.EffectFloat4[i], _426.EffectFloat5[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = Texture.sample(TextureSmplr, (sampleCoord + (direction * redOffset))).x;
            float green = Texture.sample(TextureSmplr, (sampleCoord + (direction * greenOffset))).y;
            float2 blue = Texture.sample(TextureSmplr, (sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (_381.Effects[i] == 9)
            {
                float pixelSizeInPixels = _394.EffectFloat1[i];
                float2 texSize = float2(int2(Texture.get_width(), Texture.get_height()));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                return Texture.sample(TextureSmplr, pixelated);
            }
            else
            {
                if (_381.Effects[i] == 10)
                {
                    float radius = _394.EffectFloat1[i];
                    float separation = _403.EffectFloat2[i];
                    float2 texelSize = float2(1.0) / float2(int2(Texture.get_width(), Texture.get_height()));
                    float3 maxColor = Texture.sample(TextureSmplr, uv).xyz;
                    int range = int(radius);
                    float radiusSq = radius * radius;
                    int _543 = -range;
                    for (int x = _543; x <= range; x++)
                    {
                        int _554 = -range;
                        for (int y = _554; y <= range; y++)
                        {
                            float distSq = float((x * x) + (y * y));
                            if (distSq <= radiusSq)
                            {
                                float2 offset = (float2(float(x), float(y)) * texelSize) * separation;
                                float3 sampled = Texture.sample(TextureSmplr, (uv + offset)).xyz;
                                maxColor = fast::max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, Texture.sample(TextureSmplr, uv).w);
                }
            }
        }
    }
    return Texture.sample(TextureSmplr, uv);
}

static inline __attribute__((always_inline))
float3 reconstructViewPos(thread const float2& uv, thread const float& depth, constant Uniforms& _849)
{
    float z = (depth * 2.0) - 1.0;
    float4 clipPos = float4((uv * 2.0) - float2(1.0), z, 1.0);
    float4 viewPos = _849.invProjectionMatrix * clipPos;
    viewPos /= float4(viewPos.w);
    return viewPos.xyz;
}

static inline __attribute__((always_inline))
float4 sharpen(texture2d<float> image, sampler imageSmplr, thread float2& TexCoord)
{
    spvUnsafeArray<float3, 9> sampleTex;
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.sample(imageSmplr, (TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _171[i_1]);
    }
    return float4(col, 1.0);
}

static inline __attribute__((always_inline))
float4 blur(texture2d<float> image, sampler imageSmplr, thread const float& radius, thread float2& TexCoord)
{
    float2 texelSize = float2(1.0) / float2(int2(image.get_width(), image.get_height()));
    float3 result = float3(0.0);
    float total = 0.0;
    float sigma = radius * 0.5;
    float twoSigmaSq = (2.0 * sigma) * sigma;
    int _257 = -int(radius);
    for (int x = _257; x <= int(radius); x++)
    {
        float weight = exp(f)",
R"(loat(-(x * x)) / twoSigmaSq);
        float2 offset = float2(float(x), 0.0) * texelSize;
        result += (image.sample(imageSmplr, (TexCoord + offset)).xyz * weight);
        total += weight;
    }
    result /= float3(total);
    return float4(result, 1.0);
}

static inline __attribute__((always_inline))
float4 edgeDetection(texture2d<float> image, sampler imageSmplr, thread float2& TexCoord)
{
    spvUnsafeArray<float3, 9> sampleTex;
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.sample(imageSmplr, (TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _311[i_1]);
    }
    return float4(col, 1.0);
}

static inline __attribute__((always_inline))
float4 applyFXAA(texture2d<float> tex, sampler texSmplr, thread const float2& texCoord, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr)
{
    float2 texelSize = float2(1.0) / float2(int2(tex.get_width(), tex.get_height()));
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 0.125;
    float FXAA_REDUCE_MIN = 0.0078125;
    float2 param = texCoord + (float2(-1.0) * texelSize);
    float3 rgbNW = sampleColor(param, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_1 = texCoord + (float2(1.0, -1.0) * texelSize);
    float3 rgbNE = sampleColor(param_1, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_2 = texCoord + (float2(-1.0, 1.0) * texelSize);
    float3 rgbSW = sampleColor(param_2, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_3 = texCoord + (float2(1.0) * texelSize);
    float3 rgbSE = sampleColor(param_3, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_4 = texCoord;
    float3 rgbM = sampleColor(param_4, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float3 luma = float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);
    float lumaMin = fast::min(lumaM, fast::min(fast::min(lumaNW, lumaNE), fast::min(lumaSW, lumaSE)));
    float lumaMax = fast::max(lumaM, fast::max(fast::max(lumaNW, lumaNE), fast::max(lumaSW, lumaSE)));
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);
    float dirReduce = fast::max((((lumaNW + lumaNE) + lumaSW) + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (fast::min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = fast::min(float2(FXAA_SPAN_MAX), fast::max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;
    float2 param_5 = texCoord + (dir * (-0.16666667163372039794921875));
    float2 param_6 = texCoord + (dir * 0.16666667163372039794921875);
    float3 rgbA = (sampleColor(param_5, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz + sampleColor(param_6, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz) * 0.5;
    float2 param_7 = texCoord + (dir * (-0.5));
    float2 param_8 = texCoord + (dir * 0.5);
    float3 rgbB = (rgbA * 0.5) + ((sampleColor(param_7, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz + sampleColor(param_8, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz) * 0.25);
    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return float4(rgbA, 1.0);
    }
    else
    {
        return float4(rgbB, 1.0);
    }
}

static inline __attribute__((always_inline))
float4 applyColorCorrection(thread const float4& color, thread const ColorCorrection& cc)
{
    float3 linearColor = color.xyz;
    linearColor *= powr(2.0, cc.exposure);
    linearColor = ((linearColor - float3(0.5)) * cc.contrast) + float3(0.5);
    linearColor.x += (cc.temperature * 0.0500000007450580596923828125);
    linearColor.y += (cc.tint * 0.0500000007450580596923828125);
    float luminance = dot(linearColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    linearColor = mix(float3(luminance), linearColor, float3(cc.saturation));
    linearColor = fast::clamp(linearColor, float3(0.0), float3(1.0));
    return float4(linearColor, color.w);
}

static inline __attribute__((always_inline))
float4 applyColorEffects(thread float4& color, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, constant Uniforms& _849, device EffectFloat6Buffer& _1049, thread float4& gl_FragCoord)
{
    ColorCorrection cc;
    float3 _noise;
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 0)
        {
            color = float4(float3(1.0) - color.xyz, color.w);
        }
        else
        {
            if (_381.Effects[i] == 1)
            {
                float average = ((0.2125999927520751953125 * color.x) + (0.715200006961822509765625 * color.y)) + (0.072200000286102294921875 * color.z);
                color = float4(average, average, average, color.w);
            }
            else
            {
                if (_381.Effects[i] == 5)
                {
                    cc.exposure = _394.EffectFloat1[i];
                    cc.contrast = _403.EffectFloat2[i];
                    cc.saturation = _411.EffectFloat3[i];
                    cc.gamma = _419.EffectFloat4[i];
                    cc.temperature = _426.EffectFloat5[i];
                    cc.tint = _1049.EffectFloat6[i];
                    float4 param = color;
                    ColorCorrection param_1 = cc;
                    color = applyColorCorrection(param, param_1);
                }
                else
                {
                    if (_381.Effects[i] == 8)
                    {
                        float levels = fast::max(_394.EffectFloat1[i], 1.0);
                        float grayscale = fast::max(color.x, fast::max(color.y, color.z));
                        if (grayscale > 9.9999997473787516355514526367188e-05)
                        {
                            float lower = floor(grayscale * levels) / levels;
                            float lowerDiff = abs(grayscale - lower);
                            float upper = ceil(grayscale * levels) / levels;
                            float upperDiff = abs(upper - grayscale);
                            float level0 = (lowerDiff <= upperDiff) ? lower : upper;
                            float adjustment = level0 / fast::max(grayscale, 9.9999997473787516355514526367188e-05);
                            color *= adjustment;
                        }
                    }
                    else
                    {
                        if (_381.Effects[i] == 11)
                        {
                            float amount = _394.EffectFloat1[i];
                            float3 seed = float3(gl_FragCoord.xy, _849.deltaTime * 100.0);
                            float n = dot(seed, float3(12.98980045318603515625, 78.233001708984375, 45.16400146484375));
                            _noise.x = fract(sin(n) * 43758.546875);
                            n = dot(seed, float3(93.9889984130859375, 67.345001220703125, 12.9890003204345703125));
                            _noise.y = fract(sin(n) * 28001.123046875);
                            n = dot(seed, float3(39.34600067138671875, 11.1350002288818359375, 83.154998779296875));
                            _noise.z = fract(sin(n) * 19283.45703125);
                            float3 grain = ((_noise - float3(0.5)) * 2.0) * amount;
                            float luminance = dot(color.xyz)",
R"(, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
                            float visibility = 1.0 - (abs(luminance - 0.5) * 0.5);
                            float4 _1210 = color;
                            float3 _1212 = _1210.xyz + (grain * visibility);
                            color.x = _1212.x;
                            color.y = _1212.y;
                            color.z = _1212.z;
                            float4 _1219 = color;
                            float3 _1223 = fast::clamp(_1219.xyz, float3(0.0), float3(1.0));
                            color.x = _1223.x;
                            color.y = _1223.y;
                            color.z = _1223.z;
                        }
                    }
                }
            }
        }
    }
    return color;
}

static inline __attribute__((always_inline))
float LinearizeDepth(thread const float& depth, constant Uniforms& _849)
{
    float z = (depth * 2.0) - 1.0;
    float linear = ((2.0 * _849.nearPlane) * _849.farPlane) / ((_849.farPlane + _849.nearPlane) - (z * (_849.farPlane - _849.nearPlane)));
    return linear / _849.farPlane;
}

static inline __attribute__((always_inline))
float2 rayBoxDst(thread const float3& boundsMin, thread const float3& boundsMax, thread const float3& rayOrigin, thread const float3& rayDir)
{
    float3 t0 = (boundsMin - rayOrigin) / rayDir;
    float3 t1 = (boundsMax - rayOrigin) / rayDir;
    float3 tMin = fast::min(t0, t1);
    float3 tMax = fast::max(t0, t1);
    float dstA = fast::max(fast::max(tMin.x, tMin.y), tMin.z);
    float dstB = fast::min(tMax.x, fast::min(tMax.y, tMax.z));
    float dstToContainer = fast::max(0.0, dstA);
    float dstInsideContainer = fast::max(0.0, dstB - dstToContainer);
    return float2(dstToContainer, dstInsideContainer);
}

static inline __attribute__((always_inline))
float hashNoise(thread const float3& p)
{
    return fract(sin(dot(p, float3(12.98980045318603515625, 78.233001708984375, 37.71900177001953125))) * 43758.546875);
}

static inline __attribute__((always_inline))
float saturate0(thread const float& v)
{
    return fast::clamp(v, 0.0, 1.0);
}

static inline __attribute__((always_inline))
float calculateCloudDensity(thread const float3& worldPos, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr)
{
    float3 halfExtents = fast::max(_1929.cloudSize * 0.5, float3(9.9999997473787516355514526367188e-05));
    float3 localPos = (worldPos - _1929.cloudPosition) / halfExtents;
    bool _1947 = any(localPos < float3(-1.0));
    bool _1955;
    if (!_1947)
    {
        _1955 = any(localPos > float3(1.0));
    }
    else
    {
        _1955 = _1947;
    }
    if (_1955)
    {
        return 0.0;
    }
    float3 uvw = (localPos * 0.5) + float3(0.5);
    float scale = fast::max(_1929.cloudScale, 0.001000000047497451305389404296875);
    float3 noiseCoord = fract((uvw * scale) + float3(_1929.cloudOffset));
    float4 shape = cloudsTexture.sample(cloudsTextureSmplr, noiseCoord);
    float param = _1929.cloudClusterStrength;
    float cluster = saturate0(param);
    float lowerFade = smoothstep(-0.949999988079071044921875, -0.60000002384185791015625, localPos.y);
    float upperFade = 1.0 - smoothstep(0.3499999940395355224609375, 0.949999988079071044921875, localPos.y);
    float verticalMask = lowerFade * upperFade;
    float coverageThreshold = mix(0.60000002384185791015625, 0.2800000011920928955078125, cluster);
    float coverageSoftness = mix(0.2199999988079071044921875, 0.3400000035762786865234375, cluster);
    float coverageNoise = mix(shape.x, shape.w, 0.4000000059604644775390625 + (cluster * 0.3499999940395355224609375));
    float coverage = smoothstep(coverageThreshold, coverageThreshold + coverageSoftness, coverageNoise);
    coverage = powr(coverage, mix(2.0, 0.699999988079071044921875, cluster));
    float detail = mix(smoothstep(0.25, 0.75, shape.y), smoothstep(0.20000000298023223876953125, 0.89999997615814208984375, shape.w), 0.550000011920928955078125);
    detail = powr(detail, mix(1.60000002384185791015625, 0.85000002384185791015625, cluster));
    float cavityNoise = smoothstep(0.2199999988079071044921875, 0.85000002384185791015625, shape.z);
    float gapMask = fast::clamp(1.0 - (cavityNoise * mix(0.25, 0.699999988079071044921875, cluster)), 0.0, 1.0);
    float density = coverage * mix(detail, 1.0, cluster * 0.3499999940395355224609375);
    density = fast::max(((density * gapMask) * verticalMask) - 0.0199999995529651641845703125, 0.0);
    density *= fast::max(_1929.cloudDensityMultiplier, 0.0);
    return density;
}

static inline __attribute__((always_inline))
float sampleSunTransmittance(thread const float3& worldPos, thread const float& stepSize, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr)
{
    float3 lightDir = -_1929.sunDirection;
    float dirLength = length(lightDir);
    if (dirLength < 0.001000000047497451305389404296875)
    {
        lightDir = float3(0.0, 1.0, 0.0);
    }
    else
    {
        lightDir /= float3(dirLength);
    }
    float maxDistance = length(_1929.cloudSize) * 1.5;
    float travel = 0.0;
    float attenuation = 1.0;
    float lightStep = fast::max(stepSize * _1929.cloudLightStepMultiplier, _1929.cloudMinStepLength);
    int steps = max(_1929.cloudLightSteps, 1);
    for (int i = 0; (i < steps) && (attenuation > 0.0500000007450580596923828125); i++)
    {
        travel += lightStep;
        if (travel > maxDistance)
        {
            break;
        }
        float3 samplePos = worldPos + (lightDir * travel);
        float3 param = samplePos;
        float density = calculateCloudDensity(param, _1929, cloudsTexture, cloudsTextureSmplr);
        attenuation *= exp(((-density) * lightStep) * _1929.cloudAbsorption);
        lightStep = fast::max(lightStep * _1929.cloudLightStepMultiplier, _1929.cloudMinStepLength);
    }
    return attenuation;
}

static inline __attribute__((always_inline))
float henyeyGreenstein(thread const float& cosTheta, thread const float& g)
{
    float g2 = g * g;
    float denom = powr((1.0 + g2) - ((2.0 * g) * cosTheta), 1.5);
    return (1.0 - g2) / (12.56637096405029296875 * fast::max(denom, 9.9999997473787516355514526367188e-05));
}

static inline __attribute__((always_inline))
float4 cloudRendering(thread const float4& inColor, thread float2& TexCoord, constant PushConstants& _372, constant Uniforms& _849, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr, texture2d<float> DepthTexture, sampler DepthTextureSmplr)
{
    if (_1929.hasClouds != 1)
    {
        return inColor;
    }
    float _2197;
    if (_372.hasDepthTexture == 1)
    {
        _2197 = DepthTexture.sample(DepthTextureSmplr, TexCoord).x;
    }
    else
    {
        _2197 = 1.0;
    }
    float nonLinearDepth = _2197;
    bool depthAvailable = (_372.hasDepthTexture == 1) && (nonLinearDepth < 1.0);
    float depthSample = depthAvailable ? nonLinearDepth : 1.0;
    float3 rayOrigin = float3(_1929.cameraPosition);
    float4 clipSpace = float4((TexCoord * 2.0) - float2(1.0), (depthSample * 2.0) - 1.0, 1.0);
    float4 viewSpace = _849.invProjectionMatrix * clipSpace;
    viewSpace /= float4(viewSpace.w);
    float3 worldPos = (_849.invViewMatrix * float4(viewSpace.xyz, 1.0)).xyz;
    float3 rayDir = fast::normalize(worldPos - rayOrigin);
    float _2261;
    if (depthAvailable)
    {
        _2261 = length(worldPos - rayOrigin);
    }
    else
    {
        _2261 = 1000000.0;
    }
    float sceneDistance = _2261;
    float3 boundsMin = _1929.cloudPosition - (_1929.cloudSize * 0.5);
    float3 boundsMax = _1929.cloudPosition + (_1929.cloudSize * 0.5);
    float3 param = boundsMin;
    float3 param_1 = boundsMax;
    float3 param_2 = rayOrigin;
    float3 param_3 = rayDir;
    float2 rayBoxInfo = rayBoxDst(param, param_1, param_2, param_3);
    float distToContainer = rayBoxInfo.x;
    float distInContainer = rayBoxInfo.y;
    if (distInContainer <= 0.0)
    {
        return inColor;
    }
    float dstLimit = fast::min(sceneDistance - distToContainer, distInContainer);
    dstLimit = fast::)",
R"(max(dstLimit, 0.0);
    if (dstLimit <= 9.9999997473787516355514526367188e-05)
    {
        return inColor;
    }
    int steps = max(_1929.cloudPrimarySteps, 8);
    float baseStep = dstLimit / float(steps);
    float stepSize = fast::max(baseStep, _1929.cloudMinStepLength);
    float3 param_4 = float3(TexCoord, _849.time);
    float jitter = hashNoise(param_4) - 0.5;
    float travelled = fast::clamp(jitter, -0.3499999940395355224609375, 0.3499999940395355224609375) * stepSize;
    travelled = fast::max(travelled, 0.0);
    float3 accumulatedLight = float3(0.0);
    float transmittance = 1.0;
    float3 sunDir = _1929.sunDirection;
    float sunLen = length(sunDir);
    if (sunLen > 0.001000000047497451305389404296875)
    {
        sunDir /= float3(sunLen);
    }
    else
    {
        sunDir = float3(0.0, 1.0, 0.0);
    }
    float phaseG = fast::clamp(_1929.cloudPhaseG, -0.949999988079071044921875, 0.949999988079071044921875);
    for (int _step = 0; (_step < steps) && (travelled < dstLimit); _step++)
    {
        if (transmittance <= 0.00999999977648258209228515625)
        {
            break;
        }
        float remainingDistance = dstLimit - travelled;
        if (remainingDistance <= 9.9999997473787516355514526367188e-06)
        {
            break;
        }
        float current = distToContainer + travelled;
        float3 samplePos = rayOrigin + (rayDir * current);
        float3 param_5 = samplePos;
        float density = calculateCloudDensity(param_5, _1929, cloudsTexture, cloudsTextureSmplr);
        if (density > 9.9999997473787516355514526367188e-05)
        {
            float adaptiveStep = stepSize;
            if (density < 0.0199999995529651641845703125)
            {
                adaptiveStep = stepSize * 2.5;
            }
            else
            {
                if (density < 0.0500000007450580596923828125)
                {
                    adaptiveStep = stepSize * 1.60000002384185791015625;
                }
            }
            adaptiveStep = fast::min(adaptiveStep, remainingDistance);
            float sampleWeight = density * adaptiveStep;
            float3 param_6 = samplePos;
            float param_7 = adaptiveStep;
            float lightTrans = sampleSunTransmittance(param_6, param_7, _1929, cloudsTexture, cloudsTextureSmplr);
            float cosTheta = fast::clamp(dot(rayDir, -sunDir), -1.0, 1.0);
            float param_8 = cosTheta;
            float param_9 = phaseG;
            float phase = henyeyGreenstein(param_8, param_9);
            float3 directLight = ((float3(_1929.sunColor) * _1929.sunIntensity) * lightTrans) * phase;
            float3 ambientLight = float3(_1929.cloudAmbientColor);
            float3 lighting = (((ambientLight * 0.3499999940395355224609375) + directLight) * sampleWeight) * _1929.cloudScattering;
            accumulatedLight += (lighting * transmittance);
            transmittance *= exp(((-density) * adaptiveStep) * _1929.cloudAbsorption);
            travelled += adaptiveStep;
            continue;
        }
        float emptyAdvance = fast::min(stepSize * 2.25, remainingDistance);
        float minAdvance = fast::min(stepSize * 0.5, remainingDistance);
        travelled += fast::max(emptyAdvance, minAdvance);
    }
    float3 finalColor = (inColor.xyz * transmittance) + accumulatedLight;
    return float4(fast::clamp(finalColor, float3(0.0), float3(1.0)), inColor.w);
}

static inline __attribute__((always_inline))
float4 sampleBright(thread const float2& uv, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> BrightTexture, sampler BrightTextureSmplr)
{
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 7)
        {
            float redOffset = _394.EffectFloat1[i];
            float greenOffset = _403.EffectFloat2[i];
            float blueOffset = _411.EffectFloat3[i];
            float2 focusPoint = float2(_419.EffectFloat4[i], _426.EffectFloat5[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * redOffset))).x;
            float green = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * greenOffset))).y;
            float2 blue = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (_381.Effects[i] == 9)
            {
                float pixelSizeInPixels = _394.EffectFloat1[i];
                float2 texSize = float2(int2(BrightTexture.get_width(), BrightTexture.get_height()));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                float4 color = BrightTexture.sample(BrightTextureSmplr, pixelated);
                return color;
            }
            else
            {
                if (_381.Effects[i] == 10)
                {
                    float radius = _394.EffectFloat1[i];
                    float separation = _403.EffectFloat2[i];
                    float2 texelSize = float2(1.0) / float2(int2(BrightTexture.get_width(), BrightTexture.get_height()));
                    float3 maxColor = BrightTexture.sample(BrightTextureSmplr, uv).xyz;
                    int range = int(radius);
                    float radiusSq = radius * radius;
                    int _766 = -range;
                    for (int x = _766; x <= range; x++)
                    {
                        int _777 = -range;
                        for (int y = _777; y <= range; y++)
                        {
                            float distSq = float((x * x) + (y * y));
                            if (distSq <= radiusSq)
                            {
                                float2 offset = (float2(float(x), float(y)) * texelSize) * separation;
                                float3 sampled = BrightTexture.sample(BrightTextureSmplr, (uv + offset)).xyz;
                                maxColor = fast::max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, BrightTexture.sample(BrightTextureSmplr, uv).w);
                }
            }
        }
    }
    return BrightTexture.sample(BrightTextureSmplr, uv);
}

static inline __attribute__((always_inline))
float4 composeSSR(thread const float4& color, thread const float2& uv, constant PushConstants& _372, texture2d<float> SSRTexture, sampler SSRTextureSmplr)
{
    if (_372.hasSSRTexture != 1)
    {
        return color;
    }
    float4 ssr = SSRTexture.sample(SSRTextureSmplr, uv);
    float reflectionWeight = fast::clamp(ssr.w, 0.0, 1.0);
    float baseWeight = 1.0 - reflectionWeight;
    float4 result = color;
    result.xyz = (result.xyz * baseWeight) + (ssr.xyz * reflectionWeight);
    return result;
}

static inline __attribute__((always_inline))
float4 composeLighting(thread const float2& uv, thread const float4& baseColor, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> BrightTexture, sampler BrightTextureSmplr, texture2d<float> VolumetricLightTexture, sampler VolumetricLightTextureSmplr, texture2d<float> SSRTexture, sampler SSRTextureSmplr)
{
    float4 color = baseColor;
    if (_372.hasBrightTexture == 1)
    {
        color += sampleBright(uv, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr);
    }
    if (_372.hasVolumetricLightTexture == 1)
    {
        color += VolumetricLightTexture.sample(VolumetricLightTextureSmplr, uv);
    }
    return composeSSR(color, uv, _372, SSRTexture, SSRTextureSmplr);
}

static inline __attribute__((always_inline))
float4 applyMotionBlur(thr)",
R"(ead const float2& texCoord, thread const float& size, thread const float& separation, thread const float4& color, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr, texture2d<float> BrightTexture, sampler BrightTextureSmplr, constant Uniforms& _849, texture2d<float> VolumetricLightTexture, sampler VolumetricLightTextureSmplr, texture2d<float> SSRTexture, sampler SSRTextureSmplr, texture2d<float> PositionTexture, sampler PositionTextureSmplr, texture2d<float> DepthTexture, sampler DepthTextureSmplr)
{
    float4 fallbackColor = composeLighting(texCoord, color, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
    if ((size <= 0.0) || (separation <= 0.0))
    {
        return fallbackColor;
    }
    if (_372.hasPositionTexture != 1)
    {
        return fallbackColor;
    }
    float4 worldPos = PositionTexture.sample(PositionTextureSmplr, texCoord);
    if (worldPos.w <= 0.0)
    {
        return fallbackColor;
    }
    float3 viewSpacePos = (_849.viewMatrix * worldPos).xyz;
    float distanceToCamera = length(viewSpacePos);
    if (distanceToCamera < (_849.nearPlane * 2.0))
    {
        return fallbackColor;
    }
    float4 currentClipPos = (_849.projectionMatrix * _849.viewMatrix) * worldPos;
    float _1543 = currentClipPos.w;
    float4 _1544 = currentClipPos;
    float3 _1547 = _1544.xyz / float3(_1543);
    currentClipPos.x = _1547.x;
    currentClipPos.y = _1547.y;
    currentClipPos.z = _1547.z;
    float2 currentUV = (currentClipPos.xy * 0.5) + float2(0.5);
    float4 prevClipPos = (_849.projectionMatrix * _849.lastViewMatrix) * worldPos;
    float _1569 = prevClipPos.w;
    float4 _1570 = prevClipPos;
    float3 _1573 = _1570.xyz / float3(_1569);
    prevClipPos.x = _1573.x;
    prevClipPos.y = _1573.y;
    prevClipPos.z = _1573.z;
    float2 prevUV = (prevClipPos.xy * 0.5) + float2(0.5);
    float2 velocity = (currentUV - prevUV) * separation;
    float velocityLength = length(velocity);
    float maxVelocity = 0.119999997317790985107421875;
    if (velocityLength > maxVelocity)
    {
        velocity = fast::normalize(velocity) * maxVelocity;
        velocityLength = maxVelocity;
    }
    if (_372.hasSSRTexture == 1)
    {
        float mirrorWeight = fast::clamp(SSRTexture.sample(SSRTextureSmplr, texCoord).w, 0.0, 1.0);
        float blurDamp = mix(1.0, 0.20000000298023223876953125, mirrorWeight);
        velocity *= blurDamp;
        velocityLength *= blurDamp;
    }
    if (velocityLength < 9.9999997473787516355514526367188e-05)
    {
        return fallbackColor;
    }
    int baseSamples = clamp(int(size), 2, 32);
    float sampleScale = mix(0.75, 1.75, fast::clamp(velocityLength / maxVelocity, 0.0, 1.0));
    int samples = clamp(int(float(baseSamples) * sampleScale), 3, 48);
    float centerDepth = 0.0;
    float depthSoftness = 1.0;
    if (_372.hasDepthTexture == 1)
    {
        float centerDepthRaw = DepthTexture.sample(DepthTextureSmplr, texCoord).x;
        centerDepth = LinearizeDepth(centerDepthRaw, _849);
        depthSoftness = fast::max(0.00200000009499490261077880859375, centerDepth * 0.0199999995529651641845703125);
    }
    float jitter = (fract(sin(dot(texCoord * float2(173.3000030517578125, 97.09999847412109375), float2(12.98980045318603515625, 78.233001708984375))) * 43758.546875) - 0.5) * 0.3499999940395355224609375;
    float4 result = float4(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float t = (((float(i) + 0.5) / float(samples)) * 2.0) - 1.0;
        t += jitter / float(samples);
        float2 sampleCoord = texCoord + (velocity * t);
        bool _1642 = sampleCoord.x < 0.0;
        bool _1648;
        if (!_1642)
        {
            _1648 = sampleCoord.x > 1.0;
        }
        else
        {
            _1648 = _1642;
        }
        bool _1654;
        if (!_1648)
        {
            _1654 = sampleCoord.y < 0.0;
        }
        else
        {
            _1654 = _1648;
        }
        bool _1660;
        if (!_1654)
        {
            _1660 = sampleCoord.y > 1.0;
        }
        else
        {
            _1660 = _1654;
        }
        if (_1660)
        {
            continue;
        }
        float depthWeight = 1.0;
        if (_372.hasDepthTexture == 1)
        {
            float sampleDepthRaw = DepthTexture.sample(DepthTextureSmplr, sampleCoord).x;
            float sampleDepth = LinearizeDepth(sampleDepthRaw, _849);
            float depthDelta = abs(sampleDepth - centerDepth);
            depthWeight = 1.0 - smoothstep(depthSoftness, depthSoftness * 2.5, depthDelta);
            if (depthWeight <= 0.0)
            {
                continue;
            }
        }
        float4 sampled = sampleColor(sampleCoord, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
        sampled = composeLighting(sampleCoord, sampled, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
        float gaussian = exp((-4.0) * t * t);
        float weight = gaussian * depthWeight;
        result += (sampled * weight);
        totalWeight += weight;
    }
    if (totalWeight > 0.0)
    {
        return result / float4(totalWeight);
    }
    return fallbackColor;
}

static inline __attribute__((always_inline))
float3 sampleLUT(thread const float3& rgb, thread const float& blueSlice, thread const float& sliceSize, thread const float& slicePixelOffset, constant PushConstants& _372, texture2d<float> LUTTexture, sampler LUTTextureSmplr)
{
    float sliceY = floor(blueSlice / _372.lutSize);
    float sliceX = mod(blueSlice, _372.lutSize);
    float2 uv;
    uv.x = ((sliceX * sliceSize) + slicePixelOffset) + (rgb.x * sliceSize);
    uv.y = ((sliceY * sliceSize) + slicePixelOffset) + (rgb.y * sliceSize);
    return LUTTexture.sample(LUTTextureSmplr, uv).xyz;
}

static inline __attribute__((always_inline))
float4 mapToLUT(thread const float4& color, constant PushConstants& _372, texture2d<float> LUTTexture, sampler LUTTextureSmplr)
{
    if (_372.hasLUTTexture != 1)
    {
        return color;
    }
    float sliceSize = 1.0 / _372.lutSize;
    float slicePixelOffset = sliceSize * 0.5;
    float blueIndex = color.z * (_372.lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = fast::min(sliceLow + 1.0, _372.lutSize - 1.0);
    float t = blueIndex - sliceLow;
    float3 param = color.xyz;
    float param_1 = sliceLow;
    float param_2 = sliceSize;
    float param_3 = slicePixelOffset;
    float3 lowColor = sampleLUT(param, param_1, param_2, param_3, _372, LUTTexture, LUTTextureSmplr);
    float3 param_4 = color.xyz;
    float param_5 = sliceHigh;
    float param_6 = sliceSize;
    float param_7 = slicePixelOffset;
    float3 highColor = sampleLUT(param_4, param_5, param_6, param_7, _372, LUTTexture, LUTTextureSmplr);
    float3 finalRGB = mix(lowColor, highColor, float3(t));
    return float4(finalRGB, color.w);
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread const float3& color)
{
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    return fast::clamp((color * ((color * a) + float3(b))) / ((color * ((color * c) + float3(d))) + float3(e)), float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant PushConstants& _372 [[buffer(0)]], device EffectBuffer& _381 [[buffer(1)]], device EffectFloat1Buffer& _394 [[buffer(2)]], device EffectFloat2Buffer& _403 [[buffer(3)]], device EffectFloat3Buffer& _411 [[buffer(4)]], device EffectFloat4Buffer& _419 [[buffer(5)]], device EffectFloat5Buffer& _426 [[buffer(6)]], constant Uniforms& _849 [[buffer(7)]], device EffectFloat6Bu)",
R"(ffer& _1049 [[buffer(8)]], constant Clouds& _1929 [[buffer(9)]], constant Environment& environment [[buffer(10)]], texture2d<float> Texture [[texture(0)]], texture2d<float> BrightTexture [[texture(1)]], texture2d<float> VolumetricLightTexture [[texture(2)]], texture2d<float> SSRTexture [[texture(3)]], texture2d<float> PositionTexture [[texture(4)]], texture2d<float> LUTTexture [[texture(5)]], texture3d<float> cloudsTexture [[texture(6)]], texture2d<float> DepthTexture [[texture(7)]], sampler TextureSmplr [[sampler(0)]], sampler BrightTextureSmplr [[sampler(1)]], sampler VolumetricLightTextureSmplr [[sampler(2)]], sampler SSRTextureSmplr [[sampler(3)]], sampler PositionTextureSmplr [[sampler(4)]], sampler LUTTextureSmplr [[sampler(5)]], sampler cloudsTextureSmplr [[sampler(6)]], sampler DepthTextureSmplr [[sampler(7)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float2 param = in.TexCoord;
    float4 color = sampleColor(param, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
    float depth = 1.0;
    if (_372.hasDepthTexture == 1)
    {
        depth = DepthTexture.sample(DepthTextureSmplr, in.TexCoord).x;
    }
    float2 param_1 = in.TexCoord;
    float param_2 = depth;
    float3 viewPos = reconstructViewPos(param_1, param_2, _849);
    float _distance = _372.hasDepthTexture == 1 ? length(viewPos) : _849.farPlane;
    bool useMotionBlur = false;
    float motionBlurSize = 0.0;
    float motionBlurSeparation = 0.0;
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 6)
        {
            useMotionBlur = true;
            motionBlurSize = _394.EffectFloat1[i];
            motionBlurSeparation = _403.EffectFloat2[i];
        }
    }
    for (int i_1 = 0; i_1 < _372.EffectCount; i_1++)
    {
        if (_381.Effects[i_1] == 2)
        {
            color = sharpen(Texture, TextureSmplr, in.TexCoord);
        }
        else
        {
            if (_381.Effects[i_1] == 3)
            {
                float radius = _394.EffectFloat1[i_1];
                float param_3 = radius;
                color = blur(Texture, TextureSmplr, param_3, in.TexCoord);
            }
            else
            {
                if (_381.Effects[i_1] == 4)
                {
                    color = edgeDetection(Texture, TextureSmplr, in.TexCoord);
                }
            }
        }
    }
    float2 param_4 = in.TexCoord;
    color = applyFXAA(Texture, TextureSmplr, param_4, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
    float4 param_5 = color;
    float4 _2637 = applyColorEffects(param_5, _372, _381, _394, _403, _411, _419, _426, _849, _1049, gl_FragCoord);
    color = _2637;
    if (_372.hasDepthTexture == 1)
    {
        float depthValue = DepthTexture.sample(DepthTextureSmplr, in.TexCoord).x;
        float param_6 = depthValue;
        float linearDepth = LinearizeDepth(param_6, _849);
        float coc = fast::clamp(abs(linearDepth - _849.focusDepth) / _849.focusRange, 0.0, 1.0);
        float mip = (coc * float(_849.maxMipLevel)) * 1.2000000476837158203125;
        float4 param_7 = Texture.sample(TextureSmplr, in.TexCoord, level(mip));
        float4 _2676 = applyColorEffects(param_7, _372, _381, _394, _403, _411, _419, _426, _849, _1049, gl_FragCoord);
        float3 blurred = _2676.xyz;
        float3 sharp = color.xyz;
        float4 param_8 = color;
        color = cloudRendering(param_8, in.TexCoord, _372, _849, _1929, cloudsTexture, cloudsTextureSmplr, DepthTexture, DepthTextureSmplr);
    }
    else
    {
        float4 param_9 = color;
        color = cloudRendering(param_9, in.TexCoord, _372, _849, _1929, cloudsTexture, cloudsTextureSmplr, DepthTexture, DepthTextureSmplr);
    }
    float4 hdrColor;
    if (useMotionBlur)
    {
        float2 param_10 = in.TexCoord;
        float param_11 = motionBlurSize;
        float param_12 = motionBlurSeparation;
        float4 param_13 = color;
        float4 motionBlurred = applyMotionBlur(param_10, param_11, param_12, param_13, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr, BrightTexture, BrightTextureSmplr, _849, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr, PositionTexture, PositionTextureSmplr, DepthTexture, DepthTextureSmplr);
        out.FragColor = motionBlurred;
        return out;
    }
    else
    {
        float2 param_14 = in.TexCoord;
        float4 param_15 = color;
        hdrColor = composeLighting(param_14, param_15, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
    }
    float4 param_16 = hdrColor;
    hdrColor = mapToLUT(param_16, _372, LUTTexture, LUTTextureSmplr);
    float3 param_17 = hdrColor.xyz;
    float3 _2744 = acesToneMapping(param_17);
    hdrColor.x = _2744.x;
    hdrColor.y = _2744.y;
    hdrColor.z = _2744.z;
    float fogFactor = 1.0 - exp((-_distance) * environment.fogIntensity);
    float3 finalColor = mix(hdrColor.xyz, float3(environment.fogColor), float3(fogFactor));
    out.FragColor = float4(finalColor, 1.0);
    return out;
}
)",
};
static const AtlasPackedShaderSource FULLSCREEN_FRAG = {FULLSCREEN_FRAG_PARTS, 6};

static const char* const FULLSCREEN_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.aPos.xy, 0.0, 1.0);
    out.TexCoord = in.aTexCoord;
    return out;
}

)",
};
static const AtlasPackedShaderSource FULLSCREEN_VERT = {FULLSCREEN_VERT_PARTS, 1};

static const char* const GAUSSIAN_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    uint horizontal;
    float4 weight[5];
    float radius;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _29 [[buffer(0)]], texture2d<float> image [[texture(0)]], sampler imageSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 tex_offset = (float2(1.0) / float2(int2(image.get_width(), image.get_height()))) * _29.radius;
    float3 result = image.sample(imageSmplr, in.TexCoord).xyz * _29.weight[0].x;
    if (_29.horizontal != 0u)
    {
        for (int i = 1; i < 5; i++)
        {
            result += (image.sample(imageSmplr, (in.TexCoord + float2(tex_offset.x * float(i), 0.0))).xyz * _29.weight[i].x);
            result += (image.sample(imageSmplr, (in.TexCoord - float2(tex_offset.x * float(i), 0.0))).xyz * _29.weight[i].x);
        }
    }
    else
    {
        for (int i_1 = 1; i_1 < 5; i_1++)
        {
            result += (image.sample(imageSmplr, (in.TexCoord + float2(0.0, tex_offset.y * float(i_1)))).xyz * _29.weight[i_1].x);
            result += (image.sample(imageSmplr, (in.TexCoord - float2(0.0, tex_offset.y * float(i_1)))).xyz * _29.weight[i_1].x);
        }
    }
    out.FragColor = float4(result, 1.0);
    return out;
}

)",
};
static const AtlasPackedShaderSource GAUSSIAN_FRAG = {GAUSSIAN_FRAG_PARTS, 1};

static const char* const LIGHT_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template <typename T, size_t Num> struct spvUnsafeArray {
    T elements[Num ? Num : 1];

    thread T &operator[](size_t pos) thread { return elements[pos]; }
    constexpr const thread T &operator[](size_t pos) const thread {
        return elements[pos];
    }

    device T &operator[](size_t pos) device { return elements[pos]; }
    constexpr const device T &operator[](size_t pos) const device {
        return elements[pos];
    }

    constexpr const constant T &operator[](size_t pos) const constant {
        return elements[pos];
    }

    threadgroup T &operator[](size_t pos) threadgroup { return elements[pos]; }
    constexpr const threadgroup T &operator[](size_t pos) const threadgroup {
        return elements[pos];
    }
};

// Implementation of the GLSL radians() function
template <typename T> inline T radians(T d) { return d * T(0.01745329251); }

// Returns the determinant of a 2x2 matrix.
static inline __attribute__((always_inline)) float
spvDet2x2(float a1, float a2, float b1, float b2) {
    return a1 * b2 - b1 * a2;
}

// Returns the determinant of a 3x3 matrix.
static inline __attribute__((always_inline)) float
spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1,
          float c2, float c3) {
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) +
           c1 * spvDet2x2(a2, a3, b2, b3);
}

// Returns the inverse of a matrix, by using the algorithm of calculating the
// classical adjoint and dividing by the determinant. The contents of the matrix
// are changed.
static inline __attribute__((always_inline)) float4x4
spvInverse4x4(float4x4 m) {
    float4x4 adj; // The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the
    // matrix.
    adj[0][0] = spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3],
                          m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3],
                           m[3][1], m[3][2], m[3][3]);
    adj[0][2] = spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3],
                          m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3],
                           m[2][1], m[2][2], m[2][3]);

    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3],
                           m[3][0], m[3][2], m[3][3]);
    adj[1][1] = spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3],
                          m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3],
                           m[3][0], m[3][2], m[3][3]);
    adj[1][3] = spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3],
                          m[2][0], m[2][2], m[2][3]);

    adj[2][0] = spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3],
                          m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3],
                           m[3][0], m[3][1], m[3][3]);
    adj[2][2] = spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3],
                          m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3],
                           m[2][0], m[2][1], m[2][3]);

    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],
                           m[3][0], m[3][1], m[3][2]);
    adj[3][1] = spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2],
                          m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
                           m[3][0], m[3][1], m[3][2]);
    adj[3][3] = spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
                          m[2][0], m[2][1], m[2][2]);

    // Calculate the determinant as a combination of the cofactors of the first
    // row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) +
                (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct ShadowParameters {
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    int lightIndex;
    float3 lightPos;
    int lightType;
};

struct DirectionalLight {
    float3 direction;
    float _pad1;
    float3 diffuse;
    float _pad2;
    float3 specular;
    float intensity;
};

struct PointLight {
    float3 position;
    float _pad1;
    float3 diffuse;
    float _pad2;
    float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct SpotLight {
    float3 position;
    float _pad1;
    float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    float3 diffuse;
    float _pad5;
    float3 specular;
    float _pad6;
};

struct UBO {
    packed_float3 cameraPosition;
    uint useIBL;
};

struct Environment {
    float rimLightIntensity;
    float3 rimLightColor;
};

struct PushConstants {
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
};

struct ShadowParameters_1 {
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    int lightIndex;
    packed_float3 lightPos;
    int lightType;
};

struct ShadowParams {
    spvUnsafeArray<ShadowParameters_1, 1> shadowParams;
};

struct DirectionalLight_1 {
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct DirectionalLights {
    spvUnsafeArray<DirectionalLight_1, 1> directionalLights;
};

struct PointLight_1 {
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

struct PointLights {
    spvUnsafeArray<PointLight_1, 1> pointLights;
};

struct SpotLight_1 {
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

struct SpotLights {
    spvUnsafeArray<SpotLight_1, 1> spotlights;
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

struct AreaLights {
    spvUnsafeArray<AreaLight, 1> areaLights;
};

struct AmbientLight {
    float4 color;
    float intensity;
    float3 _pad0;
};

struct ProbeSpace {
    float3 origin;
    float _pad0;

    float3 spacing;
    float _pad1;

    float3 probeCount;
    float _pad2;

    float4 debugColor;

    float4 atlasParams;
    // x = textureBorderSize
    // y = probeResolution (inner)
    // z = probesPerRow
    // w = totalProbes  (put this here on CPU!)
};

constant spvUnsafeArray<float2, 12> _660 = spvUnsafeArray<float2, 12>(
    {float2(-0.3260000050067901611328125, -0.4059999883174896240234375),
     float2(-0.839999973773956298828125, -0.07400000095367431640625),
     float2(-0.69599997997283935546875, 0.4569999873638153076171875),
     float2(-0.20299999415874481201171875, 0.620999991893768310546875),
     float2(0.96200001)",
R"(239776611328125, -0.194999992847442626953125),
     float2(0.472999989986419677734375, -0.4799999892711639404296875),
     float2(0.518999993801116943359375, 0.767000019550323486328125),
     float2(0.185000002384185791015625, -0.89300000667572021484375),
     float2(0.507000029087066650390625, 0.064000003039836883544921875),
     float2(0.89600002765655517578125, 0.412000000476837158203125),
     float2(-0.3219999969005584716796875, -0.933000028133392333984375),
     float2(-0.791999995708465576171875, -0.597999989986419677734375)});
constant spvUnsafeArray<float3, 20> _861 = spvUnsafeArray<float3, 20>(
    {float3(0.5381000041961669921875, 0.18559999763965606689453125,
            -0.4318999946117401123046875),
     float3(0.13789999485015869140625, 0.248600006103515625,
            0.4429999887943267822265625),
     float3(0.3370999991893768310546875, 0.567900002002716064453125,
            -0.0057000000961124897003173828125),
     float3(-0.699899971485137939453125, -0.0450999997556209564208984375,
            -0.0019000000320374965667724609375),
     float3(0.068899996578693389892578125, -0.159799993038177490234375,
            -0.854700028896331787109375),
     float3(0.056000001728534698486328125, 0.0068999999202787876129150390625,
            -0.184300005435943603515625),
     float3(-0.014600000344216823577880859375, 0.14020000398159027099609375,
            0.076200000941753387451171875),
     float3(0.00999999977648258209228515625, -0.19239999353885650634765625,
            -0.03440000116825103759765625),
     float3(-0.35769999027252197265625, -0.53009998798370361328125,
            -0.4357999861240386962890625),
     float3(-0.3169000148773193359375, 0.10629999637603759765625,
            0.015799999237060546875),
     float3(0.010300000198185443878173828125, -0.5868999958038330078125,
            0.0046000001020729541778564453125),
     float3(-0.08969999849796295166015625, -0.4939999878406524658203125,
            0.328700006008148193359375),
     float3(0.7118999958038330078125, -0.015399999916553497314453125,
            -0.091799996793270111083984375),
     float3(-0.053300000727176666259765625, 0.0595999993383884429931640625,
            -0.541100025177001953125),
     float3(0.03519999980926513671875, -0.063100002706050872802734375,
            0.546000003814697265625),
     float3(-0.4776000082492828369140625, 0.2847000062465667724609375,
            -0.0271000005304813385009765625),
     float3(-0.11200000345706939697265625, 0.1234000027179718017578125,
            -0.744599997997283935546875),
     float3(-0.212999999523162841796875, -0.07819999754428863525390625,
            -0.13789999485015869140625),
     float3(0.2944000065326690673828125, -0.3111999928951263427734375,
            -0.2644999921321868896484375),
     float3(-0.4564000070095062255859375, 0.4174999892711639404296875,
            -0.184300005435943603515625)});

struct main0_out {
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in {
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline)) float2 getTextureDimensions(
    thread const int &textureIndex, texture2d<float> texture1,
    sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr,
    texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4,
    sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr) {
    if (textureIndex == 0) {
        return float2(int2(texture1.get_width(), texture1.get_height()));
    } else {
        if (textureIndex == 1) {
            return float2(int2(texture2.get_width(), texture2.get_height()));
        } else {
            if (textureIndex == 2) {
                return float2(
                    int2(texture3.get_width(), texture3.get_height()));
            } else {
                if (textureIndex == 3) {
                    return float2(
                        int2(texture4.get_width(), texture4.get_height()));
                } else {
                    if (textureIndex == 4) {
                        return float2(
                            int2(texture5.get_width(), texture5.get_height()));
                    }
                }
            }
        }
    }
    return float2(0.0);
}

static inline __attribute__((always_inline)) float4 sampleCubeTextureAt(
    thread const int &textureIndex, thread const float3 &direction,
    texturecube<float> cubeMap1, sampler cubeMap1Smplr,
    texturecube<float> cubeMap2, sampler cubeMap2Smplr,
    texturecube<float> cubeMap3, sampler cubeMap3Smplr,
    texturecube<float> cubeMap4, sampler cubeMap4Smplr,
    texturecube<float> cubeMap5, sampler cubeMap5Smplr) {
    if (textureIndex == 0) {
        return cubeMap1.sample(cubeMap1Smplr, direction);
    } else {
        if (textureIndex == 1) {
            return cubeMap2.sample(cubeMap2Smplr, direction);
        } else {
            if (textureIndex == 2) {
                return cubeMap3.sample(cubeMap3Smplr, direction);
            } else {
                if (textureIndex == 3) {
                    return cubeMap4.sample(cubeMap4Smplr, direction);
                } else {
                    if (textureIndex == 4) {
                        return cubeMap5.sample(cubeMap5Smplr, direction);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline)) float calculatePointShadow(
    thread const ShadowParameters &shadowParam, thread const float3 &fragPos,
    texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2,
    sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr,
    texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5,
    sampler texture5Smplr, texturecube<float> cubeMap1, sampler cubeMap1Smplr,
    texturecube<float> cubeMap2, sampler cubeMap2Smplr,
    texturecube<float> cubeMap3, sampler cubeMap3Smplr,
    texturecube<float> cubeMap4, sampler cubeMap4Smplr,
    texturecube<float> cubeMap5, sampler cubeMap5Smplr) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture1Smplr, texture2, texture2Smplr, texture3,
        texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr);
    bool _739 = dims.x == 0.0;
    bool _746;
    if (!_739) {
        _746 = dims.y == 0.0;
    } else {
        _746 = _739;
    }
    if (_746) {
        return 0.0;
    }
    float3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);
    if (currentDepth >= shadowParam.farPlane) {
        return 0.0;
    }
    float bias0 = 0.0500000007450580596923828125;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) *
                       0.0500000007450580596923828125;
    for (int i = 0; i < 10; i++) {
        float3 sampleDir =
            fast::normalize(fragToLight + (_861[i] * diskRadius));
        int param_1 = shadowParam.textureIndex;
        float3 param_2 = sampleDir;
        float closestDepth =
            sampleCubeTextureAt(param_1, param_2, cubeMap1, cubeMap1Smplr,
                                cubeMap2, cubeMap2Smplr, cubeMap3,
                                cubeMap3Smplr, cubeMap4, cubeMap4Smplr,
                                cubeMap5, cubeMap5Smplr)
                .x *
            shadowParam.farPlane;
        if ((currentDepth - bias0) > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= 10.0;
    return shadow;
}

static inline __attribute__((always_inline)) float4 sampleTextureAt(
    thread const int &textureIndex, thread const float2 &uv,
    texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2,
    sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr,
    texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5,
    sampler texture5Smplr) {
    if (textureIndex == 0) {
        return texture1.sample(texture1Smplr, uv);
    } else {
        if (textureIndex == 1) {
            return texture2.sample(texture2Smplr, uv);
        } else {
            if (textureIn)",
R"(dex == 2) {
                return texture3.sample(texture3Smplr, uv);
            } else {
                if (textureIndex == 3) {
                    return texture4.sample(texture4Smplr, uv);
                } else {
                    if (textureIndex == 4) {
                        return texture5.sample(texture5Smplr, uv);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline)) float calculateShadow(
    thread const ShadowParameters &shadowParam, thread const float3 &fragPos,
    thread const float3 &normal, texture2d<float> texture1,
    sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr,
    texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4,
    sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr,
    constant UBO &_526) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture1Smplr, texture2, texture2Smplr, texture3,
        texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr);
    bool _411 = dims.x == 0.0;
    bool _419;
    if (!_411) {
        _419 = dims.y == 0.0;
    } else {
        _419 = _411;
    }
    if (_419) {
        return 0.0;
    }
    float4 fragPosLightSpace =
        (shadowParam.lightProjection * shadowParam.lightView) *
        float4(fragPos, 1.0);
    if (fragPosLightSpace.w == 0.0) {
        return 0.0;
    }
    float3 clipCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    float2 projCoordsXY = (clipCoords.xy * 0.5) + float2(0.5);
    float3 projCoords = float3(projCoordsXY, clipCoords.z);
    bool _456 = projCoords.x < 0.0;
    bool _463;
    if (!_456) {
        _463 = projCoords.x > 1.0;
    } else {
        _463 = _456;
    }
    bool _470;
    if (!_463) {
        _470 = projCoords.y < 0.0;
    } else {
        _470 = _463;
    }
    bool _477;
    if (!_470) {
        _477 = projCoords.y > 1.0;
    } else {
        _477 = _470;
    }
    bool _485;
    if (!_477) {
        _485 = projCoords.z < 0.0;
    } else {
        _485 = _477;
    }
    bool _493;
    if (!_485) {
        _493 = projCoords.z > 1.0;
    } else {
        _493 = _485;
    }
    if (_493) {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    bool isAreaShadow = shadowParam.lightType == 2;
    float3 lightDirWorld = fast::normalize(
        -(spvInverse4x4(shadowParam.lightView) * float4(0.0, 0.0, -1.0, 0.0))
             .xyz);
    float biasValue = shadowParam.bias0;
    if (isAreaShadow) {
        biasValue = fast::max(biasValue, 0.0012000000569969416);
    }
    float ndotl = fast::max(dot(normal, lightDirWorld), 0.0);
    float minBias =
        fast::max(isAreaShadow ? 0.0002500000118743628
                               : 4.9999998736893758177757263183594e-05,
                  biasValue * (isAreaShadow ? 0.8500000238418579 : 0.25));
    float bias0 = fast::max(biasValue * (1.0 - ndotl), minBias);
    if (isAreaShadow) {
        bias0 += 0.0002500000118743628;
    }
    float shadow = 0.0;
    float2 texelSize = float2(1.0) / dims;
    float _distance = length(float3(_526.cameraPosition) - fragPos);
    float2 shadowMapSize = dims;
    float avgDim = 0.5 * (shadowMapSize.x + shadowMapSize.y);
    float resFactor = fast::clamp(1024.0 / fast::max(avgDim, 1.0), 0.75, 1.25);
    float distFactor = fast::clamp(_distance / 800.0, 0.0, 1.0);
    float texelRadius =
        mix(isAreaShadow ? 1.7999999523162842 : 1.0,
            isAreaShadow ? 4.199999809265137 : 3.0, distFactor) *
        resFactor;
    float2 filterRadius = texelSize * texelRadius;
    int kernelSamples = isAreaShadow ? 6 : 8;
    int sampleCount = 0;
    for (int i = 0; i < kernelSamples; i++) {
        float2 offset = _660[i] * filterRadius;
        float2 uv = projCoords.xy + offset;
        bool _676 = uv.x < 0.0;
        bool _683;
        if (!_676) {
            _683 = uv.x > 1.0;
        } else {
            _683 = _676;
        }
        bool _690;
        if (!_683) {
            _690 = uv.y < 0.0;
        } else {
            _690 = _683;
        }
        bool _697;
        if (!_690) {
            _697 = uv.y > 1.0;
        } else {
            _697 = _690;
        }
        if (_697) {
            continue;
        }
        int param_1 = shadowParam.textureIndex;
        float2 param_2 = uv;
        float pcfDepth =
            sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2,
                            texture2Smplr, texture3, texture3Smplr, texture4,
                            texture4Smplr, texture5, texture5Smplr)
                .x;
        shadow += float((currentDepth - bias0) > pcfDepth);
        sampleCount++;
    }
    if (sampleCount > 0) {
        shadow /= float(sampleCount);
    }
    if (isAreaShadow) {
        shadow = smoothstep(0.18000000715255737, 0.9200000166893005, shadow);
    }
    return shadow;
}

static inline __attribute__((always_inline)) float
distributionGGX(thread const float3 &N, thread const float3 &H,
                thread const float &roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = fast::max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)) + 1.0;
    denom = (3.1415927410125732421875 * denom) * denom;
    return num / fast::max(denom, 9.9999997473787516355514526367188e-05);
}

static inline __attribute__((always_inline)) float
geometrySchlickGGX(thread const float &NdotV, thread const float &roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = (NdotV * (1.0 - k)) + k;
    return num / fast::max(denom, 9.9999997473787516355514526367188e-05);
}

static inline __attribute__((always_inline)) float
geometrySmith(thread const float3 &N, thread const float3 &V,
              thread const float3 &L, thread const float &roughness) {
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float param = NdotV;
    float param_1 = roughness;
    float ggx2 = geometrySchlickGGX(param, param_1);
    float param_2 = NdotL;
    float param_3 = roughness;
    float ggx1 = geometrySchlickGGX(param_2, param_3);
    return ggx1 * ggx2;
}

static inline __attribute__((always_inline)) float3
fresnelSchlick(thread const float &cosTheta, thread const float3 &F0) {
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline)) float3
evaluateBRDF(thread const float3 &L, thread const float3 &radiance,
             thread const float3 &N, thread const float3 &V,
             thread const float3 &F0, thread const float3 &albedo,
             thread const float &metallic, thread const float &roughness) {
    float3 H = fast::normalize(V + L);
    float3 param = N;
    float3 param_1 = H;
    float param_2 = roughness;
    float NDF = distributionGGX(param, param_1, param_2);
    float3 param_3 = N;
    float3 param_4 = V;
    float3 param_5 = L;
    float param_6 = roughness;
    float G = geometrySmith(param_3, param_4, param_5, param_6);
    float param_7 = fast::max(dot(H, V), 0.0);
    float3 param_8 = F0;
    float3 F = fresnelSchlick(param_7, param_8);
    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float3 numerator = F * (NDF * G);
    float denominator =
        fast::max((4.0 * NdotV) * NdotL, 9.9999997473787516355514526367188e-05);
    float3 specular = numerator / float3(denominator);
    return ((((kD * albedo) / float3(3.1415927410125732421875)) + specular) *
            radiance) *
           NdotL;
}

static inline __attribute__((always_inline)) float3 calcDirectionalLight(
    thread const DirectionalLight &light, thread const float3 &N,
    thread const float3 &V, thread const float3 &F0,
    thread const float3 &albedo, thread const float &metallic,
    thread const float &roughness) {
    float3 L = fast::normalize(-light.direction);
    float3 radiance = light.diffuse * fast:)",
R"(:max(light.intensity, 0.0);
    float3 param = L;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
calcPointLight(thread const PointLight &light, thread const float3 &fragPos,
               thread const float3 &N, thread const float3 &V,
               thread const float3 &F0, thread const float3 &albedo,
               thread const float &metallic, thread const float &roughness) {
    float3 L = light.position - fragPos;
    float _distance = length(L);
    float3 _1019;
    if (_distance > 0.0) {
        _1019 = L / float3(_distance);
    } else {
        _1019 = float3(0.0, 0.0, 1.0);
    }
    float3 direction = _1019;
    float attenuation =
        1.0 / fast::max((light.constant0 + (light.linear * _distance)) +
                            ((light.quadratic * _distance) * _distance),
                        9.9999997473787516355514526367188e-05);
    float fade = 1.0 - smoothstep(light.radius * 0.89999997615814208984375,
                                  light.radius, _distance);
    float3 radiance =
        ((light.diffuse * fast::max(light.intensity, 0.0)) * attenuation) *
        fade;
    float3 param = direction;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
calcSpotLight(thread const SpotLight &light, thread const float3 &fragPos,
              thread const float3 &N, thread const float3 &V,
              thread const float3 &F0, thread const float3 &albedo,
              thread const float &metallic, thread const float &roughness) {
    float3 L = light.position - fragPos;
    float _distance = length(L);
    float3 direction = fast::normalize(L);
    float3 spotDirection = fast::normalize(light.direction);
    float theta = dot(direction, -spotDirection);
    float epsilon = fast::max(light.cutOff - light.outerCutOff,
                              9.9999997473787516355514526367188e-05);
    float intensity =
        fast::clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    float range = fast::max(light.range, 0.001000000047497451305389404296875);
    float attenuation = 1.0 / ((1.0 + (_distance / range)) +
                               ((_distance * _distance) / (range * range)));
    float fade =
        1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
    float3 radiance =
        (((light.diffuse * fast::max(light.intensity, 0.0)) * attenuation) *
         intensity) *
        fade;
    float3 param = direction;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
getRimLight(thread const float3 &fragPos, thread float3 &N, thread float3 &V,
            thread const float3 &F0, thread const float3 &albedo,
            thread const float &metallic, thread const float &roughness,
            constant UBO &_526, constant Environment &environment) {
    N = fast::normalize(N);
    V = fast::normalize(V);
    float rim = powr(1.0 - fast::max(dot(N, V), 0.0), 3.0);
    rim *= mix(1.2000000476837158203125, 0.300000011920928955078125, roughness);
    float3 rimColor =
        mix(float3(1.0), albedo, float3(metallic)) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, float3(0.5));
    float rimIntensity = environment.rimLightIntensity;
    float3 rimLight = (rimColor * rim) * rimIntensity;
    float dist = length(float3(_526.cameraPosition) - fragPos);
    rimLight /= float3(1.0 + (dist * 0.100000001490116119384765625));
    return rimLight;
}

static inline __attribute__((always_inline)) float3
sampleEnvironmentRadiance(thread const float3 &direction,
                          texturecube<float> skybox, sampler skyboxSmplr) {
    return skybox.sample(skyboxSmplr, direction).xyz;
}

static inline __attribute__((always_inline)) float3
acesToneMapping(thread float3 &color) {
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    color = (color * ((color * a) + float3(b))) /
            ((color * ((color * c) + float3(d))) + float3(e));
    color = powr(fast::clamp(color, float3(0.0), float3(1.0)),
                 float3(0.4545454680919647216796875));
    return color;
}

static inline float2 octEncode(float3 n) {
    n /= (fabs(n.x) + fabs(n.y) + fabs(n.z) + 1e-6f);
    float2 e = n.xy;

    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(e.x >= 0.0f ? 1.0f : -1.0f, e.y >= 0.0f ? 1.0f : -1.0f);
        e = (1.0f - fabs(e.yx)) * signNotZero;
    }
    return e;
}

static inline float3 safeNormalizeDDGI(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10f) {
        return v * rsqrt(len2);
    }
    return fallback;
}

static inline uint probeIndexFromCoord(uint3 c, uint3 counts) {
    return c.x + counts.x * (c.y + counts.y * c.z);
}

static inline float2 ddgiAtlasUV(uint probeIndex, float3 dirWS,
                                 constant ProbeSpace &ps, uint atlasW,
                                 uint atlasH) {
    if (atlasW == 0u || atlasH == 0u) {
        return float2(0.5f);
    }

    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    if (innerRes == 0u || probesPerRow == 0u) {
        return float2(0.5f);
    }

    uint tileRes = innerRes + 2u * border;

    uint tileX = probeIndex % probesPerRow;
    uint tileY = probeIndex / probesPerRow;

    float dirLen = length(dirWS);
    float3 safeDir =
        (dirLen > 1e-6f) ? (dirWS / float3(dirLen)) : float3(0.0f, 1.0f, 0.0f);
    float2 e = octEncode(safeDir);
    float2 innerUV = e * 0.5f + 0.5f;

    float2 innerPx = float2((float)(tileX * tileRes + border),
                            (float)(tileY * tileRes + border)) +
                     innerUV * float(innerRes - 1u);

    return (innerPx + 0.5f) / float2((float)atlasW, (float)atlasH);
}

static inline float4 sampleDDGITextureBilinear(texture2d<float> tex,
                                               float2 uv) {
    uint w = tex.get_width();
    uint h = tex.get_height();
    if (w == 0u || h == 0u) {
        return float4(0.0f);
    }

    float2 pixel = uv * float2((float)w, (float)h) - 0.5f;
    int2 p0 = int2(floor(pixel));
    int2 p1 = p0 + int2(1, 0);
    int2 p2 = p0 + int2(0, 1);
    int2 p3 = p0 + int2(1, 1);

    int maxX = int(w) - 1;
    int maxY = int(h) - 1;
    p0 = int2(clamp(p0.x, 0, maxX), clamp(p0.y, 0, maxY));
    p1 = int2(clamp(p1.x, 0, maxX), clamp(p1.y, 0, maxY));
    p2 = int2(clamp(p2.x, 0, maxX), clamp(p2.y, 0, maxY));
    p3 = int2(clamp(p3.x, 0, maxX), clamp(p3.y, 0, maxY));

    float2 f = fract(pixel);
    float4 c00 = tex.read(uint2((uint)p0.x, (uint)p0.y));
    float4 c10 = tex.read(uint2((uint)p1.x, (uint)p1.y));
    float4 c01 = tex.read(uint2((uint)p2.x, (uint)p2.y));
    float4 c11 = tex.read(uint2((uint)p3.x, (uint)p3.y));
    float4 cx0 = mix(c00, c10, f.x);
    float4 cx1 = mix(c01, c11, f.x);
    return mix(cx0, cx1, f.y);
}

static inline float4
sampleProbeDirectionalRadiance(texture2d<float> ddgiTexture,
                               constant ProbeSpace &ps, uint probeIndex,
                               uint atlasW, uint atlasH, float3 dirWS) {
    float2 uv = ddgiAtlasUV(probeIndex, dirWS, ps, atlasW)",
R"(, atlasH);
    return sampleDDGITextureBilinear(ddgiTexture, uv);
}

static inline float3 sampleDDGIIrradiance(texture2d<float> ddgiTexture,
                                          texture2d<float> ddgiDistance,
                                          constant ProbeSpace &ps, float3 posWS,
                                          float3 normalWS) {
    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);

    if (counts.x == 0u || counts.y == 0u || counts.z == 0u)
        return float3(0.0);

    uint atlasW = ddgiTexture.get_width();
    uint atlasH = ddgiTexture.get_height();
    if (atlasW == 0u || atlasH == 0u) {
        return float3(0.0);
    }

    float normalLen = length(normalWS);
    float3 safeNormal = (normalLen > 1e-6f) ? (normalWS / float3(normalLen))
                                            : float3(0.0f, 1.0f, 0.0f);

    float3 safeSpacing = max(ps.spacing, float3(1e-4f));
    float spacingScale =
        max(max(safeSpacing.x, max(safeSpacing.y, safeSpacing.z)), 1e-4f);
    float3 grid = (posWS - ps.origin) / safeSpacing;

    if (counts.x < 2u || counts.y < 2u || counts.z < 2u) {
        float3 maxCoord = max(float3(0.0f), float3((float)counts.x - 1.0f,
                                                   (float)counts.y - 1.0f,
                                                   (float)counts.z - 1.0f));
        float3 clampedGrid = clamp(grid, float3(0.0f), maxCoord);
        uint3 nearest = uint3(
            (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
            (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
            (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0,
                        int(counts.z) - 1));
        uint pIndex = probeIndexFromCoord(nearest, counts);
        float4 nearestSample = sampleProbeDirectionalRadiance(
            ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
        float3 nearestProbePos = ps.origin + float3(nearest) * safeSpacing;
        float3 nearestDirection = safeNormalizeDDGI(
            posWS - nearestProbePos, safeNormal);
        float4 nearestMoments = sampleProbeDirectionalRadiance(
            ddgiDistance, ps, pIndex, atlasW, atlasH, nearestDirection);
        float nearestDistance = length(posWS - nearestProbePos);
        float nearestVariance = max(nearestMoments.y - nearestMoments.x *
                                                           nearestMoments.x,
                                    0.0001f);
        float nearestDelta = max(nearestDistance - nearestMoments.x, 0.0f);
        float nearestVisibility = nearestDelta <= 0.0f
                                      ? 1.0f
                                      : nearestVariance /
                                            (nearestVariance + nearestDelta *
                                                                   nearestDelta);
        float nearestValidity = isfinite(nearestSample.w)
                                    ? clamp(nearestSample.w, 0.0f, 1.0f)
                                    : 0.0f;
        float3 nearestIrr =
            all(isfinite(nearestSample.xyz)) ? nearestSample.xyz : float3(0.0f);
        nearestIrr *= nearestValidity * nearestVisibility;
        return max(nearestIrr, float3(0.0f));
    }

    float3 maxGrid =
        float3((float)counts.x - 1.0001f, (float)counts.y - 1.0001f,
               (float)counts.z - 1.0001f);
    float3 clampedGrid = clamp(grid, float3(0.0f), maxGrid);

    float3 baseF = floor(clampedGrid);
    float3 frac = clampedGrid - baseF;
    int3 baseI = int3(baseF);
    int3 maxBase =
        int3(int(counts.x) - 2, int(counts.y) - 2, int(counts.z) - 2);
    baseI = clamp(baseI, int3(0), maxBase);

    float3 result = float3(0.0f);
    float weightSum = 0.0f;
    float3 resultNoBackface = float3(0.0f);
    float weightSumNoBackface = 0.0f;

    for (uint dz = 0; dz <= 1u; dz++) {
        for (uint dy = 0; dy <= 1u; dy++) {
            for (uint dx = 0; dx <= 1u; dx++) {

                uint3 pc = uint3(uint(baseI.x) + dx, uint(baseI.y) + dy,
                                 uint(baseI.z) + dz);
                uint pIndex = probeIndexFromCoord(pc, counts);

                float trilinearW = ((dx == 0u) ? (1.0f - frac.x) : frac.x) *
                                   ((dy == 0u) ? (1.0f - frac.y) : frac.y) *
                                   ((dz == 0u) ? (1.0f - frac.z) : frac.z);

                float3 probePos = ps.origin + float3(pc) * safeSpacing;
                float3 surfaceToProbe = probePos - posWS;
                float sDist = length(surfaceToProbe);
                float3 dirToProbe = (sDist > 1e-4f) ? surfaceToProbe / sDist
                                                    : float3(0.0f, 1.0f, 0.0f);
                float frontW = max(dot(safeNormal, dirToProbe), 0.0f);
                float backfaceW =
                    mix(0.180000007152557373046875, 1.0f, frontW * frontW);
                float distNorm = sDist / spacingScale;
                float distNorm2 = distNorm * distNorm;
                float distanceW =
                    1.0f / (1.0f + distNorm2 * 0.85000002384185791015625);

                float w = trilinearW * backfaceW * distanceW;

                float4 irrSample = sampleProbeDirectionalRadiance(
                    ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
                float3 probeToSurface = -surfaceToProbe;
                float4 distanceSample = sampleProbeDirectionalRadiance(
                    ddgiDistance, ps, pIndex, atlasW, atlasH,
                    safeNormalizeDDGI(probeToSurface, safeNormal));
                float variance = max(distanceSample.y -
                                         distanceSample.x * distanceSample.x,
                                     0.0001f);
                float delta = max(sDist - distanceSample.x, 0.0f);
                float visibility = delta <= 0.0f
                                       ? 1.0f
                                       : variance / (variance + delta * delta);
                visibility = visibility * visibility * visibility;
                float probeValidity = isfinite(irrSample.w)
                                          ? clamp(irrSample.w, 0.0f, 1.0f)
                                          : 0.0f;
                float validityW = probeValidity;
                float3 irr = irrSample.xyz;
                if (!all(isfinite(irr))) {
                    irr = float3(0.0f);
                    validityW = 0.0f;
                }
                irr = max(irr, float3(0.0f));
                float weightedW = w * validityW * max(visibility, 0.001f);
                result += irr * weightedW;
                weightSum += weightedW;
                float noBackfaceW = trilinearW * distanceW * validityW *
                                    max(visibility, 0.001f);
                resultNoBackface += irr * noBackfaceW;
                weightSumNoBackface += noBackfaceW;
            }
        }
    }

    if (weightSum > 1e-5f) {
        return result / weightSum;
    }
    if (weightSumNoBackface > 1e-5f) {
        return resultNoBackface / weightSumNoBackface;
    }

    uint3 nearest = uint3(
        (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
        (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
        (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0, int(counts.z) - 1));
    uint nearestIndex = probeIndexFromCoord(nearest, counts);
    float4 fallbackSample = sampleProbeDirectionalRadiance(
        ddgiTexture, ps, nearestIndex, atlasW, atlasH, safeNormal);
    float fallbackValidity =
        isfinite(fallbackSample.w) ? clamp(fallbackSample.w, 0.0f, 1.0f) : 0.0f;
    float3 fallback =
        all(isfinite(fallbackSample.xyz)) ? fallbackSample.xyz : float3(0.0f);
    fallback *= fallbackValidity;
    if (!all(isfinite(fallback))) {
        fallback = float3(0.0f);
    }
    return max(fallback, float3(0.0f));
}

fragment main0_out main0(
    main0_in in [[stage_in]], constant UBO &_526 [[buffer(0)]],
    constant Environment &environment [[buffer(1)]],
   )",
R"( constant PushConstants &_1355 [[buffer(2)]],
    device ShadowParams &_1372 [[buffer(3)]],
    device DirectionalLights &_1422 [[buffer(4)]],
    device PointLights &_1465 [[buffer(5)]],
    device SpotLights &_1510 [[buffer(6)]],
    device AreaLights &_1552 [[buffer(7)]],
    constant AmbientLight &ambientLight [[buffer(8)]],
    constant ProbeSpace &ps [[buffer(9)]],
    texture2d<float> texture1 [[texture(0)]],
    texture2d<float> texture2 [[texture(1)]],
    texture2d<float> texture3 [[texture(2)]],
    texture2d<float> texture4 [[texture(3)]],
    texture2d<float> texture5 [[texture(4)]],
    texturecube<float> cubeMap1 [[texture(5)]],
    texturecube<float> cubeMap2 [[texture(6)]],
    texturecube<float> cubeMap3 [[texture(7)]],
    texturecube<float> cubeMap4 [[texture(8)]],
    texturecube<float> cubeMap5 [[texture(9)]],
    texturecube<float> skybox [[texture(10)]],
    texture2d<float> gPosition [[texture(11)]],
    texture2d<float> gNormal [[texture(12)]],
    texture2d<float> gAlbedoSpec [[texture(13)]],
    texture2d<float> gMaterial [[texture(14)]],
    texture2d<float> ssao [[texture(15)]],
    texture2d<float> irradianceMap [[texture(16)]],
    texture2d<float> ddgiDistanceMap [[texture(17)]],
    sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]],
    sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]],
    sampler texture5Smplr [[sampler(4)]], sampler cubeMap1Smplr [[sampler(5)]],
    sampler cubeMap2Smplr [[sampler(6)]], sampler cubeMap3Smplr [[sampler(7)]],
    sampler cubeMap4Smplr [[sampler(8)]], sampler cubeMap5Smplr [[sampler(9)]],
    sampler skyboxSmplr [[sampler(10)]], sampler gPositionSmplr [[sampler(11)]],
    sampler gNormalSmplr [[sampler(12)]],
    sampler gAlbedoSpecSmplr [[sampler(13)]],
    sampler gMaterialSmplr [[sampler(14)]], sampler ssaoSmplr [[sampler(15)]]) {
    main0_out out{};
    float4 gPositionSample = gPosition.sample(gPositionSmplr, in.TexCoord);
    float3 FragPos = gPositionSample.xyz;
    if (!all(isfinite(FragPos))) {
        FragPos = float3(0.0);
    }
    float depthSample = gPositionSample.w;
    float3 sampledNormal = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    float normalLength = length(sampledNormal);
    bool hasNormal = all(isfinite(sampledNormal)) &&
                     normalLength > 9.9999997473787516355514526367188e-06;
    bool hasDepth =
        isfinite(depthSample) && depthSample < 0.999989986419677734375;
    bool hasGeometry = hasNormal || hasDepth;
    if (!hasGeometry) {
        float2 ndc = in.TexCoord * 2.0f - 1.0f;
        float3 backgroundDir = fast::normalize(float3(ndc.x, -ndc.y, 1.0f));
        float3 backgroundColor =
            sampleEnvironmentRadiance(backgroundDir, skybox, skyboxSmplr);
        out.FragColor = float4(acesToneMapping(backgroundColor), 1.0);
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
        return out;
    }
    float3 N = float3(0.0, 1.0, 0.0);
    if (hasNormal) {
        N = sampledNormal / float3(normalLength);
    } else {
        float3 geomN = cross(dfdx(FragPos), dfdy(FragPos));
        float geomNLen = length(geomN);
        if (all(isfinite(geomN)) &&
            geomNLen > 9.9999997473787516355514526367188e-06) {
            N = geomN / float3(geomNLen);
        }
    }
    float3 geomNormal = cross(dfdx(FragPos), dfdy(FragPos));
    float geomNormalLen = length(geomNormal);
    bool hasGeomNormal = all(isfinite(geomNormal)) &&
                         geomNormalLen > 9.9999997473787516355514526367188e-06;
    float3 shadowNormal = N;
    float3 ddgiNormal = N;
    if (hasGeomNormal) {
        geomNormal /= float3(geomNormalLen);
        if (dot(N, geomNormal) < 0.0) {
            geomNormal = -geomNormal;
        }
        shadowNormal = geomNormal;
        ddgiNormal = geomNormal;
    }
    float4 albedoAo = gAlbedoSpec.sample(gAlbedoSpecSmplr, in.TexCoord);
    float3 albedo = fast::clamp(albedoAo.xyz, float3(0.0), float3(1.0));
    if (!all(isfinite(albedo))) {
        albedo = float3(0.0);
    }
    float4 matData = gMaterial.sample(gMaterialSmplr, in.TexCoord);
    float metallic = fast::clamp(matData.x, 0.0, 1.0);
    float roughness = fast::clamp(matData.y, 0.0, 1.0);
    float ao = fast::clamp(matData.z, 0.0, 1.0);

    float viewDistance =
        fast::max(length(float3(_526.cameraPosition) - FragPos),
                  9.9999999747524270787835121154785e-07);
    float3 V = (float3(_526.cameraPosition) - FragPos) / float3(viewDistance);
    float3 F0 =
        mix(float3(0.039999999105930328369140625), albedo, float3(metallic));
    float ssaoFactor =
        fast::clamp(ssao.sample(ssaoSmplr, in.TexCoord).x, 0.0, 1.0);

    float ssaoContrast =
        fast::clamp(powr(ssaoFactor, 1.10000002384185791015625), 0.0, 1.0);
    float occlusion =
        fast::clamp(ao * (0.0500000007450580596923828125 +
                          (0.949999988079071044921875 * ssaoContrast)),
                    0.0, 1.0);
    float lightingOcclusion =
        fast::clamp(0.0500000007450580596923828125 +
                        (0.949999988079071044921875 * ssaoContrast),
                    0.0500000007450580596923828125, 1.0);
    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    int areaShadowCount = 0;
    float pointShadow = 0.0;
    int shadowCount = _1355.shadowParamCount;
    for (int i = 0; i < shadowCount; i++) {
        if (_1372.shadowParams[i].lightType == 3) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.pointLightCount ||
                distance(float3(_1465.pointLights[lightIndex].position),
                         FragPos) >= _1465.pointLights[lightIndex].radius) {
                continue;
            }
            ShadowParameters _1386;
            _1386.lightView = _1372.shadowParams[i].lightView;
            _1386.lightProjection = _1372.shadowParams[i].lightProjection;
            _1386.bias0 = _1372.shadowParams[i].bias0;
            _1386.textureIndex = _1372.shadowParams[i].textureIndex;
            _1386.farPlane = _1372.shadowParams[i].farPlane;
            _1386.lightIndex = _1372.shadowParams[i].lightIndex;
            _1386.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1386.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param = _1386;
            float3 param_1 = FragPos;
            pointShadow =
                fast::max(pointShadow,
                          calculatePointShadow(
                              param, param_1, texture1, texture1Smplr, texture2,
                              texture2Smplr, texture3, texture3Smplr, texture4,
                              texture4Smplr, texture5, texture5Smplr, cubeMap1,
                              cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3,
                              cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5,
                              cubeMap5Smplr));
        } else if (_1372.shadowParams[i].lightType == 1) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.spotlightCount ||
                distance(float3(_1510.spotlights[lightIndex].position),
                         FragPos) >= _1510.spotlights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            spotShadow = fast::max(
                spotShadow,
                calculateShadow(param_2, param_3, pa)",
R"(ram_4, texture1,
                                texture1Smplr, texture2, texture2Smplr,
                                texture3, texture3Smplr, texture4,
                                texture4Smplr, texture5, texture5Smplr, _526));
        } else if (_1372.shadowParams[i].lightType == 2) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.areaLightCount ||
                distance(float3(_1552.areaLights[lightIndex].position),
                         FragPos) >= _1552.areaLights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            areaShadow += calculateShadow(
                param_2, param_3, param_4, texture1, texture1Smplr, texture2,
                texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr,
                texture5, texture5Smplr, _526);
            areaShadowCount++;
        } else {
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            directionalShadow = fast::max(
                directionalShadow,
                calculateShadow(param_2, param_3, param_4, texture1,
                                texture1Smplr, texture2, texture2Smplr,
                                texture3, texture3Smplr, texture4,
                                texture4Smplr, texture5, texture5Smplr, _526));
        }
    }
    if (areaShadowCount > 0) {
        areaShadow /= float(areaShadowCount);
    }
    directionalShadow =
        fast::clamp(directionalShadow * 0.85000002384185791015625, 0.0, 1.0);
    spotShadow = fast::clamp(spotShadow * 0.85000002384185791015625, 0.0, 1.0);
    areaShadow = fast::clamp(areaShadow * 0.449999988079071044921875, 0.0, 1.0);
    pointShadow =
        fast::clamp(pointShadow * 0.85000002384185791015625, 0.0, 1.0);
    float3 directionalResult = float3(0.0);
    for (int i_1 = 0; i_1 < _1355.directionalLightCount; i_1++) {
        DirectionalLight _1428;
        _1428.direction = float3(_1422.directionalLights[i_1].direction);
        _1428._pad1 = _1422.directionalLights[i_1]._pad1;
        _1428.diffuse = float3(_1422.directionalLights[i_1].diffuse);
        _1428._pad2 = _1422.directionalLights[i_1]._pad2;
        _1428.specular = float3(_1422.directionalLights[i_1].specular);
        _1428.intensity = _1422.directionalLights[i_1].intensity;
        DirectionalLight param_5 = _1428;
        float3 param_6 = N;
        float3 param_7 = V;
        float3 param_8 = F0;
        float3 param_9 = albedo;
        float param_10 = metallic;
        float param_11 = roughness;
        directionalResult += calcDirectionalLight(
            param_5, param_6, param_7, param_8, param_9, param_10, param_11);
    }
    directionalResult *= (1.0 - directionalShadow);
    float3 pointResult = float3(0.0);
    for (int i_2 = 0; i_2 < _1355.pointLightCount; i_2++) {
        PointLight _1471;
        _1471.position = float3(_1465.pointLights[i_2].position);
        _1471._pad1 = _1465.pointLights[i_2]._pad1;
        _1471.diffuse = float3(_1465.pointLights[i_2].diffuse);
        _1471._pad2 = _1465.pointLights[i_2]._pad2;
        _1471.specular = float3(_1465.pointLights[i_2].specular);
        _1471.intensity = _1465.pointLights[i_2].intensity;
        _1471.constant0 = _1465.pointLights[i_2].constant0;
        _1471.linear = _1465.pointLights[i_2].linear;
        _1471.quadratic = _1465.pointLights[i_2].quadratic;
        _1471.radius = _1465.pointLights[i_2].radius;
        _1471._pad3 = _1465.pointLights[i_2]._pad3;
        PointLight param_12 = _1471;
        float3 param_13 = FragPos;
        float3 param_14 = N;
        float3 param_15 = V;
        float3 param_16 = F0;
        float3 param_17 = albedo;
        float param_18 = metallic;
        float param_19 = roughness;
        pointResult += calcPointLight(param_12, param_13, param_14, param_15,
                                      param_16, param_17, param_18, param_19);
    }
    pointResult *= (1.0 - pointShadow);
    float3 spotResult = float3(0.0);
    for (int i_3 = 0; i_3 < _1355.spotlightCount; i_3++) {
        SpotLight _1516;
        _1516.position = float3(_1510.spotlights[i_3].position);
        _1516._pad1 = _1510.spotlights[i_3]._pad1;
        _1516.direction = float3(_1510.spotlights[i_3].direction);
        _1516.cutOff = _1510.spotlights[i_3].cutOff;
        _1516.outerCutOff = _1510.spotlights[i_3].outerCutOff;
        _1516.intensity = _1510.spotlights[i_3].intensity;
        _1516.range = _1510.spotlights[i_3].range;
        _1516._pad4 = _1510.spotlights[i_3]._pad4;
        _1516.diffuse = float3(_1510.spotlights[i_3].diffuse);
        _1516._pad5 = _1510.spotlights[i_3]._pad5;
        _1516.specular = float3(_1510.spotlights[i_3].specular);
        _1516._pad6 = _1510.spotlights[i_3]._pad6;
        SpotLight param_20 = _1516;
        float3 param_21 = FragPos;
        float3 param_22 = N;
        float3 param_23 = V;
        float3 param_24 = F0;
        float3 param_25 = albedo;
        float param_26 = metallic;
        float param_27 = roughness;
        spotResult += calcSpotLight(param_20, param_21, param_22, param_23,
                                    param_24, param_25, param_26, param_27);
    }
    spotResult *= (1.0 - spotShadow);
    float3 areaResult = float3(0.0);
    float _1639;
    for (int i_4 = 0; i_4 < _1355.areaLightCount; i_4++) {
        float3 P = float3(_1552.areaLights[i_4].position);
        float3 R = fast::normalize(float3(_1552.areaLights[i_4].right));
        float3 U = fast::normalize(float3(_1552.areaLights[i_4].up));
        float2 halfSize = _1552.areaLights[i_4].size * 0.5;
        float3 toPoint = FragPos - P;
        float s = fast::clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = fast::clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        float3 Q = (P + (R * s)) + (U * t);
        float3 Lvec = Q - FragPos;
        float dist = length(Lvec);
        if (dist > 9.9999997473787516355514526367188e-05) {
            float3 L = Lvec / float3(dist);
            float3 Nl = fast::normalize(cross(R, U));
            float ndotl = dot(Nl, -L);
            if (_1552.areaLights[i_4].castsBothSides != 0) {
                _1639 = abs(ndotl);
            } else {
                _1639 = fast::max(ndotl, 0.0);
            }
            float facing = _1639;
            float cosTheta = cos(radians(_1552.areaLights[i_4].angle));
            if ((facing >= cosTheta) && (facing > 0.0)) {
                float range = fast::max(_1552.areaLights[i_4].range,
                                        0.001000000047497451305389404296875);
                float attenuation = 1.0 / ((1.0 + (dist / range)) +
                                           ((dist * dist) / (range * range)));
                float fade = 1.0 - smoothstep(range * 0.89999997615814208984375,
                                           )",
R"(   range, dist);
                float3 radiance =
                    (((float3(_1552.areaLights[i_4].diffuse) *
                       fast::max(_1552.areaLights[i_4].intensity, 0.0)) *
                      attenuation) *
                     facing) *
                    fade;
                float3 param_28 = L;
                float3 param_29 = radiance;
                float3 param_30 = N;
                float3 param_31 = V;
                float3 param_32 = F0;
                float3 param_33 = albedo;
                float param_34 = metallic;
                float param_35 = roughness;
                areaResult +=
                    evaluateBRDF(param_28, param_29, param_30, param_31,
                                 param_32, param_33, param_34, param_35);
            }
        }
    }
    areaResult *= (1.0 - areaShadow);
    float3 param_36 = FragPos;
    float3 param_37 = N;
    float3 param_38 = V;
    float3 param_39 = F0;
    float3 param_40 = albedo;
    float param_41 = metallic;
    float param_42 = roughness;
    float3 _1714 = getRimLight(param_36, param_37, param_38, param_39, param_40,
                               param_41, param_42, _526, environment);
    float3 rimResult = _1714;
    float3 lighting =
        ((((directionalResult + pointResult) + spotResult) + areaResult) +
         rimResult) *
        lightingOcclusion;
    float3 ambientBase =
        ((ambientLight.color.xyz * ambientLight.intensity) * albedo) *
        occlusion;
    bool ddgiEnabled = ps.atlasParams.w > 0.0f;
    float3 ambient = ddgiEnabled ? ambientBase * 0.05f : ambientBase;

    float ddgiSampleBias =
        max(max(ps.spacing.x, max(ps.spacing.y, ps.spacing.z)) * 0.05f, 0.002f);
    float3 ddgiSamplePos = FragPos + ddgiNormal * ddgiSampleBias;
    float3 ddgiIrradiance =
        sampleDDGIIrradiance(irradianceMap, ddgiDistanceMap, ps, ddgiSamplePos,
                             ddgiNormal);
    if (!all(isfinite(ddgiIrradiance))) {
        ddgiIrradiance = float3(0.0f);
    }
    ddgiIrradiance = max(ddgiIrradiance, float3(0.0f));

    float ddgiDebugMode = ps.debugColor.w;
    float3 ddgiGain = max(ps.debugColor.xyz, float3(0.0f));
    if (ddgiGain.x < 1e-4f && ddgiGain.y < 1e-4f && ddgiGain.z < 1e-4f) {
        ddgiGain = float3(1.0f);
    }

    const float INV_PI = 0.31830988618379067153776752674503;
    float3 ddgiDiffuse = ddgiIrradiance * albedo * INV_PI *
                         (1.0f - metallic) * occlusion * ddgiGain;
    ambient += ddgiDiffuse;

    float3 ddgiSpecular = float3(0.0f);

    float3 iblContribution = float3(0.0);
    if (_526.useIBL != 0u) {
        float3 param_43 = N;
        float3 irradiance =
            sampleEnvironmentRadiance(param_43, skybox, skyboxSmplr);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_44 = reflection;
        float3 specularEnv =
            sampleEnvironmentRadiance(param_44, skybox, skyboxSmplr);
        float param_45 = fast::max(dot(N, V), 0.0);
        float3 param_46 = F0;
        float3 F = fresnelSchlick(param_45, param_46);
        float3 kS = F;
        float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
        float roughnessAttenuation = mix(1.0, 0.1500000059604644775390625,
                                         fast::clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution = ((kD * diffuseIBL) + (kS * specularIBL)) * occlusion;
    }
    float3 finalColor = (ambient + lighting + ddgiSpecular) + iblContribution;

    // Debug AO view mode:
    // ps.debugColor.w >= 9.5f -> visualize SSAO directly in grayscale.
    if (ddgiDebugMode >= 9.5f && ddgiDebugMode < 19.5f) {
        finalColor = float3(ssaoFactor);
    } else if (ddgiDebugMode >= 39.5f) {
        finalColor =
            max(ddgiDiffuse * 3.0f + ddgiSpecular * 2.0f, float3(0.0f));
    } else if (ddgiDebugMode >= 29.5f) {
        finalColor = ddgiIrradiance * ddgiGain * 4.0f;
    } else if (ddgiDebugMode >= 19.5f) {
        finalColor = ddgiDiffuse * 3.0f;
    } else if (!(_526.useIBL != 0u)) {
        float3 I = fast::normalize(FragPos - float3(_526.cameraPosition));
        float3 R_1 = reflect(-I, N);
        float param_47 = fast::max(dot(N, -I), 0.0);
        float3 param_48 = F0;
        float3 F_1 = fresnelSchlick(param_47, param_48);
        float3 kS_1 = F_1;
        float3 envColor = skybox.sample(skyboxSmplr, R_1).xyz;
        float3 reflection_1 = envColor * kS_1;
        float3 envMix = F0 * (1.0f - roughness) * 0.20f;
        finalColor = mix(finalColor, reflection_1, envMix);
    }
    out.FragColor = float4(finalColor, 1.0);
    float brightness =
        dot(out.FragColor.xyz,
            float3(0.2125999927520751953125, 0.715200006961822509765625,
                   0.072200000286102294921875));
    if (brightness > 0.75) {
        out.BrightColor = float4(out.FragColor.xyz, 1.0);
    } else {
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 param_49 = out.FragColor.xyz;
    float3 _1897 = acesToneMapping(param_49);
    out.FragColor.x = _1897.x;
    out.FragColor.y = _1897.y;
    out.FragColor.z = _1897.z;
    return out;
}
)",
};
static const AtlasPackedShaderSource LIGHT_FRAG = {LIGHT_FRAG_PARTS, 8};

static const char* const LIGHT_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.TexCoord = in.aTexCoord;
    out.gl_Position = float4(in.aPos, 1.0);
    return out;
}
)",
};
static const AtlasPackedShaderSource LIGHT_VERT = {LIGHT_VERT_PARTS, 1};

static const char* const MAIN_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template<typename T, size_t Num>
struct spvUnsafeArray
{
    T elements[Num ? Num : 1];
    
    thread T& operator [] (size_t pos) thread
    {
        return elements[pos];
    }
    constexpr const thread T& operator [] (size_t pos) const thread
    {
        return elements[pos];
    }
    
    device T& operator [] (size_t pos) device
    {
        return elements[pos];
    }
    constexpr const device T& operator [] (size_t pos) const device
    {
        return elements[pos];
    }
    
    constexpr const constant T& operator [] (size_t pos) const constant
    {
        return elements[pos];
    }
    
    threadgroup T& operator [] (size_t pos) threadgroup
    {
        return elements[pos];
    }
    constexpr const threadgroup T& operator [] (size_t pos) const threadgroup
    {
        return elements[pos];
    }
};

// Implementation of the GLSL radians() function
template<typename T>
inline T radians(T d)
{
    return d * T(0.01745329251);
}

static inline __attribute__((always_inline))
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

static inline __attribute__((always_inline))
float spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) + c1 * spvDet2x2(a2, a3, b2, b3);
}

static inline __attribute__((always_inline))
float4x4 spvInverse4x4(float4x4 m)
{
    float4x4 adj;
    adj[0][0] =  spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][2] =  spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);
    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][1] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
    adj[1][3] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);
    adj[2][0] =  spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][2] =  spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);
    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][1] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
    adj[3][3] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct ShadowParameters
{
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    float _pad1;
    float3 lightPos;
    int lightType;
};

struct Uniforms
{
    int4 textureTypes[16];
    int textureCount;
    float3 cameraPosition;
    float normalMapStrength;
    uint useNormalMap;
    float2 textureScale;
    float2 textureOffset;
};

struct Environment
{
    float rimLightIntensity;
    float3 rimLightColor;
};

struct PushConstants
{
    uint useTexture;
    uint useColor;
    uint useIBL;
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
};

struct DirectionalLight
{
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct DirectionalLightsUBO
{
    spvUnsafeArray<DirectionalLight, 1> directionalLights;
};

struct PointLight
{
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

struct PointLightsUBO
{
    spvUnsafeArray<PointLight, 1> pointLights;
};

struct SpotLight
{
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

struct SpotLightsUBO
{
    spvUnsafeArray<SpotLight, 1> spotlights;
};

struct Material
{
    packed_float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct ShadowParameters_1
{
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    float _pad1;
    packed_float3 lightPos;
    int lightType;
};

struct ShadowParametersUBO
{
    spvUnsafeArray<ShadowParameters_1, 1> shadowParams;
};

struct AreaLight
{
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

struct AreaLightsUBO
{
    spvUnsafeArray<AreaLight, 1> areaLights;
};

struct AmbientLight
{
    float4 color;
    float intensity;
    float3 _pad0;
};

constant spvUnsafeArray<float3, 54> _1657 = spvUnsafeArray<float3, 54>({ float3(0.5381000041961669921875, 0.18559999763965606689453125, -0.4318999946117401123046875), float3(0.13789999485015869140625, 0.248600006103515625, 0.4429999887943267822265625), float3(0.3370999991893768310546875, 0.567900002002716064453125, -0.0057000000961124897003173828125), float3(-0.699899971485137939453125, -0.0450999997556209564208984375, -0.0019000000320374965667724609375), float3(0.068899996578693389892578125, -0.159799993038177490234375, -0.854700028896331787109375), float3(0.056000001728534698486328125, 0.0068999999202787876129150390625, -0.184300005435943603515625), float3(-0.014600000344216823577880859375, 0.14020000398159027099609375, 0.076200000941753387451171875), float3(0.00999999977648258209228515625, -0.19239999353885650634765625, -0.03440000116825103759765625), float3(-0.35769999027252197265625, -0.53009998798370361328125, -0.4357999861240386962890625), float3(-0.3169000148773193359375, 0.10629999637603759765625, 0.015799999237060546875), float3(0.010300000198185443878173828125, -0.5868999958038330078125, 0.0046000001020729541778564453125), float3(-0.08969999849796295166015625, -0.4939999878406524658203125, 0.328700006008148193359375), float3(0.7118999958038330078125, -0.015399999916553497314453125, -0.091799996793270111083984375), float3(-0.053300000727176666259765625, 0.0595999993383884429931640625, -0.541100025177001953125), float3(0.03519999980926513671875, -0.063100002706050872802734375, 0.546000003814697265625), float3(-0.4776000082492828369140625, 0.2847000062465667724609375, -0.0271000005304813385009765625), float3(-0.11200000345706939697265625, 0.1234000027179718017578125, -0.744599997997283935546875), float3(-0.212999999523162841796875, -0.07819999754428863525390625, -0.13789999485015869140625), float3(0.2944000065326690673828125, -0.3111999928951263427734375, -0.2644999921321868896484375), float3(-0.4564000070095062255859375, 0.4174999892711639404296875, -0.184300005435943603515625), float3(0.123400)",
R"(0027179718017578125, -0.567799985408782958984375, 0.788999974727630615234375), float3(-0.6789000034332275390625, 0.23450000584125518798828125, -0.4566999971866607666015625), float3(0.34560000896453857421875, -0.788999974727630615234375, 0.1234000027179718017578125), float3(-0.23450000584125518798828125, 0.567799985408782958984375, -0.6789000034332275390625), float3(0.788999974727630615234375, 0.1234000027179718017578125, 0.567799985408782958984375), float3(-0.567799985408782958984375, -0.6789000034332275390625, 0.23450000584125518798828125), float3(0.4566999971866607666015625, 0.788999974727630615234375, -0.23450000584125518798828125), float3(-0.788999974727630615234375, 0.34560000896453857421875, -0.567799985408782958984375), float3(0.6789000034332275390625, -0.23450000584125518798828125, 0.788999974727630615234375), float3(-0.1234000027179718017578125, 0.6789000034332275390625, -0.4566999971866607666015625), float3(0.23450000584125518798828125, -0.567799985408782958984375, 0.6789000034332275390625), float3(-0.34560000896453857421875, 0.788999974727630615234375, -0.1234000027179718017578125), float3(0.567799985408782958984375, 0.23450000584125518798828125, -0.788999974727630615234375), float3(-0.6789000034332275390625, -0.567799985408782958984375, 0.34560000896453857421875), float3(0.788999974727630615234375, -0.34560000896453857421875, 0.4566999971866607666015625), float3(-0.23450000584125518798828125, 0.1234000027179718017578125, -0.6789000034332275390625), float3(0.4566999971866607666015625, 0.788999974727630615234375, -0.567799985408782958984375), float3(-0.567799985408782958984375, 0.23450000584125518798828125, 0.6789000034332275390625), float3(0.34560000896453857421875, -0.788999974727630615234375, -0.1234000027179718017578125), float3(-0.788999974727630615234375, 0.567799985408782958984375, -0.23450000584125518798828125), float3(0.6789000034332275390625, -0.1234000027179718017578125, 0.34560000896453857421875), float3(-0.4566999971866607666015625, 0.788999974727630615234375, 0.23450000584125518798828125), float3(0.567799985408782958984375, -0.6789000034332275390625, 0.788999974727630615234375), float3(-0.34560000896453857421875, 0.567799985408782958984375, -0.6789000034332275390625), float3(0.23450000584125518798828125, -0.788999974727630615234375, 0.567799985408782958984375), float3(-0.6789000034332275390625, 0.23450000584125518798828125, -0.1234000027179718017578125), float3(0.788999974727630615234375, -0.34560000896453857421875, -0.567799985408782958984375), float3(-0.567799985408782958984375, 0.6789000034332275390625, 0.23450000584125518798828125), float3(0.4566999971866607666015625, -0.788999974727630615234375, 0.34560000896453857421875), float3(-0.23450000584125518798828125, 0.1234000027179718017578125, -0.788999974727630615234375), float3(0.34560000896453857421875, -0.567799985408782958984375, 0.6789000034332275390625), float3(-0.788999974727630615234375, 0.4566999971866607666015625, -0.34560000896453857421875), float3(0.6789000034332275390625, -0.1234000027179718017578125, -0.567799985408782958984375), float3(-0.4566999971866607666015625, 0.23450000584125518798828125, 0.788999974727630615234375) });

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
};

static inline __attribute__((always_inline))
float4 sampleTextureAt(thread const int& textureIndex, thread const float2& uv, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    if (textureIndex == 0)
    {
        return texture1.sample(texture1Smplr, uv);
    }
    else
    {
        if (textureIndex == 1)
        {
            return texture2.sample(texture2Smplr, uv);
        }
        else
        {
            if (textureIndex == 2)
            {
                return texture3.sample(texture3Smplr, uv);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return texture4.sample(texture4Smplr, uv);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return texture5.sample(texture5Smplr, uv);
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return texture6.sample(texture6Smplr, uv);
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return texture7.sample(texture7Smplr, uv);
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return texture8.sample(texture8Smplr, uv);
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return texture9.sample(texture9Smplr, uv);
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return texture10.sample(texture10Smplr, uv);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline))
float2 parallaxMapping(thread const float2& texCoords, thread const float3& viewDir, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 v = fast::normalize(viewDir);
    float numLayers = mix(32.0, 8.0, abs(dot(float3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float2 P = (v.xy / float2(fast::max(v.z, 0.0500000007450580596923828125))) * 0.039999999105930328369140625;
    float2 deltaTexCoords = P / float2(numLayers);
    float2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 6)
        {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == (-1))
    {
        return texCoords;
    }
    int param = textureIndex;
    float2 param_1 = currentTexCoords;
    float currentDepthMapValue = 1.0 - sampleTextureAt(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCo)",
R"(ords -= deltaTexCoords;
        int param_2 = textureIndex;
        float2 param_3 = currentTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(param_2, param_3, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
        currentLayerDepth += layerDepth;
    }
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    int param_4 = textureIndex;
    float2 param_5 = prevTexCoords;
    float beforeDepth = 1.0 - sampleTextureAt(param_4, param_5, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = (prevTexCoords * weight) + (currentTexCoords * (1.0 - weight));
    return currentTexCoords;
}

static inline __attribute__((always_inline))
float4 enableTextures(thread const int& type, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, thread float2& texCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float4 color = float4(0.0);
    int count = 0;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == type)
        {
            if (i == 0)
            {
                color += texture1.sample(texture1Smplr, texCoord);
            }
            else
            {
                if (i == 1)
                {
                    color += texture2.sample(texture2Smplr, texCoord);
                }
                else
                {
                    if (i == 2)
                    {
                        color += texture3.sample(texture3Smplr, texCoord);
                    }
                    else
                    {
                        if (i == 3)
                        {
                            color += texture4.sample(texture4Smplr, texCoord);
                        }
                        else
                        {
                            if (i == 4)
                            {
                                color += texture5.sample(texture5Smplr, texCoord);
                            }
                            else
                            {
                                if (i == 5)
                                {
                                    color += texture6.sample(texture6Smplr, texCoord);
                                }
                                else
                                {
                                    if (i == 6)
                                    {
                                        color += texture7.sample(texture7Smplr, texCoord);
                                    }
                                    else
                                    {
                                        if (i == 7)
                                        {
                                            color += texture8.sample(texture8Smplr, texCoord);
                                        }
                                        else
                                        {
                                            if (i == 8)
                                            {
                                                color += texture9.sample(texture9Smplr, texCoord);
                                            }
                                            else
                                            {
                                                if (i == 9)
                                                {
                                                    color += texture10.sample(texture10Smplr, texCoord);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            count++;
        }
    }
    if (count > 0)
    {
        color /= float4(float(count));
    }
    if (count == 0)
    {
        return float4(-1.0);
    }
    return color;
}

static inline __attribute__((always_inline))
float2 getTextureDimensions(thread const int& textureIndex, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    if (textureIndex == 0)
    {
        return float2(int2(texture1.get_width(), texture1.get_height()));
    }
    else
    {
        if (textureIndex == 1)
        {
            return float2(int2(texture2.get_width(), texture2.get_height()));
        }
        else
        {
            if (textureIndex == 2)
            {
                return float2(int2(texture3.get_width(), texture3.get_height()));
            }
            else
            {
                if (textureIndex == 3)
                {
                    return float2(int2(texture4.get_width(), texture4.get_height()));
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return float2(int2(texture5.get_width(), texture5.get_height()));
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return float2(int2(texture6.get_width(), texture6.get_height()));
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return float2(int2(texture7.get_width(), texture7.get_height()));
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return float2(int2(texture8.get_width(), texture8.get_height()));
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return float2(int2(texture9.get_width(), texture9.get_height()));
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return float2(int2(texture10.get_width(), texture10.get_height()));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return float2(0.0);
}

static inline __attribute__((always_inline))
float calculateShadow(thread const ShadowParameters& shadowParam, thread const float4& fragP)",
R"(osLightSpace, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, device DirectionalLightsUBO& _1083, thread float3& Normal, thread float3& FragPos)
{
    float3 projCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    projCoords = (projCoords * 0.5) + float3(0.5);
    bool _1362 = projCoords.x < 0.0;
    bool _1369;
    if (!_1362)
    {
        _1369 = projCoords.x > 1.0;
    }
    else
    {
        _1369 = _1362;
    }
    bool _1376;
    if (!_1369)
    {
        _1376 = projCoords.y < 0.0;
    }
    else
    {
        _1376 = _1369;
    }
    bool _1383;
    if (!_1376)
    {
        _1383 = projCoords.y > 1.0;
    }
    else
    {
        _1383 = _1376;
    }
    bool _1390;
    if (!_1383)
    {
        _1390 = projCoords.z < 0.0;
    }
    else
    {
        _1390 = _1383;
    }
    bool _1398;
    if (!_1390)
    {
        _1398 = projCoords.z > 1.0;
    }
    else
    {
        _1398 = _1390;
    }
    if (_1398)
    {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    float3 lightDir = fast::normalize(-(spvInverse4x4(shadowParam.lightView) * float4(0.0, 0.0, -1.0, 0.0)).xyz);
    float3 normal = fast::normalize(Normal);
    float biasValue = shadowParam.bias0;
    float bias0 = fast::max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);
    float shadow = 0.0;
    int param = shadowParam.textureIndex;
    float2 texelSize = float2(1.0) / getTextureDimensions(param, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    float _distance = length(_163.cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, fast::clamp(_distance / 100.0, 0.0, 1.0)));
    int sampleCount = 0;
    int _1444 = -kernelSize;
    for (int x = _1444; x <= kernelSize; x++)
    {
        int _1455 = -kernelSize;
        for (int y = _1455; y <= kernelSize; y++)
        {
            int param_1 = shadowParam.textureIndex;
            float2 param_2 = projCoords.xy + (float2(float(x), float(y)) * texelSize);
            float pcfDepth = sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
            shadow += float((currentDepth - bias0) > pcfDepth);
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);
    return shadow;
}

static inline __attribute__((always_inline))
float4 sampleCubeTextureAt(thread const int& textureIndex, thread const float3& direction, texturecube<float> cubeMap1, sampler cubeMap1Smplr, texturecube<float> cubeMap2, sampler cubeMap2Smplr, texturecube<float> cubeMap3, sampler cubeMap3Smplr, texturecube<float> cubeMap4, sampler cubeMap4Smplr, texturecube<float> cubeMap5, sampler cubeMap5Smplr)
{
    if (textureIndex == 0)
    {
        return cubeMap1.sample(cubeMap1Smplr, direction);
    }
    else
    {
        if (textureIndex == 1)
        {
            return cubeMap2.sample(cubeMap2Smplr, direction);
        }
        else
        {
            if (textureIndex == 2)
            {
                return cubeMap3.sample(cubeMap3Smplr, direction);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return cubeMap4.sample(cubeMap4Smplr, direction);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return cubeMap5.sample(cubeMap5Smplr, direction);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline))
float calculatePointShadow(thread const ShadowParameters& shadowParam, thread const float3& fragPos, texturecube<float> cubeMap1, sampler cubeMap1Smplr, texturecube<float> cubeMap2, sampler cubeMap2Smplr, texturecube<float> cubeMap3, sampler cubeMap3Smplr, texturecube<float> cubeMap4, sampler cubeMap4Smplr, texturecube<float> cubeMap5, sampler cubeMap5Smplr)
{
    float3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);
    float bias0 = 0.0500000007450580596923828125;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.0500000007450580596923828125;
    for (int i = 0; i < 54; i++)
    {
        float3 sampleDir = fast::normalize(fragToLight + (_1657[i] * diskRadius));
        int param = shadowParam.textureIndex;
        float3 param_1 = sampleDir;
        float closestDepth = sampleCubeTextureAt(param, param_1, cubeMap1, cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3, cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5, cubeMap5Smplr).x * shadowParam.farPlane;
        if ((currentDepth - bias0) > closestDepth)
        {
            shadow += 1.0;
        }
    }
    shadow /= 54.0;
    return shadow;
}

static inline __attribute__((always_inline))
float distributionGGX(thread const float3& N, thread const float3& H, thread const float& roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = fast::max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)) + 1.0;
    denom = (3.1415927410125732421875 * denom) * denom;
    return num / denom;
}

static inline __attribute__((always_inline))
float geometrySchlickGGX(thread const float& NdotV, thread const float& roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = (NdotV * (1.0 - k)) + k;
    return num / denom;
}

static inline __attribute__((always_inline))
float geometrySmith(thread const float3& N, thread const float3& V, thread const float3& L, thread const float& roughness)
{
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float param = NdotV;
    float param_1 = roughness;
    float ggx2 = geometrySchlickGGX(param, param_1);
    float param_2 = NdotL;
    float param_3 = roughness;
    float ggx1 = geometrySchlickGGX(param_2, param_3);
    return ggx1 * ggx2;
}

static inline __attribute__((always_inline))
float3 fresnelSchlick(thread const float& cosTheta, thread const float3& F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline))
float3 calculatePBR(thread const float3& N, thread const float3& V, thread const float3& L, thread const float3& F0, thread const float3& radiance, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float& reflectivity)
{
    float3 H = fast::normalize(V + L);
    float3 param = N;
    float3 param_1 = H;
    float param_2 = roughness;
    float NDF = distributionGGX(param, param_1, param_2);
    float3 param_3 = N;
    float3 param_4 = V;
    float3 param_5 = L;
    float param_6 = roughness;
    float G = geometrySmith(param_3, param_4, param_5, param_6);
    float param_7 = fast::max(dot(H, V), 0.0);
    float3 param_8 = F0;
    float3 F = fresnelSchlick(param_7, param_8);
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= (1.0 - metallic);
    float3 numerator = F * (NDF * G);
    float denominator = ((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0)) + 9.9999997473787516355514526367188e-05;
    float3 specular = numerator / float3(denominator);
    float NdotL = fast::max(dot(N, L), 0.0);
    float3 Lo = ((((kD * albedo) /)",
R"( float3(3.1415927410125732421875)) + specular) * radiance) * NdotL;
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllDirectionalLights(thread const float3& N, thread const float3& V, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device DirectionalLightsUBO& _1083)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.directionalLightCount; i++)
    {
        float3 L = fast::normalize(-float3(_1083.directionalLights[i].direction));
        float3 radiance = float3(_1083.directionalLights[i].diffuse) * fast::max(_1083.directionalLights[i].intensity, 0.0);
        float3 param = N;
        float3 param_1 = V;
        float3 param_2 = L;
        float3 param_3 = F0;
        float3 param_4 = radiance;
        float3 param_5 = albedo;
        float param_6 = metallic;
        float param_7 = roughness;
        float param_8 = reflectivity;
        Lo += calculatePBR(param, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllPointLights(thread const float3& fragPos, thread const float3& N, thread const float3& V, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device PointLightsUBO& _1136)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.pointLightCount; i++)
    {
        float3 L = float3(_1136.pointLights[i].position) - fragPos;
        float _distance = length(L);
        _distance = fast::max(_distance, 0.001000000047497451305389404296875);
        L = fast::normalize(L);
        float range = fast::max(_1136.pointLights[i].radius, 0.001000000047497451305389404296875);
        float3 radiance = float3(_1136.pointLights[i].diffuse) * fast::max(_1136.pointLights[i].intensity, 0.0);
        float attenuation = 1.0 / ((1.0 + (_distance / range)) + ((_distance * _distance) / (range * range)));
        float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
        float3 radianceAttenuated = radiance * attenuation;
        radianceAttenuated *= fade;
        float3 H = fast::normalize(V + L);
        float3 param = N;
        float3 param_1 = H;
        float param_2 = roughness;
        float NDF = distributionGGX(param, param_1, param_2);
        float3 param_3 = N;
        float3 param_4 = V;
        float3 param_5 = L;
        float param_6 = roughness;
        float G = geometrySmith(param_3, param_4, param_5, param_6);
        float param_7 = fast::max(dot(H, V), 0.0);
        float3 param_8 = F0;
        float3 F = fresnelSchlick(param_7, param_8);
        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD *= (1.0 - metallic);
        float3 numerator = F * (NDF * G);
        float denominator = ((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0)) + 9.9999997473787516355514526367188e-05;
        float3 specular = numerator / float3(denominator);
        float NdotL = fast::max(dot(N, L), 0.0);
        Lo += (((((kD * albedo) / float3(3.1415927410125732421875)) + specular) * radianceAttenuated) * NdotL);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllSpotLights(thread const float3& N, thread const float3& fragPos, thread const float3& L, thread const float3& viewDir, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device SpotLightsUBO& _1268)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.spotlightCount; i++)
    {
        float3 L_1 = fast::normalize(float3(_1268.spotlights[i].position) - fragPos);
        float3 spotDirection = fast::normalize(float3(_1268.spotlights[i].direction));
        float theta = dot(L_1, -spotDirection);
        float intensity = smoothstep(_1268.spotlights[i].outerCutOff, _1268.spotlights[i].cutOff, theta);
        float _distance = length(float3(_1268.spotlights[i].position) - fragPos);
        _distance = fast::max(_distance, 0.001000000047497451305389404296875);
        float range = fast::max(_1268.spotlights[i].range, 0.001000000047497451305389404296875);
        float attenuation = 1.0 / ((1.0 + (_distance / range)) + ((_distance * _distance) / (range * range)));
        float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
        float3 radiance = (((float3(_1268.spotlights[i].diffuse) * fast::max(_1268.spotlights[i].intensity, 0.0)) * attenuation) * intensity) * fade;
        float3 param = N;
        float3 param_1 = viewDir;
        float3 param_2 = L_1;
        float3 param_3 = F0;
        float3 param_4 = radiance;
        float3 param_5 = albedo;
        float param_6 = metallic;
        float param_7 = roughness;
        float param_8 = reflectivity;
        Lo += calculatePBR(param, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 getRimLight(thread const float3& fragPos, thread float3& N, thread float3& V, thread const float3& F0, thread const float3& albedo, thread const float& metallic, thread const float& roughness, constant Uniforms& _163, constant Environment& environment)
{
    N = fast::normalize(N);
    V = fast::normalize(V);
    float rim = powr(1.0 - fast::max(dot(N, V), 0.0), 3.0);
    rim *= mix(1.2000000476837158203125, 0.300000011920928955078125, roughness);
    float3 rimColor = mix(float3(1.0), albedo, float3(metallic)) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, float3(0.5));
    float rimIntensity = environment.rimLightIntensity;
    float3 rimLight = (rimColor * rim) * rimIntensity;
    float dist = length(_163.cameraPosition - fragPos);
    rimLight /= float3(1.0 + (dist * 0.100000001490116119384765625));
    return rimLight;
}

static inline __attribute__((always_inline))
float2 directionToEquirect(thread const float3& direction)
{
    float3 dir = fast::normalize(direction);
    float phi = precise::atan2(dir.z, dir.x);
    float theta = acos(fast::clamp(dir.y, -1.0, 1.0));
    return float2((phi + 3.1415927410125732421875) / 6.283185482025146484375, theta / 3.1415927410125732421875);
}

static inline __attribute__((always_inline))
float3 sampleHDRTexture(thread const int& textureIndex, thread const float3& direction, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 param = direction;
    float2 uv = directionToEquirect(param);
    int param_1 = textureIndex;
    float2 param_2 = uv;
    return sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).xyz;
}

static inline __attribute__((always_inline))
float3 sampleEnvironmentRadiance(thread const float3& direction, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 envColor)",
R"( = float3(0.0);
    int count = 0;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 13)
        {
            int param = i;
            float3 param_1 = direction;
            envColor += sampleHDRTexture(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
            count++;
        }
    }
    if (count == 0)
    {
        return float3(0.0);
    }
    return envColor / float3(float(count));
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread float3& color)
{
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    color = (color * ((color * a) + float3(b))) / ((color * ((color * c) + float3(d))) + float3(e));
    color = powr(fast::clamp(color, float3(0.0), float3(1.0)), float3(0.4545454680919647216796875));
    return color;
}

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _163 [[buffer(0)]], constant Environment& environment [[buffer(1)]], constant PushConstants& _1073 [[buffer(2)]], device DirectionalLightsUBO& _1083 [[buffer(3)]], device PointLightsUBO& _1136 [[buffer(4)]], device SpotLightsUBO& _1268 [[buffer(5)]], constant Material& material [[buffer(6)]], device ShadowParametersUBO& _1905 [[buffer(7)]], device AreaLightsUBO& _2058 [[buffer(8)]], constant AmbientLight& ambientLight [[buffer(9)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], texturecube<float> cubeMap1 [[texture(10)]], texturecube<float> cubeMap2 [[texture(11)]], texturecube<float> cubeMap3 [[texture(12)]], texturecube<float> cubeMap4 [[texture(13)]], texturecube<float> cubeMap5 [[texture(14)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], sampler cubeMap1Smplr [[sampler(10)]], sampler cubeMap2Smplr [[sampler(11)]], sampler cubeMap3Smplr [[sampler(12)]], sampler cubeMap4Smplr [[sampler(13)]], sampler cubeMap5Smplr [[sampler(14)]])
{
    main0_out out = {};
    float3x3 TBN = {};
    TBN[0] = in.TBN_0;
    TBN[1] = in.TBN_1;
    TBN[2] = in.TBN_2;
    float2 texCoord = in.TexCoord * _163.textureScale + _163.textureOffset;
    bool hasParallaxMap = false;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 6)
        {
            hasParallaxMap = true;
            break;
        }
    }
    if (hasParallaxMap)
    {
        float3 tangentViewDir = fast::normalize(transpose(TBN) * (_163.cameraPosition - in.FragPos));
        float2 param = texCoord;
        float3 param_1 = tangentViewDir;
        texCoord = parallaxMapping(param, param_1, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    }
    int param_2 = 5;
    float4 normTexture = enableTextures(param_2, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool _1774 = normTexture.x != (-1.0);
    bool _1780;
    if (_1774)
    {
        _1780 = normTexture.y != (-1.0);
    }
    else
    {
        _1780 = _1774;
    }
    bool _1786;
    if (_1780)
    {
        _1786 = normTexture.z != (-1.0);
    }
    else
    {
        _1786 = _1780;
    }
    float normalStrength = fast::max(_163.normalMapStrength, 0.0f);
    bool useNormalMap = (_163.useNormalMap != 0u) && (normalStrength > 0.0f);
    float3 N;
    if (_1786 && useNormalMap)
    {
        float3 tangentNormal = (normTexture.xyz * 2.0) - float3(1.0);
        tangentNormal.xy *= normalStrength;
        tangentNormal = fast::normalize(tangentNormal);
        N = fast::normalize(TBN * tangentNormal);
    }
    else
    {
        N = fast::normalize(in.Normal);
    }
    N = fast::normalize(N);
    float3 V = fast::normalize(_163.cameraPosition - in.FragPos);
    float3 albedo = float3(material.albedo);
    int param_3 = 0;
    float4 albedoTex = enableTextures(param_3, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(albedoTex != float4(-1.0)))
    {
        albedo *= albedoTex.xyz;
    }
    int param_3a = 12;
    float4 opacityTex = enableTextures(param_3a, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(opacityTex != float4(-1.0)))
    {
        if (opacityTex.x < 0.100000001490116119384765625)
        {
            discard_fragment();
        }
    }
    else
    {
        if ((albedoTex.w < 0.100000001490116119384765625) && (material.albedo[3] < 0.999000012874603271484375))
        {
            discard_fragment();
        }
    }
    int param_pbr_pack = 14;
    float4 pbrPackTex = enableTextures(param_pbr_pack, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool hasPbrPack = any(pbrPackTex != float4(-1.0));
    float metallic = material.metallic;
    int param_4 = 9;
    float4 metallicTex = enableTextures(param_4, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        metallic *= pbrPackTex.z;
    }
    else if (any(metallicTex != float4(-1.0)))
    {
        metallic *= metallicTex.x;
    }
    float roughness = material.roughness;
    int param_5 = 10;
    float4 roughnessTex = enableTextures(param_5, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        roughness *= pbrPackTex.y;
    }
    else if (any(roughnessTex != float4(-1.0)))
    {
        roughness *= roughnessTex.x;
    }
    float ao = material.ao;
    int param_6 = 11;
    float4 aoTex = enableTextures(param_6, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        ao *= pbrPackTex.x;
    }
    else if (any(aoTex != float4(-1.0)))
    {
        ao *= a)",
R"(oTex.x;
    }
    float3 F0 = float3(0.039999999105930328369140625);
    F0 = mix(F0, albedo, float3(metallic));
    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    float pointShadow = 0.0;
    if (_1073.shadowParamCount > 0)
    {
        for (int i_1 = 0; i_1 < _1073.shadowParamCount; i_1++)
        {
            if (_1905.shadowParams[i_1].lightType == 0)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                areaShadow = fast::max(areaShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else if (_1905.shadowParams[i_1].lightType == 1)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                spotShadow = fast::max(spotShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else if (_1905.shadowParams[i_1].lightType == 2)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                directionalShadow = fast::max(directionalShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else
            {
                ShadowParameters _1945;
                _1945.lightView = _1905.shadowParams[i_1].lightView;
                _1945.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1945.bias0 = _1905.shadowParams[i_1].bias0;
                _1945.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1945.farPlane = _1905.shadowParams[i_1].farPlane;
                _1945._pad1 = _1905.shadowParams[i_1]._pad1;
                _1945.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1945.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_9 = _1945;
                float3 param_10 = in.FragPos;
                pointShadow = fast::max(pointShadow, calculatePointShadow(param_9, param_10, cubeMap1, cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3, cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5, cubeMap5Smplr));
            }
        }
    }
    float reflectivity = material.reflectivity;
    float3 viewDir = fast::normalize(_163.cameraPosition - in.FragPos);
    float3 lighting = float3(0.0);
    float3 param_11 = N;
    float3 param_12 = V;
    float3 param_13 = albedo;
    float param_14 = metallic;
    float param_15 = roughness;
    float3 param_16 = F0;
    float param_17 = reflectivity;
    lighting += (calcAllDirectionalLights(param_11, param_12, param_13, param_14, param_15, param_16, param_17, _1073, _1083) * (1.0 - directionalShadow));
    float3 param_18 = in.FragPos;
    float3 param_19 = N;
    float3 param_20 = V;
    float3 param_21 = albedo;
    float param_22 = metallic;
    float param_23 = roughness;
    float3 param_24 = F0;
    float param_25 = reflectivity;
    lighting += (calcAllPointLights(param_18, param_19, param_20, param_21, param_22, param_23, param_24, param_25, _1073, _1136) * (1.0 - pointShadow));
    float3 param_26 = N;
    float3 param_27 = in.FragPos;
    float3 param_28 = V;
    float3 param_29 = viewDir;
    float3 param_30 = albedo;
    float param_31 = metallic;
    float param_32 = roughness;
    float3 param_33 = F0;
    float param_34 = reflectivity;
    lighting += calcAllSpotLights(param_26, param_27, param_28, param_29, param_30, param_31, param_32, param_33, param_34, _1073, _1268) * (1.0 - spotShadow);
    float3 param_35 = in.FragPos;
    float3 param_36 = N;
    float3 param_37 = V;
    float3 param_38 = F0;
    float3 param_39 = albedo;
    float param_40 = metallic;
    float param_41 = roughness;
    float3 _2039 = getRimLight(param_35, param_36, param_37, param_38, param_39, param_40, param_41, _163, environment);
    lighting += _2039;
    float3 areaResult = float3(0.0);
    float _2144;
    for (int i_2 = 0; i_2 < _1073.areaLightCount; i_2++)
    {
        float3 P = float3(_2058.areaLights[i_2].position);
        float3 R = fast::normalize(float3(_2058.areaLights[i_2].right));
        float3 U = fast::normalize(float3(_2058.areaLights[i_2].up));
        float2 halfSize = _2058.areaLights[i_2].size * 0.5;
        float3 toPoint = in.FragPos - P;
        float s = fast::clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = fast::clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        float3 Q = (P + (R * s)) + (U * t);
        float3 Lvec = Q - in.FragPos;
        float dist = length(Lvec);
        if (dist > 9.9999997473787516355514526367188e-05)
        {
            float3 L = Lvec / float3(dist);
            float3 Nl = fast::normalize(cross(R, U));
            float ndotl = dot(Nl, -L);
            if (_2058.areaLights[i_2].castsBothSides != 0)
            {
                _2144 = abs(ndotl);
            }
            else
            {
                _2144 = fast::max(ndotl, 0.0);
            }
            float facing = _2144;
            float cosTheta = cos(radians(_2058.areaLights[i_2].angle));
            if ((facing >= cosTh)",
R"(eta) && (facing > 0.0))
            {
                float range = fast::max(_2058.areaLights[i_2].range, 0.001000000047497451305389404296875);
                float attenuation = 1.0 / ((1.0 + (dist / range)) + ((dist * dist) / (range * range)));
                float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, dist);
                float3 radiance = (((float3(_2058.areaLights[i_2].diffuse) * fast::max(_2058.areaLights[i_2].intensity, 0.0)) * attenuation) * facing) * fade;
                float3 H = fast::normalize(V + L);
                float3 param_42 = N;
                float3 param_43 = H;
                float param_44 = roughness;
                float NDF = distributionGGX(param_42, param_43, param_44);
                float3 param_45 = N;
                float3 param_46 = V;
                float3 param_47 = L;
                float param_48 = roughness;
                float G = geometrySmith(param_45, param_46, param_47, param_48);
                float param_49 = fast::max(dot(H, V), 0.0);
                float3 param_50 = F0;
                float3 F = fresnelSchlick(param_49, param_50);
                float3 kS = F;
                float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
                float3 numerator = F * (NDF * G);
                float denominator = fast::max((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0), 9.9999997473787516355514526367188e-05);
                float3 specular = numerator / float3(denominator);
                float NdotL = fast::max(dot(N, L), 0.0);
                areaResult += (((((kD * albedo) / float3(3.1415927410125732421875)) + specular) * radiance) * NdotL);
            }
        }
    }
    lighting += areaResult * (1.0 - areaShadow);
    float aoClamped = fast::clamp(ao, 0.0, 1.0);
    float aoWithFloor = fast::max(aoClamped, 0.20000000298023223876953125);
    float ambientIntensity = ambientLight.intensity;
    float3 ambientColor = ambientLight.color.xyz;
    if (ambientIntensity <= 9.9999997473787516355514526367188e-05)
    {
        ambientIntensity = 0.02999999932944774627685546875;
    }
    if (dot(ambientColor, ambientColor) <= 9.9999999747524270787835121154785e-07)
    {
        ambientColor = float3(1.0);
    }
    float3 ambient = ((albedo * ambientIntensity) * ambientColor) * aoWithFloor;
    float3 iblContribution = float3(0.0);
    if (_1073.useIBL != 0u)
    {
        float3 param_51 = N;
        float3 irradiance = sampleEnvironmentRadiance(param_51, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_52 = reflection;
        float3 specularEnv = sampleEnvironmentRadiance(param_52, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
        float param_53 = fast::max(dot(N, V), 0.0);
        float3 param_54 = F0;
        float3 F_1 = fresnelSchlick(param_53, param_54);
        float3 kS_1 = F_1;
        float3 kD_1 = float3(1.0) - kS_1;
        kD_1 *= (1.0 - metallic);
        float roughnessAttenuation = mix(1.0, 0.1500000059604644775390625, fast::clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution = ((kD_1 * diffuseIBL) + (kS_1 * specularIBL)) * aoWithFloor;
    }
    float3 color = (ambient + lighting) + iblContribution;
    out.FragColor = float4(color, 1.0);
    float brightness = dot(color, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    if (brightness > 0.75)
    {
        out.BrightColor = float4(color, 1.0);
    }
    else
    {
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 param_55 = out.FragColor.xyz;
    float3 _2399 = acesToneMapping(param_55);
    out.FragColor.x = _2399.x;
    out.FragColor.y = _2399.y;
    out.FragColor.z = _2399.z;
    return out;
}
)",
};
static const AtlasPackedShaderSource MAIN_FRAG = {MAIN_FRAG_PARTS, 8};

static const char* const MAIN_VERT_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Returns the determinant of a 2x2 matrix.
static inline __attribute__((always_inline))
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Returns the inverse of a matrix, by using the algorithm of calculating the classical
// adjoint and dividing by the determinant. The contents of the matrix are changed.
static inline __attribute__((always_inline))
float3x3 spvInverse3x3(float3x3 m)
{
    float3x3 adj;	// The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the matrix.
    adj[0][0] =  spvDet2x2(m[1][1], m[1][2], m[2][1], m[2][2]);
    adj[0][1] = -spvDet2x2(m[0][1], m[0][2], m[2][1], m[2][2]);
    adj[0][2] =  spvDet2x2(m[0][1], m[0][2], m[1][1], m[1][2]);

    adj[1][0] = -spvDet2x2(m[1][0], m[1][2], m[2][0], m[2][2]);
    adj[1][1] =  spvDet2x2(m[0][0], m[0][2], m[2][0], m[2][2]);
    adj[1][2] = -spvDet2x2(m[0][0], m[0][2], m[1][0], m[1][2]);

    adj[2][0] =  spvDet2x2(m[1][0], m[1][1], m[2][0], m[2][1]);
    adj[2][1] = -spvDet2x2(m[0][0], m[0][1], m[2][0], m[2][1]);
    adj[2][2] =  spvDet2x2(m[0][0], m[0][1], m[1][0], m[1][1]);

    // Calculate the determinant as a combination of the cofactors of the first row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 outColor [[user(locn1)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
    float4 gl_Position [[position, invariant]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float2 aTexCoord [[attribute(2)]];
    float3 aNormal [[attribute(3)]];
    float3 aTangent [[attribute(4)]];
    float3 aBitangent [[attribute(5)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& uniforms [[buffer(0)]])
{
    main0_out out = {};
    float3x3 TBN = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    float4x4 modelMatrix = uniforms.model;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        modelMatrix = instanceModel;
    }
    float4x4 mvp = (uniforms.projection * uniforms.view) * modelMatrix;
    float4 _56 = float4(in.aPos, 1.0);
    float4 _57 = mvp * _56;
    out.gl_Position = _57;
    out.FragPos = float3((modelMatrix * float4(in.aPos, 1.0)).xyz);
    out.TexCoord = in.aTexCoord;
    out.outColor = in.aColor;
    float3x3 normalMatrix = transpose(spvInverse3x3(float3x3(modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz)));
    out.Normal = fast::normalize(normalMatrix * in.aNormal);
    float3 N = out.Normal;
    float3 T = fast::normalize(normalMatrix * in.aTangent);
    float3 B = fast::normalize(normalMatrix * in.aBitangent);
    TBN = float3x3(float3(T), float3(B), float3(N));
    out.TBN_0 = TBN[0];
    out.TBN_1 = TBN[1];
    out.TBN_2 = TBN[2];
    return out;
}
)",
};
static const AtlasPackedShaderSource MAIN_VERT = {MAIN_VERT_PARTS, 1};

static const char* const PARTICLE_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    uint useTexture;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 fragTexCoord [[user(locn0)]];
    float4 fragColor [[user(locn1)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _9 [[buffer(0)]], texture2d<float> particleTexture [[texture(0)]], sampler particleTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    if (_9.useTexture != 0u)
    {
        float4 texColor = particleTexture.sample(particleTextureSmplr, in.fragTexCoord);
        if (texColor.w < 0.00999999977648258209228515625)
        {
            discard_fragment();
        }
        out.FragColor = texColor * in.fragColor;
    }
    else
    {
        float2 center = float2(0.5);
        float dist = distance(in.fragTexCoord, center);
        float alpha = 1.0 - smoothstep(0.300000011920928955078125, 0.5, dist);
        out.FragColor = float4(in.fragColor.xyz, in.fragColor.w * alpha);
        if (out.FragColor.w < 0.00999999977648258209228515625)
        {
            discard_fragment();
        }
    }
    return out;
}

)",
};
static const AtlasPackedShaderSource PARTICLE_FRAG = {PARTICLE_FRAG_PARTS, 1};

static const char* const PARTICLE_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UniformBufferObject
{
    float4x4 view;
    float4x4 projection;
    float4x4 model;
    uint isAmbient;
};

struct main0_out
{
    float2 fragTexCoord [[user(locn0)]];
    float4 fragColor [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 quadVertex [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float3 particlePos [[attribute(2)]];
    float4 particleColor [[attribute(3)]];
    float particleSize [[attribute(4)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UniformBufferObject& _12 [[buffer(0)]])
{
    main0_out out = {};
    if (_12.isAmbient != 0u)
    {
        float4 viewParticlePos = _12.view * float4(in.particlePos, 1.0);
        float3 viewPosition = viewParticlePos.xyz + float3(in.quadVertex.x * in.particleSize, in.quadVertex.y * in.particleSize, 0.0);
        out.gl_Position = (_12.projection * _12.model) * float4(viewPosition, 1.0);
    }
    else
    {
        float3 cameraRight = float3(_12.view[0].x, _12.view[1].x, _12.view[2].x);
        float3 cameraUp = float3(_12.view[0].y, _12.view[1].y, _12.view[2].y);
        float3 worldPosition = (in.particlePos + ((cameraRight * in.quadVertex.x) * in.particleSize)) + ((cameraUp * in.quadVertex.y) * in.particleSize);
        out.gl_Position = (_12.projection * _12.view) * float4(worldPosition, 1.0);
    }
    out.fragTexCoord = in.texCoord;
    out.fragColor = in.particleColor;
    return out;
}

)",
};
static const AtlasPackedShaderSource PARTICLE_VERT = {PARTICLE_VERT_PARTS, 1};

static const char* const PATH_PARTS[] = {
R"(#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

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
};

static_assert(sizeof(SceneData) == 144);
static_assert(__builtin_offsetof(SceneData, atmosphereSunDirection) == 48);
static_assert(__builtin_offsetof(SceneData, atmosphereSunIntensity) == 64);
static_assert(__builtin_offsetof(SceneData, atmosphereSunColor) == 80);
static_assert(__builtin_offsetof(SceneData, pixelStride) == 96);
static_assert(__builtin_offsetof(SceneData, ambientColor) == 112);

float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float luminance(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

float powerHeuristic(float pdfA, float pdfB) {
    float a2 = pdfA * pdfA;
    float b2 = pdfB * pdfB;
    return a2 / max(a2 + b2, 1e-8);
}

float3 clampLuminance(float3 c, float maxL) {
    float l = luminance(c);
    if (l > maxL && l > 1e-6) {
        c *= maxL / l;
    }
    return c;
}

uint wang_hash(uint s) {
    s = (s ^ 61u) ^ (s >> 16u);
    s *= 9u;
    s = s ^ (s >> 4u);
    s *= 0x27d4eb2du;
    s = s ^ (s >> 15u);
    return s;
}

float rand(thread uint &state) {
    state = wang_hash(state);
    return float(state) / 4294967296.0;
}

uint seedBase(uint2 gid, uint w, uint frame, uint sampleIndex) {
    return gid.x + gid.y * w + frame * 9781u + sampleIndex * 6271u + 1u;
}

constexpr sampler skyboxSampler(coord::normalized, address::clamp_to_edge,
                                filter::linear, mip_filter::linear);

float3 skyColor(float3 dir, float intensity, texturecube<float> skybox,
                constant SceneData &sceneData) {
    float3 sampleDir = dir;
    float len2 = dot(sampleDir, sampleDir);
    if (len2 > 1e-10) {
        sampleDir *= rsqrt(len2);
    } else {
        sampleDir = float3(0.0, 1.0, 0.0);
    }
    float3 sky = skybox.sample(skyboxSampler, sampleDir).xyz;
    if (sceneData.atmosphereEnabled != 0) {
        float horizon = pow(clamp(1.0 - abs(sampleDir.y), 0.0, 1.0), 4.0);
        float daylight = smoothstep(-0.2, 0.15,
                                    sceneData.atmosphereSunDirection.y);
        float3 zenith = float3(0.08, 0.28, 0.65);
        float3 horizonColor = float3(0.58, 0.72, 0.92);
        float3 proceduralSky = mix(zenith, horizonColor, horizon) *
                               max(daylight, 0.08);
        sky = max(sky, proceduralSky);
    }
    if (sceneData.atmosphereEnabled != 0 &&
        sceneData.atmosphereSunDirection.y > -0.15) {
        float3 sunDirection = sceneData.atmosphereSunDirection;
        float sunDirectionLength = dot(sunDirection, sunDirection);
        sunDirection = sunDirectionLength > 1e-10
                           ? sunDirection * rsqrt(sunDirectionLength)
                           : float3(0.0, 1.0, 0.0);
        float sunDot = dot(sampleDir, sunDirection);
        float sizeAdjust =
            1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.001;
        float sunSize = 0.9995 * sizeAdjust;
        float sunGlowSize =
            0.998 * (1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.003);
        float sunHaloSize =
            0.99 * (1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.015);
        float sunDisk = smoothstep(sunSize - 0.0002, sunSize, sunDot);
        float sunGlow =
            smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
        float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) *
                        (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
        float horizonFade = smoothstep(
            -0.15, 0.05, sceneData.atmosphereSunDirection.y);
        float sunIntensity = max(sceneData.atmosphereSunIntensity, 0.05);
        sky += sceneData.atmosphereSunColor *
               (sunDisk * 5.0 + sunGlow * 0.5 + sunHalo) * horizonFade *
               sunIntensity;
    }
    float scale = intensity > 0.0 ? intensity : 1.0;
    return sky * scale;
}

float3 cosineSampleHemisphere(float2 u) {
    float r = sqrt(u.x);
    float theta = 2.0 * M_PI_F * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u.x));

    return float3(x, y, z);
}

float3x3 buildOrthonormalBasis(float3 N) {
    float3 T = normalize(abs(N.x) > 0.1 ? cross(float3(0, 1, 0), N)
                                        : cross(float3(1, 0, 0), N));
    float3 B = cross(N, T);
    return float3x3(T, B, N);
}

float3 normalizeOr(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10) {
        return v * rsqrt(len2);
    }
    return fallback;
}

float rayOffsetDistance(float3 position) {
    float positionScale =
        max(abs(position.x), max(abs(position.y), abs(position.z)));
    return max(0.0002, positionScale * 0.000002);
}

float3 offsetRayOrigin(float3 position, float3 geometricNormal,
                       float3 direction) {
    float side = dot(direction, geometricNormal) >= 0.0 ? 1.0 : -1.0;
    return position + geometricNormal * (rayOffsetDistance(position) * side);
}

float2 encodeNormal(float3 normal) {
    normal /= max(abs(normal.x) + abs(normal.y) + abs(normal.z), 1e-6);
    float2 encoded = normal.xy;
    if (normal.z < 0.0) {
        float2 signValue = select(float2(-1.0), float2(1.0), encoded >= 0.0);
        encoded = (1.0 - abs(encoded.yx)) * signValue;
    }
    return encoded;
}

constexpr sampler materialTexSampler(coord::normalized, address::repeat,
                                     filter::linear, mip_filter::linear);

#define PT_MATERIAL_TEXTURE_PARAMS                                             \
    texture2d<float> materialTexture0, texture2d<float> materialTexture1,      \
        texture2d<float> materialTexture2, texture2d<float> materialTexture3,  \
        texture2d<float> materialTexture4, texture2d<float> materialTexture5,  \
        texture2d<float> materialTexture6, text)",
R"(ure2d<float> materialTexture7,  \
        texture2d<float> materialTexture8, texture2d<float> materialTexture9,  \
        texture2d<float> materialTexture10,                                    \
        texture2d<float> materialTexture11,                                    \
        texture2d<float> materialTexture12,                                    \
        texture2d<float> materialTexture13,                                    \
        texture2d<float> materialTexture14,                                    \
        texture2d<float> materialTexture15,                                    \
        texture2d<float> materialTexture16,                                    \
        texture2d<float> materialTexture17,                                    \
        texture2d<float> materialTexture18,                                    \
        texture2d<float> materialTexture19,                                    \
        texture2d<float> materialTexture20,                                    \
        texture2d<float> materialTexture21,                                    \
        texture2d<float> materialTexture22,                                    \
        texture2d<float> materialTexture23,                                    \
        texture2d<float> materialTexture24,                                    \
        texture2d<float> materialTexture25,                                    \
        texture2d<float> materialTexture26,                                    \
        texture2d<float> materialTexture27,                                    \
        texture2d<float> materialTexture28,                                    \
        texture2d<float> materialTexture29,                                    \
        texture2d<float> materialTexture30,                                    \
        texture2d<float> materialTexture31,                                    \
        texture2d<float> materialTexture32,                                    \
        texture2d<float> materialTexture33,                                    \
        texture2d<float> materialTexture34,                                    \
        texture2d<float> materialTexture35,                                    \
        texture2d<float> materialTexture36,                                    \
        texture2d<float> materialTexture37,                                    \
        texture2d<float> materialTexture38,                                    \
        texture2d<float> materialTexture39,                                    \
        texture2d<float> materialTexture40,                                    \
        texture2d<float> materialTexture41,                                    \
        texture2d<float> materialTexture42,                                    \
        texture2d<float> materialTexture43,                                    \
        texture2d<float> materialTexture44,                                    \
        texture2d<float> materialTexture45,                                    \
        texture2d<float> materialTexture46, texture2d<float> materialTexture47

#define PT_MATERIAL_TEXTURE_ARGS                                               \
    materialTexture0, materialTexture1, materialTexture2, materialTexture3,    \
        materialTexture4, materialTexture5, materialTexture6,                  \
        materialTexture7, materialTexture8, materialTexture9,                  \
        materialTexture10, materialTexture11, materialTexture12,               \
        materialTexture13, materialTexture14, materialTexture15,               \
        materialTexture16, materialTexture17, materialTexture18,               \
        materialTexture19, materialTexture20, materialTexture21,               \
        materialTexture22, materialTexture23, materialTexture24,               \
        materialTexture25, materialTexture26, materialTexture27,               \
        materialTexture28, materialTexture29, materialTexture30,               \
        materialTexture31, materialTexture32, materialTexture33,               \
        materialTexture34, materialTexture35, materialTexture36,               \
        materialTexture37, materialTexture38, materialTexture39,               \
        materialTexture40, materialTexture41, materialTexture42,               \
        materialTexture43, materialTexture44, materialTexture45,               \
        materialTexture46, materialTexture47

#define PT_MATERIAL_TEXTURE_BINDINGS                                           \
    texture2d<float> materialTexture0 [[texture(12)]],                         \
        texture2d<float> materialTexture1 [[texture(13)]],                     \
        texture2d<float> materialTexture2 [[texture(14)]],                     \
        texture2d<float> materialTexture3 [[texture(15)]],                     \
        texture2d<float> materialTexture4 [[texture(16)]],                     \
        texture2d<float> materialTexture5 [[texture(17)]],                     \
        texture2d<float> materialTexture6 [[texture(18)]],                     \
        texture2d<float> materialTexture7 [[texture(19)]],                     \
        texture2d<float> materialTexture8 [[texture(20)]],                     \
        texture2d<float> materialTexture9 [[texture(21)]],                     \
        texture2d<float> materialTexture10 [[texture(22)]],                    \
        texture2d<float> materialTexture11 [[texture(23)]],                    \
        texture2d<float> materialTexture12 [[texture(24)]],                    \
        texture2d<float> materialTexture13 [[texture(25)]],                    \
        texture2d<float> materialTexture14 [[texture(26)]],                    \
        texture2d<float> materialTexture15 [[texture(27)]],                    \
        texture2d<float> materialTexture16 [[texture(28)]],                    \
        texture2d<float> materialTexture17 [[texture(29)]],                    \
        texture2d<float> materialTexture18 [[texture(30)]],                    \
        texture2d<float> materialTexture19 [[texture(31)]],                    \
        texture2d<float> materialTexture20 [[texture(32)]],                    \
        texture2d<float> materialTexture21 [[texture(33)]],                    \
        texture2d<float> materialTexture22 [[texture(34)]],                    \
        texture2d<float> materialTexture23 [[texture(35)]],                    \
        texture2d<float> materialTexture24 [[texture(36)]],                    \
        texture2d<float> materialTexture25 [[texture(37)]],                    \
        texture2d<float> materialTexture26 [[texture(38)]],                    \
        texture2d<float> materialTexture27 [[texture(39)]],                    \
        texture2d<float> materialTexture28 [[texture(40)]],                    \
        texture2d<float> materialTexture29 [[texture(41)]],                    \
        texture2d<float> materialTexture30 [[texture(42)]],                    \
        texture2d<float> materialTexture31 [[texture(43)]],                    \
        texture2d<float> materialTexture32 [[texture(44)]],                    \
        texture2d<float> materialTexture33 [[texture(45)]],                    \
        texture2d<float> materialTexture34 [[texture(46)]],                    \
        texture2d<float> materialTexture35 [[texture(47)]],                    \
        texture2d<float> materialTexture36 [[texture(48)]],                    \
        texture2d<float> materialTexture37 [[texture(49)]],                    \
        texture2d<float> materialTexture38 [[texture(50)]],                    \
        texture2d<float> materialTexture39 [[texture(51)]],                    \
        texture2d<float> materialTexture40 [[texture(52)]],                    \
        texture2d<float> materialTexture41 [[texture(53)]],                    \
        texture2d<float> materialTexture42 [[texture(54)]],                    \
        texture2d<float> materialTexture43 [[texture(55)]],                    \
        texture2d<float> materialTexture44 [[texture(56)]],                    \
        textu)",
R"(re2d<float> materialTexture45 [[texture(57)]],                    \
        texture2d<float> materialTexture46 [[texture(58)]],                    \
        texture2d<float> materialTexture47 [[texture(59)]]

float4 sampleMaterialTexture(int textureIndex, float2 uv,
                             PT_MATERIAL_TEXTURE_PARAMS) {
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
    return float4(0.0);
}

struct MaterialTextureArguments {
    array<texture2d<float>, 256> textures [[id(0)]];
};

float4 sampleMaterialTexture(
    int textureIndex, float2 uv,
    constant MaterialTextureArguments &materialTextureArguments) {
    return materialTextureArguments.textures[textureIndex].sample(
        materialTexSampler, uv);
}

#undef PT_MATERIAL_TEXTURE_PARAMS
#undef PT_MATERIAL_TEXTURE_ARGS
#undef PT_MATERIAL_TEXTURE_BINDINGS
#define PT_MATERIAL_TEXTURE_PARAMS                                            \
    constant MaterialTextureArguments &materialTextureArguments
#define PT_MATERIAL_TEXTURE_ARGS materialTextureArguments
#define PT_MATERIAL_TEXTURE_BINDINGS                                          \
    constant MaterialTextureArguments &materialTextureArguments [[buffer(12)]]

float resolveMaterialOpacity(Material mat, float2 uv, uint textureCount,
                             PT_MATERIAL_TEXTURE_PARAMS) {
    float opacity = clamp(mat.albedo.w, 0.0, 1.0);
    if (mat.opacityTextureIndex >= 0 &&
        uint(mat.opacityTextureIndex) < textureCount) {
        float4 opacitySample = sampleMaterialTexture(
            mat.opacityTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        opacity *= mat.opacityTextureIndex == mat.albedoTextureIndex
                       ? opacitySample.w
                       : opacitySample.x;
    }
    return clamp(opacity, 0.0, 1.0);
}

void resolveMaterialParameters(Material mat, float2 uv, uint textureCount,
                               PT_MATERIAL_TEXTURE_PARAMS,
                               thread float3 &albedo, thread float &metallic,
                               thread float &roughness, thread float &ao,
                               thread float3 &emissive, thread float &outIor,
                               thread float &outTransmittance) {
    albedo = clamp(mat.albedo.xyz, float3(0.0), float3(1.0));
    metallic = mat.metallic;
    roughness = mat.roughness;
    ao = mat.ao;
    emissive = clampLuminance(float3(mat.emissiveColor) *
                                  min(max(mat.emissiveIntensity, 0.0), 8.0),
                              8.0);

    outIor = max(mat.ior, 1.0);
    outTransmittance = clamp(mat.transmittance, 0.0, 1.0);

    if (mat.albedoTextureIndex >= 0 &&
        uint(mat.albedoTextureIndex) < textureCount) {
        albedo *= clamp(sampleMaterialTexture(mat.albedoTextureIndex, uv,
                                              PT_MATERIAL_TEXTURE_ARGS)
                            .xyz,
                        float3(0.0), float3(1.0));
    }
    if (mat.metallicTextureIndex >= 0 &&
        uint(mat.metallicTextureIndex) < textureCount) {
        float4 metallicSample = sampleMaterialTexture(
            mat.metallicTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        float metallicValue = metallicSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            metallicValue = metallicSample.z;
        }
        metallic *= clamp(metallicValue, 0.0, 1.0);
    }
    if (mat.roughnessTextureIndex >= 0 &&
        uint(mat.roughnessTextureIndex) < textureCount) {
        float4 roughnessSample = sampleMaterialTexture(
            mat.roughnessTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        float roughnessValue = roughnessSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            roughnessValue = roughnessSample.y;
        }
        roughness *= clamp(roughnessValue, 0.0, 1.0);
    }
    if (mat.aoTextureIndex >= 0 && uint(mat.aoTextureIndex) < textureCount) {
        ao *= clamp(sampleMaterialTexture(mat.aoTextureIndex, uv,
                                          PT_MATERIAL_TEXTURE_ARGS)
                        .x,
                    0.0, 1.0);
    }

    albedo = clamp(albedo, float3(0.0), float3(1.0));
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.001, 1.0);
    ao = clamp(ao, 0.0, 1.0);
}

float3 resolveShadingNormal(Material mat, float2 uv, float3 localN,
                            float3 localT, float3 localB, Instance)",
R"(Data inst,
                            uint textureCount, PT_MATERIAL_TEXTURE_PARAMS) {
    float3x3 normalMatrix =
        float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
    float3 N = normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0));

    float3 T = normalizeOr((inst.model * float4(localT, 0.0)).xyz,
                           float3(1.0, 0.0, 0.0));
    float3 B = normalizeOr((inst.model * float4(localB, 0.0)).xyz,
                           float3(0.0, 0.0, 1.0));
    T = normalizeOr(T - N * dot(N, T), float3(1.0, 0.0, 0.0));
    B = normalizeOr(B - N * dot(N, B), cross(N, T));
    if (dot(cross(T, B), cross(T, B)) <= 1e-10) {
        float3x3 basis = buildOrthonormalBasis(N);
        T = basis[0];
        B = basis[1];
    }

    bool useNormalMap = mat._pad1[0] != 0;
    float normalStrength = max(mat._pad0, 0.0f);
    if (useNormalMap && normalStrength > 0.0 && mat.normalTextureIndex >= 0 &&
        uint(mat.normalTextureIndex) < textureCount) {
        float3 tangentNormal = sampleMaterialTexture(mat.normalTextureIndex, uv,
                                                     PT_MATERIAL_TEXTURE_ARGS)
                                   .xyz;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        tangentNormal = normalizeOr(tangentNormal, float3(0.0, 0.0, 1.0));
        N = normalizeOr(float3x3(T, B, N) * tangentNormal, N);
    }

    return N;
}

bool isOccluded(intersector<triangle_data> isect,
                primitive_acceleration_structure sceneAS, float3 P, float3 Ng,
                float3 L, float maxDistance, thread uint &rng,
                constant Material *materials,
                constant uint *primitiveObjects,
                constant uint *blasPrimitiveOffsets,
                constant VertexData *vertices, constant uint *indices,
                constant SceneData &sceneData, PT_MATERIAL_TEXTURE_PARAMS) {
    float shadowBias = rayOffsetDistance(P);
    ray shadowRay;
    shadowRay.origin = offsetRayOrigin(P, Ng, L);
    shadowRay.direction = L;
    shadowRay.min_distance = 0.0;
    shadowRay.max_distance = max(maxDistance - shadowBias, shadowBias + 1e-4);

    for (uint alphaStep = 0; alphaStep < 16; ++alphaStep) {
        auto shadowHit = isect.intersect(shadowRay, sceneAS);
        if (shadowHit.type == intersection_type::none) {
            return false;
        }

        uint primitiveIndex =
            blasPrimitiveOffsets[shadowHit.geometry_id] + shadowHit.primitive_id;
        uint objectIndex = primitiveObjects[primitiveIndex];
        Material material = materials[objectIndex];
        uint i0 = indices[primitiveIndex * 3 + 0];
        uint i1 = indices[primitiveIndex * 3 + 1];
        uint i2 = indices[primitiveIndex * 3 + 2];
        float2 bary = shadowHit.triangle_barycentric_coord;
        float b0 = 1.0 - bary.x - bary.y;
        float2 uv = float2(vertices[i0].uv) * b0 +
                    float2(vertices[i1].uv) * bary.x +
                    float2(vertices[i2].uv) * bary.y;
        uv = uv * float2(material.textureScale) +
             float2(material.textureOffset);
        float opacity = resolveMaterialOpacity(
            material, uv, sceneData.materialTextureCount,
            PT_MATERIAL_TEXTURE_ARGS);
        if (opacity >= 0.999 || rand(rng) < opacity) {
            return true;
        }

        float advance = shadowHit.distance + rayOffsetDistance(shadowRay.origin);
        shadowRay.origin += shadowRay.direction * advance;
        shadowRay.max_distance -= advance;
        if (shadowRay.max_distance <= shadowBias) {
            return false;
        }
    }

    return true;
}

float3 sampleDirectionalLightDirection(DirectionalLightData light,
                                       thread uint &rng) {
    float3 baseL = normalize(-light.direction);
    float3x3 basis = buildOrthonormalBasis(baseL);
    float sunRadius = 0.0025;
    float2 u = float2(rand(rng), rand(rng));
    float r = sunRadius * sqrt(u.x);
    float phi = 2.0 * M_PI_F * u.y;
    float3 jittered =
        baseL + basis[0] * (r * cos(phi)) + basis[1] * (r * sin(phi));
    return normalize(jittered);
}

// ---------------------------------------------------------------------------
// PBR helpers: GGX / Cook-Torrance
// ---------------------------------------------------------------------------

float D_GGX(float NdotH, float roughness) {
    float a = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / max(M_PI_F * d * d, 1e-6);
}

float3 F_Schlick(float cosTheta, float3 F0) {
    float c = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow5(c);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

float G1_SmithGGX(float NdotX, float roughness) {
    float alpha = max(roughness * roughness, 1e-4);
    float alphaSquared = alpha * alpha;
    float cosineSquared = NdotX * NdotX;
    return (2.0 * NdotX) /
           max(NdotX +
                   sqrt(alphaSquared + (1.0 - alphaSquared) * cosineSquared),
               1e-6);
}

float disneyDiffuseFactor(float NdotV, float NdotL, float LdotH,
                          float roughness) {
    float fd90 = 0.5 + 2.0 * LdotH * LdotH * roughness;
    float lightScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotL);
    float viewScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotV);
    return lightScatter * viewScatter;
}

// GGX importance-sampled microfacet half-vector (in local TBN space, Z=up)
float3 sampleGGX(float2 u, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * M_PI_F * u.x;
    float cosTheta = sqrt((1.0 - u.y) / max(1.0 + (a * a - 1.0) * u.y, 1e-7));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 sampleGGXVNDF(float3 localView, float roughness, float2 u) {
    float alpha = max(roughness * roughness, 1e-3);
    float3 stretchedView =
        normalizeOr(float3(alpha * localView.x, alpha * localView.y,
                           max(localView.z, 1e-5)),
                    float3(0.0, 0.0, 1.0));
    float lensq = stretchedView.x * stretchedView.x +
                  stretchedView.y * stretchedView.y;
    float3 tangentX = lensq > 1e-7
                          ? float3(-stretchedView.y, stretchedView.x, 0.0) *
                                rsqrt(lensq)
                          : float3(1.0, 0.0, 0.0);
    float3 tangentY = cross(stretchedView, tangentX);
    float radius = sqrt(u.x);
    float phi = 2.0 * M_PI_F * u.y;
    float diskX = radius * cos(phi);
    float diskY = radius * sin(phi);
    float blend = 0.5 * (1.0 + stretchedView.z);
    diskY = mix(sqrt(max(0.0, 1.0 - diskX * diskX)), diskY, blend);
    float diskZ = sqrt(max(0.0, 1.0 - diskX * diskX - diskY * diskY));
    float3 visibleNormal = diskX * tangentX + diskY * tangentY +
                           diskZ * stretchedView;
    return normalizeOr(float3(alpha * visibleNormal.x,
                              alpha * visibleNormal.y,
                              max(visibleNormal.z, 0.0)),
                       float3(0.0, 0.0, 1.0));
}

// Full Cook-Torrance PBR for a single analytic light
float3 evalPBR(float3 albedo, float metallic, float roughness,
               float reflectivity, float3 N, float3 V, float3 L,
               float3 lightColor, float intensity) {
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float clampedRoughness = clamp(roughness, 0.045, 1.0);
    float3 baseF0 = mix(float3(0.04), albedo, clamp(metallic, 0.0, 1.0));
    float3 reflectedColor = mix(float3(1.0), albedo, metallic);
    float3 F0 = mix(baseF0, reflectedColor, clamp(reflectivity, 0.0, 1.0));
    float3 F = F_Schlick(VdotH, F0);
    float D = D_GGX(NdotH, clampedRoughness);
    float G = )",
R"(G_Smith(NdotV, NdotL, clampedRoughness);

    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
    float3 kD = (1.0 - F) * (1.0 - clamp(metallic, 0.0, 1.0)) *
                (1.0 - clamp(reflectivity, 0.0, 1.0));
    float diffuseFactor = disneyDiffuseFactor(NdotV, NdotL, max(dot(L, H), 0.0),
                                              clampedRoughness);
    float3 diffuse = (kD * albedo * diffuseFactor) / M_PI_F;

    return (diffuse + specular) * lightColor * intensity * NdotL;
}

float3 evalSubsurface(float3 albedo, float3 N, float3 V, float3 L,
                      float3 lightColor, float intensity, float roughness,
                      float sssStrength, float sssThickness) {
    float NdotL = dot(N, L);
    float NdotV = max(dot(N, V), 0.0);
    float backLit = clamp(-NdotL, 0.0, 1.0);
    float wrapAmount = mix(0.2, 0.7, clamp(roughness, 0.0, 1.0));
    float wrapped = clamp((NdotL + wrapAmount) / (1.0 + wrapAmount), 0.0, 1.0);
    float viewScatter = pow(clamp(1.0 - max(dot(V, L), 0.0), 0.0, 1.0), 2.0);
    float transmission = backLit * (0.35 + 0.65 * viewScatter);
    float diffuseBleed = wrapped * (0.5 + 0.5 * (1.0 - NdotV));
    float profile = mix(diffuseBleed, transmission, 0.65);
    float thicknessFalloff = exp(-max(sssThickness, 0.01) * (1.0 - backLit));
    return (albedo * lightColor * intensity * sssStrength * profile *
            thicknessFalloff) /
           M_PI_F;
}

float3 evalTransmission(float3 albedo, float3 N, float3 V, float3 L,
                        float3 lightColor, float intensity, float roughness,
                        float ior) {
    float backLighting = max(dot(N, -L), 0.0);
    float forwardAlignment = max(dot(-V, L), 0.0);
    float3 F0 = float3(pow((ior - 1.0) / (ior + 1.0), 2.0));
    float3 F = F_Schlick(max(dot(N, V), 0.0), F0);
    float3 transmitTint = mix(float3(1.0), albedo, 0.1);
    float lobeExponent = mix(96.0, 2.0, sqrt(clamp(roughness, 0.0, 1.0)));
    float transmissionLobe = pow(forwardAlignment, lobeExponent);
    return (1.0 - F) * transmitTint * lightColor * intensity * backLighting *
           transmissionLobe;
}

// ---------------------------------------------------------------------------
// Direct lighting with full PBR (replaces old evalDirectLighting)
// ---------------------------------------------------------------------------

float3 evalDirectLightingPBR(intersector<triangle_data> isect,
                             primitive_acceleration_structure sceneAS, float3 P,
                             float3 N, float3 Ng, float3 V, float3 albedo,
                             float metallic, float roughness, float reflectivity,
                             float ior, float transmittance, float sssStrength,
                             float sssThickness,
                             thread uint &rng,
                             constant DirectionalLightData &dirLight,
                             constant SceneData &sceneData,
                             constant PointLight *pointLights,
                             constant SpotLight *spotLights,
                             constant AreaLight *areaLights,
                             constant Material *materials,
                             constant uint *primitiveObjects,
                             constant uint *blasPrimitiveOffsets,
                             constant VertexData *vertices,
                             constant uint *indices,
                             PT_MATERIAL_TEXTURE_PARAMS) {
    float3 lighting = float3(0.0);
    float surfaceOpacity =
        clamp(1.0 - transmittance * (1.0 - metallic), 0.0, 1.0);

    // Directional
    if (sceneData.numDirectionalLights > 0) {
        float3 L = sampleDirectionalLightDirection(dirLight, rng);
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, N, V, L,
                           dirLight.color, max(dirLight.intensity, 0.0));
        float3 s = evalSubsurface(albedo, N, V, L, dirLight.color,
                                  max(dirLight.intensity, 0.0), roughness,
                                  sssStrength, sssThickness);
        float3 t =
            evalTransmission(albedo, N, V, L, dirLight.color,
                             max(dirLight.intensity, 0.0), roughness, ior) *
            transmittance;
        if (!isOccluded(isect, sceneAS, P, Ng, L, 1e30, rng, materials,
                        primitiveObjects, blasPrimitiveOffsets, vertices,
                        indices, sceneData, PT_MATERIAL_TEXTURE_ARGS)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    // Point lights
    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 sampledPosition = pointLights[i].position;
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float lightRange = max(pointLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(pointLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, N, V, L,
                           pointLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, pointLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, pointLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, Ng, L, dist, rng, materials,
                        primitiveObjects, blasPrimitiveOffsets, vertices,
                        indices, sceneData, PT_MATERIAL_TEXTURE_ARGS)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    // Spot lights
    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 sampledPosition = spotLights[i].position;
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 fwd = normalize(spotLights[i].direction);
        float spotCos = dot(-L, fwd);
        float spot =
            smoothstep(spotLights[i].outerCos, spotLights[i].innerCos, spotCos);
        float lightRange = max(spotLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(spotLights[i].intensity, 0.0) * atten * spot;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, N, V, L,
                           spotLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, spotLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, spotLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, Ng, L, dist, rng, materials,
                        primitiveObjects, blasPrimitiveOffsets, vertices,
                        indices, sceneData, PT_MATERIAL_TEXTURE_ARGS)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    // Area lights
    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float2 lightSample = float2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float3 sampledPosition =
            areaLights[i].position +
            areaLights[i].right * (lightSample.x * areaLights[i].halfWidth) +
            areaLights[i].up * (lightSample.y * areaLights[i].halfHeight);
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 lightNorm =
            normalize(cross(areaLights[i].right, a)",
R"(reaLights[i].up));
        float cosLight = areaLights[i].twoSided > 0.5
                             ? abs(dot(lightNorm, -L))
                             : max(dot(lightNorm, -L), 0.0);
        float area = 4.0 * areaLights[i].halfWidth * areaLights[i].halfHeight;
        float lightPdfArea = 1.0 / max(area, 1e-6);
        float distSq = max(dist * dist, 1e-6);
        float atten = cosLight / max(distSq * lightPdfArea, 1e-6);
        float intensity = max(areaLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, N, V, L,
                           areaLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, areaLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, areaLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, Ng, L, dist, rng, materials,
                        primitiveObjects, blasPrimitiveOffsets, vertices,
                        indices, sceneData, PT_MATERIAL_TEXTURE_ARGS)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    return lighting;
}

// ---------------------------------------------------------------------------
// sampleRadiance — iterative path with GGX importance-sampled indirect bounces
// ---------------------------------------------------------------------------

float3 sampleRadiance(uint2 gid, uint sampleIndex, uint w,
                      intersector<triangle_data> isect,
                      primitive_acceleration_structure sceneAS, ray primaryRay,
                      constant Material *materials,
                      constant uint *primitiveObjects,
                      constant uint *blasPrimitiveOffsets,
                      constant VertexData *vertices, constant uint *indices,
                      constant InstanceData *instanceData,
                      constant DirectionalLightData &dirLight,
                      constant SceneData &sceneData,
                      constant PointLight *pointLights,
                      constant SpotLight *spotLights,
                      constant AreaLight *areaLights,
                      PT_MATERIAL_TEXTURE_PARAMS, texturecube<float> skybox,
                      thread float3 &primaryAlbedo,
                      thread float3 &primaryNormal,
                      thread float3 &primaryPosition,
                      thread float &primaryDepth,
                      thread float &primaryRoughness,
                      thread float &primaryHitDistance,
                      thread uint &primaryObjectId) {
    uint rng = seedBase(gid, w, sceneData.frameIndex, sampleIndex);
    uint bounceLimit = min(sceneData.maxBounces, 16u);
    ray surfaceRay = primaryRay;
    float3 radiance = float3(0.0);
    float3 throughput = float3(1.0);
    float previousBsdfPdf = 0.0;
    float previousEnvironmentPdf = 0.0;
    bool previousEventWasDelta = true;

    for (uint depth = 0; depth <= bounceLimit; ++depth) {
        auto hit = isect.intersect(surfaceRay, sceneAS);
        Material mat{};
        InstanceData inst{};
        uint surfaceObjectIndex = 0xFFFFFFFFu;
        float2 texUV = float2(0.0);
        float3 localN = float3(0.0, 1.0, 0.0);
        float3 localT = float3(1.0, 0.0, 0.0);
        float3 localB = float3(0.0, 0.0, 1.0);
        float3 geometricNormal = float3(0.0, 1.0, 0.0);
        bool foundSurface = false;

        for (uint alphaStep = 0; alphaStep < 16; ++alphaStep) {
            if (hit.type == intersection_type::none) {
                break;
            }

            uint primitiveIndex =
                blasPrimitiveOffsets[hit.geometry_id] + hit.primitive_id;
            surfaceObjectIndex = primitiveObjects[primitiveIndex];
            mat = materials[surfaceObjectIndex];
            inst = instanceData[surfaceObjectIndex];

            uint i0 = indices[primitiveIndex * 3 + 0];
            uint i1 = indices[primitiveIndex * 3 + 1];
            uint i2 = indices[primitiveIndex * 3 + 2];
            float2 bary = hit.triangle_barycentric_coord;
            float b0 = 1.0 - bary.x - bary.y;
            float b1 = bary.x;
            float b2 = bary.y;

            texUV = float2(vertices[i0].uv) * b0 +
                    float2(vertices[i1].uv) * b1 +
                    float2(vertices[i2].uv) * b2;
            texUV = texUV * float2(mat.textureScale) +
                    float2(mat.textureOffset);
            localN = normalizeOr(float3(vertices[i0].normal) * b0 +
                                     float3(vertices[i1].normal) * b1 +
                                     float3(vertices[i2].normal) * b2,
                                 float3(0.0, 1.0, 0.0));
            localT = normalizeOr(float3(vertices[i0].tangent) * b0 +
                                     float3(vertices[i1].tangent) * b1 +
                                     float3(vertices[i2].tangent) * b2,
                                 float3(1.0, 0.0, 0.0));
            localB = normalizeOr(float3(vertices[i0].bitangent) * b0 +
                                     float3(vertices[i1].bitangent) * b1 +
                                     float3(vertices[i2].bitangent) * b2,
                                 float3(0.0, 0.0, 1.0));
            float3 p0 = float3(vertices[i0].position);
            float3 p1 = float3(vertices[i1].position);
            float3 p2 = float3(vertices[i2].position);
            float3x3 normalMatrix = float3x3(
                inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
            geometricNormal = normalizeOr(
                cross(p1 - p0, p2 - p0),
                normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0)));

            float alpha = resolveMaterialOpacity(
                mat, texUV, sceneData.materialTextureCount,
                PT_MATERIAL_TEXTURE_ARGS);
            if (alpha >= 0.999 || rand(rng) < alpha) {
                foundSurface = true;
                break;
            }

            float3 rejectedPosition =
                surfaceRay.origin + surfaceRay.direction * hit.distance;
            surfaceRay.origin =
                rejectedPosition + surfaceRay.direction *
                                       rayOffsetDistance(rejectedPosition);
            surfaceRay.min_distance = 0.0;
            hit = isect.intersect(surfaceRay, sceneAS);
        }

        if (!foundSurface) {
            float misWeight = previousEventWasDelta
                                  ? 1.0
                                  : powerHeuristic(previousBsdfPdf,
                                                   previousEnvironmentPdf);
            radiance += throughput * misWeight *
                        skyColor(surfaceRay.direction, 0.0, skybox, sceneData);
            break;
        }

        float3 shadingNormal = resolveShadingNormal(
            mat, texUV, localN, localT, localB, inst,
            sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        float3 P = surfaceRay.origin + surfaceRay.direction * hit.distance;
        float3 V = normalize(-surfaceRay.direction);
        bool frontFace = dot(geometricNormal, V) >= 0.0;
        float3 Ng = frontFace ? geometricNormal : -geometricNormal;
        float3 N = dot(shadingNormal, Ng) >= 0.0 ? shadingNormal : -shadingNormal;
        float shadingNormalCosine = dot(N, Ng);
        if (shadingNormalCosine < 0.1) {
            N = normalizeOr(N + Ng * (0.1 - shadingNormalCosine), Ng);
        }

        float3 albedo;
        float metallic;
        float roughness;
        float ao;
        float3 emissive;
        float ior;
        float transmittance;
        resolveMaterialParameters(mat, texUV, sceneData.materialTextureCount,
                                  PT_MATERIAL_TEXTURE_ARGS, albedo, metallic,
                                  roughness, ao, emissive, ior, transmittance);

        if (depth == 0) {
            primaryAlbedo = albedo;
            primaryNormal = N;
            primaryPosition = P;
            prima)",
R"(ryDepth = length(P - primaryRay.origin);
            primaryRoughness = roughness;
            primaryHitDistance = hit.distance;
            primaryObjectId = surfaceObjectIndex;
        }

        float reflectivity = clamp(mat.reflectivity, 0.0, 1.0);
        float sssStrength = 0.0;
        float sssThickness = mix(0.25, 1.75, ao);
        float3 direct = evalDirectLightingPBR(
            isect, sceneAS, P, N, Ng, V, albedo, metallic, roughness,
            reflectivity, ior, transmittance, sssStrength, sssThickness, rng,
            dirLight, sceneData, pointLights, spotLights, areaLights, materials,
            primitiveObjects, blasPrimitiveOffsets, vertices, indices,
            PT_MATERIAL_TEXTURE_ARGS);
        radiance += throughput * (direct + emissive);

        if (depth == 0 && sceneData.ambientIntensity > 0.0) {
            float aoVisibility = mix(0.2, 1.0, ao);
            float3 ambientF0 = mix(float3(0.04), albedo, metallic);
            float3 ambientF = F_Schlick(max(dot(N, V), 0.0), ambientF0);
            float3 ambientDiffuse = (1.0 - ambientF) * (1.0 - metallic) *
                                    albedo * (1.0 - transmittance);
            float3 ambientSpecular =
                ambientF * mix(1.0, 0.35, roughness);
            float3 ambient = (ambientDiffuse + ambientSpecular) *
                             sceneData.ambientColor *
                             sceneData.ambientIntensity * aoVisibility;
            radiance += throughput * ambient;
        }

        float3 baseF0 = mix(float3(0.04), albedo, metallic);
        float3 reflectedColor = mix(float3(1.0), albedo, metallic);
        float3 F0 = mix(baseF0, reflectedColor, reflectivity);
        float NdotV = max(dot(N, V), 1e-4);
        float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
        float dielectricFresnel =
            F_Schlick(NdotV, float3(dielectricF0)).x;
        float specProb = metallic * mix(0.35, 0.9, 1.0 - roughness) +
                         (1.0 - metallic) * dielectricFresnel;
        float transmitProb = transmittance * (1.0 - metallic) *
                             (1.0 - dielectricFresnel);
        float diffuseProb = (1.0 - metallic) * (1.0 - transmittance);
        specProb = mix(specProb, 1.0, reflectivity);
        transmitProb *= 1.0 - reflectivity;
        diffuseProb *= 1.0 - reflectivity;
        float eta = frontFace ? 1.0 / ior : ior;
        float3 idealRefractedDirection = refract(-V, N, eta);
        bool totalInternalReflection =
            dot(idealRefractedDirection, idealRefractedDirection) < 1e-8;
        if (totalInternalReflection) {
            specProb += transmitProb;
            transmitProb = 0.0;
        }
        float probabilitySum =
            max(specProb + transmitProb + diffuseProb, 1e-4);
        specProb /= probabilitySum;
        transmitProb /= probabilitySum;
        diffuseProb /= probabilitySum;

        float3x3 basis = buildOrthonormalBasis(N);
        if (sceneData.environmentEnabled != 0 &&
            diffuseProb + specProb > 1e-4) {
            float3 localEnvironmentDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            float3 environmentDirection =
                normalizeOr(basis * localEnvironmentDirection, N);
            float NdotEnvironment = dot(N, environmentDirection);
            if (NdotEnvironment > 0.0 &&
                dot(Ng, environmentDirection) > 0.0 &&
                !isOccluded(isect, sceneAS, P, Ng, environmentDirection, 1e30,
                            rng, materials, primitiveObjects,
                            blasPrimitiveOffsets, vertices, indices, sceneData,
                            PT_MATERIAL_TEXTURE_ARGS)) {
                float3 H = normalizeOr(V + environmentDirection, N);
                float NdotH = max(dot(N, H), 1e-5);
                float VdotH = max(dot(V, H), 1e-5);
                float3 F = F_Schlick(VdotH, F0);
                float3 kD = (1.0 - F) * (1.0 - metallic) *
                            (1.0 - transmittance) * (1.0 - reflectivity);
                float diffuseFactor = disneyDiffuseFactor(
                    NdotV, NdotEnvironment,
                    max(dot(environmentDirection, H), 0.0), roughness);
                float3 reflectionBsdf =
                    kD * albedo * diffuseFactor / M_PI_F;
                float environmentPdf = NdotEnvironment / M_PI_F;
                float bsdfPdf = diffuseProb * environmentPdf;
                if (roughness > 0.025 && specProb > 1e-4) {
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L =
                        G1_SmithGGX(NdotEnvironment, roughness);
                    reflectionBsdf +=
                        D * G1V * G1L * F /
                        max(4.0 * NdotV * NdotEnvironment, 1e-6);
                    bsdfPdf += specProb * D * G1V /
                               max(4.0 * NdotV, 1e-6);
                }
                float competingBsdfPdf = depth < bounceLimit ? bsdfPdf : 0.0;
                float misWeight =
                    powerHeuristic(environmentPdf, competingBsdfPdf);
                float3 environmentRadiance = skyColor(
                    environmentDirection, 0.0, skybox, sceneData);
                radiance += throughput * reflectionBsdf *
                            environmentRadiance * NdotEnvironment * misWeight /
                            max(environmentPdf, 1e-6);
            }
        }

        if (depth == bounceLimit) {
            break;
        }

        float choice = rand(rng);
        float3 bounceWeight = float3(0.0);
        float3 nextDirection = N;
        float sampledBsdfPdf = 0.0;
        float sampledEnvironmentPdf = 0.0;
        bool sampledEventWasDelta = true;

        if (choice < specProb && specProb > 1e-4) {
            if (roughness <= 0.025 || totalInternalReflection) {
                nextDirection = reflect(-V, N);
                float3 F = totalInternalReflection
                               ? float3(1.0)
                               : F_Schlick(NdotV, F0);
                bounceWeight = F / max(specProb, 1e-4);
            } else {
                float3 localView =
                    float3(dot(V, basis[0]), dot(V, basis[1]), dot(V, N));
                float3 localH = sampleGGXVNDF(
                    localView, roughness, float2(rand(rng), rand(rng)));
                float3 H = normalizeOr(basis * localH, N);
                float VdotH = max(dot(V, H), 1e-5);
                nextDirection = reflect(-V, H);
                float NdotL = dot(N, nextDirection);
                if (NdotL > 0.0 && dot(nextDirection, Ng) > 0.0) {
                    float NdotH = max(dot(N, H), 1e-5);
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L = G1_SmithGGX(NdotL, roughness);
                    float3 F = F_Schlick(VdotH, F0);
                    float3 specularBsdf =
                        D * G1V * G1L * F /
                        max(4.0 * NdotV * NdotL, 1e-6);
                    float conditionalPdf =
                        D * G1V / max(4.0 * NdotV, 1e-6);
                    float combinedPdf = specProb * conditionalPdf;
                    bounceWeight = specularBsdf * NdotL /
                                   max(combinedPdf, 1e-6);
                    sampledBsdfPdf = combinedPdf;
                    sampledEnvironmentPdf = NdotL / M_PI_F;
                    sampledEventWasDelta = false;
                }
            }
        } else if (choice < specProb + transmitProb &&
                   transmitProb > 1e-4) {
            nextDirection = idealRefractedDirection;
            float3 F = F_Schlick(NdotV, float3(dielectricF0));
            float3 tint = mix(float3(1.0), albedo, 0.15);
            bounceWeight = (1.0 - F) * tint /
                           max(transmitProb, 1e-4);
        } else {
            float3 localDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            nextDirection = norma)",
R"(lizeOr(basis * localDirection, N);
            float NdotL = max(dot(N, nextDirection), 0.0);
            float3 H = normalizeOr(V + nextDirection, N);
            float3 F = F_Schlick(max(dot(V, H), 0.0), F0);
            float3 kD = (1.0 - F) * (1.0 - metallic);
            float diffuseFactor = disneyDiffuseFactor(
                NdotV, NdotL, max(dot(nextDirection, H), 0.0), roughness);
            float3 diffuseBsdf = kD * albedo * diffuseFactor / M_PI_F;
            float conditionalPdf = NdotL / M_PI_F;
            float combinedPdf = diffuseProb * conditionalPdf;
            bounceWeight = diffuseBsdf * NdotL /
                           max(combinedPdf, 1e-6);
            sampledBsdfPdf = combinedPdf;
            sampledEnvironmentPdf = conditionalPdf;
            sampledEventWasDelta = false;
        }

        throughput *= max(bounceWeight, float3(0.0));
        if (depth == 0) {
            throughput *= max(sceneData.indirectStrength, 0.0);
        }
        if (!all(isfinite(throughput)) || max(throughput.x,
                                             max(throughput.y, throughput.z)) <
                                               1e-5) {
            break;
        }

        if (depth >= 2) {
            float survival = clamp(max(throughput.x,
                                       max(throughput.y, throughput.z)),
                                   0.05, 0.95);
            if (rand(rng) > survival) {
                break;
            }
            throughput /= survival;
        }

        previousBsdfPdf = sampledBsdfPdf;
        previousEnvironmentPdf = sampledEnvironmentPdf;
        previousEventWasDelta = sampledEventWasDelta;

        surfaceRay.origin = offsetRayOrigin(P, Ng, nextDirection);
        surfaceRay.direction = normalizeOr(nextDirection, N);
        surfaceRay.min_distance = 0.0;
        surfaceRay.max_distance = 1.0e30;
    }

    return radiance;
}

kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  texture2d<float, access::read_write> historyTex [[texture(1)]],
                  texture2d<float, access::write> brightTex [[texture(2)]],
                  texture2d<float, access::write> albedoRoughnessTex [[texture(3)]],
                  texture2d<float, access::write> normalDepthTex [[texture(4)]],
                  texture2d<float, access::write> motionObjectTex [[texture(5)]],
                  texture2d<float, access::write> momentsHitTex [[texture(6)]],
                  texture2d<float, access::read_write> historyGuideTex [[texture(7)]],
                  primitive_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant uint *primitiveObjects [[buffer(3)]],
                  constant VertexData *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
                  constant SceneData &sceneData [[buffer(8)]],
                  constant PointLight *pointLights [[buffer(9)]],
                  constant SpotLight *spotLights [[buffer(10)]],
                  constant AreaLight *areaLights [[buffer(11)]],
                  PT_MATERIAL_TEXTURE_BINDINGS,
                  constant uint *blasPrimitiveOffsets [[buffer(13)]],
                  texturecube<float> skybox [[texture(60)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint w = outTex.get_width();
    uint h = outTex.get_height();
    uint pixelStride = max(sceneData.pixelStride, 1u);
    gid *= pixelStride;
    if (gid.x >= w || gid.y >= h)
        return;

    float2 uv = (float2(gid) + 0.5) / float2(w, h);
    float3 ro = cam.camPos;

    intersector<triangle_data> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.set_triangle_cull_mode(triangle_cull_mode::none);

    float3 color = float3(0.0);
    float3 primaryAlbedo = float3(0.0);
    float3 primaryNormal = float3(0.0);
    float3 primaryPosition = float3(0.0);
    float primaryDepth = 0.0;
    float primaryRoughness = 1.0;
    float primaryHitDistance = 0.0;
    uint primaryObjectId = 0xFFFFFFFFu;

    uint spp = max(sceneData.raysPerPixel, 1u);
    for (uint s = 0; s < spp; ++s) {
        uint cameraRng = seedBase(gid, w, sceneData.frameIndex,
                                  s + 0x9E3779B9u);
        float2 pixelJitter =
            float2(rand(cameraRng), rand(cameraRng)) - 0.5;
        float2 sampleUv = (float2(gid) + 0.5 + pixelJitter) / float2(w, h);
        float2 sampleNdc = sampleUv * 2.0 - 1.0;
        sampleNdc.y = -sampleNdc.y;
        float4 sampleClip = float4(sampleNdc, 1.0, 1.0);
        float4 sampleWorldH = cam.invViewProj * sampleClip;
        float3 sampleWorldP = sampleWorldH.xyz / sampleWorldH.w;

        ray primaryRay;
        primaryRay.origin = ro;
        primaryRay.direction = normalize(sampleWorldP - ro);
        primaryRay.min_distance = 0.001;
        primaryRay.max_distance = 1.0e30;

        float3 sampleAlbedo = float3(0.0);
        float3 sampleNormal = float3(0.0);
        float3 samplePosition = float3(0.0);
        float sampleDepth = 0.0;
        float sampleRoughness = 1.0;
        float sampleHitDistance = 0.0;
        uint sampleObjectId = 0xFFFFFFFFu;

        float3 sample = sampleRadiance(
            gid, s, w, isect, sceneAS, primaryRay, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, dirLight,
            sceneData, pointLights, spotLights, areaLights,
            PT_MATERIAL_TEXTURE_ARGS, skybox, sampleAlbedo, sampleNormal,
            samplePosition, sampleDepth, sampleRoughness, sampleHitDistance,
            sampleObjectId);
        color += sample;
        if (s == 0) {
            primaryAlbedo = sampleAlbedo;
            primaryNormal = sampleNormal;
            primaryPosition = samplePosition;
            primaryDepth = sampleDepth;
            primaryRoughness = sampleRoughness;
            primaryHitDistance = sampleHitDistance;
            primaryObjectId = sampleObjectId;
        }
    }

    color /= float(spp);
    if (!all(isfinite(color))) {
        color = float3(0.0);
    }

    int frameIndex = int(sceneData.frameIndex);

    float4 prevColor = historyTex.read(gid);
    float4 previousGuide = historyGuideTex.read(gid);
    float objectIdValue = primaryObjectId == 0xFFFFFFFFu
                              ? -1.0
                              : float(primaryObjectId);
    float2 encodedNormal = encodeNormal(primaryNormal);
    float4 currentGuide =
        float4(encodedNormal, primaryDepth, objectIdValue);
    bool historyValid = frameIndex > 0 &&
                        abs(previousGuide.z - primaryDepth) <
                            max(0.05, primaryDepth * 0.02) &&
                        distance(previousGuide.xy, encodedNormal) < 0.08 &&
                        abs(previousGuide.w - objectIdValue) < 0.5;
    if (frameIndex == 0)
        prevColor = float4(0, 0, 0, 1);
    float sampleLuminanceLimit =
        historyValid ? max(8.0, luminance(prevColor.xyz) * 6.0 + 2.0) : 128.0;
    color = clampLuminance(color, sampleLuminanceLimit);
    if (!historyValid)
        prevColor = float4(color, 1.0);

    float historyLength = historyValid ? min(float(frameIndex), 255.0) : 0.0;
    float3 lower = min(prevColor.xyz, color) - float3(0.35);
    float3 upper = max(prevColor.xyz, color) + float3(0.35);
    float3 clippedHistory = clamp(prevColor.xyz, lower, upper);
    float3 accum = mix(color, clippedHistory,
                       historyLength / (historyLength + 1.0));
    accum = clampLuminance(accum, 256.0);

    constexpr float bloomThreshold = 0.8;
    constexpr float bloomKnee = 0.35;

    float brightness = luminance(accum);
    float soft = clamp(brightness - bloomThreshold + bloomKnee, 0.0,
                       bloomKnee * 2.0);
    soft = soft * soft / max(bloomKnee * 4.0, 0.00001);
    float contribution = max(brightness - bloomThreshold, soft) /
                         )",
R"(max(brightness, 0.00001);
    float3 brightColor = accum * contribution;
	float4 previousClip = cam.prevViewProj * float4(primaryPosition, 1.0);
	float2 previousUv = previousClip.xy / max(abs(previousClip.w), 0.0001);
	previousUv = previousUv * 0.5 + 0.5;
	float2 motion = uv - previousUv;
	float moment = luminance(color);

    for (uint y = 0; y < pixelStride; ++y) {
        for (uint x = 0; x < pixelStride; ++x) {
            uint2 pixel = gid + uint2(x, y);
            if (pixel.x >= w || pixel.y >= h) {
                continue;
            }
            historyTex.write(float4(accum, 1.0), pixel);
            historyGuideTex.write(currentGuide, pixel);
            albedoRoughnessTex.write(float4(primaryAlbedo, primaryRoughness),
                                     pixel);
            normalDepthTex.write(float4(primaryNormal, primaryDepth), pixel);
            motionObjectTex.write(float4(motion, objectIdValue, 1.0), pixel);
            momentsHitTex.write(float4(moment, moment * moment,
                                       primaryRoughness, primaryHitDistance),
                                pixel);
            outTex.write(float4(accum, 1.0), pixel);
            brightTex.write(float4(brightColor, 1.0), pixel);
        }
    }
}
)",
};
static const AtlasPackedShaderSource PATH = {PATH_PARTS, 9};

static const char* const PATH_DENOISE_PARTS[] = {
R"(#include <metal_stdlib>
using namespace metal;

struct DenoiseParameters {
    int stepWidth;
};

kernel void main0(texture2d<float, access::read> inputTexture [[texture(0)]],
                  texture2d<float, access::write> outputTexture [[texture(1)]],
                  texture2d<float, access::write> brightTexture [[texture(2)]],
                  texture2d<float, access::read> guideTexture [[texture(3)]],
                  texture2d<float, access::read> albedoRoughnessTexture
                      [[texture(4)]],
                  constant DenoiseParameters &parameters [[buffer(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint width = outputTexture.get_width();
    uint height = outputTexture.get_height();
    if (gid.x >= width || gid.y >= height)
        return;

    constexpr int2 offsets[9] = {int2(0, 0),  int2(1, 0),  int2(-1, 0),
                                 int2(0, 1),  int2(0, -1), int2(1, 1),
                                 int2(-1, 1), int2(1, -1), int2(-1, -1)};
    constexpr float weights[9] = {0.28, 0.12, 0.12, 0.12, 0.12,
                                  0.06, 0.06, 0.06, 0.06};
    float3 center = inputTexture.read(gid).xyz;
    float4 centerGuide = guideTexture.read(gid);
    float4 centerAlbedoRoughness = albedoRoughnessTexture.read(gid);
    bool centerSurface = centerGuide.w > 0.0;
    float centerNormalLength = dot(centerGuide.xyz, centerGuide.xyz);
    float centerLuminance = dot(center, float3(0.2126, 0.7152, 0.0722));
    float3 filtered = float3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 9; ++i) {
        int2 samplePosition =
            clamp(int2(gid) + offsets[i] * parameters.stepWidth, int2(0),
                  int2(width - 1, height - 1));
        float3 sampleColor = inputTexture.read(uint2(samplePosition)).xyz;
        float4 sampleGuide = guideTexture.read(uint2(samplePosition));
        float4 sampleAlbedoRoughness =
            albedoRoughnessTexture.read(uint2(samplePosition));
        float sampleLuminance =
            dot(sampleColor, float3(0.2126, 0.7152, 0.0722));
        float roughness = clamp(centerAlbedoRoughness.w, 0.0, 1.0);
        float luminanceScale = mix(2.5, 7.0, roughness);
        float luminanceDifference =
            abs(sampleLuminance - centerLuminance) /
            max(1.0, max(sampleLuminance, centerLuminance));
        float edgeWeight = exp(-luminanceDifference * luminanceScale);
        bool sampleSurface = sampleGuide.w > 0.0;
        float normalWeight = centerSurface == sampleSurface ? 1.0 : 0.0;
        float depthWeight = normalWeight;
        float albedoWeight = normalWeight;
        if (centerSurface && sampleSurface) {
            float sampleNormalLength = dot(sampleGuide.xyz, sampleGuide.xyz);
            if (centerNormalLength > 1e-6 && sampleNormalLength > 1e-6) {
                float normalSimilarity =
                    dot(centerGuide.xyz * rsqrt(centerNormalLength),
                        sampleGuide.xyz * rsqrt(sampleNormalLength));
                normalWeight *= pow(max(normalSimilarity, 0.0), 24.0);
            } else if (i != 0) {
                normalWeight = 0.0;
            }
            float depthScale = max(0.01, abs(centerGuide.w) * 0.01) *
                               max(float(parameters.stepWidth), 1.0);
            depthWeight *=
                exp(-abs(sampleGuide.w - centerGuide.w) / depthScale);
            albedoWeight *= exp(-length(sampleAlbedoRoughness.xyz -
                                        centerAlbedoRoughness.xyz) *
                                8.0);
        }
        float weight = weights[i] * edgeWeight * normalWeight * depthWeight *
                       albedoWeight;
        filtered += sampleColor * weight;
        totalWeight += weight;
    }
    float3 result = totalWeight > 0.0001 ? filtered / totalWeight : center;
    float brightness = dot(result, float3(0.2126, 0.7152, 0.0722));
    constexpr float bloomThreshold = 0.8;
    constexpr float bloomKnee = 0.35;
    float soft = clamp(brightness - bloomThreshold + bloomKnee, 0.0,
                       bloomKnee * 2.0);
    soft = soft * soft / max(bloomKnee * 4.0, 0.00001);
    float contribution = max(brightness - bloomThreshold, soft) /
                         max(brightness, 0.00001);
    outputTexture.write(float4(result, 1.0), gid);
    brightTexture.write(float4(result * contribution, 1.0), gid);
}
)",
};
static const AtlasPackedShaderSource PATH_DENOISE = {PATH_DENOISE_PARTS, 1};

static const char* const POINT_DEPTH_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    packed_float3 lightPos;
    float far_plane;
};

struct main0_out
{
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float4 FragPos [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _17 [[buffer(0)]])
{
    main0_out out = {};
    float lightDistance = length(in.FragPos.xyz - float3(_17.lightPos));
    lightDistance /= _17.far_plane;
    out.gl_FragDepth = lightDistance;
    return out;
}

)",
};
static const AtlasPackedShaderSource POINT_DEPTH_FRAG = {POINT_DEPTH_FRAG_PARTS, 1};

static const char* const POINT_DEPTH_GEOM_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct ShadowMatrices
{
    float4x4 shadowMatrices[6];
};

struct main0_out
{
    float4 FragPos;
    float4 gl_Position;
    uint gl_Layer;
};

unknown main0_out main0(constant ShadowMatrices& _55 [[buffer(0)]])
{
    main0_out out = {};
    for (int face = 0; face < 6; face++)
    {
        out.gl_Layer = uint(face);
        for (int i = 0; i < 3; i++)
        {
            out.FragPos = _RESERVED_IDENTIFIER_FIXUP_gl_in[i].out.gl_Position;
            out.gl_Position = _55.shadowMatrices[face] * out.FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
    return out;
}

)",
};
static const AtlasPackedShaderSource POINT_DEPTH_GEOM = {POINT_DEPTH_GEOM_PARTS, 1};

static const char* const POINT_DEPTH_NOGEOM_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    packed_float3 lightPos;
    float far_plane;
};

struct main0_out
{
    float gl_FragDepth [[depth(any)]];
};

struct main0_in
{
    float4 FragPos [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _17 [[buffer(0)]])
{
    main0_out out = {};
    float lightDistance = length(in.FragPos.xyz - float3(_17.lightPos));
    lightDistance /= _17.far_plane;
    out.gl_FragDepth = lightDistance;
    return out;
}

)",
};
static const AtlasPackedShaderSource POINT_DEPTH_NOGEOM_FRAG = {POINT_DEPTH_NOGEOM_FRAG_PARTS, 1};

static const char* const POINT_DEPTH_NOGEOM_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    uint isInstanced;
};

struct ShadowUniforms
{
    float4x4 shadowMatrix;
    int faceIndex;
};

struct main0_out
{
    float4 FragPos [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]], constant ShadowUniforms& _62 [[buffer(1)]])
{
    main0_out out = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    float4 worldPos;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        worldPos = (_12.model * instanceModel) * float4(in.aPos, 1.0);
    }
    else
    {
        worldPos = _12.model * float4(in.aPos, 1.0);
    }
    out.FragPos = worldPos;
    out.gl_Position = _62.shadowMatrix * worldPos;
    return out;
}
)",
};
static const AtlasPackedShaderSource POINT_DEPTH_NOGEOM_VERT = {POINT_DEPTH_NOGEOM_VERT_PARTS, 1};

static const char* const POINT_DEPTH_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    uint isInstanced;
};

struct main0_out
{
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _12 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((_12.isInstanced != 0u) && hasInstanceMatrix)
    {
        out.gl_Position = (_12.model * instanceModel) * float4(in.aPos, 1.0);
    }
    else
    {
        out.gl_Position = _12.model * float4(in.aPos, 1.0);
    }
    return out;
}
)",
};
static const AtlasPackedShaderSource POINT_DEPTH_VERT = {POINT_DEPTH_VERT_PARTS, 1};

static const char* const SKYBOX_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float3 sunDirection;
    float4 sunColor;
    float3 moonDirection;
    float4 moonColor;
    int hasDayNight;
};

struct AtmosphereParams
{
    float sunTintStrength;
    float moonTintStrength;
    float sunSizeMultiplier;
    float moonSizeMultiplier;
    float starDensity;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float3 TexCoords [[user(locn0)]];
};

static inline __attribute__((always_inline))
float hash13(thread float3& p)
{
    p = fract(p * 443.897003173828125);
    p += float3(dot(p, p.yzx + float3(19.1900005340576171875)));
    return fract((p.x + p.y) * p.z);
}

static inline __attribute__((always_inline))
float3 generateStars(thread const float3& dir, thread const float& density, thread const float& nightFactor)
{
    if ((density <= 0.0) || (nightFactor <= 0.0))
    {
        return float3(0.0);
    }
    float3 starSpace = dir * 50.0;
    float3 cell = floor(starSpace);
    float3 localPos = fract(starSpace);
    float3 param = cell;
    float _165 = hash13(param);
    float rand = _165;
    if (rand < (density * 0.300000011920928955078125))
    {
        float3 param_1 = cell + float3(12.340000152587890625, 56.779998779296875, 90.12000274658203125);
        float _181 = hash13(param_1);
        float randX = _181;
        float3 param_2 = cell + float3(23.450000762939453125, 67.8899993896484375, 1.230000019073486328125);
        float _190 = hash13(param_2);
        float randY = _190;
        float3 param_3 = cell + float3(34.560001373291015625, 78.90000152587890625, 12.340000152587890625);
        float _198 = hash13(param_3);
        float randZ = _198;
        float3 starPos = float3(randX, randY, randZ);
        float dist = length(localPos - starPos);
        float3 param_4 = cell + float3(45.6699981689453125, 89.01000213623046875, 23.450000762939453125);
        float _217 = hash13(param_4);
        float starSize = 0.0199999995529651641845703125 + (_217 * 0.02999999932944774627685546875);
        float3 param_5 = cell + float3(56.779998779296875, 90.12000274658203125, 34.560001373291015625);
        float _227 = hash13(param_5);
        float brightness = 0.5 + (_227 * 0.5);
        float star = smoothstep(starSize, 0.0, dist) * brightness;
        float3 param_6 = cell + float3(67.8899993896484375, 1.230000019073486328125, 45.6699981689453125);
        float _243 = hash13(param_6);
        float twinkle = 0.800000011920928955078125 + (0.20000000298023223876953125 * sin(_243 * 100.0));
        star *= (twinkle * nightFactor);
        float3 starColor = float3(1.0);
        if (rand > 0.89999997615814208984375)
        {
            starColor = float3(0.800000011920928955078125, 0.89999997615814208984375, 1.0);
        }
        else
        {
            if (rand > 0.800000011920928955078125)
            {
                starColor = float3(1.0, 0.89999997615814208984375, 0.800000011920928955078125);
            }
        }
        return starColor * star;
    }
    return float3(0.0);
}

static inline __attribute__((always_inline))
float hash21(thread float2& p)
{
    p = fract(p * float2(123.339996337890625, 456.209991455078125));
    p += float2(dot(p, p + float2(45.31999969482421875)));
    return fract(p.x * p.y);
}

static inline __attribute__((always_inline))
float valueNoise(thread const float2& p)
{
    float2 i = floor(p);
    float2 f = fract(p);
    f = (f * f) * (float2(3.0) - (f * 2.0));
    float2 param = i;
    float _80 = hash21(param);
    float a = _80;
    float2 param_1 = i + float2(1.0, 0.0);
    float _88 = hash21(param_1);
    float b = _88;
    float2 param_2 = i + float2(0.0, 1.0);
    float _94 = hash21(param_2);
    float c = _94;
    float2 param_3 = i + float2(1.0);
    float _100 = hash21(param_3);
    float d = _100;
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

static inline __attribute__((always_inline))
float3 generateMoonTexture(thread float2& uv, thread const float& distanceFromCenter, thread const float3& tintColor)
{
    float angle = 0.5;
    float ca = cos(angle);
    float sa = sin(angle);
    uv = float2((ca * uv.x) - (sa * uv.y), (sa * uv.x) + (ca * uv.y));
    float2 param = uv * 2.0;
    float largeFeatures = valueNoise(param);
    largeFeatures = smoothstep(0.300000011920928955078125, 0.699999988079071044921875, largeFeatures);
    float2 param_1 = uv * 8.0;
    float mediumCraters = valueNoise(param_1);
    float2 craterUV = uv * 6.0;
    float2 craterCell = floor(craterUV);
    float2 craterLocal = fract(craterUV);
    float craters = 1.0;
    for (int i = 0; i < 4; i++)
    {
        float2 neighbor = float2(float(i % 2), float(i / 2));
        float2 cellPoint = craterCell + neighbor;
        float2 param_2 = cellPoint;
        float _353 = hash21(param_2);
        float2 param_3 = cellPoint + float2(13.69999980926513671875, 27.299999237060546875);
        float _360 = hash21(param_3);
        float2 craterCenter = float2(_353, _360);
        float dist = length((craterLocal - neighbor) - craterCenter);
        float2 param_4 = cellPoint + float2(5.30000019073486328125, 9.69999980926513671875);
        float _378 = hash21(param_4);
        float craterSize = 0.1500000059604644775390625 + (0.25 * _378);
        if (dist < craterSize)
        {
            float crater = smoothstep(craterSize, craterSize * 0.300000011920928955078125, dist);
            craters = fast::min(craters, 1.0 - (crater * 0.699999988079071044921875));
        }
    }
    float surface = (largeFeatures * 0.5) + (mediumCraters * 0.5);
    surface *= craters;
    float intensity = mix(0.300000011920928955078125, 0.75, surface);
    float limb = 1.0 - smoothstep(0.60000002384185791015625, 1.0, distanceFromCenter);
    intensity *= (0.4000000059604644775390625 + (0.60000002384185791015625 * limb));
    intensity *= 1.2999999523162841796875;
    return fast::clamp(tintColor * intensity, float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _452 [[buffer(0)]], constant AtmosphereParams& _484 [[buffer(1)]], texturecube<float> skybox [[texture(0)]], sampler skyboxSmplr [[sampler(0)]])
{
    main0_out out = {};
    float3 dir = fast::normalize(in.TexCoords);
    float3 color = skybox.sample(skyboxSmplr, in.TexCoords).xyz;
    if (_452.hasDayNight == 1)
    {
        float3 normSunDir = fast::normalize(_452.sunDirection);
        float3 normMoonDir = fast::normalize(_452.moonDirection);
        float sunDot = dot(dir, normSunDir);
        float moonDot = dot(dir, normMoonDir);
        float nightFactor = smoothstep(0.1500000059604644775390625, -0.20000000298023223876953125, _452.sunDirection.y);
        if (_484.starDensity > 0.0)
        {
            float3 param = dir;
            float param_1 = _484.starDensity;
            float param_2 = nightFactor;
            color += generateStars(param, param_1, param_2);
        }
        float sunHorizonFade = smoothstep(-0.1500000059604644775390625, 0.0500000007450580596923828125, _452.sunDirection.y);
        if (_452.sunDirection.y > (-0.1500000059604644775390625))
        {
            float sizeAdjust = 1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.001000000047497451305389404296875);
            float sunSize = 0.999499976634979248046875 * sizeAdjust;
            float sunGlowSize = 0.99800002574920654296875 * (1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.0030000000260770320892333984375));
            float sunHaloSize = 0.9900000095367431640625 * (1.0 - ((_484.sunSizeMultiplier - 1.0) * 0.014999999664723873138427734375));
            float sunDisk = smoothstep(sunSize - 0.00019999999494757503271102905273438, sunSize, sunDot);
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
            float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) * (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
            float horizonBoost = smoothstep(0.100000001490116119384765625, -0.0500000007450580596923828125, _452.sunDirection.y) * 2.0;
            sunHalo *= (0.300000011920928955078125 )",
R"(+ horizonBoost);
            color += ((_452.sunColor.xyz * (((sunDisk * 5.0) + (sunGlow * 0.5)) + sunHalo)) * sunHorizonFade);
        }
        float moonHorizonFade = smoothstep(-0.1500000059604644775390625, 0.0500000007450580596923828125, _452.moonDirection.y);
        if (_452.moonDirection.y > (-0.1500000059604644775390625))
        {
            float sizeAdjust_1 = 1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.001000000047497451305389404296875);
            float moonSize = 0.999599993228912353515625 * sizeAdjust_1;
            float moonGlowSize = 0.99849998950958251953125 * (1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.0030000000260770320892333984375));
            float moonHaloSize = 0.99199998378753662109375 * (1.0 - ((_484.moonSizeMultiplier - 1.0) * 0.014999999664723873138427734375));
            float moonDisk = smoothstep(moonSize - 0.00019999999494757503271102905273438, moonSize, moonDot);
            if (moonDisk > 0.00999999977648258209228515625)
            {
                float3 up = (abs(normMoonDir.y) < 0.999000012874603271484375) ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
                float3 right = fast::normalize(cross(up, normMoonDir));
                float3 actualUp = cross(normMoonDir, right);
                float3 relativeDir = dir - (normMoonDir * moonDot);
                float u = dot(relativeDir, right);
                float v = dot(relativeDir, actualUp);
                float distFromCenter = length(float2(u, v)) / sqrt(1.0 - (moonSize * moonSize));
                if (distFromCenter < 1.0)
                {
                    float2 moonUV = float2(u, v) * 200.0;
                    float2 param_3 = moonUV;
                    float param_4 = distFromCenter;
                    float3 param_5 = _452.moonColor.xyz;
                    float3 _703 = generateMoonTexture(param_3, param_4, param_5);
                    float3 moonTexture = _703;
                    color += (((moonTexture * moonDisk) * 1.5) * moonHorizonFade);
                }
                else
                {
                    color += (((_452.moonColor.xyz * moonDisk) * 1.5) * moonHorizonFade);
                }
            }
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * (1.0 - moonDisk);
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            float moonHorizonBoost = smoothstep(0.100000001490116119384765625, -0.0500000007450580596923828125, _452.moonDirection.y) * 2.0;
            moonHalo *= (0.20000000298023223876953125 + moonHorizonBoost);
            color += ((_452.moonColor.xyz * ((moonGlow * 0.300000011920928955078125) + moonHalo)) * moonHorizonFade);
        }
        bool _767 = _452.sunDirection.y > (-0.100000001490116119384765625);
        bool _773;
        if (_767)
        {
            _773 = _484.sunTintStrength > 0.0;
        }
        else
        {
            _773 = _767;
        }
        if (_773)
        {
            float sunSkyInfluence = smoothstep(0.699999988079071044921875, 0.949999988079071044921875, sunDot) * smoothstep(-0.100000001490116119384765625, 0.20000000298023223876953125, _452.sunDirection.y);
            color = mix(color, color * _452.sunColor.xyz, float3((sunSkyInfluence * 0.300000011920928955078125) * _484.sunTintStrength));
            float sunProximity = smoothstep(0.300000011920928955078125, 0.800000011920928955078125, sunDot) * smoothstep(-0.100000001490116119384765625, 0.300000011920928955078125, _452.sunDirection.y);
            color += (((_452.sunColor.xyz * sunProximity) * 0.1500000059604644775390625) * _484.sunTintStrength);
            float globalSunTint = smoothstep(-0.100000001490116119384765625, 0.5, _452.sunDirection.y);
            color = mix(color, _452.sunColor.xyz, float3((globalSunTint * _484.sunTintStrength) * 0.07999999821186065673828125));
        }
        bool _833 = _452.moonDirection.y > (-0.100000001490116119384765625);
        bool _839;
        if (_833)
        {
            _839 = _484.moonTintStrength > 0.0;
        }
        else
        {
            _839 = _833;
        }
        if (_839)
        {
            float moonSkyInfluence = smoothstep(0.800000011920928955078125, 0.949999988079071044921875, moonDot) * smoothstep(-0.100000001490116119384765625, 0.20000000298023223876953125, _452.moonDirection.y);
            color = mix(color, color * _452.moonColor.xyz, float3((moonSkyInfluence * 0.20000000298023223876953125) * _484.moonTintStrength));
            float moonProximity = smoothstep(0.5, 0.85000002384185791015625, moonDot) * smoothstep(-0.100000001490116119384765625, 0.300000011920928955078125, _452.moonDirection.y);
            color += (((_452.moonColor.xyz * moonProximity) * 0.07999999821186065673828125) * _484.moonTintStrength);
            float globalMoonTint = smoothstep(-0.100000001490116119384765625, 0.5, _452.moonDirection.y);
            color = mix(color, _452.moonColor.xyz, float3((globalMoonTint * _484.moonTintStrength) * 0.0500000007450580596923828125));
        }
    }
    out.FragColor = float4(color, 1.0);
    return out;
}

)",
};
static const AtlasPackedShaderSource SKYBOX_FRAG = {SKYBOX_FRAG_PARTS, 2};

static const char* const SKYBOX_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UniformBufferObject
{
    float4x4 projection;
    float4x4 view;
};

struct main0_out
{
    float3 TexCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UniformBufferObject& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.TexCoords = in.aPos;
    float3x3 _32 = float3x3(_19.view[0].xyz, _19.view[1].xyz, _19.view[2].xyz);
    float4x4 viewNoTranslation = float4x4(float4(_32[0].x, _32[0].y, _32[0].z, 0.0), float4(_32[1].x, _32[1].y, _32[1].z, 0.0), float4(_32[2].x, _32[2].y, _32[2].z, 0.0), float4(0.0, 0.0, 0.0, 1.0));
    float4 pos = (_19.projection * viewNoTranslation) * float4(in.aPos, 1.0);
    out.gl_Position = pos.xyww;
    return out;
}

)",
};
static const AtlasPackedShaderSource SKYBOX_VERT = {SKYBOX_VERT_PARTS, 1};

static const char* const SSAO_BLUR_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> inSSAO [[texture(0)]], sampler inSSAOSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 texelSize = float2(1.0) / float2(int2(inSSAO.get_width(), inSSAO.get_height()));
    float result = 0.0;
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += inSSAO.sample(inSSAOSmplr, (in.TexCoord + offset)).x;
        }
    }
    out.FragColor = result / 25.0;
    return out;
}

)",
};
static const AtlasPackedShaderSource SSAO_BLUR_FRAG = {SSAO_BLUR_FRAG_PARTS, 1};

static const char* const SSAO_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Paramters {
    float4x4 projection;
    float4x4 inverseProjection;
    float4x4 view;
    float2 noiseScale;
};

struct Samples {
    float3 samples[64];
};

struct main0_out {
    float FragColor [[color(0)]];
};

struct main0_in {
    float2 TexCoord [[user(locn0)]];
};

static inline float3 reconstructViewPos(float2 uv, float depth,
                                        float4x4 inverseProjection) {
    float2 ndc;
    ndc.x = uv.x * 2.0 - 1.0;
    ndc.y = (1.0 - uv.y) * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    float4 clip = float4(ndc, ndcZ, 1.0);
    float4 viewPos = inverseProjection * clip;
    float invW = 1.0 / max(abs(viewPos.w), 1e-6);
    return viewPos.xyz * invW;
}

fragment main0_out main0(main0_in in [[stage_in]],
                         constant Paramters &_43 [[buffer(0)]],
                         constant Samples &_137 [[buffer(1)]],
                         texture2d<float> gPosition [[texture(0)]],
                         texture2d<float> gNormal [[texture(1)]],
                         texture2d<float> texNoise [[texture(2)]],
                         sampler gPositionSmplr [[sampler(0)]],
                         sampler gNormalSmplr [[sampler(1)]],
                         sampler texNoiseSmplr [[sampler(2)]]) {
    main0_out out = {};
    float3 sampledNormalWorld = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    float sampledNormalLength = length(sampledNormalWorld);
    if (!all(isfinite(sampledNormalWorld)) ||
        sampledNormalLength <= 0.001000000047497451305389404296875) {
        out.FragColor = 1.0;
        return out;
    }
    float4 gPositionCenter = gPosition.sample(gPositionSmplr, in.TexCoord);
    float centerDepth = gPositionCenter.w;
    if (!isfinite(centerDepth)) {
        out.FragColor = 1.0;
        return out;
    }
    centerDepth = clamp(centerDepth, 0.0, 1.0);
    float3 fragPos = reconstructViewPos(in.TexCoord, centerDepth,
                                        _43.inverseProjection);
    float3 normalWorld = sampledNormalWorld / sampledNormalLength;
    float3 normal = fast::normalize((_43.view * float4(normalWorld, 0.0)).xyz);
    float3 randomVec = fast::normalize(
        (texNoise.sample(texNoiseSmplr, (in.TexCoord * _43.noiseScale)).xyz *
         2.0) -
        float3(1.0));
    float3 tangent =
        fast::normalize(randomVec - (normal * dot(randomVec, normal)));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(float3(tangent), float3(bitangent), float3(normal));
    float viewDepth = abs(fragPos.z);
    float baseRadius =
        mix(0.550000011920928955078125, 2.2000000476837158203125,
            clamp(viewDepth / 140.0, 0.0, 1.0));
    float3 dPosDx = dfdx(fragPos);
    float3 dPosDy = dfdy(fragPos);
    float pixelWorldScale = max(length(dPosDx), length(dPosDy));
    if (!isfinite(pixelWorldScale)) {
        pixelWorldScale = 0.0;
    }
    float sampleRadius =
        clamp(max(baseRadius, pixelWorldScale * 6.0), 0.3499999940395355,
              12.0);
    float depthBias = clamp(
        max(mix(0.006000000052154064178466796875,
                0.0240000002086162567138671875,
                clamp(viewDepth / 180.0, 0.0, 1.0)),
            sampleRadius * 0.009999999776482582),
        0.003000000026077032, 0.07999999821186066);
    float occlusion = 0.0;
    int validSamples = 0;
    for (int i = 0; i < 64; i++) {
        float3 samplePos = TBN * _137.samples[i];
        samplePos = fragPos + (samplePos * sampleRadius);
        float4 offset = _43.projection * float4(samplePos, 1.0);
        float _160 = offset.w;
        float4 _161 = offset;
        float3 _164 = _161.xyz / float3(_160);
        offset.x = _164.x;
        offset.y = _164.y;
        offset.z = _164.z;
        float4 _174 = offset;
        float2 _178 = (_174.xy * 0.5) + float2(0.5);
        offset.x = _178.x;
        offset.y = 1.0 - _178.y;
        bool _185 = offset.x < 0.0;
        bool _192;
        if (!_185) {
            _192 = offset.x > 1.0;
        } else {
            _192 = _185;
        }
        bool _199;
        if (!_192) {
            _199 = offset.y < 0.0;
        } else {
            _199 = _192;
        }
        bool _206;
        if (!_199) {
            _206 = offset.y > 1.0;
        } else {
            _206 = _199;
        }
        if (_206) {
            continue;
        }
        float sampleDepth = gPosition.sample(gPositionSmplr, offset.xy).w;
        if (!isfinite(sampleDepth)) {
            continue;
        }
        sampleDepth = clamp(sampleDepth, 0.0, 1.0);
        float3 sampleViewPos =
            reconstructViewPos(offset.xy, sampleDepth, _43.inverseProjection);
        float rangeCheck =
            smoothstep(0.0, 1.0,
                       sampleRadius / (abs(fragPos.z - sampleViewPos.z) +
                                        0.0005000000237487256526947021484375));
        occlusion +=
            (float(sampleViewPos.z >= (samplePos.z + depthBias)) * rangeCheck);
        validSamples++;
    }
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }
    out.FragColor = occlusion;
    return out;
}
)",
};
static const AtlasPackedShaderSource SSAO_FRAG = {SSAO_FRAG_PARTS, 1};

static const char* const SSR_BLUR_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

fragment void main0()
{
}

)",
};
static const AtlasPackedShaderSource SSR_BLUR_FRAG = {SSR_BLUR_FRAG_PARTS, 1};

static const char* const SSR_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 inverseProjection;
    float4x4 inverseView;
    float4x4 projection;
    float4x4 view;
    float3 cameraPosition;
};

struct SSRParameters
{
    float maxDistance;
    float resolution;
    int steps;
    float thickness;
    float maxRoughness;
    float historyWeight;
    int debugMode;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline))
float3 fresnelSchlick(thread const float& cosTheta, thread const float3& F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline))
float3 sampleSkyReflection(thread const float3& normal, thread const float3& viewDir, thread const float3& albedo, thread const float& metallic, texturecube<float> skybox, sampler skyboxSmplr)
{
    float3 reflected = reflect(-fast::normalize(viewDir), fast::normalize(normal));
    float3 skyColor = skybox.sample(skyboxSmplr, reflected).xyz;
    float skyLuma = dot(skyColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    if (skyLuma < 0.001000000047497451305389404296875)
    {
        float t = fast::clamp((reflected.y * 0.5) + 0.5, 0.0, 1.0);
        float3 horizon = float3(0.7400000095367431640625, 0.790000021457672119140625, 0.87999999523162841796875);
        float3 zenith = float3(0.5, 0.63999998569488525390625, 0.839999973773956298828125);
        skyColor = mix(horizon, zenith, float3(t));
    }
    float tintStrength = metallic * 0.3499999940395355224609375;
    float3 tint = mix(float3(1.0), albedo, float3(tintStrength));
    return skyColor * tint;
}

static inline __attribute__((always_inline))
float4 SSR(thread const float3& worldPos, thread const float3& normal, thread const float3& viewDir, thread const float& roughness, thread const float& metallic, thread const float& reflectivity, thread const float3& albedo, constant Uniforms& _42, constant SSRParameters& _96, thread float4& gl_FragCoord, texture2d<float> gPosition, sampler gPositionSmplr, texture2d<float> sceneColor, sampler sceneColorSmplr, texture2d<float> gNormal, sampler gNormalSmplr, texture2d<float> gDepth, sampler gDepthSmplr, texturecube<float> skybox, sampler skyboxSmplr)
{
    float3 viewPos = (_42.view * float4(worldPos, 1.0)).xyz;
    float3 viewNormal = fast::normalize((_42.view * float4(normal, 0.0)).xyz);
    float mirrorFactor = fast::clamp(fast::max(metallic, reflectivity) * (1.0 - roughness), 0.0, 1.0);
    float3 skyReflection = sampleSkyReflection(normal, viewDir, albedo, metallic, skybox, skyboxSmplr);
    float3 viewDirection = fast::normalize(viewPos);
    float3 viewReflect = fast::normalize(reflect(viewDirection, viewNormal));
    if (viewReflect.z > 0.0)
    {
        return float4(0.0);
    }
    float3 rayOrigin = viewPos + (viewNormal * 0.00999999977648258209228515625);
    float3 rayDir = viewReflect;
    float stepSize = _96.maxDistance / float(_96.steps);
    float3 currentPos = rayOrigin;
    float jitter = fract(sin(dot(gl_FragCoord.xy, float2(12.98980045318603515625, 78.233001708984375))) * 43758.546875) * roughness * 0.25;
    currentPos += ((rayDir * stepSize) * jitter);
    float2 hitUV = float2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    float2 fallbackUV = float2(0.0);
    bool hasFallbackUV = false;
    float3 lastPos = currentPos;
    for (int i = 0; i < _96.steps; i++)
    {
        float t = float(i) / float(_96.steps);
        float adaptiveStep = mix(0.300000011920928955078125, 1.0, t);
        currentPos += ((rayDir * stepSize) * adaptiveStep);
        float4 projectedPos = _42.projection * float4(currentPos, 1.0);
        float _186 = projectedPos.w;
        float4 _187 = projectedPos;
        float3 _190 = _187.xyz / float3(_186);
        projectedPos.x = _190.x;
        projectedPos.y = _190.y;
        projectedPos.z = _190.z;
        float2 screenUV = (projectedPos.xy * 0.5) + float2(0.5);
        bool _208 = screenUV.x < 0.0;
        bool _215;
        if (!_208)
        {
            _215 = screenUV.x > 1.0;
        }
        else
        {
            _215 = _208;
        }
        bool _222;
        if (!_215)
        {
            _222 = screenUV.y < 0.0;
        }
        else
        {
            _222 = _215;
        }
        bool _229;
        if (!_222)
        {
            _229 = screenUV.y > 1.0;
        }
        else
        {
            _229 = _222;
        }
        if (!_229)
        {
            fallbackUV = screenUV;
            hasFallbackUV = true;
        }
        else
        {
            break;
        }
        float hierarchyLevel = fast::clamp(floor(log2(1.0 + float(i) * 0.25)),
                                           0.0, 5.0);
        float rawDepth = gDepth.sample(gDepthSmplr, screenUV,
                                       level(hierarchyLevel)).x;
        if (rawDepth >= 0.99989998340606689453125)
        {
            currentPos += rayDir * stepSize * (exp2(hierarchyLevel) - 1.0);
            lastPos = currentPos;
            continue;
        }
        float3 sampleWorldPos = gPosition.sample(gPositionSmplr, screenUV).xyz;
        float3 sampleNormal = gNormal.sample(gNormalSmplr, screenUV).xyz;
        if (dot(sampleNormal, sampleNormal) < 0.00999999977648258209228515625)
        {
            lastPos = currentPos;
            continue;
        }
        if (length(sampleWorldPos - worldPos) < 0.07999999821186065673828125)
        {
            lastPos = currentPos;
            continue;
        }
        float3 sampleViewPos = (_42.view * float4(sampleWorldPos, 1.0)).xyz;
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        float3 sampleViewNormal = fast::normalize((_42.view * float4(sampleNormal, 0.0)).xyz);
        if ((dot(sampleViewNormal, viewNormal) > 0.920000016689300537109375) && (abs(sampleDepth - currentDepth) < (_96.thickness * 1.5)))
        {
            lastPos = currentPos;
            continue;
        }
        float depthDiff = sampleDepth - currentDepth;
        bool _265 = depthDiff > 0.0;
        bool _272;
        if (_265)
        {
            _272 = depthDiff < _96.thickness;
        }
        else
        {
            _272 = _265;
        }
        if (_272)
        {
            float3 binarySearchStart = lastPos;
            float3 binarySearchEnd = currentPos;
            for (int j = 0; j < 8; j++)
            {
                float3 midPoint = (binarySearchStart + binarySearchEnd) * 0.5;
                float4 midProj = _42.projection * float4(midPoint, 1.0);
                float _303 = midProj.w;
                float4 _304 = midProj;
                float3 _307 = _304.xyz / float3(_303);
                midProj.x = _307.x;
                midProj.y = _307.y;
                midProj.z = _307.z;
                float2 midUV = (midProj.xy * 0.5) + float2(0.5);
                bool _321 = midUV.x < 0.0;
                bool _328;
                if (!_321)
                {
                    _328 = midUV.x > 1.0;
                }
                else
                {
                    _328 = _321;
                }
                bool _335;
                if (!_328)
                {
                    _335 = midUV.y < 0.0;
                }
                else
                {
                    _335 = _328;
                }
                bool _342;
                if (!_335)
                {
                    _342 = midUV.y > 1.0;
                }
                else
                {
                    _342 = _335;
                }
                if (_342)
                {
                    break;
                }
                float midRawDepth = gDepth.sample(gDepthSmplr, midUV).x;
                if (midRawDepth >= 0.99989998340606689453125)
                {
                    binarySearchStart = midPoint;
                    continue;
                }
                float3 midSampleWorldPos = gPositio)",
R"(n.sample(gPositionSmplr, midUV).xyz;
                float3 midSampleViewPos = (_42.view * float4(midSampleWorldPos, 1.0)).xyz;
                float midSampleDepth = -midSampleViewPos.z;
                float midCurrentDepth = -midPoint.z;
                if (midCurrentDepth < midSampleDepth)
                {
                    binarySearchStart = midPoint;
                }
                else
                {
                    binarySearchEnd = midPoint;
                }
            }
            float4 finalProj = _42.projection * float4(binarySearchEnd, 1.0);
            float _364 = finalProj.w;
            float4 _365 = finalProj;
            float3 _368 = _365.xyz / float3(_364);
            finalProj.x = _368.x;
            finalProj.y = _368.y;
            finalProj.z = _368.z;
            hitUV = (finalProj.xy * 0.5) + float2(0.5);
            hitDepth = length(currentPos - rayOrigin);
            hit = true;
            break;
        }
        lastPos = currentPos;
    }
    if (!hit)
    {
        return float4(0.0);
    }
    float3 hitColor = sceneColor.sample(sceneColorSmplr, hitUV).xyz;
    float tintStrength = metallic * 0.3499999940395355224609375;
    float3 metalTint = mix(float3(1.0), albedo, float3(tintStrength));
    hitColor *= metalTint;
    float hitDepthRaw = gDepth.sample(gDepthSmplr, hitUV).x;
    float3 hitNormalSample = gNormal.sample(gNormalSmplr, hitUV).xyz;
    float hitNormalLenSq = dot(hitNormalSample, hitNormalSample);
    float3 hitNormalDir = (hitNormalLenSq > 9.9999997473787516355514526367188e-06) ? (hitNormalSample * rsqrt(hitNormalLenSq)) : (-rayDir);
    float geometryValidity = 1.0 - smoothstep(0.99800002574920654296875, 1.0, hitDepthRaw);
    geometryValidity *= smoothstep(0.00999999977648258209228515625, 0.100000001490116119384765625, hitNormalLenSq);
    float normalFacing = fast::max(dot(hitNormalDir, -rayDir), 0.0);
    geometryValidity *= smoothstep(0.0500000007450580596923828125, 0.25, normalFacing);
    float hitLuma = dot(hitColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    float skyLuma = dot(skyReflection, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    float luminanceValidity = smoothstep(0.00200000009499490261077880859375, 0.02999999932944774627685546875, hitLuma + (skyLuma * 0.100000001490116119384765625));
    float2 edgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), hitUV) * (float2(1.0) - smoothstep(float2(0.85000002384185791015625), float2(1.0), hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    float distanceFade = 1.0 - smoothstep(_96.maxDistance * 0.5, _96.maxDistance, hitDepth);
    float roughnessFade = 1.0 - smoothstep(0.0, _96.maxRoughness, roughness);
    float hitConfidence = (edgeFactor * distanceFade) * roughnessFade;
    hitConfidence *= geometryValidity * luminanceValidity;
    float3 F0 = mix(float3(0.039999999105930328369140625), albedo, float3(metallic));
    float3 V = fast::normalize(viewDir);
    float cosTheta = fast::max(dot(normal, V), 0.0);
    float param = cosTheta;
    float3 param_1 = F0;
    float3 fresnel = fresnelSchlick(param, param_1);
    float fresnelFactor = ((fresnel.x + fresnel.y) + fresnel.z) / 3.0;
    float fresnelWeight = mix(fresnelFactor, 1.0, mirrorFactor);
    float3 reflectionColor = mix(skyReflection, hitColor, float3(hitConfidence));
    float finalFade = mix(hitConfidence * fresnelWeight, 1.0, mirrorFactor);
    finalFade = fast::clamp(finalFade, 0.0, 1.0);
    return float4(reflectionColor, finalFade);
}

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _42 [[buffer(0)]], constant SSRParameters& _96 [[buffer(1)]], texture2d<float> gPosition [[texture(0)]], texture2d<float> sceneColor [[texture(1)]], texture2d<float> gNormal [[texture(2)]], texture2d<float> gAlbedoSpec [[texture(3)]], texture2d<float> gMaterial [[texture(4)]], texture2d<float> gDepth [[texture(5)]], texturecube<float> skybox [[texture(6)]], texture2d<float> historyTexture [[texture(7)]], sampler gPositionSmplr [[sampler(0)]], sampler sceneColorSmplr [[sampler(1)]], sampler gNormalSmplr [[sampler(2)]], sampler gAlbedoSpecSmplr [[sampler(3)]], sampler gMaterialSmplr [[sampler(4)]], sampler gDepthSmplr [[sampler(5)]], sampler skyboxSmplr [[sampler(6)]], sampler historyTextureSmplr [[sampler(7)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float3 worldPos = gPosition.sample(gPositionSmplr, in.TexCoord).xyz;
    float3 normal = fast::normalize(gNormal.sample(gNormalSmplr, in.TexCoord).xyz);
    float3 albedo = gAlbedoSpec.sample(gAlbedoSpecSmplr, in.TexCoord).xyz;
    float4 material = gMaterial.sample(gMaterialSmplr, in.TexCoord);
    float metallic = material.x;
    float roughness = material.y;
    float reflectivity = material.w;
    if (length(normal) < 0.001000000047497451305389404296875)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    if (roughness > _96.maxRoughness || fast::max(metallic, reflectivity) < 0.02)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    float3 viewDir = fast::normalize(_42.cameraPosition - worldPos);
    float3 param = worldPos;
    float3 param_1 = normal;
    float3 param_2 = viewDir;
    float param_3 = roughness;
    float param_4 = metallic;
    float param_5 = reflectivity;
    float3 param_6 = albedo;
    float4 reflection = SSR(param, param_1, param_2, param_3, param_4, param_5, param_6, _42, _96, gl_FragCoord, gPosition, gPositionSmplr, sceneColor, sceneColorSmplr, gNormal, gNormalSmplr, gDepth, gDepthSmplr, skybox, skyboxSmplr);
    if (_96.debugMode != 0)
    {
        out.FragColor = float4(1.0 - reflection.w, reflection.w, 0.0, 1.0);
        return out;
    }
    if (reflection.w > 0.0 && _96.historyWeight > 0.0)
    {
        float4 history = historyTexture.sample(historyTextureSmplr, in.TexCoord);
        float validHistory = step(0.001, history.w);
        reflection = mix(reflection, history, _96.historyWeight * validHistory * reflection.w);
    }
    out.FragColor = reflection;
    return out;
}
)",
};
static const AtlasPackedShaderSource SSR_FRAG = {SSR_FRAG_PARTS, 2};

static const char* const TERRAIN_CONTROL_TESC_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TextureCoord;
    float4 gl_Position;
};

struct main0_in
{
    float2 TexCoord [[attribute(0)]];
    float4 gl_Position [[attribute(1)]];
};

kernel void main0(main0_in in [[stage_in]], constant UBO& _56 [[buffer(0)]], uint gl_InvocationID [[thread_index_in_threadgroup]], uint gl_PrimitiveID [[threadgroup_position_in_grid]], device main0_out* spvOut [[buffer(28)]], constant uint* spvIndirectParams [[buffer(29)]], device MTLQuadTessellationFactorsHalf* spvTessLevel [[buffer(26)]], threadgroup main0_in* gl_in [[threadgroup(0)]])
{
    device main0_out* gl_out = &spvOut[gl_PrimitiveID * 4];
    if (gl_InvocationID < spvIndirectParams[0])
        gl_in[gl_InvocationID] = in;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (gl_InvocationID >= 4)
        return;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_out[gl_InvocationID].TextureCoord = gl_in[gl_InvocationID].TexCoord;
    if (gl_InvocationID == 0)
    {
        float4 eyeSpacePos00 = (_56.view * _56.model) * gl_in[0].gl_Position;
        float4 eyeSpacePos01 = (_56.view * _56.model) * gl_in[1].gl_Position;
        float4 eyeSpacePos10 = (_56.view * _56.model) * gl_in[2].gl_Position;
        float4 eyeSpacePos11 = (_56.view * _56.model) * gl_in[3].gl_Position;
        float dist00 = fast::clamp((abs(eyeSpacePos00.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist01 = fast::clamp((abs(eyeSpacePos01.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist10 = fast::clamp((abs(eyeSpacePos10.z) - 20.0) / 780.0, 0.0, 1.0);
        float dist11 = fast::clamp((abs(eyeSpacePos11.z) - 20.0) / 780.0, 0.0, 1.0);
        float tessLevel0 = mix(64.0, 4.0, fast::min(dist10, dist00));
        float tessLevel1 = mix(64.0, 4.0, fast::min(dist00, dist01));
        float tessLevel2 = mix(64.0, 4.0, fast::min(dist01, dist11));
        float tessLevel3 = mix(64.0, 4.0, fast::min(dist11, dist10));
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[0] = half(tessLevel0);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[1] = half(tessLevel1);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[2] = half(tessLevel2);
        spvTessLevel[gl_PrimitiveID].edgeTessellationFactor[3] = half(tessLevel3);
        spvTessLevel[gl_PrimitiveID].insideTessellationFactor[0] = half(fast::max(tessLevel1, tessLevel3));
        spvTessLevel[gl_PrimitiveID].insideTessellationFactor[1] = half(fast::max(tessLevel0, tessLevel2));
    }
}

)",
};
static const AtlasPackedShaderSource TERRAIN_CONTROL_TESC = {TERRAIN_CONTROL_TESC_PARTS, 1};

static const char* const TERRAIN_EVAL_TESE_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 lightViewProj;
    float maxPeak;
    float seaLevel;
    uint isFromMap;
};

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float3 FragPos [[user(locn1)]];
    float Height [[user(locn2)]];
    float4 FragPosLightSpace [[user(locn3)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float2 TextureCoord [[attribute(0)]];
    float4 gl_Position [[attribute(1)]];
};

struct main0_patchIn
{
    patch_control_point<main0_in> gl_in;
};

[[ patch(quad, 0) ]] vertex main0_out main0(main0_patchIn patchIn [[stage_in]], constant Uniforms& _84 [[buffer(0)]], constant UBO& _145 [[buffer(1)]], texture2d<float> heightMap [[texture(0)]], sampler heightMapSmplr [[sampler(0)]], float2 gl_TessCoordIn [[position_in_patch]])
{
    main0_out out = {};
    float3 gl_TessCoord = float3(gl_TessCoordIn.x, gl_TessCoordIn.y, 0.0);
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float2 t00 = patchIn.gl_in[0].TextureCoord;
    float2 t01 = patchIn.gl_in[1].TextureCoord;
    float2 t10 = patchIn.gl_in[2].TextureCoord;
    float2 t11 = patchIn.gl_in[3].TextureCoord;
    float2 t0 = ((t01 - t00) * u) + t00;
    float2 t1 = ((t11 - t10) * u) + t10;
    float2 texCoord = ((t1 - t0) * v) + t0;
    out.Height = (heightMap.sample(heightMapSmplr, texCoord, level(0.0)).x * _84.maxPeak) - _84.seaLevel;
    float4 p00 = patchIn.gl_in[0].gl_Position;
    float4 p01 = patchIn.gl_in[1].gl_Position;
    float4 p10 = patchIn.gl_in[2].gl_Position;
    float4 p11 = patchIn.gl_in[3].gl_Position;
    float4 p0 = ((p01 - p00) * u) + p00;
    float4 p1 = ((p11 - p10) * u) + p10;
    float4 position = ((p1 - p0) * v) + p0;
    position.y += out.Height;
    out.gl_Position = ((_145.projection * _145.view) * _145.model) * position;
    out.TexCoord = texCoord;
    out.FragPos = float3((_145.model * position).xyz);
    out.FragPosLightSpace = (_84.lightViewProj * _145.model) * position;
    return out;
}

)",
};
static const AtlasPackedShaderSource TERRAIN_EVAL_TESE = {TERRAIN_EVAL_TESE_PARTS, 1};

static const char* const TERRAIN_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct PushConstants
{
    uint isFromMap;
    float4 directionalColor;
    float directionalIntensity;
    uint hasLight;
    uint useShadowMap;
    float3 lightDir;
    packed_float3 viewDir;
    float ambientStrength;
    float shadowBias;
    int biomesCount;
    float diffuseStrength;
    float specularStrength;
};

struct TerrainParameters
{
    float maxPeak;
    float seaLevel;
};

struct Biome
{
    int id;
    float4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

struct Biome_1
{
    int id;
    float4 tintColor;
    int useTexture;
    int textureId;
    float minHeight;
    float maxHeight;
    float minMoisture;
    float maxMoisture;
    float minTemperature;
    float maxTemperature;
};

struct BiomeBuffer
{
    Biome_1 biomes[1];
};

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float3 FragPos [[user(locn1)]];
    float Height [[user(locn2)]];
    float4 FragPosLightSpace [[user(locn3)]];
};

static inline __attribute__((always_inline))
float3 calculateNormal(texture2d<float> heightMap, sampler heightMapSmplr, thread const float2& texCoord, thread const float& heightScale)
{
    float h = heightMap.sample(heightMapSmplr, texCoord).x * heightScale;
    float dx = dfdx(h);
    float dy = dfdy(h);
    float3 n = fast::normalize(float3(-dx, 1.0, -dy));
    return n;
}

static inline __attribute__((always_inline))
float smoothStepRange(thread const float& value, thread const float& minV, thread const float& maxV)
{
    return smoothstep(minV, maxV, value);
}

static inline __attribute__((always_inline))
float4 sampleBiomeTexture(thread const int& id, thread const float2& uv, texture2d<float> texture0, sampler texture0Smplr, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr)
{
    if (id == 0)
    {
        return texture0.sample(texture0Smplr, uv);
    }
    if (id == 1)
    {
        return texture1.sample(texture1Smplr, uv);
    }
    if (id == 2)
    {
        return texture2.sample(texture2Smplr, uv);
    }
    if (id == 3)
    {
        return texture3.sample(texture3Smplr, uv);
    }
    if (id == 4)
    {
        return texture4.sample(texture4Smplr, uv);
    }
    if (id == 5)
    {
        return texture5.sample(texture5Smplr, uv);
    }
    if (id == 6)
    {
        return texture6.sample(texture6Smplr, uv);
    }
    if (id == 7)
    {
        return texture7.sample(texture7Smplr, uv);
    }
    if (id == 8)
    {
        return texture8.sample(texture8Smplr, uv);
    }
    if (id == 9)
    {
        return texture9.sample(texture9Smplr, uv);
    }
    if (id == 10)
    {
        return texture10.sample(texture10Smplr, uv);
    }
    if (id == 11)
    {
        return texture11.sample(texture11Smplr, uv);
    }
    return float4(1.0, 0.0, 1.0, 1.0);
}

static inline __attribute__((always_inline))
float4 triplanarBlend(thread const int& idx, thread const float3& normal, thread const float3& worldPos, thread const float& scale, texture2d<float> texture0, sampler texture0Smplr, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr)
{
    float3 blend = abs(normal);
    blend = (blend - float3(0.20000000298023223876953125)) * 7.0;
    blend = fast::clamp(blend, float3(0.0), float3(1.0));
    blend /= float3((blend.x + blend.y) + blend.z);
    int param = idx;
    float2 param_1 = worldPos.yz * scale;
    float4 xProj = sampleBiomeTexture(param, param_1, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    int param_2 = idx;
    float2 param_3 = worldPos.xz * scale;
    float4 yProj = sampleBiomeTexture(param_2, param_3, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    int param_4 = idx;
    float2 param_5 = worldPos.xy * scale;
    float4 zProj = sampleBiomeTexture(param_4, param_5, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
    return ((xProj * blend.x) + (yProj * blend.y)) + (zProj * blend.z);
}

static inline __attribute__((always_inline))
float calculateShadow(thread const float4& fragPosLightSpace, thread const float3& normal, constant PushConstants& _331, texture2d<float> shadowMap, sampler shadowMapSmplr)
{
    float3 projCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    projCoords = (projCoords * 0.5) + float3(0.5);
    bool _293 = projCoords.z > 1.0;
    bool _300;
    if (!_293)
    {
        _300 = projCoords.x < 0.0;
    }
    else
    {
        _300 = _293;
    }
    bool _307;
    if (!_300)
    {
        _307 = projCoords.x > 1.0;
    }
    else
    {
        _307 = _300;
    }
    bool _314;
    if (!_307)
    {
        _314 = projCoords.y < 0.0;
    }
    else
    {
        _314 = _307;
    }
    bool _321;
    if (!_314)
    {
        _321 = projCoords.y > 1.0;
    }
    else
    {
        _321 = _314;
    }
    if (_321)
    {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    float bias0 = fast::max(_331.shadowBias * (1.0 - dot(normal, _331.lightDir)), _331.shadowBias * 0.100000001490116119384765625);
    float shadow = 0.0;
    float2 texelSize = float2(1.0) / float2(int2(shadowMap.get_width(), shadowMap.get_height()));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = shadowMap.sample(shadowMapSmplr, (projCoords.xy + (float2(float(x), float(y)) * texelSize))).x;
            shadow += float((currentDepth - bias0) > pcfDepth);
        }
    }
    shadow /= 9.0;
    return shadow;
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread const float3& color)
{
    return fast::clamp((color * ((color * 2.5099999904632568359375) + float3(0.02999999932944774627685546875))) / ((color * ((color * 2.4300000667572021484375) + float3(0.589999973773956298828125))) + float3(0.14000000059604644775390625)), float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant PushConstants& _331 [[buffer(0)]], constant TerrainParameters& _444 [[buffer(1)]], device BiomeBuffer& _534 [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], texture2d<float> texture1 [[texture(1)]], texture2d<float> texture2 [[texture(2)]], texture2d<float> texture3 [[tex)",
R"(ture(3)]], texture2d<float> texture4 [[texture(4)]], texture2d<float> texture5 [[texture(5)]], texture2d<float> texture6 [[texture(6)]], texture2d<float> texture7 [[texture(7)]], texture2d<float> texture8 [[texture(8)]], texture2d<float> texture9 [[texture(9)]], texture2d<float> texture10 [[texture(10)]], texture2d<float> texture11 [[texture(11)]], texture2d<float> shadowMap [[texture(12)]], texture2d<float> heightMap [[texture(13)]], texture2d<float> moistureMap [[texture(14)]], texture2d<float> temperatureMap [[texture(15)]], sampler texture0Smplr [[sampler(0)]], sampler texture1Smplr [[sampler(1)]], sampler texture2Smplr [[sampler(2)]], sampler texture3Smplr [[sampler(3)]], sampler texture4Smplr [[sampler(4)]], sampler texture5Smplr [[sampler(5)]], sampler texture6Smplr [[sampler(6)]], sampler texture7Smplr [[sampler(7)]], sampler texture8Smplr [[sampler(8)]], sampler texture9Smplr [[sampler(9)]], sampler texture10Smplr [[sampler(10)]], sampler texture11Smplr [[sampler(11)]], sampler shadowMapSmplr [[sampler(12)]], sampler heightMapSmplr [[sampler(13)]], sampler moistureMapSmplr [[sampler(14)]], sampler temperatureMapSmplr [[sampler(15)]])
{
    main0_out out = {};
    if (_331.biomesCount <= 0)
    {
        out.FragColor = float4(float3((in.Height + _444.seaLevel) / _444.maxPeak), 1.0);
        return out;
    }
    float _463;
    if (_331.isFromMap != 0u)
    {
        _463 = heightMap.sample(heightMapSmplr, in.TexCoord).x * 255.0;
    }
    else
    {
        _463 = ((in.Height + _444.seaLevel) / _444.maxPeak) * 255.0;
    }
    float h = _463;
    float m = moistureMap.sample(moistureMapSmplr, in.TexCoord).x * 255.0;
    float t = temperatureMap.sample(temperatureMapSmplr, in.TexCoord).x * 255.0;
    float texelSize = 1.0 / float(int2(heightMap.get_width(), heightMap.get_height()).x);
    float heightScale = 64.0;
    float2 param = in.TexCoord;
    float param_1 = heightScale;
    float3 normal = calculateNormal(heightMap, heightMapSmplr, param, param_1);
    float4 baseColor = float4(0.0);
    float totalWeight = 0.0;
    float _550;
    float _574;
    float _598;
    float4 _628;
    for (int i = 0; i < _331.biomesCount; i++)
    {
        Biome _539;
        _539.id = _534.biomes[i].id;
        _539.tintColor = _534.biomes[i].tintColor;
        _539.useTexture = _534.biomes[i].useTexture;
        _539.textureId = _534.biomes[i].textureId;
        _539.minHeight = _534.biomes[i].minHeight;
        _539.maxHeight = _534.biomes[i].maxHeight;
        _539.minMoisture = _534.biomes[i].minMoisture;
        _539.maxMoisture = _534.biomes[i].maxMoisture;
        _539.minTemperature = _534.biomes[i].minTemperature;
        _539.maxTemperature = _534.biomes[i].maxTemperature;
        Biome b = _539;
        bool _543 = b.minHeight < 0.0;
        bool _549;
        if (_543)
        {
            _549 = b.maxHeight < 0.0;
        }
        else
        {
            _549 = _543;
        }
        if (_549)
        {
            _550 = 1.0;
        }
        else
        {
            float param_2 = h;
            float param_3 = b.minHeight;
            float param_4 = b.maxHeight;
            _550 = smoothStepRange(param_2, param_3, param_4);
        }
        float hW = _550;
        bool _567 = b.minMoisture < 0.0;
        bool _573;
        if (_567)
        {
            _573 = b.maxMoisture < 0.0;
        }
        else
        {
            _573 = _567;
        }
        if (_573)
        {
            _574 = 1.0;
        }
        else
        {
            float param_5 = m;
            float param_6 = b.minMoisture;
            float param_7 = b.maxMoisture;
            _574 = smoothStepRange(param_5, param_6, param_7);
        }
        float mW = _574;
        bool _591 = b.minTemperature < 0.0;
        bool _597;
        if (_591)
        {
            _597 = b.maxTemperature < 0.0;
        }
        else
        {
            _597 = _591;
        }
        if (_597)
        {
            _598 = 1.0;
        }
        else
        {
            float param_8 = t;
            float param_9 = b.minTemperature;
            float param_10 = b.maxTemperature;
            _598 = smoothStepRange(param_8, param_9, param_10);
        }
        float tW = _598;
        float weight = ((1.0 - hW) * mW) * tW;
        if (weight > 0.001000000047497451305389404296875)
        {
            if (b.useTexture == 1)
            {
                int param_11 = i;
                float3 param_12 = normal;
                float3 param_13 = in.FragPos;
                float param_14 = 0.100000001490116119384765625;
                _628 = triplanarBlend(param_11, param_12, param_13, param_14, texture0, texture0Smplr, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr);
            }
            else
            {
                _628 = b.tintColor;
            }
            float4 texColor = _628;
            baseColor += (texColor * weight);
            totalWeight += weight;
        }
    }
    baseColor /= float4(fast::max(totalWeight, 0.001000000047497451305389404296875));
    float detail = (heightMap.sample(heightMapSmplr, (in.TexCoord * 64.0)).x * 0.100000001490116119384765625) + 0.89999997615814208984375;
    float4 _670 = baseColor;
    float3 _672 = _670.xyz * detail;
    baseColor.x = _672.x;
    baseColor.y = _672.y;
    baseColor.z = _672.z;
    float3 finalColor;
    if (_331.hasLight != 0u)
    {
        float3 L = fast::normalize(-_331.lightDir);
        float3 N = fast::normalize(normal);
        float3 V = fast::normalize(float3(_331.viewDir));
        float3 ambient = baseColor.xyz * _331.ambientStrength;
        float diff = fast::max(dot(N, L), 0.0);
        float3 diffuse = ((_331.directionalColor.xyz * (_331.diffuseStrength * diff)) * _331.directionalIntensity) * baseColor.xyz;
        float3 H = fast::normalize(L + V);
        float spec = powr(fast::max(dot(N, H), 0.0), 32.0);
        float3 specular = (_331.directionalColor.xyz * (_331.specularStrength * spec)) * _331.directionalIntensity;
        float shadow = 0.0;
        if (_331.useShadowMap != 0u)
        {
            float4 param_15 = in.FragPosLightSpace;
            float3 param_16 = N;
            shadow = calculateShadow(param_15, param_16, _331, shadowMap, shadowMapSmplr);
        }
        finalColor = ambient + ((diffuse + specular) * (1.0 - shadow));
    }
    else
    {
        finalColor = baseColor.xyz * _331.ambientStrength;
    }
    float3 param_17 = finalColor;
    out.FragColor = float4(acesToneMapping(param_17), 1.0);
    out.BrightColor = float4(0.0);
    return out;
}

)",
};
static const AtlasPackedShaderSource TERRAIN_FRAG = {TERRAIN_FRAG_PARTS, 2};

static const char* const TERRAIN_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoord [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = ((_19.projection * _19.view) * _19.model) * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    return out;
}

)",
};
static const AtlasPackedShaderSource TERRAIN_VERT = {TERRAIN_VERT_PARTS, 1};

static const char* const TEXTURE_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    uint useTexture;
    uint onlyTexture;
    int textureCount;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
    float4 outColor [[user(locn1)]];
};

static inline __attribute__((always_inline))
float4 calculateAllTextures(constant UBO& _28, texture2d<float> texture1, sampler texture1Smplr, thread float2& TexCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, texture2d<float> texture11, sampler texture11Smplr, texture2d<float> texture12, sampler texture12Smplr, texture2d<float> texture13, sampler texture13Smplr, texture2d<float> texture14, sampler texture14Smplr, texture2d<float> texture15, sampler texture15Smplr, texture2d<float> texture16, sampler texture16Smplr)
{
    float4 color = float4(0.0);
    for (int i = 0; i < _28.textureCount; i++)
    {
        if (i == 0)
        {
            color += texture1.sample(texture1Smplr, TexCoord);
        }
        else
        {
            if (i == 1)
            {
                color += texture2.sample(texture2Smplr, TexCoord);
            }
            else
            {
                if (i == 2)
                {
                    color += texture3.sample(texture3Smplr, TexCoord);
                }
                else
                {
                    if (i == 3)
                    {
                        color += texture4.sample(texture4Smplr, TexCoord);
                    }
                    else
                    {
                        if (i == 4)
                        {
                            color += texture5.sample(texture5Smplr, TexCoord);
                        }
                        else
                        {
                            if (i == 5)
                            {
                                color += texture6.sample(texture6Smplr, TexCoord);
                            }
                            else
                            {
                                if (i == 6)
                                {
                                    color += texture7.sample(texture7Smplr, TexCoord);
                                }
                                else
                                {
                                    if (i == 7)
                                    {
                                        color += texture8.sample(texture8Smplr, TexCoord);
                                    }
                                    else
                                    {
                                        if (i == 8)
                                        {
                                            color += texture9.sample(texture9Smplr, TexCoord);
                                        }
                                        else
                                        {
                                            if (i == 9)
                                            {
                                                color += texture10.sample(texture10Smplr, TexCoord);
                                            }
                                            else
                                            {
                                                if (i == 10)
                                                {
                                                    color += texture11.sample(texture11Smplr, TexCoord);
                                                }
                                                else
                                                {
                                                    if (i == 11)
                                                    {
                                                        color += texture12.sample(texture12Smplr, TexCoord);
                                                    }
                                                    else
                                                    {
                                                        if (i == 12)
                                                        {
                                                            color += texture13.sample(texture13Smplr, TexCoord);
                                                        }
                                                        else
                                                        {
                                                            if (i == 13)
                                                            {
                                                                color += texture14.sample(texture14Smplr, TexCoord);
                                                            }
                                                            else
                                                            {
                                                                if (i == 14)
                                                                {
                                                                    color += texture15.sample(texture15Smplr, TexCoord);
                                                                }
                                                                else
                                                                {
                                                                    if (i == 15)
                                                                    {
                                                                        color += texture16.sample(texture16Smplr, TexCoord);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    color /= float4(float(_28.textureCount));
    return color;
}

fragment main0_out main0(main0_in in [[stage_in]], constant UBO& _28 [[buffer(0)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], texture2d<float> texture11 [[texture(10)]], texture2d<float> texture12 [[texture(11)]], texture2d<float> texture13 [[texture(12)]], texture2d<float> texture14 [[texture(13)]], texture2d<float> texture15 [[texture(14)]], texture2d<float> texture16 [[texture(15)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], sampler texture11Smplr [[sampler(10)]], sampler texture12Smplr [[sampler(11)]], sampler texture13Smplr [[sampler(12)]], sampler texture14Smplr [[sampler(13)]], sampler texture15Smplr [[sampler(14)]], sampler texture16Smplr [[sampler(15)]])
{
 )",
R"(   main0_out out = {};
    if (_28.onlyTexture != 0u)
    {
        out.FragColor = calculateAllTextures(_28, texture1, texture1Smplr, in.TexCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr, texture12, texture12Smplr, texture13, texture13Smplr, texture14, texture14Smplr, texture15, texture15Smplr, texture16, texture16Smplr);
        return out;
    }
    if (_28.useTexture != 0u)
    {
        out.FragColor = calculateAllTextures(_28, texture1, texture1Smplr, in.TexCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, texture11, texture11Smplr, texture12, texture12Smplr, texture13, texture13Smplr, texture14, texture14Smplr, texture15, texture15Smplr, texture16, texture16Smplr) * in.outColor;
    }
    else
    {
        out.FragColor = in.outColor;
    }
    return out;
}

)",
};
static const AtlasPackedShaderSource TEXTURE_FRAG = {TEXTURE_FRAG_PARTS, 2};

static const char* const TEXTURE_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 outColor [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float2 aTexCoord [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& _13 [[buffer(0)]])
{
    main0_out out = {};
    float4x4 mvp = (_13.projection * _13.view) * _13.model;
    out.gl_Position = mvp * float4(in.aPos, 1.0);
    out.TexCoord = in.aTexCoord;
    out.outColor = in.aColor;
    return out;
}

)",
};
static const AtlasPackedShaderSource TEXTURE_VERT = {TEXTURE_VERT_PARTS, 1};

static const char* const TEXT_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct TextColor
{
    float3 textColor;
};

struct main0_out
{
    float4 color [[color(0)]];
};

struct main0_in
{
    float2 texCoords [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant TextColor& _30 [[buffer(0)]], texture2d<float> text [[texture(0)]], sampler textSmplr [[sampler(0)]])
{
    main0_out out = {};
    float4 sampled = float4(1.0, 1.0, 1.0, text.sample(textSmplr, in.texCoords).x);
    out.color = float4(_30.textColor, 1.0) * sampled;
    return out;
}

)",
};
static const AtlasPackedShaderSource TEXT_FRAG = {TEXT_FRAG_PARTS, 1};

static const char* const TEXT_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 projection;
};

struct main0_out
{
    float2 texCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 vertex0 [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant Uniforms& _19 [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = _19.projection * float4(in.vertex0.xy, 0.0, 1.0);
    out.texCoords = in.vertex0.zw;
    return out;
}

)",
};
static const AtlasPackedShaderSource TEXT_VERT = {TEXT_VERT_PARTS, 1};

static const char* const UPSAMPLE_FRAG_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Params
{
    float2 srcResolution;
    float filterRadius;
};

struct main0_out
{
    float4 upsample [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant Params& _13 [[buffer(0)]], texture2d<float> srcTexture [[texture(0)]], sampler srcTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 texelSize =
        float2(1.0) / float2(srcTexture.get_width(), srcTexture.get_height());
    float radius = clamp(_13.filterRadius, 1.0, 8.0);
    float x = radius * texelSize.x;
    float y = radius * texelSize.y;
    float2 texCoord = in.TexCoord;
    float2 step1 = float2(x, y);
    float2 step2 = step1 * 2.0;
    float3 upsampleColor = float3(0.0);
    float totalWeight = 0.0;

    float wCenter = 0.16;
    float wCardinal1 = 0.11;
    float wDiagonal1 = 0.075;
    float wCardinal2 = 0.04;
    float wDiagonal2 = 0.015;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord).xyz * wCenter;
    totalWeight += wCenter;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, step1.y)).xyz * wCardinal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, -step1.y)).xyz * wCardinal1;
    totalWeight += wCardinal1 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step1.x, -step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step1.x, -step1.y)).xyz * wDiagonal1;
    totalWeight += wDiagonal1 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, step2.y)).xyz * wCardinal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(0.0, -step2.y)).xyz * wCardinal2;
    totalWeight += wCardinal2 * 4.0;

    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(step2.x, -step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.sample(srcTextureSmplr, texCoord + float2(-step2.x, -step2.y)).xyz * wDiagonal2;
    totalWeight += wDiagonal2 * 4.0;

    upsampleColor /= float3(max(totalWeight, 1e-5));
    out.upsample = float4(upsampleColor, 1.0);
    return out;
}
)",
};
static const AtlasPackedShaderSource UPSAMPLE_FRAG = {UPSAMPLE_FRAG_PARTS, 1};

static const char* const VOLUMETRIC_FRAG_PARTS[] = {
R"(#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Sun
{
    float2 sunPos;
};

struct VolumetricParameters
{
    float density;
    float weight;
    float decay;
    float exposure;
};

struct DirectionalLight
{
    float3 color;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoords [[user(locn0)]];
};

static inline __attribute__((always_inline))
float3 computeVolumetricLighting(thread const float2& uv, constant Sun& _21, constant VolumetricParameters& _31, texture2d<float> sceneTexture, sampler sceneTextureSmplr, constant DirectionalLight& directionalLight)
{
    float3 color = float3(0.0);
    float2 deltaTexCoord = (_21.sunPos - uv) * _31.density;
    float2 coord = uv;
    float illuminationDecay = 1.0;
    for (int i = 0; i < 48; i++)
    {
        coord += deltaTexCoord;
        bool _59 = coord.x < 0.0;
        bool _66;
        if (!_59)
        {
            _66 = coord.x > 1.0;
        }
        else
        {
            _66 = _59;
        }
        bool _74;
        if (!_66)
        {
            _74 = coord.y < 0.0;
        }
        else
        {
            _74 = _66;
        }
        bool _81;
        if (!_74)
        {
            _81 = coord.y > 1.0;
        }
        else
        {
            _81 = _74;
        }
        if (_81)
        {
            break;
        }
        float3 sampled = sceneTexture.sample(sceneTextureSmplr, coord).xyz;
        float brightness = dot(sampled, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
        float3 atmosphere = directionalLight.color * 0.0199999995529651641845703125;
        if (brightness > 0.5)
        {
            atmosphere += (sampled * 0.5);
        }
        atmosphere *= (illuminationDecay * _31.weight);
        color += atmosphere;
        illuminationDecay *= _31.decay;
    }
    return color * 5.0;
}

fragment main0_out main0(main0_in in [[stage_in]], constant Sun& _21 [[buffer(0)]], constant VolumetricParameters& _31 [[buffer(1)]], constant DirectionalLight& directionalLight [[buffer(2)]], texture2d<float> sceneTexture [[texture(0)]], sampler sceneTextureSmplr [[sampler(0)]])
{
    main0_out out = {};
    float2 param = in.TexCoords;
    float3 rays = computeVolumetricLighting(param, _21, _31, sceneTexture, sceneTextureSmplr, directionalLight);
    out.FragColor = float4(rays, 1.0);
    return out;
}
)",
};
static const AtlasPackedShaderSource VOLUMETRIC_FRAG = {VOLUMETRIC_FRAG_PARTS, 1};

static const char* const VOLUMETRIC_VERT_PARTS[] = {
R"(#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 TexCoords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float2 aTexCoords [[attribute(2)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = float4(in.aPos, 1.0);
    out.TexCoords = in.aTexCoords;
    return out;
}
)",
};
static const AtlasPackedShaderSource VOLUMETRIC_VERT = {VOLUMETRIC_VERT_PARTS, 1};

#endif // ATLAS_GENERATED_SHADERS_H
