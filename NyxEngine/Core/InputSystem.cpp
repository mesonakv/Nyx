#include "InputSystem.h"
#include "Platform.h"
#include "Logger.h"
#include "../Input/Win32InputBackend.h"
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
    totalEventCount_ = 0;

    // 尝试初始化 Raw Input 后端
#ifdef _WIN32
    auto backend = std::make_unique<Win32InputBackend>();
    bool ok = backend->Initialize(window, [this](const InputEvent& ev) {
        this->ProcessRawInputEvent(ev);
    });

    if (ok) {
        rawInputBackend_ = std::move(backend);
        useRawInput_ = true;
        NYX_LOG_INFO("InputSystem: using Raw Input channel");
    } else {
        rawInputBackend_.reset();
        useRawInput_ = false;
        NYX_LOG_INFO("InputSystem: falling back to SDL channel");
    }
#else
    NYX_LOG_INFO("InputSystem: using SDL channel (non-Windows platform)");
#endif
}

void InputSystem::Shutdown() {
    if (rawInputBackend_) {
        rawInputBackend_->Shutdown();
        rawInputBackend_.reset();
    }

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

bool InputSystem::IsRawInputActive() const {
    // Raw Input 只在鼠标被捕获时有效。
    // 编辑器模式下鼠标释放，Windows 不再把 WM_INPUT 送进窗口，
    // 此时必须回退到 SDL 通道。
    return useRawInput_ && mouseCaptured_;
}

void InputSystem::ProcessEvent(const SDL_Event& e) {
    const bool rawActive = IsRawInputActive();

    switch (e.type) {
    case SDL_QUIT:
        quitRequested_ = true;
        break;

    case SDL_KEYDOWN:
        // Raw Input 激活时忽略 SDL 键盘事件（ImGui 自己会用 SDL 通道）
        if (rawActive) break;
        if (!e.key.repeat) {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device    = InputDevice::Keyboard;
            ev.type      = InputEventType::KeyDown;
            ev.value1    = e.key.keysym.scancode;
            ev.value2    = 0;
            ev.source    = InputSource::SDL;
            ApplyEvent(ev);
        }
        break;

    case SDL_KEYUP:
        if (rawActive) break;
        {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device    = InputDevice::Keyboard;
            ev.type      = InputEventType::KeyUp;
            ev.value1    = e.key.keysym.scancode;
            ev.value2    = 0;
            ev.source    = InputSource::SDL;
            ApplyEvent(ev);
        }
        break;

    case SDL_MOUSEMOTION:
        if (rawActive) break;
        if (e.motion.xrel != 0 || e.motion.yrel != 0) {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device    = InputDevice::Mouse;
            ev.type      = InputEventType::MouseMove;
            ev.value1    = e.motion.xrel;
            ev.value2    = e.motion.yrel;
            ev.source    = InputSource::SDL;
            ApplyEvent(ev);
        }
        break;

    case SDL_MOUSEBUTTONDOWN:
        if (rawActive) break;
        {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device    = InputDevice::Mouse;
            ev.type      = InputEventType::MouseButtonDown;
            ev.value1    = e.button.button;
            ev.value2    = 0;
            ev.source    = InputSource::SDL;
            ApplyEvent(ev);
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (rawActive) break;
        {
            InputEvent ev = {};
            ev.timestamp = Platform::GetTimerNanos();
            ev.device    = InputDevice::Mouse;
            ev.type      = InputEventType::MouseButtonUp;
            ev.value1    = e.button.button;
            ev.value2    = 0;
            ev.source    = InputSource::SDL;
            ApplyEvent(ev);
        }
        break;

    default:
        break;
    }
}

void InputSystem::ProcessRawInputEvent(const InputEvent& ev) {
    if (!IsRawInputActive()) return;
    ApplyEvent(ev);
}

void InputSystem::ApplyEvent(const InputEvent& ev) {
    totalEventCount_++;
    history_.Push(ev);

    if (ev.device == InputDevice::Keyboard) {
        SDL_Scancode sc = (SDL_Scancode)ev.value1;
        if (sc < 0 || sc >= kMaxKeys) return;

        if (ev.type == InputEventType::KeyDown) {
            keysDown_[sc] = true;
            keysPressedThisFrame_[sc] = true;
        } else if (ev.type == InputEventType::KeyUp) {
            keysDown_[sc] = false;
            keysReleasedThisFrame_[sc] = true;
        }
    } else if (ev.device == InputDevice::Mouse) {
        if (ev.type == InputEventType::MouseMove) {
            mouseDeltaX_ += (float)ev.value1;
            mouseDeltaY_ += (float)ev.value2;
        }
    }
}

void InputSystem::EndFrame() {
    // 保留接口
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

void InputSystem::SetUseRawInput(bool use) {
    if (!rawInputBackend_) {
        return;
    }
    if (useRawInput_ == use) return;

    useRawInput_ = use;

    // 切换时清空状态，避免"SDL 按下、Raw Input 释放"这种错位
    std::memset(keysDown_, 0, sizeof(keysDown_));
    std::memset(keysPressedThisFrame_, 0, sizeof(keysPressedThisFrame_));
    std::memset(keysReleasedThisFrame_, 0, sizeof(keysReleasedThisFrame_));
    mouseDeltaX_ = 0.0f;
    mouseDeltaY_ = 0.0f;
    history_.Clear();
    // totalEventCount_ 不清零

    NYX_LOG_INFO("InputSystem: switched to %s channel", use ? "Raw Input" : "SDL");
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