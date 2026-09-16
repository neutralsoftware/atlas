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
    float causticGain = 1.0;
    float3 entryNormal = float3(0.0);
    uint dielectricObject = 0xFFFFFFFFu;
    ray shadowRay;
    shadowRay.origin = offsetRayOrigin(P, Ng, L);
    shadowRay.direction = L;
    shadowRay.min_distance = 0.0;
    shadowRay.max_distance = max(maxDistance - shadowBias, shadowBias + 1e-4);

    for (uint alphaStep = 0; alphaStep < 32; ++alphaStep) {
        auto shadowHit = isect.intersect(shadowRay, sceneAS);
        if (shadowHit.type == intersection_type::none) {
            return min(visibility * causticGain, float4(2.5));
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
            resolveMaterialParameters(
                material, uv, sceneData.materialTextureCount,
                PT_MATERIAL_TEXTURE_ARGS, albedoRgb, metallic, roughness, ao,
                emissiveRgb, baseIor, transmittance);
            float transmission = transmittance * (1.0 - metallic);
            if (transmission <= 0.001) {
                return float4(0.0);
            }

            float3 p0 = float3(vertices[i0].position);
            float3 p1 = float3(vertices[i1].position);
            float3 p2 = float3(vertices[i2].position);
            float3 hitNormal = normalizeOr(cross(p1 - p0, p2 - p0), -L);
            hitNormal = dot(hitNormal, L) < 0.0 ? hitNormal : -hitNormal;
            float ior = evaluateIorAtWavelength(baseIor, path);
            float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
            float fresnel =
                dielectricF0 +
                (1.0 - dielectricF0) * pow5(1.0 - abs(dot(hitNormal, L)));
            float4 tint =
                mix(float4(1.0), evaluateReflectance(albedoRgb, path), 0.15);
            visibility *= tint * transmission * (1.0 - fresnel);
            if (dielectricObject == objectIndex) {
                float curvature =
                    1.0 - clamp(abs(dot(entryNormal, hitNormal)), 0.0, 1.0);
                float smoothness = 1.0 - roughness;
                float focus = 1.0 + transmission * max(ior - 1.0, 0.0) *
                                        smoothness * smoothness *
                                        (0.35 + curvature * 3.0);
                causticGain *= clamp(focus, 1.0, 2.5);
                dielectricObject = 0xFFFFFFFFu;
            } else {
                dielectricObject = objectIndex;
                entryNormal = hitNormal;
            }
            if (spectralMax(visibility) <= 0.001) {
                return float4(0.0);
            }
        }

        float advance =
            shadowHit.distance + rayOffsetDistance(shadowRay.origin);
        shadowRay.origin += shadowRay.direction * advance;
        shadowRay.max_distance -= advance;
        if (shadowRay.max_distance <= shadowBias) {
            return min(visibility * causticGain, float4(2.5));
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
