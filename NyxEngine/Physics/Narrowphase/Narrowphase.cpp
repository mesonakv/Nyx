#include "Narrowphase.h"
#include <algorithm>
#include <cmath>

// ============ 内部工具 ============

namespace {

// 两条线段上最近的一对点。
// 基于 Ericson《Real-Time Collision Detection》5.1.9。
// 返回两个点的距离平方，s 和 t 分别是两条线段上的参数 [0, 1]。
float ClosestPtSegmentSegment(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& q0, const glm::vec3& q1,
    float& outS, float& outT,
    glm::vec3& outC0, glm::vec3& outC1)
{
    glm::vec3 d1 = p1 - p0;
    glm::vec3 d2 = q1 - q0;
    glm::vec3 r = p0 - q0;

    float a = glm::dot(d1, d1);
    float e = glm::dot(d2, d2);
    float f = glm::dot(d2, r);

    float s, t;

    // 处理退化情况：两条线段都是点
    if (a <= 1e-12f && e <= 1e-12f) {
        s = t = 0.0f;
        outC0 = p0;
        outC1 = q0;
        outS = s;
        outT = t;
        glm::vec3 diff = outC0 - outC1;
        return glm::dot(diff, diff);
    }

    if (a <= 1e-12f) {
        // 第一条退化成点
        s = 0.0f;
        t = f / e;
        t = std::max(0.0f, std::min(1.0f, t));
    } else {
        float c = glm::dot(d1, r);
        if (e <= 1e-12f) {
            // 第二条退化成点
            t = 0.0f;
            s = std::max(0.0f, std::min(1.0f, -c / a));
        } else {
            float b = glm::dot(d1, d2);
            float denom = a * e - b * b;

            if (denom > 1e-12f) {
                s = (b * f - c * e) / denom;
                s = std::max(0.0f, std::min(1.0f, s));
            } else {
                // 平行
                s = 0.0f;
            }

            t = (b * s + f) / e;

            if (t < 0.0f) {
                t = 0.0f;
                s = std::max(0.0f, std::min(1.0f, -c / a));
            } else if (t > 1.0f) {
                t = 1.0f;
                s = std::max(0.0f, std::min(1.0f, (b - c) / a));
            }
        }
    }

    outS = s;
    outT = t;
    outC0 = p0 + d1 * s;
    outC1 = q0 + d2 * t;

    glm::vec3 diff = outC0 - outC1;
    return glm::dot(diff, diff);
}

} // anonymous namespace

// ============ 分派入口 ============

bool Narrowphase::Test(const Shape& a, const Transform& ta,
                       const Shape& b, const Transform& tb,
                       Contact& out) {
    ShapeType at = a.GetType();
    ShapeType bt = b.GetType();

    glm::vec3 point, normal;
    float pen = 0.0f;
    bool hit = false;

    if (at == ShapeType::Sphere && bt == ShapeType::Sphere) {
        hit = TestSphereSphere(
            static_cast<const SphereShape&>(a), ta,
            static_cast<const SphereShape&>(b), tb,
            point, normal, pen);
    }
    else if (at == ShapeType::Capsule && bt == ShapeType::Sphere) {
        hit = TestCapsuleSphere(
            static_cast<const CapsuleShape&>(a), ta,
            static_cast<const SphereShape&>(b), tb,
            point, normal, pen);
    }
    else if (at == ShapeType::Sphere && bt == ShapeType::Capsule) {
        hit = TestCapsuleSphere(
            static_cast<const CapsuleShape&>(b), tb,
            static_cast<const SphereShape&>(a), ta,
            point, normal, pen);
        if (hit) normal = -normal;
    }
    else if (at == ShapeType::Sphere && bt == ShapeType::Box) {
        hit = TestSphereBox(
            static_cast<const SphereShape&>(a), ta,
            static_cast<const BoxShape&>(b), tb,
            point, normal, pen);
    }
    else if (at == ShapeType::Box && bt == ShapeType::Sphere) {
        hit = TestSphereBox(
            static_cast<const SphereShape&>(b), tb,
            static_cast<const BoxShape&>(a), ta,
            point, normal, pen);
        if (hit) normal = -normal;
    }
    else if (at == ShapeType::Capsule && bt == ShapeType::Capsule) {
        hit = TestCapsuleCapsule(
            static_cast<const CapsuleShape&>(a), ta,
            static_cast<const CapsuleShape&>(b), tb,
            point, normal, pen);
    }
    else if (at == ShapeType::Capsule && bt == ShapeType::Box) {
        hit = TestCapsuleBox(
            static_cast<const CapsuleShape&>(a), ta,
            static_cast<const BoxShape&>(b), tb,
            point, normal, pen);
    }
    else if (at == ShapeType::Box && bt == ShapeType::Capsule) {
        hit = TestCapsuleBox(
            static_cast<const CapsuleShape&>(b), tb,
            static_cast<const BoxShape&>(a), ta,
            point, normal, pen);
        if (hit) normal = -normal;
    }

    if (hit) {
        out.point = point;
        out.normal = normal;
        out.penetration = pen;
    }
    return hit;
}

bool Narrowphase::TestRaw(const Shape& a, const Transform& ta,
                          const Shape& b, const Transform& tb,
                          glm::vec3& outPoint, glm::vec3& outNormal,
                          float& outPenetration) {
    Contact c;
    if (Test(a, ta, b, tb, c)) {
        outPoint = c.point;
        outNormal = c.normal;
        outPenetration = c.penetration;
        return true;
    }
    return false;
}

// ============ Sphere-Sphere ============

bool Narrowphase::TestSphereSphere(
    const SphereShape& a, const Transform& ta,
    const SphereShape& b, const Transform& tb,
    glm::vec3& outPoint, glm::vec3& outNormal, float& outPen)
{
    glm::vec3 d = tb.position - ta.position;
    float distSq = glm::dot(d, d);
    float radiiSum = a.GetRadius() + b.GetRadius();

    if (distSq >= radiiSum * radiiSum) return false;

    float dist = std::sqrt(std::max(distSq, 1e-12f));
    outPen = radiiSum - dist;

    if (dist > 1e-6f) {
        outNormal = d / dist;
    } else {
        outNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 aSurface = ta.position + outNormal * a.GetRadius();
    glm::vec3 bSurface = tb.position - outNormal * b.GetRadius();
    outPoint = (aSurface + bSurface) * 0.5f;

    return true;
}

// ============ Capsule-Sphere ============

bool Narrowphase::TestCapsuleSphere(
    const CapsuleShape& a, const Transform& ta,
    const SphereShape& b, const Transform& tb,
    glm::vec3& outPoint, glm::vec3& outNormal, float& outPen)
{
    glm::vec3 p0 = ta.TransformPoint(a.GetLocalP0());
    glm::vec3 p1 = ta.TransformPoint(a.GetLocalP1());

    glm::vec3 closest;
    ClosestPointOnSegment(tb.position, p0, p1, closest);

    glm::vec3 d = tb.position - closest;
    float distSq = glm::dot(d, d);
    float radiiSum = a.GetRadius() + b.GetRadius();

    if (distSq >= radiiSum * radiiSum) return false;

    float dist = std::sqrt(std::max(distSq, 1e-12f));
    outPen = radiiSum - dist;

    if (dist > 1e-6f) {
        outNormal = d / dist;
    } else {
        outNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 aSurface = closest + outNormal * a.GetRadius();
    glm::vec3 bSurface = tb.position - outNormal * b.GetRadius();
    outPoint = (aSurface + bSurface) * 0.5f;

    return true;
}

// ============ Sphere-Box ============

bool Narrowphase::TestSphereBox(
    const SphereShape& a, const Transform& ta,
    const BoxShape& b, const Transform& tb,
    glm::vec3& outPoint, glm::vec3& outNormal, float& outPen)
{
    glm::vec3 localCenter = tb.InverseTransformPoint(ta.position);

    glm::vec3 he = b.GetHalfExtents();
    AABB localBox;
    localBox.min = -he;
    localBox.max = +he;

    glm::vec3 localClosest = ClosestPointOnAABB(localCenter, localBox);

    glm::vec3 localD = localCenter - localClosest;
    float distSq = glm::dot(localD, localD);
    float r = a.GetRadius();

    if (distSq >= r * r) return false;

    float dist = std::sqrt(std::max(distSq, 1e-12f));
    outPen = r - dist;

    glm::vec3 localNormal;
    if (dist > 1e-6f) {
        localNormal = localD / dist;
    } else {
        // 球心在 box 内部：找最浅面
        glm::vec3 dMin = localCenter - localBox.min;
        glm::vec3 dMax = localBox.max - localCenter;

        float best = dMin.x;
        localNormal = glm::vec3(-1.0f, 0.0f, 0.0f);
        if (dMax.x < best) { best = dMax.x; localNormal = glm::vec3( 1.0f, 0.0f, 0.0f); }
        if (dMin.y < best) { best = dMin.y; localNormal = glm::vec3( 0.0f,-1.0f, 0.0f); }
        if (dMax.y < best) { best = dMax.y; localNormal = glm::vec3( 0.0f, 1.0f, 0.0f); }
        if (dMin.z < best) { best = dMin.z; localNormal = glm::vec3( 0.0f, 0.0f,-1.0f); }
        if (dMax.z < best) { best = dMax.z; localNormal = glm::vec3( 0.0f, 0.0f, 1.0f); }

        outPen = r + best;
    }

    // normal 是"从 box 表面指向球心"，但我们约定从 a（球）指向 b（box），
    // 所以要取反
    outNormal = -tb.TransformDirection(localNormal);

    outPoint = ta.position - outNormal * r;

    return true;
}

// ============ Capsule-Capsule ============

bool Narrowphase::TestCapsuleCapsule(
    const CapsuleShape& a, const Transform& ta,
    const CapsuleShape& b, const Transform& tb,
    glm::vec3& outPoint, glm::vec3& outNormal, float& outPen)
{
    glm::vec3 pa0 = ta.TransformPoint(a.GetLocalP0());
    glm::vec3 pa1 = ta.TransformPoint(a.GetLocalP1());
    glm::vec3 pb0 = tb.TransformPoint(b.GetLocalP0());
    glm::vec3 pb1 = tb.TransformPoint(b.GetLocalP1());

    float s, t;
    glm::vec3 ca, cb;
    float distSq = ClosestPtSegmentSegment(pa0, pa1, pb0, pb1, s, t, ca, cb);

    float radiiSum = a.GetRadius() + b.GetRadius();
    if (distSq >= radiiSum * radiiSum) return false;

    float dist = std::sqrt(std::max(distSq, 1e-12f));
    outPen = radiiSum - dist;

    if (dist > 1e-6f) {
        outNormal = (cb - ca) / dist;   // 从 a 指向 b
    } else {
        outNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 aSurface = ca + outNormal * a.GetRadius();
    glm::vec3 bSurface = cb - outNormal * b.GetRadius();
    outPoint = (aSurface + bSurface) * 0.5f;

    return true;
}

// ============ Capsule-Box ============

bool Narrowphase::TestCapsuleBox(
    const CapsuleShape& a, const Transform& ta,
    const BoxShape& b, const Transform& tb,
    glm::vec3& outPoint, glm::vec3& outNormal, float& outPen)
{
    // 策略：把胶囊的两个端点变换到 box 局部空间，
    // 在局部空间做"线段到 AABB 最近点"的采样+精化。
    //
    // 采样法而不是解析解：代码短、鲁棒、精度可控。

    glm::vec3 p0w = ta.TransformPoint(a.GetLocalP0());
    glm::vec3 p1w = ta.TransformPoint(a.GetLocalP1());

    glm::vec3 p0 = tb.InverseTransformPoint(p0w);
    glm::vec3 p1 = tb.InverseTransformPoint(p1w);

    glm::vec3 he = b.GetHalfExtents();
    AABB localBox;
    localBox.min = -he;
    localBox.max = +he;

    // ---------- 采样找最小距离的参数 t ----------
    auto evalAt = [&](float t, glm::vec3& segPt, glm::vec3& boxPt) -> float {
        segPt = p0 + (p1 - p0) * t;
        boxPt = ClosestPointOnAABB(segPt, localBox);
        glm::vec3 d = segPt - boxPt;
        return glm::dot(d, d);
    };

    const int kCoarse = 32;
    float bestT = 0.0f;
    float bestDistSq = 1e30f;

    for (int i = 0; i <= kCoarse; i++) {
        float t = (float)i / (float)kCoarse;
        glm::vec3 sp, bp;
        float d = evalAt(t, sp, bp);
        if (d < bestDistSq) {
            bestDistSq = d;
            bestT = t;
        }
    }

    // 局部精化
    float lo = std::max(0.0f, bestT - 1.0f / (float)kCoarse);
    float hi = std::min(1.0f, bestT + 1.0f / (float)kCoarse);

    for (int round = 0; round < 3; round++) {
        const int kFine = 16;
        for (int i = 0; i <= kFine; i++) {
            float t = lo + (hi - lo) * (float)i / (float)kFine;
            glm::vec3 sp, bp;
            float d = evalAt(t, sp, bp);
            if (d < bestDistSq) {
                bestDistSq = d;
                bestT = t;
            }
        }
        float span = (hi - lo) / (float)kFine * 2.0f;
        lo = std::max(0.0f, bestT - span);
        hi = std::min(1.0f, bestT + span);
    }

    glm::vec3 localSegPt = p0 + (p1 - p0) * bestT;
    glm::vec3 localBoxPt = ClosestPointOnAABB(localSegPt, localBox);

    float r = a.GetRadius();
    if (bestDistSq >= r * r) return false;

    float dist = std::sqrt(std::max(bestDistSq, 1e-12f));
    outPen = r - dist;

    glm::vec3 localNormal;   // 从 box 指向线段
    if (dist > 1e-6f) {
        localNormal = (localSegPt - localBoxPt) / dist;
    } else {
        // 线段穿过 box：取 box 中心指向线段点的方向
        glm::vec3 center = (localBox.min + localBox.max) * 0.5f;
        glm::vec3 dir = localSegPt - center;
        float lenSq = glm::dot(dir, dir);
        if (lenSq > 1e-12f) {
            localNormal = dir / std::sqrt(lenSq);
        } else {
            localNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        outPen = r + dist;
    }

    // 变换回世界空间。
    // normal 约定：从 a（capsule）指向 b（box）。
    // localNormal 是"从 box 指向 capsule"，所以取反。
    outNormal = -tb.TransformDirection(localNormal);

    // 接触点：capsule 表面点 和 box 表面点的中点
    glm::vec3 aSurfaceLocal = localSegPt + localNormal * r;
    glm::vec3 bSurfaceLocal = localBoxPt;
    glm::vec3 midLocal = (aSurfaceLocal + bSurfaceLocal) * 0.5f;
    outPoint = tb.TransformPoint(midLocal);

    return true;
}

// ============ 几何工具 ============

float Narrowphase::ClosestPointOnSegment(
    const glm::vec3& p,
    const glm::vec3& s0, const glm::vec3& s1,
    glm::vec3& outClosest)
{
    glm::vec3 d = s1 - s0;
    float lenSq = glm::dot(d, d);

    if (lenSq < 1e-12f) {
        outClosest = s0;
        return 0.0f;
    }

    float t = glm::dot(p - s0, d) / lenSq;
    t = std::max(0.0f, std::min(1.0f, t));

    outClosest = s0 + d * t;
    return t;
}

glm::vec3 Narrowphase::ClosestPointOnAABB(
    const glm::vec3& p, const AABB& box)
{
    return glm::clamp(p, box.min, box.max);
}