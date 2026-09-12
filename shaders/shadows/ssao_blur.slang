#if defined(ATLAS_FRAGMENT)
#include "utils/image.slang"

struct FragmentOutput
{
    float FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

[[vk::binding(0, 2)]] Sampler2D inSSAO;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float2 texelSize = float2(1.0) / float2(int2(textureSize(inSSAO).x, textureSize(inSSAO).y));
    float result = 0.0;
    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += inSSAO.Sample((input.TexCoord + offset)).x;
        }
    }
    output.FragColor = result / 25.0;
    return output;
}
#endif
