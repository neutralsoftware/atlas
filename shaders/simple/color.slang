#if defined(ATLAS_VERTEX)
struct UBO
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    uint isInstanced;
};

struct VertexOutput
{
    [[vk::location(0)]] float4 vertexColor : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
    [[vk::location(1)]] float4 aColor : TEXCOORD1;
    [[vk::location(6)]] float4 instanceModel_0 : TEXCOORD6;
    [[vk::location(7)]] float4 instanceModel_1 : TEXCOORD7;
    [[vk::location(8)]] float4 instanceModel_2 : TEXCOORD8;
    [[vk::location(9)]] float4 instanceModel_3 : TEXCOORD9;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    row_major float4x4 instanceModel = {};
    instanceModel[0] = input.instanceModel_0;
    instanceModel[1] = input.instanceModel_1;
    instanceModel[2] = input.instanceModel_2;
    instanceModel[3] = input.instanceModel_3;
    row_major float4x4 mvp;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        mvp = mul(instanceModel, (mul(uniforms.view, uniforms.projection)));
    }
    else
    {
        mvp = mul(uniforms.model, (mul(uniforms.view, uniforms.projection)));
    }
    output.gl_Position = mul(float4(input.aPos, 1.0), mvp);
    output.vertexColor = input.aColor;
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct FragmentOutput
{
    float4 FragColor : SV_Target0;
    float4 BrightColor : SV_Target1;
};

struct FragmentInput
{
    [[vk::location(0)]] float4 vertexColor : TEXCOORD0;
};

[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float3 color = input.vertexColor.xyz / (input.vertexColor.xyz + float3(1.0));
    output.FragColor = float4(color, input.vertexColor.w);
    if (length(color) > 1.0)
    {
        output.BrightColor = float4(color, input.vertexColor.w);
    }
    return output;
}
#endif
