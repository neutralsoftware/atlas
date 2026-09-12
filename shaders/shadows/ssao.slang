#if defined(ATLAS_FRAGMENT)
struct Paramters {
    row_major float4x4 projection;
    row_major float4x4 inverseProjection;
    row_major float4x4 view;
    float2 noiseScale;
};

struct Samples {
    float3 samples[64];
};

struct FragmentOutput {
    float FragColor : SV_Target0;
};

struct FragmentInput {
    [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

float3 reconstructViewPos(float2 uv, float depth,
                                        row_major float4x4 inverseProjection) {
    float2 ndc;
    ndc.x = uv.x * 2.0 - 1.0;
    ndc.y = (1.0 - uv.y) * 2.0 - 1.0;
    float ndcZ = depth * 2.0 - 1.0;
    float4 clip = float4(ndc, ndcZ, 1.0);
    float4 viewPos = mul(clip, inverseProjection);
    float invW = 1.0 / max(abs(viewPos.w), 1e-6);
    return viewPos.xyz * invW;
}

[[vk::binding(0, 1)]] ConstantBuffer<Paramters> paramters;
[[vk::binding(1, 1)]] ConstantBuffer<Samples> samples;
[[vk::binding(0, 2)]] Sampler2D gPosition;
[[vk::binding(1, 2)]] Sampler2D gNormal;
[[vk::binding(2, 2)]] Sampler2D texNoise;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input) {
    FragmentOutput output = {};
    float3 sampledNormalWorld = gNormal.Sample(input.TexCoord).xyz;
    float sampledNormalLength = length(sampledNormalWorld);
    if (!all(isfinite(sampledNormalWorld)) ||
        sampledNormalLength <= 0.00100000005) {
        output.FragColor = 1.0;
        return output;
    }
    float4 gPositionCenter = gPosition.Sample(input.TexCoord);
    float centerDepth = gPositionCenter.w;
    if (!isfinite(centerDepth)) {
        output.FragColor = 1.0;
        return output;
    }
    centerDepth = clamp(centerDepth, 0.0, 1.0);
    float3 fragPos = reconstructViewPos(input.TexCoord, centerDepth,
                                        paramters.inverseProjection);
    float3 normalWorld = sampledNormalWorld / sampledNormalLength;
    float3 normal = normalize((mul(float4(normalWorld, 0.0), paramters.view)).xyz);
    float3 randomVec = normalize(
        (texNoise.Sample((input.TexCoord * paramters.noiseScale)).xyz *
         2.0) -
        float3(1.0));
    float3 tangent =
        normalize(randomVec - (normal * dot(randomVec, normal)));
    float3 bitangent = cross(normal, tangent);
    row_major float3x3 TBN = float3x3(float3(tangent), float3(bitangent), float3(normal));
    float viewDepth = abs(fragPos.z);
    float baseRadius =
        lerp(0.550000012, 2.20000005,
            clamp(viewDepth / 140.0, 0.0, 1.0));
    float3 dPosDx = ddx(fragPos);
    float3 dPosDy = ddy(fragPos);
    float pixelWorldScale = max(length(dPosDx), length(dPosDy));
    if (!isfinite(pixelWorldScale)) {
        pixelWorldScale = 0.0;
    }
    float sampleRadius =
        clamp(max(baseRadius, pixelWorldScale * 6.0), 0.349999994,
              12.0);
    float depthBias = clamp(
        max(lerp(0.00600000005,
                0.0240000002,
                clamp(viewDepth / 180.0, 0.0, 1.0)),
            sampleRadius * 0.00999999978),
        0.00300000003, 0.0799999982);
    float occlusion = 0.0;
    int validSamples = 0;
    for (int i = 0; i < 64; i++) {
        float3 samplePos = mul(samples.samples[i], TBN);
        samplePos = fragPos + (samplePos * sampleRadius);
        float4 offset = mul(float4(samplePos, 1.0), paramters.projection);
        float _160 = offset.w;
        float4 _161 = offset;
        float3 _164 = _161.xyz / float3(_160);
        offset.x = _164.x;
        offset.y = _164.y;
        offset.z = _164.z;
        float4 _174 = offset;
        float2 _178 = (_174.xy * 0.5) + float2(0.5);
        offset.x = _178.x;
        offset.y = 1.0 - _178.y;
        bool _185 = offset.x < 0.0;
        bool _192;
        if (!_185) {
            _192 = offset.x > 1.0;
        } else {
            _192 = _185;
        }
        bool _199;
        if (!_192) {
            _199 = offset.y < 0.0;
        } else {
            _199 = _192;
        }
        bool _206;
        if (!_199) {
            _206 = offset.y > 1.0;
        } else {
            _206 = _199;
        }
        if (_206) {
            continue;
        }
        float sampleDepth = gPosition.Sample(offset.xy).w;
        if (!isfinite(sampleDepth)) {
            continue;
        }
        sampleDepth = clamp(sampleDepth, 0.0, 1.0);
        float3 sampleViewPos =
            reconstructViewPos(offset.xy, sampleDepth, paramters.inverseProjection);
        float rangeCheck =
            smoothstep(0.0, 1.0,
                       sampleRadius / (abs(fragPos.z - sampleViewPos.z) +
                                        0.000500000024));
        occlusion +=
            (float(sampleViewPos.z >= (samplePos.z + depthBias)) * rangeCheck);
        validSamples++;
    }
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }
    output.FragColor = occlusion;
    return output;
}
#endif
