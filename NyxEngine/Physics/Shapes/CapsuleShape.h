#pragma once
#include "Shape.h"

// ============ CapsuleShape ============
//
// 胶囊体：沿局部 Y 轴。圆柱 + 上下两个半球。
//
// 参数：
//   radius     — 半径
//   halfHeight — 中心到上下球心的距离
//
// 局部坐标系：
//   下端球心 = (0, -halfHeight, 0)
//   上端球心 = (0, +halfHeight, 0)
//   总高度   = 2 * (halfHeight + radius)
//
// 用例：
//   玩家 = CapsuleShape(0.4f, 0.85f)
//     总高 1.7 米，半径 0.4 米

class CapsuleShape : public Shape {
public:
    CapsuleShape(float radius, float halfHeight)
        : radius_(radius), halfHeight_(halfHeight) {}

    ShapeType GetType() const override { return ShapeType::Capsule; }

    float GetRadius() const { return radius_; }
    float GetHalfHeight() const { return halfHeight_; }

    // 局部坐标系下的两个端点
    glm::vec3 GetLocalP0() const { return glm::vec3(0.0f, -halfHeight_, 0.0f); }
    glm::vec3 GetLocalP1() const { return glm::vec3(0.0f, +halfHeight_, 0.0f); }

    AABB GetLocalBounds() const override;
    AABB GetWorldBounds(const Transform& t) const override;
    float GetVolume() const override;

private:
    float radius_;
    float halfHeight_;
};