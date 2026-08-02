#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

const int TEXTURE_COLOR = 0;
const int TEXTURE_DEPTH = 3;
const int TEXTURE_CUBE_DEPTH = 4;

const int EFFECT_INVERSION = 0;
const int EFFECT_GRAYSCALE = 1;
const int EFFECT_SHARPEN = 2;
const int EFFECT_BLUR = 3;
const int EFFECT_EDGE_DETECTION = 4;
const int EFFECT_COLOR_CORRECTION = 5;
const int EFFECT_MOTION_BLUR = 6;
const int EFFECT_CHROMATIC_ABERRATION = 7;
const int EFFECT_POSTERIZATION = 8;
const int EFFECT_PIXELATION = 9;
const int EFFECT_DILATION = 10;
const int EFFECT_FILM_GRAIN = 11;

const float offset = 1.0 / 300.0;
const float exposure = 1.0;

vec3 reinhardToneMapping(vec3 hdrColor) {
    vec3 color = vec3(1.0) - exp(-hdrColor * 1.0);
    color = pow(color, vec3(1.0 / 2.2));
    return color;
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec4 sharpen(sampler2D image) {
    vec2 offsets[9] = vec2[](
            vec2(-offset, offset),
            vec2(0.0f, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(offset, 0.0f),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
        );

    float kernel[9] = float[](
            -1, -1, -1,
            -1, 9, -1,
            -1, -1, -1
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }

    return vec4(col, 1.0);
}

vec4 blur(sampler2D image, float radius) {
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = vec3(0.0);
    float total = 0.0;

    float sigma = radius * 0.5;
    float twoSigmaSq = 2.0 * sigma * sigma;

    for (int x = -int(radius); x <= int(radius); x++) {
        float weight = exp(-(x * x) / twoSigmaSq);
        vec2 offset = vec2(x, 0.0) * texelSize;
        result += texture(image, TexCoord + offset).rgb * weight;
        total += weight;
    }

    result /= total;

    return vec4(result, 1.0);
}

vec4 edgeDetection(sampler2D image) {
    vec2 offsets[9] = vec2[](
            vec2(-offset, offset),
            vec2(0.0f, offset),
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(offset, 0.0f),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
        );

    float kernel[9] = float[](
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
        );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(image, TexCoord.st + offsets[i]));
    }

    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }

    return vec4(col, 1.0);
}

uniform sampler2D Texture;
uniform sampler2D BrightTexture;
uniform sampler2D DepthTexture;
uniform sampler2D VolumetricLightTexture;
uniform sampler2D PositionTexture;
uniform sampler2D LUTTexture;
uniform sampler2D SSRTexture;
uniform int hasBrightTexture;
uniform int hasDepthTexture;
uniform int hasVolumetricLightTexture;
uniform int hasPositionTexture;
uniform int hasLUTTexture;
uniform int hasSSRTexture;
uniform float lutSize;
uniform samplerCube cubeMap;
uniform bool isCubeMap;
uniform int TextureType;
uniform int EffectCount;
uniform int Effects[10];
uniform float EffectFloat1[10];
uniform float EffectFloat2[10];
uniform float EffectFloat3[10];
uniform float EffectFloat4[10];
uniform float EffectFloat5[10];
uniform float EffectFloat6[10];

struct Environment {
    vec3 fogColor;
    float fogIntensity;
};

uniform Environment environment;

uniform mat4 invProjectionMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 lastViewMatrix;

uniform float nearPlane = 0.1;
uniform float farPlane = 100.0;

uniform float focusDepth;
uniform float focusRange;

uniform int maxMipLevel;
uniform float deltaTime;
uniform float time;

uniform sampler3D cloudsTexture;
uniform vec3 cloudPosition;
uniform vec3 cloudSize;
uniform vec3 cameraPosition;
uniform float cloudScale;
uniform vec3 cloudOffset;
uniform float cloudDensityThreshold;
uniform float cloudDensityMultiplier;
uniform float cloudAbsorption;
uniform float cloudScattering;
uniform float cloudPhaseG;
uniform float cloudClusterStrength;
uniform int cloudPrimarySteps;
uniform int cloudLightSteps;
uniform float cloudLightStepMultiplier;
uniform float cloudMinStepLength;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform vec3 cloudAmbientColor;
uniform int hasClouds;

vec4 sampleColor(vec2 uv) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_CHROMATIC_ABERRATION) {
            float redOffset = EffectFloat1[i];
            float greenOffset = EffectFloat2[i];
            float blueOffset = EffectFloat3[i];
            vec2 focusPoint = vec2(EffectFloat4[i], EffectFloat5[i]);
            vec2 sampleCoord = uv;
            vec2 direction = sampleCoord - focusPoint;
            float red = texture(Texture, sampleCoord + (direction * redOffset)).r;
            float green = texture(Texture, sampleCoord + (direction * greenOffset)).g;
            vec2 blue = texture(Texture, sampleCoord + (direction * blueOffset)).ba;
            return vec4(red, green, blue);
        } else if (Effects[i] == EFFECT_PIXELATION) {
            float pixelSizeInPixels = EffectFloat1[i];
            vec2 texSize = vec2(textureSize(Texture, 0));

            vec2 pixelSize = vec2(pixelSizeInPixels) / texSize;

            vec2 pixelated = floor(uv / pixelSize) * pixelSize;

            return texture(Texture, pixelated);
        } else if (Effects[i] == EFFECT_DILATION) {
            float radius = EffectFloat1[i];
            float separation = EffectFloat2[i];
            vec2 texelSize = 1.0 / vec2(textureSize(Texture, 0));

            vec3 maxColor = texture(Texture, uv).rgb;
            int range = int(radius);
            float radiusSq = radius * radius;

            for (int x = -range; x <= range; x++) {
                for (int y = -range; y <= range; y++) {
                    float distSq = float(x * x + y * y);
                    if (distSq <= radiusSq) {
                        vec2 offset = vec2(float(x), float(y)) * texelSize * separation;
                        vec3 sampled = texture(Texture, uv + offset).rgb;
                        maxColor = max(maxColor, sampled);
                    }
                }
            }

            return vec4(maxColor, texture(Texture, uv).a);
        }
    }

    return texture(Texture, uv);
}

vec4 sampleBright(vec2 uv) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_CHROMATIC_ABERRATION) {
            float redOffset = EffectFloat1[i];
            float greenOffset = EffectFloat2[i];
            float blueOffset = EffectFloat3[i];
            vec2 focusPoint = vec2(EffectFloat4[i], EffectFloat5[i]);
            vec2 sampleCoord = uv;
            vec2 direction = sampleCoord - focusPoint;
            float red = texture(BrightTexture, sampleCoord + (direction * redOffset)).r;
            float green = texture(BrightTexture, sampleCoord + (direction * greenOffset)).g;
            vec2 blue = texture(BrightTexture, sampleCoord + (direction * blueOffset)).ba;
            return vec4(red, green, blue);
        } else if (Effects[i] == EFFECT_PIXELATION) {
            float pixelSizeInPixels = EffectFloat1[i];
            vec2 texSize = vec2(textureSize(BrightTexture, 0));

            vec2 pixelSize = vec2(pixelSizeInPixels) / texSize;

            vec2 pixelated = floor(uv / pixelSize) * pixelSize;
            vec4 color = texture(BrightTexture, pixelated);
            return color;
        } else if (Effects[i] == EFFECT_DILATION) {
            float radius = EffectFloat1[i];
            float separation = EffectFloat2[i];
            vec2 texelSize = 1.0 / vec2(textureSize(BrightTexture, 0));

            vec3 maxColor = texture(BrightTexture, uv).rgb;
            int range = int(radius);
            float radiusSq = radius * radius;

            for (int x = -range; x <= range; x++) {
                for (int y = -range; y <= range; y++) {
                    float distSq = float(x * x + y * y);
                    if (distSq <= radiusSq) {
                        vec2 offset = vec2(float(x), float(y)) * texelSize * separation;
                        vec3 sampled = texture(BrightTexture, uv + offset).rgb;
                        maxColor = max(maxColor, sampled);
                    }
                }
            }

            return vec4(maxColor, texture(BrightTexture, uv).a);
        }
    }

    return texture(BrightTexture, uv);
}

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    float linear = (2.0 * nearPlane * farPlane) /
            (farPlane + nearPlane - z * (farPlane - nearPlane));
    return linear / farPlane;
}

vec3 reconstructViewPos(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewPos = invProjectionMatrix * clipPos;
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

struct ChromaticAberrationSettings {
    float redOffset;
    float greenOffset;
    float blueOffset;
    vec2 focusPoint;
    bool enabled;
};

struct ColorCorrection {
    float exposure;
    float contrast;
    float saturation;
    float gamma;
    float temperature;
    float tint;
};

vec4 applyColorCorrection(vec4 color, ColorCorrection cc) {
    vec3 linearColor = color.rgb;

    linearColor *= pow(2.0, cc.exposure);

    linearColor = (linearColor - 0.5) * cc.contrast + 0.5;

    linearColor.r += cc.temperature * 0.05;
    linearColor.g += cc.tint * 0.05;

    float luminance = dot(linearColor, vec3(0.2126, 0.7152, 0.0722));
    linearColor = mix(vec3(luminance), linearColor, cc.saturation);

    linearColor = clamp(linearColor, 0.0, 1.0);

    return vec4(linearColor, color.a);
}

vec4 applyColorEffects(vec4 color) {
    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_INVERSION) {
            color = vec4(1.0 - color.rgb, color.a);
        } else if (Effects[i] == EFFECT_GRAYSCALE) {
            float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
            color = vec4(average, average, average, color.a);
        } else if (Effects[i] == EFFECT_COLOR_CORRECTION) {
            ColorCorrection cc;
            cc.exposure = EffectFloat1[i];
            cc.contrast = EffectFloat2[i];
            cc.saturation = EffectFloat3[i];
            cc.gamma = EffectFloat4[i];
            cc.temperature = EffectFloat5[i];
            cc.tint = EffectFloat6[i];
            color = applyColorCorrection(color, cc);
        } else if (Effects[i] == EFFECT_POSTERIZATION) {
            float levels = max(EffectFloat1[i], 1.0);
            float grayscale = max(color.r, max(color.g, color.b));
            if (grayscale > 1e-4) {
                float lower = floor(grayscale * levels) / levels;
                float lowerDiff = abs(grayscale - lower);
                float upper = ceil(grayscale * levels) / levels;
                float upperDiff = abs(upper - grayscale);
                float level = lowerDiff <= upperDiff ? lower : upper;
                float adjustment = level / max(grayscale, 1e-4);
                color = adjustment * color;
            }
        } else if (Effects[i] == EFFECT_FILM_GRAIN) {
            float amount = EffectFloat1[i];

            vec3 seed = vec3(gl_FragCoord.xy, deltaTime * 100.0);

            float n = dot(seed, vec3(12.9898, 78.233, 45.164));
            float noise = fract(sin(n) * 43758.5453);
            float grain = (noise - 0.5) * 2.0 * amount;

            float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
            float visibility = 1.0 - abs(luminance - 0.5) * 0.5;

            color.rgb += vec3(grain * visibility);
            color.rgb = clamp(color.rgb, 0.0, 1.0);
        }
    }

    return color;
}

vec4 applyFXAA(sampler2D tex, vec2 texCoord) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);

    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0 / 8.0;
    float FXAA_REDUCE_MIN = 1.0 / 128.0;

    vec3 rgbNW = sampleColor(texCoord + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = sampleColor(texCoord + vec2(1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = sampleColor(texCoord + vec2(-1.0, 1.0) * texelSize).rgb;
    vec3 rgbSE = sampleColor(texCoord + vec2(1.0, 1.0) * texelSize).rgb;
    vec3 rgbM = sampleColor(texCoord).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                dir * rcpDirMin)) * texelSize;

    vec3 rgbA = 0.5 * (
            sampleColor(texCoord + dir * (1.0 / 3.0 - 0.5)).rgb +
                sampleColor(texCoord + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
                sampleColor(texCoord + dir * -0.5).rgb +
                    sampleColor(texCoord + dir * 0.5).rgb);

    float lumaB = dot(rgbB, luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return vec4(rgbA, 1.0);
    } else {
        return vec4(rgbB, 1.0);
    }
}

vec4 composeSSR(vec4 color, vec2 uv) {
    if (hasSSRTexture != 1) {
        return color;
    }
    vec4 ssr = texture(SSRTexture, uv);
    float reflectionWeight = clamp(ssr.a, 0.0, 1.0);
    float baseWeight = 1.0 - reflectionWeight;
    color.rgb = color.rgb * baseWeight + ssr.rgb * reflectionWeight;
    return color;
}

vec4 composeLighting(vec2 uv, vec4 baseColor) {
    vec4 color = baseColor;
    if (hasBrightTexture == 1) {
        color += sampleBright(uv);
    }
    if (hasVolumetricLightTexture == 1) {
        color += texture(VolumetricLightTexture, uv);
    }
    return composeSSR(color, uv);
}

vec4 applyMotionBlur(vec2 texCoord, float size, float separation, vec4 color) {
    vec4 fallbackColor = composeLighting(texCoord, color);
    if (size <= 0.0 || separation <= 0.0) {
        return fallbackColor;
    }
    if (hasPositionTexture != 1) {
        return fallbackColor;
    }

    vec4 worldPos = texture(PositionTexture, texCoord);
    if (worldPos.a <= 0.0) {
        return fallbackColor;
    }

    vec3 viewSpacePos = (viewMatrix * worldPos).xyz;
    float distanceToCamera = length(viewSpacePos);

    if (distanceToCamera < nearPlane * 2.0) {
        return fallbackColor;
    }

    vec4 currentClipPos = projectionMatrix * viewMatrix * worldPos;
    currentClipPos.xyz /= currentClipPos.w;
    vec2 currentUV = currentClipPos.xy * 0.5 + 0.5;

    vec4 prevClipPos = projectionMatrix * lastViewMatrix * worldPos;
    prevClipPos.xyz /= prevClipPos.w;
    vec2 prevUV = prevClipPos.xy * 0.5 + 0.5;

    vec2 velocity = (currentUV - prevUV) * separation;
    float velocityLength = length(velocity);

    float maxVelocity = 0.12;
    if (velocityLength > maxVelocity) {
        velocity = normalize(velocity) * maxVelocity;
        velocityLength = maxVelocity;
    }
    if (hasSSRTexture == 1) {
        float mirrorWeight = clamp(texture(SSRTexture, texCoord).a, 0.0, 1.0);
        float blurDamp = mix(1.0, 0.2, mirrorWeight);
        velocity *= blurDamp;
        velocityLength *= blurDamp;
    }

    if (velocityLength < 0.0001) {
        return fallbackColor;
    }

    int baseSamples = clamp(int(size), 2, 32);
    float sampleScale =
        mix(0.75, 1.75, clamp(velocityLength / maxVelocity, 0.0, 1.0));
    int samples = clamp(int(float(baseSamples) * sampleScale), 3, 48);

    float centerDepth = 0.0;
    float depthSoftness = 1.0;
    if (hasDepthTexture == 1) {
        centerDepth = LinearizeDepth(texture(DepthTexture, texCoord).r);
        depthSoftness = max(0.002, centerDepth * 0.02);
    }

    float jitter = (fract(sin(dot(texCoord * vec2(173.3, 97.1),
                                  vec2(12.9898, 78.233))) *
                          43758.5453) -
                    0.5) *
        0.35;

    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < samples; i++) {
        float t = ((float(i) + 0.5) / float(samples)) * 2.0 - 1.0;
        t += jitter / float(samples);
        vec2 sampleCoord = texCoord + velocity * t;

        if (sampleCoord.x < 0.0 || sampleCoord.x > 1.0 ||
            sampleCoord.y < 0.0 || sampleCoord.y > 1.0) {
            continue;
        }

        float depthWeight = 1.0;
        if (hasDepthTexture == 1) {
            float sampleDepth = LinearizeDepth(texture(DepthTexture, sampleCoord).r);
            float depthDelta = abs(sampleDepth - centerDepth);
            depthWeight =
                1.0 - smoothstep(depthSoftness, depthSoftness * 2.5, depthDelta);
            if (depthWeight <= 0.0) {
                continue;
            }
        }

        vec4 sampled = composeLighting(sampleCoord, sampleColor(sampleCoord));
        float gaussian = exp(-4.0 * t * t);
        float weight = gaussian * depthWeight;
        result += sampled * weight;
        totalWeight += weight;
    }

    if (totalWeight > 0.0) {
        return result / totalWeight;
    }

    return fallbackColor;
}

vec3 sampleLUT(vec3 rgb, float blueSlice, float sliceSize, float slicePixelOffset) {
    float sliceY = floor(blueSlice / lutSize);
    float sliceX = mod(blueSlice, lutSize);

    vec2 uv;
    uv.x = sliceX * sliceSize + slicePixelOffset + rgb.r * sliceSize;
    uv.y = sliceY * sliceSize + slicePixelOffset + rgb.g * sliceSize;

    return texture(LUTTexture, uv).rgb;
}

vec4 mapToLUT(vec4 color) {
    if (hasLUTTexture != 1) {
        return color;
    }

    float sliceSize = 1.0 / lutSize;
    float slicePixelOffset = sliceSize * 0.5;

    float blueIndex = color.b * (lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = min(sliceLow + 1.0, lutSize - 1.0);
    float t = blueIndex - sliceLow;

    vec3 lowColor = sampleLUT(color.rgb, sliceLow, sliceSize, slicePixelOffset);
    vec3 highColor = sampleLUT(color.rgb, sliceHigh, sliceSize, slicePixelOffset);

    vec3 finalRGB = mix(lowColor, highColor, t);

    return vec4(finalRGB, color.a);
}

vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, vec3 rayOrigin, vec3 rayDir) {
    vec3 t0 = (boundsMin - rayOrigin) / rayDir;
    vec3 t1 = (boundsMax - rayOrigin) / rayDir;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);

    float dstA = max(max(tMin.x, tMin.y), tMin.z);
    float dstB = min(tMax.x, min(tMax.y, tMax.z));

    float dstToContainer = max(0.0, dstA);
    float dstInsideContainer = max(0.0, dstB - dstToContainer);

    return vec2(dstToContainer, dstInsideContainer);
}

float saturate(float v) { return clamp(v, 0.0, 1.0); }

float hashNoise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

float henyeyGreenstein(float cosTheta, float g) {
    const float PI = 3.14159265359;
    float g2 = g * g;
    float denom = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 - g2) / (4.0 * PI * max(denom, 1e-4));
}

float calculateCloudDensity(vec3 worldPos) {
    vec3 halfExtents = max(cloudSize * 0.5, vec3(1e-4));
    vec3 localPos = (worldPos - cloudPosition) / halfExtents;

    if (any(lessThan(localPos, vec3(-1.0))) ||
        any(greaterThan(localPos, vec3(1.0)))) {
        return 0.0;
    }

    vec3 uvw = localPos * 0.5 + 0.5;
    float scale = max(cloudScale, 0.001);
    vec3 noiseCoord = fract(uvw * scale + cloudOffset);

    vec4 shape = texture(cloudsTexture, noiseCoord);
    
    float cluster = saturate(cloudClusterStrength);

    float lowerFade = smoothstep(-0.95, -0.6, localPos.y);
    float upperFade = 1.0 - smoothstep(0.35, 0.95, localPos.y);
    float verticalMask = lowerFade * upperFade;

    float coverageThreshold = mix(0.6, 0.28, cluster);
    float coverageSoftness = mix(0.22, 0.34, cluster);
    float coverageNoise = mix(shape.r, shape.a, 0.4 + cluster * 0.35);
    float coverage = smoothstep(coverageThreshold,
                                coverageThreshold + coverageSoftness,
                                coverageNoise);
    coverage = pow(coverage, mix(2.0, 0.7, cluster));

    float detail = mix(smoothstep(0.25, 0.75, shape.g), 
                       smoothstep(0.2, 0.9, shape.a), 0.55);
    detail = pow(detail, mix(1.6, 0.85, cluster));

    float cavityNoise = smoothstep(0.22, 0.85, shape.b);
    float gapMask = clamp(1.0 - cavityNoise * mix(0.25, 0.7, cluster), 0.0, 1.0);

    float density = coverage * mix(detail, 1.0, cluster * 0.35);
    density = max(density * gapMask * verticalMask - 0.02, 0.0);
    density *= max(cloudDensityMultiplier, 0.0);

    return density;
}

float sampleSunTransmittance(vec3 worldPos, float stepSize) {
    vec3 lightDir = -sunDirection;
    float dirLength = length(lightDir);
    if (dirLength < 1e-3) {
        lightDir = vec3(0.0, 1.0, 0.0);
    } else {
        lightDir /= dirLength;
    }

    float maxDistance = length(cloudSize) * 1.5;
    float travel = 0.0;
    float attenuation = 1.0;
    float lightStep = max(stepSize * cloudLightStepMultiplier, cloudMinStepLength);

    int steps = max(cloudLightSteps, 1);
    for (int i = 0; i < steps && attenuation > 0.05; ++i) {
        travel += lightStep;
        if (travel > maxDistance)
            break;

        vec3 samplePos = worldPos + lightDir * travel;
        float density = calculateCloudDensity(samplePos);
        attenuation *= exp(-density * lightStep * cloudAbsorption);
        lightStep = max(lightStep * cloudLightStepMultiplier, cloudMinStepLength);
    }

    return attenuation;
}

vec4 cloudRendering(vec4 inColor) {
    if (hasClouds != 1) {
        return inColor;
    }

    float nonLinearDepth = hasDepthTexture == 1
                               ? texture(DepthTexture, TexCoord).r
                               : 1.0;
    bool depthAvailable = hasDepthTexture == 1 && nonLinearDepth < 1.0;
    float depthSample = depthAvailable ? nonLinearDepth : 1.0;

    vec3 rayOrigin = cameraPosition;

    vec4 clipSpace = vec4(TexCoord * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec4 viewSpace = invProjectionMatrix * clipSpace;
    viewSpace /= viewSpace.w;
    vec3 worldPos = (invViewMatrix * vec4(viewSpace.xyz, 1.0)).xyz;

    vec3 rayDir = normalize(worldPos - rayOrigin);

    float sceneDistance = depthAvailable ? length(worldPos - rayOrigin) : 1e6;

    vec3 boundsMin = cloudPosition - cloudSize * 0.5;
    vec3 boundsMax = cloudPosition + cloudSize * 0.5;
    vec2 rayBoxInfo = rayBoxDst(boundsMin, boundsMax, rayOrigin, rayDir);

    float distToContainer = rayBoxInfo.x;
    float distInContainer = rayBoxInfo.y;

    if (distInContainer <= 0.0) {
        return inColor;
    }

    float dstLimit = min(sceneDistance - distToContainer, distInContainer);
    dstLimit = max(dstLimit, 0.0);
    if (dstLimit <= 1e-4) {
        return inColor;
    }

    int steps = max(cloudPrimarySteps, 8);
    float baseStep = dstLimit / float(steps);
    float stepSize = max(baseStep, cloudMinStepLength);

    float jitter = hashNoise(vec3(TexCoord, time)) - 0.5;
    float travelled = clamp(jitter, -0.35, 0.35) * stepSize;
    travelled = max(travelled, 0.0);

    vec3 accumulatedLight = vec3(0.0);
    float transmittance = 1.0;

    vec3 sunDir = sunDirection;
    float sunLen = length(sunDir);
    if (sunLen > 1e-3) {
        sunDir /= sunLen;
    } else {
        sunDir = vec3(0.0, 1.0, 0.0);
    }

    float phaseG = clamp(cloudPhaseG, -0.95, 0.95);

    for (int step = 0; step < steps && travelled < dstLimit;
         ++step) {
        if (transmittance <= 0.01) {
            break;
        }

        float remainingDistance = dstLimit - travelled;
        if (remainingDistance <= 1e-5) {
            break;
        }

        float current = distToContainer + travelled;
        vec3 samplePos = rayOrigin + rayDir * current;

        float density = calculateCloudDensity(samplePos);
        if (density > 1e-4) {
            float adaptiveStep = stepSize;
            if (density < 0.02) {
                adaptiveStep = stepSize * 2.5;
            } else if (density < 0.05) {
                adaptiveStep = stepSize * 1.6;
            }
            adaptiveStep = min(adaptiveStep, remainingDistance);
            float sampleWeight = density * adaptiveStep;

            float lightTrans = sampleSunTransmittance(samplePos, adaptiveStep);
            float cosTheta = clamp(dot(rayDir, -sunDir), -1.0, 1.0);
            float phase = henyeyGreenstein(cosTheta, phaseG);

            vec3 directLight = sunColor * sunIntensity * lightTrans * phase;
            vec3 ambientLight = cloudAmbientColor;

            vec3 lighting = (ambientLight * 0.35 + directLight) *
                            sampleWeight * cloudScattering;

            accumulatedLight += lighting * transmittance;
            transmittance *= exp(-density * adaptiveStep * cloudAbsorption);
            travelled += adaptiveStep;
            continue;
        }

        float emptyAdvance = min(stepSize * 2.25, remainingDistance);
        float minAdvance = min(stepSize * 0.5, remainingDistance);
        travelled += max(emptyAdvance, minAdvance);
    }

    vec3 finalColor = inColor.rgb * transmittance + accumulatedLight;
    return vec4(clamp(finalColor, 0.0, 1.0), inColor.a);
}

void main() {
    vec4 color = sampleColor(TexCoord);
    float depth = texture(DepthTexture, TexCoord).r;
    vec3 viewPos = reconstructViewPos(TexCoord, depth);
    float distance = length(viewPos);

    bool useMotionBlur = false;
    float motionBlurSize = 0.0;
    float motionBlurSeparation = 0.0;

    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_MOTION_BLUR) {
            useMotionBlur = true;
            motionBlurSize = EffectFloat1[i];
            motionBlurSeparation = EffectFloat2[i];
        }
    }

    for (int i = 0; i < EffectCount; i++) {
        if (Effects[i] == EFFECT_SHARPEN) {
            color = sharpen(Texture);
        } else if (Effects[i] == EFFECT_BLUR) {
            float radius = EffectFloat1[i];
            color = blur(Texture, radius);
        } else if (Effects[i] == EFFECT_EDGE_DETECTION) {
            color = edgeDetection(Texture);
        }
    }

    color = applyFXAA(Texture, TexCoord);

    color = applyColorEffects(color);

    if (hasDepthTexture == 1) {
        float depthValue = texture(DepthTexture, TexCoord).r;
        float linearDepth = LinearizeDepth(depthValue);
        float coc = clamp(abs(linearDepth - focusDepth) / focusRange, 0.0, 1.0);
        float mip = coc * float(maxMipLevel) * 1.2;
        vec3 blurred = applyColorEffects(textureLod(Texture, TexCoord, mip)).rgb;
        vec3 sharp = color.rgb;
        color = cloudRendering(color);
    } else {
        color = cloudRendering(color);
    }

    vec4 hdrColor;
    if (useMotionBlur) {
        vec4 motionBlurred = applyMotionBlur(TexCoord, motionBlurSize, motionBlurSeparation, color);
        FragColor = motionBlurred;
        return;
    } else {
        hdrColor = composeLighting(TexCoord, color);
    }

    hdrColor = mapToLUT(hdrColor);
    

    hdrColor.rgb = acesToneMapping(hdrColor.rgb);


    float fogFactor = 1.0 - exp(-distance * environment.fogIntensity);
    vec3 finalColor = mix(hdrColor.rgb, environment.fogColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
