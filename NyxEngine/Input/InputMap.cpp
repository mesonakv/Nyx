#include "InputMap.h"
#include "InputSystem.h"
#include "../Core/Logger.h"
#include "../Core/FileSystem.h"
#include "../Core/JsonValue.h"
#include <SDL.h>

// ============ 默认绑定表 ============

namespace {

struct BindingDef {
    Action action;
    InputDevice device;
    int code;
};

const BindingDef kDefaultBindings[] = {
    { Action::MoveForward,   InputDevice::Keyboard, SDL_SCANCODE_W },
    { Action::MoveBackward,  InputDevice::Keyboard, SDL_SCANCODE_S },
    { Action::MoveLeft,      InputDevice::Keyboard, SDL_SCANCODE_A },
    { Action::MoveRight,     InputDevice::Keyboard, SDL_SCANCODE_D },
    { Action::Jump,          InputDevice::Keyboard, SDL_SCANCODE_SPACE },
    { Action::Dash,          InputDevice::Keyboard, SDL_SCANCODE_LSHIFT },
    { Action::Slide,         InputDevice::Keyboard, SDL_SCANCODE_LCTRL },
    { Action::PrimaryFire,   InputDevice::Mouse,    SDL_BUTTON_LEFT },
    { Action::SecondaryFire, InputDevice::Mouse,    SDL_BUTTON_RIGHT },
    { Action::ToggleEditor,  InputDevice::Keyboard, SDL_SCANCODE_F1 },
    { Action::Pause,         InputDevice::Keyboard, SDL_SCANCODE_ESCAPE },
};

const char* DeviceToString(InputDevice d) {
    switch (d) {
    case InputDevice::Keyboard: return "keyboard";
    case InputDevice::Mouse:    return "mouse";
    case InputDevice::Gamepad:  return "gamepad";
    }
    return "unknown";
}

InputDevice StringToDevice(const std::string& s) {
    if (s == "mouse") return InputDevice::Mouse;
    if (s == "gamepad") return InputDevice::Gamepad;
    return InputDevice::Keyboard;
}

} // anonymous namespace

// ============ 生命周期 ============

void InputMap::Initialize() {
    LoadDefaults();
}

void InputMap::Clear() {
    for (auto& v : bindings_) v.clear();
    down_.fill(false);
    pressed_.fill(false);
    released_.fill(false);
}

// ============ 每帧更新 ============

void InputMap::Update(const InputSystem& input) {
    for (size_t i = 0; i < kActionCount; i++) {
        bool down = false;
        bool pressed = false;
        bool released = false;

        for (const auto& b : bindings_[i]) {
            if (IsBindingDown(b, input))     down = true;
            if (WasBindingPressed(b, input)) pressed = true;
            if (WasBindingReleased(b, input)) released = true;
        }

        down_[i] = down;
        pressed_[i] = pressed;
        released_[i] = released;
    }
}

// ============ 查询 ============

bool InputMap::IsActionDown(Action action) const {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return false;
    return down_[i];
}

bool InputMap::WasActionPressed(Action action) const {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return false;
    return pressed_[i];
}

bool InputMap::WasActionReleased(Action action) const {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return false;
    return released_[i];
}

// ============ 内部：绑定状态查询 ============

bool InputMap::IsBindingDown(const InputBinding& b, const InputSystem& input) const {
    switch (b.device) {
    case InputDevice::Keyboard:
        return input.IsKeyDown((SDL_Scancode)b.code);
    case InputDevice::Mouse:
        return input.IsMouseButtonDown(b.code);
    case InputDevice::Gamepad:
        return false;   // 预留
    }
    return false;
}

bool InputMap::WasBindingPressed(const InputBinding& b, const InputSystem& input) const {
    switch (b.device) {
    case InputDevice::Keyboard:
        return input.WasKeyPressed((SDL_Scancode)b.code);
    case InputDevice::Mouse:
        return input.WasMouseButtonPressed(b.code);
    case InputDevice::Gamepad:
        return false;
    }
    return false;
}

bool InputMap::WasBindingReleased(const InputBinding& b, const InputSystem& input) const {
    switch (b.device) {
    case InputDevice::Keyboard:
        return input.WasKeyReleased((SDL_Scancode)b.code);
    case InputDevice::Mouse:
        return input.WasMouseButtonReleased(b.code);
    case InputDevice::Gamepad:
        return false;
    }
    return false;
}

// ============ 绑定管理 ============

void InputMap::LoadDefaults() {
    for (auto& v : bindings_) v.clear();

    for (const auto& def : kDefaultBindings) {
        InputBinding b;
        b.device = def.device;
        b.code = def.code;
        bindings_[static_cast<size_t>(def.action)].push_back(b);
    }
}

void InputMap::Bind(Action action, InputDevice device, int code) {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return;

    InputBinding b{device, code};
    auto& list = bindings_[i];

    for (const auto& existing : list) {
        if (existing == b) return;   // 已存在
    }
    list.push_back(b);
}

void InputMap::Unbind(Action action, InputDevice device, int code) {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return;

    InputBinding target{device, code};
    auto& list = bindings_[i];
    for (auto it = list.begin(); it != list.end(); ) {
        if (*it == target) {
            it = list.erase(it);
        } else {
            ++it;
        }
    }
}

void InputMap::ClearBindings(Action action) {
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return;
    bindings_[i].clear();
}

const std::vector<InputBinding>& InputMap::GetBindings(Action action) const {
    static const std::vector<InputBinding> kEmpty;
    size_t i = static_cast<size_t>(action);
    if (i >= kActionCount) return kEmpty;
    return bindings_[i];
}

// ============ JSON ============

bool InputMap::LoadFromJson(const std::string& path) {
    std::string text = FileSystem::ReadText(path);
    if (text.empty()) {
        NYX_LOG_WARN("InputMap: cannot read '%s', using defaults", path.c_str());
        LoadDefaults();
        return false;
    }

    std::string err;
    JsonValue root = JsonValue::Parse(text, &err);
    if (!err.empty()) {
        NYX_LOG_ERROR("InputMap: JSON parse error: %s", err.c_str());
        LoadDefaults();
        return false;
    }

    const JsonValue& bindings = root["bindings"];
    if (!bindings.IsObject()) {
        NYX_LOG_WARN("InputMap: no 'bindings' object, using defaults");
        LoadDefaults();
        return false;
    }

    // 清空所有绑定（如果文件里有，就全部替换）
    for (auto& v : bindings_) v.clear();

    size_t loadedCount = 0;
    for (const auto& key : bindings.GetKeys()) {
        Action action;
        if (!StringToAction(key.c_str(), action)) {
            NYX_LOG_WARN("InputMap: unknown action '%s', skipping", key.c_str());
            continue;
        }

        const JsonValue& arr = bindings[key];
        if (!arr.IsArray()) continue;

        size_t idx = static_cast<size_t>(action);
        for (size_t i = 0; i < arr.Size(); i++) {
            const JsonValue& item = arr[i];
            std::string devStr = item["device"].AsString("keyboard");
            int code = item["code"].AsInt(0);

            InputBinding b;
            b.device = StringToDevice(devStr);
            b.code = code;
            bindings_[idx].push_back(b);
            loadedCount++;
        }
    }

    NYX_LOG_INFO("InputMap: loaded %zu bindings from '%s'", loadedCount, path.c_str());
    return true;
}

bool InputMap::SaveToJson(const std::string& path) const {
    JsonValue root;

    JsonValue& bindings = root["bindings"];
    bindings.ClearAsObject();

    for (size_t i = 0; i < kActionCount; i++) {
        const auto& list = bindings_[i];
        if (list.empty()) continue;

        Action action = static_cast<Action>(i);
        const char* name = ActionToString(action);

        JsonValue& arr = bindings[name];
        arr.ClearAsArray();

        for (const auto& b : list) {
            JsonValue item;
            item["device"] = DeviceToString(b.device);
            item["code"] = b.code;
            arr.Push(item);
        }
    }

    std::string text = root.ToString(true);
    if (!FileSystem::WriteText(path, text)) {
        NYX_LOG_ERROR("InputMap: failed to write '%s'", path.c_str());
        return false;
    }

    NYX_LOG_INFO("InputMap: saved to '%s'", path.c_str());
    return true;
}