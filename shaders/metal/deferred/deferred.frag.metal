#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

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
    packed_float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct main0_out
{
    float4 gPosition [[color(0)]];
    float4 gNormal [[color(1)]];
    float4 gAlbedoSpec [[color(2)]];
    float4 gMaterial [[color(3)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn1)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
};

static inline __attribute__((always_inline))
float4 sampleTextureAt(thread const int& textureIndex, thread const float2& uv, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    if (textureIndex == 0)
    {
        return texture1.sample(texture1Smplr, uv);
    }
    else
    {
        if (textureIndex == 1)
        {
            return texture2.sample(texture2Smplr, uv);
        }
        else
        {
            if (textureIndex == 2)
            {
                return texture3.sample(texture3Smplr, uv);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return texture4.sample(texture4Smplr, uv);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return texture5.sample(texture5Smplr, uv);
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return texture6.sample(texture6Smplr, uv);
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return texture7.sample(texture7Smplr, uv);
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return texture8.sample(texture8Smplr, uv);
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return texture9.sample(texture9Smplr, uv);
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return texture10.sample(texture10Smplr, uv);
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

static inline __attribute__((always_inline))
float2 parallaxMapping(thread const float2& texCoords, thread const float3& viewDir, constant UBO& _46, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 v = fast::normalize(viewDir);
    float numLayers = mix(32.0, 8.0, abs(dot(float3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float2 P = (v.xy / float2(fast::max(v.z, 0.0500000007450580596923828125))) * 0.039999999105930328369140625;
    float2 deltaTexCoords = P / float2(numLayers);
    float2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == 6)
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
    float currentDepthMapValue = 1.0 - sampleTextureAt(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
    int maxIterations = int(numLayers) + 1;
    int iteration = 0;
    while (currentLayerDepth < currentDepthMapValue && iteration < maxIterations)
    {
        currentTexCoords -= deltaTexCoords;
        int param_2 = textureIndex;
        float2 param_3 = currentTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(param_2, param_3, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
        currentLayerDepth += layerDepth;
        iteration++;
    }
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    int param_4 = textureIndex;
    float2 param_5 = prevTexCoords;
    float beforeDepth = 1.0 - sampleTextureAt(param_4, param_5, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x - (currentLayerDepth - layerDepth);
    float denom = afterDepth - beforeDepth;
    float weight = abs(denom) > 9.9999997473787516355514526367188e-05 ? fast::clamp(afterDepth / denom, 0.0, 1.0) : 0.0;
    currentTexCoords = (prevTexCoords * weight) + (currentTexCoords * (1.0 - weight));
    return currentTexCoords;
}

static inline __attribute__((always_inline))
float4 enableTextures(thread const int& type, constant UBO& _46, texture2d<float> texture1, sampler texture1Smplr, thread float2& texCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float4 color = float4(0.0);
    int count = 0;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == type)
        {
            if (i == 0)
            {
                color += texture1.sample(texture1Smplr, texCoord);
            }
            else
            {
                if (i == 1)
                {
                    color += texture2.sample(texture2Smplr, texCoord);
                }
                else
                {
                    if (i == 2)
                    {
                        color += texture3.sample(texture3Smplr, texCoord);
                    }
                    else
                    {
                        if (i == 3)
                        {
                            color += texture4.sample(texture4Smplr, texCoord);
                        }
                        else
                        {
                            if (i == 4)
                            {
                                color += texture5.sample(texture5Smplr, texCoord);
                            }
                            else
                            {
                                if (i == 5)
                                {
                                    color += texture6.sample(texture6Smplr, texCoord);
                                }
                                else
                                {
                                    if (i == 6)
                                    {
                                        color += texture7.sample(texture7Smplr, texCoord);
                                    }
                                    else
                                    {
                                        if (i == 7)
                                        {
                                            color += texture8.sample(texture8Smplr, texCoord);
                                        }
                                        else
                                        {
                                            if (i == 8)
                                            {
                                                color += texture9.sample(texture9Smplr, texCoord);
                                            }
                                            else
                                            {
                                                if (i == 9)
                                                {
                                                    color += texture10.sample(texture10Smplr, texCoord);
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

fragment main0_out main0(main0_in in [[stage_in]], constant UBO& _46 [[buffer(0)]], constant MaterialPush& material [[buffer(1)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float3x3 TBN = {};
    TBN[0] = in.TBN_0;
    TBN[1] = in.TBN_1;
    TBN[2] = in.TBN_2;
    float2 texCoord = in.TexCoord * _46.textureScale + _46.textureOffset;
    bool hasParallaxMap = false;
    for (int i = 0; i < _46.textureCount; i++)
    {
        if (_46.textureTypes[i].x == 6)
        {
            hasParallaxMap = true;
            break;
        }
    }
    if (hasParallaxMap)
    {
        float3 tangentViewDir = fast::normalize(transpose(TBN) * (_46.cameraPosition - in.FragPos));
        float2 param = texCoord;
        float3 param_1 = tangentViewDir;
        texCoord = parallaxMapping(param, param_1, _46, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    }
    int param_2 = 0;
    float4 sampledColor = enableTextures(param_2, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool hasColorTexture = any(sampledColor != float4(-1.0));
    float4 baseColor = float4(material.albedo[0], material.albedo[1], material.albedo[2], 1.0);
    int param_3 = 0;
    float4 albedoTex = enableTextures(param_3, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(albedoTex != float4(-1.0)))
    {
        baseColor *= albedoTex;
    }
    int param_3a = 12;
    float4 opacityTex = enableTextures(param_3a, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(opacityTex != float4(-1.0)))
    {
        if (opacityTex.x < 0.100000001490116119384765625)
        {
            discard_fragment();
        }
    }
    else
    {
        if ((baseColor.w < 0.100000001490116119384765625) && (material.albedo[3] < 0.999000012874603271484375))
        {
            discard_fragment();
        }
    }
    int param_4 = 5;
    float4 normTexture = enableTextures(param_4, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
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
    float normalStrength = fast::max(_46.normalMapStrength, 0.0f);
    bool useNormalMap = (_46.useNormalMap != 0u) && (normalStrength > 0.0f);
    float3 normal;
    if (_540 && useNormalMap)
    {
        float3 tangentNormal = (normTexture.xyz * 2.0) - float3(1.0);
        tangentNormal.xy *= normalStrength;
        tangentNormal = fast::normalize(tangentNormal);
        normal = fast::normalize(TBN * tangentNormal);
    }
    else
    {
        normal = fast::normalize(in.Normal);
    }
    float3 albedoColor = baseColor.xyz;
    int param_pbr_pack = 14;
    float4 pbrPackTex = enableTextures(param_pbr_pack, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool hasPbrPack = any(pbrPackTex != float4(-1.0));
    float metallicValue = material.metallic;
    int param_5 = 9;
    float4 metallicTex = enableTextures(param_5, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
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
    float4 roughnessTex = enableTextures(param_6, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
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
    float4 aoTex = enableTextures(param_7, _46, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (hasPbrPack)
    {
        aoValue *= pbrPackTex.x;
    }
    else if (any(aoTex != float4(-1.0)))
    {
        aoValue *= aoTex.x;
    }
    metallicValue = fast::clamp(metallicValue, 0.0, 1.0);
    roughnessValue = fast::clamp(roughnessValue, 0.0, 1.0);
    aoValue = fast::clamp(aoValue, 0.0, 1.0);
    float nonlinearDepth = gl_FragCoord.z;
    out.gPosition = float4(in.FragPos, nonlinearDepth);
    float3 n = fast::normalize(normal);
    bool _639 = !all(n == n);
    bool _646;
    if (!_639)
    {
        _646 = length(n) < 9.9999997473787516355514526367188e-05;
    }
    else
    {
        _646 = _639;
    }
    if (_646)
    {
        n = fast::normalize(in.Normal);
    }
    out.gNormal = float4(n, 1.0);
    float3 a = fast::clamp(albedoColor, float3(0.0), float3(1.0));
    if (!all(a == a))
    {
        a = float3(0.0);
    }
    out.gAlbedoSpec = float4(a, aoValue);
    out.gMaterial = float4(metallicValue, roughnessValue, aoValue, 1.0);
    return out;
}
