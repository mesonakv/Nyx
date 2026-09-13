#pragma once
#include "Broadphase.h"

// ============ BruteForceBroadphase ============
//
// 所有形状存一个数组，两两比较。N < 100 时够用。

class BruteForceBroadphase : public Broadphase {
public:
    void Insert(ShapeHandle handle, const AABB& bounds,
                const CollisionFilter& filter) override;
    void Update(ShapeHandle handle, const AABB& bounds) override;
    void Remove(ShapeHandle handle) override;

    void Query(const AABB& bounds, const CollisionFilter& filter,
               std::vector<ShapeHandle>& out) const override;

    void GeneratePairs(std::vector<BroadphasePair>& out) const override;

    void Clear() override;
    size_t GetEntryCount() const override { return aliveCount_; }

private:
    struct Entry {
        ShapeHandle handle;
        AABB bounds;
        CollisionFilter filter;
        bool alive;
    };

    std::vector<Entry> entries_;
    size_t aliveCount_ = 0;

    int FindEntry(ShapeHandle handle) const;
    void CompactIfNeeded();
};