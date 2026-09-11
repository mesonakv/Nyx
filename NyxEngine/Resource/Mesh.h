#pragma once
#include "MeshData.h"
#include <string>

// ============ Mesh ============
//
// 一个网格资源。
//
// 目前只持有 CPU 数据。GPU 上传由 Renderer 按需完成。
// 生命周期由 ResourceManager 通过 shared_ptr 管理。

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(MeshData data) : data_(std::move(data)) {}

    const MeshData& GetData() const { return data_; }
    MeshData& GetData() { return data_; }

    bool IsValid() const { return !data_.IsEmpty(); }

private:
    MeshData data_;
};