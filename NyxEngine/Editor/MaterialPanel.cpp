#include "MaterialPanel.h"
#include "../Scene/Material.h"
#include "../../Game/Target/TargetManager.h"
#include <imgui.h>

void MaterialPanel::Draw() {
    if (!materials_ || !targets_) return;

    if (ImGui::CollapsingHeader("Material")) {
        if (ImGui::BeginCombo("Preset", materials_->GetSelected().name.c_str())) {
            for (int i = 0; i < (int)materials_->materials.size(); i++) {
                bool selected = (materials_->selectedIndex == i);
                if (ImGui::Selectable(materials_->materials[i].name.c_str(), selected)) {
                    materials_->selectedIndex = i;
                    targets_->currentMaterialIndex = i;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        Material& mat = materials_->GetSelected();
        ImGui::ColorEdit4("Color", &mat.color.x);
        ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Emissive", &mat.emissive_strength, 0.0f, 5.0f);
        ImGui::SliderFloat("Opacity", &mat.opacity, 0.0f, 1.0f);
        ImGui::SliderFloat("Reflectance", &mat.reflectance, 0.0f, 1.0f);

        if (ImGui::Button("Apply Material to All Targets")) {
            for (auto& t : targets_->targets) t.materialIndex = materials_->selectedIndex;
        }
        if (ImGui::Button("Reset Targets")) {
            targets_->currentMaterialIndex = materials_->selectedIndex;
            targets_->Spawn();
        }
    }
}