/*
 opal.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main opal core rendering implementation
 Copyright (c) 2025 maxvdec
*/

#ifndef OPAL_H
#define OPAL_H

#include "Metal/Metal.hpp"
#include "atlas/core/windowing.h"
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif
#include <cstddef>
#include <memory>
#include <glad/glad.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <glm/glm.hpp>

namespace opal {

/**
 * @file opal/opal.h
 * @brief Opal rendering abstraction layer (OpenGL/Vulkan/Metal).
 *
 * Opal provides context creation, device acquisition, command buffers, and
 * common GPU resources.
 *
 * \note This is an alpha API and may change.
 */

/**
 * @brief OpenGL context profile selection.
 */
enum class OpenGLProfile { Core, Compatibility };

/**
 * @brief Parameters used when creating an Opal `Context`.
 */
struct ContextConfiguration {
    bool useOpenGL = true;
    int majorVersion = 4;
    int minorVersion = 1;
    OpenGLProfile profile = OpenGLProfile::Core;
    std::string applicationName;
    std::string applicationVersion;
    bool createValidationLayers = true;
};

/**
 * @brief Rendering context wrapper (SDL + backend initialization).
 */
class Context {
  public:
    /** @brief Creates a new context instance. */
    static std::shared_ptr<Context> create(ContextConfiguration config = {});
    ~Context();

    void setDecorated(bool enabled);
    void setResizable(bool enabled);
    void setTransparent(bool enabled);
    void setAlwaysOnTop(bool enabled);
    void setSamples(int value);
    void setHighPixelDensity(bool enabled);
    void setHidden(bool enabled);

    SDL_Window *makeWindow(int width, int height, const char *title,
                           SDL_DisplayID displayID = 0);
    void adoptWindow(SDL_Window *existingWindow, bool takeOwnership = false);
    void setMetalTargetView(void *view);
    void *getMetalTargetView() const;
    /** @brief Returns the owned window pointer, if created. */
    SDL_Window *getWindow() const;

    /** @brief Makes the context current for the calling thread. */
    void makeCurrent();

    SDL_Window *window = nullptr;
    SDL_GLContext glContext = nullptr;
    bool ownsWindow = true;
    void *metalTargetView = nullptr;
    ContextConfiguration config;
    bool decorated = true;
    bool resizable = true;
    bool transparent = false;
    bool alwaysOnTop = false;
    bool highPixelDensity = true;
    bool hidden = false;
    int samples = 0;

#ifdef VULKAN
    void createInstance();
    void setupMessenger();
    void setupSurface();
    std::vector<const char *> getExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);

    bool hasValidationLayer();

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
#endif
};

class CommandBuffer;
class Framebuffer;
class Buffer;
class Texture;
#ifdef VULKAN
class CoreRenderPass;
#endif

#ifdef VULKAN
struct ImageCollection {
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};
#endif

struct DeviceInfo {
    /** @brief GPU/adapter name reported by the backend. */
    std::string deviceName;
    /** @brief Vendor name reported by the backend. */
    std::string vendorName;
    /** @brief Driver version string. */
    std::string driverVersion;
    /** @brief Rendering API version string. */
    std::string renderingVersion;
    /** @brief Opal runtime version string. */
    std::string opalVersion;
};

/**
 * @brief Represents the rendering device abstraction for OpenGL/Vulkan.
 * Manages command buffer acquisition, framebuffer access, and device queries.
 *
 * \subsection device-example Example
 * ```cpp
 * // Create context and acquire device
 * ContextConfiguration config;
 * config.useOpenGL = true;
 * config.majorVersion = 4;
 * config.minorVersion = 1;
 * auto context = Context::create(config);
 * auto device = Device::acquire(context);
 *
 * // Get device information
 * DeviceInfo info = device->getDeviceInfo();
 * std::cout << "GPU: " << info.deviceName << std::endl;
 *
 * // Acquire command buffer for rendering
 * auto commandBuffer = device->acquireCommandBuffer();
 * // Record rendering commands...
 * device->submitCommandBuffer(commandBuffer);
 *
 * // Access default framebuffer
 * auto framebuffer = device->getDefaultFramebuffer();
 * ```
 *
 */
class Device {
  public:
    static std::shared_ptr<Device>
    acquire(const std::shared_ptr<Context> &context);
    ~Device();
    std::shared_ptr<CommandBuffer> acquireCommandBuffer();

    void
    submitCommandBuffer(const std::shared_ptr<CommandBuffer> &commandBuffer);
    std::shared_ptr<Framebuffer> getDefaultFramebuffer();

    DeviceInfo getDeviceInfo();
#ifdef METAL
    MTL::Device *getMetalDevice() const;
#endif

  private:
    std::shared_ptr<Framebuffer> defaultFramebuffer = nullptr;

  public:
    long frameCount = 0;
    std::shared_ptr<Context> context = nullptr;
    static Device *globalInstance;

#ifdef VULKAN
    static VkDevice globalDevice;

    VkDevice logicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    ImageCollection swapChainImages;
    VkExtent2D swapChainExtent = {};
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;

    VkCommandPool commandPool = VK_NULL_HANDLE;

    bool swapchainDirty = false;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    bool deviceMeetsRequirements(VkPhysicalDevice device);
    void pickPhysicalDevice(std::shared_ptr<Context> context);
    void createLogicalDevice(std::shared_ptr<Context> context);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                         VkSurfaceKHR surface);

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails
    querySwapChainSupport(VkPhysicalDevice device,
                          std::shared_ptr<Context> context);
    void createSwapChain(std::shared_ptr<Context> context);
    void createImageViews();
    bool supportsDeviceExtension(VkPhysicalDevice device,
                                 const char *extension);

    void destroySwapChainBrightTextures();
    void createSwapChainBrightTextures();
    std::vector<std::shared_ptr<Texture>> swapChainBrightTextures;

    void destroySwapChainDepthTexture();
    void createSwapChainDepthTexture();
    std::shared_ptr<Texture> swapChainDepthTexture;

    std::shared_ptr<Buffer> getDefaultInstanceBuffer();
    std::shared_ptr<Buffer> defaultInstanceBuffer = nullptr;

    void remakeSwapChain(std::shared_ptr<Context> context);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                SDL_Window *window);
    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

#endif
};

enum class TextureType {
    Texture2D,
    TextureCubeMap,
    Texture3D,
    Texture2DArray,
    Texture2DMultisample
};
enum class TextureFormat {
    Rgba8,
    sRgba8,
    Rgb8,
    sRgb8,
    Rgba16F,
    Rgb16F,
    Depth24Stencil8,
    DepthComponent24,
    Depth32F,
    Red8,
    Red16F
};

enum class TextureWrapMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};
enum class TextureFilterMode {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapLinear
};

enum class TextureAxis { S, T, R };

enum class TextureDataFormat { Rgba, Rgb, Red, Bgr, Bgra, DepthComponent };

class Texture {
  public:
    static std::shared_ptr<Texture>
    create(TextureType type, TextureFormat format, int width, int height,
           TextureDataFormat dataFormat = TextureDataFormat::Rgba,
           const void *data = nullptr, uint mipLevels = 1);
    ~Texture();

    /**
     * @brief Creates a multisampled texture for use with multisampled
     * framebuffers.
     * @param format The texture format.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param samples Number of samples (typically 2, 4, 8, or 16).
     * @return A shared pointer to the created multisampled texture.
     */
    static std::shared_ptr<Texture> createMultisampled(TextureFormat format,
                                                       int width, int height,
                                                       int samples = 4);

    /**
     * @brief Creates a depth cubemap texture for omnidirectional shadow
     * mapping.
     * @param format The depth format (should be DepthComponent24 or Depth32F).
     * @param resolution The resolution of each face (width = height).
     * @return A shared pointer to the created depth cubemap texture.
     */
    static std::shared_ptr<Texture> createDepthCubemap(TextureFormat format,
                                                       int resolution);

    /**
     * @brief Creates a 3D texture with optional initial data.
     * @param format The texture format.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param depth Depth in pixels.
     * @param dataFormat The data format.
     * @param data Optional pointer to initial data (float data).
     * @return A shared pointer to the created 3D texture.
     */
    static std::shared_ptr<Texture>
    create3D(TextureFormat format, int width, int height, int depth,
             TextureDataFormat dataFormat = TextureDataFormat::Rgba,
             const void *data = nullptr);

    void updateFace(int faceIndex, const void *data, int width, int height,
                    TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void updateData3D(const void *data, int width, int height, int depth,
                      TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void updateData(const void *data, int width, int height,
                    TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void changeFormat(TextureFormat newFormat);
    void changeBorderColor(const glm::vec4 &borderColor);

    /**
     * @brief Reads texture data from the GPU into a buffer.
     * @param buffer Pre-allocated buffer to store the data.
     * @param dataFormat The format to read the data in.
     */
    void readData(void *buffer,
                  TextureDataFormat dataFormat = TextureDataFormat::Rgba);

    void generateMipmaps(uint levels);
    void automaticallyGenerateMipmaps();
    void setWrapMode(TextureAxis axis, TextureWrapMode mode);
    void setFilterMode(TextureFilterMode minFilter,
                       TextureFilterMode magFilter);

    // Batch parameter setter - sets all common parameters in one bind
    void setParameters(TextureWrapMode wrapS, TextureWrapMode wrapT,
                       TextureFilterMode minFilter,
                       TextureFilterMode magFilter);
    void setParameters3D(TextureWrapMode wrapS, TextureWrapMode wrapT,
                         TextureWrapMode wrapR, TextureFilterMode minFilter,
                         TextureFilterMode magFilter);
#ifdef METAL
    MTL::Texture *getMetalTexture() const;
#endif

    uint textureID = 0;
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::Rgba8;
    int width = 0;
    int height = 0;
    int samples = 1; // For multisampled textures

#ifdef VULKAN
    VkImage vkImage = VK_NULL_HANDLE;
    VkDeviceMemory vkImageMemory = VK_NULL_HANDLE;
    VkImageView vkImageView = VK_NULL_HANDLE;
    VkSampler vkSampler = VK_NULL_HANDLE;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    static std::shared_ptr<Texture> getTextureFromHandle(uint32_t handle);

    static std::shared_ptr<Texture>
    createVulkan(TextureType type, TextureFormat format, int width, int height,
                 TextureDataFormat dataFormat = TextureDataFormat::Rgba,
                 const void *data = nullptr, uint mipLevels = 1);

    static std::shared_ptr<Texture>
    createMultisampledVulkan(TextureFormat format, int width, int height,
                             int samples = 4);

    static std::shared_ptr<Texture>
    createDepthCubemapVulkan(TextureFormat format, int resolution);

    static std::shared_ptr<Texture>
    create3DVulkan(TextureFormat format, int width, int height, int depth,
                   TextureDataFormat dataFormat = TextureDataFormat::Rgba,
                   const void *data = nullptr);
#endif

  private:
    friend class Pipeline;
    friend class CommandBuffer;
    friend class Framebuffer;

    uint glType = 0;
    uint glFormat = 0;

#ifdef VULKAN
    static uint32_t
    registerTextureHandle(const std::shared_ptr<Texture> &texture);
    static std::unordered_map<uint32_t, std::weak_ptr<Texture>>
        textureHandleRegistry;
    static uint32_t nextTextureHandle;
#endif
};

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    TessellationControl,
    TessellationEvaluation,
    Compute
};

#ifdef VULKAN
/**
 * @brief Describes a uniform buffer or resource binding location in SPIR-V
 */
struct UniformBindingInfo {
    uint32_t set;
    uint32_t binding;
    uint32_t size;
    uint32_t offset;
    bool isSampler;
    bool isBuffer;
    bool isStorageBuffer;
    bool isCubemap;
};
#endif

class Shader {
  public:
    static std::shared_ptr<Shader> createFromSource(const char *source,
                                                    ShaderType type);
    ~Shader();

    void compile();

    bool getShaderStatus() const;
    void getShaderLog(char *logBuffer, size_t bufferSize) const;

    uint shaderID;
    ShaderType type;

    char *source = nullptr;

#ifdef VULKAN
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo makeShaderStageInfo() const;

    std::vector<uint32_t> spirvBytecode;

    std::unordered_map<std::string, UniformBindingInfo> uniformBindings;

    void performReflection();
#endif

#if defined(VULKAN) || defined(METAL)
    static int currentId;
#endif

  private:
#ifdef OPENGL
    static uint getGLShaderType(ShaderType type);
#endif
};

class ShaderProgram {
  public:
    static std::shared_ptr<ShaderProgram> create();
    ~ShaderProgram();
    void attachShader(const std::shared_ptr<Shader> &shader, int callerId = -1);

    void link();
    void use();

    bool getProgramStatus() const;
    void getProgramLog(char *logBuffer, size_t bufferSize) const;

    uint programID;
    std::vector<std::shared_ptr<Shader>> attachedShaders;
    bool isComputeProgram() const { return computeProgram; }

#ifdef VULKAN
    std::vector<VkPipelineShaderStageCreateInfo> getShaderStages() const;

    std::unordered_map<std::string, UniformBindingInfo> uniformBindings;

    const UniformBindingInfo *findUniform(const std::string &name) const;
#endif

#if defined(VULKAN) || defined(METAL)
    static int currentId;
#endif

  private:
    bool computeProgram = false;
    friend class Shader;
    friend class Pipeline;
    friend class CommandBuffer;
};

enum class VertexAttributeType {
    Float,
    Double,
    Int,
    UnsignedInt,
    Short,
    UnsignedShort,
    Byte,
    UnsignedByte
};

enum class PrimitiveStyle {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan,
    Patches // For tessellation shaders
};

enum class RasterizerMode { Fill, Line, Point };

enum class VertexBindingInputRate { Vertex, Instance };

enum class CullMode { None, Front, Back, FrontAndBack };

enum class FrontFace { Clockwise, CounterClockwise };

enum class CompareOp {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class BlendFunc {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha
};

enum class BlendEquation { Add, Subtract, ReverseSubtract, Min, Max };

struct VertexAttribute {
    std::string name;
    VertexAttributeType type;
    uint offset;
    uint location;
    bool normalized = false;
    uint size;
    uint stride;
    VertexBindingInputRate inputRate = VertexBindingInputRate::Vertex;
    uint divisor = 0;

    bool operator==(const VertexAttribute &other) const {
        return name == other.name && type == other.type &&
               offset == other.offset && location == other.location &&
               normalized == other.normalized && size == other.size &&
               stride == other.stride && inputRate == other.inputRate &&
               divisor == other.divisor;
    }
};

struct VertexBinding {
    uint stride;
    VertexBindingInputRate inputRate;
};

class PrimitiveAccelerationStructure;

class Pipeline {
  public:
    static std::shared_ptr<Pipeline> create();
    ~Pipeline();

    void setShaderProgram(std::shared_ptr<ShaderProgram> program);

    void setVertexAttributes(const std::vector<VertexAttribute> &attributes,
                             const VertexBinding &binding);

    void setPrimitiveStyle(PrimitiveStyle style);

    /**
     * @brief Sets the number of control points per patch for tessellation.
     * @param count Number of vertices per patch (e.g., 4 for quad patches).
     * Must be called before using PrimitiveStyle::Patches.
     */
    void setPatchVertices(int count);

    /**
     * @brief Gets the number of control points per patch for tessellation.
     * @return The patch vertex count.
     */
    int getPatchVertices() const { return patchVertices; }

    void setViewport(int x, int y, int width, int height);

    void setRasterizerMode(RasterizerMode mode);

    void setCullMode(CullMode mode);

    void setFrontFace(FrontFace face);

    void setLineWidth(float width) { lineWidth = width; }
    void setComputeThreadgroupSize(uint x, uint y = 1, uint z = 1);
    uint getComputeThreadgroupSizeX() const { return computeThreadgroupX; }
    uint getComputeThreadgroupSizeY() const { return computeThreadgroupY; }
    uint getComputeThreadgroupSizeZ() const { return computeThreadgroupZ; }

    void enableDepthTest(bool enabled);
    void setDepthCompareOp(CompareOp op);
    void enableDepthWrite(bool enabled);

    void enableBlending(bool enabled);
    void setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor);
    void setBlendEquation(BlendEquation equation);

    void enableMultisampling(bool enabled);
    void enablePolygonOffset(bool enabled);
    void setPolygonOffset(float factor, float units);
    void enableClipDistance(int index, bool enabled);

    void build();

    void bind();

    bool operator==(const std::shared_ptr<Pipeline> &pipeline) const;

    std::shared_ptr<ShaderProgram> shaderProgram;

    void setUniform3f(const std::string &name, float v0, float v1, float v2);
    void setUniform1i(const std::string &name, int v0);
    void setUniformMat4f(const std::string &name, const glm::mat4 &matrix);
    void setUniform1f(const std::string &name, float v0);
    void setUniformBool(const std::string &name, bool value);
    void setUniform2f(const std::string &name, float v0, float v1);
    void setUniform4f(const std::string &name, float v0, float v1, float v2,
                      float v3);

    /**
     * @brief Binds a buffer of data to a uniform buffer.
     * In OpenGL, this sets array uniforms (e.g., "lights[0].position").
     * In Vulkan, this creates/updates a UBO or SSBO.
     * @param name The base name of the uniform buffer (e.g., "lights")
     * @param data The data to bind
     */
    template <typename T>
    void bindBuffer(const std::string &name, const std::vector<T> &data);

    /**
     * @brief Binds a buffer of data to a uniform buffer using a shared pointer.
     * In OpenGL, this sets array uniforms (e.g., "lights[0].position").
     * In Vulkan, this creates/updates a UBO or SSBO.
     * @param name The base name of the uniform buffer (e.g., "lights")
     * @param data Shared pointer to the data vector
     */
    template <typename T>
    void bindBuffer(const std::string &name,
                    std::shared_ptr<std::vector<T>> data);

    /**
     * @brief Binds raw buffer data to a uniform buffer by name.
     * @param name The name of the uniform buffer
     * @param data Pointer to the raw data
     * @param size Size of the data in bytes
     */
    void bindBufferData(const std::string &name, const void *data, size_t size);
    void bindBuffer(const std::string &name,
                    const std::shared_ptr<Buffer> &buffer, int callerId = -1);
    void bindShaderReadWriteBuffer(const std::string &name,
                                   const std::shared_ptr<Buffer> &buffer,
                                   int callerId = -1);

    void bindTexture(const std::string &name,
                     const std::shared_ptr<Texture> &texture, int unit,
                     int callerId = -1);
    void bindTexture2D(const std::string &name, uint textureId, int unit,
                       int callerId = -1);
    void bindTexture3D(const std::string &name, uint textureId, int unit,
                       int callerId = -1);
    void bindTextureCubemap(const std::string &name, uint textureId, int unit,
                            int callerId = -1);
#ifdef METAL
    void bindTextureArray(const std::vector<std::shared_ptr<Texture>> &textures,
                          uint32_t bufferIndex);
#endif

#ifdef VULKAN
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineColorBlendStateCreateInfo colorBlending;

    std::vector<VkDynamicState> vkDynamicStates;
    std::vector<VkVertexInputBindingDescription> vkBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vkAttributeDescriptions;
    VkViewport vkViewport;
    VkRect2D vkScissor;
    std::vector<VkPipelineColorBlendAttachmentState> vkColorBlendAttachments;
    bool hasInstanceAttributes = false;

    VkFormat getFormat(VertexAttributeType type, uint size,
                       bool normalized) const;
    void buildPipelineLayout();

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorSet> descriptorSets;

    struct DescriptorBindingInfoEntry {
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        VkShaderStageFlags stageFlags = 0;
        uint32_t count = 1;
        uint32_t minBufferSize = 0;
        bool isBuffer = false;
        bool isSampler = false;
        bool isCubemap = false;
    };
    std::map<uint32_t, std::map<uint32_t, DescriptorBindingInfoEntry>>
        descriptorBindingInfo;

    struct UniformBufferAllocation {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void *mappedData = nullptr;
        VkDeviceSize size = 0;
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    };
    std::unordered_map<uint64_t, UniformBufferAllocation> uniformBuffers;
    std::unordered_map<uint64_t, std::shared_ptr<Buffer>> descriptorBuffers;

    static uint64_t makeBindingKey(uint32_t set, uint32_t binding) {
        return (static_cast<uint64_t>(set) << 32) | binding;
    }

    UniformBufferAllocation &
    getOrCreateUniformBuffer(uint32_t set, uint32_t binding, VkDeviceSize size);

    void updateUniformData(uint32_t set, uint32_t binding, uint32_t offset,
                           const void *data, size_t size);

    void buildDescriptorSets();
    void ensureDescriptorResources();
    void bindDescriptorSets(VkCommandBuffer commandBuffer);
    void bindUniformBufferDescriptor(uint32_t set, uint32_t binding);
    void bindSamplerDescriptor(uint32_t set, uint32_t binding,
                               std::shared_ptr<Texture> texture);
    void resetDescriptorSets();
    const DescriptorBindingInfoEntry *
    getDescriptorBindingInfo(uint32_t set, uint32_t binding) const;

    static std::shared_ptr<Texture> getDummyTexture();
    static std::shared_ptr<Texture> getDummyCubemap();

    // Push constant support
    std::vector<uint8_t> pushConstantData;
    uint32_t pushConstantSize = 0;
    VkShaderStageFlags pushConstantStages = 0;
    void updatePushConstant(uint32_t offset, const void *data, size_t size);
    void flushPushConstants(VkCommandBuffer commandBuffer);
    bool pushConstantsDirty = false;
#endif

    bool multisamplingEnabled = false;

  private:
    PrimitiveStyle primitiveStyle = PrimitiveStyle::Triangles;
    int patchVertices = 4; // Default patch size for tessellation
    RasterizerMode rasterizerMode = RasterizerMode::Fill;
    CullMode cullMode = CullMode::Back;
    // Both APIs use CCW; projection Y-flip doesn't affect rasterizer winding
    FrontFace frontFace = FrontFace::CounterClockwise;
    bool blendingEnabled = false;
    BlendFunc blendSrcFactor = BlendFunc::One;
    BlendFunc blendDstFactor = BlendFunc::Zero;
    BlendEquation blendEquation = BlendEquation::Add;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    CompareOp depthCompareOp = CompareOp::Less;
    bool polygonOffsetEnabled = false;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;
    float lineWidth = 1.0f;
    std::vector<int> enabledClipDistances;

    std::vector<VertexAttribute> vertexAttributes;
    VertexBinding vertexBinding;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    uint computeThreadgroupX = 8;
    uint computeThreadgroupY = 8;
    uint computeThreadgroupZ = 1;

    uint getGLBlendFactor(BlendFunc factor) const;
    uint getGLBlendEquation(BlendEquation equation) const;
    uint getGLCompareOp(CompareOp op) const;
    uint getGLPrimitiveStyle(PrimitiveStyle style) const;
    uint getGLRasterizerMode(RasterizerMode mode) const;
    uint getGLCullMode(CullMode mode) const;
    uint getGLFrontFace(FrontFace face) const;
    uint getGLVertexAttributeType(VertexAttributeType type) const;
};

// Template implementations for Pipeline::bindBuffer
template <typename T>
void Pipeline::bindBuffer(const std::string &name, const std::vector<T> &data) {
    bindBufferData(name, data.data(), data.size() * sizeof(T));
}

template <typename T>
void Pipeline::bindBuffer(const std::string &name,
                          std::shared_ptr<std::vector<T>> data) {
    if (data) {
        bindBufferData(name, data->data(), data->size() * sizeof(T));
    }
}

enum class BufferUsage {
    VertexBuffer,
    IndexArray,
    GeneralPurpose,
    UniformBuffer,
    ShaderRead,
    ShaderReadWrite
};

enum class MemoryUsageType { GPUOnly, CPUToGPU, GPUToCPU };

class Buffer {
  public:
    static std::shared_ptr<Buffer>
    create(BufferUsage usage, size_t size, const void *data = nullptr,
           MemoryUsageType memoryUsage = MemoryUsageType::GPUOnly,
           int callerId = -1);
    ~Buffer();

    void updateData(size_t offset, size_t size, const void *data);

    void bind(int callerId = -1) const;
    void unbind(int callerId = -1) const;

    uint bufferID;

    BufferUsage usage;
    MemoryUsageType memoryUsage;

#ifdef VULKAN
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vkStagingBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory vkBufferMemory = VK_NULL_HANDLE;

    static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkBuffer &buffer,
                             VkDeviceMemory &bufferMemory);
    static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                           VkDeviceSize size);
#endif
};

struct VertexAttributeBinding {
    VertexAttribute attribute;
    std::shared_ptr<Buffer> sourceBuffer = nullptr;
};

struct DrawingState {
    std::shared_ptr<Buffer> vertexBuffer = nullptr;
    std::shared_ptr<Buffer> indexBuffer = nullptr;
    std::shared_ptr<Buffer> instanceBuffer = nullptr;

    static std::shared_ptr<DrawingState>
    create(std::shared_ptr<Buffer> vertexBuffer,
           std::shared_ptr<Buffer> indexBuffer = nullptr);

    void setBuffers(std::shared_ptr<Buffer> vertexBuffer,
                    std::shared_ptr<Buffer> indexBuffer = nullptr);

    void bind() const;
    void unbind() const;
    void
    configureAttributes(const std::vector<VertexAttributeBinding> &bindings);

    uint index;
};

class Attachment {
  public:
    enum class Type { Color, Depth, Stencil, DepthStencil };

    Type type;

    std::shared_ptr<Texture> texture;

    bool operator==(const Attachment &other) const {
        return type == other.type && texture->type == other.texture->type &&
               texture->format == other.texture->format;
    }
};

class DepthStencilBuffer {
  public:
    static std::shared_ptr<DepthStencilBuffer>
    create(int width, int height,
           TextureFormat format = TextureFormat::Depth24Stencil8);

    std::shared_ptr<Texture> texture;

    uint renderbufferID;
};

class Framebuffer {
  public:
    static std::shared_ptr<Framebuffer> create(int width, int height);
    static std::shared_ptr<Framebuffer> create();
    ~Framebuffer();

    void addAttachment(const Attachment &attachment);
    void attachDepthStencilBuffer(
        std::shared_ptr<DepthStencilBuffer> depthStencilBuffer);

    /**
     * @brief Attaches a texture to this framebuffer at the specified color
     * attachment.
     * @param texture The texture to attach.
     * @param attachmentIndex The color attachment index (0 =
     * GL_COLOR_ATTACHMENT0, etc.)
     */
    void attachTexture(const std::shared_ptr<Texture> &texture,
                       int attachmentIndex);

    /**
     * @brief Attaches a cubemap texture to this framebuffer for omnidirectional
     * rendering (e.g., point light shadows).
     * @param texture The cubemap texture to attach.
     * @param attachmentType The attachment type (typically Depth for shadows).
     */
    void attachCubemap(const std::shared_ptr<Texture> &texture,
                       Attachment::Type attachmentType);

    /**
     * @brief Attaches a specific face of a cubemap texture to this framebuffer.
     * Used for multi-pass cubemap rendering on platforms without geometry
     * shaders.
     * @param texture The cubemap texture.
     * @param face The face index (0-5: +X, -X, +Y, -Y, +Z, -Z).
     * @param attachmentType The attachment type (typically Depth for shadows).
     */
    void attachCubemapFace(const std::shared_ptr<Texture> &texture, int face,
                           Attachment::Type attachmentType);

    /**
     * @brief Disables color buffer read/write for depth-only rendering.
     * Used for shadow map generation.
     */
    void disableColorBuffer();

    /**
     * @brief Sets the viewport to match this framebuffer's dimensions.
     */
    void setViewport();

    /**
     * @brief Sets the viewport to a custom size.
     */
    void setViewport(int x, int y, int viewWidth, int viewHeight);

    void bind();
    void unbind();

    void bindForRead();
    void bindForDraw();

    /**
     * @brief Sets which color attachments to draw to.
     * @param attachmentCount Number of color attachments to enable (0-N).
     */
    void setDrawBuffers(int attachmentCount);
    int getDrawBufferCount() const { return drawBufferCount; }

    bool getStatus() const;

    uint framebufferID;

    int width;
    int height;
    std::vector<Attachment> attachments;
    bool isDefaultFramebuffer = false;

#ifdef VULKAN
    std::vector<VkFramebuffer> vkFramebuffers;

    void createVulkanFramebuffers(std::shared_ptr<CoreRenderPass> renderPass);
    static void transitionImageLayout(VkImage image, VkFormat format,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout,
                                      uint32_t layerCount = 1);
#endif

  private:
    bool colorBufferDisabled = false;
    int drawBufferCount = -1;
};

#ifdef VULKAN
class CoreRenderPass {
  public:
    static std::shared_ptr<CoreRenderPass>
    create(std::shared_ptr<Pipeline> pipeline,
           std::shared_ptr<Framebuffer> framebuffer);

    // Create a CoreRenderPass that reuses an existing VkRenderPass
    // This is used when switching pipelines mid-render-pass
    static std::shared_ptr<CoreRenderPass>
    createWithExistingRenderPass(std::shared_ptr<Pipeline> pipeline,
                                 std::shared_ptr<Framebuffer> framebuffer,
                                 VkRenderPass existingRenderPass);

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    std::shared_ptr<Pipeline> opalPipeline;
    std::shared_ptr<Framebuffer> opalFramebuffer;
};
#endif

class RenderPass {
  public:
    static std::shared_ptr<RenderPass> create();

    void setFramebuffer(std::shared_ptr<Framebuffer> framebuffer);

    std::shared_ptr<Framebuffer> framebuffer;

#ifdef VULKAN
    static std::vector<std::shared_ptr<CoreRenderPass>> cachedRenderPasses;
    std::shared_ptr<CoreRenderPass> currentRenderPass;
    void applyRenderPass();
#endif
};

class ResolveAction {
  public:
    static std::shared_ptr<ResolveAction>
    create(std::shared_ptr<Framebuffer> source,
           std::shared_ptr<Framebuffer> destination);

    /**
     * @brief Creates a resolve action for a specific color attachment.
     * @param source The source (multisampled) framebuffer.
     * @param destination The destination (regular) framebuffer.
     * @param colorAttachmentIndex The index of the color attachment to resolve.
     * @return A shared pointer to the resolve action.
     */
    static std::shared_ptr<ResolveAction>
    createForColorAttachment(std::shared_ptr<Framebuffer> source,
                             std::shared_ptr<Framebuffer> destination,
                             int colorAttachmentIndex);

    /**
     * @brief Creates a resolve action for the depth buffer only.
     */
    static std::shared_ptr<ResolveAction>
    createForDepth(std::shared_ptr<Framebuffer> source,
                   std::shared_ptr<Framebuffer> destination);

    std::shared_ptr<Framebuffer> source;
    std::shared_ptr<Framebuffer> destination;
    int colorAttachmentIndex = -1; // -1 means all color attachments
    bool resolveDepth = true;
    bool resolveColor = true;
};

#ifdef METAL
struct PrimitiveVertex {
    float position[3];
    float normal[3];
    float tangent[3];
    float bitangent[3];
    float uv[2];
};

class PrimitiveAccelerationStructure {
  public:
    static std::shared_ptr<PrimitiveAccelerationStructure>
    create(const std::vector<PrimitiveVertex> &vertices,
           const std::vector<uint32_t> &indices);

    bool isBuilt = false;

  private:
    friend class CommandBuffer;
    friend class InstanceAccelerationStructure;
    std::shared_ptr<Buffer> scratch;

    MTL::AccelerationStructureDescriptor *blasDescriptor = nullptr;
    MTL::AccelerationStructure *blas = nullptr;
    std::shared_ptr<MTL::Buffer> vertexBuffer;
    std::shared_ptr<MTL::Buffer> indexBuffer;
};

static inline void writeMetalTransform3x4(const glm::mat4 &M, float out3x4[12]);

struct AccelerationStructureInstance {
    std::shared_ptr<PrimitiveAccelerationStructure> blas;
    glm::mat4 transform;
    uint32_t instanceId;
    uint32_t mask;
    bool cullDisable;
};

class InstanceAccelerationStructure {
  public:
    static std::shared_ptr<opal::InstanceAccelerationStructure>
    create(const std::vector<opal::AccelerationStructureInstance> &instances);

    bool isBuilt = false;

  private:
    std::vector<AccelerationStructureInstance> instances;
    std::shared_ptr<Buffer> instanceBuffer;
    MTL::InstanceAccelerationStructureDescriptor *tlasDescriptor = nullptr;
    MTL::AccelerationStructure *tlas = nullptr;

    std::vector<std::shared_ptr<PrimitiveAccelerationStructure>> blasRefs;
    std::vector<MTL::AccelerationStructure *> blasPtrs;

    std::shared_ptr<Buffer> scratch;

    friend class CommandBuffer;
};

#endif

class CommandBuffer {
  public:
    ~CommandBuffer();
    void start();
    void beginPass(std::shared_ptr<RenderPass> renderPass);
    void beginSampled(const std::shared_ptr<Framebuffer> &readFramebuffer,
                      const std::shared_ptr<Framebuffer> &writeFramebuffer);
    void endPass();
    void commit();

    // The different commands
    void bindPipeline(const std::shared_ptr<Pipeline> &pipeline);
    void unbindPipeline();
    void bindDrawingState(std::shared_ptr<DrawingState> drawingState);
    void unbindDrawingState();
    void draw(uint vertexCount, uint instanceCount = 1, uint firstVertex = 0,
              [[maybe_unused]] uint firstInstance = 0, int objectId = -1);
    void drawIndexed(uint indexCount, uint instanceCount = 1,
                     uint firstIndex = 0, int vertexOffset = 0,
                     uint firstInstance = 0, int objectId = -1);
    /**
     * @brief Draws using tessellation patches.
     * @param vertexCount Number of vertices to draw.
     * @param firstVertex Starting vertex offset.
     * Requires Pipeline with PrimitiveStyle::Patches and setPatchVertices().
     */
    void drawPatches(uint vertexCount, uint firstVertex = 0, int objectId = -1);
    void dispatch(uint threadCountX, uint threadCountY = 1,
                  uint threadCountZ = 1);
    void computeBarrier();
    void generateMipmaps(const std::shared_ptr<Texture> &texture);
    void performResolve(const std::shared_ptr<ResolveAction> &resolveAction);
    bool performSpatialUpscale(const std::shared_ptr<Texture> &sourceTexture);

    void clearColor(float r, float g, float b, float a);
    void clearDepth(float depth);
    void clear(float r, float g, float b, float a, float depth);

    int getAndResetDrawCallCount();

#ifdef METAL
    void buildPrimitiveAccelerationStructure(
        const std::shared_ptr<PrimitiveAccelerationStructure> &blas);

    void bindPrimitiveAccelerationStructure(
        const std::shared_ptr<PrimitiveAccelerationStructure> &blas,
        uint32_t binding);

    void buildInstanceAccelerationStructure(
        const std::shared_ptr<InstanceAccelerationStructure> &tlas);

    void bindInstanceAccelerationStructure(
        const std::shared_ptr<InstanceAccelerationStructure> &tlas,
        uint32_t binding);

#endif

  private:
#ifdef VULKAN
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool imageAcquired = false;
    bool commandBufferBegan = false;
    void record(uint32_t imageIndex);
    void beginCommandBufferIfNeeded();
    void createSyncObjects();
    void bindVertexBuffersIfNeeded();

    VkCommandBuffer getCurrentCommandBuffer() const {
        return commandBuffers.empty() ? VK_NULL_HANDLE
                                      : commandBuffers[currentFrame];
    }
#endif

    float clearColorValue[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepthValue = 1.0f;

    int drawCallCount = 0;

    bool hasStarted = false;

    friend class Device;
    std::shared_ptr<Pipeline> boundPipeline = nullptr;
    std::shared_ptr<DrawingState> boundDrawingState = nullptr;
    std::shared_ptr<RenderPass> renderPass = nullptr;
    std::shared_ptr<Framebuffer> framebuffer = nullptr;
    Device *device = nullptr;
};

#ifdef VULKAN
VkFormat opalTextureFormatToVulkanFormat(TextureFormat format);
#endif

} // namespace opal

#endif // OPAL_H
