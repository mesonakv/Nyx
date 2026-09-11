#pragma once
#include <string>

// ============ 日志级别 ============

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6
};

// ============ 日志配置 ============

struct LoggerConfig {
    LogLevel minLevel = LogLevel::Info;
    bool toConsole = true;
    bool toFile = false;
    bool toDebugOutput = true;          // VS 输出窗口
    std::string filePath = "nyx.log";
};

// ============ Logger ============
//
// 线程安全。格式化用 printf 风格。
// Trace/Debug 在 Release 下编译期裁剪。
// 单条日志上限 4096 字节，超出截断。

class Logger {
public:
    static void Initialize(const LoggerConfig& config);
    static void Shutdown();

    // 底层接口。不要直接调用，用下面的宏。
    static void Log(LogLevel level, const char* file, int line, const char* fmt, ...);

    static void SetLevel(LogLevel level);
    static LogLevel GetLevel();
};

// ============ 便捷宏 ============
//
// 用法：
//   NYX_LOG_INFO("Hello, %s", name);
//   NYX_LOG_ERROR("Failed to open %s at line %d", path, line);

#ifdef _DEBUG
    #define NYX_LOG_TRACE(...)  Logger::Log(LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
    #define NYX_LOG_DEBUG(...)  Logger::Log(LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define NYX_LOG_TRACE(...)  ((void)0)
    #define NYX_LOG_DEBUG(...)  ((void)0)
#endif

#define NYX_LOG_INFO(...)   Logger::Log(LogLevel::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define NYX_LOG_WARN(...)   Logger::Log(LogLevel::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define NYX_LOG_ERROR(...)  Logger::Log(LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)

// Fatal 会记录并 abort
#define NYX_LOG_FATAL(...)  do { \
    Logger::Log(LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__); \
    Logger::Shutdown(); \
    abort(); \
} while(0)