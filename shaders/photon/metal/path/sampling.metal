float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float luminance(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)); }

float powerHeuristic(float pdfA, float pdfB) {
    float a2 = pdfA * pdfA;
    float b2 = pdfB * pdfB;
    return a2 / max(a2 + b2, 1e-8);
}

float3 clampLuminance(float3 c, float maxL) {
    float l = luminance(c);
    if (l > maxL && l > 1e-6) {
        c *= maxL / l;
    }
    return c;
}

uint wang_hash(uint s) {
    s = (s ^ 61u) ^ (s >> 16u);
    s *= 9u;
    s = s ^ (s >> 4u);
    s *= 0x27d4eb2du;
    s = s ^ (s >> 15u);
    return s;
}

float rand(thread uint &state) {
    state = wang_hash(state);
    return float(state) / 4294967296.0;
}

uint seedBase(uint2 gid, uint w, uint frame, uint sampleIndex) {
    return gid.x + gid.y * w + frame * 9781u + sampleIndex * 6271u + 1u;
}

