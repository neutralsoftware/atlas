float2 octEncode(float3 n) {
    n /= (fabs(n.x) + fabs(n.y) + fabs(n.z) + 1e-6f);
    float2 e = n.xy;

    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(e.x >= 0.0f ? 1.0f : -1.0f, e.y >= 0.0f ? 1.0f : -1.0f);
        e = (1.0f - fabs(e.yx)) * signNotZero;
    }
    return e;
}

float3 safeNormalizeDDGI(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10f) {
        return v * rsqrt(len2);
    }
    return fallback;
}

uint probeIndexFromCoord(uint3 c, uint3 counts) {
    return c.x + counts.x * (c.y + counts.y * c.z);
}

float2 ddgiAtlasUV(uint probeIndex, float3 dirWS,
                                 ProbeSpace ps, uint atlasW,
                                 uint atlasH) {
    if (atlasW == 0u || atlasH == 0u) {
        return float2(0.5f);
    }

    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    if (innerRes == 0u || probesPerRow == 0u) {
        return float2(0.5f);
    }

    uint tileRes = innerRes + 2u * border;

    uint tileX = probeIndex % probesPerRow;
    uint tileY = probeIndex / probesPerRow;

    float dirLen = length(dirWS);
    float3 safeDir =
        (dirLen > 1e-6f) ? (dirWS / float3(dirLen)) : float3(0.0f, 1.0f, 0.0f);
    float2 e = octEncode(safeDir);
    float2 innerUV = e * 0.5f + 0.5f;

    float2 innerPx = float2((float)(tileX * tileRes + border),
                            (float)(tileY * tileRes + border)) +
                     innerUV * float(innerRes - 1u);

    return (innerPx + 0.5f) / float2((float)atlasW, (float)atlasH);
}

float4 sampleDDGITextureBilinear(Texture2D<float4> tex,
                                               float2 uv) {
    uint w = textureSize(tex).x;
    uint h = textureSize(tex).y;
    if (w == 0u || h == 0u) {
        return float4(0.0f);
    }

    float2 pixel = uv * float2((float)w, (float)h) - 0.5f;
    int2 p0 = int2(floor(pixel));
    int2 p1 = p0 + int2(1, 0);
    int2 p2 = p0 + int2(0, 1);
    int2 p3 = p0 + int2(1, 1);

    int maxX = int(w) - 1;
    int maxY = int(h) - 1;
    p0 = int2(clamp(p0.x, 0, maxX), clamp(p0.y, 0, maxY));
    p1 = int2(clamp(p1.x, 0, maxX), clamp(p1.y, 0, maxY));
    p2 = int2(clamp(p2.x, 0, maxX), clamp(p2.y, 0, maxY));
    p3 = int2(clamp(p3.x, 0, maxX), clamp(p3.y, 0, maxY));

    float2 f = frac(pixel);
    float4 c00 = tex.Load(int3(uint2((uint)p0.x, (uint)p0.y), 0));
    float4 c10 = tex.Load(int3(uint2((uint)p1.x, (uint)p1.y), 0));
    float4 c01 = tex.Load(int3(uint2((uint)p2.x, (uint)p2.y), 0));
    float4 c11 = tex.Load(int3(uint2((uint)p3.x, (uint)p3.y), 0));
    float4 cx0 = lerp(c00, c10, f.x);
    float4 cx1 = lerp(c01, c11, f.x);
    return lerp(cx0, cx1, f.y);
}

float4
sampleProbeDirectionalRadiance(Texture2D<float4> ddgiTexture,
                               ProbeSpace ps, uint probeIndex,
                               uint atlasW, uint atlasH, float3 dirWS) {
    float2 uv = ddgiAtlasUV(probeIndex, dirWS, ps, atlasW, atlasH);
    return sampleDDGITextureBilinear(ddgiTexture, uv);
}

float3 sampleDDGIIrradiance(Texture2D<float4> ddgiTexture,
                                          Texture2D<float4> ddgiDistance,
                                          ProbeSpace ps, float3 posWS,
                                          float3 normalWS) {
    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);

    if (counts.x == 0u || counts.y == 0u || counts.z == 0u)
        return float3(0.0);

    uint atlasW = textureSize(ddgiTexture).x;
    uint atlasH = textureSize(ddgiTexture).y;
    if (atlasW == 0u || atlasH == 0u) {
        return float3(0.0);
    }

    float normalLen = length(normalWS);
    float3 safeNormal = (normalLen > 1e-6f) ? (normalWS / float3(normalLen))
                                            : float3(0.0f, 1.0f, 0.0f);

    float3 safeSpacing = max(ps.spacing, float3(1e-4f));
    float spacingScale =
        max(max(safeSpacing.x, max(safeSpacing.y, safeSpacing.z)), 1e-4f);
    float3 grid = (posWS - ps.origin) / safeSpacing;

    if (counts.x < 2u || counts.y < 2u || counts.z < 2u) {
        float3 maxCoord = max(float3(0.0f), float3((float)counts.x - 1.0f,
                                                   (float)counts.y - 1.0f,
                                                   (float)counts.z - 1.0f));
        float3 clampedGrid = clamp(grid, float3(0.0f), maxCoord);
        uint3 nearest = uint3(
            (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
            (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
            (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0,
                        int(counts.z) - 1));
        uint pIndex = probeIndexFromCoord(nearest, counts);
        float4 nearestSample = sampleProbeDirectionalRadiance(
            ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
        float3 nearestProbePos = ps.origin + float3(nearest) * safeSpacing;
        float3 nearestDirection = safeNormalizeDDGI(
            posWS - nearestProbePos, safeNormal);
        float4 nearestMoments = sampleProbeDirectionalRadiance(
            ddgiDistance, ps, pIndex, atlasW, atlasH, nearestDirection);
        float nearestDistance = length(posWS - nearestProbePos);
        float nearestVariance = max(nearestMoments.y - nearestMoments.x *
                                                           nearestMoments.x,
                                    0.0001f);
        float nearestDelta = max(nearestDistance - nearestMoments.x, 0.0f);
        float nearestVisibility = nearestDelta <= 0.0f
                                      ? 1.0f
                                      : nearestVariance /
                                            (nearestVariance + nearestDelta *
                                                                   nearestDelta);
        float nearestValidity = isfinite(nearestSample.w)
                                    ? clamp(nearestSample.w, 0.0f, 1.0f)
                                    : 0.0f;
        float3 nearestIrr =
            all(isfinite(nearestSample.xyz)) ? nearestSample.xyz : float3(0.0f);
        nearestIrr *= nearestValidity * nearestVisibility;
        return max(nearestIrr, float3(0.0f));
    }

    float3 maxGrid =
        float3((float)counts.x - 1.0001f, (float)counts.y - 1.0001f,
               (float)counts.z - 1.0001f);
    float3 clampedGrid = clamp(grid, float3(0.0f), maxGrid);

    float3 baseF = floor(clampedGrid);
    float3 frac = clampedGrid - baseF;
    int3 baseI = int3(baseF);
    int3 maxBase =
        int3(int(counts.x) - 2, int(counts.y) - 2, int(counts.z) - 2);
    baseI = clamp(baseI, int3(0), maxBase);

    float3 result = float3(0.0f);
    float weightSum = 0.0f;
    float3 resultNoBackface = float3(0.0f);
    float weightSumNoBackface = 0.0f;

    for (uint dz = 0; dz <= 1u; dz++) {
        for (uint dy = 0; dy <= 1u; dy++) {
            for (uint dx = 0; dx <= 1u; dx++) {

                uint3 pc = uint3(uint(baseI.x) + dx, uint(baseI.y) + dy,
                                 uint(baseI.z) + dz);
                uint pIndex = probeIndexFromCoord(pc, counts);

                float trilinearW = ((dx == 0u) ? (1.0f - frac.x) : frac.x) *
                                   ((dy == 0u) ? (1.0f - frac.y) : frac.y) *
                                   ((dz == 0u) ? (1.0f - frac.z) : frac.z);

                float3 probePos = ps.origin + float3(pc) * safeSpacing;
                float3 surfaceToProbe = probePos - posWS;
                float sDist = length(surfaceToProbe);
                float3 dirToProbe = (sDist > 1e-4f) ? surfaceToProbe / sDist
                                                    : float3(0.0f, 1.0f, 0.0f);
                float frontW = max(dot(safeNormal, dirToProbe), 0.0f);
                float backfaceW =
                    lerp(0.180000007, 1.0f, frontW * frontW);
                float distNorm = sDist / spacingScale;
                float distNorm2 = distNorm * distNorm;
                float distanceW =
                    1.0f / (1.0f + distNorm2 * 0.850000024);

                float w = trilinearW * backfaceW * distanceW;

                float4 irrSample = sampleProbeDirectionalRadiance(
                    ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
                float3 probeToSurface = -surfaceToProbe;
                float4 distanceSample = sampleProbeDirectionalRadiance(
                    ddgiDistance, ps, pIndex, atlasW, atlasH,
                    safeNormalizeDDGI(probeToSurface, safeNormal));
                float variance = max(distanceSample.y -
                                         distanceSample.x * distanceSample.x,
                                     0.0001f);
                float delta = max(sDist - distanceSample.x, 0.0f);
                float visibility = delta <= 0.0f
                                       ? 1.0f
                                       : variance / (variance + delta * delta);
                visibility = visibility * visibility * visibility;
                float probeValidity = isfinite(irrSample.w)
                                          ? clamp(irrSample.w, 0.0f, 1.0f)
                                          : 0.0f;
                float validityW = probeValidity;
                float3 irr = irrSample.xyz;
                if (!all(isfinite(irr))) {
                    irr = float3(0.0f);
                    validityW = 0.0f;
                }
                irr = max(irr, float3(0.0f));
                float weightedW = w * validityW * max(visibility, 0.001f);
                result += irr * weightedW;
                weightSum += weightedW;
                float noBackfaceW = trilinearW * distanceW * validityW *
                                    max(visibility, 0.001f);
                resultNoBackface += irr * noBackfaceW;
                weightSumNoBackface += noBackfaceW;
            }
        }
    }

    if (weightSum > 1e-5f) {
        return result / weightSum;
    }
    if (weightSumNoBackface > 1e-5f) {
        return resultNoBackface / weightSumNoBackface;
    }

    uint3 nearest = uint3(
        (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
        (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
        (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0, int(counts.z) - 1));
    uint nearestIndex = probeIndexFromCoord(nearest, counts);
    float4 fallbackSample = sampleProbeDirectionalRadiance(
        ddgiTexture, ps, nearestIndex, atlasW, atlasH, safeNormal);
    float fallbackValidity =
        isfinite(fallbackSample.w) ? clamp(fallbackSample.w, 0.0f, 1.0f) : 0.0f;
    float3 fallback =
        all(isfinite(fallbackSample.xyz)) ? fallbackSample.xyz : float3(0.0f);
    fallback *= fallbackValidity;
    if (!all(isfinite(fallback))) {
        fallback = float3(0.0f);
    }
    return max(fallback, float3(0.0f));
}
