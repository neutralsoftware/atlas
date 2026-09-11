float4 sampleColor(float2  uv)
{
    for (int i = 0; i < parameters.EffectCount; i++)
    {
        if (EffectBuffer[i] == 7)
        {
            float redOffset = EffectFloat1Buffer[i];
            float greenOffset = EffectFloat2Buffer[i];
            float blueOffset = EffectFloat3Buffer[i];
            float2 focusPoint = float2(EffectFloat4Buffer[i], EffectFloat5Buffer[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = Texture.Sample((sampleCoord + (direction * redOffset))).x;
            float green = Texture.Sample((sampleCoord + (direction * greenOffset))).y;
            float2 blue = Texture.Sample((sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (EffectBuffer[i] == 9)
            {
                float pixelSizeInPixels = EffectFloat1Buffer[i];
                float2 texSize = float2(int2(textureSize(Texture).x, textureSize(Texture).y));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                return Texture.Sample(pixelated);
            }
            else
            {
                if (EffectBuffer[i] == 10)
                {
                    float radius = EffectFloat1Buffer[i];
                    float separation = EffectFloat2Buffer[i];
                    float2 texelSize = float2(1.0) / float2(int2(textureSize(Texture).x, textureSize(Texture).y));
                    float3 maxColor = Texture.Sample(uv).xyz;
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
                                float3 sampled = Texture.Sample((uv + offset)).xyz;
                                maxColor = max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, Texture.Sample(uv).w);
                }
            }
        }
    }
    return Texture.Sample(uv);
}

float3 reconstructViewPos(float2  uv, float  depth)
{
    float z = (depth * 2.0) - 1.0;
    float4 clipPos = float4((uv * 2.0) - float2(1.0), z, 1.0);
    float4 viewPos = mul(clipPos, uniforms.invProjectionMatrix);
    viewPos /= float4(viewPos.w);
    return viewPos.xyz;
}

float4 sharpen(Sampler2D image, inout float2  TexCoord)
{
    float3 sampleTex[9];
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.Sample((TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _171[i_1]);
    }
    return float4(col, 1.0);
}

float4 blur(Sampler2D image, float  radius, inout float2  TexCoord)
{
    float2 texelSize = float2(1.0) / float2(int2(textureSize(image).x, textureSize(image).y));
    float3 result = float3(0.0);
    float total = 0.0;
    float sigma = radius * 0.5;
    float twoSigmaSq = (2.0 * sigma) * sigma;
    int _257 = -int(radius);
    for (int x = _257; x <= int(radius); x++)
    {
        float weight = exp(float(-(x * x)) / twoSigmaSq);
        float2 offset = float2(float(x), 0.0) * texelSize;
        result += (image.Sample((TexCoord + offset)).xyz * weight);
        total += weight;
    }
    result /= float3(total);
    return float4(result, 1.0);
}

float4 edgeDetection(Sampler2D image, inout float2  TexCoord)
{
    float3 sampleTex[9];
    for (int i = 0; i < 9; i++)
    {
        sampleTex[i] = float3(image.Sample((TexCoord + _165[i])).xyz);
    }
    float3 col = float3(0.0);
    for (int i_1 = 0; i_1 < 9; i_1++)
    {
        col += (sampleTex[i_1] * _311[i_1]);
    }
    return float4(col, 1.0);
}

float4 applyFXAA(Sampler2D tex, float2  texCoord)
{
    float2 texelSize = float2(1.0) / float2(int2(textureSize(tex).x, textureSize(tex).y));
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 0.125;
    float FXAA_REDUCE_MIN = 0.0078125;
    float2 param = texCoord + (float2(-1.0) * texelSize);
    float3 rgbNW = sampleColor(param).xyz;
    float2 param_1 = texCoord + (float2(1.0, -1.0) * texelSize);
    float3 rgbNE = sampleColor(param_1).xyz;
    float2 param_2 = texCoord + (float2(-1.0, 1.0) * texelSize);
    float3 rgbSW = sampleColor(param_2).xyz;
    float2 param_3 = texCoord + (float2(1.0) * texelSize);
    float3 rgbSE = sampleColor(param_3).xyz;
    float2 param_4 = texCoord;
    float3 rgbM = sampleColor(param_4).xyz;
    float3 luma = float3(0.298999995, 0.587000012, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);
    float dirReduce = max((((lumaNW + lumaNE) + lumaSW) + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(float2(FXAA_SPAN_MAX), max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;
    float2 param_5 = texCoord + (dir * (-0.166666672));
    float2 param_6 = texCoord + (dir * 0.166666672);
    float3 rgbA = (sampleColor(param_5).xyz + sampleColor(param_6).xyz) * 0.5;
    float2 param_7 = texCoord + (dir * (-0.5));
    float2 param_8 = texCoord + (dir * 0.5);
    float3 rgbB = (rgbA * 0.5) + ((sampleColor(param_7).xyz + sampleColor(param_8).xyz) * 0.25);
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

float4 applyColorCorrection(float4  color, ColorCorrection  cc)
{
    float3 linearColor = color.xyz;
    linearColor *= powr(2.0, cc.exposure);
    linearColor = ((linearColor - float3(0.5)) * cc.contrast) + float3(0.5);
    linearColor.x += (cc.temperature * 0.0500000007);
    linearColor.y += (cc.tint * 0.0500000007);
    float luminance = dot(linearColor, float3(0.212599993, 0.715200007, 0.0722000003));
    linearColor = lerp(float3(luminance), linearColor, float3(cc.saturation));
    linearColor = clamp(linearColor, float3(0.0), float3(1.0));
    return float4(linearColor, color.w);
}

float4 applyColorEffects(inout float4  color, inout float4  gl_FragCoord)
{
    ColorCorrection cc;
    for (int i = 0; i < parameters.EffectCount; i++)
    {
        if (EffectBuffer[i] == 0)
        {
            color = float4(float3(1.0) - color.xyz, color.w);
        }
        else
        {
            if (EffectBuffer[i] == 1)
            {
                float average = ((0.212599993 * color.x) + (0.715200007 * color.y)) + (0.0722000003 * color.z);
                color = float4(average, average, average, color.w);
            }
            else
            {
                if (EffectBuffer[i] == 5)
                {
                    cc.exposure = EffectFloat1Buffer[i];
                    cc.contrast = EffectFloat2Buffer[i];
                    cc.saturation = EffectFloat3Buffer[i];
                    cc.gamma = EffectFloat4Buffer[i];
                    cc.temperature = EffectFloat5Buffer[i];
                    cc.tint = EffectFloat6Buffer[i];
                    float4 param = color;
                    ColorCorrection param_1 = cc;
                    color = applyColorCorrection(param, param_1);
                }
                else
                {
                    if (EffectBuffer[i] == 8)
                    {
                        float levels = max(EffectFloat1Buffer[i], 1.0);
                        float grayscale = max(color.x, max(color.y, color.z));
                        if (grayscale > 9.99999975e-05)
                        {
                            float lower = floor(grayscale * levels) / levels;
                            float lowerDiff = abs(grayscale - lower);
                            float upper = ceil(grayscale * levels) / levels;
                            float upperDiff = abs(upper - grayscale);
                            float level0 = (lowerDiff <= upperDiff) ? lower : upper;
                            float adjustment = level0 / max(grayscale, 9.99999975e-05);
                            color *= adjustment;
                        }
                    }
                    else
                    {
                        if (EffectBuffer[i] == 11)
                        {
                            float amount = EffectFloat1Buffer[i];
                            float3 seed = float3(gl_FragCoord.xy, uniforms.deltaTime * 100.0);
                            float n = dot(seed, float3(12.9898005, 78.2330017, 45.1640015));
                            float noise = frac(sin(n) * 43758.546875);
                            float grain = ((noise - 0.5) * 2.0) * amount;
                            float luminance = dot(color.xyz, float3(0.298999995, 0.587000012, 0.114));
                            float visibility = 1.0 - (abs(luminance - 0.5) * 0.5);
                            float4 _1210 = color;
                            float3 _1212 = _1210.xyz + float3(grain * visibility);
                            color.x = _1212.x;
                            color.y = _1212.y;
                            color.z = _1212.z;
                            float4 _1219 = color;
                            float3 _1223 = clamp(_1219.xyz, float3(0.0), float3(1.0));
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

float LinearizeDepth(float  depth)
{
    float z = (depth * 2.0) - 1.0;
    float linear = ((2.0 * uniforms.nearPlane) * uniforms.farPlane) / ((uniforms.farPlane + uniforms.nearPlane) - (z * (uniforms.farPlane - uniforms.nearPlane)));
    return linear / uniforms.farPlane;
}

float4 sampleBright(float2  uv)
{
    for (int i = 0; i < parameters.EffectCount; i++)
    {
        if (EffectBuffer[i] == 7)
        {
            float redOffset = EffectFloat1Buffer[i];
            float greenOffset = EffectFloat2Buffer[i];
            float blueOffset = EffectFloat3Buffer[i];
            float2 focusPoint = float2(EffectFloat4Buffer[i], EffectFloat5Buffer[i]);
            float2 sampleCoord = uv;
            float2 direction = sampleCoord - focusPoint;
            float red = BrightTexture.Sample((sampleCoord + (direction * redOffset))).x;
            float green = BrightTexture.Sample((sampleCoord + (direction * greenOffset))).y;
            float2 blue = BrightTexture.Sample((sampleCoord + (direction * blueOffset))).zw;
            return float4(red, green, blue);
        }
        else
        {
            if (EffectBuffer[i] == 9)
            {
                float pixelSizeInPixels = EffectFloat1Buffer[i];
                float2 texSize = float2(int2(textureSize(BrightTexture).x, textureSize(BrightTexture).y));
                float2 pixelSize = float2(pixelSizeInPixels) / texSize;
                float2 pixelated = floor(uv / pixelSize) * pixelSize;
                float4 color = BrightTexture.Sample(pixelated);
                return color;
            }
            else
            {
                if (EffectBuffer[i] == 10)
                {
                    float radius = EffectFloat1Buffer[i];
                    float separation = EffectFloat2Buffer[i];
                    float2 texelSize = float2(1.0) / float2(int2(textureSize(BrightTexture).x, textureSize(BrightTexture).y));
                    float3 maxColor = BrightTexture.Sample(uv).xyz;
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
                                float3 sampled = BrightTexture.Sample((uv + offset)).xyz;
                                maxColor = max(maxColor, sampled);
                            }
                        }
                    }
                    return float4(maxColor, BrightTexture.Sample(uv).w);
                }
            }
        }
    }
    return BrightTexture.Sample(uv);
}

float4 composeSSR(float4  color, float2  uv)
{
    if (parameters.hasSSRTexture != 1)
    {
        return color;
    }
    float4 ssr = SSRTexture.Sample(uv);
    float reflectionWeight = clamp(ssr.w, 0.0, 1.0);
    float baseWeight = 1.0 - reflectionWeight;
    float4 result = color;
    result.xyz = (result.xyz * baseWeight) + (ssr.xyz * reflectionWeight);
    return result;
}

float4 composeLighting(float2  uv, float4  baseColor)
{
    float4 color = baseColor;
    if (parameters.hasBrightTexture == 1)
    {
        color += sampleBright(uv);
    }
    if (parameters.hasVolumetricLightTexture == 1)
    {
        color += VolumetricLightTexture.Sample(uv);
    }
    return composeSSR(color, uv);
}

float4 applyMotionBlur(float2  texCoord, float  size, float  separation, float4  color)
{
    float4 fallbackColor = composeLighting(texCoord, color);
    if ((size <= 0.0) || (separation <= 0.0))
    {
        return fallbackColor;
    }
    if (parameters.hasPositionTexture != 1)
    {
        return fallbackColor;
    }
    float4 worldPos = PositionTexture.Sample(texCoord);
    if (worldPos.w <= 0.0)
    {
        return fallbackColor;
    }
    float3 viewSpacePos = (mul(worldPos, uniforms.viewMatrix)).xyz;
    float distanceToCamera = length(viewSpacePos);
    if (distanceToCamera < (uniforms.nearPlane * 2.0))
    {
        return fallbackColor;
    }
    float4 currentClipPos = mul(worldPos, (mul(uniforms.viewMatrix, uniforms.projectionMatrix)));
    float _1543 = currentClipPos.w;
    float4 _1544 = currentClipPos;
    float3 _1547 = _1544.xyz / float3(_1543);
    currentClipPos.x = _1547.x;
    currentClipPos.y = _1547.y;
    currentClipPos.z = _1547.z;
    float2 currentUV = (currentClipPos.xy * 0.5) + float2(0.5);
    float4 prevClipPos = mul(worldPos, (mul(uniforms.lastViewMatrix, uniforms.projectionMatrix)));
    float _1569 = prevClipPos.w;
    float4 _1570 = prevClipPos;
    float3 _1573 = _1570.xyz / float3(_1569);
    prevClipPos.x = _1573.x;
    prevClipPos.y = _1573.y;
    prevClipPos.z = _1573.z;
    float2 prevUV = (prevClipPos.xy * 0.5) + float2(0.5);
    float2 velocity = (currentUV - prevUV) * separation;
    float velocityLength = length(velocity);
    float maxVelocity = 0.119999997;
    if (velocityLength > maxVelocity)
    {
        velocity = normalize(velocity) * maxVelocity;
        velocityLength = maxVelocity;
    }
    if (parameters.hasSSRTexture == 1)
    {
        float mirrorWeight = clamp(SSRTexture.Sample(texCoord).w, 0.0, 1.0);
        float blurDamp = lerp(1.0, 0.200000003, mirrorWeight);
        velocity *= blurDamp;
        velocityLength *= blurDamp;
    }
    if (velocityLength < 9.99999975e-05)
    {
        return fallbackColor;
    }
    int baseSamples = clamp(int(size), 2, 32);
    float sampleScale = lerp(0.75, 1.75, clamp(velocityLength / maxVelocity, 0.0, 1.0));
    int samples = clamp(int(float(baseSamples) * sampleScale), 3, 48);
    float centerDepth = 0.0;
    float depthSoftness = 1.0;
    if (parameters.hasDepthTexture == 1)
    {
        float centerDepthRaw = DepthTexture.Sample(texCoord).x;
        centerDepth = LinearizeDepth(centerDepthRaw);
        depthSoftness = max(0.00200000009, centerDepth * 0.0199999996);
    }
    float jitter = (frac(sin(dot(texCoord * float2(173.300003, 97.0999985), float2(12.9898005, 78.2330017))) * 43758.546875) - 0.5) * 0.349999994;
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
        if (parameters.hasDepthTexture == 1)
        {
            float sampleDepthRaw = DepthTexture.Sample(sampleCoord).x;
            float sampleDepth = LinearizeDepth(sampleDepthRaw);
            float depthDelta = abs(sampleDepth - centerDepth);
            depthWeight = 1.0 - smoothstep(depthSoftness, depthSoftness * 2.5, depthDelta);
            if (depthWeight <= 0.0)
            {
                continue;
            }
        }
        float4 sampled = sampleColor(sampleCoord);
        sampled = composeLighting(sampleCoord, sampled);
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

float3 sampleLUT(float3  rgb, float  blueSlice, float  sliceSize, float  slicePixelOffset)
{
    float sliceY = floor(blueSlice / parameters.lutSize);
    float sliceX = mod(blueSlice, parameters.lutSize);
    float2 uv;
    uv.x = ((sliceX * sliceSize) + slicePixelOffset) + (rgb.x * sliceSize);
    uv.y = ((sliceY * sliceSize) + slicePixelOffset) + (rgb.y * sliceSize);
    return LUTTexture.Sample(uv).xyz;
}

float4 mapToLUT(float4  color)
{
    if (parameters.hasLUTTexture != 1)
    {
        return color;
    }
    float sliceSize = 1.0 / parameters.lutSize;
    float slicePixelOffset = sliceSize * 0.5;
    float blueIndex = color.z * (parameters.lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = min(sliceLow + 1.0, parameters.lutSize - 1.0);
    float t = blueIndex - sliceLow;
    float3 param = color.xyz;
    float param_1 = sliceLow;
    float param_2 = sliceSize;
    float param_3 = slicePixelOffset;
    float3 lowColor = sampleLUT(param, param_1, param_2, param_3);
    float3 param_4 = color.xyz;
    float param_5 = sliceHigh;
    float param_6 = sliceSize;
    float param_7 = slicePixelOffset;
    float3 highColor = sampleLUT(param_4, param_5, param_6, param_7);
    float3 finalRGB = lerp(lowColor, highColor, float3(t));
    return float4(finalRGB, color.w);
}

float3 acesToneMapping(float3  color)
{
    float a = 2.50999999;
    float b = 0.0299999993;
    float c = 2.43000007;
    float d = 0.589999974;
    float e = 0.140000001;
    return clamp((color * ((color * a) + float3(b))) / ((color * ((color * c) + float3(d))) + float3(e)), float3(0.0), float3(1.0));
}
