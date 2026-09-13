#include "InputPanel.h"
#include "../Core/InputSystem.h"
#include <imgui.h>

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
}