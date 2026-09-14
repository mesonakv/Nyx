#include "NyxEngine.h"

void NyxEngine::Initialize(SDL_Window* window, const DisplaySettings& settings) {
    vk_.Initialize(window, settings);
    imgui_.Initialize(window, vk_);
    renderer_.Initialize(vk_, window);

    input_.Initialize(window);
    input_.SetMouseCaptured(true);

    inputMap_.Initialize();

    lighting_.Initialize(config_);

    camera_.yaw = config_.camera.initialYaw;
    camera_.pitch = config_.camera.initialPitch;
    camera_.sensitivity = config_.camera.sensitivity;
    camera_.fov = config_.camera.fov;

    world_.player.position = config_.player.initialPosition;
    world_.player.eyeHeight = config_.player.eyeHeight;
}

void NyxEngine::Shutdown() {
    input_.Shutdown();
    renderer_.Cleanup(vk_);
    imgui_.Cleanup(vk_);
    vk_.Cleanup();
}

void NyxEngine::BeginFrame(float dt) {
    input_.BeginFrame();
    audioClock_.Tick(dt);
}

void NyxEngine::EndFrame() {
    input_.EndFrame();
}