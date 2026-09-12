/*
 object.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Object properties and definitions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_OBJECT_H
#define ATLAS_OBJECT_H

#include "atlas/component.h"
#include "atlas/physics.h"
#include "bezel/bezel.h"
#include "photon/illuminate.h"
#include <algorithm>
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <unordered_map>
#pragma once

#include "atlas/core/shader.h"
#include "atlas/core/renderable.h"
#include "atlas/texture.h"
#include <array>
#include <atlas/units.h>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

class Scene;
struct Light;

/**
 * @brief Alias that represents a texture coordinate in 2D space.
 *
 */
typedef std::array<float, 2> TextureCoordinate;

/**
 * @brief Structure representing the material properties of an object. Based on
 * Phong reflection model.
 *
 */
struct Material {
    /**
     * @brief Base color contribution of the surface.
     */
    Color albedo = {1.0, 1.0, 1.0, 1.0};
    /**
     * @brief Metallic factor controlling how conductive the material behaves.
     */
    float metallic = 0.0f;
    /**
     * @brief Roughness factor influencing the spread of specular highlights.
     */
    float roughness = 0.5f;
    /**
     * @brief Ambient occlusion term used to darken creases and cavities.
     */
    float ao = 1.0f;
    /**
     * @brief Additional scalar controlling mirror-like reflections.
     */
    float reflectivity = 0.0f;
    /**
     * @brief Color emitted by the surface when emissive lighting is enabled.
     */
    Color emissiveColor = {0.0, 0.0, 0.0, 1.0};
    /**
     * @brief Intensity multiplier for emissiveColor.
     */
    float emissiveIntensity = 0.0f;
    /**
     * @brief Strength applied to sampled normal maps.
     */
    float normalMapStrength = 1.0f;
    /**
     * @brief Whether normal map sampling is enabled for this material.
     */
    bool useNormalMap = true;
    TextureCoordinate textureScale = {1.0f, 1.0f};
    TextureCoordinate textureOffset = {0.0f, 0.0f};
    /**
     * @brief Fraction of transmitted light for translucent materials.
     */
    float transmittance = 0.0f;
    /**
     * @brief Index of refraction used for transmission effects.
     */
    float ior = 1.0f;
};

/**
 * @brief Structure representing a single vertex in 3D space, including
 * position, color, texture coordinates, and normal vector. \warning This is
 * meant for internal use only and should not be used directly because it's
 * difficult to read and maintain.
 * \subsection core-vertex-example Example
 * ```cpp
 * // Create a vertex at position (1, 2, 3) with red color
 * CoreVertex vertex({1.0, 2.0, 3.0}, {1.0, 0.0, 0.0, 1.0}, {0.5, 0.5}, {0.0,
 * 0.0, 1.0});
 * ```
 */
struct CoreVertex {
    /**
     * @brief The position of the vertex in 3D space.
     *
     */
    Position3d position;
    /**
     * @brief The color of the vertex.
     *
     */
    Color color = {1.0, 1.0, 1.0, 1.0};
    /**
     * @brief The texture coordinates of the vertex.
     *
     */
    TextureCoordinate textureCoordinate = {0.0, 0.0};
    /**
     * @brief The normal vector of the vertex, used for lighting calculations.
     *
     */
    Normal3d normal = {0.0, 0.0, 0.0};

    /**
     * @brief The tangent vector of the vertex, used for normal mapping and
     * parallax calculations.
     *
     */
    Normal3d tangent = {0.0, 0.0, 0.0};
    /**
     * @brief The bitangent vector of the vertex, used for normal mapping and
     * parallax calculations.
     *
     */
    Normal3d bitangent = {0.0, 0.0, 0.0};

    /**
     * @brief Function that constructs a new CoreVertex object.
     *
     * @param pos The position of the vertex in 3D space.
     * @param col The color of the vertex.
     * @param tex The texture coordinates of the vertex.
     * @param n The normal vector of the vertex.
     */
    CoreVertex(Position3d pos = {0.0, 0.0, 0.0},
               Color col = {1.0, 1.0, 1.0, 1.0},
               TextureCoordinate tex = {0.0, 0.0}, Normal3d n = {0.0, 0.0, 0.0},
               Normal3d t = {0.0, 0.0, 0.0}, Normal3d b = {0.0, 0.0, 0.0})
        : position(pos), color(col), textureCoordinate(tex), normal(n),
          tangent(t), bitangent(b) {}

    /**
     * @brief Gets the layout descriptors for the CoreVertex structure.
     *
     * @return (std::vector<LayoutDescriptor>) The layout descriptors.
     */
    static std::vector<LayoutDescriptor> getLayoutDescriptors();
};

/**
 * @brief Represents a buffer index.
 *
 */
typedef unsigned int BufferIndex;
/**
 * @brief Represents an index in a buffer.
 *
 */
typedef unsigned int Index;

class Window;

/**
 * @brief Structure representing a single instance of an object for instanced
 * rendering. Each instance has its own position, rotation, and scale.
 *
 */
struct Instance {
    /**
     * @brief The position of this instance in 3D space.
     *
     */
    Position3d position = {0.0, 0.0, 0.0};
    /**
     * @brief The rotation of this instance in 3D space.
     *
     */
    Rotation3d rotation = {0.0, 0.0, 0.0};
    /**
     * @brief The scale of this instance in 3D space.
     *
     */
    Scale3d scale = {1.0, 1.0, 1.0};

  private:
    glm::mat4 model = glm::mat4(1.0f);

  public:
    /**
     * @brief Updates the model matrix based on the instance's position,
     * rotation, and scale.
     *
     */
    void updateModelMatrix();
    /**
     * @brief Gets the current model matrix for this instance.
     *
     * @return (glm::mat4) The model matrix.
     */
    glm::mat4 getModelMatrix() const { return model; }
    /**
     * @brief Moves the instance by a delta position.
     *
     * @param deltaPosition The amount to move by.
     */
    void move(const Position3d &deltaPosition);
    /**
     * @brief Sets the position of the instance.
     *
     * @param newPosition The new position to set.
     */
    void setPosition(const Position3d &newPosition);
    /**
     * @brief Sets the rotation of the instance.
     *
     * @param newRotation The new rotation to set.
     */
    void setRotation(const Rotation3d &newRotation);
    /**
     * @brief Rotates the instance by a delta rotation.
     *
     * @param deltaRotation The amount to rotate by.
     */
    void rotate(const Rotation3d &deltaRotation);
    /**
     * @brief Sets the scale of the instance.
     *
     * @param newScale The new scale to set.
     */
    void setScale(const Scale3d &newScale);
    /**
     * @brief Scales the instance by a delta scale factor.
     *
     * @param deltaScale The scale factor to multiply by.
     */
    void scaleBy(const Scale3d &deltaScale);

    /**
     * @brief Compares two instances for equality.
     *
     * @param other The other instance to compare with.
     * @return (bool) True if instances are equal, false otherwise.
     */
    bool operator==(const Instance &other) const {
        return position == other.position && rotation == other.rotation &&
               scale == other.scale;
    }
};

/**
 * @brief Represents a 3D object in the scene, including its geometry, material
 * and every interaction with the scene that it can have. It inherits from \ref
 * GameObject and can have \ref Component classes attached to it for extended
 * functionality.
 *
 * \subsection core-object-example Example of a CoreObject
 * ```cpp
 * // (Optional) Register resources once, typically at startup
 * Workspace::get().setRootPath("assets/");
 * Workspace::get().createResource("textures/brick.png", "Brick",
 *                                ResourceType::Image);
 *
 * // Create a simple cube object
 * CoreObject cube = createBox({1.0, 1.0, 1.0}, Color::red());
 * cube.setPosition({0.0, 0.5, 0.0});
 *
 * // Attach a texture by resource name
 * cube.attachTexture(Texture::fromResourceName("Brick", TextureType::Color));
 *
 * // Attach default shaders (or supply custom GLSL via fromSource)
 * VertexShader vs = VertexShader::fromDefaultShader(AtlasVertexShader::Main);
 * FragmentShader fs =
 *     FragmentShader::fromDefaultShader(AtlasFragmentShader::Main);
 * vs.compile();
 * fs.compile();
 * cube.createAndAttachProgram(vs, fs);
 *
 * // PBR-ish material values
 * cube.material.albedo = Color::red();
 * cube.material.metallic = 0.0f;
 * cube.material.roughness = 0.6f;
 * cube.material.ao = 1.0f;
 *
 * scene.addObject(&cube);
 * ```
 *
 */
class CoreObject : public GameObject {
  public:
    /**
     * @brief The vertices of the object.
     *
     */
    std::vector<CoreVertex> vertices;
    /**
     * @brief The indices of the object. These indicate how the vertices are
     * connected to form faces.
     *
     */
    std::vector<Index> indices;
    /**
     * @brief Shader program currently associated with this object.
     */
    ShaderProgram shaderProgram;
    /**
     * @brief The textures applied to the object.
     *
     */
    std::vector<Texture> textures;
    /**
     * @brief The material properties of the object.
     *
     */
    Material material = Material();

    /**
     * @brief Vector of instances for instanced rendering. Multiple copies of
     * the object can be rendered with different transforms efficiently.
     *
     */
    std::vector<Instance> instances;

    /**
     * @brief Function to construct a new CoreObject.
     *
     */
    CoreObject();

    /**
     * @brief Cached graphics pipeline built from shader and state settings.
     */
    std::shared_ptr<opal::Pipeline> pipeline;

    /**
     * @brief Makes the object emissive by attaching a light to it.
     *
     * @param scene The scene to add the light to.
     * @param emissionColor The color of the emitted light.
     * @param intensity The intensity of the emitted light.
     */
    void makeEmissive(Scene *scene, Color emissionColor, float intensity);

    /**
     * @brief Function to attach vertices and update the object's vertex buffer.
     *
     * @param newVertices The vertices to attach.
     */
    void attachVertices(const std::vector<CoreVertex> &newVertices);
    /**
     * @brief Function to attach indices and update the object's index buffer.
     *
     * @param newIndices The indices to attach.
     */
    void attachIndices(const std::vector<Index> &newIndices);
    /**
     * @brief Binds an already compiled shader program for this object.
     */
    void attachProgram(const ShaderProgram &program) override;
    /**
     * @brief Builds a shader program from the supplied vertex and fragment
     * sources, then binds it.
     */
    void createAndAttachProgram(VertexShader &vertexShader,
                                FragmentShader &fragmentShader) override;
    /**
     * @brief Adds a texture to the object and makes it available during
     * rendering.
     */
    void attachTexture(const Texture &texture) override;
    /**
     * @brief Performs deferred setup of buffers, shaders, and state.
     */
    void initialize() override;

    /**
     * @brief Marks the pipeline dirty and rebuilds it against current window
     * state.
     */
    void refreshPipeline();

    std::optional<std::shared_ptr<opal::Pipeline>> getPipeline() override;
    void setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) override;

    /**
     * @brief Function to render the object with color and texture.
     *
     */
    void renderColorWithTexture();
    /**
     * @brief Function to render the object with only its color.
     *
     */
    void renderOnlyColor();
    /**
     * @brief Function to render the object with only its texture.
     *
     */
    void renderOnlyTexture();
    /**
     * @brief Sets a solid color override that multiplies with textures.
     */
    void setColor(const Color &color) override;

    /**
     * @brief The position of the object in 3D space.
     *
     */
    Position3d position = {0.0, 0.0, 0.0};
    /**
     * @brief The rotation of the object in 3D space.
     *
     */
    Rotation3d rotation = {0.0, 0.0, 0.0};
    /**
     * @brief The scale of the object in 3D space.
     *
     */
    Scale3d scale = {1.0, 1.0, 1.0};

    /**
     * @brief Places the object at an absolute position in world space.
     */
    void setPosition(const Position3d &newPosition) override;
    /**
     * @brief Moves the object relative to its current position.
     */
    void move(const Position3d &deltaPosition) override;
    /**
     * @brief Assigns an absolute rotation to the object.
     */
    void setRotation(const Rotation3d &newRotation) override;
    /**
     * @brief Sets orientation directly from a quaternion.
     */
    void setRotationQuat(const glm::quat &quat);
    /**
     * @brief Rotates the object so its forward vector points towards a
     * target.
     */
    void lookAt(const Position3d &target,
                const Normal3d &up = {0.0, 1.0, 0.0}) override;
    /**
     * @brief Applies an incremental rotation around the object's axes.
     */
    void rotate(const Rotation3d &deltaRotation) override;
    /**
     * @brief Uniformly scales the object along each axis.
     */
    void setScale(const Scale3d &newScale) override;

    /**
     * @brief Function that updates the model matrix based on the object's
     * position, rotation, and scale.
     *
     */
    void updateModelMatrix();
    /**
     * @brief Function that updates the object's vertex buffer.
     *
     */
    void updateVertices();

    /**
     * @brief Function that creates a copy of the object.
     *
     * @return (CoreObject) The cloned object.
     */
    CoreObject clone() const;

    void show() override { isVisible = true; }
    void hide() override { isVisible = false; }

    /**
     * @brief The light attached to this object if it's emissive. Used for
     * emissive objects created with makeEmissive().
     *
     */
    std::shared_ptr<Light> light = nullptr;

    /**
     * @brief Variable that determines if the object casts shadows.
     *
     */
    bool castsShadows = true;

    /**
     * @brief Function that adds a component to the object. The component will
     * have this CoreObject as its parent.
     *
     * @tparam T The type of component to add.
     * @param existing The existing component to add.
     */
    template <typename T>
        requires std::is_base_of_v<Component, std::remove_cvref_t<T>>
    void addComponent(T &&existing) {
        using U = std::remove_cvref_t<T>;
        std::shared_ptr<U> component =
            std::make_shared<U>(std::forward<T>(existing));
        component->object = this;
        component->atAttach();
        components.push_back(component);
    }

    template <typename T>
        requires std::is_base_of_v<Component, T>
    void addComponent(const std::shared_ptr<T> &component) {
        if (!component) {
            return;
        }
        component->object = this;
        component->atAttach();
        components.push_back(component);
    }

    /**
     * @brief Whether the object should use deferred rendering. When false, the
     * object is rendered in the forward rendering pass.
     *
     */
    bool useDeferredRendering = true;

    /**
     * @brief Switches the object to forward rendering by binding default
     * shaders.
     */
    void disableDeferredRendering() {
        useDeferredRendering = false;
        this->shaderProgram = ShaderProgram::fromDefaultShaders(
            AtlasVertexShader::Deferred, AtlasFragmentShader::Deferred);
    }

    /**
     * @brief Creates and returns a new instance for instanced rendering.
     *
     * @return (Instance&) Reference to the newly created instance.
     */
    Instance &createInstance() {
        instances.emplace_back();
        Instance &inst = instances.back();

        inst.position = this->position;
        inst.rotation = this->rotation;
        inst.scale = this->scale;
        inst.updateModelMatrix();

        return inst;
    }

  private:
    std::shared_ptr<opal::DrawingState> vao;
    std::shared_ptr<opal::Buffer> vbo;
    std::shared_ptr<opal::Buffer> ebo;
    std::shared_ptr<opal::Buffer> instanceVBO;

    std::vector<Instance> savedInstances;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool useColor = true;
    bool useTexture = false;

    bool isVisible = true;

    bool hasPhysics = false;

    friend class Window;
    friend class RenderTarget;
    friend class Skybox;
    friend class photon::PathTracing;
    friend class photon::GlobalIllumination;

    void updateInstances();

  public:
    /**
     * @brief Draws the object for the current frame.
     *
     * @param dt Time delta provided by the window loop.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Uploads the active view matrix used for transforming vertices to
     * camera space.
     */
    void setViewMatrix(const glm::mat4 &view) override;
    /**
     * @brief Uploads the projection matrix that maps camera space to clip
     * space.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /**
     * @brief Gets the world-space position of the object.
     */
    Position3d getPosition() const override { return position; }

    /**
     * @brief Returns a copy of the object's vertex collection.
     */
    std::vector<CoreVertex> getVertices() const override { return vertices; }

    /**
     * @brief Gets the current non-uniform scale applied to the object.
     */
    Size3d getScale() const override { return scale; }
    /**
     * @brief Reports whether the object is configured to cast shadows.
     */
    bool canCastShadows() const override { return castsShadows; }

    /**
     * @brief Returns the current Euler rotation (pitch, yaw, roll).
     */
    Rotation3d getRotation() const override { return rotation; }

    /**
     * @brief Performs per-frame updates such as component ticking or buffering
     * synchronization.
     */
    void update(Window &window) override;

    /**
     * @brief Indicates whether the object is eligible for the deferred
     * rendering pipeline.
     */
    bool canUseDeferredRendering() override { return useDeferredRendering; }

    void onCollisionEnter([[maybe_unused]] GameObject *other) override {
        for (auto &component : components) {
            component->onCollisionEnter(other);
        }
    }

    void onCollisionExit([[maybe_unused]] GameObject *other) override {
        for (auto &component : components) {
            component->onCollisionExit(other);
        }
    }

    void onCollisionStay([[maybe_unused]] GameObject *other) override {
        for (auto &component : components) {
            component->onCollisionStay(other);
        }
    }

    void onSignalRecieve([[maybe_unused]] const std::string &signal,
                         [[maybe_unused]] GameObject *sender) override {
        for (auto &component : components) {
            component->onSignalRecieve(signal, sender);
        }
    }

    void onSignalEnd([[maybe_unused]] const std::string &signal,
                     [[maybe_unused]] GameObject *sender) override {
        for (auto &component : components) {
            component->onSignalEnd(signal, sender);
        }
    }

    void beforePhysics() override {
        for (auto &component : components) {
            component->beforePhysics();
        }
    }

    void onQueryReceive(QueryResult &result) override {
        for (auto &component : components) {
            component->onQueryReceive(result);
        }
    }
};

/**
 * @brief Function that creates a box CoreObject with the specified size and
 * color.
 *
 * @param size The size of the box in 3D space.
 * @param color The color of the box.
 * @return (CoreObject) The created box object.
 */
CoreObject createBox(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a box CoreObject with a default texture and
 * physics body.
 *
 * @param size The size of the box in 3D space.
 * @return (CoreObject) The created debug box object.
 */
CoreObject createDebugBox(Size3d size);
/**
 * @brief Function that creates a plane CoreObject with the specified size and
 * color.
 *
 * @param size The size of the plane in 2D space.
 * @param color The color of the plane.
 * @return (CoreObject) The created plane object.
 */
CoreObject createPlane(Size2d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a debug plane CoreObject with the specified size
 * and attaches a default texture.
 *
 * @param size The size of the plane in 2D space.
 * @return (CoreObject) The created debug plane object.
 */
CoreObject createDebugPlane(Size2d size);
/**
 * @brief Function that creates a pyramid CoreObject with the specified size and
 * color.
 *
 * @param size The size of the pyramid in 3D space.
 * @param color The color of the pyramid.
 * @return (CoreObject) The created pyramid object.
 */
CoreObject createPyramid(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a sphere CoreObject with the specified radius,
 * sectorCount, stackCount and color.
 *
 * @param radius The radius of the sphere.
 * @param sectorCount The number of sectors (longitude divisions) of the sphere.
 * @param stackCount The number of stacks (latitude divisions) of the sphere.
 * @param color The color of the sphere.
 * @return (CoreObject) The created sphere object.
 */
CoreObject createSphere(double radius, unsigned int sectorCount = 36,
                        unsigned int stackCount = 18,
                        Color color = {1.0, 1.0, 1.0, 1.0});

/**
 * @brief Function that creates a debug sphere CoreObject with the specified
 * radius, sectorCount and stackCount, and attaches a default texture and
 * physics body.
 *
 * @param radius The radius of the sphere.
 * @param sectorCount The number of sectors (longitude divisions) of the
 * @param stackCount The number of stacks (latitude divisions) of the sphere.
 * @return (CoreObject) The created sphere object.
 */
CoreObject createDebugSphere(double radius, unsigned int sectorCount = 36,
                             unsigned int stackCount = 18);

struct Resource;
class aiNode;
class aiScene;
class aiMesh;
class aiMaterial;
/**
 * @brief Class that represents a 3D model composed of multiple CoreObjects. It
 * can be loaded from a resource file and manages its constituent objects.
 * \subsection model-example Example
    * ```cpp
    * // Load a 3D model from a resource file
    * Resource modelResource =
 Workspace::get().createResource("path/to/model.obj");
    * Model myModel;
    * myModel.fromResource(modelResource);
    * // Set the position of the model in the scene
    * myModel.setPosition({0.0f, 0.0f, 0.0f});
    * // Scale the model to twice its original size
    * myModel.setScale({2.0f, 2.0f, 2.0f});
    * // Rotate the model 45 degrees around the Y-axis
    * myModel.setRotation({0.0f, 45.0f, 0.0f});
    * // Add the model to the scene
    * window.addObject(&myModel);
    * ```
 *
 */
class Model : public GameObject {
  public:
    /**
     * @brief Loads the model from a resource.
     *
     * @param resource The resource to load the model from.
     */
    void fromResource(const Resource &resource);
    void fromResource(
        const Resource &resource,
        const std::function<void(float, const std::string &)> &progress);

    /**
     * @brief Gets the objects that make up the model.
     *
     * @return (std::vector<std::shared_ptr<CoreObject>>) The objects.
     */
    std::vector<std::shared_ptr<CoreObject>> &getObjects() { return objects; }
    const std::vector<std::shared_ptr<CoreObject>> &getObjects() const {
        return objects;
    }

    /**
     * @brief Moves the model by a certain amount.
     *
     * @param deltaPosition The amount to move the model by.
     */
    void move(const Position3d &deltaPosition) override {
        for (auto &obj : objects) {
            obj->move(deltaPosition);
        }
    }

    /**
     * @brief Set the Position object
     *
     * @param newPosition The new position to set.
     */
    void setPosition(const Position3d &newPosition) override {
        for (auto &obj : objects) {
            obj->setPosition(newPosition);
        }
    }

    /**
     * @brief Assigns an absolute rotation to every mesh in the model.
     */
    void setRotation(const Rotation3d &newRotation) override {
        for (auto &obj : objects) {
            obj->setRotation(newRotation);
        }
    }

    /**
     * @brief Applies a texture to all contained CoreObjects.
     */
    void attachTexture(const Texture &texture) override {
        for (auto &obj : objects) {
            obj->attachTexture(texture);
        }
    }

    /**
     * @brief Scales every mesh in the model uniformly.
     */
    void setScale(const Scale3d &newScale) override {
        for (auto &obj : objects) {
            obj->setScale(newScale);
        }
    }

    /**
     * @brief Stores the current view matrix for every sub-object.
     */
    void setViewMatrix(const glm::mat4 &view) override {
        for (auto &obj : objects) {
            obj->setViewMatrix(view);
        }
    }

    /**
     * @brief Renders each CoreObject that composes the model.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override {
        for (auto &component : components) {
            component->update(dt);
        }
        for (auto &obj : objects) {
            if (obj == nullptr) {
                continue;
            }
            bool hasAnyTexture = !obj->textures.empty();
            if (!hasAnyTexture) {
                obj->material = material;
            }
            obj->material.useNormalMap = material.useNormalMap;
            obj->material.normalMapStrength = material.normalMapStrength;
            obj->material.textureScale = material.textureScale;
            obj->material.textureOffset = material.textureOffset;
            obj->useDeferredRendering = useDeferredRendering;
            obj->render(dt, commandBuffer, updatePipeline);
        }
    }

    /**
     * @brief Updates all underlying CoreObjects to keep transforms and
     * animations synchronized.
     */
    void update(Window &window) override {
        for (auto &obj : objects) {
            if (obj == nullptr) {
                continue;
            }
            bool hasAnyTexture = !obj->textures.empty();
            if (!hasAnyTexture) {
                obj->material = material;
            }
            obj->material.useNormalMap = material.useNormalMap;
            obj->material.normalMapStrength = material.normalMapStrength;
            obj->material.textureScale = material.textureScale;
            obj->material.textureOffset = material.textureOffset;
            obj->useDeferredRendering = useDeferredRendering;
            obj->update(window);
        }
    }

    /**
     * @brief Initializes every CoreObject and attached component within the
     * model.
     */
    void initialize() override {
        for (auto &component : components) {
            component->init();
        }
        for (auto &obj : objects) {
            if (obj == nullptr) {
                continue;
            }
            bool hasAnyTexture = !obj->textures.empty();
            if (!hasAnyTexture) {
                obj->material = material;
            }
            obj->material.useNormalMap = material.useNormalMap;
            obj->material.normalMapStrength = material.normalMapStrength;
            obj->material.textureScale = material.textureScale;
            obj->material.textureOffset = material.textureOffset;
            obj->useDeferredRendering = useDeferredRendering;
            obj->initialize();
        }
    }

    /**
     * @brief Sets the projection matrix for all meshes composing the model.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override {
        for (auto &obj : objects) {
            obj->setProjectionMatrix(projection);
        }
    }

    /**
     * @brief Forces every mesh to use the provided pipeline.
     */
    void setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) override {
        for (auto &obj : objects) {
            obj->setPipeline(pipeline);
        }
    }

    /**
     * @brief Retrieves the shader program currently bound to the first mesh.
     */

    std::optional<std::shared_ptr<opal::Pipeline>> getPipeline() override {
        if (objects.empty()) {
            return std::nullopt;
        }
        return objects[0]->getPipeline();
    }

    /**
     * @brief Returns the world-space position of the first mesh in the model.
     */
    Position3d getPosition() const override {
        if (objects.empty()) {
            throw std::runtime_error("Model has no objects.");
        }
        return objects[0]->getPosition();
    }

    std::vector<CoreVertex> getVertices() const override {
        std::vector<CoreVertex> vertices;
        size_t totalVertices = 0;
        for (const auto &obj : objects) {
            if (obj == nullptr) {
                continue;
            }
            totalVertices += obj->vertices.size();
        }
        vertices.reserve(totalVertices);
        for (const auto &obj : objects) {
            if (obj == nullptr) {
                continue;
            }
            vertices.insert(vertices.end(), obj->vertices.begin(),
                            obj->vertices.end());
        }
        return vertices;
    }

    Size3d getScale() const override {
        if (objects.empty()) {
            return {1.0f, 1.0f, 1.0f};
        }
        return objects[0]->getScale();
    }

    Rotation3d getRotation() const override {
        if (objects.empty()) {
            return {0.0f, 0.0f, 0.0f};
        }
        return objects[0]->getRotation();
    }

    bool canCastShadows() const override {
        return std::any_of(objects.begin(), objects.end(), [](const auto &obj) {
            return obj != nullptr && obj->canCastShadows();
        });
    }

    Model() = default;

    /**
     * @brief The material properties shared by all objects in the model.
     *
     */
    Material material;

    /**
     * @brief Whether the model should use deferred rendering. When false, the
     * model is rendered in the forward rendering pass.
     *
     */
    bool useDeferredRendering = true;

    /**
     * @brief Checks if the model can use deferred rendering.
     *
     * @return (bool) True if deferred rendering is enabled, false otherwise.
     */
    bool canUseDeferredRendering() override { return useDeferredRendering; }

  private:
    std::vector<std::shared_ptr<CoreObject>> objects;
    std::string directory;
    std::function<void(float, const std::string &)> importProgress;
    unsigned int importedMeshCount = 0;
    unsigned int totalMeshCount = 0;

    void loadModel(
        const Resource &resource,
        const std::function<void(float, const std::string &)> &progress = {});
    void processNode(aiNode *node, const aiScene *scene,
                     glm::mat4 parentTransform,
                     std::unordered_map<std::string, Texture> &textureCache);
    void preloadMaterialTextures(
        const aiScene *scene,
        std::unordered_map<std::string, Texture> &textureCache);
    CoreObject
    processMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform,
                std::unordered_map<std::string, Texture> &textureCache);
    std::vector<Texture> loadMaterialTextures(
        aiMaterial *mat, std::any type, const std::string &typeName,
        std::unordered_map<std::string, Texture> &textureCache);
};

#endif // ATLAS_OBJECT_H
