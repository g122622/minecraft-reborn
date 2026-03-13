#include "LiquidBlock.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../util/property/FluidProperties.hpp"

namespace mc {
namespace block {

// ============================================================================
// LiquidBlock 实现
// ============================================================================

// 方块LEVEL属性 (0-15)
namespace {
    const IntegerProperty& LEVEL_0_15() {
        return BlockStateProperties::LEVEL_0_15();
    }
}

LiquidBlock::LiquidBlock(fluid::FlowingFluid& fluid, BlockProperties properties)
    : Block(properties)
    , m_fluid(fluid) {

    // 创建方块状态容器，包含LEVEL属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(LEVEL_0_15())
        .create([this](const Block& block, auto values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态（level=0，即源头）
    setDefaultState(defaultState().with(LEVEL_0_15(), 0));

    // 构建流体状态缓存
    buildFluidStateCache();
}

const mc::fluid::FluidState* LiquidBlock::getFluidState(const BlockState& state) const {
    i32 blockLevel = state.get(LEVEL_0_15());
    return m_fluidStateCache[blockLevel];
}

const CollisionShape& LiquidBlock::getCollisionShape(const BlockState& state) const {
    // 液体方块没有碰撞形状
    return CollisionShape::empty();
}

i32 LiquidBlock::blockLevelToFluidLevel(i32 blockLevel) {
    // 方块level=0 -> 流体level=8（源头）
    // 方块level=1-7 -> 流体level=8-blockLevel（反向）
    // 方块level=8-15 -> 流体level=8（下落）
    if (blockLevel == 0) {
        return 8;  // 源头
    } else if (blockLevel >= 1 && blockLevel <= 7) {
        return 8 - blockLevel;  // 1->7, 2->6, ..., 7->1
    } else {
        return 8;  // 下落，level>=8
    }
}

i32 LiquidBlock::fluidLevelToBlockLevel(i32 fluidLevel, bool falling) {
    // 流体level=8, falling=false -> 方块level=0（源头）
    // 流体level=1-7 -> 方块level=8-fluidLevel
    // 流体level=8, falling=true -> 方块level=8（下落）
    if (falling) {
        return 8;
    } else if (fluidLevel == 8) {
        return 0;  // 源头
    } else {
        return 8 - fluidLevel;
    }
}

void LiquidBlock::buildFluidStateCache() {
    m_fluidStateCache.resize(16);

    auto& levelProp = fluid::FluidProperties::LEVEL_1_8();
    auto& fallingProp = fluid::FluidProperties::FALLING();

    for (i32 blockLevel = 0; blockLevel <= 15; ++blockLevel) {
        i32 fluidLevel = blockLevelToFluidLevel(blockLevel);
        bool falling = isFallingLevel(blockLevel);

        const fluid::FluidState& baseState = m_fluid.getFlowing().defaultState();

        if (fluidLevel == 8 && !falling) {
            // 源头状态
            m_fluidStateCache[blockLevel] = &m_fluid.getStill().defaultState();
        } else {
            // 流动状态
            const fluid::FluidState* state = &baseState.with(levelProp, fluidLevel).with(fallingProp, falling);
            m_fluidStateCache[blockLevel] = state;
        }
    }
}

} // namespace block
} // namespace mc
