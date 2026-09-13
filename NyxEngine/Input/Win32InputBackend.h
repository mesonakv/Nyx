#pragma once
#include "IInputBackend.h"

// 必须先 include SDL.h 再用 SDLCALL
#include <SDL.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

// ============ Win32InputBackend ============
//
// Windows 原始输入后端。
//
// 用 Raw Input API（WM_INPUT）绕过 Windows 消息队列，
// 获得比 SDL 事件更低延迟的键盘/鼠标数据。
//
// 与 SDL 共存：
//   - 通过 SDL_SetWindowsMessageHook 拦截 WM_INPUT
//   - 不设置 RIDEV_NOLEGACY，SDL 仍能收到 legacy 事件（ImGui 需要）
//   - 事件通过 EventSink 推给 InputSystem
//
// 生命周期：
//   - Initialize 时注册设备、安装消息 hook
//   - Shutdown 时注销设备、移除 hook

class Win32InputBackend : public IInputBackend {
public:
    Win32InputBackend() = default;
    ~Win32InputBackend() override;

    Win32InputBackend(const Win32InputBackend&) = delete;
    Win32InputBackend& operator=(const Win32InputBackend&) = delete;

    bool Initialize(void* nativeWindowHandle, EventSink sink) override;
    void Shutdown() override;

private:
    // SDL 消息 hook 的静态回调
    // 返回类型是 SDL_bool（int），不是 C++ bool
#ifdef _WIN32
    static void SDLCALL MessageHook(
        void* userdata,
        void* hWnd,
        unsigned int message,
        Uint64 wParam,
        Sint64 lParam);
#endif

    // 处理单个 WM_INPUT 消息
    void HandleRawInput(void* hRawInput);

#ifdef _WIN32
    // 解析键盘原始数据
    void ParseKeyboard(const RAWKEYBOARD& kb);

    // 解析鼠标原始数据
    void ParseMouse(const RAWMOUSE& mouse);
#endif

    void* window_ = nullptr;   // SDL_Window*
    EventSink sink_;
    bool initialized_ = false;

#ifdef _WIN32
    HWND hwnd_ = nullptr;
#endif
};