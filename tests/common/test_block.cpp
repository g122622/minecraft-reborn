#include <gtest/gtest.h>
#include "../src/common/util/property/StateHolder.hpp"
#include "../src/common/util/property/StateContainer.hpp"
#include "../src/common/world/block/Material.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/world/block/BlockRegistry.hpp"
#include "../src/common/world/block/VanillaBlocks.hpp"
#include "../src/common/util/property/Properties.hpp"

using namespace mr;

// ============================================================================
// 测试用的简单方块类
// ============================================================================

class TestBlock : public Block {
public:
    explicit TestBlock(BlockProperties properties)
        : Block(properties) {
        // 创建空状态容器
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }
};

class TestBlockWithAxis : public Block {
public:
    explicit TestBlockWithAxis(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addAxis("axis")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    // 使用预定义属性
    static const EnumProperty<Axis>& AXIS() {
        return BlockStateProperties::AXIS();
    }
};

class TestBlockWithFacing : public Block {
public:
    explicit TestBlockWithFacing(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addHorizontalDirection("facing")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING() {
        return BlockStateProperties::HORIZONTAL_FACING();
    }
};

class TestBlockWithMultiple : public Block {
public:
    explicit TestBlockWithMultiple(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addHorizontalDirection("facing")
            .addBoolean("lit")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING() {
        return BlockStateProperties::HORIZONTAL_FACING();
    }

    static const BooleanProperty& LIT() {
        return BlockStateProperties::LIT();
    }
};

// ============================================================================
// Material 测试
// ============================================================================

TEST(MaterialTest, PredefinedMaterials) {
    // 空气
    EXPECT_FALSE(Material::AIR.blocksMovement());
    EXPECT_FALSE(Material::AIR.isSolid());
    EXPECT_TRUE(Material::AIR.isReplaceable());

    // 石头
    EXPECT_TRUE(Material::ROCK.isSolid());
    EXPECT_TRUE(Material::ROCK.blocksMovement());
    EXPECT_FALSE(Material::ROCK.isLiquid());

    // 水
    EXPECT_TRUE(Material::WATER.isLiquid());
    EXPECT_FALSE(Material::WATER.isSolid());
    EXPECT_TRUE(Material::WATER.isReplaceable());

    // 木头
    EXPECT_TRUE(Material::WOOD.isSolid());
    EXPECT_TRUE(Material::WOOD.isFlammable());

    // 树叶
    EXPECT_TRUE(Material::LEAVES.isSolid());
    EXPECT_TRUE(Material::LEAVES.isFlammable());
}

TEST(MaterialTest, MaterialBuilder) {
    Material customMaterial = MaterialBuilder()
        .solid()
        .flammable()
        .opaque()
        .build();

    EXPECT_TRUE(customMaterial.isSolid());
    EXPECT_TRUE(customMaterial.isFlammable());
    EXPECT_TRUE(customMaterial.isOpaque());
    EXPECT_FALSE(customMaterial.isLiquid());
}

// ============================================================================
// BlockProperties 测试
// ============================================================================

TEST(BlockPropertiesTest, BasicProperties) {
    BlockProperties props{Material::ROCK};

    // 注意: BlockProperties 存储 Material 副本，不是引用
    EXPECT_EQ(props.material().isSolid(), Material::ROCK.isSolid());
    EXPECT_EQ(props.material().blocksMovement(), Material::ROCK.blocksMovement());
    EXPECT_EQ(props.hardness(), 0.0f);
    EXPECT_EQ(props.resistance(), 0.0f);
    EXPECT_EQ(props.lightLevel(), 0);
    EXPECT_TRUE(props.hasCollision());
    EXPECT_TRUE(props.isSolid());
    EXPECT_FALSE(props.isFlammable());
}

TEST(BlockPropertiesTest, ChainProperties) {
    BlockProperties props = BlockProperties{Material::WOOD}
        .hardness(2.0f)
        .resistance(3.0f)
        .lightLevel(15)
        .flammable();

    EXPECT_EQ(props.hardness(), 2.0f);
    EXPECT_EQ(props.resistance(), 3.0f);
    EXPECT_EQ(props.lightLevel(), 15);
    EXPECT_TRUE(props.isFlammable());
}

TEST(BlockPropertiesTest, SpecialFlags) {
    BlockProperties noCollision = BlockProperties{Material::ROCK}.noCollision();
    EXPECT_FALSE(noCollision.hasCollision());

    BlockProperties notSolid = BlockProperties{Material::GLASS}.notSolid();
    EXPECT_FALSE(notSolid.isSolid());

    BlockProperties replaceable = BlockProperties{Material::PLANT}.replaceable();
    EXPECT_TRUE(replaceable.isReplaceable());
}

TEST(BlockPropertiesTest, Strength) {
    BlockProperties props = BlockProperties{Material::ROCK}.strength(2.5f);

    EXPECT_EQ(props.hardness(), 2.5f);
    EXPECT_EQ(props.resistance(), 2.5f);
}

// ============================================================================
// StateContainer 测试
// ============================================================================

TEST(StateContainerTest, EmptyContainer) {
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.0f)};

    const auto& container = block.stateContainer();

    // 空容器应该有1个状态（基础状态）
    EXPECT_EQ(container.stateCount(), 1u);

    // 基础状态应该没有属性
    const auto& baseState = container.baseState();
    EXPECT_EQ(baseState.values().size(), 0u);
}

TEST(StateContainerTest, SingleProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};

    const auto& container = block.stateContainer();

    // axis 有 3 个值 (X, Y, Z)
    EXPECT_EQ(container.stateCount(), 3u);

    // 验证所有状态
    const auto& states = container.validStates();
    EXPECT_EQ(states.size(), 3u);

    // 获取属性
    const auto* prop = container.getProperty("axis");
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->name(), "axis");
}

TEST(StateContainerTest, MultipleProperties) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    const auto& container = block.stateContainer();

    // facing: 4 values * lit: 2 values = 8 states
    EXPECT_EQ(container.stateCount(), 8u);
}

TEST(StateContainerTest, GetProperty) {
    TestBlockWithFacing block{BlockProperties{Material::ROCK}};

    const auto& container = block.stateContainer();

    const auto* facing = container.getProperty("facing");
    ASSERT_NE(facing, nullptr);
    EXPECT_EQ(facing->name(), "facing");
    EXPECT_EQ(facing->valueCount(), 4u);

    const auto* nonexistent = container.getProperty("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

// ============================================================================
// BlockState 测试
// ============================================================================

TEST(BlockStateTest, GetProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    Axis axis = state.get(TestBlockWithAxis::AXIS());
    // 默认值应该是第一个值 (X)
    EXPECT_EQ(axis, Axis::X);
}

TEST(BlockStateTest, SetProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置新值
    const auto& newState = state.with(TestBlockWithAxis::AXIS(), Axis::Y);
    EXPECT_EQ(newState.get(TestBlockWithAxis::AXIS()), Axis::Y);

    // 设置另一个值
    const auto& state3 = state.with(TestBlockWithAxis::AXIS(), Axis::Z);
    EXPECT_EQ(state3.get(TestBlockWithAxis::AXIS()), Axis::Z);
}

TEST(BlockStateTest, SetPropertySameValue) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置相同的值应该返回同一个状态
    const auto& newState = state.with(TestBlockWithAxis::AXIS(), Axis::X);
    EXPECT_EQ(&state, &newState);
}

TEST(BlockStateTest, CycleProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 循环: X -> Y
    const auto& state1 = state.cycle(TestBlockWithAxis::AXIS());
    EXPECT_EQ(state1.get(TestBlockWithAxis::AXIS()), Axis::Y);

    // 循环: Y -> Z
    const auto& state2 = state1.cycle(TestBlockWithAxis::AXIS());
    EXPECT_EQ(state2.get(TestBlockWithAxis::AXIS()), Axis::Z);

    // 循环: Z -> X (回绕)
    const auto& state3 = state2.cycle(TestBlockWithAxis::AXIS());
    EXPECT_EQ(state3.get(TestBlockWithAxis::AXIS()), Axis::X);
}

TEST(BlockStateTest, HasProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    EXPECT_TRUE(state.hasProperty(TestBlockWithAxis::AXIS()));
    EXPECT_FALSE(state.hasProperty(BlockStateProperties::LIT()));
}

TEST(BlockStateTest, StateId) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& states = block.stateContainer().validStates();

    // 每个状态应该有不同的ID
    std::set<u32> ids;
    for (const auto& s : states) {
        ids.insert(s->stateId());
    }
    EXPECT_EQ(ids.size(), states.size());
}

TEST(BlockStateTest, ToString) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    String str = state.toString();
    // 应该包含属性名和值
    EXPECT_TRUE(str.find("axis") != String::npos);
}

TEST(BlockStateTest, MultiplePropertiesInteraction) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};
    const auto& state = block.defaultState();

    // 获取并设置多个属性
    Direction facing = state.get(TestBlockWithMultiple::FACING());
    bool lit = state.get(TestBlockWithMultiple::LIT());

    const auto& state1 = state.with(TestBlockWithMultiple::FACING(), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::FACING()), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::LIT()), lit);  // lit 应该不变

    const auto& state2 = state1.with(TestBlockWithMultiple::LIT(), true);
    EXPECT_EQ(state2.get(TestBlockWithMultiple::FACING()), Direction::East);  // facing 应该不变
    EXPECT_EQ(state2.get(TestBlockWithMultiple::LIT()), true);
}

// ============================================================================
// Block 测试
// ============================================================================

TEST(BlockTest, BasicProperties) {
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.5f).resistance(6.0f)};

    EXPECT_EQ(block.hardness(), 1.5f);
    EXPECT_EQ(block.resistance(), 6.0f);
    // Material 是副本，比较属性而非地址
    EXPECT_EQ(block.material().isSolid(), Material::ROCK.isSolid());
}

TEST(BlockTest, DefaultState) {
    TestBlock block{BlockProperties{Material::ROCK}};

    const auto& state = block.defaultState();
    EXPECT_EQ(&state.owner(), &block);
}

TEST(BlockTest, IsAir) {
    // 普通方块
    TestBlock normalBlock{BlockProperties{Material::ROCK}};
    EXPECT_FALSE(normalBlock.isAir(normalBlock.defaultState()));
}

TEST(BlockTest, StateCount) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    // 4 directions * 2 lit values = 8 states
    EXPECT_EQ(block.stateContainer().stateCount(), 8u);
}

// ============================================================================
// BlockRegistry 测试
// ============================================================================

TEST(BlockRegistryTest, RegisterBlock) {
    // 注意：由于 VanillaBlocks::initialize() 可能已运行，blockId 可能不为 1
    auto& block = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg1"),
        BlockProperties{Material::ROCK}
    );

    EXPECT_NE(&block, nullptr);
    EXPECT_EQ(block.blockLocation(), ResourceLocation("test:test_block_reg1"));
    EXPECT_GT(block.blockId(), 0u);  // ID should be > 0
}

TEST(BlockRegistryTest, GetBlockById) {
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg2"),
        BlockProperties{Material::ROCK}
    );

    Block* retrieved = BlockRegistry::instance().getBlock(registered.blockId());
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, GetBlockByLocation) {
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg3"),
        BlockProperties{Material::ROCK}
    );

    Block* retrieved = BlockRegistry::instance().getBlock(ResourceLocation("test:test_block_reg3"));
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, GetBlockState) {
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg"),
        BlockProperties{Material::WOOD}
    );

    // 获取默认状态
    const auto& defaultState = block.defaultState();
    BlockState* retrieved = BlockRegistry::instance().getBlockState(defaultState.stateId());
    EXPECT_EQ(retrieved, &defaultState);
}

TEST(BlockRegistryTest, ForEachBlock) {
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockA_reg"),
        BlockProperties{Material::ROCK}
    );
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockB_reg"),
        BlockProperties{Material::WOOD}
    );

    int count = 0;
    BlockRegistry::instance().forEachBlock([&count](Block&) {
        count++;
    });

    EXPECT_GT(count, 0);
}

TEST(BlockRegistryTest, ForEachBlockState) {
    BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg2"),
        BlockProperties{Material::WOOD}
    );

    int count = 0;
    BlockRegistry::instance().forEachBlockState([&count](const BlockState&) {
        count++;
    });

    // axis has 3 values, so at least 3 states
    EXPECT_GE(count, 3);
}

// ============================================================================
// VanillaBlocks 测试
// ============================================================================

TEST(VanillaBlocksTest, Initialization) {
    VanillaBlocks::initialize();

    // 检查基础方块
    EXPECT_NE(VanillaBlocks::AIR, nullptr);
    EXPECT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_NE(VanillaBlocks::GRASS_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::DIRT, nullptr);
    EXPECT_NE(VanillaBlocks::OAK_LOG, nullptr);

    // 检查空气方块
    EXPECT_TRUE(VanillaBlocks::AIR->isAir(VanillaBlocks::AIR->defaultState()));
    EXPECT_EQ(VanillaBlocks::AIR->blockId(), 0u);

    // 检查原木有轴属性
    const auto& logState = VanillaBlocks::OAK_LOG->defaultState();
    EXPECT_TRUE(logState.hasProperty(RotatedPillarBlock::AXIS()));
}

TEST(BlockStateTest, Caching) {
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithMultiple>(
        ResourceLocation("test:block_caching"),
        BlockProperties{Material::ROCK}
    );

    const auto& state1 = block.defaultState();
    const auto& state2 = state1.with(TestBlockWithMultiple::FACING(), Direction::East);
    const auto& state3 = state2.with(TestBlockWithMultiple::LIT(), true);

    // 设置相同值应该返回相同的状态
    const auto& state4 = state3.with(TestBlockWithMultiple::FACING(), Direction::East);
    EXPECT_EQ(&state3, &state4);

    // 从不同路径到达相同状态应该返回相同状态
    const auto& state5 = state1.with(TestBlockWithMultiple::LIT(), true)
                             .with(TestBlockWithMultiple::FACING(), Direction::East);
    EXPECT_EQ(&state3, &state5);
}
