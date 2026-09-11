#if defined(ATLAS_VERTEX)
#include "utils/fullscreen.slang"
#endif

#if defined(ATLAS_FRAGMENT)
#include "utils/image.slang"

float mod(float x, float y) { return x - y * floor(x / y); }

struct ColorCorrection
{
    float exposure;
    float contrast;
    float saturation;
    float gamma;
    float temperature;
    float tint;
};

struct PushConstants
{
    int hasBrightTexture;
    int hasDepthTexture;
    int hasVolumetricLightTexture;
    int hasPositionTexture;
    int hasLUTTexture;
    int hasSSRTexture;
    int TextureType;
    float lutSize;
    int EffectCount;
};

struct Uniforms
{
    row_major float4x4 invProjectionMatrix;
    row_major float4x4 projectionMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 invViewMatrix;
    row_major float4x4 lastViewMatrix;
    float nearPlane;
    float farPlane;
    float focusDepth;
    float focusRange;
    int maxMipLevel;
    float deltaTime;
    float time;
};

struct Environment
{
    float3 fogColor;
    float fogIntensity;
};

static const float2 _165[9] = { float2(-0.00333333341, 0.00333333341), float2(0.0, 0.00333333341), float2(0.00333333341), float2(-0.00333333341, 0.0), float2(0.0), float2(0.00333333341, 0.0), float2(-0.00333333341), float2(0.0, -0.00333333341), float2(0.00333333341, -0.00333333341) };
static const float _171[9] = { -1.0, -1.0, -1.0, -1.0, 9.0, -1.0, -1.0, -1.0, -1.0 };
static const float _311[9] = { 1.0, 1.0, 1.0, 1.0, -8.0, 1.0, 1.0, 1.0, 1.0 };

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> parameters;
[[vk::binding(0, 3)]] StructuredBuffer<int> EffectBuffer;
[[vk::binding(1, 3)]] StructuredBuffer<float> EffectFloat1Buffer;
[[vk::binding(2, 3)]] StructuredBuffer<float> EffectFloat2Buffer;
[[vk::binding(3, 3)]] StructuredBuffer<float> EffectFloat3Buffer;
[[vk::binding(4, 3)]] StructuredBuffer<float> EffectFloat4Buffer;
[[vk::binding(5, 3)]] StructuredBuffer<float> EffectFloat5Buffer;
[[vk::binding(1, 1)]] ConstantBuffer<Uniforms> uniforms;
[[vk::binding(6, 3)]] StructuredBuffer<float> EffectFloat6Buffer;
[[vk::binding(0, 1)]] ConstantBuffer<Environment> environment;
[[vk::binding(0, 2)]] Sampler2D Texture;
[[vk::binding(1, 2)]] Sampler2D BrightTexture;
[[vk::binding(3, 2)]] Sampler2D VolumetricLightTexture;
[[vk::binding(6, 2)]] Sampler2D SSRTexture;
[[vk::binding(4, 2)]] Sampler2D PositionTexture;
[[vk::binding(5, 2)]] Sampler2D LUTTexture;
[[vk::binding(2, 2)]] Sampler2D DepthTexture;

#include "utils/post_processing.slang"

[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input, float4 gl_FragCoord : SV_Position)
{
    FragmentOutput output = {};
    float2 param = input.TexCoord;
    float4 color = sampleColor(param);
    float depth = 1.0;
    if (parameters.hasDepthTexture == 1)
    {
        depth = DepthTexture.Sample(input.TexCoord).x;
    }
    float2 param_1 = input.TexCoord;
    float param_2 = depth;
    float3 viewPos = reconstructViewPos(param_1, param_2);
    float _distance = parameters.hasDepthTexture == 1 ? length(viewPos) : uniforms.farPlane;
    bool useMotionBlur = false;
    float motionBlurSize = 0.0;
    float motionBlurSeparation = 0.0;
    for (int i = 0; i < parameters.EffectCount; i++)
    {
        if (EffectBuffer[i] == 6)
        {
            useMotionBlur = true;
            motionBlurSize = EffectFloat1Buffer[i];
            motionBlurSeparation = EffectFloat2Buffer[i];
        }
    }
    for (int i_1 = 0; i_1 < parameters.EffectCount; i_1++)
    {
        if (EffectBuffer[i_1] == 2)
        {
            color = sharpen(Texture, input.TexCoord);
        }
        else
        {
            if (EffectBuffer[i_1] == 3)
            {
                float radius = EffectFloat1Buffer[i_1];
                float param_3 = radius;
                color = blur(Texture, param_3, input.TexCoord);
            }
            else
            {
                if (EffectBuffer[i_1] == 4)
                {
                    color = edgeDetection(Texture, input.TexCoord);
                }
            }
        }
    }
    float2 param_4 = input.TexCoord;
    color = applyFXAA(Texture, param_4);
    float4 param_5 = color;
    float4 _2637 = applyColorEffects(param_5, gl_FragCoord);
    color = _2637;
    if (parameters.hasDepthTexture == 1)
    {
        float depthValue = DepthTexture.Sample(input.TexCoord).x;
        float param_6 = depthValue;
        float linearDepth = LinearizeDepth(param_6);
        float coc = clamp(abs(linearDepth - uniforms.focusDepth) / uniforms.focusRange, 0.0, 1.0);
        float mip = (coc * float(uniforms.maxMipLevel)) * 1.20000005;
        float4 param_7 = Texture.SampleLevel(input.TexCoord, mip);
        float4 _2676 = applyColorEffects(param_7, gl_FragCoord);
        float3 blurred = _2676.xyz;
        float3 sharp = color.xyz;
        float4 param_8 = color;
        color = param_8;
    }
    else
    {
        float4 param_9 = color;
        color = param_9;
    }
    float4 hdrColor;
    if (useMotionBlur)
    {
        float2 param_10 = input.TexCoord;
        float param_11 = motionBlurSize;
        float param_12 = motionBlurSeparation;
        float4 param_13 = color;
        float4 motionBlurred = applyMotionBlur(param_10, param_11, param_12, param_13);
        output.FragColor = motionBlurred;
        return output;
    }
    else
    {
        float2 param_14 = input.TexCoord;
        float4 param_15 = color;
        hdrColor = composeLighting(param_14, param_15);
    }
    float4 param_16 = hdrColor;
    hdrColor = mapToLUT(param_16);
    float3 param_17 = hdrColor.xyz;
    float3 _2744 = acesToneMapping(param_17);
    hdrColor.x = _2744.x;
    hdrColor.y = _2744.y;
    hdrColor.z = _2744.z;
    float fogFactor = 1.0 - exp((-_distance) * environment.fogIntensity);
    float3 finalColor = lerp(hdrColor.xyz, float3(environment.fogColor), float3(fogFactor));
    output.FragColor = float4(finalColor, 1.0);
    return output;
}
#endif
