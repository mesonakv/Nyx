#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
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
    // B1: view matrix 缓存
    mutable glm::mat4 cachedView_ = glm::mat4(1.0f);
    mutable bool hasCache_ = false;
    mutable float lastYaw_ = 0.0f;
    mutable float lastPitch_ = 0.0f;
    mutable glm::vec3 lastPosition_ = glm::vec3(0.0f);
};