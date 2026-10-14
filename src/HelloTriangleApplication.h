//
// Created by zanpisotek on 10/7/26.
//

#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#pragma once
#include <string>

// clang-format off
#define VK_CHECK(expression)                                               \
    do {                                                                   \
        const VkResult vkCheckResult = (expression);                       \
        if (vkCheckResult != VK_SUCCESS) {                                 \
            throw std::runtime_error(                                      \
                std::string("Vulkan call failed: ") + #expression +         \
                " | VkResult: " +                                          \
                std::to_string(static_cast<int>(vkCheckResult)) +           \
                " | " + __FILE__ + ":" + std::to_string(__LINE__));         \
        }                                                                  \
    } while (false)
// clang-format on

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        return {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        return {{{.location = 0,
                     .binding = 0,
                     .format = VK_FORMAT_R32G32B32_SFLOAT,
                     .offset = offsetof(Vertex, Position)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, Color)},
            {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, TexCoord)}}};
    }
    bool operator==(const Vertex& other) const {
        return Position == other.Position && Color == other.Color && TexCoord == other.TexCoord;
    }
};

struct GameObject {
    // transform
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f};

    // Uniform buffer for this object - one per frame in flight
    std::vector<VkBuffer> UniformBuffers;
    std::vector<VkDeviceMemory> UniformBufferMemory;
    std::vector<void*> UniformBuffersMapped;

    std::vector<VkDescriptorSet> DescriptorSets;

    glm::mat4 GetModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, Position);
        model = glm::rotate(model, Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Scale);

        return model;
    }
};

class HelloTriangleApplication {
  public:
    static constexpr int MAX_OBJECTS = 3;

    static constexpr uint32_t WIDTH = 1600;
    static constexpr uint32_t HEIGHT = 900;
    const std::string MODEL_PATH = "assets/models/viking_room.obj";
    const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    const std::vector<const char*> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::array<GameObject, MAX_OBJECTS> GameObjects;

#ifdef DEBUG
    static constexpr bool EnableValidationLayers = true;
#else
    static constexpr bool EnableValidationLayers = false;
#endif

    void Run();

  private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();

    void SetupGameObjects();

    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    bool IsDeviceSuitable(const VkPhysicalDevice& device);
    void CreateLogicalDevice();
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    uint32_t ChooseSwapMinImageCount(const VkSurfaceCapabilitiesKHR& capabilities);
    void CreateSwapchain();
    void RecreateSwapchain();
    void CleanupSwapchain();
    void CreateImageViews();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateColorResources();
    void CreateDepthResources();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();
    void LoadModel();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();
    void RecordCommandBuffer(uint32_t imageIndex);
    void TransitionImageLayout(const VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkAccessFlags2 srcAccessMask, VkAccessFlags2 destAccessMask, VkPipelineStageFlags2 srcStageMask,
        VkPipelineStageFlags2 destStageMask, VkImageAspectFlags imageAspectFlags);
    void TransitionImageLayout(VkCommandBuffer commandBuffer, const VkImage& image, VkImageLayout oldLayout,
        VkImageLayout newLayout, uint32_t mipLevels);
    void CreateSyncObjects();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    std::pair<VkBuffer, VkDeviceMemory> CreateBuffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);
    void CopyBufferToImage(
        VkCommandBuffer& commandBuffer, const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);
    void UpdateUniformBuffers(uint32_t currentImage);
    VkImageView CreateImageView(
        VkImage const& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    VkFormat FindSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();
    void GenerateMipmaps(VkCommandBuffer& commandBuffer, VkImage& image, VkFormat format, int32_t texWidth,
        int32_t texHeight, uint32_t mipLevels);
    VkSampleCountFlagBits GetMaxUsableSampleCount();

    void DrawFrame();

    std::vector<const char*> GetRequiredInstanceExtensions();

    // Helpers
    static std::vector<char> ReadFile(const std::string& filename);
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
    std::pair<VkImage, VkDeviceMemory> CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
        VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Loaded
    void vkCreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT* createInfo,
        VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger);

    void vkDestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks* allocator);


    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        std::cerr << "validation layer: type " << std::to_string(type) << " msg: " << pCallbackData->pMessage
                  << std::endl;
        return VK_FALSE;
    }
    static void FremebufferResizeCallback(GLFWwindow* window, int width, int height);

  private:
    GLFWwindow* m_Window = nullptr;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_QueueIndex;
    VkExtent2D m_SwapchainExtnet{};
    VkSurfaceFormatKHR m_SwapchainSurfaceFormat{};
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescripotrSets;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_PresentCompleteSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;
    uint32_t m_MipLevels = 0;
    VkImage m_TextureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_TextureImageMemory = VK_NULL_HANDLE;
    VkImageView m_TextureImageView = VK_NULL_HANDLE;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;

    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage m_ColorImage = nullptr;
    VkDeviceMemory m_ColorImageMemory = nullptr;
    VkImageView m_ColorImageView = nullptr;


    uint32_t m_FrameIndex = 0;
    bool m_FramebufferResized = false;

    // clang-format off
    //std::vector<Vertex> m_Vertices = {
    //    {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    //    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //    {{-0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    //
    //    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    //    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}
    //};
    //
    //const std::vector<uint16_t> m_Indices = {
    //    0, 1, 2, 2, 3, 0,
    //    4, 5, 6, 6, 7, 4,
    //};
    // clang-format on
};
