#pragma once
#include "Player.h"

// ============ WorldState ============
//
// 游戏逻辑的权威数据源。
//
// 单向数据流：
//   Input → GameLogic → WorldState → Renderer
//   单机时 WorldState 是唯一状态
//   多人时 WorldState 是服务器权威状态的本地副本
//
// 设计原则：
//   - 只存"逻辑状态"，不存渲染资源（不存 VkBuffer、Material）
//   - 只被 GameLogic 写，只被 Renderer 读
//   - 网络同步时，NetworkSync 也是读写方
//   - 不做加锁（单机无需；多人时锁应该在 NetworkSync 层）
//   - 返回引用而不是拷贝（避免热路径开销）
//
// 当前内容：
//   - player：玩家实体
//
// 未来会加（等真正需要时再加，不提前设计）：
//   - std::vector<Target> targets;
//   - std::vector<Projectile> projectiles;
//   - AudioClock* audioClock;  // 引用，不持有
//   - LevelState level;        // 当前关卡状态

class WorldState {
public:
    Player player;

    // 未来字段加在这里
};