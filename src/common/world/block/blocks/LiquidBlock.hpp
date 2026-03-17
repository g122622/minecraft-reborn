#pragma once

#include "../Block.hpp"
#include "../../fluid/FlowingFluid.hpp"

namespace mc {
namespace block {

/**
 * @brief 液体方块
 *
 * 与流体关联的方块，用于在世界中表示流体。
 *
 * 参考: net.minecraft.block.LiquidBlock
 *
 * 方块LEVEL (0-15) 与流体LEVEL (1-8) 的映射：
 * - 方块level=0 -> 流体level=8（源头）
 * - 方块level=1-7 -> 流体level=1-7
 * - 方块level=8-15 -> 流体level=8 + falling=true（下落）
 */
class LiquidBlock : public Block {
public:
    /**
     * @brief 构造液体方块
     *
     * @param fluid 关联的流动流体
     * @param properties 方块属性
     */
    LiquidBlock(fluid::FlowingFluid& fluid, BlockProperties properties);

    // ========== Block接口重写 ==========

    /**
     * @brief 获取流体状态
     *
     * 根据方块LEVEL属性计算对应的流体状态。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] const mc::fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 是否为空气
     *
     * 液体方块不是空气。
     */
    [[nodiscard]] bool isAir(const BlockState& state) const override {
        (void)state;
        return false;
    }

    /**
     * @brief 获取碰撞形状
     *
     * 液体方块没有碰撞形状。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取关联的流体
     */
    [[nodiscard]] fluid::FlowingFluid& getFluid() const { return m_fluid; }

    // ========== 方块与流体等级转换 ==========

    /**
     * @brief 方块等级转流体等级
     *
     * 方块level (0-15) -> 流体level (1-8)
     * - level=0 -> 流体level=8（源头）
     * - level=1-7 -> 流体level=1-7
     * - level=8-15 -> 流体level=8, falling=true
     *
     * @param blockLevel 方块等级 (0-15)
     * @return 流体等级 (1-8)
     */
    [[nodiscard]] static i32 blockLevelToFluidLevel(i32 blockLevel);

    /**
     * @brief 流体等级转方块等级
     *
     * @param fluidLevel 流体等级 (1-8)
     * @param falling 是否下落
     * @return 方块等级 (0-15)
     */
    [[nodiscard]] static i32 fluidLevelToBlockLevel(i32 fluidLevel, bool falling);

    /**
     * @brief 检查方块等级是否表示源头
     */
    [[nodiscard]] static bool isSourceLevel(i32 blockLevel) {
        return blockLevel == 0;
    }

    /**
     * @brief 检查方块等级是否表示下落
     */
    [[nodiscard]] static bool isFallingLevel(i32 blockLevel) {
        return blockLevel >= 8;
    }

private:
    fluid::FlowingFluid& m_fluid;

    // 缓存的流体状态（方块level -> 流体状态）
    // 存储FluidState对象而非指针，避免悬垂引用
    mutable std::vector<fluid::FluidState> m_fluidStateCache;

    /**
     * @brief 构建流体状态缓存
     */
    void buildFluidStateCache();
};

} // namespace block
} // namespace mc
