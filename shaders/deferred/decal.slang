#if defined(ATLAS_VERTEX)
#include "utils/matrix.slang"
#include "utils/skinning.slang"

struct DecalVertexUniforms {
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    uint isInstanced;
    uint hasSkin;
    float decalOffset;
};

struct VertexInput {
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(2)]] float2 uv : TEXCOORD2;
    [[vk::location(3)]] float3 normal : TEXCOORD3;
    [[vk::location(6)]] float4 instance0 : TEXCOORD6;
    [[vk::location(7)]] float4 instance1 : TEXCOORD7;
    [[vk::location(8)]] float4 instance2 : TEXCOORD8;
    [[vk::location(9)]] float4 instance3 : TEXCOORD9;
    [[vk::location(10)]] uint4 joints : TEXCOORD10;
    [[vk::location(11)]] float4 weights : TEXCOORD11;
};

struct VertexOutput {
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    float4 position : SV_Position;
};

[[vk::binding(0, 0)]] ConstantBuffer<DecalVertexUniforms> uniforms;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    row_major float4x4 model = uniforms.model;
    if (uniforms.isInstanced != 0 && abs(input.instance3.w) > 0.5)
        model = float4x4(input.instance0, input.instance1, input.instance2, input.instance3);
    row_major float4x4 skin = skinMatrix(input.joints, input.weights, uniforms.hasSkin);
    row_major float4x4 transform = mul(skin, model);
    float4 world = mul(float4(input.position, 1.0), transform);
    row_major float4x4 inverseTranspose = transpose(inverseMatrix(transform));
    float3 normal = mul(float4(input.normal, 0.0), inverseTranspose).xyz;
    float magnitude = dot(normal, normal);
    if (magnitude > 1e-10)
        world.xyz += normal * (uniforms.decalOffset * rsqrt(magnitude));
    VertexOutput output;
    output.uv = input.uv;
    output.position = mul(world, mul(uniforms.view, uniforms.projection));
    return output;
}
#elif defined(ATLAS_FRAGMENT)
struct DecalUniforms {
    float4 decalTint;
    float2 decalTextureScale;
    float2 decalTextureOffset;
    uint hasDecalColor;
    uint hasDecalOpacity;
};

struct FragmentInput {
    [[vk::location(1)]] float2 uv : TEXCOORD1;
};

[[vk::binding(0, 1)]] ConstantBuffer<DecalUniforms> decal;
[[vk::binding(0, 2)]] Sampler2D decalColor;
[[vk::binding(1, 2)]] Sampler2D decalOpacity;

[shader("fragment")]
float4 fragmentMain(FragmentInput input) : SV_Target0 {
    float2 uv = input.uv * decal.decalTextureScale + decal.decalTextureOffset;
    float4 color = decal.decalTint;
    if (decal.hasDecalColor != 0)
        color *= decalColor.Sample(uv);
    if (decal.hasDecalOpacity != 0)
        color.a *= decalOpacity.Sample(uv).r;
    return float4(color.rgb, saturate(color.a));
}
#endif
