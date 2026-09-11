#include "LightingPanel.h"
#include "../Core/EngineConfig.h"
#include <imgui.h>

void LightingPanel::Draw() {
    if (!config_) return;

    if (ImGui::CollapsingHeader("Lighting")) {
        float& timeOfDay = config_->lighting.timeOfDay;
        float& ambient = config_->lighting.ambientStrength;

        ImGui::SliderFloat("Time of Day", &timeOfDay, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
        if (ImGui::Button("Set Noon")) timeOfDay = 0.5f;
        ImGui::SameLine();
        if (ImGui::Button("Set Midnight")) timeOfDay = 0.0f;
        ImGui::SameLine();
        if (ImGui::Button("Set Dawn")) timeOfDay = 0.25f;
        ImGui::SameLine();
        if (ImGui::Button("Set Dusk")) timeOfDay = 0.75f;
    }
}