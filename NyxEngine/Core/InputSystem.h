#pragma once
#include <SDL.h>

// ============ InputSystem ============
//
// 职责：采集 SDL 事件，缓存鼠标/键盘状态。
//
// 设计原则：
//   - 不决定业务逻辑（不知道 F1 是切编辑器）
//   - 只提供"现在鼠标动了多少"、"这个键是不是按住"、"这个键本帧刚按下"这类查询
//   - 每帧流程：BeginFrame() -> ProcessEvent() * N -> 使用 -> EndFrame()
//
// 未来扩展：
//   - 手柄输入
//   - 灵敏度曲线
//   - 按键映射

class InputSystem {
public:
    void Initialize(SDL_Window* window);
    void Shutdown();

    // 每帧调用
    void BeginFrame();                        // 清空本帧临时状态（鼠标 delta）
    void ProcessEvent(const SDL_Event& e);    // 处理单个 SDL 事件
    void EndFrame();                          // 归档本帧状态，用于下一帧计算 pressed/released

    // ---------- 鼠标 ----------
    float GetMouseDeltaX() const { return mouseDeltaX_; }
    float GetMouseDeltaY() const { return mouseDeltaY_; }

    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return mouseCaptured_; }

    // ---------- 键盘 ----------
    bool IsKeyDown(SDL_Scancode key) const;       // 按住
    bool WasKeyPressed(SDL_Scancode key) const;   // 本帧刚按下
    bool WasKeyReleased(SDL_Scancode key) const;  // 本帧刚释放

    // ---------- 退出 ----------
    bool ShouldQuit() const { return quitRequested_; }

private:
    static constexpr int kMaxKeys = SDL_NUM_SCANCODES;

    SDL_Window* window_ = nullptr;

    // 键盘状态（每帧两张快照，用于计算 pressed/released）
    bool keysDown_[kMaxKeys] = {};
    bool keysDownLastFrame_[kMaxKeys] = {};

    // 鼠标
    float mouseDeltaX_ = 0.0f;
    float mouseDeltaY_ = 0.0f;
    bool mouseCaptured_ = false;

    // 退出
    bool quitRequested_ = false;
};