#pragma once
#include "../Core/VulkanContext.h"
#include "../Core/ImGuiManager.h"
#include "../Scene/Material.h"
#include "FrameData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <SDL.h>

class Renderer {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    void Initialize(VulkanContext& ctx, SDL_Window* window);
    void RecreatePipeline(VulkanContext& ctx);
    void DrawFrame(VulkanContext& ctx, const FrameData& frame);
    void Cleanup(VulkanContext& ctx);

private:
    // ---------- 初始化步骤 ----------
    void CreateBallMesh(VulkanContext& ctx);
    void CreateCrosshairMesh(VulkanContext& ctx);
    void CreateGroundMesh(VulkanContext& ctx);
    void CreateShadowMesh(VulkanContext& ctx);
    void CreateSkyMesh(VulkanContext& ctx);

    void CreateDescriptorSetLayout(VulkanContext& ctx);
    void CreateUniformBuffers(VulkanContext& ctx);
    void CreateDescriptorPool(VulkanContext& ctx);
    void CreateDescriptorSets(VulkanContext& ctx);
    void DestroyUniformResources(VulkanContext& ctx);

    void CreateGraphicsPipeline(VulkanContext& ctx);
    void CreateSkyPipeline(VulkanContext& ctx);

    void CreateSyncObjects(VulkanContext& ctx);
    void DestroySyncObjects(VulkanContext& ctx);

    void RecordCommandBuffer(VulkanContext& ctx, uint32_t imageIndex, const FrameData& frame);

    void UploadMeshData(VulkanContext& ctx, const void* vertexData, size_t vertexBytes,
                        VkBuffer& outBuffer, VkDeviceMemory& outMemory);

    // ---------- Vulkan 资源 ----------
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

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBufferMemories;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> imagesInFlight;

    uint32_t currentFrame = 0;
    SDL_Window* window = nullptr;
};