#include "FileSystem.h"
#include "Logger.h"
#include <fstream>
#include <SDL.h>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    // Windows.h 定义了这些宏，会干扰同名函数。undef 掉
    #undef CreateDirectory
    #undef CreateFile
    #undef DeleteFile
    #undef MoveFile
    #undef CopyFile
#else
    #include <unistd.h>
#endif

// ---------- 内部工具 ----------

namespace {

bool IsPathSeparator(char c) {
    return c == '/' || c == '\\';
}

// 从末尾往前找最后一个分隔符的位置，找不到返回 npos
size_t FindLastSeparator(const std::string& path) {
    for (size_t i = path.size(); i > 0; --i) {
        if (IsPathSeparator(path[i - 1])) return i - 1;
    }
    return std::string::npos;
}

} // anonymous namespace

// ============ 文件 IO ============

std::vector<uint8_t> FileSystem::ReadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        NYX_LOG_ERROR("FileSystem: failed to open '%s' for reading", path.c_str());
        return {};
    }

    std::streampos end = file.tellg();
    if (end < 0) {
        NYX_LOG_ERROR("FileSystem: failed to get size of '%s'", path.c_str());
        return {};
    }

    size_t size = (size_t)end;
    if (size == 0) return {};

    std::vector<uint8_t> buffer(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();

    return buffer;
}

std::string FileSystem::ReadText(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        NYX_LOG_ERROR("FileSystem: failed to open '%s' for reading", path.c_str());
        return {};
    }

    std::streampos end = file.tellg();
    if (end < 0) {
        NYX_LOG_ERROR("FileSystem: failed to get size of '%s'", path.c_str());
        return {};
    }

    size_t size = (size_t)end;
    if (size == 0) return {};

    std::string buffer(size, '\0');
    file.seekg(0);
    file.read(&buffer[0], size);
    file.close();

    return buffer;
}

bool FileSystem::WriteBinary(const std::string& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        NYX_LOG_ERROR("FileSystem: failed to open '%s' for writing", path.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(data), size);
    bool ok = file.good();
    file.close();

    if (!ok) {
        NYX_LOG_ERROR("FileSystem: failed to write '%s'", path.c_str());
    }
    return ok;
}

bool FileSystem::WriteText(const std::string& path, const std::string& text) {
    return WriteBinary(path, text.data(), text.size());
}

// ============ 检查 ============

bool FileSystem::Exists(const std::string& path) {
    // SDL 提供跨平台的存在检查
    SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
    if (rw) {
        SDL_RWclose(rw);
        return true;
    }
    return false;
}

bool FileSystem::IsDirectory(const std::string& path) {
    // 简化实现：暂时返回 false，避免误判
    (void)path;
    return false;
}

bool FileSystem::IsFile(const std::string& path) {
    return Exists(path);
}

size_t FileSystem::GetFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return 0;

    std::streampos end = file.tellg();
    file.close();

    if (end < 0) return 0;
    return (size_t)end;
}

// ============ 目录 ============

bool FileSystem::CreateDirectory(const std::string& path) {
#ifdef _WIN32
    if (CreateDirectoryA(path.c_str(), nullptr)) return true;
    return GetLastError() == ERROR_ALREADY_EXISTS;
#else
    (void)path;
    return false;
#endif
}

bool FileSystem::CreateDirectories(const std::string& path) {
    // 简化实现：暂时只创建单层
    return CreateDirectory(path);
}

// ============ 路径 ============

std::string FileSystem::GetExecutableDir() {
    char* basePath = SDL_GetBasePath();
    if (!basePath) {
        NYX_LOG_ERROR("FileSystem: SDL_GetBasePath failed");
        return {};
    }

    std::string result(basePath);
    SDL_free(basePath);

    for (char& c : result) {
        if (c == '\\') c = '/';
    }

    return result;
}

std::string FileSystem::GetWorkingDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (len == 0 || len >= MAX_PATH) {
        NYX_LOG_ERROR("FileSystem: GetCurrentDirectory failed");
        return {};
    }

    std::string result(buffer);
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    return result;
#else
    char buffer[1024];
    if (!getcwd(buffer, sizeof(buffer))) {
        NYX_LOG_ERROR("FileSystem: getcwd failed");
        return {};
    }
    return std::string(buffer);
#endif
}

std::string FileSystem::Join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;

    bool aEndsWithSep = IsPathSeparator(a.back());
    bool bStartsWithSep = IsPathSeparator(b.front());

    if (aEndsWithSep && bStartsWithSep) {
        return a + b.substr(1);
    } else if (aEndsWithSep || bStartsWithSep) {
        return a + b;
    } else {
        return a + "/" + b;
    }
}

std::string FileSystem::Normalize(const std::string& path) {
    std::string result = path;
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    return result;
}

std::string FileSystem::GetFileName(const std::string& path) {
    size_t pos = FindLastSeparator(path);
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string FileSystem::GetFileStem(const std::string& path) {
    std::string name = GetFileName(path);
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return name;
    return name.substr(0, dot);
}

std::string FileSystem::GetFileExtension(const std::string& path) {
    std::string name = GetFileName(path);
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return {};
    return name.substr(dot);
}

std::string FileSystem::GetDirectory(const std::string& path) {
    size_t pos = FindLastSeparator(path);
    if (pos == std::string::npos) return {};
    if (pos == 0) return "/";
    return path.substr(0, pos);
}