#pragma once

#include <metal_stdlib>
using namespace metal;

#include "sampling.metal"

constant float PHOTON_LAMBDA_MIN_NM = 380.0;
constant float PHOTON_LAMBDA_MAX_NM = 830.0;
constant float PHOTON_LAMBDA_RANGE_NM =
    PHOTON_LAMBDA_MAX_NM - PHOTON_LAMBDA_MIN_NM;

constant float PHOTON_CIE_Y_INTEGRAL = 106.856895f;

struct SpectralPath {
    float wavelengthNm;
    float wavelengthPdf;

    float throughput;
    float radiance;
};

SpectralPath createSpectralPath(thread uint &rng) {
    SpectralPath path;

    float u = rand(rng);

    path.wavelengthNm = PHOTON_LAMBDA_MIN_NM + u * PHOTON_LAMBDA_RANGE_NM;
    path.wavelengthPdf = 1.0 / PHOTON_LAMBDA_RANGE_NM;

    path.throughput = 1.0;
    path.radiance = 0.0;

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

    float3 weights = rgbSpectralWeights(wavelengthNm);

    return clamp(dot(rgb, weights), 0.0f, 1.0f);
}
float rgbToEmissionAtWavelength(float3 rgb, float wavelengthNm) {
    rgb = max(rgb, 0.0f);

    float3 weights = rgbSpectralWeights(wavelengthNm);

    return max(dot(rgb, weights), 0.0f);
}

float evaluateReflectance(float3 rgb, thread const SpectralPath &path) {
    return rgbToReflectanceAtWavelength(rgb, path.wavelengthNm);
}

float evaluateEmission(float3 rgb, thread const SpectralPath &path) {
    return rgbToEmissionAtWavelength(rgb, path.wavelengthNm);
}

float evaluateIorAtWavelength(float baseIor, thread const SpectralPath &path) {
    return max(baseIor, 1.0001f);
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

float3 spectralRadianceToXYZ(float radiance, thread const SpectralPath &path) {
    if (path.wavelengthPdf <= 0.0f) {
        return float3(0.0f);
    }

    float3 xyzMatching = cieXYZ1931(path.wavelengthNm);

    return radiance * xyzMatching /
           (path.wavelengthPdf * PHOTON_CIE_Y_INTEGRAL);
}

float3 xyzToLinearSRGB(float3 xyz) {
    return float3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,

                  -0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,

                  0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
}