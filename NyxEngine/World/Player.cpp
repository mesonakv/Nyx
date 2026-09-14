#include "Player.h"
#include "../Physics/PhysicsWorld.h"
#include <algorithm>
#include <cmath>

void Player::Update(float dt,
                    const glm::vec3& moveDir,
                    bool jumpPressed,
                    bool jumpHeld,
                    float yaw,
                    PhysicsWorld& physics)
{
    (void)yaw;
    wasOnGround = onGround;

    // ---------- 1. 计算 wish direction（水平） ----------
    glm::vec3 wishDir = moveDir;
    wishDir.y = 0.0f;
    float wishLen = glm::length(wishDir);
    if (wishLen > 0.001f) {
        wishDir /= wishLen;
    } else {
        wishDir = glm::vec3(0.0f);
    }

    // ---------- 2. 水平移动 ----------
    if (onGround) {
        if (wishLen > 0.001f) {
            // 先衰减垂直于 wishDir 的速度分量
            // 这样"改变移动方向"时旧方向的速度会快速消失
            ApplyLateralFriction(wishDir, dt);
            // 再沿 wishDir 加速
            Accelerate(wishDir, groundAccel, maxSpeed, dt);
        } else {
            ApplyFriction(dt);
        }
    } else {
        // 空中不衰减垂直速度（保留动量，允许跳跃后的方向控制）
        if (wishLen > 0.001f) {
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

    // ---------- 12. 落地帧 ----------
    if (!wasOnGround && onGround) {
        // 未来：落地音效 / 粒子 / 状态机切 Land
    }
}

// ============ 摩擦 ============

void Player::ApplyFriction(float dt) {
    // 无输入时：整体衰减水平速度
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
    // 有输入时：只衰减垂直于 wishDir 的速度分量。
    // 保留沿 wishDir 的分量，避免"保持方向不变"时速度流失。
    //
    // 数学：
    //   vel2d = 水平速度向量
    //   along = vel2d · wishDir （沿 wishDir 的标量分量）
    //   lateral = vel2d - wishDir * along （垂直分量）
    //   衰减 lateral，保留 along

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
    // Source 引擎风格：只在沿 wishDir 的分速度 < maxSpeed 时加速
    float currentSpeed = velocity.x * wishDir.x + velocity.z * wishDir.z;
    float addSpeed = maxSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;

    float accelSpeed = accel * dt;
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;

    velocity.x += wishDir.x * accelSpeed;
    velocity.z += wishDir.z * accelSpeed;
}