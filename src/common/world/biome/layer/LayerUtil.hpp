#pragma once

#include "Layer.hpp"
#include "LazyArea.hpp"
#include "transformers/BasicTransformers.hpp"
#include "transformers/BiomeTransformers.hpp"
#include "../BiomeProvider.hpp"
#include <memory>
#include <vector>

namespace mr {

/**
 * @brief 层堆叠构建器
 *
 * 构建层叠生物群系生成管道。
 * 参考 MC LayerStack / LayerUtil
 */
class LayerStack {
public:
    /**
     * @brief 构建层堆叠
     * @param seed 世界种子
     * @param isLargeBiomes 是否使用大型生物群系
     */
    explicit LayerStack(u64 seed, bool isLargeBiomes = false);
    ~LayerStack() = default;

    /**
     * @brief 采样指定位置的生物群系
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 生物群系 ID
     */
    [[nodiscard]] BiomeId sample(i32 x, i32 z) const;

    /**
     * @brief 采样指定区域的生物群系
     * @param startX 起始 X
     * @param startZ 起始 Z
     * @param width 宽度
     * @param height 高度
     * @return 生物群系数组
     */
    [[nodiscard]] std::vector<BiomeId> sampleArea(i32 startX, i32 startZ,
                                                   i32 width, i32 height) const;

    /**
     * @brief 获取区域上下文
     */
    [[nodiscard]] std::shared_ptr<IAreaContext> getContext() const { return m_context; }

private:
    u64 m_seed;
    bool m_isLargeBiomes;
    std::shared_ptr<IAreaContext> m_context;
    std::vector<std::unique_ptr<IArea>> m_layers;

    /**
     * @brief 初始化层堆叠
     */
    void initLayers();
};

/**
 * @brief 基于层的生物群系提供者
 *
 * 使用 Layer 系统生成生物群系。
 * 参考 MC 1.16.5 OverworldBiomeProvider
 */
class LayerBiomeProvider : public BiomeProvider {
public:
    /**
     * @brief 构造基于层的生物群系提供者
     * @param seed 世界种子
     * @param isLargeBiomes 是否使用大型生物群系
     */
    LayerBiomeProvider(u64 seed, bool isLargeBiomes = false);
    ~LayerBiomeProvider() override = default;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    [[nodiscard]] const Biome& getBiomeDefinition(BiomeId id) const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

private:
    std::unique_ptr<LayerStack> m_layerStack;

    // 缓存的生物群系定义
    mutable std::vector<const Biome*> m_biomeCache;
};

// ============================================================================
// 层工具函数
// ============================================================================

namespace LayerUtil {

/**
 * @brief 创建主世界层堆叠
 * @param seed 世界种子
 * @param isLargeBiomes 是否使用大型生物群系
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createOverworldLayers(u64 seed, bool isLargeBiomes = false);

/**
 * @brief 创建下界层堆叠
 * @param seed 世界种子
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createNetherLayers(u64 seed);

/**
 * @brief 创建末地层堆叠
 * @param seed 世界种子
 * @return 层堆叠
 */
std::unique_ptr<LayerStack> createEndLayers(u64 seed);

} // namespace LayerUtil

} // namespace mr
