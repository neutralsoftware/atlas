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
    [[vk::location(1)]] float3 WorldPos : TEXCOORD1;
    [[vk::location(2)]] float3 WorldNormal : TEXCOORD2;
    [[vk::location(3)]] float3 WorldTangent : TEXCOORD3;
    [[vk::location(4)]] float3 WorldBitangent : TEXCOORD4;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
    [[vk::location(1)]] float2 aTexCoord : TEXCOORD1;
    [[vk::location(2)]] float3 aNormal : TEXCOORD2;
    [[vk::location(3)]] float3 aTangent : TEXCOORD3;
    [[vk::location(4)]] float3 aBitangent : TEXCOORD4;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    output.gl_Position = mul(float4(input.aPos, 1.0), (mul(uniforms.model, (mul(uniforms.view, uniforms.projection)))));
    output.TexCoord = input.aTexCoord;
    output.WorldPos = float3((mul(float4(input.aPos, 1.0), uniforms.model)).xyz);
    output.WorldNormal = normalize(mul(input.aNormal, float3x3(uniforms.model[0].xyz, uniforms.model[1].xyz, uniforms.model[2].xyz)));
    output.WorldTangent = normalize(mul(input.aTangent, float3x3(uniforms.model[0].xyz, uniforms.model[1].xyz, uniforms.model[2].xyz)));
    output.WorldBitangent = normalize(mul(input.aBitangent, float3x3(uniforms.model[0].xyz, uniforms.model[1].xyz, uniforms.model[2].xyz)));
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct Uniforms
{
    float4 waterColor;
    float3 cameraPos;
    float time;
    row_major float4x4 projection;
    row_major float4x4 view;
    row_major float4x4 invProjection;
    row_major float4x4 invView;
    float3 lightDirection;
    float3 lightColor;
};

struct Parameters
{
    float refractionStrength;
    float reflectionStrength;
    float depthFade;
    int hasNormalTexture;
    int hasMovementTexture;
    float3 windForce;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
    float4 BrightColor : SV_Target1;
};

struct FragmentInput
{
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
    [[vk::location(1)]] float3 WorldPos : TEXCOORD1;
    [[vk::location(2)]] float3 WorldNormal : TEXCOORD2;
    [[vk::location(3)]] float3 WorldTangent : TEXCOORD3;
    [[vk::location(4)]] float3 WorldBitangent : TEXCOORD4;
};

[[vk::binding(0, 1)]] ConstantBuffer<Uniforms> uniforms;
[[vk::binding(1, 1)]] ConstantBuffer<Parameters> parameters;
[[vk::binding(4, 2)]] Sampler2D movementTexture;
[[vk::binding(5, 2)]] Sampler2D normalTexture;
[[vk::binding(2, 2)]] Sampler2D reflectionTexture;
[[vk::binding(3, 2)]] Sampler2D refractionTexture;
[[vk::binding(0, 2)]] Sampler2D sceneTexture;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float3 normal = normalize(input.WorldNormal);
    float3 viewDir = normalize(float3(uniforms.cameraPos) - input.WorldPos);
    float4 clipSpace = mul(float4(input.WorldPos, 1.0), (mul(uniforms.view, uniforms.projection)));
    float3 ndc = clipSpace.xyz / float3(clipSpace.w);
    float2 screenUV = (ndc.xy * 0.5) + float2(0.5);
    float windStrength = length(parameters.windForce);
    float2 _78;
    if (windStrength > 0.00100000005)
    {
        _78 = normalize(parameters.windForce.xy);
    }
    else
    {
        _78 = float2(1.0, 0.0);
    }
    float2 windDir = _78;
    float waveSpeed = 0.150000006 + (windStrength * 0.300000012);
    float waveAmplitude = 0.00999999978 + (windStrength * 0.0199999996);
    float waveFrequency = 30.0 + (windStrength * 10.0);
    float2 waveOffset;
    waveOffset.x = sin(((input.TexCoord.x * windDir.x) + (uniforms.time * waveSpeed)) * waveFrequency);
    waveOffset.y = cos(((input.TexCoord.y * windDir.y) - (uniforms.time * waveSpeed)) * waveFrequency);
    waveOffset *= waveAmplitude;
    float2 flowOffset = float2(0.0);
    if (parameters.hasMovementTexture == 1)
    {
        float2 windUV = (parameters.windForce.xy * uniforms.time) * 0.0500000007;
        float2 movementUV = (input.TexCoord * 2.0) + windUV;
        float2 movementSample = movementTexture.Sample(movementUV).xy;
        movementSample = (movementSample * 2.0) - float2(1.0);
        flowOffset = (movementSample * windStrength) * 0.150000006;
        waveOffset += (flowOffset * 0.5);
    }
    if (parameters.hasNormalTexture == 1)
    {
        float3 T = normalize(input.WorldTangent);
        float3 B = normalize(input.WorldBitangent);
        float3 N = normalize(input.WorldNormal);
        row_major float3x3 TBN = float3x3(float3(T), float3(B), float3(N));
        float normalSpeed = 0.0299999993 + (windStrength * 0.0500000007);
        float2 normalUV1 = ((input.TexCoord * 5.0) + (waveOffset * 10.0)) + ((parameters.windForce.xy * uniforms.time) * normalSpeed);
        float2 normalUV2 = ((input.TexCoord * 3.0) - (waveOffset * 8.0)) - (((parameters.windForce.xy * uniforms.time) * normalSpeed) * 0.800000012);
        float3 normalMap1 = normalTexture.Sample(normalUV1).xyz;
        float3 normalMap2 = normalTexture.Sample(normalUV2).xyz;
        normalMap1 = (normalMap1 * 2.0) - float3(1.0);
        normalMap2 = (normalMap2 * 2.0) - float3(1.0);
        float3 blendedNormal = normalize(normalMap1 + normalMap2);
        float3 worldSpaceNormal = normalize(mul(blendedNormal, TBN));
        float normalStrength = 0.5 + (windStrength * 0.300000012);
        normal = normalize(lerp(N, worldSpaceNormal, float3(normalStrength)));
    }
    float2 reflectionUV = screenUV;
    reflectionUV.y = 1.0 - reflectionUV.y;
    reflectionUV += (waveOffset * 0.300000012);
    float2 refractionUV = screenUV - (waveOffset * 0.200000003);
    bool _324 = reflectionUV.x >= 0.0;
    bool _330;
    if (_324)
    {
        _330 = reflectionUV.x <= 1.0;
    }
    else
    {
        _330 = _324;
    }
    bool _336;
    if (_330)
    {
        _336 = reflectionUV.y >= 0.0;
    }
    else
    {
        _336 = _330;
    }
    bool _342;
    if (_336)
    {
        _342 = reflectionUV.y <= 1.0;
    }
    else
    {
        _342 = _336;
    }
    bool reflectionInBounds = _342;
    bool _346 = refractionUV.x >= 0.0;
    bool _352;
    if (_346)
    {
        _352 = refractionUV.x <= 1.0;
    }
    else
    {
        _352 = _346;
    }
    bool _358;
    if (_352)
    {
        _358 = refractionUV.y >= 0.0;
    }
    else
    {
        _358 = _352;
    }
    bool _364;
    if (_358)
    {
        _364 = refractionUV.y <= 1.0;
    }
    else
    {
        _364 = _358;
    }
    bool refractionInBounds = _364;
    reflectionUV = clamp(reflectionUV, float2(0.0500000007), float2(0.949999988));
    refractionUV = clamp(refractionUV, float2(0.0500000007), float2(0.949999988));
    float2 reflectionEdgeFade = smoothstep(float2(0.0), float2(0.150000006), reflectionUV) * smoothstep(float2(1.0), float2(0.850000024), reflectionUV);
    float reflectionFadeFactor = reflectionEdgeFade.x * reflectionEdgeFade.y;
    float2 refractionEdgeFade = smoothstep(float2(0.0), float2(0.150000006), refractionUV) * smoothstep(float2(1.0), float2(0.850000024), refractionUV);
    float refractionFadeFactor = refractionEdgeFade.x * refractionEdgeFade.y;
    if (!reflectionInBounds)
    {
        reflectionFadeFactor *= 0.300000012;
    }
    if (!refractionInBounds)
    {
        refractionFadeFactor *= 0.300000012;
    }
    float4 reflectionSample = reflectionTexture.Sample(reflectionUV);
    float4 refractionSample = refractionTexture.Sample(refractionUV);
    float4 sceneFallback = sceneTexture.Sample(screenUV);
    bool validReflection = length(reflectionSample.xyz) > 0.00999999978;
    bool validRefraction = length(refractionSample.xyz) > 0.00999999978;
    if (!validReflection)
    {
        reflectionSample = sceneFallback;
    }
    if (!validRefraction)
    {
        refractionSample = sceneFallback;
    }
    float fresnel = powr(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 3.0);
    fresnel = lerp(0.0199999996, 1.0, fresnel);
    fresnel *= parameters.reflectionStrength;
    float3 combined = lerp(refractionSample.xyz, reflectionSample.xyz, float3(fresnel));
    float waterTint = clamp(parameters.depthFade * 0.300000012, 0.0, 0.5);
    combined = lerp(combined, uniforms.waterColor.xyz, float3(waterTint));
    float foamAmount = 0.0;
    if (parameters.hasMovementTexture == 1)
    {
        float foamFactor = smoothstep(0.699999988, 1.0, length(flowOffset) * windStrength);
        foamAmount = foamFactor * 0.200000003;
        combined = lerp(combined, float3(1.0), float3(foamAmount));
    }
    float3 lightDir = normalize(-uniforms.lightDirection);
    float diffuse = max(dot(normal, lightDir), 0.0);
    diffuse *= 0.300000012;
    float3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(normal, halfDir), 0.0);
    float specular = powr(specAngle, 128.0);
    specular *= ((fresnel * 0.5) + 0.5);
    float waveVariation = (sin((input.TexCoord.x * 50.0) + (uniforms.time * 2.0)) * 0.5) + 0.5;
    waveVariation *= ((cos((input.TexCoord.y * 50.0) - (uniforms.time * 1.5)) * 0.5) + 0.5);
    specular *= (0.699999988 + (waveVariation * 0.300000012));
    float3 lighting = uniforms.lightColor * (diffuse + (specular * 2.0));
    combined += lighting;
    float3 specularHighlight = (uniforms.lightColor * specular) * 2.5;
    specularHighlight += float3(foamAmount * 2.0);
    float alpha = lerp(0.699999988, 0.949999988, fresnel);
    output.FragColor = float4(combined, alpha);
    float luminance = max(max(combined.x, combined.y), combined.z);
    float3 bloomColor = float3(0.0);
    if (luminance > 1.0)
    {
        bloomColor = combined - float3(1.0);
    }
    bloomColor += specularHighlight;
    output.BrightColor = float4(bloomColor, alpha);
    return output;
}
#endif
