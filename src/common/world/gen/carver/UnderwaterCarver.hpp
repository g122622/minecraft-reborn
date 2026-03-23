#pragma once

#include "WorldCarver.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc::world::gen::carver {

/**
 * @brief 水下洞穴雕刻器配置
 */
struct UnderwaterCaveConfig {
    f32 probability;  ///< 生成概率
    i32 minLength;    ///< 最小长度
    i32 maxLength;    ///< 最大长度
    f32 horizontalScale;  ///< 水平缩放
    f32 verticalScale;    ///< 垂直缩放

    UnderwaterCaveConfig()
        : probability(0.02f)
        , minLength(8)
        , maxLength(16)
        , horizontalScale(1.0f)
        , verticalScale(1.0f) {}
};

/**
 * @brief 水下洞穴雕刻器
 *
 * 生成填充水的洞穴，而非空气。
 * 参考 MC 1.16.5: UnderwaterCaveWorldCarver
 */
class UnderwaterCaveCarver : public WorldCarver<UnderwaterCaveConfig> {
public:
    /**
     * @brief 构造函数
     */
    UnderwaterCaveCarver();

    /**
     * @brief 在区块中执行雕刻
     */
    bool carve(ChunkPrimer& chunk,
               const BiomeProvider& biomeProvider,
               i32 seaLevel,
               ChunkCoord chunkX,
               ChunkCoord chunkZ,
               CarvingMask& carvingMask,
               const UnderwaterCaveConfig& config) override;

    /**
     * @brief 检查是否应该在这个区块执行雕刻
     */
    [[nodiscard]] bool shouldCarve(
        math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const UnderwaterCaveConfig& config) const override;

protected:
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;
};

/**
 * @brief 水下峡谷雕刻器配置
 */
struct UnderwaterCanyonConfig {
    f32 probability;      ///< 生成概率
    i32 minLength;        ///< 最小长度
    i32 maxLength;        ///< 最大长度
    f32 horizontalScale;  ///< 水平缩放
    f32 verticalScale;    ///< 垂直缩放
    f32 thickness;        ///< 厚度

    UnderwaterCanyonConfig()
        : probability(0.02f)
        , minLength(20)
        , maxLength(64)
        , horizontalScale(1.0f)
        , verticalScale(1.0f)
        , thickness(2.0f) {}
};

/**
 * @brief 水下峡谷雕刻器
 *
 * 生成填充水的峡谷，而非空气。
 * 参考 MC 1.16.5: UnderwaterCanyonWorldCarver
 */
class UnderwaterCanyonCarver : public WorldCarver<UnderwaterCanyonConfig> {
public:
    /**
     * @brief 构造函数
     */
    UnderwaterCanyonCarver();

    /**
     * @brief 在区块中执行雕刻
     */
    bool carve(ChunkPrimer& chunk,
               const BiomeProvider& biomeProvider,
               i32 seaLevel,
               ChunkCoord chunkX,
               ChunkCoord chunkZ,
               CarvingMask& carvingMask,
               const UnderwaterCanyonConfig& config) override;

    /**
     * @brief 检查是否应该在这个区块执行雕刻
     */
    [[nodiscard]] bool shouldCarve(
        math::IRandom& rng,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        const UnderwaterCanyonConfig& config) const override;

protected:
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;
};

/**
 * @brief 创建水下洞穴雕刻器
 */
std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver();

/**
 * @brief 创建水下峡谷雕刻器
 */
std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver();

} // namespace mc::world::gen::carver
