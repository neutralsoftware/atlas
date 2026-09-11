#if defined(ATLAS_VERTEX)
struct Uniforms
{
    row_major float4x4 projection;
};

struct VertexOutput
{
    [[vk::location(0)]] float2 texCoords : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float4 vertex0 : TEXCOORD0;
};

[[vk::binding(0, 0)]] ConstantBuffer<Uniforms> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    output.gl_Position = mul(float4(input.vertex0.xy, 0.0, 1.0), uniforms.projection);
    output.texCoords = input.vertex0.zw;
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct TextColor
{
    float3 textColor;
};

struct FragmentOutput
{
    float4 color : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 texCoords : TEXCOORD0;
};

[[vk::binding(0, 1)]] ConstantBuffer<TextColor> textColor;
[[vk::binding(0, 2)]] Sampler2D text;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float4 sampled = float4(1.0, 1.0, 1.0, text.Sample(input.texCoords).x);
    output.color = float4(textColor.textColor, 1.0) * sampled;
    return output;
}
#endif
