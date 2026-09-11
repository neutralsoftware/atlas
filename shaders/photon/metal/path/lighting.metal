float3 evalEmissiveTriangleLighting(
    intersector<triangle_data> isect,
    primitive_acceleration_structure sceneAS, float3 P, float3 N, float3 Ng,
    float3 V, float3 albedo, float metallic, float roughness,
    float reflectivity, float ior, float transmittance, thread uint &rng,
    constant SceneData &sceneData,
    constant EmissiveTriangle *emissiveTriangles,
    constant Material *materials, constant uint *primitiveObjects,
    constant uint *blasPrimitiveOffsets, constant VertexData *vertices,
    constant uint *indices, constant InstanceData *instanceData,
    PT_MATERIAL_TEXTURE_PARAMS) {
    if (sceneData.numEmissiveTriangles == 0) {
        return float3(0.0);
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
    float3 lightPosition = light.p0.xyz * b0 + light.p1.xyz * b1 +
                           light.p2.xyz * b2;
    float3 toLight = lightPosition - P;
    float distanceSquared = dot(toLight, toLight);
    if (distanceSquared <= 1e-8) {
        return float3(0.0);
    }
    float distanceToLight = sqrt(distanceSquared);
    float3 L = toLight / distanceToLight;
    float surfaceCosine = dot(N, L);
    float lightCosine = abs(dot(light.normal.xyz, -L));
    if (surfaceCosine <= 0.0 || dot(Ng, L) <= 0.0 ||
        lightCosine <= 1e-5 || light.area <= 1e-8 ||
        light.selectionPdf <= 1e-8) {
        return float3(0.0);
    }
    float solidAnglePdf = light.selectionPdf * distanceSquared /
                          max(lightCosine * light.area, 1e-8);
    float3 visibility = traceShadowVisibility(
        isect, sceneAS, P, Ng, L, distanceToLight, rng, materials,
        primitiveObjects, blasPrimitiveOffsets, vertices, indices,
        instanceData, sceneData, PT_MATERIAL_TEXTURE_ARGS);
    return evalPBR(albedo, metallic, roughness, reflectivity, ior,
                   transmittance, N, V, L, light.emission,
                   1.0 / max(solidAnglePdf, 1e-8)) *
           visibility;
}

// ---------------------------------------------------------------------------
// Direct lighting with full PBR (replaces old evalDirectLighting)
// ---------------------------------------------------------------------------

float3 evalDirectLightingPBR(intersector<triangle_data> isect,
                             primitive_acceleration_structure sceneAS, float3 P,
                             float3 N, float3 Ng, float3 V, float3 albedo,
                             float metallic, float roughness, float reflectivity,
                             float ior, float transmittance, float sssStrength,
                             float sssThickness,
                             thread uint &rng,
                             constant DirectionalLightData &dirLight,
                             constant SceneData &sceneData,
                             constant PointLight *pointLights,
                             constant SpotLight *spotLights,
                             constant AreaLight *areaLights,
                             constant EmissiveTriangle *emissiveTriangles,
                             constant Material *materials,
                             constant uint *primitiveObjects,
                             constant uint *blasPrimitiveOffsets,
                             constant VertexData *vertices,
                             constant uint *indices,
                             constant InstanceData *instanceData,
                             PT_MATERIAL_TEXTURE_PARAMS) {
    float3 lighting = float3(0.0);
    // Directional
    if (sceneData.numDirectionalLights > 0) {
        float3 L = sampleDirectionalLightDirection(dirLight, rng);
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, ior,
                           transmittance, N, V, L, dirLight.color,
                           max(dirLight.intensity, 0.0));
        float3 s = evalSubsurface(albedo, N, V, L, dirLight.color,
                                  max(dirLight.intensity, 0.0), roughness,
                                  sssStrength, sssThickness);
        float3 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, 1e30, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            PT_MATERIAL_TEXTURE_ARGS);
        lighting += (c + s * (1.0 - transmittance)) * visibility;
    }

    // Point lights
    for (uint i = 0; i < sceneData.numPointLights; ++i) {
        float3 sampledPosition = pointLights[i].position;
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float lightRange = max(pointLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(pointLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, ior,
                           transmittance, N, V, L, pointLights[i].color,
                           intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, pointLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            PT_MATERIAL_TEXTURE_ARGS);
        lighting += (c + s * (1.0 - transmittance)) * visibility;
    }

    // Spot lights
    for (uint i = 0; i < sceneData.numSpotLights; ++i) {
        float3 sampledPosition = spotLights[i].position;
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 fwd = normalize(spotLights[i].direction);
        float spotCos = dot(-L, fwd);
        float spot =
            smoothstep(spotLights[i].outerCos, spotLights[i].innerCos, spotCos);
        float lightRange = max(spotLights[i].range, 1e-4);
        float minDist = max(lightRange * 0.08, 0.15);
        float distSq = dist * dist + minDist * minDist;
        float rangeFade = 1.0 - smoothstep(lightRange * 0.75, lightRange, dist);
        float atten = rangeFade / max(distSq, 1e-4);
        float intensity = max(spotLights[i].intensity, 0.0) * atten * spot;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, ior,
                           transmittance, N, V, L, spotLights[i].color,
                           intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, spotLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            PT_MATERIAL_TEXTURE_ARGS);
        lighting += (c + s * (1.0 - transmittance)) * visibility;
    }

    // Area lights
    for (uint i = 0; i < sceneData.numAreaLights; ++i) {
        float2 lightSample = float2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float3 sampledPosition =
            areaLights[i].position +
            areaLights[i].right * (lightSample.x * areaLights[i].halfWidth) +
            areaLights[i].up * (lightSample.y * areaLights[i].halfHeight);
        float3 toLight = sampledPosition - P;
        float dist = max(length(toLight), 1e-4);
        float3 L = toLight / dist;
        float3 lightNorm =
            normalize(cross(areaLights[i].right, areaLights[i].up));
        float cosLight = areaLights[i].twoSided > 0.5
                             ? abs(dot(lightNorm, -L))
                             : max(dot(lightNorm, -L), 0.0);
        float area = 4.0 * areaLights[i].halfWidth * areaLights[i].halfHeight;
        float lightPdfArea = 1.0 / max(area, 1e-6);
        float distSq = max(dist * dist, 1e-6);
        float atten = cosLight / max(distSq * lightPdfArea, 1e-6);
        float intensity = max(areaLights[i].intensity, 0.0) * atten;
        float3 c = evalPBR(albedo, metallic, roughness, reflectivity, ior,
                           transmittance, N, V, L, areaLights[i].color,
                           intensity);
        float3 s =
            evalSubsurface(albedo, N, V, L, areaLights[i].color, intensity,
                           roughness, sssStrength, sssThickness);
        float3 visibility = traceShadowVisibility(
            isect, sceneAS, P, Ng, L, dist, rng, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, sceneData,
            PT_MATERIAL_TEXTURE_ARGS);
        lighting += (c + s * (1.0 - transmittance)) * visibility;
    }

    lighting += evalEmissiveTriangleLighting(
        isect, sceneAS, P, N, Ng, V, albedo, metallic, roughness,
        reflectivity, ior, transmittance, rng, sceneData, emissiveTriangles,
        materials, primitiveObjects, blasPrimitiveOffsets, vertices, indices,
        instanceData, PT_MATERIAL_TEXTURE_ARGS);

    return lighting;
}

// ---------------------------------------------------------------------------
// sampleRadiance — iterative path with GGX importance-sampled indirect bounces
// ---------------------------------------------------------------------------

