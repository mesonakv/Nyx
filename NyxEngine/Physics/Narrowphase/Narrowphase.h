#pragma once
#include "Contact.h"
#include "../Shapes/Shape.h"
#include "../Shapes/SphereShape.h"
#include "../Shapes/CapsuleShape.h"
#include "../Shapes/BoxShape.h"

// ============ Narrowphase ============
//
// 精确碰撞检测。
//
// 输入：两个形状 + 它们的世界 transform
// 输出：是否碰撞，以及接触信息
//
// 实现方式：
//   - 每个形状对有一个专用函数
//   - Test 用 type switch 分派
//   - 对称对（A-B 和 B-A）只写一份，另一份交换后翻转 normal
//
// 支持的形状对：
//   - Sphere-Sphere
//   - Capsule-Sphere
//   - Sphere-Box
//   - Capsule-Capsule
//   - Capsule-Box

class Narrowphase {
public:
    static bool Test(const Shape& a, const Transform& ta,
                     const Shape& b, const Transform& tb,
                     Contact& out);

    static bool TestRaw(const Shape& a, const Transform& ta,
                        const Shape& b, const Transform& tb,
                        glm::vec3& outPoint, glm::vec3& outNormal,
                        float& outPenetration);

private:
    // ---------- 具体形状对 ----------
    static bool TestSphereSphere(
        const SphereShape& a, const Transform& ta,
        const SphereShape& b, const Transform& tb,
        glm::vec3& outPoint, glm::vec3& outNormal, float& outPen);

    static bool TestCapsuleSphere(
        const CapsuleShape& a, const Transform& ta,
        const SphereShape& b, const Transform& tb,
        glm::vec3& outPoint, glm::vec3& outNormal, float& outPen);

    static bool TestSphereBox(
        const SphereShape& a, const Transform& ta,
        const BoxShape& b, const Transform& tb,
        glm::vec3& outPoint, glm::vec3& outNormal, float& outPen);

    static bool TestCapsuleCapsule(
        const CapsuleShape& a, const Transform& ta,
        const CapsuleShape& b, const Transform& tb,
        glm::vec3& outPoint, glm::vec3& outNormal, float& outPen);

    static bool TestCapsuleBox(
        const CapsuleShape& a, const Transform& ta,
        const BoxShape& b, const Transform& tb,
        glm::vec3& outPoint, glm::vec3& outNormal, float& outPen);

    // ---------- 几何工具 ----------

    static float ClosestPointOnSegment(
        const glm::vec3& p,
        const glm::vec3& s0, const glm::vec3& s1,
        glm::vec3& outClosest);

    static glm::vec3 ClosestPointOnAABB(
        const glm::vec3& p, const AABB& box);
};