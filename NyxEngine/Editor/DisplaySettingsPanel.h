#pragma once
#include <SDL.h>
#include <vector>
#include <utility>

struct DisplaySettings;

class DisplaySettingsPanel {
public:
    void Initialize(const std::vector<SDL_DisplayMode>& displayModes,
                    DisplaySettings& pendingSettings,
                    bool& pendingDisplayChange);
    void Draw();

private:
    void UpdateRefreshRates(int width, int height);

    const std::vector<SDL_DisplayMode>* displayModes_ = nullptr;
    DisplaySettings* pendingSettings_ = nullptr;
    bool* pendingDisplayChange_ = nullptr;

    std::vector<std::pair<int,int>> uniqueResolutions_;
    std::vector<int> currentRefreshRates_;
};