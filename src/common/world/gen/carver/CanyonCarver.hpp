#pragma once

#include "CaveCarver.hpp"

namespace mr {

/**
 * @brief 峡谷雕刻器
 *
 * 参考 MC CanyonWorldCarver，生成峡谷地形。
 *
 * 使用方法：
 * @code
 * CanyonCarver carver(seed, config);
 * carver.carve(chunk, biomeProvider, seaLevel, chunkX, chunkZ);
 * @endcode
 *
 * @note 峡谷雕刻应在 NOISE 阶段之后、SURFACE 阶段之前进行
 * @note 参考 MC 1.16.5 CanyonWorldCarver
 */
class CanyonCarver {
public:
    /**
     * @brief 构造峡谷雕刻器
     * @param seed 世界种子
     * @param config 雕刻器配置
     */
    explicit CanyonCarver(u64 seed, const CarverConfig& config = CarverConfig{});

    ~CanyonCarver() = default;

    /**
     * @brief 在区块中雕刻峡谷
     *
     * @param chunk 要雕刻的区块
     * @param biomeProvider 生物群系提供者
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
     * @brief 检查是否应该在这个区块生成峡谷
     */
    [[nodiscard]] bool shouldCarve(std::mt19937_64& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const;

private:
    u64 m_seed;
    CarverConfig m_config;
    std::mt19937_64 m_rng;

    /**
     * @brief 生成蜿蜒峡谷
     *
     * @param chunk 区块数据
     * @param rng 随机数生成器
     * @param seed 区块种子
     * @param startX 起始 X 坐标
     * @param startY 起始 Y 坐标
     * @param startZ 起始 Z 坐标
     * @param yaw 偏航角（水平方向）
     * @param pitch 俯仰角（垂直方向）
     * @param radius 基础半径
     * @param length 峡谷长度
     * @param carvingMask 雕刻掩码
     */
    void generateCanyon(ChunkPrimer& chunk,
                        std::mt19937_64& rng,
                        i64 seed,
                        f64 startX, f64 startY, f64 startZ,
                        f32 yaw, f32 pitch, f32 radius,
                        i32 length);

    /**
     * @brief 更新半径（峡谷入口宽，深处窄）
     * @param baseRadius 基础半径
     * @param progress 进度 (0.0 - 1.0)
     * @return 更新后的半径
     */
    [[nodiscard]] f32 updateRadius(f32 baseRadius, f32 progress) const;

    /**
     * @brief 更新偏航角和俯仰角（蜿蜒曲线）
     */
    void updateYawAndPitch(f32& yaw, f32& pitch,
                           std::mt19937_64& rng,
                           f32 progress) const;

    /**
     * @brief 雕刻一个椭球区域
     */
    bool carveEllipsoid(ChunkPrimer& chunk,
                        ChunkCoord chunkX, ChunkCoord chunkZ,
                        f64 centerX, f64 centerY, f64 centerZ,
                        f64 horizontalRadius, f64 verticalRadius);
};

} // namespace mr
