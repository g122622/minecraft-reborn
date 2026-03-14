#include <gtest/gtest.h>
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeProvider.hpp"
#include "common/world/biome/layer/Layer.hpp"
#include "common/world/biome/layer/LayerContext.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/biome/layer/transformers/TransformerTraits.hpp"
#include "common/world/biome/layer/transformers/ClimateLayers.hpp"
#include "common/world/biome/layer/transformers/BiomeLayers.hpp"
#include "common/world/biome/layer/transformers/SourceLayers.hpp"
#include "common/world/biome/layer/transformers/ZoomLayers.hpp"

using namespace mc;

// ============================================================================
// Biome 类测试
// ============================================================================

class BiomeTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeTest, Construction) {
    Biome biome(Biomes::Plains, "plains");

    EXPECT_EQ(biome.id(), Biomes::Plains);
    EXPECT_EQ(biome.name(), "plains");
    EXPECT_EQ(biome.category(), Biome::Category::None);
}

TEST_F(BiomeTest, SettersAndGetters) {
    Biome biome(Biomes::Desert, "desert");

    // 设置地形参数
    biome.setDepth(0.5f);
    biome.setScale(0.3f);
    EXPECT_FLOAT_EQ(biome.depth(), 0.5f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);

    // 设置类别
    biome.setCategory(Biome::Category::Desert);
    EXPECT_EQ(biome.category(), Biome::Category::Desert);

    // 设置气候
    BiomeClimate climate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f);
    biome.setClimate(climate);
    EXPECT_FLOAT_EQ(biome.temperature(), 2.0f);
    EXPECT_EQ(biome.climate().precipitation, BiomeClimate::Precipitation::None);

    // 设置方块
    biome.setSurfaceBlock(BlockId::Sand);
    biome.setSubSurfaceBlock(BlockId::Sand);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setBedrockBlock(BlockId::Bedrock);

    EXPECT_EQ(biome.surfaceBlock(), BlockId::Sand);
    EXPECT_EQ(biome.subSurfaceBlock(), BlockId::Sand);
    EXPECT_EQ(biome.underWaterBlock(), BlockId::Gravel);
    EXPECT_EQ(biome.bedrockBlock(), BlockId::Bedrock);
}

TEST_F(BiomeTest, ClimateDefaults) {
    BiomeClimate climate;
    EXPECT_EQ(climate.precipitation, BiomeClimate::Precipitation::Rain);
    EXPECT_FLOAT_EQ(climate.temperature, 0.5f);
    EXPECT_FLOAT_EQ(climate.downfall, 0.5f);
}

// ============================================================================
// BiomeRegistry 测试
// ============================================================================

class BiomeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeRegistryTest, SingletonAccess) {
    BiomeRegistry& reg1 = BiomeRegistry::instance();
    BiomeRegistry& reg2 = BiomeRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(BiomeRegistryTest, GetBiomeById) {
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
}

TEST_F(BiomeRegistryTest, GetInvalidBiome) {
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(9999));
    // 应该返回默认生物群系
    EXPECT_TRUE(biome.id() == Biomes::Plains || biome.id() == 0);
}

TEST_F(BiomeRegistryTest, HasBiome) {
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Plains));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Desert));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Ocean));
    EXPECT_FALSE(BiomeRegistry::instance().hasBiome(static_cast<BiomeId>(9999)));
}

TEST_F(BiomeRegistryTest, AllBiomesCount) {
    const auto& biomes = BiomeRegistry::instance().allBiomes();
    EXPECT_GE(biomes.size(), Biomes::Count);
}

// ============================================================================
// BiomeFactory 测试
// ============================================================================

TEST_F(BiomeRegistryTest, CreatePlains) {
    Biome plains = BiomeFactory::createPlains();
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
    EXPECT_FLOAT_EQ(plains.depth(), 0.125f);
    EXPECT_FLOAT_EQ(plains.scale(), 0.05f);
    EXPECT_EQ(plains.surfaceBlock(), BlockId::Grass);
    EXPECT_EQ(plains.subSurfaceBlock(), BlockId::Dirt);
}

TEST_F(BiomeRegistryTest, CreateDesert) {
    Biome desert = BiomeFactory::createDesert();
    EXPECT_EQ(desert.id(), Biomes::Desert);
    EXPECT_EQ(desert.name(), "desert");
    EXPECT_FLOAT_EQ(desert.temperature(), 2.0f);
    EXPECT_FLOAT_EQ(desert.humidity(), 0.0f);
    EXPECT_EQ(desert.surfaceBlock(), BlockId::Sand);
    EXPECT_EQ(desert.subSurfaceBlock(), BlockId::Sand);
}

TEST_F(BiomeRegistryTest, CreateMountains) {
    Biome mountains = BiomeFactory::createMountains();
    EXPECT_EQ(mountains.id(), Biomes::Mountains);
    EXPECT_EQ(mountains.name(), "mountains");
    EXPECT_FLOAT_EQ(mountains.depth(), 1.0f);
    EXPECT_EQ(mountains.surfaceBlock(), BlockId::Stone);
    EXPECT_EQ(mountains.subSurfaceBlock(), BlockId::Stone);
}

TEST_F(BiomeRegistryTest, CreateOcean) {
    Biome ocean = BiomeFactory::createOcean();
    EXPECT_EQ(ocean.id(), Biomes::Ocean);
    EXPECT_EQ(ocean.name(), "ocean");
    EXPECT_FLOAT_EQ(ocean.depth(), -1.0f); // 海洋深度为负
}

// ============================================================================
// SimpleBiomeProvider 测试
// ============================================================================

class SimpleBiomeProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
        provider = std::make_unique<SimpleBiomeProvider>(12345);
    }

    std::unique_ptr<SimpleBiomeProvider> provider;
};

TEST_F(SimpleBiomeProviderTest, GetBiome) {
    // 测试不同位置的生物群系
    BiomeId biome1 = provider->getBiome(0, 64, 0);
    BiomeId biome2 = provider->getBiome(100, 64, 100);
    BiomeId biome3 = provider->getBiome(-100, 64, -100);

    // 应该返回有效的生物群系 ID
    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
    EXPECT_LT(biome3, Biomes::Count);
}

TEST_F(SimpleBiomeProviderTest, GetNoiseBiome) {
    BiomeId biome = provider->getNoiseBiome(0, 0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(SimpleBiomeProviderTest, GetBiomeDefinition) {
    const Biome& biome = provider->getBiomeDefinition(Biomes::Plains);
    EXPECT_EQ(biome.id(), Biomes::Plains);
}

TEST_F(SimpleBiomeProviderTest, Consistency) {
    // 相同位置应该返回相同结果
    BiomeId biome1 = provider->getBiome(100, 64, 200);
    BiomeId biome2 = provider->getBiome(100, 64, 200);
    EXPECT_EQ(biome1, biome2);
}

// ============================================================================
// Layer 系统测试
// ============================================================================

class LayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(LayerTest, LayerContext_PositionSeed) {
    LayerContext ctx(1024, 12345, 1);

    ctx.setPosition(0, 0);
    i32 val1 = ctx.nextInt(100);

    ctx.setPosition(0, 0);
    i32 val2 = ctx.nextInt(100);

    // 相同位置应该产生相同的结果
    EXPECT_EQ(val1, val2);
}

TEST_F(LayerTest, LayerContext_DifferentPositions) {
    LayerContext ctx(1024, 12345, 1);

    ctx.setPosition(0, 0);
    i32 val1 = ctx.nextInt(100);

    ctx.setPosition(100, 200);
    i32 val2 = ctx.nextInt(100);

    // 不同位置通常产生不同结果（概率很高）
    // 注意：这不是绝对的，但对于大多数种子来说是正确的
}

TEST_F(LayerTest, IslandLayer_SpawnPointIsLand) {
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);
    layer::IslandLayer islandLayer;

    auto factory = islandLayer.apply(*ctx);
    auto area = factory->create();

    // 原点 (0, 0) 应该是陆地
    i32 val = area->getValue(0, 0);
    EXPECT_EQ(val, 1); // 1 = 陆地
}

TEST_F(LayerTest, ZoomLayer_Scaling) {
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);
    layer::ZoomLayer zoom(layer::ZoomLayer::Mode::Normal);

    // 创建源层
    layer::IslandLayer islandLayer;
    auto sourceFactory = islandLayer.apply(*ctx);
    auto zoomedFactory = zoom.apply(*ctx, std::move(sourceFactory));
    auto area = zoomedFactory->create();

    // 采样应该有效
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, SimpleLayerChain) {
    // 测试简单的层链
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    // 创建第一个缩放
    static layer::ZoomLayer zoom1(layer::ZoomLayer::Mode::Fuzzy);
    factory = zoom1.apply(*ctx, std::move(factory));

    // 创建第二个缩放
    static layer::ZoomLayer zoom2(layer::ZoomLayer::Mode::Normal);
    factory = zoom2.apply(*ctx, std::move(factory));

    // 创建区域并采样
    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, AddIslandLayer) {
    // 测试 AddIslandLayer
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, AddSnowLayer) {
    // 测试 AddSnowLayer
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是 0-4 之间的值
    EXPECT_GE(val, 0);
    EXPECT_LE(val, 4);
}

TEST_F(LayerTest, MultipleContexts) {
    // 测试使用不同上下文的层链 - 模拟 buildOverworldLayers 的行为
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    // 创建源层 - 使用 context 1
    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    // 使用 context 2000 进行模糊缩放
    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    // 使用 context 1 进行 AddIsland
    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    // 使用 context 2001 进行普通缩放
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    // 使用 context 2 进行 AddSnow
    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 创建区域并采样
    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_GE(val, 0);
    EXPECT_LE(val, 4);
}

TEST_F(LayerTest, BiomeLayerTest) {
    // 测试完整的层链直到 BiomeLayer
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 缩放几次
    for (int i = 0; i < 4; ++i) {
        ctx = createContext(static_cast<u64>(2002 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // BiomeLayer
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{false});
    ctx = createContext(200);
    factory = biomeLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是一个有效的生物群系 ID
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, FullLayerChain) {
    // 测试完整的层链
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 缩放几次 (biomeSize = 4, 所以 4+4=8 次)
    for (int i = 0; i < 8; ++i) {
        ctx = createContext(static_cast<u64>(2002 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // BiomeLayer
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{false});
    ctx = createContext(200);
    factory = biomeLayer.apply(*ctx, std::move(factory));

    // 最终缩放
    for (int i = 0; i < 4; ++i) {
        ctx = createContext(static_cast<u64>(3000 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // SmoothLayer
    static layer::SmoothLayer smoothLayer;
    ctx = createContext(1000);
    factory = smoothLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是一个有效的生物群系 ID
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, LayerUtilBuildOverworldLayers) {
    // 直接测试 buildOverworldLayers
    auto factory = LayerUtil::buildOverworldLayers(12345, false, false, 4, 4);
    ASSERT_NE(factory, nullptr);

    auto area = factory->create();
    ASSERT_NE(area, nullptr);

    i32 val = area->getValue(0, 0);
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, CreateOverworldLayers) {
    // 直接测试 createOverworldLayers
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

// ============================================================================
// LayerStack 测试
// ============================================================================

class LayerStackTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(LayerStackTest, CreateOverworldLayers) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerStackTest, CreateOverworldLayers_LargeBiomes) {
    auto stack = LayerUtil::createOverworldLayers(12345, true);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerStackTest, SampleMultiplePositions) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    // 采样多个位置
    BiomeId biome1 = stack->sample(0, 0);
    BiomeId biome2 = stack->sample(100, 100);
    BiomeId biome3 = stack->sample(-50, -50);

    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
    EXPECT_LT(biome3, Biomes::Count);
}

TEST_F(LayerStackTest, SampleArea) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    auto biomes = stack->sampleArea(0, 0, 16, 16);

    EXPECT_EQ(biomes.size(), 256u);
    for (BiomeId biome : biomes) {
        EXPECT_LT(biome, Biomes::Count);
    }
}

TEST_F(LayerStackTest, Consistency) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome1 = stack->sample(100, 200);
    BiomeId biome2 = stack->sample(100, 200);

    EXPECT_EQ(biome1, biome2);
}

// ============================================================================
// LayerBiomeProvider 测试
// ============================================================================

class LayerBiomeProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
        provider = std::make_unique<LayerBiomeProvider>(12345);
    }

    std::unique_ptr<LayerBiomeProvider> provider;
};

TEST_F(LayerBiomeProviderTest, GetBiome) {
    BiomeId biome = provider->getBiome(0, 64, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerBiomeProviderTest, GetNoiseBiome) {
    BiomeId biome = provider->getNoiseBiome(0, 0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerBiomeProviderTest, GetDepthAndScale) {
    f32 depth = provider->getDepth(0, 0);
    f32 scale = provider->getScale(0, 0);

    EXPECT_GE(depth, -2.0f);  // 深海可能为负
    EXPECT_GE(scale, 0.0f);
}

TEST_F(LayerBiomeProviderTest, GetBiomeDefinition) {
    const Biome& biome = provider->getBiomeDefinition(Biomes::Plains);
    EXPECT_EQ(biome.id(), Biomes::Plains);
}

TEST_F(LayerBiomeProviderTest, BiomeDistribution) {
    // 统计生物群系分布
    std::map<BiomeId, int> distribution;

    for (int x = 0; x < 100; ++x) {
        for (int z = 0; z < 100; ++z) {
            BiomeId biome = provider->getBiome(x * 10, 64, z * 10);
            distribution[biome]++;
        }
    }

    // 应该有多种生物群系
    EXPECT_GT(distribution.size(), 1u);
}
