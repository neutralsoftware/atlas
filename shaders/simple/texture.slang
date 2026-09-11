#if defined(ATLAS_VERTEX)
struct UBO
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
};

struct VertexOutput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(1)]] float4 outColor : TEXCOORD1;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
    [[vk::location(1)]] float4 aColor : TEXCOORD1;
    [[vk::location(2)]] float2 aTexCoord : TEXCOORD2;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    row_major float4x4 mvp = mul(uniforms.model, (mul(uniforms.view, uniforms.projection)));
    output.gl_Position = mul(float4(input.aPos, 1.0), mvp);
    output.TexCoord = input.aTexCoord;
    output.outColor = input.aColor;
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct UBO
{
    uint useTexture;
    uint onlyTexture;
    int textureCount;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(1)]] float4 outColor : TEXCOORD1;
};

float4 calculateAllTextures(UBO  uniforms, Sampler2D texture1, inout float2  TexCoord, Sampler2D texture2, Sampler2D texture3, Sampler2D texture4, Sampler2D texture5, Sampler2D texture6, Sampler2D texture7, Sampler2D texture8, Sampler2D texture9, Sampler2D texture10, Sampler2D texture11, Sampler2D texture12, Sampler2D texture13, Sampler2D texture14, Sampler2D texture15, Sampler2D texture16)
{
    float4 color = float4(0.0);
    for (int i = 0; i < uniforms.textureCount; i++)
    {
        if (i == 0)
        {
            color += texture1.Sample(TexCoord);
        }
        else
        {
            if (i == 1)
            {
                color += texture2.Sample(TexCoord);
            }
            else
            {
                if (i == 2)
                {
                    color += texture3.Sample(TexCoord);
                }
                else
                {
                    if (i == 3)
                    {
                        color += texture4.Sample(TexCoord);
                    }
                    else
                    {
                        if (i == 4)
                        {
                            color += texture5.Sample(TexCoord);
                        }
                        else
                        {
                            if (i == 5)
                            {
                                color += texture6.Sample(TexCoord);
                            }
                            else
                            {
                                if (i == 6)
                                {
                                    color += texture7.Sample(TexCoord);
                                }
                                else
                                {
                                    if (i == 7)
                                    {
                                        color += texture8.Sample(TexCoord);
                                    }
                                    else
                                    {
                                        if (i == 8)
                                        {
                                            color += texture9.Sample(TexCoord);
                                        }
                                        else
                                        {
                                            if (i == 9)
                                            {
                                                color += texture10.Sample(TexCoord);
                                            }
                                            else
                                            {
                                                if (i == 10)
                                                {
                                                    color += texture11.Sample(TexCoord);
                                                }
                                                else
                                                {
                                                    if (i == 11)
                                                    {
                                                        color += texture12.Sample(TexCoord);
                                                    }
                                                    else
                                                    {
                                                        if (i == 12)
                                                        {
                                                            color += texture13.Sample(TexCoord);
                                                        }
                                                        else
                                                        {
                                                            if (i == 13)
                                                            {
                                                                color += texture14.Sample(TexCoord);
                                                            }
                                                            else
                                                            {
                                                                if (i == 14)
                                                                {
                                                                    color += texture15.Sample(TexCoord);
                                                                }
                                                                else
                                                                {
                                                                    if (i == 15)
                                                                    {
                                                                        color += texture16.Sample(TexCoord);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    color /= float4(float(uniforms.textureCount));
    return color;
}

[[vk::binding(0, 1)]] ConstantBuffer<UBO> uniforms;
[[vk::binding(0, 2)]] Sampler2D texture1;
[[vk::binding(1, 2)]] Sampler2D texture2;
[[vk::binding(2, 2)]] Sampler2D texture3;
[[vk::binding(3, 2)]] Sampler2D texture4;
[[vk::binding(4, 2)]] Sampler2D texture5;
[[vk::binding(5, 2)]] Sampler2D texture6;
[[vk::binding(6, 2)]] Sampler2D texture7;
[[vk::binding(7, 2)]] Sampler2D texture8;
[[vk::binding(8, 2)]] Sampler2D texture9;
[[vk::binding(9, 2)]] Sampler2D texture10;
[[vk::binding(10, 2)]] Sampler2D texture11;
[[vk::binding(11, 2)]] Sampler2D texture12;
[[vk::binding(12, 2)]] Sampler2D texture13;
[[vk::binding(13, 2)]] Sampler2D texture14;
[[vk::binding(14, 2)]] Sampler2D texture15;
[[vk::binding(15, 2)]] Sampler2D texture16;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    if (uniforms.onlyTexture != 0u)
    {
        output.FragColor = calculateAllTextures(uniforms, texture1, input.TexCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10, texture11, texture12, texture13, texture14, texture15, texture16);
        return output;
    }
    if (uniforms.useTexture != 0u)
    {
        output.FragColor = calculateAllTextures(uniforms, texture1, input.TexCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10, texture11, texture12, texture13, texture14, texture15, texture16) * input.outColor;
    }
    else
    {
        output.FragColor = input.outColor;
    }
    return output;
}
#endif
