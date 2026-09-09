#pragma once
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>

enum class WindowMode {
    Windowed,
    Borderless,
    ExclusiveFullscreen
};

struct DisplaySettings {
    int width = 1280;
    int height = 720;
    int refreshRate = 0;          // 0 = 系统默认
    WindowMode windowMode = WindowMode::Windowed;
    bool vsync = false;
    int msaaSamples = 4;          // 1,2,4,8
};

class VulkanContext {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // 深度缓冲
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    // MSAA 颜色附件
    VkImage msaaColorImage = VK_NULL_HANDLE;
    VkDeviceMemory msaaColorImageMemory = VK_NULL_HANDLE;
    VkImageView msaaColorImageView = VK_NULL_HANDLE;

    int graphicsQueueFamily = -1;
    int presentQueueFamily = -1;

    DisplaySettings settings;

    void Initialize(SDL_Window* window, const DisplaySettings& initialSettings);
    void Cleanup();

    void UpdateWindowMode(SDL_Window* window, WindowMode mode);
    void RecreateSwapchain(SDL_Window* window);
    void DestroySwapchainResources();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkSampleCountFlagBits GetMSAASamples() const;
};