#pragma once

#include <metal_stdlib>
using namespace metal;

#include "sampling.metal"

constant uint PHOTON_SPECTRAL_LANE_COUNT = 4;
constant float PHOTON_LAMBDA_MIN_NM = 380.0;
constant float PHOTON_LAMBDA_MAX_NM = 830.0;
constant float PHOTON_LAMBDA_RANGE_NM =
    PHOTON_LAMBDA_MAX_NM - PHOTON_LAMBDA_MIN_NM;
constant float PHOTON_CIE_Y_INTEGRAL = 106.856895f;

struct SpectralPath {
    float4 wavelengthNm;
    float4 wavelengthPdf;
    float4 throughput;
    float4 radiance;
};

SpectralPath createSpectralPath(thread uint &rng) {
    SpectralPath path;
    float4 strata = float4(0.0, 1.0, 2.0, 3.0);
    float4 u = (strata + float4(rand(rng))) /
               float(PHOTON_SPECTRAL_LANE_COUNT);
    path.wavelengthNm =
        PHOTON_LAMBDA_MIN_NM + u * PHOTON_LAMBDA_RANGE_NM;
    path.wavelengthPdf = float4(1.0 / PHOTON_LAMBDA_RANGE_NM);
    path.throughput = float4(1.0);
    path.radiance = float4(0.0);
    return path;
}

float spectralGaussian(float wavelengthNm, float centerNm, float sigmaNm) {
    float x = (wavelengthNm - centerNm) / sigmaNm;
    return exp(-0.5f * x * x);
}

float3 rgbSpectralWeights(float wavelengthNm) {
    float red = spectralGaussian(wavelengthNm, 610.0f, 60.0f);
    float green = spectralGaussian(wavelengthNm, 545.0f, 50.0f);
    float blue = spectralGaussian(wavelengthNm, 460.0f, 45.0f);
    float3 weights = float3(red, green, blue);
    return weights / max(weights.x + weights.y + weights.z, 1e-6f);
}

float rgbToReflectanceAtWavelength(float3 rgb, float wavelengthNm) {
    rgb = clamp(rgb, 0.0f, 1.0f);
    return clamp(dot(rgb, rgbSpectralWeights(wavelengthNm)), 0.0f, 1.0f);
}

float rgbToEmissionAtWavelength(float3 rgb, float wavelengthNm) {
    rgb = max(rgb, 0.0f);
    return max(dot(rgb, rgbSpectralWeights(wavelengthNm)), 0.0f);
}

float4 evaluateReflectance(float3 rgb, thread const SpectralPath &path) {
    return float4(
        rgbToReflectanceAtWavelength(rgb, path.wavelengthNm.x),
        rgbToReflectanceAtWavelength(rgb, path.wavelengthNm.y),
        rgbToReflectanceAtWavelength(rgb, path.wavelengthNm.z),
        rgbToReflectanceAtWavelength(rgb, path.wavelengthNm.w));
}

float4 evaluateEmission(float3 rgb, thread const SpectralPath &path) {
    return float4(rgbToEmissionAtWavelength(rgb, path.wavelengthNm.x),
                  rgbToEmissionAtWavelength(rgb, path.wavelengthNm.y),
                  rgbToEmissionAtWavelength(rgb, path.wavelengthNm.z),
                  rgbToEmissionAtWavelength(rgb, path.wavelengthNm.w));
}

float evaluateIorAtWavelength(float baseIor, thread const SpectralPath &path) {
    return max(baseIor, 1.0001f);
}

float spectralAverage(float4 spectrum) {
    return dot(spectrum, float4(0.25));
}

float spectralMax(float4 spectrum) {
    return max(max(spectrum.x, spectrum.y), max(spectrum.z, spectrum.w));
}

float cieX1931(float wavelengthNm) {
    float t1 =
        (wavelengthNm - 442.0f) * (wavelengthNm < 442.0f ? 0.0624f : 0.0374f);
    float t2 =
        (wavelengthNm - 599.8f) * (wavelengthNm < 599.8f ? 0.0264f : 0.0323f);
    float t3 =
        (wavelengthNm - 501.1f) * (wavelengthNm < 501.1f ? 0.0490f : 0.0382f);
    return 0.362f * exp(-0.5f * t1 * t1) + 1.056f * exp(-0.5f * t2 * t2) -
           0.065f * exp(-0.5f * t3 * t3);
}

float cieY1931(float wavelengthNm) {
    float t1 =
        (wavelengthNm - 568.8f) * (wavelengthNm < 568.8f ? 0.0213f : 0.0247f);
    float t2 =
        (wavelengthNm - 530.9f) * (wavelengthNm < 530.9f ? 0.0613f : 0.0322f);
    return 0.821f * exp(-0.5f * t1 * t1) + 0.286f * exp(-0.5f * t2 * t2);
}

float cieZ1931(float wavelengthNm) {
    float t1 =
        (wavelengthNm - 437.0f) * (wavelengthNm < 437.0f ? 0.0845f : 0.0278f);
    float t2 =
        (wavelengthNm - 459.0f) * (wavelengthNm < 459.0f ? 0.0385f : 0.0725f);
    return 1.217f * exp(-0.5f * t1 * t1) + 0.681f * exp(-0.5f * t2 * t2);
}

float3 cieXYZ1931(float wavelengthNm) {
    return max(float3(cieX1931(wavelengthNm), cieY1931(wavelengthNm),
                      cieZ1931(wavelengthNm)),
               float3(0.0f));
}

float3 spectralRadianceToXYZ(float4 radiance,
                             thread const SpectralPath &path) {
    float3 xyz = float3(0.0);
    for (uint lane = 0; lane < PHOTON_SPECTRAL_LANE_COUNT; ++lane) {
        float pdf = path.wavelengthPdf[lane];
        if (pdf > 0.0) {
            xyz += radiance[lane] * cieXYZ1931(path.wavelengthNm[lane]) /
                   (pdf * PHOTON_CIE_Y_INTEGRAL);
        }
    }
    return xyz / float(PHOTON_SPECTRAL_LANE_COUNT);
}

float3 xyzToLinearSRGB(float3 xyz) {
    return float3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
                  -0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
                  0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
}
