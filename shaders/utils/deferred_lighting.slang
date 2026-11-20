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

float filteredShadowComparison(int index, float2 uv, float depth, float2 dims, float2 depthGradient,
                               Sampler2D texture1, Sampler2D texture2,
                               Sampler2D texture3, Sampler2D texture4, Sampler2D texture5) {
    float2 pixel = uv * dims - 0.5;
    float2 base = floor(pixel);
    float2 f = frac(pixel);
    float result = 0.0;
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            float2 tap = (base + float2(x, y) + 0.5) / dims;
            if (any(tap < 0.0) || any(tap > 1.0)) continue;
            float stored = sampleTextureAt(index, tap, texture1, texture2,
                                            texture3, texture4, texture5).x;
            result += float(depth + dot(depthGradient, tap - uv) > stored) * (x == 0 ? 1.0 - f.x : f.x) *
                      (y == 0 ? 1.0 - f.y : f.y);
        }
    }
    return result;
}

float shadowViewDepth(float depth, float4x4 inverseProjection) {
    float4 position = mul(float4(0.0, 0.0, depth, 1.0), inverseProjection);
    return abs(position.z / position.w);
}

float calculateShadow(
    ShadowParameters shadowParam, float3 fragPos,
    float3 normal, Sampler2D texture1, Sampler2D texture2,
    Sampler2D texture3, Sampler2D texture4, Sampler2D texture5,
    UBO uniforms, float2 emitterSize) {
    int index = shadowParam.textureIndex;
    float2 dims = getTextureDimensions(index, texture1, texture2, texture3, texture4, texture5);
    if (any(dims <= 0.0)) return 0.0;
    float4 lightPosition = mul(float4(fragPos, 1.0), shadowParam.lightView);
    float4 clip = mul(lightPosition, shadowParam.lightProjection);
    if (clip.w <= 0.0) return 0.0;
    float3 projected = clip.xyz / clip.w;
    float2 uv = projected.xy * 0.5 + 0.5;
#if defined(ATLAS_BACKEND_METAL) || defined(ATLAS_BACKEND_VULKAN)
    uv.y = 1.0 - uv.y;
#endif
    if (any(uv < 0.0) || any(uv > 1.0) || projected.z < 0.0 || projected.z > 1.0)
        return 0.0;
    float3 lightDirection = normalize(
        -mul(float4(0.0, 0.0, -1.0, 0.0), inverseMatrix(shadowParam.lightView)).xyz);
    float bias = max(shadowParam.bias0 * (1.0 - saturate(dot(normal, lightDirection))),
                     shadowParam.bias0 * 0.25);
    float depth = projected.z - max(bias, 0.00001);
    float2 uvDx = ddx(uv);
    float2 uvDy = ddy(uv);
    float depthDx = ddx(projected.z);
    float depthDy = ddy(projected.z);
    float determinant = uvDx.x * uvDy.y - uvDx.y * uvDy.x;
    float2 depthGradient = abs(determinant) > 1e-10
        ? float2(uvDy.y * depthDx - uvDx.y * depthDy,
                 uvDx.x * depthDy - uvDy.x * depthDx) / determinant
        : float2(0.0);
    float rotation = 0.0;
    float2 filterRadius = 1.5 / dims;
    if (shadowParam.lightType == 2 && any(emitterSize > 0.0) &&
        abs(shadowParam.lightProjection[3][3]) < 0.0001) {
        float4x4 inverseProjection = inverseMatrix(shadowParam.lightProjection);
        float receiver = max(abs(lightPosition.z), 0.0001);
        float2 projectedEmitter = emitterSize * 0.25 *
            abs(float2(shadowParam.lightProjection[0][0], shadowParam.lightProjection[1][1]));
        float2 searchRadius = clamp(projectedEmitter / receiver, 2.0 / dims, 48.0 / dims);
        float blockerSum = 0.0;
        float blockers = 0.0;
        for (int i = 0; i < 48; ++i) {
            float angle = float(i) * 2.39996323 + rotation;
            float2 disk = sqrt((float(i) + 0.5) / 48.0) * float2(cos(angle), sin(angle));
            float2 tap = uv + disk * searchRadius;
            if (any(tap < 0.0) || any(tap > 1.0)) continue;
            float stored = sampleTextureAt(index, tap, texture1, texture2,
                                            texture3, texture4, texture5).x;
            if (stored < depth + dot(depthGradient, tap - uv)) {
                blockerSum += shadowViewDepth(stored, inverseProjection);
                blockers += 1.0;
            }
        }
        if (blockers > 0.0) {
            float blocker = max(blockerSum / blockers, 0.0001);
            float separation = max(receiver - blocker, 0.0) / blocker;
            filterRadius = clamp(projectedEmitter * separation / receiver,
                                 1.5 / dims, 32.0 / dims);
        }
    }
    float shadow = 0.0;
    for (int i = 0; i < 64; ++i) {
        float angle = float(i) * 2.39996323 + rotation;
        float2 disk = sqrt((float(i) + 0.5) / 64.0) * float2(cos(angle), sin(angle));
        shadow += filteredShadowComparison(index, uv + disk * filterRadius,
                                            depth + dot(depthGradient, disk * filterRadius), dims, depthGradient,
                                            texture1, texture2, texture3, texture4, texture5);
    }
    return shadow / 64.0;
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
             float metallic, float roughness, float transmission) {
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
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic) *
                (1.0 - transmission);
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
    float roughness, float transmission) {
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
    float param_8 = transmission;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7, param_8);
}

float3
calcPointLight(PointLight light, float3 fragPos,
               float3 N, float3 V,
               float3 F0, float3 albedo,
               float metallic, float roughness, float transmission) {
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
    float param_8 = transmission;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7, param_8);
}

float3
calcSpotLight(SpotLight light, float3 fragPos,
              float3 N, float3 V,
              float3 F0, float3 albedo,
              float metallic, float roughness, float transmission) {
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
    float attenuation = 1.0 / max(_distance * _distance, 1e-4);
    float fade =
        1.0 - smoothstep(range * 0.75, range, _distance);
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
    float param_8 = transmission;
    return evaluateBRDF(param, param_1, param_2, param_3, param_4, param_5,
                        param_6, param_7, param_8);
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
