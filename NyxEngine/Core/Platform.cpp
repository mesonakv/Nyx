#include "Platform.h"
#include "Logger.h"
#include <chrono>
#include <thread>
#include <cmath>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;

uint64_t g_startTicks = 0;
bool g_initialized = false;

} // anonymous namespace

// ============ 生命周期 ============

void Platform::Initialize() {
    if (g_initialized) return;

    g_startTicks = (uint64_t)Clock::now().time_since_epoch().count();
    g_initialized = true;

    NYX_LOG_INFO("Platform: %s, %u CPU cores",
                 GetPlatformName(), GetCPUCoreCount());
}

void Platform::Shutdown() {
    g_initialized = false;
}

// ============ 计时 ============

uint64_t Platform::GetTimerNanos() {
    auto now = Clock::now().time_since_epoch();
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

double Platform::TicksToSeconds(uint64_t nanos) {
    return (double)nanos / 1.0e9;
}

// ============ 睡眠 ============

void Platform::SleepMs(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void Platform::SleepUs(uint32_t microseconds) {
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

// ============ 平台信息 ============

const char* Platform::GetPlatformName() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

uint32_t Platform::GetCPUCoreCount() {
    unsigned int count = std::thread::hardware_concurrency();
    return count > 0 ? (uint32_t)count : 1;
}