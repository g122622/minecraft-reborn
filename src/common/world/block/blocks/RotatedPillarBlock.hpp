#pragma once

#include "../Block.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/DirectionProperty.hpp"

namespace mr {

// Forward declarations
class BlockState;

/**
 * @brief 旋转柱状方块基类
 *
 * 用于原木、柱状玄武岩等可绕Y轴旋转的方块。
 * 拥有 axis 属性（X、Y、Z）。
 *
 * 参考: net.minecraft.block.RotatedPillarBlock
 */
class RotatedPillarBlock : public Block {
public:
    /**
     * @brief 获取AXIS属性
     */
    static const EnumProperty<Axis>& AXIS();

    /**
     * @brief 构造函数
     */
    RotatedPillarBlock(BlockProperties properties);

    /**
     * @brief 获取方块的轴
     */
    Axis getAxis(const BlockState& state) const;

    /**
     * @brief 设置方块的轴
     * @return 新状态
     */
    const BlockState& withAxis(const BlockState& state, Axis axis) const;
};

} // namespace mr
