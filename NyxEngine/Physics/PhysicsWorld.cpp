#include "PhysicsWorld.h"
#include "Narrowphase/Narrowphase.h"
#include <algorithm>
#include <cmath>

// ============ 匿名工具 ============

namespace {

// Ray vs Sphere（解析解）
// ray: p(t) = o + t*d, t ∈ [0, maxT]
// sphere: |p - c|^2 = r^2
bool RaySphere(const glm::vec3& o, const glm::vec3& d, float maxT,
               const glm::vec3& c, float r,
               float& outT, glm::vec3& outNormal)
{
    glm::vec3 m = o - c;
    float b = glm::dot(m, d);
    float cc = glm::dot(m, m) - r * r;

    // 射线起点在球外且指向远离球的方向
    if (cc > 0.0f && b > 0.0f) return false;

    float discr = b * b - cc;
    if (discr < 0.0f) return false;

    float t = -b - std::sqrt(discr);
    if (t < 0.0f) t = 0.0f;      // 起点在球内

    if (t > maxT) return false;

    outT = t;
    glm::vec3 p = o + d * t;
    outNormal = glm::normalize(p - c);
    return true;
}

// Ray vs AABB（slab method），在 AABB 的坐标系下
bool RayAABB(const glm::vec3& o, const glm::vec3& d, float maxT,
             const AABB& box,
             float& outT, glm::vec3& outNormal)
{
    float tmin = 0.0f;
    float tmax = maxT;
    int hitAxis = -1;
    float hitSign = 0.0f;

    for (int i = 0; i < 3; i++) {
        float oi = o[i];
        float di = d[i];
        float mn = box.min[i];
        float mx = box.max[i];

        if (std::abs(di) < 1e-8f) {
            if (oi < mn || oi > mx) return false;
        } else {
            float inv = 1.0f / di;
            float t1 = (mn - oi) * inv;
            float t2 = (mx - oi) * inv;
            float sign = -1.0f;
            if (t1 > t2) {
                std::swap(t1, t2);
                sign = 1.0f;
            }
            if (t1 > tmin) {
                tmin = t1;
                hitAxis = i;
                hitSign = sign;
            }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }

    outT = tmin;
    outNormal = glm::vec3(0.0f);
    if (hitAxis >= 0) {
        outNormal[hitAxis] = hitSign;
    } else {
        outNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return true;
}

// Ray vs Capsule（采样近似）
// 用 kSteps 个球覆盖胶囊线段，取最近命中。
// 精度不完美但足够角色控制器用。
bool RayCapsule(const glm::vec3& o, const glm::vec3& d, float maxT,
                const glm::vec3& p0, const glm::vec3& p1, float r,
                float& outT, glm::vec3& outNormal)
{
    constexpr int kSteps = 8;

    float bestT = maxT + 1.0f;
    glm::vec3 bestN(0.0f, 1.0f, 0.0f);
    bool hitAny = false;

    for (int i = 0; i <= kSteps; i++) {
        float t01 = (float)i / (float)kSteps;
        glm::vec3 sphereCenter = p0 + (p1 - p0) * t01;

        float t;
        glm::vec3 n;
        if (RaySphere(o, d, bestT, sphereCenter, r, t, n)) {
            if (t < bestT) {
                bestT = t;
                bestN = n;
                hitAny = true;
            }
        }
    }

    if (!hitAny) return false;
    outT = bestT;
    outNormal = bestN;
    return true;
}

// 形状到点的距离（用于保守推进）
// 返回接触点、法线、穿透深度。positive 表示有穿透。
struct ShapeDistanceResult {
    bool valid = false;
    glm::vec3 point;
    glm::vec3 normal;
    float distance = 0.0f;   // 负值 = 穿透
};

} // anonymous namespace

// ============ 生命周期 ============

PhysicsWorld::PhysicsWorld() {
    broadphase_ = std::make_unique<BruteForceBroadphase>();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::Clear() {
    entries_.clear();
    freeIndices_.clear();
    aliveCount_ = 0;
    broadphase_->Clear();
}

// ============ 形状管理 ============

ShapeHandle PhysicsWorld::AllocHandle() {
    ShapeHandle h;
    if (!freeIndices_.empty()) {
        uint32_t idx = freeIndices_.back();
        freeIndices_.pop_back();
        h.index = idx;
        h.generation = entries_[idx].handle.generation + 1;  // 递增，旧句柄失效
    } else {
        h.index = (uint32_t)entries_.size();
        h.generation = 1;
        entries_.emplace_back();
    }
    return h;
}

ShapeHandle PhysicsWorld::CreateShape(std::unique_ptr<Shape> shape,
                                      const Transform& initialTransform,
                                      const CollisionFilter& filter)
{
    ShapeHandle h = AllocHandle();

    Entry& e = entries_[h.index];
    e.shape = std::move(shape);
    e.transform = initialTransform;
    e.filter = filter;
    e.handle = h;
    e.alive = true;

    aliveCount_++;

    // 加入 Broadphase
    broadphase_->Insert(h, e.shape->GetWorldBounds(e.transform), e.filter);

    return h;
}

void PhysicsWorld::DestroyShape(ShapeHandle handle) {
    int idx = FindIndex(handle);
    if (idx < 0) return;

    entries_[idx].alive = false;
    entries_[idx].shape.reset();
    aliveCount_--;

    broadphase_->Remove(handle);
    freeIndices_.push_back((uint32_t)idx);
}

void PhysicsWorld::UpdateTransform(ShapeHandle handle, const Transform& t) {
    int idx = FindIndex(handle);
    if (idx < 0) return;

    entries_[idx].transform = t;
    SyncBroadphase((uint32_t)idx);
}

void PhysicsWorld::SyncBroadphase(uint32_t index) {
    Entry& e = entries_[index];
    broadphase_->Update(e.handle, e.shape->GetWorldBounds(e.transform));
}

// ============ 访问 ============

int PhysicsWorld::FindIndex(ShapeHandle handle) const {
    if (!handle.IsValid()) return -1;
    if (handle.index >= entries_.size()) return -1;
    const Entry& e = entries_[handle.index];
    if (!e.alive) return -1;
    if (e.handle != handle) return -1;
    return (int)handle.index;
}

bool PhysicsWorld::IsValid(ShapeHandle handle) const {
    return FindIndex(handle) >= 0;
}

const Shape* PhysicsWorld::GetShape(ShapeHandle handle) const {
    int idx = FindIndex(handle);
    if (idx < 0) return nullptr;
    return entries_[idx].shape.get();
}

const Transform* PhysicsWorld::GetTransform(ShapeHandle handle) const {
    int idx = FindIndex(handle);
    if (idx < 0) return nullptr;
    return &entries_[idx].transform;
}

const CollisionFilter* PhysicsWorld::GetFilter(ShapeHandle handle) const {
    int idx = FindIndex(handle);
    if (idx < 0) return nullptr;
    return &entries_[idx].filter;
}

// ============ Raycast ============

RaycastHit PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction,
                                 float maxDistance, uint32_t queryCategory) const
{
    RaycastHit best;
    best.hit = false;
    best.distance = maxDistance;

    glm::vec3 d = direction;
    float len = glm::length(d);
    if (len < 1e-8f) return best;
    d /= len;

    // 构造查询 filter：category 是 queryCategory，mask 取 All
    CollisionFilter queryFilter = CollisionFilter::Make(queryCategory, PhysicsLayer::All);

    for (const auto& e : entries_) {
        if (!e.alive) continue;
        if (!e.filter.CanCollideWith(queryFilter)) continue;

        float t;
        glm::vec3 normal;
        bool hit = false;

        switch (e.shape->GetType()) {
        case ShapeType::Sphere: {
            const auto& s = static_cast<const SphereShape&>(*e.shape);
            hit = RaySphere(origin, d, best.distance, e.transform.position, s.GetRadius(), t, normal);
            break;
        }
        case ShapeType::Capsule: {
            const auto& c = static_cast<const CapsuleShape&>(*e.shape);
            glm::vec3 p0 = e.transform.TransformPoint(c.GetLocalP0());
            glm::vec3 p1 = e.transform.TransformPoint(c.GetLocalP1());
            hit = RayCapsule(origin, d, best.distance, p0, p1, c.GetRadius(), t, normal);
            break;
        }
        case ShapeType::Box: {
            const auto& b = static_cast<const BoxShape&>(*e.shape);
            // 把射线变换到 box 局部空间
            glm::vec3 localO = e.transform.InverseTransformPoint(origin);
            glm::vec3 localD = e.transform.InverseTransformDirection(d);

            AABB localBox;
            glm::vec3 he = b.GetHalfExtents();
            localBox.min = -he;
            localBox.max = +he;

            hit = RayAABB(localO, localD, best.distance, localBox, t, normal);
            if (hit) {
                // normal 变回世界空间
                normal = e.transform.TransformDirection(normal);
            }
            break;
        }
        }

        if (hit && t < best.distance) {
            best.hit = true;
            best.shape = e.handle;
            best.distance = t;
            best.point = origin + d * t;
            best.normal = normal;
        }
    }

    return best;
}

// ============ Sweep ============

SweepHit PhysicsWorld::Sweep(const Shape& shape, const Transform& start,
                             const glm::vec3& direction, float maxDistance,
                             uint32_t queryCategory) const
{
    SweepHit result;
    result.hit = false;

    float dirLen = glm::length(direction);
    if (dirLen < 1e-8f || maxDistance < 1e-8f) return result;

    glm::vec3 d = direction / dirLen;

    // 保守推进（conservative advancement）：
    // 每步检查当前形状是否和其他形状碰撞。
    // 如果有碰撞，记录命中；如果没有，前进 step 距离。
    // 简单但有效。

    CollisionFilter queryFilter = CollisionFilter::Make(queryCategory, PhysicsLayer::All);

    constexpr float kStepSize = 0.02f;
    constexpr int kMaxSteps = 1024;

    Transform current = start;
    float traveled = 0.0f;

    for (int i = 0; i < kMaxSteps && traveled < maxDistance; i++) {
        // 检查当前形状是否与任何形状重叠
        for (const auto& e : entries_) {
            if (!e.alive) continue;
            if (!e.filter.CanCollideWith(queryFilter)) continue;

            Contact c;
            if (Narrowphase::Test(shape, current, *e.shape, e.transform, c)) {
                result.hit = true;
                result.shape = e.handle;
                result.t = traveled / maxDistance;
                result.point = c.point;
                result.normal = c.normal;
                return result;
            }
        }

        // 前进
        float step = std::min(kStepSize, maxDistance - traveled);
        current.position += d * step;
        traveled += step;
    }

    return result;
}

// ============ Overlap ============

void PhysicsWorld::Overlap(const Shape& shape, const Transform& t,
                           uint32_t queryCategory,
                           std::vector<ShapeHandle>& out) const
{
    CollisionFilter queryFilter = CollisionFilter::Make(queryCategory, PhysicsLayer::All);

    for (const auto& e : entries_) {
        if (!e.alive) continue;
        if (!e.filter.CanCollideWith(queryFilter)) continue;

        Contact c;
        if (Narrowphase::Test(shape, t, *e.shape, e.transform, c)) {
            out.push_back(e.handle);
        }
    }
}

// ============ 碰撞对 ============

void PhysicsWorld::GenerateContacts(std::vector<Contact>& out) const {
    // 1. Broadphase 生成候选对
    std::vector<BroadphasePair> pairs;
    broadphase_->GeneratePairs(pairs);

    // 2. 对每对跑 Narrowphase
    out.clear();
    out.reserve(pairs.size());

    for (const auto& p : pairs) {
        int ia = FindIndex(p.a);
        int ib = FindIndex(p.b);
        if (ia < 0 || ib < 0) continue;

        Contact c;
        if (Narrowphase::Test(*entries_[ia].shape, entries_[ia].transform,
                              *entries_[ib].shape, entries_[ib].transform,
                              c)) {
            c.a = p.a;
            c.b = p.b;
            out.push_back(c);
        }
    }
}

// ============ 每帧 ============

void PhysicsWorld::Step(float dt) {
    // 现在空。将来加刚体求解器时在这里实现。
    (void)dt;
}