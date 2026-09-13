#pragma once
#include "Core/EngineConfig.h"
#include "Core/VulkanContext.h"
#include "Core/ImGuiManager.h"
#include "Core/InputSystem.h"
#include "Render/Renderer.h"
#include "Render/LightingSystem.h"
#include "Scene/Camera.h"
#include "Audio/AudioClock.h"
#include "World/WorldState.h"
#include <SDL.h>

// ============ NyxEngine 实例 ============
//
// 引擎拥有所有子系统。
// main 只负责：SDL 初始化、窗口创建、Game 逻辑、主循环。
//
// 生命周期：
//   Initialize(window, settings) -> 创建并初始化所有子系统
//   主循环: BeginFrame(dt) -> [game update] -> [render] -> EndFrame()
//   Shutdown() -> 逆序销毁

class NyxEngine {
public:
    void Initialize(SDL_Window* window, const DisplaySettings& settings);
    void Shutdown();

    void BeginFrame(float dt);
    void EndFrame();

    // ---------- 访问器 ----------
    EngineConfig& GetConfig()           { return config_; }
    VulkanContext& GetVulkanContext()   { return vk_; }
    ImGuiManager& GetImGuiManager()     { return imgui_; }
    Renderer& GetRenderer()             { return renderer_; }
    InputSystem& GetInput()             { return input_; }
    LightingSystem& GetLighting()       { return lighting_; }
    Camera& GetCamera()                 { return camera_; }
    AudioClock& GetAudioClock()         { return audioClock_; }
    WorldState& GetWorldState()         { return world_; }

private:
    EngineConfig config_;
    VulkanContext vk_;
    ImGuiManager imgui_;
    Renderer renderer_;
    InputSystem input_;
    LightingSystem lighting_;
    Camera camera_;
    AudioClock audioClock_;
    WorldState world_;
};