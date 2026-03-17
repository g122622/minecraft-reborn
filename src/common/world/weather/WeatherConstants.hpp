#pragma once

#include "../../core/Types.hpp"

namespace mc::weather {

/**
 * @brief 天气持续时间常量
 *
 * 参考 MC 1.16.5 ServerWorld 和 WorldInfo
 * 所有时间单位为 ticks (20 ticks = 1秒)
 *
 * 时间分布计算：
 * - 晴天: 平均约 84545 ticks (约 70 分钟)
 * - 降雨: 平均约 18000 ticks (约 15 分钟)
 * - 雷暴: 平均约 9600 ticks (约 8 分钟)
 *
 * 概率：
 * - 晴天: 109/121 ≈ 90.1%
 * - 降雨: 1/11 ≈ 9.1%
 * - 雷暴: 1/121 ≈ 0.8%
 */
namespace WeatherConstants {

/// 晴天最短持续时间 (ticks) - 约 10 分钟
constexpr i32 MIN_CLEAR_TIME = 12000;

/// 晴天最长持续时间 (ticks) - 约 140 分钟
constexpr i32 MAX_CLEAR_TIME = 168000;

/// 降雨最短持续时间 (ticks) - 约 10 分钟
constexpr i32 MIN_RAIN_TIME = 12000;

/// 降雨最长持续时间 (ticks) - 约 20 分钟
constexpr i32 MAX_RAIN_TIME = 24000;

/// 雷暴最短持续时间 (ticks) - 约 3 分钟
constexpr i32 MIN_THUNDER_TIME = 3600;

/// 雷暴最长持续时间 (ticks) - 约 13 分钟
constexpr i32 MAX_THUNDER_TIME = 15600;

/// 默认命令持续时间 (ticks) - 5 分钟
constexpr i32 DEFAULT_COMMAND_DURATION = 6000;

/// 天气强度渐变速率 (每 tick)
/// 从 0.0 到 1.0 需要 100 ticks (5秒)
constexpr f32 STRENGTH_CHANGE_RATE = 0.01f;

/// 闪电生成概率分母 (每 tick)
/// 雷暴时每 tick 有 1/100000 概率生成闪电
constexpr i32 LIGHTNING_CHANCE_DENOMINATOR = 100000;

/// 骷髅马陷阱生成概率
/// 闪电击中时有概率生成骷髅马陷阱
constexpr f32 SKELETON_HORSE_TRAP_CHANCE = 0.01f;

/// 降雨开始时玩家可睡觉的时间范围
constexpr i64 RAIN_BED_START_TIME = 12010;
constexpr i64 RAIN_BED_END_TIME = 23991;

/// 晴天时玩家可睡觉的时间范围
constexpr i64 CLEAR_BED_START_TIME = 12542;
constexpr i64 CLEAR_BED_END_TIME = 23459;

/// 雷暴时天空光照上限
constexpr u8 THUNDER_SKY_LIGHT_LIMIT = 10;

/// 降雨时天空光照上限
constexpr u8 RAIN_SKY_LIGHT_LIMIT = 12;

/// 降雨强度阈值（超过此值认为正在下雨）
constexpr f32 RAIN_THRESHOLD = 0.2f;

/// 雷暴强度阈值（超过此值认为正在雷暴）
constexpr f32 THUNDER_THRESHOLD = 0.9f;

} // namespace WeatherConstants

} // namespace mc::weather
