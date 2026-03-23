#pragma once

#include "TrunkPlacer.hpp"
#include <memory>

namespace mc {

/**
 * @brief 深色橡树树干放置器
 *
 * 生成 2x2 的深色橡树树干。
 * 参考 MC 1.16.5: DarkOakTrunkPlacer
 */
class DarkOakTrunkPlacer : public TrunkPlacer {
public:
    /**
     * @brief 构造函数
     * @param baseHeight 基础高度
     * @param heightRandA 高度随机值A
     * @param heightRandB 高度随机值B
     */
    DarkOakTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) override;

    [[nodiscard]] const char* name() const override { return "dark_oak"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;
};

/**
 * @brief 精美树干放置器
 *
 * 生成弯曲的树干，用于精美橡树。
 * 参考 MC 1.16.5: FancyTrunkPlacer
 */
class FancyTrunkPlacer : public TrunkPlacer {
public:
    FancyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) override;

    [[nodiscard]] const char* name() const override { return "fancy"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;
};

/**
 * @brief 分叉树干放置器
 *
 * 生成带有分叉的树干，用于金合欢树。
 * 参考 MC 1.16.5: ForkyTrunkPlacer
 */
class ForkyTrunkPlacer : public TrunkPlacer {
public:
    ForkyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) override;

    [[nodiscard]] const char* name() const override { return "forky"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;

private:
    /**
     * @brief 生成分叉
     * @return 分叉末端位置
     */
    BlockPos generateBranch(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& startPos,
        i32 length,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    );
};

/**
 * @brief 巨型树干放置器
 *
 * 生成 2x2 的巨型树干，用于巨型云杉。
 * 参考 MC 1.16.5: GiantTrunkPlacer
 */
class GiantTrunkPlacer : public TrunkPlacer {
public:
    GiantTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) override;

    [[nodiscard]] const char* name() const override { return "giant"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;
};

/**
 * @brief 巨型丛林木树干放置器
 *
 * 生成 2x2 的丛林木树干，并在树干上生成藤蔓。
 * 参考 MC 1.16.5: MegaJungleTrunkPlacer
 */
class MegaJungleTrunkPlacer : public TrunkPlacer {
public:
    MegaJungleTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock
    ) override;

    [[nodiscard]] const char* name() const override { return "mega_jungle"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;
};

} // namespace mc
