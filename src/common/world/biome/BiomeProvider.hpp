#pragma once

#include "Biome.hpp"
#include "../chunk/IChunk.hpp"
#include "../gen/noise/OctavesNoiseGenerator.hpp"
#include "../../core/Types.hpp"
#include <memory>
#include <random>

namespace mr {

/**
 * @brief 生物群系提供者基类
 *
 * 参考 MC BiomeProvider，负责提供世界中的生物群系信息。
 *
 * 使用方法：
 * @code
 * auto provider = createBiomeProvider(seed);
 * BiomeId biome = provider->getBiome(x, y, z);
 * @endcode
 *
 * @note 参考 MC 1.16.5 BiomeProvider
 */
class BiomeProvider {
public:
    explicit BiomeProvider(u64 seed);
    virtual ~BiomeProvider() = default;

    /**
     * @brief 获取世界坐标处的生物群系
     * @param x 世界 X 坐标
     * @param y 世界 Y 坐标
     * @param z 世界 Z 坐标
     * @return 生物群系ID
     */
    [[nodiscard]] virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取噪声坐标处的生物群系
     *
     * 噪声坐标是 4x4 方块一个大块
     * @param noiseX 噪声 X 坐标 (x / 4)
     * @param noiseY 噪声 Y 坐标 (y / 4)
     * @param noiseZ 噪声 Z 坐标 (z / 4)
     * @return 生物群系ID
     */
    [[nodiscard]] virtual BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const = 0;

    /**
     * @brief 获取生物群系的深度参数
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 深度值（影响地形高度）
     */
    [[nodiscard]] virtual f32 getDepth(i32 x, i32 z) const = 0;

    /**
     * @brief 获取生物群系的比例参数
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 比例值（影响高度变化）
     */
    [[nodiscard]] virtual f32 getScale(i32 x, i32 z) const = 0;

    /**
     * @brief 获取生物群系定义
     * @param id 生物群系ID
     * @return 生物群系定义
     */
    [[nodiscard]] virtual const Biome& getBiomeDefinition(BiomeId id) const;

    /**
     * @brief 填充区块的生物群系数据
     * @param container 生物群系容器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    virtual void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) = 0;

    [[nodiscard]] u64 seed() const { return m_seed; }

protected:
    u64 m_seed;
};

/**
 * @brief 简单的生物群系提供者
 *
 * 使用噪声生成生物群系分布。
 * 这是一个简化版本，完整的版本需要使用 Layer 系统。
 *
 * @note 参考 MC 1.16.5 OverworldBiomeProvider
 */
class SimpleBiomeProvider : public BiomeProvider {
public:
    /**
     * @brief 构造简单生物群系提供者
     * @param seed 世界种子
     */
    explicit SimpleBiomeProvider(u64 seed);

    ~SimpleBiomeProvider() override = default;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    [[nodiscard]] const Biome& getBiomeDefinition(BiomeId id) const override;

    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

private:
    std::unique_ptr<PerlinNoiseGenerator> m_temperatureNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_humidityNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_continentalnessNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_erosionNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_depthNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_scaleNoise;

    /**
     * @brief 根据噪声参数选择生物群系
     */
    [[nodiscard]] BiomeId selectBiome(f32 temperature, f32 humidity, f32 depth, f32 scale) const;
};

} // namespace mr
