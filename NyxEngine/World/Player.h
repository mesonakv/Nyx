#pragma once
#include <glm/glm.hpp>
#include "../Physics/PhysicsTypes.h"

class PhysicsWorld;

// ============ PlayerState ============
//
// 玩家动作状态。
//
// 状态转换：
//   Idle  ←→ Run           （地面，有无水平输入）
//   Idle/Run → Jump        （起跳）
//   Jump → Fall            （到达最高点）
//   Fall → Land            （落地）
//   Land → Idle/Run        （landDuration 之后）
//
// Land 状态是"标志性"的：不阻塞移动和跳跃，
// 只是提供一个短暂的窗口给音效、粒子、动画。

enum class PlayerState : uint8_t {
    Idle,
    Run,
    Jump,
    Fall,
    Land,
    Count
};

const char* PlayerStateToString(PlayerState s);

// ============ Player ============

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

    // ---------- 状态机 ----------
    PlayerState state = PlayerState::Idle;
    float stateTimer = 0.0f;       // 当前状态已持续的时间（秒）
    float landDuration = 0.1f;     // Land 状态的持续时间

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

    void UpdateState(float dt, bool hasMoveInput);
};