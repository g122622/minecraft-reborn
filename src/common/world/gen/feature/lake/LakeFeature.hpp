#pragma once

#include "../Feature.hpp"
#include <memory>

// Forward declarations
namespace mc {
class Block;
class BlockState;
class IWorldWriter;
}

namespace mc::world::gen::feature::lake {

/**
 * @brief 湖泊特征配置
 *
 * 配置湖泊或熔岩湖的参数。
 */
struct LakeFeatureConfig {
    Block* fluidBlock;          ///< 流体方块（用于比较）
    Block* borderBlock;         ///< 边界方块（用于比较）
    const BlockState* fluidState;   ///< 流体方块状态
    const BlockState* borderState;  ///< 边界方块状态

    LakeFeatureConfig(Block* fluid = nullptr, Block* border = nullptr)
        : fluidBlock(fluid)
        , borderBlock(border)
        , fluidState(fluid ? &fluid->defaultState() : nullptr)
        , borderState(border ? &border->defaultState() : nullptr) {}
};

/**
 * @brief 湖泊特征
 *
 * 生成湖泊或熔岩湖。
 * 参考 MC 1.16.5: net.minecraft.world.gen.feature.LakesFeature
 */
class LakeFeature {
public:
    /**
     * @brief 构造函数
     * @param config 配置
     */
    explicit LakeFeature(const LakeFeatureConfig& config);

    /**
     * @brief 生成湖泊
     * @param world 世界写入器
     * @param rng 随机数生成器
     * @param x 中心 X 坐标
     * @param y 中心 Y 坐标
     * @param z 中心 Z 坐标
     * @return 是否成功生成
     */
    bool place(IWorldWriter& world, math::Random& rng, i32 x, i32 y, i32 z);

    /**
     * @brief 创建水湖配置
     */
    static LakeFeatureConfig createWaterLake();

    /**
     * @brief 创建熔岩湖配置
     */
    static LakeFeatureConfig createLavaLake();

private:
    /**
     * @brief 检查位置是否适合生成湖泊
     */
    [[nodiscard]] bool canPlaceAt(IWorldWriter& world, i32 x, i32 y, i32 z) const;

    LakeFeatureConfig m_config;
};

/**
 * @brief 创建水湖特征
 */
std::unique_ptr<LakeFeature> createWaterLakeFeature();

/**
 * @brief 创建熔岩湖特征
 */
std::unique_ptr<LakeFeature> createLavaLakeFeature();

} // namespace mc::world::gen::feature::lake
