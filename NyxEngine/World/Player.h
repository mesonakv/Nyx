#pragma once
#include <glm/glm.hpp>

// ============ Player ============
//
// 玩家实体。
//
// 位置语义：
//   position 是玩家"脚底"的世界坐标。
//   眼睛位置 = position + (0, eyeHeight, 0)。
//   跳跃、蹲下、被击退等动作改 position 即可，相机自动跟随。
//
// 职责（当前）：
//   - 持有世界位置
//   - 提供眼睛位置（相机用）
//
// 职责（未来）：
//   - 持有 velocity、state（Idle/Run/Jump/Dash/Slide/WallRun...）
//   - 被动作系统驱动
//
// 为什么在 NyxEngine/World/ 下：
//   - WorldState 持有 Player，WorldState 属于引擎
//   - 如果 Player 放在 Game/，WorldState 就要 include Game/ 头文件，方向反了

class Player {
public:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 8.0f);
    float eyeHeight = 1.7f;

    glm::vec3 GetEyePosition() const {
        return position + glm::vec3(0.0f, eyeHeight, 0.0f);
    }

    void Update(float dt);
};