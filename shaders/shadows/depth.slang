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
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    row_major float4x4 instanceModel = {};
    instanceModel[0] = input.instanceModel_0;
    instanceModel[1] = input.instanceModel_1;
    instanceModel[2] = input.instanceModel_2;
    instanceModel[3] = input.instanceModel_3;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        row_major float4x4 _36 = mul(uniforms.view, uniforms.projection);
        row_major float4x4 _40 = mul(instanceModel, _36);
        float4 _49 = float4(input.aPos, 1.0);
        float4 _50 = mul(_49, _40);
        output.gl_Position = _50;
    }
    else
    {
        row_major float4x4 _58 = mul(uniforms.view, uniforms.projection);
        row_major float4x4 _61 = mul(uniforms.model, _58);
        float4 _66 = float4(input.aPos, 1.0);
        float4 _67 = mul(_66, _61);
        output.gl_Position = _67;
    }
    return output;
}
#endif
