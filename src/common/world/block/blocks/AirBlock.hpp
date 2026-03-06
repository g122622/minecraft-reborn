#pragma once

#include "../Block.hpp"

namespace mr {

/**
 * @brief 空气方块
 *
 * 无碰撞、非固体、非不透明的方块。
 * 参考: net.minecraft.block.AirBlock
 */
class AirBlock : public Block {
public:
    /**
     * @brief 构造空气方块
     */
    explicit AirBlock(BlockProperties properties);

    /**
     * @brief 获取渲染形状（空）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（空）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 是否为空气（始终返回true）
     */
    [[nodiscard]] bool isAir(const BlockState& state) const override;

    /**
     * @brief 是否为固体（始终返回false）
     */
    [[nodiscard]] bool isSolid(const BlockState& state) const override;

    /**
     * @brief 是否不透明（始终返回false）
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override;
};

} // namespace mr
