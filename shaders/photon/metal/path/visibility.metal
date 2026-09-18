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

    for (uint alphaStep = 0; alphaStep < 32; ++alphaStep) {
        auto shadowHit = isect.intersect(shadowRay, sceneAS);
        if (shadowHit.type == intersection_type::none) {
            return visibility;
        }

        uint primitiveIndex = blasPrimitiveOffsets[shadowHit.geometry_id] +
                              shadowHit.primitive_id;
        uint objectIndex = primitiveObjects[primitiveIndex];
        Material material = materials[objectIndex];
        uint i0 = indices[primitiveIndex * 3 + 0];
        uint i1 = indices[primitiveIndex * 3 + 1];
        uint i2 = indices[primitiveIndex * 3 + 2];
        float2 bary = shadowHit.triangle_barycentric_coord;
        float b0 = 1.0 - bary.x - bary.y;
        float2 uv = float2(vertices[i0].uv) * b0 +
                    float2(vertices[i1].uv) * bary.x +
                    float2(vertices[i2].uv) * bary.y;
        uv =
            uv * float2(material.textureScale) + float2(material.textureOffset);
        float opacity =
            resolveMaterialOpacity(material, uv, sceneData.materialTextureCount,
                                   PT_MATERIAL_TEXTURE_ARGS);
        if (opacity >= 0.999 || rand(rng) < opacity) {
            float3 albedoRgb;
            float metallic;
            float roughness;
            float ao;
            float3 emissiveRgb;
            float baseIor;
            float transmittance;
            float abbeNumber;

            resolveMaterialParameters(
                material, uv, sceneData.materialTextureCount,
                PT_MATERIAL_TEXTURE_ARGS, albedoRgb, metallic, roughness, ao,
                emissiveRgb, baseIor, transmittance, abbeNumber);

            float transmission = transmittance * (1.0f - metallic);
            if (roughness <= 0.025f && transmission > 0.001f) {
                return float4(0.0f);
            }

            if (transmission <= 0.001f) {
                return float4(0.0f);
            }

            float3 p0 = float3(vertices[i0].position);
            float3 p1 = float3(vertices[i1].position);
            float3 p2 = float3(vertices[i2].position);

            float3 hitNormal =
                normalizeOr(normalizeOr(cross(p1 - p0, p2 - p0), -L), -L);
            hitNormal = dot(hitNormal, L) < 0.0f ? hitNormal : -hitNormal;

            float4 ior = evaluateIorAtWavelength(baseIor, abbeNumber, path);
            float4 dielectricF0 = pow((ior - 1.0f) / (ior + 1.0f), 2.0f);
            float cosTheta = abs(dot(hitNormal, L));
            float schlick = pow5(1.0f - cosTheta);
            float4 fresnel = dielectricF0 + (1.0f - dielectricF0) * schlick;

            float4 tint =
                mix(float4(1.0f), evaluateReflectance(albedoRgb, path), 0.15f);

            visibility *= tint * transmission * (1.0f - fresnel);

            if (spectralMax(visibility) <= 0.001f) {
                return float4(0.0f);
            }
        }

        float advance =
            shadowHit.distance + rayOffsetDistance(shadowRay.origin);
        shadowRay.origin += shadowRay.direction * advance;
        shadowRay.max_distance -= advance;
        if (shadowRay.max_distance <= shadowBias) {
            return visibility;
        }
    }

    return float4(0.0);
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
