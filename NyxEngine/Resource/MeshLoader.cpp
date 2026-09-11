#include "MeshLoader.h"
#include "NyxEngine/Core/FileSystem.h"
#include "NyxEngine/Core/Logger.h"
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstdlib>

namespace {

struct ObjVertexRef {
    int v = 0;   // 1-based
    int vn = 0;  // 1-based，0 表示没有
};

// 解析 "v x y z [r g b]"
bool ParsePositionLine(const std::string& line, glm::vec3& pos, glm::vec3& color, bool& hasColor) {
    std::istringstream ss(line);
    std::string keyword;
    ss >> keyword;
    if (!(ss >> pos.x >> pos.y >> pos.z)) return false;
    if (ss >> color.r >> color.g >> color.b) {
        hasColor = true;
    } else {
        hasColor = false;
        color = glm::vec3(1.0f);
    }
    return true;
}

// 解析 "vn x y z"
bool ParseNormalLine(const std::string& line, glm::vec3& normal) {
    std::istringstream ss(line);
    std::string keyword;
    ss >> keyword;
    return static_cast<bool>(ss >> normal.x >> normal.y >> normal.z);
}

// 解析 "f v1 v2 v3 ..."
// 支持格式：v、v/vt、v//vn、v/vt/vn
bool ParseFaceLine(const std::string& line, std::vector<ObjVertexRef>& out) {
    std::istringstream ss(line);
    std::string keyword;
    ss >> keyword;

    out.clear();
    std::string token;
    while (ss >> token) {
        ObjVertexRef ref;

        // 找第一个 '/'
        size_t slash1 = token.find('/');
        if (slash1 == std::string::npos) {
            ref.v = std::atoi(token.c_str());
        } else {
            ref.v = std::atoi(token.substr(0, slash1).c_str());

            size_t slash2 = token.find('/', slash1 + 1);
            if (slash2 != std::string::npos && slash2 + 1 < token.size()) {
                // v/vt/vn 或 v//vn
                ref.vn = std::atoi(token.substr(slash2 + 1).c_str());
            }
            // v/vt 忽略 vt
        }

        out.push_back(ref);
    }

    return out.size() >= 3;
}

// 把 1-based 索引转成 0-based，负数表示从末尾数
int ResolveIndex(int idx, size_t total) {
    if (idx > 0) return idx - 1;
    if (idx < 0) return (int)total + idx;
    return -1;
}

} // anonymous namespace

bool MeshLoader::LoadObjFromString(const std::string& source, MeshData& out, std::string* outError) {
    out.Clear();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<bool> hasColorPerVertex;
    std::vector<glm::vec3> normals;

    // 每个唯一的 (v, vn) 组合对应一个 MeshVertex
    std::unordered_map<uint64_t, uint32_t> vertexCache;

    std::istringstream stream(source);
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        lineNumber++;

        // 跳过空行和注释
        if (line.empty() || line[0] == '#') continue;

        // 去掉行尾 \r（Windows 文件）
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // 跳过前导空格
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.compare(0, 2, "v ") == 0) {
            glm::vec3 pos, color;
            bool hasColor = false;
            if (!ParsePositionLine(line, pos, color, hasColor)) {
                if (outError) *outError = "Failed to parse vertex at line " + std::to_string(lineNumber);
                return false;
            }
            positions.push_back(pos);
            colors.push_back(color);
            hasColorPerVertex.push_back(hasColor);
        }
        else if (line.compare(0, 3, "vn ") == 0) {
            glm::vec3 n;
            if (!ParseNormalLine(line, n)) {
                if (outError) *outError = "Failed to parse normal at line " + std::to_string(lineNumber);
                return false;
            }
            normals.push_back(n);
        }
        else if (line.compare(0, 2, "f ") == 0) {
            std::vector<ObjVertexRef> refs;
            if (!ParseFaceLine(line, refs)) {
                if (outError) *outError = "Failed to parse face at line " + std::to_string(lineNumber);
                return false;
            }

            // 扇形三角化
            for (size_t i = 1; i + 1 < refs.size(); i++) {
                ObjVertexRef tri[3] = { refs[0], refs[i], refs[i + 1] };

                for (int k = 0; k < 3; k++) {
                    int vi = ResolveIndex(tri[k].v, positions.size());
                    int ni = (tri[k].vn != 0) ? ResolveIndex(tri[k].vn, normals.size()) : -1;

                    if (vi < 0 || vi >= (int)positions.size()) {
                        if (outError) *outError = "Invalid vertex index at line " + std::to_string(lineNumber);
                        return false;
                    }
                    if (ni >= (int)normals.size()) {
                        if (outError) *outError = "Invalid normal index at line " + std::to_string(lineNumber);
                        return false;
                    }

                    // 缓存键：(vi, ni)
                    uint64_t key = ((uint64_t)(uint32_t)vi << 32) | (uint32_t)(ni + 1);
                    auto it = vertexCache.find(key);

                    if (it == vertexCache.end()) {
                        MeshVertex mv;
                        mv.position = positions[vi];
                        mv.color = hasColorPerVertex[vi] ? colors[vi] : glm::vec3(1.0f);
                        if (ni >= 0) {
                            mv.normal = normals[ni];
                        } else {
                            mv.normal = glm::vec3(0.0f, 1.0f, 0.0f);   // 默认向上
                        }

                        uint32_t newIndex = (uint32_t)out.vertices.size();
                        out.vertices.push_back(mv);
                        vertexCache[key] = newIndex;
                        out.indices.push_back(newIndex);
                    } else {
                        out.indices.push_back(it->second);
                    }
                }
            }
        }
        // 忽略 vt、o、g、s、mtllib、usemtl 等
    }

    // 如果没有法线，计算面法线
    if (normals.empty() && !out.vertices.empty()) {
        // 简单做法：每个顶点用位置归一到单位向量（仅对球体等有效）
        // 更好的做法是求面法线，但这里先不做
        // 使用者可以在 shader 里忽略 normal
    }

    if (out.vertices.empty()) {
        if (outError) *outError = "No vertices in obj data";
        return false;
    }

    return true;
}

bool MeshLoader::LoadObj(const std::string& path, MeshData& out, std::string* outError) {
    std::string source = FileSystem::ReadText(path);
    if (source.empty()) {
        if (outError) *outError = "Failed to read file: " + path;
        return false;
    }
    return LoadObjFromString(source, out, outError);
}