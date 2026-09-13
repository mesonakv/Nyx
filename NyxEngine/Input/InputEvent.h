#pragma once
#include <cstdint>

// ============ InputEvent ============
//
// 一次输入事件的记录。
//
// 每个按键按下、释放，每次鼠标移动，都会生成一个 InputEvent。
// 保存在 InputEventHistory 的环形缓冲里，保留最近 N 毫秒。
//
// 用途：
//   - 输入缓冲（跳跃提前 100ms 按也生效）
//   - 连招窗口
//   - 精确判定（地板踩拍）
//   - 未来 Raw Input / 独立输入线程的去重
//
// 时间单位：纳秒（Platform::GetTimerNanos()）

enum class InputDevice : uint8_t {
    Keyboard = 0,
    Mouse    = 1,
    Gamepad  = 2,   // 预留，当前未实现
};

enum class InputEventType : uint8_t {
    KeyDown       = 0,
    KeyUp         = 1,
    MouseMove     = 2,
    MouseButtonDown = 3,
    MouseButtonUp   = 4,
    GamepadButtonDown = 5,   // 预留
    GamepadButtonUp   = 6,   // 预留
    GamepadAxisMove   = 7,   // 预留
};

// 事件来源。阶段 1 只有 SDL，阶段 2 会加 RawInput。
// 用于去重：如果同一时刻来自两个通道的同一个按键，只处理一次。
enum class InputSource : uint8_t {
    SDL       = 0,
    RawInput  = 1,   // 预留
};

struct InputEvent {
    uint64_t timestamp;      // 纳秒，来自 Platform::GetTimerNanos()
    int32_t  value1;         // Keyboard: SDL_Scancode
                             // Mouse button: SDL_BUTTON_*
                             // Mouse move: dx
                             // Gamepad: button/axis id
    int32_t  value2;         // Mouse move: dy
                             // Gamepad axis: value
                             // 其他: 0
    InputDevice  device;
    InputEventType type;
    InputSource  source;

    // ---------- 便捷判断 ----------

    bool IsKeyboard() const { return device == InputDevice::Keyboard; }
    bool IsMouse() const { return device == InputDevice::Mouse; }
    bool IsGamepad() const { return device == InputDevice::Gamepad; }

    bool IsPress() const {
        return type == InputEventType::KeyDown
            || type == InputEventType::MouseButtonDown
            || type == InputEventType::GamepadButtonDown;
    }

    bool IsRelease() const {
        return type == InputEventType::KeyUp
            || type == InputEventType::MouseButtonUp
            || type == InputEventType::GamepadButtonUp;
    }

    bool IsMouseMove() const {
        return type == InputEventType::MouseMove;
    }
};