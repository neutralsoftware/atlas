#if defined(ATLAS_FRAGMENT)
#include "utils/image.slang"

struct Params
{
    float2 srcResolution;
    float filterRadius;
};

struct FragmentOutput
{
    float4 upsample : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

[[vk::binding(0, 1)]] ConstantBuffer<Params> params;
[[vk::binding(0, 2)]] Sampler2D srcTexture;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float2 texelSize =
        float2(1.0) / float2(textureSize(srcTexture).x, textureSize(srcTexture).y);
    float radius = clamp(params.filterRadius, 1.0, 8.0);
    float x = radius * texelSize.x;
    float y = radius * texelSize.y;
    float2 texCoord = input.TexCoord;
    float2 step1 = float2(x, y);
    float2 step2 = step1 * 2.0;
    float3 upsampleColor = float3(0.0);
    float totalWeight = 0.0;

    float wCenter = 0.16;
    float wCardinal1 = 0.11;
    float wDiagonal1 = 0.075;
    float wCardinal2 = 0.04;
    float wDiagonal2 = 0.015;

    upsampleColor += srcTexture.Sample(texCoord).xyz * wCenter;
    totalWeight += wCenter;

    upsampleColor += srcTexture.Sample(texCoord + float2(step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step1.x, 0.0)).xyz * wCardinal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(0.0, step1.y)).xyz * wCardinal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(0.0, -step1.y)).xyz * wCardinal1;
    totalWeight += wCardinal1 * 4.0;

    upsampleColor += srcTexture.Sample(texCoord + float2(step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step1.x, step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(step1.x, -step1.y)).xyz * wDiagonal1;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step1.x, -step1.y)).xyz * wDiagonal1;
    totalWeight += wDiagonal1 * 4.0;

    upsampleColor += srcTexture.Sample(texCoord + float2(step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step2.x, 0.0)).xyz * wCardinal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(0.0, step2.y)).xyz * wCardinal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(0.0, -step2.y)).xyz * wCardinal2;
    totalWeight += wCardinal2 * 4.0;

    upsampleColor += srcTexture.Sample(texCoord + float2(step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step2.x, step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(step2.x, -step2.y)).xyz * wDiagonal2;
    upsampleColor += srcTexture.Sample(texCoord + float2(-step2.x, -step2.y)).xyz * wDiagonal2;
    totalWeight += wDiagonal2 * 4.0;

    upsampleColor /= float3(max(totalWeight, 1e-5));
    output.upsample = float4(upsampleColor, 1.0);
    return output;
}
#endif
