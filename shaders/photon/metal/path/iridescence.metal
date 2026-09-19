#pragma once

#include <metal_stdlib>
using namespace metal;

#include "spectral.metal"

float refractAngle(float n0, float n1, float cosTheta0) {
    float sinTheta0 = sqrt(max(0.0, 1.0 - cosTheta0 * cosTheta0));
    float sinTheta1 = n0 / n1 * sinTheta0;

    if (sinTheta1 >= 1.0) {
        return 0.0;
    }

    float cosTheta1 = sqrt(max(0.0, 1.0 - sinTheta1 * sinTheta1));

    return cosTheta1;
}

float4 thinFilmPhase(float cosThetaIncident, float incidentIor, float filmIor,
                     float substrateIor, float filmThickness,
                     const thread SpectralPath &spectralPath) {
    float4 phaseDifference = 4.0 * M_PI_F * filmIor * filmThickness *
                             cosThetaIncident / spectralPath.wavelengthNm;

    return phaseDifference;
}

float2 calculateSPPolarizedFresnel(float n0, float n1, float cosTheta0,
                                   float cosTheta1) {
    float rS =
        (n0 * cosTheta0 - n1 * cosTheta1) / (n0 * cosTheta0 + n1 * cosTheta1);
    float rP =
        (n1 * cosTheta0 - n0 * cosTheta1) / (n1 * cosTheta0 + n0 * cosTheta1);

    return float2(rS, rP);
}

float4 thinFilmFresnel(float cosThetaIncident, float incidentIor, float filmIor,
                       float substrateIor, float filmThickness,
                       const thread SpectralPath &spectralPath) {
    float4 phaseDifference =
        thinFilmPhase(cosThetaIncident, incidentIor, filmIor, substrateIor,
                      filmThickness, spectralPath);

    float cosThetaFilm = refractAngle(incidentIor, filmIor, cosThetaIncident);
    float cosThetaSubstrate = refractAngle(filmIor, substrateIor, cosThetaFilm);

    float2 r1 = calculateSPPolarizedFresnel(incidentIor, filmIor,
                                            cosThetaIncident, cosThetaFilm);
    float2 r2 = calculateSPPolarizedFresnel(filmIor, substrateIor, cosThetaFilm,
                                            cosThetaSubstrate);

    float4 RSTop =
        (r1.x * r1.x + r1.y * r1.y) + 2.0 * r1.x * r1.y * cos(phaseDifference);
    float4 RSBottom = 1.0 + (r1.x * r1.x) * (r1.y * r1.y) +
                      2.0 * r1.x * r1.y * cos(phaseDifference);
    float4 RS = RSTop / RSBottom;

    float4 RPTTop =
        (r2.x * r2.x + r2.y * r2.y) + 2.0 * r2.x * r2.y * cos(phaseDifference);
    float4 RPTBottom = 1.0 + (r2.x * r2.x) * (r2.y * r2.y) +
                       2.0 * r2.x * r2.y * cos(phaseDifference);
    float4 RP = RPTTop / RPTBottom;

    return 0.5 * (RS + RP);
}
