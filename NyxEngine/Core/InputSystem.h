#pragma once
#include <SDL.h>
#include "../Input/InputEventHistory.h"

// ============ InputSystem ============
//
// 职责：采集 SDL 事件，缓存鼠标/键盘状态。
//
// 设计原则：
//   - 不决定业务逻辑（不知道 F1 是切编辑器）
//   - 只提供"现在鼠标动了多少"、"这个键是不是按住"、"这个键本帧刚按下"这类查询
//   - 每帧流程：BeginFrame() -> ProcessEvent() * N -> 使用 -> EndFrame()
//
// 输入不丢失保证：
//   - keysDown_ 记录持续状态
//   - keysPressedThisFrame_ / keysReleasedThisFrame_ 记录本帧的瞬时事件
//   - 即便玩家在两次 PollEvent 之间完成按下+释放，这两个表也能正确记录
//
// 事件历史：
//   - 所有输入事件写入 InputEventHistory，保留最近 250ms
//   - 用于输入缓冲、连招窗口、精确判定
//   - 阶段 2（Raw Input）和阶段 3（独立输入线程）会用到
//
// 未来扩展：
//   - 手柄输入
//   - 灵敏度曲线
//   - 按键映射（InputMap，独立模块）
//   - Raw Input（阶段 2）

class InputSystem {
public:
    void Initialize(SDL_Window* window);
    void Shutdown();

    // 每帧调用
    void BeginFrame();                        // 清空本帧临时状态、清理过期历史
    void ProcessEvent(const SDL_Event& e);    // 处理单个 SDL 事件
    void EndFrame();                          // 保留接口，当前为空

    // ---------- 鼠标 ----------
    float GetMouseDeltaX() const { return mouseDeltaX_; }
    float GetMouseDeltaY() const { return mouseDeltaY_; }

    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return mouseCaptured_; }

    // ---------- 键盘 ----------
    bool IsKeyDown(SDL_Scancode key) const;       // 按住
    bool WasKeyPressed(SDL_Scancode key) const;   // 本帧刚按下（不丢输入）
    bool WasKeyReleased(SDL_Scancode key) const;  // 本帧刚释放（不丢输入）

    // ---------- 退出 ----------
    bool ShouldQuit() const { return quitRequested_; }

    // ---------- 事件历史 ----------
    const InputEventHistory& GetEventHistory() const { return history_; }

    // 返回指定按键最近一次按下的时间戳（纳秒），0 表示历史里没有
    uint64_t GetLastKeyPressTime(SDL_Scancode key) const;
    uint64_t GetLastKeyReleaseTime(SDL_Scancode key) const;

    // 返回指定按键在 sinceTimestamp 之后是否被按下过
    bool WasKeyPressedSince(SDL_Scancode key, uint64_t sinceTimestamp) const;

private:
    static constexpr int kMaxKeys = SDL_NUM_SCANCODES;

    SDL_Window* window_ = nullptr;

    // 键盘状态
    bool keysDown_[kMaxKeys] = {};
    bool keysPressedThisFrame_[kMaxKeys] = {};
    bool keysReleasedThisFrame_[kMaxKeys] = {};

    // 鼠标
    float mouseDeltaX_ = 0.0f;
    float mouseDeltaY_ = 0.0f;
    bool mouseCaptured_ = false;

    // 退出
    bool quitRequested_ = false;

    // 事件历史（环形缓冲）
    InputEventHistory history_;
};