#pragma once
#include <glm/glm.hpp>
#include "../Core/EngineConfig.h"

// ============ LightingData ============
//
// 每帧由 LightingSystem 计算，传给 Renderer。
// 所有值都是"世界空间、已经算好、可直接使用"的。

struct LightingData {
    glm::vec3 lightDir;       // 太阳/月亮方向（已归一化）
    glm::vec3 lightColor;     // 直接光颜色
    float lightIntensity;     // 直接光强度
    glm::vec3 ambientColor;   // 环境光（由天空颜色算出）
    glm::vec4 skyTopColor;    // 天空顶部颜色
    glm::vec4 skyBottomColor; // 天空底部颜色
};

// ============ LightingSystem ============
//
// 职责：
//   - 从 EngineConfig.lighting 读取参数
//   - 根据 timeOfDay 计算太阳/月亮方向、颜色、强度
//   - 采样天空渐变
//   - 计算环境光
//
// 不拥有配置，只引用 EngineConfig。
// 未来扩展：天气、闪电、极光、大气散射

class LightingSystem {
public:
    void Initialize(EngineConfig& config);
    LightingData Update() const;

private:
    EngineConfig* config_ = nullptr;
};