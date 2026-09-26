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
#include "volumes.metal"

float4 evalSubsurfaceDirectLighting(
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 incomingDirection, float anisotropy, uint boundaryObject,
    float4 sigmaT,
    thread uint &rng, thread const SpectralPath &path,
    constant DirectionalLightData &dirLight, constant SceneData &sceneData,
    constant PointLight *pointLights, constant SpotLight *spotLights,
    constant AreaLight *areaLights, constant Material *materials,
    constant uint *primitiveObjects, constant uint *blasPrimitiveOffsets,
    constant VertexData *vertices, constant uint *indices,
    constant InstanceData *instanceData, PT_MATERIAL_TEXTURE_PARAMS) {
    float4 lighting = float4(0.0f);
    if (sceneData.numDirectionalLights > 0) {
        float3 L = sampleDirectionalLightDirection(dirLight, rng);
        float phase = henyeyGreensteinPhase(dot(incomingDirection, L),
                                            anisotropy);
        float4 visibility = traceSubsurfaceVisibility(
            isect, sceneAS, P, L, 1.0e30f, boundaryObject, sigmaT, rng,
            materials,
            primitiveObjects, blasPrimitiveOffsets, vertices, indices,
            instanceData, sceneData, path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += evaluateEmission(dirLight.color, path) *
                    max(dirLight.intensity, 0.0f) * phase * visibility;
    }

    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 toLight = float3(pointLights[i].position) - P;
        float distance = max(length(toLight), 1e-4f);
        float3 L = toLight / distance;
        float range = max(pointLights[i].range, 1e-4f);
        float minimumDistance = max(range * 0.08f, 0.15f);
        float rangeFade = 1.0f - smoothstep(range * 0.75f, range, distance);
        float intensity = max(pointLights[i].intensity, 0.0f) * rangeFade /
                          max(distance * distance +
                                  minimumDistance * minimumDistance,
                              1e-4f);
        float phase = henyeyGreensteinPhase(dot(incomingDirection, L),
                                            anisotropy);
        float4 visibility = traceSubsurfaceVisibility(
            isect, sceneAS, P, L, distance, boundaryObject, sigmaT, rng,
            materials,
            primitiveObjects, blasPrimitiveOffsets, vertices, indices,
            instanceData, sceneData, path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += evaluateEmission(float3(pointLights[i].color), path) *
                    intensity * phase * visibility;
    }

    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 toLight = float3(spotLights[i].position) - P;
        float distance = max(length(toLight), 1e-4f);
        float3 L = toLight / distance;
        float range = max(spotLights[i].range, 1e-4f);
        float minimumDistance = max(range * 0.08f, 0.15f);
        float spotCosine =
            dot(-L, normalize(float3(spotLights[i].direction)));
        float spot = smoothstep(spotLights[i].outerCos,
                                spotLights[i].innerCos, spotCosine);
        float rangeFade = 1.0f - smoothstep(range * 0.75f, range, distance);
        float intensity = max(spotLights[i].intensity, 0.0f) * spot *
                          rangeFade /
                          max(distance * distance +
                                  minimumDistance * minimumDistance,
                              1e-4f);
        float phase = henyeyGreensteinPhase(dot(incomingDirection, L),
                                            anisotropy);
        float4 visibility = traceSubsurfaceVisibility(
            isect, sceneAS, P, L, distance, boundaryObject, sigmaT, rng,
            materials,
            primitiveObjects, blasPrimitiveOffsets, vertices, indices,
            instanceData, sceneData, path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += evaluateEmission(float3(spotLights[i].color), path) *
                    intensity * phase * visibility;
    }

    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float2 lightSample = float2(rand(rng), rand(rng)) * 2.0f - 1.0f;
        float3 lightPosition =
            float3(areaLights[i].position) +
            float3(areaLights[i].right) *
                (lightSample.x * areaLights[i].halfWidth) +
            float3(areaLights[i].up) *
                (lightSample.y * areaLights[i].halfHeight);
        float3 toLight = lightPosition - P;
        float distance = max(length(toLight), 1e-4f);
        float3 L = toLight / distance;
        float3 lightNormal = normalize(
            cross(float3(areaLights[i].right), float3(areaLights[i].up)));
        float lightCosine = areaLights[i].twoSided > 0.5f
                                ? abs(dot(lightNormal, -L))
                                : max(dot(lightNormal, -L), 0.0f);
        if (lightCosine < areaLights[i].emissionCos)
            continue;
        float area = 4.0f * areaLights[i].halfWidth * areaLights[i].halfHeight;
        float intensity = max(areaLights[i].intensity, 0.0f) * lightCosine *
                          area / max(distance * distance, 1e-6f);
        float phase = henyeyGreensteinPhase(dot(incomingDirection, L),
                                            anisotropy);
        float4 visibility = traceSubsurfaceVisibility(
            isect, sceneAS, P, L, distance, boundaryObject, sigmaT, rng,
            materials,
            primitiveObjects, blasPrimitiveOffsets, vertices, indices,
            instanceData, sceneData, path, PT_MATERIAL_TEXTURE_ARGS);
        lighting += evaluateEmission(float3(areaLights[i].color), path) *
                    intensity * phase * visibility;
    }
    return lighting;
}

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

uint directLightCandidateCount(constant SceneData &sceneData) {
    return sceneData.numDirectionalLights + sceneData.numPointLights +
           sceneData.numSpotLights + sceneData.numAreaLights +
           (sceneData.numEmissiveTriangles > 0 ? 1u : 0u);
}

DirectLightSample sampleDirectLight(
    thread uint &rng, constant SceneData &sceneData,
    constant EmissiveTriangle *emissiveTriangles) {
    DirectLightSample sample{};
    uint candidateCount = directLightCandidateCount(sceneData);
    if (candidateCount == 0) {
        sample.type = 0xFFFFFFFFu;
        return sample;
    }
    uint selected = min(uint(rand(rng) * float(candidateCount)),
                        candidateCount - 1);
    sample.uv = float2(rand(rng), rand(rng));
    if (selected < sceneData.numDirectionalLights) {
        sample.type = 0;
        sample.index = selected;
        return sample;
    }
    selected -= sceneData.numDirectionalLights;
    if (selected < sceneData.numPointLights) {
        sample.type = 1;
        sample.index = selected;
        return sample;
    }
    selected -= sceneData.numPointLights;
    if (selected < sceneData.numSpotLights) {
        sample.type = 2;
        sample.index = selected;
        return sample;
    }
    selected -= sceneData.numSpotLights;
    if (selected < sceneData.numAreaLights) {
        sample.type = 3;
        sample.index = selected;
        return sample;
    }
    sample.type = 4;
    uint first = 0;
    uint last = sceneData.numEmissiveTriangles - 1;
    float selector = sample.uv.x;
    while (first < last) {
        uint middle = first + (last - first) / 2;
        if (selector <= emissiveTriangles[middle].cdf) {
            last = middle;
        } else {
            first = middle + 1;
        }
    }
    sample.index = first;
    sample.uv.x = rand(rng);
    return sample;
}

float4 evaluateDirectLightSample(
    DirectLightSample lightSample, bool includeVisibility,
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 N, float3 Ng, float3 V, float4 albedo, float metallic,
    float roughness, float reflectivity, float4 ior, float transmittance,
    float substrateIor, float substrateAbbe, float iridescenceFactor,
    float iridescenceIor, float iridescenceAbbe,
    float iridescenceThickness, bool isFront, thread uint &rng,
    thread const SpectralPath &path, constant DirectionalLightData &dirLight,
    constant SceneData &sceneData, constant PointLight *pointLights,
    constant SpotLight *spotLights, constant AreaLight *areaLights,
    constant EmissiveTriangle *emissiveTriangles, constant Material *materials,
    constant uint *primitiveObjects, constant uint *blasPrimitiveOffsets,
    constant VertexData *vertices, constant uint *indices,
    constant InstanceData *instanceData, PT_MATERIAL_TEXTURE_PARAMS) {
    float3 L = float3(0.0f);
    float distanceToLight = 1.0e30f;
    float4 lightRadiance = float4(0.0f);
    float intensity = 0.0f;
    bool areaContribution = false;

    if (lightSample.type == 0 && sceneData.numDirectionalLights > 0) {
        L = sampleDirectionalLightDirection(dirLight, lightSample.uv);
        lightRadiance = evaluateEmission(dirLight.color, path);
        intensity = max(dirLight.intensity, 0.0f);
    } else if (lightSample.type == 1 &&
               lightSample.index < sceneData.numPointLights) {
        PointLight light = pointLights[lightSample.index];
        float3 toLight = float3(light.position) - P;
        distanceToLight = max(length(toLight), 1e-4f);
        L = toLight / distanceToLight;
        float range = max(light.range, 1e-4f);
        float minimumDistance = max(range * 0.08f, 0.15f);
        float rangeFade =
            1.0f - smoothstep(range * 0.75f, range, distanceToLight);
        intensity = max(light.intensity, 0.0f) * rangeFade /
                    max(distanceToLight * distanceToLight +
                            minimumDistance * minimumDistance,
                        1e-4f);
        lightRadiance = evaluateEmission(float3(light.color), path);
    } else if (lightSample.type == 2 &&
               lightSample.index < sceneData.numSpotLights) {
        SpotLight light = spotLights[lightSample.index];
        float3 toLight = float3(light.position) - P;
        distanceToLight = max(length(toLight), 1e-4f);
        L = toLight / distanceToLight;
        float range = max(light.range, 1e-4f);
        float minimumDistance = max(range * 0.08f, 0.15f);
        float cone = smoothstep(light.outerCos, light.innerCos,
                                dot(-L, normalize(float3(light.direction))));
        float rangeFade =
            1.0f - smoothstep(range * 0.75f, range, distanceToLight);
        intensity = max(light.intensity, 0.0f) * cone * rangeFade /
                    max(distanceToLight * distanceToLight +
                            minimumDistance * minimumDistance,
                        1e-4f);
        lightRadiance = evaluateEmission(float3(light.color), path);
    } else if (lightSample.type == 3 &&
               lightSample.index < sceneData.numAreaLights) {
        AreaLight light = areaLights[lightSample.index];
        float2 offset = lightSample.uv * 2.0f - 1.0f;
        float3 lightPosition = float3(light.position) +
                               float3(light.right) *
                                   (offset.x * light.halfWidth) +
                               float3(light.up) *
                                   (offset.y * light.halfHeight);
        float3 toLight = lightPosition - P;
        distanceToLight = max(length(toLight), 1e-4f);
        L = toLight / distanceToLight;
        float3 lightNormal =
            normalize(cross(float3(light.right), float3(light.up)));
        float lightCosine = light.twoSided > 0.5f
                                ? abs(dot(lightNormal, -L))
                                : max(dot(lightNormal, -L), 0.0f);
        if (lightCosine < light.emissionCos) {
            return float4(0.0f);
        }
        float area = 4.0f * light.halfWidth * light.halfHeight;
        intensity = max(light.intensity, 0.0f) * lightCosine * area /
                    max(distanceToLight * distanceToLight, 1e-6f);
        lightRadiance = evaluateEmission(float3(light.color), path);
        areaContribution = true;
    } else if (lightSample.type == 4 &&
               lightSample.index < sceneData.numEmissiveTriangles) {
        EmissiveTriangle light = emissiveTriangles[lightSample.index];
        float sqrtU = sqrt(lightSample.uv.x);
        float b0 = 1.0f - sqrtU;
        float b1 = sqrtU * (1.0f - lightSample.uv.y);
        float b2 = sqrtU * lightSample.uv.y;
        float3 lightPosition =
            light.p0.xyz * b0 + light.p1.xyz * b1 + light.p2.xyz * b2;
        float3 toLight = lightPosition - P;
        float distanceSquared = dot(toLight, toLight);
        if (distanceSquared <= 1e-8f) {
            return float4(0.0f);
        }
        distanceToLight = sqrt(distanceSquared);
        L = toLight / distanceToLight;
        float surfaceCosine = dot(N, L);
        float lightCosine = abs(dot(light.normal.xyz, -L));
        if (surfaceCosine <= 0.0f || dot(Ng, L) <= 0.0f ||
            lightCosine <= 1e-5f || light.area <= 1e-8f ||
            light.selectionPdf <= 1e-8f) {
            return float4(0.0f);
        }
        float solidAnglePdf = light.selectionPdf * distanceSquared /
                              max(lightCosine * light.area, 1e-8f);
        intensity = 1.0f / max(solidAnglePdf, 1e-8f);
        lightRadiance = evaluateEmission(float3(light.emission), path);
    } else {
        return float4(0.0f);
    }

    float4 contribution = evalPBR(
        albedo, metallic, roughness, reflectivity, ior, transmittance, N, V, L,
        lightRadiance, intensity, substrateIor, substrateAbbe,
        iridescenceFactor, iridescenceIor, iridescenceAbbe,
        iridescenceThickness, isFront, path, !areaContribution);
    if (!includeVisibility || spectralMax(contribution) <= 0.0f) {
        return contribution;
    }
    return contribution * traceShadowVisibility(
                              isect, sceneAS, P, Ng, L, distanceToLight, rng,
                              materials, primitiveObjects, blasPrimitiveOffsets,
                              vertices, indices, instanceData, sceneData, path,
                              PT_MATERIAL_TEXTURE_ARGS);
}

float4 evalSampledDirectLightingPBR(
    intersector<triangle_data> isect, primitive_acceleration_structure sceneAS,
    float3 P, float3 N, float3 Ng, float3 V, float4 albedo, float metallic,
    float roughness, float reflectivity, float4 ior, float transmittance,
    float substrateIor, float substrateAbbe, float iridescenceFactor,
    float iridescenceIor, float iridescenceAbbe,
    float iridescenceThickness, bool isFront, thread uint &rng,
    thread const SpectralPath &path, constant DirectionalLightData &dirLight,
    constant SceneData &sceneData, constant PointLight *pointLights,
    constant SpotLight *spotLights, constant AreaLight *areaLights,
    constant EmissiveTriangle *emissiveTriangles, constant Material *materials,
    constant uint *primitiveObjects, constant uint *blasPrimitiveOffsets,
    constant VertexData *vertices, constant uint *indices,
    constant InstanceData *instanceData, PT_MATERIAL_TEXTURE_PARAMS) {
    uint candidateCount = directLightCandidateCount(sceneData);
    if (candidateCount == 0) {
        return float4(0.0f);
    }
    DirectLightSample lightSample =
        sampleDirectLight(rng, sceneData, emissiveTriangles);
    return evaluateDirectLightSample(
               lightSample, true, isect, sceneAS, P, N, Ng, V, albedo,
               metallic, roughness, reflectivity, ior, transmittance,
               substrateIor, substrateAbbe, iridescenceFactor, iridescenceIor,
               iridescenceAbbe, iridescenceThickness, isFront, rng, path,
               dirLight, sceneData, pointLights, spotLights, areaLights,
               emissiveTriangles, materials, primitiveObjects,
               blasPrimitiveOffsets, vertices, indices, instanceData,
               PT_MATERIAL_TEXTURE_ARGS) *
           float(candidateCount);
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
        float intensity = max(areaLights[i].intensity, 0.0f) * cosLight /
                          max(distSq * lightPdfArea, 1e-6);
        float4 lightRadiance =
            evaluateEmission(float3(areaLights[i].color), path);
        float4 contribution =
            evalPBR(albedo, metallic, roughness, reflectivity, ior,
                    transmittance, N, V, L, lightRadiance, intensity,
                    substrateIor, substrateAbbe, iridescenceFactor,
                    iridescenceIor, iridescenceAbbe, iridescenceThickness,
                    isFront, path, false);
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
