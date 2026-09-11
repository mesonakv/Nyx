#include "LightingSystem.h"
#include "Core/NyxMath.h"
#include <algorithm>
#include <cmath>

// ============ 天空渐变关键帧 ============
// 只在本文件内使用，不导出

namespace {

struct SkyKeyframe {
    float t;
    glm::vec4 top;
    glm::vec4 bottom;
};

const glm::vec4 kNoonTop    (0.2f,  0.5f,  1.0f,  1.0f);
const glm::vec4 kNoonBottom (0.7f,  0.8f,  1.0f,  1.0f);
const glm::vec4 kNightTop   (0.01f, 0.01f, 0.08f, 1.0f);
const glm::vec4 kNightBottom(0.08f, 0.08f, 0.2f,  1.0f);
const glm::vec4 kDuskTop    (0.15f, 0.1f,  0.3f,  1.0f);
const glm::vec4 kDuskBottom (1.0f,  0.5f,  0.2f,  1.0f);

const SkyKeyframe kSkyKeyframes[] = {
    {0.00f, kNightTop, kNightBottom},
    {0.21f, kNightTop, kNightBottom},
    {0.25f, kDuskTop,  kDuskBottom},
    {0.29f, kNoonTop,  kNoonBottom},
    {0.71f, kNoonTop,  kNoonBottom},
    {0.75f, kDuskTop,  kDuskBottom},
    {0.79f, kNightTop, kNightBottom},
    {1.00f, kNightTop, kNightBottom},
};
const int kSkyKeyframeCount = sizeof(kSkyKeyframes) / sizeof(kSkyKeyframes[0]);

void SampleSkyGradient(float t, glm::vec4& outTop, glm::vec4& outBottom) {
    if (t <= kSkyKeyframes[0].t) {
        outTop = kSkyKeyframes[0].top;
        outBottom = kSkyKeyframes[0].bottom;
        return;
    }
    if (t >= kSkyKeyframes[kSkyKeyframeCount - 1].t) {
        outTop = kSkyKeyframes[kSkyKeyframeCount - 1].top;
        outBottom = kSkyKeyframes[kSkyKeyframeCount - 1].bottom;
        return;
    }
    for (int i = 0; i < kSkyKeyframeCount - 1; i++) {
        if (t >= kSkyKeyframes[i].t && t <= kSkyKeyframes[i + 1].t) {
            float span = kSkyKeyframes[i + 1].t - kSkyKeyframes[i].t;
            float f = (span > 0.0001f) ? (t - kSkyKeyframes[i].t) / span : 0.0f;
            outTop = glm::mix(kSkyKeyframes[i].top, kSkyKeyframes[i + 1].top, f);
            outBottom = glm::mix(kSkyKeyframes[i].bottom, kSkyKeyframes[i + 1].bottom, f);
            return;
        }
    }
    outTop = kSkyKeyframes[0].top;
    outBottom = kSkyKeyframes[0].bottom;
}

} // anonymous namespace

// ============ LightingSystem ============

void LightingSystem::Initialize(EngineConfig& config) {
    config_ = &config;
}

LightingData LightingSystem::Update() const {
    LightingData out;

    const LightingConfig& cfg = config_->lighting;
    const float timeOfDay = cfg.timeOfDay;

    // ---------- 太阳/月亮方向 ----------
    // timeOfDay: 0.00 午夜, 0.25 日出, 0.50 正午, 0.75 日落
    const float sunAngle = (timeOfDay - 0.25f) * nyx::TwoPi;
    out.lightDir = glm::normalize(glm::vec3(cos(sunAngle), sin(sunAngle), 0.3f));

    // ---------- 强度 ----------
    const float daylight = std::max(0.0f, sin(sunAngle));
    const float moonlight = std::max(0.0f, -sin(sunAngle));
    out.lightIntensity = daylight * cfg.sunIntensityScale
                       + moonlight * cfg.moonIntensityScale;

    // ---------- 颜色 ----------
    out.lightColor = glm::mix(
        cfg.moonColor,
        cfg.sunColor,
        daylight / std::max(daylight + moonlight, 0.01f)
    );

    // ---------- 天空 ----------
    SampleSkyGradient(timeOfDay, out.skyTopColor, out.skyBottomColor);

    // ---------- 环境光 ----------
    glm::vec3 skyBottomRgb(out.skyBottomColor.r, out.skyBottomColor.g, out.skyBottomColor.b);
    glm::vec3 skyTopRgb(out.skyTopColor.r, out.skyTopColor.g, out.skyTopColor.b);
    glm::vec3 skyAvg = glm::mix(skyBottomRgb, skyTopRgb, 0.5f);
    out.ambientColor = skyAvg * cfg.ambientStrength;

    return out;
}