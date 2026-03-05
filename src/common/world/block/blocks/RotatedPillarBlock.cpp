#include "RotatedPillarBlock.hpp"

namespace mr {

namespace {
    // 静态属性实例
    std::unique_ptr<EnumProperty<Axis>> g_axisProperty;
}

const EnumProperty<Axis>& RotatedPillarBlock::AXIS() {
    if (!g_axisProperty) {
        g_axisProperty = EnumProperty<Axis>::createAll("axis");
    }
    return *g_axisProperty;
}

RotatedPillarBlock::RotatedPillarBlock(BlockProperties properties)
    : Block(properties) {
    // 创建带有axis属性的状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(AXIS())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

Axis RotatedPillarBlock::getAxis(const BlockState& state) const {
    return state.get(AXIS());
}

const BlockState& RotatedPillarBlock::withAxis(const BlockState& state, Axis axis) const {
    return state.with(AXIS(), axis);
}

} // namespace mr
