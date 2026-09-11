#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>

#include "NyxEngine/NyxEngine.h"
#include "NyxEngine/Core/VulkanContext.h"
#include "NyxEngine/Scene/Material.h"
#include "NyxEngine/Editor/EditorPanel.h"
#include "Game/Target/TargetManager.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>

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

int main(int argc, char* argv[]) {
    // ============ 平台初始化 ============
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    DisplaySettings displaySettings;
    displaySettings.width = 1280;
    displaySettings.height = 720;
    displaySettings.windowMode = WindowMode::Borderless;
    displaySettings.vsync = false;
    displaySettings.refreshRate = 0;
    displaySettings.msaaSamples = 4;

    SDL_Window* window = SDL_CreateWindow("Nyx",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        displaySettings.width, displaySettings.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // ============ 引擎实例 ============
    NyxEngine engine;
    engine.Initialize(window, displaySettings);

    // 引用（让下面的代码更短）
    InputSystem& input = engine.GetInput();
    Camera& camera = engine.GetCamera();
    EngineConfig& config = engine.GetConfig();

    // ============ Game 层 ============
    MaterialLibrary materials;
    materials.LoadDefaults();

    TargetManager targets;
    targets.currentMaterialIndex = materials.selectedIndex;
    targets.Spawn();

    // ============ 编辑器状态 ============
    bool editorMode = false;
    bool pendingDisplayChange = false;
    DisplaySettings pendingSettings = engine.GetVulkanContext().settings;
    auto displayModes = GetAvailableDisplayModes();

    bool windowMinimized = false;
    bool swapchainDestroyed = false;
    bool exclusiveFullscreenSuspended = false;
    bool running = true;
    SDL_Event event;
    uint32_t lastShotTime = 0;

    uint32_t frameCount = 0;
    uint32_t fpsTimer = 0;
    uint32_t currentFPS = 0;

    std::deque<float> frameTimes;
    const size_t maxFrameTimes = 600;
    uint64_t performanceFrequency = SDL_GetPerformanceFrequency();
    uint64_t frameStartCounter = 0;
    uint64_t lastFrameCounter = SDL_GetPerformanceCounter();

    // ============ EditorPanel ============
    EditorPanel editor;
    {
        EditorPanel::Context ctx;
        ctx.camera = &camera;
        ctx.materials = &materials;
        ctx.targets = &targets;
        ctx.config = &config;
        ctx.pendingSettings = &pendingSettings;
        ctx.displayModes = &displayModes;
        ctx.frameTimes = &frameTimes;
        ctx.pendingDisplayChange = &pendingDisplayChange;
        editor.Initialize(ctx);
    }

    // ============ 主循环 ============
    while (running) {
        frameStartCounter = SDL_GetPerformanceCounter();

        uint64_t currentFrameCounter = frameStartCounter;
        float dt = (float)((currentFrameCounter - lastFrameCounter) * 1000.0 / performanceFrequency) / 1000.0f;
        lastFrameCounter = currentFrameCounter;
        if (dt > 0.1f) dt = 0.1f;

        engine.BeginFrame();

        // ---------- 事件 ----------
        while (SDL_PollEvent(&event)) {
            input.ProcessEvent(event);

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    windowMinimized = true;
                } else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    windowMinimized = false;
                    if (swapchainDestroyed) pendingDisplayChange = true;
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    if (engine.GetVulkanContext().settings.windowMode == WindowMode::ExclusiveFullscreen) {
                        exclusiveFullscreenSuspended = true;
                    }
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    windowMinimized = false;
                    if (swapchainDestroyed) pendingDisplayChange = true;
                    if (exclusiveFullscreenSuspended) {
                        pendingSettings = engine.GetVulkanContext().settings;
                        pendingDisplayChange = true;
                        exclusiveFullscreenSuspended = false;
                    }
                }
            }

            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        // ---------- 输入业务逻辑 ----------
        if (input.ShouldQuit()) running = false;
        if (input.WasKeyPressed(SDL_SCANCODE_ESCAPE)) running = false;
        if (input.WasKeyPressed(SDL_SCANCODE_F1)) {
            editorMode = !editorMode;
            input.SetMouseCaptured(!editorMode);
        }

        if (!editorMode) {
            camera.ProcessMouseDelta(input.GetMouseDeltaX(), input.GetMouseDeltaY());
        }

        // ---------- 最小化分支 ----------
        if (windowMinimized) {
            if (!swapchainDestroyed) {
                vkDeviceWaitIdle(engine.GetVulkanContext().device);
                engine.GetVulkanContext().DestroySwapchainResources();
                swapchainDestroyed = true;
            }
            SDL_Delay(50);
            engine.EndFrame();
            continue;
        }

        // ---------- 显示设置变更 ----------
        if (pendingDisplayChange) {
            VulkanContext& vk = engine.GetVulkanContext();
            vk.settings = pendingSettings;

            bool useExclusive = (vk.settings.windowMode == WindowMode::ExclusiveFullscreen);
            SDL_SetWindowFullscreen(window, 0);

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
                    if (SDL_SetWindowDisplayMode(window, &closestMode) != 0) {
                        useExclusive = false;
                        vk.settings.windowMode = WindowMode::Borderless;
                    }
                }
            }

            if (vk.settings.windowMode == WindowMode::Windowed) {
                SDL_SetWindowFullscreen(window, 0);
                SDL_SetWindowSize(window, vk.settings.width, vk.settings.height);
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            } else if (vk.settings.windowMode == WindowMode::Borderless) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                SDL_SetWindowPosition(window, 0, 0);
                SDL_SetWindowSize(window, vk.settings.width, vk.settings.height);
            }

            vk.RecreateSwapchain(window);
            engine.GetRenderer().RecreatePipeline(vk);
            engine.GetImGuiManager().RecreatePipeline(vk);
            swapchainDestroyed = false;

            pendingDisplayChange = false;
        }

        // ---------- Game 逻辑 ----------
        uint32_t currentTime = SDL_GetTicks();

        if (!editorMode) {
            if (input.IsKeyDown(SDL_SCANCODE_SPACE) && currentTime - lastShotTime > 200) {
                targets.Shoot(camera.position, camera.GetDirection());
                lastShotTime = currentTime;
            }
            if (input.IsKeyDown(SDL_SCANCODE_R)) {
                targets.Spawn();
                targets.currentMaterialIndex = materials.selectedIndex;
            }
            targets.Update(dt);
        }

        // ---------- FPS 统计 ----------
        frameCount++;
        uint32_t now = SDL_GetTicks();
        if (now - fpsTimer >= 500) {
            currentFPS = frameCount * 2;
            frameCount = 0;
            fpsTimer = now;
        }

        SDL_DisplayMode actualMode;
        SDL_GetWindowDisplayMode(window, &actualMode);
        std::string title = "Nyx | Score: " + std::to_string(targets.score) +
                            " | FPS: " + std::to_string(currentFPS) +
                            " | " + std::to_string(actualMode.w) + "x" + std::to_string(actualMode.h) +
                            "@" + std::to_string(actualMode.refresh_rate) + "Hz";
        SDL_SetWindowTitle(window, title.c_str());

        // ---------- 光照 ----------
        LightingData lightingData = engine.GetLighting().Update();

        // ---------- 编辑器 ----------
        engine.GetImGuiManager().NewFrame();
        if (editorMode) {
            editor.Draw();
        }

        // ---------- 渲染 ----------
        std::vector<Material> targetMaterials;
        for (auto& t : targets.targets) {
            if (t.alive) {
                int idx = t.materialIndex;
                if (idx < 0 || idx >= (int)materials.materials.size()) idx = 0;
                targetMaterials.push_back(materials.materials[idx]);
            }
        }

        VulkanContext& vk = engine.GetVulkanContext();
        float aspect = (float)vk.swapchainExtent.width / (float)vk.swapchainExtent.height;
        engine.GetRenderer().DrawFrame(vk,
                   camera.GetViewMatrix(), camera.GetProjectionMatrix(aspect),
                   camera.position,
                   targets.GetAlivePositions(), targets.GetAliveScales(), targetMaterials,
                   lightingData.lightDir, lightingData.lightColor, lightingData.lightIntensity,
                   lightingData.ambientColor,
                   lightingData.skyTopColor, lightingData.skyBottomColor,
                   &engine.GetImGuiManager());

        // ---------- 帧计时 ----------
        uint64_t frameEndCounter = SDL_GetPerformanceCounter();
        float frameTimeMs = (float)((frameEndCounter - frameStartCounter) * 1000.0 / performanceFrequency);
        frameTimes.push_back(frameTimeMs);
        if (frameTimes.size() > maxFrameTimes) frameTimes.pop_front();

        engine.EndFrame();
    }

    // ============ 退出 ============
    engine.Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}