#pragma once
#include "../PhysicsTypes.h"

// ============ Contact ============
//
// 一次碰撞的接触信息。
//
// 约定：
//   - normal 从 a 指向 b
//   - penetration 是正数（穿透深度）
//   - point 是世界空间的接触点
//
// 分离公式：
//   a 沿 -normal * penetration 移动 → 分离
//   b 沿 +normal * penetration 移动 → 分离

struct Contact {
    ShapeHandle a;
    ShapeHandle b;
    glm::vec3 point = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    float penetration = 0.0f;
};