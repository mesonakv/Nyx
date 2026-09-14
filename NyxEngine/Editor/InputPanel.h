#pragma once
class InputSystem;
class InputMap;

class InputPanel {
public:
    void Initialize(InputSystem& input, InputMap& inputMap) {
        input_ = &input;
        inputMap_ = &inputMap;
    }
    void Draw();

private:
    InputSystem* input_ = nullptr;
    InputMap* inputMap_ = nullptr;
};