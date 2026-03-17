#include "WaterFluid.hpp"
#include "../FluidRegistry.hpp"
#include "../../../util/property/FluidProperties.hpp"

namespace mc {
namespace fluid {

// ============================================================================
// WaterFluid 基类实现
// ============================================================================

const BlockState* WaterFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回对应的水方块状态
    // 方块LEVEL映射:
    // - 源头(level=8, isSource=true) -> 方块level=0
    // - 流动(level=1-7) -> 方块level=8-level
    // - 下落(level=8, falling=true) -> 方块level=8
    return nullptr;  // TODO: 实际实现
}

void WaterFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                       const BlockState* state) {
    // 水替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
}

i32 WaterSourceFluid::getLevelDecrease(IWorld& world) const {
    (void)world;
    return 1;
}

i32 WaterSourceFluid::getSpreadDistance(IWorld& world) const {
    (void)world;
    return 8;
}

// ============================================================================
// WaterSourceFluid 实现
// ============================================================================

WaterSourceFluid::WaterSourceFluid() {
    // 源头没有LEVEL属性，只有FALLING
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

const BlockState* WaterSourceFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回水源头方块状态
    (void)state;
    return nullptr;
}

void WaterSourceFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                             const BlockState* state) {
    // 水替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
}

FlowingFluid& WaterSourceFluid::getFlowing() {
    return *static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(
        ResourceLocation("minecraft:flowing_water")));
}

// ============================================================================
// WaterFlowingFluid 实现
// ============================================================================

WaterFlowingFluid::WaterFlowingFluid() {
    // 流动水有LEVEL_1_8和FALLING属性
    auto container = StateContainer<Fluid, FluidState>::Builder(*this)
        .add(FluidProperties::LEVEL_1_8())
        .add(FluidProperties::FALLING())
        .create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 WaterFlowingFluid::getLevel(const FluidState& state) const {
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : 8;
}

i32 WaterFlowingFluid::getLevelDecrease(IWorld& world) const {
    (void)world;
    return 1;
}

i32 WaterFlowingFluid::getSpreadDistance(IWorld& world) const {
    (void)world;
    return 8;
}

const BlockState* WaterFlowingFluid::getBlockState(const FluidState& state) const {
    // TODO: 返回对应的流动水方块状态
    (void)state;
    return nullptr;
}

void WaterFlowingFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                              const BlockState* state) {
    // 水替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
}

FlowingFluid& WaterFlowingFluid::getStill() {
    return *static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(
        ResourceLocation("minecraft:water")));
}

} // namespace fluid
} // namespace mc
