#include "CapsuleShape.h"

AABB CapsuleShape::GetLocalBounds() const {
    AABB b;
    glm::vec3 r(radius_);
    b.min = glm::vec3(0.0f, -halfHeight_, 0.0f) - r;
    b.max = glm::vec3(0.0f, +halfHeight_, 0.0f) + r;
    return b;
}

AABB CapsuleShape::GetWorldBounds(const Transform& t) const {
    // 把两个端点变换到世界空间，用半径扩展包围盒
    glm::vec3 p0 = t.TransformPoint(GetLocalP0());
    glm::vec3 p1 = t.TransformPoint(GetLocalP1());

    AABB b;
    b.min = glm::min(p0, p1) - glm::vec3(radius_);
    b.max = glm::max(p0, p1) + glm::vec3(radius_);
    return b;
}

float CapsuleShape::GetVolume() const {
    const float pi = 3.14159265358979323846f;
    // 圆柱 + 球
    float cylinder = pi * radius_ * radius_ * (2.0f * halfHeight_);
    float sphere   = (4.0f / 3.0f) * pi * radius_ * radius_ * radius_;
    return cylinder + sphere;
}