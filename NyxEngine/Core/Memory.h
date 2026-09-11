#pragma once
#include <cstddef>
#include <cstdlib>

// ============ Memory ============
//
// 统一内存分配接口。
//
// Debug 模式（_DEBUG）：
//   - 每次分配记录 header（大小、标签、文件、行号）
//   - 追踪表记录所有活跃分配
//   - Shutdown 时报告未释放的分配
//   - 支持按 tag 统计
//
// Release 模式：
//   - 仅保留最小 header（用于 free 时获取原始指针）
//   - 不做追踪表操作
//   - 开销接近 malloc
//
// 用法：
//   Foo* f = NYX_NEW(Foo, "Render");
//   NYX_DELETE(Foo, f);
//
//   void* buf = NYX_ALLOC(1024, "Temp");
//   NYX_FREE(buf);

class Memory {
public:
    static void Initialize();
    static void Shutdown();

    // 底层接口。不要直接调用，用下面的宏。
    static void* Allocate(size_t size, const char* tag, const char* file, int line);
    static void* Reallocate(void* ptr, size_t newSize, const char* tag, const char* file, int line);
    static void  Free(void* ptr);

    // 查询
    static size_t GetSizeOfAllocation(void* ptr);
    static size_t GetTotalAllocated();
    static size_t GetPeakAllocated();

    // 统计输出
    static void PrintStats();
};

// ============ 宏 ============

#define NYX_ALLOC(size, tag) \
    Memory::Allocate((size), (tag), __FILE__, __LINE__)

#define NYX_REALLOC(ptr, newSize, tag) \
    Memory::Reallocate((ptr), (newSize), (tag), __FILE__, __LINE__)

#define NYX_FREE(ptr) \
    Memory::Free(ptr)

#define NYX_NEW(T, tag) \
    new (Memory::Allocate(sizeof(T), (tag), __FILE__, __LINE__)) T

#define NYX_NEW_ARGS(T, tag, ...) \
    new (Memory::Allocate(sizeof(T), (tag), __FILE__, __LINE__)) T(__VA_ARGS__)

#define NYX_DELETE(T, ptr) \
    do { if (ptr) { (ptr)->~T(); Memory::Free(ptr); (ptr) = nullptr; } } while(0)