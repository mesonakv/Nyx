#pragma once
#include "MeshData.h"
#include <string>

// ============ MeshLoader ============
//
// 从 .obj 文件加载网格。
//
// 支持的 obj 子集：
//   v x y z [r g b]     顶点位置（可选 RGB 颜色）
//   vn x y z            顶点法线
//   f v1 v2 v3 ...      面（支持 v、v//vn、v/vt/vn）
//
// 不支持：
//   vt（纹理坐标）
//   mtl（材质库）
//   o、g、s（对象、组、平滑组）
//   四边形及以上的面会自动三角化（扇形）

class MeshLoader {
public:
    // 从文件加载
    static bool LoadObj(const std::string& path, MeshData& out, std::string* outError = nullptr);

    // 从内存字符串加载（便于测试）
    static bool LoadObjFromString(const std::string& source, MeshData& out, std::string* outError = nullptr);
};