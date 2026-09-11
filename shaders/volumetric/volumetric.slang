#if defined(ATLAS_VERTEX)
#include "utils/fullscreen.slang"
#endif

#if defined(ATLAS_FRAGMENT)
struct Sun
{
    float2 sunPos;
};

struct VolumetricParameters
{
    float density;
    float weight;
    float decay;
    float exposure;
};

struct DirectionalLight
{
    float3 color;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoords : TEXCOORD0;
};

float3 computeVolumetricLighting(float2  uv, Sun  sun, VolumetricParameters  volumetricParameters, Sampler2D sceneTexture, DirectionalLight  directionalLight)
{
    float3 color = float3(0.0);
    float2 deltaTexCoord = (sun.sunPos - uv) * volumetricParameters.density;
    float2 coord = uv;
    float illuminationDecay = 1.0;
    for (int i = 0; i < 48; i++)
    {
        coord += deltaTexCoord;
        bool _59 = coord.x < 0.0;
        bool _66;
        if (!_59)
        {
            _66 = coord.x > 1.0;
        }
        else
        {
            _66 = _59;
        }
        bool _74;
        if (!_66)
        {
            _74 = coord.y < 0.0;
        }
        else
        {
            _74 = _66;
        }
        bool _81;
        if (!_74)
        {
            _81 = coord.y > 1.0;
        }
        else
        {
            _81 = _74;
        }
        if (_81)
        {
            break;
        }
        float3 sampled = sceneTexture.Sample(coord).xyz;
        float brightness = dot(sampled, float3(0.298999995, 0.587000012, 0.114));
        float3 atmosphere = directionalLight.color * 0.0199999996;
        if (brightness > 0.5)
        {
            atmosphere += (sampled * 0.5);
        }
        atmosphere *= (illuminationDecay * volumetricParameters.weight);
        color += atmosphere;
        illuminationDecay *= volumetricParameters.decay;
    }
    return color * 5.0;
}

[[vk::binding(0, 1)]] ConstantBuffer<Sun> sun;
[[vk::binding(2, 1)]] ConstantBuffer<VolumetricParameters> volumetricParameters;
[[vk::binding(1, 1)]] ConstantBuffer<DirectionalLight> directionalLight;
[[vk::binding(0, 2)]] Sampler2D sceneTexture;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float2 param = input.TexCoords;
    float3 rays = computeVolumetricLighting(param, sun, volumetricParameters, sceneTexture, directionalLight);
    output.FragColor = float4(rays, 1.0);
    return output;
}
#endif
