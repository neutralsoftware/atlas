#if defined(ATLAS_VERTEX)
#include "utils/fullscreen.slang"
#endif

#if defined(ATLAS_FRAGMENT)
#include "utils/matrix.slang"
#include "utils/image.slang"

struct ShadowParameters {
    row_major float4x4 lightView;
    row_major float4x4 lightProjection;
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
    float3 cameraPosition;
    uint useIBL;
    int useContactOcclusion;
    int useRayTracedShadows;
};

struct Environment {
    float rimLightIntensity;
    float3 rimLightColor;
    float bloomThreshold;
};

struct PushConstants {
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
    uint featureFlags;
};

static const uint FEATURE_DIRECT_LIGHTING = 1u << 0;
static const uint FEATURE_SHADOWS = 1u << 1;
static const uint FEATURE_ENVIRONMENT_LIGHTING = 1u << 2;
static const uint FEATURE_INDIRECT_LIGHTING = 1u << 4;
static const uint FEATURE_TRANSMISSION = 1u << 5;
static const uint FEATURE_DISPERSION = 1u << 6;
static const uint FEATURE_EMISSION = 1u << 3;
static const uint FEATURE_VOLUMES = 1u << 9;

struct AreaLight {
    float3 position;
    float _pad1;
    float3 right;
    float _pad2;
    float3 up;
    float _pad3;
    float2 size;
    float _pad4;
    float _pad5;
    float3 diffuse;
    float _pad6;
    float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
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

};

static const float2 _660[12] = {float2(-0.326000005, -0.405999988),
     float2(-0.839999974, -0.074000001),
     float2(-0.69599998, 0.456999987),
     float2(-0.202999994, 0.620999992),
     float2(0.962000012, -0.194999993),
     float2(0.47299999, -0.479999989),
     float2(0.518999994, 0.76700002),
     float2(0.185000002, -0.893000007),
     float2(0.507000029, 0.064000003),
     float2(0.896000028, 0.412),
     float2(-0.321999997, -0.933000028),
     float2(-0.791999996, -0.59799999)};
static const float3 _861[20] = {float3(0.538100004, 0.185599998,
            -0.431899995),
     float3(0.137899995, 0.248600006,
            0.442999989),
     float3(0.337099999, 0.567900002,
            -0.0057000001),
     float3(-0.699899971, -0.0450999998,
            -0.00190000003),
     float3(0.0688999966, -0.159799993,
            -0.854700029),
     float3(0.0560000017, 0.00689999992,
            -0.184300005),
     float3(-0.0146000003, 0.140200004,
            0.0762000009),
     float3(0.00999999978, -0.192399994,
            -0.0344000012),
     float3(-0.35769999, -0.530099988,
            -0.435799986),
     float3(-0.316900015, 0.106299996,
            0.0157999992),
     float3(0.0103000002, -0.586899996,
            0.0046000001),
     float3(-0.0896999985, -0.493999988,
            0.328700006),
     float3(0.711899996, -0.0153999999,
            -0.0917999968),
     float3(-0.0533000007, 0.0595999993,
            -0.541100025),
     float3(0.0351999998, -0.0631000027,
            0.546000004),
     float3(-0.477600008, 0.284700006,
            -0.0271000005),
     float3(-0.112000003, 0.123400003,
            -0.744599998),
     float3(-0.213, -0.0781999975,
            -0.137899995),
     float3(0.294400007, -0.311199993,
            -0.264499992),
     float3(-0.456400007, 0.417499989,
            -0.184300005)};

struct FragmentOutput {
    float4 FragColor : SV_Target0;
    float4 BrightColor : SV_Target1;
};

struct FragmentInput {
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

#include "utils/deferred_lighting.slang"
#include "utils/ddgi.slang"

[[vk::binding(0, 1)]] ConstantBuffer<UBO> uniforms;
[[vk::binding(1, 1)]] ConstantBuffer<Environment> environment;
[[vk::push_constant]] ConstantBuffer<PushConstants> parameters;
[[vk::binding(0, 5)]] StructuredBuffer<ShadowParameters> ShadowParams;
[[vk::binding(0, 4)]] StructuredBuffer<DirectionalLight> DirectionalLights;
[[vk::binding(1, 4)]] StructuredBuffer<PointLight> PointLights;
[[vk::binding(2, 4)]] StructuredBuffer<SpotLight> SpotLights;
[[vk::binding(3, 4)]] StructuredBuffer<AreaLight> AreaLights;
[[vk::binding(2, 1)]] ConstantBuffer<AmbientLight> ambientLight;
[[vk::binding(9, 1)]] ConstantBuffer<ProbeSpace> ps;
[[vk::binding(5, 2)]] Sampler2D texture1;
[[vk::binding(6, 2)]] Sampler2D texture2;
[[vk::binding(7, 2)]] Sampler2D texture3;
[[vk::binding(8, 2)]] Sampler2D texture4;
[[vk::binding(9, 2)]] Sampler2D texture5;
[[vk::binding(0, 3)]] SamplerCube cubeMap1;
[[vk::binding(1, 3)]] SamplerCube cubeMap2;
[[vk::binding(2, 3)]] SamplerCube cubeMap3;
[[vk::binding(3, 3)]] SamplerCube cubeMap4;
[[vk::binding(4, 3)]] SamplerCube cubeMap5;
[[vk::binding(5, 3)]] SamplerCube skybox;
[[vk::binding(0, 2)]] Sampler2D gPosition;
[[vk::binding(1, 2)]] Sampler2D gNormal;
[[vk::binding(2, 2)]] Sampler2D gAlbedoSpec;
[[vk::binding(3, 2)]] Sampler2D gMaterial;
[[vk::binding(4, 2)]] Sampler2D ssao;
[[vk::binding(18, 2)]] Texture2D<float4> gOptical;
[[vk::binding(19, 2)]] Texture2D<float4> gMedium;
[[vk::binding(20, 2)]] Sampler2D gEmission;
[[vk::binding(16, 6)]] Texture2D<float4> irradianceMap;
[[vk::binding(17, 6)]] Texture2D<float4> ddgiDistanceMap;
[[vk::binding(21, 2)]] Texture2D<float4> rayShadowMap;
float shadowVisibilityForLight(int lightType, int lightIndex, float3 position, float3 normal, float2 rayShadowUV) {
    if ((parameters.featureFlags & FEATURE_SHADOWS) == 0u) return 1.0;
    if (uniforms.useRayTracedShadows != 0) {
        int slot = lightType == 0 && lightIndex < 2 ? lightIndex :
                   lightType == 3 && lightIndex < 4 ? 2 + lightIndex :
                   lightType == 1 && lightIndex < 2 ? 6 + lightIndex :
                   lightType == 2 && lightIndex < 2 ? 8 + lightIndex : -1;
        if (slot >= 0) {
            uint width, height;
            rayShadowMap.GetDimensions(width, height);
            uint tileHeight = height / 3;
            float2 pixelUv = rayShadowUV;
            uint2 pixel = min(uint2(pixelUv * float2(width, tileHeight)), uint2(width-1, tileHeight-1));
            return rayShadowMap.Load(int3(pixel.x, pixel.y + (slot / 4) * tileHeight, 0))[slot % 4];
        }
    }
    float shadow = 0.0;
    for (int index = 0; index < parameters.shadowParamCount; ++index) {
        ShadowParameters data = ShadowParams[index];
        if (data.lightType != lightType || data.lightIndex != lightIndex) continue;
        float value = lightType == 3
            ? calculatePointShadow(data, position, texture1, texture2, texture3,
                                   texture4, texture5, cubeMap1, cubeMap2, cubeMap3, cubeMap4, cubeMap5)
            : calculateShadow(data, position, normal, texture1, texture2, texture3,
                              texture4, texture5, uniforms,
                              lightType == 2 ? AreaLights[lightIndex].size : float2(0.0));
        shadow = max(shadow, value);
    }
    return 1.0 - saturate(shadow);
}

float contactOcclusion(float2 uv, float3 position, float3 normal) {
    float2 texelSize = 1.0 / float2(textureSize(gPosition));
    float occlusion = 0.0;
    float radius = 0.75;
    for (int i = 0; i < 16; ++i) {
        float angle = float(i % 8) * 0.785398163;
        float2 direction = float2(cos(angle), sin(angle));
        float2 sampleUV = uv + direction * texelSize * (i < 8 ? 3.0 : 9.0);
        if (any(sampleUV < float2(0.0)) || any(sampleUV > float2(1.0))) {
            continue;
        }
        float3 sampleNormal = gNormal.SampleLevel(sampleUV, 0.0).xyz;
        float3 samplePosition = gPosition.SampleLevel(sampleUV, 0.0).xyz;
        if (!all(isfinite(sampleNormal)) || !all(isfinite(samplePosition)) ||
            dot(sampleNormal, sampleNormal) < 0.0001) {
            continue;
        }
        float3 delta = samplePosition - position;
        float distanceSquared = dot(delta, delta);
        if (distanceSquared < 0.000001 || distanceSquared >= radius * radius) {
            continue;
        }
        float distance = sqrt(distanceSquared);
        float horizon = max(dot(normal, delta) / distance - 0.05, 0.0);
        occlusion += horizon * (1.0 - smoothstep(0.0, radius, distance));
    }
    return saturate(1.0 - occlusion * (2.0 / 16.0));
}

[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input) {
    FragmentOutput output = {};
    float4 gPositionSample = gPosition.Sample(input.TexCoord);
    float3 FragPos = gPositionSample.xyz;
    if (!all(isfinite(FragPos))) {
        FragPos = float3(0.0);
    }
    float depthSample = gPositionSample.w;
    float3 sampledNormal = gNormal.Sample(input.TexCoord).xyz;
    float normalLength = length(sampledNormal);
    bool hasNormal = all(isfinite(sampledNormal)) &&
                     normalLength > 9.99999975e-06;
    bool hasDepth =
        isfinite(depthSample) && depthSample < 0.999989986;
    bool hasGeometry = hasNormal || hasDepth;
    if (!hasGeometry) {
        float2 ndc = input.TexCoord * 2.0f - 1.0f;
        float3 backgroundDir = normalize(float3(ndc.x, -ndc.y, 1.0f));
        float3 backgroundColor =
            sampleEnvironmentRadiance(backgroundDir, skybox);
        output.FragColor = float4(backgroundColor, 1.0);
        output.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
        return output;
    }
    float3 N = float3(0.0, 1.0, 0.0);
    if (hasNormal) {
        N = sampledNormal / float3(normalLength);
    } else {
        float3 geomN = cross(ddx(FragPos), ddy(FragPos));
        float geomNLen = length(geomN);
        if (all(isfinite(geomN)) &&
            geomNLen > 9.99999975e-06) {
            N = geomN / float3(geomNLen);
        }
    }
    float3 geomNormal = cross(ddx(FragPos), ddy(FragPos));
    float geomNormalLen = length(geomNormal);
    bool hasGeomNormal = all(isfinite(geomNormal)) &&
                         geomNormalLen > 9.99999975e-06;
    float3 shadowNormal = N;
    float3 ddgiNormal = N;
    if (hasGeomNormal && !hasNormal) {
        geomNormal /= float3(geomNormalLen);
        if (dot(N, geomNormal) < 0.0) {
            geomNormal = -geomNormal;
        }
        shadowNormal = geomNormal;
        ddgiNormal = geomNormal;
    }
    float4 albedoAo = gAlbedoSpec.Sample(input.TexCoord);
    float3 albedo = clamp(albedoAo.xyz, float3(0.0), float3(1.0));
    if (!all(isfinite(albedo))) {
        albedo = float3(0.0);
    }
    float4 matData = gMaterial.Sample(input.TexCoord);
    float metallic = clamp(matData.x, 0.0, 1.0);
    float roughness = clamp(matData.y, 0.0, 1.0);
    float ao = clamp(matData.z, 0.0, 1.0);
    float reflectivity = clamp(matData.w, 0.0, 1.0);
    bool transmissionEnabled =
        (parameters.featureFlags & FEATURE_TRANSMISSION) != 0u;
    bool dispersionEnabled =
        (parameters.featureFlags & FEATURE_DISPERSION) != 0u;
    bool volumesEnabled = (parameters.featureFlags & FEATURE_VOLUMES) != 0u;
    bool environmentEnabled =
        (parameters.featureFlags & FEATURE_ENVIRONMENT_LIGHTING) != 0u &&
        uniforms.useIBL != 0u;
    bool indirectEnabled =
        (parameters.featureFlags & FEATURE_INDIRECT_LIGHTING) != 0u;
    float4 opticalData = float4(0.0, 1.5, 0.0, 1.0);
    float4 mediumData = float4(1.0, 1.0, 1.0, 0.0);
    if (transmissionEnabled) {
        uint opticalWidth;
        uint opticalHeight;
        gOptical.GetDimensions(opticalWidth, opticalHeight);
        uint2 opticalCoordinate = min(
            uint2(input.TexCoord * float2(opticalWidth, opticalHeight)),
            uint2(opticalWidth - 1, opticalHeight - 1));
        opticalData = gOptical.Load(int3(opticalCoordinate, 0));
        if (opticalData.x > 0.0001) {
            mediumData = gMedium.Load(int3(opticalCoordinate, 0));
        }
    }
    float transmission = transmissionEnabled
                             ? clamp(opticalData.x, 0.0, 1.0)
                             : 0.0;
    float materialIor = opticalData.y > 1.001 ? opticalData.y : 1.5;
    float abbeNumber = dispersionEnabled ? max(opticalData.z, 0.0) : 0.0;
    float attenuationDistance = max(opticalData.w, 0.0001);
    float3 attenuationColor = clamp(mediumData.xyz, float3(0.0001),
                                    float3(1.0));
    float mediumExtinction =
        volumesEnabled ? max(mediumData.w, 0.0) : 0.0;

    float viewDistance =
        max(length(float3(uniforms.cameraPosition) - FragPos),
                  9.99999997e-07);
    float3 V = (float3(uniforms.cameraPosition) - FragPos) / float3(viewDistance);
    float dielectricF0 = powr((materialIor - 1.0) / (materialIor + 1.0), 2.0);
    dielectricF0 = max(dielectricF0, reflectivity);
    float3 F0 = lerp(float3(dielectricF0), albedo, float3(metallic));
    float ssaoFactor =
        uniforms.useContactOcclusion != 0
            ? contactOcclusion(input.TexCoord, FragPos, N)
            : clamp(ssao.SampleLevel(input.TexCoord, 0.0).x, 0.0, 1.0);

    float ssaoContrast =
        clamp(powr(ssaoFactor, 1.10000002), 0.0, 1.0);
    float occlusion =
        clamp(ao * (0.0500000007 +
                          (0.949999988 * ssaoContrast)),
                    0.0, 1.0);
    bool directLightingEnabled =
        (parameters.featureFlags & FEATURE_DIRECT_LIGHTING) != 0u;
    float3 directionalResult = 0.0;
    float3 pointResult = 0.0;
    float3 spotResult = 0.0;
    float3 areaResult = 0.0;
    if (directLightingEnabled) {
        for (int i = 0; i < parameters.directionalLightCount; ++i)
            directionalResult += calcDirectionalLight(DirectionalLights[i], N, V, F0,
                albedo, metallic, roughness, transmission) *
                shadowVisibilityForLight(0, i, FragPos, shadowNormal, input.TexCoord);
        for (int i = 0; i < parameters.pointLightCount; ++i)
            pointResult += calcPointLight(PointLights[i], FragPos, N, V, F0,
                albedo, metallic, roughness, transmission) *
                shadowVisibilityForLight(3, i, FragPos, shadowNormal, input.TexCoord);
        for (int i = 0; i < parameters.spotlightCount; ++i)
            spotResult += calcSpotLight(SpotLights[i], FragPos, N, V, F0,
                albedo, metallic, roughness, transmission) *
                shadowVisibilityForLight(1, i, FragPos, shadowNormal, input.TexCoord);
        for (int i = 0; i < parameters.areaLightCount; ++i) {
            AreaLight light = AreaLights[i];
            float3 right = normalize(light.right);
            float3 up = normalize(light.up);
            float3 lightNormal = normalize(cross(right, up));
            float3 contribution = 0.0;
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    float2 sample = (float2(x, y) + 0.5) / 4.0 - 0.5;
                    float3 delta = light.position + right * sample.x * light.size.x +
                                   up * sample.y * light.size.y - FragPos;
                    float distanceSquared = max(dot(delta, delta), 1e-4);
                    float3 direction = delta * rsqrt(distanceSquared);
                    float cosine = dot(lightNormal, -direction);
                    cosine = light.castsBothSides != 0 ? abs(cosine) : max(cosine, 0.0);
                    if (cosine < cos(radians(light.angle))) continue;
                    float3 radiance = light.diffuse * max(light.intensity, 0.0) * cosine *
                                      max(light.size.x * light.size.y, 0.0) / (distanceSquared * 16.0);
                    contribution += evaluateBRDF(direction, radiance, N, V, F0, albedo,
                                                 metallic, roughness, transmission);
                }
            }
            areaResult += contribution * shadowVisibilityForLight(2, i, FragPos, shadowNormal, input.TexCoord);
        }
    }
    float3 param_36 = FragPos;
    float3 param_37 = N;
    float3 param_38 = V;
    float3 param_39 = F0;
    float3 param_40 = albedo;
    float param_41 = metallic;
    float param_42 = roughness;
    float3 _1714 = getRimLight(param_36, param_37, param_38, param_39, param_40,
                               param_41, param_42, uniforms, environment);
    float3 rimResult = _1714;
    float3 lighting =
        ((((directionalResult + pointResult) + spotResult) + areaResult) +
         rimResult);
    float3 ambientBase =
        ((ambientLight.color.xyz * ambientLight.intensity) * albedo) *
        occlusion * (1.0 - metallic) * (1.0 - transmission);
    bool ddgiEnabled = ps.atlasParams.w > 0.0f && indirectEnabled;
    float3 ambient = ddgiEnabled ? float3(0.0) : ambientBase;

    float ddgiSampleBias =
        max(min(ps.spacing.x, min(ps.spacing.y, ps.spacing.z)) * 0.02f, 0.001f);
    float3 ddgiSamplePos = FragPos + ddgiNormal * ddgiSampleBias;
    float3 ddgiIrradiance = float3(0.0f);
    if (ddgiEnabled) {
        ddgiIrradiance = sampleDDGIIrradiance(
            irradianceMap, ddgiDistanceMap, ps, ddgiSamplePos, ddgiNormal);
    }
    if (!all(isfinite(ddgiIrradiance))) {
        ddgiIrradiance = float3(0.0f);
    }
    ddgiIrradiance = max(ddgiIrradiance, float3(0.0f));

    float ddgiDebugMode = ps.debugColor.w;
    float3 ddgiGain = max(ps.debugColor.xyz, float3(0.0f));
    if (ddgiGain.x < 1e-4f && ddgiGain.y < 1e-4f && ddgiGain.z < 1e-4f) {
        ddgiGain = float3(1.0f);
    }

    const float INV_PI = 0.318309886;
    float3 ddgiDiffuse = ddgiIrradiance * albedo * INV_PI *
                         (1.0f - metallic) * (1.0f - transmission) *
                         occlusion * ddgiGain;
    ambient += ddgiDiffuse;

    float3 reflectionDirection = reflect(-V, N);
    reflectionDirection = normalize(lerp(
        reflectionDirection, N, roughness * roughness * 0.25));
    float3 ddgiReflection = float3(0.0f);
    bool needsDdgiSpecular = ddgiEnabled && roughness < 0.65 &&
                             max(metallic, reflectivity) > 0.02;
    if (needsDdgiSpecular) {
        ddgiReflection = sampleDDGIDirectionalRadiance(
            irradianceMap, ddgiDistanceMap, ps, ddgiSamplePos, ddgiNormal,
            reflectionDirection);
    }
    float3 viewFresnel = fresnelSchlick(max(dot(N, V), 0.0), F0);
    float reflectionRoughness = lerp(1.0, 0.12, roughness);
    float3 ddgiSpecular = ddgiReflection * INV_PI * viewFresnel *
                          reflectionRoughness * occlusion * ddgiGain;

    float3 iblContribution = float3(0.0);
    if (environmentEnabled) {
        float3 param_43 = N;
        float3 irradiance =
            sampleEnvironmentRadiance(param_43, skybox);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_44 = reflection;
        float3 specularEnv = sampleEnvironmentRadiance(param_44, skybox);
        float specularLuminance =
            dot(specularEnv, float3(0.2126, 0.7152, 0.0722));
        float3 ambientSpecular = ambientLight.color.xyz *
                                 ambientLight.intensity;
        specularEnv = specularLuminance > 0.0001
                          ? specularEnv
                          : ambientSpecular;
        float param_45 = max(dot(N, V), 0.0);
        float3 param_46 = F0;
        float3 F = fresnelSchlick(param_45, param_46);
        float3 kS = F;
        float3 kD = (float3(1.0) - kS) * (1.0 - metallic) *
                    (1.0 - transmission);
        float roughnessAttenuation = lerp(1.0, 0.150000006,
                                         clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution =
            ((ddgiEnabled ? float3(0.0) : kD * diffuseIBL) + (kS * specularIBL)) *
            occlusion;
    }
    float3 finalColor = (ambient + lighting + ddgiSpecular) + iblContribution;

    if (transmission > 0.0001 && metallic < 0.999) {
        float dispersionSpread = abbeNumber > 0.0
                                     ? max(materialIor - 1.0, 0.0) /
                                           max(abbeNumber, 1.0)
                                     : 0.0;
        float3 iorRgb = max(float3(materialIor - dispersionSpread * 0.5,
                                   materialIor,
                                   materialIor + dispersionSpread * 0.5),
                            float3(1.0001));
        float3 redDirection = refract(-V, N, 1.0 / iorRgb.x);
        float3 greenDirection = refract(-V, N, 1.0 / iorRgb.y);
        float3 blueDirection = refract(-V, N, 1.0 / iorRgb.z);
        if (dot(greenDirection, greenDirection) > 0.000001) {
            redDirection = dot(redDirection, redDirection) > 0.000001
                               ? normalize(redDirection)
                               : normalize(greenDirection);
            greenDirection = normalize(greenDirection);
            blueDirection = dot(blueDirection, blueDirection) > 0.000001
                                ? normalize(blueDirection)
                                : greenDirection;
            float3 probeTransmission = ddgiEnabled
                                           ? ddgiIrradiance
                                           : float3(0.0f);
            float3 environmentTransmission = environmentEnabled
                ? float3(skybox.Sample(redDirection).x,
                         skybox.Sample(greenDirection).y,
                         skybox.Sample(blueDirection).z)
                : float3(0.0f);
            float probeEnergy = dot(probeTransmission,
                                    float3(0.2126, 0.7152, 0.0722));
            float3 transmittedRadiance =
                probeEnergy > 0.0001
                    ? lerp(probeTransmission, environmentTransmission,
                           dispersionEnabled ? 0.3 : 0.12)
                    : environmentTransmission;
            float probeSpacing =
                max(max(ps.spacing.x, max(ps.spacing.y, ps.spacing.z)),
                    0.05);
            float opticalThickness =
                min(max(probeSpacing * 0.35, 0.05), attenuationDistance);
            float3 absorption = powr(
                attenuationColor,
                float3(opticalThickness / attenuationDistance));
            float volumeTransmission =
                exp(-mediumExtinction * opticalThickness);
            float3 mediumScatter =
                ddgiIrradiance * attenuationColor *
                (1.0 - volumeTransmission) * INV_PI;
            transmittedRadiance = transmittedRadiance * absorption *
                                      volumeTransmission +
                                  mediumScatter;
            finalColor += transmittedRadiance *
                          (float3(1.0) - viewFresnel) * transmission;
        }
    }

    if (ddgiDebugMode >= 9.5f && ddgiDebugMode < 19.5f) {
        finalColor = float3(ssaoFactor);
    } else if (ddgiDebugMode >= 39.5f) {
        finalColor =
            max(ddgiDiffuse * 3.0f + ddgiSpecular * 2.0f, float3(0.0f));
    } else if (ddgiDebugMode >= 29.5f) {
        finalColor = ddgiIrradiance * ddgiGain * 4.0f;
    } else if (ddgiDebugMode >= 19.5f) {
        finalColor = ddgiDiffuse * 3.0f;
    }
    if ((parameters.featureFlags & FEATURE_EMISSION) != 0u)
        finalColor += max(gEmission.Sample(input.TexCoord).xyz, 0.0);
    output.FragColor = float4(finalColor, 1.0);
    float brightness =
        dot(output.FragColor.xyz,
            float3(0.212599993, 0.715200007,
                   0.0722000003));
    float bloomContribution =
        max(brightness - environment.bloomThreshold, 0.0) /
        max(brightness, 0.00001);
    output.BrightColor =
        float4(finalColor * bloomContribution, 1.0);
    return output;
}
#endif
