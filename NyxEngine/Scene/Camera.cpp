#include "Camera.h"
#include <algorithm>
#include <cmath>

void Camera::ProcessMouseDelta(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch -= dy * sensitivity;

    const float maxPitch = glm::radians(89.0f);
    const float minPitch = glm::radians(-89.0f);
    pitch = std::max(minPitch, std::min(maxPitch, pitch));

    const float twoPi = 2.0f * 3.14159265358979323846f;
    yaw = fmod(yaw, twoPi);
}

glm::vec3 Camera::GetDirection() const {
    glm::vec3 dir;
    dir.x = cos(pitch) * sin(yaw);
    dir.y = sin(pitch);
    dir.z = -cos(pitch) * cos(yaw);
    return glm::normalize(dir);
}

glm::mat4 Camera::GetViewMatrix() const {
    // B1: yaw/pitch/position 都没变时返回缓存
    if (hasCache_ &&
        lastYaw_ == yaw &&
        lastPitch_ == pitch &&
        lastPosition_ == position) {
        return cachedView_;
    }

    glm::vec3 dir = GetDirection();
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));
    cachedView_ = glm::lookAt(position, position + dir, up);

    lastYaw_ = yaw;
    lastPitch_ = pitch;
    lastPosition_ = position;
    hasCache_ = true;

    return cachedView_;
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1.0f;   // Vulkan 需要翻转 Y 轴
    return proj;
}