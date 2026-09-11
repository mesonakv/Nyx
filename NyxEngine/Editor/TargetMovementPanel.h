#pragma once
class TargetManager;

class TargetMovementPanel {
public:
    void Initialize(TargetManager& targets) { targets_ = &targets; }
    void Draw();
private:
    TargetManager* targets_ = nullptr;
};