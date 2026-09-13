#pragma once
#include <glm/glm.hpp>
#include "../Physics/PhysicsTypes.h"

class PhysicsWorld;

// ============ Player ============
//
// 玩家实体。
//
// 位置语义：
//   position 是玩家"脚底"的世界坐标。
//   眼睛位置 = position + (0, eyeHeight, 0)。
//
// 物理语义：
//   velocity 是线速度（m/s）。
//   onGround 由地面检测更新。
//   physicsBody 是 WorldState.physics 中的胶囊句柄。

class Player {
public:
    // ---------- 位置 ----------
    // 出生在地面上（地面顶部 y = -2.0）
    glm::vec3 position = glm::vec3(0.0f, -2.0f, 8.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float eyeHeight = 1.7f;

    // ---------- 物理 ----------
    static constexpr float kCapsuleRadius = 0.4f;
    static constexpr float kCapsuleHalfHeight = 0.45f;

    ShapeHandle physicsBody;
    bool onGround = false;

    // ---------- 手感参数 ----------
    float moveSpeed = 8.0f;
    float gravity = 25.0f;
    float jumpSpeed = 8.0f;
    float groundCheckDist = 0.15f;

    // 地面检测的最大射线距离。
    // 足够长以容忍高速下落时的一帧穿透。
    // 如果玩家掉落超过这个距离，地面检测失效，玩家会继续下坠。
    float groundRayMaxDist = 6.0f;

    // ---------- 查询 ----------
    glm::vec3 GetEyePosition() const {
        return position + glm::vec3(0.0f, eyeHeight, 0.0f);
    }

    glm::vec3 GetCapsuleCenter() const {
        return position + glm::vec3(0.0f, kCapsuleHalfHeight + kCapsuleRadius, 0.0f);
    }

    // ---------- 更新 ----------
    void Update(float dt,
                const glm::vec3& moveDir,
                bool jumpPressed,
                float yaw,
                PhysicsWorld& physics);
};