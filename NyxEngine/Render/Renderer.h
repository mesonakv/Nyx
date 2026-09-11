#pragma once
#include "../Core/VulkanContext.h"
#include "../Core/ImGuiManager.h"
#include "../Scene/Material.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <SDL.h>

class Renderer {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkPipeline skyPipeline = VK_NULL_HANDLE;
    VkPipelineLayout skyPipelineLayout = VK_NULL_HANDLE;
    VkBuffer skyVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory skyVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;

    VkBuffer crosshairVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory crosshairVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t crosshairVertexCount = 0;

    VkBuffer groundVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory groundVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t groundVertexCount = 0;

    VkBuffer shadowVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory shadowVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t shadowVertexCount = 0;

    // 每帧一个（大小 MAX_FRAMES_IN_FLIGHT）
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkFence> inFlightFences;

    // 每张 swapchain image 一个
    std::vector<VkSemaphore> renderFinishedSemaphores;

    // 追踪每张 swapchain image 当前被哪个 inFlightFence 占用
    std::vector<VkFence> imagesInFlight;

    uint32_t currentFrame = 0;

    // 用于 swapchain 重建
    SDL_Window* window = nullptr;

    void Initialize(VulkanContext& ctx, SDL_Window* window);
    void CreateBallMesh(VulkanContext& ctx);
    void CreateCrosshairMesh(VulkanContext& ctx);
    void CreateGroundMesh(VulkanContext& ctx);
    void CreateShadowMesh(VulkanContext& ctx);
    void CreateSkyMesh(VulkanContext& ctx);
    void CreateGraphicsPipeline(VulkanContext& ctx);
    void CreateSkyPipeline(VulkanContext& ctx);
    void RecreatePipeline(VulkanContext& ctx);

    void CreateSyncObjects(VulkanContext& ctx);
    void DestroySyncObjects(VulkanContext& ctx);

    void RecordCommandBuffer(VulkanContext& ctx, uint32_t imageIndex,
                             glm::mat4 view, glm::mat4 proj,
                             const std::vector<glm::vec3>& targetPositions,
                             const std::vector<float>& targetScales,
                             const std::vector<Material>& targetMaterials,
                             const glm::vec3& lightDir,
                             const glm::vec3& lightColor,
                             float lightIntensity,
                             const glm::vec4& skyTopColor,
                             const glm::vec4& skyBottomColor,
                             ImGuiManager* imgui);

    void DrawFrame(VulkanContext& ctx, glm::mat4 view, glm::mat4 proj,
                   const std::vector<glm::vec3>& targetPositions,
                   const std::vector<float>& targetScales,
                   const std::vector<Material>& targetMaterials,
                   const glm::vec3& lightDir,
                   const glm::vec3& lightColor,
                   float lightIntensity,
                   const glm::vec4& skyTopColor,
                   const glm::vec4& skyBottomColor,
                   ImGuiManager* imgui);

    void Cleanup(VulkanContext& ctx);
};