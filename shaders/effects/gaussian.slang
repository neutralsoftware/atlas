#if defined(ATLAS_FRAGMENT)
#include "utils/image.slang"

struct Params
{
    uint horizontal;
    float4 weight[5];
    float radius;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

[[vk::binding(0, 1)]] ConstantBuffer<Params> params;
[[vk::binding(0, 2)]] Sampler2D image;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float2 tex_offset = (float2(1.0) / float2(int2(textureSize(image).x, textureSize(image).y))) * params.radius;
    float3 result = image.Sample(input.TexCoord).xyz * params.weight[0].x;
    if (params.horizontal != 0u)
    {
        for (int i = 1; i < 5; i++)
        {
            result += (image.Sample((input.TexCoord + float2(tex_offset.x * float(i), 0.0))).xyz * params.weight[i].x);
            result += (image.Sample((input.TexCoord - float2(tex_offset.x * float(i), 0.0))).xyz * params.weight[i].x);
        }
    }
    else
    {
        for (int i_1 = 1; i_1 < 5; i_1++)
        {
            result += (image.Sample((input.TexCoord + float2(0.0, tex_offset.y * float(i_1)))).xyz * params.weight[i_1].x);
            result += (image.Sample((input.TexCoord - float2(0.0, tex_offset.y * float(i_1)))).xyz * params.weight[i_1].x);
        }
    }
    output.FragColor = float4(result, 1.0);
    return output;
}
#endif
