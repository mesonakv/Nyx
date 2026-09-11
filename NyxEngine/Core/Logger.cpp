#include "Logger.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <fstream>
#include <chrono>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

namespace {

// ============ 内部状态 ============
// 用函数内静态变量保证线程安全的延迟初始化

struct LoggerState {
    LoggerConfig config;
    std::mutex mutex;
    std::ofstream fileStream;
    bool initialized = false;
};

LoggerState& GetState() {
    static LoggerState state;
    return state;
}

// ============ 工具函数 ============

const char* LevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    default:              return "?????";
    }
}

// 从路径中提取文件名（去掉目录部分）
const char* ExtractFileName(const char* path) {
    if (!path) return "";
    const char* lastSlash = strrchr(path, '/');
    const char* lastBackslash = strrchr(path, '\\');
    const char* last = lastSlash > lastBackslash ? lastSlash : lastBackslash;
    return last ? last + 1 : path;
}

// 生成时间戳："HH:MM:SS.mmm"
void FormatTimestamp(char* out, size_t size) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto time = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    snprintf(out, size, "%02d:%02d:%02d.%03d",
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count());
}

} // anonymous namespace

// ============ Logger 实现 ============

void Logger::Initialize(const LoggerConfig& config) {
    LoggerState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.config = config;
    state.initialized = true;

    if (state.config.toFile) {
        state.fileStream.open(state.config.filePath, std::ios::out | std::ios::app);
        if (!state.fileStream.is_open()) {
            // 打开文件失败，禁用文件输出
            state.config.toFile = false;
        }
    }
}

void Logger::Shutdown() {
    LoggerState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.fileStream.is_open()) {
        state.fileStream.flush();
        state.fileStream.close();
    }
    state.initialized = false;
}

void Logger::SetLevel(LogLevel level) {
    LoggerState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.config.minLevel = level;
}

LogLevel Logger::GetLevel() {
    LoggerState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.config.minLevel;
}

void Logger::Log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    // 快速过滤（无锁读取）
    if (level < GetState().config.minLevel) return;

    // 格式化用户消息
    char message[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // 生成时间戳
    char timestamp[32];
    FormatTimestamp(timestamp, sizeof(timestamp));

    // 组装完整日志行
    // 格式：[HH:MM:SS.mmm] [LEVEL] [file:line] message
    const char* shortFile = ExtractFileName(file);

    char finalLine[4096 + 256];
    snprintf(finalLine, sizeof(finalLine), "[%s] [%s] [%s:%d] %s",
             timestamp, LevelToString(level), shortFile, line, message);

    LoggerState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.config.toConsole) {
        FILE* out = (level >= LogLevel::Error) ? stderr : stdout;
        fprintf(out, "%s\n", finalLine);
    }

    if (state.config.toDebugOutput) {
#ifdef _WIN32
        OutputDebugStringA(finalLine);
        OutputDebugStringA("\n");
#endif
    }

    if (state.config.toFile && state.fileStream.is_open()) {
        state.fileStream << finalLine << "\n";
        // 每条 flush，崩溃时日志不会丢
        state.fileStream.flush();
    }
}