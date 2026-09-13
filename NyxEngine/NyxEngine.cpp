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
    // 注意：Camera 不再持有 position，位置由 Game 层的 Player 提供
    camera_.yaw = config_.camera.initialYaw;
    camera_.pitch = config_.camera.initialPitch;
    camera_.sensitivity = config_.camera.sensitivity;
    camera_.fov = config_.camera.fov;
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