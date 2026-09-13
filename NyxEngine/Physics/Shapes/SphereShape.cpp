#include "SphereShape.h"

AABB SphereShape::GetLocalBounds() const {
    AABB b;
    b.min = glm::vec3(-radius_);
    b.max = glm::vec3( radius_);
    return b;
}

AABB SphereShape::GetWorldBounds(const Transform& t) const {
    AABB b;
    b.min = t.position - glm::vec3(radius_);
    b.max = t.position + glm::vec3(radius_);
    return b;
}

float SphereShape::GetVolume() const {
    return (4.0f / 3.0f) * 3.14159265358979323846f * radius_ * radius_ * radius_;
}