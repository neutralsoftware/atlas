#if defined(ATLAS_FRAGMENT)
struct Uniforms
{
    row_major float4x4 inverseProjection;
    row_major float4x4 inverseView;
    row_major float4x4 projection;
    row_major float4x4 view;
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

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

float3 fresnelSchlick(float  cosTheta, float3  F0)
{
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

float3 sampleSkyReflection(float3  normal, float3  viewDir, float3  albedo, float  metallic, SamplerCube skybox)
{
    float3 reflected = reflect(-normalize(viewDir), normalize(normal));
    float3 skyColor = skybox.Sample(reflected).xyz;
    float skyLuma = dot(skyColor, float3(0.212599993, 0.715200007, 0.0722000003));
    if (skyLuma < 0.00100000005)
    {
        float t = clamp((reflected.y * 0.5) + 0.5, 0.0, 1.0);
        float3 horizon = float3(0.74000001, 0.790000021, 0.879999995);
        float3 zenith = float3(0.5, 0.639999986, 0.839999974);
        skyColor = lerp(horizon, zenith, float3(t));
    }
    float tintStrength = metallic * 0.349999994;
    float3 tint = lerp(float3(1.0), albedo, float3(tintStrength));
    return skyColor * tint;
}

float4 SSR(float3  worldPos, float3  normal, float3  viewDir, float  roughness, float  metallic, float  reflectivity, float3  albedo, Uniforms  uniforms, SSRParameters  sSRParameters, inout float4  gl_FragCoord, Sampler2D gPosition, Sampler2D sceneColor, Sampler2D gNormal, Sampler2D gDepth, SamplerCube skybox)
{
    float3 viewPos = (mul(float4(worldPos, 1.0), uniforms.view)).xyz;
    float3 viewNormal = normalize((mul(float4(normal, 0.0), uniforms.view)).xyz);
    float mirrorFactor = clamp(max(metallic, reflectivity) * (1.0 - roughness), 0.0, 1.0);
    float3 skyReflection = sampleSkyReflection(normal, viewDir, albedo, metallic, skybox);
    float3 viewDirection = normalize(viewPos);
    float3 viewReflect = normalize(reflect(viewDirection, viewNormal));
    if (viewReflect.z > 0.0)
    {
        return float4(0.0);
    }
    float3 rayOrigin = viewPos + (viewNormal * 0.00999999978);
    float3 rayDir = viewReflect;
    float stepSize = sSRParameters.maxDistance / float(sSRParameters.steps);
    float3 currentPos = rayOrigin;
    float jitter = frac(sin(dot(gl_FragCoord.xy, float2(12.9898005, 78.2330017))) * 43758.546875) * roughness * 0.25;
    currentPos += ((rayDir * stepSize) * jitter);
    float2 hitUV = float2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    float2 fallbackUV = float2(0.0);
    bool hasFallbackUV = false;
    float3 lastPos = currentPos;
    for (int i = 0; i < sSRParameters.steps; i++)
    {
        float t = float(i) / float(sSRParameters.steps);
        float adaptiveStep = lerp(0.300000012, 1.0, t);
        currentPos += ((rayDir * stepSize) * adaptiveStep);
        float4 projectedPos = mul(float4(currentPos, 1.0), uniforms.projection);
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
        float hierarchyLevel = clamp(floor(log2(1.0 + float(i) * 0.25)),
                                           0.0, 5.0);
        float rawDepth = gDepth.SampleLevel(screenUV,
                                       hierarchyLevel).x;
        if (rawDepth >= 0.999899983)
        {
            currentPos += rayDir * stepSize * (exp2(hierarchyLevel) - 1.0);
            lastPos = currentPos;
            continue;
        }
        float3 sampleWorldPos = gPosition.Sample(screenUV).xyz;
        float3 sampleNormal = gNormal.Sample(screenUV).xyz;
        if (dot(sampleNormal, sampleNormal) < 0.00999999978)
        {
            lastPos = currentPos;
            continue;
        }
        if (length(sampleWorldPos - worldPos) < 0.0799999982)
        {
            lastPos = currentPos;
            continue;
        }
        float3 sampleViewPos = (mul(float4(sampleWorldPos, 1.0), uniforms.view)).xyz;
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        float3 sampleViewNormal = normalize((mul(float4(sampleNormal, 0.0), uniforms.view)).xyz);
        if ((dot(sampleViewNormal, viewNormal) > 0.920000017) && (abs(sampleDepth - currentDepth) < (sSRParameters.thickness * 1.5)))
        {
            lastPos = currentPos;
            continue;
        }
        float depthDiff = sampleDepth - currentDepth;
        bool _265 = depthDiff > 0.0;
        bool _272;
        if (_265)
        {
            _272 = depthDiff < sSRParameters.thickness;
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
                float4 midProj = mul(float4(midPoint, 1.0), uniforms.projection);
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
                float midRawDepth = gDepth.Sample(midUV).x;
                if (midRawDepth >= 0.999899983)
                {
                    binarySearchStart = midPoint;
                    continue;
                }
                float3 midSampleWorldPos = gPosition.Sample(midUV).xyz;
                float3 midSampleViewPos = (mul(float4(midSampleWorldPos, 1.0), uniforms.view)).xyz;
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
            float4 finalProj = mul(float4(binarySearchEnd, 1.0), uniforms.projection);
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
    float3 hitColor = sceneColor.Sample(hitUV).xyz;
    float tintStrength = metallic * 0.349999994;
    float3 metalTint = lerp(float3(1.0), albedo, float3(tintStrength));
    hitColor *= metalTint;
    float hitDepthRaw = gDepth.Sample(hitUV).x;
    float3 hitNormalSample = gNormal.Sample(hitUV).xyz;
    float hitNormalLenSq = dot(hitNormalSample, hitNormalSample);
    float3 hitNormalDir = (hitNormalLenSq > 9.99999975e-06) ? (hitNormalSample * rsqrt(hitNormalLenSq)) : (-rayDir);
    float geometryValidity = 1.0 - smoothstep(0.998000026, 1.0, hitDepthRaw);
    geometryValidity *= smoothstep(0.00999999978, 0.100000001, hitNormalLenSq);
    float normalFacing = max(dot(hitNormalDir, -rayDir), 0.0);
    geometryValidity *= smoothstep(0.0500000007, 0.25, normalFacing);
    float hitLuma = dot(hitColor, float3(0.212599993, 0.715200007, 0.0722000003));
    float skyLuma = dot(skyReflection, float3(0.212599993, 0.715200007, 0.0722000003));
    float luminanceValidity = smoothstep(0.00200000009, 0.0299999993, hitLuma + (skyLuma * 0.100000001));
    float2 edgeFade = smoothstep(float2(0.0), float2(0.150000006), hitUV) * (float2(1.0) - smoothstep(float2(0.850000024), float2(1.0), hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    float distanceFade = 1.0 - smoothstep(sSRParameters.maxDistance * 0.5, sSRParameters.maxDistance, hitDepth);
    float roughnessFade = 1.0 - smoothstep(0.0, sSRParameters.maxRoughness, roughness);
    float hitConfidence = (edgeFactor * distanceFade) * roughnessFade;
    hitConfidence *= geometryValidity * luminanceValidity;
    float3 F0 = lerp(float3(0.0399999991), albedo, float3(metallic));
    float3 V = normalize(viewDir);
    float cosTheta = max(dot(normal, V), 0.0);
    float param = cosTheta;
    float3 param_1 = F0;
    float3 fresnel = fresnelSchlick(param, param_1);
    float fresnelFactor = ((fresnel.x + fresnel.y) + fresnel.z) / 3.0;
    float fresnelWeight = lerp(fresnelFactor, 1.0, mirrorFactor);
    float3 reflectionColor = lerp(skyReflection, hitColor, float3(hitConfidence));
    float finalFade = lerp(hitConfidence * fresnelWeight, 1.0, mirrorFactor);
    finalFade = clamp(finalFade, 0.0, 1.0);
    return float4(reflectionColor, finalFade);
}

[[vk::binding(0, 1)]] ConstantBuffer<Uniforms> uniforms;
[[vk::binding(1, 1)]] ConstantBuffer<SSRParameters> sSRParameters;
[[vk::binding(0, 2)]] Sampler2D gPosition;
[[vk::binding(4, 2)]] Sampler2D sceneColor;
[[vk::binding(1, 2)]] Sampler2D gNormal;
[[vk::binding(2, 2)]] Sampler2D gAlbedoSpec;
[[vk::binding(3, 2)]] Sampler2D gMaterial;
[[vk::binding(5, 2)]] Sampler2D gDepth;
[[vk::binding(0, 3)]] SamplerCube skybox;
[[vk::binding(6, 2)]] Sampler2D historyTexture;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input, float4 gl_FragCoord : SV_Position)
{
    FragmentOutput output = {};
    float3 worldPos = gPosition.Sample(input.TexCoord).xyz;
    float3 normal = normalize(gNormal.Sample(input.TexCoord).xyz);
    float3 albedo = gAlbedoSpec.Sample(input.TexCoord).xyz;
    float4 material = gMaterial.Sample(input.TexCoord);
    float metallic = material.x;
    float roughness = material.y;
    float reflectivity = material.w;
    if (length(normal) < 0.00100000005)
    {
        output.FragColor = float4(0.0);
        return output;
    }
    if (roughness > sSRParameters.maxRoughness || max(metallic, reflectivity) < 0.02)
    {
        output.FragColor = float4(0.0);
        return output;
    }
    float3 viewDir = normalize(uniforms.cameraPosition - worldPos);
    float3 param = worldPos;
    float3 param_1 = normal;
    float3 param_2 = viewDir;
    float param_3 = roughness;
    float param_4 = metallic;
    float param_5 = reflectivity;
    float3 param_6 = albedo;
    float4 reflection = SSR(param, param_1, param_2, param_3, param_4, param_5, param_6, uniforms, sSRParameters, gl_FragCoord, gPosition, sceneColor, gNormal, gDepth, skybox);
    if (sSRParameters.debugMode != 0)
    {
        output.FragColor = float4(1.0 - reflection.w, reflection.w, 0.0, 1.0);
        return output;
    }
    if (reflection.w > 0.0 && sSRParameters.historyWeight > 0.0)
    {
        float4 history = historyTexture.Sample(input.TexCoord);
        float validHistory = step(0.001, history.w);
        reflection = lerp(reflection, history, sSRParameters.historyWeight * validHistory * reflection.w);
    }
    output.FragColor = reflection;
    return output;
}
#endif
