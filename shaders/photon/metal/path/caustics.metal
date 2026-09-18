#pragma once

#include "types.metal"
#include "sampling.metal"
#include "spectral.metal"
#include "geometry.metal"
#include "materials.metal"
#include "visibility.metal"
#include "brdf.metal"

uint causticBucket(int3 cell) {
    uint3 c = as_type<uint3>(cell);
    return wang_hash(c.x * 73856093u ^ c.y * 19349663u ^ c.z * 83492791u) &
           (CAUSTIC_BUCKET_COUNT - 1);
}

float4 gatherCaustics(float3 P, float3 N, float3 Ng, float3 V, uint objectId,
                      float4 albedo, float metallic, float roughness,
                      float reflectivity, float4 ior, float transmittance,
                      thread const SpectralPath &path,
                      constant CausticSettings &caustics,
                      device const CausticPhoton *photons,
                      device const uint *photonSlots) {
    float4 result = float4(0.0f);
    float radiusSquared = caustics.radius * caustics.radius;
    int3 center = int3(floor(P / caustics.radius));
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                int3 cell = center + int3(x, y, z);
                uint base =
                    causticBucket(cell) * (CAUSTIC_BUCKET_SAMPLES * 2 + 1);
                if (photonSlots[base] == 0)
                    continue;
                for (uint slot = 0; slot < CAUSTIC_BUCKET_SAMPLES; ++slot) {
                    uint count = photonSlots[base + 1 + slot * 2];
                    if (count == 0)
                        continue;
                    uint key = photonSlots[base + 2 + slot * 2];
                    CausticPhoton photon = photons[key & 65535u];
                    if (uint(photon.incomingObject.w) != objectId ||
                        any(int3(floor(photon.positionWavelength.xyz /
                                       caustics.radius)) != cell))
                        continue;
                    float3 delta = photon.positionWavelength.xyz - P;
                    float distanceSquared = dot(delta, delta);
                    float cosine = dot(N, photon.incomingObject.xyz);
                    if (distanceSquared >= radiusSquared || cosine <= 1e-4f ||
                        dot(Ng, photon.normalPower.xyz) < 0.95f ||
                        abs(dot(delta, Ng)) > caustics.radius * 0.1f)
                        continue;
                    float densityWeight =
                        2.0f * (1.0f - distanceSquared / radiusSquared) /
                        (M_PI_F * radiusSquared);
                    float4 wavelengthDelta =
                        (path.wavelengthNm - photon.positionWavelength.w) /
                        10.0f;
                    float4 spectrum =
                        exp(-0.5f * wavelengthDelta * wavelengthDelta) /
                        25.06628275f;
                    float4 irradiance = spectrum * photon.normalPower.w *
                                        float(count) * densityWeight;
                    result +=
                        evalPBR(albedo, metallic, roughness, reflectivity, ior,
                                transmittance, N, V, photon.incomingObject.xyz,
                                irradiance, 1.0f / cosine);
                }
            }
        }
    }
    return result;
}

kernel void clearCaustics(device atomic_uint *photonSlots [[buffer(16)]],
                          uint id [[thread_position_in_grid]]) {
    if (id >= CAUSTIC_BUCKET_COUNT)
        return;
    uint base = id * (CAUSTIC_BUCKET_SAMPLES * 2 + 1);
    atomic_store_explicit(&photonSlots[base], 0u, memory_order_relaxed);
    for (uint slot = 0; slot < CAUSTIC_BUCKET_SAMPLES; ++slot) {
        atomic_store_explicit(&photonSlots[base + 1 + slot * 2], 0u,
                              memory_order_relaxed);
        atomic_store_explicit(&photonSlots[base + 2 + slot * 2], 0xFFFFFFFFu,
                              memory_order_relaxed);
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
    float2 wavelength = sampleVisibleWavelength(
        (float(id) + rand(rng)) / float(CAUSTIC_PHOTON_COUNT));
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
        float cosine = 1.0f - 2.0f * rand(rng);
        float angle = 2.0f * M_PI_F * rand(rng);
        float sine = sqrt(max(0.0f, 1.0f - cosine * cosine));
        photonRay.origin = float3(source.position);
        photonRay.direction =
            float3(sine * cos(angle), sine * sin(angle), cosine);
        emission = float3(source.color) * max(source.intensity, 0.0f);
        flux *= 4.0f * M_PI_F;
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
        float sineSquared = rand(rng) *
                            (1.0f - source.emissionCos * source.emissionCos);
        float angle = 2.0f * M_PI_F * rand(rng);
        photonRay.direction = buildOrthonormalBasis(normal) *
            float3(sqrt(sineSquared) * cos(angle),
                   sqrt(sineSquared) * sin(angle), sqrt(1.0f - sineSquared));
        emission = float3(source.color) * max(source.intensity, 0.0f);
        flux *= M_PI_F * 4.0f * source.halfWidth * source.halfHeight *
                (1.0f - source.emissionCos * source.emissionCos) *
                (source.twoSided > 0.5f ? 2.0f : 1.0f);
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
        photonRay.direction =
            buildOrthonormalBasis(normal) *
            cosineSampleHemisphere(float2(rand(rng), rand(rng)));
        photonRay.origin =
            offsetRayOrigin(photonRay.origin, normal, photonRay.direction);
        emission = float3(source.emission);
        flux *= 2.0f * M_PI_F * source.area / max(source.selectionPdf, 1e-8f);
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
        float3 normal = normalizeOr(
            cross(float3(vertices[i1].position) - float3(vertices[i0].position),
                  float3(vertices[i2].position) -
                      float3(vertices[i0].position)),
            -photonRay.direction);
        bool frontFace = dot(normal, photonRay.direction) < 0.0f;
        float3 Ng = frontFace ? normal : -normal;
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
        bool smooth = roughness <= 0.025f;
        if (specularPath &&
            (!smooth || (1.0f - metallic) * (1.0f - transmission) > 0.001f)) {
            CausticPhoton photon;
            photon.positionWavelength = float4(P, wavelength.x);
            photon.normalPower = float4(Ng, flux);
            photon.incomingObject =
                float4(-photonRay.direction, float(objectId));
            photons[id] = photon;
            uint base = causticBucket(int3(floor(P / caustics.radius))) *
                        (CAUSTIC_BUCKET_SAMPLES * 2 + 1);
            uint slot =
                wang_hash(id ^ 0x68bc21ebu) & (CAUSTIC_BUCKET_SAMPLES - 1);
            uint priority =
                wang_hash(id ^ caustics.seed ^ 0x967a889bu) & 65535u;
            uint key = (priority << 16) | id;
            atomic_fetch_add_explicit(&photonSlots[base], 1u,
                                      memory_order_relaxed);
            atomic_fetch_add_explicit(&photonSlots[base + 1 + slot * 2], 1u,
                                      memory_order_relaxed);
            atomic_fetch_min_explicit(&photonSlots[base + 2 + slot * 2], key,
                                      memory_order_relaxed);
            return;
        }
        if (!smooth)
            return;
        float ior = evaluateIorAtWavelength(baseIor, abbe, path).x;
        float eta = frontFace ? 1.0f / ior : ior;
        float cosine = max(dot(-photonRay.direction, N), 0.0f);
        float fresnel = dielectricFresnel(cosine, float4(eta)).x;
        float reflectance =
            mix(fresnel, rgbToReflectanceAtWavelength(albedo, wavelength.x),
                metallic);
        float transmittance =
            (1.0f - fresnel) * transmission * (1.0f - metallic);
        float choice = rand(rng);
        float3 nextDirection;
        if (choice < reflectance) {
            nextDirection = reflect(photonRay.direction, N);
        } else if (choice < reflectance + transmittance) {
            nextDirection = refract(photonRay.direction, N, eta);
            flux *=
                mix(1.0f, rgbToReflectanceAtWavelength(albedo, wavelength.x),
                    0.15f);
        } else
            return;
        specularPath = true;
        photonRay.origin = offsetRayOrigin(P, Ng, nextDirection);
        photonRay.direction = normalize(nextDirection);
    }
}
