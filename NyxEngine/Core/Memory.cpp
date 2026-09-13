#include "Memory.h"
#include "Logger.h"
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <string>

namespace {

// ============ 分配 header ============
// 32 字节，保证 16 字节对齐（满足 max_align_t 要求）
//
// TODO: 如果以后要用 SIMD 类型（__m256 需要 32 字节对齐），
//       header 需要扩展到 64 字节，或者用 aligned_alloc + 手动偏移。

struct AllocationHeader {
    size_t size;        // 8
    const char* tag;    // 8
    const char* file;   // 8
    int line;           // 4
    uint32_t magic;     // 4
};
static_assert(sizeof(AllocationHeader) == 32, "AllocationHeader must be 32 bytes");

constexpr uint32_t kMagicValue = 0xDEADBEEF;
constexpr size_t kHeaderSize = sizeof(AllocationHeader);

// ============ 内部状态 ============

struct MemoryState {
    std::mutex mutex;

#if defined(_DEBUG)
    std::unordered_map<void*, AllocationHeader> allocations;
#endif

    size_t totalAllocated = 0;   // 当前活跃字节数
    size_t peakAllocated = 0;    // 峰值字节数
    size_t totalAllocCount = 0;
    size_t totalFreeCount = 0;
    bool initialized = false;
};

MemoryState& GetState() {
    static MemoryState state;
    return state;
}

} // anonymous namespace

// ============ 实现 ============

void Memory::Initialize() {
    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.initialized) return;

#if defined(_DEBUG)
    state.allocations.reserve(1024);
#endif
    state.initialized = true;
}

void Memory::Shutdown() {
    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (!state.initialized) return;

#if defined(_DEBUG)
    if (!state.allocations.empty()) {
        NYX_LOG_WARN("Memory: %zu allocations still live at shutdown", state.allocations.size());
        for (auto& pair : state.allocations) {
            const AllocationHeader& h = pair.second;
            NYX_LOG_WARN("  Leak: %zu bytes at %s:%d (tag=%s)",
                         h.size,
                         h.file ? h.file : "?",
                         h.line,
                         h.tag ? h.tag : "?");
        }
    }
    state.allocations.clear();
#endif

    state.initialized = false;
}

void* Memory::Allocate(size_t size, const char* tag, const char* file, int line) {
    // C++ 的 new 语义：size=0 也要返回一个有效且可释放的指针。
    // malloc(0) 的行为是实现定义的，可能返回 nullptr。
    // 为了让 NYX_NEW 之类的宏行为一致，size=0 时分配 1 字节。
    size_t actualSize = (size == 0) ? 1 : size;

    void* raw = std::malloc(kHeaderSize + actualSize);
    if (!raw) {
        NYX_LOG_FATAL("Memory: malloc failed for %zu bytes", actualSize);
    }

    AllocationHeader* header = (AllocationHeader*)raw;
    header->size = actualSize;
    header->tag = tag;
    header->file = file;
    header->line = line;
    header->magic = kMagicValue;

    void* userPtr = (char*)raw + kHeaderSize;

    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.totalAllocated += actualSize;
    if (state.totalAllocated > state.peakAllocated) {
        state.peakAllocated = state.totalAllocated;
    }
    state.totalAllocCount++;

#if defined(_DEBUG)
    state.allocations[userPtr] = *header;
#endif

    return userPtr;
}

void* Memory::Reallocate(void* ptr, size_t newSize, const char* tag, const char* file, int line) {
    if (!ptr) return Allocate(newSize, tag, file, line);
    if (newSize == 0) { Free(ptr); return nullptr; }

    size_t oldSize = GetSizeOfAllocation(ptr);

    void* newPtr = Allocate(newSize, tag, file, line);
    if (oldSize > 0) {
        memcpy(newPtr, ptr, oldSize < newSize ? oldSize : newSize);
    }
    Free(ptr);
    return newPtr;
}

void Memory::Free(void* ptr) {
    if (!ptr) return;

    void* raw = (char*)ptr - kHeaderSize;
    AllocationHeader* header = (AllocationHeader*)raw;

    if (header->magic != kMagicValue) {
        NYX_LOG_FATAL("Memory: corrupted header or invalid pointer at %p", ptr);
    }

    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    size_t size = header->size;
    state.totalAllocated -= size;
    state.totalFreeCount++;

#if defined(_DEBUG)
    auto it = state.allocations.find(ptr);
    if (it == state.allocations.end()) {
        NYX_LOG_FATAL("Memory: double free or unknown pointer %p", ptr);
    }
    state.allocations.erase(it);
#endif

    header->magic = 0;  // 防止 double free 时 magic 检查通过
    std::free(raw);
}

size_t Memory::GetSizeOfAllocation(void* ptr) {
    if (!ptr) return 0;
    void* raw = (char*)ptr - kHeaderSize;
    AllocationHeader* header = (AllocationHeader*)raw;
    if (header->magic != kMagicValue) return 0;
    return header->size;
}

size_t Memory::GetTotalAllocated() {
    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.totalAllocated;
}

size_t Memory::GetPeakAllocated() {
    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.peakAllocated;
}

void Memory::PrintStats() {
    MemoryState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);

    NYX_LOG_INFO("=== Memory Stats ===");
    NYX_LOG_INFO("Live bytes:   %zu", state.totalAllocated);
    NYX_LOG_INFO("Peak bytes:   %zu", state.peakAllocated);
    NYX_LOG_INFO("Total allocs: %zu", state.totalAllocCount);
    NYX_LOG_INFO("Total frees:  %zu", state.totalFreeCount);

#if defined(_DEBUG)
    NYX_LOG_INFO("Live allocations: %zu", state.allocations.size());

    if (!state.allocations.empty()) {
        std::unordered_map<std::string, size_t> tagSizes;
        std::unordered_map<std::string, size_t> tagCounts;
        for (auto& pair : state.allocations) {
            const char* t = pair.second.tag ? pair.second.tag : "untagged";
            tagSizes[t] += pair.second.size;
            tagCounts[t]++;
        }
        NYX_LOG_INFO("By tag:");
        for (auto& pair : tagSizes) {
            NYX_LOG_INFO("  %-20s %6zu bytes (%zu allocs)",
                         pair.first.c_str(), pair.second, tagCounts[pair.first]);
        }
    }
#endif
}