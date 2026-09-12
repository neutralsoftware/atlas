#include <metal_stdlib>
#include <metal_raytracing>
using namespace metal;
using namespace raytracing;

#include "path/types.metal"
#include "path/sampling.metal"
#include "path/environment.metal"
#include "path/geometry.metal"
#include "path/materials.metal"
#include "path/visibility.metal"
#include "path/brdf.metal"
#include "path/lighting.metal"
#include "path/integrator.metal"
kernel void main0(texture2d<float, access::write> outTex [[texture(0)]],
                  texture2d<float, access::read> historyTex [[texture(1)]],
                  texture2d<float, access::write> brightTex [[texture(2)]],
                  texture2d<float, access::write> albedoRoughnessTex [[texture(3)]],
                  texture2d<float, access::write> normalDepthTex [[texture(4)]],
                  texture2d<float, access::write> motionObjectTex [[texture(5)]],
                  texture2d<float, access::read> historyMomentsTex [[texture(6)]],
                  texture2d<float, access::read> historyGuideTex [[texture(7)]],
                  texture2d<float, access::write> historyOutTex [[texture(8)]],
                  texture2d<float, access::write> historyGuideOutTex [[texture(9)]],
                  texture2d<float, access::write> historyMomentsOutTex [[texture(10)]],
                  primitive_acceleration_structure sceneAS [[buffer(0)]],
                  constant CameraUniforms &cam [[buffer(1)]],
                  constant Material *materials [[buffer(2)]],
                  constant uint *primitiveObjects [[buffer(3)]],
                  constant VertexData *vertices [[buffer(4)]],
                  constant uint *indices [[buffer(5)]],
                  constant InstanceData *instanceData [[buffer(6)]],
                  constant DirectionalLightData &dirLight [[buffer(7)]],
                  constant SceneData &sceneData [[buffer(8)]],
                  constant PointLight *pointLights [[buffer(9)]],
                  constant SpotLight *spotLights [[buffer(10)]],
                  constant AreaLight *areaLights [[buffer(11)]],
                  constant EmissiveTriangle *emissiveTriangles [[buffer(14)]],
                  PT_MATERIAL_TEXTURE_BINDINGS,
                  constant uint *blasPrimitiveOffsets [[buffer(13)]],
                  texturecube<float> skybox [[texture(60)]],
                  uint2 gid [[thread_position_in_grid]]) {
    uint w = outTex.get_width();
    uint h = outTex.get_height();
    uint pixelStride = max(sceneData.pixelStride, 1u);
    gid *= pixelStride;
    if (gid.x >= w || gid.y >= h)
        return;

    float2 uv = (float2(gid) + 0.5) / float2(w, h);
    float3 ro = cam.camPos;

    intersector<triangle_data> isect;
    isect.assume_geometry_type(geometry_type::triangle);
    isect.set_triangle_cull_mode(triangle_cull_mode::none);

    float3 color = float3(0.0);
    float3 primaryAlbedo = float3(0.0);
    float3 primaryNormal = float3(0.0);
    float3 primaryPosition = float3(0.0);
    float primaryDepth = 0.0;
    float primaryRoughness = 1.0;
    float primaryHitDistance = 0.0;
    uint primaryObjectId = 0xFFFFFFFFu;

    uint spp = max(sceneData.raysPerPixel, 1u);
    for (uint s = 0; s < spp; ++s) {
        uint cameraRng = seedBase(gid, w, sceneData.frameIndex,
                                  s + 0x9E3779B9u);
        float2 pixelJitter = s == 0
                                 ? float2(0.0)
                                 : float2(rand(cameraRng), rand(cameraRng)) -
                                       0.5;
        float2 sampleUv = (float2(gid) + 0.5 + pixelJitter) / float2(w, h);
        float2 sampleNdc = sampleUv * 2.0 - 1.0;
        sampleNdc.y = -sampleNdc.y;
        float4 sampleClip = float4(sampleNdc, 1.0, 1.0);
        float4 sampleWorldH = cam.invViewProj * sampleClip;
        float3 sampleWorldP = sampleWorldH.xyz / sampleWorldH.w;

        ray primaryRay;
        primaryRay.origin = ro;
        primaryRay.direction = normalize(sampleWorldP - ro);
        primaryRay.min_distance = 0.001;
        primaryRay.max_distance = 1.0e30;

        float3 sampleAlbedo = float3(0.0);
        float3 sampleNormal = float3(0.0);
        float3 samplePosition = float3(0.0);
        float sampleDepth = 0.0;
        float sampleRoughness = 1.0;
        float sampleHitDistance = 0.0;
        uint sampleObjectId = 0xFFFFFFFFu;

        float3 sample = sampleRadiance(
            gid, s, w, isect, sceneAS, primaryRay, materials, primitiveObjects,
            blasPrimitiveOffsets, vertices, indices, instanceData, dirLight,
            sceneData, pointLights, spotLights, areaLights,
            emissiveTriangles, PT_MATERIAL_TEXTURE_ARGS, skybox, sampleAlbedo,
            sampleNormal, samplePosition, sampleDepth, sampleRoughness,
            sampleHitDistance, sampleObjectId);
        if (!all(isfinite(sample))) {
            sample = float3(0.0);
        }
        color += clampLuminance(max(sample, float3(0.0)),
                                max(sceneData.fireflyClamp, 1.0));
        if (s == 0) {
            primaryAlbedo = sampleAlbedo;
            primaryNormal = sampleNormal;
            primaryPosition = samplePosition;
            primaryDepth = sampleDepth;
            primaryRoughness = sampleRoughness;
            primaryHitDistance = sampleHitDistance;
            primaryObjectId = sampleObjectId;
        }
    }

    color /= float(spp);
    if (!all(isfinite(color))) {
        color = float3(0.0);
    }

    float objectIdValue = primaryObjectId == 0xFFFFFFFFu
                              ? -1.0
                              : float(primaryObjectId);
    float2 encodedNormal = encodeNormal(primaryNormal);
    float4 currentGuide =
        float4(encodedNormal, primaryDepth, objectIdValue);
    float4 previousClip = cam.prevViewProj * float4(primaryPosition, 1.0);
    float2 previousNdc = previousClip.xy / max(abs(previousClip.w), 0.0001);
    float2 previousUv = float2(previousNdc.x * 0.5 + 0.5,
                               0.5 - previousNdc.y * 0.5);
    bool previousUvValid = previousClip.w > 0.0 &&
                           all(previousUv >= float2(0.0)) &&
                           all(previousUv <= float2(1.0));
    uint2 previousPixel = gid;
    if (primaryObjectId != 0xFFFFFFFFu && previousUvValid) {
        previousPixel = uint2(clamp(previousUv * float2(w, h), float2(0.0),
                                    float2(w - 1, h - 1)));
    }
    float4 prevColor = historyTex.read(previousPixel);
    float4 previousGuide = historyGuideTex.read(previousPixel);
    float4 previousMoments = historyMomentsTex.read(previousPixel);
    bool historyValid = sceneData.frameIndex > 0 && prevColor.w > 0.0 &&
                        abs(previousGuide.z - primaryDepth) <
                            max(0.02, primaryDepth * 0.01) &&
                        distance(previousGuide.xy, encodedNormal) < 0.04 &&
                        abs(previousGuide.w - objectIdValue) < 0.5;
    if (!historyValid) {
        prevColor = float4(0.0);
        previousMoments = float4(0.0);
    }
    float previousMean = historyValid ? previousMoments.x : 0.0;
    float previousVariance =
        historyValid
            ? max(previousMoments.y - previousMean * previousMean, 0.0)
            : 0.0;
    float sampleLuminanceLimit = max(sceneData.fireflyClamp, 1.0);
    if (historyValid && prevColor.w >= 4.0) {
        float statisticalLimit = previousMean +
                                 max(0.5, 6.0 * sqrt(previousVariance));
        sampleLuminanceLimit =
            min(sampleLuminanceLimit, max(4.0, statisticalLimit));
    }
    color = clampLuminance(color, sampleLuminanceLimit);
    float historyLimit = max(float(sceneData.accumulationFrameLimit), 1.0);
    float previousWeight =
        historyValid ? min(prevColor.w, max(historyLimit - 1.0, 0.0)) : 0.0;
    float newHistoryLength = min(previousWeight + 1.0, historyLimit);
    float accumulationDenominator = max(previousWeight + 1.0, 1.0);
    float3 accum =
        (prevColor.xyz * previousWeight + color) / accumulationDenominator;
    float moment = luminance(color);
    float accumulatedMoment =
        (previousMoments.x * previousWeight + moment) /
        accumulationDenominator;
    float accumulatedMomentSquared =
        (previousMoments.y * previousWeight + moment * moment) /
        accumulationDenominator;
    float variance = max(accumulatedMomentSquared -
                             accumulatedMoment * accumulatedMoment,
                         0.0);

    constexpr float bloomKnee = 0.35;

    float brightness = luminance(accum);
    float soft = clamp(brightness - sceneData.bloomThreshold + bloomKnee, 0.0,
                       bloomKnee * 2.0);
    soft = soft * soft / max(bloomKnee * 4.0, 0.00001);
    float contribution = max(brightness - sceneData.bloomThreshold, soft) /
                         max(brightness, 0.00001);
    float3 brightColor = accum * contribution;
    float2 motion = previousUvValid ? uv - previousUv : float2(0.0);

    for (uint y = 0; y < pixelStride; ++y) {
        for (uint x = 0; x < pixelStride; ++x) {
            uint2 pixel = gid + uint2(x, y);
            if (pixel.x >= w || pixel.y >= h) {
                continue;
            }
            historyOutTex.write(float4(accum, newHistoryLength), pixel);
            historyGuideOutTex.write(currentGuide, pixel);
            albedoRoughnessTex.write(float4(primaryAlbedo, primaryRoughness),
                                     pixel);
            normalDepthTex.write(float4(primaryNormal, primaryDepth), pixel);
            motionObjectTex.write(float4(motion, objectIdValue, 1.0), pixel);
            historyMomentsOutTex.write(
                float4(accumulatedMoment, accumulatedMomentSquared, variance,
                       primaryHitDistance),
                pixel);
            outTex.write(float4(accum, 1.0), pixel);
            brightTex.write(float4(brightColor, 1.0), pixel);
        }
    }
}
