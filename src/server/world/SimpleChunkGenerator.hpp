#pragma once

#include "../../common/world/gen/IChunkGenerator.hpp"
#include "../../common/world/TerrainGenerator.hpp"
#include <memory>

namespace mr::server {

/**
 * @brief 简单区块生成器适配器
 *
 * 将现有的 ITerrainGenerator 接口适配到 IChunkGenerator 接口。
 * 用于 IntegratedServer 等不需要完整 NoiseChunkGenerator 的场景。
 *
 * 使用方法：
 * @code
 * auto terrainGen = TerrainGenFactory::createStandard(seed);
 * auto chunkGen = std::make_unique<SimpleChunkGenerator>(std::move(terrainGen));
 * ServerChunkManager manager(std::move(chunkGen));
 * @endcode
 */
class SimpleChunkGenerator : public IChunkGenerator {
public:
    /**
     * @brief 从种子创建简单区块生成器
     * @param seed 世界种子
     */
    explicit SimpleChunkGenerator(u64 seed);

    /**
     * @brief 从现有地形生成器创建
     * @param terrainGen 地形生成器
     */
    explicit SimpleChunkGenerator(std::unique_ptr<ITerrainGenerator> terrainGen);

    ~SimpleChunkGenerator() override = default;

    // === IChunkGenerator 接口 ===

    void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] const DimensionSettings& settings() const override { return m_settings; }
    [[nodiscard]] i32 seaLevel() const override { return m_settings.seaLevel; }
    [[nodiscard]] i32 getGroundHeight() const override { return 64; }

private:
    u64 m_seed;
    DimensionSettings m_settings;
    std::unique_ptr<ITerrainGenerator> m_terrainGen;

    /**
     * @brief 从 ChunkPrimer 创建 ChunkData 用于 ITerrainGenerator
     */
    std::unique_ptr<ChunkData> m_tempChunkData;
};

} // namespace mr::server
