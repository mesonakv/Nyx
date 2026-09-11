#pragma once
#include <glm/glm.hpp>
#include "../../NyxEngine/Scene/Material.h"
#include <vector>

enum class MovementType {
    Static,
    Linear,
    SineWave,
    RandomDirection
};

class TargetManager {
public:
    struct Target {
        glm::vec3 position;
        glm::vec3 initialPosition;   // 用于正弦等基准
        float phase;                 // 每个目标的随机相位
        float scale;
        bool alive;
        int materialIndex;
    };

    std::vector<Target> targets;
    int score = 0;
    int currentMaterialIndex = 1;

    // 移动参数
    MovementType movementType = MovementType::Static;
    float moveSpeed = 1.0f;
    float sineAmplitude = 1.0f;
    float sineFrequency = 1.0f;
    float randomChangeInterval = 1.0f;

    float elapsedTime = 0.0f;
    float randomTimer = 0.0f;
    glm::vec3 randomDir = glm::vec3(1.0f, 0.0f, 0.0f);

    void Spawn();
    void Update(float dt);
    void Shoot(glm::vec3 cameraPos, glm::vec3 direction);

    // A3: 返回内部缓存的引用，零分配
    const std::vector<glm::vec3>& GetAlivePositions() const;
    const std::vector<float>& GetAliveScales() const;
    std::vector<int> GetAliveMaterialIndices() const;

private:
    // A3: 每次查询时重建。clear() 保留 capacity，所以稳定后零分配
    mutable std::vector<glm::vec3> alivePositionsCache_;
    mutable std::vector<float> aliveScalesCache_;
};