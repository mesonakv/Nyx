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
//
// 移动模型（Source 引擎风格）：
//   - 水平移动分地面/空中两种加速度
//   - 地面有摩擦，松开方向键后滑行停下
//   - 跳跃有 coyote time（离地后仍可跳的窗口）
//     和 jump buffer（提前按跳跃的缓冲）
//   - 松开跳跃键可截断跳跃（jump cut）

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
    // 这些参数可以被编辑器实时修改，改动立刻生效。

    // 水平移动
    float maxSpeed = 8.0f;           // 最大水平速度（m/s）
    float groundAccel = 60.0f;       // 地面加速度（m/s²）
    float groundFriction = 8.0f;     // 地面摩擦系数
    float airAccel = 15.0f;          // 空中加速度（m/s²）

    // 跳跃
    float jumpSpeed = 8.0f;          // 跳跃初速度（m/s）
    float gravity = 25.0f;           // 重力加速度（m/s²）
    float jumpCutMultiplier = 0.5f;  // 松开跳跃键时的速度衰减
    float coyoteTime = 0.1f;         // 离地后仍可跳的窗口（秒）
    float jumpBuffer = 0.1f;         // 提前按跳跃的缓冲（秒）

    // 地面检测
    float groundRayMaxDist = 6.0f;

    // ---------- 查询 ----------
    glm::vec3 GetEyePosition() const {
        return position + glm::vec3(0.0f, eyeHeight, 0.0f);
    }

    glm::vec3 GetCapsuleCenter() const {
        return position + glm::vec3(0.0f, kCapsuleHalfHeight + kCapsuleRadius, 0.0f);
    }

    // ---------- 更新 ----------
    // moveDir：世界空间的期望移动方向（水平，可未归一化）
    // jumpPressed：本帧刚按下跳跃键
    // jumpHeld：跳跃键当前是否按住
    // yaw：相机偏航角（暂未使用，moveDir 已在世界空间）
    // physics：物理世界
    void Update(float dt,
                const glm::vec3& moveDir,
                bool jumpPressed,
                bool jumpHeld,
                float yaw,
                PhysicsWorld& physics);

private:
    // 跳跃状态
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    bool jumping_ = false;   // 用于 jump cut

    void ApplyFriction(float dt);
    void Accelerate(const glm::vec3& wishDir, float accel, float maxSpeed, float dt);
};