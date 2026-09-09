#include "ImGuiManager.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"

void ImGuiManager::Initialize(SDL_Window* window, VulkanContext& ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = ctx.instance;
    initInfo.PhysicalDevice = ctx.physicalDevice;
    initInfo.Device = ctx.device;
    initInfo.QueueFamily = ctx.graphicsQueueFamily;
    initInfo.Queue = ctx.graphicsQueue;
    initInfo.MinImageCount = (uint32_t)ctx.swapchainImages.size();
    initInfo.ImageCount = (uint32_t)ctx.swapchainImages.size();
    initInfo.DescriptorPoolSize = 100;
    initInfo.PipelineInfoMain.RenderPass = ctx.renderPass;
    initInfo.PipelineInfoMain.MSAASamples = ctx.GetMSAASamples();

    ImGui_ImplVulkan_Init(&initInfo);
}

void ImGuiManager::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::Render(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiManager::RecreatePipeline(VulkanContext& ctx) {
    ImGui_ImplVulkan_PipelineInfo pipelineInfo = {};
    pipelineInfo.RenderPass = ctx.renderPass;
    pipelineInfo.MSAASamples = ctx.GetMSAASamples();
    ImGui_ImplVulkan_CreateMainPipeline(&pipelineInfo);
}

void ImGuiManager::Cleanup(VulkanContext& ctx) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}