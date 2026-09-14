#include "EditorPanel.h"
#include "../Core/InputSystem.h"
#include "../Input/InputMap.h"
#include "../World/Player.h"
#include <imgui.h>

void EditorPanel::Initialize(const Context& ctx) {
    if (ctx.camera) cameraPanel_.Initialize(*ctx.camera);
    if (ctx.materials && ctx.targets) materialPanel_.Initialize(*ctx.materials, *ctx.targets);
    if (ctx.targets) targetMovementPanel_.Initialize(*ctx.targets);
    if (ctx.displayModes && ctx.pendingSettings && ctx.pendingDisplayChange) {
        displayPanel_.Initialize(*ctx.displayModes, *ctx.pendingSettings, *ctx.pendingDisplayChange);
    }
    if (ctx.config) lightingPanel_.Initialize(*ctx.config);
    if (ctx.frameTimes) performancePanel_.Initialize(*ctx.frameTimes);
    if (ctx.input && ctx.inputMap) inputPanel_.Initialize(*ctx.input, *ctx.inputMap);
    if (ctx.player) playerPanel_.Initialize(*ctx.player);
}

void EditorPanel::Draw() {
    ImGui::Begin("Nyx Editor");
    cameraPanel_.Draw();
    materialPanel_.Draw();
    targetMovementPanel_.Draw();
    playerPanel_.Draw();
    displayPanel_.Draw();
    lightingPanel_.Draw();
    performancePanel_.Draw();
    inputPanel_.Draw();
    ImGui::End();
}