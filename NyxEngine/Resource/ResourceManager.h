#pragma once
#include "Mesh.h"
#include <memory>
#include <string>
#include <unordered_map>

// ============ ResourceManager ============
//
// 资源缓存 + 引用计数。
//
// 当前只支持 Mesh。其他资源（Texture、Shader、Audio）以后按需加。
//
// 用法：
//   ResourceManager res;
//   auto mesh = res.LoadMesh("assets/cube.obj");
//   if (mesh) {
//       const MeshData& data = mesh->GetData();
//   }
//
// 缓存策略：
//   - 按路径缓存。相同路径只加载一次
//   - 返回 shared_ptr。所有引用释放后，资源自动销毁
//   - 缓存本身持有 weak_ptr，不会阻止资源释放
//
// 线程安全：
//   - 当前实现不线程安全。加载在主线程。
//   - 未来如果需要异步加载，在内部加锁

class ResourceManager {
public:
    // 加载 Mesh（带缓存）
    // 失败返回 nullptr
    std::shared_ptr<Mesh> LoadMesh(const std::string& path);

    // 清空缓存（weak_ptr 表）
    void ClearCache();

    // 当前缓存的资源数（包括已释放的槽位）
    size_t GetCacheSize() const { return meshCache_.size(); }

    // 从内存字符串加载（便于测试，不进缓存）
    static std::shared_ptr<Mesh> LoadMeshFromString(const std::string& source);

private:
    std::unordered_map<std::string, std::weak_ptr<Mesh>> meshCache_;
};