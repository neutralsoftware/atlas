struct DenoiseParameters {
    int stepWidth;
    float bloomThreshold;
};

[[vk::binding(0, 2)]] Texture2D<float4> inputTexture;
[[vk::binding(1, 2)]] RWTexture2D<float4> outputTexture;
[[vk::binding(2, 2)]] RWTexture2D<float4> brightTexture;
[[vk::binding(3, 2)]] Texture2D<float4> guideTexture;
[[vk::binding(4, 2)]] Texture2D<float4> albedoRoughnessTexture;
[[vk::binding(5, 2)]] Texture2D<float4> momentsTexture;
[[vk::binding(0, 0)]] ConstantBuffer<DenoiseParameters> parameters;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint2 gid = dispatchThreadID.xy;
    uint width, height;
    outputTexture.GetDimensions(width, height);
    if (gid.x >= width || gid.y >= height)
        return;

    const int2 offsets[9] = {int2(0, 0),  int2(1, 0),  int2(-1, 0),
                                 int2(0, 1),  int2(0, -1), int2(1, 1),
                                 int2(-1, 1), int2(1, -1), int2(-1, -1)};
    const float weights[9] = {0.28, 0.12, 0.12, 0.12, 0.12,
                                  0.06, 0.06, 0.06, 0.06};
    float3 center = inputTexture.Load(int3(gid, 0)).xyz;
    float4 centerGuide = guideTexture.Load(int3(gid, 0));
    float4 centerAlbedoRoughness = albedoRoughnessTexture.Load(int3(gid, 0));
    float4 centerMoments = momentsTexture.Load(int3(gid, 0));
    bool centerSurface = centerGuide.w > 0.0;
    float centerNormalLength = dot(centerGuide.xyz, centerGuide.xyz);
    float centerLuminance = dot(center, float3(0.2126, 0.7152, 0.0722));
    float neighborLuminance = 0.0;
    float neighborWeight = 0.0;
    for (int i = 1; i < 9; ++i) {
        int2 samplePosition =
            clamp(int2(gid) + offsets[i] * parameters.stepWidth, int2(0),
                  int2(width - 1, height - 1));
        float4 sampleGuide = guideTexture.Load(int3(uint2(samplePosition), 0));
        bool sampleSurface = sampleGuide.w > 0.0;
        if (sampleSurface != centerSurface)
            continue;
        float sampleLuminance = dot(
            inputTexture.Load(int3(uint2(samplePosition), 0)).xyz,
            float3(0.2126, 0.7152, 0.0722));
        neighborLuminance += sampleLuminance;
        neighborWeight += 1.0;
    }
    if (neighborWeight > 1.0) {
        float localLimit = max(3.0, neighborLuminance / neighborWeight * 3.0);
        if (centerLuminance > localLimit) {
            center *= localLimit / max(centerLuminance, 0.00001);
            centerLuminance = localLimit;
        }
    }
    float3 filtered = float3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 9; ++i) {
        int2 samplePosition =
            clamp(int2(gid) + offsets[i] * parameters.stepWidth, int2(0),
                  int2(width - 1, height - 1));
        float3 sampleColor = inputTexture.Load(int3(uint2(samplePosition), 0)).xyz;
        float4 sampleGuide = guideTexture.Load(int3(uint2(samplePosition), 0));
        float4 sampleAlbedoRoughness =
            albedoRoughnessTexture.Load(int3(uint2(samplePosition), 0));
        float sampleLuminance =
            dot(sampleColor, float3(0.2126, 0.7152, 0.0722));
        float roughness = clamp(centerAlbedoRoughness.w, 0.0, 1.0);
        float luminanceScale = lerp(2.5, 7.0, roughness);
        float luminanceDifference =
            abs(sampleLuminance - centerLuminance) /
            max(1.0, max(sampleLuminance, centerLuminance));
        float edgeWeight = exp(-luminanceDifference * luminanceScale);
        bool sampleSurface = sampleGuide.w > 0.0;
        float normalWeight = centerSurface == sampleSurface ? 1.0 : 0.0;
        float depthWeight = normalWeight;
        float albedoWeight = normalWeight;
        if (centerSurface && sampleSurface) {
            float sampleNormalLength = dot(sampleGuide.xyz, sampleGuide.xyz);
            if (centerNormalLength > 1e-6 && sampleNormalLength > 1e-6) {
                float normalSimilarity =
                    dot(centerGuide.xyz * rsqrt(centerNormalLength),
                        sampleGuide.xyz * rsqrt(sampleNormalLength));
                normalWeight *= pow(max(normalSimilarity, 0.0), 24.0);
            } else if (i != 0) {
                normalWeight = 0.0;
            }
            float depthScale = max(0.01, abs(centerGuide.w) * 0.01) *
                               max(float(parameters.stepWidth), 1.0);
            depthWeight *=
                exp(-abs(sampleGuide.w - centerGuide.w) / depthScale);
            albedoWeight *= exp(-length(sampleAlbedoRoughness.xyz -
                                        centerAlbedoRoughness.xyz) *
                                8.0);
        }
        float weight = weights[i] * edgeWeight * normalWeight * depthWeight *
                       albedoWeight;
        filtered += sampleColor * weight;
        totalWeight += weight;
    }
    float3 spatialResult =
        totalWeight > 0.0001 ? filtered / totalWeight : center;
    float roughness = clamp(centerAlbedoRoughness.w, 0.0, 1.0);
    float relativeNoise = sqrt(max(centerMoments.z, 0.0)) /
                          max(centerLuminance, 0.05);
    float filterStrength = clamp(relativeNoise * 1.5, 0.02, 1.0) *
                           lerp(0.12, 1.0, roughness * roughness);
    if (!centerSurface) {
        filterStrength *= 0.25;
    }
    float3 result = lerp(center, spatialResult, filterStrength);
    float brightness = dot(result, float3(0.2126, 0.7152, 0.0722));
    const float bloomKnee = 0.35;
    float soft = clamp(brightness - parameters.bloomThreshold + bloomKnee, 0.0,
                       bloomKnee * 2.0);
    soft = soft * soft / max(bloomKnee * 4.0, 0.00001);
    float contribution = max(brightness - parameters.bloomThreshold, soft) /
                         max(brightness, 0.00001);
    outputTexture[gid] = float4(result, 1.0);
    brightTexture[gid] = float4(result * contribution, 1.0);
}
