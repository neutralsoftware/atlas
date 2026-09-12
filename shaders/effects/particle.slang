#if defined(ATLAS_VERTEX)
struct UniformBufferObject
{
    row_major float4x4 view;
    row_major float4x4 projection;
    row_major float4x4 model;
    uint isAmbient;
};

struct VertexOutput
{
    [[vk::location(0)]] float2 fragTexCoord : TEXCOORD0;
    [[vk::location(1)]] float4 fragColor : TEXCOORD1;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 quadVertex : TEXCOORD0;
    [[vk::location(1)]] float2 texCoord : TEXCOORD1;
    [[vk::location(2)]] float3 particlePos : TEXCOORD2;
    [[vk::location(3)]] float4 particleColor : TEXCOORD3;
    [[vk::location(4)]] float particleSize : TEXCOORD4;
};

[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    if (uniforms.isAmbient != 0u)
    {
        float4 viewParticlePos = mul(float4(input.particlePos, 1.0), uniforms.view);
        float3 viewPosition = viewParticlePos.xyz + float3(input.quadVertex.x * input.particleSize, input.quadVertex.y * input.particleSize, 0.0);
        output.gl_Position = mul(float4(viewPosition, 1.0), (mul(uniforms.model, uniforms.projection)));
    }
    else
    {
        float3 cameraRight = float3(uniforms.view[0].x, uniforms.view[1].x, uniforms.view[2].x);
        float3 cameraUp = float3(uniforms.view[0].y, uniforms.view[1].y, uniforms.view[2].y);
        float3 worldPosition = (input.particlePos + ((cameraRight * input.quadVertex.x) * input.particleSize)) + ((cameraUp * input.quadVertex.y) * input.particleSize);
        output.gl_Position = mul(float4(worldPosition, 1.0), (mul(uniforms.view, uniforms.projection)));
    }
    output.fragTexCoord = input.texCoord;
    output.fragColor = input.particleColor;
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct Params
{
    uint useTexture;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 fragTexCoord : TEXCOORD0;
    [[vk::location(1)]] float4 fragColor : TEXCOORD1;
};

[[vk::binding(0, 1)]] ConstantBuffer<Params> params;
[[vk::binding(0, 2)]] Sampler2D particleTexture;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    if (params.useTexture != 0u)
    {
        float4 texColor = particleTexture.Sample(input.fragTexCoord);
        if (texColor.w < 0.00999999978)
        {
            discard;
        }
        output.FragColor = texColor * input.fragColor;
    }
    else
    {
        float2 center = float2(0.5);
        float dist = distance(input.fragTexCoord, center);
        float alpha = 1.0 - smoothstep(0.300000012, 0.5, dist);
        output.FragColor = float4(input.fragColor.xyz, input.fragColor.w * alpha);
        if (output.FragColor.w < 0.00999999978)
        {
            discard;
        }
    }
    return output;
}
#endif
