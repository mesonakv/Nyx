#pragma once
#include "PhysicsTypes.h"
#include "Shapes/Shape.h"
#include "Broadphase/Broadphase.h"
#include "Broadphase/BruteForceBroadphase.h"
#include "Narrowphase/Contact.h"
#include <memory>
#include <vector>

// ============ PhysicsWorld ============
//
// 物理世界的容器。持有所有形状和 Broadphase。
//
// 职责：
//   - 形状的生命周期（创建、销毁、变换更新）
//   - 场景查询（Raycast、Sweep、Overlap）
//   - 生成碰撞对（Broadphase + Narrowphase）
//
// 不职责：
//   - 物理响应（没有刚体求解器）
//   - 角色控制（在 Layer 2）
//
// 句柄策略：
//   - 内部用 vector 存 Entry，删除用懒删除
//   - 句柄 = {index, generation}
//   - 索引复用后 generation++，防止旧句柄误命中
//   - 上限 2^32 次复用一个槽位才会冲突，实际不可达

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // ---------- 形状管理 ----------

    // 创建形状。返回有效句柄。
    ShapeHandle CreateShape(std::unique_ptr<Shape> shape,
                            const Transform& initialTransform,
                            const CollisionFilter& filter);

    // 销毁形状。句柄失效。
    void DestroyShape(ShapeHandle handle);

    // 更新形状的变换（每帧移动后调用）
    void UpdateTransform(ShapeHandle handle, const Transform& t);

    // ---------- 访问 ----------

    const Shape* GetShape(ShapeHandle handle) const;
    const Transform* GetTransform(ShapeHandle handle) const;
    const CollisionFilter* GetFilter(ShapeHandle handle) const;

    bool IsValid(ShapeHandle handle) const;
    size_t GetEntryCount() const { return aliveCount_; }

    // ---------- 查询 ----------

    // 射线检测。返回最近的命中。
    // queryLayer: 只想查询哪些层（作为 category 用的快捷方式，mask 自动取 All）
    RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction,
                       float maxDistance, uint32_t queryCategory) const;

    // 扫掠。形状沿方向移动，返回第一次命中。
    // 如果形状起点就重叠，返回 t=0 的命中。
    SweepHit Sweep(const Shape& shape, const Transform& start,
                   const glm::vec3& direction, float maxDistance,
                   uint32_t queryCategory) const;

    // 重叠。返回所有与给定形状相交的形状句柄。
    void Overlap(const Shape& shape, const Transform& t,
                 uint32_t queryCategory,
                 std::vector<ShapeHandle>& out) const;

    // ---------- 碰撞对 ----------

    // 生成所有候选对，并对每对跑 Narrowphase，输出所有接触。
    void GenerateContacts(std::vector<Contact>& out) const;

    // ---------- 每帧更新 ----------

    // 现在空。将来加刚体求解器时在这里 Step。
    void Step(float dt);

    // 清空所有形状
    void Clear();

private:
    struct Entry {
        std::unique_ptr<Shape> shape;
        Transform transform;
        CollisionFilter filter;
        ShapeHandle handle;
        bool alive = false;
    };

    std::vector<Entry> entries_;
    std::vector<uint32_t> freeIndices_;   // 复用的槽位索引
    size_t aliveCount_ = 0;

    std::unique_ptr<Broadphase> broadphase_;

    // 内部：找 handle 对应的槽位索引，找不到返回 -1
    int FindIndex(ShapeHandle handle) const;

    // 内部：分配一个新句柄
    ShapeHandle AllocHandle();

    // 内部：把 Broadphase 更新为 Entry 的当前 AABB
    void SyncBroadphase(uint32_t index);
};