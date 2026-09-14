#pragma once
#include <SDL.h>
#include <memory>
#include "../Input/InputEventHistory.h"
#include "../Input/IInputBackend.h"

// ============ InputSystem ============
//
// 职责：采集输入事件，缓存鼠标/键盘状态。
//
// 双通道设计：
//   - Raw Input 通道（Windows，游戏模式）：Win32InputBackend
//   - SDL 通道（Fallback 和编辑器模式）：SDL_Event
//
// 有效通道判定：useRawInput_ && mouseCaptured_
//
// 输入不丢失保证：
//   - keysDown_ 记录持续状态
//   - keysPressedThisFrame_ / keysReleasedThisFrame_ 记录本帧瞬时事件
//   - 鼠标按钮同理

class InputSystem {
public:
    void Initialize(SDL_Window* window);
    void Shutdown();

    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);
    void ProcessRawInputEvent(const InputEvent& ev);
    void EndFrame();

    // ---------- 鼠标移动 ----------
    float GetMouseDeltaX() const { return mouseDeltaX_; }
    float GetMouseDeltaY() const { return mouseDeltaY_; }

    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return mouseCaptured_; }

    // ---------- 鼠标按钮 ----------
    // button 用 SDL_BUTTON_LEFT / SDL_BUTTON_RIGHT / ... （1~5）
    bool IsMouseButtonDown(int button) const;
    bool WasMouseButtonPressed(int button) const;
    bool WasMouseButtonReleased(int button) const;

    // ---------- 键盘 ----------
    bool IsKeyDown(SDL_Scancode key) const;
    bool WasKeyPressed(SDL_Scancode key) const;
    bool WasKeyReleased(SDL_Scancode key) const;

    // ---------- 退出 ----------
    bool ShouldQuit() const { return quitRequested_; }

    // ---------- 事件历史 ----------
    const InputEventHistory& GetEventHistory() const { return history_; }
    uint64_t GetTotalEventCount() const { return totalEventCount_; }

    uint64_t GetLastKeyPressTime(SDL_Scancode key) const;
    uint64_t GetLastKeyReleaseTime(SDL_Scancode key) const;
    bool WasKeyPressedSince(SDL_Scancode key, uint64_t sinceTimestamp) const;

    // ---------- 后端切换 ----------
    bool IsUsingRawInput() const { return useRawInput_; }
    bool IsRawInputAvailable() const { return rawInputBackend_ != nullptr; }
    void SetUseRawInput(bool use);

private:
    static constexpr int kMaxKeys = SDL_NUM_SCANCODES;
    static constexpr int kMaxMouseButtons = 8;

    void ApplyEvent(const InputEvent& ev);
    bool IsRawInputActive() const;

    SDL_Window* window_ = nullptr;

    // 键盘
    bool keysDown_[kMaxKeys] = {};
    bool keysPressedThisFrame_[kMaxKeys] = {};
    bool keysReleasedThisFrame_[kMaxKeys] = {};

    // 鼠标按钮（index 0~7，对应 SDL_BUTTON_*）
    bool mouseButtonsDown_[kMaxMouseButtons] = {};
    bool mouseButtonsPressedThisFrame_[kMaxMouseButtons] = {};
    bool mouseButtonsReleasedThisFrame_[kMaxMouseButtons] = {};

    // 鼠标移动
    float mouseDeltaX_ = 0.0f;
    float mouseDeltaY_ = 0.0f;
    bool mouseCaptured_ = false;

    // 退出
    bool quitRequested_ = false;

    // 事件历史
    InputEventHistory history_;
    uint64_t totalEventCount_ = 0;

    // Raw Input 后端
    std::unique_ptr<IInputBackend> rawInputBackend_;
    bool useRawInput_ = false;
};