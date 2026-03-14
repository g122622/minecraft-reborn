#include <gtest/gtest.h>
#include <cmath>

#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/ChunkHolder.hpp"
#include "common/world/gen/noise/ImprovedNoiseGenerator.hpp"
#include "common/world/gen/noise/OctavesNoiseGenerator.hpp"
#include "common/world/gen/settings/NoiseSettings.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/math/random/Random.hpp"

using namespace mc;

// ============================================================================
// ChunkStatus 测试
// ============================================================================

TEST(ChunkStatus, BasicProperties) {
    EXPECT_EQ(ChunkStatus::EMPTY.name(), "empty");
    EXPECT_EQ(ChunkStatus::EMPTY.ordinal(), 0);
    EXPECT_EQ(ChunkStatus::EMPTY.parent(), &ChunkStatus::EMPTY);  // EMPTY 是根

    EXPECT_EQ(ChunkStatus::BIOMES.name(), "biomes");
    EXPECT_EQ(ChunkStatus::BIOMES.ordinal(), 1);
    EXPECT_EQ(ChunkStatus::BIOMES.parent(), &ChunkStatus::EMPTY);

    EXPECT_EQ(ChunkStatus::FULL.name(), "full");
    EXPECT_EQ(ChunkStatus::FULL.ordinal(), 7);
    EXPECT_EQ(ChunkStatus::FULL.parent(), &ChunkStatus::HEIGHTMAPS);
}

TEST(ChunkStatus, Ordering) {
    // isAtLeast 测试
    EXPECT_TRUE(ChunkStatus::FULL.isAtLeast(ChunkStatus::EMPTY));
    EXPECT_TRUE(ChunkStatus::FULL.isAtLeast(ChunkStatus::BIOMES));
    EXPECT_TRUE(ChunkStatus::FULL.isAtLeast(ChunkStatus::FULL));

    EXPECT_FALSE(ChunkStatus::EMPTY.isAtLeast(ChunkStatus::FULL));
    EXPECT_FALSE(ChunkStatus::BIOMES.isAtLeast(ChunkStatus::NOISE));

    // isBefore 测试
    EXPECT_TRUE(ChunkStatus::EMPTY.isBefore(ChunkStatus::FULL));
    EXPECT_TRUE(ChunkStatus::BIOMES.isBefore(ChunkStatus::NOISE));
    EXPECT_FALSE(ChunkStatus::FULL.isBefore(ChunkStatus::EMPTY));

    // 比较运算符
    EXPECT_TRUE(ChunkStatus::EMPTY < ChunkStatus::FULL);
    EXPECT_TRUE(ChunkStatus::BIOMES <= ChunkStatus::BIOMES);
    EXPECT_TRUE(ChunkStatus::FULL > ChunkStatus::EMPTY);
}

TEST(ChunkStatus, TaskRange) {
    // FEATURES 阶段需要邻居区块
    EXPECT_EQ(ChunkStatus::FEATURES.taskRange(), 8);

    // 其他阶段不需要邻居
    EXPECT_EQ(ChunkStatus::EMPTY.taskRange(), 0);
    EXPECT_EQ(ChunkStatus::BIOMES.taskRange(), 0);
    EXPECT_EQ(ChunkStatus::NOISE.taskRange(), 0);
    EXPECT_EQ(ChunkStatus::FULL.taskRange(), 0);
}

TEST(ChunkStatus, GetAll) {
    const auto& all = ChunkStatus::getAll();
    EXPECT_EQ(all.size(), 8u);

    // 验证顺序
    EXPECT_EQ(all[0], ChunkStatus::EMPTY);
    EXPECT_EQ(all[1], ChunkStatus::BIOMES);
    EXPECT_EQ(all[2], ChunkStatus::NOISE);
    EXPECT_EQ(all[3], ChunkStatus::SURFACE);
    EXPECT_EQ(all[4], ChunkStatus::CARVERS);
    EXPECT_EQ(all[5], ChunkStatus::FEATURES);
    EXPECT_EQ(all[6], ChunkStatus::HEIGHTMAPS);
    EXPECT_EQ(all[7], ChunkStatus::FULL);
}

TEST(ChunkStatus, ByName) {
    const ChunkStatus* status = ChunkStatus::byName("noise");
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatus::NOISE);

    EXPECT_EQ(ChunkStatus::byName("nonexistent"), nullptr);
}

TEST(ChunkStatus, ByOrdinal) {
    const ChunkStatus* status = ChunkStatus::byOrdinal(3);
    ASSERT_NE(status, nullptr);
    EXPECT_EQ(*status, ChunkStatus::SURFACE);

    EXPECT_EQ(ChunkStatus::byOrdinal(-1), nullptr);
    EXPECT_EQ(ChunkStatus::byOrdinal(100), nullptr);
}

// ============================================================================
// ImprovedNoiseGenerator 测试
// ============================================================================

TEST(ImprovedNoiseGenerator, BasicNoise) {
    ImprovedNoiseGenerator noise(12345);

    // 噪声值应在合理范围内
    for (int i = 0; i < 10; ++i) {
        f32 value = noise.noise(i * 10.0, i * 10.0, i * 10.0);
        EXPECT_GE(value, -1.0);
        EXPECT_LE(value, 1.0);
    }
}

TEST(ImprovedNoiseGenerator, Consistency) {
    ImprovedNoiseGenerator noise1(12345);
    ImprovedNoiseGenerator noise2(12345);

    // 相同种子应产生相同结果
    EXPECT_DOUBLE_EQ(
        noise1.noise(10.0, 20.0, 30.0),
        noise2.noise(10.0, 20.0, 30.0)
    );
}

TEST(ImprovedNoiseGenerator, DifferentSeeds) {
    ImprovedNoiseGenerator noise1(12345);
    ImprovedNoiseGenerator noise2(54321);

    // 不同种子应产生不同结果
    int differences = 0;
    for (int i = 0; i < 100; ++i) {
        f32 val1 = noise1.noise(i * 0.1, i * 0.2, i * 0.3);
        f32 val2 = noise2.noise(i * 0.1, i * 0.2, i * 0.3);
        if (std::abs(val1 - val2) > 0.01) {
            differences++;
        }
    }

    EXPECT_GT(differences, 50);
}

TEST(ImprovedNoiseGenerator, Smoothness) {
    ImprovedNoiseGenerator noise(12345);

    // 相邻点应该平滑过渡
    f32 val1 = noise.noise(10.0, 10.0, 10.0);
    f32 val2 = noise.noise(10.01, 10.0, 10.0);
    f32 val3 = noise.noise(10.0, 10.01, 10.0);

    // Perlin 噪声是连续的
    EXPECT_LT(std::abs(val1 - val2), 0.1);
    EXPECT_LT(std::abs(val1 - val3), 0.1);
}

// ============================================================================
// OctavesNoiseGenerator 测试
// ============================================================================

TEST(OctavesNoiseGenerator, BasicNoise) {
    math::Random rng(12345);
    OctavesNoiseGenerator noise(rng, -7, 0);  // 8 倍频

    // 噪声值应在合理范围内
    for (int i = 0; i < 10; ++i) {
        f32 value = noise.noise(i * 10.0, i * 10.0, i * 10.0);
        // 多倍频噪声范围更大，但应该有界
        EXPECT_GE(value, -5.0);
        EXPECT_LE(value, 5.0);
    }
}

TEST(OctavesNoiseGenerator, Consistency) {
    math::Random rng1(12345);
    math::Random rng2(12345);

    OctavesNoiseGenerator noise1(rng1, -7, 0);
    OctavesNoiseGenerator noise2(rng2, -7, 0);

    EXPECT_DOUBLE_EQ(
        noise1.noise(10.0, 20.0, 30.0),
        noise2.noise(10.0, 20.0, 30.0)
    );
}

TEST(OctavesNoiseGenerator, GetOctave) {
    math::Random rng(12345);
    OctavesNoiseGenerator noise(rng, -7, 0);

    // 获取特定倍频
    ImprovedNoiseGenerator* octave = noise.getOctave(0);
    EXPECT_NE(octave, nullptr);

    // 超出范围的倍频应返回 nullptr
    ImprovedNoiseGenerator* invalid = noise.getOctave(100);
    EXPECT_EQ(invalid, nullptr);
}

TEST(OctavesNoiseGenerator, MaintainPrecision) {
    // 大坐标精度测试
    f32 large = 1000000000.0;
    f32 maintained = OctavesNoiseGenerator::maintainPrecision(large);

    // maintained 应该更接近原点
    EXPECT_LT(std::abs(maintained), std::abs(large));
}

// ============================================================================
// NoiseSettings 测试
// ============================================================================

TEST(NoiseSettings, OverworldDefaults) {
    NoiseSettings settings = NoiseSettings::overworld();

    EXPECT_EQ(settings.height, 256);
    EXPECT_EQ(settings.sizeHorizontal, 1);
    EXPECT_EQ(settings.sizeVertical, 2);
    EXPECT_DOUBLE_EQ(settings.densityFactor, 1.0);
    EXPECT_DOUBLE_EQ(settings.densityOffset, -0.46875);
    EXPECT_EQ(settings.topSlide.target, -10);
    EXPECT_EQ(settings.topSlide.size, 3);
    EXPECT_EQ(settings.bottomSlide.target, -30);
}

TEST(NoiseSettings, NoiseSizeCalculations) {
    NoiseSettings settings = NoiseSettings::overworld();

    // noiseSizeX = 16 / (sizeHorizontal * 4) = 16 / 4 = 4
    EXPECT_EQ(settings.noiseSizeX(), 4);

    // noiseSizeY = height / (sizeVertical * 4) = 256 / 8 = 32
    EXPECT_EQ(settings.noiseSizeY(), 32);

    // noiseSizeZ = 16 / (sizeHorizontal * 4) = 4
    EXPECT_EQ(settings.noiseSizeZ(), 4);
}

TEST(DimensionSettings, OverworldDefaults) {
    DimensionSettings settings = DimensionSettings::overworld();

    EXPECT_EQ(settings.seaLevel, 63);
    EXPECT_EQ(settings.defaultBlock, BlockId::Stone);
    EXPECT_EQ(settings.defaultFluid, BlockId::Water);
}

// ============================================================================
// ChunkPrimer 测试
// ============================================================================

TEST(ChunkPrimer, BasicProperties) {
    ChunkPrimer primer(10, 20);

    EXPECT_EQ(primer.x(), 10);
    EXPECT_EQ(primer.z(), 20);
    EXPECT_EQ(primer.pos(), ChunkPos(10, 20));
}

TEST(ChunkPrimer, StatusManagement) {
    ChunkPrimer primer(0, 0);

    EXPECT_EQ(primer.getStatus(), ChunkLoadStatus::Empty);
    EXPECT_TRUE(primer.getChunkStatus().isAtLeast(ChunkStatus::EMPTY));

    primer.setChunkStatus(ChunkStatus::NOISE);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatus::BIOMES));
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatus::NOISE));
    EXPECT_FALSE(primer.hasCompletedStatus(ChunkStatus::SURFACE));
}

TEST(ChunkPrimer, BiomeContainer) {
    ChunkPrimer primer(0, 0);
    BiomeContainer& biomes = primer.getBiomes();

    // 设置生物群系
    biomes.setBiome(0, 0, 0, Biomes::Plains);
    biomes.setBiome(1, 1, 1, Biomes::Desert);

    EXPECT_EQ(biomes.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(biomes.getBiome(1, 1, 1), Biomes::Desert);

    // 测试方块位置查询
    EXPECT_EQ(primer.getBiomeAtBlock(0, 0, 0), Biomes::Plains);
}

TEST(ChunkPrimer, Heightmap) {
    ChunkPrimer primer(0, 0);

    // 获取高度图
    Heightmap& heightmap = primer.getHeightmap(HeightmapType::WorldSurfaceWG);
    EXPECT_EQ(heightmap.getType(), HeightmapType::WorldSurfaceWG);

    // 初始高度应为 0
    EXPECT_EQ(heightmap.getHeight(0, 0), 0);
}

TEST(ChunkPrimer, PackUnpack) {
    // 测试坐标打包
    u16 packed = ChunkPrimer::packToLocal(5, 10, 7);
    EXPECT_EQ(packed, (5 & 0xF) | ((10 & 0xF) << 4) | ((7 & 0xF) << 8));

    // 测试坐标解包
    BlockCoord x, y, z;
    ChunkPrimer::unpackFromLocal(packed, 1, 0, 0, x, y, z);
    EXPECT_EQ(x, 5);
    EXPECT_EQ(y, 26);  // (10 & 0xF) + (1 << 4) = 10 + 16 = 26
    EXPECT_EQ(z, 7);
}

// ============================================================================
// ChunkHolder 测试
// ============================================================================

TEST(ChunkHolder, BasicProperties) {
    ChunkHolder holder(10, 20);

    EXPECT_EQ(holder.x(), 10);
    EXPECT_EQ(holder.z(), 20);
    EXPECT_EQ(holder.pos(), ChunkPos(10, 20));

    // 初始状态
    EXPECT_EQ(holder.getLevel(), 33);  // 默认级别
    EXPECT_FALSE(holder.shouldLoad());
}

TEST(ChunkHolder, LevelManagement) {
    ChunkHolder holder(0, 0);

    // 设置级别
    holder.setLevel(30);
    EXPECT_EQ(holder.getLevel(), 30);
    EXPECT_TRUE(holder.shouldLoad());  // <= 33

    // 高级别（不加载）
    holder.setLevel(40);
    EXPECT_FALSE(holder.shouldLoad());
}

TEST(ChunkHolder, StatusManagement) {
    ChunkHolder holder(0, 0);

    // 初始状态
    EXPECT_EQ(holder.getStatus(), ChunkStatus::EMPTY);

    // 创建生成区块
    ChunkPrimer* primer = holder.createGeneratingChunk();
    ASSERT_NE(primer, nullptr);
    EXPECT_EQ(primer->x(), 0);
    EXPECT_EQ(primer->z(), 0);

    // 更新状态
    primer->setChunkStatus(ChunkStatus::BIOMES);
    EXPECT_TRUE(holder.getGeneratingChunk()->hasCompletedStatus(ChunkStatus::BIOMES));
}

TEST(ChunkHolder, TicketManagement) {
    ChunkHolder holder(0, 0);

    EXPECT_FALSE(holder.hasTickets());
    EXPECT_EQ(holder.ticketCount(), 0u);

    // 添加票据（简化测试，不验证票据内容）
    EXPECT_FALSE(holder.hasTickets());
}

TEST(ChunkHolder, PlayerTracking) {
    ChunkHolder holder(0, 0);

    EXPECT_FALSE(holder.hasTrackingPlayers());
    EXPECT_EQ(holder.trackingPlayerCount(), 0u);

    // 添加玩家
    holder.addTrackingPlayer(1);
    EXPECT_TRUE(holder.hasTrackingPlayers());
    EXPECT_EQ(holder.trackingPlayerCount(), 1u);

    holder.addTrackingPlayer(2);
    EXPECT_EQ(holder.trackingPlayerCount(), 2u);

    // 移除玩家
    holder.removeTrackingPlayer(1);
    EXPECT_EQ(holder.trackingPlayerCount(), 1u);

    // 重复移除不应崩溃
    holder.removeTrackingPlayer(1);
    EXPECT_EQ(holder.trackingPlayerCount(), 1u);
}

// ============================================================================
// Biome 测试
// ============================================================================

TEST(Biome, PlainsDefaults) {
    Biome plains = BiomeFactory::createPlains();

    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
    EXPECT_FLOAT_EQ(plains.depth(), 0.125f);
    EXPECT_FLOAT_EQ(plains.scale(), 0.05f);
    EXPECT_EQ(plains.surfaceBlock(), BlockId::Grass);
    EXPECT_EQ(plains.subSurfaceBlock(), BlockId::Dirt);
}

TEST(Biome, DesertDefaults) {
    Biome desert = BiomeFactory::createDesert();

    EXPECT_EQ(desert.id(), Biomes::Desert);
    EXPECT_FLOAT_EQ(desert.temperature(), 2.0f);
    EXPECT_FLOAT_EQ(desert.humidity(), 0.0f);
    EXPECT_EQ(desert.surfaceBlock(), BlockId::Sand);
    EXPECT_EQ(desert.subSurfaceBlock(), BlockId::Sand);
}

TEST(Biome, MountainsDefaults) {
    Biome mountains = BiomeFactory::createMountains();

    EXPECT_EQ(mountains.id(), Biomes::Mountains);
    EXPECT_FLOAT_EQ(mountains.depth(), 1.0f);
    EXPECT_EQ(mountains.surfaceBlock(), BlockId::Stone);
    EXPECT_EQ(mountains.subSurfaceBlock(), BlockId::Stone);
}

// ============================================================================
// WorldGenRegion 测试
// ============================================================================

TEST(WorldGenRegion, BasicProperties) {
    // 创建测试区块数组
    std::array<IChunk*, 9> chunks{};
    ChunkPrimer center(0, 0);
    chunks[4] = &center;  // 中心区块

    WorldGenRegion region(0, 0, chunks);

    EXPECT_EQ(region.mainX(), 0);
    EXPECT_EQ(region.mainZ(), 0);
    EXPECT_EQ(region.getMainChunk(), &center);
}

TEST(WorldGenRegion, ChunkAccess) {
    std::array<IChunk*, 9> chunks{};
    ChunkPrimer center(0, 0);
    ChunkPrimer north(0, -1);
    ChunkPrimer south(0, 1);
    ChunkPrimer east(1, 0);
    ChunkPrimer west(-1, 0);

    chunks[4] = &center;
    chunks[1] = &north;
    chunks[7] = &south;
    chunks[5] = &east;
    chunks[3] = &west;

    WorldGenRegion region(0, 0, chunks);

    // 测试邻居访问
    EXPECT_EQ(region.getChunk(0, 0), &center);
    EXPECT_EQ(region.getChunk(0, -1), &north);
    EXPECT_EQ(region.getChunk(0, 1), &south);
    EXPECT_EQ(region.getChunk(1, 0), &east);
    EXPECT_EQ(region.getChunk(-1, 0), &west);

    // 超出范围返回 nullptr
    EXPECT_EQ(region.getChunk(2, 0), nullptr);
}

// ============================================================================
// NoiseChunkGenerator 基础测试
// ============================================================================

TEST(NoiseChunkGenerator, Creation) {
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    EXPECT_EQ(generator.seed(), 12345);
    EXPECT_EQ(generator.seaLevel(), 63);
}

TEST(NoiseChunkGenerator, NoiseSettings) {
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    const NoiseSettings& noise = generator.noiseSettings();

    EXPECT_EQ(noise.height, 256);
    EXPECT_EQ(noise.sizeHorizontal, 1);
    EXPECT_EQ(noise.sizeVertical, 2);
}

TEST(NoiseChunkGenerator, BiomeProvider) {
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    // 测试生物群系获取
    BiomeId biome = generator.getBiome(0, 64, 0);
    EXPECT_NE(biome, Biomes::Ocean);  // 原点不太可能是海洋
}

TEST(NoiseChunkGenerator, HeightEstimation) {
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    // 获取高度估计
    i32 height = generator.getHeight(0, 0, HeightmapType::WorldSurfaceWG);

    // 高度应该在合理范围内
    EXPECT_GT(height, 40);
    EXPECT_LT(height, 200);
}

// ============================================================================
// Integration Test: 完整区块生成
// ============================================================================

class ChunkGenerationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }
};

TEST_F(ChunkGenerationTest, GenerateChunkPrimer) {
    // 创建生成器
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    // 创建区块
    ChunkPrimer primer(0, 0);

    // 创建世界生成区域（简化，无邻居）
    std::array<IChunk*, 9> chunks{};
    chunks[4] = &primer;
    WorldGenRegion region(0, 0, chunks);

    // 执行生成阶段
    generator.generateBiomes(region, primer);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatus::BIOMES));

    generator.generateNoise(region, primer);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatus::NOISE));

    generator.buildSurface(region, primer);
    EXPECT_TRUE(primer.hasCompletedStatus(ChunkStatus::SURFACE));

    // 检查区块是否有一些方块
    // 注意：由于噪声生成，这个测试可能偶尔失败
    bool hasAnyBlock = false;
    for (int y = 0; y < 256 && !hasAnyBlock; ++y) {
        for (int x = 0; x < 16 && !hasAnyBlock; ++x) {
            for (int z = 0; z < 16 && !hasAnyBlock; ++z) {
                if (primer.getBlockStateId(x, y, z) != 0) {
                    hasAnyBlock = true;
                }
            }
        }
    }

    EXPECT_TRUE(hasAnyBlock);
}

TEST_F(ChunkGenerationTest, TerrainHeightVariation) {
    // 创建生成器
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    // 生成多个区块来检查高度变化
    std::vector<i32> heights;

    for (int cx = -2; cx <= 2; ++cx) {
        for (int cz = -2; cz <= 2; ++cz) {
            ChunkPrimer primer(cx, cz);
            std::array<IChunk*, 9> chunks{};
            chunks[4] = &primer;
            WorldGenRegion region(cx, cz, chunks);

            generator.generateBiomes(region, primer);
            generator.generateNoise(region, primer);
            generator.buildSurface(region, primer);

            // 获取中心高度
            i32 height = primer.getTopBlockY(HeightmapType::WorldSurfaceWG, 8, 8);
            heights.push_back(height);
        }
    }

    // 计算高度统计
    i32 minHeight = *std::min_element(heights.begin(), heights.end());
    i32 maxHeight = *std::max_element(heights.begin(), heights.end());
    i32 avgHeight = 0;
    for (i32 h : heights) avgHeight += h;
    avgHeight /= static_cast<i32>(heights.size());

    // 地形高度应该在合理范围内
    // 平坦地形：63 左右（海平面）
    // 山地：100+
    // 海洋：40-
    EXPECT_GT(minHeight, 30) << "Min height too low: " << minHeight;
    EXPECT_LT(maxHeight, 200) << "Max height too high: " << maxHeight;

    // 地形应该有一定的高度变化（不是完全平坦）
    // 注意：这取决于种子和生物群系分布
    // 但至少应该有一些变化
    EXPECT_GE(maxHeight - minHeight, 5) << "Terrain too flat: min=" << minHeight << ", max=" << maxHeight;

    // 平均高度应该在海平面附近或以上
    EXPECT_GT(avgHeight, 50) << "Average height too low: " << avgHeight;
    EXPECT_LT(avgHeight, 120) << "Average height too high: " << avgHeight;
}

TEST_F(ChunkGenerationTest, SingleChunkHeightVariation) {
    // 检查单个区块内的高度变化
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(54321, std::move(settings));

    ChunkPrimer primer(0, 0);
    std::array<IChunk*, 9> chunks{};
    chunks[4] = &primer;
    WorldGenRegion region(0, 0, chunks);

    generator.generateBiomes(region, primer);
    generator.generateNoise(region, primer);
    generator.buildSurface(region, primer);

    // 收集区块内多个位置的高度
    std::vector<i32> heights;
    for (int x = 0; x < 16; x += 4) {
        for (int z = 0; z < 16; z += 4) {
            i32 h = primer.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            heights.push_back(h);
        }
    }

    i32 minHeight = *std::min_element(heights.begin(), heights.end());
    i32 maxHeight = *std::max_element(heights.begin(), heights.end());

    // 输出调试信息
    std::cout << "Heights in single chunk: ";
    for (i32 h : heights) std::cout << h << " ";
    std::cout << "\nMin: " << minHeight << ", Max: " << maxHeight << std::endl;

    // 单个区块内应该有一些高度变化
    EXPECT_GT(maxHeight - minHeight, 0) << "No height variation in single chunk";
}

TEST_F(ChunkGenerationTest, NoiseDensityDebug) {
    // 调试噪声密度值
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(12345, std::move(settings));

    ChunkPrimer primer(0, 0);
    std::array<IChunk*, 9> chunks{};
    chunks[4] = &primer;
    WorldGenRegion region(0, 0, chunks);

    generator.generateBiomes(region, primer);
    generator.generateNoise(region, primer);

    // 检查不同位置的方块
    std::cout << "\n=== Block samples at Y=64 ===" << std::endl;
    for (int x = 0; x < 16; x += 4) {
        for (int z = 0; z < 16; z += 4) {
            const BlockState* state = primer.getBlock(x, 64, z);
            std::string blockName = state ? std::to_string(state->blockId()) : "null";
            std::cout << "(" << x << "," << z << "): " << blockName << " ";
        }
        std::cout << std::endl;
    }

    // 检查生物群系分布
    std::cout << "\n=== Biome samples ===" << std::endl;
    for (int x = 0; x < 16; x += 4) {
        for (int z = 0; z < 16; z += 4) {
            BiomeId biome = primer.getBiomeAtBlock(x, 64, z);
            std::cout << "(" << x << "," << z << "): B" << biome << " ";
        }
        std::cout << std::endl;
    }

    // 检查更多位置的高度
    std::cout << "\n=== Height samples ===" << std::endl;
    i32 minH = 256, maxH = 0;
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            i32 h = primer.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            minH = std::min(minH, h);
            maxH = std::max(maxH, h);
        }
    }
    std::cout << "Height range: " << minH << " - " << maxH << std::endl;
}

TEST_F(ChunkGenerationTest, BiomeDistribution) {
    // 创建生成器
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(54321, std::move(settings));

    // 检查更大范围的生物群系
    std::set<BiomeId> foundBiomes;
    std::map<BiomeId, int> biomeCounts;

    // 采样范围 -500 到 500，更大范围更多生物群系
    for (int x = -500; x <= 500; x += 50) {
        for (int z = -500; z <= 500; z += 50) {
            BiomeId biome = generator.getBiome(x, 64, z);
            foundBiomes.insert(biome);
            biomeCounts[biome]++;
        }
    }

    // 打印发现的生物群系
    std::cout << "Found " << foundBiomes.size() << " biomes:" << std::endl;
    for (const auto& [biomeId, count] : biomeCounts) {
        std::cout << "  Biome " << biomeId << ": " << count << " samples" << std::endl;
    }

    // 应该发现多种生物群系
    EXPECT_GT(foundBiomes.size(), 2u) << "Should find multiple biomes, found: " << foundBiomes.size();

    // 不应该全部是海洋
    bool hasNonOcean = false;
    for (BiomeId biome : foundBiomes) {
        if (biome != Biomes::Ocean && biome != Biomes::DeepOcean) {
            hasNonOcean = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonOcean) << "Should have non-ocean biomes";
}
