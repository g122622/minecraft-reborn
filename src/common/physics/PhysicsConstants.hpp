#pragma once

#include "../core/Types.hpp"

namespace mc {
namespace physics {

// ============================================================================
// 重力和阻力
// ============================================================================

/// 标准 MC 重力加速度 (blocks/tick²)
constexpr f32 GRAVITY = 0.02f;

/// 空气阻力系数 (每 tick)
constexpr f32 DRAG_AIR = 0.98f;

/// 地面摩擦系数
constexpr f32 DRAG_GROUND = 0.91f;

/// 水中阻力
constexpr f32 DRAG_WATER = 0.8f;

/// 岩浆阻力
constexpr f32 DRAG_LAVA = 0.5f;

// ============================================================================
// 运动参数
// ============================================================================

/// 标准跳跃初速度
constexpr f32 JUMP_VELOCITY = 0.42f;

/// 玩家最大步进高度
constexpr f32 STEP_HEIGHT = 0.6f;

/// 速度归零阈值
constexpr f32 MOTION_THRESHOLD = 0.003f;

/// 默认滑度系数
constexpr f32 SLIPPERINESS_DEFAULT = 0.6f;

/// 冰滑度系数
constexpr f32 SLIPPERINESS_ICE = 0.98f;

/// 滑度冰滑度系数
constexpr f32 SLIPPERINESS_SLIME = 0.8f;

/// 地面移动因子计算
/// MC 公式: speed * (0.21600002F / (slipperiness^3))
constexpr f32 getGroundMoveFactor(f32 speed, f32 slipperiness = SLIPPERINESS_DEFAULT) {
    return speed * 0.21600002f / (slipperiness * slipperiness * slipperiness);
}

// ============================================================================
// 物品物理
// ============================================================================

/// 物品重力
constexpr f32 ITEM_GRAVITY = 0.04f;

/// 物品阻力
constexpr f32 ITEM_DRAG = 0.98f;

/// 物品水平阻力
constexpr f32 ITEM_HORIZONTAL_DRAG = 0.98f;

// ============================================================================
// 粒子物理
// ============================================================================

/// 雨滴重力
constexpr f32 RAIN_GRAVITY = 0.06f;

/// 雪花重力
constexpr f32 SNOW_GRAVITY = 0.02f;

// ============================================================================
// 实体运动限制
// ============================================================================

/// 最大移动速度 (blocks/tick)
constexpr f32 MAX_MOVEMENT_SPEED = 100.0f;

/// 最大下落速度
constexpr f32 MAX_FALL_SPEED = 3.0f;

// ============================================================================
// 游泳和潜水
// ============================================================================

/// 水中跳跃初速度
constexpr f32 SWIM_JUMP_VELOCITY = 0.3f;

/// 水中重力（浮力抵消）
constexpr f32 WATER_GRAVITY = 0.02f;

// ============================================================================
// 爆炸
// ============================================================================

/// 爆炸基础伤害衰减距离
constexpr f32 EXPLOSION_RADIUS_SCALE = 2.0f;

} // namespace physics
} // namespace mc
