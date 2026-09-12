struct ScreenVertexInput {
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(2)]] float2 texCoord : TEXCOORD2;
};

struct ScreenVertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 texCoord : TEXCOORD0;
};

[shader("vertex")]
ScreenVertexOutput vertexMain(ScreenVertexInput input) {
    return {float4(input.position.xy, 0.0, 1.0), input.texCoord};
}

[shader("vertex")]
ScreenVertexOutput screenVertexMain(ScreenVertexInput input) {
    return {float4(input.position, 1.0), input.texCoord};
}
