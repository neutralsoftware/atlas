#include <metal_stdlib>
using namespace metal;

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

static inline uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

static inline float3 octDecode(float2 e) {
    float3 n = float3(e.x, e.y, 1.0f - fabs(e.x) - fabs(e.y));
    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(n.x >= 0.0f ? 1.0f : -1.0f, n.y >= 0.0f ? 1.0f : -1.0f);
        float2 folded = (1.0f - fabs(n.yx)) * signNotZero;
        n.x = folded.x;
        n.y = folded.y;
    }
    return normalize(n);
}

static inline float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    const float GOLDEN_RATIO = 1.6180339887498949f;
    const float TAU = 6.28318530718f;
    float i = float(index) + 0.5f;
    float phi = TAU * fract(i / GOLDEN_RATIO);
    float cosTheta = 1.0f - (2.0f * i) / float(count);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    uint rotSeed = wangHash(frameIndex * 1471u + 5743u);
    float rotAngle = float(rotSeed & 0xFFFFu) / 65536.0f * TAU;
    phi += rotAngle;
    return float3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));
}

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]],
                  texture2d<float, access::read> prevTexture [[texture(1)]],
                  texture2d<float, access::write> outDistance [[texture(2)]],
                  texture2d<float, access::read> prevDistance [[texture(3)]],
                  device float4 *probeRadiance [[buffer(0)]],
                  constant ProbeSpace &ps [[buffer(1)]],
                  constant RaytracingSettings &rt [[buffer(2)]],
                  uint2 gid [[thread_position_in_grid]]) {
    const float FOUR_PI = 12.566370614359172f;

    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    uint totalProbes = (uint)ps.atlasParams.w;

    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height())
        return;

    uint tileRes = innerRes + 2u * border;
    if (tileRes == 0u || probesPerRow == 0u || totalProbes == 0u ||
        innerRes == 0u) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint tileX = gid.x / tileRes;
    uint tileY = gid.y / tileRes;
    uint probeIndex = tileX + tileY * probesPerRow;

    if (probeIndex >= totalProbes) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint updateStride = max(rt.probeUpdateStride, 1u);
    uint updateOffset =
        (updateStride > 1u) ? (rt.probeUpdateOffset % updateStride) : 0u;
    bool probeIsActive =
        (updateStride <= 1u) ||
        ((probeIndex >= updateOffset) &&
         (((probeIndex - updateOffset) % updateStride) == 0u));
    if (!probeIsActive) {
        outTexture.write(prevTexture.read(gid), gid);
        outDistance.write(prevDistance.read(gid), gid);
        return;
    }

    uint localX = gid.x - tileX * tileRes;
    uint localY = gid.y - tileY * tileRes;

    int innerX = clamp(int(localX) - int(border), 0, int(innerRes) - 1);
    int innerY = clamp(int(localY) - int(border), 0, int(innerRes) - 1);

    float2 e = (float2(float(innerX), float(innerY)) + 0.5f) / float(innerRes) * 2.0f - 1.0f;
    float3 texelDir = octDecode(e);

    uint raysPerProbe = max(rt.raysPerProbe, 1u);
    uint baseOffset = probeIndex * raysPerProbe;
    uint rayStep = (raysPerProbe >= 128u) ? 2u : 1u;
    uint sampledRayCount = 0u;

    float3 sum = float3(0.0f);
    float weightSum = 0.0f;
    float distanceSum = 0.0f;
    float distanceSquaredSum = 0.0f;
    float nearHitCount = 0.0f;
    float spacingScale =
        max(max(ps.spacing.x, max(ps.spacing.y, ps.spacing.z)), 1e-4f);
    float nearHitThreshold =
        max(max(rt.normalBias * 2.0f, spacingScale * 0.1f), 0.002f);

    for (uint r = 0; r < raysPerProbe; r += rayStep) {
        sampledRayCount++;
        float4 raySample = probeRadiance[baseOffset + r];
        float hitDistance = raySample.w;
        if (hitDistance > 0.0f && hitDistance < nearHitThreshold) {
            nearHitCount += 1.0f;
        }

        float3 rayDir = sphericalFibonacci(r, raysPerProbe, rt.frameIndex);
        float w = max(0.0f, dot(texelDir, rayDir));
        if (w > 1e-6f) {
            float3 rad = raySample.xyz;
            if (all(isfinite(rad))) {
                sum += rad * w;
                weightSum += w;
                float distance = hitDistance > 0.0f
                                     ? min(hitDistance, rt.maxRayDistance)
                                     : rt.maxRayDistance;
                distanceSum += distance * w;
                distanceSquaredSum += distance * distance * w;
            }
        }
    }

    float3 irradiance = float3(0.0f);
    float2 distanceMoments = float2(rt.maxRayDistance,
                                    rt.maxRayDistance * rt.maxRayDistance);
    float invRayCount = 1.0f / float(max(sampledRayCount, 1u));
    if (weightSum > 1e-6f) {
        irradiance = sum * (FOUR_PI * invRayCount);
        distanceMoments = float2(distanceSum, distanceSquaredSum) / weightSum;
    }

    if (!all(isfinite(irradiance))) {
        irradiance = float3(0.0f);
    }

    float4 prev = prevTexture.read(gid);
    float4 previousDistance = prevDistance.read(gid);
    float3 prevValue = all(isfinite(prev.xyz)) ? prev.xyz : float3(0.0f);
    float prevValidity = isfinite(prev.w) ? clamp(prev.w, 0.0f, 1.0f) : 1.0f;

    float nearFraction = nearHitCount * invRayCount;
    float nearPenalty = smoothstep(0.82f, 0.995f, nearFraction);
    float probeValidity = 1.0f - nearPenalty;
    probeValidity = clamp(probeValidity, 0.0f, 1.0f);

    float h = clamp(rt.hysteresis, 0.0f, 0.995f);
    bool firstProbeUpdate = rt.frameIndex < updateStride;
    float3 blended = firstProbeUpdate ? irradiance : mix(irradiance, prevValue, h);
    float2 blendedDistance =
        firstProbeUpdate
            ? distanceMoments
            : mix(distanceMoments, previousDistance.xy, h);
    float blendedValidity =
        firstProbeUpdate
            ? probeValidity
            : mix(probeValidity, prevValidity, h);

    outTexture.write(float4(max(blended, float3(0.0f)), blendedValidity), gid);
    outDistance.write(float4(max(blendedDistance, float2(0.0f)),
                             blendedValidity, 0.0f), gid);
}
