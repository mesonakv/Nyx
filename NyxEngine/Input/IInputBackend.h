#pragma once
#include "InputEvent.h"
#include <functional>

// ============ IInputBackend ============
//
// 输入后端接口。每个平台实现一个。
//
// 目前只有 Win32InputBackend（Raw Input）。
// 未来会加：
//   - PS5InputBackend（索尼 SDK）
//   - XboxInputBackend（GDK）
//   - AndroidInputBackend（NDK）
//   - LinuxInputBackend（evdev / libinput）
//
// 设计原则：
//   - 输入后端只负责"采集原始事件"，不做去重、不做时间戳统一
//   - 时间戳由后端在采集时打上（设备延迟最低）
//   - 通过 EventSink 回调把事件推给 InputSystem
//   - 输入后端不理解"Action"，那是 InputMap 的事

class IInputBackend {
public:
    using EventSink = std::function<void(const InputEvent&)>;

    virtual ~IInputBackend() = default;

    // nativeWindowHandle：平台相关的窗口句柄
    //   - Windows: SDL_Window*（后端自己从里面取 HWND）
    //   - PS5: 平台自己的句柄
    //   - 传入 void* 是为了让接口不依赖任何平台头文件
    //
    // 返回 true 表示初始化成功，false 表示失败（调用者应 fallback）
    virtual bool Initialize(void* nativeWindowHandle, EventSink sink) = 0;

    virtual void Shutdown() = 0;
};