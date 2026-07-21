#version 450
layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D gPosition;
layout(set = 2, binding = 1) uniform sampler2D gNormal;
layout(set = 2, binding = 2) uniform sampler2D gAlbedoSpec;
layout(set = 2, binding = 3) uniform sampler2D gMaterial;
layout(set = 2, binding = 4) uniform sampler2D sceneColor;
layout(set = 2, binding = 5) uniform sampler2D gDepth;
layout(set = 2, binding = 6) uniform sampler2D historyTexture;
layout(set = 3, binding = 0) uniform samplerCube skybox;

layout(set = 1, binding = 0) uniform Uniforms {
    mat4 inverseProjection;
    mat4 inverseView;
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
};

layout(set = 1, binding = 1) uniform SSRParameters {
    float maxDistance;
    float resolution;
    int steps;
    float thickness;
    float maxRoughness;
    float historyWeight;
    int debugMode;
};

const float PI = 3.14159265359;

float LinearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 sampleSkyReflection(vec3 normal, vec3 viewDir, vec3 albedo, float metallic) {
    vec3 reflected = reflect(-normalize(viewDir), normalize(normal));
    vec3 skyColor = texture(skybox, reflected).rgb;
    float skyLuma = dot(skyColor, vec3(0.2126, 0.7152, 0.0722));
    if (skyLuma < 0.001) {
        float t = clamp(reflected.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 horizon = vec3(0.74, 0.79, 0.88);
        vec3 zenith = vec3(0.50, 0.64, 0.84);
        skyColor = mix(horizon, zenith, t);
    }
    float tintStrength = metallic * 0.35;
    vec3 tint = mix(vec3(1.0), albedo, tintStrength);
    return skyColor * tint;
}

vec4 SSR(vec3 worldPos, vec3 normal, vec3 viewDir, float roughness, float metallic, float reflectivity, vec3 albedo) {
    vec3 viewPos = (view * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize((view * vec4(normal, 0.0)).xyz);
    float mirrorFactor = clamp(max(metallic, reflectivity) * (1.0 - roughness), 0.0, 1.0);
    vec3 skyReflection = sampleSkyReflection(normal, viewDir, albedo, metallic);
    
    vec3 viewDirection = normalize(viewPos);
    vec3 viewReflect = normalize(reflect(viewDirection, viewNormal));
    
    if (viewReflect.z > 0.0) {
        return vec4(0.0);
    }
    
    vec3 rayOrigin = viewPos + viewNormal * 0.01;
    vec3 rayDir = viewReflect;
    
    float stepSize = maxDistance / float(steps);
    vec3 currentPos = rayOrigin;
    
    float jitter = hash(gl_FragCoord.xy) * roughness * 0.25;
    currentPos += rayDir * stepSize * jitter;
    
    vec2 hitUV = vec2(-1.0);
    float hitDepth = 0.0;
    bool hit = false;
    vec2 fallbackUV = TexCoord;
    bool hasFallbackUV = false;
    
    vec3 lastPos = currentPos;
    
    for (int i = 0; i < steps; i++) {
        float t = float(i) / float(steps);
        float adaptiveStep = mix(0.3, 1.0, t);
        currentPos += rayDir * stepSize * adaptiveStep;
        
        vec4 projectedPos = projection * vec4(currentPos, 1.0);
        projectedPos.xyz /= projectedPos.w;
        vec2 screenUV = projectedPos.xy * 0.5 + 0.5;
        
        if (screenUV.x >= 0.0 && screenUV.x <= 1.0 &&
            screenUV.y >= 0.0 && screenUV.y <= 1.0) {
            fallbackUV = screenUV;
            hasFallbackUV = true;
        } else {
            break;
        }

        float hierarchyLevel = 0.0;
        float rawDepth = texture(gDepth, screenUV).r;
        if (rawDepth >= 0.9999) {
            currentPos += rayDir * stepSize * (exp2(hierarchyLevel) - 1.0);
            lastPos = currentPos;
            continue;
        }
        
        vec3 sampleWorldPos = texture(gPosition, screenUV).xyz;
        vec3 sampleNormal = texture(gNormal, screenUV).xyz;
        if (dot(sampleNormal, sampleNormal) < 0.01) {
            lastPos = currentPos;
            continue;
        }
        if (length(sampleWorldPos - worldPos) < 0.08) {
            lastPos = currentPos;
            continue;
        }
        vec3 sampleViewPos = (view * vec4(sampleWorldPos, 1.0)).xyz;
        
        float sampleDepth = -sampleViewPos.z;
        float currentDepth = -currentPos.z;
        vec3 sampleViewNormal = normalize((view * vec4(sampleNormal, 0.0)).xyz);
        if (dot(sampleViewNormal, viewNormal) > 0.92 &&
            abs(sampleDepth - currentDepth) < thickness * 1.5) {
            lastPos = currentPos;
            continue;
        }
        
        float depthDiff = sampleDepth - currentDepth;
        
        if (depthDiff > 0.0 && depthDiff < thickness) {
            vec3 binarySearchStart = lastPos;
            vec3 binarySearchEnd = currentPos;
            
            for (int j = 0; j < 8; j++) {  
                vec3 midPoint = (binarySearchStart + binarySearchEnd) * 0.5;
                
                vec4 midProj = projection * vec4(midPoint, 1.0);
                midProj.xyz /= midProj.w;
                vec2 midUV = midProj.xy * 0.5 + 0.5;
                if (midUV.x < 0.0 || midUV.x > 1.0 || midUV.y < 0.0 || midUV.y > 1.0) {
                    break;
                }

                float midRawDepth = texture(gDepth, midUV).r;
                if (midRawDepth >= 0.9999) {
                    binarySearchStart = midPoint;
                    continue;
                }
                
                vec3 midSampleWorldPos = texture(gPosition, midUV).xyz;
                vec3 midSampleViewPos = (view * vec4(midSampleWorldPos, 1.0)).xyz;
                float midSampleDepth = -midSampleViewPos.z;  
                float midCurrentDepth = -midPoint.z;        
                
                if (midCurrentDepth < midSampleDepth) {
                    binarySearchStart = midPoint;
                } else {
                    binarySearchEnd = midPoint;
                }
            }
            
            vec4 finalProj = projection * vec4(binarySearchEnd, 1.0);
            finalProj.xyz /= finalProj.w;
            hitUV = finalProj.xy * 0.5 + 0.5;
            hitDepth = length(currentPos - rayOrigin);
            hit = true;
            break;
        }
        
        lastPos = currentPos;
    }
    
    if (!hit) {
        return vec4(0.0);
    }
    
    vec3 hitColor = texture(sceneColor, hitUV).rgb;
    float tintStrength = metallic * 0.35;
    vec3 metalTint = mix(vec3(1.0), albedo, tintStrength);
    hitColor *= metalTint;
    float hitDepthRaw = texture(gDepth, hitUV).r;
    vec3 hitNormalSample = texture(gNormal, hitUV).xyz;
    float hitNormalLenSq = dot(hitNormalSample, hitNormalSample);
    vec3 hitNormalDir = hitNormalLenSq > 1e-5
        ? hitNormalSample * inversesqrt(hitNormalLenSq)
        : -rayDir;
    float geometryValidity = 1.0 - smoothstep(0.998, 1.0, hitDepthRaw);
    geometryValidity *= smoothstep(0.01, 0.1, hitNormalLenSq);
    float normalFacing = max(dot(hitNormalDir, -rayDir), 0.0);
    geometryValidity *= smoothstep(0.05, 0.25, normalFacing);
    float hitLuma = dot(hitColor, vec3(0.2126, 0.7152, 0.0722));
    float skyLuma = dot(skyReflection, vec3(0.2126, 0.7152, 0.0722));
    float luminanceValidity = smoothstep(0.002, 0.03, hitLuma + skyLuma * 0.1);
    
    vec2 edgeFade = smoothstep(0.0, 0.15, hitUV) * (1.0 - smoothstep(0.85, 1.0, hitUV));
    float edgeFactor = edgeFade.x * edgeFade.y;
    
    float distanceFade = 1.0 - smoothstep(maxDistance * 0.5, maxDistance, hitDepth);
    
    float roughnessFade = 1.0 - smoothstep(0.0, maxRoughness, roughness);
    float hitConfidence = edgeFactor * distanceFade * roughnessFade;
    hitConfidence *= geometryValidity * luminanceValidity;
    
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 V = normalize(viewDir);
    float cosTheta = max(dot(normal, V), 0.0);
    vec3 fresnel = fresnelSchlick(cosTheta, F0);
    float fresnelFactor = (fresnel.r + fresnel.g + fresnel.b) / 3.0;
    float fresnelWeight = mix(fresnelFactor, 1.0, mirrorFactor);
    
    vec3 reflectionColor = mix(skyReflection, hitColor, hitConfidence);
    float finalFade = mix(hitConfidence * fresnelWeight, 1.0, mirrorFactor);
    finalFade = clamp(finalFade, 0.0, 1.0);
    
    return vec4(reflectionColor, finalFade);
}

void main() {
    vec3 worldPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoord).xyz);
    vec3 albedo = texture(gAlbedoSpec, TexCoord).rgb;
    vec4 material = texture(gMaterial, TexCoord);
    
    float metallic = material.r;
    float roughness = material.g;
    float reflectivity = material.a;
    
    if (length(normal) < 0.001) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    if (roughness > maxRoughness || max(metallic, reflectivity) < 0.02) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    vec3 viewDir = normalize(cameraPosition - worldPos);
    
    vec4 reflection = SSR(worldPos, normal, viewDir, roughness, metallic, reflectivity, albedo);
    if (debugMode != 0) {
        FragColor = vec4(1.0 - reflection.a, reflection.a, 0.0, 1.0);
        return;
    }
    if (reflection.a > 0.0 && historyWeight > 0.0) {
        vec4 history = texture(historyTexture, TexCoord);
        float validHistory = step(0.001, history.a);
        reflection = mix(reflection, history,
                         historyWeight * validHistory * reflection.a);
    }
    
    FragColor = reflection;
}
