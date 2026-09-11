#pragma once
#include <SDL.h>
#include <vector>
#include <cstdint>

#include "NyxEngine/Scene/Material.h"
#include "NyxEngine/Editor/EditorPanel.h"
#include "NyxEngine/Core/FrameTimeHistory.h"
#include "NyxEngine/Core/VulkanContext.h"
#include "Game/Target/TargetManager.h"

class NyxEngine;

// ============ MyGame ============
//
// 训练器游戏逻辑的容器。
//
// 职责：
//   - 持有游戏对象（Materials, Targets, Editor）
//   - 持有游戏状态（editorMode, running, window 状态）
//   - 驱动主循环（Run 内部有 while）
//   - 处理 SDL 事件、显示设置变更、标题更新
//
// 不职责：
//   - 引擎初始化（NyxEngine 负责）
//   - 渲染管线（Renderer 负责）
//   - 光照计算（LightingSystem 负责）

class MyGame {
public:
    void Initialize(NyxEngine& engine, SDL_Window* window);
    void Shutdown();
    void Run();

private:
    void ProcessEvent(const SDL_Event& e);
    void Update(float dt);
    void Render();
    void HandleDisplayChange();
    void UpdateTitle();

    NyxEngine* engine_ = nullptr;
    SDL_Window* window_ = nullptr;

    // ---------- 游戏对象 ----------
    MaterialLibrary materials_;
    TargetManager targets_;
    EditorPanel editor_;

    // ---------- 编辑器状态 ----------
    bool editorMode_ = false;
    bool pendingDisplayChange_ = false;
    DisplaySettings pendingSettings_;
    std::vector<SDL_DisplayMode> displayModes_;

    // ---------- 窗口状态 ----------
    bool windowMinimized_ = false;
    bool swapchainDestroyed_ = false;
    bool exclusiveFullscreenSuspended_ = false;
    bool running_ = true;

    // ---------- 游戏状态 ----------
    uint32_t lastShotTime_ = 0;

    // ---------- 性能统计 ----------
    uint32_t frameCount_ = 0;
    uint32_t fpsTimer_ = 0;
    uint32_t currentFPS_ = 0;
    FrameTimeHistory frameTimes_{600};
    uint64_t performanceFrequency_ = 0;
    uint64_t lastFrameCounter_ = 0;

    // ---------- 标题缓存（A1 + A2）----------
    int lastTitleScore_ = -1;
    uint32_t lastTitleFPS_ = 0xFFFFFFFFu;
    SDL_DisplayMode cachedDisplayMode_ = {};
    bool displayModeDirty_ = true;
};