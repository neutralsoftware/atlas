#pragma once

#include "spectral.metal"

struct VolumeCoefficients {
    float4 sigmaA;
    float4 sigmaS;
    float4 sigmaT;
};

VolumeCoefficients
calculateVolumeCoefficients(float3 absorptionColor, float absorptionStrength,
                            float3 scatteringColor, float scatteringStrength,
                            float density,
                            thread const SpectralPath &spectralPath) {
    VolumeCoefficients result;

    float4 absorptionSpectrum =
        clamp(evaluateReflectance(absorptionColor, spectralPath), float4(1e-4f),
              float4(1.0f));

    float4 scatteringSpectrum =
        max(evaluateReflectance(scatteringColor, spectralPath), float4(0.0f));

    result.sigmaA = density * absorptionStrength * -log(absorptionSpectrum);

    result.sigmaS = density * scatteringStrength * scatteringSpectrum;

    result.sigmaT = result.sigmaA + result.sigmaS;

    return result;
}

float henyeyGreensteinPhase(float cosTheta, float g) {
    g = clamp(g, -0.99f, 0.99f);

    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;

    return (1.0f - g2) / (4.0f * M_PI_F * denom * sqrt(denom));
}

float3 sampleHenyeyGreenstein(float3 incomingDirection, float g, float2 u) {
    g = clamp(g, -0.99f, 0.99f);

    float cosTheta;

    if (abs(g) < 1e-3f) {
        cosTheta = 1.0f - 2.0f * u.x;
    } else {
        float term = (1.0f - g * g) / (1.0f - g + 2.0f * g * u.x);

        cosTheta = (1.0f + g * g - term * term) / (2.0f * g);
    }

    cosTheta = clamp(cosTheta, -1.0f, 1.0f);

    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));

    float phi = 2.0f * M_PI_F * u.y;

    float3x3 basis = buildOrthonormalBasis(normalize(incomingDirection));

    return normalize(
        basis * float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
}