uint hashValue(uint value) {
    value = (value ^ 61u) ^ (value >> 16);
    value *= 9u;
    value ^= value >> 4;
    value *= 0x27d4eb2du;
    return value ^ (value >> 15);
}

float3 sphericalFibonacci(uint index, uint count, uint frameIndex) {
    float value = float(index) + 0.5;
    float phi = 6.28318530718 * frac(value / 1.61803398875);
    phi += float(hashValue(frameIndex * 1471u + 5743u) & 0xFFFFu) / 65536.0 *
           6.28318530718;
    float cosine = 1.0 - 2.0 * value / float(max(count, 1u));
    float sine = sqrt(max(0.0, 1.0 - cosine * cosine));
    float3 direction = float3(sine * cos(phi), cosine, sine * sin(phi));
    uint tiltSeed = hashValue(frameIndex * 3253u + 7919u);
    float tiltCos = 2.0 * float(tiltSeed & 65535u) / 65535.0 - 1.0;
    float tiltSin = sqrt(max(0.0, 1.0 - tiltCos * tiltCos));
    float spin = float(hashValue(tiltSeed) & 65535u) / 65536.0 * 6.28318530718;
    float3 tilted = float3(tiltCos * direction.x - tiltSin * direction.y,
                           tiltSin * direction.x + tiltCos * direction.y,
                           direction.z);
    return float3(cos(spin) * tilted.x - sin(spin) * tilted.z,
                  tilted.y, sin(spin) * tilted.x + cos(spin) * tilted.z);
}

