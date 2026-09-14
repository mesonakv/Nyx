#include "Win32InputBackend.h"

#ifdef _WIN32

#include <SDL.h>
#include <SDL_syswm.h>

#include "../Core/Platform.h"
#include "../Core/Logger.h"

// ============ VK → SDL_Scancode 映射 ============
//
// Windows 虚拟键码（VK_*）到 SDL_Scancode 的映射。
// 覆盖标准 PC 键盘的全部按键。
//
// SDL 的 scancode 基于 USB HID Usage Table，
// Windows 的 VK 是历史遗留的虚拟键码。
// 两者不是一一对应，必须手写映射。

static SDL_Scancode VKToSDLScancode(UINT vk) {
    // 字母 A-Z
    if (vk >= 'A' && vk <= 'Z') {
        return (SDL_Scancode)(SDL_SCANCODE_A + (vk - 'A'));
    }

    // 数字 0-9
    if (vk >= '0' && vk <= '9') {
        return (SDL_Scancode)(SDL_SCANCODE_0 + (vk - '0'));
    }

    // F1-F12
    if (vk >= VK_F1 && vk <= VK_F12) {
        return (SDL_Scancode)(SDL_SCANCODE_F1 + (vk - VK_F1));
    }

    // 小键盘 0-9
    // 注意：SDL 的 KP_0 ~ KP_9 scancode 不是线性的，
    // KP_0=98，KP_1=89，KP_2=90，...，KP_9=97
    // 必须逐一映射，不能用偏移量。
    switch (vk) {
    case VK_NUMPAD0: return SDL_SCANCODE_KP_0;
    case VK_NUMPAD1: return SDL_SCANCODE_KP_1;
    case VK_NUMPAD2: return SDL_SCANCODE_KP_2;
    case VK_NUMPAD3: return SDL_SCANCODE_KP_3;
    case VK_NUMPAD4: return SDL_SCANCODE_KP_4;
    case VK_NUMPAD5: return SDL_SCANCODE_KP_5;
    case VK_NUMPAD6: return SDL_SCANCODE_KP_6;
    case VK_NUMPAD7: return SDL_SCANCODE_KP_7;
    case VK_NUMPAD8: return SDL_SCANCODE_KP_8;
    case VK_NUMPAD9: return SDL_SCANCODE_KP_9;
    default: break;
    }

    switch (vk) {
    case VK_SPACE:     return SDL_SCANCODE_SPACE;
    case VK_RETURN:    return SDL_SCANCODE_RETURN;
    case VK_ESCAPE:    return SDL_SCANCODE_ESCAPE;
    case VK_TAB:       return SDL_SCANCODE_TAB;
    case VK_BACK:      return SDL_SCANCODE_BACKSPACE;
    case VK_DELETE:    return SDL_SCANCODE_DELETE;
    case VK_INSERT:    return SDL_SCANCODE_INSERT;
    case VK_HOME:      return SDL_SCANCODE_HOME;
    case VK_END:       return SDL_SCANCODE_END;
    case VK_PRIOR:     return SDL_SCANCODE_PAGEUP;
    case VK_NEXT:      return SDL_SCANCODE_PAGEDOWN;

    case VK_LEFT:      return SDL_SCANCODE_LEFT;
    case VK_RIGHT:     return SDL_SCANCODE_RIGHT;
    case VK_UP:        return SDL_SCANCODE_UP;
    case VK_DOWN:      return SDL_SCANCODE_DOWN;

    case VK_LSHIFT:    return SDL_SCANCODE_LSHIFT;
    case VK_RSHIFT:    return SDL_SCANCODE_RSHIFT;
    case VK_LCONTROL:  return SDL_SCANCODE_LCTRL;
    case VK_RCONTROL:  return SDL_SCANCODE_RCTRL;
    case VK_LMENU:     return SDL_SCANCODE_LALT;
    case VK_RMENU:     return SDL_SCANCODE_RALT;
    case VK_LWIN:      return SDL_SCANCODE_LGUI;
    case VK_RWIN:      return SDL_SCANCODE_RGUI;

    case VK_CAPITAL:   return SDL_SCANCODE_CAPSLOCK;
    case VK_NUMLOCK:   return SDL_SCANCODE_NUMLOCKCLEAR;
    case VK_SCROLL:    return SDL_SCANCODE_SCROLLLOCK;
    case VK_SNAPSHOT:  return SDL_SCANCODE_PRINTSCREEN;
    case VK_PAUSE:     return SDL_SCANCODE_PAUSE;
    case VK_APPS:      return SDL_SCANCODE_APPLICATION;

    case VK_OEM_1:     return SDL_SCANCODE_SEMICOLON;      // ;:
    case VK_OEM_PLUS:  return SDL_SCANCODE_EQUALS;         // =+
    case VK_OEM_COMMA: return SDL_SCANCODE_COMMA;          // ,<
    case VK_OEM_MINUS: return SDL_SCANCODE_MINUS;          // -_
    case VK_OEM_PERIOD:return SDL_SCANCODE_PERIOD;         // .>
    case VK_OEM_2:     return SDL_SCANCODE_SLASH;          // /?
    case VK_OEM_3:     return SDL_SCANCODE_GRAVE;          // `~
    case VK_OEM_4:     return SDL_SCANCODE_LEFTBRACKET;    // [{
    case VK_OEM_5:     return SDL_SCANCODE_BACKSLASH;      // \|
    case VK_OEM_6:     return SDL_SCANCODE_RIGHTBRACKET;   // ]}
    case VK_OEM_7:     return SDL_SCANCODE_APOSTROPHE;     // '"

    case VK_MULTIPLY:  return SDL_SCANCODE_KP_MULTIPLY;
    case VK_ADD:       return SDL_SCANCODE_KP_PLUS;
    case VK_SUBTRACT:  return SDL_SCANCODE_KP_MINUS;
    case VK_DECIMAL:   return SDL_SCANCODE_KP_PERIOD;
    case VK_DIVIDE:    return SDL_SCANCODE_KP_DIVIDE;

    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}

// ============ 生命周期 ============

Win32InputBackend::~Win32InputBackend() {
    if (initialized_) {
        Shutdown();
    }
}

bool Win32InputBackend::Initialize(void* nativeWindowHandle, EventSink sink) {
    if (initialized_) {
        NYX_LOG_WARN("Win32InputBackend: already initialized");
        return true;
    }

    window_ = nativeWindowHandle;
    sink_ = std::move(sink);

    // 从 SDL_Window 取 HWND
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo((SDL_Window*)window_, &wmInfo)) {
        NYX_LOG_ERROR("Win32InputBackend: SDL_GetWindowWMInfo failed: %s", SDL_GetError());
        return false;
    }
    if (wmInfo.subsystem != SDL_SYSWM_WINDOWS) {
        NYX_LOG_ERROR("Win32InputBackend: not a Windows window");
        return false;
    }
    hwnd_ = wmInfo.info.win.window;

    // 读系统"主鼠标按钮"设置。
    // SM_SWAPBUTTON 非零表示系统把右键设为主键。
    // Raw Input 走硬件层，系统不自动交换，需要手动应用。
    // 只读一次，之后静态使用。
    swapButtons_ = (GetSystemMetrics(SM_SWAPBUTTON) != 0);
    NYX_LOG_INFO("Win32InputBackend: swap buttons = %s",
                 swapButtons_ ? "yes" : "no");

    // 注册 Raw Input 设备
    // dwFlags = 0：不设 RIDEV_NOLEGACY，保留 legacy 消息给 SDL/ImGui
    RAWINPUTDEVICE rid[2] = {};

    rid[0].usUsagePage = 0x01;   // Generic Desktop Controls
    rid[0].usUsage     = 0x06;   // Keyboard
    rid[0].dwFlags     = 0;
    rid[0].hwndTarget  = hwnd_;

    rid[1].usUsagePage = 0x01;
    rid[1].usUsage     = 0x02;   // Mouse
    rid[1].dwFlags     = 0;
    rid[1].hwndTarget  = hwnd_;

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
        NYX_LOG_ERROR("Win32InputBackend: RegisterRawInputDevices failed (error %lu)", GetLastError());
        return false;
    }

    // 安装 SDL 消息 hook
    SDL_SetWindowsMessageHook(&Win32InputBackend::MessageHook, this);

    initialized_ = true;
    NYX_LOG_INFO("Win32InputBackend initialized (Raw Input, IME disabled)");
    return true;
}

void Win32InputBackend::Shutdown() {
    if (!initialized_) return;

    // 移除 SDL 消息 hook
    SDL_SetWindowsMessageHook(nullptr, nullptr);

    // 注销 Raw Input 设备
    RAWINPUTDEVICE rid[2] = {};
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage     = 0x06;
    rid[0].dwFlags     = RIDEV_REMOVE;
    rid[0].hwndTarget  = nullptr;

    rid[1].usUsagePage = 0x01;
    rid[1].usUsage     = 0x02;
    rid[1].dwFlags     = RIDEV_REMOVE;
    rid[1].hwndTarget  = nullptr;

    RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));

    // 不恢复 IME——窗口销毁时系统自动清理。

    hwnd_ = nullptr;
    window_ = nullptr;
    sink_ = nullptr;
    initialized_ = false;

    NYX_LOG_INFO("Win32InputBackend shutdown");
}

// ============ 消息 hook ============

void SDLCALL Win32InputBackend::MessageHook(
    void* userdata,
    void* hWnd,
    unsigned int message,
    Uint64 wParam,
    Sint64 lParam)
{
    auto* self = static_cast<Win32InputBackend*>(userdata);
    if (!self) return;

    if (message == WM_INPUT) {
        // lParam 是 HRAWINPUT，从 Sint64 强转回指针
        self->HandleRawInput((void*)(HRAWINPUT)lParam);
    }
}

// ============ Raw Input 解析 ============

void Win32InputBackend::HandleRawInput(void* hRawInput) {
    UINT size = 0;
    if (GetRawInputData((HRAWINPUT)hRawInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0) {
        return;
    }
    if (size == 0 || size > 256) return;

    BYTE buffer[256];
    UINT read = GetRawInputData((HRAWINPUT)hRawInput, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));
    if (read != size) return;

    RAWINPUT* raw = (RAWINPUT*)buffer;

    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        ParseKeyboard(raw->data.keyboard);
    } else if (raw->header.dwType == RIM_TYPEMOUSE) {
        ParseMouse(raw->data.mouse);
    }
}

void Win32InputBackend::ParseKeyboard(const RAWKEYBOARD& kb) {
    if (kb.VKey == 0xFF) return;

    SDL_Scancode scancode = VKToSDLScancode(kb.VKey);
    if (scancode == SDL_SCANCODE_UNKNOWN) return;

    bool isDown = (kb.Flags & RI_KEY_BREAK) == 0;

    InputEvent ev = {};
    ev.timestamp = Platform::GetTimerNanos();
    ev.device    = InputDevice::Keyboard;
    ev.type      = isDown ? InputEventType::KeyDown : InputEventType::KeyUp;
    ev.value1    = (int32_t)scancode;
    ev.value2    = 0;
    ev.source    = InputSource::RawInput;

    if (sink_) sink_(ev);
}

void Win32InputBackend::ParseMouse(const RAWMOUSE& mouse) {
    uint64_t ts = Platform::GetTimerNanos();

    // ---------- 移动 ----------
    if (mouse.lLastX != 0 || mouse.lLastY != 0) {
        InputEvent ev = {};
        ev.timestamp = ts;
        ev.device    = InputDevice::Mouse;
        ev.type      = InputEventType::MouseMove;
        ev.value1    = mouse.lLastX;
        ev.value2    = mouse.lLastY;
        ev.source    = InputSource::RawInput;

        if (sink_) sink_(ev);
    }

    // ---------- 按钮 ----------
    USHORT flags = mouse.usButtonFlags;

    struct ButtonMapping {
        USHORT downFlag;
        USHORT upFlag;
        int    sdlButton;
    };

    // 物理按键 1 和 2 的语义根据系统主按键设置交换。
    // swapButtons_ = false：1=左键，2=右键
    // swapButtons_ = true ：1=右键，2=左键
    // 中间键、侧键不交换。
    const int btn1 = swapButtons_ ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
    const int btn2 = swapButtons_ ? SDL_BUTTON_LEFT  : SDL_BUTTON_RIGHT;

    const ButtonMapping kButtonMap[] = {
        { RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP, btn1              },
        { RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP, btn2              },
        { RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP, SDL_BUTTON_MIDDLE },
        { RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, SDL_BUTTON_X1     },
        { RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, SDL_BUTTON_X2     },
    };

    for (const auto& m : kButtonMap) {
        if (flags & m.downFlag) {
            InputEvent ev = {};
            ev.timestamp = ts;
            ev.device    = InputDevice::Mouse;
            ev.type      = InputEventType::MouseButtonDown;
            ev.value1    = m.sdlButton;
            ev.value2    = 0;
            ev.source    = InputSource::RawInput;
            if (sink_) sink_(ev);
        }
        if (flags & m.upFlag) {
            InputEvent ev = {};
            ev.timestamp = ts;
            ev.device    = InputDevice::Mouse;
            ev.type      = InputEventType::MouseButtonUp;
            ev.value1    = m.sdlButton;
            ev.value2    = 0;
            ev.source    = InputSource::RawInput;
            if (sink_) sink_(ev);
        }
    }

    // 滚轮暂不处理
}

#endif // _WIN32