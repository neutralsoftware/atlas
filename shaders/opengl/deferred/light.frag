#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterial;
uniform sampler2D ssao;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform samplerCube cubeMap1;
uniform samplerCube cubeMap2;
uniform samplerCube cubeMap3;
uniform samplerCube cubeMap4;
uniform samplerCube cubeMap5;
uniform samplerCube skybox;

struct AmbientLight {
    vec4 color;
    float intensity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    vec3 diffuse;
    vec3 specular;
};

struct AreaLight {
    vec3 position;
    vec3 right;
    vec3 up;
    vec2 size;
    vec3 diffuse;
    vec3 specular;
    float angle;
    float intensity;
    float range;
    int castsBothSides;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    int lightIndex;
    vec3 lightPos;
    int lightType;
};

struct Environment {
    float rimLightIntensity;
    vec3 rimLightColor;
    float bloomThreshold;
};

uniform AmbientLight ambientLight;
uniform DirectionalLight directionalLights[4];
uniform int directionalLightCount;
uniform PointLight pointLights[32];
uniform int pointLightCount;
uniform SpotLight spotlights[32];
uniform int spotlightCount;
uniform AreaLight areaLights[32];
uniform int areaLightCount;
uniform ShadowParameters shadowParams[10];
uniform int shadowParamCount;
uniform vec3 cameraPosition;
uniform bool useIBL;

uniform Environment environment;

const float PI = 3.14159265;

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    return vec4(0.0);
}

vec4 sampleCubeTextureAt(int textureIndex, vec3 direction) {
    if (textureIndex == 0) return texture(cubeMap1, direction);
    else if (textureIndex == 1) return texture(cubeMap2, direction);
    else if (textureIndex == 2) return texture(cubeMap3, direction);
    else if (textureIndex == 3) return texture(cubeMap4, direction);
    else if (textureIndex == 4) return texture(cubeMap5, direction);
    return vec4(0.0);
}

vec2 getTextureDimensions(int textureIndex) {
    if (textureIndex == 0) return vec2(textureSize(texture1, 0));
    else if (textureIndex == 1) return vec2(textureSize(texture2, 0));
    else if (textureIndex == 2) return vec2(textureSize(texture3, 0));
    else if (textureIndex == 3) return vec2(textureSize(texture4, 0));
    else if (textureIndex == 4) return vec2(textureSize(texture5, 0));
    return vec2(0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float calculateShadow(ShadowParameters shadowParam, vec3 fragPos, vec3 normal) {
    vec4 fragPosLightSpace = shadowParam.lightProjection * shadowParam.lightView * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDirWorld = normalize((inverse(shadowParam.lightView) * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
    float biasValue = shadowParam.bias;
    float ndotl = max(dot(normal, lightDirWorld), 0.0);
    float minBias = 0.0005;
    float bias = max(biasValue * (1.0 - ndotl), minBias);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - fragPos);
    vec2 shadowMapSize = getTextureDimensions(shadowParam.textureIndex);
    float avgDim = 0.5 * (shadowMapSize.x + shadowMapSize.y);
    float resFactor = clamp(1024.0 / max(avgDim, 1.0), 0.75, 1.25);
    float distFactor = clamp(distance / 800.0, 0.0, 1.0);
    float desiredKernel = mix(1.0, 1.5, distFactor) * resFactor;
    int kernelSize = int(clamp(floor(desiredKernel + 0.5), 1.0, 2.0));

    const vec2 poissonDisk[8] = vec2[](
            vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457),
            vec2(-0.203, 0.621), vec2(0.962, -0.195), vec2(0.473, -0.480),
            vec2(0.519, 0.767), vec2(0.185, -0.893)
        );
    float texelRadius = mix(1.0, 3.0, distFactor) * resFactor;
    vec2 filterRadius = texelSize * texelRadius;

    int sampleCount = 0;
    for (int i = 0; i < 8; ++i) {
        vec2 offset = poissonDisk[i] * filterRadius;
        vec2 uv = projCoords.xy + offset;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }
        float pcfDepth = sampleTextureAt(shadowParam.textureIndex, uv).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        sampleCount++;
    }
    if (sampleCount > 0) {
        shadow /= float(sampleCount);
    }

    return shadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos) {
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);
    if (currentDepth >= shadowParam.farPlane) return 0.0;

    float bias = 0.05;
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 8;
    const vec3 sampleOffsetDirections[] = vec3[](
            vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),
            vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),
            vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),
            vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),
            vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),
            vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),
            vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),
            vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271),
            vec3(-0.1120, 0.1234, -0.7446), vec3(-0.2130, -0.0782, -0.1379),
            vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843)
        );

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples);
    return shadow;
}

float shadowForLight(int lightType, int lightIndex, vec3 fragPos, vec3 normal) {
    for (int i = 0; i < shadowParamCount; ++i) {
        if (shadowParams[i].lightType != lightType ||
            shadowParams[i].lightIndex != lightIndex) continue;
        float shadow = lightType == 3
            ? calculatePointShadow(shadowParams[i], fragPos)
            : calculateShadow(shadowParams[i], fragPos, normal);
        return clamp(shadow * 0.85, 0.0, 1.0);
    }
    return 0.0;
}

vec3 evaluateBRDF(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    vec3 numerator = NDF * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.0001);
    vec3 specular = numerator / denominator;

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(-light.direction);
    vec3 radiance = light.diffuse * max(light.intensity, 0.0);
    return evaluateBRDF(L, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 calcPointLight(PointLight light, vec3 fragPos, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = light.position - fragPos;
    float distance = length(L);
    vec3 direction = distance > 0.0 ? (L / distance) : vec3(0.0, 0.0, 1.0);
    float attenuation = 1.0 / max(light.constant + light.linear * distance + light.quadratic * distance * distance, 0.0001);
    float fade = 1.0 - smoothstep(light.radius * 0.9, light.radius, distance);
    vec3 radiance = light.diffuse * max(light.intensity, 0.0) * attenuation * fade;
    return evaluateBRDF(direction, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 calcSpotLight(SpotLight light, vec3 fragPos, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = light.position - fragPos;
    float distance = length(L);
    vec3 direction = normalize(L);

    vec3 spotDirection = normalize(light.direction);
    float theta = dot(direction, -spotDirection);
    float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    float range = max(light.range, 0.001);
    float attenuation = 1.0 / (1.0 + (distance / range) + (distance * distance) / (range * range));
    float fade = 1.0 - smoothstep(range * 0.9, range, distance);
    vec3 radiance = light.diffuse * max(light.intensity, 0.0) * attenuation * intensity * fade;
    return evaluateBRDF(direction, radiance, N, V, F0, albedo, metallic, roughness);
}

vec3 sampleEnvironmentRadiance(vec3 direction) {
    return texture(skybox, direction).rgb;
}

vec3 acesToneMapping(vec3 color) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    return color;
}

vec3 getRimLight(
    vec3 fragPos,
    vec3 N,
    vec3 V,
    vec3 F0,
    vec3 albedo,
    float metallic,
    float roughness
) {
    N = normalize(N);
    V = normalize(V);

    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    rim *= mix(1.2, 0.3, roughness);

    vec3 rimColor = mix(vec3(1.0), albedo, metallic) * environment.rimLightColor;
    rimColor = mix(rimColor, F0, 0.5);

    float rimIntensity = environment.rimLightIntensity;

    vec3 rimLight = rimColor * rim * rimIntensity;

    float dist = length(cameraPosition - fragPos);
    rimLight /= (1.0 + dist * 0.1);

    return rimLight;
}

void main() {
    vec3 FragPos = texture(gPosition, TexCoord).xyz;
    vec3 N = normalize(texture(gNormal, TexCoord).xyz);
    vec4 albedoAo = texture(gAlbedoSpec, TexCoord);
    vec3 albedo = albedoAo.rgb;
    vec4 matData = texture(gMaterial, TexCoord);
    float metallic = clamp(matData.r, 0.0, 1.0);
    float roughness = clamp(matData.g, 0.0, 1.0);
    float ao = clamp(matData.b, 0.0, 1.0);

    float viewDistance = max(length(cameraPosition - FragPos), 1e-6);
    vec3 V = (cameraPosition - FragPos) / viewDistance;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float ssaoFactor = clamp(texture(ssao, TexCoord).r, 0.0, 1.0);
    float ssaoDesaturated = mix(1.0, ssaoFactor, 0.35);
    float occlusion = clamp(ao * (0.2 + 0.8 * ssaoDesaturated), 0.0, 1.0);
    float lightingOcclusion = clamp(ssaoDesaturated, 0.2, 1.0);

    vec3 directionalResult = vec3(0.0);
    for (int i = 0; i < directionalLightCount; ++i) {
        float shadow = shadowForLight(0, i, FragPos, N);
        directionalResult += calcDirectionalLight(directionalLights[i], N, V, F0, albedo, metallic, roughness) * (1.0 - shadow);
    }

    vec3 pointResult = vec3(0.0);
    for (int i = 0; i < pointLightCount; ++i) {
        if (length(pointLights[i].position - FragPos) >= pointLights[i].radius) continue;
        float shadow = shadowForLight(3, i, FragPos, N);
        pointResult += calcPointLight(pointLights[i], FragPos, N, V, F0, albedo, metallic, roughness) * (1.0 - shadow);
    }

    vec3 spotResult = vec3(0.0);
    for (int i = 0; i < spotlightCount; ++i) {
        if (length(spotlights[i].position - FragPos) >= spotlights[i].range) continue;
        float shadow = shadowForLight(1, i, FragPos, N);
        spotResult += calcSpotLight(spotlights[i], FragPos, N, V, F0, albedo, metallic, roughness) * (1.0 - shadow);
    }

    vec3 areaResult = vec3(0.0);
    for (int i = 0; i < areaLightCount; ++i) {
        vec3 P = areaLights[i].position;
        vec3 R = normalize(areaLights[i].right);
        vec3 U = normalize(areaLights[i].up);
        vec2 halfSize = areaLights[i].size * 0.5;

        vec3 toPoint = FragPos - P;
        float s = clamp(dot(toPoint, R), -halfSize.x, halfSize.x);
        float t = clamp(dot(toPoint, U), -halfSize.y, halfSize.y);
        vec3 Q = P + R * s + U * t;

        vec3 Lvec = Q - FragPos;
        float dist = length(Lvec);
        if (dist > 0.0001) {
            vec3 L = Lvec / dist;
            vec3 Nl = normalize(cross(R, U));
            float ndotl = dot(Nl, -L);

            float facing = (areaLights[i].castsBothSides != 0) ? abs(ndotl) : max(ndotl, 0.0);
            float cosTheta = cos(radians(areaLights[i].angle));

            if (facing >= cosTheta && facing > 0.0) {
                float range = max(areaLights[i].range, 0.001);
                float attenuation = 1.0 / (1.0 + (dist / range) + (dist * dist) / (range * range));
                float fade = 1.0 - smoothstep(range * 0.9, range, dist);
                vec3 radiance = areaLights[i].diffuse * max(areaLights[i].intensity, 0.0) * attenuation * facing * fade;
                float shadow = shadowForLight(2, i, FragPos, N);
                areaResult += evaluateBRDF(L, radiance, N, V, F0, albedo, metallic, roughness) * (1.0 - shadow);
            }
        }
    }
    vec3 rimResult = getRimLight(FragPos, N, V, F0, albedo, metallic, roughness);
    vec3 lighting = (directionalResult + pointResult + spotResult + areaResult + rimResult) * lightingOcclusion;

    vec3 ambient = ambientLight.color.rgb * ambientLight.intensity * albedo * occlusion;

    vec3 iblContribution = vec3(0.0);
    if (useIBL) {
        vec3 irradiance = sampleEnvironmentRadiance(N);
        vec3 diffuseIBL = irradiance * albedo;

        vec3 reflection = reflect(-V, N);
        vec3 specularEnv = sampleEnvironmentRadiance(reflection);

        vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float roughnessAttenuation = mix(1.0, 0.15, clamp(roughness, 0.0, 1.0));
        vec3 specularIBL = specularEnv * roughnessAttenuation;

        iblContribution = (kD * diffuseIBL + kS * specularIBL) * occlusion;
    }

    vec3 finalColor = ambient + lighting + iblContribution;

    if (!useIBL) {
        vec3 I = normalize(FragPos - cameraPosition);
        vec3 R = reflect(-I, N);

        vec3 F = fresnelSchlick(max(dot(N, -I), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 envColor = texture(skybox, R).rgb;
        vec3 reflection = envColor * kS;

        finalColor = mix(finalColor, reflection, F0);
    }

    FragColor = vec4(finalColor, 1.0);

    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > environment.bloomThreshold) {
        BrightColor = vec4(FragColor.rgb, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    FragColor.rgb = acesToneMapping(FragColor.rgb);
}
