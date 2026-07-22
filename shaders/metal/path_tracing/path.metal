#include <metal_stdlib>
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

struct MeshData {
    uint vertexOffset;
    uint indexOffset;
    uint _pad0;
    uint _pad1;
};

struct VertexData {
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
    float _pad0;
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

float3 sampleSphereOffset(thread uint &rng, float radius) {
    float2 u = float2(rand(rng), rand(rng));
    float z = u.x * 2.0 - 1.0;
    float radial = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * M_PI_F * u.y;
    return float3(radial * cos(phi), radial * sin(phi), z) * radius;
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

constexpr sampler materialTexSampler(coord::normalized, address::repeat,
                                     filter::linear, mip_filter::linear);

#define PT_MATERIAL_TEXTURE_PARAMS                                             \
    texture2d<float> materialTexture0, texture2d<float> materialTexture1,      \
        texture2d<float> materialTexture2, texture2d<float> materialTexture3,  \
        texture2d<float> materialTexture4, texture2d<float> materialTexture5,  \
        texture2d<float> materialTexture6, texture2d<float> materialTexture7,  \
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
        texture2d<float> materialTexture45 [[texture(57)]],                    \
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
                            float3 localT, float3 localB, InstanceData inst,
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

float3 lambert(float3 albedo, float3 N, float3 L, float3 lightColor,
               float intensity) {
    float ndl = max(dot(N, L), 0.0);
    return albedo * lightColor * intensity * ndl;
}

bool isOccluded(intersector<triangle_data, instancing> isect,
                instance_acceleration_structure sceneAS, float3 P, float3 N,
                float3 L, float maxDistance) {
    float ndlAbs = abs(dot(N, L));
    float shadowBias = mix(0.003, 0.0008, ndlAbs);
    ray shadowRay;
    shadowRay.origin = P + N * shadowBias;
    shadowRay.direction = L;
    shadowRay.min_distance = shadowBias;
    shadowRay.max_distance = max(maxDistance - shadowBias, shadowBias + 1e-4);

    auto shadowHit = isect.intersect(shadowRay, sceneAS, 0xFF);
    return shadowHit.type != intersection_type::none;
}

bool isOccludedDirectionalLight(DirectionalLightData light, float3 P, float3 N,
                                thread uint &rng,
                                intersector<triangle_data, instancing> isect,
                                instance_acceleration_structure sceneAS) {
    float3 baseL = normalize(-light.direction);
    float3x3 basis = buildOrthonormalBasis(baseL);
    float sunRadius = 0.0025;
    float2 u = float2(rand(rng), rand(rng));
    float r = sunRadius * sqrt(u.x);
    float phi = 2.0 * M_PI_F * u.y;
    float3 jittered =
        baseL + basis[0] * (r * cos(phi)) + basis[1] * (r * sin(phi));
    float3 L = normalize(jittered);
    return isOccluded(isect, sceneAS, P, N, L, 1e30);
}

bool isOccludedPointLight(PointLight light, float3 P, float3 N,
                          thread uint &rng,
                          intersector<triangle_data, instancing> isect,
                          instance_acceleration_structure sceneAS) {
    float lightRadius = clamp(light.range * 0.006, 0.005, 0.04);
    float2 u = float2(rand(rng), rand(rng));
    float z = u.x * 2.0 - 1.0;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * M_PI_F * u.y;
    float3 sphereOffset =
        float3(r * cos(phi), r * sin(phi), z) * lightRadius;
    float3 sampledLightPos = light.position + sphereOffset;

    float3 toLight = sampledLightPos - P;
    float dist2 = max(dot(toLight, toLight), 0.001);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

bool isOccludedSpotLight(SpotLight light, float3 P, float3 N,
                         thread uint &rng,
                         intersector<triangle_data, instancing> isect,
                         instance_acceleration_structure sceneAS) {
    float lightRadius = clamp(light.range * 0.004, 0.004, 0.03);
    float2 u = float2(rand(rng), rand(rng));
    float z = u.x * 2.0 - 1.0;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * M_PI_F * u.y;
    float3 sphereOffset =
        float3(r * cos(phi), r * sin(phi), z) * lightRadius;
    float3 sampledLightPos = light.position + sphereOffset;

    float3 toLight = sampledLightPos - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
}

bool isOccludedAreaLight(AreaLight light, float3 P, float3 N,
                         thread uint &rng,
                         intersector<triangle_data, instancing> isect,
                         instance_acceleration_structure sceneAS) {
    float2 u = float2(rand(rng), rand(rng));
    float2 rect = (u * 2.0 - 1.0) * 0.25;
    float3 sampledLightPos = light.position + light.right * rect.x +
                             light.up * rect.y;

    float3 toLight = sampledLightPos - P;
    float dist2 = max(dot(toLight, toLight), 1e-4);
    float dist = sqrt(dist2);
    float3 L = toLight / dist;
    return isOccluded(isect, sceneAS, P, N, L, dist - 0.001);
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

// Full Cook-Torrance PBR for a single analytic light
float3 evalPBR(float3 albedo, float metallic, float roughness, float3 N,
               float3 V, float3 L, float3 lightColor, float intensity) {
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float clampedRoughness = clamp(roughness, 0.045, 1.0);
    float3 F0 = mix(float3(0.04), albedo, clamp(metallic, 0.0, 1.0));
    float3 F = F_Schlick(VdotH, F0);
    float D = D_GGX(NdotH, clampedRoughness);
    float G = G_Smith(NdotV, NdotL, clampedRoughness);

    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
    float3 kD = (1.0 - F) * (1.0 - clamp(metallic, 0.0, 1.0));
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
    float3 H = normalize(V + L);
    float NdotL = max(dot(N, -L), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float3 F0 = float3(pow((ior - 1.0) / (ior + 1.0), 2.0));
    float3 F = F_Schlick(VdotH, F0);
    float3 transmitTint = mix(float3(1.0), albedo, 0.1);
    float3 transmitFactor = (1.0 - F) * transmitTint;
    float D = D_GGX(max(dot(N, H), 0.0), roughness);
    return transmitFactor * lightColor * intensity * D * NdotL * 2.2 / M_PI_F;
}

// ---------------------------------------------------------------------------
// Direct lighting with full PBR (replaces old evalDirectLighting)
// ---------------------------------------------------------------------------

float3 evalDirectLightingPBR(intersector<triangle_data, instancing> isect,
                             instance_acceleration_structure sceneAS, float3 P,
                             float3 N, float3 V, float3 albedo, float metallic,
                             float roughness, float ior, float transmittance,
                             float sssStrength, float sssThickness,
                             thread uint &rng,
                             constant DirectionalLightData &dirLight,
                             constant SceneData &sceneData,
                             constant PointLight *pointLights,
                             constant SpotLight *spotLights,
                             constant AreaLight *areaLights) {
    float3 lighting = float3(0.0);
    float surfaceOpacity =
        clamp(1.0 - transmittance * (1.0 - metallic), 0.0, 1.0);

    // Directional
    if (sceneData.numDirectionalLights > 0) {
        float3 L = normalize(-dirLight.direction);
        float3 c = evalPBR(albedo, metallic, roughness, N, V, L, dirLight.color,
                           max(dirLight.intensity, 0.0));
        float3 s = evalSubsurface(albedo, N, V, L, dirLight.color,
                                  max(dirLight.intensity, 0.0), roughness,
                                  sssStrength, sssThickness);
        float3 t =
            evalTransmission(albedo, N, V, L, dirLight.color,
                             max(dirLight.intensity, 0.0), roughness, ior) *
            transmittance;
        if (!isOccludedDirectionalLight(dirLight, P, N, rng, isect, sceneAS)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    // Point lights
    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float lightRadius = clamp(pointLights[i].range * 0.006, 0.005, 0.04);
        float3 sampledPosition =
            pointLights[i].position + sampleSphereOffset(rng, lightRadius);
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float lightRange = max(pointLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(pointLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, N, V, L,
                           pointLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, pointLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, pointLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, N, L, dist - 0.001)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    // Spot lights
    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float lightRadius = clamp(spotLights[i].range * 0.004, 0.004, 0.03);
        float3 sampledPosition =
            spotLights[i].position + sampleSphereOffset(rng, lightRadius);
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
        float3 c = evalPBR(albedo, metallic, roughness, N, V, L,
                           spotLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, spotLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, spotLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, N, L, dist - 0.001)) {
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
            normalize(cross(areaLights[i].right, areaLights[i].up));
        float cosLight = areaLights[i].twoSided > 0.5
                             ? abs(dot(lightNorm, -L))
                             : max(dot(lightNorm, -L), 0.0);
        float area = 4.0 * areaLights[i].halfWidth * areaLights[i].halfHeight;
        float minDist = max(
            max(areaLights[i].halfWidth, areaLights[i].halfHeight) * 0.5, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float atten = (cosLight * area) / max(distSq, 1e-4);
        float intensity = max(areaLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, N, V, L,
                           areaLights[i].color, intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, areaLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 t = evalTransmission(albedo, N, V, L, areaLights[i].color,
                                    intensity, roughness, ior) *
                   transmittance;
        if (!isOccluded(isect, sceneAS, P, N, L, dist - 0.001)) {
            lighting += (c + s) * surfaceOpacity + t;
        }
    }

    return lighting;
}

// ---------------------------------------------------------------------------
// sampleRadiance — iterative path with GGX importance-sampled indirect bounces
// ---------------------------------------------------------------------------

float3 sampleRadiance(uint2 gid, uint sampleIndex, uint w,
                      intersector<triangle_data, instancing> isect,
                      instance_acceleration_structure sceneAS, ray primaryRay,
                      constant Material *materials, constant MeshData *meshData,
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

    for (uint depth = 0; depth <= bounceLimit; ++depth) {
        auto hit = isect.intersect(surfaceRay, sceneAS, 0xFF);
        Material mat{};
        MeshData mesh{};
        InstanceData inst{};
        float2 texUV = float2(0.0);
        float3 localN = float3(0.0, 1.0, 0.0);
        float3 localT = float3(1.0, 0.0, 0.0);
        float3 localB = float3(0.0, 0.0, 1.0);
        bool foundSurface = false;

        for (uint alphaStep = 0; alphaStep < 4; ++alphaStep) {
            if (hit.type == intersection_type::none) {
                break;
            }

            uint instanceIndex = hit.instance_id;
            uint primitiveIndex = hit.primitive_id;
            mat = materials[instanceIndex];
            mesh = meshData[instanceIndex];
            inst = instanceData[instanceIndex];

            uint i0 = indices[mesh.indexOffset + primitiveIndex * 3 + 0];
            uint i1 = indices[mesh.indexOffset + primitiveIndex * 3 + 1];
            uint i2 = indices[mesh.indexOffset + primitiveIndex * 3 + 2];
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

            float alpha = 1.0;
            if (mat.opacityTextureIndex >= 0 &&
                uint(mat.opacityTextureIndex) < sceneData.materialTextureCount) {
                alpha = clamp(sampleMaterialTexture(
                                  mat.opacityTextureIndex, texUV,
                                  PT_MATERIAL_TEXTURE_ARGS)
                                  .x,
                              0.0, 1.0);
            }
            if (alpha >= 0.1) {
                foundSurface = true;
                break;
            }

            surfaceRay.origin +=
                surfaceRay.direction * (hit.distance + 0.001);
            surfaceRay.min_distance = 0.0;
            hit = isect.intersect(surfaceRay, sceneAS, 0xFF);
        }

        if (!foundSurface) {
            radiance += throughput *
                        skyColor(surfaceRay.direction, 0.0, skybox, sceneData);
            break;
        }

        float3 shadingNormal = resolveShadingNormal(
            mat, texUV, localN, localT, localB, inst,
            sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        float3 P = surfaceRay.origin + surfaceRay.direction * hit.distance;
        float3 V = normalize(-surfaceRay.direction);
        bool frontFace = dot(shadingNormal, V) >= 0.0;
        float3 N = frontFace ? shadingNormal : -shadingNormal;

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
            primaryDepth = length(P - primaryRay.origin);
            primaryRoughness = roughness;
            primaryHitDistance = hit.distance;
            primaryObjectId = hit.instance_id;
        }

        float reflectivity = clamp(mat.reflectivity, 0.0, 1.0);
        float sssStrength =
            clamp(1.0 - mat.albedo.w, 0.0, 1.0) * (1.0 - metallic);
        float sssThickness = mix(0.25, 1.75, ao);
        float3 direct = evalDirectLightingPBR(
            isect, sceneAS, P, N, V, albedo, metallic, roughness, ior,
            transmittance, sssStrength, sssThickness, rng, dirLight, sceneData,
            pointLights, spotLights, areaLights);
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

        if (depth == bounceLimit) {
            break;
        }

        float3 baseF0 = mix(float3(0.04), albedo, metallic);
        float3 reflectedColor = mix(float3(1.0), albedo, metallic);
        float3 F0 = mix(baseF0, reflectedColor, reflectivity);
        float NdotV = max(dot(N, V), 1e-4);
        float3 viewFresnel = F_Schlick(NdotV, F0);
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
        float probabilitySum =
            max(specProb + transmitProb + diffuseProb, 1e-4);
        specProb /= probabilitySum;
        transmitProb /= probabilitySum;
        diffuseProb /= probabilitySum;

        float choice = rand(rng);
        float3 bounceWeight = float3(0.0);
        float3 nextDirection = N;
        bool transmissionEvent = false;
        float3x3 basis = buildOrthonormalBasis(N);

        if (choice < specProb && specProb > 1e-4) {
            float3 localH = sampleGGX(float2(rand(rng), rand(rng)), roughness);
            float3 H = normalizeOr(basis * localH, N);
            nextDirection = reflect(-V, H);
            if (dot(nextDirection, N) <= 0.0) {
                H = N;
                nextDirection = reflect(-V, N);
            }
            float NdotL = max(dot(N, nextDirection), 1e-4);
            float NdotH = max(dot(N, H), 1e-4);
            float VdotH = max(dot(V, H), 1e-4);
            float3 F = F_Schlick(VdotH, F0);
            float G = G_Smith(NdotV, NdotL, roughness);
            bounceWeight =
                F * G * VdotH /
                max(NdotV * NdotH * specProb, 1e-4);
        } else if (choice < specProb + transmitProb &&
                   transmitProb > 1e-4) {
            transmissionEvent = true;
            float eta = frontFace ? 1.0 / ior : ior;
            nextDirection = refract(-V, N, eta);
            if (dot(nextDirection, nextDirection) < 1e-8) {
                transmissionEvent = false;
                nextDirection = reflect(-V, N);
                bounceWeight = float3(1.0) / max(transmitProb, 1e-4);
            } else {
                float3 F =
                    F_Schlick(NdotV, float3(dielectricF0));
                float3 tint = mix(float3(1.0), albedo, 0.15);
                bounceWeight = (1.0 - F) * tint /
                               max(transmitProb, 1e-4);
            }
        } else {
            float3 localDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            nextDirection = normalizeOr(basis * localDirection, N);
            float3 kD = (1.0 - viewFresnel) * (1.0 - metallic);
            bounceWeight = kD * albedo / max(diffuseProb, 1e-4);
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

        surfaceRay.origin =
            P + (transmissionEvent ? -N : N) * 0.0015;
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
                  instance_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant MeshData *meshData [[buffer(3)]],
                  constant VertexData *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
                  constant SceneData &sceneData [[buffer(8)]],
                  constant PointLight *pointLights [[buffer(9)]],
                  constant SpotLight *spotLights [[buffer(10)]],
                  constant AreaLight *areaLights [[buffer(11)]],
                  PT_MATERIAL_TEXTURE_BINDINGS,
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

    intersector<triangle_data, instancing> isect;
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
            gid, s, w, isect, sceneAS, primaryRay, materials, meshData,
            vertices, indices, instanceData, dirLight, sceneData, pointLights,
            spotLights, areaLights, PT_MATERIAL_TEXTURE_ARGS, skybox,
            sampleAlbedo, sampleNormal, samplePosition, sampleDepth,
            sampleRoughness, sampleHitDistance, sampleObjectId);
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
    float4 currentGuide =
        float4(primaryNormal.xy, primaryDepth, objectIdValue);
    bool historyValid = frameIndex > 0 &&
                        abs(previousGuide.z - primaryDepth) <
                            max(0.05, primaryDepth * 0.02) &&
                        distance(previousGuide.xy, primaryNormal.xy) < 0.12 &&
                        abs(previousGuide.w - objectIdValue) < 0.5;
    if (frameIndex == 0)
        prevColor = float4(0, 0, 0, 1);
    if (!historyValid)
        prevColor = float4(color, 1.0);

    float historyLength = historyValid ? min(float(frameIndex), 255.0) : 0.0;
    float3 lower = min(prevColor.xyz, color) - float3(0.35);
    float3 upper = max(prevColor.xyz, color) + float3(0.35);
    float3 clippedHistory = clamp(prevColor.xyz, lower, upper);
    float3 accum = mix(color, clippedHistory,
                       historyLength / (historyLength + 1.0));
    accum = clampLuminance(accum, 256.0);

    constexpr float bloomThreshold = 1.0;
	constexpr float bloomKnee = 0.5;

	float brightness = luminance(accum);
	float soft = clamp(
    	brightness - bloomThreshold + bloomKnee,
    	0.0,
    	bloomKnee * 2.0
	);

	soft = soft * soft / max(bloomKnee * 4.0, 0.00001);

	float contribution =
    	max(brightness - bloomThreshold, soft) /
    	max(brightness, 0.00001);

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
