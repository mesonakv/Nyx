#pragma once
#include "Action.h"
#include "InputBinding.h"
#include <array>
#include <vector>
#include <string>

class InputSystem;

// ============ InputMap ============
//
// 把原始输入翻译成语义 Action。
//
// 数据流：
//   InputSystem（原始按键状态）
//     ↓
//   InputMap::Update()（翻译）
//     ↓
//   GameLogic 查询 IsActionDown(Action::Jump)
//
// 一个 Action 可以有多个绑定：
//   Jump → [空格] [鼠标侧键]
//   任意一个按下就算 Action 按下。
//
// 配置持久化：JSON 文件，路径由 Game 层提供。

class InputMap {
public:
    void Initialize();
    void Clear();

    // 每帧调用：在事件处理之后、Game Update 之前
    void Update(const InputSystem& input);

    // ---------- 查询 ----------
    bool IsActionDown(Action action) const;
    bool WasActionPressed(Action action) const;
    bool WasActionReleased(Action action) const;

    // ---------- 绑定管理 ----------
    void LoadDefaults();

    void Bind(Action action, InputDevice device, int code);
    void Unbind(Action action, InputDevice device, int code);
    void ClearBindings(Action action);

    const std::vector<InputBinding>& GetBindings(Action action) const;

    // ---------- JSON ----------
    bool LoadFromJson(const std::string& path);
    bool SaveToJson(const std::string& path) const;

private:
    static constexpr size_t kActionCount = static_cast<size_t>(Action::Count);

    std::array<std::vector<InputBinding>, kActionCount> bindings_;

    // 每帧快照
    std::array<bool, kActionCount> down_{};
    std::array<bool, kActionCount> pressed_{};
    std::array<bool, kActionCount> released_{};

    bool IsBindingDown(const InputBinding& b, const InputSystem& input) const;
    bool WasBindingPressed(const InputBinding& b, const InputSystem& input) const;
    bool WasBindingReleased(const InputBinding& b, const InputSystem& input) const;
};