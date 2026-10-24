struct PhotonBeam {
    float4 originRadius;
    float4 directionLength;
    float4 powerDistance;
    float4 spread;
};

[[vk::binding(18, 1)]]
RWStructuredBuffer<PhotonBeam> photonBeams;

float3 scatteredPhotonBeams(Material material, float3 origin,
                            float3 direction, float distance) {
    if ((sceneData.featureFlags & PHOTON_FEATURE_CAUSTICS) == 0) {
        return 0.0;
    }
    float3 result = 0.0;
    float3 scattering = max(material.volumeScatteringColor, 0.0) *
        max(material.volumeScatteringStrength, 0.0) * max(material.volumeDensity, 0.0);
    uint pathCount = sceneData.numSpotLights * 6u;
    for (uint path = 0; path < pathCount; ++path) {
        uint base = path * sceneData.beamSegmentLimit;
        uint count = min(uint(photonBeams[base].directionLength.w), sceneData.beamSegmentLimit);
        for (uint segment = 1; segment < count; ++segment) {
            uint index = base + segment;
            PhotonBeam beam = photonBeams[index];
            if (beam.directionLength.w <= 0.0 || max(beam.powerDistance.x,
                max(beam.powerDistance.y, beam.powerDistance.z)) <= 0.0) {
                continue;
            }
            float spread = max(beam.spread.x, 1e-5);
            SpotLight cone = {};
            cone.position = beam.originRadius.xyz - beam.directionLength.xyz *
                (beam.originRadius.w / spread);
            cone.direction = beam.directionLength.xyz;
            cone.innerCos = rsqrt(1.0 + spread * spread);
            cone.outerCos = cone.innerCos;
            float begin = 0.0;
            float end = distance;
            float axialOrigin = dot(origin - beam.originRadius.xyz, cone.direction);
            float axialDirection = dot(direction, cone.direction);
            if (abs(axialDirection) < 1e-7) {
                if (axialOrigin < 0.0 || axialOrigin > beam.directionLength.w) {
                    continue;
                }
            } else {
                float a = -axialOrigin / axialDirection;
                float b = (beam.directionLength.w - axialOrigin) / axialDirection;
                begin = max(begin, min(a, b));
                end = min(end, max(a, b));
            }
            if (end <= begin || !clipSpotlightSegment(origin, direction, cone, begin, end)) {
                continue;
            }
            float stepLength = (end - begin) * 0.25;
            for (uint sample = 0; sample < 4; ++sample) {
                float t = begin + (float(sample) + 0.5) * stepLength;
                float along = clamp(dot(origin + direction * t - beam.originRadius.xyz,
                    cone.direction), 0.0, beam.directionLength.w);
                float traveled = max(beam.powerDistance.w + along, 0.01);
                float3 transmittance = volumeShadowTransmittance(
                    beam.originRadius.xyz, cone.direction, along);
                result += volumeTransmittance(material, t) * scattering *
                    volumePhase(dot(direction, -cone.direction), material.volumeAnisotropy) *
                    beam.powerDistance.xyz * transmittance * stepLength / (traveled * traveled);
            }
        }
    }
    return result;
}

[shader("compute")][numthreads(64, 1, 1)]
void traceLightBeams(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint path = dispatchThreadID.x;
    if (path >= sceneData.numSpotLights * 6u) {
        return;
    }
    uint base = path * sceneData.beamSegmentLimit;
    for (uint slot = 0; slot < sceneData.beamSegmentLimit; ++slot) {
        PhotonBeam empty = {};
        photonBeams[base + slot] = empty;
    }
    if ((sceneData.featureFlags & PHOTON_FEATURE_CAUSTICS) == 0) {
        return;
    }
    SpotLight light = spotLights[path / 6u];
    uint channel = path % 3u;
    bool reflectedBranch = (path % 6u) >= 3u;
    float3 power = light.color * max(light.intensity, 0.0) *
        (channel == 0 ? float3(1.0, 0.0, 0.0) :
         channel == 1 ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0));
    float3 origin = light.position;
    float3 direction = safeNormal(light.direction, float3(0.0, -1.0, 0.0));
    float cosine = clamp(min(light.innerCos, light.outerCos), 1e-4, 1.0);
    float spread = sqrt(max(1.0 - cosine * cosine, 0.0)) / cosine;
    float traveled = 0.0;
    bool insideGlass = false;
    float3 absorption = 0.0;
    for (uint bounce = 0; bounce < min(sceneData.maxBounces, sceneData.beamSegmentLimit); ++bounce) {
        SurfaceHit hit = traceNonVolumeSurface(origin, direction);
        float distance = min(hit.valid ? hit.distance : 1e20,
            max(light.range - traveled, 0.0));
        if (distance <= 0.001) {
            break;
        }
        if (bounce > 0 && !insideGlass) {
            PhotonBeam beam;
            beam.originRadius = float4(origin, max(spread * traveled, 0.001));
            beam.directionLength = float4(direction, distance);
            beam.powerDistance = float4(power, traveled);
            beam.spread = float4(spread, 0.0, 0.0, 0.0);
            photonBeams[base + bounce] = beam;
            photonBeams[base].directionLength.w = float(bounce + 1u);
        }
        power *= insideGlass ? exp(-absorption * distance)
                             : volumeShadowTransmittance(origin, direction, distance);
        traveled += distance;
        if (!hit.valid || distance < hit.distance ||
            max(power.x, max(power.y, power.z)) < 1e-8) {
            break;
        }
        float transmission = (sceneData.featureFlags & PHOTON_FEATURE_TRANSMISSION) != 0
            ? saturate(hit.material.transmittance) : 0.0;
        if (hit.material.roughness >= 0.25 ||
            (transmission < 0.001 && hit.material.metallic < 0.001)) {
            break;
        }
        float ior = dispersionIor(hit.material.ior, hit.material.abbeNumber, channel);
        float f0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
        float fresnel = f0 + (1.0 - f0) * pow5(1.0 - saturate(dot(-direction, hit.normal)));
        float3 refracted = refract(direction, hit.normal, hit.frontFace ? 1.0 / ior : ior);
        if (transmission < 0.001) {
            if (bounce == 0 && !reflectedBranch) {
                break;
            }
            direction = reflect(direction, hit.normal);
            power *= hit.material.albedo.xyz;
        } else if (dot(refracted, refracted) <= 1e-8) {
            if (bounce == 0 && reflectedBranch) {
                break;
            }
            direction = reflect(direction, hit.normal);
        } else if (bounce == 0 && reflectedBranch) {
            direction = reflect(direction, hit.normal);
            power *= fresnel;
        } else {
            direction = safeNormal(refracted, direction);
            power *= transmission * (1.0 - fresnel);
            insideGlass = hit.frontFace;
            absorption = hit.material.attenuationDistance > 0.001
                ? -log(clamp(hit.material.attenuationColor, 0.001, 1.0)) /
                    hit.material.attenuationDistance : float3(0.0);
        }
        origin = hit.position + direction * 0.002;
    }
}
