/*
 window.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Window creation and customization functions
 Copyright (c) 2025 maxvdec
*/

#ifndef WINDOW_H
#define WINDOW_H

#include "atlas/camera.h"
#include "atlas/core/windowing.h"
#include "atlas/core/renderable.h"
#include "atlas/input.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "bezel/bezel.h"
#include "finewave/audio.h"
#include "opal/opal.h"
#include "photon/illuminate.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using CoreWindowReference = SDL_Window *;
using CoreMonitorReference = SDL_DisplayID;

constexpr int WINDOW_CENTERED = -1;
constexpr int DEFAULT_ASPECT_RATIO = -1;

enum class EditorControlMode {
    None = 0,
    Move = 1,
    Rotate = 2,
    Scale = 3,
};

/**
 * @brief Structure representing the configuration options for creating a
 * window.
 *
 */
struct WindowConfiguration {
    /**
     * @brief The title of the window.
     *
     */
    std::string title;
    /**
     * @brief The width of the window in pixels.
     *
     */
    int width;
    /**
     * @brief The height of the window in pixels.
     *
     */
    int height;
    /**
     * @brief Internal rendering resolution scale factor. Values below 1.0
     * render at a lower resolution and upscale to the window, improving
     * performance.
     */
    float renderScale = 0.75f;
    /**
     * @brief Whether the mouse cursor should be captured and hidden.
     *
     */
    bool mouseCaptured = true;
    /**
     * @brief The X position of the window. Use WINDOW_CENTERED to center.
     *
     */
    int posX = WINDOW_CENTERED;
    /**
     * @brief The Y position of the window. Use WINDOW_CENTERED to center.
     *
     */
    int posY = WINDOW_CENTERED;
    /**
     * @brief Whether to enable multisampling anti-aliasing.
     *
     */
    bool multisampling = true;
    /**
     * @brief Whether the window should have decorations (title bar, borders).
     *
     */
    bool decorations = true;
    /**
     * @brief Whether the window can be resized by the user.
     *
     */
    bool resizable = true;
    /**
     * @brief Whether the window framebuffer should be transparent.
     *
     */
    bool transparent = false;
    /**
     * @brief Whether the window should always stay on top of other windows.
     *
     */
    bool alwaysOnTop = false;
    /**
     * @brief The opacity of the window. 1.0 is fully opaque, 0.0 is fully
     * transparent.
     *
     */
    float opacity = 1.0f;
    /**
     * @brief The width component of the aspect ratio. Use -1 for no
     * constraint.
     *
     */
    int aspectRatioX = -1;
    /**
     * @brief The height component of the aspect ratio. Use -1 for no
     * constraint.
     *
     */
    int aspectRatioY = -1;
    /**
     * @brief Resolution scale used specifically for SSAO passes. Lower values
     * reduce quality but greatly boost performance.
     */
    float ssaoScale = 0.5f;

    /**
     * @brief Optional target NSView pointer for Metal presentation.
     *
     * When set (Metal backend only), Atlas renders directly into this view's
     * CAMetalLayer instead of the SDL window's content view.
     */
    void *metalTargetView = nullptr;

    /**
     * @brief Optional SDL window handle retained for SDL subsystems.
     *
     * This is useful when embedding rendering into an external NSView while
     * still using SDL input/audio facilities tied to a host SDL window.
     */
    CoreWindowReference sdlInputWindow = nullptr;

    bool editorControls = false;
};

/**
 * @brief Structure representing a video mode with resolution and refresh rate.
 *
 */
struct VideoMode {
    int width;
    int height;
    int refreshRate;
};

/**
 * @brief Class representing a monitor with video mode querying capabilities.
 *
 */
class Monitor {
  public:
    /**
     * @brief The unique identifier for this monitor.
     *
     */
    int monitorID;
    /**
     * @brief Whether this is the primary monitor.
     *
     */
    bool primary;

    /**
     * @brief Queries all available video modes for this monitor.
     *
     * @return (std::vector<VideoMode>) Vector of available video modes.
     */
    std::vector<VideoMode> queryVideoModes() const;
    /**
     * @brief Gets the current video mode of this monitor.
     *
     * @return (VideoMode) The current video mode.
     */
    VideoMode getCurrentVideoMode() const;

    /**
     * @brief Gets the physical size of the monitor in millimeters.
     *
     * @return (std::tuple<int, int>) Width and height in millimeters.
     */
    std::tuple<int, int> getPhysicalSize() const; // in millimeters
    /**
     * @brief Gets the position of the monitor in the desktop coordinate system.
     *
     * @return (std::tuple<int, int>) X and Y position coordinates.
     */
    std::tuple<int, int> getPosition() const;
    /**
     * @brief Gets the content scale factors for this monitor.
     *
     * @return (std::tuple<float, float>) Horizontal and vertical scale factors.
     */
    std::tuple<float, float> getContentScale() const;
    /**
     * @brief Gets the human-readable name of this monitor.
     *
     * @return (std::string) The monitor name.
     */
    std::string getName() const;

    /**
     * @brief Constructs a Monitor object.
     *
     * @param ref Core monitor reference.
     * @param id Monitor ID.
     * @param isPrimary Whether this is the primary monitor.
     */
    Monitor(CoreMonitorReference ref, int id, bool isPrimary);

    /**
     * @brief Internal reference to the monitor object.
     *
     */
    CoreMonitorReference monitorRef;
};

enum class ControllerAxis {
    LeftStick,
    LeftStickX,
    LeftStickY,
    RightStick,
    RightStickX,
    RightStickY,
    Trigger,
    LeftTrigger,
    RightTrigger
};

enum class ControllerButton : int {
    A = 0,
    B,
    X,
    Y,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftThumb,
    RightThumb,
    DPadUp,
    DPadRight,
    DPadDown,
    DPadLeft,
    ButtonCount,
};

enum class NintendoControllerButton : int {
    B = 0,
    A,
    Y,
    X,
    L,
    R,
    ZL,
    ZR,
    Minus,
    Plus,
    LeftStick,
    RightStick,
    DPadUp,
    DPadRight,
    DPadDown,
    DPadLeft,
    ButtonCount,
};

enum class SonyControllerButton : int {
    Cross = 0,
    Circle,
    Square,
    Triangle,
    L1,
    R1,
    L2,
    R2,
    Share,
    Options,
    LeftStick,
    RightStick,
    DPadUp,
    DPadRight,
    DPadDown,
    DPadLeft,
    ButtonCount,
};

#define CONTROLLER_UNDEFINED (-2)

struct Gamepad {
    int controllerID;
    std::string name;
    bool connected;

    AxisTrigger getAxisTrigger(const ControllerAxis &axis) const;
    static AxisTrigger getGlobalAxisTrigger(const ControllerAxis &axis);
    Trigger getButtonTrigger(int buttonIndex) const;
    static Trigger getGlobalButtonTrigger(int buttonIndex);

    void rumble(float strength, float duration) const;
};

using Controller = Gamepad;

struct Joystick {
    int joystickID;
    std::string name;
    bool connected;

    AxisTrigger getSingleAxisTrigger(int axisIndex) const;
    AxisTrigger getDualAxisTrigger(int axisIndexX, int axisIndexY) const;
    Trigger getButtonTrigger(int buttonIndex) const;

    int getAxisCount() const;
    int getButtonCount() const;
};

struct ControllerID {
    int id;
    std::string name;
    bool isJoystick;
};

struct ShaderProgram;
struct Fluid;

/**
 * @brief Structure representing a window in the application. This contains the
 * main interface for interacting with the engine.
 * \subsection window-example Example
 * ```cpp
 * // Create a window with specific configuration
 * WindowConfiguration config;
 * config.title = "My Game";
 * config.width = 1280;
 * config.height = 720;
 * Window window(config);
 * // Set up camera and scene
 * Camera camera;
 * Scene scene;
 * window.setCamera(&camera);
 * window.setScene(&scene);
 * // Add objects to the scene
 * CoreObject obj;
 * scene.addObject(&obj);
 * // Run the main window loop
 * window.run();
 * ```
 *
 */
class Window {
  public:
    /**
     * @brief The title of the window displayed in the title bar.
     *
     */
    std::string title;
    /**
     * @brief The width of the window in pixels.
     *
     */
    int width;
    /**
     * @brief The height of the window in pixels.
     *
     */
    int height;

    /**
     * @brief Index of the frame currently being rendered.
     */
    int currentFrame;

    /**
     * @brief Constructs a window with the specified configuration.
     *
     * @param config Window configuration settings.
     */
    Window(const WindowConfiguration &config);
    /**
     * @brief Destructor for Window.
     *
     */
    ~Window();
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    /**
     * @brief Sets the clear color used before each render pass.
     */
    void setClearColor(const Color &color) { this->clearColor = color; }

    /**
     * @brief Starts the main window loop and begins rendering.
     *
     */
    void run();
    /**
     * @brief Advances rendering by a single frame.
     *
     * Useful for host-driven embedding scenarios where the host owns the
     * main loop.
     *
     * @return (bool) True while rendering should continue.
     */
    bool stepFrame();
    void resize(int width, int height, float scale = 1.0f);
    void setEditorControlsEnabled(bool enabled);
    bool areEditorControlsEnabled() const { return editorControlsEnabled; }
    void setEditorSimulationEnabled(bool enabled);
    bool isEditorSimulationEnabled() const { return editorSimulationEnabled; }
    void setEditorControlMode(EditorControlMode mode);
    EditorControlMode getEditorControlMode() const { return editorControlMode; }
    void editorPointerEvent(int action, float x, float y, int button,
                            float scale = 1.0f);
    void editorKeyEvent(int key, bool pressed);
    GameObject *getSelectedEditorObject() const { return selectedEditorObject; }
    unsigned int getSelectedEditorObjectId() const;
    /**
     * @brief Tears down state created by stepFrame()/run().
     */
    void endRunLoop();
    /**
     * @brief Closes the window and terminates the application.
     *
     */
    void close();
    /**
     * @brief Sets the window to fullscreen mode.
     *
     * @param enable True to enable fullscreen, false to disable.
     */
    void setFullscreen(bool enable);
    /**
     * @brief Sets the window to fullscreen on a specific monitor.
     *
     * @param monitor The monitor to use for fullscreen.
     */
    void setFullscreen(Monitor &monitor);
    /**
     * @brief Sets the window to windowed mode with new configuration.
     *
     * @param config New window configuration.
     */
    void setWindowed(const WindowConfiguration &config);
    /**
     * @brief Enumerates all available monitors.
     *
     * @return (std::vector<Monitor>) Vector of available monitors.
     */
    std::vector<Monitor> static enumerateMonitors();

    std::vector<ControllerID> getControllers() const;
    Controller getController(const ControllerID &controllerID) const;
    Joystick getJoystick(const ControllerID &joystickID) const;

#ifdef METAL
    void enableGlobalIllumination();
    void enablePathTracing();
#endif

    /**
     * @brief Adds a renderable object to the window.
     *
     * @param object The renderable object to add. \warning The object must be
     * long-lived. This means that declaring it as a class property is a good
     * idea.
     */
    void addObject(Renderable *object);
    /**
     * @brief Removes a previously registered renderable from the window.
     */
    void removeObject(Renderable *object);
    /**
     * @brief Adds a renderable object with higher rendering priority.
     *
     * @param object The renderable object to add with preference. \warning The
     * object must be long-lived. This means that declaring it as a class
     * property is a good idea.
     */
    void addPreferencedObject(Renderable *object);
    /**
     * @brief Adds a renderable object to be rendered first.
     *
     * @param object The renderable object to add to the prelude. \warning The
     * object must be long-lived. This means that declaring it as a class
     * property is a good idea.
     */
    void addPreludeObject(Renderable *object);

    /**
     * @brief Registers a UI renderable so it is drawn after world geometry.
     */
    void addUIObject(Renderable *object);

    /**
     * @brief Adds a renderable to the late forward queue. Late forward
     * renderables are evaluated after the main forward pass.
     */
    void addLateForwardObject(Renderable *object);

    /**
     * @brief Sets the camera for the window.
     *
     * @param newCamera The camera to use for rendering. \warning The camera
     * must be long-lived. This means that declaring it as a class property is
     * a good idea.
     */
    void setCamera(Camera *newCamera);
    /**
     * @brief Sets the scene for the window.
     *
     * @param scene The scene to render.
     */
    void setScene(Scene *scene);

    /**
     * @brief Gets the current time since window creation.
     *
     * @return (float) Time in seconds.
     */
    float getTime();

    /**
     * @brief Checks if a key is currently pressed.
     *
     * @param key The key to check.
     * @return (bool) True if the key is pressed, false otherwise.
     */
    bool isKeyActive(Key key);
    /**
     * @brief Checks if a key was clicked (pressed and released) this frame.
     *
     * @param key The key to check.
     * @return (bool) True if the key was clicked, false otherwise.
     */
    bool isKeyPressed(Key key);

    bool isMouseButtonActive(MouseButton button);
    bool isMouseButtonPressed(MouseButton button);
    const std::string &getTextInput() const { return textInputBuffer; }
    void startTextInput();
    void stopTextInput();
    bool isTextInputActive() const { return textInputActive; }

    bool isControllerButtonPressed(int controllerID, int buttonIndex);
    float getControllerAxisValue(int controllerID, int axisIndex);
    std::pair<float, float> getControllerAxisPairValue(int controllerID,
                                                       int axisIndexX,
                                                       int axisIndexY);

    /**
     * @brief Releases mouse capture, allowing the cursor to move freely.
     *
     */
    void releaseMouse();
    /**
     * @brief Captures the mouse cursor for camera control.
     *
     */
    void captureMouse();

    /**
     * @brief Gets the current cursor position.
     *
     * @return (std::tuple<int, int>) X and Y cursor coordinates.
     */
    std::tuple<int, int> getCursorPosition();

    /**
     * @brief Static pointer to the main window instance. Used for global
     * access to the primary window.
     *
     */
    static Window *mainWindow;

    /**
     * @brief Gets the current scene being rendered.
     *
     * @return (Scene*) Pointer to the current scene.
     */
    Scene *getCurrentScene() { return currentScene; }
    /**
     * @brief Gets the current camera.
     *
     * @return (Camera*) Pointer to the current camera.
     */
    Camera *getCamera() { return camera; }
    /**
     * @brief Adds a render target to the window.
     *
     * @param target The render target to add.
     */
    void addRenderTarget(RenderTarget *target);

    /**
     * @brief Gets the framebuffer size of the window.
     *
     * @return (Size2d) The width and height of the framebuffer.
     */
    Size2d getSize() {
        int fbw, fbh;
        this->queryDrawableSizeInPixels(&fbw, &fbh);
        return {static_cast<float>(fbw), static_cast<float>(fbh)};
    }

    /**
     * @brief Activates debug mode for the window.
     *
     */
    void activateDebug() { this->debug = true; }
    /**
     * @brief Deactivates debug mode for the window.
     *
     */
    void deactivateDebug() { this->debug = false; }

    /**
     * @brief Gets the delta time between frames.
     *
     * @return (float) Delta time in seconds.
     */
    float getDeltaTime() const { return this->deltaTime; }
    /**
     * @brief Gets the current frames per second.
     *
     * @return (float) Frames per second value.
     */
    float getFramesPerSecond() const { return this->framesPerSecond; }

    /**
     * @brief The gravity constant applied to physics bodies. Default is 9.81
     * m/s².
     *
     */
    float gravity = 9.81f;

    void useTracer(bool enable) { this->waitForTracer = enable; }
    /**
     * @brief Configures console log visibility per severity.
     */
    void setLogOutput(bool showLogs, bool showWarnings, bool showErrors);

    /**
     * @brief The audio engine instance for managing spatial audio. Shared
     * across the window.
     *
     */
    std::shared_ptr<AudioEngine> audioEngine;

    /**
     * @brief Enables deferred rendering for the window. This allows for more
     * advanced lighting and post-processing effects.
     *
     */
    void useDeferredRendering();

    /**
     * @brief Whether the window is using deferred rendering.
     *
     */
    bool usesDeferred = false;

    /**
     * @brief Returns the active internal render scale.
     */
    float getRenderScale() const { return this->renderScale; }

    /**
     * @brief Enables Metal-based upscaling and sets the internal ratio.
     */
    void useMetalUpscaling(float ratio = 0.75f);
    bool isMetalUpscalingEnabled() const { return this->metalUpscalingEnabled; }
    float getMetalUpscalingRatio() const { return this->metalUpscalingRatio; }
    bool isRenderingToExternalMetalView() const {
        return this->renderToExternalMetalView;
    }

    /**
     * @brief Returns the SSAO-specific render scale.
     */
    float getSSAORenderScale() const { return this->ssaoRenderScale; }

    /**
     * @brief Returns the opal device instance for rendering.
     */
    std::shared_ptr<opal::Device> getDevice() const { return this->device; }

    /**
     * @brief Returns the lazily created deferred geometry buffer.
     *
     * @return (RenderTarget*) Pointer to the G-buffer contents.
     */
    RenderTarget *getGBuffer() const { return gBuffer.get(); }

    void enableSSR(bool enabled = true) { this->useSSR = enabled; }
    bool isSSREnabled() const { return this->useSSR; }

    /**
     * @brief Points to the render target currently bound for drawing.
     */
    RenderTarget *currentRenderTarget = nullptr;

    /** @brief Destination blend factor for forward pipelines. */
    opal::BlendFunc dstBlend = opal::BlendFunc::DstAlpha;
    /** @brief Source blend factor for forward pipelines. */
    opal::BlendFunc srcBlend = opal::BlendFunc::OneMinusSrcAlpha;
    /** @brief Front-face winding used by forward rendering. */
    opal::FrontFace frontFace = opal::FrontFace::CounterClockwise;
    /** @brief Front-face winding used by deferred passes. */
    opal::FrontFace deferredFrontFace = opal::FrontFace::CounterClockwise;
    /** @brief Active culling mode for generated pipelines. */
    opal::CullMode cullMode = opal::CullMode::Back;
    /** @brief Depth comparison function for generated pipelines. */
    opal::CompareOp depthCompareOp = opal::CompareOp::Less;
    /** @brief Polygon rasterization mode (fill/line/point). */
    opal::RasterizerMode rasterizerMode = opal::RasterizerMode::Fill;
    /** @brief Primitive topology used for draw calls. */
    opal::PrimitiveStyle primitiveStyle = opal::PrimitiveStyle::Triangles;
    /** @brief Line width used when primitive style is lines. */
    float lineWidth = 1.0f;
    /** @brief Enables depth testing when true. */
    bool useDepth = true;
    /** @brief Enables color blending when true. */
    bool useBlending = true;
    /** @brief Enables depth buffer writes when true. */
    bool writeDepth = true;
    /** @brief Enables multisampling in generated pipelines when available. */
    bool useMultisampling = true;
    /** @brief Cached viewport X origin. */
    int viewportX = 0;
    /** @brief Cached viewport Y origin. */
    int viewportY = 0;
    /** @brief Cached viewport width. */
    int viewportWidth = 0;
    /** @brief Cached viewport height. */
    int viewportHeight = 0;
    /** @brief Shared rendering device used by the window. */
    std::shared_ptr<opal::Device> device;

    /** @brief Physics world bound to this window's simulation step. */
    std::shared_ptr<bezel::PhysicsWorld> physicsWorld;

    /** @brief True only during the very first rendered frame. */
    bool firstFrame = true;

    /** @brief Whether DDGI lighting is enabled on supported platforms. */
    bool usesGlobalIllumination = false;
    /** @brief Whether the path tracing pass is currently enabled. */
    bool usePathTracing = false;
    std::shared_ptr<photon::GlobalIllumination> ddgiSystem;
    std::shared_ptr<photon::PathTracing> pathTracer;

    /**
     * @brief Computes the world-space bounds that enclose visible scene
     * geometry.
     */
    BoundingBox getSceneBoundingBox();

    void addInputAction(const std::shared_ptr<InputAction> &action) {
        inputActions.push_back(action);
    }
    void resetInputActions() { inputActions.clear(); }
    std::shared_ptr<InputAction> getInputAction(const std::string &name) const {
        for (const auto &action : inputActions) {
            if (action->name == name) {
                return action;
            }
        }
        return nullptr;
    }

    bool isActionTriggered(const std::string &actionName);
    bool isActionCurrentlyActive(const std::string &actionName);
    AxisPacket getAxisActionValue(const std::string &actionName);

  private:
    bool isTriggerActive(const Trigger &trigger);
    bool isTriggerPressed(const Trigger &trigger);
    Position2d relativeMousePos;
    std::vector<std::shared_ptr<InputAction>> inputActions;
    std::array<bool, SDL_SCANCODE_COUNT> keysPressedThisFrame{};
    std::array<bool, 9> mouseButtonsPressedThisFrame{};
    std::string textInputBuffer;
    bool textInputActive = false;
    std::shared_ptr<opal::CommandBuffer> activeCommandBuffer = nullptr;
    CoreWindowReference windowRef;
    std::vector<Renderable *> pendingObjects;
    std::vector<Renderable *> pendingRemovals;
    std::vector<Renderable *> renderables;
    std::vector<Renderable *> preferenceRenderables;
    std::vector<Renderable *> firstRenderables;
    std::vector<Renderable *> uiRenderables;
    std::vector<Renderable *> lateForwardRenderables;
    std::vector<Fluid *> lateFluids;
    std::vector<RenderTarget *> renderTargets;
    std::shared_ptr<RenderTarget> screenRenderTarget;

    std::shared_ptr<RenderTarget> gBuffer;
    std::shared_ptr<RenderTarget> ssaoBuffer;
    std::shared_ptr<RenderTarget> ssaoBlurBuffer;
    std::shared_ptr<RenderTarget> volumetricBuffer;
    std::shared_ptr<RenderTarget> lightBuffer;
    std::shared_ptr<RenderTarget> ssrFramebuffer;
    std::shared_ptr<BloomRenderTarget> bloomBuffer;

    bool waitForTracer = false;

    std::vector<glm::vec3> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise;
    Texture noiseTexture;

    Color clearColor = Color(0.0f, 0.0f, 0.0f, 1.0f);

    void setupSSAO();
    void applyScene(Scene *scene);

    glm::mat4 calculateProjectionMatrix();
    glm::mat4 lastViewMatrix = glm::mat4(1.0f);
    Scene *currentScene = nullptr;
    Scene *pendingScene = nullptr;
    bool hasPendingSceneChange = false;

    void renderLightsToShadowMaps(
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    Size2d getFurthestPositions();

    [[maybe_unused]]
    void renderPingpong(RenderTarget *target);
    void renderPhysicalBloom(RenderTarget *target);
    void deferredRendering(
        RenderTarget *target,
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    void
    renderSSAO(std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    void updateFluidCaptures(
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    void captureFluidReflection(
        Fluid &fluid,
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    void captureFluidRefraction(
        Fluid &fluid,
        std::shared_ptr<opal::CommandBuffer> commandBuffer = nullptr);
    void markPipelineStateDirty();
    bool shouldRefreshPipeline(Renderable *renderable);
    void setViewportState(int x, int y, int newViewportWidth,
                          int newViewportHeight);
    void updateBackbufferTarget(int backbufferWidth, int backbufferHeight);
    void renderEditorControls(
        const std::shared_ptr<opal::CommandBuffer> &commandBuffer);
    void updateEditorControlGeometry();
    void selectEditorObjectAt(float x, float y, float scale);
    void updateEditorDrag(float x, float y, float scale);
    void updateEditorCameraDrag(float x, float y, float scale);
    void updateEditorCameraMovement(float deltaTime);
    void queryDrawableSizeInPixels(int *width, int *height) const;
    void initializeRunLoop();
    void pollEvents();

    template <typename T> void updatePipelineStateField(T &field, T value) {
        if (field != value) {
            field = value;
            markPipelineStateDirty();
        }
    }

    Camera *camera = nullptr;
    float lastMouseX;
    float lastMouseY;

    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    float framesPerSecond = 0.0f;
    bool shouldClose = false;
    bool runLoopInitialized = false;
    SDL_WindowID runLoopWindowID = 0;
    std::shared_ptr<opal::RenderPass> runLoopRenderPass = nullptr;

    ShaderProgram depthProgram;
    ShaderProgram pointDepthProgram;
    ShaderProgram deferredProgram;
    ShaderProgram lightProgram;
    ShaderProgram ssaoProgram;
    ShaderProgram ssaoBlurProgram;
    ShaderProgram volumetricProgram;
    ShaderProgram bloomBlurProgram;
    ShaderProgram ssrProgram;

    bool debug = false;
    bool useSSR = false;

    /**
     * @brief Whether to use multi-pass point light shadow rendering.
     * This is true on platforms without geometry shader support (e.g.,
     * macOS/MoltenVK). When true, point light shadows are rendered with 6
     * separate passes instead of 1.
     */
    bool useMultiPassPointShadows = false;

    bool clipPlaneEnabled = false;
    glm::vec4 clipPlaneEquation{0.0f};

    std::array<std::shared_ptr<opal::Framebuffer>, 2> pingpongFramebuffers;
    std::array<std::shared_ptr<opal::Texture>, 2> pingpongTextures;
    int pingpongWidth = 0;
    int pingpongHeight = 0;

    float renderScale = 0.75f;
    float ssaoRenderScale = 0.5f;
    bool metalUpscalingEnabled = false;
    float metalUpscalingRatio = 1.0f;
    bool renderToExternalMetalView = false;
    bool showHostWindow = true;
    void *externalMetalView = nullptr;
    unsigned int bloomBlurPasses = 4;
    int ssaoKernelSize = 32;
    float ssaoUpdateInterval = 1.0f / 45.0f;
    float ssaoUpdateCooldown = 0.0f;
    bool ssaoMapsDirty = true;
    std::optional<Position3d> lastSSAOCameraPosition;
    std::optional<Normal3d> lastSSAOCameraDirection;
    float shadowUpdateInterval = 1.0f / 30.0f;
    float shadowUpdateCooldown = 0.0f;
    bool shadowMapsDirty = true;
    std::optional<Position3d> lastShadowCameraPosition;
    std::optional<Normal3d> lastShadowCameraDirection;
    std::vector<glm::vec3> cachedDirectionalLightDirections;
    std::vector<glm::vec3> cachedPointLightPositions;
    std::vector<glm::vec3> cachedSpotlightPositions;
    std::vector<glm::vec3> cachedSpotlightDirections;
    std::vector<glm::vec3> cachedAreaLightPositions;
    std::vector<glm::vec3> cachedAreaLightNormals;
    std::vector<glm::vec4> cachedAreaLightProperties;
    std::size_t lastShadowCasterSignature = 0;
    bool hasShadowCasterSignature = false;

    bool editorControlsEnabled = false;
    bool editorSimulationEnabled = true;
    EditorControlMode editorControlMode = EditorControlMode::None;
    GameObject *selectedEditorObject = nullptr;
    bool editorDragging = false;
    bool editorCameraDragging = false;
    float editorDragStartX = 0.0f;
    float editorDragStartY = 0.0f;
    float editorCameraLastX = 0.0f;
    float editorCameraLastY = 0.0f;
    float editorDragStartScale = 1.0f;
    Position3d editorDragStartPosition;
    Rotation3d editorDragStartRotation;
    Scale3d editorDragStartObjectScale;
    Position3d editorOrbitPivot;
    float editorOrbitDistance = 3.0f;
    bool editorOrbitPivotInitialized = false;
    std::unique_ptr<CoreObject> editorGridObject;
    std::unique_ptr<CoreObject> editorOutlineObject;
    std::unique_ptr<CoreObject> editorGizmoObject;
    bool editorGridInitialized = false;
    bool editorOutlineInitialized = false;
    bool editorGizmoInitialized = false;
    std::array<bool, 6> editorCameraKeys{};

    void prepareDefaultPipeline(Renderable *renderable, int fbWidth,
                                int fbHeight);

    uint64_t pipelineStateVersion = 1;
    std::unordered_map<Renderable *, uint64_t> renderablePipelineVersions;

    friend class CoreObject;
    friend class RenderTarget;
    friend class DirectionalLight;
    friend class Text;
    friend class Terrain;
    friend class photon::GlobalIllumination;
    friend class photon::PathTracing;
    friend struct Fluid;
};

#endif // WINDOW_H
