#pragma once

#include "../Feature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../placement/Placement.hpp"
#include <memory>

namespace mr {

// 前向声明
class WorldGenRegion;

/**
 * @brief 矿石特征
 *
 * 参考 MC OreFeature，生成矿脉形状的矿石。
 * 使用球形采样算法在石头中放置矿石。
 */
class OreFeature {
public:
    /**
     * @brief 在指定位置放置矿石
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param random 随机数生成器
     * @param origin 起始位置
     * @param config 矿石配置
     * @return 是否成功放置了任何方块
     */
    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        Random& random,
        const BlockPos& origin,
        const OreFeatureConfig& config);

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] static const char* name() { return "ore"; }

private:
    /**
     * @brief 在指定范围内生成球形矿石
     * @param chunk 区块数据
     * @param random 随机数生成器
     * @param config 矿石配置
     * @param x1 起点X
     * @param y1 起点Y
     * @param z1 起点Z
     * @param x2 终点X
     * @param y2 终点Y
     * @param z2 终点Z
     * @param minX 边界最小X
     * @param minY 边界最小Y
     * @param minZ 边界最小Z
     * @param sizeX 范围大小X
     * @param sizeY 范围大小Y
     * @param sizeZ 范围大小Z
     * @param placedCount 已放置计数（输出）
     */
    void generateSphere(
        ChunkPrimer& chunk,
        Random& random,
        const OreFeatureConfig& config,
        f64 x1, f64 y1, f64 z1,
        f64 x2, f64 y2, f64 z2,
        i32 minX, i32 minY, i32 minZ,
        i32 sizeX, i32 sizeY, i32 sizeZ,
        i32& placedCount);
};

/**
 * @brief 预配置的矿石特征
 *
 * 组合矿石特征和配置，方便注册和使用。
 */
class ConfiguredOreFeature {
public:
    ConfiguredOreFeature(
        std::unique_ptr<OreFeatureConfig> featureConfig,
        std::unique_ptr<ConfiguredPlacement> placement);

    /**
     * @brief 在区块中生成矿石
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param random 随机数生成器
     */
    void generate(WorldGenRegion& region, ChunkPrimer& chunk, Random& random);

    /**
     * @brief 获取矿石配置
     */
    [[nodiscard]] const OreFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<OreFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
};

/**
 * @brief 矿石注册表
 *
 * 存储所有预配置的矿石特征。
 */
class OreFeatures {
public:
    /**
     * @brief 初始化所有矿石特征
     */
    static void initialize();

    /**
     * @brief 获取所有矿石特征
     */
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredOreFeature>>& getAllFeatures();

    // 主世界矿石
    static std::unique_ptr<ConfiguredOreFeature> createCoalOre();
    static std::unique_ptr<ConfiguredOreFeature> createIronOre();
    static std::unique_ptr<ConfiguredOreFeature> createGoldOre();
    static std::unique_ptr<ConfiguredOreFeature> createRedstoneOre();
    static std::unique_ptr<ConfiguredOreFeature> createDiamondOre();
    static std::unique_ptr<ConfiguredOreFeature> createLapisOre();
    static std::unique_ptr<ConfiguredOreFeature> createEmeraldOre();
    static std::unique_ptr<ConfiguredOreFeature> createCopperOre();

    // 下界矿石
    static std::unique_ptr<ConfiguredOreFeature> createNetherQuartzOre();
    static std::unique_ptr<ConfiguredOreFeature> createNetherGoldOre();
    static std::unique_ptr<ConfiguredOreFeature> createAncientDebris();

private:
    static std::vector<std::unique_ptr<ConfiguredOreFeature>> s_features;
};

} // namespace mr
