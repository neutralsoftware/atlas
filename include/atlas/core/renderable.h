/*
 renderable.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Renderable definition and concept
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_RENDERABLE_H
#define ATLAS_RENDERABLE_H

#include "atlas/core/shader.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

struct CoreVertex;
class Window;

/**
 * @brief An abstract base class representing any object that can be rendered in
 * a Window. Contains virtual methods for rendering, initialization, updating,
 * and setting view/projection matrices.
 *
 * \subsection renderable-example Example
 * ```cpp
 * // Create a custom renderable object
 * class CustomRenderable : public Renderable {
 * private:
 *     Position3d position;
 *     glm::mat4 viewMatrix, projectionMatrix;
 *     std::shared_ptr<opal::Pipeline> pipeline;
 *     std::shared_ptr<opal::Buffer> vertexBuffer;
 *
 * public:
 *     void initialize() override {
 *         // Set up buffers, load shaders, create pipeline
 *         opal::Device& device = opal::Device::get();
 *         pipeline = device.createPipeline(...);
 *         vertexBuffer = device.createBuffer(...);
 *     }
 *
 *     void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
 *                 bool updatePipeline) override {
 *         // Bind pipeline and draw
 *         commandBuffer->bindPipeline(pipeline);
 *         commandBuffer->bindVertexBuffer(vertexBuffer);
 *         commandBuffer->draw(vertexCount, 1, 0, 0);
 *     }
 *
 *     void update(Window& window) override {
 *         // Update logic (movement, animation, etc.)
 *         position += Position3d(0.01f, 0.0f, 0.0f);
 *     }
 *
 *     void setViewMatrix(const glm::mat4& view) override {
 *         viewMatrix = view;
 *     }
 *
 *     void setProjectionMatrix(const glm::mat4& projection) override {
 *         projectionMatrix = projection;
 *     }
 *
 *     Position3d getPosition() const override { return position; }
 *     bool canCastShadows() const override { return true; }
 * };
 * ```
 *
 */
class Renderable {
  public:
    /**
     * @brief Function to render the object. Must be implemented by derived
     * classes.
     *
     * @param dt Delta time since the last frame, useful for animations.
     * @param updatePipeline When true, the renderable should rebuild or fetch
     * the graphics pipeline to reflect the window's current state before
     * drawing. When false, the previously prepared pipeline may be reused.
     */
    virtual void render(float dt,
                        std::shared_ptr<opal::CommandBuffer> commandBuffer,
                        bool updatePipeline = false) = 0;
    /**
     * @brief Function to initialize the object. Can be overridden by derived.
     *
     */
    virtual void initialize() {};

    virtual unsigned int getId() { return 0; };
    /**
     * @brief Function to update the object each frame.
     * \warning It runs before the rendering phase and it should only contain
     * logic updates.
     *
     * @param window The window where the object is being rendered.
     */
    virtual void update([[maybe_unused]] Window &window) {};
    /**
     * @brief Function to set the view matrix for the object. Called from \ref
     * Window for internal purposes.
     *
     * @param view The view matrix to set.
     */
    virtual void setViewMatrix([[maybe_unused]] const glm::mat4 &view) {};
    /**
     * @brief Function to set the projection matrix for the object. Called from
     * \ref Window for internal purposes.
     *
     * @param projection The projection matrix to set.
     */
    virtual void
    setProjectionMatrix([[maybe_unused]] const glm::mat4 &projection) {};
    virtual std::optional<std::shared_ptr<opal::Pipeline>> getPipeline() {
        return std::nullopt;
    };

    virtual void
    setPipeline([[maybe_unused]] std::shared_ptr<opal::Pipeline> &pipeline) {};

    /**
     * @brief Returns the currently bound shader program, if any.
     */
    virtual std::optional<ShaderProgram> getShaderProgram() {
        return std::nullopt;
    };

    /**
     * @brief Replaces the shader program bound to the renderable.
     */
    virtual void
    setShader([[maybe_unused]] const ShaderProgram &shaderProgram) {};
    /**
     * @brief Function to get the position of the object in 3D space.
     *
     * @return The position of the object as a Position3d struct.
     */
    virtual Position3d getPosition() const { return {0.0f, 0.0f, 0.0f}; };
    /**
     * @brief Function to get the vertices of the object in 3D space.
     *
     * @return The vertices of the object as a vector of CoreVertex structs.
     */
    virtual std::vector<CoreVertex> getVertices() const { return {}; };
    /**
     * @brief Function to get the scale of the object in 3D space.
     *
     * @return The scale of the object as a Size3d struct.
     */
    virtual Size3d getScale() const { return {1.0f, 1.0f, 1.0f}; };
    /**
     * @brief Function to determine if the object can cast shadows. Can be
     * overridden by derived classes.
     *
     * @return true if the object can cast shadows, false otherwise.
     */
    virtual bool canCastShadows() const { return false; };
    virtual ~Renderable() = default;
    /**
     * @brief Function to determine if the object can use deferred rendering.
     *
     * @return (bool) True if the object supports deferred rendering, false
     * otherwise.
     */
    virtual bool canUseDeferredRendering() { return true; };
    virtual void beforePhysics() {}

    /**
     * @brief Whether the object should be included in depth of field
     * calculations. When true, contributes to blur effects.
     *
     */
    bool renderDepthOfView = false;

    /**
     * @brief Whether the object must be moved to the late forward rendering
     * pipeline. Objects flagged here are skipped during the primary forward
     * pass and rendered only after all standard forward elements.
     */
    bool renderLateForward = false;

    /**
     * @brief Whether the object is an editor-only object (like light debug markers).
     * Objects flagged here will not be rendered if editor controls are disabled.
     */
    bool editorOnly = false;
};

#endif // ATLAS_RENDERABLE_H
