#include "TargetManager.h"
#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530718f;
}

float TargetManager::RandomAngle() {
    std::uniform_real_distribution<float> dist(0.0f, kTwoPi);
    return dist(rng_);
}

void TargetManager::Spawn() {
    targets.clear();
    const float spacing = 1.5f;
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        Target t;
        t.position = glm::vec3((col - 1) * spacing, (1 - row) * spacing, 0.0f);
        t.initialPosition = t.position;
        t.phase = RandomAngle();
        t.scale = 0.6f;
        t.alive = true;
        t.materialIndex = currentMaterialIndex;
        targets.push_back(t);
    }
    score = 0;
    elapsedTime = 0.0f;
    randomTimer = 0.0f;
}

void TargetManager::Update(float dt) {
    elapsedTime += dt;

    switch (movementType) {
    case MovementType::Static:
        break;

    case MovementType::Linear:
        for (auto& t : targets) {
            if (t.alive) {
                t.position += glm::vec3(1.0f, 0.0f, 0.0f) * moveSpeed * dt;
            }
        }
        break;

    case MovementType::SineWave:
        for (auto& t : targets) {
            if (t.alive) {
                float offsetX = sin(elapsedTime * sineFrequency + t.phase) * sineAmplitude;
                t.position = t.initialPosition + glm::vec3(offsetX, 0.0f, 0.0f);
            }
        }
        break;

    case MovementType::RandomDirection:
        randomTimer -= dt;
        if (randomTimer <= 0.0f) {
            randomTimer = randomChangeInterval;
            float angle = RandomAngle();
            randomDir = glm::vec3(cos(angle), 0.0f, sin(angle));
        }
        for (auto& t : targets) {
            if (t.alive) {
                t.position += randomDir * moveSpeed * dt;
            }
        }
        break;
    }
}

void TargetManager::Shoot(glm::vec3 cameraPos, glm::vec3 direction) {
    for (auto& t : targets) {
        if (!t.alive) continue;
        glm::vec3 toTarget = t.position - cameraPos;
        float distance = glm::length(toTarget);
        glm::vec3 toTargetDir = glm::normalize(toTarget);
        float dot = glm::dot(direction, toTargetDir);
        float angle = acos(std::max(-1.0f, std::min(1.0f, dot)));
        float targetRadius = 0.5f * t.scale;
        float hitAngle = atan(targetRadius / distance);
        if (angle < hitAngle) {
            t.alive = false;
            score++;
        }
    }
}

const std::vector<glm::vec3>& TargetManager::GetAlivePositions() const {
    alivePositionsCache_.clear();
    for (const auto& t : targets) {
        if (t.alive) alivePositionsCache_.push_back(t.position);
    }
    return alivePositionsCache_;
}

const std::vector<float>& TargetManager::GetAliveScales() const {
    aliveScalesCache_.clear();
    for (const auto& t : targets) {
        if (t.alive) aliveScalesCache_.push_back(t.scale);
    }
    return aliveScalesCache_;
}

const std::vector<int>& TargetManager::GetAliveMaterialIndices() const {
    aliveMaterialIndicesCache_.clear();
    for (const auto& t : targets) {
        if (t.alive) aliveMaterialIndicesCache_.push_back(t.materialIndex);
    }
    return aliveMaterialIndicesCache_;
}