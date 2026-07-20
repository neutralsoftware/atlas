#version 450

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec4 outColor;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 FragPos;
layout(location = 4) in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_DEPTH_CUBE = 4;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;
const int TEXTURE_METALLIC = 9;
const int TEXTURE_ROUGHNESS = 10;
const int TEXTURE_AO = 11;
const int TEXTURE_OPACITY = 12;
const int TEXTURE_HDR_ENVIRONMENT = 13;

const float PI = 3.14159265;

vec2 texCoord;

// ----- Structures -----
struct DirectionalLight {
    vec3 direction;
    float _pad1;
    vec3 diffuse;
    float _pad2;
    vec3 specular;
    float intensity;
};

struct PointLight {
    vec3 position;
    float _pad1;
    vec3 diffuse;
    float _pad2;
    vec3 specular;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
    float _pad3;
};

struct SpotLight {
    vec3 position;
    float _pad1;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float intensity;
    float range;
    float _pad4;
    vec3 diffuse;
    float _pad5;
    vec3 specular;
    float _pad6;
};

struct AreaLight {
    vec3 position;
    float _pad1;
    vec3 right;
    float _pad2;
    vec3 up;
    float _pad3;
    vec2 size;
    float _pad4;
    float _pad5;
    vec3 diffuse;
    float _pad6;
    vec3 specular;
    float angle;
    int castsBothSides;
    float intensity;
    float range;
    float _pad9;
};

struct ShadowParameters {
    mat4 lightView;
    mat4 lightProjection;
    float bias;
    int textureIndex;
    float farPlane;
    float _pad1;
    vec3 lightPos;
    int lightType;
};

// ----- Textures -----
layout(set = 2, binding = 0) uniform sampler2D texture1;
layout(set = 2, binding = 1) uniform sampler2D texture2;
layout(set = 2, binding = 2) uniform sampler2D texture3;
layout(set = 2, binding = 3) uniform sampler2D texture4;
layout(set = 2, binding = 4) uniform sampler2D texture5;
layout(set = 2, binding = 5) uniform sampler2D texture6;
layout(set = 2, binding = 6) uniform sampler2D texture7;
layout(set = 2, binding = 7) uniform sampler2D texture8;
layout(set = 2, binding = 8) uniform sampler2D texture9;
layout(set = 2, binding = 9) uniform sampler2D texture10;
layout(set = 2, binding = 10) uniform samplerCube skybox;
layout(set = 2, binding = 11) uniform samplerCube cubeMap1;
layout(set = 2, binding = 12) uniform samplerCube cubeMap2;
layout(set = 2, binding = 13) uniform samplerCube cubeMap3;
layout(set = 2, binding = 14) uniform samplerCube cubeMap4;
layout(set = 2, binding = 15) uniform samplerCube cubeMap5;

// ----- Uniforms -----
layout(set = 1, binding = 0) uniform Uniforms {
    int textureTypes[16];
    int textureCount;
    vec3 cameraPosition;
    vec2 textureScale;
    vec2 textureOffset;
};

layout(push_constant) uniform PushConstants {
    bool useTexture;
    bool useColor;
    bool useIBL;
    
    int directionalLightCount;
    int pointLightCount;
    int spotlightCount;
    int areaLightCount;
    int shadowParamCount;
};

layout(set = 1, binding = 1) uniform Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
} material;

layout(set = 1, binding = 2) uniform Environment {
    float rimLightIntensity;
    vec3 rimLightColor;
} environment;

layout(set = 1, binding = 3) uniform AmbientLight {
    vec4 color;
    float intensity;
    vec3 _pad0;
} ambientLight;

layout(set = 3, binding = 1) buffer DirectionalLightsUBO {
    DirectionalLight directionalLights[];
};

layout(set = 3, binding = 2) buffer PointLightsUBO {
    PointLight pointLights[];
};

layout(set = 3, binding = 3) buffer SpotLightsUBO {
    SpotLight spotlights[];
};

layout(set = 3, binding = 4) buffer AreaLightsUBO {
    AreaLight areaLights[];
};

layout(set = 3, binding = 5) buffer ShadowParametersUBO {
    ShadowParameters shadowParams[];
};

// ----- Helper Functions -----
vec4 enableTextures(int type) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == type) {
            if (i == 0) color += texture(texture1, texCoord);
            else if (i == 1) color += texture(texture2, texCoord);
            else if (i == 2) color += texture(texture3, texCoord);
            else if (i == 3) color += texture(texture4, texCoord);
            else if (i == 4) color += texture(texture5, texCoord);
            else if (i == 5) color += texture(texture6, texCoord);
            else if (i == 6) color += texture(texture7, texCoord);
            else if (i == 7) color += texture(texture8, texCoord);
            else if (i == 8) color += texture(texture9, texCoord);
            else if (i == 9) color += texture(texture10, texCoord);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
}

vec4 enableCubeMaps(int type, vec3 direction) {
    vec4 color = vec4(0.0);
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (type == i + 10) {
            if (i == 0) color += texture(cubeMap1, direction);
            else if (i == 1) color += texture(cubeMap2, direction);
            else if (i == 2) color += texture(cubeMap3, direction);
            else if (i == 3) color += texture(cubeMap4, direction);
            else if (i == 4) color += texture(cubeMap5, direction);
            count++;
        }
    }
    if (count > 0) color /= float(count);
    if (count == 0) return vec4(-1.0);
    return color;
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
    else if (textureIndex == 5) return vec2(textureSize(texture6, 0));
    else if (textureIndex == 6) return vec2(textureSize(texture7, 0));
    else if (textureIndex == 7) return vec2(textureSize(texture8, 0));
    else if (textureIndex == 8) return vec2(textureSize(texture9, 0));
    else if (textureIndex == 9) return vec2(textureSize(texture10, 0));
    return vec2(0);
}

vec4 sampleTextureAt(int textureIndex, vec2 uv) {
    if (textureIndex == 0) return texture(texture1, uv);
    else if (textureIndex == 1) return texture(texture2, uv);
    else if (textureIndex == 2) return texture(texture3, uv);
    else if (textureIndex == 3) return texture(texture4, uv);
    else if (textureIndex == 4) return texture(texture5, uv);
    else if (textureIndex == 5) return texture(texture6, uv);
    else if (textureIndex == 6) return texture(texture7, uv);
    else if (textureIndex == 7) return texture(texture8, uv);
    else if (textureIndex == 8) return texture(texture9, uv);
    else if (textureIndex == 9) return texture(texture10, uv);
    return vec4(0.0);
}

vec3 getSpecularColor() {
    vec4 specTex = enableTextures(TEXTURE_SPECULAR);
    vec3 specColor = material.albedo;
    if (specTex.r != -1.0 || specTex.g != -1.0 || specTex.b != -1.0) {
        specColor *= specTex.rgb;
    }
    return specColor;
}

vec4 applyGammaCorrection(vec4 color, float gamma) {
    return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}

vec2 directionToEquirect(vec3 direction) {
    vec3 dir = normalize(direction);
    float phi = atan(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    return vec2((phi + PI) / (2.0 * PI), theta / PI);
}

vec3 sampleHDRTexture(int textureIndex, vec3 direction) {
    vec2 uv = directionToEquirect(direction);
    return sampleTextureAt(textureIndex, uv).rgb;
}

vec3 sampleEnvironmentRadiance(vec3 direction) {
    vec3 envColor = vec3(0.0);
    int count = 0;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_HDR_ENVIRONMENT) {
            envColor += sampleHDRTexture(i, direction);
            count++;
        }
    }
    if (count == 0) {
        return vec3(0.0);
    }
    return envColor / float(count);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    vec3 v = normalize(viewDir);
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), v)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    const float heightScale = 0.04;
    vec2 P = (v.xy / max(v.z, 0.05)) * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    int textureIndex = -1;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            textureIndex = i;
            break;
        }
    }
    if (textureIndex == -1) return texCoords;
    float currentDepthMapValue = 1.0 - sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return currentTexCoords;
}

vec3 reinhardToneMapping(vec3 hdrColor) {
    vec3 color = vec3(1.0) - exp(-hdrColor * 1.0);
    color = pow(color, vec3(1.0 / 2.2));
    return color;
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

// ----- Environment Mapping -----
vec4 getEnvironmentReflected(vec4 color) {
    vec3 I = normalize(FragPos - cameraPosition);
    vec3 R = reflect(I, normalize(Normal));
    return mix(color, vec4(texture(skybox, R).rgb, 1.0), material.reflectivity);
}

// ----- Rim Light -----
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

// ----- PBR -----
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
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

vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 radiance, vec3 albedo, float metallic, float roughness, float reflectivity) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
    return Lo;
}

vec3 calculatePBRWithAttenuation(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 radianceAttenuated, vec3 albedo, float metallic, float roughness, float reflectivity) {
    vec3 H = normalize(V + L);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    vec3 Lo = (kD * albedo / 3.14159265 + specular) * radianceAttenuated * NdotL;
    return Lo;
}

// ----- Directional Light -----
vec3 calcAllDirectionalLights(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < directionalLightCount; i++) {
        vec3 L = normalize(-directionalLights[i].direction);
        vec3 radiance = directionalLights[i].diffuse * max(directionalLights[i].intensity, 0.0);
        Lo += calculatePBR(N, V, L, F0, radiance, albedo, metallic, roughness, reflectivity);
    }

    return Lo;
}

// ----- Point Light -----
float calcAttenuation(PointLight light, vec3 fragPos) {
    float distance = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);
}

vec3 calcAllPointLights(vec3 fragPos, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pointLightCount; i++) {
        vec3 L = pointLights[i].position - fragPos;
        float distance = length(L);

        distance = max(distance, 0.001);

        L = normalize(L);

        float range = max(pointLights[i].radius, 0.001);
        vec3 radiance = pointLights[i].diffuse * max(pointLights[i].intensity, 0.0);
        float attenuation = 1.0 / (1.0 + (distance / range) + (distance * distance) / (range * range));
        float fade = 1.0 - smoothstep(range * 0.9, range, distance);
        vec3 radianceAttenuated = radiance * attenuation;
        radianceAttenuated *= fade;

        vec3 H = normalize(V + L);

        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / 3.14159265 + specular) * radianceAttenuated * NdotL;
    }
    return Lo;
}

// ----- Spot Light -----
vec3 calcAllSpotLights(vec3 N, vec3 fragPos, vec3 L, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0, float reflectivity) {
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < spotlightCount; i++) {
        vec3 L = normalize(spotlights[i].position - fragPos);

        vec3 spotDirection = normalize(spotlights[i].direction);
        float theta = dot(L, -spotDirection);
        float intensity = smoothstep(spotlights[i].outerCutOff, spotlights[i].cutOff, theta);

        float distance = length(spotlights[i].position - fragPos);
        distance = max(distance, 0.001);
        float range = max(spotlights[i].range, 0.001);
        float attenuation = 1.0 / (1.0 + (distance / range) + (distance * distance) / (range * range));
        float fade = 1.0 - smoothstep(range * 0.9, range, distance);

        vec3 radiance = spotlights[i].diffuse * max(spotlights[i].intensity, 0.0) * attenuation * intensity * fade;

        Lo += calculatePBR(N, viewDir, L, F0, radiance, albedo, metallic, roughness, reflectivity);
    }

    return Lo;
}

// ----- Shadow Calculations -----
float calculateShadow(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform from [-1,1] to [0,1] for texture sampling
    projCoords = projCoords * 0.5 + 0.5;
    // Flip Y because shadow map was rendered with OpenGL-style projection but sampled in Vulkan
    projCoords.y = 1.0 - projCoords.y;
    // Note: Shadow maps are rendered without Y-flip projection, so no Y-flip needed here

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    vec3 lightDir = normalize((inverse(shadowParam.lightView) * vec4(0.0, 0.0, -1.0, 0.0)).xyz);
    vec3 normal = normalize(Normal);
    float biasValue = shadowParam.bias;
    float bias = max(biasValue * (1.0 - dot(normal, lightDir)), biasValue);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / getTextureDimensions(shadowParam.textureIndex);

    float distance = length(cameraPosition - FragPos);
    int kernelSize = int(mix(1.0, 3.0, clamp(distance / 100.0, 0.0, 1.0)));

    int sampleCount = 0;
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            float pcfDepth = sampleTextureAt(shadowParam.textureIndex,
                    projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);

    return shadow;
}

float calculateShadowRaw(ShadowParameters shadowParam, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
            projCoords.y < 0.0 || projCoords.y > 1.0 ||
            projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float closestDepth = sampleTextureAt(shadowParam.textureIndex, projCoords.xy).r;

    return currentDepth > closestDepth ? 1.0 : 0.0;
}

float calculateAllShadows() {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        vec4 fragPosLightSpace = shadowParams[i].lightProjection * shadowParams[i].lightView * vec4(FragPos, 1.0);
        float shadow = calculateShadow(shadowParams[i], fragPosLightSpace);
        totalShadow = max(totalShadow, shadow);
    }
    return totalShadow;
}

float calculatePointShadow(ShadowParameters shadowParam, vec3 fragPos)
{
    vec3 fragToLight = fragPos - shadowParam.lightPos;
    float currentDepth = length(fragToLight);

    float bias = 0.05;
    float shadow = 0.0;

    float diskRadius = (1.0 + (currentDepth / shadowParam.farPlane)) * 0.05;

    const int samples = 54;
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
            vec3(0.2944, -0.3112, -0.2645), vec3(-0.4564, 0.4175, -0.1843),
            // remaining random-ish points
            vec3(0.1234, -0.5678, 0.7890), vec3(-0.6789, 0.2345, -0.4567),
            vec3(0.3456, -0.7890, 0.1234), vec3(-0.2345, 0.5678, -0.6789),
            vec3(0.7890, 0.1234, 0.5678), vec3(-0.5678, -0.6789, 0.2345),
            vec3(0.4567, 0.7890, -0.2345), vec3(-0.7890, 0.3456, -0.5678),
            vec3(0.6789, -0.2345, 0.7890), vec3(-0.1234, 0.6789, -0.4567),
            vec3(0.2345, -0.5678, 0.6789), vec3(-0.3456, 0.7890, -0.1234),
            vec3(0.5678, 0.2345, -0.7890), vec3(-0.6789, -0.5678, 0.3456),
            vec3(0.7890, -0.3456, 0.4567), vec3(-0.2345, 0.1234, -0.6789),
            vec3(0.4567, 0.7890, -0.5678), vec3(-0.5678, 0.2345, 0.6789),
            vec3(0.3456, -0.7890, -0.1234), vec3(-0.7890, 0.5678, -0.2345),
            vec3(0.6789, -0.1234, 0.3456), vec3(-0.4567, 0.7890, 0.2345),
            vec3(0.5678, -0.6789, 0.7890), vec3(-0.3456, 0.5678, -0.6789),
            vec3(0.2345, -0.7890, 0.5678), vec3(-0.6789, 0.2345, -0.1234),
            vec3(0.7890, -0.3456, -0.5678), vec3(-0.5678, 0.6789, 0.2345),
            vec3(0.4567, -0.7890, 0.3456), vec3(-0.2345, 0.1234, -0.7890),
            vec3(0.3456, -0.5678, 0.6789), vec3(-0.7890, 0.4567, -0.3456),
            vec3(0.6789, -0.1234, -0.5678), vec3(-0.4567, 0.2345, 0.7890)
        );

    for (int i = 0; i < samples; ++i)
    {
        vec3 sampleDir = normalize(fragToLight + sampleOffsetDirections[i] * diskRadius);
        float closestDepth = sampleCubeTextureAt(shadowParam.textureIndex, sampleDir).r * shadowParam.farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);
    return shadow;
}

float calculateAllPointShadows(vec3 fragPos) {
    float totalShadow = 0.0;
    for (int i = 0; i < shadowParamCount; i++) {
        if (shadowParams[i].lightType == 3) {
            float shadow = calculatePointShadow(shadowParams[i], fragPos);
            totalShadow = max(totalShadow, shadow);
        }
    }
    return totalShadow;
}

// ----- Main -----
void main() {
    texCoord = TexCoord * textureScale + textureOffset;

    bool hasParallaxMap = false;
    for (int i = 0; i < textureCount; i++) {
        if (textureTypes[i] == TEXTURE_PARALLAX) {
            hasParallaxMap = true;
            break;
        }
    }

    if (hasParallaxMap) {
        vec3 tangentViewDir = normalize(transpose(TBN) * (cameraPosition - FragPos));
        texCoord = parallaxMapping(texCoord, tangentViewDir);
    }

    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 N;

    if (normTexture.r != -1.0 && normTexture.g != -1.0 && normTexture.b != -1.0) {
        vec3 tangentNormal = normalize(normTexture.rgb * 2.0 - 1.0);
        N = normalize(TBN * tangentNormal);
    } else {
        N = normalize(Normal);
    }

    N = normalize(N);
    vec3 V = normalize(cameraPosition - FragPos);

    vec3 albedo = material.albedo;
    vec4 albedoTex = enableTextures(TEXTURE_COLOR);
    if (albedoTex != vec4(-1.0)) {
        albedo *= albedoTex.rgb;
    }
    vec4 opacityTex = enableTextures(TEXTURE_OPACITY);
    if (opacityTex != vec4(-1.0)) {
        if (opacityTex.r < 0.1) {
            discard;
        }
    } else if (albedoTex != vec4(-1.0) && albedoTex.a < 0.1) {
        discard;
    }

    float metallic = material.metallic;
    vec4 metallicTex = enableTextures(TEXTURE_METALLIC);
    if (metallicTex != vec4(-1.0)) {
        metallic *= metallicTex.r;
    }

    float roughness = material.roughness;
    vec4 roughnessTex = enableTextures(TEXTURE_ROUGHNESS);
    if (roughnessTex != vec4(-1.0)) {
        roughness *= roughnessTex.r;
    }

    float ao = material.ao;
    vec4 aoTex = enableTextures(TEXTURE_AO);
    if (aoTex != vec4(-1.0)) {
        ao *= aoTex.r;
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float directionalShadow = 0.0;
    float spotShadow = 0.0;
    float areaShadow = 0.0;
    float pointShadow = 0.0;

    if (shadowParamCount > 0) {
        for (int i = 0; i < shadowParamCount; i++) {
            if (shadowParams[i].lightType == 0) {
                vec4 fragPosLightSpace = shadowParams[i].lightProjection *
                        shadowParams[i].lightView *
                        vec4(FragPos, 1.0);
                directionalShadow = max(directionalShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
            } else if (shadowParams[i].lightType == 1) {
                vec4 fragPosLightSpace = shadowParams[i].lightProjection *
                        shadowParams[i].lightView *
                        vec4(FragPos, 1.0);
                spotShadow = max(spotShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
            } else if (shadowParams[i].lightType == 2) {
                vec4 fragPosLightSpace = shadowParams[i].lightProjection *
                        shadowParams[i].lightView *
                        vec4(FragPos, 1.0);
                areaShadow = max(areaShadow, calculateShadow(shadowParams[i], fragPosLightSpace));
            } else if (shadowParams[i].lightType == 3) {
                pointShadow = max(pointShadow, calculatePointShadow(shadowParams[i], FragPos));
            }
        }
    }

    float reflectivity = material.reflectivity;
    vec3 viewDir = normalize(cameraPosition - FragPos);

    vec3 lighting = vec3(0.0);

    lighting += calcAllDirectionalLights(N, V, albedo, metallic, roughness, F0, reflectivity) * (1.0 - directionalShadow);
    lighting += calcAllPointLights(FragPos, N, V, albedo, metallic, roughness, F0, reflectivity) * (1.0 - pointShadow);
    lighting += calcAllSpotLights(N, FragPos, V, viewDir, albedo, metallic, roughness, F0, reflectivity) * (1.0 - spotShadow);
    lighting += getRimLight(FragPos, N, V, F0, albedo, metallic, roughness);

    {
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
                    vec3 H = normalize(V + L);
                    float NDF = distributionGGX(N, H, roughness);
                    float G = geometrySmith(N, V, L, roughness);
                    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
                    vec3 kS = F;
                    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
                    vec3 numerator = NDF * G * F;
                    float denominator = max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);
                    vec3 specular = numerator / denominator;
                    float NdotL = max(dot(N, L), 0.0);
                    areaResult += (kD * albedo / PI + specular) * radiance * NdotL;
                }
            }
        }
        lighting += areaResult * (1.0 - areaShadow);
    }

    float aoClamped = clamp(ao, 0.0, 1.0);
    float aoWithFloor = max(aoClamped, 0.2);

    float ambientIntensity = ambientLight.intensity;
    vec3 ambientColor = ambientLight.color.rgb;
    if (ambientIntensity <= 0.0001) {
        ambientIntensity = 0.03;
    }
    if (dot(ambientColor, ambientColor) <= 1e-6) {
        ambientColor = vec3(1.0);
    }

    vec3 ambient = albedo * ambientIntensity * ambientColor * aoWithFloor;

    vec3 iblContribution = vec3(0.0);
    if (useIBL) {
        vec3 irradiance = sampleEnvironmentRadiance(N);
        vec3 diffuseIBL = irradiance * albedo;

        vec3 reflection = reflect(-V, N);
        vec3 specularEnv = sampleEnvironmentRadiance(reflection);

        vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float roughnessAttenuation = mix(1.0, 0.15, clamp(roughness, 0.0, 1.0));
        vec3 specularIBL = specularEnv * roughnessAttenuation;

        iblContribution = (kD * diffuseIBL + kS * specularIBL) * aoWithFloor;
    }

    vec3 color = ambient + lighting + iblContribution;

    FragColor = vec4(color, 1.0);

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        BrightColor = vec4(color, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    FragColor.rgb = acesToneMapping(FragColor.rgb);
}
