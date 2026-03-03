#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace mr {

// ============================================================================
// 基础类型定义
// ============================================================================

// 整数类型
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// 浮点类型
using f32 = float;
using f64 = double;

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

// 字符类型
using char8 = char;
using char16 = char16_t;
using char32 = char32_t;

// 字符串类型
using String = std::string;
using StringView = std::string_view;

// 尺寸类型
using Size = std::size_t;
using PtrDiff = std::ptrdiff_t;

// ============================================================================
// 游戏特定类型
// ============================================================================

// 区块坐标类型
using ChunkCoord = i32;

// 方块坐标类型（区块内）
using BlockCoord = i32;

// 世界高度
using WorldHeight = i32;

// 实体ID类型
using EntityId = u64;

// 物品ID类型
using ItemId = u16;

// 生物群系ID类型
using BiomeId = u16;

// 维度ID类型
using DimensionId = i32;

// 玩家ID类型
using PlayerId = u64;

// ============================================================================
// 游戏常量
// ============================================================================

namespace constants {

// 区块尺寸
inline constexpr i32 CHUNK_WIDTH = 16;
inline constexpr i32 CHUNK_HEIGHT = 256;
inline constexpr i32 CHUNK_SECTION_HEIGHT = 16;
inline constexpr i32 CHUNK_SECTIONS = CHUNK_HEIGHT / CHUNK_SECTION_HEIGHT;

// 世界边界
inline constexpr i32 MIN_BUILD_HEIGHT = 0;
inline constexpr i32 MAX_BUILD_HEIGHT = CHUNK_HEIGHT;

// 方块状态
inline constexpr u16 MAX_BLOCK_STATES = 16;

// 实体限制
inline constexpr Size MAX_ENTITIES_PER_CHUNK = 1024;
inline constexpr Size MAX_PLAYERS = 256;

// 网络
inline constexpr u16 DEFAULT_PORT = 19132;
inline constexpr Size MAX_PACKET_SIZE = 2 * 1024 * 1024; // 2MB
inline constexpr Size PACKET_BUFFER_SIZE = 64 * 1024;    // 64KB

// 游戏刻
inline constexpr f32 TICK_RATE = 20.0f;          // 每秒20刻
inline constexpr f32 TICK_DURATION = 1.0f / 20.0f; // 每刻50ms

} // namespace constants

// ============================================================================
// 枚举类型
// ============================================================================

/**
 * @brief 维度类型
 */
enum class Dimension : DimensionId {
    Overworld = 0,
    Nether = 1,
    TheEnd = 2
};

/**
 * @brief 游戏模式
 */
enum class GameMode : u8 {
    Survival = 0,
    Creative = 1,
    Adventure = 2,
    Spectator = 3
};

/**
 * @brief 难度等级
 */
enum class Difficulty : u8 {
    Peaceful = 0,
    Easy = 1,
    Normal = 2,
    Hard = 3
};

/**
 * @brief 方块朝向
 */
enum class BlockFace : u8 {
    Bottom = 0, // Y-
    Top = 1,    // Y+
    North = 2,  // Z-
    South = 3,  // Z+
    West = 4,   // X-
    East = 5    // X+
};

/**
 * @brief 方块形状（用于碰撞）
 */
enum class BlockShape : u8 {
    Empty = 0,
    Full = 1,
    Partial = 2,
    Custom = 3
};

} // namespace mr
