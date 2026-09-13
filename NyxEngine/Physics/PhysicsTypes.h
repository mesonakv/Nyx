#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ============ PhysicsTypes ============
//
// 物理系统的基础类型。不依赖任何具体形状或算法。

// ---------- 形状句柄 ----------

struct ShapeHandle {
    uint32_t index = 0;
    uint32_t generation = 0;

    bool IsValid() const { return generation != 0; }

    bool operator==(const ShapeHandle& o) const {
        return index == o.index && generation == o.generation;
    }
    bool operator!=(const ShapeHandle& o) const { return !(*this == o); }
};

// ---------- AABB ----------

struct AABB {
    glm::vec3 min = glm::vec3( 1e30f);
    glm::vec3 max = glm::vec3(-1e30f);

    bool IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
    glm::vec3 GetExtents() const { return (max - min) * 0.5f; }

    void ExpandToInclude(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    void ExpandToInclude(const AABB& o) {
        if (!o.IsValid()) return;
        min = glm::min(min, o.min);
        max = glm::max(max, o.max);
    }

    void Expand(float amount) {
        min -= glm::vec3(amount);
        max += glm::vec3(amount);
    }

    bool Overlaps(const AABB& o) const {
        if (max.x < o.min.x || min.x > o.max.x) return false;
        if (max.y < o.min.y || min.y > o.max.y) return false;
        if (max.z < o.min.z || min.z > o.max.z) return false;
        return true;
    }

    bool Contains(const glm::vec3& p) const {
        return p.x >= min.x && p.x <= max.x
            && p.y >= min.y && p.y <= max.y
            && p.z >= min.z && p.z <= max.z;
    }
};

// ---------- Transform ----------

struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 TransformPoint(const glm::vec3& p) const {
        return position + rotation * p;
    }

    glm::vec3 TransformDirection(const glm::vec3& d) const {
        return rotation * d;
    }

    glm::vec3 InverseTransformPoint(const glm::vec3& p) const {
        return glm::conjugate(rotation) * (p - position);
    }

    glm::vec3 InverseTransformDirection(const glm::vec3& d) const {
        return glm::conjugate(rotation) * d;
    }
};

// ---------- LayerMask ----------

namespace PhysicsLayer {
    constexpr uint32_t Default    = 1u << 0;
    constexpr uint32_t Player     = 1u << 1;
    constexpr uint32_t Target     = 1u << 2;
    constexpr uint32_t Projectile = 1u << 3;
    constexpr uint32_t StaticGeo  = 1u << 4;
    constexpr uint32_t Boss       = 1u << 5;
    constexpr uint32_t Debris     = 1u << 6;
    constexpr uint32_t All        = 0xFFFFFFFFu;
}

// ---------- 碰撞过滤 ----------
//
// 每个形状有 category（我属于哪些层）和 mask（我愿意和哪些层碰撞）。
// 两个形状 a、b 可以碰撞的条件：
//   (a.category & b.mask) != 0 && (b.category & a.mask) != 0
//
// 示例：
//   玩家：category = Player, mask = StaticGeo | Target | Boss
//   地面：category = StaticGeo, mask = Player | Projectile | Debris
//   玩家-地面：匹配
//   玩家-玩家：不匹配（玩家不关心自己）

struct CollisionFilter {
    uint32_t category = PhysicsLayer::Default;
    uint32_t mask = PhysicsLayer::All;

    bool CanCollideWith(const CollisionFilter& o) const {
        return (category & o.mask) != 0 && (o.category & mask) != 0;
    }

    // 便捷构造
    static CollisionFilter Make(uint32_t category, uint32_t mask) {
        CollisionFilter f;
        f.category = category;
        f.mask = mask;
        return f;
    }
};

// ---------- Broadphase 输出 ----------

struct BroadphasePair {
    ShapeHandle a;
    ShapeHandle b;

    bool operator==(const BroadphasePair& o) const {
        return (a == o.a && b == o.b) || (a == o.b && b == o.a);
    }
};

// ---------- 射线检测结果 ----------

struct RaycastHit {
    bool hit = false;
    ShapeHandle shape;
    glm::vec3 point = glm::vec3(0.0f);        // 世界空间命中点
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);  // 表面法线（指向射线来的方向）
    float distance = 0.0f;                    // 从原点到命中点的距离
};

// ---------- 扫掠检测结果 ----------

struct SweepHit {
    bool hit = false;
    ShapeHandle shape;
    float t = 0.0f;                           // 参数 [0, 1]，命中时形状中心在 start + dir*t
    glm::vec3 point = glm::vec3(0.0f);        // 接触点
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
};