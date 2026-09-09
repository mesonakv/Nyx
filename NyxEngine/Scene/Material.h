#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Material {
    std::string name = "default";
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