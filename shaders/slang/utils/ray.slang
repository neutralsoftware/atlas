module Raytracing;

public struct Ray {
    public float3 origin;
    public float3 direction;
    public float3 tMin;
    public float3 tMax;

    public float2 boxDst() {
        float3 t0 = (tMin - origin) / direction;
        float3 t1 = (tMax - origin) / direction;
        float3 tMinBox = min(t0, t1);
        float3 tMaxBox = max(t0, t1);

        float dstA = max(max(tMinBox.x, tMinBox.y), tMinBox.z);
        float dstB = min(min(tMaxBox.x, tMaxBox.y), tMaxBox.z);

        float dstToContainer = max(dstA, 0.0);
        float dstInsideContainer = (max(0.0, dstB - dstToContainer));

        return float2(dstToContainer, dstInsideContainer);
    }
}
