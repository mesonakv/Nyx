#include "ResourceManager.h"
#include "MeshLoader.h"
#include "NyxEngine/Core/Logger.h"

std::shared_ptr<Mesh> ResourceManager::LoadMesh(const std::string& path) {
    // 先查缓存
    auto it = meshCache_.find(path);
    if (it != meshCache_.end()) {
        if (auto cached = it->second.lock()) {
            NYX_LOG_INFO("ResourceManager: cache hit for '%s'", path.c_str());
            return cached;
        }
        // weak_ptr 已过期，稍后覆盖
    }

    // 加载
    MeshData data;
    std::string err;
    if (!MeshLoader::LoadObj(path, data, &err)) {
        NYX_LOG_ERROR("ResourceManager: failed to load mesh '%s': %s",
                      path.c_str(), err.c_str());
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>(std::move(data));
    meshCache_[path] = mesh;

    NYX_LOG_INFO("ResourceManager: loaded '%s' (%zu vertices, %zu indices, %zu triangles)",
                 path.c_str(),
                 mesh->GetData().GetVertexCount(),
                 mesh->GetData().GetIndexCount(),
                 mesh->GetData().GetTriangleCount());

    return mesh;
}

std::shared_ptr<Mesh> ResourceManager::LoadMeshFromString(const std::string& source) {
    MeshData data;
    std::string err;
    if (!MeshLoader::LoadObjFromString(source, data, &err)) {
        NYX_LOG_ERROR("ResourceManager: failed to load mesh from string: %s", err.c_str());
        return nullptr;
    }
    return std::make_shared<Mesh>(std::move(data));
}

void ResourceManager::ClearCache() {
    meshCache_.clear();
}