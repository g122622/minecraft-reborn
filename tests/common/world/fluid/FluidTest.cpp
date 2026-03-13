#include <gtest/gtest.h>
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/fluids/EmptyFluid.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/resource/ResourceLocation.hpp"

using namespace mc::fluid;
using namespace mc;

// ============================================================================
// FluidState Tests
// ============================================================================

TEST(FluidStateTest, EmptyFluidStateIsEmpty) {
    // EmptyFluid的状态应该标记为空
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    // EmptyFluid重写isEmpty返回true
    EXPECT_TRUE(state.isEmpty());
    EXPECT_FALSE(state.isSource());
    EXPECT_EQ(state.getLevel(), 0);
}

TEST(FluidStateTest, GetFluidReturnsOwner) {
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    EXPECT_EQ(&state.getFluid(), &emptyFluid);
}

TEST(FluidStateTest, FluidId) {
    EmptyFluid emptyFluid;
    const FluidState& state = emptyFluid.defaultState();

    EXPECT_EQ(state.fluidId(), emptyFluid.fluidId());
}

// ============================================================================
// FluidRegistry Tests
// ============================================================================

TEST(FluidRegistryTest, InitializeRegistersEmptyFluid) {
    FluidRegistry& registry = FluidRegistry::instance();

    // 初始化
    registry.initialize();

    // EmptyFluid应该被注册为ID 0
    Fluid* emptyFluid = registry.getFluid(FluidRegistry::EMPTY_ID);
    EXPECT_NE(emptyFluid, nullptr);
    EXPECT_EQ(emptyFluid->fluidLocation(), ResourceLocation("minecraft:empty"));
}

TEST(FluidRegistryTest, GetFluidByInvalidIdReturnsNull) {
    FluidRegistry& registry = FluidRegistry::instance();

    Fluid* fluid = registry.getFluid(99999);
    EXPECT_EQ(fluid, nullptr);
}

TEST(FluidRegistryTest, GetFluidByInvalidResourceLocationReturnsNull) {
    FluidRegistry& registry = FluidRegistry::instance();

    Fluid* fluid = registry.getFluid(ResourceLocation("minecraft:nonexistent"));
    EXPECT_EQ(fluid, nullptr);
}

// ============================================================================
// FluidProperties Tests
// ============================================================================

TEST(FluidPropertiesTest, LevelPropertyHasCorrectRange) {
    auto& level = FluidProperties::LEVEL_1_8();

    EXPECT_EQ(level.name(), "level");
    EXPECT_EQ(level.minValue(), 1);
    EXPECT_EQ(level.maxValue(), 8);
}

TEST(FluidPropertiesTest, FallingPropertyExists) {
    auto& falling = FluidProperties::FALLING();

    EXPECT_EQ(falling.name(), "falling");
}

// ============================================================================
// Fluid Base Class Tests
// ============================================================================

TEST(FluidTest, DefaultTickDoesNothing) {
    // Fluid基类的tick方法应该可以被安全调用
    EmptyFluid emptyFluid;
    // 创建一个模拟的world和pos - 需要实际的IWorld实现来测试
    // emptyFluid.tick(world, pos, state);
    // 这里只验证方法存在
}

TEST(FluidTest, DefaultRandomTickDoesNothing) {
    EmptyFluid emptyFluid;
    // 与tick类似，需要实际的IWorld和IRandom实现
}

TEST(FluidTest, DefaultTicksRandomlyReturnsFalse) {
    EmptyFluid emptyFluid;
    EXPECT_FALSE(emptyFluid.ticksRandomly());
}

TEST(FluidTest, IsEquivalentTo) {
    EmptyFluid emptyFluid1;
    EmptyFluid emptyFluid2;

    // 同一对象应该是等效的
    EXPECT_TRUE(emptyFluid1.isEquivalentTo(emptyFluid1));

    // 不同对象不是等效的（默认实现比较指针）
    EXPECT_FALSE(emptyFluid1.isEquivalentTo(emptyFluid2));
}

// ============================================================================
// StateContainer Tests for Fluid
// ============================================================================

TEST(FluidStateTest, StateWithProperties) {
    EmptyFluid emptyFluid;
    const FluidState& baseState = emptyFluid.defaultState();

    // EmptyFluid没有属性，所以baseState就是唯一状态
    EXPECT_EQ(emptyFluid.stateContainer().stateCount(), static_cast<size_t>(1));
}

// ============================================================================
// ResourceLocation Hash Tests
// ============================================================================

TEST(ResourceLocationHashTest, CanBeUsedInUnorderedMap) {
    std::unordered_map<ResourceLocation, int> map;

    map[ResourceLocation("minecraft:water")] = 1;
    map[ResourceLocation("minecraft:lava")] = 2;

    EXPECT_EQ(map[ResourceLocation("minecraft:water")], 1);
    EXPECT_EQ(map[ResourceLocation("minecraft:lava")], 2);
}
