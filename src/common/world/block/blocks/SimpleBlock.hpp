#pragma once

#include "../Block.hpp"

namespace mc {

/**
 * @brief 简单方块基类
 *
 * 没有状态属性的简单方块。
 * 大多数基础方块（如石头、泥土等）都继承自此类。
 *
 * 参考: net.minecraft.block.Block (无属性的简单情况)
 */
class SimpleBlock : public Block {
public:
    /**
     * @brief 构造简单方块
     */
    explicit SimpleBlock(BlockProperties properties);

    /**
     * @brief 是否为固体
     */
    [[nodiscard]] bool isSolid(const BlockState& state) const override;
};

} // namespace mc
