#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    float4x4 inverseProjection;
    float4x4 inverseView;
    float4x4 projection;
    float4x4 view;
    float3 cameraPosition;
};

struct SSRParameters
{
    float maxDistance;
    float resolution;
    int steps;
    float thickness;
    float maxRoughness;
    float historyWeight;
    int debugMode;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

struct main0_in
{
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline))
float3 fresnelSchlick(thread const float& cosTheta, thread const float3& F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline))
float3 sampleSkyReflection(thread const float3& normal, thread const float3& viewDir, thread const float3& albedo, thread const float& metallic, texturecube<float> skybox, sampler skyboxSmplr)
{
    float3 reflected = reflect(-fast::normalize(viewDir), fast::normalize(normal));
    float3 skyColor = skybox.sample(skyboxSmplr, reflected).xyz;
    float skyLuma = dot(skyColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    if (skyLuma < 0.001000000047497451305389404296875)
    {
        float t = fast::clamp((reflected.y * 0.5) + 0.5, 0.0, 1.0);
        float3 horizon = float3(0.7400000095367431640625, 0.790000021457672119140625, 0.87999999523162841796875);
        float3 zenith = float3(0.5, 0.63999998569488525390625, 0.839999973773956298828125);
        skyColor = mix(horizon, zenith, float3(t));
    }
    float tintStrength = metallic * 0.3499999940395355224609375;
    float3 tint = mix(float3(1.0), albedo, float3(tintStrength));
    return skyColor * tint;
}

static inline __attribute__((always_inline))
float4 SSR(thread const float3& worldPos, thread const float3& normal, thread const float3& viewDir, thread const float& roughness, thread const float& metallic, thread const float& reflectivity, thread const float3& albedo, constant Uniforms& _42, constant SSRParameters& _96, thread float4& gl_FragCoord, texture2d<float> gPosition, sampler gPositionSmplr, texture2d<float> sceneColor, sampler sceneColorSmplr, texture2d<float> gNormal, sampler gNormalSmplr, texture2d<float> gDepth, sampler gDepthSmplr, texturecube<float> skybox, sampler skyboxSmplr)
{
    float3 viewPos = (_42.view * float4(worldPos, 1.0)).xyz;
    float3 viewNormal = fast::normalize((_42.view * float4(normal, 0.0)).xyz);
    float mirrorFactor = fast::clamp(fast::max(metallic, reflectivity) * (1.0 - roughness), 0.0, 1.0);
    float3 skyReflection = sampleSkyReflection(normal, viewDir, albedo, metallic, skybox, skyboxSmplr);
    float3 viewDirection = fast::normalize(viewPos);
    float3 viewReflect = fast::normalize(reflect(viewDirection, viewNormal));
    if (viewReflect.z > 0.0)
    {
        return float4(0.0);
    }
    float3 rayOrigin = viewPos + (viewNormal * 0.00999999977648258209228515625);
    float3 rayDir = viewReflect;
    float stepSize = _96.maxDistance / float(_96.steps);
    float3 currentPos = rayOrigin;
    float jitter = fract(sin(dot(gl_FragCoord.xy, float2(12.98980045318603515625, 78.233001708984375))) * 43758.546875) * roughness * 0.25;
    currentPos += ((rayDir * stepSize) * jitter);
    float2 hitUV = float2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    float2 fallbackUV = float2(0.0);
    bool hasFallbackUV = false;
    float3 lastPos = currentPos;
    for (int i = 0; i < _96.steps; i++)
    {
        float t = float(i) / float(_96.steps);
        float adaptiveStep = mix(0.300000011920928955078125, 1.0, t);
        currentPos += ((rayDir * stepSize) * adaptiveStep);
        float4 projectedPos = _42.projection * float4(currentPos, 1.0);
        float _186 = projectedPos.w;
        float4 _187 = projectedPos;
        float3 _190 = _187.xyz / float3(_186);
        projectedPos.x = _190.x;
        projectedPos.y = _190.y;
        projectedPos.z = _190.z;
        float2 screenUV = (projectedPos.xy * 0.5) + float2(0.5);
        bool _208 = screenUV.x < 0.0;
        bool _215;
        if (!_208)
        {
            _215 = screenUV.x > 1.0;
        }
        else
        {
            _215 = _208;
        }
        bool _222;
        if (!_215)
        {
            _222 = screenUV.y < 0.0;
        }
        else
        {
            _222 = _215;
        }
        bool _229;
        if (!_222)
        {
            _229 = screenUV.y > 1.0;
        }
        else
        {
            _229 = _222;
        }
        if (!_229)
        {
            fallbackUV = screenUV;
            hasFallbackUV = true;
        }
        else
        {
            break;
        }
        float hierarchyLevel = fast::clamp(floor(log2(1.0 + float(i) * 0.25)),
                                           0.0, 5.0);
        float rawDepth = gDepth.sample(gDepthSmplr, screenUV,
                                       level(hierarchyLevel)).x;
        if (rawDepth >= 0.99989998340606689453125)
        {
            currentPos += rayDir * stepSize * (exp2(hierarchyLevel) - 1.0);
            lastPos = currentPos;
            continue;
        }
        float3 sampleWorldPos = gPosition.sample(gPositionSmplr, screenUV).xyz;
        float3 sampleNormal = gNormal.sample(gNormalSmplr, screenUV).xyz;
        if (dot(sampleNormal, sampleNormal) < 0.00999999977648258209228515625)
        {
            lastPos = currentPos;
            continue;
        }
        if (length(sampleWorldPos - worldPos) < 0.07999999821186065673828125)
        {
            lastPos = currentPos;
            continue;
        }
        float3 sampleViewPos = (_42.view * float4(sampleWorldPos, 1.0)).xyz;
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        float3 sampleViewNormal = fast::normalize((_42.view * float4(sampleNormal, 0.0)).xyz);
        if ((dot(sampleViewNormal, viewNormal) > 0.920000016689300537109375) && (abs(sampleDepth - currentDepth) < (_96.thickness * 1.5)))
        {
            lastPos = currentPos;
            continue;
        }
        float depthDiff = sampleDepth - currentDepth;
        bool _265 = depthDiff > 0.0;
        bool _272;
        if (_265)
        {
            _272 = depthDiff < _96.thickness;
        }
        else
        {
            _272 = _265;
        }
        if (_272)
        {
            float3 binarySearchStart = lastPos;
            float3 binarySearchEnd = currentPos;
            for (int j = 0; j < 8; j++)
            {
                float3 midPoint = (binarySearchStart + binarySearchEnd) * 0.5;
                float4 midProj = _42.projection * float4(midPoint, 1.0);
                float _303 = midProj.w;
                float4 _304 = midProj;
                float3 _307 = _304.xyz / float3(_303);
                midProj.x = _307.x;
                midProj.y = _307.y;
                midProj.z = _307.z;
                float2 midUV = (midProj.xy * 0.5) + float2(0.5);
                bool _321 = midUV.x < 0.0;
                bool _328;
                if (!_321)
                {
                    _328 = midUV.x > 1.0;
                }
                else
                {
                    _328 = _321;
                }
                bool _335;
                if (!_328)
                {
                    _335 = midUV.y < 0.0;
                }
                else
                {
                    _335 = _328;
                }
                bool _342;
                if (!_335)
                {
                    _342 = midUV.y > 1.0;
                }
                else
                {
                    _342 = _335;
                }
                if (_342)
                {
                    break;
                }
                float midRawDepth = gDepth.sample(gDepthSmplr, midUV).x;
                if (midRawDepth >= 0.99989998340606689453125)
                {
                    binarySearchStart = midPoint;
                    continue;
                }
                float3 midSampleWorldPos = gPosition.sample(gPositionSmplr, midUV).xyz;
                float3 midSampleViewPos = (_42.view * float4(midSampleWorldPos, 1.0)).xyz;
                float midSampleDepth = -midSampleViewPos.z;
                float midCurrentDepth = -midPoint.z;
                if (midCurrentDepth < midSampleDepth)
                {
                    binarySearchStart = midPoint;
                }
                else
                {
                    binarySearchEnd = midPoint;
                }
            }
            float4 finalProj = _42.projection * float4(binarySearchEnd, 1.0);
            float _364 = finalProj.w;
            float4 _365 = finalProj;
            float3 _368 = _365.xyz / float3(_364);
            finalProj.x = _368.x;
            finalProj.y = _368.y;
            finalProj.z = _368.z;
            hitUV = (finalProj.xy * 0.5) + float2(0.5);
            hitDepth = length(currentPos - rayOrigin);
            hit = true;
            break;
        }
        lastPos = currentPos;
    }
    if (!hit)
    {
        return float4(0.0);
    }
    float3 hitColor = sceneColor.sample(sceneColorSmplr, hitUV).xyz;
    float tintStrength = metallic * 0.3499999940395355224609375;
    float3 metalTint = mix(float3(1.0), albedo, float3(tintStrength));
    hitColor *= metalTint;
    float hitDepthRaw = gDepth.sample(gDepthSmplr, hitUV).x;
    float3 hitNormalSample = gNormal.sample(gNormalSmplr, hitUV).xyz;
    float hitNormalLenSq = dot(hitNormalSample, hitNormalSample);
    float3 hitNormalDir = (hitNormalLenSq > 9.9999997473787516355514526367188e-06) ? (hitNormalSample * rsqrt(hitNormalLenSq)) : (-rayDir);
    float geometryValidity = 1.0 - smoothstep(0.99800002574920654296875, 1.0, hitDepthRaw);
    geometryValidity *= smoothstep(0.00999999977648258209228515625, 0.100000001490116119384765625, hitNormalLenSq);
    float normalFacing = fast::max(dot(hitNormalDir, -rayDir), 0.0);
    geometryValidity *= smoothstep(0.0500000007450580596923828125, 0.25, normalFacing);
    float hitLuma = dot(hitColor, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    float skyLuma = dot(skyReflection, float3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875));
    float luminanceValidity = smoothstep(0.00200000009499490261077880859375, 0.02999999932944774627685546875, hitLuma + (skyLuma * 0.100000001490116119384765625));
    float2 edgeFade = smoothstep(float2(0.0), float2(0.1500000059604644775390625), hitUV) * (float2(1.0) - smoothstep(float2(0.85000002384185791015625), float2(1.0), hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    float distanceFade = 1.0 - smoothstep(_96.maxDistance * 0.5, _96.maxDistance, hitDepth);
    float roughnessFade = 1.0 - smoothstep(0.0, _96.maxRoughness, roughness);
    float hitConfidence = (edgeFactor * distanceFade) * roughnessFade;
    hitConfidence *= geometryValidity * luminanceValidity;
    float3 F0 = mix(float3(0.039999999105930328369140625), albedo, float3(metallic));
    float3 V = fast::normalize(viewDir);
    float cosTheta = fast::max(dot(normal, V), 0.0);
    float param = cosTheta;
    float3 param_1 = F0;
    float3 fresnel = fresnelSchlick(param, param_1);
    float fresnelFactor = ((fresnel.x + fresnel.y) + fresnel.z) / 3.0;
    float fresnelWeight = mix(fresnelFactor, 1.0, mirrorFactor);
    float3 reflectionColor = mix(skyReflection, hitColor, float3(hitConfidence));
    float finalFade = mix(hitConfidence * fresnelWeight, 1.0, mirrorFactor);
    finalFade = fast::clamp(finalFade, 0.0, 1.0);
    return float4(reflectionColor, finalFade);
}

fragment main0_out main0(main0_in in [[stage_in]], constant Uniforms& _42 [[buffer(0)]], constant SSRParameters& _96 [[buffer(1)]], texture2d<float> gPosition [[texture(0)]], texture2d<float> sceneColor [[texture(1)]], texture2d<float> gNormal [[texture(2)]], texture2d<float> gAlbedoSpec [[texture(3)]], texture2d<float> gMaterial [[texture(4)]], texture2d<float> gDepth [[texture(5)]], texturecube<float> skybox [[texture(6)]], texture2d<float> historyTexture [[texture(7)]], sampler gPositionSmplr [[sampler(0)]], sampler sceneColorSmplr [[sampler(1)]], sampler gNormalSmplr [[sampler(2)]], sampler gAlbedoSpecSmplr [[sampler(3)]], sampler gMaterialSmplr [[sampler(4)]], sampler gDepthSmplr [[sampler(5)]], sampler skyboxSmplr [[sampler(6)]], sampler historyTextureSmplr [[sampler(7)]], float4 gl_FragCoord [[position]])
{
    main0_out out = {};
    float3 worldPos = gPosition.sample(gPositionSmplr, in.TexCoord).xyz;
    float3 normal = fast::normalize(gNormal.sample(gNormalSmplr, in.TexCoord).xyz);
    float3 albedo = gAlbedoSpec.sample(gAlbedoSpecSmplr, in.TexCoord).xyz;
    float4 material = gMaterial.sample(gMaterialSmplr, in.TexCoord);
    float metallic = material.x;
    float roughness = material.y;
    float reflectivity = material.w;
    if (length(normal) < 0.001000000047497451305389404296875)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    if (roughness > _96.maxRoughness || fast::max(metallic, reflectivity) < 0.02)
    {
        out.FragColor = float4(0.0);
        return out;
    }
    float3 viewDir = fast::normalize(_42.cameraPosition - worldPos);
    float3 param = worldPos;
    float3 param_1 = normal;
    float3 param_2 = viewDir;
    float param_3 = roughness;
    float param_4 = metallic;
    float param_5 = reflectivity;
    float3 param_6 = albedo;
    float4 reflection = SSR(param, param_1, param_2, param_3, param_4, param_5, param_6, _42, _96, gl_FragCoord, gPosition, gPositionSmplr, sceneColor, sceneColorSmplr, gNormal, gNormalSmplr, gDepth, gDepthSmplr, skybox, skyboxSmplr);
    if (_96.debugMode != 0)
    {
        out.FragColor = float4(1.0 - reflection.w, reflection.w, 0.0, 1.0);
        return out;
    }
    if (reflection.w > 0.0 && _96.historyWeight > 0.0)
    {
        float4 history = historyTexture.sample(historyTextureSmplr, in.TexCoord);
        float validHistory = step(0.001, history.w);
        reflection = mix(reflection, history, _96.historyWeight * validHistory * reflection.w);
    }
    out.FragColor = reflection;
    return out;
}
