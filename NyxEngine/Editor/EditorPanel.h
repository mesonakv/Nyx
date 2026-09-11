#pragma once
#include "CameraPanel.h"
#include "MaterialPanel.h"
#include "TargetMovementPanel.h"
#include "DisplaySettingsPanel.h"
#include "LightingPanel.h"
#include "PerformancePanel.h"
#include <SDL.h>
#include <deque>
#include <vector>

class Camera;
class MaterialLibrary;
class TargetManager;
struct EngineConfig;
struct DisplaySettings;

class EditorPanel {
public:
    struct Context {
        Camera* camera = nullptr;
        MaterialLibrary* materials = nullptr;
        TargetManager* targets = nullptr;
        EngineConfig* config = nullptr;
        DisplaySettings* pendingSettings = nullptr;
        const std::vector<SDL_DisplayMode>* displayModes = nullptr;
        const std::deque<float>* frameTimes = nullptr;
        bool* pendingDisplayChange = nullptr;
    };

    void Initialize(const Context& ctx);
    void Draw();

private:
    CameraPanel cameraPanel_;
    MaterialPanel materialPanel_;
    TargetMovementPanel targetMovementPanel_;
    DisplaySettingsPanel displayPanel_;
    LightingPanel lightingPanel_;
    PerformancePanel performancePanel_;
};