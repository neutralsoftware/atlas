module Rendering;

import Image;

public float4 applyFXAA(ImageSampleState state) {
    float2 texelSize = 1.0 / state.image.dimensions();

    const float FXAA_SPAN_MAX = 8.0;
    const float FXAA_REDUCE_MUL = 1.0 / 8.0;
    const float FXAA_REDUCE_MIN = 1.0 / 128.0;

    float3 rgbNW =
        state.image.sample(state.uv + float2(-1.0, -1.0) * texelSize).rgb;
    float3 rgbNE =
        state.image.sample(state.uv + float2(1.0, -1.0) * texelSize).rgb;
    float3 rgbSW =
        state.image.sample(state.uv + float2(-1.0, 1.0) * texelSize).rgb;
    float3 rgbSE =
        state.image.sample(state.uv + float2(1.0, 1.0) * texelSize).rgb;
    float3 rgbM = state.image.sample(state.uv).rgb;

    float3 luma = float3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce =
        max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
            FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(float2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) *
          texelSize;

    float3 rgbA =
        0.5 * (state.image.sample(state.uv + dir * (1.0 / 3.0 - 0.5)).rgb +
               state.image.sample(state.uv + dir * (2.0 / 3.0 - 0.5)).rgb);

    float3 rgbB =
        rgbA * 0.5 + 0.25 * (state.image.sample(state.uv + dir * -0.5).rgb +
                             state.image.sample(state.uv + dir * 0.5).rgb);

    float lumaB = dot(rgbB, luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        return float4(rgbA, 1.0);
    } else {
        return float4(rgbB, 1.0);
    }
}

