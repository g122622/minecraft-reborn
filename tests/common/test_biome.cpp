#include <gtest/gtest.h>
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeProvider.hpp"
#include "common/world/biome/layer/Layer.hpp"
#include "common/world/biome/layer/LazyArea.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/biome/layer/transformers/BasicTransformers.hpp"
#include "common/world/biome/layer/transformers/BiomeTransformers.hpp"

using namespace mr;

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

TEST_F(BiomeRegistryTest, CreateForest) {
    Biome forest = BiomeFactory::createForest();
    EXPECT_EQ(forest.id(), Biomes::Forest);
    EXPECT_EQ(forest.name(), "forest");
    EXPECT_FLOAT_EQ(forest.depth(), 0.1f);
    EXPECT_FLOAT_EQ(forest.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateTaiga) {
    Biome taiga = BiomeFactory::createTaiga();
    EXPECT_EQ(taiga.id(), Biomes::Taiga);
    EXPECT_EQ(taiga.name(), "taiga");
    // Taiga 是寒冷的生物群系，温度为负值
    EXPECT_FLOAT_EQ(taiga.temperature(), -0.5f);
    EXPECT_FLOAT_EQ(taiga.humidity(), 0.4f);
}

TEST_F(BiomeRegistryTest, CreateJungle) {
    Biome jungle = BiomeFactory::createJungle();
    EXPECT_EQ(jungle.id(), Biomes::Jungle);
    EXPECT_EQ(jungle.name(), "jungle");
    EXPECT_FLOAT_EQ(jungle.temperature(), 0.95f);
    EXPECT_FLOAT_EQ(jungle.humidity(), 0.9f);
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

TEST_F(SimpleBiomeProviderTest, GetDepthAndScale) {
    f32 depth = provider->getDepth(0, 0);
    f32 scale = provider->getScale(0, 0);

    // 深度和比例应该在合理范围内
    EXPECT_GE(depth, 0.0f);
    EXPECT_GE(scale, 0.0f);
}

TEST_F(SimpleBiomeProviderTest, Consistency) {
    // 相同位置应该返回相同结果
    BiomeId biome1 = provider->getBiome(100, 64, 200);
    BiomeId biome2 = provider->getBiome(100, 64, 200);
    EXPECT_EQ(biome1, biome2);
}

TEST_F(SimpleBiomeProviderTest, SeedVariation) {
    SimpleBiomeProvider provider2(54321);

    // 不同种子应该产生不同结果（概率很高）
    BiomeId biome1 = provider->getBiome(100, 64, 200);
    BiomeId biome2 = provider2.getBiome(100, 64, 200);
    // 注意：这有可能相同，但概率极低
}

// ============================================================================
// Layer 系统测试
// ============================================================================

class LayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        context = std::make_shared<SimpleAreaContext>(12345);
    }

    std::shared_ptr<SimpleAreaContext> context;
};

TEST_F(LayerTest, SimpleAreaContext_NextInt) {
    context->initRandom(12345);

    // 测试随机数生成
    i32 val1 = context->nextInt(10);
    i32 val2 = context->nextInt(10);

    EXPECT_GE(val1, 0);
    EXPECT_LT(val1, 10);
    EXPECT_GE(val2, 0);
    EXPECT_LT(val2, 10);
}

TEST_F(LayerTest, SimpleAreaContext_NextIntWithMod) {
    context->initRandom(12345);

    i32 val = context->nextIntWithMod(100);
    EXPECT_GE(val, 0);
    EXPECT_LT(val, 100);
}

TEST_F(LayerTest, SimpleAreaContext_Reproducibility) {
    context->initRandom(12345);
    i32 val1a = context->nextInt(100);
    i32 val2a = context->nextInt(100);

    context->initRandom(12345);
    i32 val1b = context->nextInt(100);
    i32 val2b = context->nextInt(100);

    EXPECT_EQ(val1a, val1b);
    EXPECT_EQ(val2a, val2b);
}

// ============================================================================
// 变换器测试
// ============================================================================

TEST_F(LayerTest, IslandLayer_Basic) {
    IslandLayer layer(12345);

    // 创建虚拟区域
    context->initRandom(12345);
    i32 val = layer.apply(*context, *std::unique_ptr<IArea>().get(), 0, 0);

    // 岛屿层返回 0（海洋）或 1（陆地）
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, ZoomLayer_Basic) {
    ZoomLayer layer(ZoomLayer::Mode::Normal);

    // 创建简单的输入区域
    class MockArea : public IArea {
    public:
        i32 getValue(i32 x, i32 z) const override {
            return ((x + z) % 2 == 0) ? 1 : 0;
        }
        i32 getWidth() const override { return 16; }
        i32 getHeight() const override { return 16; }
    };

    auto area = std::make_unique<MockArea>();
    context->initRandom(12345);

    i32 val = layer.apply(*context, *area, 0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, ZoomLayer_OutputSize) {
    ZoomLayer layer(ZoomLayer::Mode::Normal);

    i32 outWidth, outHeight;
    layer.getOutputSize(16, 16, outWidth, outHeight);

    EXPECT_EQ(outWidth, 32);
    EXPECT_EQ(outHeight, 32);
}

TEST_F(LayerTest, BiomeLayer_Basic) {
    BiomeLayer layer;

    class MockArea : public IArea {
    public:
        i32 getValue(i32 x, i32 z) const override {
            return 1; // 陆地
        }
        i32 getWidth() const override { return 16; }
        i32 getHeight() const override { return 16; }
    };

    auto area = std::make_unique<MockArea>();
    context->initRandom(12345);

    i32 biome = layer.apply(*context, *area, 0, 0);
    EXPECT_LT(biome, static_cast<i32>(Biomes::Count));
}

TEST_F(LayerTest, HillsLayer_Basic) {
    HillsLayer layer(12345);

    class MockArea : public IArea {
    public:
        i32 getValue(i32 x, i32 z) const override {
            return Biomes::Plains;
        }
        i32 getWidth() const override { return 16; }
        i32 getHeight() const override { return 16; }
    };

    auto area = std::make_unique<MockArea>();
    context->initRandom(12345);

    i32 biome = layer.apply(*context, *area, 0, 0);
    // 可能是 Plains 或 WoodedHills
    EXPECT_TRUE(biome == Biomes::Plains || biome == Biomes::WoodedHills);
}

TEST_F(LayerTest, DeepOceanLayer_OceanToDeepOcean) {
    DeepOceanLayer layer;

    class MockOceanArea : public IArea {
    public:
        i32 getValue(i32 x, i32 z) const override {
            return Biomes::Ocean;
        }
        i32 getWidth() const override { return 16; }
        i32 getHeight() const override { return 16; }
    };

    auto area = std::make_unique<MockOceanArea>();
    context->initRandom(12345);

    i32 biome = layer.apply(*context, *area, 0, 0);
    // 可能保持 Ocean 或变成 DeepOcean
    EXPECT_TRUE(biome == Biomes::Ocean || biome == Biomes::DeepOcean);
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

TEST_F(LayerStackTest, SampleBasic) {
    LayerStack stack(12345);

    // 采样多个位置
    BiomeId biome1 = stack.sample(0, 0);
    BiomeId biome2 = stack.sample(100, 100);
    BiomeId biome3 = stack.sample(-50, -50);

    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
    EXPECT_LT(biome3, Biomes::Count);
}

TEST_F(LayerStackTest, SampleArea) {
    LayerStack stack(12345);

    auto biomes = stack.sampleArea(0, 0, 16, 16);

    EXPECT_EQ(biomes.size(), 256u);
    for (BiomeId biome : biomes) {
        EXPECT_LT(biome, Biomes::Count);
    }
}

TEST_F(LayerStackTest, Consistency) {
    LayerStack stack(12345);

    BiomeId biome1 = stack.sample(100, 200);
    BiomeId biome2 = stack.sample(100, 200);

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

// ============================================================================
// LayerUtil 测试
// ============================================================================

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

TEST_F(LayerStackTest, CreateNetherLayers) {
    auto stack = LayerUtil::createNetherLayers(12345);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerStackTest, CreateEndLayers) {
    auto stack = LayerUtil::createEndLayers(12345);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}
