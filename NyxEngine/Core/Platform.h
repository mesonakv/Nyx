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
    // 返回当前时间（纳秒）。单调递增，不受系统时间调整影响。
    static uint64_t GetTimerNanos();

    // 从纳秒转换为秒
    static double TicksToSeconds(uint64_t nanos);

    // ---------- 睡眠 ----------
    static void SleepMs(uint32_t milliseconds);
    static void SleepUs(uint32_t microseconds);

    // ---------- 平台信息 ----------
    static const char* GetPlatformName();   // "Windows" / "Linux" / "macOS"
    static uint32_t GetCPUCoreCount();
};