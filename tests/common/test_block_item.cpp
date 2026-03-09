#include <gtest/gtest.h>

#include "common/entity/Player.hpp"
#include "common/item/BlockItemRegistry.hpp"
#include "common/item/BlockItemUseContext.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/VanillaBlocks.hpp"

#include <unordered_map>

using namespace mr;

namespace {

class TestBlockReader final : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(key(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override {
        (void)x;
        (void)z;
        return y >= 0 && y < 256;
    }

    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) {
        m_blocks[key(x, y, z)] = state;
    }

private:
    static i64 key(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
};

}

class BlockItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

TEST_F(BlockItemTest, RegistryMapsStoneBlockItem) {
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(&item->block(), VanillaBlocks::STONE);
}

TEST_F(BlockItemTest, CreativeInventoryGetsRegisteredBlockItems) {
    Player player(1, "test");
    player.setCreativeModeInventory();

    const ItemStack selected = player.inventory().getSelectedStack();
    EXPECT_FALSE(selected.isEmpty());
    ASSERT_NE(selected.getItem(), nullptr);
    EXPECT_TRUE(BlockItemRegistry::instance().isBlockItem(selected.getItem()));
}

TEST_F(BlockItemTest, PlacementContextUsesAdjacentPosForSolidBlock) {
    TestBlockReader world;
    world.setBlock(0, 64, 0, &VanillaBlocks::STONE->defaultState());

    const BlockItem* stoneItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(stoneItem, nullptr);

    ItemStack stack(*stoneItem, 64);
    BlockItemUseContext context(world,
                                nullptr,
                                stack,
                                Vector3(0.5f, 64.99f, 0.5f),
                                BlockPos(0, 64, 0),
                                Direction::Up,
                                0.0f);

    EXPECT_FALSE(context.replacingClickedBlock());
    EXPECT_EQ(context.placementPos(), BlockPos(0, 65, 0));
    EXPECT_TRUE(context.canPlace());
}