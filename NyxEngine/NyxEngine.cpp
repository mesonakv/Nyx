#include "NyxEngine.h"

void NyxEngine::Initialize(SDL_Window* window, const DisplaySettings& settings) {
    // Vulkan 上下文（最先，其他都依赖它）
    vk_.Initialize(window, settings);

    // ImGui
    imgui_.Initialize(window, vk_);

    // Renderer
    renderer_.Initialize(vk_, window);

    // Input
    input_.Initialize(window);
    input_.SetMouseCaptured(true);

    // Lighting
    lighting_.Initialize(config_);

    // Camera 初始化（从 config 读初值）
    camera_.position = config_.camera.initialPosition;
    camera_.pitch = config_.camera.initialPitch;
    camera_.yaw = config_.camera.initialYaw;
}

void NyxEngine::Shutdown() {
    input_.Shutdown();
    renderer_.Cleanup(vk_);
    imgui_.Cleanup(vk_);
    vk_.Cleanup();
}

void NyxEngine::BeginFrame() {
    input_.BeginFrame();
}

void NyxEngine::EndFrame() {
    input_.EndFrame();
}