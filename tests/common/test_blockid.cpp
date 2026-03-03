#include <gtest/gtest.h>

#include "common/world/BlockID.hpp"

using namespace mr;

// ============================================================================
// BlockState 测试
// ============================================================================

TEST(BlockState, DefaultConstruction) {
    BlockState state;
    EXPECT_TRUE(state.isAir());
    EXPECT_EQ(state.id(), BlockId::Air);
    EXPECT_EQ(state.data(), 0);
}

TEST(BlockState, ConstructionWithId) {
    BlockState state(BlockId::Stone);
    EXPECT_EQ(state.id(), BlockId::Stone);
    EXPECT_EQ(state.data(), 0);
    EXPECT_FALSE(state.isAir());
}

TEST(BlockState, ConstructionWithIdAndData) {
    BlockState state(BlockId::Dirt, 5);
    EXPECT_EQ(state.id(), BlockId::Dirt);
    EXPECT_EQ(state.data(), 5);
}

TEST(BlockState, SettersAndGetters) {
    BlockState state;
    state.setId(BlockId::GrassBlock);
    state.setData(10);

    EXPECT_EQ(state.id(), BlockId::GrassBlock);
    EXPECT_EQ(state.data(), 10);
}

TEST(BlockState, Comparison) {
    BlockState state1(BlockId::Stone, 0);
    BlockState state2(BlockId::Stone, 0);
    BlockState state3(BlockId::Stone, 1);
    BlockState state4(BlockId::Dirt, 0);

    EXPECT_EQ(state1, state2);
    EXPECT_NE(state1, state3);
    EXPECT_NE(state1, state4);
}

TEST(BlockState, IsAir) {
    BlockState air(BlockId::Air);
    BlockState stone(BlockId::Stone);

    EXPECT_TRUE(air.isAir());
    EXPECT_FALSE(stone.isAir());
}

// ============================================================================
// BlockFlags 测试
// ============================================================================

TEST(BlockFlags, BitOperations) {
    BlockFlags flags = BlockFlags::Solid;
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Solid));
    EXPECT_FALSE(hasFlag(flags, BlockFlags::Transparent));

    flags = flags | BlockFlags::Opaque;
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Solid));
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Opaque));

    flags = flags | BlockFlags::Flammable;
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Flammable));
}

TEST(BlockFlags, CombinedFlags) {
    BlockFlags flags = BlockFlags::Solid | BlockFlags::Opaque | BlockFlags::Flammable;

    EXPECT_TRUE(hasFlag(flags, BlockFlags::Solid));
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Opaque));
    EXPECT_TRUE(hasFlag(flags, BlockFlags::Flammable));
    EXPECT_FALSE(hasFlag(flags, BlockFlags::Transparent));
    EXPECT_FALSE(hasFlag(flags, BlockFlags::Liquid));
}

TEST(BlockFlags, FlagAndOperation) {
    BlockFlags flags1 = BlockFlags::Solid | BlockFlags::Opaque;
    BlockFlags flags2 = BlockFlags::Opaque | BlockFlags::Transparent;

    BlockFlags result = flags1 & flags2;
    EXPECT_TRUE(hasFlag(result, BlockFlags::Opaque));
    EXPECT_FALSE(hasFlag(result, BlockFlags::Solid));
    EXPECT_FALSE(hasFlag(result, BlockFlags::Transparent));
}

// ============================================================================
// BlockRegistry 测试
// ============================================================================

class BlockRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保注册表已初始化
        BlockRegistry::instance().initialize();
    }
};

TEST_F(BlockRegistryTest, Initialization) {
    // 注册表应该已经初始化
    // 初始化多次应该是安全的
    BlockRegistry::instance().initialize();
    BlockRegistry::instance().initialize();
}

TEST_F(BlockRegistryTest, GetVanillaBlocks) {
    auto* air = BlockRegistry::instance().getBlock(BlockId::Air);
    ASSERT_NE(air, nullptr);
    EXPECT_EQ(air->id, BlockId::Air);
    EXPECT_STREQ(air->idName, "air");
    EXPECT_TRUE(air->transparent);
    EXPECT_FALSE(air->solid);

    auto* stone = BlockRegistry::instance().getBlock(BlockId::Stone);
    ASSERT_NE(stone, nullptr);
    EXPECT_EQ(stone->id, BlockId::Stone);
    EXPECT_STREQ(stone->idName, "stone");
    EXPECT_TRUE(stone->solid);
    EXPECT_FALSE(stone->transparent);
}

TEST_F(BlockRegistryTest, GetBlockInfo) {
    auto* dirt = BlockRegistry::instance().getBlock(BlockId::Dirt);
    ASSERT_NE(dirt, nullptr);
    EXPECT_EQ(dirt->id, BlockId::Dirt);
    EXPECT_TRUE(dirt->solid);
    EXPECT_FALSE(dirt->transparent);
    EXPECT_FALSE(dirt->liquid);
    EXPECT_FLOAT_EQ(dirt->hardness, 0.5f);
}

TEST_F(BlockRegistryTest, LiquidBlocks) {
    auto* water = BlockRegistry::instance().getBlock(BlockId::Water);
    ASSERT_NE(water, nullptr);
    EXPECT_FALSE(water->solid);
    EXPECT_TRUE(water->transparent);
    EXPECT_TRUE(water->liquid);

    auto* lava = BlockRegistry::instance().getBlock(BlockId::Lava);
    ASSERT_NE(lava, nullptr);
    EXPECT_TRUE(lava->liquid);
    EXPECT_EQ(lava->lightLevel, 15); // 岩浆发光
}

TEST_F(BlockRegistryTest, LightEmittingBlocks) {
    auto* glowstone = BlockRegistry::instance().getBlock(BlockId::Glowstone);
    ASSERT_NE(glowstone, nullptr);
    EXPECT_TRUE(hasFlag(glowstone->flags, BlockFlags::LightEmitting));
    EXPECT_EQ(glowstone->lightLevel, 15);
}

TEST_F(BlockRegistryTest, InvalidBlockId) {
    auto* invalid = BlockRegistry::instance().getBlock(static_cast<BlockId>(9999));
    EXPECT_EQ(invalid, nullptr);
}

TEST_F(BlockRegistryTest, GetBlockIdByName) {
    BlockId stoneId = BlockRegistry::instance().getBlockId("stone");
    EXPECT_EQ(stoneId, BlockId::Stone);

    BlockId dirtId = BlockRegistry::instance().getBlockId("dirt");
    EXPECT_EQ(dirtId, BlockId::Dirt);

    BlockId unknownId = BlockRegistry::instance().getBlockId("unknown_block");
    EXPECT_EQ(unknownId, BlockId::Air);
}

// ============================================================================
// BlockState 属性测试（需要Registry）
// ============================================================================

class BlockStatePropertiesTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockRegistry::instance().initialize();
    }
};

TEST_F(BlockStatePropertiesTest, IsSolid) {
    BlockState stone(BlockId::Stone);
    EXPECT_TRUE(stone.isSolid());

    BlockState air(BlockId::Air);
    EXPECT_FALSE(air.isSolid());

    BlockState water(BlockId::Water);
    EXPECT_FALSE(water.isSolid());
}

TEST_F(BlockStatePropertiesTest, IsTransparent) {
    BlockState air(BlockId::Air);
    EXPECT_TRUE(air.isTransparent());

    BlockState stone(BlockId::Stone);
    EXPECT_FALSE(stone.isTransparent());

    BlockState water(BlockId::Water);
    EXPECT_TRUE(water.isTransparent());
}

TEST_F(BlockStatePropertiesTest, IsLiquid) {
    BlockState water(BlockId::Water);
    EXPECT_TRUE(water.isLiquid());

    BlockState lava(BlockId::Lava);
    EXPECT_TRUE(lava.isLiquid());

    BlockState stone(BlockId::Stone);
    EXPECT_FALSE(stone.isLiquid());
}

TEST_F(BlockStatePropertiesTest, GetName) {
    BlockState stone(BlockId::Stone);
    EXPECT_STREQ(stone.getName(), "Stone");

    BlockState dirt(BlockId::Dirt);
    EXPECT_STREQ(dirt.getName(), "Dirt");
}

TEST_F(BlockStatePropertiesTest, GetHardness) {
    BlockState stone(BlockId::Stone);
    EXPECT_FLOAT_EQ(stone.getHardness(), 1.5f);

    BlockState bedrock(BlockId::Bedrock);
    EXPECT_FLOAT_EQ(bedrock.getHardness(), -1.0f); // 不可破坏

    BlockState air(BlockId::Air);
    EXPECT_FLOAT_EQ(air.getHardness(), 0.0f);
}

TEST_F(BlockStatePropertiesTest, GetBlastResistance) {
    BlockState stone(BlockId::Stone);
    EXPECT_FLOAT_EQ(stone.getBlastResistance(), 6.0f);

    BlockState obsidian(BlockId::Obsidian);
    EXPECT_FLOAT_EQ(obsidian.getBlastResistance(), 1200.0f);
}

// ============================================================================
// Blocks命名空间测试
// ============================================================================

TEST(BlocksNamespace, ConstantsMatch) {
    EXPECT_EQ(Blocks::Air, BlockId::Air);
    EXPECT_EQ(Blocks::Stone, BlockId::Stone);
    EXPECT_EQ(Blocks::Dirt, BlockId::Dirt);
    EXPECT_EQ(Blocks::GrassBlock, BlockId::GrassBlock);
    EXPECT_EQ(Blocks::Water, BlockId::Water);
    EXPECT_EQ(Blocks::Lava, BlockId::Lava);
    EXPECT_EQ(Blocks::Bedrock, BlockId::Bedrock);
}

// ============================================================================
// BlockInfo结构测试
// ============================================================================

TEST_F(BlockRegistryTest, BlockInfoFields) {
    auto* planks = BlockRegistry::instance().getBlock(BlockId::OakPlanks);
    ASSERT_NE(planks, nullptr);

    EXPECT_EQ(planks->id, BlockId::OakPlanks);
    EXPECT_STREQ(planks->idName, "oak_planks");
    EXPECT_STREQ(planks->displayName, "Oak Planks");
    EXPECT_TRUE(planks->solid);
    EXPECT_FALSE(planks->transparent);
    EXPECT_FALSE(planks->liquid);
    EXPECT_FLOAT_EQ(planks->hardness, 2.0f);
    EXPECT_FLOAT_EQ(planks->blastResistance, 3.0f);
    EXPECT_EQ(planks->lightLevel, 0);
    EXPECT_EQ(planks->lightOpacity, 15);
    EXPECT_TRUE(hasFlag(planks->flags, BlockFlags::Flammable));
}

TEST_F(BlockRegistryTest, OreBlocks) {
    auto* coalOre = BlockRegistry::instance().getBlock(BlockId::CoalOre);
    ASSERT_NE(coalOre, nullptr);
    EXPECT_TRUE(coalOre->solid);

    auto* ironOre = BlockRegistry::instance().getBlock(BlockId::IronOre);
    ASSERT_NE(ironOre, nullptr);
    EXPECT_TRUE(ironOre->solid);

    auto* goldOre = BlockRegistry::instance().getBlock(BlockId::GoldOre);
    ASSERT_NE(goldOre, nullptr);
    EXPECT_TRUE(goldOre->solid);

    auto* diamondOre = BlockRegistry::instance().getBlock(BlockId::DiamondOre);
    ASSERT_NE(diamondOre, nullptr);
    EXPECT_TRUE(diamondOre->solid);
}
