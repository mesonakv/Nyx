#pragma once
#include "Shape.h"

// ============ BoxShape ============
//
// 盒体。局部坐标系下轴对齐，中心在原点。
//
// 命名说明：
//   - 叫 BoxShape 而不是 AABBShape，因为世界空间下带旋转时，
//     它就不再是 AABB 了（变成 OBB）
//   - 现在只支持"局部轴对齐"，即世界空间下旋转由 Transform 提供
//
// 用例：
//   地面 = BoxShape(glm::vec3(30, 0.1, 30))
//   墙   = BoxShape(glm::vec3(0.5, 5, 10))

class BoxShape : public Shape {
public:
    explicit BoxShape(const glm::vec3& halfExtents)
        : halfExtents_(halfExtents) {}

    ShapeType GetType() const override { return ShapeType::Box; }
    glm::vec3 GetHalfExtents() const { return halfExtents_; }

    AABB GetLocalBounds() const override;
    AABB GetWorldBounds(const Transform& t) const override;
    float GetVolume() const override;

private:
    glm::vec3 halfExtents_;
};