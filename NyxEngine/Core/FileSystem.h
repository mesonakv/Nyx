#pragma once
#include <string>
#include <vector>
#include <cstdint>

// ============ FileSystem ============
//
// 文件 IO 和路径处理。
//
// 设计原则：
//   - 不抛异常。所有失败通过返回值表示
//   - 失败时记日志
//   - 路径分隔符统一用 '/'，Windows 也认
//   - 不做热路径缓存（调用者自己缓存）
//
// 用法：
//   auto bytes = FileSystem::ReadBinary("Shaders/ball.vert.spv");
//   if (bytes.empty()) { /* 处理错误 */ }
//
//   std::string path = FileSystem::Join(FileSystem::GetExecutableDir(), "config.json");

class FileSystem {
public:
    // ---------- 文件 IO ----------

    // 读取整个文件。失败返回空 vector
    static std::vector<uint8_t> ReadBinary(const std::string& path);

    // 读取文本文件。失败返回空字符串
    static std::string ReadText(const std::string& path);

    // 写入文件（覆盖）。成功返回 true
    static bool WriteBinary(const std::string& path, const void* data, size_t size);
    static bool WriteText(const std::string& path, const std::string& text);

    // ---------- 检查 ----------

    static bool Exists(const std::string& path);
    static bool IsDirectory(const std::string& path);
    static bool IsFile(const std::string& path);

    // 返回文件字节数。不存在或失败返回 0
    static size_t GetFileSize(const std::string& path);

    // ---------- 目录 ----------

    // 创建单层目录。已存在视为成功
    static bool CreateDirectory(const std::string& path);

    // 创建多层目录。已存在视为成功
    static bool CreateDirectories(const std::string& path);

    // ---------- 路径 ----------

    // 可执行文件所在目录，带尾部 '/'。失败返回空字符串
    static std::string GetExecutableDir();

    // 当前工作目录，不带尾部 '/'。失败返回空字符串
    static std::string GetWorkingDir();

    // 拼接路径。自动处理分隔符
    static std::string Join(const std::string& a, const std::string& b);

    // 路径的规范化版本（去掉 . / ..，统一分隔符）
    static std::string Normalize(const std::string& path);

    // 提取路径的各个部分
    static std::string GetFileName(const std::string& path);       // "a/b/c.txt" -> "c.txt"
    static std::string GetFileStem(const std::string& path);       // "a/b/c.txt" -> "c"
    static std::string GetFileExtension(const std::string& path);  // "a/b/c.txt" -> ".txt"（带点）
    static std::string GetDirectory(const std::string& path);      // "a/b/c.txt" -> "a/b"
};