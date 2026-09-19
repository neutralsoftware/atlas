#pragma once

#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

#include "types.metal"
#include "sampling.metal"
#include "spectral.metal"
#include "geometry.metal"
#include "materials.metal"

float4 traceShadowVisibility(
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 Ng, float3 L, float maxDistance, thread uint &rng,
    constant Material *materials, constant uint *primitiveObjects,
    constant uint *blasPrimitiveOffsets, constant VertexData *vertices,
    constant uint *indices, constant InstanceData *instanceData,
    constant SceneData &sceneData, thread const SpectralPath &path,
    PT_MATERIAL_TEXTURE_PARAMS) {
    float shadowBias = rayOffsetDistance(P);
    float4 visibility = float4(1.0);
    ray shadowRay;
    shadowRay.origin = offsetRayOrigin(P, Ng, L);
    shadowRay.direction = L;
    shadowRay.min_distance = 0.0;
    shadowRay.max_distance = max(maxDistance - shadowBias, shadowBias + 1e-4);
    uint mediumObject = 0xFFFFFFFFu;
    uint volumeObject = 0xFFFFFFFFu;
    float4 volumeSigmaA = float4(0.0f);

    for (uint alphaStep = 0; alphaStep < 32; ++alphaStep) {
        auto shadowHit = isect.intersect(shadowRay, sceneAS);

        if (shadowHit.type == intersection_type::none)
            return visibility;

        if (volumeObject != 0xFFFFFFFFu)
            visibility *= exp(-volumeSigmaA * shadowHit.distance);

        uint primitiveIndex = blasPrimitiveOffsets[shadowHit.geometry_id] +
                              shadowHit.primitive_id;

        uint objectIndex = primitiveObjects[primitiveIndex];
        Material material = materials[objectIndex];

        uint i0 = indices[primitiveIndex * 3 + 0];
        uint i1 = indices[primitiveIndex * 3 + 1];
        uint i2 = indices[primitiveIndex * 3 + 2];

        float2 bary = shadowHit.triangle_barycentric_coord;
        float b0 = 1.0f - bary.x - bary.y;

        float2 uv = float2(vertices[i0].uv) * b0 +
                    float2(vertices[i1].uv) * bary.x +
                    float2(vertices[i2].uv) * bary.y;

        uv =
            uv * float2(material.textureScale) + float2(material.textureOffset);

        float opacity = resolveMaterialOpacity(
            material, uv, sceneData.materialTextureCount,
            PT_MATERIAL_TEXTURE_ARGS);

        if (material.isVolume == 0 && opacity < 0.999f &&
            rand(rng) >= opacity) {
            float advance =
                shadowHit.distance + rayOffsetDistance(shadowRay.origin);
            shadowRay.origin += shadowRay.direction * advance;
            shadowRay.max_distance -= advance;
            if (shadowRay.max_distance <= shadowBias)
                return visibility;
            continue;
        }

        if (material.isVolume != 0) {
            if (volumeObject == objectIndex) {
                volumeObject = 0xFFFFFFFFu;
                volumeSigmaA = float4(0.0f);
            } else {
                float3 absorptionColor = clamp(
                    float3(material.volumeAbsorptionColor), float3(0.001f),
                    float3(1.0f));
                float density = max(material.volumeDensity, 0.0f) *
                                max(material.volumeAbsorptionStrength, 0.0f);
                volumeSigmaA =
                    -log(clamp(evaluateReflectance(absorptionColor, path),
                               float4(0.001f), float4(1.0f))) *
                    density;
                volumeObject = objectIndex;
            }
        } else {
            float transmission =
                clamp(material.transmittance, 0.0f, 1.0f) *
                (1.0f - clamp(material.metallic, 0.0f, 1.0f));
            if (transmission <= 0.001f)
                return float4(0.0f);

            float3 p0 = float3(vertices[i0].position);
            float3 p1 = float3(vertices[i1].position);
            float3 p2 = float3(vertices[i2].position);
            float3 localNormal =
                normalizeOr(cross(p1 - p0, p2 - p0), float3(0.0f, 1.0f, 0.0f));
            InstanceData instance = instanceData[objectIndex];
            float3x3 normalMatrix = float3x3(
                instance.normalCol0.xyz, instance.normalCol1.xyz,
                instance.normalCol2.xyz);
            float3 geometricNormal = normalizeOr(
                normalMatrix * localNormal, float3(0.0f, 1.0f, 0.0f));
            bool frontFace = dot(geometricNormal, shadowRay.direction) < 0.0f;
            float cosine = abs(dot(geometricNormal, shadowRay.direction));
            float4 ior = evaluateIorAtWavelength(
                max(material.ior, 1.0001f), max(material.abbeNumber, 0.0f),
                path);
            float4 eta = frontFace ? 1.0f / ior : ior;
            visibility *=
                (1.0f - dielectricFresnel(cosine, eta)) * transmission;

            if (frontFace) {
                mediumObject = objectIndex;
            } else if (mediumObject == objectIndex &&
                       material.attenuationDistance > 0.001f) {
                float3 attenuationColor =
                    clamp(float3(material.attenuationColor), float3(0.001f),
                          float3(1.0f));
                float4 spectralAttenuation =
                    clamp(evaluateReflectance(attenuationColor, path),
                          float4(0.001f), float4(1.0f));
                visibility *= pow(
                    spectralAttenuation,
                    float4(shadowHit.distance /
                           max(material.attenuationDistance, 0.001f)));
                mediumObject = 0xFFFFFFFFu;
            }
        }

        if (spectralMax(visibility) <= 1e-5f)
            return float4(0.0f);

        float advance =
            shadowHit.distance + rayOffsetDistance(shadowRay.origin);

        shadowRay.origin += shadowRay.direction * advance;
        shadowRay.max_distance -= advance;

        if (shadowRay.max_distance <= shadowBias)
            return visibility;
    }

    return float4(0.0f);
}

float3 sampleDirectionalLightDirection(DirectionalLightData light,
                                       thread uint &rng) {
    float3 baseL = normalize(-light.direction);
    float3x3 basis = buildOrthonormalBasis(baseL);
    float sunRadius = 0.0025;
    float2 u = float2(rand(rng), rand(rng));
    float r = sunRadius * sqrt(u.x);
    float phi = 2.0 * M_PI_F * u.y;
    float3 jittered =
        baseL + basis[0] * (r * cos(phi)) + basis[1] * (r * sin(phi));
    return normalize(jittered);
}
