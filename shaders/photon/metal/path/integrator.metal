float3 sampleRadiance(uint2 gid, uint sampleIndex, uint w,
                      intersector<triangle_data> isect,
                      primitive_acceleration_structure sceneAS, ray primaryRay,
                      constant Material *materials,
                      constant uint *primitiveObjects,
                      constant uint *blasPrimitiveOffsets,
                      constant VertexData *vertices, constant uint *indices,
                      constant InstanceData *instanceData,
                      constant DirectionalLightData &dirLight,
                      constant SceneData &sceneData,
                      constant PointLight *pointLights,
                      constant SpotLight *spotLights,
                      constant AreaLight *areaLights,
                      constant EmissiveTriangle *emissiveTriangles,
                      PT_MATERIAL_TEXTURE_PARAMS, texturecube<float> skybox,
                      thread float3 &primaryAlbedo,
                      thread float3 &primaryNormal,
                      thread float3 &primaryPosition,
                      thread float &primaryDepth,
                      thread float &primaryRoughness,
                      thread float &primaryHitDistance,
                      thread uint &primaryObjectId) {
    uint rng = seedBase(gid, w, sceneData.frameIndex, sampleIndex);
    uint bounceLimit = min(sceneData.maxBounces, 16u);
    ray surfaceRay = primaryRay;
    float3 radiance = float3(0.0);
    float3 throughput = float3(1.0);
    float previousBsdfPdf = 0.0;
    float previousEnvironmentPdf = 0.0;
    bool previousEventWasDelta = true;

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
                    float2(vertices[i1].uv) * b1 +
                    float2(vertices[i2].uv) * b2;
            texUV = texUV * float2(mat.textureScale) +
                    float2(mat.textureOffset);
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
            geometricNormal = normalizeOr(
                cross(p1 - p0, p2 - p0),
                normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0)));

            float alpha = resolveMaterialOpacity(
                mat, texUV, sceneData.materialTextureCount,
                PT_MATERIAL_TEXTURE_ARGS);
            if (alpha >= 0.999 || rand(rng) < alpha) {
                foundSurface = true;
                break;
            }

            float3 rejectedPosition =
                surfaceRay.origin + surfaceRay.direction * hit.distance;
            surfaceRay.origin =
                rejectedPosition + surfaceRay.direction *
                                       rayOffsetDistance(rejectedPosition);
            surfaceRay.min_distance = 0.0;
            hit = isect.intersect(surfaceRay, sceneAS);
        }

        if (!foundSurface) {
            float misWeight = previousEventWasDelta
                                  ? 1.0
                                  : powerHeuristic(previousBsdfPdf,
                                                   previousEnvironmentPdf);
            radiance += throughput * misWeight *
                        skyColor(surfaceRay.direction, 0.0, skybox, sceneData);
            break;
        }

        float3 shadingNormal = resolveShadingNormal(
            mat, texUV, localN, localT, localB, inst,
            sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        float3 P = surfaceRay.origin + surfaceRay.direction * hit.distance;
        float3 V = normalize(-surfaceRay.direction);
        bool frontFace = dot(geometricNormal, V) >= 0.0;
        float3 Ng = frontFace ? geometricNormal : -geometricNormal;
        float3 N = dot(shadingNormal, Ng) >= 0.0 ? shadingNormal : -shadingNormal;
        float shadingNormalCosine = dot(N, Ng);
        if (shadingNormalCosine < 0.1) {
            N = normalizeOr(N + Ng * (0.1 - shadingNormalCosine), Ng);
        }

        float3 albedo;
        float metallic;
        float roughness;
        float ao;
        float3 emissive;
        float ior;
        float transmittance;
        resolveMaterialParameters(mat, texUV, sceneData.materialTextureCount,
                                  PT_MATERIAL_TEXTURE_ARGS, albedo, metallic,
                                  roughness, ao, emissive, ior, transmittance);

        if (depth == 0) {
            primaryAlbedo = albedo;
            primaryNormal = N;
            primaryPosition = P;
            primaryDepth = length(P - primaryRay.origin);
            primaryRoughness = roughness;
            primaryHitDistance = hit.distance;
            primaryObjectId = surfaceObjectIndex;
        }

        float reflectivity = clamp(mat.reflectivity, 0.0, 1.0);
        float sssStrength = 0.0;
        float sssThickness = mix(0.25, 1.75, ao);
        float3 direct = evalDirectLightingPBR(
            isect, sceneAS, P, N, Ng, V, albedo, metallic, roughness,
            reflectivity, ior, transmittance, sssStrength, sssThickness, rng,
            dirLight, sceneData, pointLights, spotLights, areaLights,
            emissiveTriangles, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData,
            PT_MATERIAL_TEXTURE_ARGS);
        radiance += throughput * direct;
        if (depth == 0 || previousEventWasDelta ||
            sceneData.numEmissiveTriangles == 0) {
            radiance += throughput * emissive;
        }

        if (depth == 0 && sceneData.ambientIntensity > 0.0) {
            float aoVisibility = mix(0.2, 1.0, ao);
            float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
            float3 ambientF0 = mix(float3(dielectricF0), albedo, metallic);
            float3 ambientF = F_Schlick(max(dot(N, V), 0.0), ambientF0);
            float3 ambientDiffuse = (1.0 - ambientF) * (1.0 - metallic) *
                                    albedo * (1.0 - transmittance);
            float3 ambientSpecular = ambientF * mix(1.0, 0.35, roughness) *
                                     (1.0 - transmittance);
            float3 ambient = (ambientDiffuse + ambientSpecular) *
                             sceneData.ambientColor *
                             sceneData.ambientIntensity * aoVisibility;
            radiance += throughput * ambient;
        }

        float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
        float3 F0 = materialF0(albedo, metallic, mat.reflectivity, ior);
        float NdotV = max(dot(N, V), 1e-4);
        float3 viewFresnel = F_Schlick(NdotV, F0);
        float fresnelProbability = clamp(luminance(viewFresnel), 0.001, 0.999);
        float specProb = fresnelProbability;
        float transmitProb = transmittance * (1.0 - metallic) *
                             (1.0 - fresnelProbability);
        float diffuseProb = (1.0 - metallic) * (1.0 - transmittance) *
                            (1.0 - fresnelProbability);
        float eta = frontFace ? 1.0 / ior : ior;
        float3 idealRefractedDirection = refract(-V, N, eta);
        bool totalInternalReflection =
            dot(idealRefractedDirection, idealRefractedDirection) < 1e-8;
        if (totalInternalReflection) {
            specProb += transmitProb;
            transmitProb = 0.0;
        }
        float probabilitySum =
            max(specProb + transmitProb + diffuseProb, 1e-4);
        specProb /= probabilitySum;
        transmitProb /= probabilitySum;
        diffuseProb /= probabilitySum;

        float3x3 basis = buildOrthonormalBasis(N);
        if (sceneData.environmentEnabled != 0 &&
            diffuseProb + specProb > 1e-4) {
            float3 localEnvironmentDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            float3 environmentDirection =
                normalizeOr(basis * localEnvironmentDirection, N);
            float NdotEnvironment = dot(N, environmentDirection);
            if (NdotEnvironment > 0.0 &&
                dot(Ng, environmentDirection) > 0.0) {
                float3 visibility = traceShadowVisibility(
                    isect, sceneAS, P, Ng, environmentDirection, 1e30, rng,
                    materials, primitiveObjects, blasPrimitiveOffsets,
                    vertices, indices, instanceData, sceneData,
                    PT_MATERIAL_TEXTURE_ARGS);
                if (luminance(visibility) <= 0.001) {
                    visibility = float3(0.0);
                }
                float3 H = normalizeOr(V + environmentDirection, N);
                float NdotH = max(dot(N, H), 1e-5);
                float VdotH = max(dot(V, H), 1e-5);
                float3 F = F_Schlick(VdotH, F0);
                float3 kD = (1.0 - F) * (1.0 - metallic) *
                            (1.0 - transmittance);
                float diffuseFactor = disneyDiffuseFactor(
                    NdotV, NdotEnvironment,
                    max(dot(environmentDirection, H), 0.0), roughness);
                float3 reflectionBsdf =
                    kD * albedo * diffuseFactor / M_PI_F;
                float environmentPdf = NdotEnvironment / M_PI_F;
                float bsdfPdf = diffuseProb * environmentPdf;
                if (roughness > 0.025 && specProb > 1e-4) {
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L =
                        G1_SmithGGX(NdotEnvironment, roughness);
                    reflectionBsdf +=
                        D * G1V * G1L * F /
                        max(4.0 * NdotV * NdotEnvironment, 1e-6);
                    bsdfPdf += specProb * D * G1V /
                               max(4.0 * NdotV, 1e-6);
                }
                float competingBsdfPdf = depth < bounceLimit ? bsdfPdf : 0.0;
                float misWeight =
                    powerHeuristic(environmentPdf, competingBsdfPdf);
                float3 environmentRadiance = skyColor(
                    environmentDirection, 0.0, skybox, sceneData);
                radiance += throughput * reflectionBsdf *
                            environmentRadiance * visibility * NdotEnvironment *
                            misWeight / max(environmentPdf, 1e-6);
            }
        }

        if (depth == bounceLimit) {
            break;
        }

        float choice = rand(rng);
        float3 bounceWeight = float3(0.0);
        float3 nextDirection = N;
        float sampledBsdfPdf = 0.0;
        float sampledEnvironmentPdf = 0.0;
        bool sampledEventWasDelta = true;

        if (choice < specProb && specProb > 1e-4) {
            if (roughness <= 0.025 || totalInternalReflection) {
                nextDirection = reflect(-V, N);
                float3 F = totalInternalReflection
                               ? float3(1.0)
                               : F_Schlick(NdotV, F0);
                bounceWeight = F / max(specProb, 1e-4);
            } else {
                float3 localView =
                    float3(dot(V, basis[0]), dot(V, basis[1]), dot(V, N));
                float3 localH = sampleGGXVNDF(
                    localView, roughness, float2(rand(rng), rand(rng)));
                float3 H = normalizeOr(basis * localH, N);
                float VdotH = max(dot(V, H), 1e-5);
                nextDirection = reflect(-V, H);
                float NdotL = dot(N, nextDirection);
                if (NdotL > 0.0 && dot(nextDirection, Ng) > 0.0) {
                    float NdotH = max(dot(N, H), 1e-5);
                    float D = D_GGX(NdotH, roughness);
                    float G1V = G1_SmithGGX(NdotV, roughness);
                    float G1L = G1_SmithGGX(NdotL, roughness);
                    float3 F = F_Schlick(VdotH, F0);
                    float3 specularBsdf =
                        D * G1V * G1L * F /
                        max(4.0 * NdotV * NdotL, 1e-6);
                    float conditionalPdf =
                        D * G1V / max(4.0 * NdotV, 1e-6);
                    float combinedPdf = specProb * conditionalPdf;
                    bounceWeight = specularBsdf * NdotL /
                                   max(combinedPdf, 1e-6);
                    sampledBsdfPdf = combinedPdf;
                    sampledEnvironmentPdf = NdotL / M_PI_F;
                    sampledEventWasDelta = false;
                }
            }
        } else if (choice < specProb + transmitProb &&
                   transmitProb > 1e-4) {
            nextDirection = idealRefractedDirection;
            float fresnelCosine = NdotV;
            if (roughness > 0.025) {
                float3 localView =
                    float3(dot(V, basis[0]), dot(V, basis[1]), dot(V, N));
                float3 localH = sampleGGXVNDF(
                    localView, roughness, float2(rand(rng), rand(rng)));
                float3 H = normalizeOr(basis * localH, N);
                float3 roughRefractedDirection = refract(-V, H, eta);
                if (dot(roughRefractedDirection, roughRefractedDirection) >
                        1e-8 &&
                    dot(roughRefractedDirection, Ng) < 0.0) {
                    nextDirection = roughRefractedDirection;
                    fresnelCosine = max(dot(V, H), 0.0);
                }
            }
            float3 F = F_Schlick(fresnelCosine, float3(dielectricF0));
            float3 tint = mix(float3(1.0), albedo, 0.15);
            bounceWeight = (1.0 - F) * tint /
                           max(transmitProb, 1e-4);
        } else {
            float3 localDirection =
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            nextDirection = normalizeOr(basis * localDirection, N);
            float NdotL = max(dot(N, nextDirection), 0.0);
            float3 H = normalizeOr(V + nextDirection, N);
            float3 F = F_Schlick(max(dot(V, H), 0.0), F0);
            float3 kD = (1.0 - F) * (1.0 - metallic) *
                        (1.0 - transmittance);
            float diffuseFactor = disneyDiffuseFactor(
                NdotV, NdotL, max(dot(nextDirection, H), 0.0), roughness);
            float3 diffuseBsdf = kD * albedo * diffuseFactor / M_PI_F;
            float conditionalPdf = NdotL / M_PI_F;
            float combinedPdf = diffuseProb * conditionalPdf;
            bounceWeight = diffuseBsdf * NdotL /
                           max(combinedPdf, 1e-6);
            sampledBsdfPdf = combinedPdf;
            sampledEnvironmentPdf = conditionalPdf;
            sampledEventWasDelta = false;
        }

        bounceWeight = clampLuminance(max(bounceWeight, float3(0.0)), 16.0);
        throughput *= bounceWeight;
        throughput = clampLuminance(throughput, 32.0);
        if (depth == 0) {
            throughput *= max(sceneData.indirectStrength, 0.0);
        }
        if (!all(isfinite(throughput)) || max(throughput.x,
                                             max(throughput.y, throughput.z)) <
                                               1e-5) {
            break;
        }

        if (depth >= 2) {
            float survival = clamp(max(throughput.x,
                                       max(throughput.y, throughput.z)),
                                   0.05, 0.95);
            if (rand(rng) > survival) {
                break;
            }
            throughput /= survival;
        }

        previousBsdfPdf = sampledBsdfPdf;
        previousEnvironmentPdf = sampledEnvironmentPdf;
        previousEventWasDelta = sampledEventWasDelta;

        surfaceRay.origin = offsetRayOrigin(P, Ng, nextDirection);
        surfaceRay.direction = normalizeOr(nextDirection, N);
        surfaceRay.min_distance = 0.0;
        surfaceRay.max_distance = 1.0e30;
    }

    return radiance;
}

