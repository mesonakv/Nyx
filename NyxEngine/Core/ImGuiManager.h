#pragma once
#include "VulkanContext.h"
#include <SDL.h>

class ImGuiManager {
public:
    void Initialize(SDL_Window* window, VulkanContext& ctx);
    void NewFrame();
    void Render(VkCommandBuffer cmd);
    void RecreatePipeline(VulkanContext& ctx);
    void Cleanup(VulkanContext& ctx);
};