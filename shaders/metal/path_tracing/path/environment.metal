constexpr sampler skyboxSampler(coord::normalized, address::clamp_to_edge,
                                filter::linear, mip_filter::linear);

float3 skyColor(float3 dir, float intensity, texturecube<float> skybox,
                constant SceneData &sceneData) {
    float3 sampleDir = dir;
    float len2 = dot(sampleDir, sampleDir);
    if (len2 > 1e-10) {
        sampleDir *= rsqrt(len2);
    } else {
        sampleDir = float3(0.0, 1.0, 0.0);
    }
    float3 sky = skybox.sample(skyboxSampler, sampleDir).xyz;
    if (sceneData.atmosphereEnabled != 0) {
        float horizon = pow(clamp(1.0 - abs(sampleDir.y), 0.0, 1.0), 4.0);
        float daylight = smoothstep(-0.2, 0.15,
                                    sceneData.atmosphereSunDirection.y);
        float3 zenith = float3(0.08, 0.28, 0.65);
        float3 horizonColor = float3(0.58, 0.72, 0.92);
        float3 proceduralSky = mix(zenith, horizonColor, horizon) *
                               max(daylight, 0.08);
        sky = max(sky, proceduralSky);
    }
    if (sceneData.atmosphereEnabled != 0 &&
        sceneData.atmosphereSunDirection.y > -0.15) {
        float3 sunDirection = sceneData.atmosphereSunDirection;
        float sunDirectionLength = dot(sunDirection, sunDirection);
        sunDirection = sunDirectionLength > 1e-10
                           ? sunDirection * rsqrt(sunDirectionLength)
                           : float3(0.0, 1.0, 0.0);
        float sunDot = dot(sampleDir, sunDirection);
        float sizeAdjust =
            1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.001;
        float sunSize = 0.9995 * sizeAdjust;
        float sunGlowSize =
            0.998 * (1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.003);
        float sunHaloSize =
            0.99 * (1.0 - (sceneData.atmosphereSunSize - 1.0) * 0.015);
        float sunDisk = smoothstep(sunSize - 0.0002, sunSize, sunDot);
        float sunGlow =
            smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
        float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) *
                        (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
        float horizonFade = smoothstep(
            -0.15, 0.05, sceneData.atmosphereSunDirection.y);
        float sunIntensity = max(sceneData.atmosphereSunIntensity, 0.05);
        sky += sceneData.atmosphereSunColor *
               (sunDisk * 5.0 + sunGlow * 0.5 + sunHalo) * horizonFade *
               sunIntensity;
    }
    float scale = intensity > 0.0 ? intensity : 1.0;
    return sky * scale;
}

