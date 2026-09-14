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
    // 地面：有输入就加速，没输入就摩擦
    // 空中：有输入就加速（低加速度），没输入就不施加摩擦
    if (onGround) {
        if (wishLen > 0.001f) {
            Accelerate(wishDir, groundAccel, maxSpeed, dt);
        } else {
            ApplyFriction(dt);
        }
    } else {
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
    // buffer 和 coyote 同时有效时触发
    if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
        velocity.y = jumpSpeed;
        jumpBufferTimer_ = 0.0f;
        coyoteTimer_ = 0.0f;
        jumping_ = true;
        onGround = false;
    }

    // ---------- 6. Jump cut（松开跳跃键） ----------
    // 上升过程中松开跳跃键 → 截断上升速度
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

void Player::Accelerate(const glm::vec3& wishDir, float accel, float maxSpeed, float dt) {
    // Source 引擎风格的加速。
    // 只在"沿 wishDir 的分速度 < maxSpeed"时加速，
    // 且不超过 maxSpeed。这样斜向移动也不会超过最大速度。
    float currentSpeed = velocity.x * wishDir.x + velocity.z * wishDir.z;
    float addSpeed = maxSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;

    float accelSpeed = accel * dt;
    if (accelSpeed > addSpeed) accelSpeed = addSpeed;

    velocity.x += wishDir.x * accelSpeed;
    velocity.z += wishDir.z * accelSpeed;
}