#include "Material.h"

void MaterialLibrary::LoadDefaults() {
    materials.clear();

    Material pearlPink;
    pearlPink.name = "Pearl Pink";
    pearlPink.color = glm::vec4(0.85f, 0.56f, 0.66f, 1.0f);
    pearlPink.metallic = 0.1f;
    pearlPink.roughness = 0.35f;
    materials.push_back(pearlPink);

    Material ultramarine;
    ultramarine.name = "Ultramarine";
    ultramarine.color = glm::vec4(0.36f, 0.36f, 0.84f, 1.0f);
    ultramarine.metallic = 0.0f;
    ultramarine.roughness = 0.5f;
    materials.push_back(ultramarine);

    Material mirror;
    mirror.name = "Mirror";
    mirror.opacity = 0.0f;
    mirror.reflectance = 1.0f;
    mirror.roughness = 0.01f;
    materials.push_back(mirror);

    Material glass;
    glass.name = "Glass";
    glass.opacity = 0.3f;
    glass.reflectance = 0.7f;
    glass.roughness = 0.02f;
    materials.push_back(glass);

    Material matteMetal;
    matteMetal.name = "Brushed Metal";
    matteMetal.color = glm::vec4(0.6f, 0.6f, 0.65f, 1.0f);
    matteMetal.metallic = 0.8f;
    matteMetal.roughness = 0.4f;
    materials.push_back(matteMetal);

    Material emissive;
    emissive.name = "Emissive";
    emissive.emissive = glm::vec3(1.0f);
    emissive.emissive_strength = 2.0f;
    emissive.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    materials.push_back(emissive);

    selectedIndex = 1;
}

Material& MaterialLibrary::GetSelected() {
    return materials[selectedIndex];
}