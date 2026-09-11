#if defined(ATLAS_VERTEX)
struct UBO
{
    row_major float4x4 model;
    uint isInstanced;
};

struct ShadowUniforms
{
    row_major float4x4 shadowMatrix;
    int faceIndex;
};

struct VertexOutput
{
    [[vk::location(0)]] float4 FragPos : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
    [[vk::location(6)]] float4 instanceModel_0 : TEXCOORD6;
    [[vk::location(7)]] float4 instanceModel_1 : TEXCOORD7;
    [[vk::location(8)]] float4 instanceModel_2 : TEXCOORD8;
    [[vk::location(9)]] float4 instanceModel_3 : TEXCOORD9;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> uniforms;
[[vk::binding(0, 2)]] ConstantBuffer<ShadowUniforms> shadow;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    row_major float4x4 instanceModel = {};
    instanceModel[0] = input.instanceModel_0;
    instanceModel[1] = input.instanceModel_1;
    instanceModel[2] = input.instanceModel_2;
    instanceModel[3] = input.instanceModel_3;
    float4 worldPos;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        worldPos = mul(float4(input.aPos, 1.0), (mul(instanceModel, uniforms.model)));
    }
    else
    {
        worldPos = mul(float4(input.aPos, 1.0), uniforms.model);
    }
    output.FragPos = worldPos;
    output.gl_Position = mul(worldPos, shadow.shadowMatrix);
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct Uniforms
{
    float3 lightPos;
    float far_plane;
};

struct FragmentOutput
{
    float gl_FragDepth : SV_Depth;
};

struct FragmentInput
{
    [[vk::location(0)]] float4 FragPos : TEXCOORD0;
};

[[vk::binding(0, 1)]] ConstantBuffer<Uniforms> uniforms;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float lightDistance = length(input.FragPos.xyz - float3(uniforms.lightPos));
    lightDistance /= uniforms.far_plane;
    output.gl_FragDepth = lightDistance;
    return output;
}
#endif
