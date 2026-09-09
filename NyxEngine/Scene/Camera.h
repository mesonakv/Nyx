#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL.h>

class Camera {
public:
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    float sensitivity = 0.003f;
    float fov = 90.0f;

    void ProcessMouse(SDL_Event& event);
    glm::vec3 GetDirection() const;
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;
};