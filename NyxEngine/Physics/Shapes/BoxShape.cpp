#include "BoxShape.h"
#include <cmath>

AABB BoxShape::GetLocalBounds() const {
    AABB b;
    b.min = -halfExtents_;
    b.max = +halfExtents_;
    return b;
}

AABB BoxShape::GetWorldBounds(const Transform& t) const {
    // 世界空间半尺寸 = |R| * halfExtents
    // 其中 |R| 是旋转矩阵取每个元素的绝对值
    glm::mat3 R = glm::mat3_cast(t.rotation);

    glm::mat3 absR(
        glm::abs(R[0]),   // 第一列
        glm::abs(R[1]),
        glm::abs(R[2])
    );

    glm::vec3 worldHalf = absR * halfExtents_;

    AABB b;
    b.min = t.position - worldHalf;
    b.max = t.position + worldHalf;
    return b;
}

float BoxShape::GetVolume() const {
    return 8.0f * halfExtents_.x * halfExtents_.y * halfExtents_.z;
}