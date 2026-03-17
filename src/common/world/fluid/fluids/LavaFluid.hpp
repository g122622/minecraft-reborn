#pragma once

#include "../FlowingFluid.hpp"
#include "../../../math/Vector3.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 岩浆流体基类
 *
 * 参考MC 1.16.5 LavaFluid
 *
 * 特性：
 * - tick延迟: 主世界30tick，下界10tick
 * - 每格衰减: 主世界2级，下界1级
 * - 最大距离: 主世界4格，下界6格
 * - 不能形成无限源
 * - 随机tick可能引燃周围方块
 */
class LavaFluid : public FlowingFluid {
public:
    [[nodiscard]] i32 getTickDelay() const override {
        // TODO: 根据维度返回不同值
        // 主世界: 30 tick
        // 下界: 10 tick
        return 30;
    }

    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override {
        // TODO: 根据维度返回不同值
        // 主世界: 2
        // 下界: 1
        (void)world;
        return 2;
    }

    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override {
        // TODO: 根据维度返回不同值
        // 主世界: 4
        // 下界: 6
        (void)world;
        return 4;
    }

    [[nodiscard]] bool canSourcesMultiply() const override { return false; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }

    void randomTick(IWorld& world, const BlockPos& pos,
                    const FluidState& state, IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                              const BlockState* state) override;

    /**
     * @brief 检查岩浆是否会变成石头或黑曜石
     *
     * 当岩浆遇到水时：
     * - 岩浆源头 + 水 = 黑曜石
     * - 流动岩浆 + 水 = 石头（圆石）
     *
     * @param world 世界
     * @param pos 位置
     * @param direction 接触方向
     * @return 是否发生了转换
     */
    bool checkForMixing(IWorld& world, const BlockPos& pos, Direction direction);
};

/**
 * @brief 岩浆源头
 *
 * 源头岩浆方块，level=8，没有LEVEL属性。
 */
class LavaSourceFluid : public FlowingFluid {
public:
    LavaSourceFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override {
        (void)state;
        return true;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override {
        (void)state;
        return 8;
    }

    [[nodiscard]] i32 getTickDelay() const override { return 30; }
    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override;
    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override;
    [[nodiscard]] bool canSourcesMultiply() const override { return false; }
    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;
    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }
    [[nodiscard]] FlowingFluid& getFlowing() override;
    [[nodiscard]] FlowingFluid& getStill() override { return *this; }

    void randomTick(IWorld& world, const BlockPos& pos,
                    const FluidState& state, IRandom& random) override;
    [[nodiscard]] bool ticksRandomly() const override { return true; }

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                              const BlockState* state) override;
};

/**
 * @brief 流动岩浆
 *
 * 流动岩浆方块，有LEVEL属性(1-8)和FALLING属性。
 */
class LavaFlowingFluid : public FlowingFluid {
public:
    LavaFlowingFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override {
        (void)state;
        return false;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override;

    [[nodiscard]] i32 getTickDelay() const override { return 30; }
    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override;
    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override;
    [[nodiscard]] bool canSourcesMultiply() const override { return false; }
    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;
    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }
    [[nodiscard]] FlowingFluid& getFlowing() override { return *this; }
    [[nodiscard]] FlowingFluid& getStill() override;

    void randomTick(IWorld& world, const BlockPos& pos,
                    const FluidState& state, IRandom& random) override;
    [[nodiscard]] bool ticksRandomly() const override { return true; }

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                              const BlockState* state) override;
};

} // namespace fluid
} // namespace mc
