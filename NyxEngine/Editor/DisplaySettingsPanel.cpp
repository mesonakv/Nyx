#include "DisplaySettingsPanel.h"
#include "../Core/VulkanContext.h"
#include <imgui.h>
#include <algorithm>
#include <string>

void DisplaySettingsPanel::Initialize(const std::vector<SDL_DisplayMode>& displayModes,
                                       DisplaySettings& pendingSettings,
                                       bool& pendingDisplayChange) {
    displayModes_ = &displayModes;
    pendingSettings_ = &pendingSettings;
    pendingDisplayChange_ = &pendingDisplayChange;

    // 构建去重分辨率列表
    uniqueResolutions_.clear();
    for (auto& m : displayModes) {
        auto res = std::make_pair(m.w, m.h);
        if (std::find(uniqueResolutions_.begin(), uniqueResolutions_.end(), res) == uniqueResolutions_.end()) {
            uniqueResolutions_.push_back(res);
        }
    }

    UpdateRefreshRates(pendingSettings.width, pendingSettings.height);
}

void DisplaySettingsPanel::UpdateRefreshRates(int width, int height) {
    currentRefreshRates_.clear();
    for (auto& m : *displayModes_) {
        if (m.w == width && m.h == height) {
            if (std::find(currentRefreshRates_.begin(), currentRefreshRates_.end(), m.refresh_rate) == currentRefreshRates_.end()) {
                currentRefreshRates_.push_back(m.refresh_rate);
            }
        }
    }
}

void DisplaySettingsPanel::Draw() {
    if (!pendingSettings_ || !pendingDisplayChange_) return;

    if (ImGui::CollapsingHeader("Display")) {
        ImGui::Text("Settings are applied when you press Apply");

        DisplaySettings& s = *pendingSettings_;

        std::string currentRes = std::to_string(s.width) + "x" + std::to_string(s.height);
        if (ImGui::BeginCombo("Resolution", currentRes.c_str())) {
            for (auto& res : uniqueResolutions_) {
                std::string label = std::to_string(res.first) + "x" + std::to_string(res.second);
                bool selected = (s.width == res.first && s.height == res.second);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    s.width = res.first;
                    s.height = res.second;
                    s.refreshRate = 0;
                    UpdateRefreshRates(s.width, s.height);
                }
            }
            ImGui::EndCombo();
        }

        std::string currentHz = (s.refreshRate == 0) ? "Default" : std::to_string(s.refreshRate) + "Hz";
        if (ImGui::BeginCombo("Refresh Rate", currentHz.c_str())) {
            for (int rate : currentRefreshRates_) {
                std::string label = (rate == 0 ? "Default" : std::to_string(rate) + "Hz");
                bool selected = (s.refreshRate == rate);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    s.refreshRate = rate;
                }
            }
            ImGui::EndCombo();
        }

        const char* windowModeNames[] = {"Windowed", "Borderless", "Exclusive Fullscreen"};
        int currentModeIndex = (int)s.windowMode;
        if (ImGui::Combo("Window Mode", &currentModeIndex, windowModeNames, 3)) {
            s.windowMode = (WindowMode)currentModeIndex;
        }

        ImGui::Checkbox("VSync", &s.vsync);

        const char* msaaNames[] = {"Off", "2x", "4x", "8x"};
        int msaaIndex = 0;
        switch (s.msaaSamples) {
            case 1: msaaIndex = 0; break;
            case 2: msaaIndex = 1; break;
            case 4: msaaIndex = 2; break;
            case 8: msaaIndex = 3; break;
        }
        if (ImGui::Combo("MSAA", &msaaIndex, msaaNames, 4)) {
            switch (msaaIndex) {
                case 0: s.msaaSamples = 1; break;
                case 1: s.msaaSamples = 2; break;
                case 2: s.msaaSamples = 4; break;
                case 3: s.msaaSamples = 8; break;
            }
        }

        if (ImGui::Button("Apply Settings")) {
            *pendingDisplayChange_ = true;
        }
    }
}