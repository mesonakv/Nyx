#pragma once
#include "Shape.h"

// ============ SphereShape ============
//
// 球体。局部坐标系下球心在原点。

class SphereShape : public Shape {
public:
    explicit SphereShape(float radius) : radius_(radius) {}

    ShapeType GetType() const override { return ShapeType::Sphere; }
    float GetRadius() const { return radius_; }

    AABB GetLocalBounds() const override;
    AABB GetWorldBounds(const Transform& t) const override;
    float GetVolume() const override;

private:
    float radius_;
};