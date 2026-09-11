#include "TargetMovementPanel.h"
#include "../../Game/Target/TargetManager.h"
#include <imgui.h>

void TargetMovementPanel::Draw() {
    if (!targets_) return;

    if (ImGui::CollapsingHeader("Target Movement")) {
        const char* movementNames[] = {"Static", "Linear", "Sine Wave", "Random Direction"};
        int currentMovement = (int)targets_->movementType;
        if (ImGui::Combo("Type", &currentMovement, movementNames, 4)) {
            targets_->movementType = (MovementType)currentMovement;
            targets_->Spawn();
        }
        ImGui::SliderFloat("Speed", &targets_->moveSpeed, 0.0f, 10.0f);
        if (targets_->movementType == MovementType::SineWave) {
            ImGui::SliderFloat("Amplitude", &targets_->sineAmplitude, 0.1f, 5.0f);
            ImGui::SliderFloat("Frequency", &targets_->sineFrequency, 0.1f, 5.0f);
        } else if (targets_->movementType == MovementType::RandomDirection) {
            ImGui::SliderFloat("Change Interval", &targets_->randomChangeInterval, 0.5f, 5.0f);
        }
    }
}