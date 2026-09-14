#include "PlayerPanel.h"
#include "../World/Player.h"
#include <imgui.h>

void PlayerPanel::Draw() {
    if (!player_) return;

    if (ImGui::CollapsingHeader("Player Movement")) {
        ImGui::Text("Horizontal");
        ImGui::SliderFloat("Max Speed", &player_->maxSpeed, 1.0f, 20.0f, "%.1f m/s");
        ImGui::SliderFloat("Ground Accel", &player_->groundAccel, 5.0f, 200.0f, "%.0f m/s²");
        ImGui::SliderFloat("Ground Friction", &player_->groundFriction, 0.0f, 30.0f, "%.1f");
        ImGui::SliderFloat("Air Accel", &player_->airAccel, 1.0f, 100.0f, "%.0f m/s²");

        ImGui::Separator();
        ImGui::Text("Jump");
        ImGui::SliderFloat("Jump Speed", &player_->jumpSpeed, 1.0f, 20.0f, "%.1f m/s");
        ImGui::SliderFloat("Gravity", &player_->gravity, 5.0f, 60.0f, "%.1f m/s²");
        ImGui::SliderFloat("Jump Cut", &player_->jumpCutMultiplier, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Coyote Time", &player_->coyoteTime, 0.0f, 0.3f, "%.3f s");
        ImGui::SliderFloat("Jump Buffer", &player_->jumpBuffer, 0.0f, 0.3f, "%.3f s");

        ImGui::Separator();
        ImGui::Text("Status");
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                    player_->position.x, player_->position.y, player_->position.z);
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)",
                    player_->velocity.x, player_->velocity.y, player_->velocity.z);
        ImGui::Text("Horizontal Speed: %.2f m/s",
                    std::sqrt(player_->velocity.x * player_->velocity.x +
                              player_->velocity.z * player_->velocity.z));
        ImGui::Text("On Ground: %s", player_->onGround ? "yes" : "no");

        if (ImGui::Button("Reset Player")) {
            player_->position = glm::vec3(0.0f, -2.0f, 8.0f);
            player_->velocity = glm::vec3(0.0f);
        }
    }
}