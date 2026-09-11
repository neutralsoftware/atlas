float3 cosineSampleHemisphere(float2 u) {
    float r = sqrt(u.x);
    float theta = 2.0 * M_PI_F * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u.x));

    return float3(x, y, z);
}

float3x3 buildOrthonormalBasis(float3 N) {
    float3 T = normalize(abs(N.x) > 0.1 ? cross(float3(0, 1, 0), N)
                                        : cross(float3(1, 0, 0), N));
    float3 B = cross(N, T);
    return float3x3(T, B, N);
}

float3 normalizeOr(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10) {
        return v * rsqrt(len2);
    }
    return fallback;
}

float rayOffsetDistance(float3 position) {
    float positionScale =
        max(abs(position.x), max(abs(position.y), abs(position.z)));
    return max(0.0002, positionScale * 0.000002);
}

float3 offsetRayOrigin(float3 position, float3 geometricNormal,
                       float3 direction) {
    float side = dot(direction, geometricNormal) >= 0.0 ? 1.0 : -1.0;
    return position + geometricNormal * (rayOffsetDistance(position) * side);
}

float2 encodeNormal(float3 normal) {
    normal /= max(abs(normal.x) + abs(normal.y) + abs(normal.z), 1e-6);
    float2 encoded = normal.xy;
    if (normal.z < 0.0) {
        float2 signValue = select(float2(-1.0), float2(1.0), encoded >= 0.0);
        encoded = (1.0 - abs(encoded.yx)) * signValue;
    }
    return encoded;
}

