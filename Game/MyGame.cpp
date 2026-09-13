#include "MyGame.h"

#include <SDL.h>
#include <cstdio>
#include <memory>

#include "NyxEngine/NyxEngine.h"
#include "NyxEngine/Core/Logger.h"
#include "NyxEngine/Core/Platform.h"
#include "NyxEngine/Core/InputSystem.h"
#include "NyxEngine/Core/ImGuiManager.h"
#include "NyxEngine/Render/Renderer.h"
#include "NyxEngine/Render/FrameData.h"
#include "NyxEngine/Scene/Camera.h"
#include "NyxEngine/Render/DebugDraw.h"
#include "NyxEngine/Physics/PhysicsWorld.h"
#include "NyxEngine/Physics/Shapes/BoxShape.h"
#include "NyxEngine/Physics/Shapes/CapsuleShape.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>

namespace {

std::vector<SDL_DisplayMode> GetAvailableDisplayModes() {
    std::vector<SDL_DisplayMode> modes;
    int displayCount = SDL_GetNumVideoDisplays();
    if (displayCount < 1) return modes;

    int modeCount = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < modeCount; i++) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0) {
            modes.push_back(mode);
        }
    }
    return modes;
}

} // anonymous namespace

// ============ 生命周期 ============

void MyGame::Initialize(NyxEngine& engine, SDL_Window* window) {
    engine_ = &engine;
    window_ = window;

    performanceFrequency_ = SDL_GetPerformanceFrequency();
    lastFrameCounter_ = SDL_GetPerformanceCounter();

    // 材质
    materials_.LoadDefaults();

    // 目标
    targets_.currentMaterialIndex = materials_.selectedIndex;
    targets_.Spawn();

    // 显示模式
    displayModes_ = GetAvailableDisplayModes();
    pendingSettings_ = engine.GetVulkanContext().settings;

    // ---------- 物理世界初始化 ----------
    {
        PhysicsWorld& physics = engine.GetWorldState().physics;
        Player& player = engine.GetWorldState().player;

        // 地面
        CollisionFilter groundFilter = CollisionFilter::Make(
            PhysicsLayer::StaticGeo,
            PhysicsLayer::Player | PhysicsLayer::Projectile | PhysicsLayer::Debris);

        Transform groundT;
        groundT.position = glm::vec3(0.0f, -2.1f, 0.0f);

        physics.CreateShape(
            std::make_unique<BoxShape>(glm::vec3(30.0f, 0.1f, 30.0f)),
            groundT,
            groundFilter);

        // 玩家胶囊
        CollisionFilter playerFilter = CollisionFilter::Make(
            PhysicsLayer::Player,
            PhysicsLayer::StaticGeo | PhysicsLayer::Target | PhysicsLayer::Boss);

        Transform playerT;
        playerT.position = player.GetCapsuleCenter();

        player.physicsBody = physics.CreateShape(
            std::make_unique<CapsuleShape>(Player::kCapsuleRadius, Player::kCapsuleHalfHeight),
            playerT,
            playerFilter);

        NYX_LOG_INFO("Physics: ground + player capsule created");
    }

    // 编辑器面板
    EditorPanel::Context ctx;
    ctx.camera = &engine.GetCamera();
    ctx.materials = &materials_;
    ctx.targets = &targets_;
    ctx.config = &engine.GetConfig();
    ctx.pendingSettings = &pendingSettings_;
    ctx.displayModes = &displayModes_;
    ctx.frameTimes = &frameTimes_;
    ctx.pendingDisplayChange = &pendingDisplayChange_;
    ctx.input = &engine.GetInput();
    editor_.Initialize(ctx);

    NYX_LOG_INFO("MyGame initialized");
}

void MyGame::Shutdown() {
    NYX_LOG_INFO("MyGame shutdown");
}

// ============ 主循环 ============

void MyGame::Run() {
    while (running_) {
        uint64_t frameStartCounter = SDL_GetPerformanceCounter();

        uint64_t currentFrameCounter = frameStartCounter;
        float dt = (float)((currentFrameCounter - lastFrameCounter_) * 1000.0 / performanceFrequency_) / 1000.0f;
        lastFrameCounter_ = currentFrameCounter;
        if (dt > 0.1f) dt = 0.1f;

        engine_->BeginFrame(dt);

        // ---------- 事件 ----------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            engine_->GetInput().ProcessEvent(event);
            ProcessEvent(event);
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        // ---------- 更新 ----------
        Update(dt);

        // ---------- 渲染 ----------
        Render();

        // ---------- 帧计时 ----------
        uint64_t frameEndCounter = SDL_GetPerformanceCounter();
        float frameTimeMs = (float)((frameEndCounter - frameStartCounter) * 1000.0 / performanceFrequency_);
        frameTimes_.push_back(frameTimeMs);

        engine_->EndFrame();
    }
}

// ============ 事件处理 ============

void MyGame::ProcessEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        running_ = false;
        return;
    }

    if (e.type != SDL_WINDOWEVENT) return;

    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
        e.window.event == SDL_WINDOWEVENT_RESIZED ||
        e.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
        e.window.event == SDL_WINDOWEVENT_RESTORED
#if SDL_VERSION_ATLEAST(2, 0, 18)
        || e.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED
#endif
        ) {
        displayModeDirty_ = true;
    }

    if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
        windowMinimized_ = true;
    } else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
        windowMinimized_ = false;
        if (swapchainDestroyed_) pendingDisplayChange_ = true;
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        if (engine_->GetVulkanContext().settings.windowMode == WindowMode::ExclusiveFullscreen) {
            exclusiveFullscreenSuspended_ = true;
        }
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        windowMinimized_ = false;
        if (swapchainDestroyed_) pendingDisplayChange_ = true;
        if (exclusiveFullscreenSuspended_) {
            pendingSettings_ = engine_->GetVulkanContext().settings;
            pendingDisplayChange_ = true;
            exclusiveFullscreenSuspended_ = false;
        }
    }
}

// ============ 更新 ============

void MyGame::Update(float dt) {
    InputSystem& input = engine_->GetInput();
    Camera& camera = engine_->GetCamera();
    Player& player = engine_->GetWorldState().player;

    // ---------- 输入业务逻辑 ----------
    if (input.ShouldQuit()) running_ = false;
    if (input.WasKeyPressed(SDL_SCANCODE_ESCAPE)) running_ = false;
    if (input.WasKeyPressed(SDL_SCANCODE_F1)) {
        editorMode_ = !editorMode_;
        input.SetMouseCaptured(!editorMode_);
    }

    if (!editorMode_) {
        camera.ProcessMouseDelta(input.GetMouseDeltaX(), input.GetMouseDeltaY());
    }

    // ---------- 最小化分支 ----------
    if (windowMinimized_) {
        if (!swapchainDestroyed_) {
            vkDeviceWaitIdle(engine_->GetVulkanContext().device);
            engine_->GetVulkanContext().DestroySwapchainResources();
            swapchainDestroyed_ = true;
        }
        SDL_Delay(50);
        return;
    }

    // ---------- 显示设置变更 ----------
    if (pendingDisplayChange_) {
        HandleDisplayChange();
    }

    // ---------- 玩家更新 ----------
    // 从 WASD 生成移动方向（相机 yaw 空间的水平方向）
    glm::vec3 moveDir(0.0f);
    if (!editorMode_) {
        float yaw = camera.yaw;
        glm::vec3 forward(sin(yaw), 0.0f, -cos(yaw));
        glm::vec3 right(cos(yaw), 0.0f, sin(yaw));

        if (input.IsKeyDown(SDL_SCANCODE_W)) moveDir += forward;
        if (input.IsKeyDown(SDL_SCANCODE_S)) moveDir -= forward;
        if (input.IsKeyDown(SDL_SCANCODE_D)) moveDir += right;
        if (input.IsKeyDown(SDL_SCANCODE_A)) moveDir -= right;
    }

    bool jumpPressed = !editorMode_ && input.WasKeyPressed(SDL_SCANCODE_SPACE);

    player.Update(dt, moveDir, jumpPressed, camera.yaw, engine_->GetWorldState().physics);

    // ---------- 游戏逻辑 ----------
    uint32_t currentTime = SDL_GetTicks();

    if (!editorMode_) {
        if (input.IsKeyDown(SDL_SCANCODE_SPACE) && currentTime - lastShotTime_ > 200) {
            targets_.Shoot(player.GetEyePosition(), camera.GetDirection());
            lastShotTime_ = currentTime;
        }
        if (input.IsKeyDown(SDL_SCANCODE_R)) {
            targets_.Spawn();
            targets_.currentMaterialIndex = materials_.selectedIndex;
        }
        targets_.Update(dt);
    }

    // ---------- FPS 统计 ----------
    frameCount_++;
    uint32_t now = SDL_GetTicks();
    if (now - fpsTimer_ >= 500) {
        currentFPS_ = frameCount_ * 2;
        frameCount_ = 0;
        fpsTimer_ = now;
    }

    // ---------- 标题 ----------
    UpdateTitle();
}

// ============ 显示设置变更 ============

void MyGame::HandleDisplayChange() {
    VulkanContext& vk = engine_->GetVulkanContext();
    vk.settings = pendingSettings_;

    bool useExclusive = (vk.settings.windowMode == WindowMode::ExclusiveFullscreen);
    SDL_SetWindowFullscreen(window_, 0);

    if (useExclusive) {
        SDL_DisplayMode targetMode;
        targetMode.format = SDL_PIXELFORMAT_UNKNOWN;
        targetMode.w = vk.settings.width;
        targetMode.h = vk.settings.height;
        targetMode.refresh_rate = (vk.settings.refreshRate == 0) ? 0 : vk.settings.refreshRate;

        SDL_DisplayMode closestMode;
        if (SDL_GetClosestDisplayMode(0, &targetMode, &closestMode) == nullptr) {
            useExclusive = false;
            vk.settings.windowMode = WindowMode::Borderless;
        } else {
            if (SDL_SetWindowDisplayMode(window_, &closestMode) != 0) {
                useExclusive = false;
                vk.settings.windowMode = WindowMode::Borderless;
            }
        }
    }

    if (vk.settings.windowMode == WindowMode::Windowed) {
        SDL_SetWindowFullscreen(window_, 0);
        SDL_SetWindowSize(window_, vk.settings.width, vk.settings.height);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    } else if (vk.settings.windowMode == WindowMode::Borderless) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN);
        SDL_SetWindowPosition(window_, 0, 0);
        SDL_SetWindowSize(window_, vk.settings.width, vk.settings.height);
    }

    vk.RecreateSwapchain(window_);
    engine_->GetRenderer().RecreatePipeline(vk);
    engine_->GetImGuiManager().RecreatePipeline(vk);
    swapchainDestroyed_ = false;

    displayModeDirty_ = true;

    pendingDisplayChange_ = false;
}

// ============ 标题更新 ============

void MyGame::UpdateTitle() {
    bool modeChanged = false;
    if (displayModeDirty_) {
        SDL_DisplayMode newMode;
        SDL_GetWindowDisplayMode(window_, &newMode);
        if (newMode.w != cachedDisplayMode_.w ||
            newMode.h != cachedDisplayMode_.h ||
            newMode.refresh_rate != cachedDisplayMode_.refresh_rate) {
            cachedDisplayMode_ = newMode;
            modeChanged = true;
        }
        displayModeDirty_ = false;
    }

    if (targets_.score != lastTitleScore_ || currentFPS_ != lastTitleFPS_ || modeChanged) {
        char titleBuf[256];
        snprintf(titleBuf, sizeof(titleBuf), "Nyx | Score: %d | FPS: %u | %dx%d@%dHz",
                 targets_.score, currentFPS_,
                 cachedDisplayMode_.w, cachedDisplayMode_.h, cachedDisplayMode_.refresh_rate);
        SDL_SetWindowTitle(window_, titleBuf);
        lastTitleScore_ = targets_.score;
        lastTitleFPS_ = currentFPS_;
    }
}

// ============ 渲染 ============

void MyGame::Render() {
    // ---------- 光照 ----------
    LightingData lightingData = engine_->GetLighting().Update();

    // ---------- 编辑器 ----------
    engine_->GetImGuiManager().NewFrame();
    if (editorMode_) {
        editor_.Draw();
    }

    // ---------- 帧数据 ----------
    VulkanContext& vk = engine_->GetVulkanContext();
    Camera& camera = engine_->GetCamera();
    Player& player = engine_->GetWorldState().player;
    float aspect = (float)vk.swapchainExtent.width / (float)vk.swapchainExtent.height;

    const auto& alivePositions = targets_.GetAlivePositions();
    const auto& aliveScales = targets_.GetAliveScales();
    const auto& aliveMaterialIndices = targets_.GetAliveMaterialIndices();

    glm::vec3 eyePos = player.GetEyePosition();

    FrameData frameData;
    frameData.view = camera.GetViewMatrix(eyePos);
    frameData.proj = camera.GetProjectionMatrix(aspect);
    frameData.cameraPos = eyePos;
    frameData.targetPositions = &alivePositions;
    frameData.targetScales = &aliveScales;
    frameData.targetMaterialIndices = &aliveMaterialIndices;
    frameData.materialLibrary = &materials_;
    frameData.lighting = lightingData;
    frameData.imgui = &engine_->GetImGuiManager();

    // ---------- Debug Draw：物理世界可视化 ----------
    {
        DebugDraw& dbg = engine_->GetRenderer().GetDebugDraw();
        PhysicsWorld& physics = engine_->GetWorldState().physics;
        dbg.Begin();

        // 画出物理世界中的所有形状
        // 简化：只画已知的几个（地面 + 玩家胶囊），
        // 等 PhysicsWorld 有 ForEachShape 再加
        dbg.Box(glm::vec3(0.0f, -2.1f, 0.0f),
                glm::vec3(30.0f, 0.1f, 30.0f),
                glm::quat(1, 0, 0, 0),
                glm::vec3(0.4f, 0.4f, 0.4f));

        // 玩家胶囊
        glm::vec3 capsuleCenter = player.GetCapsuleCenter();
        glm::vec3 p0 = capsuleCenter - glm::vec3(0, Player::kCapsuleHalfHeight, 0);
        glm::vec3 p1 = capsuleCenter + glm::vec3(0, Player::kCapsuleHalfHeight, 0);
        dbg.Capsule(p0, p1, Player::kCapsuleRadius, glm::vec3(0.3f, 0.9f, 0.3f));

        // 黄射线：从眼睛往前
        dbg.Ray(eyePos, camera.GetDirection(), 10.0f, glm::vec3(1, 1, 0));
    }
    // ---------- Debug Draw 结束 ----------

    engine_->GetRenderer().DrawFrame(vk, frameData);
}