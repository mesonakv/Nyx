#pragma once
class Camera;

class CameraPanel {
public:
    void Initialize(Camera& camera) { camera_ = &camera; }
    void Draw();
private:
    Camera* camera_ = nullptr;
};