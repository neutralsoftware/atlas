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
    float3 albedo;
    float _pad0;
    float3 emissiveColor;
    float _pad1;
    float4 optical;
    float4 attenuation;
    float4 volume;
    float4 volumeAbsorption;
    float4 volumeScattering;
    float4 volumeEmission;
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
    int3 padding;
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

struct AreaLight {
    float3 position;
    float _pad1;
    float3 right;
    float _pad2;
    float3 up;
    float _pad3;
    float2 size;
    float2 _pad4;
    float3 diffuse;
    float _pad6;
    float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

[[vk::binding(10, 3)]] RaytracingAccelerationStructure sceneAS;
[[vk::binding(0, 1)]] RWStructuredBuffer<float4> probeRadianceOut;
[[vk::binding(1, 1)]] StructuredBuffer<Triangle> tris;
[[vk::binding(2, 1)]] StructuredBuffer<Material> materials;
[[vk::binding(6, 1)]] StructuredBuffer<DirectionalLight> directionalLights;
[[vk::binding(7, 1)]] StructuredBuffer<PointLight> pointLights;
[[vk::binding(8, 1)]] StructuredBuffer<SpotLight> spotLights;
[[vk::binding(9, 1)]] StructuredBuffer<AreaLight> areaLights;
[[vk::binding(3, 0)]] ConstantBuffer<SceneCounts> sc;
[[vk::binding(4, 0)]] ConstantBuffer<ProbeSpace> ps;
[[vk::binding(5, 0)]] ConstantBuffer<RaytracingSettings> rt;
[[vk::binding(10, 2)]] Sampler2D materialTextures[48];
[[vk::binding(60, 2)]] SamplerCube skybox;
[[vk::binding(61, 2)]] Sampler2D previousIrradiance;

uint hashValue(uint value) {
    value = (value ^ 61u) ^ (value >> 16);
    value *= 9u;
    value ^= value >> 4;
    value *= 0x27d4eb2du;
    return value ^ (value >> 15);
}

float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    float value = float(index) + 0.5;
    float phi = 6.28318530718 * frac(value / 1.61803398875);
    phi += float(hashValue(frameIndex * 1471u + 5743u) & 0xFFFFu) /
           65536.0 * 6.28318530718;
    float cosine = 1.0 - 2.0 * value / float(max(count, 1u));
    float sine = sqrt(max(0.0, 1.0 - cosine * cosine));
    return float3(sine * cos(phi), cosine, sine * sin(phi));
}

float3 probePosition(uint index) {
    uint3 counts = max(uint3(ps.probeCount), uint3(1));
    uint x = index % counts.x;
    uint y = (index / counts.x) % counts.y;
    uint z = index / (counts.x * counts.y);
    return ps.origin + float3(x, y, z) * ps.spacing;
}

float3 skyRadiance(float3 direction) {
    if (rt.useSkybox != 0) {
        return max(skybox.Sample(direction).xyz, 0.0);
    }
    return lerp(rt.skyColor * 0.08, rt.skyColor,
                saturate(direction.y * 0.5 + 0.5));
}

float4 sampleMaterialTexture(int index, float2 uv) {
    if (index < 0 || uint(index) >= sc.textureCount) {
        return 1.0;
    }
    return materialTextures[NonUniformResourceIndex(index)].Sample(frac(uv));
}

float3 evaluateLights(float3 position, float3 normal) {
    float3 radiance = 0.0;
    for (uint index = 0; index < sc.directionalLightCount; ++index) {
        float3 direction = normalize(-directionalLights[index].direction);
        radiance += directionalLights[index].diffuse *
                    directionalLights[index].intensity *
                    max(dot(normal, direction), 0.0);
    }
    for (uint index = 0; index < sc.pointLightCount; ++index) {
        float3 delta = pointLights[index].position - position;
        float distanceSquared = max(dot(delta, delta), 1e-4);
        float distance = sqrt(distanceSquared);
        if (distance >= pointLights[index].radius) {
            continue;
        }
        float attenuation = 1.0 /
            max(pointLights[index].constant0 + pointLights[index].linear * distance +
                    pointLights[index].quadratic * distanceSquared,
                1e-4);
        radiance += pointLights[index].diffuse * pointLights[index].intensity *
                    max(dot(normal, delta / distance), 0.0) * attenuation;
    }
    return radiance;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint rayIndex = dispatchThreadID.x;
    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    uint activeProbe = rayIndex / raysPerProbe;
    if (activeProbe >= rt.probeUpdateCount) {
        return;
    }
    uint stride = max(rt.probeUpdateStride, 1u);
    uint probeIndex = (rt.probeUpdateOffset % stride) + activeProbe * stride;
    uint totalProbes = uint(max(ps.atlasParams.w, 0.0));
    if (probeIndex >= totalProbes) {
        return;
    }
    uint localRay = rayIndex % raysPerProbe;
    float3 origin = probePosition(probeIndex);
    float3 direction = sphericalFibonacci(localRay, raysPerProbe, rt.frameIndex);
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = max(rt.normalBias, 0.0001);
    ray.TMax = max(rt.maxRayDistance, ray.TMin);
    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(sceneAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);
    while (query.Proceed()) {}
    float3 radiance = skyRadiance(direction);
    float distance = -1.0;
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        uint primitive = query.CommittedPrimitiveIndex();
        if (primitive < sc.triCount) {
            float2 bary = query.CommittedTriangleBarycentrics();
            float3 weights = float3(1.0 - bary.x - bary.y, bary.x, bary.y);
            Triangle triangle = tris[primitive];
            float3 normal = normalize(triangle.n0.xyz * weights.x +
                                      triangle.n1.xyz * weights.y +
                                      triangle.n2.xyz * weights.z);
            float3 position = origin + direction * query.CommittedRayT();
            uint materialIndex = uint(max(triangle.materialID, 0));
            if (materialIndex < sc.materialCount) {
                Material material = materials[materialIndex];
                float2 uv = triangle.uv0.xy * weights.x +
                            triangle.uv1.xy * weights.y +
                            triangle.uv2.xy * weights.z;
                float3 albedo = material.albedo;
                if (material.albedoTextureIndex >= 0) {
                    albedo *= sampleMaterialTexture(
                                  material.albedoTextureIndex, uv)
                                  .xyz;
                }
                float2 previousUv = frac(direction.xz * 0.5 + 0.5);
                float3 previous = previousIrradiance.Sample(previousUv).xyz;
                radiance = material.emissiveColor * material.emissiveIntensity +
                           albedo * evaluateLights(position, normal) *
                               (1.0 / 3.14159265359) +
                           previous * 0.15;
            }
            distance = query.CommittedRayT();
        }
    }
    probeRadianceOut[probeIndex * raysPerProbe + localRay] =
        float4(max(radiance, 0.0), distance);
}
