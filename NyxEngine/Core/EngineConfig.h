#pragma once
#include <glm/glm.hpp>

// ============ 引擎配置结构 ============
//
// 设计原则：
//   - 这里只放"引擎层可控选项"，即引擎或编辑器可以调整的参数
//   - 不放运行时状态（分数、目标位置等），那些属于 Game 层
//   - 不放资产数据（材质、mesh），那些属于项目文件
//
// 当前阶段（T3.0）：只是把散落在 main.cpp 的参数收进来
// 未来阶段：加 JSON 序列化、分组、元数据表

// ---------- 光照 ----------
struct LightingConfig {
    // 时间：0.00 午夜，0.25 日出，0.50 正午，0.75 日落，1.00 午夜
    float timeOfDay = 0.5f;

    // 环境光强度：0 = 全黑（塔科夫风），1 = 完全由天空照亮
    float ambientStrength = 0.35f;

    // 太阳 / 月亮强度系数
    float sunIntensityScale = 3.0f;
    float moonIntensityScale = 0.8f;

    // 太阳 / 月亮颜色
    glm::vec3 sunColor  = {1.0f, 0.95f, 0.85f};
    glm::vec3 moonColor = {0.35f, 0.45f, 1.0f};
};

// ---------- 相机 ----------
struct CameraConfig {
    glm::vec3 initialPosition = {0.0f, 1.5f, 8.0f};
    float initialPitch = 0.15f;
    float initialYaw = 0.0f;

    // 注意：sensitivity 和 fov 目前仍由 Camera 类持有
    // 等 T3.4 NyxEngine 收口时，会统一挪到这里
};

// ---------- 渲染 ----------
struct RenderConfig {
    // 预留给 PrismSpectra、后处理、阴影质量等
    // 目前 DisplaySettings（MSAA、VSync、分辨率）继续由 VulkanContext 管理
};

// ---------- 引擎配置总入口 ----------
struct EngineConfig {
    LightingConfig lighting;
    CameraConfig camera;
    RenderConfig render;
};