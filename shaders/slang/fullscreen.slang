
struct VertexIn {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
}

struct VertexOut {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
}

[shader("vertex")]
VertexOut vertexMain(VertexIn input) {
    VertexOut output;
    output.position = float4(input.position.xy, 0.0, 1.0);
    output.texCoord = input.texCoord;
    return output;
}

Texture2D<float4> inputTexture : register(t0);
Texture2D<float4> brightTexture : reigster(t1);
Texture2D<float4> depthTexture : register(t2);
Texture2D<float4> volumetricLightTexture : register(t3);
Texture2D<float4> LUTTexture : register(t4);
Texture2D<float4> SSRTexture : register(t5);

SamplerState samplerState : register(s0);

struct Flags {
    int hasBrightTexture;
    int hasDepthTexture;
    int hasVolumetricLightTexture;
    int hasLUTTexture;
    int hasSSRTexture;
}

ConstantBuffer<Flags> flags : register(b0);

