#include "Player.h"
#include "../Physics/PhysicsWorld.h"
#include <algorithm>
#include <cmath>

const char* PlayerStateToString(PlayerState s) {
    switch (s) {
    case PlayerState::Idle: return "Idle";
    case PlayerState::Run:  return "Run";
    case PlayerState::Jump: return "Jump";
    case PlayerState::Fall: return "Fall";
    case PlayerState::Land: return "Land";
    default: return "?";
    }
}

void Player::Update(float dt,
                    const glm::vec3& moveDir,
                    bool jumpPressed,
                    bool jumpHeld,
                    float yaw,
                    PhysicsWorld& physics)
{
    (void)yaw;
    wasOnGround = onGround;

    // ---------- 1. 计算 wish direction ----------
    glm::vec3 wishDir = moveDir;
    wishDir.y = 0.0f;
    float wishLen = glm::length(wishDir);
    bool hasMoveInput = (wishLen > 0.001f);
    if (hasMoveInput) {
        wishDir /= wishLen;
    } else {
        wishDir = glm::vec3(0.0f);
    }

    // ---------- 2. 水平移动 ----------
    if (onGround) {
        if (hasMoveInput) {
            ApplyLateralFriction(wishDir, dt);
            Accelerate(wishDir, groundAccel, maxSpeed, dt);
        } else {
            ApplyFriction(dt);
        }
    } else {
        if (hasMoveInput) {
            Accelerate(wishDir, airAccel, maxSpeed, dt);
        }
    }

    // ---------- 3. Coyote time ----------
    if (onGround) {
        coyoteTimer_ = coyoteTime;
    } else {
        coyoteTimer_ -= dt;
        if (coyoteTimer_ < 0.0f) coyoteTimer_ = 0.0f;
    }

    // ---------- 4. Jump buffer ----------
    if (jumpPressed) {
        jumpBufferTimer_ = jumpBuffer;
    } else {
        jumpBufferTimer_ -= dt;
        if (jumpBufferTimer_ < 0.0f) jumpBufferTimer_ = 0.0f;
    }

    // ---------- 5. 跳跃触发 ----------
    if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
        velocity.y = jumpSpeed;
        jumpBufferTimer_ = 0.0f;
        coyoteTimer_ = 0.0f;
        jumping_ = true;
        onGround = false;
    }

    // ---------- 6. Jump cut ----------
    if (jumping_ && !jumpHeld && velocity.y > 0.0f) {
        velocity.y *= jumpCutMultiplier;
        jumping_ = false;
    }
    if (velocity.y <= 0.0f) {
        jumping_ = false;
    }

    // ---------- 7. 重力 ----------
    velocity.y -= gravity * dt;

    const float kMaxFallSpeed = 50.0f;
    if (velocity.y < -kMaxFallSpeed) velocity.y = -kMaxFallSpeed;

    // ---------- 8. 计算期望新位置 ----------
    glm::vec3 newPos = position + velocity * dt;

    // ---------- 9. 地面检测 ----------
    const float footOffset = kCapsuleHalfHeight + kCapsuleRadius;
    glm::vec3 rayOrigin = newPos + glm::vec3(0.0f, footOffset, 0.0f);
    float rayLength = footOffset + groundRayMaxDist;

    RaycastHit groundHit = physics.Raycast(
        rayOrigin,
        glm::vec3(0.0f, -1.0f, 0.0f),
        rayLength,
        PhysicsLayer::StaticGeo);

    onGround = false;
    if (groundHit.hit) {
        float groundTopY = groundHit.point.y;
        if (newPos.y <= groundTopY) {
            newPos.y = groundTopY;
            if (velocity.y < 0.0f) velocity.y = 0.0f;
            onGround = true;
            jumping_ = false;
        }
    }

    // ---------- 10. 应用位置 ----------
    position = newPos;

    // ---------- 11. 同步到物理世界 ----------
    if (physicsBody.IsValid()) {
        Transform t;
        t.position = GetCapsuleCenter();
        t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        physics.UpdateTransform(physicsBody, t);
    }

    // ---------- 12. 状态机 ----------
    UpdateState(dt, hasMoveInput);
}

// ============ 状态机 ============

void Player::UpdateState(float dt, bool hasMoveInput) {
    PlayerState newState;

    if (!onGround) {
        // 空中：上升期是 Jump，下落期是 Fall
        newState = (velocity.y > 0.0f) ? PlayerState::Jump : PlayerState::Fall;
    } else if (!wasOnGround) {
        // 刚落地：强制进入 Land
        newState = PlayerState::Land;
    } else if (state == PlayerState::Land && stateTimer < landDuration) {
        // Land 状态未结束：保持
        newState = PlayerState::Land;
    } else {
        // 地面稳定状态：Idle 或 Run
        newState = hasMoveInput ? PlayerState::Run : PlayerState::Idle;
    }

    if (newState != state) {
        state = newState;
        stateTimer = 0.0f;

        // 进入 Land 时的钩子（将来：音效、粒子、相机抖动）
        // if (state == PlayerState::Land) { ... }
    } else {
        stateTimer += dt;
    }
}

// ============ 摩擦 ============

void Player::ApplyFriction(float dt) {
    float speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    if (speed < 0.1f) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        return;
    }

    float drop = speed * groundFriction * dt;
    float newSpeed = std::max(0.0f, speed - drop);
    float ratio = newSpeed / speed;
    velocity.x *= ratio;
    velocity.z *= ratio;
}

void Player::ApplyLateralFriction(const glm::vec3& wishDir, float dt) {
    glm::vec3 vel2d(velocity.x, 0.0f, velocity.z);
    float along = glm::dot(vel2d, wishDir);
    glm::vec3 lateral = vel2d - wishDir * along;

    float lateralSpeed = glm::length(lateral);
    if (lateralSpeed < 0.01f) return;

    float drop = lateralSpeed * groundFriction * dt;
    float newSpeed = std::max(0.0f, lateralSpeed - drop);
    float ratio = newSpeed / lateralSpeed;
    lateral *= ratio;

    velocity.x = wishDir.x * along + lateral.x;
    velocity.z = wishDir.z * along + lateral.z;
}

// ============ 加速 ============

void Player::Accelerate(const glm::vec3& wishDir, float accel, float maxSpeed, float dt) {
    float currentSpeed = velocity.x * wishDir.x + velocity.z * wishDir.z;
    float addSpeed = maxSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;

    float accelSpeed = accel * dt;
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;

    velocity.x += wishDir.x * accelSpeed;
    velocity.z += wishDir.z * accelSpeed;
}