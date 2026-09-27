struct ProbeSpace {
    float3 origin;
    float _pad0;
    float3 spacing;
    float _pad1;
    float3 probeCount;
    float _pad2;
    float4 debugColor;
    float4 atlasParams;
};

struct RaytracingSettings {
    uint raysPerProbe;
    float maxRayDistance;
    float normalBias;
    float hysteresis;
    uint frameIndex;
    uint probeUpdateOffset;
    uint probeUpdateStride;
    uint probeUpdateCount;
};

[[vk::binding(0, 1)]] StructuredBuffer<float4> probeRadiance;
[[vk::binding(1, 0)]] ConstantBuffer<ProbeSpace> ps;
[[vk::binding(2, 0)]] ConstantBuffer<RaytracingSettings> rt;
[[vk::binding(0, 2)]] RWTexture2D<float4> outTexture;
[[vk::binding(1, 2)]] Texture2D<float4> prevTexture;
[[vk::binding(2, 2)]] RWTexture2D<float4> outDistance;
[[vk::binding(3, 2)]] Texture2D<float4> prevDistance;

float3 octDecode(float2 encoded) {
    float3 normal = float3(encoded.x, encoded.y,
                           1.0 - abs(encoded.x) - abs(encoded.y));
    if (normal.z < 0.0) {
        normal.xy = (1.0 - abs(normal.yx)) *
                    float2(normal.x >= 0.0 ? 1.0 : -1.0,
                           normal.y >= 0.0 ? 1.0 : -1.0);
    }
    return normalize(normal);
}

float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    float value = float(index) + 0.5;
    float phi = 6.28318530718 * frac(value / 1.61803398875);
    phi += float((frameIndex * 1471u + 5743u) & 0xFFFFu) /
           65536.0 * 6.28318530718;
    float cosine = 1.0 - 2.0 * value / float(max(count, 1u));
    float sine = sqrt(max(0.0, 1.0 - cosine * cosine));
    return float3(sine * cos(phi), cosine, sine * sin(phi));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint border = uint(ps.atlasParams.x);
    uint innerResolution = uint(ps.atlasParams.y);
    uint probesPerRow = uint(ps.atlasParams.z);
    uint totalProbes = uint(ps.atlasParams.w);
    uint tileResolution = innerResolution + border * 2u;
    uint activeProbe = dispatchThreadID.x / max(tileResolution, 1u);
    if (tileResolution == 0 || activeProbe >= rt.probeUpdateCount ||
        dispatchThreadID.y >= tileResolution) {
        return;
    }
    uint stride = max(rt.probeUpdateStride, 1u);
    uint probeIndex = (rt.probeUpdateOffset % stride) + activeProbe * stride;
    if (probeIndex >= totalProbes) {
        return;
    }
    uint localX = dispatchThreadID.x - activeProbe * tileResolution;
    uint localY = dispatchThreadID.y;
    uint2 pixel = uint2((probeIndex % probesPerRow) * tileResolution + localX,
                        (probeIndex / probesPerRow) * tileResolution + localY);
    uint width, height;
    outTexture.GetDimensions(width, height);
    if (pixel.x >= width || pixel.y >= height) {
        return;
    }
    int2 inner = clamp(int2(localX, localY) - int(border), int2(0),
                       int2(int(innerResolution) - 1));
    float2 encoded = (float2(inner) + 0.5) /
                     float(max(innerResolution, 1u)) * 2.0 - 1.0;
    float3 direction = octDecode(encoded);
    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    float3 radiance = 0.0;
    float weightSum = 0.0;
    float distanceSum = 0.0;
    float distanceSquaredSum = 0.0;
    for (uint rayIndex = 0; rayIndex < raysPerProbe; ++rayIndex) {
        float3 rayDirection = sphericalFibonacci(rayIndex, raysPerProbe,
                                                 rt.frameIndex);
        float weight = max(dot(direction, rayDirection), 0.0);
        float4 sample = probeRadiance[probeIndex * raysPerProbe + rayIndex];
        float distance = sample.w > 0.0
                             ? min(sample.w, rt.maxRayDistance)
                             : rt.maxRayDistance;
        radiance += sample.xyz * weight;
        distanceSum += distance * weight;
        distanceSquaredSum += distance * distance * weight;
        weightSum += weight;
    }
    float3 irradiance = weightSum > 1e-6
                            ? radiance * (12.5663706144 / float(raysPerProbe))
                            : 0.0;
    float2 distanceMoments = weightSum > 1e-6
                                 ? float2(distanceSum, distanceSquaredSum) /
                                       weightSum
                                 : float2(rt.maxRayDistance,
                                          rt.maxRayDistance * rt.maxRayDistance);
    float history = rt.frameIndex < stride ? 0.0 : saturate(rt.hysteresis);
    outTexture[pixel] = float4(lerp(irradiance,
                                         prevTexture.Load(int3(pixel, 0)).xyz,
                                         history),
                               1.0);
    outDistance[pixel] = float4(lerp(distanceMoments,
                                          prevDistance.Load(int3(pixel, 0)).xy,
                                          history),
                                0.0, 1.0);
}
