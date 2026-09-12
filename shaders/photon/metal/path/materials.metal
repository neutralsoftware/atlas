constexpr sampler materialTexSampler(coord::normalized, address::repeat,
                                     filter::linear, mip_filter::linear);

#define PT_MATERIAL_TEXTURE_PARAMS                                             \
    texture2d<float> materialTexture0, texture2d<float> materialTexture1,      \
        texture2d<float> materialTexture2, texture2d<float> materialTexture3,  \
        texture2d<float> materialTexture4, texture2d<float> materialTexture5,  \
        texture2d<float> materialTexture6, texture2d<float> materialTexture7,  \
        texture2d<float> materialTexture8, texture2d<float> materialTexture9,  \
        texture2d<float> materialTexture10,                                    \
        texture2d<float> materialTexture11,                                    \
        texture2d<float> materialTexture12,                                    \
        texture2d<float> materialTexture13,                                    \
        texture2d<float> materialTexture14,                                    \
        texture2d<float> materialTexture15,                                    \
        texture2d<float> materialTexture16,                                    \
        texture2d<float> materialTexture17,                                    \
        texture2d<float> materialTexture18,                                    \
        texture2d<float> materialTexture19,                                    \
        texture2d<float> materialTexture20,                                    \
        texture2d<float> materialTexture21,                                    \
        texture2d<float> materialTexture22,                                    \
        texture2d<float> materialTexture23,                                    \
        texture2d<float> materialTexture24,                                    \
        texture2d<float> materialTexture25,                                    \
        texture2d<float> materialTexture26,                                    \
        texture2d<float> materialTexture27,                                    \
        texture2d<float> materialTexture28,                                    \
        texture2d<float> materialTexture29,                                    \
        texture2d<float> materialTexture30,                                    \
        texture2d<float> materialTexture31,                                    \
        texture2d<float> materialTexture32,                                    \
        texture2d<float> materialTexture33,                                    \
        texture2d<float> materialTexture34,                                    \
        texture2d<float> materialTexture35,                                    \
        texture2d<float> materialTexture36,                                    \
        texture2d<float> materialTexture37,                                    \
        texture2d<float> materialTexture38,                                    \
        texture2d<float> materialTexture39,                                    \
        texture2d<float> materialTexture40,                                    \
        texture2d<float> materialTexture41,                                    \
        texture2d<float> materialTexture42,                                    \
        texture2d<float> materialTexture43,                                    \
        texture2d<float> materialTexture44,                                    \
        texture2d<float> materialTexture45,                                    \
        texture2d<float> materialTexture46, texture2d<float> materialTexture47

#define PT_MATERIAL_TEXTURE_ARGS                                               \
    materialTexture0, materialTexture1, materialTexture2, materialTexture3,    \
        materialTexture4, materialTexture5, materialTexture6,                  \
        materialTexture7, materialTexture8, materialTexture9,                  \
        materialTexture10, materialTexture11, materialTexture12,               \
        materialTexture13, materialTexture14, materialTexture15,               \
        materialTexture16, materialTexture17, materialTexture18,               \
        materialTexture19, materialTexture20, materialTexture21,               \
        materialTexture22, materialTexture23, materialTexture24,               \
        materialTexture25, materialTexture26, materialTexture27,               \
        materialTexture28, materialTexture29, materialTexture30,               \
        materialTexture31, materialTexture32, materialTexture33,               \
        materialTexture34, materialTexture35, materialTexture36,               \
        materialTexture37, materialTexture38, materialTexture39,               \
        materialTexture40, materialTexture41, materialTexture42,               \
        materialTexture43, materialTexture44, materialTexture45,               \
        materialTexture46, materialTexture47

#define PT_MATERIAL_TEXTURE_BINDINGS                                           \
    texture2d<float> materialTexture0 [[texture(12)]],                         \
        texture2d<float> materialTexture1 [[texture(13)]],                     \
        texture2d<float> materialTexture2 [[texture(14)]],                     \
        texture2d<float> materialTexture3 [[texture(15)]],                     \
        texture2d<float> materialTexture4 [[texture(16)]],                     \
        texture2d<float> materialTexture5 [[texture(17)]],                     \
        texture2d<float> materialTexture6 [[texture(18)]],                     \
        texture2d<float> materialTexture7 [[texture(19)]],                     \
        texture2d<float> materialTexture8 [[texture(20)]],                     \
        texture2d<float> materialTexture9 [[texture(21)]],                     \
        texture2d<float> materialTexture10 [[texture(22)]],                    \
        texture2d<float> materialTexture11 [[texture(23)]],                    \
        texture2d<float> materialTexture12 [[texture(24)]],                    \
        texture2d<float> materialTexture13 [[texture(25)]],                    \
        texture2d<float> materialTexture14 [[texture(26)]],                    \
        texture2d<float> materialTexture15 [[texture(27)]],                    \
        texture2d<float> materialTexture16 [[texture(28)]],                    \
        texture2d<float> materialTexture17 [[texture(29)]],                    \
        texture2d<float> materialTexture18 [[texture(30)]],                    \
        texture2d<float> materialTexture19 [[texture(31)]],                    \
        texture2d<float> materialTexture20 [[texture(32)]],                    \
        texture2d<float> materialTexture21 [[texture(33)]],                    \
        texture2d<float> materialTexture22 [[texture(34)]],                    \
        texture2d<float> materialTexture23 [[texture(35)]],                    \
        texture2d<float> materialTexture24 [[texture(36)]],                    \
        texture2d<float> materialTexture25 [[texture(37)]],                    \
        texture2d<float> materialTexture26 [[texture(38)]],                    \
        texture2d<float> materialTexture27 [[texture(39)]],                    \
        texture2d<float> materialTexture28 [[texture(40)]],                    \
        texture2d<float> materialTexture29 [[texture(41)]],                    \
        texture2d<float> materialTexture30 [[texture(42)]],                    \
        texture2d<float> materialTexture31 [[texture(43)]],                    \
        texture2d<float> materialTexture32 [[texture(44)]],                    \
        texture2d<float> materialTexture33 [[texture(45)]],                    \
        texture2d<float> materialTexture34 [[texture(46)]],                    \
        texture2d<float> materialTexture35 [[texture(47)]],                    \
        texture2d<float> materialTexture36 [[texture(48)]],                    \
        texture2d<float> materialTexture37 [[texture(49)]],                    \
        texture2d<float> materialTexture38 [[texture(50)]],                    \
        texture2d<float> materialTexture39 [[texture(51)]],                    \
        texture2d<float> materialTexture40 [[texture(52)]],                    \
        texture2d<float> materialTexture41 [[texture(53)]],                    \
        texture2d<float> materialTexture42 [[texture(54)]],                    \
        texture2d<float> materialTexture43 [[texture(55)]],                    \
        texture2d<float> materialTexture44 [[texture(56)]],                    \
        texture2d<float> materialTexture45 [[texture(57)]],                    \
        texture2d<float> materialTexture46 [[texture(58)]],                    \
        texture2d<float> materialTexture47 [[texture(59)]]

float4 sampleMaterialTexture(int textureIndex, float2 uv,
                             PT_MATERIAL_TEXTURE_PARAMS) {
    switch (textureIndex) {
    case 0:
        return materialTexture0.sample(materialTexSampler, uv);
    case 1:
        return materialTexture1.sample(materialTexSampler, uv);
    case 2:
        return materialTexture2.sample(materialTexSampler, uv);
    case 3:
        return materialTexture3.sample(materialTexSampler, uv);
    case 4:
        return materialTexture4.sample(materialTexSampler, uv);
    case 5:
        return materialTexture5.sample(materialTexSampler, uv);
    case 6:
        return materialTexture6.sample(materialTexSampler, uv);
    case 7:
        return materialTexture7.sample(materialTexSampler, uv);
    case 8:
        return materialTexture8.sample(materialTexSampler, uv);
    case 9:
        return materialTexture9.sample(materialTexSampler, uv);
    case 10:
        return materialTexture10.sample(materialTexSampler, uv);
    case 11:
        return materialTexture11.sample(materialTexSampler, uv);
    case 12:
        return materialTexture12.sample(materialTexSampler, uv);
    case 13:
        return materialTexture13.sample(materialTexSampler, uv);
    case 14:
        return materialTexture14.sample(materialTexSampler, uv);
    case 15:
        return materialTexture15.sample(materialTexSampler, uv);
    case 16:
        return materialTexture16.sample(materialTexSampler, uv);
    case 17:
        return materialTexture17.sample(materialTexSampler, uv);
    case 18:
        return materialTexture18.sample(materialTexSampler, uv);
    case 19:
        return materialTexture19.sample(materialTexSampler, uv);
    case 20:
        return materialTexture20.sample(materialTexSampler, uv);
    case 21:
        return materialTexture21.sample(materialTexSampler, uv);
    case 22:
        return materialTexture22.sample(materialTexSampler, uv);
    case 23:
        return materialTexture23.sample(materialTexSampler, uv);
    case 24:
        return materialTexture24.sample(materialTexSampler, uv);
    case 25:
        return materialTexture25.sample(materialTexSampler, uv);
    case 26:
        return materialTexture26.sample(materialTexSampler, uv);
    case 27:
        return materialTexture27.sample(materialTexSampler, uv);
    case 28:
        return materialTexture28.sample(materialTexSampler, uv);
    case 29:
        return materialTexture29.sample(materialTexSampler, uv);
    case 30:
        return materialTexture30.sample(materialTexSampler, uv);
    case 31:
        return materialTexture31.sample(materialTexSampler, uv);
    case 32:
        return materialTexture32.sample(materialTexSampler, uv);
    case 33:
        return materialTexture33.sample(materialTexSampler, uv);
    case 34:
        return materialTexture34.sample(materialTexSampler, uv);
    case 35:
        return materialTexture35.sample(materialTexSampler, uv);
    case 36:
        return materialTexture36.sample(materialTexSampler, uv);
    case 37:
        return materialTexture37.sample(materialTexSampler, uv);
    case 38:
        return materialTexture38.sample(materialTexSampler, uv);
    case 39:
        return materialTexture39.sample(materialTexSampler, uv);
    case 40:
        return materialTexture40.sample(materialTexSampler, uv);
    case 41:
        return materialTexture41.sample(materialTexSampler, uv);
    case 42:
        return materialTexture42.sample(materialTexSampler, uv);
    case 43:
        return materialTexture43.sample(materialTexSampler, uv);
    case 44:
        return materialTexture44.sample(materialTexSampler, uv);
    case 45:
        return materialTexture45.sample(materialTexSampler, uv);
    case 46:
        return materialTexture46.sample(materialTexSampler, uv);
    case 47:
        return materialTexture47.sample(materialTexSampler, uv);
    default:
        break;
    }
    return float4(0.0);
}

struct MaterialTextureArguments {
    array<texture2d<float>, 256> textures [[id(0)]];
};

float4 sampleMaterialTexture(
    int textureIndex, float2 uv,
    constant MaterialTextureArguments &materialTextureArguments) {
    return materialTextureArguments.textures[textureIndex].sample(
        materialTexSampler, uv);
}

#undef PT_MATERIAL_TEXTURE_PARAMS
#undef PT_MATERIAL_TEXTURE_ARGS
#undef PT_MATERIAL_TEXTURE_BINDINGS
#define PT_MATERIAL_TEXTURE_PARAMS                                            \
    constant MaterialTextureArguments &materialTextureArguments
#define PT_MATERIAL_TEXTURE_ARGS materialTextureArguments
#define PT_MATERIAL_TEXTURE_BINDINGS                                          \
    constant MaterialTextureArguments &materialTextureArguments [[buffer(12)]]

float resolveMaterialOpacity(Material mat, float2 uv, uint textureCount,
                             PT_MATERIAL_TEXTURE_PARAMS) {
    float opacity = clamp(mat.albedo.w, 0.0, 1.0);
    if (mat.opacityTextureIndex >= 0 &&
        uint(mat.opacityTextureIndex) < textureCount) {
        float4 opacitySample = sampleMaterialTexture(
            mat.opacityTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        opacity *= mat.opacityTextureIndex == mat.albedoTextureIndex
                       ? opacitySample.w
                       : opacitySample.x;
    }
    return clamp(opacity, 0.0, 1.0);
}

void resolveMaterialParameters(Material mat, float2 uv, uint textureCount,
                               PT_MATERIAL_TEXTURE_PARAMS,
                               thread float3 &albedo, thread float &metallic,
                               thread float &roughness, thread float &ao,
                               thread float3 &emissive, thread float &outIor,
                               thread float &outTransmittance) {
    albedo = clamp(mat.albedo.xyz, float3(0.0), float3(1.0));
    metallic = mat.metallic;
    roughness = mat.roughness;
    ao = mat.ao;
    emissive = max(float3(mat.emissiveColor) *
                       max(mat.emissiveIntensity, 0.0),
                   float3(0.0));

    outIor = max(mat.ior, 1.0);
    outTransmittance = clamp(mat.transmittance, 0.0, 1.0);

    if (mat.albedoTextureIndex >= 0 &&
        uint(mat.albedoTextureIndex) < textureCount) {
        albedo *= clamp(sampleMaterialTexture(mat.albedoTextureIndex, uv,
                                              PT_MATERIAL_TEXTURE_ARGS)
                            .xyz,
                        float3(0.0), float3(1.0));
    }
    if (mat.metallicTextureIndex >= 0 &&
        uint(mat.metallicTextureIndex) < textureCount) {
        float4 metallicSample = sampleMaterialTexture(
            mat.metallicTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        float metallicValue = metallicSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            metallicValue = metallicSample.z;
        }
        metallic *= clamp(metallicValue, 0.0, 1.0);
    }
    if (mat.roughnessTextureIndex >= 0 &&
        uint(mat.roughnessTextureIndex) < textureCount) {
        float4 roughnessSample = sampleMaterialTexture(
            mat.roughnessTextureIndex, uv, PT_MATERIAL_TEXTURE_ARGS);
        float roughnessValue = roughnessSample.x;
        if (mat.roughnessTextureIndex == mat.metallicTextureIndex) {
            roughnessValue = roughnessSample.y;
        }
        roughness *= clamp(roughnessValue, 0.0, 1.0);
    }
    if (mat.aoTextureIndex >= 0 && uint(mat.aoTextureIndex) < textureCount) {
        ao *= clamp(sampleMaterialTexture(mat.aoTextureIndex, uv,
                                          PT_MATERIAL_TEXTURE_ARGS)
                        .x,
                    0.0, 1.0);
    }

    albedo = clamp(albedo, float3(0.0), float3(1.0));
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.001, 1.0);
    ao = clamp(ao, 0.0, 1.0);
}

float3 resolveShadingNormal(Material mat, float2 uv, float3 localN,
                            float3 localT, float3 localB, InstanceData inst,
                            uint textureCount, PT_MATERIAL_TEXTURE_PARAMS) {
    float3x3 normalMatrix =
        float3x3(inst.normalCol0.xyz, inst.normalCol1.xyz, inst.normalCol2.xyz);
    float3 N = normalizeOr(normalMatrix * localN, float3(0.0, 1.0, 0.0));

    float3 T = normalizeOr((inst.model * float4(localT, 0.0)).xyz,
                           float3(1.0, 0.0, 0.0));
    float3 B = normalizeOr((inst.model * float4(localB, 0.0)).xyz,
                           float3(0.0, 0.0, 1.0));
    T = normalizeOr(T - N * dot(N, T), float3(1.0, 0.0, 0.0));
    B = normalizeOr(B - N * dot(N, B), cross(N, T));
    if (dot(cross(T, B), cross(T, B)) <= 1e-10) {
        float3x3 basis = buildOrthonormalBasis(N);
        T = basis[0];
        B = basis[1];
    }

    bool useNormalMap = mat._pad1[0] != 0;
    float normalStrength = max(mat._pad0, 0.0f);
    if (useNormalMap && normalStrength > 0.0 && mat.normalTextureIndex >= 0 &&
        uint(mat.normalTextureIndex) < textureCount) {
        float3 tangentNormal = sampleMaterialTexture(mat.normalTextureIndex, uv,
                                                     PT_MATERIAL_TEXTURE_ARGS)
                                   .xyz;
        tangentNormal = tangentNormal * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        tangentNormal = normalizeOr(tangentNormal, float3(0.0, 0.0, 1.0));
        N = normalizeOr(float3x3(T, B, N) * tangentNormal, N);
    }

    return N;
}

