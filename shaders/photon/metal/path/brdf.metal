#pragma once

#include <metal_stdlib>
using namespace metal;

#include "sampling.metal"
#include "geometry.metal"
#include "spectral.metal"
#include "iridescence.metal"

float D_GGX(float NdotH, float roughness) {
    float a = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / max(M_PI_F * d * d, 1e-6);
}

float4 F_Schlick(float cosTheta, float4 F0) {
    float c = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow5(c);
}

float4 materialF0(float4 albedo, float metallic, float reflectivity,
                  float4 ior) {
    float4 safeIor = max(ior, float4(1.0001f));

    float4 dielectricF0 =
        pow((safeIor - 1.0f) / (safeIor + 1.0f), float4(2.0f));

    return mix(dielectricF0, albedo, float4(clamp(metallic, 0.0f, 1.0f)));
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

float G1_SmithGGX(float NdotX, float roughness) {
    float alpha = max(roughness * roughness, 1e-4);
    float alphaSquared = alpha * alpha;
    float cosineSquared = NdotX * NdotX;
    return (2.0 * NdotX) /
           max(NdotX +
                   sqrt(alphaSquared + (1.0 - alphaSquared) * cosineSquared),
               1e-6);
}

float disneyDiffuseFactor(float NdotV, float NdotL, float LdotH,
                          float roughness) {
    float fd90 = 0.5 + 2.0 * LdotH * LdotH * roughness;
    float lightScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotL);
    float viewScatter = 1.0 + (fd90 - 1.0) * pow5(1.0 - NdotV);
    return lightScatter * viewScatter;
}

// GGX importance-sampled microfacet half-vector (in local TBN space, Z=up)
float3 sampleGGX(float2 u, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * M_PI_F * u.x;
    float cosTheta = sqrt((1.0 - u.y) / max(1.0 + (a * a - 1.0) * u.y, 1e-7));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 sampleGGXVNDF(float3 localView, float roughness, float2 u) {
    float alpha = max(roughness * roughness, 1e-3);
    float3 stretchedView =
        normalizeOr(float3(alpha * localView.x, alpha * localView.y,
                           max(localView.z, 1e-5)),
                    float3(0.0, 0.0, 1.0));
    float lensq =
        stretchedView.x * stretchedView.x + stretchedView.y * stretchedView.y;
    float3 tangentX =
        lensq > 1e-7
            ? float3(-stretchedView.y, stretchedView.x, 0.0) * rsqrt(lensq)
            : float3(1.0, 0.0, 0.0);
    float3 tangentY = cross(stretchedView, tangentX);
    float radius = sqrt(u.x);
    float phi = 2.0 * M_PI_F * u.y;
    float diskX = radius * cos(phi);
    float diskY = radius * sin(phi);
    float blend = 0.5 * (1.0 + stretchedView.z);
    diskY = mix(sqrt(max(0.0, 1.0 - diskX * diskX)), diskY, blend);
    float diskZ = sqrt(max(0.0, 1.0 - diskX * diskX - diskY * diskY));
    float3 visibleNormal =
        diskX * tangentX + diskY * tangentY + diskZ * stretchedView;
    return normalizeOr(float3(alpha * visibleNormal.x, alpha * visibleNormal.y,
                              max(visibleNormal.z, 0.0)),
                       float3(0.0, 0.0, 1.0));
}

// Full Cook-Torrance PBR for a single analytic light
float4 evalPBR(float4 albedo, float metallic, float roughness,
               float reflectivity, float4 ior, float transmittance, float3 N,
               float3 V, float3 L, float4 lightRadiance, float intensity) {
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float clampedRoughness = clamp(roughness, 0.045, 1.0);
    float4 F0 = materialF0(albedo, metallic, reflectivity, ior);
    float4 F = F_Schlick(VdotH, F0);
    float D = D_GGX(NdotH, clampedRoughness);
    float G = G1_SmithGGX(NdotV, clampedRoughness) *
              G1_SmithGGX(NdotL, clampedRoughness);

    float4 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
    float4 kD = (1.0 - F) * (1.0 - clamp(metallic, 0.0, 1.0)) *
                (1.0 - clamp(transmittance, 0.0, 1.0));
    float diffuseFactor = disneyDiffuseFactor(NdotV, NdotL, max(dot(L, H), 0.0),
                                              clampedRoughness);
    float4 diffuse = (kD * albedo * diffuseFactor) / M_PI_F;

    return (diffuse + specular) * lightRadiance * intensity * NdotL;
}

float4 evalTransmission(float4 albedo, float3 N, float3 V, float3 L,
                        float4 lightRadiance, float intensity, float roughness,
                        float ior) {
    float backLighting = max(dot(N, -L), 0.0);
    float forwardAlignment = max(dot(-V, L), 0.0);
    float F0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
    float4 F = F_Schlick(max(dot(N, V), 0.0), float4(F0));
    float lobeExponent = mix(96.0, 2.0, sqrt(clamp(roughness, 0.0, 1.0)));
    float transmissionLobe = pow(forwardAlignment, lobeExponent);
    return (1.0 - F) * lightRadiance * intensity * backLighting *
           transmissionLobe;
}

float4 evalIridescence(float cosTheta, float4 ordinaryFresnel,
                       float incidentIor, float substrateIor,
                       float substrateAbbe, float iridescenceFactor,
                       float iridescenceIor, float iridescenceAbbe,
                       float iridescenceThickness, bool isFront,
                       thread const SpectralPath &spectralPath) {
    if (clamp(iridescenceFactor, 0.0f, 1.0f) <= 0.0f) {
        return ordinaryFresnel;
    }

    float2 n0 = float2(incidentIor, 0.0);
    float2 n1 = float2(iridescenceIor, iridescenceAbbe);
    float2 n2 = float2(substrateIor, substrateAbbe);

    if (!isFront) {
        n0 = float2(substrateIor, substrateAbbe);
        n1 = float2(iridescenceIor, iridescenceAbbe);
        n2 = float2(incidentIor, 0.0);
    }

    float4 iridescenceFresnel =
        thinFilmFresnel(cosTheta, n0.x, n0.y, n1.x, n1.y, n2.x, n2.y,
                        iridescenceThickness, spectralPath);
    return mix(ordinaryFresnel, iridescenceFresnel,
               float4(clamp(iridescenceFactor, 0.0f, 1.0f)));
}