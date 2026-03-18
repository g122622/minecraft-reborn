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
    (void)state;
    return nullptr;
}

void WaterFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                       const BlockState* state) {
    // 水替换方块前无特殊处理
    (void)world;
    (void)pos;
    (void)state;
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

FlowingFluid& WaterSourceFluid::getFlowing() {
    if (m_flowingCache == nullptr) {
        m_flowingCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_water")));
    }
    return *m_flowingCache;
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

FlowingFluid& WaterFlowingFluid::getStill() {
    if (m_stillCache == nullptr) {
        m_stillCache = static_cast<FlowingFluid*>(
            FluidRegistry::instance().getFluid(ResourceLocation("minecraft:water")));
    }
    return *m_stillCache;
}

} // namespace fluid
} // namespace mc
