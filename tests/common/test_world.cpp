#include <gtest/gtest.h>
#include <cmath>

#include "common/world/chunk/ChunkData.hpp"
#include "common/world/TerrainGenerator.hpp"
#include "common/math/Noise.hpp"

using namespace mr;
using namespace mr::world;

// ============================================================================
// ChunkSection 测试
// ============================================================================

TEST(ChunkSection, Construction) {
    ChunkSection section;
    EXPECT_EQ(section.getBlockCount(), 0);
    EXPECT_TRUE(section.isEmpty());
}

TEST(ChunkSection, SetGetBlock) {
    ChunkSection section;

    // 设置方块
    section.setBlock(5, 10, 7, BlockState(BlockId::Stone));
    EXPECT_EQ(section.getBlockCount(), 1);
    EXPECT_FALSE(section.isEmpty());

    // 获取方块
    BlockState block = section.getBlock(5, 10, 7);
    EXPECT_EQ(block.id(), BlockId::Stone);
    EXPECT_EQ(block.data(), 0);

    // 边界检查 - 返回空气
    BlockState outOfBounds = section.getBlock(16, 0, 0);
    EXPECT_TRUE(outOfBounds.isAir());
}

TEST(ChunkSection, FastAccess) {
    ChunkSection section;
    i32 index = ChunkSection::blockIndex(3, 5, 7);

    section.setBlockFast(index, BlockState(BlockId::Dirt, 1));
    BlockState block = section.getBlock(3, 5, 7);
    EXPECT_EQ(block.id(), BlockId::Dirt);
    EXPECT_EQ(block.data(), 1);
}

TEST(ChunkSection, BlockCount) {
    ChunkSection section;

    // 添加方块
    section.setBlock(0, 0, 0, BlockState(BlockId::Stone));
    section.setBlock(1, 1, 1, BlockState(BlockId::Dirt));
    EXPECT_EQ(section.getBlockCount(), 2);

    // 替换方块 (不增加计数)
    section.setBlock(0, 0, 0, BlockState(BlockId::GrassBlock));
    EXPECT_EQ(section.getBlockCount(), 2);

    // 移除方块 (设为空气)
    section.setBlock(0, 0, 0, BlockState(BlockId::Air));
    EXPECT_EQ(section.getBlockCount(), 1);

    // 再次移除 (计数不变)
    section.setBlock(0, 0, 0, BlockState(BlockId::Air));
    EXPECT_EQ(section.getBlockCount(), 1);
}

TEST(ChunkSection, LightAccess) {
    ChunkSection section;

    // 天空光照
    section.setSkyLight(5, 5, 5, 10);
    EXPECT_EQ(section.getSkyLight(5, 5, 5), 10);

    // 方块光照
    section.setBlockLight(5, 5, 5, 12);
    EXPECT_EQ(section.getBlockLight(5, 5, 5), 12);

    // 边界检查 - 天空光照返回15，方块光照返回0
    EXPECT_EQ(section.getSkyLight(-1, 0, 0), 15);
    EXPECT_EQ(section.getBlockLight(-1, 0, 0), 0);
}

TEST(ChunkSection, Serialization) {
    ChunkSection original;
    original.setBlock(0, 0, 0, BlockState(BlockId::Stone));
    original.setBlock(7, 7, 7, BlockState(BlockId::Dirt, 5));
    original.setSkyLight(3, 3, 3, 15);
    original.setBlockLight(3, 3, 3, 10);

    // 序列化
    auto data = original.serialize();
    EXPECT_FALSE(data.empty());

    // 反序列化
    auto result = ChunkSection::deserialize(data.data(), data.size());
    EXPECT_TRUE(result.success());

    auto& restored = result.value();
    EXPECT_EQ(restored->getBlock(0, 0, 0).id(), BlockId::Stone);
    EXPECT_EQ(restored->getBlock(7, 7, 7).id(), BlockId::Dirt);
    EXPECT_EQ(restored->getBlock(7, 7, 7).data(), 5);
    EXPECT_EQ(restored->getSkyLight(3, 3, 3), 15);
    EXPECT_EQ(restored->getBlockLight(3, 3, 3), 10);
}

TEST(ChunkSection, Fill) {
    ChunkSection section;
    section.fill(BlockState(BlockId::Stone, 2));

    EXPECT_EQ(section.getBlockCount(), ChunkSection::VOLUME);

    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                BlockState block = section.getBlock(x, y, z);
                EXPECT_EQ(block.id(), BlockId::Stone);
                EXPECT_EQ(block.data(), 2);
            }
        }
    }
}

// ============================================================================
// ChunkData 测试
// ============================================================================

TEST(ChunkData, Construction) {
    ChunkData chunk(10, 20);
    EXPECT_EQ(chunk.x(), 10);
    EXPECT_EQ(chunk.z(), 20);
    EXPECT_FALSE(chunk.isLoaded());
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_FALSE(chunk.isFullyGenerated());
}

TEST(ChunkData, SetGetBlock) {
    ChunkData chunk;

    // 设置方块
    chunk.setBlock(5, 100, 7, BlockState(BlockId::Stone));
    EXPECT_TRUE(chunk.isDirty());

    // 获取方块
    BlockState block = chunk.getBlock(5, 100, 7);
    EXPECT_EQ(block.id(), BlockId::Stone);

    // 边界检查
    BlockState outOfBounds = chunk.getBlock(-1, 0, 0);
    EXPECT_TRUE(outOfBounds.isAir());
    outOfBounds = chunk.getBlock(0, 500, 0);
    EXPECT_TRUE(outOfBounds.isAir());
}

TEST(ChunkData, SectionManagement) {
    ChunkData chunk;

    // 初始没有段
    EXPECT_FALSE(chunk.hasSection(0));
    EXPECT_FALSE(chunk.hasSection(10));

    // 设置方块会创建段
    // Y=50, sectionIndex = 50 / CHUNK_SECTION_HEIGHT(16) = 3
    chunk.setBlock(5, 50, 7, BlockState(BlockId::Dirt));

    // 段3应该被创建
    EXPECT_TRUE(chunk.hasSection(3));
    EXPECT_FALSE(chunk.hasSection(0));  // 段0不应该被创建
}

TEST(ChunkData, HeightMap) {
    ChunkData chunk;

    // 设置一些方块
    chunk.setBlock(0, 10, 0, BlockState(BlockId::Stone));
    chunk.setBlock(0, 20, 0, BlockState(BlockId::Dirt));
    chunk.setBlock(0, 30, 0, BlockState(BlockId::GrassBlock));

    // 最高方块应该是30
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 30);

    // 移除最高方块
    chunk.setBlock(0, 30, 0, BlockState(BlockId::Air));

    // 更新高度图
    chunk.updateHeightMap(0, 0);
    EXPECT_EQ(chunk.getHighestBlock(0, 0), 20);
}

TEST(ChunkData, Serialization) {
    ChunkData original(5, 10);
    original.setBlock(0, 0, 0, BlockState(BlockId::Stone));
    original.setBlock(8, 100, 8, BlockState(BlockId::Dirt, 3));
    original.setFullyGenerated(true);

    // 序列化
    auto data = original.serialize();
    EXPECT_FALSE(data.empty());

    // 反序列化
    auto result = ChunkData::deserialize(data.data(), data.size());
    EXPECT_TRUE(result.success());

    auto& restored = result.value();
    EXPECT_EQ(restored->x(), 5);
    EXPECT_EQ(restored->z(), 10);
    EXPECT_TRUE(restored->isFullyGenerated());
    EXPECT_EQ(restored->getBlock(0, 0, 0).id(), BlockId::Stone);
    EXPECT_EQ(restored->getBlock(8, 100, 8).id(), BlockId::Dirt);
    EXPECT_EQ(restored->getBlock(8, 100, 8).data(), 3);
}

// ============================================================================
// PerlinNoise 测试
// ============================================================================

TEST(PerlinNoise, Basic2D) {
    PerlinNoise noise(12345);

    // 噪声值应在 [0, 1] 范围内
    for (int i = 0; i < 10; ++i) {
        f32 value = noise.noise2D(static_cast<f32>(i) * 0.1f, static_cast<f32>(i) * 0.2f);
        EXPECT_GE(value, 0.0f);
        EXPECT_LE(value, 1.0f);
    }
}

TEST(PerlinNoise, Consistency) {
    PerlinNoise noise1(12345);
    PerlinNoise noise2(12345);

    // 相同种子应产生相同结果
    EXPECT_FLOAT_EQ(noise1.noise2D(10.0f, 20.0f), noise2.noise2D(10.0f, 20.0f));
    EXPECT_FLOAT_EQ(noise1.noise3D(5.0f, 10.0f, 15.0f), noise2.noise3D(5.0f, 10.0f, 15.0f));
}

TEST(PerlinNoise, DifferentSeeds) {
    PerlinNoise noise1(12345);
    PerlinNoise noise2(54321);

    // 收集两个噪声生成器在不同位置的值
    std::vector<f32> values1;
    std::vector<f32> values2;

    for (int i = 0; i < 100; ++i) {
        f32 x = static_cast<f32>(i) * 0.1f;
        f32 z = static_cast<f32>(i) * 0.15f;
        values1.push_back(noise1.noise2D(x, z));
        values2.push_back(noise2.noise2D(x, z));
    }

    // 计算两个值序列的差异
    int differences = 0;
    for (size_t i = 0; i < values1.size(); ++i) {
        if (std::abs(values1[i] - values2[i]) > 0.01f) {
            differences++;
        }
    }

    // 不同种子应该产生显著不同的噪声模式
    // 100个点中至少要有一些不同
    EXPECT_GT(differences, 10);
}

TEST(PerlinNoise, OctaveNoise) {
    PerlinNoise noise(12345);

    f32 single = noise.noise2D(10.0f, 20.0f);
    f32 octave = noise.octave2D(10.0f, 20.0f, 4, 0.5f);

    // 分形噪声应该产生不同的结果
    // 值应该在合理范围内
    EXPECT_GE(octave, 0.0f);
    EXPECT_LE(octave, 1.0f);
}

// ============================================================================
// TerrainGenerator 测试
// ============================================================================

TEST(TerrainGenerator, EmptyGenerator) {
    auto generator = TerrainGenFactory::createEmpty();

    ChunkData chunk(0, 0);
    generator->generateChunk(chunk);

    // 空世界应该没有方块
    EXPECT_TRUE(chunk.isFullyGenerated());

    // 所有位置应该是空气
    for (i32 y = 0; y < 64; ++y) {
        EXPECT_TRUE(chunk.getBlock(8, y, 8).isAir());
    }

    EXPECT_EQ(generator->getHeight(0, 0), MIN_BUILD_HEIGHT);
}

TEST(TerrainGenerator, FlatGenerator) {
    auto generator = TerrainGenFactory::createFlat(4);

    ChunkData chunk(0, 0);
    generator->generateChunk(chunk);

    EXPECT_TRUE(chunk.isFullyGenerated());

    // 检查层级
    // 从 MIN_BUILD_HEIGHT 开始: 基岩、石头层、泥土层、草地层
    EXPECT_EQ(chunk.getBlock(8, MIN_BUILD_HEIGHT, 8).id(), BlockId::Bedrock);

    // 高度应该是 MIN_BUILD_HEIGHT + layers
    i32 expectedHeight = MIN_BUILD_HEIGHT + 4;
    EXPECT_EQ(generator->getHeight(0, 0), expectedHeight);
}

TEST(TerrainGenerator, StandardGenerator) {
    WorldGenConfig config;
    config.seed = 12345;
    config.terrainScale = 0.01f;
    config.terrainHeight = 64.0f;

    auto generator = TerrainGenFactory::createStandard(12345);

    ChunkData chunk(0, 0);
    generator->generateChunk(chunk);

    EXPECT_TRUE(chunk.isFullyGenerated());

    // 高度应该在合理范围内
    i32 height = generator->getHeight(8, 8);
    EXPECT_GT(height, MIN_BUILD_HEIGHT);
    EXPECT_LT(height, MAX_BUILD_HEIGHT);

    // 生物群系应该是有效的
    BiomeType biome = generator->getBiome(0, 0);
    EXPECT_GE(static_cast<int>(biome), 0);
    EXPECT_LT(static_cast<int>(biome), 12);

    // 生物群系信息应该有效
    const BiomeInfo& info = generator->getBiomeInfo(biome);
    EXPECT_EQ(info.type, biome);
}

TEST(TerrainGenerator, BiomeGeneration) {
    auto generator = TerrainGenFactory::createStandard(54321);

    // 不同位置可能有不同生物群系
    BiomeType biome1 = generator->getBiome(0, 0);
    BiomeType biome2 = generator->getBiome(1000, 1000);
    BiomeType biome3 = generator->getBiome(1000, 0);

    // 生物群系信息应该都能获取
    (void)generator->getBiomeInfo(biome1);
    (void)generator->getBiomeInfo(biome2);
    (void)generator->getBiomeInfo(biome3);
}

TEST(TerrainGenerator, ChunkConsistency) {
    auto generator = TerrainGenFactory::createStandard(99999);

    // 生成相邻区块，检查边界一致性
    ChunkData chunk1(0, 0);
    ChunkData chunk2(0, 1);

    generator->generateChunk(chunk1);
    generator->generateChunk(chunk2);

    // 边界高度应该一致 (大致)
    // 由于噪声的连续性，相邻位置的高度应该接近
    i32 height1 = generator->getHeight(15, 0);
    i32 height2 = generator->getHeight(16, 0); // chunk2 的边缘

    // 高度差不应该太大 (允许一定差异)
    EXPECT_LT(std::abs(height1 - height2), 10);
}
