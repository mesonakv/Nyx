#pragma once
class MaterialLibrary;
class TargetManager;

class MaterialPanel {
public:
    void Initialize(MaterialLibrary& materials, TargetManager& targets) {
        materials_ = &materials;
        targets_ = &targets;
    }
    void Draw();
private:
    MaterialLibrary* materials_ = nullptr;
    TargetManager* targets_ = nullptr;
};