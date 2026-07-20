#version 450
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec4 gMaterial;

layout(location = 0) in vec4 outColor;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 FragPos;
layout(location = 4) in mat3 TBN;

const int TEXTURE_COLOR = 0;
const int TEXTURE_SPECULAR = 1;
const int TEXTURE_NORMAL = 5;
const int TEXTURE_PARALLAX = 6;
const int TEXTURE_METALLIC = 9;
const int TEXTURE_ROUGHNESS = 10;
const int TEXTURE_AO = 11;
const int TEXTURE_OPACITY = 12;

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

layout(set = 1, binding = 0) uniform UBO {
    int textureTypes[16];
    int textureCount;
    bool useTexture;
    bool useColor;
    vec3 cameraPosition;
    vec2 textureScale;
    vec2 textureOffset;
};

layout(push_constant) uniform MaterialPush {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float reflectivity;
} material;

vec2 texCoord;

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
    if (textureIndex == -1) return currentTexCoords;

    float currentDepthMapValue = 1.0 - sampleTextureAt(textureIndex, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - sampleTextureAt(textureIndex, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - sampleTextureAt(textureIndex, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float denom = afterDepth - beforeDepth;
    float weight = abs(denom) > 1e-4
                       ? clamp(afterDepth / denom, 0.0, 1.0)
                       : 0.0;
    currentTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return currentTexCoords;
}

void main() {
    texCoord = TexCoord * textureScale + textureOffset;

    // Only apply parallax mapping if a parallax texture exists
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

    vec4 sampledColor = enableTextures(TEXTURE_COLOR);
    bool hasColorTexture = sampledColor != vec4(-1.0);

    vec4 baseColor = vec4(material.albedo, 1.0);
    vec4 albedoTex = enableTextures(TEXTURE_COLOR);
    if (albedoTex != vec4(-1.0)) {
        baseColor *= albedoTex;
    }
    vec4 opacityTex = enableTextures(TEXTURE_OPACITY);
    if (opacityTex != vec4(-1.0)) {
        if (opacityTex.r < 0.1) {
            discard;
        }
    } else if (baseColor.a < 0.1) {
        discard;
    }


    vec4 normTexture = enableTextures(TEXTURE_NORMAL);
    vec3 normal;
    if (normTexture.r != -1.0 && normTexture.g != -1.0 && normTexture.b != -1.0) {
        vec3 tangentNormal = normalize(normTexture.rgb * 2.0 - 1.0);
        normal = normalize(TBN * tangentNormal);
    } else {
        normal = normalize(Normal);
    }

    vec3 albedoColor = baseColor.rgb;

    float metallicValue = material.metallic;
    vec4 metallicTex = enableTextures(TEXTURE_METALLIC);
    if (metallicTex != vec4(-1.0)) {
        metallicValue *= metallicTex.r;
    }

    float roughnessValue = material.roughness;
    vec4 roughnessTex = enableTextures(TEXTURE_ROUGHNESS);
    if (roughnessTex != vec4(-1.0)) {
        roughnessValue *= roughnessTex.r;
    }

    float aoValue = material.ao;
    vec4 aoTex = enableTextures(TEXTURE_AO);
    if (aoTex != vec4(-1.0)) {
        aoValue *= aoTex.r;
    }

    metallicValue = clamp(metallicValue, 0.0, 1.0);
    roughnessValue = clamp(roughnessValue, 0.0, 1.0);
    aoValue = clamp(aoValue, 0.0, 1.0);

    float nonlinearDepth = gl_FragCoord.z;
    gPosition = vec4(FragPos, nonlinearDepth);

    vec3 n = normalize(normal);
    if (!all(equal(n, n)) || length(n) < 1e-4) {
        n = normalize(Normal);
    }
    gNormal = vec4(n, 1.0);

    vec3 a = clamp(albedoColor, 0.0, 1.0);
    if (!all(equal(a, a))) {
        a = vec3(0.0);
    }
    gAlbedoSpec = vec4(a, aoValue);

    gMaterial = vec4(metallicValue, roughnessValue, aoValue, 1.0);
}
