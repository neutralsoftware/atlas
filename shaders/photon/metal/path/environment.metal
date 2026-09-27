#pragma once

#include <metal_stdlib>
using namespace metal;

#include "types.metal"
#include "spectral.metal"

constexpr sampler skyboxSampler(coord::normalized, address::clamp_to_edge,
                                filter::linear, mip_filter::linear);

float cloudHash(float2 p) {
    return fract(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float cloudValueNoise(float2 p) {
    float2 i = floor(p);
    float2 f = fract(p);
    f = f * f * (3.0f - 2.0f * f);
    return mix(mix(cloudHash(i), cloudHash(i + float2(1.0f, 0.0f)), f.x),
               mix(cloudHash(i + float2(0.0f, 1.0f)),
                   cloudHash(i + float2(1.0f)), f.x),
               f.y);
}

float cloudFbm(float2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (uint octave = 0; octave < 5; ++octave) {
        value += cloudValueNoise(p) * amplitude;
        p = p * 2.03f + float2(13.1f, 7.7f);
        amplitude *= 0.5f;
    }
    return value;
}

float4 skyColor(float3 dir, float intensity, texturecube<float> skybox,
                constant SceneData &sceneData,
                thread const SpectralPath &path) {
    float3 sampleDir = dir;
    float len2 = dot(sampleDir, sampleDir);

    if (len2 > 1e-10) {
        sampleDir *= rsqrt(len2);
    } else {
        sampleDir = float3(0.0, 1.0, 0.0);
    }

    float3 skyRGB = skybox.sample(skyboxSampler, sampleDir).xyz;
    if (sceneData.atmosphereSkyEnabled == 0) {
        skyRGB += sceneData.ambientColor * max(sceneData.ambientIntensity, 0.0f);
    }

    if (sceneData.atmosphereSkyEnabled != 0) {
        float horizon = pow(clamp(1.0 - abs(sampleDir.y), 0.0, 1.0), 4.0);

        float daylight =
            smoothstep(-0.2, 0.15, sceneData.atmosphereSunDirection.y);

        float3 zenith = float3(0.08, 0.28, 0.65);

        float3 horizonColor = float3(0.58, 0.72, 0.92);

        float3 proceduralSky =
            mix(zenith, horizonColor, horizon) * max(daylight, 0.08);

        skyRGB = max(skyRGB, proceduralSky);
    }

    if (sceneData.atmosphereSkyEnabled != 0) {
        skyRGB = max((skyRGB - 0.25f) * 1.22f + 0.25f, float3(0.0f)) *
                 0.82f;
    }

    if (sceneData.atmosphereSkyEnabled != 0 &&
        sceneData.atmosphereSunDirection.y > -0.15) {

        float3 sunDirection = sceneData.atmosphereSunDirection;

        float sunDirectionLength = dot(sunDirection, sunDirection);

        sunDirection = sunDirectionLength > 1e-10
                           ? sunDirection * rsqrt(sunDirectionLength)
                           : float3(0.0, 1.0, 0.0);

        float sunDot = dot(sampleDir, sunDirection);

        float sizeAdjust = 1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.001;

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

        float horizonFade =
            smoothstep(-0.15, 0.05, sceneData.atmosphereSunDirection.y);

        float sunIntensity = max(sceneData.atmosphereSunIntensity, 0.05);

        skyRGB += sceneData.atmosphereSunColor *
                  (sunDisk * 5.0 + sunGlow * 0.5 + sunHalo) * horizonFade *
                  sunIntensity;
    }

    if (sceneData.atmosphereSkyEnabled != 0 &&
        sceneData.cloudsEnabled != 0) {
        float horizonFade = smoothstep(-0.08f, 0.22f, sampleDir.y);
        float2 cloudUv = sampleDir.xz /
                         max(abs(sampleDir.y) + 0.18f, 0.18f);
        cloudUv = cloudUv * max(sceneData.cloudSettings.x, 0.01f) +
                  sceneData.cloudLighting.zw;
        float baseNoise = cloudFbm(cloudUv);
        float detailNoise = cloudFbm(cloudUv * 3.7f + float2(4.2f, 9.1f));
        float shapedNoise = mix(baseNoise, baseNoise * detailNoise,
                                clamp(sceneData.cloudLighting.y, 0.0f, 1.0f));
        float threshold = 1.0f - clamp(sceneData.cloudSettings.y, 0.0f, 1.0f);
        float cloudAmount = smoothstep(
            threshold, threshold + 0.18f,
            shapedNoise * max(sceneData.cloudSettings.z, 0.0f));
        cloudAmount *= horizonFade;
        float3 sunDirection = normalize(sceneData.atmosphereSunDirection);
        float sunLighting = clamp(dot(sampleDir, sunDirection) * 0.5f + 0.5f,
                                  0.0f, 1.0f);
        float3 cloudLight = mix(
            float3(0.2f, 0.23f, 0.3f), sceneData.atmosphereSunColor,
            sunLighting * clamp(sceneData.cloudLighting.x, 0.0f, 2.0f));
        cloudLight *= exp(-max(sceneData.cloudSettings.w, 0.0f) *
                          (1.0f - sunLighting) * 0.35f);
        skyRGB = mix(skyRGB, cloudLight, cloudAmount * 0.85f);
    }

    float scale = intensity > 0.0 ? intensity : 1.0;

    skyRGB *= scale;

    float4 reuslt = float4(0.0);
    for (uint i = 0; i < PHOTON_SPECTRAL_LANE_COUNT; ++i) {
        reuslt[i] = rgbToEmissionAtWavelength(skyRGB, path.wavelengthNm[i]);
    }
    return reuslt;
}
