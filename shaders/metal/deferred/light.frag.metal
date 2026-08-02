#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wmissing-braces"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

template <typename T, size_t Num> struct spvUnsafeArray {
    T elements[Num ? Num : 1];

    thread T &operator[](size_t pos) thread { return elements[pos]; }
    constexpr const thread T &operator[](size_t pos) const thread {
        return elements[pos];
    }

    device T &operator[](size_t pos) device { return elements[pos]; }
    constexpr const device T &operator[](size_t pos) const device {
        return elements[pos];
    }

    constexpr const constant T &operator[](size_t pos) const constant {
        return elements[pos];
    }

    threadgroup T &operator[](size_t pos) threadgroup { return elements[pos]; }
    constexpr const threadgroup T &operator[](size_t pos) const threadgroup {
        return elements[pos];
    }
};

// Implementation of the GLSL radians() function
template <typename T> inline T radians(T d) { return d * T(0.01745329251); }

// Returns the determinant of a 2x2 matrix.
static inline __attribute__((always_inline)) float
spvDet2x2(float a1, float a2, float b1, float b2) {
    return a1 * b2 - b1 * a2;
}

// Returns the determinant of a 3x3 matrix.
static inline __attribute__((always_inline)) float
spvDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1,
          float c2, float c3) {
    return a1 * spvDet2x2(b2, b3, c2, c3) - b1 * spvDet2x2(a2, a3, c2, c3) +
           c1 * spvDet2x2(a2, a3, b2, b3);
}

// Returns the inverse of a matrix, by using the algorithm of calculating the
// classical adjoint and dividing by the determinant. The contents of the matrix
// are changed.
static inline __attribute__((always_inline)) float4x4
spvInverse4x4(float4x4 m) {
    float4x4 adj; // The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the
    // matrix.
    adj[0][0] = spvDet3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3],
                          m[3][1], m[3][2], m[3][3]);
    adj[0][1] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3],
                           m[3][1], m[3][2], m[3][3]);
    adj[0][2] = spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3],
                          m[3][1], m[3][2], m[3][3]);
    adj[0][3] = -spvDet3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3],
                           m[2][1], m[2][2], m[2][3]);

    adj[1][0] = -spvDet3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3],
                           m[3][0], m[3][2], m[3][3]);
    adj[1][1] = spvDet3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3],
                          m[3][0], m[3][2], m[3][3]);
    adj[1][2] = -spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3],
                           m[3][0], m[3][2], m[3][3]);
    adj[1][3] = spvDet3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3],
                          m[2][0], m[2][2], m[2][3]);

    adj[2][0] = spvDet3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3],
                          m[3][0], m[3][1], m[3][3]);
    adj[2][1] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3],
                           m[3][0], m[3][1], m[3][3]);
    adj[2][2] = spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3],
                          m[3][0], m[3][1], m[3][3]);
    adj[2][3] = -spvDet3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3],
                           m[2][0], m[2][1], m[2][3]);

    adj[3][0] = -spvDet3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],
                           m[3][0], m[3][1], m[3][2]);
    adj[3][1] = spvDet3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2],
                          m[3][0], m[3][1], m[3][2]);
    adj[3][2] = -spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
                           m[3][0], m[3][1], m[3][2]);
    adj[3][3] = spvDet3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
                          m[2][0], m[2][1], m[2][2]);

    // Calculate the determinant as a combination of the cofactors of the first
    // row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) +
                (adj[0][2] * m[2][0]) + (adj[0][3] * m[3][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

struct ShadowParameters {
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    int lightIndex;
    float3 lightPos;
    int lightType;
};

struct DirectionalLight {
    float3 direction;
    float _pad1;
    float3 diffuse;
    float _pad2;
    float3 specular;
    float intensity;
};

struct PointLight {
    float3 position;
    float _pad1;
    float3 diffuse;
    float _pad2;
    float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct SpotLight {
    float3 position;
    float _pad1;
    float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    float3 diffuse;
    float _pad5;
    float3 specular;
    float _pad6;
};

struct UBO {
    packed_float3 cameraPosition;
    uint useIBL;
};

struct Environment {
    float rimLightIntensity;
    float3 rimLightColor;
    float bloomThreshold;
};

struct PushConstants {
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
};

struct ShadowParameters_1 {
    float4x4 lightView;
    float4x4 lightProjection;
    float bias0;
    int textureIndex;
    float farPlane;
    int lightIndex;
    packed_float3 lightPos;
    int lightType;
};

struct ShadowParams {
    spvUnsafeArray<ShadowParameters_1, 1> shadowParams;
};

struct DirectionalLight_1 {
    packed_float3 direction;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
};

struct DirectionalLights {
    spvUnsafeArray<DirectionalLight_1, 1> directionalLights;
};

struct PointLight_1 {
    packed_float3 position;
    float _pad1;
    packed_float3 diffuse;
    float _pad2;
    packed_float3 specular;
    float intensity;
    float constant0;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct PointLights {
    spvUnsafeArray<PointLight_1, 1> pointLights;
};

struct SpotLight_1 {
    packed_float3 position;
    float _pad1;
    packed_float3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    packed_float3 diffuse;
    float _pad5;
    packed_float3 specular;
    float _pad6;
};

struct SpotLights {
    spvUnsafeArray<SpotLight_1, 1> spotlights;
};

struct AreaLight {
    packed_float3 position;
    float _pad1;
    packed_float3 right;
    float _pad2;
    packed_float3 up;
    float _pad3;
    float2 size;
    float _pad4;
    float _pad5;
    packed_float3 diffuse;
    float _pad6;
    packed_float3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

struct AreaLights {
    spvUnsafeArray<AreaLight, 1> areaLights;
};

struct AmbientLight {
    float4 color;
    float intensity;
    float3 _pad0;
};

struct ProbeSpace {
    float3 origin;
    float _pad0;

    float3 spacing;
    float _pad1;

    float3 probeCount;
    float _pad2;

    float4 debugColor;

    float4 atlasParams;
    // x = textureBorderSize
    // y = probeResolution (inner)
    // z = probesPerRow
    // w = totalProbes  (put this here on CPU!)
};

constant spvUnsafeArray<float2, 12> _660 = spvUnsafeArray<float2, 12>(
    {float2(-0.3260000050067901611328125, -0.4059999883174896240234375),
     float2(-0.839999973773956298828125, -0.07400000095367431640625),
     float2(-0.69599997997283935546875, 0.4569999873638153076171875),
     float2(-0.20299999415874481201171875, 0.620999991893768310546875),
     float2(0.96200001239776611328125, -0.194999992847442626953125),
     float2(0.472999989986419677734375, -0.4799999892711639404296875),
     float2(0.518999993801116943359375, 0.767000019550323486328125),
     float2(0.185000002384185791015625, -0.89300000667572021484375),
     float2(0.507000029087066650390625, 0.064000003039836883544921875),
     float2(0.89600002765655517578125, 0.412000000476837158203125),
     float2(-0.3219999969005584716796875, -0.933000028133392333984375),
     float2(-0.791999995708465576171875, -0.597999989986419677734375)});
constant spvUnsafeArray<float3, 20> _861 = spvUnsafeArray<float3, 20>(
    {float3(0.5381000041961669921875, 0.18559999763965606689453125,
            -0.4318999946117401123046875),
     float3(0.13789999485015869140625, 0.248600006103515625,
            0.4429999887943267822265625),
     float3(0.3370999991893768310546875, 0.567900002002716064453125,
            -0.0057000000961124897003173828125),
     float3(-0.699899971485137939453125, -0.0450999997556209564208984375,
            -0.0019000000320374965667724609375),
     float3(0.068899996578693389892578125, -0.159799993038177490234375,
            -0.854700028896331787109375),
     float3(0.056000001728534698486328125, 0.0068999999202787876129150390625,
            -0.184300005435943603515625),
     float3(-0.014600000344216823577880859375, 0.14020000398159027099609375,
            0.076200000941753387451171875),
     float3(0.00999999977648258209228515625, -0.19239999353885650634765625,
            -0.03440000116825103759765625),
     float3(-0.35769999027252197265625, -0.53009998798370361328125,
            -0.4357999861240386962890625),
     float3(-0.3169000148773193359375, 0.10629999637603759765625,
            0.015799999237060546875),
     float3(0.010300000198185443878173828125, -0.5868999958038330078125,
            0.0046000001020729541778564453125),
     float3(-0.08969999849796295166015625, -0.4939999878406524658203125,
            0.328700006008148193359375),
     float3(0.7118999958038330078125, -0.015399999916553497314453125,
            -0.091799996793270111083984375),
     float3(-0.053300000727176666259765625, 0.0595999993383884429931640625,
            -0.541100025177001953125),
     float3(0.03519999980926513671875, -0.063100002706050872802734375,
            0.546000003814697265625),
     float3(-0.4776000082492828369140625, 0.2847000062465667724609375,
            -0.0271000005304813385009765625),
     float3(-0.11200000345706939697265625, 0.1234000027179718017578125,
            -0.744599997997283935546875),
     float3(-0.212999999523162841796875, -0.07819999754428863525390625,
            -0.13789999485015869140625),
     float3(0.2944000065326690673828125, -0.3111999928951263427734375,
            -0.2644999921321868896484375),
     float3(-0.4564000070095062255859375, 0.4174999892711639404296875,
            -0.184300005435943603515625)});

struct main0_out {
    float4 FragColor [[color(0)]];
    float4 BrightColor [[color(1)]];
};

struct main0_in {
    float2 TexCoord [[user(locn0)]];
};

static inline __attribute__((always_inline)) float2 getTextureDimensions(
    thread const int &textureIndex, texture2d<float> texture1,
    sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr,
    texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4,
    sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr) {
    if (textureIndex == 0) {
        return float2(int2(texture1.get_width(), texture1.get_height()));
    } else {
        if (textureIndex == 1) {
            return float2(int2(texture2.get_width(), texture2.get_height()));
        } else {
            if (textureIndex == 2) {
                return float2(
                    int2(texture3.get_width(), texture3.get_height()));
            } else {
                if (textureIndex == 3) {
                    return float2(
                        int2(texture4.get_width(), texture4.get_height()));
                } else {
                    if (textureIndex == 4) {
                        return float2(
                            int2(texture5.get_width(), texture5.get_height()));
                    }
                }
            }
        }
    }
    return float2(0.0);
}

static inline __attribute__((always_inline)) float4 sampleCubeTextureAt(
    thread const int &textureIndex, thread const float3 &direction,
    texturecube<float> cubeMap1, sampler cubeMap1Smplr,
    texturecube<float> cubeMap2, sampler cubeMap2Smplr,
    texturecube<float> cubeMap3, sampler cubeMap3Smplr,
    texturecube<float> cubeMap4, sampler cubeMap4Smplr,
    texturecube<float> cubeMap5, sampler cubeMap5Smplr) {
    if (textureIndex == 0) {
        return cubeMap1.sample(cubeMap1Smplr, direction);
    } else {
        if (textureIndex == 1) {
            return cubeMap2.sample(cubeMap2Smplr, direction);
        } else {
            if (textureIndex == 2) {
                return cubeMap3.sample(cubeMap3Smplr, direction);
            } else {
                if (textureIndex == 3) {
                    return cubeMap4.sample(cubeMap4Smplr, direction);
                } else {
                    if (textureIndex == 4) {
                        return cubeMap5.sample(cubeMap5Smplr, direction);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline)) float calculatePointShadow(
    thread const ShadowParameters &shadowParam, thread const float3 &fragPos,
    texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2,
    sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr,
    texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5,
    sampler texture5Smplr, texturecube<float> cubeMap1, sampler cubeMap1Smplr,
    texturecube<float> cubeMap2, sampler cubeMap2Smplr,
    texturecube<float> cubeMap3, sampler cubeMap3Smplr,
    texturecube<float> cubeMap4, sampler cubeMap4Smplr,
    texturecube<float> cubeMap5, sampler cubeMap5Smplr) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture1Smplr, texture2, texture2Smplr, texture3,
        texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr);
    bool _739 = dims.x == 0.0;
    bool _746;
    if (!_739) {
        _746 = dims.y == 0.0;
    } else {
        _746 = _739;
    }
    if (_746) {
        return 0.0;
    }
    float3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);
    if (currentDepth >= shadowParam.farPlane) {
        return 0.0;
    }
    float bias0 = 0.0500000007450580596923828125;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) *
                       0.0500000007450580596923828125;
    for (int i = 0; i < 10; i++) {
        float3 sampleDir =
            fast::normalize(fragToLight + (_861[i] * diskRadius));
        int param_1 = shadowParam.textureIndex;
        float3 param_2 = sampleDir;
        float closestDepth =
            sampleCubeTextureAt(param_1, param_2, cubeMap1, cubeMap1Smplr,
                                cubeMap2, cubeMap2Smplr, cubeMap3,
                                cubeMap3Smplr, cubeMap4, cubeMap4Smplr,
                                cubeMap5, cubeMap5Smplr)
                .x *
            shadowParam.farPlane;
        if ((currentDepth - bias0) > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= 10.0;
    return shadow;
}

static inline __attribute__((always_inline)) float4 sampleTextureAt(
    thread const int &textureIndex, thread const float2 &uv,
    texture2d<float> texture1, sampler texture1Smplr, texture2d<float> texture2,
    sampler texture2Smplr, texture2d<float> texture3, sampler texture3Smplr,
    texture2d<float> texture4, sampler texture4Smplr, texture2d<float> texture5,
    sampler texture5Smplr) {
    if (textureIndex == 0) {
        return texture1.sample(texture1Smplr, uv);
    } else {
        if (textureIndex == 1) {
            return texture2.sample(texture2Smplr, uv);
        } else {
            if (textureIndex == 2) {
                return texture3.sample(texture3Smplr, uv);
            } else {
                if (textureIndex == 3) {
                    return texture4.sample(texture4Smplr, uv);
                } else {
                    if (textureIndex == 4) {
                        return texture5.sample(texture5Smplr, uv);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

static inline __attribute__((always_inline)) float calculateShadow(
    thread const ShadowParameters &shadowParam, thread const float3 &fragPos,
    thread const float3 &normal, texture2d<float> texture1,
    sampler texture1Smplr, texture2d<float> texture2, sampler texture2Smplr,
    texture2d<float> texture3, sampler texture3Smplr, texture2d<float> texture4,
    sampler texture4Smplr, texture2d<float> texture5, sampler texture5Smplr,
    constant UBO &_526) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture1Smplr, texture2, texture2Smplr, texture3,
        texture3Smplr, texture4, texture4Smplr, texture5, texture5Smplr);
    bool _411 = dims.x == 0.0;
    bool _419;
    if (!_411) {
        _419 = dims.y == 0.0;
    } else {
        _419 = _411;
    }
    if (_419) {
        return 0.0;
    }
    float4 fragPosLightSpace =
        (shadowParam.lightProjection * shadowParam.lightView) *
        float4(fragPos, 1.0);
    if (fragPosLightSpace.w == 0.0) {
        return 0.0;
    }
    float3 clipCoords = fragPosLightSpace.xyz / float3(fragPosLightSpace.w);
    float2 projCoordsXY = (clipCoords.xy * 0.5) + float2(0.5);
    float3 projCoords = float3(projCoordsXY, clipCoords.z);
    bool _456 = projCoords.x < 0.0;
    bool _463;
    if (!_456) {
        _463 = projCoords.x > 1.0;
    } else {
        _463 = _456;
    }
    bool _470;
    if (!_463) {
        _470 = projCoords.y < 0.0;
    } else {
        _470 = _463;
    }
    bool _477;
    if (!_470) {
        _477 = projCoords.y > 1.0;
    } else {
        _477 = _470;
    }
    bool _485;
    if (!_477) {
        _485 = projCoords.z < 0.0;
    } else {
        _485 = _477;
    }
    bool _493;
    if (!_485) {
        _493 = projCoords.z > 1.0;
    } else {
        _493 = _485;
    }
    if (_493) {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    bool isAreaShadow = shadowParam.lightType == 2;
    float3 lightDirWorld = fast::normalize(
        -(spvInverse4x4(shadowParam.lightView) * float4(0.0, 0.0, -1.0, 0.0))
             .xyz);
    float biasValue = shadowParam.bias0;
    if (isAreaShadow) {
        biasValue = fast::max(biasValue, 0.0012000000569969416);
    }
    float ndotl = fast::max(dot(normal, lightDirWorld), 0.0);
    float minBias =
        fast::max(isAreaShadow ? 0.0002500000118743628
                               : 4.9999998736893758177757263183594e-05,
                  biasValue * (isAreaShadow ? 0.8500000238418579 : 0.25));
    float bias0 = fast::max(biasValue * (1.0 - ndotl), minBias);
    if (isAreaShadow) {
        bias0 += 0.0002500000118743628;
    }
    float shadow = 0.0;
    float2 texelSize = float2(1.0) / dims;
    float _distance = length(float3(_526.cameraPosition) - fragPos);
    float2 shadowMapSize = dims;
    float avgDim = 0.5 * (shadowMapSize.x + shadowMapSize.y);
    float resFactor = fast::clamp(1024.0 / fast::max(avgDim, 1.0), 0.75, 1.25);
    float distFactor = fast::clamp(_distance / 800.0, 0.0, 1.0);
    float texelRadius =
        mix(isAreaShadow ? 1.7999999523162842 : 1.0,
            isAreaShadow ? 4.199999809265137 : 3.0, distFactor) *
        resFactor;
    float2 filterRadius = texelSize * texelRadius;
    int kernelSamples = isAreaShadow ? 6 : 8;
    int sampleCount = 0;
    for (int i = 0; i < kernelSamples; i++) {
        float2 offset = _660[i] * filterRadius;
        float2 uv = projCoords.xy + offset;
        bool _676 = uv.x < 0.0;
        bool _683;
        if (!_676) {
            _683 = uv.x > 1.0;
        } else {
            _683 = _676;
        }
        bool _690;
        if (!_683) {
            _690 = uv.y < 0.0;
        } else {
            _690 = _683;
        }
        bool _697;
        if (!_690) {
            _697 = uv.y > 1.0;
        } else {
            _697 = _690;
        }
        if (_697) {
            continue;
        }
        int param_1 = shadowParam.textureIndex;
        float2 param_2 = uv;
        float pcfDepth =
            sampleTextureAt(param_1, param_2, texture1, texture1Smplr, texture2,
                            texture2Smplr, texture3, texture3Smplr, texture4,
                            texture4Smplr, texture5, texture5Smplr)
                .x;
        shadow += float((currentDepth - bias0) > pcfDepth);
        sampleCount++;
    }
    if (sampleCount > 0) {
        shadow /= float(sampleCount);
    }
    if (isAreaShadow) {
        shadow = smoothstep(0.18000000715255737, 0.9200000166893005, shadow);
    }
    return shadow;
}

static inline __attribute__((always_inline)) float
distributionGGX(thread const float3 &N, thread const float3 &H,
                thread const float &roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = fast::max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)) + 1.0;
    denom = (3.1415927410125732421875 * denom) * denom;
    return num / fast::max(denom, 9.9999997473787516355514526367188e-05);
}

static inline __attribute__((always_inline)) float
geometrySchlickGGX(thread const float &NdotV, thread const float &roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = (NdotV * (1.0 - k)) + k;
    return num / fast::max(denom, 9.9999997473787516355514526367188e-05);
}

static inline __attribute__((always_inline)) float
geometrySmith(thread const float3 &N, thread const float3 &V,
              thread const float3 &L, thread const float &roughness) {
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float param = NdotV;
    float param_1 = roughness;
    float ggx2 = geometrySchlickGGX(param, param_1);
    float param_2 = NdotL;
    float param_3 = roughness;
    float ggx1 = geometrySchlickGGX(param_2, param_3);
    return ggx1 * ggx2;
}

static inline __attribute__((always_inline)) float3
fresnelSchlick(thread const float &cosTheta, thread const float3 &F0) {
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

static inline __attribute__((always_inline)) float3
evaluateBRDF(thread const float3 &L, thread const float3 &radiance,
             thread const float3 &N, thread const float3 &V,
             thread const float3 &F0, thread const float3 &albedo,
             thread const float &metallic, thread const float &roughness) {
    float3 H = fast::normalize(V + L);
    float3 param = N;
    float3 param_1 = H;
    float param_2 = roughness;
    float NDF = distributionGGX(param, param_1, param_2);
    float3 param_3 = N;
    float3 param_4 = V;
    float3 param_5 = L;
    float param_6 = roughness;
    float G = geometrySmith(param_3, param_4, param_5, param_6);
    float param_7 = fast::max(dot(H, V), 0.0);
    float3 param_8 = F0;
    float3 F = fresnelSchlick(param_7, param_8);
    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
    float NdotV = fast::max(dot(N, V), 0.0);
    float NdotL = fast::max(dot(N, L), 0.0);
    float3 numerator = F * (NDF * G);
    float denominator =
        fast::max((4.0 * NdotV) * NdotL, 9.9999997473787516355514526367188e-05);
    float3 specular = numerator / float3(denominator);
    return ((((kD * albedo) / float3(3.1415927410125732421875)) + specular) *
            radiance) *
           NdotL;
}

static inline __attribute__((always_inline)) float3 calcDirectionalLight(
    thread const DirectionalLight &light, thread const float3 &N,
    thread const float3 &V, thread const float3 &F0,
    thread const float3 &albedo, thread const float &metallic,
    thread const float &roughness) {
    float3 L = fast::normalize(-light.direction);
    float3 radiance = light.diffuse * fast::max(light.intensity, 0.0);
    float3 param = L;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
calcPointLight(thread const PointLight &light, thread const float3 &fragPos,
               thread const float3 &N, thread const float3 &V,
               thread const float3 &F0, thread const float3 &albedo,
               thread const float &metallic, thread const float &roughness) {
    float3 L = light.position - fragPos;
    float _distance = length(L);
    float3 _1019;
    if (_distance > 0.0) {
        _1019 = L / float3(_distance);
    } else {
        _1019 = float3(0.0, 0.0, 1.0);
    }
    float3 direction = _1019;
    float attenuation =
        1.0 / fast::max((light.constant0 + (light.linear * _distance)) +
                            ((light.quadratic * _distance) * _distance),
                        9.9999997473787516355514526367188e-05);
    float fade = 1.0 - smoothstep(light.radius * 0.89999997615814208984375,
                                  light.radius, _distance);
    float3 radiance =
        ((light.diffuse * fast::max(light.intensity, 0.0)) * attenuation) *
        fade;
    float3 param = direction;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
calcSpotLight(thread const SpotLight &light, thread const float3 &fragPos,
              thread const float3 &N, thread const float3 &V,
              thread const float3 &F0, thread const float3 &albedo,
              thread const float &metallic, thread const float &roughness) {
    float3 L = light.position - fragPos;
    float _distance = length(L);
    float3 direction = fast::normalize(L);
    float3 spotDirection = fast::normalize(light.direction);
    float theta = dot(direction, -spotDirection);
    float epsilon = fast::max(light.cutOff - light.outerCutOff,
                              9.9999997473787516355514526367188e-05);
    float intensity =
        fast::clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    float range = fast::max(light.range, 0.001000000047497451305389404296875);
    float attenuation = 1.0 / ((1.0 + (_distance / range)) +
                               ((_distance * _distance) / (range * range)));
    float fade =
        1.0 - smoothstep(range * 0.89999997615814208984375, range, _distance);
    float3 radiance =
        (((light.diffuse * fast::max(light.intensity, 0.0)) * attenuation) *
         intensity) *
        fade;
    float3 param = direction;
    float3 param_1 = radiance;
    float3 param_2 = N;
    float3 param_3 = V;
    float3 param_4 = F0;
    float3 param_5 = albedo;
    float param_6 = metallic;
    float param_7 = roughness;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7);
}

static inline __attribute__((always_inline)) float3
getRimLight(thread const float3 &fragPos, thread float3 &N, thread float3 &V,
            thread const float3 &F0, thread const float3 &albedo,
            thread const float &metallic, thread const float &roughness,
            constant UBO &_526, constant Environment &environment) {
    N = fast::normalize(N);
    V = fast::normalize(V);
    float rim = powr(1.0 - fast::max(dot(N, V), 0.0), 3.0);
    rim *= mix(1.2000000476837158203125, 0.300000011920928955078125, roughness);
    float3 rimColor =
        mix(float3(1.0), albedo, float3(metallic)) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, float3(0.5));
    float rimIntensity = environment.rimLightIntensity;
    float3 rimLight = (rimColor * rim) * rimIntensity;
    float dist = length(float3(_526.cameraPosition) - fragPos);
    rimLight /= float3(1.0 + (dist * 0.100000001490116119384765625));
    return rimLight;
}

static inline __attribute__((always_inline)) float3
sampleEnvironmentRadiance(thread const float3 &direction,
                          texturecube<float> skybox, sampler skyboxSmplr) {
    return skybox.sample(skyboxSmplr, direction).xyz;
}

static inline __attribute__((always_inline)) float3
acesToneMapping(thread float3 &color) {
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    color = (color * ((color * a) + float3(b))) /
            ((color * ((color * c) + float3(d))) + float3(e));
    color = powr(fast::clamp(color, float3(0.0), float3(1.0)),
                 float3(0.4545454680919647216796875));
    return color;
}

static inline float2 octEncode(float3 n) {
    n /= (fabs(n.x) + fabs(n.y) + fabs(n.z) + 1e-6f);
    float2 e = n.xy;

    if (n.z < 0.0f) {
        float2 signNotZero =
            float2(e.x >= 0.0f ? 1.0f : -1.0f, e.y >= 0.0f ? 1.0f : -1.0f);
        e = (1.0f - fabs(e.yx)) * signNotZero;
    }
    return e;
}

static inline float3 safeNormalizeDDGI(float3 v, float3 fallback) {
    float len2 = dot(v, v);
    if (len2 > 1e-10f) {
        return v * rsqrt(len2);
    }
    return fallback;
}

static inline uint probeIndexFromCoord(uint3 c, uint3 counts) {
    return c.x + counts.x * (c.y + counts.y * c.z);
}

static inline float2 ddgiAtlasUV(uint probeIndex, float3 dirWS,
                                 constant ProbeSpace &ps, uint atlasW,
                                 uint atlasH) {
    if (atlasW == 0u || atlasH == 0u) {
        return float2(0.5f);
    }

    uint border = (uint)ps.atlasParams.x;
    uint innerRes = (uint)ps.atlasParams.y;
    uint probesPerRow = (uint)ps.atlasParams.z;
    if (innerRes == 0u || probesPerRow == 0u) {
        return float2(0.5f);
    }

    uint tileRes = innerRes + 2u * border;

    uint tileX = probeIndex % probesPerRow;
    uint tileY = probeIndex / probesPerRow;

    float dirLen = length(dirWS);
    float3 safeDir =
        (dirLen > 1e-6f) ? (dirWS / float3(dirLen)) : float3(0.0f, 1.0f, 0.0f);
    float2 e = octEncode(safeDir);
    float2 innerUV = e * 0.5f + 0.5f;

    float2 innerPx = float2((float)(tileX * tileRes + border),
                            (float)(tileY * tileRes + border)) +
                     innerUV * float(innerRes - 1u);

    return (innerPx + 0.5f) / float2((float)atlasW, (float)atlasH);
}

static inline float4 sampleDDGITextureBilinear(texture2d<float> tex,
                                               float2 uv) {
    uint w = tex.get_width();
    uint h = tex.get_height();
    if (w == 0u || h == 0u) {
        return float4(0.0f);
    }

    float2 pixel = uv * float2((float)w, (float)h) - 0.5f;
    int2 p0 = int2(floor(pixel));
    int2 p1 = p0 + int2(1, 0);
    int2 p2 = p0 + int2(0, 1);
    int2 p3 = p0 + int2(1, 1);

    int maxX = int(w) - 1;
    int maxY = int(h) - 1;
    p0 = int2(clamp(p0.x, 0, maxX), clamp(p0.y, 0, maxY));
    p1 = int2(clamp(p1.x, 0, maxX), clamp(p1.y, 0, maxY));
    p2 = int2(clamp(p2.x, 0, maxX), clamp(p2.y, 0, maxY));
    p3 = int2(clamp(p3.x, 0, maxX), clamp(p3.y, 0, maxY));

    float2 f = fract(pixel);
    float4 c00 = tex.read(uint2((uint)p0.x, (uint)p0.y));
    float4 c10 = tex.read(uint2((uint)p1.x, (uint)p1.y));
    float4 c01 = tex.read(uint2((uint)p2.x, (uint)p2.y));
    float4 c11 = tex.read(uint2((uint)p3.x, (uint)p3.y));
    float4 cx0 = mix(c00, c10, f.x);
    float4 cx1 = mix(c01, c11, f.x);
    return mix(cx0, cx1, f.y);
}

static inline float4
sampleProbeDirectionalRadiance(texture2d<float> ddgiTexture,
                               constant ProbeSpace &ps, uint probeIndex,
                               uint atlasW, uint atlasH, float3 dirWS) {
    float2 uv = ddgiAtlasUV(probeIndex, dirWS, ps, atlasW, atlasH);
    return sampleDDGITextureBilinear(ddgiTexture, uv);
}

static inline float3 sampleDDGIIrradiance(texture2d<float> ddgiTexture,
                                          texture2d<float> ddgiDistance,
                                          constant ProbeSpace &ps, float3 posWS,
                                          float3 normalWS) {
    uint3 counts = uint3((uint)ps.probeCount.x, (uint)ps.probeCount.y,
                         (uint)ps.probeCount.z);

    if (counts.x == 0u || counts.y == 0u || counts.z == 0u)
        return float3(0.0);

    uint atlasW = ddgiTexture.get_width();
    uint atlasH = ddgiTexture.get_height();
    if (atlasW == 0u || atlasH == 0u) {
        return float3(0.0);
    }

    float normalLen = length(normalWS);
    float3 safeNormal = (normalLen > 1e-6f) ? (normalWS / float3(normalLen))
                                            : float3(0.0f, 1.0f, 0.0f);

    float3 safeSpacing = max(ps.spacing, float3(1e-4f));
    float spacingScale =
        max(max(safeSpacing.x, max(safeSpacing.y, safeSpacing.z)), 1e-4f);
    float3 grid = (posWS - ps.origin) / safeSpacing;

    if (counts.x < 2u || counts.y < 2u || counts.z < 2u) {
        float3 maxCoord = max(float3(0.0f), float3((float)counts.x - 1.0f,
                                                   (float)counts.y - 1.0f,
                                                   (float)counts.z - 1.0f));
        float3 clampedGrid = clamp(grid, float3(0.0f), maxCoord);
        uint3 nearest = uint3(
            (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
            (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
            (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0,
                        int(counts.z) - 1));
        uint pIndex = probeIndexFromCoord(nearest, counts);
        float4 nearestSample = sampleProbeDirectionalRadiance(
            ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
        float3 nearestProbePos = ps.origin + float3(nearest) * safeSpacing;
        float3 nearestDirection = safeNormalizeDDGI(
            posWS - nearestProbePos, safeNormal);
        float4 nearestMoments = sampleProbeDirectionalRadiance(
            ddgiDistance, ps, pIndex, atlasW, atlasH, nearestDirection);
        float nearestDistance = length(posWS - nearestProbePos);
        float nearestVariance = max(nearestMoments.y - nearestMoments.x *
                                                           nearestMoments.x,
                                    0.0001f);
        float nearestDelta = max(nearestDistance - nearestMoments.x, 0.0f);
        float nearestVisibility = nearestDelta <= 0.0f
                                      ? 1.0f
                                      : nearestVariance /
                                            (nearestVariance + nearestDelta *
                                                                   nearestDelta);
        float nearestValidity = isfinite(nearestSample.w)
                                    ? clamp(nearestSample.w, 0.0f, 1.0f)
                                    : 0.0f;
        float3 nearestIrr =
            all(isfinite(nearestSample.xyz)) ? nearestSample.xyz : float3(0.0f);
        nearestIrr *= nearestValidity * nearestVisibility;
        return max(nearestIrr, float3(0.0f));
    }

    float3 maxGrid =
        float3((float)counts.x - 1.0001f, (float)counts.y - 1.0001f,
               (float)counts.z - 1.0001f);
    float3 clampedGrid = clamp(grid, float3(0.0f), maxGrid);

    float3 baseF = floor(clampedGrid);
    float3 frac = clampedGrid - baseF;
    int3 baseI = int3(baseF);
    int3 maxBase =
        int3(int(counts.x) - 2, int(counts.y) - 2, int(counts.z) - 2);
    baseI = clamp(baseI, int3(0), maxBase);

    float3 result = float3(0.0f);
    float weightSum = 0.0f;
    float3 resultNoBackface = float3(0.0f);
    float weightSumNoBackface = 0.0f;

    for (uint dz = 0; dz <= 1u; dz++) {
        for (uint dy = 0; dy <= 1u; dy++) {
            for (uint dx = 0; dx <= 1u; dx++) {

                uint3 pc = uint3(uint(baseI.x) + dx, uint(baseI.y) + dy,
                                 uint(baseI.z) + dz);
                uint pIndex = probeIndexFromCoord(pc, counts);

                float trilinearW = ((dx == 0u) ? (1.0f - frac.x) : frac.x) *
                                   ((dy == 0u) ? (1.0f - frac.y) : frac.y) *
                                   ((dz == 0u) ? (1.0f - frac.z) : frac.z);

                float3 probePos = ps.origin + float3(pc) * safeSpacing;
                float3 surfaceToProbe = probePos - posWS;
                float sDist = length(surfaceToProbe);
                float3 dirToProbe = (sDist > 1e-4f) ? surfaceToProbe / sDist
                                                    : float3(0.0f, 1.0f, 0.0f);
                float frontW = max(dot(safeNormal, dirToProbe), 0.0f);
                float backfaceW =
                    mix(0.180000007152557373046875, 1.0f, frontW * frontW);
                float distNorm = sDist / spacingScale;
                float distNorm2 = distNorm * distNorm;
                float distanceW =
                    1.0f / (1.0f + distNorm2 * 0.85000002384185791015625);

                float w = trilinearW * backfaceW * distanceW;

                float4 irrSample = sampleProbeDirectionalRadiance(
                    ddgiTexture, ps, pIndex, atlasW, atlasH, safeNormal);
                float3 probeToSurface = -surfaceToProbe;
                float4 distanceSample = sampleProbeDirectionalRadiance(
                    ddgiDistance, ps, pIndex, atlasW, atlasH,
                    safeNormalizeDDGI(probeToSurface, safeNormal));
                float variance = max(distanceSample.y -
                                         distanceSample.x * distanceSample.x,
                                     0.0001f);
                float delta = max(sDist - distanceSample.x, 0.0f);
                float visibility = delta <= 0.0f
                                       ? 1.0f
                                       : variance / (variance + delta * delta);
                visibility = visibility * visibility * visibility;
                float probeValidity = isfinite(irrSample.w)
                                          ? clamp(irrSample.w, 0.0f, 1.0f)
                                          : 0.0f;
                float validityW = probeValidity;
                float3 irr = irrSample.xyz;
                if (!all(isfinite(irr))) {
                    irr = float3(0.0f);
                    validityW = 0.0f;
                }
                irr = max(irr, float3(0.0f));
                float weightedW = w * validityW * max(visibility, 0.001f);
                result += irr * weightedW;
                weightSum += weightedW;
                float noBackfaceW = trilinearW * distanceW * validityW *
                                    max(visibility, 0.001f);
                resultNoBackface += irr * noBackfaceW;
                weightSumNoBackface += noBackfaceW;
            }
        }
    }

    if (weightSum > 1e-5f) {
        return result / weightSum;
    }
    if (weightSumNoBackface > 1e-5f) {
        return resultNoBackface / weightSumNoBackface;
    }

    uint3 nearest = uint3(
        (uint)clamp(int(floor(clampedGrid.x + 0.5f)), 0, int(counts.x) - 1),
        (uint)clamp(int(floor(clampedGrid.y + 0.5f)), 0, int(counts.y) - 1),
        (uint)clamp(int(floor(clampedGrid.z + 0.5f)), 0, int(counts.z) - 1));
    uint nearestIndex = probeIndexFromCoord(nearest, counts);
    float4 fallbackSample = sampleProbeDirectionalRadiance(
        ddgiTexture, ps, nearestIndex, atlasW, atlasH, safeNormal);
    float fallbackValidity =
        isfinite(fallbackSample.w) ? clamp(fallbackSample.w, 0.0f, 1.0f) : 0.0f;
    float3 fallback =
        all(isfinite(fallbackSample.xyz)) ? fallbackSample.xyz : float3(0.0f);
    fallback *= fallbackValidity;
    if (!all(isfinite(fallback))) {
        fallback = float3(0.0f);
    }
    return max(fallback, float3(0.0f));
}

fragment main0_out main0(
    main0_in in [[stage_in]], constant UBO &_526 [[buffer(0)]],
    constant Environment &environment [[buffer(1)]],
    constant PushConstants &_1355 [[buffer(2)]],
    device ShadowParams &_1372 [[buffer(3)]],
    device DirectionalLights &_1422 [[buffer(4)]],
    device PointLights &_1465 [[buffer(5)]],
    device SpotLights &_1510 [[buffer(6)]],
    device AreaLights &_1552 [[buffer(7)]],
    constant AmbientLight &ambientLight [[buffer(8)]],
    constant ProbeSpace &ps [[buffer(9)]],
    texture2d<float> texture1 [[texture(0)]],
    texture2d<float> texture2 [[texture(1)]],
    texture2d<float> texture3 [[texture(2)]],
    texture2d<float> texture4 [[texture(3)]],
    texture2d<float> texture5 [[texture(4)]],
    texturecube<float> cubeMap1 [[texture(5)]],
    texturecube<float> cubeMap2 [[texture(6)]],
    texturecube<float> cubeMap3 [[texture(7)]],
    texturecube<float> cubeMap4 [[texture(8)]],
    texturecube<float> cubeMap5 [[texture(9)]],
    texturecube<float> skybox [[texture(10)]],
    texture2d<float> gPosition [[texture(11)]],
    texture2d<float> gNormal [[texture(12)]],
    texture2d<float> gAlbedoSpec [[texture(13)]],
    texture2d<float> gMaterial [[texture(14)]],
    texture2d<float> ssao [[texture(15)]],
    texture2d<float> irradianceMap [[texture(16)]],
    texture2d<float> ddgiDistanceMap [[texture(17)]],
    sampler texture1Smplr [[sampler(0)]], sampler texture2Smplr [[sampler(1)]],
    sampler texture3Smplr [[sampler(2)]], sampler texture4Smplr [[sampler(3)]],
    sampler texture5Smplr [[sampler(4)]], sampler cubeMap1Smplr [[sampler(5)]],
    sampler cubeMap2Smplr [[sampler(6)]], sampler cubeMap3Smplr [[sampler(7)]],
    sampler cubeMap4Smplr [[sampler(8)]], sampler cubeMap5Smplr [[sampler(9)]],
    sampler skyboxSmplr [[sampler(10)]], sampler gPositionSmplr [[sampler(11)]],
    sampler gNormalSmplr [[sampler(12)]],
    sampler gAlbedoSpecSmplr [[sampler(13)]],
    sampler gMaterialSmplr [[sampler(14)]], sampler ssaoSmplr [[sampler(15)]]) {
    main0_out out{};
    float4 gPositionSample = gPosition.sample(gPositionSmplr, in.TexCoord);
    float3 FragPos = gPositionSample.xyz;
    if (!all(isfinite(FragPos))) {
        FragPos = float3(0.0);
    }
    float depthSample = gPositionSample.w;
    float3 sampledNormal = gNormal.sample(gNormalSmplr, in.TexCoord).xyz;
    float normalLength = length(sampledNormal);
    bool hasNormal = all(isfinite(sampledNormal)) &&
                     normalLength > 9.9999997473787516355514526367188e-06;
    bool hasDepth =
        isfinite(depthSample) && depthSample < 0.999989986419677734375;
    bool hasGeometry = hasNormal || hasDepth;
    if (!hasGeometry) {
        float2 ndc = in.TexCoord * 2.0f - 1.0f;
        float3 backgroundDir = fast::normalize(float3(ndc.x, -ndc.y, 1.0f));
        float3 backgroundColor =
            sampleEnvironmentRadiance(backgroundDir, skybox, skyboxSmplr);
        out.FragColor = float4(acesToneMapping(backgroundColor), 1.0);
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
        return out;
    }
    float3 N = float3(0.0, 1.0, 0.0);
    if (hasNormal) {
        N = sampledNormal / float3(normalLength);
    } else {
        float3 geomN = cross(dfdx(FragPos), dfdy(FragPos));
        float geomNLen = length(geomN);
        if (all(isfinite(geomN)) &&
            geomNLen > 9.9999997473787516355514526367188e-06) {
            N = geomN / float3(geomNLen);
        }
    }
    float3 geomNormal = cross(dfdx(FragPos), dfdy(FragPos));
    float geomNormalLen = length(geomNormal);
    bool hasGeomNormal = all(isfinite(geomNormal)) &&
                         geomNormalLen > 9.9999997473787516355514526367188e-06;
    float3 shadowNormal = N;
    float3 ddgiNormal = N;
    if (hasGeomNormal) {
        geomNormal /= float3(geomNormalLen);
        if (dot(N, geomNormal) < 0.0) {
            geomNormal = -geomNormal;
        }
        shadowNormal = geomNormal;
        ddgiNormal = geomNormal;
    }
    float4 albedoAo = gAlbedoSpec.sample(gAlbedoSpecSmplr, in.TexCoord);
    float3 albedo = fast::clamp(albedoAo.xyz, float3(0.0), float3(1.0));
    if (!all(isfinite(albedo))) {
        albedo = float3(0.0);
    }
    float4 matData = gMaterial.sample(gMaterialSmplr, in.TexCoord);
    float metallic = fast::clamp(matData.x, 0.0, 1.0);
    float roughness = fast::clamp(matData.y, 0.0, 1.0);
    float ao = fast::clamp(matData.z, 0.0, 1.0);

    float viewDistance =
        fast::max(length(float3(_526.cameraPosition) - FragPos),
                  9.9999999747524270787835121154785e-07);
    float3 V = (float3(_526.cameraPosition) - FragPos) / float3(viewDistance);
    float3 F0 =
        mix(float3(0.039999999105930328369140625), albedo, float3(metallic));
    float ssaoFactor =
        fast::clamp(ssao.sample(ssaoSmplr, in.TexCoord).x, 0.0, 1.0);

    float ssaoContrast =
        fast::clamp(powr(ssaoFactor, 1.10000002384185791015625), 0.0, 1.0);
    float occlusion =
        fast::clamp(ao * (0.0500000007450580596923828125 +
                          (0.949999988079071044921875 * ssaoContrast)),
                    0.0, 1.0);
    float lightingOcclusion =
        fast::clamp(0.0500000007450580596923828125 +
                        (0.949999988079071044921875 * ssaoContrast),
                    0.0500000007450580596923828125, 1.0);
    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    int areaShadowCount = 0;
    float pointShadow = 0.0;
    int shadowCount = _1355.shadowParamCount;
    for (int i = 0; i < shadowCount; i++) {
        if (_1372.shadowParams[i].lightType == 3) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.pointLightCount ||
                distance(float3(_1465.pointLights[lightIndex].position),
                         FragPos) >= _1465.pointLights[lightIndex].radius) {
                continue;
            }
            ShadowParameters _1386;
            _1386.lightView = _1372.shadowParams[i].lightView;
            _1386.lightProjection = _1372.shadowParams[i].lightProjection;
            _1386.bias0 = _1372.shadowParams[i].bias0;
            _1386.textureIndex = _1372.shadowParams[i].textureIndex;
            _1386.farPlane = _1372.shadowParams[i].farPlane;
            _1386.lightIndex = _1372.shadowParams[i].lightIndex;
            _1386.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1386.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param = _1386;
            float3 param_1 = FragPos;
            pointShadow =
                fast::max(pointShadow,
                          calculatePointShadow(
                              param, param_1, texture1, texture1Smplr, texture2,
                              texture2Smplr, texture3, texture3Smplr, texture4,
                              texture4Smplr, texture5, texture5Smplr, cubeMap1,
                              cubeMap1Smplr, cubeMap2, cubeMap2Smplr, cubeMap3,
                              cubeMap3Smplr, cubeMap4, cubeMap4Smplr, cubeMap5,
                              cubeMap5Smplr));
        } else if (_1372.shadowParams[i].lightType == 1) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.spotlightCount ||
                distance(float3(_1510.spotlights[lightIndex].position),
                         FragPos) >= _1510.spotlights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            spotShadow = fast::max(
                spotShadow,
                calculateShadow(param_2, param_3, param_4, texture1,
                                texture1Smplr, texture2, texture2Smplr,
                                texture3, texture3Smplr, texture4,
                                texture4Smplr, texture5, texture5Smplr, _526));
        } else if (_1372.shadowParams[i].lightType == 2) {
            int lightIndex = _1372.shadowParams[i].lightIndex;
            if (lightIndex < 0 || lightIndex >= _1355.areaLightCount ||
                distance(float3(_1552.areaLights[lightIndex].position),
                         FragPos) >= _1552.areaLights[lightIndex].range) {
                continue;
            }
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            areaShadow += calculateShadow(
                param_2, param_3, param_4, texture1, texture1Smplr, texture2,
                texture2Smplr, texture3, texture3Smplr, texture4, texture4Smplr,
                texture5, texture5Smplr, _526);
            areaShadowCount++;
        } else {
            ShadowParameters _1397;
            _1397.lightView = _1372.shadowParams[i].lightView;
            _1397.lightProjection = _1372.shadowParams[i].lightProjection;
            _1397.bias0 = _1372.shadowParams[i].bias0;
            _1397.textureIndex = _1372.shadowParams[i].textureIndex;
            _1397.farPlane = _1372.shadowParams[i].farPlane;
            _1397.lightIndex = _1372.shadowParams[i].lightIndex;
            _1397.lightPos = float3(_1372.shadowParams[i].lightPos);
            _1397.lightType = _1372.shadowParams[i].lightType;
            ShadowParameters param_2 = _1397;
            float3 param_3 = FragPos;
            float3 param_4 = shadowNormal;
            directionalShadow = fast::max(
                directionalShadow,
                calculateShadow(param_2, param_3, param_4, texture1,
                                texture1Smplr, texture2, texture2Smplr,
                                texture3, texture3Smplr, texture4,
                                texture4Smplr, texture5, texture5Smplr, _526));
        }
    }
    if (areaShadowCount > 0) {
        areaShadow /= float(areaShadowCount);
    }
    directionalShadow =
        fast::clamp(directionalShadow * 0.85000002384185791015625, 0.0, 1.0);
    spotShadow = fast::clamp(spotShadow * 0.85000002384185791015625, 0.0, 1.0);
    areaShadow = fast::clamp(areaShadow * 0.449999988079071044921875, 0.0, 1.0);
    pointShadow =
        fast::clamp(pointShadow * 0.85000002384185791015625, 0.0, 1.0);
    float3 directionalResult = float3(0.0);
    for (int i_1 = 0; i_1 < _1355.directionalLightCount; i_1++) {
        DirectionalLight _1428;
        _1428.direction = float3(_1422.directionalLights[i_1].direction);
        _1428._pad1 = _1422.directionalLights[i_1]._pad1;
        _1428.diffuse = float3(_1422.directionalLights[i_1].diffuse);
        _1428._pad2 = _1422.directionalLights[i_1]._pad2;
        _1428.specular = float3(_1422.directionalLights[i_1].specular);
        _1428.intensity = _1422.directionalLights[i_1].intensity;
        DirectionalLight param_5 = _1428;
        float3 param_6 = N;
        float3 param_7 = V;
        float3 param_8 = F0;
        float3 param_9 = albedo;
        float param_10 = metallic;
        float param_11 = roughness;
        directionalResult += calcDirectionalLight(
            param_5, param_6, param_7, param_8, param_9, param_10, param_11);
    }
    directionalResult *= (1.0 - directionalShadow);
    float3 pointResult = float3(0.0);
    for (int i_2 = 0; i_2 < _1355.pointLightCount; i_2++) {
        PointLight _1471;
        _1471.position = float3(_1465.pointLights[i_2].position);
        _1471._pad1 = _1465.pointLights[i_2]._pad1;
        _1471.diffuse = float3(_1465.pointLights[i_2].diffuse);
        _1471._pad2 = _1465.pointLights[i_2]._pad2;
        _1471.specular = float3(_1465.pointLights[i_2].specular);
        _1471.intensity = _1465.pointLights[i_2].intensity;
        _1471.constant0 = _1465.pointLights[i_2].constant0;
        _1471.linear = _1465.pointLights[i_2].linear;
        _1471.quadratic = _1465.pointLights[i_2].quadratic;
        _1471.radius = _1465.pointLights[i_2].radius;
        _1471._pad3 = _1465.pointLights[i_2]._pad3;
        PointLight param_12 = _1471;
        float3 param_13 = FragPos;
        float3 param_14 = N;
        float3 param_15 = V;
        float3 param_16 = F0;
        float3 param_17 = albedo;
        float param_18 = metallic;
        float param_19 = roughness;
        pointResult += calcPointLight(param_12, param_13, param_14, param_15,
                                      param_16, param_17, param_18, param_19);
    }
    pointResult *= (1.0 - pointShadow);
    float3 spotResult = float3(0.0);
    for (int i_3 = 0; i_3 < _1355.spotlightCount; i_3++) {
        SpotLight _1516;
        _1516.position = float3(_1510.spotlights[i_3].position);
        _1516._pad1 = _1510.spotlights[i_3]._pad1;
        _1516.direction = float3(_1510.spotlights[i_3].direction);
        _1516.cutOff = _1510.spotlights[i_3].cutOff;
        _1516.outerCutOff = _1510.spotlights[i_3].outerCutOff;
        _1516.intensity = _1510.spotlights[i_3].intensity;
        _1516.range = _1510.spotlights[i_3].range;
        _1516._pad4 = _1510.spotlights[i_3]._pad4;
        _1516.diffuse = float3(_1510.spotlights[i_3].diffuse);
        _1516._pad5 = _1510.spotlights[i_3]._pad5;
        _1516.specular = float3(_1510.spotlights[i_3].specular);
        _1516._pad6 = _1510.spotlights[i_3]._pad6;
        SpotLight param_20 = _1516;
        float3 param_21 = FragPos;
        float3 param_22 = N;
        float3 param_23 = V;
        float3 param_24 = F0;
        float3 param_25 = albedo;
        float param_26 = metallic;
        float param_27 = roughness;
        spotResult += calcSpotLight(param_20, param_21, param_22, param_23,
                                    param_24, param_25, param_26, param_27);
    }
    spotResult *= (1.0 - spotShadow);
    float3 areaResult = float3(0.0);
    float _1639;
    for (int i_4 = 0; i_4 < _1355.areaLightCount; i_4++) {
        float3 P = float3(_1552.areaLights[i_4].position);
        float3 R = fast::normalize(float3(_1552.areaLights[i_4].right));
        float3 U = fast::normalize(float3(_1552.areaLights[i_4].up));
        float2 halfSize = _1552.areaLights[i_4].size * 0.5;
        float3 toPoint = FragPos - P;
        float s = fast::clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = fast::clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        float3 Q = (P + (R * s)) + (U * t);
        float3 Lvec = Q - FragPos;
        float dist = length(Lvec);
        if (dist > 9.9999997473787516355514526367188e-05) {
            float3 L = Lvec / float3(dist);
            float3 Nl = fast::normalize(cross(R, U));
            float ndotl = dot(Nl, -L);
            if (_1552.areaLights[i_4].castsBothSides != 0) {
                _1639 = abs(ndotl);
            } else {
                _1639 = fast::max(ndotl, 0.0);
            }
            float facing = _1639;
            float cosTheta = cos(radians(_1552.areaLights[i_4].angle));
            if ((facing >= cosTheta) && (facing > 0.0)) {
                float range = fast::max(_1552.areaLights[i_4].range,
                                        0.001000000047497451305389404296875);
                float attenuation = 1.0 / ((1.0 + (dist / range)) +
                                           ((dist * dist) / (range * range)));
                float fade = 1.0 - smoothstep(range * 0.89999997615814208984375,
                                              range, dist);
                float3 radiance =
                    (((float3(_1552.areaLights[i_4].diffuse) *
                       fast::max(_1552.areaLights[i_4].intensity, 0.0)) *
                      attenuation) *
                     facing) *
                    fade;
                float3 param_28 = L;
                float3 param_29 = radiance;
                float3 param_30 = N;
                float3 param_31 = V;
                float3 param_32 = F0;
                float3 param_33 = albedo;
                float param_34 = metallic;
                float param_35 = roughness;
                areaResult +=
                    evaluateBRDF(param_28, param_29, param_30, param_31,
                                 param_32, param_33, param_34, param_35);
            }
        }
    }
    areaResult *= (1.0 - areaShadow);
    float3 param_36 = FragPos;
    float3 param_37 = N;
    float3 param_38 = V;
    float3 param_39 = F0;
    float3 param_40 = albedo;
    float param_41 = metallic;
    float param_42 = roughness;
    float3 _1714 = getRimLight(param_36, param_37, param_38, param_39, param_40,
                               param_41, param_42, _526, environment);
    float3 rimResult = _1714;
    float3 lighting =
        ((((directionalResult + pointResult) + spotResult) + areaResult) +
         rimResult) *
        lightingOcclusion;
    float3 ambientBase =
        ((ambientLight.color.xyz * ambientLight.intensity) * albedo) *
        occlusion;
    bool ddgiEnabled = ps.atlasParams.w > 0.0f;
    float3 ambient = ddgiEnabled ? ambientBase * 0.05f : ambientBase;

    float ddgiSampleBias =
        max(max(ps.spacing.x, max(ps.spacing.y, ps.spacing.z)) * 0.05f, 0.002f);
    float3 ddgiSamplePos = FragPos + ddgiNormal * ddgiSampleBias;
    float3 ddgiIrradiance =
        sampleDDGIIrradiance(irradianceMap, ddgiDistanceMap, ps, ddgiSamplePos,
                             ddgiNormal);
    if (!all(isfinite(ddgiIrradiance))) {
        ddgiIrradiance = float3(0.0f);
    }
    ddgiIrradiance = max(ddgiIrradiance, float3(0.0f));

    float ddgiDebugMode = ps.debugColor.w;
    float3 ddgiGain = max(ps.debugColor.xyz, float3(0.0f));
    if (ddgiGain.x < 1e-4f && ddgiGain.y < 1e-4f && ddgiGain.z < 1e-4f) {
        ddgiGain = float3(1.0f);
    }

    const float INV_PI = 0.31830988618379067153776752674503;
    float3 ddgiDiffuse = ddgiIrradiance * albedo * INV_PI *
                         (1.0f - metallic) * occlusion * ddgiGain;
    ambient += ddgiDiffuse;

    float3 ddgiSpecular = float3(0.0f);

    float3 iblContribution = float3(0.0);
    if (_526.useIBL != 0u) {
        float3 param_43 = N;
        float3 irradiance =
            sampleEnvironmentRadiance(param_43, skybox, skyboxSmplr);
        float3 diffuseIBL = irradiance * albedo;
        float3 reflection = reflect(-V, N);
        float3 param_44 = reflection;
        float3 specularEnv =
            sampleEnvironmentRadiance(param_44, skybox, skyboxSmplr);
        float param_45 = fast::max(dot(N, V), 0.0);
        float3 param_46 = F0;
        float3 F = fresnelSchlick(param_45, param_46);
        float3 kS = F;
        float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
        float roughnessAttenuation = mix(1.0, 0.1500000059604644775390625,
                                         fast::clamp(roughness, 0.0, 1.0));
        float3 specularIBL = specularEnv * roughnessAttenuation;
        iblContribution = ((kD * diffuseIBL) + (kS * specularIBL)) * occlusion;
    }
    float3 finalColor = (ambient + lighting + ddgiSpecular) + iblContribution;

    // Debug AO view mode:
    // ps.debugColor.w >= 9.5f -> visualize SSAO directly in grayscale.
    if (ddgiDebugMode >= 9.5f && ddgiDebugMode < 19.5f) {
        finalColor = float3(ssaoFactor);
    } else if (ddgiDebugMode >= 39.5f) {
        finalColor =
            max(ddgiDiffuse * 3.0f + ddgiSpecular * 2.0f, float3(0.0f));
    } else if (ddgiDebugMode >= 29.5f) {
        finalColor = ddgiIrradiance * ddgiGain * 4.0f;
    } else if (ddgiDebugMode >= 19.5f) {
        finalColor = ddgiDiffuse * 3.0f;
    } else if (!(_526.useIBL != 0u)) {
        float3 I = fast::normalize(FragPos - float3(_526.cameraPosition));
        float3 R_1 = reflect(-I, N);
        float param_47 = fast::max(dot(N, -I), 0.0);
        float3 param_48 = F0;
        float3 F_1 = fresnelSchlick(param_47, param_48);
        float3 kS_1 = F_1;
        float3 envColor = skybox.sample(skyboxSmplr, R_1).xyz;
        float3 reflection_1 = envColor * kS_1;
        float3 envMix = F0 * (1.0f - roughness) * 0.20f;
        finalColor = mix(finalColor, reflection_1, envMix);
    }
    out.FragColor = float4(finalColor, 1.0);
    float brightness =
        dot(out.FragColor.xyz,
            float3(0.2125999927520751953125, 0.715200006961822509765625,
                   0.072200000286102294921875));
    if (brightness > environment.bloomThreshold) {
        out.BrightColor = float4(out.FragColor.xyz, 1.0);
    } else {
        out.BrightColor = float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 param_49 = out.FragColor.xyz;
    float3 _1897 = acesToneMapping(param_49);
    out.FragColor.x = _1897.x;
    out.FragColor.y = _1897.y;
    out.FragColor.z = _1897.z;
    return out;
}
