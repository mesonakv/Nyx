#include "Camera.h"
#include <algorithm>

void Camera::ProcessMouse(SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        // 修复：鼠标右移视角右转
        yaw += event.motion.xrel * sensitivity;
        pitch -= event.motion.yrel * sensitivity;

        const float maxPitch = glm::radians(89.0f);
        const float minPitch = glm::radians(-89.0f);
        pitch = std::max(minPitch, std::min(maxPitch, pitch));
    }
}

glm::vec3 Camera::GetDirection() const {
    glm::vec3 dir;
    dir.x = cos(pitch) * sin(yaw);
    dir.y = sin(pitch);
    dir.z = -cos(pitch) * cos(yaw);
    return glm::normalize(dir);
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::vec3 dir = GetDirection();
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));
    return glm::lookAt(position, position + dir, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1.0f;   // Vulkan 需要翻转 Y 轴
    return proj;
}