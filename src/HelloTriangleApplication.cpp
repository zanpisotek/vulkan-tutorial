//
// Created by zanpisotek on 10/7/26.
//

#include "HelloTriangleApplication.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <vector>

namespace std {
template <> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.TexCoord) << 1);
    }
};
} // namespace std

std::vector<const char*> HelloTriangleApplication::GetRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

std::vector<char> HelloTriangleApplication::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();
    return buffer;
}

VkShaderModule HelloTriangleApplication::CreateShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}
std::pair<VkImage, VkDeviceMemory> HelloTriangleApplication::CreateImage(uint32_t width, uint32_t height,
    uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
    VkImageCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    VkImage image;
    VK_CHECK(vkCreateImage(m_Device, &createInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties),
    };
    VkDeviceMemory imageMemory;
    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory));
    VK_CHECK(vkBindImageMemory(m_Device, image, imageMemory, 0));

    return {image, imageMemory};
}
VkCommandBuffer HelloTriangleApplication::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}
void HelloTriangleApplication::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, nullptr));
    VK_CHECK(vkQueueWaitIdle(m_GraphicsQueue));
}


void HelloTriangleApplication::Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void HelloTriangleApplication::InitWindow() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FremebufferResizeCallback);
}


void HelloTriangleApplication::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateColorResources();
    CreateDepthResources();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    LoadModel();
    CreateVertexBuffer();
    CreateIndexBuffer();
    SetupGameObjects();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void HelloTriangleApplication::MainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        DrawFrame();
    }
    VK_CHECK(vkDeviceWaitIdle(m_Device));
}

void HelloTriangleApplication::Cleanup() {
    glfwDestroyWindow(m_Window);

    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
    vkDestroyImage(m_Device, m_TextureImage, nullptr);
    vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

    for (const auto& gameObject : GameObjects) {
        for (size_t i = 0; i < gameObject.UniformBuffers.size(); ++i) {
            vkUnmapMemory(m_Device, gameObject.UniformBufferMemory[i]);
            vkDestroyBuffer(m_Device, gameObject.UniformBuffers[i], nullptr);
            vkFreeMemory(m_Device, gameObject.UniformBufferMemory[i], nullptr);
        }
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        vkDestroySemaphore(m_Device, m_PresentCompleteSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < m_SwapchainImages.size(); ++i) {
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
    }
    vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
    vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    CleanupSwapchain();
    vkDestroyDevice(m_Device, nullptr);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    glfwTerminate();
}
void HelloTriangleApplication::SetupGameObjects() {
    // Object 1 - Center
    GameObjects[0].Position = {0.0f, 0.0f, 0.0f};
    GameObjects[0].Rotation = {0.0f, 0.0f, 0.0f};
    GameObjects[0].Scale = {1.0f, 1.0f, 1.0f};

    // Object 2 - Left
    GameObjects[1].Position = {-2.0f, 0.0f, -1.0f};
    GameObjects[1].Rotation = {0.0f, glm::radians(45.0f), 0.0f};
    GameObjects[1].Scale = {0.75f, 0.75f, 0.75f};

    // Object 3 - Right
    GameObjects[2].Position = {2.0f, 0.0f, -1.0f};
    GameObjects[2].Rotation = {0.0f, glm::radians(-45.0f), 0.0f};
    GameObjects[2].Scale = {0.75f, 0.75f, 0.75f};
}

void HelloTriangleApplication::CreateInstance() {
    VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4};

    // Get the required instance extensions form GLFW
    auto requiredExtensions = GetRequiredInstanceExtensions();
    // Get the available instance extensions
    uint32_t extensionCount = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data()));
    extensionProperties.resize(extensionCount);
    auto unsupportedPropertyIt =
        std::ranges::find_if(requiredExtensions, [&extensionProperties](const auto& requiredExtension) {
        return std::ranges::none_of(extensionProperties, [&requiredExtension](const auto& extensionProperty) {
            return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
        });
    });
    if (unsupportedPropertyIt != requiredExtensions.end()) {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

    std::vector<const char*> requiredLayers;
    if (EnableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Get required layers
    uint32_t layerCount = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<VkLayerProperties> layerProperties(layerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data()));
    layerProperties.resize(layerCount);

    // check if layers are supported
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](const auto& requiredLayer) {
        return std::ranges::none_of(layerProperties, [requiredLayer](const auto& layerProperty) {
            return strcmp(layerProperty.layerName, requiredLayer) == 0;
        });
    });
    if (unsupportedLayerIt != requiredLayers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
}

void HelloTriangleApplication::SetupDebugMessenger() {
    if constexpr (!EnableValidationLayers) {
        return;
    }
    VkDebugUtilsMessageSeverityFlagsEXT severityFlags =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &DebugCallback,
    };
    vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
}

void HelloTriangleApplication::CreateSurface() {
    if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != 0) {
        throw std::runtime_error("Failed to create window surface");
    }
}

void HelloTriangleApplication::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()));
    devices.resize(deviceCount);

    if (devices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    // Did this on the fly cause I wanted discrete one
    for (const auto& physicalDevice : devices) {
        if (IsDeviceSuitable(physicalDevice)) {
            m_PhysicalDevice = physicalDevice;

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                break;
        }
    }
    m_MsaaSamples = GetMaxUsableSampleCount();
    // Just for info...
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
    std::cout << "selected GPU: " << props.deviceName << std::endl;
}

bool HelloTriangleApplication::IsDeviceSuitable(const VkPhysicalDevice& device) {
    // check if device supports vulkan 1.3+
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    bool supportsVulkan1_3 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;

    uint32_t queuePropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueProperties(queuePropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuePropertiesCount, queueProperties.data());
    queueProperties.resize(queuePropertiesCount);

    bool supportsGraphics = std::ranges::any_of(queueProperties, [](const auto& qfp) {
        return !!(qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT); // !! converts bitwise expression to bool
    });

    uint32_t deviceExtensionCount = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
    VK_CHECK(
        vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableDeviceExtensions.data()));
    availableDeviceExtensions.resize(deviceExtensionCount);

    bool supportsAllRequiredExtensions = std::ranges::all_of(
        requiredDeviceExtensions, [&availableDeviceExtensions](const auto& requiredDeviceExtension) {
        return std::ranges::any_of(
            availableDeviceExtensions, [requiredDeviceExtension](const auto& availableDeviceExtension) {
            return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
        });
    });

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT};

    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &dynamicStateFeatures};

    VkPhysicalDeviceVulkan11Features features11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &features13};

    VkPhysicalDeviceFeatures2 features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features11};

    vkGetPhysicalDeviceFeatures2(device, &features);

    bool supportsRequiredFeatures =
        features11.shaderDrawParameters && features13.dynamicRendering && dynamicStateFeatures.extendedDynamicState;

    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void HelloTriangleApplication::CreateLogicalDevice() {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
    queueFamilies.resize(queueFamilyCount);

    // auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilies, [](const auto& qfp) {
    //     return (qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT) != static_cast<VkQueueFlags>(0);
    // });
    // auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilies.begin(), graphicsQueueFamilyProperty));

    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilies.size(); ++qfpIndex) {
        VkBool32 supportsPresent = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, qfpIndex, m_Surface, &supportsPresent));
        if ((queueFamilies[qfpIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent) {
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0) {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    m_QueueIndex = queueIndex;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .extendedDynamicState = VK_TRUE,
    };
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &dynamicStateFeatures,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceVulkan11Features features11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &features13,
        .shaderDrawParameters = VK_TRUE,
    };
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features11,
        .features = {.sampleRateShading = VK_TRUE, .samplerAnisotropy = true},
    };

    float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = m_QueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
    };

    VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
    vkGetDeviceQueue(m_Device, m_QueueIndex, 0, &m_GraphicsQueue);
}

VkSurfaceFormatKHR HelloTriangleApplication::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    assert(!availableFormats.empty());

    const auto formatIt = std::ranges::find_if(availableFormats, [](const auto& format) {
        return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
        return presentMode == VK_PRESENT_MODE_FIFO_KHR;
    }));

    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = presentMode;
        }
    }
    return chosenPresentMode;
}

VkExtent2D HelloTriangleApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

uint32_t HelloTriangleApplication::ChooseSwapMinImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
    auto minImageCount = std::max(3u, capabilities.minImageCount);
    if ((0 < capabilities.maxImageCount) && capabilities.maxImageCount < minImageCount) {
        minImageCount = capabilities.maxImageCount;
    }
    return minImageCount;
}

void HelloTriangleApplication::CreateSwapchain() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities));
    m_SwapchainExtnet = ChooseSwapExtent(surfaceCapabilities);
    uint32_t minImageCount = ChooseSwapMinImageCount(surfaceCapabilities);

    uint32_t surfaceFormatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, nullptr));

    std::vector<VkSurfaceFormatKHR> availableFormats(surfaceFormatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_PhysicalDevice, m_Surface, &surfaceFormatCount, availableFormats.data()));
    availableFormats.resize(surfaceFormatCount);
    m_SwapchainSurfaceFormat = ChooseSwapSurfaceFormat(availableFormats);

    uint32_t presentModeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr));

    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_PhysicalDevice, m_Surface, &presentModeCount, availablePresentModes.data()));
    availablePresentModes.resize(presentModeCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = minImageCount,
        .imageFormat = m_SwapchainSurfaceFormat.format,
        .imageColorSpace = m_SwapchainSurfaceFormat.colorSpace,
        .imageExtent = m_SwapchainExtnet,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = ChooseSwapPresentMode(availablePresentModes),
        .clipped = true,
        .oldSwapchain = nullptr};
    VK_CHECK(vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain));

    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr));

    m_SwapchainImages.resize(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data()));
    m_SwapchainImages.resize(imageCount);
}
void HelloTriangleApplication::RecreateSwapchain() {
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    // Minimization: Halt until width or height are not 0 and window should not close
    while ((width == 0 || height == 0) && !glfwWindowShouldClose(m_Window)) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }
    VK_CHECK(vkDeviceWaitIdle(m_Device));

    CleanupSwapchain();

    CreateSwapchain();
    CreateImageViews();
    CreateColorResources();
    CreateDepthResources();
}
void HelloTriangleApplication::CleanupSwapchain() {
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

    vkDestroyImageView(m_Device, m_ColorImageView, nullptr);
    vkDestroyImage(m_Device, m_ColorImage, nullptr);
    vkFreeMemory(m_Device, m_ColorImageMemory, nullptr);

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    for (auto& imageView : m_SwapchainImageViews) {
        vkDestroyImageView(m_Device, imageView, nullptr);
    }
    m_SwapchainImageViews.clear();
    m_Swapchain = VK_NULL_HANDLE;
}


void HelloTriangleApplication::CreateImageViews() {
    assert(m_SwapchainImageViews.empty());

    for (const auto& image : m_SwapchainImages) {
        m_SwapchainImageViews.push_back(
            CreateImageView(image, m_SwapchainSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1));
    }
}
void HelloTriangleApplication::CreateDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout));
}

void HelloTriangleApplication::CreateGraphicsPipeline() {
    auto vertexCode = ReadFile("shaders/Basic.vert.spv");
    auto fragmentCode = ReadFile("shaders/Basic.frag.spv");

    VkShaderModule vertexShaderModule = CreateShaderModule(vertexCode);
    VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentCode);

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "main"};

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShaderModule,
        .pName = "main"};

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkViewport viewport = {
        0.0f, 0.0f, static_cast<float>(m_SwapchainExtnet.width), static_cast<float>(m_SwapchainExtnet.height)};

    // Draw to entire framebuffer so scissor rect should cover it entirely
    VkRect2D scissor{VkOffset2D(0, 0), m_SwapchainExtnet};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    VkPipelineViewportStateCreateInfo viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // we flipped y
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = m_MsaaSamples,
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 0.2f // min fraction for sample shading - closer to 1 is smoother
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_DescriptorSetLayout,
        .pushConstantRangeCount = 0,
    };

    VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_SwapchainSurfaceFormat.format,
        .depthAttachmentFormat = FindDepthFormat(),
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_PipelineLayout,
        .renderPass = nullptr,
    };

    VK_CHECK(vkCreateGraphicsPipelines(m_Device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_Pipeline));

    vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
}
void HelloTriangleApplication::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_QueueIndex,
    };

    VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
}
void HelloTriangleApplication::CreateColorResources() {
    VkFormat colorFormat = m_SwapchainSurfaceFormat.format;

    // we are using 1 mip level since this is enforced by Vulkan spec in case of images with more than one
    // sample per pixel. Also this color buffer doesn't need mip levels since it's not used as a texture
    std::tie(m_ColorImage, m_ColorImageMemory) =
        CreateImage(m_SwapchainExtnet.width, m_SwapchainExtnet.height, 1, m_MsaaSamples, colorFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_ColorImageView = CreateImageView(m_ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
void HelloTriangleApplication::CreateDepthResources() {
    VkFormat depthFormat = FindDepthFormat();

    std::tie(m_DepthImage, m_DepthImageMemory) =
        CreateImage(m_SwapchainExtnet.width, m_SwapchainExtnet.height, 1, m_MsaaSamples, depthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}
void HelloTriangleApplication::CreateTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image");
    }

    auto [stagingBuffer, stagingBufferMemory] = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(m_Device, stagingBufferMemory);

    stbi_image_free(pixels);

    std::tie(m_TextureImage, m_TextureImageMemory) = CreateImage(texWidth, texHeight, m_MipLevels,
        VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
    TransitionImageLayout(
        commandBuffer, m_TextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
    CopyBufferToImage(commandBuffer, stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight));
    GenerateMipmaps(commandBuffer, m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_MipLevels);

    // TransitionImageLayout(commandBuffer, m_TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);

    EndSingleTimeCommands(commandBuffer);

    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}
void HelloTriangleApplication::CreateTextureImageView() {
    m_TextureImageView =
        CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
}
void HelloTriangleApplication::CreateTextureSampler() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE};
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler));
}
void HelloTriangleApplication::LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };
            vertex.TexCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertex.Color = {1.0f, 1.0f, 1.0f};

            auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(m_Vertices.size())});

            if (inserted) {
                m_Vertices.push_back(vertex);
            }
            m_Indices.push_back(it->second);
        }
    }
}
void HelloTriangleApplication::CreateVertexBuffer() {

    VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

    auto [stagingBuffer, stagingBufferMemory] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* dataStaging;
    VK_CHECK(vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &dataStaging));
    memcpy(dataStaging, m_Vertices.data(), bufferSize);
    vkUnmapMemory(m_Device, stagingBufferMemory);

    std::tie(m_VertexBuffer, m_VertexBufferMemory) = CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}
void HelloTriangleApplication::CreateIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

    auto [stagingBuffer, stagingBufferMemory] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    VK_CHECK(vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data));
    memcpy(data, m_Indices.data(), bufferSize);
    vkUnmapMemory(m_Device, stagingBufferMemory);

    std::tie(m_IndexBuffer, m_IndexBufferMemory) = CreateBuffer(bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}
void HelloTriangleApplication::CreateUniformBuffers() {
    const VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for (auto& gameObject : GameObjects) {
        gameObject.UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        gameObject.UniformBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
        gameObject.UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            auto [buffer, memory] = CreateBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            gameObject.UniformBuffers[i] = buffer;
            gameObject.UniformBufferMemory[i] = memory;

            VK_CHECK(vkMapMemory(
                m_Device, memory, 0, bufferSize, 0,
                &gameObject.UniformBuffersMapped[i]));
        }
    }
}
void HelloTriangleApplication::CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSize{};
    poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT;
    poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes = poolSize.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));
}
void HelloTriangleApplication::CreateDescriptorSets() {

    for (auto& gameObject : GameObjects) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
        gameObject.DescriptorSets.resize(layouts.size());

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_DescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
        };

        VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocInfo, gameObject.DescriptorSets.data()));

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VkDescriptorBufferInfo bufferInfo = {
                .buffer = gameObject.UniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject),
            };

            VkDescriptorImageInfo imageInfo = {
                .sampler = m_TextureSampler,
                .imageView = m_TextureImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = gameObject.DescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = gameObject.DescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(
                m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}
void HelloTriangleApplication::CreateCommandBuffers() {
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()));
}
void HelloTriangleApplication::RecordCommandBuffer(uint32_t imageIndex) {
    auto& commandBuffer = m_CommandBuffers[m_FrameIndex];

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    TransitionImageLayout(m_SwapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, {}, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // transition the multisampled color image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    TransitionImageLayout(m_ColorImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // transition depth image to depth attachemnt optimal layout
    TransitionImageLayout(m_DepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT);


    VkClearValue clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    VkClearValue clearDepth = {.depthStencil = {1.0f, 0}};

    VkRenderingAttachmentInfo attachmentInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_SwapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };

    VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_ColorImageView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
        .resolveImageView = m_SwapchainImageViews[imageIndex],
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor,
    };

    VkRenderingAttachmentInfo depthAttachmentInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_DepthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = clearDepth,
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.offset = {0, 0}, .extent = m_SwapchainExtnet},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachmentInfo,
    };

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, offset, VK_INDEX_TYPE_UINT32);

    for (const auto& gameObject : GameObjects) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0,
            1, &gameObject.DescriptorSets[m_FrameIndex], 0, nullptr);

        VkViewport viewport{.x = 0.0f,
            .y = static_cast<float>(m_SwapchainExtnet.height),
            .width = static_cast<float>(m_SwapchainExtnet.width),
            .height = -static_cast<float>(
                m_SwapchainExtnet.height), // flipped so the image is not upside down (can be done differently probs)
            .minDepth = 0.0f,
            .maxDepth = 1.0f};

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor{
            .offset = {0, 0},
            .extent = m_SwapchainExtnet,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // vkCmdDraw(commandBuffer, m_Vertices.size(), 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
    }


    vkCmdEndRendering(commandBuffer);

    // Transition image layou for presentation
    TransitionImageLayout(m_SwapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, {},
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));
}
// some special shit right there
void HelloTriangleApplication::TransitionImageLayout(const VkImage& image, VkImageLayout oldLayout,
    VkImageLayout newLayout, VkAccessFlags2 srcAccessMask, VkAccessFlags2 destAccessMask,
    VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 destStageMask, VkImageAspectFlags imageAspectFlags) {

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = destStageMask,
        .dstAccessMask = destAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = imageAspectFlags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkDependencyInfo dependencyInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier};
    vkCmdPipelineBarrier2(m_CommandBuffers[m_FrameIndex], &dependencyInfo);
}
void HelloTriangleApplication::TransitionImageLayout(VkCommandBuffer commandBuffer, const VkImage& image,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = mipLevels,
                .layerCount = 1,
            },
    };
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("Unsupported layout transition");
    }
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
        0,          // dependency flags
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &barrier // image memory barriers
    );
}
void HelloTriangleApplication::CreateSyncObjects() {
    assert(m_PresentCompleteSemaphores.empty() && m_RenderFinishedSemaphores.empty() && m_InFlightFences.empty());

    VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (size_t i = 0; i < m_SwapchainImages.size(); ++i) {
        VkSemaphore renderFinishedSemaphore;
        VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
        m_RenderFinishedSemaphores.push_back(renderFinishedSemaphore);
    }

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT // important
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkSemaphore presentCompleteSemaphore;
        VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &presentCompleteSemaphore));
        m_PresentCompleteSemaphores.push_back(presentCompleteSemaphore);

        VkFence inFlightFence;
        VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &inFlightFence));
        m_InFlightFences.push_back(inFlightFence);
    }
}
uint32_t HelloTriangleApplication::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    // basically we just iterate through mam types and check if corresponding bit is set to 1 - meaning we have found it
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type!");
}
std::pair<VkBuffer, VkDeviceMemory> HelloTriangleApplication::CreateBuffer(
    VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    VK_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_Device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties),
    };

    VkDeviceMemory bufferMemory;
    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory));
    VK_CHECK(vkBindBufferMemory(m_Device, buffer, bufferMemory, 0));

    return {buffer, bufferMemory};
}
void HelloTriangleApplication::CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size) {

    VkCommandBuffer commandCopyBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion = {.srcOffset = 0, .dstOffset = 0, .size = size};
    vkCmdCopyBuffer(commandCopyBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(commandCopyBuffer);
}
void HelloTriangleApplication::CopyBufferToImage(
    VkCommandBuffer& commandBuffer, const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) {
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
void HelloTriangleApplication::UpdateUniformBuffers(uint32_t currentImage) {

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Camera and projection matrices
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        static_cast<float>(m_SwapchainExtnet.width) / static_cast<float>(m_SwapchainExtnet.height), 0.1f, 20.0f);

    for (auto& gameObject : GameObjects) {

        gameObject.Rotation.y += 0.01f;

        glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 model = gameObject.GetModelMatrix() * initialRotation;

        UniformBufferObject ubo{.Model = model, .View = view, .Proj = proj};

        memcpy(gameObject.UniformBuffersMapped[m_FrameIndex], &ubo, sizeof(ubo));
    }
}

VkImageView HelloTriangleApplication::CreateImageView(
    VkImage const& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    VkImageView imageView;
    VK_CHECK(vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));

    return imageView;
}
VkFormat HelloTriangleApplication::FindSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (const auto& format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
            (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            return format;
        }
    }
    throw std::runtime_error("Failed to find supported format");
}
VkFormat HelloTriangleApplication::FindDepthFormat() {
    return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
void HelloTriangleApplication::GenerateMipmaps(VkCommandBuffer& commandBuffer, VkImage& image, VkFormat format,
    int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

    if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("Texture image does not support linear blitting");
    }

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
    };

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {
            .srcSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i - 1, .layerCount = 1},
            .srcOffsets = {{0, 0, 0}, {mipWidth, mipHeight, 1}},
            .dstSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i, .layerCount = 1},
            .dstOffsets = {{0, 0, 0}, 1 < mipWidth ? mipWidth / 2 : 1, 1 < mipHeight ? mipHeight / 2 : 1, 1}};

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) {
            mipWidth /= 2;
        }
        if (mipHeight > 1) {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &barrier);
}
VkSampleCountFlagBits HelloTriangleApplication::GetMaxUsableSampleCount() {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    VkSampleCountFlags counts =
        properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}
void HelloTriangleApplication::DrawFrame() {
    VkResult fenceResult = vkWaitForFences(m_Device, 1, &m_InFlightFences[m_FrameIndex], VK_TRUE, UINT64_MAX);
    if (fenceResult != VK_SUCCESS) {
        throw std::runtime_error("failed to wait for fence");
    }

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        m_Device, m_Swapchain, UINT64_MAX, m_PresentCompleteSemaphores[m_FrameIndex], nullptr, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        assert(acquireResult == VK_TIMEOUT || acquireResult == VK_NOT_READY);
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    // only reset fence if we are submitting work; otherwise there can be deadlock
    VK_CHECK(vkResetFences(m_Device, 1, &m_InFlightFences[m_FrameIndex]));

    VK_CHECK(vkResetCommandBuffer(m_CommandBuffers[m_FrameIndex], 0));
    RecordCommandBuffer(imageIndex);

    UpdateUniformBuffers(m_FrameIndex);

    VkPipelineStageFlags waitDestinationStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_PresentCompleteSemaphores[m_FrameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_CommandBuffers[m_FrameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_RenderFinishedSemaphores[imageIndex],
    };

    VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIndex]));

    const VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_RenderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain,
        .pImageIndices = &imageIndex};

    VkResult presentResult = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

    if (presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapchain();
    } else {
        // there are no other codes that Success; on any error code, present KHR already throws
        assert(presentResult == VK_SUCCESS);
    }

    m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Loaded
void HelloTriangleApplication::vkCreateDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));

    if (!func) {
        throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT");
    }
    if (func(m_Instance, createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }
}

void HelloTriangleApplication::vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks* allocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (!func) {
        throw std::runtime_error("Failed to load vkDestroyDebugUtilsMessengerEXT");
    }
    func(instance, debugMessenger, allocator);
}
void HelloTriangleApplication::FremebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
}
