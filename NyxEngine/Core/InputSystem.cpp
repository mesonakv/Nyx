#include "InputSystem.h"
#include <cstring>

void InputSystem::Initialize(SDL_Window* window) {
    window_ = window;
    std::memset(keysDown_, 0, sizeof(keysDown_));
    std::memset(keysPressedThisFrame_, 0, sizeof(keysPressedThisFrame_));
    std::memset(keysReleasedThisFrame_, 0, sizeof(keysReleasedThisFrame_));
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
    // 清空本帧的瞬时状态
    mouseDeltaX_ = 0.0f;
    mouseDeltaY_ = 0.0f;
    std::memset(keysPressedThisFrame_, 0, sizeof(keysPressedThisFrame_));
    std::memset(keysReleasedThisFrame_, 0, sizeof(keysReleasedThisFrame_));
}

void InputSystem::ProcessEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT:
        quitRequested_ = true;
        break;

    case SDL_KEYDOWN:
        if (!e.key.repeat) {
            keysDown_[e.key.keysym.scancode] = true;
            keysPressedThisFrame_[e.key.keysym.scancode] = true;
        }
        break;

    case SDL_KEYUP:
        keysDown_[e.key.keysym.scancode] = false;
        keysReleasedThisFrame_[e.key.keysym.scancode] = true;
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
    // 本帧的瞬时状态已在 BeginFrame 清空，这里不需要做任何事。
    // 保留接口是为了未来如果有"跨帧延迟派发"之类的需求可以扩展。
}

bool InputSystem::IsKeyDown(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return keysDown_[key];
}

bool InputSystem::WasKeyPressed(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return keysPressedThisFrame_[key];
}

bool InputSystem::WasKeyReleased(SDL_Scancode key) const {
    if (key < 0 || key >= kMaxKeys) return false;
    return keysReleasedThisFrame_[key];
}

void InputSystem::SetMouseCaptured(bool captured) {
    if (mouseCaptured_ == captured) return;
    mouseCaptured_ = captured;

    if (captured) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        if (window_) {
            int w = 0, h = 0;
            SDL_GetWindowSize(window_, &w, &h);
            SDL_WarpMouseInWindow(window_, w / 2, h / 2);
        }
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
}