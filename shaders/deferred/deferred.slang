#if defined(ATLAS_VERTEX)
#include "utils/matrix.slang"

struct UBO
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    uint isInstanced;
};

struct VertexOutput
{
    [[vk::location(0)]] float4 outColor : TEXCOORD0;
    [[vk::location(1)]] float2 TexCoord : TEXCOORD1;
    [[vk::location(2)]] float3 Normal : TEXCOORD2;
    [[vk::location(3)]] float3 FragPos : TEXCOORD3;
    [[vk::location(4)]] float3 TBN_0 : TEXCOORD4;
    [[vk::location(5)]] float3 TBN_1 : TEXCOORD5;
    [[vk::location(6)]] float3 TBN_2 : TEXCOORD6;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
    [[vk::location(1)]] float4 aColor : TEXCOORD1;
    [[vk::location(2)]] float2 aTexCoord : TEXCOORD2;
    [[vk::location(3)]] float3 aNormal : TEXCOORD3;
    [[vk::location(4)]] float3 aTangent : TEXCOORD4;
    [[vk::location(5)]] float3 aBitangent : TEXCOORD5;
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
    row_major float3x3 TBN = {};
    row_major float4x4 instanceModel = {};
    instanceModel[0] = input.instanceModel_0;
    instanceModel[1] = input.instanceModel_1;
    instanceModel[2] = input.instanceModel_2;
    instanceModel[3] = input.instanceModel_3;
    row_major float4x4 finalModel;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        finalModel = instanceModel;
    }
    else
    {
        finalModel = uniforms.model;
    }
    float4 worldPos = mul(float4(input.aPos, 1.0), finalModel);
    output.FragPos = worldPos.xyz;
    output.gl_Position = mul(worldPos, (mul(uniforms.view, uniforms.projection)));
    output.TexCoord = input.aTexCoord;
    output.outColor = input.aColor;
    row_major float4x4 _81 = transpose(inverseMatrix(finalModel));
    row_major float3x3 normalMatrix = float3x3(_81[0].xyz, _81[1].xyz, _81[2].xyz);
    output.Normal = normalize(mul(input.aNormal, normalMatrix));
    float3 T = normalize(mul(input.aTangent, normalMatrix));
    float3 B = normalize(mul(input.aBitangent, normalMatrix));
    float3 N = normalize(mul(input.aNormal, normalMatrix));
    TBN = float3x3(float3(T), float3(B), float3(N));
    output.TBN_0 = TBN[0];
    output.TBN_1 = TBN[1];
    output.TBN_2 = TBN[2];
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct UBO
{
    int4 textureTypes[16];
    int textureCount;
    uint useTexture;
    uint useColor;
    float3 cameraPosition;
    float normalMapStrength;
    uint useNormalMap;
    float2 textureScale;
    float2 textureOffset;
};

struct MaterialPush
{
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct FragmentOutput
{
    float4 gPosition : SV_Target0;
    float4 gNormal : SV_Target1;
    float4 gAlbedoSpec : SV_Target2;
    float4 gMaterial : SV_Target3;
};

struct FragmentInput
{
    [[vk::location(1)]] float2 TexCoord : TEXCOORD1;
    [[vk::location(2)]] float3 Normal : TEXCOORD2;
    [[vk::location(3)]] float3 FragPos : TEXCOORD3;
    [[vk::location(4)]] float3 TBN_0 : TEXCOORD4;
    [[vk::location(5)]] float3 TBN_1 : TEXCOORD5;
    [[vk::location(6)]] float3 TBN_2 : TEXCOORD6;
};

float4 sampleTextureAt(int  textureIndex, float2  uv, Sampler2D texture1, Sampler2D texture2, Sampler2D texture3, Sampler2D texture4, Sampler2D texture5, Sampler2D texture6, Sampler2D texture7, Sampler2D texture8, Sampler2D texture9, Sampler2D texture10)
{
    if (textureIndex == 0)
    {
        return texture1.Sample(uv);
    }
    else
    {
        if (textureIndex == 1)
        {
            return texture2.Sample(uv);
        }
        else
        {
            if (textureIndex == 2)
            {
                return texture3.Sample(uv);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return texture4.Sample(uv);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return texture5.Sample(uv);
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return texture6.Sample(uv);
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return texture7.Sample(uv);
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return texture8.Sample(uv);
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return texture9.Sample(uv);
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return texture10.Sample(uv);
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
    return float4(0.0);
}

float2 parallaxMapping(float2  texCoords, float3  viewDir, UBO  uniforms, Sampler2D texture1, Sampler2D texture2, Sampler2D texture3, Sampler2D texture4, Sampler2D texture5, Sampler2D texture6, Sampler2D texture7, Sampler2D texture8, Sampler2D texture9, Sampler2D texture10)
{
    float3 v = normalize(viewDir);
    float numLayers = lerp(32.0, 8.0, abs(dot(float3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float2 P = (v.xy / float2(max(v.z, 0.0500000007))) * 0.0399999991;
    float2 deltaTexCoords = P / float2(numLayers);
    float2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < uniforms.textureCount; i++)
    {
        if (uniforms.textureTypes[i].x == 6)
        {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == (-1))
    {
        return currentTexCoords;
    }
    int param = textureIndex;
    float2 param_1 = currentTexCoords;
    float currentDepthMapValue = 1.0 - sampleTextureAt(param, param_1, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10).x;
    int maxIterations = int(numLayers) + 1;
    int iteration = 0;
    while (currentLayerDepth < currentDepthMapValue && iteration < maxIterations)
    {
        currentTexCoords -= deltaTexCoords;
        int param_2 = textureIndex;
        float2 param_3 = currentTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(param_2, param_3, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10).x;
        currentLayerDepth += layerDepth;
        iteration++;
    }
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    int param_4 = textureIndex;
    float2 param_5 = prevTexCoords;
    float beforeDepth = 1.0 - sampleTextureAt(param_4, param_5, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10).x - (currentLayerDepth - layerDepth);
    float denom = afterDepth - beforeDepth;
    float weight = abs(denom) > 9.99999975e-05 ? clamp(afterDepth / denom, 0.0, 1.0) : 0.0;
    currentTexCoords = (prevTexCoords * weight) + (currentTexCoords * (1.0 - weight));
    return currentTexCoords;
}

float4 enableTextures(int  type, UBO  uniforms, Sampler2D texture1, inout float2  texCoord, Sampler2D texture2, Sampler2D texture3, Sampler2D texture4, Sampler2D texture5, Sampler2D texture6, Sampler2D texture7, Sampler2D texture8, Sampler2D texture9, Sampler2D texture10)
{
    float4 color = float4(0.0);
    int count = 0;
    for (int i = 0; i < uniforms.textureCount; i++)
    {
        if (uniforms.textureTypes[i].x == type)
        {
            if (i == 0)
            {
                color += texture1.Sample(texCoord);
            }
            else
            {
                if (i == 1)
                {
                    color += texture2.Sample(texCoord);
                }
                else
                {
                    if (i == 2)
                    {
                        color += texture3.Sample(texCoord);
                    }
                    else
                    {
                        if (i == 3)
                        {
                            color += texture4.Sample(texCoord);
                        }
                        else
                        {
                            if (i == 4)
                            {
                                color += texture5.Sample(texCoord);
                            }
                            else
                            {
                                if (i == 5)
                                {
                                    color += texture6.Sample(texCoord);
                                }
                                else
                                {
                                    if (i == 6)
                                    {
                                        color += texture7.Sample(texCoord);
                                    }
                                    else
                                    {
                                        if (i == 7)
                                        {
                                            color += texture8.Sample(texCoord);
                                        }
                                        else
                                        {
                                            if (i == 8)
                                            {
                                                color += texture9.Sample(texCoord);
                                            }
                                            else
                                            {
                                                if (i == 9)
                                                {
                                                    color += texture10.Sample(texCoord);
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
            count++;
        }
    }
    if (count > 0)
    {
        color /= float4(float(count));
    }
    if (count == 0)
    {
        return float4(-1.0);
    }
    return color;
}

[[vk::binding(0, 1)]] ConstantBuffer<UBO> uniforms;
[[vk::push_constant]] ConstantBuffer<MaterialPush> material;
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
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input, float4 gl_FragCoord : SV_Position)
{
    FragmentOutput output = {};
    row_major float3x3 TBN = {};
    TBN[0] = input.TBN_0;
    TBN[1] = input.TBN_1;
    TBN[2] = input.TBN_2;
    float2 texCoord = input.TexCoord * uniforms.textureScale + uniforms.textureOffset;
    bool hasParallaxMap = false;
    for (int i = 0; i < uniforms.textureCount; i++)
    {
        if (uniforms.textureTypes[i].x == 6)
        {
            hasParallaxMap = true;
            break;
        }
    }
    if (hasParallaxMap)
    {
        float3 tangentViewDir = normalize(mul((uniforms.cameraPosition - input.FragPos), transpose(TBN)));
        float2 param = texCoord;
        float3 param_1 = tangentViewDir;
        texCoord = parallaxMapping(param, param_1, uniforms, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    }
    int param_2 = 0;
    float4 sampledColor = enableTextures(param_2, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    bool hasColorTexture = any(sampledColor != float4(-1.0));
    float4 baseColor = float4(material.albedo[0], material.albedo[1], material.albedo[2], 1.0);
    int param_3 = 0;
    float4 albedoTex = enableTextures(param_3, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    if (any(albedoTex != float4(-1.0)))
    {
        baseColor *= albedoTex;
    }
    int param_3a = 12;
    float4 opacityTex = enableTextures(param_3a, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    if (any(opacityTex != float4(-1.0)))
    {
        if (opacityTex.x < 0.100000001)
        {
            discard;
        }
    }
    else
    {
        if ((baseColor.w < 0.100000001) && (material.albedo[3] < 0.999000013))
        {
            discard;
        }
    }
    int param_4 = 5;
    float4 normTexture = enableTextures(param_4, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    bool _527 = normTexture.x != (-1.0);
    bool _534;
    if (_527)
    {
        _534 = normTexture.y != (-1.0);
    }
    else
    {
        _534 = _527;
    }
    bool _540;
    if (_534)
    {
        _540 = normTexture.z != (-1.0);
    }
    else
    {
        _540 = _534;
    }
    float normalStrength = max(uniforms.normalMapStrength, 0.0f);
    bool useNormalMap = (uniforms.useNormalMap != 0u) && (normalStrength > 0.0f);
    float3 normal;
    if (_540 && useNormalMap)
    {
        float3 tangentNormal = (normTexture.xyz * 2.0) - float3(1.0);
        tangentNormal.xy *= normalStrength;
        tangentNormal = normalize(tangentNormal);
        normal = normalize(mul(tangentNormal, TBN));
    }
    else
    {
        normal = normalize(input.Normal);
    }
    float3 albedoColor = baseColor.xyz;
    int param_pbr_pack = 14;
    float4 pbrPackTex = enableTextures(param_pbr_pack, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    bool hasPbrPack = any(pbrPackTex != float4(-1.0));
    float metallicValue = material.metallic;
    int param_5 = 9;
    float4 metallicTex = enableTextures(param_5, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    if (hasPbrPack)
    {
        metallicValue *= pbrPackTex.z;
    }
    else if (any(metallicTex != float4(-1.0)))
    {
        metallicValue *= metallicTex.x;
    }
    float roughnessValue = material.roughness;
    int param_6 = 10;
    float4 roughnessTex = enableTextures(param_6, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    if (hasPbrPack)
    {
        roughnessValue *= pbrPackTex.y;
    }
    else if (any(roughnessTex != float4(-1.0)))
    {
        roughnessValue *= roughnessTex.x;
    }
    float aoValue = material.ao;
    int param_7 = 11;
    float4 aoTex = enableTextures(param_7, uniforms, texture1, texCoord, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10);
    if (hasPbrPack)
    {
        aoValue *= pbrPackTex.x;
    }
    else if (any(aoTex != float4(-1.0)))
    {
        aoValue *= aoTex.x;
    }
    metallicValue = clamp(metallicValue, 0.0, 1.0);
    roughnessValue = clamp(roughnessValue, 0.0, 1.0);
    aoValue = clamp(aoValue, 0.0, 1.0);
    float nonlinearDepth = gl_FragCoord.z;
    output.gPosition = float4(input.FragPos, nonlinearDepth);
    float3 n = normalize(normal);
    bool _639 = !all(n == n);
    bool _646;
    if (!_639)
    {
        _646 = length(n) < 9.99999975e-05;
    }
    else
    {
        _646 = _639;
    }
    if (_646)
    {
        n = normalize(input.Normal);
    }
    output.gNormal = float4(n, 1.0);
    float3 a = clamp(albedoColor, float3(0.0), float3(1.0));
    if (!all(a == a))
    {
        a = float3(0.0);
    }
    output.gAlbedoSpec = float4(a, aoValue);
    output.gMaterial = float4(metallicValue, roughnessValue, aoValue, clamp(material.reflectivity, 0.0, 1.0));
    return output;
}
#endif
