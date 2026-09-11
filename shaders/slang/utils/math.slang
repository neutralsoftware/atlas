module Math;

public float3x3 inverse3x3(float3x3 m) {
    float a00 = m[0][0];
    float a01 = m[0][1];
    float a02 = m[0][2];

    float a10 = m[1][0];
    float a11 = m[1][1];
    float a12 = m[1][2];

    float a20 = m[2][0];
    float a21 = m[2][1];
    float a22 = m[2][2];

    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;

    float det = a00 * b01 + a01 * b11 + a02 * b21;

    float invDet = 1.0 / det;

    return float3x3(b01 * invDet, (-a22 * a01 + a02 * a21) * invDet,
                    (a12 * a01 - a02 * a11) * invDet,

                    b11 * invDet, (a22 * a00 - a02 * a20) * invDet,
                    (-a12 * a00 + a02 * a10) * invDet,

                    b21 * invDet, (-a21 * a00 + a01 * a20) * invDet,
                    (a11 * a00 - a01 * a10) * invDet);
}

float3 reconstructViewPos(float2 uv, float depth, float4x4 invProjection) {
    float z = depth * 2.0 - 1.0;
    float4 clipPos = float4(uv * 2.0 - 1.0, z, 1.0);
    float4 viewPos = mul(invProjection, clipPos);
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

float linearizeDepth(float depth, float nearPlane, float farPlane) {
    float z = depth * 2.0 - 1.0;
    float linear = (2.0 * nearPlane * farPlane) /
                   (farPlane + nearPlane - z * (farPlane - nearPlane));
    return linear / farPlane;
}

float hashNoise(float3 p) {
    return fract(sin(dot(p, float3(12.9898, 78.233, 37.719))) * 43758.5453);
}
