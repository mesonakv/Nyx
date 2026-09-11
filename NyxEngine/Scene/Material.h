#pragma once
#include <glm/glm.hpp>
#include <vector>

// D2: name 改成 const char*，避免拷贝 Material 时拷贝字符串。
// name 指向的字符串生命周期必须 >= Material。
// 目前所有 name 都是字符串字面量，生命周期是程序全程。

struct Material {
    const char* name = "default";
    glm::vec4 color = glm::vec4(0.36f, 0.36f, 0.84f, 1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float emissive_strength = 0.0f;
    float opacity = 1.0f;
    float reflectance = 0.0f;
};

class MaterialLibrary {
public:
    std::vector<Material> materials;
    int selectedIndex = 0;

    void LoadDefaults();
    Material& GetSelected();
};