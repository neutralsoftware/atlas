float2 getTextureDimensions(
    int textureIndex, Sampler2D texture1, Sampler2D texture2,
    Sampler2D texture3, Sampler2D texture4, Sampler2D texture5) {
    if (textureIndex == 0) {
        return float2(int2(textureSize(texture1).x, textureSize(texture1).y));
    } else {
        if (textureIndex == 1) {
            return float2(int2(textureSize(texture2).x, textureSize(texture2).y));
        } else {
            if (textureIndex == 2) {
                return float2(
                    int2(textureSize(texture3).x, textureSize(texture3).y));
            } else {
                if (textureIndex == 3) {
                    return float2(
                        int2(textureSize(texture4).x, textureSize(texture4).y));
                } else {
                    if (textureIndex == 4) {
                        return float2(
                            int2(textureSize(texture5).x, textureSize(texture5).y));
                    }
                }
            }
        }
    }
    return float2(0.0);
}

float4 sampleCubeTextureAt(
    int textureIndex, float3 direction,
    SamplerCube cubeMap1,
    SamplerCube cubeMap2,
    SamplerCube cubeMap3,
    SamplerCube cubeMap4,
    SamplerCube cubeMap5) {
    if (textureIndex == 0) {
        return cubeMap1.Sample(direction);
    } else {
        if (textureIndex == 1) {
            return cubeMap2.Sample(direction);
        } else {
            if (textureIndex == 2) {
                return cubeMap3.Sample(direction);
            } else {
                if (textureIndex == 3) {
                    return cubeMap4.Sample(direction);
                } else {
                    if (textureIndex == 4) {
                        return cubeMap5.Sample(direction);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

float calculatePointShadow(
    ShadowParameters shadowParam, float3 fragPos,
    Sampler2D texture1, Sampler2D texture2, Sampler2D texture3,
    Sampler2D texture4, Sampler2D texture5, SamplerCube cubeMap1,
    SamplerCube cubeMap2,
    SamplerCube cubeMap3,
    SamplerCube cubeMap4,
    SamplerCube cubeMap5) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture2, texture3, texture4, texture5);
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
    float bias0 = 0.0500000007;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) *
                       0.0500000007;
    for (int i = 0; i < 10; i++) {
        float3 sampleDir =
            normalize(fragToLight + (_861[i] * diskRadius));
        int param_1 = shadowParam.textureIndex;
        float3 param_2 = sampleDir;
        float closestDepth =
            sampleCubeTextureAt(param_1, param_2, cubeMap1,
                                cubeMap2, cubeMap3, cubeMap4,
                                cubeMap5)
                .x *
            shadowParam.farPlane;
        if ((currentDepth - bias0) > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= 10.0;
    return shadow;
}

float4 sampleTextureAt(
    int textureIndex, float2 uv,
    Sampler2D texture1, Sampler2D texture2, Sampler2D texture3,
    Sampler2D texture4, Sampler2D texture5) {
    if (textureIndex == 0) {
        return texture1.Sample(uv);
    } else {
        if (textureIndex == 1) {
            return texture2.Sample(uv);
        } else {
            if (textureIndex == 2) {
                return texture3.Sample(uv);
            } else {
                if (textureIndex == 3) {
                    return texture4.Sample(uv);
                } else {
                    if (textureIndex == 4) {
                        return texture5.Sample(uv);
                    }
                }
            }
        }
    }
    return float4(0.0);
}

float calculateShadow(
    ShadowParameters shadowParam, float3 fragPos,
    float3 normal, Sampler2D texture1, Sampler2D texture2,
    Sampler2D texture3, Sampler2D texture4, Sampler2D texture5,
    UBO uniforms) {
    int param = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(
        param, texture1, texture2, texture3, texture4, texture5);
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
        mul(float4(fragPos, 1.0), (mul(shadowParam.lightView, shadowParam.lightProjection)));
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
    float3 lightDirWorld = normalize(
        -(mul(float4(0.0, 0.0, -1.0, 0.0), inverseMatrix(shadowParam.lightView)))
             .xyz);
    float biasValue = shadowParam.bias0;
    if (isAreaShadow) {
        biasValue = max(biasValue, 0.00120000006);
    }
    float ndotl = max(dot(normal, lightDirWorld), 0.0);
    float minBias =
        max(isAreaShadow ? 0.000250000012
                               : 4.99999987e-05,
                  biasValue * (isAreaShadow ? 0.850000024 : 0.25));
    float bias0 = max(biasValue * (1.0 - ndotl), minBias);
    if (isAreaShadow) {
        bias0 += 0.000250000012;
    }
    float shadow = 0.0;
    float2 texelSize = float2(1.0) / dims;
    float _distance = length(float3(uniforms.cameraPosition) - fragPos);
    float2 shadowMapSize = dims;
    float avgDim = 0.5 * (shadowMapSize.x + shadowMapSize.y);
    float resFactor = clamp(1024.0 / max(avgDim, 1.0), 0.75, 1.25);
    float distFactor = clamp(_distance / 800.0, 0.0, 1.0);
    float texelRadius =
        lerp(isAreaShadow ? 1.79999995 : 1.0,
            isAreaShadow ? 4.19999981 : 3.0, distFactor) *
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
            sampleTextureAt(param_1, param_2, texture1, texture2, texture3, texture4, texture5)
                .x;
        shadow += float((currentDepth - bias0) > pcfDepth);
        sampleCount++;
    }
    if (sampleCount > 0) {
        shadow /= float(sampleCount);
    }
    if (isAreaShadow) {
        shadow = smoothstep(0.180000007, 0.920000017, shadow);
    }
    return shadow;
}

float
distributionGGX(float3 N, float3 H,
                float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0)) + 1.0;
    denom = (3.14159274 * denom) * denom;
    return num / max(denom, 9.99999975e-05);
}

float
geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = (NdotV * (1.0 - k)) + k;
    return num / max(denom, 9.99999975e-05);
}

float
geometrySmith(float3 N, float3 V,
              float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float param = NdotV;
    float param_1 = roughness;
    float ggx2 = geometrySchlickGGX(param, param_1);
    float param_2 = NdotL;
    float param_3 = roughness;
    float ggx1 = geometrySchlickGGX(param_2, param_3);
    return ggx1 * ggx2;
}

float3
fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + ((float3(1.0) - F0) * powr(1.0 - cosTheta, 5.0));
}

float3
evaluateBRDF(float3 L, float3 radiance,
             float3 N, float3 V,
             float3 F0, float3 albedo,
             float metallic, float roughness) {
    float3 H = normalize(V + L);
    float3 param = N;
    float3 param_1 = H;
    float param_2 = roughness;
    float NDF = distributionGGX(param, param_1, param_2);
    float3 param_3 = N;
    float3 param_4 = V;
    float3 param_5 = L;
    float param_6 = roughness;
    float G = geometrySmith(param_3, param_4, param_5, param_6);
    float param_7 = max(dot(H, V), 0.0);
    float3 param_8 = F0;
    float3 F = fresnelSchlick(param_7, param_8);
    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float3 numerator = F * (NDF * G);
    float denominator =
        max((4.0 * NdotV) * NdotL, 9.99999975e-05);
    float3 specular = numerator / float3(denominator);
    return ((((kD * albedo) / float3(3.14159274)) + specular) *
            radiance) *
           NdotL;
}

float3 calcDirectionalLight(
    DirectionalLight light, float3 N,
    float3 V, float3 F0,
    float3 albedo, float metallic,
    float roughness) {
    float3 L = normalize(-light.direction);
    float3 radiance = light.diffuse * max(light.intensity, 0.0);
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

float3
calcPointLight(PointLight light, float3 fragPos,
               float3 N, float3 V,
               float3 F0, float3 albedo,
               float metallic, float roughness) {
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
        1.0 / max((light.constant0 + (light.linear * _distance)) +
                            ((light.quadratic * _distance) * _distance),
                        9.99999975e-05);
    float fade = 1.0 - smoothstep(light.radius * 0.899999976,
                                  light.radius, _distance);
    float3 radiance =
        ((light.diffuse * max(light.intensity, 0.0)) * attenuation) *
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

float3
calcSpotLight(SpotLight light, float3 fragPos,
              float3 N, float3 V,
              float3 F0, float3 albedo,
              float metallic, float roughness) {
    float3 L = light.position - fragPos;
    float _distance = length(L);
    float3 direction = normalize(L);
    float3 spotDirection = normalize(light.direction);
    float theta = dot(direction, -spotDirection);
    float epsilon = max(light.cutOff - light.outerCutOff,
                              9.99999975e-05);
    float intensity =
        clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    float range = max(light.range, 0.00100000005);
    float attenuation = 1.0 / ((1.0 + (_distance / range)) +
                               ((_distance * _distance) / (range * range)));
    float fade =
        1.0 - smoothstep(range * 0.899999976, range, _distance);
    float3 radiance =
        (((light.diffuse * max(light.intensity, 0.0)) * attenuation) *
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

float3
getRimLight(float3 fragPos, inout float3 N, inout float3 V,
            float3 F0, float3 albedo,
            float metallic, float roughness,
            UBO uniforms, Environment environment) {
    N = normalize(N);
    V = normalize(V);
    float rim = powr(1.0 - max(dot(N, V), 0.0), 3.0);
    rim *= lerp(1.20000005, 0.300000012, roughness);
    float3 rimColor =
        lerp(float3(1.0), albedo, float3(metallic)) * environment.rimLightColor;
    rimColor = lerp(rimColor, F0, float3(0.5));
    float rimIntensity = environment.rimLightIntensity;
    float3 rimLight = (rimColor * rim) * rimIntensity;
    float dist = length(float3(uniforms.cameraPosition) - fragPos);
    rimLight /= float3(1.0 + (dist * 0.100000001));
    return rimLight;
}

float3
sampleEnvironmentRadiance(float3 direction,
                          SamplerCube skybox) {
    return skybox.Sample(direction).xyz;
}

float3
acesToneMapping(inout float3 color) {
    float a = 2.50999999;
    float b = 0.0299999993;
    float c = 2.43000007;
    float d = 0.589999974;
    float e = 0.140000001;
    color = (color * ((color * a) + float3(b))) /
            ((color * ((color * c) + float3(d))) + float3(e));
    color = powr(clamp(color, float3(0.0), float3(1.0)),
                 float3(0.454545468));
    return color;
}
