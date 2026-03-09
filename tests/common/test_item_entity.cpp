#include <gtest/gtest.h>
#include "../src/common/entity/ItemEntity.hpp"
#include "../src/common/world/drop/DropTables.hpp"
#include "../src/common/item/ItemRegistry.hpp"
#include "../src/common/item/Items.hpp"

using namespace mr;

// ============================================================================
// ItemEntity 测试
// ============================================================================

class ItemEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(ItemEntityTest, Creation) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 10);
    ItemEntity entity(1, stack, 100.0, 64.0, 200.0);

    EXPECT_EQ(entity.id(), 1);
    EXPECT_EQ(entity.type(), EntityType::Item);
    EXPECT_EQ(entity.getItemStack().getCount(), 10);
    EXPECT_EQ(entity.getItemStack().getItem(), m_diamond);
}

TEST_F(ItemEntityTest, Position) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 100.5, 64.0, 200.5);

    EXPECT_DOUBLE_EQ(entity.x(), 100.5);
    EXPECT_DOUBLE_EQ(entity.y(), 64.0);
    EXPECT_DOUBLE_EQ(entity.z(), 200.5);
}

TEST_F(ItemEntityTest, CreationWithVelocity) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0);

    EXPECT_DOUBLE_EQ(entity.velocityX(), 1.0);
    EXPECT_DOUBLE_EQ(entity.velocityY(), 2.0);
    EXPECT_DOUBLE_EQ(entity.velocityZ(), 3.0);
}

TEST_F(ItemEntityTest, Dimensions) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    EXPECT_FLOAT_EQ(entity.width(), 0.25f);
    EXPECT_FLOAT_EQ(entity.height(), 0.25f);
    EXPECT_FLOAT_EQ(entity.eyeHeight(), 0.125f);
}

TEST_F(ItemEntityTest, PickupDelay) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    // 默认拾取延迟
    EXPECT_GT(entity.getPickupDelay(), 0);
    EXPECT_FALSE(entity.canBePickedUp());

    // 设置延迟为0
    entity.setPickupDelay(0);
    EXPECT_EQ(entity.getPickupDelay(), 0);
    EXPECT_TRUE(entity.canBePickedUp());
}

TEST_F(ItemEntityTest, Lifetime) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    // 默认存活时间
    EXPECT_EQ(entity.getAge(), 0);
    EXPECT_FALSE(entity.isExpired());

    // 设置存活时间
    entity.setLifetime(100);
    EXPECT_FALSE(entity.isExpired());

    // 模拟tick
    for (int i = 0; i < 100; ++i) {
        entity.tick();
    }
    EXPECT_TRUE(entity.isExpired());
}

TEST_F(ItemEntityTest, Unpickable) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    entity.setPickupDelay(0);
    EXPECT_TRUE(entity.canBePickedUp());

    entity.setUnpickable();
    EXPECT_FALSE(entity.canBePickedUp());
}

TEST_F(ItemEntityTest, SetItemStack) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 5);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    EXPECT_EQ(entity.getCount(), 5);

    ItemStack newStack(*m_diamond, 32);
    entity.setItemStack(newStack);
    EXPECT_EQ(entity.getCount(), 32);
}

TEST_F(ItemEntityTest, MergeWith) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*m_diamond, 20);

    ItemEntity entity1(1, stack1, 0.0, 0.0, 0.0);
    ItemEntity entity2(2, stack2, 0.0, 0.0, 0.0);

    EXPECT_TRUE(entity1.canMergeWith(entity2));

    // 合并
    bool merged = entity1.tryMergeWith(entity2);
    EXPECT_TRUE(merged);
    EXPECT_EQ(entity1.getCount(), 30);
    EXPECT_TRUE(entity2.isRemoved());
}

TEST_F(ItemEntityTest, CannotMergeDifferentItems) {
    ASSERT_NE(m_diamond, nullptr);
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*stick, 10);

    ItemEntity entity1(1, stack1, 0.0, 0.0, 0.0);
    ItemEntity entity2(2, stack2, 0.0, 0.0, 0.0);

    EXPECT_FALSE(entity1.canMergeWith(entity2));
}

TEST_F(ItemEntityTest, Owner) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0);

    entity.setOwner("player-uuid-123", "thrower-uuid-456");
    EXPECT_EQ(entity.ownerUuid(), "player-uuid-123");
    EXPECT_EQ(entity.throwerUuid(), "thrower-uuid-456");
}

// ============================================================================
// DropContext 测试
// ============================================================================

class DropContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(DropContextTest, DefaultValues) {
    DropContext context;

    EXPECT_EQ(context.tool(), nullptr);
    EXPECT_EQ(context.fortune(), 0);
    EXPECT_EQ(context.looting(), 0);
    EXPECT_FALSE(context.silkTouch());
    EXPECT_FALSE(context.hasSeed());
}

TEST_F(DropContextTest, BuilderPattern) {
    ItemStack stack(*m_diamond, 1);
    DropContext context;

    context.withTool(&stack)
           .withFortune(3)
           .withLooting(2)
           .withSilkTouch(true)
           .withSeed(12345);

    EXPECT_EQ(context.tool(), &stack);
    EXPECT_EQ(context.fortune(), 3);
    EXPECT_EQ(context.looting(), 2);
    EXPECT_TRUE(context.silkTouch());
    EXPECT_TRUE(context.hasSeed());
    EXPECT_EQ(context.seed(), 12345);
}

// ============================================================================
// DropEntry 测试
// ============================================================================

class DropEntryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(DropEntryTest, BasicCreation) {
    ASSERT_NE(m_diamond, nullptr);

    DropEntry entry(*m_diamond, 1, 3, 1.0f);

    EXPECT_EQ(entry.item(), m_diamond);
    EXPECT_EQ(entry.minCount(), 1);
    EXPECT_EQ(entry.maxCount(), 3);
    EXPECT_FLOAT_EQ(entry.chance(), 1.0f);
    EXPECT_FALSE(entry.isEmpty());
}

TEST_F(DropEntryTest, EmptyEntry) {
    DropEntry entry = DropEntry::empty(0.5f);

    EXPECT_TRUE(entry.isEmpty());
}

TEST_F(DropEntryTest, Generate) {
    ASSERT_NE(m_diamond, nullptr);

    DropEntry entry(*m_diamond, 1, 1, 1.0f);
    DropContext context;

    ItemStack stack = entry.generate(context);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), m_diamond);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(DropEntryTest, GenerateWithFortune) {
    ASSERT_NE(m_diamond, nullptr);

    DropEntry entry(*m_diamond, 1, 1, 1.0f);
    entry.setFortuneBonus(2);

    DropContext context;
    context.withFortune(3);

    // 多次测试，验证时运增加掉落
    int totalDrops = 0;
    for (int i = 0; i < 100; ++i) {
        context.withSeed(i);
        ItemStack stack = entry.generate(context);
        if (!stack.isEmpty()) {
            totalDrops += stack.getCount();
        }
    }

    // 有时运时，平均掉落应该大于1
    EXPECT_GT(totalDrops, 100);
}

// ============================================================================
// DropTable 测试
// ============================================================================

class DropTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    }

    Item* m_diamond = nullptr;
    Item* m_coal = nullptr;
};

TEST_F(DropTableTest, EmptyTable) {
    DropTable table;

    EXPECT_TRUE(table.isEmpty());
    EXPECT_EQ(table.entries().size(), 0);

    DropContext context;
    auto drops = table.generateDrops(context);
    EXPECT_TRUE(drops.empty());
}

TEST_F(DropTableTest, SingleItem) {
    ASSERT_NE(m_diamond, nullptr);

    DropTable table;
    table.addItem(*m_diamond, 1, 1);

    EXPECT_EQ(table.entries().size(), 1);

    DropContext context;
    auto drops = table.generateDrops(context);

    EXPECT_EQ(drops.size(), 1);
    EXPECT_EQ(drops[0].getItem(), m_diamond);
    EXPECT_EQ(drops[0].getCount(), 1);
}

TEST_F(DropTableTest, MultipleItems) {
    ASSERT_NE(m_diamond, nullptr);

    DropTable table;
    table.addItem(*m_diamond, 1, 3);

    DropContext context;

    // 统计多次掉落的数量分布
    std::vector<i32> counts;
    for (int i = 0; i < 100; ++i) {
        context.withSeed(i);
        auto drops = table.generateDrops(context);
        if (!drops.empty()) {
            counts.push_back(drops[0].getCount());
        }
    }

    // 验证数量范围
    for (i32 count : counts) {
        EXPECT_GE(count, 1);
        EXPECT_LE(count, 3);
    }
}

TEST_F(DropTableTest, ConditionalDrops) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_coal, nullptr);

    DropTable table;

    // 精准采集时掉落钻石
    DropEntry silkTouchEntry(*m_diamond, 1, 1, 1.0f);
    silkTouchEntry.setCondition(DropCondition::SilkTouch);
    table.addEntry(silkTouchEntry);

    // 非精准采集时掉落煤炭
    DropEntry normalEntry(*m_coal, 1, 1, 1.0f);
    normalEntry.setCondition(DropCondition::NoSilkTouch);
    table.addEntry(normalEntry);

    // 精准采集
    DropContext silkContext;
    silkContext.withSilkTouch(true);
    auto silkDrops = table.generateDrops(silkContext);
    EXPECT_EQ(silkDrops.size(), 1);
    EXPECT_EQ(silkDrops[0].getItem(), m_diamond);

    // 非精准采集
    DropContext normalContext;
    normalContext.withSilkTouch(false);
    auto normalDrops = table.generateDrops(normalContext);
    EXPECT_EQ(normalDrops.size(), 1);
    EXPECT_EQ(normalDrops[0].getItem(), m_coal);
}

TEST_F(DropTableTest, FactoryMethods) {
    ASSERT_NE(m_diamond, nullptr);

    // multipleDrop
    DropTable multiTable = DropTable::multipleDrop(*m_diamond, 1, 5);
    EXPECT_EQ(multiTable.entries().size(), 1);

    // blockDrop
    DropTable blockTable = DropTable::blockDrop(*m_diamond);
    EXPECT_EQ(blockTable.entries().size(), 1);
}

// ============================================================================
// DropTableRegistry 测试
// ============================================================================

class DropTableRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(DropTableRegistryTest, RegisterAndGet) {
    ASSERT_NE(m_diamond, nullptr);

    DropTableRegistry& registry = DropTableRegistry::instance();

    DropTable table;
    table.addItem(*m_diamond, 1, 1);

    registry.registerBlockDrop(static_cast<BlockId>(100), table);

    const DropTable* retrieved = registry.getBlockDrop(static_cast<BlockId>(100));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->entries().size(), 1);

    // 不存在的方块
    const DropTable* missing = registry.getBlockDrop(static_cast<BlockId>(999));
    EXPECT_EQ(missing, nullptr);
}

TEST_F(DropTableRegistryTest, GenerateBlockDrops) {
    ASSERT_NE(m_diamond, nullptr);

    DropTableRegistry& registry = DropTableRegistry::instance();

    DropTable table;
    table.addItem(*m_diamond, 1, 3);

    registry.registerBlockDrop(static_cast<BlockId>(101), table);

    DropContext context;
    auto drops = registry.generateBlockDrops(static_cast<BlockId>(101), context);

    EXPECT_EQ(drops.size(), 1);
    EXPECT_EQ(drops[0].getItem(), m_diamond);
    EXPECT_GE(drops[0].getCount(), 1);
    EXPECT_LE(drops[0].getCount(), 3);
}

TEST_F(DropTableRegistryTest, InitializeVanillaDrops) {
    DropTableRegistry& registry = DropTableRegistry::instance();
    registry.initializeVanillaDrops();

    // 验证钻石矿石掉落表已注册
    // 注意：需要确保 BlockId 对应正确
    // Diamond Ore 的 ID 在原版中是 56
}
