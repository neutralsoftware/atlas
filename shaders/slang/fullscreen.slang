
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
