#pragma once
#include <cstdint>

// ============ Action ============
//
// 输入的语义动作。游戏逻辑只认识 Action，不认识按键。
//
// 命名约定：
//   - 移动类：Move*
//   - 动作类：Jump / Dash / Slide / ...
//   - 战斗类：*Fire / Aim
//   - 系统类：ToggleEditor / Pause

enum class Action : uint16_t {
    // 移动
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,

    // 动作
    Jump,
    Dash,
    Slide,

    // 战斗
    PrimaryFire,
    SecondaryFire,

    // 系统
    ToggleEditor,
    Pause,

    Count
};

const char* ActionToString(Action a);
bool StringToAction(const char* s, Action& out);