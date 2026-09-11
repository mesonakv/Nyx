#pragma once

// ============ NyxMath ============
//
// 项目统一的数学接口。
//
// 【重要】文件名不能叫 Math.h，会与系统 <math.h> 冲突（Windows 大小写不敏感）。
//
// 当前是 glm 的薄封装。所有类型和函数都放在 nyx 命名空间下。
//
// 使用策略：
//   - 新代码：使用 nyx:: 命名空间
//   - 老代码：继续用 glm::，两者完全互通
//   - 未来如果要脱离 glm，只需要改这一个文件

#include <cmath>
#include <algorithm>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

namespace nyx {

// ============ 类型别名 ============

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;

using UVec2 = glm::uvec2;
using UVec3 = glm::uvec3;
using UVec4 = glm::uvec4;

using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

using Quat = glm::quat;

// ============ 常量 ============

constexpr float PI          = 3.14159265358979323846f;
constexpr float TwoPi       = PI * 2.0f;
constexpr float HalfPi      = PI * 0.5f;
constexpr float QuarterPi   = PI * 0.25f;
constexpr float Epsilon     = 1e-6f;

// ============ 角度转换 ============

inline float DegToRad(float degrees) { return degrees * PI / 180.0f; }
inline float RadToDeg(float radians) { return radians * 180.0f / PI; }

// ============ 通用工具 ============

inline float Clamp(float value, float lo, float hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

inline int Clamp(int value, int lo, int hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

inline float Min(float a, float b) { return a < b ? a : b; }
inline float Max(float a, float b) { return a > b ? a : b; }
inline int   Min(int a, int b)     { return a < b ? a : b; }
inline int   Max(int a, int b)     { return a > b ? a : b; }

inline float Abs(float v) { return v < 0.0f ? -v : v; }
inline int   Abs(int v)   { return v < 0 ? -v : v; }

// ============ 插值 ============

inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline Vec2 Lerp(const Vec2& a, const Vec2& b, float t) {
    return a + (b - a) * t;
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}

inline Vec4 Lerp(const Vec4& a, const Vec4& b, float t) {
    return a + (b - a) * t;
}

inline float SmoothStep(float edge0, float edge1, float x) {
    float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ============ 比较 ============

inline bool Approximately(float a, float b, float epsilon = Epsilon) {
    return Abs(a - b) < epsilon;
}

inline bool Approximately(const Vec3& a, const Vec3& b, float epsilon = Epsilon) {
    return Approximately(a.x, b.x, epsilon)
        && Approximately(a.y, b.y, epsilon)
        && Approximately(a.z, b.z, epsilon);
}

// ============ 向量快捷函数 ============

inline Vec3 Normalize(const Vec3& v) { return glm::normalize(v); }
inline float Length(const Vec3& v) { return glm::length(v); }
inline float LengthSquared(const Vec3& v) { return glm::dot(v, v); }
inline float Dot(const Vec3& a, const Vec3& b) { return glm::dot(a, b); }
inline Vec3 Cross(const Vec3& a, const Vec3& b) { return glm::cross(a, b); }

// ============ 矩阵快捷函数 ============

inline Mat4 Identity() { return Mat4(1.0f); }

inline Mat4 Translate(const Vec3& t) {
    return glm::translate(Mat4(1.0f), t);
}

inline Mat4 Scale(const Vec3& s) {
    return glm::scale(Mat4(1.0f), s);
}

inline Mat4 Scale(float s) {
    return glm::scale(Mat4(1.0f), Vec3(s));
}

inline Mat4 Rotate(float radians, const Vec3& axis) {
    return glm::rotate(Mat4(1.0f), radians, axis);
}

inline Mat4 Perspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
    return glm::perspective(fovRadians, aspect, nearPlane, farPlane);
}

inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return glm::lookAt(eye, center, up);
}

} // namespace nyx