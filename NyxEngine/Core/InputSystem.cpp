#include "InputSystem.h"
#include <cstring>

void InputSystem::Initialize(SDL_Window* window) {
    window_ = window;
    std::memset(keysDown_, 0, sizeof(keysDown_));
    std::memset(keysDownLastFrame_, 0, sizeof(keysDownLastFrame_));
    mouseDeltaX_ = 0.0f;
    mouseDeltaY_ = 0.0f;
    mouseCaptured_ = false;
    quitRequested_ = false;
}

void InputSystem::Shutdown() {
    if (mouseCaptured_) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouseCaptured_ = false;
    }
    window_ = nullptr;
}

void InputSystem::BeginFrame() {
    // 清空本帧鼠标累计 delta。每帧开始时归零，ProcessEvent 里累加
    mouseDeltaX_ = 0.0f;
    mouseDeltaY_ = 0.0f;
}

void InputSystem::ProcessEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT:
        quitRequested_ = true;
        break;

    case SDL_KEYDOWN:
        if (!e.key.repeat) {
            keysDown_[e.key.keysym.scancode] = true;
        }
        break;

    case SDL_KEYUP:
        keysDown_[e.key.keysym.scancode] = false;
        break;

    case SDL_MOUSEMOTION:
        mouseDeltaX_ += (float)e.motion.xrel;
        mouseDeltaY_ += (float)e.motion.yrel;
        break;

    default:
        break;
    }
}

void InputSystem::EndFrame() {
    // 归档本帧的键盘状态，供下一帧计算 pressed/released
    std::memcpy(keysDownLastFrame_, keysDown_, sizeof(keysDown_));
}

bool InputSystem::IsKeyDown(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return keysDown_[key];
}

bool InputSystem::WasKeyPressed(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return keysDown_[key] && !keysDownLastFrame_[key];
}

bool InputSystem::WasKeyReleased(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return !keysDown_[key] && keysDownLastFrame_[key];
}

void InputSystem::SetMouseCaptured(bool captured) {
    if (mouseCaptured_ == captured) return;
    mouseCaptured_ = captured;

    if (captured) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        // 捕获时把鼠标挪到窗口中心，避免"第一次移动就是大 delta"
        if (window_) {
            int w = 0, h = 0;
            SDL_GetWindowSize(window_, &w, &h);
            SDL_WarpMouseInWindow(window_, w / 2, h / 2);
        }
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
}