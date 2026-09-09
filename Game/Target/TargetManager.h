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
        glm::vec3 initialPosition;   // 用于正弦等基�?
        float phase;                 // 每个目标的随机相�?
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
    std::vector<glm::vec3> GetAlivePositions() const;
    std::vector<float> GetAliveScales() const;
    std::vector<int> GetAliveMaterialIndices() const;
};
