#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template<typename T, size_t Num>
struct spvUnsafeArray
{
    T elements[Num ? Num : 1];
    
    thread T& operator [] (size_t pos) thread
    {
        return elements[pos];
    }
    constexpr const thread T& operator [] (size_t pos) const thread
    {
        return elements[pos];
    }
    
    device T& operator [] (size_t pos) device
    {
        return elements[pos];
    }
    constexpr const device T& operator [] (size_t pos) const device
    {
        return elements[pos];
    }
    
    constexpr const constant T& operator [] (size_t pos) const constant
    {
        return elements[pos];
    }
    
    threadgroup T& operator [] (size_t pos) threadgroup
    {
        return elements[pos];
    }
    constexpr const threadgroup T& operator [] (size_t pos) const threadgroup
    {
        return elements[pos];
    }
};

// Implementation of the GLSL radians() function
template<typename T>
inline T radians(T d)
{
    return d * T(0.01745329251);
}

static inline __attribute__((always_inline))
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

static inline __attribute__((always_inline))
float spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) + c1 * spvDet2x2(a2, a3, b2, b3);
}

static inline __attribute__((always_inline))
float4x4 spvInverse4x4(float4x4 m)
{
    float4x4 adj;
    adj[0][0] =  spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
    adj[0][2] =  spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);
    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][1] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
    adj[1][3] =  spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);
    adj[2][0] =  spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
    adj[2][2] =  spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);
    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][1] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
    adj[3][3] =  spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct ShadowParameters
{
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    float _pad1;
    float3 lightPos;
    int lightType;
};

struct Uniforms
{
    int4 textureTypes[16];
    int textureCount;
    float3 cameraPosition;
    float normalMapStrength;
    uint useNormalMap;
    float2 textureScale;
    float2 textureOffset;
};

struct Environment
{
    float rimLightIntensity;
    float3 rimLightColor;
};

struct PushConstants
{
    uint useTexture;
    uint useColor;
    uint useIBL;
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
};

struct DirectionalLight
{
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct DirectionalLightsUBO
{
    spvUnsafeArray<DirectionalLight, 1> directionalLights;
};

struct PointLight
{
    packed_float3 position;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct PointLightsUBO
{
    spvUnsafeArray<PointLight, 1> pointLights;
};

struct SpotLight
{
    packed_float3 position;
    float _pad1;
    packed_float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    packed_float3 diffuse;
    float _pad5;
    packed_float3 specular;
    float _pad6;
};

struct SpotLightsUBO
{
    spvUnsafeArray<SpotLight, 1> spotlights;
};

struct Material
{
    packed_float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
};

struct ShadowParameters_1
{
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    float _pad1;
    packed_float3 lightPos;
    int lightType;
};

struct ShadowParametersUBO
{
    spvUnsafeArray<ShadowParameters_1, 1> shadowParams;
};

struct AreaLight
{
    packed_float3 position;
    float _pad1;
    packed_float3 right;
    float _pad2;
    packed_float3 up;
    float _pad3;
    float2 size;
    float _pad4;
    float _pad5;
    packed_float3 diffuse;
    float _pad6;
    packed_float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

struct AreaLightsUBO
{
    spvUnsafeArray<AreaLight, 1> areaLights;
};

struct AmbientLight
{
    float4 color;
    float intensity;
    float3 _pad0;
};

constant spvUnsafeArray<float3, 54> _1657 = spvUnsafeArray<float3, 54>({ float3(0.5381000041961669921875, 0.18559999763965606689453125, -0.4318999946117401123046875), float3(0.13789999485015869140625, 0.248600006103515625, 0.4429999887943267822265625), float3(0.3370999991893768310546875, 0.567900002002716064453125, -0.0057000000961124897003173828125), float3(-0.699899971485137939453125, -0.0450999997556209564208984375, -0.0019000000320374965667724609375), float3(0.068899996578693389892578125, -0.159799993038177490234375, -0.854700028896331787109375), float3(0.056000001728534698486328125, 0.0068999999202787876129150390625, -0.184300005435943603515625), float3(-0.014600000344216823577880859375, 0.14020000398159027099609375, 0.076200000941753387451171875), float3(0.00999999977648258209228515625, -0.19239999353885650634765625, -0.03440000116825103759765625), float3(-0.35769999027252197265625, -0.53009998798370361328125, -0.4357999861240386962890625), float3(-0.3169000148773193359375, 0.10629999637603759765625, 0.015799999237060546875), float3(0.010300000198185443878173828125, -0.5868999958038330078125, 0.0046000001020729541778564453125), float3(-0.08969999849796295166015625, -0.4939999878406524658203125, 0.328700006008148193359375), float3(0.7118999958038330078125, -0.015399999916553497314453125, -0.091799996793270111083984375), float3(-0.053300000727176666259765625, 0.0595999993383884429931640625, -0.541100025177001953125), float3(0.03519999980926513671875, -0.063100002706050872802734375, 0.546000003814697265625), float3(-0.4776000082492828369140625, 0.2847000062465667724609375, -0.0271000005304813385009765625), float3(-0.11200000345706939697265625, 0.1234000027179718017578125, -0.744599997997283935546875), float3(-0.212999999523162841796875, -0.07819999754428863525390625, -0.13789999485015869140625), float3(0.2944000065326690673828125, -0.3111999928951263427734375, -0.2644999921321868896484375), float3(-0.4564000070095062255859375, 0.4174999892711639404296875, -0.184300005435943603515625), float3(0.1234000027179718017578125, -0.567799985408782958984375, 0.788999974727630615234375), float3(-0.6789000034332275390625, 0.23450000584125518798828125, -0.4566999971866607666015625), float3(0.34560000896453857421875, -0.788999974727630615234375, 0.1234000027179718017578125), float3(-0.23450000584125518798828125, 0.567799985408782958984375, -0.6789000034332275390625), float3(0.788999974727630615234375, 0.1234000027179718017578125, 0.567799985408782958984375), float3(-0.567799985408782958984375, -0.6789000034332275390625, 0.23450000584125518798828125), float3(0.4566999971866607666015625, 0.788999974727630615234375, -0.23450000584125518798828125), float3(-0.788999974727630615234375, 0.34560000896453857421875, -0.567799985408782958984375), float3(0.6789000034332275390625, -0.23450000584125518798828125, 0.788999974727630615234375), float3(-0.1234000027179718017578125, 0.6789000034332275390625, -0.4566999971866607666015625), float3(0.23450000584125518798828125, -0.567799985408782958984375, 0.6789000034332275390625), float3(-0.34560000896453857421875, 0.788999974727630615234375, -0.1234000027179718017578125), float3(0.567799985408782958984375, 0.23450000584125518798828125, -0.788999974727630615234375), float3(-0.6789000034332275390625, -0.567799985408782958984375, 0.34560000896453857421875), float3(0.788999974727630615234375, -0.34560000896453857421875, 0.4566999971866607666015625), float3(-0.23450000584125518798828125, 0.1234000027179718017578125, -0.6789000034332275390625), float3(0.4566999971866607666015625, 0.788999974727630615234375, -0.567799985408782958984375), float3(-0.567799985408782958984375, 0.23450000584125518798828125, 0.6789000034332275390625), float3(0.34560000896453857421875, -0.788999974727630615234375, -0.1234000027179718017578125), float3(-0.788999974727630615234375, 0.567799985408782958984375, -0.23450000584125518798828125), float3(0.6789000034332275390625, -0.1234000027179718017578125, 0.34560000896453857421875), float3(-0.4566999971866607666015625, 0.788999974727630615234375, 0.23450000584125518798828125), float3(0.567799985408782958984375, -0.6789000034332275390625, 0.788999974727630615234375), float3(-0.34560000896453857421875, 0.567799985408782958984375, -0.6789000034332275390625), float3(0.23450000584125518798828125, -0.788999974727630615234375, 0.567799985408782958984375), float3(-0.6789000034332275390625, 0.23450000584125518798828125, -0.1234000027179718017578125), float3(0.788999974727630615234375, -0.34560000896453857421875, -0.567799985408782958984375), float3(-0.567799985408782958984375, 0.6789000034332275390625, 0.23450000584125518798828125), float3(0.4566999971866607666015625, -0.788999974727630615234375, 0.34560000896453857421875), float3(-0.23450000584125518798828125, 0.1234000027179718017578125, -0.788999974727630615234375), float3(0.34560000896453857421875, -0.567799985408782958984375, 0.6789000034332275390625), float3(-0.788999974727630615234375, 0.4566999971866607666015625, -0.34560000896453857421875), float3(0.6789000034332275390625, -0.1234000027179718017578125, -0.567799985408782958984375), float3(-0.4566999971866607666015625, 0.23450000584125518798828125, 0.788999974727630615234375) });

struct main0_out
{
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
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
float2 parallaxMapping(thread const float2& texCoords, thread const float3& viewDir, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 v = fast::normalize(viewDir);
    float numLayers = mix(32.0, 8.0, abs(dot(float3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float2 P = (v.xy / float2(fast::max(v.z, 0.0500000007450580596923828125))) * 0.039999999105930328369140625;
    float2 deltaTexCoords = P / float2(numLayers);
    float2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 6)
        {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == (-1))
    {
        return texCoords;
    }
    int param = textureIndex;
    float2 param_1 = currentTexCoords;
    float currentDepthMapValue = 1.0 - sampleTextureAt(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        int param_2 = textureIndex;
        float2 param_3 = currentTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(param_2, param_3, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
        currentLayerDepth += layerDepth;
    }
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    int param_4 = textureIndex;
    float2 param_5 = prevTexCoords;
    float beforeDepth = 1.0 - sampleTextureAt(param_4, param_5, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = (prevTexCoords * weight) + (currentTexCoords * (1.0 - weight));
    return currentTexCoords;
}

static inline __attribute__((always_inline))
float4 enableTextures(thread const int& type, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, thread float2& texCoord, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float4 color = float4(0.0);
    int count = 0;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == type)
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

static inline __attribute__((always_inline))
float2 getTextureDimensions(thread const int& textureIndex, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    if (textureIndex == 0)
    {
        return float2(int2(texture1.get_width(), texture1.get_height()));
    }
    else
    {
        if (textureIndex == 1)
        {
            return float2(int2(texture2.get_width(), texture2.get_height()));
        }
        else
        {
            if (textureIndex == 2)
            {
                return float2(int2(texture3.get_width(), texture3.get_height()));
            }
            else
            {
                if (textureIndex == 3)
                {
                    return float2(int2(texture4.get_width(), texture4.get_height()));
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return float2(int2(texture5.get_width(), texture5.get_height()));
                    }
                    else
                    {
                        if (textureIndex == 5)
                        {
                            return float2(int2(texture6.get_width(), texture6.get_height()));
                        }
                        else
                        {
                            if (textureIndex == 6)
                            {
                                return float2(int2(texture7.get_width(), texture7.get_height()));
                            }
                            else
                            {
                                if (textureIndex == 7)
                                {
                                    return float2(int2(texture8.get_width(), texture8.get_height()));
                                }
                                else
                                {
                                    if (textureIndex == 8)
                                    {
                                        return float2(int2(texture9.get_width(), texture9.get_height()));
                                    }
                                    else
                                    {
                                        if (textureIndex == 9)
                                        {
                                            return float2(int2(texture10.get_width(), texture10.get_height()));
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
    return float2(0.0);
}

static inline __attribute__((always_inline))
float calculateShadow(thread const ShadowParameters& shadowParam, thread const float4& fragPosLightSpace, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr, device DirectionalLightsUBO& _1083, thread float3& Normal, thread float3& FragPos)
{
    float3 projCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    projCoords = (projCoords * 0.5) + float3(0.5);
    bool _1362 = projCoords.x < 0.0;
    bool _1369;
    if (!_1362)
    {
        _1369 = projCoords.x > 1.0;
    }
    else
    {
        _1369 = _1362;
    }
    bool _1376;
    if (!_1369)
    {
        _1376 = projCoords.y < 0.0;
    }
    else
    {
        _1376 = _1369;
    }
    bool _1383;
    if (!_1376)
    {
        _1383 = projCoords.y > 1.0;
    }
    else
    {
        _1383 = _1376;
    }
    bool _1390;
    if (!_1383)
    {
        _1390 = projCoords.z < 0.0;
    }
    else
    {
        _1390 = _1383;
    }
    bool _1398;
    if (!_1390)
    {
        _1398 = projCoords.z > 1.0;
    }
    else
    {
        _1398 = _1390;
    }
    if (_1398)
    {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    float3 lightDir = fast::normalize(-(spvInverse4x4(shadowParam.lightView) * float4(0.0, 0.0, -1.0, 0.0)).xyz);
    float3 normal = fast::normalize(Normal);
    float biasValue = shadowParam.bias0;
    float bias0 = fast::max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);
    float shadow = 0.0;
    int param = shadowParam.textureIndex;
    float2 texelSize = float2(1.0) / getTextureDimensions(param, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    float _distance = length(_163.cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, fast::clamp(_distance / 100.0, 0.0, 1.0)));
    int sampleCount = 0;
    int _1444 = -kernelSize;
    for (int x = _1444; x <= kernelSize; x++)
    {
        int _1455 = -kernelSize;
        for (int y = _1455; y <= kernelSize; y++)
        {
            int param_1 = shadowParam.textureIndex;
            float2 param_2 = projCoords.xy + (float2(float(x), float(y)) * texelSize);
            float pcfDepth = sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).x;
            shadow += float((currentDepth - bias0) > pcfDepth);
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);
    return shadow;
}

static inline __attribute__((always_inline))
float4 sampleCubeTextureAt(thread const int& textureIndex, thread const float3& direction, texturecube<float> cubeMap1, sampler cubeMap1Smplr, texturecube<float> cubeMap2, sampler cubeMap2Smplr, texturecube<float> cubeMap3, sampler cubeMap3Smplr, texturecube<float> cubeMap4, sampler cubeMap4Smplr, texturecube<float> cubeMap5, sampler cubeMap5Smplr)
{
    if (textureIndex == 0)
    {
        return cubeMap1.sample(cubeMap1Smplr, direction);
    }
    else
    {
        if (textureIndex == 1)
        {
            return cubeMap2.sample(cubeMap2Smplr, direction);
        }
        else
        {
            if (textureIndex == 2)
            {
                return cubeMap3.sample(cubeMap3Smplr, direction);
            }
            else
            {
                if (textureIndex == 3)
                {
                    return cubeMap4.sample(cubeMap4Smplr, direction);
                }
                else
                {
                    if (textureIndex == 4)
                    {
                        return cubeMap5.sample(cubeMap5Smplr, direction);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline))
float calculatePointShadow(thread const ShadowParameters& shadowParam, thread const float3& fragPos, texturecube<float> cubeMap1, sampler cubeMap1Smplr, texturecube<float> cubeMap2, sampler cubeMap2Smplr, texturecube<float> cubeMap3, sampler cubeMap3Smplr, texturecube<float> cubeMap4, sampler cubeMap4Smplr, texturecube<float> cubeMap5, sampler cubeMap5Smplr)
{
    float3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);
    float bias0 = 0.0500000007450580596923828125;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.0500000007450580596923828125;
    for (int i = 0; i < 54; i++)
    {
        float3 sampleDir = fast::normalize(fragToLight + (_1657[i] * diskRadius));
        int param = shadowParam.textureIndex;
        float3 param_1 = sampleDir;
        float closestDepth = sampleCubeTextureAt(param, param_1, cubeMap1, cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3, cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5, cubeMap5Smplr).x * shadowParam.farPlane;
        if ((currentDepth - bias0) > closestDepth)
        {
            shadow += 1.0;
        }
    }
    shadow /= 54.0;
    return shadow;
}

static inline __attribute__((always_inline))
float distributionGGX(thread const float3& N, thread const float3& H, thread const float& roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = fast::max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)) + 1.0;
    denom = (3.1415927410125732421875 * denom) * denom;
    return num / denom;
}

static inline __attribute__((always_inline))
float geometrySchlickGGX(thread const float& NdotV, thread const float& roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = (NdotV * (1.0 - k)) + k;
    return num / denom;
}

static inline __attribute__((always_inline))
float geometrySmith(thread const float3& N, thread const float3& V, thread const float3& L, thread const float& roughness)
{
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float param = NdotV;
    float param_1 = roughness;
    float ggx2 = geometrySchlickGGX(param, param_1);
    float param_2 = NdotL;
    float param_3 = roughness;
    float ggx1 = geometrySchlickGGX(param_2, param_3);
    return ggx1 * ggx2;
}

static inline __attribute__((always_inline))
float3 fresnelSchlick(thread const float& cosTheta, thread const float3& F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline))
float3 calculatePBR(thread const float3& N, thread const float3& V, thread const float3& L, thread const float3& F0, thread const float3& radiance, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float& reflectivity)
{
    float3 H = fast::normalize(V + L);
    float3 param = N;
    float3 param_1 = H;
    float param_2 = roughness;
    float NDF = distributionGGX(param, param_1, param_2);
    float3 param_3 = N;
    float3 param_4 = V;
    float3 param_5 = L;
    float param_6 = roughness;
    float G = geometrySmith(param_3, param_4, param_5, param_6);
    float param_7 = fast::max(dot(H, V), 0.0);
    float3 param_8 = F0;
    float3 F = fresnelSchlick(param_7, param_8);
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= (1.0 - metallic);
    float3 numerator = F * (NDF * G);
    float denominator = ((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0)) + 9.9999997473787516355514526367188e-05;
    float3 specular = numerator / float3(denominator);
    float NdotL = fast::max(dot(N, L), 0.0);
    float3 Lo = ((((kD * albedo) / float3(3.1415927410125732421875)) + specular) * radiance) * NdotL;
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllDirectionalLights(thread const float3& N, thread const float3& V, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device DirectionalLightsUBO& _1083)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.directionalLightCount; i++)
    {
        float3 L = fast::normalize(-float3(_1083.directionalLights[i].direction));
        float3 radiance = float3(_1083.directionalLights[i].diffuse) * fast::max(_1083.directionalLights[i].intensity, 0.0);
        float3 param = N;
        float3 param_1 = V;
        float3 param_2 = L;
        float3 param_3 = F0;
        float3 param_4 = radiance;
        float3 param_5 = albedo;
        float param_6 = metallic;
        float param_7 = roughness;
        float param_8 = reflectivity;
        Lo += calculatePBR(param, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllPointLights(thread const float3& fragPos, thread const float3& N, thread const float3& V, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device PointLightsUBO& _1136)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.pointLightCount; i++)
    {
        float3 L = float3(_1136.pointLights[i].position) - fragPos;
        float _distance = length(L);
        _distance = fast::max(_distance, 0.001000000047497451305389404296875);
        L = fast::normalize(L);
        float range = fast::max(_1136.pointLights[i].radius, 0.001000000047497451305389404296875);
        float3 radiance = float3(_1136.pointLights[i].diffuse) * fast::max(_1136.pointLights[i].intensity, 0.0);
        float attenuation = 1.0 / ((1.0 + (_distance / range)) + ((_distance * _distance) / (range * range)));
        float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
        float3 radianceAttenuated = radiance * attenuation;
        radianceAttenuated *= fade;
        float3 H = fast::normalize(V + L);
        float3 param = N;
        float3 param_1 = H;
        float param_2 = roughness;
        float NDF = distributionGGX(param, param_1, param_2);
        float3 param_3 = N;
        float3 param_4 = V;
        float3 param_5 = L;
        float param_6 = roughness;
        float G = geometrySmith(param_3, param_4, param_5, param_6);
        float param_7 = fast::max(dot(H, V), 0.0);
        float3 param_8 = F0;
        float3 F = fresnelSchlick(param_7, param_8);
        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD *= (1.0 - metallic);
        float3 numerator = F * (NDF * G);
        float denominator = ((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0)) + 9.9999997473787516355514526367188e-05;
        float3 specular = numerator / float3(denominator);
        float NdotL = fast::max(dot(N, L), 0.0);
        Lo += (((((kD * albedo) / float3(3.1415927410125732421875)) + specular) * radianceAttenuated) * NdotL);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 calcAllSpotLights(thread const float3& N, thread const float3& fragPos, thread const float3& L, thread const float3& viewDir, thread const float3& albedo, thread const float& metallic, thread const float& roughness, thread const float3& F0, thread const float& reflectivity, constant PushConstants& _1073, device SpotLightsUBO& _1268)
{
    float3 Lo = float3(0.0);
    for (int i = 0; i < _1073.spotlightCount; i++)
    {
        float3 L_1 = fast::normalize(float3(_1268.spotlights[i].position) - fragPos);
        float3 spotDirection = fast::normalize(float3(_1268.spotlights[i].direction));
        float theta = dot(L_1, -spotDirection);
        float intensity = smoothstep(_1268.spotlights[i].outerCutOff, _1268.spotlights[i].cutOff, theta);
        float _distance = length(float3(_1268.spotlights[i].position) - fragPos);
        _distance = fast::max(_distance, 0.001000000047497451305389404296875);
        float range = fast::max(_1268.spotlights[i].range, 0.001000000047497451305389404296875);
        float attenuation = 1.0 / ((1.0 + (_distance / range)) + ((_distance * _distance) / (range * range)));
        float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
        float3 radiance = (((float3(_1268.spotlights[i].diffuse) * fast::max(_1268.spotlights[i].intensity, 0.0)) * attenuation) * intensity) * fade;
        float3 param = N;
        float3 param_1 = viewDir;
        float3 param_2 = L_1;
        float3 param_3 = F0;
        float3 param_4 = radiance;
        float3 param_5 = albedo;
        float param_6 = metallic;
        float param_7 = roughness;
        float param_8 = reflectivity;
        Lo += calculatePBR(param, param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
    }
    return Lo;
}

static inline __attribute__((always_inline))
float3 getRimLight(thread const float3& fragPos, thread float3& N, thread float3& V, thread const float3& F0, thread const float3& albedo, thread const float& metallic, thread const float& roughness, constant Uniforms& _163, constant Environment& environment)
{
    N = fast::normalize(N);
    V = fast::normalize(V);
    float rim = powr(1.0 - fast::max(dot(N, V), 0.0), 3.0);
    rim *= mix(1.2000000476837158203125, 0.300000011920928955078125, roughness);
    float3 rimColor = mix(float3(1.0), albedo, float3(metallic)) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, float3(0.5));
    float rimIntensity = environment.rimLightIntensity;
    float3 rimLight = (rimColor * rim) * rimIntensity;
    float dist = length(_163.cameraPosition - fragPos);
    rimLight /= float3(1.0 + (dist * 0.100000001490116119384765625));
    return rimLight;
}

static inline __attribute__((always_inline))
float2 directionToEquirect(thread const float3& direction)
{
    float3 dir = fast::normalize(direction);
    float phi = precise::atan2(dir.z, dir.x);
    float theta = acos(fast::clamp(dir.y, -1.0, 1.0));
    return float2((phi + 3.1415927410125732421875) / 6.283185482025146484375, theta / 3.1415927410125732421875);
}

static inline __attribute__((always_inline))
float3 sampleHDRTexture(thread const int& textureIndex, thread const float3& direction, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 param = direction;
    float2 uv = directionToEquirect(param);
    int param_1 = textureIndex;
    float2 param_2 = uv;
    return sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr).xyz;
}

static inline __attribute__((always_inline))
float3 sampleEnvironmentRadiance(thread const float3& direction, constant Uniforms& _163, texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr, texture2d<float> texture6, sampler texture6Smplr, texture2d<float> texture7, sampler texture7Smplr, texture2d<float> texture8, sampler texture8Smplr, texture2d<float> texture9, sampler texture9Smplr, texture2d<float> texture10, sampler texture10Smplr)
{
    float3 envColor = float3(0.0);
    int count = 0;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 13)
        {
            int param = i;
            float3 param_1 = direction;
            envColor += sampleHDRTexture(param, param_1, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
            count++;
        }
    }
    if (count == 0)
    {
        return float3(0.0);
    }
    return envColor / float3(float(count));
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread float3& color)
{
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    color = (color * ((color * a) + float3(b))) / ((color * ((color * c) + float3(d))) + float3(e));
    color = powr(fast::clamp(color, float3(0.0), float3(1.0)), float3(0.4545454680919647216796875));
    return color;
}

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _163 [[buffer(0)]], constant Environment& environment [[buffer(1)]], constant PushConstants& _1073 [[buffer(2)]], device DirectionalLightsUBO& _1083 [[buffer(3)]], device PointLightsUBO& _1136 [[buffer(4)]], device SpotLightsUBO& _1268 [[buffer(5)]], constant Material& material [[buffer(6)]], device ShadowParametersUBO& _1905 [[buffer(7)]], device AreaLightsUBO& _2058 [[buffer(8)]], constant AmbientLight& ambientLight [[buffer(9)]], texture2d<float> texture1 [[texture(0)]], texture2d<float> texture2 [[texture(1)]], texture2d<float> texture3 [[texture(2)]], texture2d<float> texture4 [[texture(3)]], texture2d<float> texture5 [[texture(4)]], texture2d<float> texture6 [[texture(5)]], texture2d<float> texture7 [[texture(6)]], texture2d<float> texture8 [[texture(7)]], texture2d<float> texture9 [[texture(8)]], texture2d<float> texture10 [[texture(9)]], texturecube<float> cubeMap1 [[texture(10)]], texturecube<float> cubeMap2 [[texture(11)]], texturecube<float> cubeMap3 [[texture(12)]], texturecube<float> cubeMap4 [[texture(13)]], texturecube<float> cubeMap5 [[texture(14)]], sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]], sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]], sampler texture5Smplr [[sampler(4)]], sampler texture6Smplr [[sampler(5)]], sampler texture7Smplr [[sampler(6)]], sampler texture8Smplr [[sampler(7)]], sampler texture9Smplr [[sampler(8)]], sampler texture10Smplr [[sampler(9)]], sampler cubeMap1Smplr [[sampler(10)]], sampler cubeMap2Smplr [[sampler(11)]], sampler cubeMap3Smplr [[sampler(12)]], sampler cubeMap4Smplr [[sampler(13)]], sampler cubeMap5Smplr [[sampler(14)]])
{
    main0_out out = {};
    float3x3 TBN = {};
    TBN[0] = in.TBN_0;
    TBN[1] = in.TBN_1;
    TBN[2] = in.TBN_2;
    float2 texCoord = in.TexCoord * _163.textureScale + _163.textureOffset;
    bool hasParallaxMap = false;
    for (int i = 0; i < _163.textureCount; i++)
    {
        if (_163.textureTypes[i].x == 6)
        {
            hasParallaxMap = true;
            break;
        }
    }
    if (hasParallaxMap)
    {
        float3 tangentViewDir = fast::normalize(transpose(TBN) * (_163.cameraPosition - in.FragPos));
        float2 param = texCoord;
        float3 param_1 = tangentViewDir;
        texCoord = parallaxMapping(param, param_1, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    }
    int param_2 = 5;
    float4 normTexture = enableTextures(param_2, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    bool _1774 = normTexture.x != (-1.0);
    bool _1780;
    if (_1774)
    {
        _1780 = normTexture.y != (-1.0);
    }
    else
    {
        _1780 = _1774;
    }
    bool _1786;
    if (_1780)
    {
        _1786 = normTexture.z != (-1.0);
    }
    else
    {
        _1786 = _1780;
    }
    float normalStrength = fast::max(_163.normalMapStrength, 0.0f);
    bool useNormalMap = (_163.useNormalMap != 0u) && (normalStrength > 0.0f);
    float3 N;
    if (_1786 && useNormalMap)
    {
        float3 tangentNormal = (normTexture.xyz * 2.0) - float3(1.0);
        tangentNormal.xy *= normalStrength;
        tangentNormal = fast::normalize(tangentNormal);
        N = fast::normalize(TBN * tangentNormal);
    }
    else
    {
        N = fast::normalize(in.Normal);
    }
    N = fast::normalize(N);
    float3 V = fast::normalize(_163.cameraPosition - in.FragPos);
    float3 albedo = float3(material.albedo);
    int param_3 = 0;
    float4 albedoTex = enableTextures(param_3, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(albedoTex != float4(-1.0)))
    {
        albedo *= albedoTex.xyz;
    }
    int param_3a = 12;
    float4 opacityTex = enableTextures(param_3a, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(opacityTex != float4(-1.0)))
    {
        if (opacityTex.x < 0.100000001490116119384765625)
        {
            discard_fragment();
        }
    }
    else
    {
        if ((albedoTex.w < 0.100000001490116119384765625) && (material.albedo[3] < 0.999000012874603271484375))
        {
            discard_fragment();
        }
    }
    float metallic = material.metallic;
    int param_4 = 9;
    float4 metallicTex = enableTextures(param_4, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(metallicTex != float4(-1.0)))
    {
        metallic *= metallicTex.x;
    }
    float roughness = material.roughness;
    int param_5 = 10;
    float4 roughnessTex = enableTextures(param_5, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(roughnessTex != float4(-1.0)))
    {
        roughness *= roughnessTex.x;
    }
    float ao = material.ao;
    int param_6 = 11;
    float4 aoTex = enableTextures(param_6, _163, texture1, texture1Smplr, texCoord, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
    if (any(aoTex != float4(-1.0)))
    {
        ao *= aoTex.x;
    }
    float3 F0 = float3(0.039999999105930328369140625);
    F0 = mix(F0, albedo, float3(metallic));
    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    float pointShadow = 0.0;
    if (_1073.shadowParamCount > 0)
    {
        for (int i_1 = 0; i_1 < _1073.shadowParamCount; i_1++)
        {
            if (_1905.shadowParams[i_1].lightType == 0)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                areaShadow = fast::max(areaShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else if (_1905.shadowParams[i_1].lightType == 1)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                spotShadow = fast::max(spotShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else if (_1905.shadowParams[i_1].lightType == 2)
            {
                float4 fragPosLightSpace = (_1905.shadowParams[i_1].lightProjection * _1905.shadowParams[i_1].lightView) * float4(in.FragPos, 1.0);
                ShadowParameters _1934;
                _1934.lightView = _1905.shadowParams[i_1].lightView;
                _1934.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1934.bias0 = _1905.shadowParams[i_1].bias0;
                _1934.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1934.farPlane = _1905.shadowParams[i_1].farPlane;
                _1934._pad1 = _1905.shadowParams[i_1]._pad1;
                _1934.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1934.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_7 = _1934;
                float4 param_8 = fragPosLightSpace;
                directionalShadow = fast::max(directionalShadow, calculateShadow(param_7, param_8, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr, _1083, in.Normal, in.FragPos));
            }
            else
            {
                ShadowParameters _1945;
                _1945.lightView = _1905.shadowParams[i_1].lightView;
                _1945.lightProjection = _1905.shadowParams[i_1].lightProjection;
                _1945.bias0 = _1905.shadowParams[i_1].bias0;
                _1945.textureIndex = _1905.shadowParams[i_1].textureIndex;
                _1945.farPlane = _1905.shadowParams[i_1].farPlane;
                _1945._pad1 = _1905.shadowParams[i_1]._pad1;
                _1945.lightPos = float3(_1905.shadowParams[i_1].lightPos);
                _1945.lightType = _1905.shadowParams[i_1].lightType;
                ShadowParameters param_9 = _1945;
                float3 param_10 = in.FragPos;
                pointShadow = fast::max(pointShadow, calculatePointShadow(param_9, param_10, cubeMap1, cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3, cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5, cubeMap5Smplr));
            }
        }
    }
    float reflectivity = material.reflectivity;
    float3 viewDir = fast::normalize(_163.cameraPosition - in.FragPos);
    float3 lighting = float3(0.0);
    float3 param_11 = N;
    float3 param_12 = V;
    float3 param_13 = albedo;
    float param_14 = metallic;
    float param_15 = roughness;
    float3 param_16 = F0;
    float param_17 = reflectivity;
    lighting += (calcAllDirectionalLights(param_11, param_12, param_13, param_14, param_15, param_16, param_17, _1073, _1083) * (1.0 - directionalShadow));
    float3 param_18 = in.FragPos;
    float3 param_19 = N;
    float3 param_20 = V;
    float3 param_21 = albedo;
    float param_22 = metallic;
    float param_23 = roughness;
    float3 param_24 = F0;
    float param_25 = reflectivity;
    lighting += (calcAllPointLights(param_18, param_19, param_20, param_21, param_22, param_23, param_24, param_25, _1073, _1136) * (1.0 - pointShadow));
    float3 param_26 = N;
    float3 param_27 = in.FragPos;
    float3 param_28 = V;
    float3 param_29 = viewDir;
    float3 param_30 = albedo;
    float param_31 = metallic;
    float param_32 = roughness;
    float3 param_33 = F0;
    float param_34 = reflectivity;
    lighting += calcAllSpotLights(param_26, param_27, param_28, param_29, param_30, param_31, param_32, param_33, param_34, _1073, _1268) * (1.0 - spotShadow);
    float3 param_35 = in.FragPos;
    float3 param_36 = N;
    float3 param_37 = V;
    float3 param_38 = F0;
    float3 param_39 = albedo;
    float param_40 = metallic;
    float param_41 = roughness;
    float3 _2039 = getRimLight(param_35, param_36, param_37, param_38, param_39, param_40, param_41, _163, environment);
    lighting += _2039;
    float3 areaResult = float3(0.0);
    float _2144;
    for (int i_2 = 0; i_2 < _1073.areaLightCount; i_2++)
    {
        float3 P = float3(_2058.areaLights[i_2].position);
        float3 R = fast::normalize(float3(_2058.areaLights[i_2].right));
        float3 U = fast::normalize(float3(_2058.areaLights[i_2].up));
        float2 halfSize = _2058.areaLights[i_2].size * 0.5;
        float3 toPoint = in.FragPos - P;
        float s = fast::clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = fast::clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        float3 Q = (P + (R * s)) + (U * t);
        float3 Lvec = Q - in.FragPos;
        float dist = length(Lvec);
        if (dist > 9.9999997473787516355514526367188e-05)
        {
            float3 L = Lvec / float3(dist);
            float3 Nl = fast::normalize(cross(R, U));
            float ndotl = dot(Nl, -L);
            if (_2058.areaLights[i_2].castsBothSides != 0)
            {
                _2144 = abs(ndotl);
            }
            else
            {
                _2144 = fast::max(ndotl, 0.0);
            }
            float facing = _2144;
            float cosTheta = cos(radians(_2058.areaLights[i_2].angle));
            if ((facing >= cosTheta) && (facing > 0.0))
            {
                float range = fast::max(_2058.areaLights[i_2].range, 0.001000000047497451305389404296875);
                float attenuation = 1.0 / ((1.0 + (dist / range)) + ((dist * dist) / (range * range)));
                float fade = 1.0 - smoothstep(range * 0.89999997615814208984375, range, dist);
                float3 radiance = (((float3(_2058.areaLights[i_2].diffuse) * fast::max(_2058.areaLights[i_2].intensity, 0.0)) * attenuation) * facing) * fade;
                float3 H = fast::normalize(V + L);
                float3 param_42 = N;
                float3 param_43 = H;
                float param_44 = roughness;
                float NDF = distributionGGX(param_42, param_43, param_44);
                float3 param_45 = N;
                float3 param_46 = V;
                float3 param_47 = L;
                float param_48 = roughness;
                float G = geometrySmith(param_45, param_46, param_47, param_48);
                float param_49 = fast::max(dot(H, V), 0.0);
                float3 param_50 = F0;
                float3 F = fresnelSchlick(param_49, param_50);
                float3 kS = F;
                float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
                float3 numerator = F * (NDF * G);
                float denominator = fast::max((4.0 * fast::max(dot(N, V), 0.0)) * fast::max(dot(N, L), 0.0), 9.9999997473787516355514526367188e-05);
                float3 specular = numerator / float3(denominator);
                float NdotL = fast::max(dot(N, L), 0.0);
                areaResult += (((((kD * albedo) / float3(3.1415927410125732421875)) + specular) * radiance) * NdotL);
            }
        }
    }
    lighting += areaResult * (1.0 - areaShadow);
    float aoClamped = fast::clamp(ao, 0.0, 1.0);
    float aoWithFloor = fast::max(aoClamped, 0.20000000298023223876953125);
    float ambientIntensity = ambientLight.intensity;
    float3 ambientColor = ambientLight.color.xyz;
    if (ambientIntensity <= 9.9999997473787516355514526367188e-05)
    {
        ambientIntensity = 0.02999999932944774627685546875;
    }
    if (dot(ambientColor, ambientColor) <= 9.9999999747524270787835121154785e-07)
    {
        ambientColor = float3(1.0);
    }
    float3 ambient = ((albedo * ambientIntensity) * ambientColor) * aoWithFloor;
    float3 iblContribution = float3(0.0);
    if (_1073.useIBL != 0u)
    {
        float3 param_51 = N;
        float3 irradiance = sampleEnvironmentRadiance(param_51, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_52 = reflection;
        float3 specularEnv = sampleEnvironmentRadiance(param_52, _163, texture1, texture1Smplr, texture2, texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr, texture6, texture6Smplr, texture7, texture7Smplr, texture8, texture8Smplr, texture9, texture9Smplr, texture10, texture10Smplr);
        float param_53 = fast::max(dot(N, V), 0.0);
        float3 param_54 = F0;
        float3 F_1 = fresnelSchlick(param_53, param_54);
        float3 kS_1 = F_1;
        float3 kD_1 = float3(1.0) - kS_1;
        kD_1 *= (1.0 - metallic);
        float roughnessAttenuation = mix(1.0, 0.1500000059604644775390625, fast::clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution = ((kD_1 * diffuseIBL) + (kS_1 * specularIBL)) * aoWithFloor;
    }
    float3 color = (ambient + lighting) + iblContribution;
    out.FragColor = float4(color, 1.0);
    float brightness = dot(color, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    if (brightness > 0.75)
    {
        out.BrightColor = float4(color, 1.0);
    }
    else
    {
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 param_55 = out.FragColor.xyz;
    float3 _2399 = acesToneMapping(param_55);
    out.FragColor.x = _2399.x;
    out.FragColor.y = _2399.y;
    out.FragColor.z = _2399.z;
    return out;
}
