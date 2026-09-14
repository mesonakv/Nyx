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

class Player {
public:
    // ---------- 位置 ----------
    glm::vec3 position = glm::vec3(0.0f, -2.0f, 8.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float eyeHeight = 1.7f;

    // ---------- 物理 ----------
    static constexpr float kCapsuleRadius = 0.4f;
    static constexpr float kCapsuleHalfHeight = 0.45f;

    ShapeHandle physicsBody;
    bool onGround = false;
    bool wasOnGround = false;

    // ---------- 手感参数 ----------
    float maxSpeed = 8.0f;
    float groundAccel = 60.0f;
    float groundFriction = 8.0f;
    float airAccel = 15.0f;

    float jumpSpeed = 8.0f;
    float gravity = 25.0f;
    float jumpCutMultiplier = 0.5f;
    float coyoteTime = 0.1f;
    float jumpBuffer = 0.1f;

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
                bool jumpHeld,
                float yaw,
                PhysicsWorld& physics);

private:
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    bool jumping_ = false;

    void ApplyFriction(float dt);
    void ApplyLateralFriction(const glm::vec3& wishDir, float dt);
    void Accelerate(const glm::vec3& wishDir, float accel, float maxSpeed, float dt);
};