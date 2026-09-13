#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../Scene/Material.h"
#include "LightingSystem.h"

class ImGuiManager;

// ============ FrameData ============
//
// 每帧渲染所需的所有数据。
// 由 MyGame 构建，传给 Renderer::DrawFrame。
//
// 指针字段约定：调用者保证非空，且生命周期覆盖本帧。

struct FrameData {
    // ---------- Camera ----------
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    glm::vec3 cameraPos = glm::vec3(0.0f);

    // ---------- Scene ----------
    // 目标位置、缩放、材质索引三个数组等长
    const std::vector<glm::vec3>* targetPositions = nullptr;
    const std::vector<float>* targetScales = nullptr;
    const std::vector<int>* targetMaterialIndices = nullptr;

    // 材质库。用索引查具体材质，避免每帧拷贝 Material
    const MaterialLibrary* materialLibrary = nullptr;

    // ---------- Lighting ----------
    LightingData lighting;

    // ---------- ImGui ----------
    ImGuiManager* imgui = nullptr;
};