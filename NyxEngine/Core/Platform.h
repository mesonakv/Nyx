#pragma once
#include <cstdint>
#include <string>

// ============ Platform ============
//
// 平台抽象。
//
// 当前提供：
//   - 高精度计时器
//   - 睡眠
//   - 平台信息
//   - CPU 核心数
//
// 未来扩展：
//   - 动态库加载
//   - 文件监视
//   - 系统事件（电源、网络状态）

class Platform {
public:
    static void Initialize();
    static void Shutdown();

    // ---------- 计时 ----------
    // 返回当前时间（纳秒）。单调递增，不受系统时间调整影响
    static uint64_t GetTimerNanos();

    // 返回计时器频率（每秒多少 tick）。用 GetTimerNanos 时是 10^9
    static uint64_t GetTimerFrequency();

    // 返回一个用于计时的当前 tick
    static uint64_t GetTimerTick();

    // 从 tick 转换为秒
    static double TicksToSeconds(uint64_t ticks);

    // ---------- 睡眠 ----------
    static void SleepMs(uint32_t milliseconds);
    static void SleepUs(uint32_t microseconds);

    // ---------- 平台信息 ----------
    static const char* GetPlatformName();   // "Windows" / "Linux" / "macOS"
    static uint32_t GetCPUCoreCount();
};