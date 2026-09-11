module Image;
import Types;

public struct Image {
    public Texture2D<float4> texture;
    public SamplerState sampler;

    public static Image make(Texture2D<float4> tex, SamplerState samp) {
        Image img;
        img.texture = tex;
        img.sampler = samp;
        return img;
    }

    public float4 sample(float2 uv) { return texture.Sample(sampler, uv); }
    public float2 dimensions() {
        uint width;
        uint height;
        texture.GetDimensions(width, height);
        return float2(width, height);
    }
}

public struct ImageSampleState {
    public Image image;
    public float2 uv;

    public static ImageSampleState make(Image img, float2 uv) {
        ImageSampleState state;
        state.image = img;
        state.uv = uv;
        return state;
    }

    public static ImageSampleState make(Image img, float2 uv, float2 offset) {
        ImageSampleState state;
        state.image = img;
        state.uv = uv + offset;
        return state;
    }

    public static ImageSampleState make(Texture2D<float4> tex,
                                        SamplerState samp, float2 uv) {
        ImageSampleState state;
        state.image = Image.make(tex, samp);
        state.uv = uv;
        return state;
    }
}

public float3 reinhardToneMapping(float3 hdrColor) {
    float3 color = float3(1.0) - exp(-hdrColor);
    color = pow(color, float3(1.0 / 2.2));
    return color;
}

public float3 acesToneMapping(float3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0,
                 1.0);
}

public float3 filmicToneMapping(float3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    color = (color * (A * color + C * B) + D * E) /
            (color * (A * color + B) + D * F);
    return pow(color, float3(1.0 / 2.2));
}

// EFFECTS

float4 kernelEffect(ImageSampleState state, float2 offsets[9],
                    float kernel[9]) {
    float3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = state.image.sample(state.uv + offsets[i]).rgb;
    }

    float3 color = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < 9; i++) {
        color += sampleTex[i] * kernel[i];
    }

    return float4(color, 1.0);
}

public float4 sharpen(ImageSampleState state) {
    const float offset = 1.0 / 300.0;
    float2 offsets[9] = {
        float2(-offset, offset),  // top-left
        float2(0.0f, offset),     // top-center
        float2(offset, offset),   // top-right
        float2(-offset, 0.0f),    // center-left
        float2(0.0f, 0.0f),       // center-center
        float2(offset, 0.0f),     // center-right
        float2(-offset, -offset), // bottom-left
        float2(0.0f, -offset),    // bottom-center
        float2(offset, -offset)   // bottom-right
    };

    float kernel[9] = {
        -1, -1, -1, //
        -1, 9,  -1, //
        -1, -1, -1  //
    };

    return kernelEffect(state, offsets, kernel);
}

public float4 blur(ImageSampleState state, float radius) {
    float2 texelSize = 1.0 / state.image.dimensions();
    float3 result = float3(0.0, 0.0, 0.0);
    float total = 0.0;

    float sigma = radius * 0.5;
    float twoSigmaSq = 2.0 * sigma * sigma;

    for (int x = -int(radius); x <= int(radius); x++) {
        float weight = exp(-(x * x) / twoSigmaSq);
        float2 offset = float2(x, 0.0) * texelSize;
        result += state.image.sample(state.uv + offset).rgb * weight;
        total += weight;
    }

    result /= total;

    return float4(result, 1.0);
}

public float4 edgeDetection(ImageSampleState state) {
    const float offset = 1.0 / 300.0;
    float2 offsets[9] = {
        float2(-offset, offset),  // top-left
        float2(0.0f, offset),     // top-center
        float2(offset, offset),   // top-right
        float2(-offset, 0.0f),    // center-left
        float2(0.0f, 0.0f),       // center-center
        float2(offset, 0.0f),     // center-right
        float2(-offset, -offset), // bottom-left
        float2(0.0f, -offset),    // bottom-center
        float2(offset, -offset)   // bottom-right
    };

    float kernel[9] = {
        1, 1,  1, //
        1, -8, 1, //
        1, 1,  1  //
    };

    return kernelEffect(state, offsets, kernel);
}

public float4 applyColorCorrection(float4 color, ColorCorrection cc) {
    float3 linearColor = color.rgb;

    linearColor *= pow(2.0, cc.exposure);

    linearColor = (linearColor - 0.5) * cc.contrast + 0.5;

    linearColor.r += cc.temperature * 0.05;
    linearColor.g += cc.tint * 0.05;

    float luminance = dot(linearColor, float3(0.2126, 0.7152, 0.0722));
    linearColor = lerp(float3(luminance), linearColor, cc.saturation);

    return float4(linearColor, color.a);
}

public float3 sampleLUT(float3 rgb, float blueSlice, float sliceSize,
                        float slicePixelOffset, Image lutTexture,
                        float lutSize) {
    float sliceX = floor(blueSlice / lutSize);
    float sliceY = modf(blueSlice, lutSize);

    float2 uv;
    uv.x = sliceX * sliceSize + slicePixelOffset + rgb.r * sliceSize;
    uv.y = sliceY * sliceSize + slicePixelOffset + rgb.g * sliceSize;

    return lutTexture.sample(uv).rgb;
}

float4 mapToLUT(float4 color, Image lutTexture, float lutSize,
                bool apply = true) {
    if (!apply) {
        return color;
    }

    float sliceSize = 1.0 / lutSize;
    float slicePixelOffset = sliceSize * 0.5;

    float blueIndex = color.b * (lutSize - 1.0);
    float sliceLow = floor(blueIndex);
    float sliceHigh = min(sliceLow + 1.0, lutSize - 1.0);
    float t = blueIndex - sliceLow;

    float3 lowColor = sampleLUT(color.rgb, sliceLow, sliceSize,
                                slicePixelOffset, lutTexture, lutSize);
    float3 highColor = sampleLUT(color.rgb, sliceHigh, sliceSize,
                                 slicePixelOffset, lutTexture, lutSize);

    float3 finalColor = lerp(lowColor, highColor, t);

    return float4(finalColor, color.a);
}

float saturate(float v) { return clamp(v, 0.0, 1.0); }
