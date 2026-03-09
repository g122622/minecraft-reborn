#pragma once

#include "../core/Types.hpp"
#include "../core/Constants.hpp"

#include <cmath>
#include <algorithm>
#include <limits>

namespace mr::math {

// ============================================================================
// 基本数学函数
// ============================================================================

/**
 * @brief 度转弧度
 */
[[nodiscard]] inline constexpr f32 toRadians(f32 degrees) noexcept
{
    return degrees * DEG_TO_RAD;
}

/**
 * @brief 弧度转度
 */
[[nodiscard]] inline constexpr f32 toDegrees(f32 radians) noexcept
{
    return radians * RAD_TO_DEG;
}

/**
 * @brief 数值限制在范围内
 */
template<typename T>
[[nodiscard]] inline constexpr T clamp(T value, T minVal, T maxVal) noexcept
{
    return std::clamp(value, minVal, maxVal);
}

/**
 * @brief 线性插值
 */
[[nodiscard]] inline constexpr f32 lerp(f32 a, f32 b, f32 t) noexcept
{
    return a + (b - a) * t;
}

/**
 * @brief 平滑插值 (smoothstep)
 */
[[nodiscard]] inline constexpr f32 smoothstep(f32 edge0, f32 edge1, f32 x) noexcept
{
    const f32 t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief 平方
 */
template<typename T>
[[nodiscard]] inline constexpr T square(T x) noexcept
{
    return x * x;
}

/**
 * @brief 立方
 */
template<typename T>
[[nodiscard]] inline constexpr T cube(T x) noexcept
{
    return x * x * x;
}

/**
 * @brief 判断是否接近零
 */
[[nodiscard]] inline bool isZero(f32 x, f32 epsilon = EPSILON) noexcept
{
    return std::abs(x) < epsilon;
}

/**
 * @brief 判断两个浮点数是否近似相等
 */
[[nodiscard]] inline bool approxEqual(f32 a, f32 b, f32 epsilon = EPSILON) noexcept
{
    return std::abs(a - b) < epsilon;
}

/**
 * @brief 向上取整到整数
 */
template<typename T>
[[nodiscard]] inline T ceilTo(f32 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向下取整到整数
 */
template<typename T>
[[nodiscard]] inline T floorTo(f32 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 四舍五入到整数
 */
template<typename T>
[[nodiscard]] inline T roundTo(f32 x) noexcept
{
    return static_cast<T>(std::round(x));
}

// ============================================================================
// 区块相关计算
// ============================================================================

/**
 * @brief 世界坐标转区块坐标
 */
[[nodiscard]] inline ChunkCoord toChunkCoord(f32 worldCoord) noexcept
{
    return static_cast<ChunkCoord>(std::floor(worldCoord / static_cast<f32>(world::CHUNK_WIDTH)));
}

/**
 * @brief 世界坐标转区块坐标（整数版本）
 */
[[nodiscard]] inline constexpr ChunkCoord toChunkCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord >= 0
               ? worldCoord / world::CHUNK_WIDTH
               : (worldCoord - world::CHUNK_WIDTH + 1) / world::CHUNK_WIDTH;
}

/**
 * @brief 区块坐标转世界坐标（区块原点）
 */
[[nodiscard]] inline constexpr BlockCoord toWorldCoord(ChunkCoord chunkCoord) noexcept
{
    return chunkCoord * world::CHUNK_WIDTH;
}

/**
 * @brief 世界坐标转区块内坐标
 */
[[nodiscard]] inline BlockCoord toLocalCoord(f32 worldCoord) noexcept
{
    return static_cast<BlockCoord>(std::floor(worldCoord)) & world::CHUNK_MASK;
}

/**
 * @brief 世界坐标转区块内坐标（整数版本）
 */
[[nodiscard]] inline constexpr BlockCoord toLocalCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord & world::CHUNK_MASK;
}

/**
 * @brief 区块坐标转换为64位唯一ID
 */
[[nodiscard]] inline constexpr u64 chunkPosToId(ChunkCoord x, ChunkCoord z) noexcept
{
    const u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFFLL);
    const u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFFLL);
    return (ux << 32) | uz;
}

/**
 * @brief 从64位ID解包区块坐标
 */
inline void idToChunkPos(u64 id, ChunkCoord& x, ChunkCoord& z) noexcept
{
    x = static_cast<ChunkCoord>(static_cast<i32>((id >> 32) & 0xFFFFFFFF));
    z = static_cast<ChunkCoord>(static_cast<i32>(id & 0xFFFFFFFF));
}

} // namespace mr::math
