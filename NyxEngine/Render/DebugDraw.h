#pragma once
#include "../Core/VulkanContext.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <cstdint>

struct AABB;

// ============ DebugDraw ============
//
// 3D 线段调试渲染器。
//
// 用途：
//   - 物理形状可视化（胶囊、球、盒）
//   - 射线可视化
//   - 接触点可视化
//   - 弹幕轨迹、AI 感知范围、Boss 攻击范围
//
// 设计：
//   - 每帧 Begin() 清空，若干次 Line/Box/Sphere/Capsule 调用，然后 Renderer 画
//   - 使用动态顶点缓冲（host visible + coherent），每帧 memcpy
//   - 线段用 VK_PRIMITIVE_TOPOLOGY_LINE_LIST
//   - 每个顶点 24 字节（position + color）

class DebugDraw {
public:
    // 顶点容量上限。超过后多余线条被丢弃。
    static constexpr size_t kMaxVertices = 65536;

    // 顶点格式。public 是为了让 DebugDraw.cpp 里的匿名工具函数能引用它。
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    void Initialize(VulkanContext& ctx);
    void Shutdown(VulkanContext& ctx);

    // 每帧调用
    void Begin();

    // ---------- 绘图方法 ----------
    void Line(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color);

    // 世界空间 AABB
    void Box(const AABB& box, const glm::vec3& color);

    // 带旋转的盒体（center + halfExtents + rotation）
    void Box(const glm::vec3& center, const glm::vec3& halfExtents,
             const glm::quat& rotation, const glm::vec3& color);

    // 球体：画 3 个大圆
    void Sphere(const glm::vec3& center, float radius, const glm::vec3& color);

    // 胶囊：两个端点 + 半径
    void Capsule(const glm::vec3& p0, const glm::vec3& p1, float radius,
                 const glm::vec3& color);

    // 射线：起点 + 方向 + 长度
    void Ray(const glm::vec3& origin, const glm::vec3& direction, float length,
             const glm::vec3& color);

    // 十字准星（世界空间的小十字）
    void Cross(const glm::vec3& center, float size, const glm::vec3& color);

    // ---------- 内部 ----------

    // 上传 CPU 数据到 GPU。Renderer 在 RecordCommandBuffer 前调用。
    void Upload(VulkanContext& ctx);

    // 供 Renderer 使用
    VkBuffer GetVertexBuffer() const { return vertexBuffer_; }
    uint32_t GetVertexCount() const { return vertexCount_; }
    bool IsEmpty() const { return vertexCount_ == 0; }

    // 供 DebugDraw.cpp 里的匿名工具函数使用
    std::vector<Vertex>& GetVertices() { return vertices_; }

private:
    std::vector<Vertex> vertices_;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    void* mapped_ = nullptr;
    uint32_t vertexCount_ = 0;
};