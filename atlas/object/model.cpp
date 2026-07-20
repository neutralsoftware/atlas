//
// model.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Model object implementation
// Copyright (c) 2025 Max Van den Eynde
//
#include <any>
#include <assimp/material.h>
#include <assimp/scene.h>
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/tracer/data.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/postprocess.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/epsilon.hpp>

namespace {
class ModelImportProgressHandler : public Assimp::ProgressHandler {
  public:
    explicit ModelImportProgressHandler(
        std::function<void(float, const std::string &)> callback)
        : callback(std::move(callback)) {}

    bool Update(float percentage = -1.0f) override {
        if (callback)
            callback(std::clamp(percentage, 0.0f, 1.0f) * 0.85f,
                     "Reading and optimizing model");
        return true;
    }

  private:
    std::function<void(float, const std::string &)> callback;
};

glm::mat4 assimpToGlmMatrix(const aiMatrix4x4 &matrix) {
    return glm::transpose(glm::make_mat4(&matrix.a1));
}

float saturate(float value) { return std::clamp(value, 0.0f, 1.0f); }

float luminance(const aiColor3D &color) {
    return (color.r * 0.2126f) + (color.g * 0.7152f) + (color.b * 0.0722f);
}

bool hasVisibleColor(const aiColor3D &color) {
    return color.r > 1e-4f || color.g > 1e-4f || color.b > 1e-4f;
}

float roughnessFromShininess(float shininess, float strength) {
    const float effectiveShininess =
        std::max(0.0f, shininess) * std::max(0.0f, strength);
    return saturate(std::sqrt(2.0f / (effectiveShininess + 2.0f)));
}

void importMaterialProperties(aiMaterial *material, CoreObject &object) {
    aiColor3D diffuseColor;

    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
        object.material.albedo.r = diffuseColor.r;
        object.material.albedo.g = diffuseColor.g;
        object.material.albedo.b = diffuseColor.b;
    } else {
        aiColor4D baseColor;

        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
            object.material.albedo = {
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a,
            };
        }
    }
    float opacity = 1.0f;
    if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        object.material.albedo.a = saturate(opacity);
    } else {
        float transparency = 0.0f;
        if (material->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency) ==
            AI_SUCCESS) {
            object.material.albedo.a = saturate(1.0f - transparency);
        }
    }

    float metallic = 0.0f;
    if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        object.material.metallic = saturate(metallic);
    }

    float roughness = 0.0f;
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        object.material.roughness = saturate(roughness);
    } else {
        float glossiness = 0.0f;
        if (material->Get(AI_MATKEY_GLOSSINESS_FACTOR, glossiness) ==
            AI_SUCCESS) {
            object.material.roughness = saturate(1.0f - glossiness);
        } else {
            float shininess = 0.0f;
            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                float shininessStrength = 1.0f;
                material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
                object.material.roughness =
                    roughnessFromShininess(shininess, shininessStrength);
            }
        }
    }

    aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
    const bool hasEmissiveColor =
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS;
    if (hasEmissiveColor) {
        object.material.emissiveColor = {emissiveColor.r, emissiveColor.g,
                                         emissiveColor.b, 1.0f};
    }

    float emissiveIntensity = 0.0f;
    if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) ==
        AI_SUCCESS) {
        object.material.emissiveIntensity = std::max(0.0f, emissiveIntensity);
    } else if (hasEmissiveColor && hasVisibleColor(emissiveColor)) {
        object.material.emissiveIntensity = 1.0f;
    }

    float bumpScaling = 1.0f;
    if (material->Get(AI_MATKEY_BUMPSCALING, bumpScaling) == AI_SUCCESS) {
        object.material.normalMapStrength = std::max(0.0f, bumpScaling);
    }

    float reflectivity = 0.0f;
    if (material->Get(AI_MATKEY_REFLECTIVITY, reflectivity) == AI_SUCCESS) {
        object.material.reflectivity = saturate(reflectivity);
    } else {
        aiColor3D specularColor(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) ==
            AI_SUCCESS) {
            object.material.reflectivity = saturate(luminance(specularColor));
        }
    }
}
} // namespace

void Model::fromResource(const Resource &resource) { loadModel(resource); }

void Model::fromResource(
    const Resource &resource,
    const std::function<void(float, const std::string &)> &progress) {
    loadModel(resource, progress);
}

void Model::loadModel(
    const Resource &resource,
    const std::function<void(float, const std::string &)> &progress) {
    Assimp::Importer importer;
    if (resource.type != ResourceType::Model) {
        atlas_warning("Resource is not a model: " + resource.name);
        return;
    }

    atlas_log("Loading model: " + resource.name);

    if (progress) {
        progress(0.0f, "Opening model");
        importer.SetProgressHandler(new ModelImportProgressHandler(progress));
    }

    unsigned int importFlags =
        aiProcess_Triangulate | aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType | aiProcess_GenSmoothNormals;

    const aiScene *scene =
        importer.ReadFile(resource.path.string(), importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        atlas_error("Assimp error: " + std::string(importer.GetErrorString()));
        throw std::runtime_error("Assimp error: " +
                                 std::string(importer.GetErrorString()));
        return;
    }
    directory = resource.path.parent_path().string();
    importProgress = progress;
    importedMeshCount = 0;
    totalMeshCount = scene->mNumMeshes;
    if (progress)
        progress(0.88f, "Loading meshes and materials");
    // Texture cache to avoid loading the same texture multiple times
    std::unordered_map<std::string, Texture> textureCache;

    processNode(scene->mRootNode, scene, glm::mat4(1.0f), textureCache);

    if (progress)
        progress(1.0f, "Finishing import");
    importProgress = {};

    atlas_log("Model loaded successfully: " + resource.name + " (" +
              std::to_string(objects.size()) + " objects, " +
              std::to_string(scene->mNumMeshes) + " meshes)");

    // std::cout << "Created model from resource: " << resource.name << " with "
    //           << objects.size() << " objects." << std::endl;
    // std::cout << "Model path: " << resource.path << std::endl;
    // std::cout << "Model directory: " << directory << std::endl;
    // std::cout << "First model object has "
    //           << (objects.size() > 0 ? objects[0]->getVertices().size() : 0)
    //           << " vertices." << std::endl;

    // std::cout << "Model loaded successfully." << std::endl;
    // std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    // std::cout << "Unique textures loaded: " << textureCache.size() <<
    // std::endl;

    // size_t totalVertices = 0;
    // size_t totalTriangles = 0;
    // for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //     aiMesh *mesh = scene->mMeshes[i];
    //     totalVertices += mesh->mNumVertices;
    //     totalTriangles +=
    //         mesh->mNumFaces; // each face is a triangle (after Triangulate)
    // }

    // std::cout << "Total Vertices: " << totalVertices << std::endl;
    // std::cout << "Total Triangles: " << totalTriangles << std::endl;
}

void Model::processNode(
    aiNode *node, const aiScene *scene, glm::mat4 parentTransform,
    std::unordered_map<std::string, Texture> &textureCache) {
    glm::mat4 nodeTransform =
        parentTransform * assimpToGlmMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto obj = std::make_shared<CoreObject>(
            processMesh(mesh, scene, nodeTransform, textureCache));
        this->objects.push_back(obj);
        importedMeshCount++;
        if (importProgress && totalMeshCount > 0) {
            const float completed = static_cast<float>(importedMeshCount) /
                                    static_cast<float>(totalMeshCount);
            importProgress(0.88f + completed * 0.11f,
                           "Loading meshes and materials");
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, nodeTransform, textureCache);
    }
}

CoreObject
Model::processMesh(aiMesh *mesh, const aiScene *scene,
                   const glm::mat4 &transform,
                   std::unordered_map<std::string, Texture> &textureCache) {
    CoreObject object;
    std::vector<CoreVertex> vertices;
    std::vector<unsigned int> indices;
    const glm::mat3 linearTransform = glm::mat3(transform);
    const glm::mat3 normalTransform =
        glm::transpose(glm::inverse(linearTransform));

    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        glm::vec4 pos =
            transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                  mesh->mVertices[i].z, 1.0f);

        CoreVertex vertex;
        vertex.position = Position3d::fromGlm(glm::vec3(pos));

        // ---------- Normals ----------
        if (mesh->mNormals) {
            glm::vec3 normal = normalTransform * glm::vec3(mesh->mNormals[i].x,
                                                           mesh->mNormals[i].y,
                                                           mesh->mNormals[i].z);
            if (glm::length(normal) < 1e-6f) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }
            vertex.normal = Normal3d::fromGlm(normal);
        } else {
            vertex.normal = Normal3d::fromGlm(glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // ---------- Tangents ----------
        glm::vec3 normal = vertex.normal.toGlm();
        glm::vec3 tangent(1.0f, 0.0f, 0.0f);
        if (mesh->mTangents) {
            tangent = linearTransform * glm::vec3(mesh->mTangents[i].x,
                                                  mesh->mTangents[i].y,
                                                  mesh->mTangents[i].z);
        }
        tangent = tangent - normal * glm::dot(normal, tangent);
        if (glm::length(tangent) < 1e-6f) {
            glm::vec3 up = std::abs(normal.y) < 0.999f
                               ? glm::vec3(0.0f, 1.0f, 0.0f)
                               : glm::vec3(1.0f, 0.0f, 0.0f);
            tangent = glm::cross(up, normal);
        }
        tangent = glm::normalize(tangent);

        glm::vec3 bitangent(0.0f, 0.0f, 1.0f);
        if (mesh->mBitangents) {
            bitangent = linearTransform * glm::vec3(mesh->mBitangents[i].x,
                                                    mesh->mBitangents[i].y,
                                                    mesh->mBitangents[i].z);
        } else {
            bitangent = glm::cross(normal, tangent);
        }
        bitangent = bitangent - normal * glm::dot(normal, bitangent);
        if (glm::length(bitangent) < 1e-6f ||
            glm::length(glm::cross(tangent, bitangent)) < 1e-6f) {
            bitangent = glm::cross(normal, tangent);
        }
        bitangent = glm::normalize(bitangent);
        if (glm::dot(glm::cross(tangent, bitangent), normal) < 0.0f) {
            bitangent = -bitangent;
        }

        vertex.tangent = Normal3d::fromGlm(tangent);
        vertex.bitangent = Normal3d::fromGlm(bitangent);

        // ---------- Texture Coordinates ----------
        if (mesh->mTextureCoords[0]) {
            glm::vec2 uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                                     mesh->mTextureCoords[0][i].y);
            if (glm::all(glm::isnan(uv)) || glm::any(glm::isinf(uv)))
                uv = glm::vec2(0.0f, 0.0f);
            vertex.textureCoordinate = TextureCoordinate{uv.x, uv.y};
        } else {
            vertex.textureCoordinate = TextureCoordinate{0.0f, 0.0f};
        }

        // ---------- Vertex Color ----------
        if (mesh->mColors[0]) {
            glm::vec4 col =
                glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                          mesh->mColors[0][i].b, mesh->mColors[0][i].a);
            if (glm::all(glm::isnan(col)) || glm::any(glm::isinf(col)))
                col = glm::vec4(1.0f);
            vertex.color = Color{col.r, col.g, col.b, col.a};
        } else {
            vertex.color = Color{1.0f, 1.0f, 1.0f, 1.0f};
        }

        vertices.push_back(vertex);
    }

    // ---------- Indices ----------
    // Pre-calculate total indices for reservation
    size_t totalIndices = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        totalIndices += mesh->mFaces[i].mNumIndices;
    }
    indices.reserve(totalIndices);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace &face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    std::vector<Texture> textures;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        importMaterialProperties(material, object);

        auto diffuseMaps =
            loadMaterialTextures(material, std::any(aiTextureType_DIFFUSE),
                                 "texture_diffuse", textureCache);
        if (diffuseMaps.empty()) {
            auto baseColorMaps = loadMaterialTextures(
                material, std::any(aiTextureType_BASE_COLOR), "texture_diffuse",
                textureCache);
            diffuseMaps.insert(diffuseMaps.end(), baseColorMaps.begin(),
                               baseColorMaps.end());
        }
        if (diffuseMaps.empty()) {
            auto ambientMaps =
                loadMaterialTextures(material, std::any(aiTextureType_AMBIENT),
                                     "texture_diffuse", textureCache);
            diffuseMaps.insert(diffuseMaps.end(), ambientMaps.begin(),
                               ambientMaps.end());
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps =
            loadMaterialTextures(material, std::any(aiTextureType_SPECULAR),
                                 "texture_specular", textureCache);
        textures.insert(textures.end(), specularMaps.begin(),
                        specularMaps.end());

        auto normalMaps =
            loadMaterialTextures(material, std::any(aiTextureType_NORMALS),
                                 "texture_normal", textureCache);
        if (normalMaps.empty()) {
            auto heightAsNormalMaps =
                loadMaterialTextures(material, std::any(aiTextureType_HEIGHT),
                                     "texture_normal", textureCache);
            normalMaps.insert(normalMaps.end(), heightAsNormalMaps.begin(),
                              heightAsNormalMaps.end());
        }
        if (normalMaps.empty()) {
            auto displacementAsNormalMaps = loadMaterialTextures(
                material, std::any(aiTextureType_DISPLACEMENT),
                "texture_normal", textureCache);
            normalMaps.insert(normalMaps.end(),
                              displacementAsNormalMaps.begin(),
                              displacementAsNormalMaps.end());
        }
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        auto metallicMaps =
            loadMaterialTextures(material, std::any(aiTextureType_METALNESS),
                                 "texture_metallic", textureCache);
        textures.insert(textures.end(), metallicMaps.begin(),
                        metallicMaps.end());

        auto roughnessMaps = loadMaterialTextures(
            material, std::any(aiTextureType_DIFFUSE_ROUGHNESS),
            "texture_roughness", textureCache);
        textures.insert(textures.end(), roughnessMaps.begin(),
                        roughnessMaps.end());

        auto aoMaps = loadMaterialTextures(
            material, std::any(aiTextureType_AMBIENT_OCCLUSION), "texture_ao",
            textureCache);
        if (aoMaps.empty()) {
            auto lightmapMaps =
                loadMaterialTextures(material, std::any(aiTextureType_LIGHTMAP),
                                     "texture_ao", textureCache);
            aoMaps.insert(aoMaps.end(), lightmapMaps.begin(),
                          lightmapMaps.end());
        }
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

        auto opacityMaps =
            loadMaterialTextures(material, std::any(aiTextureType_OPACITY),
                                 "texture_opacity", textureCache);
        textures.insert(textures.end(), opacityMaps.begin(), opacityMaps.end());
    }

    if (!textures.empty()) {
        for (const auto &texture : textures) {
            object.attachTexture(texture);
        }
    }

    object.attachVertices(vertices);
    object.attachIndices(indices);

    ResourceEventInfo info;
    info.resourceType = DebugResourceType::Mesh;
    info.callerObject = std::to_string(object.getId());
    info.operation = DebugResourceOperation::Created;
    info.frameNumber =
        Window::mainWindow != nullptr && Window::mainWindow->device != nullptr
            ? Window::mainWindow->device->frameCount
            : 0;
    info.sizeMb = static_cast<float>((vertices.size() * sizeof(CoreVertex)) +
                                     (indices.size() * sizeof(unsigned int))) /
                  (1024.0f * 1024.0f);

    info.send();
    return object;
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat, std::any type, const std::string &typeName,
    std::unordered_map<std::string, Texture> &textureCache) {
    aiMaterial *material = mat;
    aiTextureType textureType = std::any_cast<aiTextureType>(type);
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++) {
        aiString str;
        material->GetTexture(textureType, i, &str);
        std::string filename = std::string(str.C_Str());
        std::string fullPath = directory + "/" + filename;
        std::string cacheKey = fullPath + "|" + typeName;

        // Check if texture is already cached
        auto cacheIt = textureCache.find(cacheKey);
        if (cacheIt != textureCache.end()) {
            textures.push_back(cacheIt->second);
            continue;
        }

        // std::cout << "Loading texture: " << str.C_Str() << " of type "
        //           << typeName << std::endl;

        ResourceType resType = ResourceType::Image;
        if (typeName == "texture_specular") {
            resType = ResourceType::SpecularMap;
        }
        Resource resource =
            Workspace::get().createResource(fullPath, filename, resType);
        TextureType texType = TextureType::Color;
        if (typeName == "texture_specular") {
            texType = TextureType::Specular;
        } else if (typeName == "texture_normal") {
            texType = TextureType::Normal;
        } else if (typeName == "texture_height") {
            texType = TextureType::Parallax;
        } else if (typeName == "texture_metallic") {
            texType = TextureType::Metallic;
        } else if (typeName == "texture_roughness") {
            texType = TextureType::Roughness;
        } else if (typeName == "texture_ao") {
            texType = TextureType::AO;
        } else if (typeName == "texture_opacity") {
            texType = TextureType::Opacity;
        }

        try {
            Texture loadedTexture = Texture::fromResource(resource, texType);
            textureCache[cacheKey] = loadedTexture;
            textures.push_back(loadedTexture);
        } catch (const std::exception &ex) {
            atlas_warning("Failed to load texture '" + filename +
                          "': " + ex.what());
            std::cerr << "Failed to load texture '" << filename
                      << "': " << ex.what() << std::endl;
        }
    }
    return textures;
}
