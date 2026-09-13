#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // 注意：position 是临时的。等 Player 类加入后，
    // position 应该挪到 Player，Camera 只保留 yaw/pitch + eyeOffset。
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    float sensitivity = 0.003f;
    float fov = 90.0f;

    void ProcessMouseDelta(float dx, float dy);
    glm::vec3 GetDirection() const;
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

private:
    // view matrix 缓存
    mutable glm::mat4 cachedView_ = glm::mat4(1.0f);
    mutable bool viewCacheValid_ = false;
    mutable float lastYaw_ = 0.0f;
    mutable float lastPitch_ = 0.0f;
    mutable glm::vec3 lastPosition_ = glm::vec3(0.0f);

    // projection matrix 缓存
    mutable glm::mat4 cachedProj_ = glm::mat4(1.0f);
    mutable bool projCacheValid_ = false;
    mutable float lastFov_ = 0.0f;
    mutable float lastAspect_ = 0.0f;
};