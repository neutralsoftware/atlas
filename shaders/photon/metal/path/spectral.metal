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
constant uint PHOTON_WAVELENGTH_CDF_BIN_COUNT = 32;
constant float PHOTON_WAVELENGTH_CDF[33] = {
    0.000000000f, 0.013349540f, 0.026769640f, 0.040377409f, 0.054438285f,
    0.069490706f, 0.086506470f, 0.107052655f, 0.133634668f, 0.171206903f,
    0.227843254f, 0.304508651f, 0.390584771f, 0.479164775f, 0.564236129f,
    0.640360711f, 0.703557178f, 0.752611910f, 0.788882128f, 0.815347485f,
    0.835360285f, 0.851703036f, 0.866231865f, 0.879978311f, 0.893429295f,
    0.906782339f, 0.920106833f, 0.933423997f, 0.946739502f, 0.960054675f,
    0.973369790f, 0.986684896f, 1.000000000f};

struct SpectralPath {
    float4 wavelengthNm;
    float4 wavelengthPdf;
    float4 throughput;
    float4 radiance;

    uint heroIndex;
};

float2 sampleVisibleWavelength(float u) {
    uint low = 0;
    uint high = PHOTON_WAVELENGTH_CDF_BIN_COUNT;
    for (uint iteration = 0; iteration < 5; ++iteration) {
        uint middle = (low + high) / 2;
        if (u < PHOTON_WAVELENGTH_CDF[middle]) {
            high = middle;
        } else {
            low = middle;
        }
    }
    uint bin = min(low, PHOTON_WAVELENGTH_CDF_BIN_COUNT - 1);
    float cdfMinimum = PHOTON_WAVELENGTH_CDF[bin];
    float cdfMaximum = PHOTON_WAVELENGTH_CDF[bin + 1];
    float binProbability = max(cdfMaximum - cdfMinimum, 1e-8f);
    float binPosition = clamp((u - cdfMinimum) / binProbability, 0.0f, 1.0f);
    float binWidth =
        PHOTON_LAMBDA_RANGE_NM / float(PHOTON_WAVELENGTH_CDF_BIN_COUNT);
    float wavelength =
        PHOTON_LAMBDA_MIN_NM + (float(bin) + binPosition) * binWidth;
    return float2(wavelength, binProbability / binWidth);
}

SpectralPath createSpectralPath(thread uint &rng) {
    SpectralPath path;
    float4 strata = float4(0.0, 1.0, 2.0, 3.0);
    float4 u = (strata + float4(rand(rng))) / float(PHOTON_SPECTRAL_LANE_COUNT);
    float2 sample0 = sampleVisibleWavelength(u.x);
    float2 sample1 = sampleVisibleWavelength(u.y);
    float2 sample2 = sampleVisibleWavelength(u.z);
    float2 sample3 = sampleVisibleWavelength(u.w);
    path.wavelengthNm = float4(sample0.x, sample1.x, sample2.x, sample3.x);
    path.wavelengthPdf = float4(sample0.y, sample1.y, sample2.y, sample3.y);
    path.throughput = float4(1.0);
    path.radiance = float4(0.0);

    path.heroIndex = min(uint(rand(rng) * float(PHOTON_SPECTRAL_LANE_COUNT)),
                         PHOTON_SPECTRAL_LANE_COUNT - 1);
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
    return float4(rgbToReflectanceAtWavelength(rgb, path.wavelengthNm.x),
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

float4 evaluateIorAtWavelength(float nd, float abbe,
                               thread const SpectralPath &path) {
    if (abbe <= 0.0f)
        return float4(nd);

    constexpr float lambdaF = 0.4861327f;
    constexpr float lambdaD = 0.5875618f;
    constexpr float lambdaC = 0.6562725f;

    float invF2 = 1.0f / (lambdaF * lambdaF);
    float invD2 = 1.0f / (lambdaD * lambdaD);
    float invC2 = 1.0f / (lambdaC * lambdaC);

    float deltaFC = (nd - 1.0f) / max(abbe, 1e-4f);

    float B = deltaFC / (invF2 - invC2);

    float A = nd - B * invD2;

    float4 lambdaUm =
        clamp(path.wavelengthNm, float4(380.0f), float4(830.0f)) * 0.001f;

    return max(A + B / (lambdaUm * lambdaUm), float4(1.0001f));
}

float spectralAverage(float4 spectrum) { return dot(spectrum, float4(0.25)); }

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

float3 spectralRadianceToXYZ(float4 radiance, thread const SpectralPath &path) {
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
    float3 rgb =
        float3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
               -0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
               0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
    return rgb / float3(1.1994129f, 0.9511085f, 0.9081339f);
}

float4 dielectricFresnel(float cosine, float4 eta) {
    float4 sinSquared = eta * eta * max(0.0f, 1.0f - cosine * cosine);
    float4 transmittedCosine = sqrt(max(float4(0.0f), 1.0f - sinSquared));
    float4 parallel = (cosine - eta * transmittedCosine) /
                      max(cosine + eta * transmittedCosine, float4(1e-7f));
    float4 perpendicular = (eta * cosine - transmittedCosine) /
                           max(eta * cosine + transmittedCosine, float4(1e-7f));
    return select(0.5f * (parallel * parallel + perpendicular * perpendicular),
                  float4(1.0f), sinSquared >= 1.0f);
}
