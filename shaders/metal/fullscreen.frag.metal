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

// Implementation of the GLSL mod() function, which is slightly different than Metal fmod()
template<typename Tx, typename Ty>
inline Tx mod(Tx x, Ty y)
{
    return x - y * floor(x / y);
}

struct ColorCorrection
{
    float exposure;
    float contrast;
    float saturation;
    float gamma;
    float temperature;
    float tint;
};

struct PushConstants
{
    int hasBrightTexture;
    int hasDepthTexture;
    int hasVolumetricLightTexture;
    int hasPositionTexture;
    int hasLUTTexture;
    int hasSSRTexture;
    int TextureType;
    float lutSize;
    int EffectCount;
};

struct EffectBuffer
{
    int Effects[1];
};

struct EffectFloat1Buffer
{
    float EffectFloat1[1];
};

struct EffectFloat2Buffer
{
    float EffectFloat2[1];
};

struct EffectFloat3Buffer
{
    float EffectFloat3[1];
};

struct EffectFloat4Buffer
{
    float EffectFloat4[1];
};

struct EffectFloat5Buffer
{
    float EffectFloat5[1];
};

struct Uniforms
{
    float4x4 invProjectionMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 invViewMatrix;
    float4x4 lastViewMatrix;
    float nearPlane;
    float farPlane;
    float focusDepth;
    float focusRange;
    int maxMipLevel;
    float deltaTime;
    float time;
};

struct EffectFloat6Buffer
{
    float EffectFloat6[1];
};

struct Clouds
{
    float3 cloudPosition;
    float3 cloudSize;
    packed_float3 cameraPosition;
    float cloudScale;
    packed_float3 cloudOffset;
    float cloudDensityThreshold;
    float cloudDensityMultiplier;
    float cloudAbsorption;
    float cloudScattering;
    float cloudPhaseG;
    float cloudClusterStrength;
    int cloudPrimarySteps;
    int cloudLightSteps;
    float cloudLightStepMultiplier;
    float cloudMinStepLength;
    float3 sunDirection;
    packed_float3 sunColor;
    float sunIntensity;
    packed_float3 cloudAmbientColor;
    int hasClouds;
};

struct Environment
{
    packed_float3 fogColor;
    float fogIntensity;
};

constant spvUnsafeArray<float2, 9> _165 = spvUnsafeArray<float2, 9>({ float2(-0.0033333334140479564666748046875, 0.0033333334140479564666748046875), float2(0.0, 0.0033333334140479564666748046875), float2(0.0033333334140479564666748046875), float2(-0.0033333334140479564666748046875, 0.0), float2(0.0), float2(0.0033333334140479564666748046875, 0.0), float2(-0.0033333334140479564666748046875), float2(0.0, -0.0033333334140479564666748046875), float2(0.0033333334140479564666748046875, -0.0033333334140479564666748046875) });
constant spvUnsafeArray<float, 9> _171 = spvUnsafeArray<float, 9>({ -1.0, -1.0, -1.0, -1.0, 9.0, -1.0, -1.0, -1.0, -1.0 });
constant spvUnsafeArray<float, 9> _311 = spvUnsafeArray<float, 9>({ 1.0, 1.0, 1.0, 1.0, -8.0, 1.0, 1.0, 1.0, 1.0 });

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline))
float4 sampleColor(thread const float2& uv, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr)
{
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 7)
        {
            float redOffset = _394.EffectFloat1[i];
            float greenOffset = _403.EffectFloat2[i];
            float blueOffset = _411.EffectFloat3[i];
            float2 focusPoint = float2(_419.EffectFloat4[i], _426.EffectFloat5[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = Texture.sample(TextureSmplr, (sampleCoord + (direction * redOffset))).x;
            float green = Texture.sample(TextureSmplr, (sampleCoord + (direction * greenOffset))).y;
            float2 blue = Texture.sample(TextureSmplr, (sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (_381.Effects[i] == 9)
            {
                float pixelSizeInPixels = _394.EffectFloat1[i];
                float2 texSize = float2(int2(Texture.get_width(), Texture.get_height()));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                return Texture.sample(TextureSmplr, pixelated);
            }
            else
            {
                if (_381.Effects[i] == 10)
                {
                    float radius = _394.EffectFloat1[i];
                    float separation = _403.EffectFloat2[i];
                    float2 texelSize = float2(1.0) / float2(int2(Texture.get_width(), Texture.get_height()));
                    float3 maxColor = Texture.sample(TextureSmplr, uv).xyz;
                    int range = int(radius);
                    float radiusSq = radius * radius;
                    int _543 = -range;
                    for (int x = _543; x <= range; x++)
                    {
                        int _554 = -range;
                        for (int y = _554; y <= range; y++)
                        {
                            float distSq = float((x * x) + (y * y));
                            if (distSq <= radiusSq)
                            {
                                float2 offset = (float2(float(x), float(y)) * texelSize) * separation;
                                float3 sampled = Texture.sample(TextureSmplr, (uv + offset)).xyz;
                                maxColor = fast::max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, Texture.sample(TextureSmplr, uv).w);
                }
            }
        }
    }
    return Texture.sample(TextureSmplr, uv);
}

static inline __attribute__((always_inline))
float3 reconstructViewPos(thread const float2& uv, thread const float& depth, constant Uniforms& _849)
{
    float z = (depth * 2.0) - 1.0;
    float4 clipPos = float4((uv * 2.0) - float2(1.0), z, 1.0);
    float4 viewPos = _849.invProjectionMatrix * clipPos;
    viewPos /= float4(viewPos.w);
    return viewPos.xyz;
}

static inline __attribute__((always_inline))
float4 sharpen(texture2d<float> image, sampler imageSmplr, thread float2& TexCoord)
{
    spvUnsafeArray<float3, 9> sampleTex;
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.sample(imageSmplr, (TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _171[i_1]);
    }
    return float4(col, 1.0);
}

static inline __attribute__((always_inline))
float4 blur(texture2d<float> image, sampler imageSmplr, thread const float& radius, thread float2& TexCoord)
{
    float2 texelSize = float2(1.0) / float2(int2(image.get_width(), image.get_height()));
    float3 result = float3(0.0);
    float total = 0.0;
    float sigma = radius * 0.5;
    float twoSigmaSq = (2.0 * sigma) * sigma;
    int _257 = -int(radius);
    for (int x = _257; x <= int(radius); x++)
    {
        float weight = exp(float(-(x * x)) / twoSigmaSq);
        float2 offset = float2(float(x), 0.0) * texelSize;
        result += (image.sample(imageSmplr, (TexCoord + offset)).xyz * weight);
        total += weight;
    }
    result /= float3(total);
    return float4(result, 1.0);
}

static inline __attribute__((always_inline))
float4 edgeDetection(texture2d<float> image, sampler imageSmplr, thread float2& TexCoord)
{
    spvUnsafeArray<float3, 9> sampleTex;
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.sample(imageSmplr, (TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _311[i_1]);
    }
    return float4(col, 1.0);
}

static inline __attribute__((always_inline))
float4 applyFXAA(texture2d<float> tex, sampler texSmplr, thread const float2& texCoord, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr)
{
    float2 texelSize = float2(1.0) / float2(int2(tex.get_width(), tex.get_height()));
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 0.125;
    float FXAA_REDUCE_MIN = 0.0078125;
    float2 param = texCoord + (float2(-1.0) * texelSize);
    float3 rgbNW = sampleColor(param, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_1 = texCoord + (float2(1.0, -1.0) * texelSize);
    float3 rgbNE = sampleColor(param_1, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_2 = texCoord + (float2(-1.0, 1.0) * texelSize);
    float3 rgbSW = sampleColor(param_2, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_3 = texCoord + (float2(1.0) * texelSize);
    float3 rgbSE = sampleColor(param_3, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float2 param_4 = texCoord;
    float3 rgbM = sampleColor(param_4, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz;
    float3 luma = float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);
    float lumaMin = fast::min(lumaM, fast::min(fast::min(lumaNW, lumaNE), fast::min(lumaSW, lumaSE)));
    float lumaMax = fast::max(lumaM, fast::max(fast::max(lumaNW, lumaNE), fast::max(lumaSW, lumaSE)));
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);
    float dirReduce = fast::max((((lumaNW + lumaNE) + lumaSW) + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (fast::min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = fast::min(float2(FXAA_SPAN_MAX), fast::max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;
    float2 param_5 = texCoord + (dir * (-0.16666667163372039794921875));
    float2 param_6 = texCoord + (dir * 0.16666667163372039794921875);
    float3 rgbA = (sampleColor(param_5, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz + sampleColor(param_6, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz) * 0.5;
    float2 param_7 = texCoord + (dir * (-0.5));
    float2 param_8 = texCoord + (dir * 0.5);
    float3 rgbB = (rgbA * 0.5) + ((sampleColor(param_7, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz + sampleColor(param_8, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr).xyz) * 0.25);
    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return float4(rgbA, 1.0);
    }
    else
    {
        return float4(rgbB, 1.0);
    }
}

static inline __attribute__((always_inline))
float4 applyColorCorrection(thread const float4& color, thread const ColorCorrection& cc)
{
    float3 linearColor = color.xyz;
    linearColor *= powr(2.0, cc.exposure);
    linearColor = ((linearColor - float3(0.5)) * cc.contrast) + float3(0.5);
    linearColor.x += (cc.temperature * 0.0500000007450580596923828125);
    linearColor.y += (cc.tint * 0.0500000007450580596923828125);
    float luminance = dot(linearColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    linearColor = mix(float3(luminance), linearColor, float3(cc.saturation));
    linearColor = fast::clamp(linearColor, float3(0.0), float3(1.0));
    return float4(linearColor, color.w);
}

static inline __attribute__((always_inline))
float4 applyColorEffects(thread float4& color, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, constant Uniforms& _849, device EffectFloat6Buffer& _1049, thread float4& gl_FragCoord)
{
    ColorCorrection cc;
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 0)
        {
            color = float4(float3(1.0) - color.xyz, color.w);
        }
        else
        {
            if (_381.Effects[i] == 1)
            {
                float average = ((0.2125999927520751953125 * color.x) + (0.715200006961822509765625 * color.y)) + (0.072200000286102294921875 * color.z);
                color = float4(average, average, average, color.w);
            }
            else
            {
                if (_381.Effects[i] == 5)
                {
                    cc.exposure = _394.EffectFloat1[i];
                    cc.contrast = _403.EffectFloat2[i];
                    cc.saturation = _411.EffectFloat3[i];
                    cc.gamma = _419.EffectFloat4[i];
                    cc.temperature = _426.EffectFloat5[i];
                    cc.tint = _1049.EffectFloat6[i];
                    float4 param = color;
                    ColorCorrection param_1 = cc;
                    color = applyColorCorrection(param, param_1);
                }
                else
                {
                    if (_381.Effects[i] == 8)
                    {
                        float levels = fast::max(_394.EffectFloat1[i], 1.0);
                        float grayscale = fast::max(color.x, fast::max(color.y, color.z));
                        if (grayscale > 9.9999997473787516355514526367188e-05)
                        {
                            float lower = floor(grayscale * levels) / levels;
                            float lowerDiff = abs(grayscale - lower);
                            float upper = ceil(grayscale * levels) / levels;
                            float upperDiff = abs(upper - grayscale);
                            float level0 = (lowerDiff <= upperDiff) ? lower : upper;
                            float adjustment = level0 / fast::max(grayscale, 9.9999997473787516355514526367188e-05);
                            color *= adjustment;
                        }
                    }
                    else
                    {
                        if (_381.Effects[i] == 11)
                        {
                            float amount = _394.EffectFloat1[i];
                            float3 seed = float3(gl_FragCoord.xy, _849.deltaTime * 100.0);
                            float n = dot(seed, float3(12.98980045318603515625, 78.233001708984375, 45.16400146484375));
                            float noise = fract(sin(n) * 43758.546875);
                            float grain = ((noise - 0.5) * 2.0) * amount;
                            float luminance = dot(color.xyz, float3(0.2989999949932098388671875, 0.58700001239776611328125, 0.114000000059604644775390625));
                            float visibility = 1.0 - (abs(luminance - 0.5) * 0.5);
                            float4 _1210 = color;
                            float3 _1212 = _1210.xyz + float3(grain * visibility);
                            color.x = _1212.x;
                            color.y = _1212.y;
                            color.z = _1212.z;
                            float4 _1219 = color;
                            float3 _1223 = fast::clamp(_1219.xyz, float3(0.0), float3(1.0));
                            color.x = _1223.x;
                            color.y = _1223.y;
                            color.z = _1223.z;
                        }
                    }
                }
            }
        }
    }
    return color;
}

static inline __attribute__((always_inline))
float LinearizeDepth(thread const float& depth, constant Uniforms& _849)
{
    float z = (depth * 2.0) - 1.0;
    float linear = ((2.0 * _849.nearPlane) * _849.farPlane) / ((_849.farPlane + _849.nearPlane) - (z * (_849.farPlane - _849.nearPlane)));
    return linear / _849.farPlane;
}

static inline __attribute__((always_inline))
float2 rayBoxDst(thread const float3& boundsMin, thread const float3& boundsMax, thread const float3& rayOrigin, thread const float3& rayDir)
{
    float3 t0 = (boundsMin - rayOrigin) / rayDir;
    float3 t1 = (boundsMax - rayOrigin) / rayDir;
    float3 tMin = fast::min(t0, t1);
    float3 tMax = fast::max(t0, t1);
    float dstA = fast::max(fast::max(tMin.x, tMin.y), tMin.z);
    float dstB = fast::min(tMax.x, fast::min(tMax.y, tMax.z));
    float dstToContainer = fast::max(0.0, dstA);
    float dstInsideContainer = fast::max(0.0, dstB - dstToContainer);
    return float2(dstToContainer, dstInsideContainer);
}

static inline __attribute__((always_inline))
float hashNoise(thread const float3& p)
{
    return fract(sin(dot(p, float3(12.98980045318603515625, 78.233001708984375, 37.71900177001953125))) * 43758.546875);
}

static inline __attribute__((always_inline))
float saturate0(thread const float& v)
{
    return fast::clamp(v, 0.0, 1.0);
}

static inline __attribute__((always_inline))
float calculateCloudDensity(thread const float3& worldPos, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr)
{
    float3 halfExtents = fast::max(_1929.cloudSize * 0.5, float3(9.9999997473787516355514526367188e-05));
    float3 localPos = (worldPos - _1929.cloudPosition) / halfExtents;
    bool _1947 = any(localPos < float3(-1.0));
    bool _1955;
    if (!_1947)
    {
        _1955 = any(localPos > float3(1.0));
    }
    else
    {
        _1955 = _1947;
    }
    if (_1955)
    {
        return 0.0;
    }
    float3 uvw = (localPos * 0.5) + float3(0.5);
    float scale = fast::max(_1929.cloudScale, 0.001000000047497451305389404296875);
    float3 noiseCoord = fract((uvw * scale) + float3(_1929.cloudOffset));
    float4 shape = cloudsTexture.sample(cloudsTextureSmplr, noiseCoord);
    float param = _1929.cloudClusterStrength;
    float cluster = saturate0(param);
    float lowerFade = smoothstep(-0.949999988079071044921875, -0.60000002384185791015625, localPos.y);
    float upperFade = 1.0 - smoothstep(0.3499999940395355224609375, 0.949999988079071044921875, localPos.y);
    float verticalMask = lowerFade * upperFade;
    float coverageThreshold = mix(0.60000002384185791015625, 0.2800000011920928955078125, cluster);
    float coverageSoftness = mix(0.2199999988079071044921875, 0.3400000035762786865234375, cluster);
    float coverageNoise = mix(shape.x, shape.w, 0.4000000059604644775390625 + (cluster * 0.3499999940395355224609375));
    float coverage = smoothstep(coverageThreshold, coverageThreshold + coverageSoftness, coverageNoise);
    coverage = powr(coverage, mix(2.0, 0.699999988079071044921875, cluster));
    float detail = mix(smoothstep(0.25, 0.75, shape.y), smoothstep(0.20000000298023223876953125, 0.89999997615814208984375, shape.w), 0.550000011920928955078125);
    detail = powr(detail, mix(1.60000002384185791015625, 0.85000002384185791015625, cluster));
    float cavityNoise = smoothstep(0.2199999988079071044921875, 0.85000002384185791015625, shape.z);
    float gapMask = fast::clamp(1.0 - (cavityNoise * mix(0.25, 0.699999988079071044921875, cluster)), 0.0, 1.0);
    float density = coverage * mix(detail, 1.0, cluster * 0.3499999940395355224609375);
    density = fast::max(((density * gapMask) * verticalMask) - 0.0199999995529651641845703125, 0.0);
    density *= fast::max(_1929.cloudDensityMultiplier, 0.0);
    return density;
}

static inline __attribute__((always_inline))
float sampleSunTransmittance(thread const float3& worldPos, thread const float& stepSize, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr)
{
    float3 lightDir = -_1929.sunDirection;
    float dirLength = length(lightDir);
    if (dirLength < 0.001000000047497451305389404296875)
    {
        lightDir = float3(0.0, 1.0, 0.0);
    }
    else
    {
        lightDir /= float3(dirLength);
    }
    float maxDistance = length(_1929.cloudSize) * 1.5;
    float travel = 0.0;
    float attenuation = 1.0;
    float lightStep = fast::max(stepSize * _1929.cloudLightStepMultiplier, _1929.cloudMinStepLength);
    int steps = max(_1929.cloudLightSteps, 1);
    for (int i = 0; (i < steps) && (attenuation > 0.0500000007450580596923828125); i++)
    {
        travel += lightStep;
        if (travel > maxDistance)
        {
            break;
        }
        float3 samplePos = worldPos + (lightDir * travel);
        float3 param = samplePos;
        float density = calculateCloudDensity(param, _1929, cloudsTexture, cloudsTextureSmplr);
        attenuation *= exp(((-density) * lightStep) * _1929.cloudAbsorption);
        lightStep = fast::max(lightStep * _1929.cloudLightStepMultiplier, _1929.cloudMinStepLength);
    }
    return attenuation;
}

static inline __attribute__((always_inline))
float henyeyGreenstein(thread const float& cosTheta, thread const float& g)
{
    float g2 = g * g;
    float denom = powr((1.0 + g2) - ((2.0 * g) * cosTheta), 1.5);
    return (1.0 - g2) / (12.56637096405029296875 * fast::max(denom, 9.9999997473787516355514526367188e-05));
}

static inline __attribute__((always_inline))
float4 cloudRendering(thread const float4& inColor, thread float2& TexCoord, constant PushConstants& _372, constant Uniforms& _849, constant Clouds& _1929, texture3d<float> cloudsTexture, sampler cloudsTextureSmplr, texture2d<float> DepthTexture, sampler DepthTextureSmplr)
{
    if (_1929.hasClouds != 1)
    {
        return inColor;
    }
    float _2197;
    if (_372.hasDepthTexture == 1)
    {
        _2197 = DepthTexture.sample(DepthTextureSmplr, TexCoord).x;
    }
    else
    {
        _2197 = 1.0;
    }
    float nonLinearDepth = _2197;
    bool depthAvailable = (_372.hasDepthTexture == 1) && (nonLinearDepth < 1.0);
    float depthSample = depthAvailable ? nonLinearDepth : 1.0;
    float3 rayOrigin = float3(_1929.cameraPosition);
    float4 clipSpace = float4((TexCoord * 2.0) - float2(1.0), (depthSample * 2.0) - 1.0, 1.0);
    float4 viewSpace = _849.invProjectionMatrix * clipSpace;
    viewSpace /= float4(viewSpace.w);
    float3 worldPos = (_849.invViewMatrix * float4(viewSpace.xyz, 1.0)).xyz;
    float3 rayDir = fast::normalize(worldPos - rayOrigin);
    float _2261;
    if (depthAvailable)
    {
        _2261 = length(worldPos - rayOrigin);
    }
    else
    {
        _2261 = 1000000.0;
    }
    float sceneDistance = _2261;
    float3 boundsMin = _1929.cloudPosition - (_1929.cloudSize * 0.5);
    float3 boundsMax = _1929.cloudPosition + (_1929.cloudSize * 0.5);
    float3 param = boundsMin;
    float3 param_1 = boundsMax;
    float3 param_2 = rayOrigin;
    float3 param_3 = rayDir;
    float2 rayBoxInfo = rayBoxDst(param, param_1, param_2, param_3);
    float distToContainer = rayBoxInfo.x;
    float distInContainer = rayBoxInfo.y;
    if (distInContainer <= 0.0)
    {
        return inColor;
    }
    float dstLimit = fast::min(sceneDistance - distToContainer, distInContainer);
    dstLimit = fast::max(dstLimit, 0.0);
    if (dstLimit <= 9.9999997473787516355514526367188e-05)
    {
        return inColor;
    }
    int steps = max(_1929.cloudPrimarySteps, 8);
    float baseStep = dstLimit / float(steps);
    float stepSize = fast::max(baseStep, _1929.cloudMinStepLength);
    float3 param_4 = float3(TexCoord, _849.time);
    float jitter = hashNoise(param_4) - 0.5;
    float travelled = fast::clamp(jitter, -0.3499999940395355224609375, 0.3499999940395355224609375) * stepSize;
    travelled = fast::max(travelled, 0.0);
    float3 accumulatedLight = float3(0.0);
    float transmittance = 1.0;
    float3 sunDir = _1929.sunDirection;
    float sunLen = length(sunDir);
    if (sunLen > 0.001000000047497451305389404296875)
    {
        sunDir /= float3(sunLen);
    }
    else
    {
        sunDir = float3(0.0, 1.0, 0.0);
    }
    float phaseG = fast::clamp(_1929.cloudPhaseG, -0.949999988079071044921875, 0.949999988079071044921875);
    for (int _step = 0; (_step < steps) && (travelled < dstLimit); _step++)
    {
        if (transmittance <= 0.00999999977648258209228515625)
        {
            break;
        }
        float remainingDistance = dstLimit - travelled;
        if (remainingDistance <= 9.9999997473787516355514526367188e-06)
        {
            break;
        }
        float current = distToContainer + travelled;
        float3 samplePos = rayOrigin + (rayDir * current);
        float3 param_5 = samplePos;
        float density = calculateCloudDensity(param_5, _1929, cloudsTexture, cloudsTextureSmplr);
        if (density > 9.9999997473787516355514526367188e-05)
        {
            float adaptiveStep = stepSize;
            if (density < 0.0199999995529651641845703125)
            {
                adaptiveStep = stepSize * 2.5;
            }
            else
            {
                if (density < 0.0500000007450580596923828125)
                {
                    adaptiveStep = stepSize * 1.60000002384185791015625;
                }
            }
            adaptiveStep = fast::min(adaptiveStep, remainingDistance);
            float sampleWeight = density * adaptiveStep;
            float3 param_6 = samplePos;
            float param_7 = adaptiveStep;
            float lightTrans = sampleSunTransmittance(param_6, param_7, _1929, cloudsTexture, cloudsTextureSmplr);
            float cosTheta = fast::clamp(dot(rayDir, -sunDir), -1.0, 1.0);
            float param_8 = cosTheta;
            float param_9 = phaseG;
            float phase = henyeyGreenstein(param_8, param_9);
            float3 directLight = ((float3(_1929.sunColor) * _1929.sunIntensity) * lightTrans) * phase;
            float3 ambientLight = float3(_1929.cloudAmbientColor);
            float3 lighting = (((ambientLight * 0.3499999940395355224609375) + directLight) * sampleWeight) * _1929.cloudScattering;
            accumulatedLight += (lighting * transmittance);
            transmittance *= exp(((-density) * adaptiveStep) * _1929.cloudAbsorption);
            travelled += adaptiveStep;
            continue;
        }
        float emptyAdvance = fast::min(stepSize * 2.25, remainingDistance);
        float minAdvance = fast::min(stepSize * 0.5, remainingDistance);
        travelled += fast::max(emptyAdvance, minAdvance);
    }
    float3 finalColor = (inColor.xyz * transmittance) + accumulatedLight;
    return float4(fast::clamp(finalColor, float3(0.0), float3(1.0)), inColor.w);
}

static inline __attribute__((always_inline))
float4 sampleBright(thread const float2& uv, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> BrightTexture, sampler BrightTextureSmplr)
{
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 7)
        {
            float redOffset = _394.EffectFloat1[i];
            float greenOffset = _403.EffectFloat2[i];
            float blueOffset = _411.EffectFloat3[i];
            float2 focusPoint = float2(_419.EffectFloat4[i], _426.EffectFloat5[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * redOffset))).x;
            float green = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * greenOffset))).y;
            float2 blue = BrightTexture.sample(BrightTextureSmplr, (sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (_381.Effects[i] == 9)
            {
                float pixelSizeInPixels = _394.EffectFloat1[i];
                float2 texSize = float2(int2(BrightTexture.get_width(), BrightTexture.get_height()));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                float4 color = BrightTexture.sample(BrightTextureSmplr, pixelated);
                return color;
            }
            else
            {
                if (_381.Effects[i] == 10)
                {
                    float radius = _394.EffectFloat1[i];
                    float separation = _403.EffectFloat2[i];
                    float2 texelSize = float2(1.0) / float2(int2(BrightTexture.get_width(), BrightTexture.get_height()));
                    float3 maxColor = BrightTexture.sample(BrightTextureSmplr, uv).xyz;
                    int range = int(radius);
                    float radiusSq = radius * radius;
                    int _766 = -range;
                    for (int x = _766; x <= range; x++)
                    {
                        int _777 = -range;
                        for (int y = _777; y <= range; y++)
                        {
                            float distSq = float((x * x) + (y * y));
                            if (distSq <= radiusSq)
                            {
                                float2 offset = (float2(float(x), float(y)) * texelSize) * separation;
                                float3 sampled = BrightTexture.sample(BrightTextureSmplr, (uv + offset)).xyz;
                                maxColor = fast::max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, BrightTexture.sample(BrightTextureSmplr, uv).w);
                }
            }
        }
    }
    return BrightTexture.sample(BrightTextureSmplr, uv);
}

static inline __attribute__((always_inline))
float4 composeSSR(thread const float4& color, thread const float2& uv, constant PushConstants& _372, texture2d<float> SSRTexture, sampler SSRTextureSmplr)
{
    if (_372.hasSSRTexture != 1)
    {
        return color;
    }
    float4 ssr = SSRTexture.sample(SSRTextureSmplr, uv);
    float reflectionWeight = fast::clamp(ssr.w, 0.0, 1.0);
    float baseWeight = 1.0 - reflectionWeight;
    float4 result = color;
    result.xyz = (result.xyz * baseWeight) + (ssr.xyz * reflectionWeight);
    return result;
}

static inline __attribute__((always_inline))
float4 composeLighting(thread const float2& uv, thread const float4& baseColor, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> BrightTexture, sampler BrightTextureSmplr, texture2d<float> VolumetricLightTexture, sampler VolumetricLightTextureSmplr, texture2d<float> SSRTexture, sampler SSRTextureSmplr)
{
    float4 color = baseColor;
    if (_372.hasBrightTexture == 1)
    {
        color += sampleBright(uv, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr);
    }
    if (_372.hasVolumetricLightTexture == 1)
    {
        color += VolumetricLightTexture.sample(VolumetricLightTextureSmplr, uv);
    }
    return composeSSR(color, uv, _372, SSRTexture, SSRTextureSmplr);
}

static inline __attribute__((always_inline))
float4 applyMotionBlur(thread const float2& texCoord, thread const float& size, thread const float& separation, thread const float4& color, constant PushConstants& _372, device EffectBuffer& _381, device EffectFloat1Buffer& _394, device EffectFloat2Buffer& _403, device EffectFloat3Buffer& _411, device EffectFloat4Buffer& _419, device EffectFloat5Buffer& _426, texture2d<float> Texture, sampler TextureSmplr, texture2d<float> BrightTexture, sampler BrightTextureSmplr, constant Uniforms& _849, texture2d<float> VolumetricLightTexture, sampler VolumetricLightTextureSmplr, texture2d<float> SSRTexture, sampler SSRTextureSmplr, texture2d<float> PositionTexture, sampler PositionTextureSmplr, texture2d<float> DepthTexture, sampler DepthTextureSmplr)
{
    float4 fallbackColor = composeLighting(texCoord, color, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
    if ((size <= 0.0) || (separation <= 0.0))
    {
        return fallbackColor;
    }
    if (_372.hasPositionTexture != 1)
    {
        return fallbackColor;
    }
    float4 worldPos = PositionTexture.sample(PositionTextureSmplr, texCoord);
    if (worldPos.w <= 0.0)
    {
        return fallbackColor;
    }
    float3 viewSpacePos = (_849.viewMatrix * worldPos).xyz;
    float distanceToCamera = length(viewSpacePos);
    if (distanceToCamera < (_849.nearPlane * 2.0))
    {
        return fallbackColor;
    }
    float4 currentClipPos = (_849.projectionMatrix * _849.viewMatrix) * worldPos;
    float _1543 = currentClipPos.w;
    float4 _1544 = currentClipPos;
    float3 _1547 = _1544.xyz / float3(_1543);
    currentClipPos.x = _1547.x;
    currentClipPos.y = _1547.y;
    currentClipPos.z = _1547.z;
    float2 currentUV = (currentClipPos.xy * 0.5) + float2(0.5);
    float4 prevClipPos = (_849.projectionMatrix * _849.lastViewMatrix) * worldPos;
    float _1569 = prevClipPos.w;
    float4 _1570 = prevClipPos;
    float3 _1573 = _1570.xyz / float3(_1569);
    prevClipPos.x = _1573.x;
    prevClipPos.y = _1573.y;
    prevClipPos.z = _1573.z;
    float2 prevUV = (prevClipPos.xy * 0.5) + float2(0.5);
    float2 velocity = (currentUV - prevUV) * separation;
    float velocityLength = length(velocity);
    float maxVelocity = 0.119999997317790985107421875;
    if (velocityLength > maxVelocity)
    {
        velocity = fast::normalize(velocity) * maxVelocity;
        velocityLength = maxVelocity;
    }
    if (_372.hasSSRTexture == 1)
    {
        float mirrorWeight = fast::clamp(SSRTexture.sample(SSRTextureSmplr, texCoord).w, 0.0, 1.0);
        float blurDamp = mix(1.0, 0.20000000298023223876953125, mirrorWeight);
        velocity *= blurDamp;
        velocityLength *= blurDamp;
    }
    if (velocityLength < 9.9999997473787516355514526367188e-05)
    {
        return fallbackColor;
    }
    int baseSamples = clamp(int(size), 2, 32);
    float sampleScale = mix(0.75, 1.75, fast::clamp(velocityLength / maxVelocity, 0.0, 1.0));
    int samples = clamp(int(float(baseSamples) * sampleScale), 3, 48);
    float centerDepth = 0.0;
    float depthSoftness = 1.0;
    if (_372.hasDepthTexture == 1)
    {
        float centerDepthRaw = DepthTexture.sample(DepthTextureSmplr, texCoord).x;
        centerDepth = LinearizeDepth(centerDepthRaw, _849);
        depthSoftness = fast::max(0.00200000009499490261077880859375, centerDepth * 0.0199999995529651641845703125);
    }
    float jitter = (fract(sin(dot(texCoord * float2(173.3000030517578125, 97.09999847412109375), float2(12.98980045318603515625, 78.233001708984375))) * 43758.546875) - 0.5) * 0.3499999940395355224609375;
    float4 result = float4(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float t = (((float(i) + 0.5) / float(samples)) * 2.0) - 1.0;
        t += jitter / float(samples);
        float2 sampleCoord = texCoord + (velocity * t);
        bool _1642 = sampleCoord.x < 0.0;
        bool _1648;
        if (!_1642)
        {
            _1648 = sampleCoord.x > 1.0;
        }
        else
        {
            _1648 = _1642;
        }
        bool _1654;
        if (!_1648)
        {
            _1654 = sampleCoord.y < 0.0;
        }
        else
        {
            _1654 = _1648;
        }
        bool _1660;
        if (!_1654)
        {
            _1660 = sampleCoord.y > 1.0;
        }
        else
        {
            _1660 = _1654;
        }
        if (_1660)
        {
            continue;
        }
        float depthWeight = 1.0;
        if (_372.hasDepthTexture == 1)
        {
            float sampleDepthRaw = DepthTexture.sample(DepthTextureSmplr, sampleCoord).x;
            float sampleDepth = LinearizeDepth(sampleDepthRaw, _849);
            float depthDelta = abs(sampleDepth - centerDepth);
            depthWeight = 1.0 - smoothstep(depthSoftness, depthSoftness * 2.5, depthDelta);
            if (depthWeight <= 0.0)
            {
                continue;
            }
        }
        float4 sampled = sampleColor(sampleCoord, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
        sampled = composeLighting(sampleCoord, sampled, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
        float gaussian = exp((-4.0) * t * t);
        float weight = gaussian * depthWeight;
        result += (sampled * weight);
        totalWeight += weight;
    }
    if (totalWeight > 0.0)
    {
        return result / float4(totalWeight);
    }
    return fallbackColor;
}

static inline __attribute__((always_inline))
float3 sampleLUT(thread const float3& rgb, thread const float& blueSlice, thread const float& sliceSize, thread const float& slicePixelOffset, constant PushConstants& _372, texture2d<float> LUTTexture, sampler LUTTextureSmplr)
{
    float sliceY = floor(blueSlice / _372.lutSize);
    float sliceX = mod(blueSlice, _372.lutSize);
    float2 uv;
    uv.x = ((sliceX * sliceSize) + slicePixelOffset) + (rgb.x * sliceSize);
    uv.y = ((sliceY * sliceSize) + slicePixelOffset) + (rgb.y * sliceSize);
    return LUTTexture.sample(LUTTextureSmplr, uv).xyz;
}

static inline __attribute__((always_inline))
float4 mapToLUT(thread const float4& color, constant PushConstants& _372, texture2d<float> LUTTexture, sampler LUTTextureSmplr)
{
    if (_372.hasLUTTexture != 1)
    {
        return color;
    }
    float sliceSize = 1.0 / _372.lutSize;
    float slicePixelOffset = sliceSize * 0.5;
    float blueIndex = color.z * (_372.lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = fast::min(sliceLow + 1.0, _372.lutSize - 1.0);
    float t = blueIndex - sliceLow;
    float3 param = color.xyz;
    float param_1 = sliceLow;
    float param_2 = sliceSize;
    float param_3 = slicePixelOffset;
    float3 lowColor = sampleLUT(param, param_1, param_2, param_3, _372, LUTTexture, LUTTextureSmplr);
    float3 param_4 = color.xyz;
    float param_5 = sliceHigh;
    float param_6 = sliceSize;
    float param_7 = slicePixelOffset;
    float3 highColor = sampleLUT(param_4, param_5, param_6, param_7, _372, LUTTexture, LUTTextureSmplr);
    float3 finalRGB = mix(lowColor, highColor, float3(t));
    return float4(finalRGB, color.w);
}

static inline __attribute__((always_inline))
float3 acesToneMapping(thread const float3& color)
{
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    return fast::clamp((color * ((color * a) + float3(b))) / ((color * ((color * c) + float3(d))) + float3(e)), float3(0.0), float3(1.0));
}

fragment main0_out main0(main0_in in [[stage_in]], constant PushConstants& _372 [[buffer(0)]], device EffectBuffer& _381 [[buffer(1)]], device EffectFloat1Buffer& _394 [[buffer(2)]], device EffectFloat2Buffer& _403 [[buffer(3)]], device EffectFloat3Buffer& _411 [[buffer(4)]], device EffectFloat4Buffer& _419 [[buffer(5)]], device EffectFloat5Buffer& _426 [[buffer(6)]], constant Uniforms& _849 [[buffer(7)]], device EffectFloat6Buffer& _1049 [[buffer(8)]], constant Clouds& _1929 [[buffer(9)]], constant Environment& environment [[buffer(10)]], texture2d<float> Texture [[texture(0)]], texture2d<float> BrightTexture [[texture(1)]], texture2d<float> VolumetricLightTexture [[texture(2)]], texture2d<float> SSRTexture [[texture(3)]], texture2d<float> PositionTexture [[texture(4)]], texture2d<float> LUTTexture [[texture(5)]], texture3d<float> cloudsTexture [[texture(6)]], texture2d<float> DepthTexture [[texture(7)]], sampler TextureSmplr [[sampler(0)]], sampler BrightTextureSmplr [[sampler(1)]], sampler VolumetricLightTextureSmplr [[sampler(2)]], sampler SSRTextureSmplr [[sampler(3)]], sampler PositionTextureSmplr [[sampler(4)]], sampler LUTTextureSmplr [[sampler(5)]], sampler cloudsTextureSmplr [[sampler(6)]], sampler DepthTextureSmplr [[sampler(7)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float2 param = in.TexCoord;
    float4 color = sampleColor(param, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
    float depth = 1.0;
    if (_372.hasDepthTexture == 1)
    {
        depth = DepthTexture.sample(DepthTextureSmplr, in.TexCoord).x;
    }
    float2 param_1 = in.TexCoord;
    float param_2 = depth;
    float3 viewPos = reconstructViewPos(param_1, param_2, _849);
    float _distance = _372.hasDepthTexture == 1 ? length(viewPos) : _849.farPlane;
    bool useMotionBlur = false;
    float motionBlurSize = 0.0;
    float motionBlurSeparation = 0.0;
    for (int i = 0; i < _372.EffectCount; i++)
    {
        if (_381.Effects[i] == 6)
        {
            useMotionBlur = true;
            motionBlurSize = _394.EffectFloat1[i];
            motionBlurSeparation = _403.EffectFloat2[i];
        }
    }
    for (int i_1 = 0; i_1 < _372.EffectCount; i_1++)
    {
        if (_381.Effects[i_1] == 2)
        {
            color = sharpen(Texture, TextureSmplr, in.TexCoord);
        }
        else
        {
            if (_381.Effects[i_1] == 3)
            {
                float radius = _394.EffectFloat1[i_1];
                float param_3 = radius;
                color = blur(Texture, TextureSmplr, param_3, in.TexCoord);
            }
            else
            {
                if (_381.Effects[i_1] == 4)
                {
                    color = edgeDetection(Texture, TextureSmplr, in.TexCoord);
                }
            }
        }
    }
    float2 param_4 = in.TexCoord;
    color = applyFXAA(Texture, TextureSmplr, param_4, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr);
    float4 param_5 = color;
    float4 _2637 = applyColorEffects(param_5, _372, _381, _394, _403, _411, _419, _426, _849, _1049, gl_FragCoord);
    color = _2637;
    if (_372.hasDepthTexture == 1)
    {
        float depthValue = DepthTexture.sample(DepthTextureSmplr, in.TexCoord).x;
        float param_6 = depthValue;
        float linearDepth = LinearizeDepth(param_6, _849);
        float coc = fast::clamp(abs(linearDepth - _849.focusDepth) / _849.focusRange, 0.0, 1.0);
        float mip = (coc * float(_849.maxMipLevel)) * 1.2000000476837158203125;
        float4 param_7 = Texture.sample(TextureSmplr, in.TexCoord, level(mip));
        float4 _2676 = applyColorEffects(param_7, _372, _381, _394, _403, _411, _419, _426, _849, _1049, gl_FragCoord);
        float3 blurred = _2676.xyz;
        float3 sharp = color.xyz;
        float4 param_8 = color;
        color = cloudRendering(param_8, in.TexCoord, _372, _849, _1929, cloudsTexture, cloudsTextureSmplr, DepthTexture, DepthTextureSmplr);
    }
    else
    {
        float4 param_9 = color;
        color = cloudRendering(param_9, in.TexCoord, _372, _849, _1929, cloudsTexture, cloudsTextureSmplr, DepthTexture, DepthTextureSmplr);
    }
    float4 hdrColor;
    if (useMotionBlur)
    {
        float2 param_10 = in.TexCoord;
        float param_11 = motionBlurSize;
        float param_12 = motionBlurSeparation;
        float4 param_13 = color;
        float4 motionBlurred = applyMotionBlur(param_10, param_11, param_12, param_13, _372, _381, _394, _403, _411, _419, _426, Texture, TextureSmplr, BrightTexture, BrightTextureSmplr, _849, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr, PositionTexture, PositionTextureSmplr, DepthTexture, DepthTextureSmplr);
        out.FragColor = motionBlurred;
        return out;
    }
    else
    {
        float2 param_14 = in.TexCoord;
        float4 param_15 = color;
        hdrColor = composeLighting(param_14, param_15, _372, _381, _394, _403, _411, _419, _426, BrightTexture, BrightTextureSmplr, VolumetricLightTexture, VolumetricLightTextureSmplr, SSRTexture, SSRTextureSmplr);
    }
    float4 param_16 = hdrColor;
    hdrColor = mapToLUT(param_16, _372, LUTTexture, LUTTextureSmplr);
    float3 param_17 = hdrColor.xyz;
    float3 _2744 = acesToneMapping(param_17);
    hdrColor.x = _2744.x;
    hdrColor.y = _2744.y;
    hdrColor.z = _2744.z;
    float fogFactor = 1.0 - exp((-_distance) * environment.fogIntensity);
    float3 finalColor = mix(hdrColor.xyz, float3(environment.fogColor), float3(fogFactor));
    out.FragColor = float4(finalColor, 1.0);
    return out;
}
