#pragma once
struct EngineConfig;

class LightingPanel {
public:
    void Initialize(EngineConfig& config) { config_ = &config; }
    void Draw();
private:
    EngineConfig* config_ = nullptr;
};