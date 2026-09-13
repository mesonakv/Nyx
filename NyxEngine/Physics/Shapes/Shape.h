#pragma once
#include "../PhysicsTypes.h"

// ============ Shape ============
//
// 碰撞形状的抽象基类。
//
// 每个形状只知道"在局部坐标系下我是什么"。
// 世界空间的位置由 Transform 提供。
//
// 设计原则：
//   - 虚函数只用于"描述自己"（GetLocalBounds、GetVolume），
//     这些每帧每形状调用 1-2 次，成本可忽略
//   - 碰撞检测不用虚函数分派，用 type switch + 具体函数
//     （因为每一对形状需要专用算法，虚函数帮不上忙）

enum class ShapeType : uint8_t {
    Sphere,
    Capsule,
    Box,
    // 将来：
    // OBB, Mesh
};

class Shape {
public:
    virtual ~Shape() = default;

    virtual ShapeType GetType() const = 0;

    // 局部坐标系下的 AABB（不含 transform）
    virtual AABB GetLocalBounds() const = 0;

    // 世界空间的 AABB（含 transform）
    virtual AABB GetWorldBounds(const Transform& t) const = 0;

    // 体积（刚体求惯性张量用；角色控制器不用）
    virtual float GetVolume() const = 0;
};