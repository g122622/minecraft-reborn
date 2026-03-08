#pragma once

#include "../../../core/Types.hpp"
#include "../../block/Block.hpp"
#include <memory>
#include <random>

namespace mr {

// 前向声明
class ChunkPrimer;
class BiomeProvider;

/**
 * @brief 世界雕刻器配置
 *
 * 参考 MC 1.16.5 的雕刻器配置。
 */
struct CarverConfig {
    /// 雕刻概率 (0.0 - 1.0)
    f32 probability = 0.14285715f;  // 1/7，MC 默认洞穴生成概率

    /// 最大高度
    i32 maxHeight = 256;

    /// 生成尝试次数范围
    i32 minGenerationAttempts = 15;

    /// 雕刻器范围（以区块为单位）
    i32 range = 4;
};

/**
 * @brief 洞穴世界雕刻器
 *
 * 参考 MC CaveWorldCarver，生成洞穴系统。
 *
 * 使用方法：
 * @code
 * CaveCarver carver(seed, config);
 * carver.carve(region, chunk, biomeProvider, seaLevel);
 * @endcode
 *
 * @note 洞穴雕刻应在 NOISE 阶段之后、SURFACE 阶段之前进行
 * @note 参考 MC 1.16.5 CaveWorldCarver
 */
class CaveCarver {
public:
    /**
     * @brief 构造洞穴雕刻器
     * @param seed 世界种子
     * @param config 雕刻器配置
     */
    CaveCarver(u64 seed, const CarverConfig& config = CarverConfig{});

    ~CaveCarver() = default;

    /**
     * @brief 在区块中雕刻洞穴
     *
     * @param chunk 要雕刻的区块
     * @param biomeProvider 生物群系提供者（用于获取地表方块）
     * @param seaLevel 海平面高度
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否雕刻了任何方块
     */
    bool carve(ChunkPrimer& chunk,
               const BiomeProvider& biomeProvider,
               i32 seaLevel,
               ChunkCoord chunkX,
               ChunkCoord chunkZ);

    /**
     * @brief 检查是否应该在这个区块生成洞穴
     */
    [[nodiscard]] bool shouldCarve(std::mt19937_64& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const;

    /**
     * @brief 检查方块是否可以被雕刻
     *
     * 这是一个公共静态方法，可以被其他雕刻器使用。
     */
    [[nodiscard]] static bool isCarvable(BlockId block);

private:
    u64 m_seed;
    CarverConfig m_config;
    std::mt19937_64 m_rng;

    /**
     * @brief 生成单个洞穴系统
     */
    void carveTunnel(ChunkPrimer& chunk,
                     i32 seaLevel,
                     ChunkCoord chunkX,
                     ChunkCoord chunkZ,
                     f64 startX, f64 startY, f64 startZ,
                     f32 radius,
                     f32 yaw, f32 pitch,
                     i32 startIdx, i32 endIdx,
                     f64 yawMod, f64 pitchMod);

    /**
     * @brief 生成圆形洞穴房间
     */
    void carveRoom(ChunkPrimer& chunk,
                   i32 seaLevel,
                   ChunkCoord chunkX,
                   ChunkCoord chunkZ,
                   f64 centerX, f64 centerY, f64 centerZ,
                   f32 radius,
                   f64 verticalScale);

    /**
     * @brief 雕刻一个椭球区域
     *
     * @return 是否雕刻了任何方块
     */
    bool carveEllipsoid(ChunkPrimer& chunk,
                        ChunkCoord chunkX, ChunkCoord chunkZ,
                        f64 centerX, f64 centerY, f64 centerZ,
                        f64 horizontalRadius, f64 verticalRadius);

    /**
     * @brief 获取洞穴起始 Y 坐标
     */
    [[nodiscard]] i32 getCaveStartY(std::mt19937_64& rng) const;

    /**
     * @brief 获取洞穴半径
     */
    [[nodiscard]] f32 getCaveRadius(std::mt19937_64& rng) const;

    /**
     * @brief 检查是否在雕刻范围内
     */
    [[nodiscard]] static bool isInCarvingRange(
        ChunkCoord chunkX, ChunkCoord chunkZ,
        f64 x, f64 z,
        i32 step, i32 maxSteps,
        f32 radius);
};

} // namespace mr
