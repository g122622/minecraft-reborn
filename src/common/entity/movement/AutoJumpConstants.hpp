#pragma once

#include "../../core/Types.hpp"

namespace mc {
namespace entity {
namespace movement {

/**
 * @brief 自动跳跃系统常量
 *
 * 所有常量基于 MC 1.16.5 ClientPlayerEntity 实现。
 * 参考: net.minecraft.client.entity.player.ClientPlayerEntity
 */
namespace AutoJumpConstants {

/// 基础最大跳跃高度（方块数）
/// 玩家可以自动跳上的最大高度，不含跳跃药水效果
/// MC 源码: f7 = 1.2F
constexpr f32 BASE_JUMP_HEIGHT = 1.2f;

/// 跳跃药水效果每级增加的跳跃高度
/// MC 源码: f7 += (float)(this.getActivePotionEffect(Effects.JUMP_BOOST).getAmplifier() + 1) * 0.75F
constexpr f32 JUMP_BOOST_PER_LEVEL = 0.75f;

/// 最小跳跃高度阈值
/// 高度差小于此值时不触发自动跳跃（如台阶）
/// MC 逻辑: if (f14 <= 0.5F) return;
constexpr f32 MIN_JUMP_HEIGHT = 0.5f;

/// 前进方向检测阈值
/// 移动方向与朝向的点积小于此值时不触发（向后移动时不触发）
/// MC 源码: if (f13 < -0.15F) return;
constexpr f32 FORWARD_THRESHOLD = -0.15f;

/// 检测距离乘数
/// 检测距离 = 移动速度 * 此乘数
/// MC 源码: f8 = Math.max(f * 7.0F, 1.0F / f12)
constexpr f32 DETECTION_DISTANCE_MULTIPLIER = 7.0f;

/// 检测高度偏移
/// 检测线在玩家脚部上方此偏移量处
/// MC 使用 player.getPosY() + 0.51D 作为检测线起点
constexpr f32 DETECTION_HEIGHT_OFFSET = 0.51f;

/// 自动跳跃冷却时间（ticks）
/// 每次自动跳跃后需要等待的 tick 数
/// MC 源码: this.autoJumpTime = 1;
constexpr i32 AUTO_JUMP_COOLDOWN = 1;

/// 移动输入阈值平方
/// 用于判断是否有有效移动输入
/// MC 源码: f1 <= 0.001F
constexpr f32 MOVEMENT_THRESHOLD_SQ = 0.001f;

/// 跳跃因子阈值
/// 跳跃因子低于此值时不触发自动跳跃（如蜂蜜块上）
/// MC 源码: (double)this.getJumpFactor() >= 1.0D
constexpr f64 JUMP_FACTOR_THRESHOLD = 1.0;

/// 检测线偏移比例
/// 检测线相对于玩家宽度的偏移比例
/// MC 源码: vector3d6 = vector3d5.scale((double)(f9 * 0.5F))
/// 其中 f9 是玩家宽度
constexpr f32 LINE_OFFSET_RATIO = 0.5f;

/// 头部空间检测高度
/// 检查玩家上方多少格是否有障碍物
/// MC 检查玩家眼睛上方一格和两格位置
constexpr i32 HEAD_SPACE_CHECK_HEIGHT = 2;

} // namespace AutoJumpConstants

} // namespace movement
} // namespace entity
} // namespace mc
