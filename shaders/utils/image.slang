uint2 textureSize(Sampler2D tex) { uint w, h; tex.GetDimensions(w, h); return uint2(w, h); }

uint2 textureSize(Texture2D<float4> tex) { uint w, h; tex.GetDimensions(w, h); return uint2(w, h); }
