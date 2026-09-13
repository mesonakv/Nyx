#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ============ Camera ============
//
// 相机只负责"看"：yaw / pitch / fov / sensitivity。
// 眼睛位置由调用者提供（通常来自 Player::GetEyePosition()）。
//
// 为什么位置不在这里：
//   - 位置属于 Player，相机不该持有别人的位置
//   - 编辑器可以用同一个 Camera 观察任意位置，不依赖 Player
//   - 数据流单向：Player 决定"在哪"，Camera 决定"看哪"

class Camera {
public:
    float yaw = 0.0f;
    float pitch = 0.0f;
    float sensitivity = 0.003f;
    float fov = 90.0f;

    void ProcessMouseDelta(float dx, float dy);
    glm::vec3 GetDirection() const;

    // eyePos：相机在世界空间的位置（一般来自 Player::GetEyePosition()）
    glm::mat4 GetViewMatrix(const glm::vec3& eyePos) const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

private:
    // view matrix 缓存
    mutable glm::mat4 cachedView_ = glm::mat4(1.0f);
    mutable bool viewCacheValid_ = false;
    mutable float lastYaw_ = 0.0f;
    mutable float lastPitch_ = 0.0f;
    mutable glm::vec3 lastEyePos_ = glm::vec3(0.0f);

    // projection matrix 缓存
    mutable glm::mat4 cachedProj_ = glm::mat4(1.0f);
    mutable bool projCacheValid_ = false;
    mutable float lastFov_ = 0.0f;
    mutable float lastAspect_ = 0.0f;
};