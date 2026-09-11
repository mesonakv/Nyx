#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <atomic>
#include <chrono>

#include "NyxEngine/NyxEngine.h"
#include "NyxEngine/Core/Logger.h"
#include "NyxEngine/Core/Memory.h"
#include "NyxEngine/Core/Platform.h"
#include "NyxEngine/Core/EventBus.h"
#include "NyxEngine/Core/JobSystem.h"
#include "NyxEngine/Core/NyxMath.h"
#include "NyxEngine/Core/FileSystem.h"
#include "NyxEngine/Core/VulkanContext.h"
#include "NyxEngine/Scene/Material.h"
#include "NyxEngine/Editor/EditorPanel.h"
#include "Game/Target/TargetManager.h"
#include "NyxEngine/Core/JsonValue.h"

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

    // ============ Logger + Memory + Platform 初始化 ============
    LoggerConfig logConfig;
#ifdef _DEBUG
    logConfig.minLevel = LogLevel::Trace;
#else
    logConfig.minLevel = LogLevel::Info;
#endif
    logConfig.toConsole = true;
    logConfig.toFile = false;
    logConfig.toDebugOutput = true;
    Logger::Initialize(logConfig);

    Memory::Initialize();
    Platform::Initialize();

    NYX_LOG_INFO("NyxEngine starting...");
    NYX_LOG_INFO("Executable dir: %s", FileSystem::GetExecutableDir().c_str());
    NYX_LOG_INFO("Working dir:    %s", FileSystem::GetWorkingDir().c_str());

    // ============ EventBus 自测 ============
    {
        struct TestEventA { int value; };
        struct TestEventB { const char* name; };

        auto h1 = EventBus::Subscribe<TestEventA>([](const TestEventA& e) {
            NYX_LOG_INFO("  [A ] received: value = %d", e.value);
        });
        auto h2 = EventBus::Subscribe<TestEventA>([](const TestEventA& e) {
            NYX_LOG_INFO("  [A2] received: value = %d", e.value);
        });
        auto h3 = EventBus::Subscribe<TestEventB>([](const TestEventB& e) {
            NYX_LOG_INFO("  [B ] received: name = %s", e.name);
        });

        NYX_LOG_INFO("EventBus test: emit A{1}");
        EventBus::Emit(TestEventA{1});

        NYX_LOG_INFO("EventBus test: unsubscribe handler 2");
        EventBus::Unsubscribe(h2);

        NYX_LOG_INFO("EventBus test: emit A{2}");
        EventBus::Emit(TestEventA{2});

        NYX_LOG_INFO("EventBus test: emit B{\"hello\"}");
        EventBus::Emit(TestEventB{"hello"});

        NYX_LOG_INFO("EventBus test: subscriptions = %zu", EventBus::GetSubscriptionCount());

        EventBus::Unsubscribe(h1);
        EventBus::Unsubscribe(h3);
    }
	
#include "NyxEngine/Core/JsonValue.h"

// ... 现有 include ...

    // ============ JsonValue 自测 ============
    {
        NYX_LOG_INFO("JSON test: build");
        JsonValue root;
        root["name"] = "NyxEngine";
        root["version"] = 1;
        root["debug"] = false;
        root["pi"] = 3.14159f;

        root["lighting"]["timeOfDay"] = 0.5f;
        root["lighting"]["ambientStrength"] = 0.35f;

        root["tags"].Push("engine");
        root["tags"].Push("realtime");
        root["tags"].Push("vulkan");

        std::string compact = root.ToString(false);
        NYX_LOG_INFO("JSON compact: %s", compact.c_str());

        std::string pretty = root.ToString(true);
        NYX_LOG_INFO("JSON pretty: %s", pretty.c_str());

        // 解析回来
        std::string err;
        JsonValue parsed = JsonValue::Parse(compact, &err);
        if (!err.empty()) {
            NYX_LOG_ERROR("JSON parse error: %s", err.c_str());
        } else {
            NYX_LOG_INFO("JSON parsed: name=%s, version=%d, timeOfDay=%.2f, tags=%zu",
                         parsed["name"].AsString().c_str(),
                         parsed["version"].AsInt(),
                         parsed["lighting"]["timeOfDay"].AsFloat(),
                         parsed["tags"].Size());
        }

        // 错误案例
        std::string err2;
        JsonValue bad = JsonValue::Parse("{\"a\":1,}", &err2);
        NYX_LOG_INFO("JSON error case: %s", err2.c_str());

        std::string err3;
        JsonValue bad2 = JsonValue::Parse("[1, 2, ", &err3);
        NYX_LOG_INFO("JSON error case: %s", err3.c_str());

        std::string err4;
        JsonValue bad3 = JsonValue::Parse("{\"a\": }", &err4);
        NYX_LOG_INFO("JSON error case: %s", err4.c_str());
    }

    // ============ JobSystem 自测 ============
    {
        JobSystem jobs;
        jobs.Initialize();
        NYX_LOG_INFO("JobSystem test: %u workers", jobs.GetWorkerCount());

        std::atomic<int> counter{0};
        const int totalJobs = 20;

        auto startTime = std::chrono::steady_clock::now();

        for (int i = 0; i < totalJobs; i++) {
            jobs.Submit([&counter]() {
                // 模拟一点工作
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                counter.fetch_add(1);
            });
        }

        jobs.WaitAll();

        auto endTime = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        NYX_LOG_INFO("JobSystem test: completed %d/%d jobs in %lld ms",
                     counter.load(), totalJobs, (long long)elapsedMs);
        NYX_LOG_INFO("JobSystem test: pending = %zu, active = %zu",
                     jobs.GetPendingJobCount(), jobs.GetActiveJobCount());

        jobs.Shutdown();
    }

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
    NYX_LOG_INFO("NyxEngine shutting down...");

    Memory::PrintStats();

    EventBus::Clear();
    engine.Shutdown();
    Platform::Shutdown();
    Memory::Shutdown();
    Logger::Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}