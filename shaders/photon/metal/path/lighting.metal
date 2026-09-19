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
#include "visibility.metal"
#include "brdf.metal"

float4 evalEmissiveTriangleLighting(
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 N, float3 Ng, float3 V, float4 albedo, float metallic,
    float roughness, float reflectivity, float4 ior, float transmittance,
    float substrateIor, float substrateAbbe, float iridescenceFactor,
    float iridescenceIor, float iridescenceAbbe,
    float iridescenceThickness, bool isFront,
    thread uint &rng, thread const SpectralPath &path,
    constant SceneData &sceneData, constant EmissiveTriangle *emissiveTriangles,
    constant Material *materials, constant uint *primitiveObjects,
    constant uint *blasPrimitiveOffsets, constant VertexData *vertices,
    constant uint *indices, constant InstanceData *instanceData,
    PT_MATERIAL_TEXTURE_PARAMS) {
    if (sceneData.numEmissiveTriangles == 0) {
        return float4(0.0);
    }
    float selector = rand(rng);
    uint first = 0;
    uint last = sceneData.numEmissiveTriangles - 1;
    while (first < last) {
        uint middle = first + (last - first) / 2;
        if (selector <= emissiveTriangles[middle].cdf) {
            last = middle;
        } else {
            first = middle + 1;
        }
    }
    EmissiveTriangle light = emissiveTriangles[first];
    float sqrtU = sqrt(rand(rng));
    float barycentricV = rand(rng);
    float b0 = 1.0 - sqrtU;
    float b1 = sqrtU * (1.0 - barycentricV);
    float b2 = sqrtU * barycentricV;
    float3 lightPosition =
        light.p0.xyz * b0 + light.p1.xyz * b1 + light.p2.xyz * b2;
    float3 toLight = lightPosition - P;
    float distanceSquared = dot(toLight, toLight);
    if (distanceSquared <= 1e-8) {
        return float4(0.0);
    }
    float distanceToLight = sqrt(distanceSquared);
    float3 L = toLight / distanceToLight;
    float surfaceCosine = dot(N, L);
    float lightCosine = abs(dot(light.normal.xyz, -L));
    if (surfaceCosine <= 0.0 || dot(Ng, L) <= 0.0 || lightCosine <= 1e-5 ||
        light.area <= 1e-8 || light.selectionPdf <= 1e-8) {
        return float4(0.0);
    }
    float solidAnglePdf = light.selectionPdf * distanceSquared /
                          max(lightCosine * light.area, 1e-8);
    float4 visibility = traceShadowVisibility(
        isect, sceneAS, P, Ng, L, distanceToLight, rng, materials,
        primitiveObjects, blasPrimitiveOffsets, vertices, indices, instanceData,
        sceneData, path, PT_MATERIAL_TEXTURE_ARGS);
    float4 lightRadiance = evaluateEmission(float3(light.emission), path);
    return evalPBR(albedo, metallic, roughness, reflectivity, ior,
                   transmittance, N, V, L, lightRadiance,
                   1.0 / max(solidAnglePdf, 1e-8), substrateIor,
                   substrateAbbe, iridescenceFactor, iridescenceIor,
                   iridescenceAbbe, iridescenceThickness, isFront, path) *
           visibility;
}

float4 evalDirectLightingPBR(
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 N, float3 Ng, float3 V, float4 albedo, float metallic,
    float roughness, float reflectivity, float4 ior, float transmittance,
    float substrateIor, float substrateAbbe, float iridescenceFactor,
    float iridescenceIor, float iridescenceAbbe,
    float iridescenceThickness, bool isFront,
    thread uint &rng, thread const SpectralPath &path,
    constant DirectionalLightData &dirLight, constant SceneData &sceneData,
    constant PointLight *pointLights, constant SpotLight *spotLights,
    constant AreaLight *areaLights,
    constant EmissiveTriangle *emissiveTriangles, constant Material *materials,
    constant uint *primitiveObjects, constant uint *blasPrimitiveOffsets,
    constant VertexData *vertices, constant uint *indices,
    constant InstanceData *instanceData, PT_MATERIAL_TEXTURE_PARAMS) {
    float4 lighting = float4(0.0);
    if (sceneData.numDirectionalLights > 0) {
        float3 L = sampleDirectionalLightDirection(dirLight, rng);
        float4 lightRadiance = evaluateEmission(dirLight.color, path);
        float4 contribution = evalPBR(
            albedo, metallic, roughness, reflectivity, ior, transmittance, N, V,
            L, lightRadiance, max(dirLight.intensity, 0.0), substrateIor,
            substrateAbbe, iridescenceFactor, iridescenceIor,
            iridescenceAbbe, iridescenceThickness, isFront, path);
        float4 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, 1e30, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += contribution * visibility;
    }

    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 toLight = float3(pointLights[i].position) - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float lightRange = max(pointLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float intensity =
            max(pointLights[i].intensity, 0.0) * rangeFade / max(distSq, 1e-4);
        float4 lightRadiance =
            evaluateEmission(float3(pointLights[i].color), path);
        float4 contribution =
            evalPBR(albedo, metallic, roughness, reflectivity, ior,
                    transmittance, N, V, L, lightRadiance, intensity,
                    substrateIor, substrateAbbe, iridescenceFactor,
                    iridescenceIor, iridescenceAbbe, iridescenceThickness,
                    isFront, path);
        float4 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += contribution * visibility;
    }

    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 toLight = float3(spotLights[i].position) - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 forward = normalize(float3(spotLights[i].direction));
        float spotCos = dot(-L, forward);
        float spot =
            smoothstep(spotLights[i].outerCos, spotLights[i].innerCos, spotCos);
        float lightRange = max(spotLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float intensity = max(spotLights[i].intensity, 0.0) * rangeFade * spot /
                          max(distSq, 1e-4);
        float4 lightRadiance =
            evaluateEmission(float3(spotLights[i].color), path);
        float4 contribution =
            evalPBR(albedo, metallic, roughness, reflectivity, ior,
                    transmittance, N, V, L, lightRadiance, intensity,
                    substrateIor, substrateAbbe, iridescenceFactor,
                    iridescenceIor, iridescenceAbbe, iridescenceThickness,
                    isFront, path);
        float4 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += contribution * visibility;
    }

    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float2 lightSample = float2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float3 sampledPosition = float3(areaLights[i].position) +
                                 float3(areaLights[i].right) *
                                     (lightSample.x * areaLights[i].halfWidth) +
                                 float3(areaLights[i].up) *
                                     (lightSample.y * areaLights[i].halfHeight);
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 lightNormal = normalize(
            cross(float3(areaLights[i].right), float3(areaLights[i].up)));
        float cosLight = areaLights[i].twoSided > 0.5
                             ? abs(dot(lightNormal, -L))
                             : max(dot(lightNormal, -L), 0.0);
        float area = 4.0 * areaLights[i].halfWidth * areaLights[i].halfHeight;
        if (cosLight < areaLights[i].emissionCos)
            continue;
        float lightPdfArea = 1.0 / max(area, 1e-6);
        float distSq = max(dist * dist, 1e-6);
        float intensity = max(areaLights[i].intensity, 0.0) * cosLight /
                          max(distSq * lightPdfArea, 1e-6);
        float4 lightRadiance =
            evaluateEmission(float3(areaLights[i].color), path);
        float4 contribution =
            evalPBR(albedo, metallic, roughness, reflectivity, ior,
                    transmittance, N, V, L, lightRadiance, intensity,
                    substrateIor, substrateAbbe, iridescenceFactor,
                    iridescenceIor, iridescenceAbbe, iridescenceThickness,
                    isFront, path);
        float4 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += contribution * visibility;
    }

    lighting += evalEmissiveTriangleLighting(
        isect, sceneAS, P, N, Ng, V, albedo, metallic, roughness, reflectivity,
        ior, transmittance, substrateIor, substrateAbbe, iridescenceFactor,
        iridescenceIor, iridescenceAbbe, iridescenceThickness, isFront, rng,
        path, sceneData, emissiveTriangles, materials,
        primitiveObjects, blasPrimitiveOffsets, vertices, indices,
        instanceData, PT_MATERIAL_TEXTURE_ARGS);

    return lighting;
}

float3 intersectAreaEmitters(ray r, float surfaceDistance,
                             constant SceneData &sceneData,
                             constant AreaLight *areaLights,
                             thread bool &foundEmitter) {
    float nearest = surfaceDistance;
    float3 emission = float3(0.0f);
    foundEmitter = false;
    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        AreaLight source = areaLights[i];
        float3 right = float3(source.right);
        float3 up = float3(source.up);
        float3 normal = normalize(cross(right, up));
        float cosine = dot(normal, r.direction);
        if (abs(cosine) < 1e-6f || (source.twoSided < 0.5f && cosine >= 0.0f))
            continue;
        if (abs(cosine) < source.emissionCos)
            continue;
        float distance =
            dot(float3(source.position) - r.origin, normal) / cosine;
        if (distance <= r.min_distance || distance >= nearest)
            continue;
        float3 offset =
            r.origin + r.direction * distance - float3(source.position);
        if (abs(dot(offset, right)) > source.halfWidth ||
            abs(dot(offset, up)) > source.halfHeight)
            continue;
        nearest = distance;
        emission = float3(source.color) * max(source.intensity, 0.0f);
        foundEmitter = true;
    }
    return emission;
}
