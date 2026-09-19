#pragma once

#include <metal_stdlib>
using namespace metal;

#include "spectral.metal"

float4 refractCosTheta(float4 n0, float4 n1, float4 cosTheta0) {
    float4 sinTheta0 = sqrt(max(float4(0.0f), 1.0f - cosTheta0 * cosTheta0));

    float4 sinTheta1 = (n0 / n1) * sinTheta0;

    return sqrt(max(float4(0.0f), 1.0f - sinTheta1 * sinTheta1));
}

float4 thinFilmPhase(float4 cosThetaIncident, float4 filmIor,
                     float4 filmThickness,
                     const thread SpectralPath &spectralPath) {
    float4 phaseDifference = 4.0 * M_PI_F * filmIor * filmThickness *
                             cosThetaIncident / spectralPath.wavelengthNm;

    return phaseDifference;
}

struct PolarizedFresnel {
    float4 s;
    float4 p;
};

PolarizedFresnel calculateSPPolarizedFresnel(float4 n0, float4 n1,
                                             float4 cosTheta0,
                                             float4 cosTheta1) {
    float4 rS =
        (n0 * cosTheta0 - n1 * cosTheta1) / (n0 * cosTheta0 + n1 * cosTheta1);

    float4 rP =
        (n1 * cosTheta0 - n0 * cosTheta1) / (n1 * cosTheta0 + n0 * cosTheta1);

    return {rS, rP};
}

float4 interferenceReflectance(float4 ra, float4 rb, float4 phase) {
    float4 numerator = ra * ra + rb * rb + 2.0 * ra * rb * cos(phase);
    float4 denominator = 1.0 + ra * ra * rb * rb + 2.0 * ra * rb * cos(phase);
    return numerator / denominator;
}

float4 thinFilmFresnel(float cosThetaIncident, float incidentIor,
                       float incidentAbbe, float filmIor, float filmAbbe,
                       float substrateIor, float subtrateAbbe,
                       float filmThickness,
                       thread const SpectralPath &spectralPath) {
    float4 cosTheta0 = float4(cosThetaIncident);

    float4 incidentIorWaved =
        evaluateIorAtWavelength(incidentIor, incidentAbbe, spectralPath);
    float4 filmIorWaved =
        evaluateIorAtWavelength(filmIor, filmAbbe, spectralPath);
    float4 substrateIorWaved =
        evaluateIorAtWavelength(substrateIor, subtrateAbbe, spectralPath);

    float4 cosThetaFilm =
        refractCosTheta(incidentIorWaved, filmIorWaved, cosTheta0);
    float4 cosThetaSubstrate =
        refractCosTheta(filmIorWaved, substrateIorWaved, cosThetaFilm);

    float4 phaseDifference =
        thinFilmPhase(cosThetaFilm, filmIorWaved, filmThickness, spectralPath);

    PolarizedFresnel r01 = calculateSPPolarizedFresnel(
        incidentIorWaved, filmIorWaved, cosTheta0, cosThetaFilm);
    PolarizedFresnel r12 = calculateSPPolarizedFresnel(
        filmIorWaved, substrateIorWaved, cosThetaFilm, cosThetaSubstrate);

    float4 RS = interferenceReflectance(r01.s, r12.s, phaseDifference);
    float4 RP = interferenceReflectance(r01.p, r12.p, phaseDifference);

    return 0.5 * (RS + RP);
}
