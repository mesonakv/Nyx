#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

// ============ MeshData ============
//
// CPU 端的网格数据。和 GPU 无关。
//
// 顶点布局：
//   position (12) + normal (12) + color (12) = 36 字节
//   和 Renderer 现有的 vertex buffer 布局一致。

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;

    void Clear() {
        vertices.clear();
        indices.clear();
    }

    bool IsEmpty() const {
        return vertices.empty();
    }

    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }
    size_t GetTriangleCount() const { return indices.size() / 3; }
};