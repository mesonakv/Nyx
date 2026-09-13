#include "DebugDraw.h"
#include "../Physics/PhysicsTypes.h"
#include <cstring>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

// 生成一个圆（给定圆心、半径、两个轴方向、分段数）
void AppendCircle(std::vector<DebugDraw::Vertex>& out,
                  const glm::vec3& center,
                  const glm::vec3& axisU, const glm::vec3& axisV,
                  float radius, int segments,
                  const glm::vec3& color)
{
    for (int i = 0; i < segments; i++) {
        float a0 = (float)i / segments * 2.0f * kPi;
        float a1 = (float)(i + 1) / segments * 2.0f * kPi;

        glm::vec3 p0 = center + axisU * (cosf(a0) * radius) + axisV * (sinf(a0) * radius);
        glm::vec3 p1 = center + axisU * (cosf(a1) * radius) + axisV * (sinf(a1) * radius);

        out.push_back({p0, color});
        out.push_back({p1, color});
    }
}

// 生成半圆（从 a0 到 a1）
void AppendArc(std::vector<DebugDraw::Vertex>& out,
               const glm::vec3& center,
               const glm::vec3& axisU, const glm::vec3& axisV,
               float radius, float a0, float a1, int segments,
               const glm::vec3& color)
{
    for (int i = 0; i < segments; i++) {
        float t0 = a0 + (a1 - a0) * (float)i / segments;
        float t1 = a0 + (a1 - a0) * (float)(i + 1) / segments;

        glm::vec3 p0 = center + axisU * (cosf(t0) * radius) + axisV * (sinf(t0) * radius);
        glm::vec3 p1 = center + axisU * (cosf(t1) * radius) + axisV * (sinf(t1) * radius);

        out.push_back({p0, color});
        out.push_back({p1, color});
    }
}
} // anonymous namespace

// ============ 生命周期 ============

void DebugDraw::Initialize(VulkanContext& ctx) {
    VkDeviceSize bufferSize = sizeof(Vertex) * kMaxVertices;

    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = bufferSize;
    bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(ctx.device, &bi, nullptr, &vertexBuffer_);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, vertexBuffer_, &memReqs);

    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = ctx.FindMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(ctx.device, &ai, nullptr, &vertexBufferMemory_);
    vkBindBufferMemory(ctx.device, vertexBuffer_, vertexBufferMemory_, 0);

    vkMapMemory(ctx.device, vertexBufferMemory_, 0, bufferSize, 0, &mapped_);

    vertices_.reserve(kMaxVertices);
}

void DebugDraw::Shutdown(VulkanContext& ctx) {
    if (mapped_) {
        vkUnmapMemory(ctx.device, vertexBufferMemory_);
        mapped_ = nullptr;
    }
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(ctx.device, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(ctx.device, vertexBufferMemory_, nullptr);
        vertexBufferMemory_ = VK_NULL_HANDLE;
    }
}

// ============ 每帧 ============

void DebugDraw::Begin() {
    vertices_.clear();
    vertexCount_ = 0;
}

void DebugDraw::Upload(VulkanContext& ctx) {
    (void)ctx;
    if (vertices_.empty()) {
        vertexCount_ = 0;
        return;
    }

    size_t count = vertices_.size();
    if (count > kMaxVertices) {
        count = kMaxVertices;
    }

    memcpy(mapped_, vertices_.data(), count * sizeof(Vertex));
    vertexCount_ = (uint32_t)count;
}

// ============ 基础线段 ============

void DebugDraw::Line(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color) {
    if (vertices_.size() + 2 > kMaxVertices) return;
    vertices_.push_back({a, color});
    vertices_.push_back({b, color});
}

// ============ 盒体 ============

void DebugDraw::Box(const AABB& box, const glm::vec3& color) {
    if (!box.IsValid()) return;

    glm::vec3 c000(box.min.x, box.min.y, box.min.z);
    glm::vec3 c001(box.min.x, box.min.y, box.max.z);
    glm::vec3 c010(box.min.x, box.max.y, box.min.z);
    glm::vec3 c011(box.min.x, box.max.y, box.max.z);
    glm::vec3 c100(box.max.x, box.min.y, box.min.z);
    glm::vec3 c101(box.max.x, box.min.y, box.max.z);
    glm::vec3 c110(box.max.x, box.max.y, box.min.z);
    glm::vec3 c111(box.max.x, box.max.y, box.max.z);

    Line(c000, c001, color); Line(c001, c011, color); Line(c011, c010, color); Line(c010, c000, color);
    Line(c100, c101, color); Line(c101, c111, color); Line(c111, c110, color); Line(c110, c100, color);
    Line(c000, c100, color); Line(c001, c101, color); Line(c010, c110, color); Line(c011, c111, color);
}

void DebugDraw::Box(const glm::vec3& center, const glm::vec3& halfExtents,
                    const glm::quat& rotation, const glm::vec3& color)
{
    glm::vec3 corners[8] = {
        {-halfExtents.x, -halfExtents.y, -halfExtents.z},
        {-halfExtents.x, -halfExtents.y,  halfExtents.z},
        {-halfExtents.x,  halfExtents.y, -halfExtents.z},
        {-halfExtents.x,  halfExtents.y,  halfExtents.z},
        { halfExtents.x, -halfExtents.y, -halfExtents.z},
        { halfExtents.x, -halfExtents.y,  halfExtents.z},
        { halfExtents.x,  halfExtents.y, -halfExtents.z},
        { halfExtents.x,  halfExtents.y,  halfExtents.z},
    };

    glm::vec3 w[8];
    for (int i = 0; i < 8; i++) {
        w[i] = center + rotation * corners[i];
    }

    Line(w[0], w[1], color); Line(w[1], w[3], color); Line(w[3], w[2], color); Line(w[2], w[0], color);
    Line(w[4], w[5], color); Line(w[5], w[7], color); Line(w[7], w[6], color); Line(w[6], w[4], color);
    Line(w[0], w[4], color); Line(w[1], w[5], color); Line(w[2], w[6], color); Line(w[3], w[7], color);
}

// ============ 球体 ============

void DebugDraw::Sphere(const glm::vec3& center, float radius, const glm::vec3& color) {
    constexpr int kSegments = 24;

    AppendCircle(vertices_, center, glm::vec3(1,0,0), glm::vec3(0,1,0), radius, kSegments, color);
    AppendCircle(vertices_, center, glm::vec3(1,0,0), glm::vec3(0,0,1), radius, kSegments, color);
    AppendCircle(vertices_, center, glm::vec3(0,1,0), glm::vec3(0,0,1), radius, kSegments, color);
}

// ============ 胶囊 ============

void DebugDraw::Capsule(const glm::vec3& p0, const glm::vec3& p1, float radius,
                        const glm::vec3& color)
{
    glm::vec3 axis = p1 - p0;
    float len = glm::length(axis);
    if (len < 1e-6f) {
        Sphere(p0, radius, color);
        return;
    }

    glm::vec3 dir = axis / len;

    glm::vec3 up = (fabsf(dir.y) > 0.99f) ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
    glm::vec3 u = glm::normalize(glm::cross(dir, up));
    glm::vec3 v = glm::cross(dir, u);

    constexpr int kSegments = 16;

    AppendCircle(vertices_, p0, u, v, radius, kSegments, color);
    AppendCircle(vertices_, p1, u, v, radius, kSegments, color);

    Line(p0 + u * radius, p1 + u * radius, color);
    Line(p0 - u * radius, p1 - u * radius, color);
    Line(p0 + v * radius, p1 + v * radius, color);
    Line(p0 - v * radius, p1 - v * radius, color);

    AppendArc(vertices_, p0, u, dir, radius, kPi, 2.0f * kPi, kSegments, color);
    AppendArc(vertices_, p1, u, dir, radius, 0.0f, kPi, kSegments, color);
    AppendArc(vertices_, p0, v, dir, radius, kPi, 2.0f * kPi, kSegments, color);
    AppendArc(vertices_, p1, v, dir, radius, 0.0f, kPi, kSegments, color);
}

// ============ 射线 ============

void DebugDraw::Ray(const glm::vec3& origin, const glm::vec3& direction,
                    float length, const glm::vec3& color)
{
    float len = glm::length(direction);
    if (len < 1e-8f) return;
    glm::vec3 d = direction / len;

    glm::vec3 end = origin + d * length;
    Line(origin, end, color);

    Cross(end, 0.05f, color);
}

// ============ 十字 ============

void DebugDraw::Cross(const glm::vec3& center, float size, const glm::vec3& color) {
    Line(center - glm::vec3(size, 0, 0), center + glm::vec3(size, 0, 0), color);
    Line(center - glm::vec3(0, size, 0), center + glm::vec3(0, size, 0), color);
    Line(center - glm::vec3(0, 0, size), center + glm::vec3(0, 0, size), color);
}