#pragma once
#include "../PhysicsTypes.h"
#include <vector>

// ============ Broadphase ============
//
// 粗筛层。快速过滤不可能碰撞的形状对。
//
// 契约：
//   - Insert / Update / Remove 由 PhysicsWorld 调用
//   - Query / GeneratePairs 返回的候选对可能包含假阳性
//     （AABB 重叠但形状不重叠），由 Narrowphase 精确检测
//   - 不负责接触生成、不负责物理响应

class Broadphase {
public:
    virtual ~Broadphase() = default;

    virtual void Insert(ShapeHandle handle, const AABB& bounds,
                        const CollisionFilter& filter) = 0;

    virtual void Update(ShapeHandle handle, const AABB& bounds) = 0;

    virtual void Remove(ShapeHandle handle) = 0;

    virtual void Query(const AABB& bounds, const CollisionFilter& filter,
                       std::vector<ShapeHandle>& out) const = 0;

    virtual void GeneratePairs(std::vector<BroadphasePair>& out) const = 0;

    virtual void Clear() = 0;

    virtual size_t GetEntryCount() const = 0;
};