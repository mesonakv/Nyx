#pragma once
#include "Player.h"
#include "../Physics/PhysicsWorld.h"

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
//   - 只存"逻辑状态"，不存渲染资源
//   - 只被 GameLogic 写，只被 Renderer 读
//   - 不做加锁（单机无需；多人时锁在 NetworkSync 层）

class WorldState {
public:
    Player player;
    PhysicsWorld physics;
};