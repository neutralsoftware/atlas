module Types;

public enum TextureType {
    Color,
    Specular,
    Cubemap,
    Depth,
    DepthCube,
    Normal,
    Parallax,
    SSAONoise,
    SSAO,
    Metallic,
    Roughness,
    AO,
    Opacity,
    HDR,
    PBRPack
}

public enum EffectType {
    Invert,
    Grayscale,
    Sharpen,
    Blur,
    EdgeDetection,
    ColorCorrection,
    MotionBlur,
    ChromaticAberration,
    Posterization,
    Pixelation,
    Dilation,
    FilmGrain,
}

public struct ChromaticAberrationSettings {
    public float redOffset;
    public float greenOffset;
    public float blueOffset;
    public float2 focusPoint;
    public bool enabled;
}

public struct ColorCorrection {
    public float exposure;
    public float contrast;
    public float saturation;
    public float gamma;
    public float temperature;
    public float tint;
}


