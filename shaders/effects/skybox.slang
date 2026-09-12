#if defined(ATLAS_VERTEX)
struct UniformBufferObject
{
    row_major float4x4 projection;
    row_major float4x4 view;
};

struct VertexOutput
{
    [[vk::location(0)]] float3 TexCoords : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

struct VertexInput
{
    [[vk::location(0)]] float3 aPos : TEXCOORD0;
};

[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> uniforms;
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output = {};
    output.TexCoords = input.aPos;
    row_major float3x3 _32 = float3x3(uniforms.view[0].xyz, uniforms.view[1].xyz, uniforms.view[2].xyz);
    row_major float4x4 viewNoTranslation = float4x4(float4(_32[0].x, _32[0].y, _32[0].z, 0.0), float4(_32[1].x, _32[1].y, _32[1].z, 0.0), float4(_32[2].x, _32[2].y, _32[2].z, 0.0), float4(0.0, 0.0, 0.0, 1.0));
    float4 pos = mul(float4(input.aPos, 1.0), (mul(viewNoTranslation, uniforms.projection)));
    output.gl_Position = pos.xyww;
    return output;
}
#endif

#if defined(ATLAS_FRAGMENT)
struct Params
{
    float3 sunDirection;
    float4 sunColor;
    float3 moonDirection;
    float4 moonColor;
    int hasDayNight;
};

struct AtmosphereParams
{
    float sunTintStrength;
    float moonTintStrength;
    float sunSizeMultiplier;
    float moonSizeMultiplier;
    float starDensity;
};

struct FragmentOutput
{
    float4 FragColor : SV_Target0;
};

struct FragmentInput
{
    [[vk::location(0)]] float3 TexCoords : TEXCOORD0;
};

float hash13(inout float3  p)
{
    p = frac(p * 443.897003);
    p += float3(dot(p, p.yzx + float3(19.1900005)));
    return frac((p.x + p.y) * p.z);
}

float3 generateStars(float3  dir, float  density, float  nightFactor)
{
    if ((density <= 0.0) || (nightFactor <= 0.0))
    {
        return float3(0.0);
    }
    float3 starSpace = dir * 50.0;
    float3 cell = floor(starSpace);
    float3 localPos = frac(starSpace);
    float3 param = cell;
    float _165 = hash13(param);
    float rand = _165;
    if (rand < (density * 0.300000012))
    {
        float3 param_1 = cell + float3(12.3400002, 56.7799988, 90.1200027);
        float _181 = hash13(param_1);
        float randX = _181;
        float3 param_2 = cell + float3(23.4500008, 67.8899994, 1.23000002);
        float _190 = hash13(param_2);
        float randY = _190;
        float3 param_3 = cell + float3(34.5600014, 78.9000015, 12.3400002);
        float _198 = hash13(param_3);
        float randZ = _198;
        float3 starPos = float3(randX, randY, randZ);
        float dist = length(localPos - starPos);
        float3 param_4 = cell + float3(45.6699982, 89.0100021, 23.4500008);
        float _217 = hash13(param_4);
        float starSize = 0.0199999996 + (_217 * 0.0299999993);
        float3 param_5 = cell + float3(56.7799988, 90.1200027, 34.5600014);
        float _227 = hash13(param_5);
        float brightness = 0.5 + (_227 * 0.5);
        float star = smoothstep(starSize, 0.0, dist) * brightness;
        float3 param_6 = cell + float3(67.8899994, 1.23000002, 45.6699982);
        float _243 = hash13(param_6);
        float twinkle = 0.800000012 + (0.200000003 * sin(_243 * 100.0));
        star *= (twinkle * nightFactor);
        float3 starColor = float3(1.0);
        if (rand > 0.899999976)
        {
            starColor = float3(0.800000012, 0.899999976, 1.0);
        }
        else
        {
            if (rand > 0.800000012)
            {
                starColor = float3(1.0, 0.899999976, 0.800000012);
            }
        }
        return starColor * star;
    }
    return float3(0.0);
}

float hash21(inout float2  p)
{
    p = frac(p * float2(123.339996, 456.209991));
    p += float2(dot(p, p + float2(45.3199997)));
    return frac(p.x * p.y);
}

float valueNoise(float2  p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = (f * f) * (float2(3.0) - (f * 2.0));
    float2 param = i;
    float _80 = hash21(param);
    float a = _80;
    float2 param_1 = i + float2(1.0, 0.0);
    float _88 = hash21(param_1);
    float b = _88;
    float2 param_2 = i + float2(0.0, 1.0);
    float _94 = hash21(param_2);
    float c = _94;
    float2 param_3 = i + float2(1.0);
    float _100 = hash21(param_3);
    float d = _100;
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float3 generateMoonTexture(inout float2  uv, float  distanceFromCenter, float3  tintColor)
{
    float angle = 0.5;
    float ca = cos(angle);
    float sa = sin(angle);
    uv = float2((ca * uv.x) - (sa * uv.y), (sa * uv.x) + (ca * uv.y));
    float2 param = uv * 2.0;
    float largeFeatures = valueNoise(param);
    largeFeatures = smoothstep(0.300000012, 0.699999988, largeFeatures);
    float2 param_1 = uv * 8.0;
    float mediumCraters = valueNoise(param_1);
    float2 craterUV = uv * 6.0;
    float2 craterCell = floor(craterUV);
    float2 craterLocal = frac(craterUV);
    float craters = 1.0;
    for (int i = 0; i < 4; i++)
    {
        float2 neighbor = float2(float(i % 2), float(i / 2));
        float2 cellPoint = craterCell + neighbor;
        float2 param_2 = cellPoint;
        float _353 = hash21(param_2);
        float2 param_3 = cellPoint + float2(13.6999998, 27.2999992);
        float _360 = hash21(param_3);
        float2 craterCenter = float2(_353, _360);
        float dist = length((craterLocal - neighbor) - craterCenter);
        float2 param_4 = cellPoint + float2(5.30000019, 9.69999981);
        float _378 = hash21(param_4);
        float craterSize = 0.150000006 + (0.25 * _378);
        if (dist < craterSize)
        {
            float crater = smoothstep(craterSize, craterSize * 0.300000012, dist);
            craters = min(craters, 1.0 - (crater * 0.699999988));
        }
    }
    float surface = (largeFeatures * 0.5) + (mediumCraters * 0.5);
    surface *= craters;
    float intensity = lerp(0.300000012, 0.75, surface);
    float limb = 1.0 - smoothstep(0.600000024, 1.0, distanceFromCenter);
    intensity *= (0.400000006 + (0.600000024 * limb));
    intensity *= 1.29999995;
    return clamp(tintColor * intensity, float3(0.0), float3(1.0));
}

[[vk::binding(0, 1)]] ConstantBuffer<Params> params;
[[vk::binding(1, 1)]] ConstantBuffer<AtmosphereParams> atmosphereParams;
[[vk::binding(0, 2)]] SamplerCube skybox;
[shader("fragment")]
FragmentOutput fragmentMain(FragmentInput input)
{
    FragmentOutput output = {};
    float3 dir = normalize(input.TexCoords);
    float3 color = skybox.Sample(input.TexCoords).xyz;
    if (params.hasDayNight == 1)
    {
        float3 normSunDir = normalize(params.sunDirection);
        float3 normMoonDir = normalize(params.moonDirection);
        float sunDot = dot(dir, normSunDir);
        float moonDot = dot(dir, normMoonDir);
        float nightFactor = smoothstep(0.150000006, -0.200000003, params.sunDirection.y);
        if (atmosphereParams.starDensity > 0.0)
        {
            float3 param = dir;
            float param_1 = atmosphereParams.starDensity;
            float param_2 = nightFactor;
            color += generateStars(param, param_1, param_2);
        }
        float sunHorizonFade = smoothstep(-0.150000006, 0.0500000007, params.sunDirection.y);
        if (params.sunDirection.y > (-0.150000006))
        {
            float sizeAdjust = 1.0 - ((atmosphereParams.sunSizeMultiplier - 1.0) * 0.00100000005);
            float sunSize = 0.999499977 * sizeAdjust;
            float sunGlowSize = 0.998000026 * (1.0 - ((atmosphereParams.sunSizeMultiplier - 1.0) * 0.00300000003));
            float sunHaloSize = 0.99000001 * (1.0 - ((atmosphereParams.sunSizeMultiplier - 1.0) * 0.0149999997));
            float sunDisk = smoothstep(sunSize - 0.000199999995, sunSize, sunDot);
            float sunGlow = smoothstep(sunGlowSize, sunSize, sunDot) * (1.0 - sunDisk);
            float sunHalo = smoothstep(sunHaloSize, sunSize, sunDot) * (1.0 - smoothstep(sunSize, sunGlowSize, sunDot));
            float horizonBoost = smoothstep(0.100000001, -0.0500000007, params.sunDirection.y) * 2.0;
            sunHalo *= (0.300000012 + horizonBoost);
            color += ((params.sunColor.xyz * (((sunDisk * 5.0) + (sunGlow * 0.5)) + sunHalo)) * sunHorizonFade);
        }
        float moonHorizonFade = smoothstep(-0.150000006, 0.0500000007, params.moonDirection.y);
        if (params.moonDirection.y > (-0.150000006))
        {
            float sizeAdjust_1 = 1.0 - ((atmosphereParams.moonSizeMultiplier - 1.0) * 0.00100000005);
            float moonSize = 0.999599993 * sizeAdjust_1;
            float moonGlowSize = 0.99849999 * (1.0 - ((atmosphereParams.moonSizeMultiplier - 1.0) * 0.00300000003));
            float moonHaloSize = 0.991999984 * (1.0 - ((atmosphereParams.moonSizeMultiplier - 1.0) * 0.0149999997));
            float moonDisk = smoothstep(moonSize - 0.000199999995, moonSize, moonDot);
            if (moonDisk > 0.00999999978)
            {
                float3 up = (abs(normMoonDir.y) < 0.999000013) ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
                float3 right = normalize(cross(up, normMoonDir));
                float3 actualUp = cross(normMoonDir, right);
                float3 relativeDir = dir - (normMoonDir * moonDot);
                float u = dot(relativeDir, right);
                float v = dot(relativeDir, actualUp);
                float distFromCenter = length(float2(u, v)) / sqrt(1.0 - (moonSize * moonSize));
                if (distFromCenter < 1.0)
                {
                    float2 moonUV = float2(u, v) * 200.0;
                    float2 param_3 = moonUV;
                    float param_4 = distFromCenter;
                    float3 param_5 = params.moonColor.xyz;
                    float3 _703 = generateMoonTexture(param_3, param_4, param_5);
                    float3 moonTexture = _703;
                    color += (((moonTexture * moonDisk) * 1.5) * moonHorizonFade);
                }
                else
                {
                    color += (((params.moonColor.xyz * moonDisk) * 1.5) * moonHorizonFade);
                }
            }
            float moonGlow = smoothstep(moonGlowSize, moonSize, moonDot) * (1.0 - moonDisk);
            float moonHalo = smoothstep(moonHaloSize, moonSize, moonDot) * (1.0 - smoothstep(moonSize, moonGlowSize, moonDot));
            float moonHorizonBoost = smoothstep(0.100000001, -0.0500000007, params.moonDirection.y) * 2.0;
            moonHalo *= (0.200000003 + moonHorizonBoost);
            color += ((params.moonColor.xyz * ((moonGlow * 0.300000012) + moonHalo)) * moonHorizonFade);
        }
        bool _767 = params.sunDirection.y > (-0.100000001);
        bool _773;
        if (_767)
        {
            _773 = atmosphereParams.sunTintStrength > 0.0;
        }
        else
        {
            _773 = _767;
        }
        if (_773)
        {
            float sunSkyInfluence = smoothstep(0.699999988, 0.949999988, sunDot) * smoothstep(-0.100000001, 0.200000003, params.sunDirection.y);
            color = lerp(color, color * params.sunColor.xyz, float3((sunSkyInfluence * 0.300000012) * atmosphereParams.sunTintStrength));
            float sunProximity = smoothstep(0.300000012, 0.800000012, sunDot) * smoothstep(-0.100000001, 0.300000012, params.sunDirection.y);
            color += (((params.sunColor.xyz * sunProximity) * 0.150000006) * atmosphereParams.sunTintStrength);
            float globalSunTint = smoothstep(-0.100000001, 0.5, params.sunDirection.y);
            color = lerp(color, params.sunColor.xyz, float3((globalSunTint * atmosphereParams.sunTintStrength) * 0.0799999982));
        }
        bool _833 = params.moonDirection.y > (-0.100000001);
        bool _839;
        if (_833)
        {
            _839 = atmosphereParams.moonTintStrength > 0.0;
        }
        else
        {
            _839 = _833;
        }
        if (_839)
        {
            float moonSkyInfluence = smoothstep(0.800000012, 0.949999988, moonDot) * smoothstep(-0.100000001, 0.200000003, params.moonDirection.y);
            color = lerp(color, color * params.moonColor.xyz, float3((moonSkyInfluence * 0.200000003) * atmosphereParams.moonTintStrength));
            float moonProximity = smoothstep(0.5, 0.850000024, moonDot) * smoothstep(-0.100000001, 0.300000012, params.moonDirection.y);
            color += (((params.moonColor.xyz * moonProximity) * 0.0799999982) * atmosphereParams.moonTintStrength);
            float globalMoonTint = smoothstep(-0.100000001, 0.5, params.moonDirection.y);
            color = lerp(color, params.moonColor.xyz, float3((globalMoonTint * atmosphereParams.moonTintStrength) * 0.0500000007));
        }
    }
    output.FragColor = float4(color, 1.0);
    return output;
}
#endif
