/*
 texture.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture definition and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include "atlas/core/shader.h"
#include <array>
#include <utility>
#include <vector>
#pragma once

#include "atlas/core/renderable.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include "opal/opal.h"
#include <memory>
#include <string>

class Window;
class CoreObject;

/**
 * @brief Structure that holds the creation data for a texture.
 *
 */
struct TextureCreationData {
    /**
     * @brief Width of the texture in pixels.
     */
    int width = 0;
    /**
     * @brief Height of the texture in pixels.
     */
    int height = 0;
    /**
     * @brief Number of color channels in the texture data.
     */
    int channels = 0;
};

/**
 * @brief Enumeration of texture wrapping modes. These modes determine how
 * textures are sampled when texture coordinates fall outside the standard range
 * of [0, 1].
 *
 */
enum class TextureWrappingMode {
    /**
     * @brief Repeats the texture infinitely in both directions.
     */
    Repeat,
    /**
     * @brief Alternates the texture direction on every repeat for a mirrored
     * look.
     */
    MirroredRepeat,
    /**
     * @brief Clamps coordinates to the texture edge, stretching edge pixels.
     */
    ClampToEdge,
    /**
     * @brief Uses a constant border color outside the [0,1] range.
     */
    ClampToBorder
};

/**
 * @brief Enumeration of texture filtering modes. These modes determine how
 * textures are sampled when they are minified or magnified.
 *
 */
enum class TextureFilteringMode {
    /**
     * @brief Picks the texel closest to the lookup coordinate.
     */
    Nearest,
    /**
     * @brief Linearly interpolates texels for smoother sampling.
     */
    Linear
};

/**
 * @brief Structure that holds the parameters for texture creation.
 *
 */
struct TextureParameters {
    /**
     * @brief Wrapping mode for the horizontal (S) axis.
     */
    TextureWrappingMode wrappingModeS = TextureWrappingMode::Repeat;
    /**
     * @brief Wrapping mode for the vertical (T) axis.
     */
    TextureWrappingMode wrappingModeT = TextureWrappingMode::Repeat;
    /**
     * @brief Filter applied when the texture is minified.
     */
    TextureFilteringMode minifyingFilter = TextureFilteringMode::Linear;
    /**
     * @brief Filter applied when the texture is magnified.
     */
    TextureFilteringMode magnifyingFilter = TextureFilteringMode::Linear;
};

/**
 * @brief Type that represents a texture ID.
 *
 */
enum class TextureType : int {
    Color = 0,
    Specular = 1,
    Cubemap = 2,
    Depth = 3,
    DepthCube = 4,
    Normal = 5,
    Parallax = 6,
    SSAONoise = 7,
    SSAO = 8,
    Metallic = 9,
    Roughness = 10,
    AO = 11,
    Opacity = 12,
    HDR = 13,
    PBRPack = 14
};

/**
 * @brief Structure that holds the data for a checkerboard texture tile.
 *
 */
struct CheckerTile {
    /** @brief First alternating checker color. */
    Color color1;
    /** @brief Second alternating checker color. */
    Color color2;
    /** @brief Size of each checker cell in pixels. */
    int checkSize;
};

/**
 * @brief Structure that holds the data for a texture. It is created from a
 * resource.
 * \subsection texture-example Example
 * ```cpp
 * // Load a texture from a resource
 * Texture myTexture = Texture::fromResourceName("MyTextureResource",
 * TextureType::Color);
 * // Create a checkerboard texture
 * Texture checkerTexture = Texture::createCheckerboard(512, 512, 32,
 * Color::white(), Color::black());
 * // Create a tiled checkerboard texture
 * std::vector<CheckerTile> tiles = {
 *     {Color::red(), Color::black(), 64},
 *     {Color::green(), Color::black(), 32},
 *     {Color::blue(), Color::black(), 16}
 * };
 * Texture tiledTexture = Texture::createTiledCheckerboard(512, 512, tiles);
 * ```
 *
 */
struct Texture {
    /**
     * @brief Resource from which the texture was created. \warning When
     * creating the texture from a checkerboard or other method, this will be
     * empty.
     *
     */
    Resource resource;
    /**
     * @brief The data for the texture creation.
     *
     */
    TextureCreationData creationData;
    /**
     * @brief The core Id for the texture in OpenGL.
     *
     */
    Id id = 0;
    std::shared_ptr<opal::Texture> texture;
    /**
     * @brief The type of the texture (e.g., Color, Specular, Cubemap).
     *
     */
    TextureType type = TextureType::Color;
    /**
     * @brief The border color used when the wrapping mode is set to use.
     *
     */
    Color borderColor = {0, 0, 0, 0};

    /**
     * @brief Creates a texture from a resource.
     *
     * @param resource The resource from which to create the texture.
     * @param type The type of texture to create.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created texture instance.
     */
    static Texture fromResource(const Resource &resource,
                                TextureType type = TextureType::Color,
                                TextureParameters params = {},
                                Color borderColor = {0, 0, 0, 0});
    /**
     * @brief Creates a texture from a resource name.
     *
     * @param resourceName The target resource's name.
     * @param type The type of texture to create.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created texture instance.
     */
    static Texture fromResourceName(const std::string &resourceName,
                                    TextureType type = TextureType::Color,
                                    TextureParameters params = {},
                                    Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param checkSize The size of each checker square.
     * @param color1 The first color for the checkerboard.
     * @param color2 The second color for the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created checkerboard texture instance.
     */
    static Texture createCheckerboard(int width, int height, int checkSize,
                                      Color color1, Color color2,
                                      TextureParameters params = {},
                                      Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a double checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param checkSizeBig The size of the bigger checker squares.
     * @param checkSizeSmall The size of the smaller checker squares.
     * @param color1 The first color of the checkerboard.
     * @param color2 The second color of the checkerboard.
     * @param color3 The third color of the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created double checkerboard texture instance.
     */
    static Texture createDoubleCheckerboard(int width, int height,
                                            int checkSizeBig,
                                            int checkSizeSmall, Color color1,
                                            Color color2, Color color3,
                                            TextureParameters params = {},
                                            Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a tiled checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param tiles The tiles to use for the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created tiled checkerboard texture instance.
     */
    static Texture createTiledCheckerboard(
        int width, int height, const std::vector<CheckerTile> &tiles,
        TextureParameters params = {}, Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a vertically stretched streak texture suited for rain
     * particles.
     */
    static Texture createRainStreak(int width, int height,
                                    TextureParameters params = {},
                                    Color borderColor = {0, 0, 0, 0});
    /**
     * @brief Creates an empty texture with explicit GPU/data formats.
     */
    static Texture
    create(int width, int height,
           opal::TextureFormat format = opal::TextureFormat::Rgba8,
           opal::TextureDataFormat dataFormat = opal::TextureDataFormat::Rgba,
           TextureType type = TextureType::Color, TextureParameters params = {},
           Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a screen-aligned preview quad and displays this texture.
     */
    void display(Window &window, float zindex = 0);

    /**
     * @brief Optional helper object used when displaying the texture directly.
     */
    std::shared_ptr<CoreObject> object = nullptr;

  private:
    static void
    applyWrappingMode(TextureWrappingMode mode, opal::TextureAxis axis,
                      const std::shared_ptr<opal::Texture> &texture);
    static void
    applyFilteringModes(TextureFilteringMode minMode,
                        TextureFilteringMode magMode,
                        const std::shared_ptr<opal::Texture> &texture);
};

/**
 * @brief Structure that holds the data for a cubemap texture.
 *
 */
struct Cubemap {
    /**
     * @brief The resources for the six faces of the cubemap, in the order:
     * +X, -X, +Y, -Y, +Z, -Z
     */
    std::array<Resource, 6> resources;
    /**
     * @brief The data for the texture creation.
     *
     */
    TextureCreationData creationData;
    /**
     * @brief The core Id for the texture in OpenGL.
     *
     */
    Id id = 0;
    /**
     * @brief The opal texture object.
     *
     */
    std::shared_ptr<opal::Texture> texture;
    /**
     * @brief The type of the texture (Cubemap).
     *
     */
    TextureType type = TextureType::Cubemap;

    /**
     * @brief Average color sampled across the cubemap faces for ambient
     * approximations.
     */
    Color averageColor = Color::white();

    /**
     * @brief Flag indicating whether averageColor contains sampled data.
     */
    bool hasAverageColor = false;

    /**
     * @brief Creates a cubemap from a resource group.
     *
     * @param resourceGroup The resource group from which to create the cubemap.
     * It must contain exactly six resources.
     * @return (Cubemap) The created cubemap instance.
     */
    static Cubemap fromResourceGroup(ResourceGroup &resourceGroup);

    /**
     * @brief Creates a cubemap where each face is initialized from a solid
     * color.
     */
    static Cubemap fromColors(const std::array<Color, 6> &colors, int size);

    /**
     * @brief Updates the existing cubemap faces with new solid colors.
     */
    void updateWithColors(const std::array<Color, 6> &colors);
};

/**
 * @brief The type of the texture (RenderTarget).
 *
 */
enum class RenderTargetType {
    Scene,
    Multisampled,
    Shadow,
    CubeShadow,
    GBuffer,
    SSAO,
    SSAOBlur,
    SSR,
};

class Effect;

/**
 * @brief Class that represents a render target, which is a texture in which a
 * scene can be rendered. This is useful for applying post-processing effects or
 * for rendering to textures for use in materials.
 * \subsection render-target-example Example
 * ```cpp
 * // Create a render target with a resolution of 1024x1024
 * RenderTarget renderTarget(window, RenderTargetType::Scene, 1024);
 * // Display the render target in the window
 * renderTarget.display(window, -1.0f);
 * // Hide the render target
 * renderTarget.hide();
 * // Show the render target again
 * renderTarget.show();
 * ```
 *
 */
class RenderTarget : public Renderable {
  public:
    RenderTarget() = default;

    /**
     * @brief The texture associated with the render target. This will be the
     * output of the rendering process.
     */
    Texture texture;
    /**
     * @brief Texture containing bright areas for bloom effects.
     *
     */
    Texture brightTexture;
    /**
     * @brief Texture containing the blurred version for post-processing.
     *
     */
    Texture blurredTexture;
    /**
     * @brief Texture containing depth information for depth-based effects.
     *
     */
    Texture depthTexture;

    /**
     * @brief G-buffer texture for world-space positions (deferred rendering).
     *
     */
    Texture gPosition;
    /**
     * @brief G-buffer texture for normals (deferred rendering).
     *
     */
    Texture gNormal;
    /**
     * @brief G-buffer texture for albedo and specular intensity (deferred
     * rendering).
     *
     */
    Texture gAlbedoSpec;
    /**
     * @brief G-buffer texture for material properties (deferred rendering).
     *
     */
    Texture gMaterial;

    /**
     * @brief Multisampled texture for anti-aliasing.
     *
     */
    Texture msTexture;
    /**
     * @brief Multisampled depth texture for anti-aliasing.
     *
     */
    Texture msDepthTexture;
    /**
     * @brief Multisampled bright texture for anti-aliasing.
     *
     */
    Texture msBrightTexture;
    /**
     * @brief Texture for volumetric lighting.
     */
    Texture volumetricLightTexture;
    Texture ssrTexture;
    Texture LUT;
    /**
     * @brief The type of the render target.
     *
     */
    RenderTargetType type;

    /**
     * @brief Function that constructs a new RenderTarget object.
     *
     * @param window The window in which the render target will be used.
     * @param type The type of render target to create.
     * @param resolution The resolution of the render target (it will be created
     * with this resolution).
     */
    RenderTarget(Window &window,
                 RenderTargetType type = RenderTargetType::Scene,
                 int resolution = 1024);

    /**
     * @brief Displays the render target in the window.
     *
     * @param window The window in which to display the render target.
     * @param zindex The z-index at which to display the render target.
     */
    void display(Window &window, float zindex = 0);
    /**
     * @brief Hides the render target from the window.
     *
     */
    void hide() const;
    /**
     * @brief Shows the render target in the window.
     *
     */
    void show() const;

    /**
     * @brief The CoreObject used to render the render target.
     *
     */
    std::shared_ptr<CoreObject> object = nullptr;

    /**
     * @brief Renders the quad representing this render target, applying any
     * queued post-processing effects.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Resolves the render target by copying multisampled buffers to
     * regular textures.
     *
     */
    void resolve();

    /**
     * @brief Width of the render target in pixels.
     */
    int getWidth() const;
    /**
     * @brief Height of the render target in pixels.
     */
    int getHeight() const;

    /**
     * @brief Adds a post-processing effect to the render target.
     *
     * @param effect The effect to add.
     */
    void addEffect(const std::shared_ptr<Effect> &effect) {
        effects.push_back(effect);
    }

    /**
     * @brief Binds this render target's framebuffer for rendering.
     */
    void bind();

    /**
     * @brief Binds a specific face of a cubemap render target for rendering.
     * Used for multi-pass cubemap rendering on platforms without geometry
     * shaders.
     * @param face The face index (0-5: +X, -X, +Y, -Y, +Z, -Z).
     */
    void bindCubemapFace(int face);

    /**
     * @brief Unbinds the framebuffer (binds the default framebuffer).
     */
    void unbind();

    /**
     * @brief Returns the primary framebuffer for this render target.
     * @return The framebuffer, or nullptr if not created.
     */
    const std::shared_ptr<opal::Framebuffer> &getFramebuffer() const;

    /**
     * @brief Returns the resolve framebuffer for multisampled targets.
     * @return The resolve framebuffer, or nullptr if not applicable.
     */
    const std::shared_ptr<opal::Framebuffer> &getResolveFramebuffer() const;

  private:
    std::shared_ptr<opal::Framebuffer> fb = nullptr;
    std::shared_ptr<opal::Framebuffer> resolveFb = nullptr;
    std::shared_ptr<opal::DepthStencilBuffer> renderbuffer = nullptr;
    std::vector<std::shared_ptr<Effect>> effects;

    friend class Window;
    friend struct Fluid;
};

/**
 * @brief Class that represents a skybox, which is a large cube that surrounds
 * the scene to introduce stunning backgrounds.
 * \subsection skybox-example Example
 * ```cpp
 * // Load a cubemap texture for the skybox
 * Cubemap skyboxCubemap = Cubemap::fromResourceGroup(
 *     Workspace::get().getResourceGroup("SkyboxResources"));
 * // Create a skybox with the cubemap texture
 * Skybox skybox;
 * skybox.cubemap = skyboxCubemap;
 * // Display the skybox in the window
 * skybox.display(window);
 * // Hide the skybox
 * skybox.hide();
 * // Show the skybox again
 * skybox.show();
 * ```
 *
 */
class Skybox : public Renderable {
  public:
    static std::shared_ptr<Skybox> create(Cubemap cubemap, Window &window) {
        auto skybox = std::make_shared<Skybox>();
        skybox->cubemap = std::move(cubemap);
        skybox->display(window);
        return skybox;
    }

    /**
     * @brief The cubemap texture used for the skybox.
     *
     */
    Cubemap cubemap;

    /**
     * @brief The CoreObject used to render the skybox.
     *
     */
    std::shared_ptr<CoreObject> object = nullptr;

    /**
     * @brief Displays the skybox in the window.
     *
     * @param window The window in which to display the skybox.
     */
    void display(Window &window);
    /**
     * @brief Hides the skybox from the window.
     *
     */
    void hide() const;
    /**
     * @brief Shows the skybox in the window.
     *
     */
    void show() const;

    /**
     * @brief Renders the skybox cube using the stored cubemap texture.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Updates the view matrix while removing translation for correct
     * skybox rendering.
     */
    void setViewMatrix(const glm::mat4 &newView) override;
    /**
     * @brief Stores the projection matrix used for skybox rendering.
     */
    void setProjectionMatrix(const glm::mat4 &newProjection) override;

    bool canUseDeferredRendering() override { return false; }

    Skybox() = default;

  private:
    friend class Window;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
};

struct BloomElement {
    /** @brief Size of this bloom mip level in floating-point units. */
    glm::vec2 size;
    /** @brief Integer size of this bloom mip level in pixels. */
    glm::ivec2 intSize;
    /** @brief Backend texture identifier for the mip level. */
    Id textureId;
    /** @brief Texture object that stores this bloom level. */
    std::shared_ptr<opal::Texture> texture;
};

/**
 * @brief Helper that manages downsample/upsample chains for bloom rendering.
 */
class BloomRenderTarget {
  public:
    BloomRenderTarget() = default;
    ~BloomRenderTarget() = default;

    /**
     * @brief Initializes the bloom chain with the desired resolution and mip
     * length.
     */
    void init(int width, int height, int chainLength = 5);
    /**
     * @brief Releases GPU resources associated with the bloom chain.
     */
    void destroy();
    /**
     * @brief Binds the bloom FBO so that downsample passes can write results.
     */
    void bindForWriting();
    /**
     * @brief Runs the full bloom blur and upsample pass using the source
     * texture.
     */
    void renderBloomTexture(
        unsigned int srcTexture, float filterRadius,
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    /**
     * @brief Returns the bloom chain hierarchy for inspection or custom
     * rendering.
     */
    const std::vector<BloomElement> &getElements() const;
    /**
     * @brief Retrieves the final composite bloom texture.
     */
    unsigned int getBloomTexture();

  private:
    friend class Window;

    void renderDownsamples(
        unsigned int srcTexture,
        const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    void
    renderUpsamples(float filterRadius,
                    const std::shared_ptr<opal::CommandBuffer> &commandBuffer);

    std::vector<BloomElement> elements;
    bool initialized = false;
    std::shared_ptr<opal::Framebuffer> framebuffer;
    glm::ivec2 srcViewportSize;
    glm::vec2 srcViewportSizef;
    ShaderProgram downsampleProgram;
    ShaderProgram upsampleProgram;

    std::shared_ptr<opal::DrawingState> quadState = nullptr;
    std::shared_ptr<opal::Buffer> quadBuffer = nullptr;
};

#endif // TEXTURE_H
