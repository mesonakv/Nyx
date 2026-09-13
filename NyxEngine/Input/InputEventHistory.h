#pragma once
#include "InputEvent.h"
#include <array>
#include <cstddef>

// ============ InputEventHistory ============
//
// 环形缓冲，保留最近 N 纳秒的输入事件。
//
// 设计：
//   - 固定容量 kCapacity，零分配（构造时 std::array 就在栈/静态区）
//   - head_ 指向最老事件，count_ 是有效事件数
//   - Push 是 O(1)：满了就覆盖最老的
//   - Prune 是 O(1) 均摊：每个事件只会被移除一次
//   - 索引访问：At(0) 是最老的，At(Size()-1) 是最新的
//
// 时间窗口：
//   默认 250ms，够覆盖输入缓冲、连招窗口、双击检测。
//   窗口外的旧事件在 BeginFrame 时被清理。

class InputEventHistory {
public:
    static constexpr size_t kCapacity = 4096;
    static constexpr uint64_t kWindowNanos = 250ULL * 1000ULL * 1000ULL;  // 250ms

    void Clear() {
        head_ = 0;
        count_ = 0;
    }

    void Push(const InputEvent& e) {
        size_t tail = (head_ + count_) % kCapacity;
        if (count_ == kCapacity) {
            // 满了：覆盖最老的，head 前进
            buffer_[head_] = e;
            head_ = (head_ + 1) % kCapacity;
        } else {
            buffer_[tail] = e;
            count_++;
        }
    }

    // 移除所有 timestamp < cutoffTimestamp 的事件
    void PruneOlderThan(uint64_t cutoffTimestamp) {
        while (count_ > 0 && buffer_[head_].timestamp < cutoffTimestamp) {
            head_ = (head_ + 1) % kCapacity;
            count_--;
        }
    }

    // 索引访问。index 范围 [0, Size())
    const InputEvent& At(size_t index) const {
        return buffer_[(head_ + index) % kCapacity];
    }

    size_t Size() const { return count_; }
    bool Empty() const { return count_ == 0; }

    // 最新的事件（Size()==0 时未定义）
    const InputEvent& Back() const {
        return buffer_[(head_ + count_ - 1) % kCapacity];
    }

    // 最老的事件（Size()==0 时未定义）
    const InputEvent& Front() const {
        return buffer_[head_];
    }

private:
    std::array<InputEvent, kCapacity> buffer_ = {};
    size_t head_ = 0;
    size_t count_ = 0;
};