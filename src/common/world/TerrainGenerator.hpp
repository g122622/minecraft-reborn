#pragma once

#include "../core/Types.hpp"
#include "block/Block.hpp"
#include "chunk/ChunkData.hpp"
#include "WorldConstants.hpp"
#include "../math/Noise.hpp"
#include <memory>
#include <functional>

namespace mr {

// ============================================================================
// 生物群系类型
// ============================================================================

enum class BiomeType : u8 {
    Plains,
    Forest,
    Desert,
    Mountains,
    Ocean,
    Snow,
    Jungle,
    Swamp,
    Taiga,
    Savanna,
    Badlands,
    // 可扩展更多
};

// ============================================================================
// 生物群系信息
// ============================================================================

struct BiomeInfo {
    BiomeType type;
    f32 minHeight;      // 最低高度偏移
    f32 maxHeight;      // 最高高度偏移
    f32 temperature;    // 温度 (0-1)
    f32 humidity;       // 湿度 (0-1)
    const BlockState* surfaceBlock = nullptr;    // 地表方块
    const BlockState* subsurfaceBlock = nullptr; // 次地表方块
    const BlockState* fillBlock = nullptr;       // 填充方块 (如水)
};

// ============================================================================
// 世界生成配置
// ============================================================================

struct WorldGenConfig {
    // 种子
    u64 seed = 0;

    // 地形参数
    f32 terrainScale = 0.01f;       // 地形缩放
    f32 terrainHeight = 64.0f;       // 基础地形高度
    f32 terrainVariation = 32.0f;    // 地形高度变化

    // 洞穴参数
    f32 caveFrequency = 0.02f;
    f32 caveThreshold = 0.5f;

    // 矿石参数
    f32 oreFrequency = 0.01f;

    // 海平面
    i32 seaLevel = 62;

    // 生物群系大小
    f32 biomeScale = 0.005f;

    // 噪声八度
    i32 terrainOctaves = 4;
    i32 caveOctaves = 3;
    i32 biomeOctaves = 2;
};

// ============================================================================
// 地形生成器接口
// ============================================================================

class ITerrainGenerator {
public:
    virtual ~ITerrainGenerator() = default;

    // 生成区块
    virtual void generateChunk(ChunkData& chunk) = 0;

    // 生成装饰物 (树木、矿石等)
    virtual void populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) = 0;

    // 获取生物群系
    [[nodiscard]] virtual BiomeType getBiome(i32 x, i32 z) const = 0;

    // 获取生物群系信息
    [[nodiscard]] virtual const BiomeInfo& getBiomeInfo(BiomeType type) const = 0;

    // 获取高度 (用于预览)
    [[nodiscard]] virtual i32 getHeight(i32 x, i32 z) const = 0;

    // 获取配置
    [[nodiscard]] virtual const WorldGenConfig& getConfig() const = 0;
};

// ============================================================================
// 标准地形生成器
// ============================================================================

class StandardTerrainGenerator : public ITerrainGenerator {
public:
    explicit StandardTerrainGenerator(const WorldGenConfig& config = WorldGenConfig{});
    ~StandardTerrainGenerator() override = default;

    // ITerrainGenerator 接口实现
    void generateChunk(ChunkData& chunk) override;
    void populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) override;
    [[nodiscard]] BiomeType getBiome(i32 x, i32 z) const override;
    [[nodiscard]] const BiomeInfo& getBiomeInfo(BiomeType type) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] const WorldGenConfig& getConfig() const override { return m_config; }

    // 额外方法
    void setSeed(u64 seed);

private:
    // 地形生成步骤
    void generateBaseTerrain(ChunkData& chunk);
    void generateCaves(ChunkData& chunk);
    void generateOres(ChunkData& chunk);
    void generateStructures(ChunkData& chunk);

    // 辅助方法
    [[nodiscard]] f32 getTerrainNoise(i32 x, i32 z) const;
    [[nodiscard]] f32 getCaveNoise(i32 x, i32 y, i32 z) const;
    [[nodiscard]] i32 calculateHeight(i32 x, i32 z) const;

    // 生物群系生成
    void initializeBiomes();
    [[nodiscard]] BiomeType calculateBiome(i32 x, i32 z) const;

    // 装饰生成
    void generateTrees(ChunkData& chunk, ChunkData* neighbors[8]);
    void generateFlowers(ChunkData& chunk);
    void generateGrass(ChunkData& chunk);

    WorldGenConfig m_config;
    std::unique_ptr<PerlinNoise> m_terrainNoise;
    std::unique_ptr<PerlinNoise> m_caveNoise;
    std::unique_ptr<PerlinNoise> m_biomeNoise;
    std::unique_ptr<PerlinNoise> m_detailNoise;

    std::array<BiomeInfo, 12> m_biomes;
};

// ============================================================================
// 平坦世界生成器
// ============================================================================

class FlatTerrainGenerator : public ITerrainGenerator {
public:
    explicit FlatTerrainGenerator(i32 layers = 4);
    ~FlatTerrainGenerator() override = default;

    void generateChunk(ChunkData& chunk) override;
    void populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) override;
    [[nodiscard]] BiomeType getBiome(i32 x, i32 z) const override;
    [[nodiscard]] const BiomeInfo& getBiomeInfo(BiomeType type) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] const WorldGenConfig& getConfig() const override { return m_config; }

    void setLayers(i32 layers) { m_layers = layers; }
    void setBlock(const BlockState* block) { m_block = block; }

private:
    i32 m_layers;
    const BlockState* m_block = nullptr;
    WorldGenConfig m_config;
    BiomeInfo m_defaultBiome;
};

// ============================================================================
// 空世界生成器 (用于测试)
// ============================================================================

class EmptyTerrainGenerator : public ITerrainGenerator {
public:
    EmptyTerrainGenerator() = default;
    ~EmptyTerrainGenerator() override = default;

    void generateChunk(ChunkData& chunk) override;
    void populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) override;
    [[nodiscard]] BiomeType getBiome(i32 x, i32 z) const override;
    [[nodiscard]] const BiomeInfo& getBiomeInfo(BiomeType type) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] const WorldGenConfig& getConfig() const override { return m_config; }

private:
    WorldGenConfig m_config;
    BiomeInfo m_defaultBiome;
};

// ============================================================================
// 生成器工厂
// ============================================================================

namespace TerrainGenFactory {

    enum class GeneratorType {
        Standard,
        Flat,
        Empty
    };

    [[nodiscard]] std::unique_ptr<ITerrainGenerator> create(
        GeneratorType type,
        const WorldGenConfig& config = WorldGenConfig{}
    );

    [[nodiscard]] std::unique_ptr<ITerrainGenerator> createStandard(u64 seed);
    [[nodiscard]] std::unique_ptr<ITerrainGenerator> createFlat(i32 layers = 4);
    [[nodiscard]] std::unique_ptr<ITerrainGenerator> createEmpty();

} // namespace TerrainGenFactory

} // namespace mr
