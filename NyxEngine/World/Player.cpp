#include "Player.h"
#include "../Physics/PhysicsWorld.h"

void Player::Update(float dt,
                    const glm::vec3& moveDir,
                    bool jumpPressed,
                    float yaw,
                    PhysicsWorld& physics)
{
    (void)yaw;   // 暂时不用，moveDir 已经在世界空间算好了

    // ---------- 1. 水平速度 ----------
    glm::vec3 wishDir = moveDir;
    float len = glm::length(wishDir);
    if (len > 0.001f) {
        wishDir /= len;
    } else {
        wishDir = glm::vec3(0.0f);
    }
    velocity.x = wishDir.x * moveSpeed;
    velocity.z = wishDir.z * moveSpeed;

    // ---------- 2. 跳跃 ----------
    if (jumpPressed && onGround) {
        velocity.y = jumpSpeed;
        onGround = false;
    }

    // ---------- 3. 重力 ----------
    velocity.y -= gravity * dt;

    const float kMaxFallSpeed = 50.0f;
    if (velocity.y < -kMaxFallSpeed) velocity.y = -kMaxFallSpeed;

    // ---------- 4. 计算期望新位置 ----------
    glm::vec3 newPos = position + velocity * dt;

    // ---------- 5. 地面检测（从新位置往下打长射线） ----------
    const float footOffset = kCapsuleHalfHeight + kCapsuleRadius;

    // 射线起点：新位置的胶囊中心
    glm::vec3 rayOrigin = newPos + glm::vec3(0.0f, footOffset, 0.0f);
    float rayLength = footOffset + groundRayMaxDist;

    RaycastHit groundHit = physics.Raycast(
        rayOrigin,
        glm::vec3(0.0f, -1.0f, 0.0f),
        rayLength,
        PhysicsLayer::StaticGeo);

    bool wasOnGround = onGround;
    onGround = false;

    if (groundHit.hit) {
        float groundTopY = groundHit.point.y;

        // 如果新位置的脚底在地面之下（或贴地），就 clamp 到地面
        if (newPos.y <= groundTopY) {
            newPos.y = groundTopY;
            if (velocity.y < 0.0f) velocity.y = 0.0f;
            onGround = true;
        }
    }

    // ---------- 6. 应用位置 ----------
    position = newPos;

    // ---------- 7. 同步到物理世界 ----------
    if (physicsBody.IsValid()) {
        Transform t;
        t.position = GetCapsuleCenter();
        t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        physics.UpdateTransform(physicsBody, t);
    }

    // ---------- 8. 落地帧 ----------
    if (!wasOnGround && onGround) {
        // 未来：落地音效 / 粒子 / 状态机切 Land
    }
}