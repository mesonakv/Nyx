#pragma once

class FrameTimeHistory;

class PerformancePanel {
public:
    void Initialize(const FrameTimeHistory& history) { history_ = &history; }
    void Draw();
private:
    const FrameTimeHistory* history_ = nullptr;
};