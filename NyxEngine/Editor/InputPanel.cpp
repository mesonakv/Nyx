#include "InputPanel.h"
#include "../Core/InputSystem.h"
#include "../Input/InputMap.h"
#include <imgui.h>
#include <SDL.h>

void InputPanel::Draw() {
    if (!input_) return;

    if (ImGui::CollapsingHeader("Input")) {
        bool available = input_->IsRawInputAvailable();
        bool usingRaw  = input_->IsUsingRawInput();
        bool captured  = input_->IsMouseCaptured();

        if (available) {
            bool value = usingRaw;
            if (ImGui::Checkbox("Use Raw Input", &value)) {
                input_->SetUseRawInput(value);
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(Raw Input available)");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Raw Input not available");
            ImGui::Text("Using SDL channel");
        }

        const auto& hist = input_->GetEventHistory();
        ImGui::Text("Total events: %llu", (unsigned long long)input_->GetTotalEventCount());
        ImGui::Text("History (250ms): %zu entries", hist.Size());
        ImGui::Text("Mouse captured: %s", captured ? "yes" : "no");
        ImGui::Text("Active channel: %s",
                    (usingRaw && captured) ? "Raw Input" : "SDL");
    }

    if (inputMap_ && ImGui::CollapsingHeader("Input Bindings")) {
        ImGui::TextDisabled("Edit config/input.json to rebind");
        ImGui::Separator();

        for (int i = 0; i < (int)Action::Count; i++) {
            Action a = (Action)i;
            const auto& binds = inputMap_->GetBindings(a);

            ImGui::Text("%s", ActionToString(a));
            ImGui::Indent();

            if (binds.empty()) {
                ImGui::TextDisabled("(unbound)");
            } else {
                for (const auto& b : binds) {
                    if (b.device == InputDevice::Keyboard) {
                        const char* name = SDL_GetScancodeName((SDL_Scancode)b.code);
                        ImGui::Text("Key: %s", name && name[0] ? name : "?");
                    } else if (b.device == InputDevice::Mouse) {
                        const char* name = "?";
                        switch (b.code) {
                            case SDL_BUTTON_LEFT:   name = "Left"; break;
                            case SDL_BUTTON_RIGHT:  name = "Right"; break;
                            case SDL_BUTTON_MIDDLE: name = "Middle"; break;
                            case SDL_BUTTON_X1:     name = "X1"; break;
                            case SDL_BUTTON_X2:     name = "X2"; break;
                        }
                        ImGui::Text("Mouse: %s", name);
                    } else {
                        ImGui::Text("Gamepad: %d", b.code);
                    }
                }
            }
            ImGui::Unindent();
        }
    }
}