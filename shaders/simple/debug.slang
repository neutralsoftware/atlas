#if defined(ATLAS_VERTEX)
struct VertexOutput
{
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    output.gl_Position = float4(input.aPos, 1.0);
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

[shader("fragment")]
FragmentOutput fragmentMain()
{
    FragmentOutput output = {};
    output.FragColor = float4(1.0, 0.0, 1.0, 1.0);
    return output;
}
#endif
