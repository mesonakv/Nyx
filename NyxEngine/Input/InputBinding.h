#pragma once
#include "InputEvent.h"   // 为了 InputDevice

// ============ InputBinding ============
//
// 一个"设备 + 编码"对。表示"按了什么"。
//
// code 的含义：
//   Keyboard: SDL_Scancode
//   Mouse:    SDL_BUTTON_LEFT 等（1~5）
//   Gamepad:  预留

struct InputBinding {
    InputDevice device = InputDevice::Keyboard;
    int code = 0;

    bool operator==(const InputBinding& o) const {
        return device == o.device && code == o.code;
    }
};