#pragma once

#include "Fluid.hpp"
#include "../../util/property/FluidProperties.hpp"
#include "../../util/Direction.hpp"
#include <unordered_map>

namespace mc {
namespace fluid {

/**
 * @brief 流动流体抽象类
 *
 * 实现流体流动的核心算法，包括向下流动、水平扩散、源头形成等。
 * 水和岩浆都继承此类。
 *
 * 参考: net.minecraft.fluid.FlowingFluid
 *
 * 流动规则：
 * 1. 优先向下流动
 * 2. 水平扩散，距离由getSpreadDistance()决定
 * 3. 水可以形成无限源（2+相邻源头）
 * 4. 岩浆不能形成无限源
 */
class FlowingFluid : public Fluid {
public:
    // ========== 属性 ==========

    /**
     * @brief 获取流动流体实例
     *
     * 返回对应的流动版本（非源头）。
     */
    [[nodiscard]] virtual FlowingFluid& getFlowing() = 0;

    /**
     * @brief 获取源头流体实例
     */
    [[nodiscard]] virtual FlowingFluid& getStill() = 0;

    /**
     * @brief 每格高度衰减
     *
     * 水: 1
     * 岩浆(主世界): 2
     * 岩浆(下界): 1
     */
    [[nodiscard]] virtual i32 getLevelDecrease(IWorld& world) const = 0;

    /**
     * @brief 最大流动距离
     *
     * 水: 8格
     * 岩浆(主世界): 4格
     * 岩浆(下界): 6格
     */
    [[nodiscard]] virtual i32 getSpreadDistance(IWorld& world) const = 0;

    /**
     * @brief 获取流动状态
     *
     * @param level 等级 (1-8)
     * @param falling 是否下落
     * @return 流体状态
     */
    [[nodiscard]] FluidState getFlowingState(i32 level, bool falling);

    /**
     * @brief 获取源头状态
     *
     * @param falling 是否下落（水源头通常为false）
     * @return 源头流体状态
     */
    [[nodiscard]] FluidState getStillState(bool falling);

    // ========== Fluid接口实现 ==========

    void tick(IWorld& world, const BlockPos& pos, FluidState& state) override;

    [[nodiscard]] Vector3 getFlow(IBlockReader& world, const BlockPos& pos,
                                   const FluidState& state) const override;

    [[nodiscard]] CollisionShape getShape(const FluidState& state, IBlockReader& world,
                                           const BlockPos& pos) const override;

protected:
    /**
     * @brief 替换方块前的处理
     *
     * 当流体替换方块时调用。岩浆会在此处理与水的交互。
     *
     * @param world 世界
     * @param pos 位置
     * @param state 被替换的方块状态
     */
    virtual void beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                                       const BlockState* state) = 0;

    /**
     * @brief 执行流动扩散
     *
     * 计算流体应该流向哪些方向，并执行流动。
     *
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前流体状态
     */
    void flowAround(IWorld& world, const BlockPos& pos, const FluidState& state);

    /**
     * @brief 尝试向指定方向流动
     *
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前流体状态
     * @param dir 流动方向
     */
    void tryFlow(IWorld& world, const BlockPos& pos, const FluidState& state, Direction dir);

    /**
     * @brief 流入指定位置
     *
     * @param world 世界
     * @param pos 目标位置
     * @param blockState 目标位置的方块状态
     * @param dir 流入方向
     * @param state 流入的流体状态
     */
    void flowInto(IWorld& world, const BlockPos& pos, const BlockState* blockState,
                  Direction dir, const FluidState& state);

    /**
     * @brief 计算正确的流动状态
     *
     * 根据周围流体计算当前位置应该有的流体状态。
     * 包括源头形成检测和下落流体检测。
     *
     * @param world 世界
     * @param pos 位置
     * @param blockState 当前方块状态
     * @return 正确的流体状态，如果应该消失则返回空
     */
    [[nodiscard]] FluidState calculateCorrectFlowingState(IWorld& world,
                                                           const BlockPos& pos,
                                                           const BlockState* blockState);

    /**
     * @brief 检查是否可以流向指定位置
     *
     * @param world 世界
     * @param fromPos 源位置
     * @param fromBlock 源方块状态
     * @param dir 流动方向
     * @param toPos 目标位置
     * @param toBlock 目标方块状态
     * @param toFluid 目标流体状态
     * @param fluid 流入的流体类型
     * @return 是否可以流动
     */
    [[nodiscard]] bool canFlow(IBlockReader& world, const BlockPos& fromPos,
                                const BlockState* fromBlock, Direction dir,
                                const BlockPos& toPos, const BlockState* toBlock,
                                const FluidState& toFluid, const Fluid& fluid) const;

    /**
     * @brief 检查侧面是否有孔洞
     *
     * 判断两个方块之间是否可以通过流体。
     *
     * @param dir 方向
     * @param world 世界
     * @param pos 当前位置
     * @param state 当前方块状态
     * @param neighborPos 相邻位置
     * @param neighborState 相邻方块状态
     * @return 是否有孔洞
     */
    [[nodiscard]] bool doesSideHaveHoles(Direction dir, IBlockReader& world,
                                          const BlockPos& pos, const BlockState* state,
                                          const BlockPos& neighborPos,
                                          const BlockState* neighborState) const;

    /**
     * @brief 检查是否可以形成源头
     *
     * 水: 需要2+相邻源头且下方为固体或同种流体
     * 岩浆: 永远返回false
     *
     * @param world 世界
     * @param pos 位置
     * @return 是否可以形成源头
     */
    [[nodiscard]] bool canFormSource(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取水平相邻源头数量
     *
     * @param world 世界
     * @param pos 位置
     * @return 相邻源头数量
     */
    [[nodiscard]] i32 getHorizontalSourceCount(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查方块是否可以被流体替换
     *
     * @param block 方块状态
     * @param world 世界
     * @param pos 位置
     * @param fluid 流体类型
     * @return 是否可替换
     */
    [[nodiscard]] bool isBlocked(IBlockReader& world, const BlockPos& pos,
                                  const BlockState* block, const Fluid& fluid) const;

    /**
     * @brief 检查是否为相同流体或空
     *
     * @param state 流体状态
     * @return 是否为相同流体或空
     */
    [[nodiscard]] bool isSameOrEmpty(const FluidState& state) const;

    /**
     * @brief 检查是否为同种流体源头
     *
     * @param state 流体状态
     * @return 是否为同种流体源头
     */
    [[nodiscard]] bool isSameSource(const FluidState& state) const;

    /**
     * @brief 获取相邻流动方向和状态
     *
     * 计算流体应该流向哪些方向，按优先级排序。
     *
     * @param world 世界
     * @param pos 位置
     * @param blockState 方块状态
     * @return 方向 -> 流体状态的映射
     */
    [[nodiscard]] std::unordered_map<Direction, FluidState> getFlowDirections(
        IWorld& world, const BlockPos& pos, const BlockState* blockState);
};

} // namespace fluid
} // namespace mc
