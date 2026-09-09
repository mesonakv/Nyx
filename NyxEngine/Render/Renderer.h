#pragma once
#include "../Core/VulkanContext.h"
#include "../Core/ImGuiManager.h"
#include "../Scene/Material.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Renderer {
public:
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // 天空
    VkPipeline skyPipeline = VK_NULL_HANDLE;
    VkPipelineLayout skyPipelineLayout = VK_NULL_HANDLE;
    VkBuffer skyVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory skyVertexBufferMemory = VK_NULL_HANDLE;

    // 球体
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;

    // 准星
    VkBuffer crosshairVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory crosshairVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t crosshairVertexCount = 0;

    // 地面
    VkBuffer groundVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory groundVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t groundVertexCount = 0;

    // 阴影圆片
    VkBuffer shadowVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory shadowVertexBufferMemory = VK_NULL_HANDLE;
    uint32_t shadowVertexCount = 0;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    void Initialize(VulkanContext& ctx);
    void CreateBallMesh(VulkanContext& ctx);
    void CreateCrosshairMesh(VulkanContext& ctx);
    void CreateGroundMesh(VulkanContext& ctx);
    void CreateShadowMesh(VulkanContext& ctx);
    void CreateSkyMesh(VulkanContext& ctx);         // 新增
    void CreateGraphicsPipeline(VulkanContext& ctx);
    void CreateSkyPipeline(VulkanContext& ctx);     // 新增
    void RecreatePipeline(VulkanContext& ctx);
    void RecordCommandBuffers(VulkanContext& ctx, glm::mat4 view, glm::mat4 proj,
                              const std::vector<glm::vec3>& targetPositions,
                              const std::vector<float>& targetScales,
                              const std::vector<Material>& targetMaterials,
                              const glm::vec3& lightDir,
                              const glm::vec3& lightColor,
                              float lightIntensity,
                              const glm::vec4& skyTopColor,      // 新增
                              const glm::vec4& skyBottomColor,   // 新增
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
