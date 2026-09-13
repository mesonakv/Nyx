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
//   - Raw Input 通道（Windows，游戏模式）：通过 Win32InputBackend 采集
//   - SDL 通道（Fallback 和编辑器模式）：通过 SDL_Event 采集
//
// 有效通道判定（IsRawInputActive）：
//   useRawInput_ && mouseCaptured_
//   - 游戏模式：鼠标被捕获，Raw Input 有效
//   - 编辑器模式：鼠标被释放，Raw Input 无效，退回 SDL
//
// ImGui 始终从 SDL 事件读（不受影响）。
// 窗口事件（resize/focus/quit）始终走 SDL。
//
// 输入不丢失保证：
//   - keysDown_ 记录持续状态
//   - keysPressedThisFrame_ / keysReleasedThisFrame_ 记录本帧的瞬时事件
//
// 事件历史：
//   - 所有输入事件写入 InputEventHistory，保留最近 250ms
//   - 用于输入缓冲、连招窗口、精确判定

class InputSystem {
public:
    void Initialize(SDL_Window* window);
    void Shutdown();

    // 每帧调用
    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);        // SDL 通道
    void ProcessRawInputEvent(const InputEvent& ev); // Raw Input 通道
    void EndFrame();

    // ---------- 鼠标 ----------
    float GetMouseDeltaX() const { return mouseDeltaX_; }
    float GetMouseDeltaY() const { return mouseDeltaY_; }

    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return mouseCaptured_; }

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

    // 内部：把事件应用到状态表
    void ApplyEvent(const InputEvent& ev);

    // 当前是否应该用 Raw Input 通道
    // 需要 useRawInput_ 且鼠标被捕获（编辑器模式下鼠标释放，Raw Input 无效）
    bool IsRawInputActive() const;

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

    // 事件历史
    InputEventHistory history_;
    uint64_t totalEventCount_ = 0;

    // Raw Input 后端
    std::unique_ptr<IInputBackend> rawInputBackend_;
    bool useRawInput_ = false;
};