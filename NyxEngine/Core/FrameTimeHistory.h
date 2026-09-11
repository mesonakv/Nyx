#pragma once
#include <vector>
#include <cstring>
#include <cstddef>

// ============ FrameTimeHistory ============
//
// 固定容量的帧时间历史。
//
// 满了之后用 memmove 滑动，保证数据线性存储。
// push_back 不分配内存（构造时一次性 reserve）。
//
// 为何不用 std::deque：
//   deque 的 pop_front 在某些实现下会释放内存块，产生分配开销。
//
// 为何不用环形 buffer：
//   环形 buffer 的数据在逻辑上不线性，遍历时需要取模或提供迭代器。
//   对 600 个 float（2.4KB）来说，memmove 的开销远小于维护环形的复杂度。

class FrameTimeHistory {
public:
    explicit FrameTimeHistory(size_t capacity) {
        buffer_.reserve(capacity);
    }

    void push_back(float v) {
        if (buffer_.size() >= buffer_.capacity()) {
            // 满了：丢弃最老的，整体前移一位
            std::memmove(buffer_.data(), buffer_.data() + 1,
                         (buffer_.size() - 1) * sizeof(float));
            buffer_.back() = v;
        } else {
            buffer_.push_back(v);
        }
    }

    size_t size() const { return buffer_.size(); }
    size_t capacity() const { return buffer_.capacity(); }
    bool empty() const { return buffer_.empty(); }

    const float* data() const { return buffer_.data(); }
    float operator[](size_t i) const { return buffer_[i]; }

    const float* begin() const { return buffer_.data(); }
    const float* end() const { return buffer_.data() + buffer_.size(); }

private:
    std::vector<float> buffer_;
};