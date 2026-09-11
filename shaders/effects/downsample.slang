#if defined(ATLAS_FRAGMENT)
#include "utils/image.slang"

struct Params
{
    float2 srcResolution;
};

struct FragmentOutput
{
    float4 downsample : SV_Target0;
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
    float2 srcTexelSize =
        float2(1.0) / float2(textureSize(srcTexture).x, textureSize(srcTexture).y);
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    float2 texCoord = input.TexCoord;
    float3 a = srcTexture.Sample(float2(texCoord.x - (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 b = srcTexture.Sample(float2(texCoord.x, texCoord.y + (2.0 * y))).xyz;
    float3 c = srcTexture.Sample(float2(texCoord.x + (2.0 * x), texCoord.y + (2.0 * y))).xyz;
    float3 d = srcTexture.Sample(float2(texCoord.x - (2.0 * x), texCoord.y)).xyz;
    float3 e = srcTexture.Sample(float2(texCoord.x, texCoord.y)).xyz;
    float3 f = srcTexture.Sample(float2(texCoord.x + (2.0 * x), texCoord.y)).xyz;
    float3 g = srcTexture.Sample(float2(texCoord.x - (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 h = srcTexture.Sample(float2(texCoord.x, texCoord.y - (2.0 * y))).xyz;
    float3 i = srcTexture.Sample(float2(texCoord.x + (2.0 * x), texCoord.y - (2.0 * y))).xyz;
    float3 j = srcTexture.Sample(float2(texCoord.x - x, texCoord.y + y)).xyz;
    float3 k = srcTexture.Sample(float2(texCoord.x + x, texCoord.y + y)).xyz;
    float3 l = srcTexture.Sample(float2(texCoord.x - x, texCoord.y - y)).xyz;
    float3 m = srcTexture.Sample(float2(texCoord.x + x, texCoord.y - y)).xyz;
    float3 downsampleColor = e * 0.125;
    downsampleColor += ((((a + c) + g) + i) * 0.03125);
    downsampleColor += ((((b + d) + f) + h) * 0.0625);
    downsampleColor += ((((j + k) + l) + m) * 0.125);
    output.downsample = float4(downsampleColor, 1.0);
    return output;
}
#endif
