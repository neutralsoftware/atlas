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
};

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
[[vk::binding(16, 6)]] Texture2D<float4> irradianceMap;
[[vk::binding(17, 6)]] Texture2D<float4> ddgiDistanceMap;
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
        output.FragColor = float4(acesToneMapping(backgroundColor), 1.0);
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
    if (hasGeomNormal) {
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

    float viewDistance =
        max(length(float3(uniforms.cameraPosition) - FragPos),
                  9.99999997e-07);
    float3 V = (float3(uniforms.cameraPosition) - FragPos) / float3(viewDistance);
    float3 F0 =
        lerp(float3(0.0399999991), albedo, float3(metallic));
    float ssaoFactor =
        clamp(ssao.Sample(input.TexCoord).x, 0.0, 1.0);

    float ssaoContrast =
        clamp(powr(ssaoFactor, 1.10000002), 0.0, 1.0);
    float occlusion =
        clamp(ao * (0.0500000007 +
                          (0.949999988 * ssaoContrast)),
                    0.0, 1.0);
    float lightingOcclusion =
        clamp(0.0500000007 +
                        (0.949999988 * ssaoContrast),
                    0.0500000007, 1.0);
    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    int areaShadowCount = 0;
    float pointShadow = 0.0;
    int shadowCount = parameters.shadowParamCount;
    for (int i = 0; i < shadowCount; i++) {
        if (ShadowParams[i].lightType == 3) {
            int lightIndex = ShadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= parameters.pointLightCount ||
                distance(float3(PointLights[lightIndex].position),
                         FragPos) >= PointLights[lightIndex].radius) {
                continue;
            }
            ShadowParameters _1386;
            _1386.lightView = ShadowParams[i].lightView;
            _1386.lightProjection = ShadowParams[i].lightProjection;
            _1386.bias0 = ShadowParams[i].bias0;
            _1386.textureIndex = ShadowParams[i].textureIndex;
            _1386.farPlane = ShadowParams[i].farPlane;
            _1386.lightIndex = ShadowParams[i].lightIndex;
            _1386.lightPos = float3(ShadowParams[i].lightPos);
            _1386.lightType = ShadowParams[i].lightType;
            ShadowParameters param = _1386;
            float3 param_1 = FragPos;
            pointShadow =
                max(pointShadow,
                          calculatePointShadow(
                              param, param_1, texture1, texture2, texture3, texture4, texture5, cubeMap1, cubeMap2, cubeMap3, cubeMap4, cubeMap5));
        } else if (ShadowParams[i].lightType == 1) {
            int lightIndex = ShadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= parameters.spotlightCount ||
                distance(float3(SpotLights[lightIndex].position),
                         FragPos) >= SpotLights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = ShadowParams[i].lightView;
            _1397.lightProjection = ShadowParams[i].lightProjection;
            _1397.bias0 = ShadowParams[i].bias0;
            _1397.textureIndex = ShadowParams[i].textureIndex;
            _1397.farPlane = ShadowParams[i].farPlane;
            _1397.lightIndex = ShadowParams[i].lightIndex;
            _1397.lightPos = float3(ShadowParams[i].lightPos);
            _1397.lightType = ShadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            spotShadow = max(
                spotShadow,
                calculateShadow(param_2, param_3, param_4, texture1, texture2,
                                texture3, texture4, texture5, uniforms));
        } else if (ShadowParams[i].lightType == 2) {
            int lightIndex = ShadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= parameters.areaLightCount ||
                distance(float3(AreaLights[lightIndex].position),
                         FragPos) >= AreaLights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = ShadowParams[i].lightView;
            _1397.lightProjection = ShadowParams[i].lightProjection;
            _1397.bias0 = ShadowParams[i].bias0;
            _1397.textureIndex = ShadowParams[i].textureIndex;
            _1397.farPlane = ShadowParams[i].farPlane;
            _1397.lightIndex = ShadowParams[i].lightIndex;
            _1397.lightPos = float3(ShadowParams[i].lightPos);
            _1397.lightType = ShadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            areaShadow += calculateShadow(
                param_2, param_3, param_4, texture1, texture2, texture3, texture4,
                texture5, uniforms);
            areaShadowCount++;
        } else {
            ShadowParameters _1397;
            _1397.lightView = ShadowParams[i].lightView;
            _1397.lightProjection = ShadowParams[i].lightProjection;
            _1397.bias0 = ShadowParams[i].bias0;
            _1397.textureIndex = ShadowParams[i].textureIndex;
            _1397.farPlane = ShadowParams[i].farPlane;
            _1397.lightIndex = ShadowParams[i].lightIndex;
            _1397.lightPos = float3(ShadowParams[i].lightPos);
            _1397.lightType = ShadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            directionalShadow = max(
                directionalShadow,
                calculateShadow(param_2, param_3, param_4, texture1, texture2,
                                texture3, texture4, texture5, uniforms));
        }
    }
    if (areaShadowCount > 0) {
        areaShadow /= float(areaShadowCount);
    }
    directionalShadow =
        clamp(directionalShadow * 0.850000024, 0.0, 1.0);
    spotShadow = clamp(spotShadow * 0.850000024, 0.0, 1.0);
    areaShadow = clamp(areaShadow * 0.449999988, 0.0, 1.0);
    pointShadow =
        clamp(pointShadow * 0.850000024, 0.0, 1.0);
    float3 directionalResult = float3(0.0);
    for (int i_1 = 0; i_1 < parameters.directionalLightCount; i_1++) {
        DirectionalLight _1428;
        _1428.direction = float3(DirectionalLights[i_1].direction);
        _1428._pad1 = DirectionalLights[i_1]._pad1;
        _1428.diffuse = float3(DirectionalLights[i_1].diffuse);
        _1428._pad2 = DirectionalLights[i_1]._pad2;
        _1428.specular = float3(DirectionalLights[i_1].specular);
        _1428.intensity = DirectionalLights[i_1].intensity;
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
    for (int i_2 = 0; i_2 < parameters.pointLightCount; i_2++) {
        PointLight _1471;
        _1471.position = float3(PointLights[i_2].position);
        _1471._pad1 = PointLights[i_2]._pad1;
        _1471.diffuse = float3(PointLights[i_2].diffuse);
        _1471._pad2 = PointLights[i_2]._pad2;
        _1471.specular = float3(PointLights[i_2].specular);
        _1471.intensity = PointLights[i_2].intensity;
        _1471.constant0 = PointLights[i_2].constant0;
        _1471.linear = PointLights[i_2].linear;
        _1471.quadratic = PointLights[i_2].quadratic;
        _1471.radius = PointLights[i_2].radius;
        _1471._pad3 = PointLights[i_2]._pad3;
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
    for (int i_3 = 0; i_3 < parameters.spotlightCount; i_3++) {
        SpotLight _1516;
        _1516.position = float3(SpotLights[i_3].position);
        _1516._pad1 = SpotLights[i_3]._pad1;
        _1516.direction = float3(SpotLights[i_3].direction);
        _1516.cutOff = SpotLights[i_3].cutOff;
        _1516.outerCutOff = SpotLights[i_3].outerCutOff;
        _1516.intensity = SpotLights[i_3].intensity;
        _1516.range = SpotLights[i_3].range;
        _1516._pad4 = SpotLights[i_3]._pad4;
        _1516.diffuse = float3(SpotLights[i_3].diffuse);
        _1516._pad5 = SpotLights[i_3]._pad5;
        _1516.specular = float3(SpotLights[i_3].specular);
        _1516._pad6 = SpotLights[i_3]._pad6;
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
    for (int i_4 = 0; i_4 < parameters.areaLightCount; i_4++) {
        float3 P = float3(AreaLights[i_4].position);
        float3 R = normalize(float3(AreaLights[i_4].right));
        float3 U = normalize(float3(AreaLights[i_4].up));
        float2 halfSize = AreaLights[i_4].size * 0.5;
        float3 toPoint = FragPos - P;
        float s = clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        float3 Q = (P + (R * s)) + (U * t);
        float3 Lvec = Q - FragPos;
        float dist = length(Lvec);
        if (dist > 9.99999975e-05) {
            float3 L = Lvec / float3(dist);
            float3 Nl = normalize(cross(R, U));
            float ndotl = dot(Nl, -L);
            if (AreaLights[i_4].castsBothSides != 0) {
                _1639 = abs(ndotl);
            } else {
                _1639 = max(ndotl, 0.0);
            }
            float facing = _1639;
            float cosTheta = cos(radians(AreaLights[i_4].angle));
            if ((facing >= cosTheta) && (facing > 0.0)) {
                float range = max(AreaLights[i_4].range,
                                        0.00100000005);
                float attenuation = 1.0 / ((1.0 + (dist / range)) +
                                           ((dist * dist) / (range * range)));
                float fade = 1.0 - smoothstep(range * 0.899999976,
                                              range, dist);
                float3 radiance =
                    (((float3(AreaLights[i_4].diffuse) *
                       max(AreaLights[i_4].intensity, 0.0)) *
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
                               param_41, param_42, uniforms, environment);
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

    const float INV_PI = 0.318309886;
    float3 ddgiDiffuse = ddgiIrradiance * albedo * INV_PI *
                         (1.0f - metallic) * occlusion * ddgiGain;
    ambient += ddgiDiffuse;

    float3 ddgiSpecular = float3(0.0f);

    float3 iblContribution = float3(0.0);
    if (uniforms.useIBL != 0u) {
        float3 param_43 = N;
        float3 irradiance =
            sampleEnvironmentRadiance(param_43, skybox);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_44 = reflection;
        float3 specularEnv =
            sampleEnvironmentRadiance(param_44, skybox);
        float param_45 = max(dot(N, V), 0.0);
        float3 param_46 = F0;
        float3 F = fresnelSchlick(param_45, param_46);
        float3 kS = F;
        float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
        float roughnessAttenuation = lerp(1.0, 0.150000006,
                                         clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution = ((kD * diffuseIBL) + (kS * specularIBL)) * occlusion;
    }
    float3 finalColor = (ambient + lighting + ddgiSpecular) + iblContribution;

    if (ddgiDebugMode >= 9.5f && ddgiDebugMode < 19.5f) {
        finalColor = float3(ssaoFactor);
    } else if (ddgiDebugMode >= 39.5f) {
        finalColor =
            max(ddgiDiffuse * 3.0f + ddgiSpecular * 2.0f, float3(0.0f));
    } else if (ddgiDebugMode >= 29.5f) {
        finalColor = ddgiIrradiance * ddgiGain * 4.0f;
    } else if (ddgiDebugMode >= 19.5f) {
        finalColor = ddgiDiffuse * 3.0f;
    } else if (!(uniforms.useIBL != 0u)) {
        float3 I = normalize(FragPos - float3(uniforms.cameraPosition));
        float3 R_1 = reflect(-I, N);
        float param_47 = max(dot(N, -I), 0.0);
        float3 param_48 = F0;
        float3 F_1 = fresnelSchlick(param_47, param_48);
        float3 kS_1 = F_1;
        float3 envColor = skybox.Sample(R_1).xyz;
        float3 reflection_1 = envColor * kS_1;
        float3 envMix = F0 * (1.0f - roughness) * 0.20f;
        finalColor = lerp(finalColor, reflection_1, envMix);
    }
    output.FragColor = float4(finalColor, 1.0);
    float brightness =
        dot(output.FragColor.xyz,
            float3(0.212599993, 0.715200007,
                   0.0722000003));
    if (brightness > environment.bloomThreshold) {
        output.BrightColor = float4(output.FragColor.xyz, 1.0);
    } else {
        output.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 param_49 = output.FragColor.xyz;
    float3 _1897 = acesToneMapping(param_49);
    output.FragColor.x = _1897.x;
    output.FragColor.y = _1897.y;
    output.FragColor.z = _1897.z;
    return output;
}
#endif
