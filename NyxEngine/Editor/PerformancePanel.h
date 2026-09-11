#pragma once
#include <deque>

class PerformancePanel {
public:
    void Initialize(const std::deque<float>& frameTimes) { frameTimes_ = &frameTimes; }
    void Draw();
private:
    const std::deque<float>* frameTimes_ = nullptr;
};