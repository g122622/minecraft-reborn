#pragma once

#include "../Feature.hpp"
#include "trunk/TrunkPlacer.hpp"
#include "foliage/FoliagePlacer.hpp"
#include "../../../block/Block.hpp"
#include <memory>

namespace mr {

/**
 * @brief 树木特征配置
 *
 * 参考: net.minecraft.world.gen.feature.BaseTreeFeatureConfig
 */
struct TreeFeatureConfig : public IFeatureConfig {
    /// 树干方块ID
    BlockId trunkBlock = BlockId::OakLog;

    /// 树叶方块ID
    BlockId foliageBlock = BlockId::OakLeaves;

    /// 树干放置器
    std::unique_ptr<TrunkPlacer> trunkPlacer;

    /// 树叶放置器
    std::unique_ptr<FoliagePlacer> foliagePlacer;

    /// 最大水深（树木不能生成在深水中）
    i32 maxWaterDepth = 0;

    /// 是否忽略藤蔓
    bool ignoreVines = false;

    /// 强制放置（跳过高度检查）
    bool forcePlacement = false;

    /// 最小高度
    i32 minHeight = 4;

    TreeFeatureConfig() = default;

    TreeFeatureConfig(
        BlockId trunk,
        BlockId foliage,
        std::unique_ptr<TrunkPlacer> trunkPlacer_,
        std::unique_ptr<FoliagePlacer> foliagePlacer_
    ) : trunkBlock(trunk)
      , foliageBlock(foliage)
      , trunkPlacer(std::move(trunkPlacer_))
      , foliagePlacer(std::move(foliagePlacer_))
    {}
};

/**
 * @brief 树木特征
 *
 * 生成树木的主要特征类。
 *
 * 参考: net.minecraft.world.gen.feature.TreeFeature
 */
class TreeFeature {
public:
    /**
     * @brief 默认构造函数
     */
    TreeFeature() = default;

    /**
     * @brief 放置树木
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param startPos 起始位置
     * @param config 树木配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& startPos,
        const TreeFeatureConfig& config
    );

    /**
     * @brief 检查位置是否可以放置树干
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否可以放置
     */
    [[nodiscard]] static bool isReplaceableAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是空气或树叶
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是空气或树叶
     */
    [[nodiscard]] static bool isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是泥土或耕地
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是泥土或耕地
     */
    [[nodiscard]] static bool isDirtOrFarmlandAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是水
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是水
     */
    [[nodiscard]] static bool isWaterAt(WorldGenRegion& world, const BlockPos& pos);

private:
    /**
     * @brief 计算可用的树干高度
     *
     * 从起始位置向上检查，找到可以放置树干的最大高度。
     *
     * @param world 世界区域
     * @param maxHeight 最大高度
     * @param startPos 起始位置
     * @param config 树木配置
     * @return 可用高度
     */
    [[nodiscard]] i32 calculateAvailableHeight(
        WorldGenRegion& world,
        i32 maxHeight,
        const BlockPos& startPos,
        const TreeFeatureConfig& config
    ) const;

    /**
     * @brief 设置树叶距离属性
     *
     * 遍历树叶方块，设置其到最近树干的距离。
     * 这个距离用于树叶腐烂机制。
     *
     * @param world 世界区域
     * @param trunkBlocks 树干方块集合
     * @param foliageBlocks 树叶方块集合
     */
    void setFoliageDistance(
        WorldGenRegion& world,
        const std::set<BlockPos>& trunkBlocks,
        const std::set<BlockPos>& foliageBlocks
    );
};

/**
 * @brief 预定义的树木配置
 */
struct TreeFeatures {
    /// 橡树配置
    static TreeFeatureConfig oak();

    /// 白桦配置
    static TreeFeatureConfig birch();

    /// 云杉配置
    static TreeFeatureConfig spruce();

    /// 丛林木配置
    static TreeFeatureConfig jungle();
};

} // namespace mr
