#include "InputSystem.h"
#include "../Core/Platform.h"
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
    history_.Clear();
}

void InputSystem::Shutdown() {
    if (mouseCaptured_) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouseCaptured_ = false;
    }
    window_ = nullptr;
    history_.Clear();
}

void InputSystem::BeginFrame() {
    // 清空本帧的瞬时状态
    mouseDeltaX_ = 0.0f;
    mouseDeltaY_ = 0.0f;
    std::memset(keysPressedThisFrame_, 0, sizeof(keysPressedThisFrame_));
    std::memset(keysReleasedThisFrame_, 0, sizeof(keysReleasedThisFrame_));

    // 清理 250ms 之前的事件
    uint64_t now = Platform::GetTimerNanos();
    uint64_t cutoff = (now > InputEventHistory::kWindowNanos)
                    ? (now - InputEventHistory::kWindowNanos)
                    : 0;
    history_.PruneOlderThan(cutoff);
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

            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device = InputDevice::Keyboard;
            ev.type = InputEventType::KeyDown;
            ev.value1 = e.key.keysym.scancode;
            ev.value2 = 0;
            ev.source = InputSource::SDL;
            history_.Push(ev);
        }
        break;

    case SDL_KEYUP: {
        keysDown_[e.key.keysym.scancode] = false;
        keysReleasedThisFrame_[e.key.keysym.scancode] = true;

        InputEvent ev = {};
        ev.timestamp = Platform::GetTimerNanos();
        ev.device = InputDevice::Keyboard;
        ev.type = InputEventType::KeyUp;
        ev.value1 = e.key.keysym.scancode;
        ev.value2 = 0;
        ev.source = InputSource::SDL;
        history_.Push(ev);
        break;
    }

    case SDL_MOUSEMOTION: {
        float dx = (float)e.motion.xrel;
        float dy = (float)e.motion.yrel;
        mouseDeltaX_ += dx;
        mouseDeltaY_ += dy;

        // 只有非零 delta 才记录，避免静止时污染历史
        if (e.motion.xrel != 0 || e.motion.yrel != 0) {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device = InputDevice::Mouse;
            ev.type = InputEventType::MouseMove;
            ev.value1 = e.motion.xrel;
            ev.value2 = e.motion.yrel;
            ev.source = InputSource::SDL;
            history_.Push(ev);
        }
        break;
    }

    case SDL_MOUSEBUTTONDOWN: {
        InputEvent ev = {};
        ev.timestamp = Platform::GetTimerNanos();
        ev.device = InputDevice::Mouse;
        ev.type = InputEventType::MouseButtonDown;
        ev.value1 = e.button.button;   // SDL_BUTTON_LEFT 等
        ev.value2 = 0;
        ev.source = InputSource::SDL;
        history_.Push(ev);
        break;
    }

    case SDL_MOUSEBUTTONUP: {
        InputEvent ev = {};
        ev.timestamp = Platform::GetTimerNanos();
        ev.device = InputDevice::Mouse;
        ev.type = InputEventType::MouseButtonUp;
        ev.value1 = e.button.button;
        ev.value2 = 0;
        ev.source = InputSource::SDL;
        history_.Push(ev);
        break;
    }

    default:
        break;
    }
}

void InputSystem::EndFrame() {
    // 本帧瞬时状态已在 BeginFrame 清空。
    // 保留接口用于未来扩展。
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

uint64_t InputSystem::GetLastKeyPressTime(SDL_Scancode key) const {
    for (size_t i = history_.Size(); i > 0; i--) {
        const InputEvent& ev = history_.At(i - 1);
        if (ev.device == InputDevice::Keyboard
            && ev.type == InputEventType::KeyDown
            && ev.value1 == (int32_t)key) {
            return ev.timestamp;
        }
    }
    return 0;
}

uint64_t InputSystem::GetLastKeyReleaseTime(SDL_Scancode key) const {
    for (size_t i = history_.Size(); i > 0; i--) {
        const InputEvent& ev = history_.At(i - 1);
        if (ev.device == InputDevice::Keyboard
            && ev.type == InputEventType::KeyUp
            && ev.value1 == (int32_t)key) {
            return ev.timestamp;
        }
    }
    return 0;
}

bool InputSystem::WasKeyPressedSince(SDL_Scancode key, uint64_t sinceTimestamp) const {
    for (size_t i = 0; i < history_.Size(); i++) {
        const InputEvent& ev = history_.At(i);
        if (ev.timestamp < sinceTimestamp) continue;
        if (ev.device == InputDevice::Keyboard
            && ev.type == InputEventType::KeyDown
            && ev.value1 == (int32_t)key) {
            return true;
        }
    }
    return false;
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