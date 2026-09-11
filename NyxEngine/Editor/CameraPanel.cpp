#include "CameraPanel.h"
#include "../Scene/Camera.h"
#include <imgui.h>

void CameraPanel::Draw() {
    if (!camera_) return;
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat("Sensitivity", &camera_->sensitivity, 0.0001f, 0.01f, "%.5f");
        ImGui::SliderFloat("FOV", &camera_->fov, 30.0f, 120.0f);
    }
}