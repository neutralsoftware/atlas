static const uint PHOTON_FEATURE_DIRECT_LIGHTING = 1u << 0;
static const uint PHOTON_FEATURE_SHADOWS = 1u << 1;
static const uint PHOTON_FEATURE_ENVIRONMENT_LIGHTING = 1u << 2;
static const uint PHOTON_FEATURE_EMISSIVE_LIGHTING = 1u << 3;
static const uint PHOTON_FEATURE_INDIRECT_LIGHTING = 1u << 4;
static const uint PHOTON_FEATURE_TRANSMISSION = 1u << 5;
static const uint PHOTON_FEATURE_CAUSTICS = 1u << 8;

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

struct SurfaceHit {
    bool valid;
    float distance;
    float3 position;
    float3 normal;
    float2 uv;
    uint primitive;
    uint objectId;
    Material material;
};

[[vk::binding(0, 3)]] RaytracingAccelerationStructure sceneAS;
[[vk::binding(1, 0)]] ConstantBuffer<CameraUniforms> cam;
[[vk::binding(7, 0)]] ConstantBuffer<DirectionalLightData> dirLight;
[[vk::binding(8, 0)]] ConstantBuffer<SceneData> sceneData;
[[vk::binding(17, 0)]] ConstantBuffer<CausticSettings> caustics;
[[vk::binding(2, 1)]] StructuredBuffer<Material> materials;
[[vk::binding(3, 1)]] StructuredBuffer<uint> primitiveObjects;
[[vk::binding(4, 1)]] StructuredBuffer<VertexData> vertices;
[[vk::binding(5, 1)]] StructuredBuffer<uint> indices;
[[vk::binding(6, 1)]] StructuredBuffer<InstanceData> instanceData;
[[vk::binding(9, 1)]] StructuredBuffer<PointLight> pointLights;
[[vk::binding(10, 1)]] StructuredBuffer<SpotLight> spotLights;
[[vk::binding(11, 1)]] StructuredBuffer<AreaLight> areaLights;
[[vk::binding(13, 1)]] StructuredBuffer<uint> blasPrimitiveOffsets;
[[vk::binding(14, 1)]] StructuredBuffer<EmissiveTriangle> emissiveTriangles;
[[vk::binding(15, 1)]] RWStructuredBuffer<CausticPhoton> photons;
[[vk::binding(16, 1)]] RWStructuredBuffer<uint> photonSlots;
[[vk::binding(12, 1)]] Sampler2D materialTextures[256];
[[vk::binding(0, 2)]] RWTexture2D<float4> outTex;
[[vk::binding(1, 2)]] Texture2D<float4> historyTex;
[[vk::binding(2, 2)]] RWTexture2D<float4> brightTex;
[[vk::binding(3, 2)]] RWTexture2D<float4> albedoRoughnessTex;
[[vk::binding(4, 2)]] RWTexture2D<float4> normalDepthTex;
[[vk::binding(5, 2)]] RWTexture2D<float4> motionObjectTex;
[[vk::binding(6, 2)]] Texture2D<float4> historyMomentsTex;
[[vk::binding(7, 2)]] Texture2D<float4> historyGuideTex;
[[vk::binding(8, 2)]] RWTexture2D<float4> historyOutTex;
[[vk::binding(9, 2)]] RWTexture2D<float4> historyGuideOutTex;
[[vk::binding(10, 2)]] RWTexture2D<float4> historyMomentsOutTex;
[[vk::binding(61, 2)]] Texture2D<float4> directReservoirSampleTex;
[[vk::binding(62, 2)]] Texture2D<float4> directReservoirStatsTex;
[[vk::binding(63, 2)]] RWTexture2D<float4> directReservoirSampleOutTex;
[[vk::binding(64, 2)]] RWTexture2D<float4> directReservoirStatsOutTex;
[[vk::binding(60, 2)]] SamplerCube skybox;

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

float4 sampleMaterialTexture(int index, float2 uv) {
    if (index < 0 || uint(index) >= sceneData.materialTextureCount) {
        return 1.0;
    }
    return materialTextures[NonUniformResourceIndex(index)].Sample(frac(uv));
}

float3 cosineDirection(float3 normal, inout uint state) {
    float2 sample = float2(randomValue(state), randomValue(state));
    float radius = sqrt(sample.x);
    float angle = 6.28318530718 * sample.y;
    float3 tangent = safeNormal(abs(normal.x) > 0.1
                                    ? cross(float3(0.0, 1.0, 0.0), normal)
                                    : cross(float3(1.0, 0.0, 0.0), normal),
                                float3(1.0, 0.0, 0.0));
    float3 bitangent = cross(normal, tangent);
    return safeNormal(tangent * (radius * cos(angle)) +
                          bitangent * (radius * sin(angle)) +
                          normal * sqrt(max(0.0, 1.0 - sample.x)),
                      normal);
}

bool traceOcclusion(float3 origin, float3 direction, float maximumDistance) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = maximumDistance;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
             RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(sceneAS,
                         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
                             RAY_FLAG_FORCE_OPAQUE,
                         0xFF, ray);
    while (query.Proceed()) {}
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
    while (query.Proceed()) {}
    if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT) {
        return hit;
    }
    uint geometry = query.CommittedGeometryIndex();
    uint primitive = blasPrimitiveOffsets[geometry] +
                     query.CommittedPrimitiveIndex();
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
    float3x3 normalMatrix = float3x3(
        transform.normalCol0.xyz, transform.normalCol1.xyz,
        transform.normalCol2.xyz);
    hit.valid = true;
    hit.distance = query.CommittedRayT();
    hit.position = origin + direction * hit.distance;
    hit.normal = safeNormal(mul(normalMatrix, localNormal), float3(0.0, 1.0, 0.0));
    if (dot(hit.normal, direction) > 0.0) {
        hit.normal = -hit.normal;
    }
    hit.uv = vertices[i0].uv * weights.x + vertices[i1].uv * weights.y +
             vertices[i2].uv * weights.z;
    hit.primitive = primitive;
    hit.objectId = objectId;
    hit.material = materials[objectId];
    float2 materialUv = hit.uv * hit.material.textureScale +
                        hit.material.textureOffset;
    hit.material.albedo *=
        sampleMaterialTexture(hit.material.albedoTextureIndex, materialUv);
    float4 metallicSample = sampleMaterialTexture(
        hit.material.metallicTextureIndex, materialUv);
    float4 roughnessSample = sampleMaterialTexture(
        hit.material.roughnessTextureIndex, materialUv);
    float4 aoSample =
        sampleMaterialTexture(hit.material.aoTextureIndex, materialUv);
    if (hit.material.metallicTextureIndex >= 0) {
        hit.material.metallic *=
            hit.material.metallicTextureIndex ==
                    hit.material.roughnessTextureIndex
                ? metallicSample.z
                : metallicSample.x;
    }
    if (hit.material.roughnessTextureIndex >= 0) {
        hit.material.roughness *=
            hit.material.roughnessTextureIndex ==
                    hit.material.metallicTextureIndex
                ? roughnessSample.y
                : roughnessSample.x;
    }
    if (hit.material.aoTextureIndex >= 0) {
        hit.material.ao *= aoSample.x;
    }
    return hit;
}

float3 environmentColor(float3 direction) {
    if (sceneData.environmentEnabled != 0) {
        float3 sampled = skybox.Sample(direction).xyz;
        if (dot(sampled, sampled) > 1e-8) {
            return sampled * max(sceneData.ambientIntensity, 1.0);
        }
    }
    float skyAmount = saturate(direction.y * 0.5 + 0.5);
    float3 horizon = sceneData.ambientColor * sceneData.ambientIntensity;
    float3 zenith = max(sceneData.atmosphereSunColor *
                            sceneData.atmosphereSunIntensity * 0.025,
                        horizon);
    return lerp(horizon, zenith, skyAmount);
}

float3 directLighting(SurfaceHit hit) {
    if ((sceneData.featureFlags & PHOTON_FEATURE_DIRECT_LIGHTING) == 0) {
        return 0.0;
    }
    float3 result = 0.0;
    if (sceneData.numDirectionalLights > 0) {
        float3 lightDirection = safeNormal(-dirLight.direction, float3(0.0, 1.0, 0.0));
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002,
                           lightDirection, 1.0e20)) {
            visibility = 0.0;
        }
        result += dirLight.color * dirLight.intensity *
                  max(dot(hit.normal, lightDirection), 0.0) * visibility;
    }
    for (uint index = 0; index < sceneData.numPointLights; ++index) {
        float3 delta = pointLights[index].position - hit.position;
        float distanceSquared = max(dot(delta, delta), 1e-4);
        float distance = sqrt(distanceSquared);
        if (pointLights[index].range > 0.0 && distance > pointLights[index].range) {
            continue;
        }
        float3 lightDirection = delta / distance;
        float visibility = 1.0;
        if ((sceneData.featureFlags & PHOTON_FEATURE_SHADOWS) != 0 &&
            traceOcclusion(hit.position + hit.normal * 0.002,
                           lightDirection, distance - 0.004)) {
            visibility = 0.0;
        }
        result += pointLights[index].color * pointLights[index].intensity *
                  max(dot(hit.normal, lightDirection), 0.0) * visibility /
                  distanceSquared;
    }
    return result;
}

float3 samplePath(float3 origin, float3 direction, inout uint state,
                  out SurfaceHit primaryHit) {
    float3 radiance = 0.0;
    float3 throughput = 1.0;
    primaryHit = {};
    uint bounceCount = max(sceneData.maxBounces, 1u);
    for (uint bounce = 0; bounce < bounceCount; ++bounce) {
        SurfaceHit hit = traceSurface(origin, direction);
        if (!hit.valid) {
            if ((sceneData.featureFlags & PHOTON_FEATURE_ENVIRONMENT_LIGHTING) != 0) {
                radiance += throughput * environmentColor(direction);
            }
            break;
        }
        if (bounce == 0) {
            primaryHit = hit;
        }
        float3 albedo = max(hit.material.albedo.xyz, 0.0);
        if ((sceneData.featureFlags & PHOTON_FEATURE_EMISSIVE_LIGHTING) != 0) {
            radiance += throughput * hit.material.emissiveColor *
                        max(hit.material.emissiveIntensity, 0.0);
        }
        radiance += throughput * albedo * directLighting(hit) *
                    (1.0 / 3.14159265359);
        if ((sceneData.featureFlags & PHOTON_FEATURE_INDIRECT_LIGHTING) == 0) {
            break;
        }
        float3 diffuseDirection = cosineDirection(hit.normal, state);
        float3 reflectedDirection = reflect(direction, hit.normal);
        float specularProbability = saturate(lerp(0.04, 1.0, hit.material.metallic) +
                                             (1.0 - hit.material.roughness) * 0.2);
        if (randomValue(state) < specularProbability) {
            direction = safeNormal(lerp(reflectedDirection, diffuseDirection,
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
            float survival = saturate(max(throughput.x, max(throughput.y, throughput.z)));
            if (randomValue(state) > survival) {
                break;
            }
            throughput /= max(survival, 0.05);
        }
    }
    return radiance;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 gid = dispatchThreadID.xy * max(sceneData.pixelStride, 1u);
    uint width, height;
    outTex.GetDimensions(width, height);
    if (gid.x >= width || gid.y >= height) {
        return;
    }
    uint state = hashValue(gid.x + gid.y * width + sceneData.frameIndex * 1664525u);
    float3 color = 0.0;
    SurfaceHit primaryHit = {};
    uint samples = max(sceneData.raysPerPixel, 1u);
    for (uint sampleIndex = 0; sampleIndex < samples; ++sampleIndex) {
        float2 jitter = sampleIndex == 0
                            ? 0.0
                            : float2(randomValue(state), randomValue(state)) - 0.5;
        float2 uv = (float2(gid) + 0.5 + jitter) / float2(width, height);
        float2 ndc = uv * 2.0 - 1.0;
        ndc.y = -ndc.y;
        float4 world = mul(cam.invViewProj, float4(ndc, 1.0, 1.0));
        float3 direction = safeNormal(world.xyz / world.w - cam.camPos,
                                      float3(0.0, 0.0, 1.0));
        SurfaceHit sampleHit;
        color += samplePath(cam.camPos, direction, state, sampleHit);
        if (sampleIndex == 0) {
            primaryHit = sampleHit;
        }
    }
    color /= float(samples);
    float4 previous = historyTex.Load(int3(gid, 0));
    float previousWeight = sceneData.frameIndex > 0
                               ? min(previous.w, float(max(sceneData.accumulationFrameLimit, 1u) - 1u))
                               : 0.0;
    float historyLength = min(previousWeight + 1.0,
                              float(max(sceneData.accumulationFrameLimit, 1u)));
    float3 accumulated = (previous.xyz * previousWeight + color) /
                         max(previousWeight + 1.0, 1.0);
    float luminance = dot(accumulated, float3(0.2126, 0.7152, 0.0722));
    float bloom = max(luminance - sceneData.bloomThreshold, 0.0) /
                  max(luminance, 1e-5);
    float objectId = primaryHit.valid ? float(primaryHit.objectId) : -1.0;
    float3 normal = primaryHit.valid ? primaryHit.normal : 0.0;
    float depth = primaryHit.valid ? primaryHit.distance : 0.0;
    float3 albedo = primaryHit.valid ? primaryHit.material.albedo.xyz : 0.0;
    float roughness = primaryHit.valid ? primaryHit.material.roughness : 1.0;
    float4 moments = historyMomentsTex.Load(int3(gid, 0));
    float currentLuminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float mean = (moments.x * previousWeight + currentLuminance) /
                 max(previousWeight + 1.0, 1.0);
    float meanSquared = (moments.y * previousWeight + currentLuminance * currentLuminance) /
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
            historyGuideOutTex[pixel] = float4(normal, objectId);
            historyMomentsOutTex[pixel] = float4(mean, meanSquared,
                                                  max(meanSquared - mean * mean, 0.0), depth);
            albedoRoughnessTex[pixel] = float4(albedo, roughness);
            normalDepthTex[pixel] = float4(normal, depth);
            motionObjectTex[pixel] = float4(0.0, 0.0, objectId, 1.0);
            directReservoirSampleOutTex[pixel] = 0.0;
            directReservoirStatsOutTex[pixel] = 0.0;
        }
    }
}

[shader("compute")]
[numthreads(64, 1, 1)]
void clearCaustics(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;
    if (index < (1u << 17) * 34u) {
        photonSlots[index] = 0u;
    }
}

[shader("compute")]
[numthreads(64, 1, 1)]
void emitCaustics(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint index = dispatchThreadID.x;
    if (index >= (1u << 19)) {
        return;
    }
    uint state = hashValue(index + caustics.seed);
    float3 direction = safeNormal(float3(randomValue(state) * 2.0 - 1.0,
                                         randomValue(state) * 2.0 - 1.0,
                                         randomValue(state) * 2.0 - 1.0),
                                  float3(0.0, -1.0, 0.0));
    SurfaceHit hit = traceSurface(cam.camPos, direction);
    CausticPhoton photon = {};
    if (hit.valid) {
        photon.positionWavelength = float4(hit.position, 550.0);
        photon.normalPower = float4(hit.normal, 0.0);
        photon.incomingObject = float4(-direction, float(hit.objectId));
    }
    photons[index] = photon;
}
