#pragma once

#include "../core/Types.hpp"
#include "../core/Constants.hpp"

#include <cmath>
#include <algorithm>
#include <limits>
#include <random>

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

// ============================================================================
// 随机数
// ============================================================================

/**
 * @brief 随机数生成器
 */
class Random {
public:
    explicit Random(u64 seed = 0)
        : m_engine(seed)
    {
    }

    void setSeed(u64 seed)
    {
        m_engine.seed(seed);
    }

    /// 返回 [0, UINT64_MAX] 范围的随机数
    [[nodiscard]] u64 nextU64()
    {
        return m_engine();
    }

    /// 返回 [0, max) 范围的随机整数
    [[nodiscard]] u32 nextU32(u32 max)
    {
        return static_cast<u32>(nextU64() % max);
    }

    /// 返回 [min, max] 范围的随机整数
    [[nodiscard]] i32 nextInt(i32 min, i32 max)
    {
        return min + static_cast<i32>(nextU64() % static_cast<u64>(max - min + 1));
    }

    /// 返回 [0.0, 1.0) 范围的随机浮点数
    [[nodiscard]] f32 nextFloat()
    {
        return static_cast<f32>(m_engine()) / static_cast<f32>(std::numeric_limits<u64>::max());
    }

    /// 返回 [min, max) 范围的随机浮点数
    [[nodiscard]] f32 nextFloat(f32 min, f32 max)
    {
        return min + nextFloat() * (max - min);
    }

    /// 返回 [0.0, 1.0) 范围的随机双精度浮点数
    [[nodiscard]] f64 nextDouble()
    {
        return static_cast<f64>(m_engine()) / static_cast<f64>(std::numeric_limits<u64>::max());
    }

    /// 高斯分布随机数
    [[nodiscard]] f32 nextGaussian(f32 mean = 0.0f, f32 stddev = 1.0f)
    {
        std::normal_distribution<f32> dist(mean, stddev);
        return dist(m_engine);
    }

private:
    std::mt19937_64 m_engine;
};

// ============================================================================
// 噪声函数（用于地形生成）
// ============================================================================

/**
 * @brief 简单的Perlin噪声实现
 */
class PerlinNoise {
public:
    explicit PerlinNoise(u64 seed = 0);

    [[nodiscard]] f32 noise(f32 x) const;
    [[nodiscard]] f32 noise(f32 x, f32 y) const;
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const;

    /// 分形噪声 (FBM)
    [[nodiscard]] f32 octaveNoise(f32 x, f32 y, i32 octaves, f32 persistence = 0.5f) const;

private:
    static constexpr i32 PERMUTATION_SIZE = 256;
    i32 m_permutation[512]; // NOLINT

    [[nodiscard]] static f32 fade(f32 t);
    [[nodiscard]] static f32 grad(i32 hash, f32 x, f32 y, f32 z);
};

} // namespace mr::math
