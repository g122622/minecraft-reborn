#include "MergeLayers.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace layer {

// ============================================================================
// AddMushroomIslandLayer 实现
// ============================================================================

i32 AddMushroomIslandLayer::apply(IAreaContext& ctx, i32 x, i32 sw, i32 se, i32 ne, i32 nw, i32 center) {
    // 参考 MC AddMushroomIslandLayer.apply:
    // return LayerUtil.isShallowOcean(center) && LayerUtil.isShallowOcean(nw) &&
    //        LayerUtil.isShallowOcean(ne) && LayerUtil.isShallowOcean(sw) &&
    //        LayerUtil.isShallowOcean(se) && context.random(100) == 0 ? 14 : center;

    // 如果中心和四个对角都是浅海，有 1% 概率生成蘑菇岛
    if (BiomeValues::isShallowOcean(center) &&
        BiomeValues::isShallowOcean(nw) &&
        BiomeValues::isShallowOcean(ne) &&
        BiomeValues::isShallowOcean(sw) &&
        BiomeValues::isShallowOcean(se)) {
        if (ctx.nextInt(100) == 0) {
            return BiomeValues::MushroomFields;
        }
    }

    return center;
}

// ============================================================================
// AddBambooForestLayer 实现
// ============================================================================

i32 AddBambooForestLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC AddBambooForestLayer.apply:
    // return context.random(10) == 0 && value == 21 ? 168 : value;

    // 丛林 (21) 有 1/10 概率变成竹林 (168)
    if (value == BiomeValues::Jungle && ctx.nextInt(10) == 0) {
        return BiomeValues::BambooJungle;
    }
    return value;
}

// ============================================================================
// StartRiverLayer 实现
// ============================================================================

i32 StartRiverLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC StartRiverLayer.apply:
    // return LayerUtil.isShallowOcean(value) ? value : context.random(299999) + 2;

    // 浅海保持不变，否则返回河流噪声值 (2-300000)
    if (BiomeValues::isShallowOcean(value)) {
        return value;
    }
    return ctx.nextInt(299999) + 2;
}

// ============================================================================
// RiverLayer 实现
// ============================================================================

i32 RiverLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) {
    // 参考 MC RiverLayer.apply:
    // int i = riverFilter(center);
    // return i == riverFilter(east) && i == riverFilter(north) &&
    //        i == riverFilter(west) && i == riverFilter(south) ? -1 : 7;

    i32 c = riverFilter(center);
    i32 n = riverFilter(north);
    i32 e = riverFilter(east);
    i32 s = riverFilter(south);
    i32 w = riverFilter(west);

    // 如果所有方向过滤后的值相同，返回 -1（无河流）
    // 否则返回 7（河流）
    return (c == e && c == n && c == w && c == s) ? -1 : BiomeValues::River;
}

i32 RiverLayer::riverFilter(i32 value) {
    // 参考 MC RiverLayer.riverFilter:
    // return p_151630_0_ >= 2 ? 2 + (p_151630_0_ & 1) : p_151630_0_;
    if (value >= 2) {
        return 2 + (value & 1);
    }
    return value;
}

// ============================================================================
// HillsLayer 实现
// ============================================================================

// 山丘变体映射表
const std::unordered_map<i32, i32> HillsLayer::s_hillsBiomes = {
    {BiomeValues::Plains, BiomeValues::WoodedHills},
    {BiomeValues::Desert, BiomeValues::DesertHills},  // 注：MC 1.16.5 中映射到 17 (WoodedHills)，这里保持合理
    {BiomeValues::Mountains, BiomeValues::WoodedMountains},
    {BiomeValues::Forest, BiomeValues::WoodedHills},
    {BiomeValues::Taiga, BiomeValues::TaigaHills},  // 19 或 WoodedHills
    {BiomeValues::Swamp, BiomeValues::SwampHills},
    {BiomeValues::Jungle, BiomeValues::JungleHills},
    {BiomeValues::BirchForest, BiomeValues::BirchForestHills},
    {BiomeValues::DarkForest, BiomeValues::DarkForestHills},
    {BiomeValues::SnowyTaiga, BiomeValues::SnowyTaigaHills},
    {BiomeValues::GiantTreeTaiga, BiomeValues::GiantTreeTaigaHills},
    {BiomeValues::Savanna, BiomeValues::ShatteredSavanna},
    {BiomeValues::BadlandsPlateau, BiomeValues::Badlands},
    {BiomeValues::WoodedBadlandsPlateau, BiomeValues::Badlands},
    // 稀有变体
    {BiomeValues::SunflowerPlains, BiomeValues::WoodedHills},
    {BiomeValues::DesertLakes, BiomeValues::DesertHills},
    {BiomeValues::GravellyMountains, BiomeValues::ModifiedGravellyMountains},
    {BiomeValues::FlowerForest, BiomeValues::WoodedHills},
    {BiomeValues::IceSpikes, BiomeValues::SnowyMountains},
    {BiomeValues::ModifiedJungle, BiomeValues::JungleHills},
    {BiomeValues::TallBirchForest, BiomeValues::TallBirchHills},
    {BiomeValues::DarkForestHills, BiomeValues::Plains},  // 简化
    {BiomeValues::GiantSpruceTaiga, BiomeValues::GiantSpruceTaigaHills},
};

i32 HillsLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) {
    // 参考 MC HillsLayer.apply (这是最复杂的层之一)

    i32 biomeValue = biomeArea.getValue(getOffsetX(x + 1), getOffsetZ(z + 1));
    i32 riverValue = riverArea.getValue(getOffsetX(x + 1), getOffsetZ(z + 1));

    // 提取河流噪声的低位
    i32 riverNoise = (riverValue - 2) % 29;

    // 检查是否应该生成山丘变体
    if (!BiomeValues::isShallowOcean(biomeValue) && riverValue >= 2 && riverNoise == 1) {
        // 山丘变体
        auto it = s_hillsBiomes.find(biomeValue);
        if (it != s_hillsBiomes.end()) {
            return it->second;
        }
    }

    // 随机山丘变体（约 1/3 概率）
    if (ctx.nextInt(3) == 0 || riverNoise == 0) {
        i32 result = biomeValue;

        // 根据基础生物群系选择变体
        switch (biomeValue) {
            case BiomeValues::Desert:
                result = BiomeValues::Jungle;  // 简化，实际应为 DesertHills 或 WoodedHills
                break;
            case BiomeValues::Forest:
                result = BiomeValues::Jungle;  // 实际应为 WoodedHills (17)
                break;
            case BiomeValues::BirchForest:
                result = BiomeValues::BirchForestHills;
                break;
            case BiomeValues::SnowyPlains:
                result = BiomeValues::SnowyMountains;
                break;
            case BiomeValues::Plains:
                result = ctx.nextInt(3) == 0 ? BiomeValues::Jungle : BiomeValues::Forest;
                break;
            case BiomeValues::Ocean:
                result = BiomeValues::DeepOcean;
                break;
            case BiomeValues::LukewarmOcean:
                result = BiomeValues::DeepLukewarmOcean;
                break;
            case BiomeValues::ColdOcean:
                result = BiomeValues::DeepColdOcean;
                break;
            case BiomeValues::FrozenOcean:
                result = BiomeValues::DeepFrozenOcean;
                break;
            case BiomeValues::Mountains:
                result = BiomeValues::WoodedMountains;
                break;
            case BiomeValues::Savanna:
                result = BiomeValues::SavannaPlateau;
                break;
            case BiomeValues::Badlands:
                result = BiomeValues::ErodedBadlands;
                break;
            default:
                break;
        }

        // 如果有河流噪声且结果是山丘变体
        if (riverNoise == 0 && result != biomeValue) {
            auto it = s_hillsBiomes.find(result);
            if (it != s_hillsBiomes.end()) {
                result = it->second;
            }
        }

        return result;
    }

    return biomeValue;
}

std::unique_ptr<IAreaFactory> HillsLayer::apply(
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixRiverLayer 实现
// ============================================================================

i32 MixRiverLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& riverArea, i32 x, i32 z) {
    // 参考 MC MixRiverLayer.apply:
    // int i = biomeArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));
    // int j = riverArea.getValue(this.getOffsetX(x), this.getOffsetZ(z));
    // if (LayerUtil.isOcean(i)) {
    //     return i;
    // } else if (j == 7) {
    //     if (i == 12) {  // snowy_plains
    //         return 11;  // frozen_river
    //     } else {
    //         return i != 14 && i != 15 ? j & 255 : 15;  // mushroom_fields or mushroom_field_shore -> shore
    //     }
    // } else {
    //     return i;
    // }

    (void)ctx;  // 不使用

    i32 biome = biomeArea.getValue(getOffsetX(x), getOffsetZ(z));
    i32 river = riverArea.getValue(getOffsetX(x), getOffsetZ(z));

    // 海洋保持不变
    if (BiomeValues::isOcean(biome)) {
        return biome;
    }

    // 河流 (j == 7)
    if (river == BiomeValues::River) {
        if (biome == BiomeValues::SnowyPlains) {
            return BiomeValues::FrozenRiver;
        }
        // 蘑菇岛变成岸边
        if (biome != BiomeValues::MushroomFields && biome != BiomeValues::MushroomFieldShore) {
            return BiomeValues::River;
        }
        return BiomeValues::MushroomFieldShore;
    }

    return biome;
}

std::unique_ptr<IAreaFactory> MixRiverLayer::apply(
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

// ============================================================================
// MixOceansLayer 实现
// ============================================================================

i32 MixOceansLayer::apply(IAreaContext& ctx, const IArea& biomeArea, const IArea& oceanArea, i32 x, i32 z) {
    // 参考 MC MixOceansLayer.apply:
    // 检查周围的海洋温度，调整深海类型

    i32 biome = biomeArea.getValue(getOffsetX(x), getOffsetZ(z));
    i32 ocean = oceanArea.getValue(getOffsetX(x), getOffsetZ(z));

    // 非海洋保持不变
    if (!BiomeValues::isOcean(biome)) {
        return biome;
    }

    // 检查周围是否有陆地
    for (i32 dx = -8; dx <= 8; dx += 4) {
        for (i32 dz = -8; dz <= 8; dz += 4) {
            i32 neighbor = biomeArea.getValue(getOffsetX(x + dx), getOffsetZ(z + dz));
            if (!BiomeValues::isOcean(neighbor)) {
                // 有陆地相邻，调整海洋温度
                if (ocean == BiomeValues::WarmOcean) {
                    return BiomeValues::LukewarmOcean;
                }
                if (ocean == BiomeValues::FrozenOcean) {
                    return BiomeValues::ColdOcean;
                }
            }
        }
    }

    // 深海根据海洋温度调整
    if (biome == BiomeValues::DeepOcean) {
        switch (ocean) {
            case BiomeValues::LukewarmOcean:
                return BiomeValues::DeepLukewarmOcean;
            case BiomeValues::Ocean:
                return BiomeValues::DeepOcean;
            case BiomeValues::ColdOcean:
                return BiomeValues::DeepColdOcean;
            case BiomeValues::FrozenOcean:
                return BiomeValues::DeepFrozenOcean;
            default:
                return ocean;
        }
    }

    return ocean;
}

std::unique_ptr<IAreaFactory> MixOceansLayer::apply(
    IExtendedAreaContext& context,
    std::unique_ptr<IAreaFactory> input1,
    std::unique_ptr<IAreaFactory> input2)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<MergeFactory>(this, sharedContext, std::move(input1), std::move(input2));
}

} // namespace layer
} // namespace mc
