#pragma once

#include "../Layer.hpp"
#include "../../Biome.hpp"
#include <functional>

namespace mc {

/**
 * @brief 生物群系分配层
 *
 * 根据温度和湿度分配生物群系。
 * 参考 MC BiomeLayer / BiomeSource
 */
class BiomeLayer : public IAreaTransformer {
public:
    /**
     * @brief 生物群系分配配置
     */
    struct Config {
        i32 warmBiomeId = Biomes::Plains;       // 温暖生物群系
        i32 lukewarmBiomeId = Biomes::Savanna;  // 微温生物群系
        i32 coldBiomeId = Biomes::Taiga;        // 寒冷生物群系
        i32 frozenBiomeId = Biomes::SnowyPlains;// 冰冻生物群系
        i32 specialChance = 10;                 // 特殊生物群系概率
    };

    explicit BiomeLayer(const Config& config = Config{});

    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;

private:
    Config m_config;
};

/**
 * @brief 山丘层
 *
 * 添加山丘和山地变体生物群系。
 * 参考 MC HillsLayer
 */
class HillsLayer : public IAreaTransformer {
public:
    explicit HillsLayer(u64 seed);

    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;

private:
    u64 m_seed;

    /**
     * @brief 获取山丘变体生物群系
     * @param baseBiome 基础生物群系
     * @param rnd 随机值
     * @return 山丘变体
     */
    [[nodiscard]] static i32 getHillsBiome(i32 baseBiome, i32 rnd);
};

/**
 * @brief 海岸层
 *
 * 添加海岸生物群系（沙滩、石岸等）。
 * 参考 MC ShoreLayer
 */
class ShoreLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 平滑层
 *
 * 平滑生物群系边缘。
 * 参考 MC SmoothLayer
 */
class SmoothLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 河流层
 *
 * 生成河流生物群系。
 * 参考 MC RiverLayer
 */
class RiverLayer : public IAreaTransformer {
public:
    explicit RiverLayer(u64 seed);

    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;

private:
    u64 m_seed;
};

/**
 * @brief 河流混合层
 *
 * 将河流与生物群系混合。
 * 参考 MC MixRiverLayer
 */
class MixRiverLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

} // namespace mc
