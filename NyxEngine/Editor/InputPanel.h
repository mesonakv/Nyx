#pragma once
class InputSystem;

class InputPanel {
public:
    void Initialize(InputSystem& input) { input_ = &input; }
    void Draw();

private:
    InputSystem* input_ = nullptr;
};