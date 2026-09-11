#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>

#include "NyxEngine/Core/VulkanContext.h"
#include "NyxEngine/Core/ImGuiManager.h"
#include "NyxEngine/Render/Renderer.h"
#include "NyxEngine/Scene/Camera.h"
#include "NyxEngine/Scene/Material.h"
#include "Game/Target/TargetManager.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>

std::vector<SDL_DisplayMode> GetAvailableDisplayModes() {
    std::vector<SDL_DisplayMode> modes;
    int displayCount = SDL_GetNumVideoDisplays();
    if (displayCount < 1) return modes;

    int modeCount = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < modeCount; i++) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0) {
            modes.push_back(mode);
        }
    }
    return modes;
}

int main(int argc, char* argv[]) {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    DisplaySettings displaySettings;
    displaySettings.width = 1280;
    displaySettings.height = 720;
    displaySettings.windowMode = WindowMode::Borderless;
    displaySettings.vsync = false;
    displaySettings.refreshRate = 0;
    displaySettings.msaaSamples = 4;

    SDL_Window* window = SDL_CreateWindow("Nyx",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        displaySettings.width, displaySettings.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    VulkanContext vk;
    vk.Initialize(window, displaySettings);

    ImGuiManager imgui;
    imgui.Initialize(window, vk);

    Renderer renderer;
    renderer.Initialize(vk, window);

    Camera camera;
    camera.position = glm::vec3(0.0f, 1.5f, 8.0f);
    camera.pitch = 0.15f;
    camera.yaw = 0.0f;

    MaterialLibrary materials;
    materials.LoadDefaults();

    TargetManager targets;
    targets.currentMaterialIndex = materials.selectedIndex;
    targets.Spawn();

    SDL_SetRelativeMouseMode(SDL_TRUE);

    auto displayModes = GetAvailableDisplayModes();
    std::vector<std::pair<int,int>> uniqueResolutions;
    for (auto& m : displayModes) {
        auto res = std::make_pair(m.w, m.h);
        if (std::find(uniqueResolutions.begin(), uniqueResolutions.end(), res) == uniqueResolutions.end()) {
            uniqueResolutions.push_back(res);
        }
    }

    std::vector<int> currentRefreshRates;
    auto updateRefreshRates = [&](int width, int height) {
        currentRefreshRates.clear();
        for (auto& m : displayModes) {
            if (m.w == width && m.h == height) {
                if (std::find(currentRefreshRates.begin(), currentRefreshRates.end(), m.refresh_rate) == currentRefreshRates.end()) {
                    currentRefreshRates.push_back(m.refresh_rate);
                }
            }
        }
    };
    updateRefreshRates(vk.settings.width, vk.settings.height);

    bool running = true;
    SDL_Event event;
    uint32_t lastShotTime = 0;
    bool editorMode = false;
    bool pendingDisplayChange = false;
    bool windowMinimized = false;
    bool swapchainDestroyed = false;
    bool exclusiveFullscreenSuspended = false;
    DisplaySettings pendingSettings = vk.settings;

    float timeOfDay = 0.5f;

    uint32_t frameCount = 0;
    uint32_t fpsTimer = 0;
    uint32_t currentFPS = 0;

    std::deque<float> frameTimes;
    const size_t maxFrameTimes = 600;
    uint64_t performanceFrequency = SDL_GetPerformanceFrequency();
    uint64_t frameStartCounter = 0;
    uint64_t lastFrameCounter = SDL_GetPerformanceCounter();

    while (running) {
        frameStartCounter = SDL_GetPerformanceCounter();

        uint64_t currentFrameCounter = frameStartCounter;
        float dt = (float)((currentFrameCounter - lastFrameCounter) * 1000.0 / performanceFrequency) / 1000.0f;
        lastFrameCounter = currentFrameCounter;
        if (dt > 0.1f) dt = 0.1f;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
                editorMode = !editorMode;
                SDL_SetRelativeMouseMode(editorMode ? SDL_FALSE : SDL_TRUE);
            }

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    windowMinimized = true;
                } else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    windowMinimized = false;
                    if (swapchainDestroyed) pendingDisplayChange = true;
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    // 独占全屏失去焦点：只标记，不改变 windowMode
                    if (vk.settings.windowMode == WindowMode::ExclusiveFullscreen) {
                        exclusiveFullscreenSuspended = true;
                    }
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    windowMinimized = false;
                    if (swapchainDestroyed) pendingDisplayChange = true;
                    // 独占全屏重新获得焦点：重建 swapchain 恢复独占全屏
                    if (exclusiveFullscreenSuspended) {
                        pendingSettings = vk.settings;
                        pendingDisplayChange = true;
                        exclusiveFullscreenSuspended = false;
                    }
                }
            }

            if (!editorMode && event.type == SDL_MOUSEMOTION) {
                camera.ProcessMouseDelta((float)event.motion.xrel, (float)event.motion.yrel);
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        if (windowMinimized) {
            if (!swapchainDestroyed) {
                vkDeviceWaitIdle(vk.device);
                vk.DestroySwapchainResources();
                swapchainDestroyed = true;
            }
            SDL_Delay(50);
            continue;
        }

        if (pendingDisplayChange) {
            vk.settings = pendingSettings;

            bool useExclusive = (vk.settings.windowMode == WindowMode::ExclusiveFullscreen);
            SDL_SetWindowFullscreen(window, 0);

            if (useExclusive) {
                SDL_DisplayMode targetMode;
                targetMode.format = SDL_PIXELFORMAT_UNKNOWN;
                targetMode.w = vk.settings.width;
                targetMode.h = vk.settings.height;
                targetMode.refresh_rate = (vk.settings.refreshRate == 0) ? 0 : vk.settings.refreshRate;

                SDL_DisplayMode closestMode;
                if (SDL_GetClosestDisplayMode(0, &targetMode, &closestMode) == nullptr) {
                    useExclusive = false;
                    vk.settings.windowMode = WindowMode::Borderless;
                } else {
                    if (SDL_SetWindowDisplayMode(window, &closestMode) != 0) {
                        useExclusive = false;
                        vk.settings.windowMode = WindowMode::Borderless;
                    }
                }
            }

            if (vk.settings.windowMode == WindowMode::Windowed) {
                SDL_SetWindowFullscreen(window, 0);
                SDL_SetWindowSize(window, vk.settings.width, vk.settings.height);
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            } else if (vk.settings.windowMode == WindowMode::Borderless) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            } else {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                SDL_SetWindowPosition(window, 0, 0);
                SDL_SetWindowSize(window, vk.settings.width, vk.settings.height);
            }

            vk.RecreateSwapchain(window);
            renderer.RecreatePipeline(vk);
            imgui.RecreatePipeline(vk);
            swapchainDestroyed = false;

            if (!editorMode) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_SetRelativeMouseMode(SDL_TRUE);
                SDL_WarpMouseInWindow(window, vk.settings.width / 2, vk.settings.height / 2);
            }
            pendingDisplayChange = false;
        }

        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        uint32_t currentTime = SDL_GetTicks();

        if (!editorMode) {
            if (keyState[SDL_SCANCODE_SPACE] && currentTime - lastShotTime > 200) {
                targets.Shoot(camera.position, camera.GetDirection());
                lastShotTime = currentTime;
            }
            if (keyState[SDL_SCANCODE_R]) {
                targets.Spawn();
                targets.currentMaterialIndex = materials.selectedIndex;
            }
            targets.Update(dt);
        }

        frameCount++;
        uint32_t now = SDL_GetTicks();
        if (now - fpsTimer >= 500) {
            currentFPS = frameCount * 2;
            frameCount = 0;
            fpsTimer = now;
        }

        SDL_DisplayMode actualMode;
        SDL_GetWindowDisplayMode(window, &actualMode);
        std::string title = "Nyx | Score: " + std::to_string(targets.score) +
                            " | FPS: " + std::to_string(currentFPS) +
                            " | " + std::to_string(actualMode.w) + "x" + std::to_string(actualMode.h) +
                            "@" + std::to_string(actualMode.refresh_rate) + "Hz";
        SDL_SetWindowTitle(window, title.c_str());

        float sunAngle = timeOfDay * 2.0f * 3.14159f;
        glm::vec3 lightDir = glm::normalize(glm::vec3(cos(sunAngle), sin(sunAngle), 0.3f));
        float daylight = std::max(0.0f, sin(sunAngle));
        float moonlight = std::max(0.0f, -sin(sunAngle));
        float lightIntensity = daylight * 3.0f + moonlight * 0.8f;
        glm::vec3 lightColor = glm::mix(
            glm::vec3(0.35f, 0.45f, 1.0f),
            glm::vec3(1.0f, 0.95f, 0.85f),
            daylight / std::max(daylight + moonlight, 0.01f)
        );

        glm::vec4 skyTopColor;
        glm::vec4 skyBottomColor;
        {
            glm::vec4 noonTop(0.2f, 0.5f, 1.0f, 1.0f);
            glm::vec4 noonBottom(0.7f, 0.8f, 1.0f, 1.0f);
            glm::vec4 nightTop(0.01f, 0.01f, 0.08f, 1.0f);
            glm::vec4 nightBottom(0.08f, 0.08f, 0.2f, 1.0f);
            glm::vec4 duskTop(0.15f, 0.1f, 0.3f, 1.0f);
            glm::vec4 duskBottom(1.0f, 0.5f, 0.2f, 1.0f);

            float t = timeOfDay;
            if (t > 0.21f && t < 0.29f) {
                float f = (t - 0.21f) / 0.08f;
                skyTopColor = glm::mix(nightTop, noonTop, f);
                skyBottomColor = glm::mix(nightBottom, duskBottom, f);
            } else if (t >= 0.29f && t < 0.71f) {
                skyTopColor = noonTop;
                skyBottomColor = noonBottom;
            } else if (t >= 0.71f && t < 0.79f) {
                float f = (t - 0.71f) / 0.08f;
                skyTopColor = glm::mix(noonTop, duskTop, f);
                skyBottomColor = glm::mix(noonBottom, duskBottom, f);
            } else {
                skyTopColor = nightTop;
                skyBottomColor = nightBottom;
            }
        }

        imgui.NewFrame();

        if (editorMode) {
            ImGui::Begin("Nyx Editor");

            if (ImGui::CollapsingHeader("Camera")) {
                ImGui::SliderFloat("Sensitivity", &camera.sensitivity, 0.0001f, 0.01f, "%.5f");
                ImGui::SliderFloat("FOV", &camera.fov, 30.0f, 120.0f);
            }

            if (ImGui::CollapsingHeader("Material")) {
                if (ImGui::BeginCombo("Preset", materials.GetSelected().name.c_str())) {
                    for (int i = 0; i < (int)materials.materials.size(); i++) {
                        bool selected = (materials.selectedIndex == i);
                        if (ImGui::Selectable(materials.materials[i].name.c_str(), selected)) {
                            materials.selectedIndex = i;
                            targets.currentMaterialIndex = i;
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                Material& mat = materials.GetSelected();
                ImGui::ColorEdit4("Color", &mat.color.x);
                ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f);
                ImGui::SliderFloat("Emissive", &mat.emissive_strength, 0.0f, 5.0f);
                ImGui::SliderFloat("Opacity", &mat.opacity, 0.0f, 1.0f);
                ImGui::SliderFloat("Reflectance", &mat.reflectance, 0.0f, 1.0f);

                if (ImGui::Button("Apply Material to All Targets")) {
                    for (auto& t : targets.targets) t.materialIndex = materials.selectedIndex;
                }
                if (ImGui::Button("Reset Targets")) {
                    targets.currentMaterialIndex = materials.selectedIndex;
                    targets.Spawn();
                }
            }

            if (ImGui::CollapsingHeader("Target Movement")) {
                const char* movementNames[] = {"Static", "Linear", "Sine Wave", "Random Direction"};
                int currentMovement = (int)targets.movementType;
                if (ImGui::Combo("Type", &currentMovement, movementNames, 4)) {
                    targets.movementType = (MovementType)currentMovement;
                    targets.Spawn();
                }
                ImGui::SliderFloat("Speed", &targets.moveSpeed, 0.0f, 10.0f);
                if (targets.movementType == MovementType::SineWave) {
                    ImGui::SliderFloat("Amplitude", &targets.sineAmplitude, 0.1f, 5.0f);
                    ImGui::SliderFloat("Frequency", &targets.sineFrequency, 0.1f, 5.0f);
                } else if (targets.movementType == MovementType::RandomDirection) {
                    ImGui::SliderFloat("Change Interval", &targets.randomChangeInterval, 0.5f, 5.0f);
                }
            }

            if (ImGui::CollapsingHeader("Display")) {
                ImGui::Text("Settings are applied when you press Apply");

                std::string currentRes = std::to_string(pendingSettings.width) + "x" + std::to_string(pendingSettings.height);
                if (ImGui::BeginCombo("Resolution", currentRes.c_str())) {
                    for (auto& res : uniqueResolutions) {
                        std::string label = std::to_string(res.first) + "x" + std::to_string(res.second);
                        bool selected = (pendingSettings.width == res.first && pendingSettings.height == res.second);
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            pendingSettings.width = res.first;
                            pendingSettings.height = res.second;
                            pendingSettings.refreshRate = 0;
                            updateRefreshRates(pendingSettings.width, pendingSettings.height);
                        }
                    }
                    ImGui::EndCombo();
                }

                std::string currentHz = (pendingSettings.refreshRate == 0) ? "Default" : std::to_string(pendingSettings.refreshRate) + "Hz";
                if (ImGui::BeginCombo("Refresh Rate", currentHz.c_str())) {
                    for (int rate : currentRefreshRates) {
                        std::string label = (rate == 0 ? "Default" : std::to_string(rate) + "Hz");
                        bool selected = (pendingSettings.refreshRate == rate);
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            pendingSettings.refreshRate = rate;
                        }
                    }
                    ImGui::EndCombo();
                }

                const char* windowModeNames[] = {"Windowed", "Borderless", "Exclusive Fullscreen"};
                int currentModeIndex = (int)pendingSettings.windowMode;
                if (ImGui::Combo("Window Mode", &currentModeIndex, windowModeNames, 3)) {
                    pendingSettings.windowMode = (WindowMode)currentModeIndex;
                }

                ImGui::Checkbox("VSync", &pendingSettings.vsync);

                const char* msaaNames[] = {"Off", "2x", "4x", "8x"};
                int msaaIndex = 0;
                switch (pendingSettings.msaaSamples) {
                    case 1: msaaIndex = 0; break;
                    case 2: msaaIndex = 1; break;
                    case 4: msaaIndex = 2; break;
                    case 8: msaaIndex = 3; break;
                }
                if (ImGui::Combo("MSAA", &msaaIndex, msaaNames, 4)) {
                    switch (msaaIndex) {
                        case 0: pendingSettings.msaaSamples = 1; break;
                        case 1: pendingSettings.msaaSamples = 2; break;
                        case 2: pendingSettings.msaaSamples = 4; break;
                        case 3: pendingSettings.msaaSamples = 8; break;
                    }
                }

                if (ImGui::Button("Apply Settings")) {
                    pendingDisplayChange = true;
                }
            }

            if (ImGui::CollapsingHeader("Lighting")) {
                ImGui::SliderFloat("Time of Day", &timeOfDay, 0.0f, 1.0f);
                if (ImGui::Button("Set Noon")) timeOfDay = 0.5f;
                ImGui::SameLine();
                if (ImGui::Button("Set Midnight")) timeOfDay = 0.0f;
                ImGui::SameLine();
                if (ImGui::Button("Set Dawn")) timeOfDay = 0.25f;
                ImGui::SameLine();
                if (ImGui::Button("Set Dusk")) timeOfDay = 0.75f;
            }

            if (ImGui::CollapsingHeader("Performance")) {
                if (!frameTimes.empty()) {
                    float sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f);
                    float avg = sum / frameTimes.size();

                    float sq_sum = 0.0f;
                    for (float t : frameTimes) sq_sum += (t - avg) * (t - avg);
                    float stddev = std::sqrt(sq_sum / frameTimes.size());

                    std::vector<float> sorted(frameTimes.begin(), frameTimes.end());
                    std::sort(sorted.begin(), sorted.end());

                    size_t p99_index = (size_t)(sorted.size() * 0.99);
                    if (p99_index >= sorted.size()) p99_index = sorted.size() - 1;
                    float p99 = sorted[p99_index];

                    size_t worst_count = std::max<size_t>(1, sorted.size() / 100);
                    float worst_sum = 0.0f;
                    for (size_t i = sorted.size() - worst_count; i < sorted.size(); ++i) worst_sum += sorted[i];
                    float worst1pct = worst_sum / worst_count;

                    ImGui::Text("Avg: %.3f ms", avg);
                    ImGui::Text("P99: %.3f ms", p99);
                    ImGui::Text("Worst 1%% avg: %.3f ms", worst1pct);
                    ImGui::Text("StdDev: %.3f ms", stddev);
                } else {
                    ImGui::Text("Collecting frame data...");
                }
            }

            ImGui::End();
        }

        std::vector<Material> targetMaterials;
        for (auto& t : targets.targets) {
            if (t.alive) {
                int idx = t.materialIndex;
                if (idx < 0 || idx >= (int)materials.materials.size()) idx = 0;
                targetMaterials.push_back(materials.materials[idx]);
            }
        }

        float aspect = (float)vk.swapchainExtent.width / (float)vk.swapchainExtent.height;
        renderer.DrawFrame(vk, camera.GetViewMatrix(), camera.GetProjectionMatrix(aspect),
                           targets.GetAlivePositions(), targets.GetAliveScales(), targetMaterials,
                           lightDir, lightColor, lightIntensity,
                           skyTopColor, skyBottomColor, &imgui);

        uint64_t frameEndCounter = SDL_GetPerformanceCounter();
        float frameTimeMs = (float)((frameEndCounter - frameStartCounter) * 1000.0 / performanceFrequency);
        frameTimes.push_back(frameTimeMs);
        if (frameTimes.size() > maxFrameTimes) frameTimes.pop_front();
    }

    renderer.Cleanup(vk);
    imgui.Cleanup(vk);
    vk.Cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}