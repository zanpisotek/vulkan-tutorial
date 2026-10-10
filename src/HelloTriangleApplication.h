//
// Created by zanpisotek on 10/7/26.
//

#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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
    glm::vec2 Position;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription() {
        return {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        return {{
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, Position)},
            {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, Color)},
        }};
    }
};

class HelloTriangleApplication {
  public:
    static constexpr uint32_t WIDTH = 1600;
    static constexpr uint32_t HEIGHT = 900;
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    const std::vector<const char*> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();
    void RecordCommandBuffer(uint32_t imageIndex);
    void TransitionImageLayout(uint32_t imageIndex, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkAccessFlags2 srcAccessMask, VkAccessFlags2 destAccessMask, VkPipelineStageFlags2 srcStageMask,
        VkPipelineStageFlags2 destStageMask);
    void CreateSyncObjects();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    std::pair<VkBuffer, VkDeviceMemory> CreateBuffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);
    void UpdateUniformBuffer(uint32_t currentImage);

    void DrawFrame();

    std::vector<const char*> GetRequiredInstanceExtensions();

    // Helpers
    static std::vector<char> ReadFile(const std::string& filename);
    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char>& code) const;

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

    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    uint32_t m_FrameIndex = 0;
    bool m_FramebufferResized = false;

    // clang-format off
    std::vector<Vertex> m_Vertices = {
        {{-0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}
    };
    // clang-format on

    const std::vector<uint16_t> m_Indices = {0, 1, 2, 2, 3, 0};
};
