#pragma once

#include "TransformerTraits.hpp"
#include "../BiomeValues.hpp"

namespace mc {
namespace layer {

/**
 * @brief 生物群系分配层
 *
 * 将温度值转换为实际的生物群系 ID。
 * 参考 MC BiomeLayer (IC0Transformer)
 */
class BiomeLayer : public IC0Transformer {
public:
    /**
     * @brief 生物群系配置
     */
    struct Config {
        bool legacyDesertInit = false;  // 是否使用旧的沙漠初始化
    };

    explicit BiomeLayer(const Config& config = Config{});

    using IC0Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;

private:
    Config m_config;

    // 各温度区域的生物群系列表
    static constexpr i32 WARM_BIOMES[] = {BiomeValues::Desert, BiomeValues::Desert, BiomeValues::Desert,
                                           BiomeValues::Savanna, BiomeValues::Savanna, BiomeValues::Plains};
    static constexpr i32 COOL_BIOMES[] = {BiomeValues::Forest, BiomeValues::Mountains, BiomeValues::Mountains,
                                          BiomeValues::Plains, BiomeValues::Taiga, BiomeValues::Plains};
    static constexpr i32 ICY_BIOMES[] = {BiomeValues::SnowyPlains, BiomeValues::SnowyPlains, BiomeValues::SnowyPlains,
                                         BiomeValues::SnowyTaiga, BiomeValues::SnowyTaiga, BiomeValues::SnowyPlains};
};

/**
 * @brief 稀有生物群系层
 *
 * 在基础生物群系中生成稀有变体。
 * 参考 MC RareBiomeLayer (IC1Transformer)
 */
class RareBiomeLayer : public IC1Transformer {
public:
    using IC1Transformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 value) override;
};

/**
 * @brief 海岸层
 *
 * 在陆地与海洋交界处生成海岸生物群系。
 * 参考 MC ShoreLayer (ICastleTransformer)
 */
class ShoreLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;

private:
    /**
     * @brief 检查是否与丛林兼容
     */
    [[nodiscard]] static bool isJungleCompatible(i32 biome);

    /**
     * @brief 检查是否为恶地
     */
    [[nodiscard]] static bool isMesa(i32 biome);
};

/**
 * @brief 平滑层
 *
 * 平滑生物群系边界。
 * 参考 MC SmoothLayer (ICastleTransformer)
 */
class SmoothLayer : public ICastleTransformer {
public:
    using ICastleTransformer::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) override;
};

} // namespace layer
} // namespace mc
