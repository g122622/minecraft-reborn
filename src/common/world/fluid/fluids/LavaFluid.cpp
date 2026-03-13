#include "LavaFluid.hpp"
#include "../FluidRegistry.hpp"
#include "../../../util/property/FluidProperties.hpp"
#include <cmath>

namespace mc {
namespace fluid {

// ============================================================================
// LavaFluid 基类实现
// ============================================================================

const BlockState* LavaFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回对应的岩浆方块状态
    (void)state;
    return nullptr;
}

void LavaFluid::randomTick(IWorld& world, const BlockPos& pos,
                            const FluidState& state, IRandom& random) {
    // 岩浆的随机tick可能引燃周围方块
    // TODO: 实现引燃逻辑

    // 1%概率在周围生成火
    // if (random.nextInt(100) == 0) {
    //     // 检查周围是否有可燃物
    //     // 如果有，放置火方块
    // }
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

void LavaFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                      const BlockState* state) {
    // 岩浆替换方块前无特殊处理
    // 但如果替换的是可燃物，可能会引燃
    (void)world;
    (void)pos;
    (void)state;
}

bool LavaFluid::checkForMixing(IWorld& world, const BlockPos& pos, Direction direction) {
    // TODO: 检查岩浆是否遇到水
    // 如果是，生成石头或黑曜石
    (void)world;
    (void)pos;
    (void)direction;
    return false;
}

i32 LavaSourceFluid::getLevelDecrease(IWorld& world) const {
    (void)world;
    return 2;
}

i32 LavaSourceFluid::getSpreadDistance(IWorld& world) const {
    (void)world;
    return 4;
}

// ============================================================================
// LavaSourceFluid 实现
// ============================================================================

LavaSourceFluid::LavaSourceFluid() {
    // 源头没有LEVEL属性，只有FALLING
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

const BlockState* LavaSourceFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回岩浆源头方块状态
    (void)state;
    return nullptr;
}

void LavaSourceFluid::randomTick(IWorld& world, const BlockPos& pos,
                                  const FluidState& state, IRandom& random) {
    // 岩浆的随机tick可能引燃周围方块
    // TODO: 实现引燃逻辑
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

void LavaSourceFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                            const BlockState* state) {
    // 岩浆替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
}

FlowingFluid& LavaSourceFluid::getFlowing() {
    return *static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(
        ResourceLocation("minecraft:flowing_lava")));
}

// ============================================================================
// LavaFlowingFluid 实现
// ============================================================================

LavaFlowingFluid::LavaFlowingFluid() {
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::LEVEL_1_8())
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 LavaFlowingFluid::getLevel(const FluidState& state) const {
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : 8;
}

i32 LavaFlowingFluid::getLevelDecrease(IWorld& world) const {
    (void)world;
    return 2;
}

i32 LavaFlowingFluid::getSpreadDistance(IWorld& world) const {
    (void)world;
    return 4;
}

const BlockState* LavaFlowingFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回对应的流动岩浆方块状态
    (void)state;
    return nullptr;
}

void LavaFlowingFluid::randomTick(IWorld& world, const BlockPos& pos,
                                   const FluidState& state, IRandom& random) {
    // 岩浆的随机tick可能引燃周围方块
    // TODO: 实现引燃逻辑
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

void LavaFlowingFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                             const BlockState* state) {
    // 岩浆替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
}

FlowingFluid& LavaFlowingFluid::getStill() {
    return *static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(
        ResourceLocation("minecraft:lava")));
}

} // namespace fluid
} // namespace mc
