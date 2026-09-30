static const uint PHOTON_FEATURE_DIRECT_LIGHTING = 1u << 0;
static const uint PHOTON_FEATURE_SHADOWS = 1u << 1;
static const uint PHOTON_FEATURE_ENVIRONMENT_LIGHTING = 1u << 2;
static const uint PHOTON_FEATURE_EMISSIVE_LIGHTING = 1u << 3;
static const uint PHOTON_FEATURE_INDIRECT_LIGHTING = 1u << 4;
static const uint PHOTON_FEATURE_TRANSMISSION = 1u << 5;
static const uint PHOTON_FEATURE_DISPERSION = 1u << 6;
static const uint PHOTON_FEATURE_IRIDESCENCE = 1u << 7;
static const uint PHOTON_FEATURE_CAUSTICS = 1u << 8;
static const uint PHOTON_FEATURE_VOLUMES = 1u << 9;
static const uint PHOTON_FEATURE_SUBSURFACE = 1u << 10;
static const uint PHOTON_FEATURE_NORMAL_MAPS = 1u << 11;
static const uint PHOTON_FEATURE_MATERIAL_TEXTURES = 1u << 12;
static const uint PHOTON_FEATURE_ALPHA_TRANSPARENCY = 1u << 13;
static const uint CAUSTIC_BUCKET_COUNT = 1u << 17;
static const uint CAUSTIC_BUCKET_SAMPLES = 32u;
static const uint CAUSTIC_BUCKET_WORDS = 34u;
static const uint CAUSTIC_INVALID_BUCKET = 0xffffffffu;

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
    float3 emissiveColor;
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
    float2 textureScale;
    float2 textureOffset;
    float3 attenuationColor;
    float attenuationDistance;
    int isVolume;
    float volumeDensity;
    float3 volumeAbsorptionColor;
    float volumeAbsorptionStrength;
    float3 volumeScatteringColor;
    float volumeScatteringStrength;
    float volumeAnisotropy;
    float3 volumeEmissionColor;
    float volumeEmissionStrength;
    float _pad3;
    float iridescenceFactor;
    float iridescenceIor;
    float iridescenceThickness;
    float iridescenceAbbeNumber;
    float subsurfaceWeight;
    float subsurfaceScale;
    float subsurfaceAnisotropy;
    float _pad4;
    float3 subsurfaceColor;
    float _pad5;
    float3 subsurfaceRadius;
    float _pad6;
};

struct VertexData {
    float3 position;
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
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
    float3 position;
    float intensity;
    float3 color;
    float range;
};

struct SpotLight {
    float3 position;
    float intensity;
    float3 direction;
    float innerCos;
    float3 color;
    float outerCos;
    float range;
    float3 _pad0;
};

struct AreaLight {
    float3 position;
    float intensity;
    float3 right;
    float halfWidth;
    float3 up;
    float halfHeight;
    float3 color;
    float twoSided;
    float emissionCos;
    float3 _pad;
};

struct EmissiveTriangle {
    float4 p0;
    float4 p1;
    float4 p2;
    float4 normal;
    float3 emission;
    float area;
    float cdf;
    float selectionPdf;
    float2 _pad;
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

struct SurfaceHit {
    bool valid;
    bool frontFace;
    float distance;
    float3 position;
    float3 normal;
    float2 uv;
    uint primitive;
    uint objectId;
    Material material;
};

[[vk::binding(0, 3)]]
RaytracingAccelerationStructure sceneAS;
[[vk::binding(1, 0)]]
ConstantBuffer<CameraUniforms> cam;
[[vk::binding(7, 0)]]
ConstantBuffer<DirectionalLightData> dirLight;
[[vk::binding(8, 0)]]
ConstantBuffer<SceneData> sceneData;
[[vk::binding(17, 0)]]
ConstantBuffer<CausticSettings> caustics;
[[vk::binding(2, 1)]]
StructuredBuffer<Material> materials;
[[vk::binding(3, 1)]]
StructuredBuffer<uint> primitiveObjects;
[[vk::binding(4, 1)]]
StructuredBuffer<VertexData> vertices;
[[vk::binding(5, 1)]]
StructuredBuffer<uint> indices;
[[vk::binding(6, 1)]]
StructuredBuffer<InstanceData> instanceData;
[[vk::binding(9, 1)]]
StructuredBuffer<PointLight> pointLights;
[[vk::binding(10, 1)]]
StructuredBuffer<SpotLight> spotLights;
[[vk::binding(11, 1)]]
StructuredBuffer<AreaLight> areaLights;
[[vk::binding(13, 1)]]
StructuredBuffer<uint> blasPrimitiveOffsets;
[[vk::binding(14, 1)]]
StructuredBuffer<EmissiveTriangle> emissiveTriangles;
[[vk::binding(15, 1)]]
RWStructuredBuffer<CausticPhoton> photons;
[[vk::binding(16, 1)]]
RWStructuredBuffer<uint> photonSlots;
[[vk::binding(12, 1)]]
Sampler2D materialTextures[256];
[[vk::binding(0, 2)]]
RWTexture2D<float4> outTex;
[[vk::binding(1, 2)]]
Texture2D<float4> historyTex;
[[vk::binding(2, 2)]]
RWTexture2D<float4> brightTex;
[[vk::binding(3, 2)]]
RWTexture2D<float4> albedoRoughnessTex;
[[vk::binding(4, 2)]]
RWTexture2D<float4> normalDepthTex;
[[vk::binding(5, 2)]]
RWTexture2D<float4> motionObjectTex;
[[vk::binding(6, 2)]]
Texture2D<float4> historyMomentsTex;
[[vk::binding(7, 2)]]
Texture2D<float4> historyGuideTex;
[[vk::binding(8, 2)]]
RWTexture2D<float4> historyOutTex;
[[vk::binding(9, 2)]]
RWTexture2D<float4> historyGuideOutTex;
[[vk::binding(10, 2)]]
RWTexture2D<float4> historyMomentsOutTex;
[[vk::binding(61, 2)]]
Texture2D<float4> directReservoirSampleTex;
[[vk::binding(62, 2)]]
Texture2D<float4> directReservoirStatsTex;
[[vk::binding(63, 2)]]
RWTexture2D<float4> directReservoirSampleOutTex;
[[vk::binding(64, 2)]]
RWTexture2D<float4> directReservoirStatsOutTex;
[[vk::binding(60, 2)]]
SamplerCube skybox;

uint hashValue(uint value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

float randomValue(inout uint state) {
    state = hashValue(state);
    return float(state) * (1.0 / 4294967296.0);
}

float3 safeNormal(float3 value, float3 fallback) {
    float magnitude = dot(value, value);
    return magnitude > 1e-10 ? value * rsqrt(magnitude) : fallback;
}

float pow5(float value) {
    float squared = value * value;
    return squared * squared * value;
}

float luminance(float3 value) {
    return dot(value, float3(0.2126, 0.7152, 0.0722));
}

float2 encodeNormal(float3 normal) {
    normal /= max(abs(normal.x) + abs(normal.y) + abs(normal.z), 1e-6);
    float2 encoded = normal.xy;
    if (normal.z < 0.0) {
        encoded = (1.0 - abs(encoded.yx)) *
                  select(bool2(encoded.x >= 0.0, encoded.y >= 0.0), float2(1.0),
                         float2(-1.0));
    }
    return encoded * 0.5 + 0.5;
}

float distributionGGX(float normalHalf, float roughness) {
    float alpha = max(roughness * roughness, 1e-4);
    float alphaSquared = alpha * alpha;
    float denominator = normalHalf * normalHalf * (alphaSquared - 1.0) + 1.0;
    return alphaSquared / max(3.14159265359 * denominator * denominator, 1e-7);
}

float geometrySmith(float normalView, float normalLight, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float view = normalView / max(normalView * (1.0 - k) + k, 1e-6);
    float light = normalLight / max(normalLight * (1.0 - k) + k, 1e-6);
    return view * light;
}

float3 fresnelSchlick(float cosine, float3 baseReflectance) {
    return baseReflectance +
           (1.0 - baseReflectance) * pow5(1.0 - saturate(cosine));
}

float3 thinFilmReflectance(float cosine, float3 baseReflectance, float factor,
                           float filmIor, float thickness) {
    float filmCosine = sqrt(
        max(1.0 - (1.0 - cosine * cosine) / max(filmIor * filmIor, 1e-4), 0.0));
    float phase = 4.0 * 3.14159265359 * filmIor * thickness * filmCosine;
    float3 wavelengths = float3(650.0, 510.0, 440.0);
    float3 interference = 0.5 + 0.5 * cos(phase / wavelengths * 6.28318530718);
    float3 filmReflectance =
        saturate(baseReflectance * 0.35 + interference * 0.65);
    return lerp(baseReflectance, filmReflectance, saturate(factor));
}

float3 volumeTransmittance(Material material, float distance) {
    float3 absorption =
        max(material.volumeAbsorptionColor * material.volumeAbsorptionStrength,
            0.0);
    float3 scattering =
        max(material.volumeScatteringColor * material.volumeScatteringStrength,
            0.0);
    return exp(-(absorption + scattering) * max(material.volumeDensity, 0.0) *
               max(distance, 0.0));
}

float dispersionIor(float baseIor, float abbeNumber, uint channel) {
    if ((sceneData.featureFlags & PHOTON_FEATURE_DISPERSION) == 0 ||
        abbeNumber <= 1.0) {
        return max(baseIor, 1.0001);
    }
    float offset = channel == 0 ? -0.5 : channel == 2 ? 0.5 : 0.0;
    return max(baseIor + offset * max(baseIor - 1.0, 0.0) / abbeNumber, 1.0001);
}

float3 evaluateBRDF(SurfaceHit hit, float3 view, float3 light, float3 radiance,
                    float intensity) {
    float3 normal = hit.normal;
    float3 albedo = saturate(hit.material.albedo.xyz);
    float roughness = clamp(hit.material.roughness, 0.045, 1.0);
    float metallic = saturate(hit.material.metallic);
    float transmission = saturate(hit.material.transmittance);
    float ior = max(hit.material.ior, 1.0001);
    float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    float3 f0 = lerp(float3(max(hit.material.reflectivity, dielectricF0)),
                     albedo, metallic);
    float3 halfVector = safeNormal(view + light, normal);
    float normalLight = max(dot(normal, light), 0.0);
    float normalView = max(dot(normal, view), 1e-4);
    float normalHalf = max(dot(normal, halfVector), 0.0);
    float viewHalf = max(dot(view, halfVector), 0.0);
    float3 fresnel = fresnelSchlick(viewHalf, f0);
    if ((sceneData.featureFlags & PHOTON_FEATURE_IRIDESCENCE) != 0 &&
        hit.material.iridescenceFactor > 0.0) {
        fresnel = thinFilmReflectance(
            viewHalf, fresnel, hit.material.iridescenceFactor,
            max(hit.material.iridescenceIor, 1.01),
            max(hit.material.iridescenceThickness, 0.0));
    }
    float distribution = distributionGGX(normalHalf, roughness);
    float geometry = geometrySmith(normalView, normalLight, roughness);
    float3 specular = distribution * geometry * fresnel /
                      max(4.0 * normalView * normalLight, 1e-5);
    float3 diffuse = (1.0 - fresnel) * (1.0 - metallic) * (1.0 - transmission) *
                     albedo / 3.14159265359;
    if ((sceneData.featureFlags & PHOTON_FEATURE_SUBSURFACE) != 0 &&
        hit.material.subsurfaceWeight > 0.0) {
        float wrap = saturate((dot(-view, light) + 0.35) / 1.35);
        float radius = max(
            dot(hit.material.subsurfaceRadius, float3(0.2126, 0.7152, 0.0722)) *
                max(hit.material.subsurfaceScale, 0.0),
            1e-4);
        float falloff = exp(-1.0 / radius);
        float3 subsurface = saturate(hit.material.subsurfaceColor) * albedo *
                            wrap * falloff / 3.14159265359;
        diffuse =
            lerp(diffuse, subsurface, saturate(hit.material.subsurfaceWeight));
    }
    float3 result = (diffuse + specular) * radiance * intensity * normalLight;
    if (transmission > 0.0) {
        float backLight = max(dot(normal, -light), 0.0);
        float forward =
            pow(max(dot(-view, light), 0.0), lerp(96.0, 2.0, sqrt(roughness)));
        result += (1.0 - fresnel) * albedo * radiance * intensity *
                  transmission * backLight * forward;
    }
    return result;
}

float4 sampleMaterialTexture(int index, float2 uv) {
    if (index < 0 || uint(index) >= sceneData.materialTextureCount) {
        return 1.0;
    }

    return materialTextures[NonUniformResourceIndex(index)].SampleLevel(
        frac(uv), 0.0);
}

float3 cosineDirection(float3 normal, inout uint state) {
    float2 sample = float2(randomValue(state), randomValue(state));
    float radius = sqrt(sample.x);
    float angle = 6.28318530718 * sample.y;
    float3 tangent =
        safeNormal(abs(normal.x) > 0.1 ? cross(float3(0.0, 1.0, 0.0), normal)
                                       : cross(float3(1.0, 0.0, 0.0), normal),
                   float3(1.0, 0.0, 0.0));
    float3 bitangent = cross(normal, tangent);
    return safeNormal(tangent * (radius * cos(angle)) +
                          bitangent * (radius * sin(angle)) +
                          normal * sqrt(max(0.0, 1.0 - sample.x)),
                      normal);
}

uint causticCellKey(int3 cell) {
    uint3 value = asuint(cell);
    uint key = hashValue(value.x * 73856093u ^ value.y * 19349663u ^
                         value.z * 83492791u);
    return key == 0u ? 1u : key;
}

uint findCausticCell(int3 cell) {
    uint key = causticCellKey(cell);
    uint initial = hashValue(key ^ 0x9e3779b9u) & (CAUSTIC_BUCKET_COUNT - 1u);
    for (uint probe = 0u; probe < 128u; ++probe) {
        uint bucket = (initial + probe) & (CAUSTIC_BUCKET_COUNT - 1u);
        uint stored = photonSlots[bucket * CAUSTIC_BUCKET_WORDS];
        if (stored == 0u) {
            return CAUSTIC_INVALID_BUCKET;
        }
        if (stored == key) {
            return bucket;
        }
    }
    return CAUSTIC_INVALID_BUCKET;
}

bool insertCausticPhoton(int3 cell, uint photonId, inout uint state) {
    uint key = causticCellKey(cell);
    uint initial = hashValue(key ^ 0x9e3779b9u) & (CAUSTIC_BUCKET_COUNT - 1u);
    for (uint probe = 0u; probe < 128u; ++probe) {
        uint bucket = (initial + probe) & (CAUSTIC_BUCKET_COUNT - 1u);
        uint base = bucket * CAUSTIC_BUCKET_WORDS;
        uint stored = photonSlots[base];
        if (stored != key) {
            uint original;
            InterlockedCompareExchange(photonSlots[base], 0u, key, original);
            if (original != 0u && original != key) {
                continue;
            }
        }
        uint oldCount;
        InterlockedAdd(photonSlots[base + 1u], 1u, oldCount);
        uint slot = oldCount;
        if (slot >= CAUSTIC_BUCKET_SAMPLES) {
            uint candidate = hashValue(state ^ oldCount) % (oldCount + 1u);
            if (candidate >= CAUSTIC_BUCKET_SAMPLES) {
                return true;
            }
            slot = candidate;
        }
        photonSlots[base + 2u + slot] = photonId;
        return true;
    }
    return false;
}

float3 gatherCaustics(SurfaceHit hit) {
    if (sceneData.causticsEnabled == 0 || caustics.radius <= 0.0) {
        return 0.0;
    }
    float3 result = 0.0;
    float radiusSquared = caustics.radius * caustics.radius;
    int3 center = int3(floor(hit.position / caustics.radius));
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                int3 cell = center + int3(x, y, z);
                uint bucket = findCausticCell(cell);
                if (bucket == CAUSTIC_INVALID_BUCKET) {
                    continue;
                }
                uint base = bucket * CAUSTIC_BUCKET_WORDS;
                uint total = photonSlots[base + 1u];
                uint stored = min(total, CAUSTIC_BUCKET_SAMPLES);
                float reservoirWeight =
                    stored > 0u ? float(total) / float(stored) : 0.0;
                for (uint slot = 0u; slot < stored; ++slot) {
                    uint photonId = photonSlots[base + 2u + slot];
                    if (photonId == CAUSTIC_INVALID_BUCKET) {
                        continue;
                    }
                    CausticPhoton photon = photons[photonId];
                    if (uint(photon.incomingObject.w) != hit.objectId) {
                        continue;
                    }
                    float3 delta = photon.positionWavelength.xyz - hit.position;
                    float distanceSquared = dot(delta, delta);
                    float cosine = dot(hit.normal, photon.incomingObject.xyz);
                    if (distanceSquared >= radiusSquared || cosine <= 0.0 ||
                        dot(hit.normal, photon.normalPower.xyz) < 0.9) {
                        continue;
                    }
                    float kernel = 2.0 *
                                   (1.0 - distanceSquared / radiusSquared) /
                                   (3.14159265359 * radiusSquared);
                    uint channel = uint(photon.positionWavelength.w);
                    float3 spectral = channel == 0u   ? float3(3.0, 0.0, 0.0)
                                      : channel == 1u ? float3(0.0, 3.0, 0.0)
                                                      : float3(0.0, 0.0, 3.0);
                    result += spectral * photon.normalPower.w *
                              reservoirWeight * kernel * cosine *
                              hit.material.albedo.xyz / 3.14159265359;
                }
            }
        }
    }
    return result;
}

bool traceOcclusion(float3 origin, float3 direction, float maximumDistance) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maximumDistance;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE>
        query;
    query.TraceRayInline(sceneAS,
                         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
                             RAY_FLAG_FORCE_OPAQUE,
                         0xFF, ray);
    while (query.Proceed()) {
    }
    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

SurfaceHit traceSurface(float3 origin, float3 direction) {
    SurfaceHit hit = {};
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 1.0e30;
    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(sceneAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);
    while (query.Proceed()) {
    }
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT) {
        return hit;
    }
    uint geometry = query.CommittedGeometryIndex();
    uint primitive =
        blasPrimitiveOffsets[geometry] + query.CommittedPrimitiveIndex();
    uint indexBase = primitive * 3;
    uint i0 = indices[indexBase];
    uint i1 = indices[indexBase + 1];
    uint i2 = indices[indexBase + 2];
    float2 bary = query.CommittedTriangleBarycentrics();
    float3 weights = float3(1.0 - bary.x - bary.y, bary.x, bary.y);
    uint objectId = primitiveObjects[primitive];
    InstanceData transform = instanceData[objectId];
    float3 localNormal = vertices[i0].normal * weights.x +
                         vertices[i1].normal * weights.y +
                         vertices[i2].normal * weights.z;
    float3x3 normalMatrix =
        float3x3(transform.normalCol0.xyz, transform.normalCol1.xyz,
                 transform.normalCol2.xyz);
    hit.valid = true;
    hit.distance = query.CommittedRayT();
    hit.position = origin + direction * hit.distance;
    hit.normal =
        safeNormal(mul(normalMatrix, localNormal), float3(0.0, 1.0, 0.0));
    hit.frontFace = dot(hit.normal, direction) <= 0.0;
    if (!hit.frontFace) {
        hit.normal = -hit.normal;
    }
    hit.uv = vertices[i0].uv * weights.x + vertices[i1].uv * weights.y +
             vertices[i2].uv * weights.z;
    hit.primitive = primitive;
    hit.objectId = objectId;
    hit.material = materials[objectId];
    float2 materialUv =
        hit.uv * hit.material.textureScale + hit.material.textureOffset;
    if ((sceneData.featureFlags & PHOTON_FEATURE_MATERIAL_TEXTURES) != 0) {
        hit.material.albedo *=
            sampleMaterialTexture(hit.material.albedoTextureIndex, materialUv);
        float4 metallicSample = sampleMaterialTexture(
            hit.material.metallicTextureIndex, materialUv);
        float4 roughnessSample = sampleMaterialTexture(
            hit.material.roughnessTextureIndex, materialUv);
        float4 aoSample =
            sampleMaterialTexture(hit.material.aoTextureIndex, materialUv);
        if (hit.material.metallicTextureIndex >= 0) {
            hit.material.metallic *= hit.material.metallicTextureIndex ==
                                             hit.material.roughnessTextureIndex
                                         ? metallicSample.z
                                         : metallicSample.x;
        }
        if (hit.material.roughnessTextureIndex >= 0) {
            hit.material.roughness *= hit.material.roughnessTextureIndex ==
                                              hit.material.metallicTextureIndex
                                          ? roughnessSample.y
                                          : roughnessSample.x;
        }
        if (hit.material.aoTextureIndex >= 0) {
            hit.material.ao *= aoSample.x;
        }
        if (hit.material.opacityTextureIndex >= 0) {
            hit.material.albedo.w *=
                sampleMaterialTexture(hit.material.opacityTextureIndex,
                                      materialUv)
                    .x;
        }
    }
    if ((sceneData.featureFlags & PHOTON_FEATURE_NORMAL_MAPS) != 0 &&
        hit.material.useNormalMap != 0.0 &&
        hit.material.normalTextureIndex >= 0) {
        float3 tangent = safeNormal(vertices[i0].tangent * weights.x +
                                        vertices[i1].tangent * weights.y +
                                        vertices[i2].tangent * weights.z,
                                    float3(1.0, 0.0, 0.0));
        tangent = safeNormal(tangent - hit.normal * dot(hit.normal, tangent),
                             float3(1.0, 0.0, 0.0));
        float3 bitangent =
            safeNormal(cross(hit.normal, tangent), float3(0.0, 0.0, 1.0));
        float3 tangentNormal =
            sampleMaterialTexture(hit.material.normalTextureIndex, materialUv)
                    .xyz *
                2.0 -
            1.0;
        tangentNormal = safeNormal(tangentNormal, float3(0.0, 0.0, 1.0));
        hit.normal =
            safeNormal(tangent * tangentNormal.x + bitangent * tangentNormal.y +
                           hit.normal * tangentNormal.z,
                       hit.normal);
    }
    return hit;
}

float3 environmentColor(float3 direction) {
    if (sceneData.environmentEnabled != 0) {
        float3 sampled = skybox.SampleLevel(direction, 0.0).xyz;
        if (dot(sampled, sampled) > 1e-8) {
            return sampled * max(sceneData.ambientIntensity, 1.0);
        }
    }
    float skyAmount = saturate(direction.y * 0.5 + 0.5);
    float3 horizon = sceneData.ambientColor * sceneData.ambientIntensity;
    float3 zenith = max(sceneData.atmosphereSunColor *
                            sceneData.atmosphereSunIntensity * 0.025,
                        horizon);
    float3 sky = lerp(horizon, zenith, skyAmount);
    if (sceneData.atmosphereSkyEnabled != 0) {
        float sunAmount =
            pow(max(dot(direction, safeNormal(sceneData.atmosphereSunDirection,
                                              float3(0.0, 1.0, 0.0))),
                    0.0),
                max(sceneData.atmosphereSunSize, 0.001));
        sky += sceneData.atmosphereSunColor * sceneData.atmosphereSunIntensity *
               sunAmount;
    }
    if (sceneData.cloudsEnabled != 0 && direction.y > -0.1) {
        float2 cloudUv = direction.xz / max(direction.y + 0.35, 0.1);
        float2 cell = floor(cloudUv * sceneData.cloudSettings.x);
        float2 local = frac(cloudUv * sceneData.cloudSettings.x);
        float cloud = 0.0;
        float amplitude = 0.5;
        for (uint octave = 0; octave < 4; ++octave) {
            float2 lattice = cell + local;
            float2 hashInput = lattice + float(octave) * 17.0;
            float value =
                frac(sin(dot(hashInput, float2(127.1, 311.7))) * 43758.5453);
            cloud += value * amplitude;
            amplitude *= 0.5;
            cell *= 2.03;
            local = frac(local * 2.03);
        }
        cloud = smoothstep(sceneData.cloudSettings.y, sceneData.cloudSettings.z,
                           cloud);
        sky = lerp(sky, sceneData.cloudLighting.xyz,
                   cloud * sceneData.cloudLighting.w);
    }
    return sky;
}

float3 directLighting(SurfaceHit hit, float3 view) {
    if ((sceneData.featureFlags & PHOTON_FEATURE_DIRECT_LIGHTING) == 0) {
        return 0.0;
    }
    float3 result = 0.0;
    if (sceneData.numDirectionalLights > 0) {
        float3 lightDirection =
            safeNormal(-dirLight.direction, float3(0.0, 1.0, 0.0));
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002, lightDirection,
                           1.0e20)) {
            visibility = 0.0;
        }
        result += evaluateBRDF(hit, view, lightDirection, dirLight.color,
                               dirLight.intensity) *
                  visibility;
    }
    for (uint index = 0; index < sceneData.numPointLights; ++index) {
        float3 delta = pointLights[index].position - hit.position;
        float distanceSquared = max(dot(delta, delta), 1e-4);
        float distance = sqrt(distanceSquared);
        if (pointLights[index].range > 0.0 &&
            distance > pointLights[index].range) {
            continue;
        }
        float3 lightDirection = delta / distance;
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002, lightDirection,
                           distance - 0.004)) {
            visibility = 0.0;
        }
        float range = max(pointLights[index].range, 1e-4);
        float rangeFade = 1.0 - smoothstep(range * 0.75, range, distance);
        float intensity =
            pointLights[index].intensity * rangeFade / distanceSquared;
        result += evaluateBRDF(hit, view, lightDirection,
                               pointLights[index].color, intensity) *
                  visibility;
    }
    for (uint index = 0; index < sceneData.numSpotLights; ++index) {
        float3 delta = spotLights[index].position - hit.position;
        float distanceSquared = max(dot(delta, delta), 1e-4);
        float distance = sqrt(distanceSquared);
        if (spotLights[index].range > 0.0 &&
            distance > spotLights[index].range) {
            continue;
        }
        float3 lightDirection = delta / distance;
        float cone = smoothstep(
            spotLights[index].outerCos, spotLights[index].innerCos,
            dot(-lightDirection, safeNormal(spotLights[index].direction,
                                            float3(0.0, -1.0, 0.0))));
        float range = max(spotLights[index].range, 1e-4);
        float rangeFade = 1.0 - smoothstep(range * 0.75, range, distance);
        float intensity =
            spotLights[index].intensity * cone * rangeFade / distanceSquared;
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002, lightDirection,
                           distance - 0.004)) {
            visibility = 0.0;
        }
        result += evaluateBRDF(hit, view, lightDirection,
                               spotLights[index].color, intensity) *
                  visibility;
    }
    for (uint index = 0; index < sceneData.numAreaLights; ++index) {
        AreaLight light = areaLights[index];
        uint lightState = hashValue(
            asuint(hit.position.x) ^ (asuint(hit.position.y) << 7) ^
            (asuint(hit.position.z) >> 5) ^ sceneData.frameIndex ^ index);
        float2 sample =
            float2(randomValue(lightState), randomValue(lightState)) * 2.0 -
            1.0;
        float3 position = light.position +
                          light.right * sample.x * light.halfWidth +
                          light.up * sample.y * light.halfHeight;
        float3 delta = position - hit.position;
        float distanceSquared = max(dot(delta, delta), 1e-4);
        float distance = sqrt(distanceSquared);
        float3 lightDirection = delta / distance;
        float3 lightNormal =
            safeNormal(cross(light.right, light.up), float3(0.0, 1.0, 0.0));
        float lightCosine = light.twoSided > 0.5
                                ? abs(dot(lightNormal, -lightDirection))
                                : max(dot(lightNormal, -lightDirection), 0.0);
        if (lightCosine < light.emissionCos) {
            continue;
        }
        float area = max(4.0 * light.halfWidth * light.halfHeight, 1e-4);
        float intensity =
            light.intensity * lightCosine * area / distanceSquared;
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002, lightDirection,
                           distance - 0.004)) {
            visibility = 0.0;
        }
        result +=
            evaluateBRDF(hit, view, lightDirection, light.color, intensity) *
            visibility;
    }
    return result;
}

float3 samplePath(float3 origin, float3 direction, inout uint state,
                  out SurfaceHit primaryHit) {
    float3 radiance = 0.0;
    float3 throughput = 1.0;
    primaryHit = {};
    bool insideMedium = false;
    uint mediumObjectId = 0xffffffffu;
    Material mediumMaterial = {};
    uint bounceCount = max(sceneData.maxBounces, 1u);
    for (uint bounce = 0; bounce < bounceCount; ++bounce) {
        SurfaceHit hit = traceSurface(origin, direction);
        if (!hit.valid) {
            if ((sceneData.featureFlags &
                 PHOTON_FEATURE_ENVIRONMENT_LIGHTING) != 0) {
                radiance += throughput * environmentColor(direction);
            }
            break;
        }
        if (insideMedium &&
            (sceneData.featureFlags & PHOTON_FEATURE_VOLUMES) != 0) {
            float3 transmittance =
                volumeTransmittance(mediumMaterial, hit.distance);
            float3 emission = max(mediumMaterial.volumeEmissionColor, 0.0) *
                              max(mediumMaterial.volumeEmissionStrength, 0.0) *
                              (1.0 - transmittance);
            radiance += throughput * emission;
            throughput *= transmittance;
        }
        if (hit.material.isVolume != 0) {
            if ((sceneData.featureFlags & PHOTON_FEATURE_VOLUMES) == 0) {
                origin = hit.position + direction * 0.002;
                continue;
            }
            if (insideMedium && mediumObjectId == hit.objectId) {
                insideMedium = false;
                mediumObjectId = 0xffffffffu;
            } else {
                insideMedium = true;
                mediumObjectId = hit.objectId;
                mediumMaterial = hit.material;
            }
            origin = hit.position + direction * 0.002;
            continue;
        }
        if ((sceneData.featureFlags & PHOTON_FEATURE_ALPHA_TRANSPARENCY) != 0 &&
            randomValue(state) > saturate(hit.material.albedo.w)) {
            origin = hit.position + direction * 0.002;
            continue;
        }
        if (bounce == 0) {
            primaryHit = hit;
        }
        float3 albedo = max(hit.material.albedo.xyz, 0.0);
        if ((sceneData.featureFlags & PHOTON_FEATURE_EMISSIVE_LIGHTING) != 0) {
            radiance += throughput * hit.material.emissiveColor *
                        max(hit.material.emissiveIntensity, 0.0);
        }
        float3 view = safeNormal(-direction, hit.normal);
        radiance += throughput * directLighting(hit, view);
        if ((sceneData.featureFlags & PHOTON_FEATURE_CAUSTICS) != 0 &&
            hit.material.transmittance < 0.001 &&
            hit.material.metallic < 0.001) {
            radiance += throughput * gatherCaustics(hit);
        }
        if ((sceneData.featureFlags & PHOTON_FEATURE_INDIRECT_LIGHTING) == 0) {
            break;
        }
        float transmission =
            (sceneData.featureFlags & PHOTON_FEATURE_TRANSMISSION) != 0
                ? saturate(hit.material.transmittance)
                : 0.0;
        if (transmission > 0.0 && randomValue(state) < transmission) {
            uint channel = min(uint(randomValue(state) * 3.0), 2u);
            float ior = dispersionIor(hit.material.ior, hit.material.abbeNumber,
                                      channel);
            bool entering = hit.frontFace;
            float eta = entering ? 1.0 / ior : ior;
            float3 refracted = refract(direction, hit.normal, eta);
            if (dot(refracted, refracted) <= 1e-8) {
                direction = reflect(direction, hit.normal);
            } else {
                direction = safeNormal(refracted, direction);
                if ((sceneData.featureFlags & PHOTON_FEATURE_DISPERSION) != 0 &&
                    hit.material.abbeNumber > 1.0) {
                    throughput *= channel == 0   ? float3(3.0, 0.0, 0.0)
                                  : channel == 1 ? float3(0.0, 3.0, 0.0)
                                                 : float3(0.0, 0.0, 3.0);
                }
                float3 attenuation =
                    clamp(hit.material.attenuationColor, 0.001, 1.0);
                throughput *=
                    pow(attenuation,
                        1.0 / max(hit.material.attenuationDistance, 1e-4));
            }
            throughput *= max(hit.material.albedo.xyz, 0.0);
            origin = hit.position + direction * 0.002;
            continue;
        }
        float3 diffuseDirection = cosineDirection(hit.normal, state);
        float3 reflectedDirection = reflect(direction, hit.normal);
        float specularProbability =
            saturate(lerp(0.04, 1.0, hit.material.metallic) +
                     (1.0 - hit.material.roughness) * 0.2);
        if (randomValue(state) < specularProbability) {
            direction = safeNormal(
                lerp(reflectedDirection, diffuseDirection,
                     hit.material.roughness * hit.material.roughness),
                reflectedDirection);
            throughput *= lerp(float3(0.04), albedo, hit.material.metallic) /
                          max(specularProbability, 0.05);
        } else {
            direction = diffuseDirection;
            throughput *= albedo / max(1.0 - specularProbability, 0.05);
        }
        throughput *= bounce == 0 ? 1.0 : sceneData.indirectStrength;
        origin = hit.position + hit.normal * 0.002;
        if (bounce >= 2) {
            float survival =
                saturate(max(throughput.x, max(throughput.y, throughput.z)));
            if (randomValue(state) > survival) {
                break;
            }
            throughput /= max(survival, 0.05);
        }
    }
    return radiance;
}

[shader("compute")][numthreads(8, 8, 1)] void
computeMain(uint3 dispatchThreadID
            : SV_DispatchThreadID) {
    uint2 gid = dispatchThreadID.xy * max(sceneData.pixelStride, 1u);
    uint width, height;
    outTex.GetDimensions(width, height);
    if (gid.x >= width || gid.y >= height) {
        return;
    }
    uint state =
        hashValue(gid.x + gid.y * width + sceneData.frameIndex * 1664525u);
    float3 color = 0.0;
    SurfaceHit primaryHit = {};
    float2 primaryUv = (float2(gid) + 0.5) / float2(width, height);
    float3 primaryDirection = float3(0.0, 0.0, 1.0);
    uint samples = max(sceneData.raysPerPixel, 1u);
    for (uint sampleIndex = 0; sampleIndex < samples; ++sampleIndex) {
        float2 jitter =
            sampleIndex == 0
                ? 0.0
                : float2(randomValue(state), randomValue(state)) - 0.5;
        float2 uv = (float2(gid) + 0.5 + jitter) / float2(width, height);
        float2 ndc = uv * 2.0 - 1.0;
        ndc.y = -ndc.y;
        float4 world = mul(cam.invViewProj, float4(ndc, 1.0, 1.0));
        float3 direction =
            safeNormal(world.xyz / world.w - cam.camPos, float3(0.0, 0.0, 1.0));
        SurfaceHit sampleHit;
        color += samplePath(cam.camPos, direction, state, sampleHit);
        if (sampleIndex == 0) {
            primaryHit = sampleHit;
            primaryUv = uv;
            primaryDirection = direction;
        }
    }
    color /= float(samples);
    float4 previous = historyTex.Load(int3(gid, 0));
    float previousWeight =
        sceneData.frameIndex > 0
            ? min(previous.w,
                  float(max(sceneData.accumulationFrameLimit, 1u) - 1u))
            : 0.0;
    float historyLength = min(previousWeight + 1.0,
                              float(max(sceneData.accumulationFrameLimit, 1u)));
    float3 accumulated = (previous.xyz * previousWeight + color) /
                         max(previousWeight + 1.0, 1.0);
    float4 previousGuide = historyGuideTex.Load(int3(gid, 0));
    float luminance = dot(accumulated, float3(0.2126, 0.7152, 0.0722));
    float bloom =
        max(luminance - sceneData.bloomThreshold, 0.0) / max(luminance, 1e-5);
    float objectId = primaryHit.valid ? float(primaryHit.objectId) : -1.0;
    float3 normal = primaryHit.valid ? primaryHit.normal : 0.0;
    float depth = primaryHit.valid ? primaryHit.distance : 0.0;
    float2 encodedNormal = encodeNormal(normal);
    float4 currentGuide = float4(encodedNormal, depth, objectId);
    bool historyCompatible =
        sceneData.frameIndex > 0 && previous.w > 0.0 &&
        abs(previousGuide.z - depth) <= max(0.03, depth * 0.02) &&
        distance(previousGuide.xy, encodedNormal) <= 0.08 &&
        abs(previousGuide.w - objectId) <= 0.5;
    if (!historyCompatible) {
        previousWeight = 0.0;
        historyLength = 1.0;
        accumulated = color;
    }
    float3 albedo = primaryHit.valid ? primaryHit.material.albedo.xyz : 0.0;
    float roughness = primaryHit.valid ? primaryHit.material.roughness : 1.0;
    float4 previousClip =
        mul(cam.prevViewProj,
            float4(primaryHit.valid
                       ? cam.camPos + primaryDirection * primaryHit.distance
                       : cam.camPos,
                   1.0));
    float2 previousNdc = previousClip.xy / max(abs(previousClip.w), 1e-5);
    float2 previousUv =
        float2(previousNdc.x * 0.5 + 0.5, 0.5 - previousNdc.y * 0.5);
    bool previousUvValid = previousClip.w > 0.0 && all(previousUv >= 0.0) &&
                           all(previousUv <= 1.0);
    float4 moments = historyMomentsTex.Load(int3(gid, 0));
    float currentLuminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float mean = (moments.x * previousWeight + currentLuminance) /
                 max(previousWeight + 1.0, 1.0);
    float meanSquared =
        (moments.y * previousWeight + currentLuminance * currentLuminance) /
        max(previousWeight + 1.0, 1.0);
    uint stride = max(sceneData.pixelStride, 1u);
    for (uint y = 0; y < stride; ++y) {
        for (uint x = 0; x < stride; ++x) {
            uint2 pixel = gid + uint2(x, y);
            if (pixel.x >= width || pixel.y >= height) {
                continue;
            }
            outTex[pixel] = float4(accumulated, 1.0);
            brightTex[pixel] = float4(accumulated * bloom, 1.0);
            historyOutTex[pixel] = float4(accumulated, historyLength);
            historyGuideOutTex[pixel] = currentGuide;
            historyMomentsOutTex[pixel] = float4(
                mean, meanSquared, max(meanSquared - mean * mean, 0.0), depth);
            albedoRoughnessTex[pixel] = float4(albedo, roughness);
            normalDepthTex[pixel] = float4(normal, depth);
            motionObjectTex[pixel] =
                float4(previousUvValid ? primaryUv - previousUv : float2(0.0),
                       objectId, 1.0);
            directReservoirSampleOutTex[pixel] = 0.0;
            directReservoirStatsOutTex[pixel] = 0.0;
        }
    }
}

    [shader("compute")][numthreads(64, 1, 1)] void clearCaustics(
        uint3 dispatchThreadID
        : SV_DispatchThreadID) {
    uint bucket = dispatchThreadID.x;
    if (bucket < CAUSTIC_BUCKET_COUNT) {
        uint base = bucket * CAUSTIC_BUCKET_WORDS;
        photonSlots[base] = 0u;
        photonSlots[base + 1u] = 0u;
        for (uint slot = 0u; slot < CAUSTIC_BUCKET_SAMPLES; ++slot) {
            photonSlots[base + 2u + slot] = CAUSTIC_INVALID_BUCKET;
        }
    }
}

[shader("compute")][numthreads(64, 1, 1)] void
emitCaustics(uint3 dispatchThreadID
             : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;
    if (index >= (1u << 19)) {
        return;
    }
    uint state = hashValue(index + caustics.seed * 9781u);
    CausticPhoton photon = {};
    uint lightCount = sceneData.numDirectionalLights +
                      sceneData.numPointLights + sceneData.numSpotLights +
                      sceneData.numAreaLights;
    if (lightCount == 0u) {
        photons[index] = photon;
        return;
    }
    uint light =
        min(uint(randomValue(state) * float(lightCount)), lightCount - 1u);
    float3 origin;
    float3 direction;
    float3 power;
    if (light < sceneData.numDirectionalLights) {
        direction = safeNormal(-dirLight.direction, float3(0.0, -1.0, 0.0));
        float2 disk = float2(randomValue(state), randomValue(state));
        float radius = sqrt(disk.x) * caustics.bounds.w;
        float angle = disk.y * 6.28318530718;
        float3 tangent = safeNormal(cross(direction, float3(0.0, 1.0, 0.0)),
                                    float3(1.0, 0.0, 0.0));
        float3 bitangent = cross(direction, tangent);
        origin = caustics.bounds.xyz - direction * caustics.launchDistance +
                 radius * (cos(angle) * tangent + sin(angle) * bitangent);
        power = dirLight.color * max(dirLight.intensity, 0.0);
    } else if ((light -= sceneData.numDirectionalLights) <
               sceneData.numPointLights) {
        origin = pointLights[light].position;
        direction = safeNormal(caustics.bounds.xyz - origin +
                                   float3(randomValue(state) - 0.5,
                                          randomValue(state) - 0.5,
                                          randomValue(state) - 0.5) *
                                       caustics.bounds.w,
                               float3(0.0, -1.0, 0.0));
        power =
            pointLights[light].color * max(pointLights[light].intensity, 0.0);
    } else if ((light -= sceneData.numPointLights) < sceneData.numSpotLights) {
        origin = spotLights[light].position;
        direction =
            safeNormal(spotLights[light].direction, float3(0.0, -1.0, 0.0));
        power = spotLights[light].color * max(spotLights[light].intensity, 0.0);
    } else {
        light -= sceneData.numSpotLights;
        AreaLight source = areaLights[light];
        float2 uv = float2(randomValue(state), randomValue(state)) * 2.0 - 1.0;
        origin = source.position + source.right * uv.x * source.halfWidth +
                 source.up * uv.y * source.halfHeight;
        float3 normal =
            safeNormal(cross(source.right, source.up), float3(0.0, -1.0, 0.0));
        direction = cosineDirection(normal, state);
        power = source.color * max(source.intensity, 0.0);
    }
    SurfaceHit specularHit = traceSurface(origin, direction);
    if (!specularHit.valid || specularHit.material.roughness >= 0.25 ||
        (specularHit.material.transmittance <= 0.001 &&
         specularHit.material.metallic <= 0.001)) {
        photons[index] = photon;
        return;
    }
    float3 nextDirection;
    if (specularHit.material.transmittance > 0.001 &&
        (sceneData.featureFlags & PHOTON_FEATURE_TRANSMISSION) != 0) {
        uint channel = min(index % 3u, 2u);
        float ior = dispersionIor(specularHit.material.ior,
                                  specularHit.material.abbeNumber, channel);
        nextDirection = refract(direction, specularHit.normal,
                                specularHit.frontFace ? 1.0 / ior : ior);
        if (dot(nextDirection, nextDirection) <= 1e-8) {
            nextDirection = reflect(direction, specularHit.normal);
        }
    } else {
        nextDirection = reflect(direction, specularHit.normal);
    }
    nextDirection = safeNormal(nextDirection, direction);
    SurfaceHit receiver = traceSurface(
        specularHit.position + nextDirection * 0.002, nextDirection);
    if (receiver.valid && receiver.material.transmittance < 0.001 &&
        receiver.material.metallic < 0.001) {
        uint channel = min(index % 3u, 2u);
        float channelPower = channel == 0u   ? power.x
                             : channel == 1u ? power.y
                                             : power.z;
        photon.positionWavelength = float4(receiver.position, float(channel));
        photon.normalPower =
            float4(receiver.normal, channelPower / float(1u << 19));
        photon.incomingObject =
            float4(-nextDirection, float(receiver.objectId));
        photons[index] = photon;
        insertCausticPhoton(int3(floor(receiver.position / caustics.radius)),
                            index, state);
        return;
    }
    photons[index] = photon;
}
