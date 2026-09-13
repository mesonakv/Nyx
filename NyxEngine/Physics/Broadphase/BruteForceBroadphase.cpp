#include "BruteForceBroadphase.h"

// ============ 插入 / 更新 / 移除 ============

void BruteForceBroadphase::Insert(ShapeHandle handle, const AABB& bounds,
                                  const CollisionFilter& filter) {
    Entry e;
    e.handle = handle;
    e.bounds = bounds;
    e.filter = filter;
    e.alive = true;
    entries_.push_back(e);
    aliveCount_++;
}

void BruteForceBroadphase::Update(ShapeHandle handle, const AABB& bounds) {
    int idx = FindEntry(handle);
    if (idx < 0) return;
    entries_[idx].bounds = bounds;
}

void BruteForceBroadphase::Remove(ShapeHandle handle) {
    int idx = FindEntry(handle);
    if (idx < 0) return;
    if (!entries_[idx].alive) return;

    entries_[idx].alive = false;
    aliveCount_--;

    CompactIfNeeded();
}

int BruteForceBroadphase::FindEntry(ShapeHandle handle) const {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (entries_[i].alive && entries_[i].handle == handle) {
            return (int)i;
        }
    }
    return -1;
}

void BruteForceBroadphase::CompactIfNeeded() {
    if (entries_.empty()) return;
    if (aliveCount_ * 2 >= entries_.size()) return;

    std::vector<Entry> compacted;
    compacted.reserve(aliveCount_);
    for (auto& e : entries_) {
        if (e.alive) {
            compacted.push_back(e);
        }
    }
    entries_ = std::move(compacted);
}

// ============ 查询 ============

void BruteForceBroadphase::Query(const AABB& bounds, const CollisionFilter& filter,
                                 std::vector<ShapeHandle>& out) const {
    for (const auto& e : entries_) {
        if (!e.alive) continue;
        if (!e.filter.CanCollideWith(filter)) continue;
        if (!e.bounds.Overlaps(bounds)) continue;
        out.push_back(e.handle);
    }
}

// ============ 生成候选对 ============

void BruteForceBroadphase::GeneratePairs(std::vector<BroadphasePair>& out) const {
    for (size_t i = 0; i < entries_.size(); i++) {
        if (!entries_[i].alive) continue;

        for (size_t j = i + 1; j < entries_.size(); j++) {
            if (!entries_[j].alive) continue;

            if (!entries_[i].filter.CanCollideWith(entries_[j].filter)) continue;
            if (!entries_[i].bounds.Overlaps(entries_[j].bounds)) continue;

            BroadphasePair p;
            p.a = entries_[i].handle;
            p.b = entries_[j].handle;
            out.push_back(p);
        }
    }
}

// ============ 清空 ============

void BruteForceBroadphase::Clear() {
    entries_.clear();
    aliveCount_ = 0;
}