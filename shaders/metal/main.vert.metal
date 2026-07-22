#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Returns the determinant of a 2x2 matrix.
static inline __attribute__((always_inline))
float spvDet2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Returns the inverse of a matrix, by using the algorithm of calculating the classical
// adjoint and dividing by the determinant. The contents of the matrix are changed.
static inline __attribute__((always_inline))
float3x3 spvInverse3x3(float3x3 m)
{
    float3x3 adj;	// The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the matrix.
    adj[0][0] =  spvDet2x2(m[1][1], m[1][2], m[2][1], m[2][2]);
    adj[0][1] = -spvDet2x2(m[0][1], m[0][2], m[2][1], m[2][2]);
    adj[0][2] =  spvDet2x2(m[0][1], m[0][2], m[1][1], m[1][2]);

    adj[1][0] = -spvDet2x2(m[1][0], m[1][2], m[2][0], m[2][2]);
    adj[1][1] =  spvDet2x2(m[0][0], m[0][2], m[2][0], m[2][2]);
    adj[1][2] = -spvDet2x2(m[0][0], m[0][2], m[1][0], m[1][2]);

    adj[2][0] =  spvDet2x2(m[1][0], m[1][1], m[2][0], m[2][1]);
    adj[2][1] = -spvDet2x2(m[0][0], m[0][1], m[2][0], m[2][1]);
    adj[2][2] =  spvDet2x2(m[0][0], m[0][1], m[1][0], m[1][1]);

    // Calculate the determinant as a combination of the cofactors of the first row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    uint isInstanced;
};

struct main0_out
{
    float2 TexCoord [[user(locn0)]];
    float4 outColor [[user(locn1)]];
    float3 Normal [[user(locn2)]];
    float3 FragPos [[user(locn3)]];
    float3 TBN_0 [[user(locn4)]];
    float3 TBN_1 [[user(locn5)]];
    float3 TBN_2 [[user(locn6)]];
    float4 gl_Position [[position, invariant]];
};

struct main0_in
{
    float3 aPos [[attribute(0)]];
    float4 aColor [[attribute(1)]];
    float2 aTexCoord [[attribute(2)]];
    float3 aNormal [[attribute(3)]];
    float3 aTangent [[attribute(4)]];
    float3 aBitangent [[attribute(5)]];
    float4 instanceModel_0 [[attribute(6)]];
    float4 instanceModel_1 [[attribute(7)]];
    float4 instanceModel_2 [[attribute(8)]];
    float4 instanceModel_3 [[attribute(9)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& uniforms [[buffer(0)]])
{
    main0_out out = {};
    float3x3 TBN = {};
    float4x4 instanceModel = {};
    instanceModel[0] = in.instanceModel_0;
    instanceModel[1] = in.instanceModel_1;
    instanceModel[2] = in.instanceModel_2;
    instanceModel[3] = in.instanceModel_3;
    float4x4 modelMatrix = uniforms.model;
    bool hasInstanceMatrix = abs(instanceModel[3].w) > 0.5;
    if ((uniforms.isInstanced != 0u) && hasInstanceMatrix)
    {
        modelMatrix = instanceModel;
    }
    float4x4 mvp = (uniforms.projection * uniforms.view) * modelMatrix;
    float4 _56 = float4(in.aPos, 1.0);
    float4 _57 = mvp * _56;
    out.gl_Position = _57;
    out.FragPos = float3((modelMatrix * float4(in.aPos, 1.0)).xyz);
    out.TexCoord = in.aTexCoord;
    out.outColor = in.aColor;
    float3x3 normalMatrix = transpose(spvInverse3x3(float3x3(modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz)));
    out.Normal = fast::normalize(normalMatrix * in.aNormal);
    float3 N = out.Normal;
    float3 T = fast::normalize(normalMatrix * in.aTangent);
    float3 B = fast::normalize(normalMatrix * in.aBitangent);
    TBN = float3x3(float3(T), float3(B), float3(N));
    out.TBN_0 = TBN[0];
    out.TBN_1 = TBN[1];
    out.TBN_2 = TBN[2];
    return out;
}
