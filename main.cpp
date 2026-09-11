#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>
#include <atomic>
#include <chrono>

#include "NyxEngine/NyxEngine.h"
#include "NyxEngine/Core/Logger.h"
#include "NyxEngine/Core/Memory.h"
#include "NyxEngine/Core/Platform.h"
#include "NyxEngine/Core/EventBus.h"
#include "NyxEngine/Core/JobSystem.h"
#include "NyxEngine/Core/FileSystem.h"
#include "NyxEngine/Core/JsonValue.h"
#include "NyxEngine/Core/VulkanContext.h"
#include "Game/MyGame.h"

namespace {

// ============ 引擎自测 ============
// 用于快速验证核心模块是否正常工作。
// 每个自测独立作用域，跑完即清理。

void RunEventBusTest() {
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

void RunJsonTest() {
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

void RunJobSystemTest() {
    JobSystem jobs;
    jobs.Initialize();
    NYX_LOG_INFO("JobSystem test: %u workers", jobs.GetWorkerCount());

    std::atomic<int> counter{0};
    const int totalJobs = 20;

    auto startTime = std::chrono::steady_clock::now();

    for (int i = 0; i < totalJobs; i++) {
        jobs.Submit([&counter]() {
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

void RunSelfTests() {
    RunEventBusTest();
    RunJsonTest();
    RunJobSystemTest();
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // ============ 平台初始化 ============
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    // ============ 引擎基础设施初始化 ============
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

    // ============ 引擎自测 ============
    RunSelfTests();

    // ============ 窗口 ============
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

    // ============ 游戏 ============
    MyGame game;
    game.Initialize(engine, window);
    game.Run();
    game.Shutdown();

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