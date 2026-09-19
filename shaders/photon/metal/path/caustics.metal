#pragma once

#include "types.metal"
#include "sampling.metal"
#include "spectral.metal"
#include "geometry.metal"
#include "materials.metal"
#include "visibility.metal"
#include "brdf.metal"

constant uint CAUSTIC_EMPTY_KEY = 0u;
constant uint CAUSTIC_INVALID_BUCKET = 0xFFFFFFFFu;

constant uint CAUSTIC_BUCKET_HEADER_WORDS = 2u;

constant uint CAUSTIC_BUCKET_WORDS =
    CAUSTIC_BUCKET_HEADER_WORDS + CAUSTIC_BUCKET_SAMPLES;

constant uint CAUSTIC_MAX_PROBES = 128u;

uint causticCellKey(int3 cell) {
    uint3 c = as_type<uint3>(cell);

    uint h = c.x * 73856093u ^ c.y * 19349663u ^ c.z * 83492791u;

    h = wang_hash(h);

    if (h == CAUSTIC_EMPTY_KEY) {
        h = 1u;
    }

    return h;
}
uint causticInitialBucket(uint key) {
    return wang_hash(key ^ 0x9E3779B9u) & (CAUSTIC_BUCKET_COUNT - 1u);
}

uint findCausticCell(device const uint *photonSlots, int3 cell) {
    uint key = causticCellKey(cell);
    uint initialBucket = causticInitialBucket(key);

    for (uint probe = 0u; probe < CAUSTIC_MAX_PROBES; ++probe) {

        uint bucket = (initialBucket + probe) & (CAUSTIC_BUCKET_COUNT - 1u);

        uint base = bucket * CAUSTIC_BUCKET_WORDS;

        uint storedKey = photonSlots[base + 0u];

        if (storedKey == CAUSTIC_EMPTY_KEY) {
            return CAUSTIC_INVALID_BUCKET;
        }

        if (storedKey == key) {
            return bucket;
        }
    }

    return CAUSTIC_INVALID_BUCKET;
}

uint findOrCreateCausticCell(device atomic_uint *photonSlots, int3 cell) {
    uint key = causticCellKey(cell);
    uint initialBucket = causticInitialBucket(key);

    for (uint probe = 0u; probe < CAUSTIC_MAX_PROBES; ++probe) {

        uint bucket = (initialBucket + probe) & (CAUSTIC_BUCKET_COUNT - 1u);

        uint base = bucket * CAUSTIC_BUCKET_WORDS;

        uint storedKey =
            atomic_load_explicit(&photonSlots[base], memory_order_relaxed);

        if (storedKey == key) {
            return bucket;
        }

        if (storedKey == CAUSTIC_EMPTY_KEY) {
            uint expected = CAUSTIC_EMPTY_KEY;
            for (;;) {

                bool claimed = atomic_compare_exchange_weak_explicit(
                    &photonSlots[base], &expected, key, memory_order_relaxed,
                    memory_order_relaxed);

                if (claimed) {
                    return bucket;
                }

                if (expected == key) {
                    return bucket;
                }

                if (expected != CAUSTIC_EMPTY_KEY) {
                    break;
                }

                expected = CAUSTIC_EMPTY_KEY;
            }
        }
    }

    return CAUSTIC_INVALID_BUCKET;
}

bool insertCausticPhoton(device atomic_uint *photonSlots, int3 cell,
                         uint photonId, uint seed) {
    uint bucket = findOrCreateCausticCell(photonSlots, cell);

    if (bucket == CAUSTIC_INVALID_BUCKET) {
        return false;
    }

    uint base = bucket * CAUSTIC_BUCKET_WORDS;

    uint oldCount = atomic_fetch_add_explicit(&photonSlots[base + 1u], 1u,
                                              memory_order_relaxed);

    uint newCount = oldCount + 1u;

    uint reservoirSlot;

    if (oldCount < CAUSTIC_BUCKET_SAMPLES) {
        reservoirSlot = oldCount;
    } else {
        uint randomValue = wang_hash(photonId ^ (seed * 0x9E3779B9u) ^
                                     (newCount * 0x85EBCA6Bu));

        uint candidate = randomValue % newCount;

        if (candidate >= CAUSTIC_BUCKET_SAMPLES) {
            return true;
        }

        reservoirSlot = candidate;
    }

    atomic_store_explicit(
        &photonSlots[base + CAUSTIC_BUCKET_HEADER_WORDS + reservoirSlot],
        photonId, memory_order_relaxed);

    return true;
}

float3 gatherCausticsXYZ(float3 P, float3 N, float3 Ng, uint objectId,
                         float3 albedoRgb, constant CausticSettings &caustics,
                         device const CausticPhoton *photons,
                         device const uint *photonSlots) {
    float3 resultXYZ = float3(0.0f);
    uint acceptedPhotons = 0u;
    float contributionSum = 0.0f;
    float contributionSquaredSum = 0.0f;

    if (caustics.radius <= 0.0f) {
        return resultXYZ;
    }

    float radiusSquared = caustics.radius * caustics.radius;

    int3 center = int3(floor(P / caustics.radius));

    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {

                int3 cell = center + int3(x, y, z);

                uint bucket = findCausticCell(photonSlots, cell);

                if (bucket == CAUSTIC_INVALID_BUCKET) {
                    continue;
                }

                uint base = bucket * CAUSTIC_BUCKET_WORDS;

                uint totalCount = photonSlots[base + 1u];

                if (totalCount == 0u) {
                    continue;
                }

                uint storedCount =
                    min(totalCount, uint(CAUSTIC_BUCKET_SAMPLES));

                if (storedCount == 0u) {
                    continue;
                }

                float reservoirWeight = float(totalCount) / float(storedCount);

                for (uint slot = 0u; slot < storedCount; ++slot) {

                    uint photonId =
                        photonSlots[base + CAUSTIC_BUCKET_HEADER_WORDS + slot];

                    if (photonId == 0xFFFFFFFFu) {
                        continue;
                    }

                    CausticPhoton photon = photons[photonId];

                    if (uint(photon.incomingObject.w) != objectId) {
                        continue;
                    }

                    int3 photonCell = int3(
                        floor(photon.positionWavelength.xyz / caustics.radius));

                    if (any(photonCell != cell)) {
                        continue;
                    }

                    float3 delta = photon.positionWavelength.xyz - P;

                    float distanceSquared = dot(delta, delta);

                    if (distanceSquared >= radiusSquared) {
                        continue;
                    }

                    float3 incoming = photon.incomingObject.xyz;

                    float cosine = dot(N, incoming);

                    if (cosine <= 1e-4f) {
                        continue;
                    }

                    if (dot(Ng, photon.normalPower.xyz) < 0.95f) {
                        continue;
                    }

                    if (abs(dot(delta, Ng)) > caustics.radius * 0.1f) {
                        continue;
                    }

                    float densityWeight =
                        2.0f * (1.0f - distanceSquared / radiusSquared) /
                        (M_PI_F * radiusSquared);

                    float wavelengthNm = photon.positionWavelength.w;

                    float irradiance =
                        photon.normalPower.w * reservoirWeight * densityWeight;

                    float receiverReflectance =
                        rgbToReflectanceAtWavelength(albedoRgb, wavelengthNm);

                    float bsdf = receiverReflectance / M_PI_F;

                    float spectralRadiance = irradiance * bsdf;

                    float3 contribution =
                        spectralRadiance * cieXYZ1931(wavelengthNm) /
                        PHOTON_CIE_Y_INTEGRAL;
                    float weight = max(contribution.y, 0.0f);
                    if (all(isfinite(contribution)) && weight > 0.0f) {
                        resultXYZ += contribution;
                        contributionSum += weight;
                        contributionSquaredSum += weight * weight;
                        acceptedPhotons++;
                    }
                }
            }
        }
    }

    if (acceptedPhotons < 16u || contributionSquaredSum <= 0.0f) {
        return float3(0.0f);
    }
    float effectivePhotonCount = contributionSum * contributionSum /
                                 contributionSquaredSum;
    float confidence = smoothstep(12.0f, 32.0f, effectivePhotonCount);
    return resultXYZ * confidence;
}

kernel void clearCaustics(device atomic_uint *photonSlots [[buffer(16)]],
                          uint id [[thread_position_in_grid]]) {
    if (id >= CAUSTIC_BUCKET_COUNT) {
        return;
    }

    uint base = id * CAUSTIC_BUCKET_WORDS;

    atomic_store_explicit(&photonSlots[base + 0u], CAUSTIC_EMPTY_KEY,
                          memory_order_relaxed);

    atomic_store_explicit(&photonSlots[base + 1u], 0u, memory_order_relaxed);

    for (uint slot = 0u; slot < CAUSTIC_BUCKET_SAMPLES; ++slot) {

        atomic_store_explicit(
            &photonSlots[base + CAUSTIC_BUCKET_HEADER_WORDS + slot],
            0xFFFFFFFFu, memory_order_relaxed);
    }
}

kernel void emitCaustics(primitive_acceleration_structure sceneAS [[buffer(0)]],
                         constant Material *materials [[buffer(2)]],
                         constant uint *primitiveObjects [[buffer(3)]],
                         constant VertexData *vertices [[buffer(4)]],
                         constant uint *indices [[buffer(5)]],
                         constant InstanceData *instanceData [[buffer(6)]],
                         constant DirectionalLightData &dirLight [[buffer(7)]],
                         constant SceneData &sceneData [[buffer(8)]],
                         constant PointLight *pointLights [[buffer(9)]],
                         constant SpotLight *spotLights [[buffer(10)]],
                         constant AreaLight *areaLights [[buffer(11)]],
                         constant uint *blasPrimitiveOffsets [[buffer(13)]],
                         constant EmissiveTriangle *emissiveTriangles
                         [[buffer(14)]],
                         device CausticPhoton *photons [[buffer(15)]],
                         device atomic_uint *photonSlots [[buffer(16)]],
                         constant CausticSettings &caustics [[buffer(17)]],
                         PT_MATERIAL_TEXTURE_BINDINGS,
                         uint id [[thread_position_in_grid]]) {
    if (id >= CAUSTIC_PHOTON_COUNT)
        return;
    uint rng = wang_hash(id + caustics.seed * 9781u + 1u);
    SpectralPath path = createSpectralPath(rng);
    float2 wavelength = sampleVisibleWavelength((float(id) + rand(rng)) /
                                                float(CAUSTIC_PHOTON_COUNT));
    path.wavelengthNm = float4(wavelength.x);
    uint lightCount = sceneData.numDirectionalLights +
                      sceneData.numPointLights + sceneData.numSpotLights +
                      sceneData.numAreaLights +
                      (sceneData.numEmissiveTriangles > 0 ? 1u : 0u);
    if (lightCount == 0)
        return;
    uint light = min(uint(rand(rng) * float(lightCount)), lightCount - 1);
    ray photonRay;
    photonRay.min_distance = 0.0f;
    photonRay.max_distance = 1e30f;
    float3 emission = float3(0.0f);
    float flux =
        float(lightCount) / (float(CAUSTIC_PHOTON_COUNT) * wavelength.y);
    float sourceRange = 0.0f;
    float sourceDistance = 0.0f;

    bool photonInsideMedium = false;
    uint photonMediumObjectId = 0xFFFFFFFFu;
    float photonSigmaA = 0.0f;
    if (light < sceneData.numDirectionalLights) {
        float3 direction = -sampleDirectionalLightDirection(dirLight, rng);
        float3x3 basis = buildOrthonormalBasis(direction);
        float angle = 2.0f * M_PI_F * rand(rng);
        float radius = caustics.bounds.w * sqrt(rand(rng));
        photonRay.origin =
            caustics.bounds.xyz - direction * caustics.launchDistance +
            radius * (cos(angle) * basis[0] + sin(angle) * basis[1]);
        photonRay.direction = direction;
        emission = dirLight.color * max(dirLight.intensity, 0.0f);
        flux *= M_PI_F * caustics.bounds.w * caustics.bounds.w;
    } else if ((light -= sceneData.numDirectionalLights) <
               sceneData.numPointLights) {
        PointLight source = pointLights[light];
        photonRay.origin = float3(source.position);
        float3 toBounds = caustics.bounds.xyz - photonRay.origin;
        float distanceSquared = dot(toBounds, toBounds);
        float radiusSquared = caustics.bounds.w * caustics.bounds.w;
        if (distanceSquared > radiusSquared) {
            float coneCosine =
                sqrt(max(0.0f, 1.0f - radiusSquared / distanceSquared));
            float cosine = mix(coneCosine, 1.0f, rand(rng));
            float angle = 2.0f * M_PI_F * rand(rng);
            float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
            photonRay.direction =
                buildOrthonormalBasis(normalize(toBounds)) *
                float3(sine * cos(angle), sine * sin(angle), cosine);
            flux *= 2.0f * M_PI_F * (1.0f - coneCosine);
        } else {
            float cosine = 1.0f - 2.0f * rand(rng);
            float angle = 2.0f * M_PI_F * rand(rng);
            float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
            photonRay.direction =
                float3(sine * cos(angle), sine * sin(angle), cosine);
            flux *= 4.0f * M_PI_F;
        }
        emission = float3(source.color) * max(source.intensity, 0.0f);
        sourceRange = source.range;
    } else if ((light -= sceneData.numPointLights) < sceneData.numSpotLights) {
        SpotLight source = spotLights[light];
        float cosine = mix(source.outerCos, 1.0f, rand(rng));
        float angle = 2.0f * M_PI_F * rand(rng);
        float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
        photonRay.origin = float3(source.position);
        photonRay.direction =
            buildOrthonormalBasis(normalize(float3(source.direction))) *
            float3(sine * cos(angle), sine * sin(angle), cosine);
        emission = float3(source.color) * max(source.intensity, 0.0f) *
                   smoothstep(source.outerCos, source.innerCos, cosine);
        flux *= 2.0f * M_PI_F * (1.0f - source.outerCos);
        sourceRange = source.range;
    } else if ((light -= sceneData.numSpotLights) < sceneData.numAreaLights) {
        AreaLight source = areaLights[light];
        float2 uv = float2(rand(rng), rand(rng)) * 2.0f - 1.0f;
        photonRay.origin = float3(source.position) +
                           float3(source.right) * uv.x * source.halfWidth +
                           float3(source.up) * uv.y * source.halfHeight;
        float3 normal =
            normalize(cross(float3(source.right), float3(source.up)));
        if (source.twoSided > 0.5f && rand(rng) < 0.5f)
            normal = -normal;
        float sineSquared =
            rand(rng) * (1.0f - source.emissionCos * source.emissionCos);
        float angle = 2.0f * M_PI_F * rand(rng);
        photonRay.direction =
            buildOrthonormalBasis(normal) *
            float3(sqrt(sineSquared) * cos(angle),
                   sqrt(sineSquared) * sin(angle), sqrt(1.0f - sineSquared));
        emission = float3(source.color) * max(source.intensity, 0.0f);
        flux *= M_PI_F * 4.0f * source.halfWidth * source.halfHeight *
                (1.0f - source.emissionCos * source.emissionCos) *
                (source.twoSided > 0.5f ? 2.0f : 1.0f);
        float3 toBounds = caustics.bounds.xyz - photonRay.origin;
        float distanceSquared = dot(toBounds, toBounds);
        float radiusSquared = caustics.bounds.w * caustics.bounds.w;
        if (source.emissionCos < 0.01f && distanceSquared > radiusSquared) {
            float coneCos =
                sqrt(max(0.0f, 1.0f - radiusSquared / distanceSquared));
            float cosine = mix(coneCos, 1.0f, rand(rng));
            float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
            float azimuth = 2.0f * M_PI_F * rand(rng);
            photonRay.direction =
                buildOrthonormalBasis(normalize(toBounds)) *
                float3(sine * cos(azimuth), sine * sin(azimuth), cosine);
            float emissionCosine = dot(normal, photonRay.direction);
            if (emissionCosine <= source.emissionCos)
                return;
            flux *= 2.0f * (1.0f - coneCos) * emissionCosine /
                    (1.0f - source.emissionCos * source.emissionCos);
        }
    } else {
        float selector = rand(rng);
        uint first = 0, last = sceneData.numEmissiveTriangles - 1;
        while (first < last) {
            uint middle = (first + last) / 2;
            if (selector <= emissiveTriangles[middle].cdf)
                last = middle;
            else
                first = middle + 1;
        }
        EmissiveTriangle source = emissiveTriangles[first];
        float u = sqrt(rand(rng)), v = rand(rng);
        photonRay.origin = source.p0.xyz * (1.0f - u) +
                           source.p1.xyz * u * (1.0f - v) +
                           source.p2.xyz * u * v;
        float3 normal =
            rand(rng) < 0.5f ? source.normal.xyz : -source.normal.xyz;
        float3 toBounds = caustics.bounds.xyz - photonRay.origin;
        float distanceSquared = dot(toBounds, toBounds);
        float radiusSquared = caustics.bounds.w * caustics.bounds.w;
        if (distanceSquared > radiusSquared) {
            float coneCosine =
                sqrt(max(0.0f, 1.0f - radiusSquared / distanceSquared));
            float cosine = mix(coneCosine, 1.0f, rand(rng));
            float angle = 2.0f * M_PI_F * rand(rng);
            float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
            photonRay.direction =
                buildOrthonormalBasis(normalize(toBounds)) *
                float3(sine * cos(angle), sine * sin(angle), cosine);
            float emissionCosine = dot(normal, photonRay.direction);
            if (emissionCosine <= 0.0f)
                return;
            float solidAngle = 2.0f * M_PI_F * (1.0f - coneCosine);
            flux *= 2.0f * solidAngle * emissionCosine * source.area /
                    max(source.selectionPdf, 1e-8f);
        } else {
            photonRay.direction =
                buildOrthonormalBasis(normal) *
                cosineSampleHemisphere(float2(rand(rng), rand(rng)));
            flux *=
                2.0f * M_PI_F * source.area /
                max(source.selectionPdf, 1e-8f);
        }
        photonRay.origin =
            offsetRayOrigin(photonRay.origin, normal, photonRay.direction);
        emission = float3(source.emission);
    }
    flux *= rgbToEmissionAtWavelength(emission, wavelength.x);
    intersector<triangle_data> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.set_triangle_cull_mode(triangle_cull_mode::none);
    bool specularPath = false;
    uint interactions = 0;
    for (uint step = 0; step < 32 && interactions < 12; ++step) {
        auto hit = isect.intersect(photonRay, sceneAS);
        if (hit.type == intersection_type::none || flux <= 0.0f ||
            !isfinite(flux))
            return;
        if (photonInsideMedium) {
            flux *= exp(-photonSigmaA * hit.distance);

            if (flux <= 1e-8f || !isfinite(flux)) {
                return;
            }
        }

        sourceDistance += hit.distance;
        uint primitive =
            blasPrimitiveOffsets[hit.geometry_id] + hit.primitive_id;
        uint objectId = primitiveObjects[primitive];
        Material mat = materials[objectId];
        uint i0 = indices[primitive * 3], i1 = indices[primitive * 3 + 1],
             i2 = indices[primitive * 3 + 2];
        float2 bary = hit.triangle_barycentric_coord;
        float2 uv = (float2(vertices[i0].uv) * (1.0f - bary.x - bary.y) +
                     float2(vertices[i1].uv) * bary.x +
                     float2(vertices[i2].uv) * bary.y) *
                        float2(mat.textureScale) +
                    float2(mat.textureOffset);
        float3 P = photonRay.origin + hit.distance * photonRay.direction;
        float opacity = resolveMaterialOpacity(
            mat, uv, sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        if (rand(rng) >= opacity) {
            photonRay.origin = P + photonRay.direction * rayOffsetDistance(P);
            continue;
        }
        if (interactions == 0 && sourceRange > 0.0f) {
            float minDistance = max(sourceRange * 0.08f, 0.15f);
            flux *=
                sourceDistance * sourceDistance /
                (sourceDistance * sourceDistance + minDistance * minDistance);
            flux *= 1.0f - smoothstep(sourceRange * 0.75f, sourceRange,
                                      sourceDistance);
        }
        ++interactions;
        float3 p0 = float3(vertices[i0].position);
        float3 p1 = float3(vertices[i1].position);
        float3 p2 = float3(vertices[i2].position);

        float3 localGeometricNormal =
            normalizeOr(cross(p1 - p0, p2 - p0), float3(0.0, 1.0, 0.0));

        float3 worldGeometricNormal = normalizeOr(
            localGeometricNormal, float3(0.0, 1.0, 0.0));

        bool frontFace = dot(worldGeometricNormal, photonRay.direction) < 0.0f;
        float3 Ng = frontFace ? worldGeometricNormal : -worldGeometricNormal;
        float3 albedo, emissive;
        float metallic, roughness, ao, baseIor, transmission, abbe;
        resolveMaterialParameters(mat, uv, sceneData.materialTextureCount,
                                  PT_MATERIAL_TEXTURE_ARGS, albedo, metallic,
                                  roughness, ao, emissive, baseIor,
                                  transmission, abbe);
        float b0 = 1.0f - bary.x - bary.y;
        float3 localN = float3(vertices[i0].normal) * b0 +
                        float3(vertices[i1].normal) * bary.x +
                        float3(vertices[i2].normal) * bary.y;
        float3 localT = float3(vertices[i0].tangent) * b0 +
                        float3(vertices[i1].tangent) * bary.x +
                        float3(vertices[i2].tangent) * bary.y;
        float3 localB = float3(vertices[i0].bitangent) * b0 +
                        float3(vertices[i1].bitangent) * bary.x +
                        float3(vertices[i2].bitangent) * bary.y;
        float3 N = resolveShadingNormal(
            mat, uv, localN, localT, localB, instanceData[objectId],
            sceneData.materialTextureCount, PT_MATERIAL_TEXTURE_ARGS);
        N = dot(N, Ng) >= 0.0f ? N : -N;
        if (dot(N, Ng) < 0.1f)
            N = normalizeOr(N + Ng * (0.1f - dot(N, Ng)), Ng);
        bool deltaSurface = roughness <= 0.005f;

        bool causticTransportSurface =
            roughness < 0.25f && (transmission > 0.001f || metallic > 0.001f);
        if (specularPath && !causticTransportSurface) {
            CausticPhoton photon;

            photon.positionWavelength = float4(P, wavelength.x);

            photon.normalPower = float4(Ng, flux);

            photon.incomingObject =
                float4(-photonRay.direction, float(objectId));

            photons[id] = photon;

            int3 cell = int3(floor(P / caustics.radius));

            insertCausticPhoton(photonSlots, cell, id, caustics.seed);

            return;
        }
        if (!causticTransportSurface)
            return;
        float3 opticalNormal = N;
        float3 interfaceNormal = opticalNormal;

        if (!deltaSurface) {
            float3 V = -photonRay.direction;

            float3x3 basis = buildOrthonormalBasis(opticalNormal);

            float3 localView = float3(dot(V, basis[0]), dot(V, basis[1]),
                                      dot(V, opticalNormal));

            float3 localH = sampleGGXVNDF(localView, roughness,
                                          float2(rand(rng), rand(rng)));

            interfaceNormal = normalizeOr(basis * localH, opticalNormal);
        }

        float ior = evaluateIorAtWavelength(baseIor, abbe, path).x;
        float eta = frontFace ? 1.0f / ior : ior;
        float cosine = max(dot(-photonRay.direction, interfaceNormal), 0.0f);
        float fresnel = dielectricFresnel(cosine, float4(eta)).x;
        float reflectance =
            mix(fresnel, rgbToReflectanceAtWavelength(albedo, wavelength.x),
                metallic);
        float transmittance =
            (1.0f - fresnel) * transmission * (1.0f - metallic);
        float choice = rand(rng);

        float3 nextDirection;
        if (choice < reflectance) {
            nextDirection = reflect(photonRay.direction, interfaceNormal);
        } else if (choice < reflectance + transmittance) {
            nextDirection = refract(photonRay.direction, interfaceNormal, eta);

            if (dot(nextDirection, nextDirection) < 1e-8f) {
                nextDirection = reflect(photonRay.direction, interfaceNormal);
            } else {
                if (frontFace) {
                    photonInsideMedium = true;
                    photonMediumObjectId = objectId;

                    float3 attenuationColor =
                        clamp(float3(mat.attenuationColor), float3(0.001f),
                              float3(1.0f));

                    float attenuationDistance =
                        max(mat.attenuationDistance, 1e-4f);

                    float spectralAttenuation =
                        clamp(rgbToReflectanceAtWavelength(attenuationColor,
                                                           wavelength.x),
                              0.001f, 1.0f);

                    photonSigmaA =
                        -log(spectralAttenuation) / attenuationDistance;
                } else if (photonInsideMedium &&
                           photonMediumObjectId == objectId) {
                    photonInsideMedium = false;
                    photonMediumObjectId = 0xFFFFFFFFu;
                    photonSigmaA = 0.0f;
                }
            }
        } else {
            return;
        }
        specularPath = true;
        photonRay.origin = offsetRayOrigin(P, Ng, nextDirection);
        photonRay.direction = normalize(nextDirection);
    }
}
