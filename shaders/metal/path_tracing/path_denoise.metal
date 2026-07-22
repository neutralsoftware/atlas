#include <metal_stdlib>
using namespace metal;

struct DenoiseParameters {
    int stepWidth;
};

kernel void main0(texture2d<float, access::read> inputTexture [[texture(0)]],
                  texture2d<float, access::write> outputTexture [[texture(1)]],
                  texture2d<float, access::write> brightTexture [[texture(2)]],
                  texture2d<float, access::read> guideTexture [[texture(3)]],
                  constant DenoiseParameters &parameters [[buffer(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint width = outputTexture.get_width();
    uint height = outputTexture.get_height();
    if (gid.x >= width || gid.y >= height)
        return;

    constexpr int2 offsets[9] = {int2(0, 0),  int2(1, 0),  int2(-1, 0),
                                 int2(0, 1),  int2(0, -1), int2(1, 1),
                                 int2(-1, 1), int2(1, -1), int2(-1, -1)};
    constexpr float weights[9] = {0.28, 0.12, 0.12, 0.12, 0.12,
                                  0.06, 0.06, 0.06, 0.06};
    float3 center = inputTexture.read(gid).xyz;
    float4 centerGuide = guideTexture.read(gid);
    bool centerSurface = centerGuide.w > 0.0;
    float centerNormalLength = dot(centerGuide.xyz, centerGuide.xyz);
    float centerLuminance = dot(center, float3(0.2126, 0.7152, 0.0722));
    float3 filtered = float3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 9; ++i) {
        int2 samplePosition =
            clamp(int2(gid) + offsets[i] * parameters.stepWidth, int2(0),
                  int2(width - 1, height - 1));
        float3 sampleColor = inputTexture.read(uint2(samplePosition)).xyz;
        float4 sampleGuide = guideTexture.read(uint2(samplePosition));
        float sampleLuminance =
            dot(sampleColor, float3(0.2126, 0.7152, 0.0722));
        float edgeWeight = exp(-abs(sampleLuminance - centerLuminance) * 6.0);
        bool sampleSurface = sampleGuide.w > 0.0;
        float normalWeight = centerSurface == sampleSurface ? 1.0 : 0.0;
        float depthWeight = normalWeight;
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
            depthWeight *= exp(-abs(sampleGuide.w - centerGuide.w) /
                               max(0.05, abs(centerGuide.w) * 0.02));
        }
        float weight = weights[i] * edgeWeight * normalWeight * depthWeight;
        filtered += sampleColor * weight;
        totalWeight += weight;
    }
    float3 result = totalWeight > 0.0001 ? filtered / totalWeight : center;
    float brightness = dot(result, float3(0.2126, 0.7152, 0.0722));
    float contribution = smoothstep(0.5, 1.5, brightness);
    outputTexture.write(float4(result, 1.0), gid);
    brightTexture.write(float4(result * contribution, 1.0), gid);
}
