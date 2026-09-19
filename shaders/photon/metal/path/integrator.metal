#pragma once

#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

#include "types.metal"
#include "sampling.metal"
#include "spectral.metal"
#include "environment.metal"
#include "geometry.metal"
#include "materials.metal"
#include "visibility.metal"
#include "brdf.metal"
#include "lighting.metal"
#include "caustics.metal"
#include "volumes.metal"

float3 sampleRadiance(
    uint2 gid, uint sampleIndex, uint w, intersector<triangle_data> isect,
    primitive_acceleration_structure sceneAS, ray primaryRay,
    constant Material *materials, constant uint *primitiveObjects,
    constant uint *blasPrimitiveOffsets, constant VertexData *vertices,
    constant uint *indices, constant InstanceData *instanceData,
    constant DirectionalLightData &dirLight, constant SceneData &sceneData,
    constant PointLight *pointLights, constant SpotLight *spotLights,
    constant AreaLight *areaLights,
    constant EmissiveTriangle *emissiveTriangles,
    device const CausticPhoton *photons, device const uint *photonSlots,
    constant CausticSettings &caustics, PT_MATERIAL_TEXTURE_PARAMS,
    texturecube<float> skybox, thread float3 &primaryAlbedo,
    thread float3 &primaryNormal, thread float3 &primaryPosition,
    thread float &primaryDepth, thread float &primaryRoughness,
    thread float &primaryHitDistance, thread uint &primaryObjectId) {
    uint rng = seedBase(gid, w, sceneData.frameIndex, sampleIndex);
    SpectralPath spectralPath = createSpectralPath(rng);
    float3 causticXYZ = float3(0.0f);
    uint bounceLimit = min(sceneData.maxBounces, 16u);
    ray surfaceRay = primaryRay;
    float previousBsdfPdf = 0.0;
    float previousEnvironmentPdf = 0.0;
    bool previousEventWasDelta = true;
    bool previousEventWasDiffuse = false;
    bool wavelengthSelected = false;
    bool causticConnection = false;
    bool hasNonDeltaVertex = false;

    bool insideMedium = false;
    uint mediumObjectId = 0xFFFFFFFFu;
    float4 mediumSigmaA = float4(0.0);
    float4 mediumSigmaS = float4(0.0);
    float4 mediumSigmaT = float4(0.0);
    float4 mediumEmission = float4(0.0);
    float mediumAnisotropy = 0.0f;

    for (uint depth = 0; depth <= bounceLimit; ++depth) {
        auto hit = isect.intersect(surfaceRay, sceneAS);
        Material mat{};
        InstanceData inst{};
        uint surfaceObjectIndex = 0xFFFFFFFFu;
        float2 texUV = float2(0.0);
        float3 localN = float3(0.0, 1.0, 0.0);
        float3 localT = float3(1.0, 0.0, 0.0);
        float3 localB = float3(0.0, 0.0, 1.0);
        float3 geometricNormal = float3(0.0, 1.0, 0.0);
        bool foundSurface = false;

        for (uint alphaStep = 0; alphaStep < 16; ++alphaStep) {
            if (hit.type == intersection_type::none) {
                break;
            }

            uint primitiveIndex =
                blasPrimitiveOffsets[hit.geometry_id] + hit.primitive_id;
            surfaceObjectIndex = primitiveObjects[primitiveIndex];
            mat = materials[surfaceObjectIndex];
            inst = instanceData[surfaceObjectIndex];

            uint i0 = indices[primitiveIndex * 3 + 0];
            uint i1 = indices[primitiveIndex * 3 + 1];
            uint i2 = indices[primitiveIndex * 3 + 2];
            float2 bary = hit.triangle_barycentric_coord;
            float b0 = 1.0 - bary.x - bary.y;
            float b1 = bary.x;
            float b2 = bary.y;

            texUV = float2(vertices[i0].uv) * b0 +
                    float2(vertices[i1].uv) * b1 + float2(vertices[i2].uv) * b2;
            texUV =
                texUV * float2(mat.textureScale) + float2(mat.textureOffset);
            localN = normalizeOr(float3(vertices[i0].normal) * b0 +
                                     float3(vertices[i1].normal) * b1 +
                                     float3(vertices[i2].normal) * b2,
                                 float3(0.0, 1.0, 0.0));
            localT = normalizeOr(float3(vertices[i0].tangent) * b0 +
                                     float3(vertices[i1].tangent) * b1 +
                                     float3(vertices[i2].tangent) * b2,
                                 float3(1.0, 0.0, 0.0));
            localB = normalizeOr(float3(vertices[i0].bitangent) * b0 +
                                     float3(vertices[i1].bitangent) * b1 +
                                     float3(vertices[i2].bitangent) * b2,
                                 float3(0.0, 0.0, 1.0));
            float3 p0 = float3(vertices[i0].position);
            float3 p1 = float3(vertices[i1].position);
            float3 p2 = float3(vertices[i2].position);
            float3x3 normalMatrix = float3x3(
                inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
            float3 localGeometricNormal =
                normalizeOr(cross(p1 - p0, p2 - p0), localN);

            geometricNormal = normalizeOr(
                localGeometricNormal,
                normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0)));

            float alpha = resolveMaterialOpacity(mat, texUV,
                                                 sceneData.materialTextureCount,
                                                 PT_MATERIAL_TEXTURE_ARGS);
            if (alpha >= 0.999 || rand(rng) < alpha) {
                foundSurface = true;
                break;
            }

            float3 rejectedPosition =
                surfaceRay.origin + surfaceRay.direction * hit.distance;
            surfaceRay.origin =
                rejectedPosition +
                surfaceRay.direction * rayOffsetDistance(rejectedPosition);
            surfaceRay.min_distance = 0.0;
            hit = isect.intersect(surfaceRay, sceneAS);
        }

        if (!foundSurface) {
            float misWeight =
                previousEventWasDelta
                    ? 1.0
                    : powerHeuristic(previousBsdfPdf, previousEnvironmentPdf);
            spectralPath.radiance += spectralPath.throughput * misWeight *
                                     skyColor(surfaceRay.direction, 0.0, skybox,
                                              sceneData, spectralPath);
            break;
        }

        if (insideMedium) {
            float distance = max(hit.distance, 0.0f);
            float heroSigmaT = mediumSigmaT[spectralPath.heroIndex];
            bool canScatter = spectralMax(mediumSigmaS) > 1e-6f;
            float sampledDistance =
                canScatter && heroSigmaT > 1e-6f
                    ? -log(max(1.0f - rand(rng), 1e-6f)) / heroSigmaT
                    : 1e30f;
            float traveledDistance = min(distance, sampledDistance);
            float4 spectralTransmittance =
                exp(-mediumSigmaT * traveledDistance);
            float4 emissionIntegral = (1.0f - spectralTransmittance) /
                                      max(mediumSigmaT, float4(1e-6f));
            spectralPath.radiance +=
                spectralPath.throughput * mediumEmission * emissionIntegral;

            if (sampledDistance < distance) {
                float heroTransmittance =
                    max(exp(-heroSigmaT * sampledDistance), 1e-6f);
                float samplingPdf = max(heroSigmaT * heroTransmittance, 1e-6f);
                spectralPath.throughput *=
                    spectralTransmittance * mediumSigmaS / samplingPdf;
                float3 scatteringPosition =
                    surfaceRay.origin + surfaceRay.direction * sampledDistance;
                float3 scatteringDirection = sampleHenyeyGreenstein(
                    surfaceRay.direction, mediumAnisotropy,
                    float2(rand(rng), rand(rng)));
                previousBsdfPdf = henyeyGreensteinPhase(
                    dot(surfaceRay.direction, scatteringDirection),
                    mediumAnisotropy);
                previousEnvironmentPdf = 0.0f;
                previousEventWasDelta = false;
                previousEventWasDiffuse = false;
                hasNonDeltaVertex = true;
                causticConnection = false;
                surfaceRay.origin =
                    scatteringPosition +
                    scatteringDirection * rayOffsetDistance(scatteringPosition);
                surfaceRay.direction = scatteringDirection;
                surfaceRay.min_distance = 0.0f;
                surfaceRay.max_distance = 1.0e30f;
                if (!all(isfinite(spectralPath.throughput)) ||
                    spectralMax(spectralPath.throughput) < 1e-6f) {
                    break;
                }
                continue;
            }

            float heroTransmittance = max(exp(-heroSigmaT * distance), 1e-6f);
            spectralPath.throughput *=
                canScatter ? spectralTransmittance / heroTransmittance
                           : spectralTransmittance;

            if (!all(isfinite(spectralPath.throughput)) ||
                spectralMax(spectralPath.throughput) < 1e-6f) {
                break;
            }
        }

        float3 shadingNormal = resolveShadingNormal(
            mat, texUV, localN, localT, localB, inst,
            sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        float3 P = surfaceRay.origin + surfaceRay.direction * hit.distance;
        float3 V = normalize(-surfaceRay.direction);
        bool frontFace = dot(geometricNormal, V) >= 0.0;
        float3 Ng = frontFace ? geometricNormal : -geometricNormal;
        float3 N =
            dot(shadingNormal, Ng) >= 0.0 ? shadingNormal : -shadingNormal;
        float shadingNormalCosine = dot(N, Ng);
        if (shadingNormalCosine < 0.1) {
            N = normalizeOr(N + Ng * (0.1 - shadingNormalCosine), Ng);
        }

        float3 albedoRgb;
        float metallic;
        float roughness;
        float ao;
        float3 emissiveRgb;
        float baseIor;
        float transmittance;
        float abbeNumber;
        resolveMaterialParameters(mat, texUV, sceneData.materialTextureCount,
                                  PT_MATERIAL_TEXTURE_ARGS, albedoRgb, metallic,
                                  roughness, ao, emissiveRgb, baseIor,
                                  transmittance, abbeNumber);
        bool hasVolume =
            transmittance > 0.001f && mat.attenuationDistance > 0.001f;
        float4 albedo = evaluateReflectance(albedoRgb, spectralPath);
        float4 emissive = evaluateEmission(emissiveRgb, spectralPath);
        float4 ior = evaluateIorAtWavelength(baseIor, abbeNumber, spectralPath);

        if (depth == 0) {
            primaryAlbedo = albedoRgb;
            primaryNormal = N;
            primaryPosition = P;
            primaryDepth = length(P - primaryRay.origin);
            primaryRoughness = roughness;
            primaryHitDistance = hit.distance;
            primaryObjectId = surfaceObjectIndex;
        }

        if (mat.isVolume != 0) {
            if (frontFace) {
                VolumeCoefficients coefficients = calculateVolumeCoefficients(
                    float3(mat.volumeAbsorptionColor),
                    max(mat.volumeAbsorptionStrength, 0.0f),
                    float3(mat.volumeScatteringColor),
                    max(mat.volumeScatteringStrength, 0.0f),
                    max(mat.volumeDensity, 0.0f), spectralPath);
                insideMedium = true;
                mediumObjectId = surfaceObjectIndex;
                mediumSigmaA = coefficients.sigmaA;
                mediumSigmaS = coefficients.sigmaS;
                mediumSigmaT = coefficients.sigmaT;
                mediumEmission =
                    evaluateEmission(float3(mat.volumeEmissionColor),
                                     spectralPath) *
                    max(mat.volumeEmissionStrength, 0.0f) *
                    max(mat.volumeDensity, 0.0f);
                mediumAnisotropy = clamp(mat.volumeAnisotropy, -0.99f, 0.99f);
            } else if (insideMedium && mediumObjectId == surfaceObjectIndex) {
                insideMedium = false;
                mediumObjectId = 0xFFFFFFFFu;
                mediumSigmaA = float4(0.0f);
                mediumSigmaS = float4(0.0f);
                mediumSigmaT = float4(0.0f);
                mediumEmission = float4(0.0f);
                mediumAnisotropy = 0.0f;
            }
            surfaceRay.origin = P + surfaceRay.direction * rayOffsetDistance(P);
            surfaceRay.min_distance = 0.0f;
            surfaceRay.max_distance = 1.0e30f;
            continue;
        }

        float reflectivity = clamp(mat.reflectivity, 0.0, 1.0);
        bool deltaDielectric =
            roughness <= 0.005f && transmittance > 0.999f && metallic < 0.001f;
        bool deltaMirror = roughness <= 0.005f && metallic > 0.999f;
        if (depth == 0 && sceneData.causticsEnabled != 0 && !deltaDielectric &&
            !deltaMirror) {
            causticXYZ +=
                gatherCausticsXYZ(P, N, Ng, surfaceObjectIndex, albedoRgb,
                                  caustics, photons, photonSlots);
        }
        if (!deltaDielectric && !deltaMirror) {
            float4 direct = evalDirectLightingPBR(
                isect, sceneAS, P, N, Ng, V, albedo, metallic, roughness,
                reflectivity, ior, transmittance, baseIor, abbeNumber,
                mat.iridescenceFactor, mat.iridescenceIor,
                mat.iridescenceAbbeNumber, mat.iridescenceThickness, frontFace,
                rng, spectralPath, dirLight, sceneData, pointLights, spotLights,
                areaLights, emissiveTriangles, materials, primitiveObjects,
                blasPrimitiveOffsets, vertices, indices, instanceData,
                PT_MATERIAL_TEXTURE_ARGS);
            spectralPath.radiance += spectralPath.throughput * direct;
        }
        if (!(sceneData.causticsEnabled != 0 && causticConnection) &&
            (depth == 0 || previousEventWasDelta ||
             sceneData.numEmissiveTriangles == 0)) {
            spectralPath.radiance += spectralPath.throughput * emissive;
        }

        if (depth == 0 && sceneData.ambientIntensity > 0.0f) {
            float aoVisibility = mix(0.2f, 1.0f, ao);
            float4 ambientF0 = materialF0(albedo, metallic, reflectivity, ior);
            float4 ambientOrdinaryF =
                F_Schlick(max(dot(N, V), 0.0f), ambientF0);
            float4 ambientF = evalIridescence(
                max(dot(N, V), 0.0f), ambientOrdinaryF, 1.0f, baseIor,
                abbeNumber, mat.iridescenceFactor, mat.iridescenceIor,
                mat.iridescenceAbbeNumber, mat.iridescenceThickness, frontFace,
                spectralPath);
            float4 ambientDiffuse = (1.0f - ambientF) * (1.0f - metallic) *
                                    albedo * (1.0f - transmittance);
            float4 ambientSpecular = ambientF * mix(1.0f, 0.35f, roughness);
            float4 ambientRadiance =
                evaluateEmission(sceneData.ambientColor, spectralPath) *
                sceneData.ambientIntensity;
            spectralPath.radiance += spectralPath.throughput *
                                     (ambientDiffuse + ambientSpecular) *
                                     ambientRadiance * aoVisibility;
        }

        float4 F0 = materialF0(albedo, metallic, reflectivity, ior);
        float NdotV = max(dot(N, V), 1e-4f);
        float3 interfaceN = N;
        float interfaceNdotV = max(dot(interfaceN, V), 1e-4f);
        float4 etaPacket = frontFace ? 1.0f / ior : ior;

        float4 ordinaryViewFresnel =
            deltaDielectric ? dielectricFresnel(interfaceNdotV, etaPacket)
                            : F_Schlick(NdotV, F0);

        float4 viewFresnel =
            evalIridescence(interfaceNdotV, ordinaryViewFresnel, 1.0f, baseIor,
                            abbeNumber, mat.iridescenceFactor,
                            mat.iridescenceIor, mat.iridescenceAbbeNumber,
                            mat.iridescenceThickness, frontFace, spectralPath);

        float fresnelProbability =
            clamp(spectralAverage(viewFresnel), 0.001f, 0.999f);
        float specProb = fresnelProbability;
        float transmitProb =
            transmittance * (1.0f - metallic) * (1.0f - fresnelProbability);

        float diffuseProb = (1.0f - metallic) * (1.0f - transmittance) *
                            (1.0f - fresnelProbability);

        uint heroIndex = spectralPath.heroIndex;
        float eta = etaPacket[heroIndex];
        float3 idealRefractedDirection = refract(-V, interfaceN, eta);
        bool totalInternalReflection =
            transmitProb > 1e-4f &&
            dot(idealRefractedDirection, idealRefractedDirection) < 1e-8f;

        if (totalInternalReflection) {
            specProb += transmitProb;
            transmitProb = 0.0f;
        }
        float probabilitySum = max(specProb + transmitProb + diffuseProb, 1e-4);
        specProb /= probabilitySum;
        transmitProb /= probabilitySum;
        diffuseProb /= probabilitySum;
        if (transmittance < 0.001f && roughness < 0.35f && specProb > 0.0f &&
            diffuseProb > 0.0f) {
            specProb = max(specProb, 0.25f);
            diffuseProb = max(1.0f - specProb - transmitProb, 0.0f);
        }

        float3x3 basis = buildOrthonormalBasis(N);
        if (sceneData.environmentEnabled != 0 &&
            diffuseProb + specProb > 1e-4) {
            float3 localEnvironmentDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            float3 environmentDirection =
                normalizeOr(basis * localEnvironmentDirection, N);
            float NdotEnvironment = dot(N, environmentDirection);
            if (NdotEnvironment > 0.0 && dot(Ng, environmentDirection) > 0.0) {
                float4 visibility = traceShadowVisibility(
                    isect, sceneAS, P, Ng, environmentDirection, 1e30, rng,
                    materials, primitiveObjects, blasPrimitiveOffsets, vertices,
                    indices, instanceData, sceneData, spectralPath,
                    PT_MATERIAL_TEXTURE_ARGS);
                float3 H = normalizeOr(V + environmentDirection, N);
                float NdotH = max(dot(N, H), 1e-5);
                float VdotH = max(dot(V, H), 1e-5);
                float4 ordinaryF = F_Schlick(VdotH, F0);

                float4 F = evalIridescence(
                    VdotH, ordinaryF, 1.0f, baseIor, abbeNumber,
                    mat.iridescenceFactor, mat.iridescenceIor,
                    mat.iridescenceAbbeNumber, mat.iridescenceThickness,
                    frontFace, spectralPath);

                float4 kD =
                    (1.0 - F) * (1.0 - metallic) * (1.0 - transmittance);
                float diffuseFactor = disneyDiffuseFactor(
                    NdotV, NdotEnvironment,
                    max(dot(environmentDirection, H), 0.0), roughness);
                float4 reflectionBsdf = kD * albedo * diffuseFactor / M_PI_F;
                float environmentPdf = NdotEnvironment / M_PI_F;
                float bsdfPdf = diffuseProb * environmentPdf;
                if (specProb > 1e-4) {
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L = G1_SmithGGX(NdotEnvironment, roughness);
                    reflectionBsdf += D * G1V * G1L * F /
                                      max(4.0 * NdotV * NdotEnvironment, 1e-6);
                    bsdfPdf += specProb * D * G1V / max(4.0 * NdotV, 1e-6);
                }
                float competingBsdfPdf = depth < bounceLimit ? bsdfPdf : 0.0;
                float misWeight =
                    powerHeuristic(environmentPdf, competingBsdfPdf);
                float4 environmentRadiance = skyColor(
                    environmentDirection, 0.0, skybox, sceneData, spectralPath);
                spectralPath.radiance += spectralPath.throughput *
                                         reflectionBsdf * environmentRadiance *
                                         visibility * NdotEnvironment *
                                         misWeight / max(environmentPdf, 1e-6);
            }
        }

        if (depth == bounceLimit) {
            break;
        }

        float choice = rand(rng);
        float4 bounceWeight = float4(0.0);
        float3 nextDirection = N;
        float sampledBsdfPdf = 0.0;
        float sampledEnvironmentPdf = 0.0;
        bool sampledEventWasDelta = true;
        bool sampledRoughTransmission = false;
        bool sampledEventWasDiffuse = false;

        if (choice < specProb && specProb > 1e-4) {
            if (deltaDielectric || totalInternalReflection) {
                nextDirection = reflect(-V, interfaceN);
                float4 F = totalInternalReflection ? float4(1.0f) : viewFresnel;
                bounceWeight = F / max(specProb, 1e-4f);
            } else if (roughness <= 0.005) {
                nextDirection = reflect(-V, interfaceN);
                float4 F = totalInternalReflection ? float4(1.0) : viewFresnel;
                bounceWeight = F / max(specProb, 1e-4);
            } else {
                float3 localView =
                    float3(dot(V, basis[0]), dot(V, basis[1]), dot(V, N));
                float3 localH = sampleGGXVNDF(localView, roughness,
                                              float2(rand(rng), rand(rng)));
                float3 H = normalizeOr(basis * localH, N);
                float VdotH = max(dot(V, H), 1e-5);
                nextDirection = reflect(-V, H);
                float NdotL = dot(N, nextDirection);
                if (NdotL > 0.0 && dot(nextDirection, Ng) > 0.0) {
                    float NdotH = max(dot(N, H), 1e-5);
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L = G1_SmithGGX(NdotL, roughness);
                    float4 ordinaryF = F_Schlick(VdotH, F0);
                    float4 F = evalIridescence(
                        VdotH, ordinaryF, 1.0f, baseIor, abbeNumber,
                        mat.iridescenceFactor, mat.iridescenceIor,
                        mat.iridescenceAbbeNumber, mat.iridescenceThickness,
                        frontFace, spectralPath);

                    float4 specularBsdf =
                        D * G1V * G1L * F / max(4.0 * NdotV * NdotL, 1e-6);
                    float conditionalPdf = D * G1V / max(4.0 * NdotV, 1e-6);
                    float combinedPdf = specProb * conditionalPdf;
                    bounceWeight =
                        specularBsdf * NdotL / max(combinedPdf, 1e-6);
                    sampledBsdfPdf = combinedPdf;
                    sampledEnvironmentPdf = NdotL / M_PI_F;
                    sampledEventWasDelta = false;
                }
            }
        } else if (choice < specProb + transmitProb && transmitProb > 1e-4) {
            nextDirection = idealRefractedDirection;
            float fresnelCosine = interfaceNdotV;

            if (!deltaDielectric) {
                sampledRoughTransmission = true;
                float3 localView =
                    float3(dot(V, basis[0]), dot(V, basis[1]), dot(V, N));
                float3 localH = sampleGGXVNDF(localView, roughness,
                                              float2(rand(rng), rand(rng)));
                float3 H = normalizeOr(basis * localH, N);
                float3 roughRefractedDirection = refract(-V, H, eta);
                if (dot(roughRefractedDirection, roughRefractedDirection) >
                        1e-8 &&
                    dot(roughRefractedDirection, Ng) < 0.0) {
                    nextDirection = roughRefractedDirection;
                    fresnelCosine = max(dot(V, H), 0.0);
                }
            }
            float4 ordinaryF = dielectricFresnel(fresnelCosine, etaPacket);
            float4 F = evalIridescence(
                fresnelCosine, ordinaryF, 1.0f, baseIor, abbeNumber,
                mat.iridescenceFactor, mat.iridescenceIor,
                mat.iridescenceAbbeNumber, mat.iridescenceThickness, frontFace,
                spectralPath);
            bounceWeight = (1.0 - F) * transmittance * (1.0 - metallic) *
                           etaPacket * etaPacket / max(transmitProb, 1e-4);

            if (hasVolume) {
                if (frontFace) {
                    insideMedium = true;
                    mediumObjectId = surfaceObjectIndex;

                    float3 attenuationColor =
                        clamp(float3(mat.attenuationColor), float3(0.001f),
                              float3(1.0f));

                    float attenuationDistance =
                        max(mat.attenuationDistance, 1e-4f);

                    float4 spectralAttenuation = clamp(
                        evaluateReflectance(attenuationColor, spectralPath),
                        float4(0.001f), float4(1.0f));

                    mediumSigmaA =
                        -log(spectralAttenuation) / attenuationDistance;
                    mediumSigmaS = float4(0.0f);
                    mediumSigmaT = mediumSigmaA;
                    mediumEmission = float4(0.0f);
                    mediumAnisotropy = 0.0f;

                } else if (insideMedium &&
                           mediumObjectId == surfaceObjectIndex) {

                    insideMedium = false;
                    mediumObjectId = 0xFFFFFFFFu;
                    mediumSigmaA = float4(0.0f);
                    mediumSigmaS = float4(0.0f);
                    mediumSigmaT = float4(0.0f);
                    mediumEmission = float4(0.0f);
                    mediumAnisotropy = 0.0f;
                }
            }

            if (abbeNumber > 0.0 && !wavelengthSelected) {
                float4 heroMask = float4(
                    heroIndex == 0 ? 1.0f : 0.0f, heroIndex == 1 ? 1.0f : 0.0f,
                    heroIndex == 2 ? 1.0f : 0.0f, heroIndex == 3 ? 1.0f : 0.0f);
                bounceWeight *= heroMask * float(PHOTON_SPECTRAL_LANE_COUNT);
                wavelengthSelected = true;
            }
        } else {
            sampledEventWasDiffuse = true;
            float3 localDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            nextDirection = normalizeOr(basis * localDirection, N);
            float NdotL = max(dot(N, nextDirection), 0.0);
            float3 H = normalizeOr(V + nextDirection, N);
            float4 ordinaryF = F_Schlick(max(dot(V, H), 0.0), F0);
            float4 F = evalIridescence(
                max(dot(N, H), 0.0), ordinaryF, 1.0f, baseIor, abbeNumber,
                mat.iridescenceFactor, mat.iridescenceIor,
                mat.iridescenceAbbeNumber, mat.iridescenceThickness, frontFace,
                spectralPath);
            float4 kD = (1.0 - F) * (1.0 - metallic) * (1.0 - transmittance);
            float diffuseFactor = disneyDiffuseFactor(
                NdotV, NdotL, max(dot(nextDirection, H), 0.0), roughness);
            float4 diffuseBsdf = kD * albedo * diffuseFactor / M_PI_F;
            float conditionalPdf = NdotL / M_PI_F;
            float combinedPdf = diffuseProb * conditionalPdf;
            bounceWeight = diffuseBsdf * NdotL / max(combinedPdf, 1e-6);
            sampledBsdfPdf = combinedPdf;
            sampledEnvironmentPdf = conditionalPdf;
            sampledEventWasDelta = false;
        }

        bounceWeight = clamp(bounceWeight, float4(0.0), float4(16.0));
        spectralPath.throughput *= bounceWeight;
        spectralPath.throughput = min(spectralPath.throughput, float4(32.0));
        if (depth == 0) {
            spectralPath.throughput *= max(sceneData.indirectStrength, 0.0);
        }
        if (!all(isfinite(spectralPath.throughput)) ||
            spectralMax(spectralPath.throughput) < 1e-5) {
            break;
        }

        if (depth >= 2) {
            float survival =
                clamp(spectralMax(spectralPath.throughput), 0.05, 0.95);
            if (rand(rng) > survival) {
                break;
            }
            spectralPath.throughput /= survival;
        }

        if (sampledRoughTransmission) {
            hasNonDeltaVertex = false;
            causticConnection = false;
        } else if (!sampledEventWasDelta) {
            hasNonDeltaVertex = depth == 0 && sampledEventWasDiffuse;
            causticConnection = false;
        } else if (hasNonDeltaVertex) {
            causticConnection = true;
        }
        previousBsdfPdf = sampledBsdfPdf;
        previousEnvironmentPdf = sampledEnvironmentPdf;
        previousEventWasDelta = sampledEventWasDelta;
        previousEventWasDiffuse = sampledEventWasDiffuse;

        surfaceRay.origin = offsetRayOrigin(P, Ng, nextDirection);
        surfaceRay.direction = normalizeOr(nextDirection, N);
        surfaceRay.min_distance = 0.0;
        surfaceRay.max_distance = 1.0e30;
    }

    float3 xyz = spectralRadianceToXYZ(max(spectralPath.radiance, float4(0.0)),
                                       spectralPath);
    xyz += causticXYZ;
    return xyzToLinearSRGB(xyz);
}
