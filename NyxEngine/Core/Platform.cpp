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
    // steady_clock 在不同平台上精度不同。
    // 我们用它的 tick 数，配合 TicksToSeconds 转换。
    // 大多数平台（Windows/macOS）tick 是 100ns；Linux 是 1ns。
    // 为了接口稳定，我们返回 tick 数，由 TicksToSeconds 处理。
    auto now = Clock::now().time_since_epoch();
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

uint64_t Platform::GetTimerFrequency() {
    // 用 nanoseconds 精度，所以频率是 10^9
    return 1000000000ULL;
}

uint64_t Platform::GetTimerTick() {
    auto now = Clock::now().time_since_epoch();
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

double Platform::TicksToSeconds(uint64_t ticks) {
    return (double)ticks / (double)GetTimerFrequency();
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