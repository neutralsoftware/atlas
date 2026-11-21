struct SkinPalette
{
    float4 skinColumns[1024];
};

[[vk::binding(1, 0)]] ConstantBuffer<SkinPalette> skinPalette;

float4x4 skinMatrix(uint4 joints, float4 weights, uint enabled)
{
    if (enabled == 0 || dot(weights, float4(1.0)) <= 0.0)
        return float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

    row_major float4x4 transform = {};
    for (uint influence = 0; influence < 4; ++influence)
    {
        if (weights[influence] <= 0.0)
            continue;
        uint base = 4 * joints[influence];
        for (uint column = 0; column < 4; ++column)
            transform[column] += skinPalette.skinColumns[base + column] * weights[influence];
    }
    return transform;
}
