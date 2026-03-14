#include "BiomeLayers.hpp"
#include <unordered_set>

namespace mc {
namespace layer {

// ============================================================================
// BiomeLayer 实现
// ============================================================================

constexpr i32 BiomeLayer::WARM_BIOMES[];
constexpr i32 BiomeLayer::COOL_BIOMES[];
constexpr i32 BiomeLayer::ICY_BIOMES[];

BiomeLayer::BiomeLayer(const Config& config)
    : m_config(config)
{
}

i32 BiomeLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC BiomeLayer.apply:
    // int i = (value & 3840) >> 8;  // 提取特殊位
    // value = value & -3841;        // 清除特殊位
    //
    // if (!LayerUtil.isOcean(value) && value != 14) {  // 非海洋且非蘑菇岛
    //     switch(value) {
    //         case 1:  // Warm
    //             if (i > 0) {
    //                 return context.random(3) == 0 ? 39 : 38;  // wooded_badlands_plateau or badlands_plateau
    //             }
    //             return getBiomeId(DESERT, context);
    //         case 2:  // Medium/Lukewarm
    //             if (i > 0) {
    //                 return 21;  // jungle
    //             }
    //             return getBiomeId(WARM, context);
    //         case 3:  // Cool
    //             if (i > 0) {
    //                 return 32;  // birch_forest
    //             }
    //             return getBiomeId(COOL, context);
    //         case 4:  // Icy
    //             return getBiomeId(ICY, context);
    //         default:
    //             return 14;  // mushroom_fields
    //     }
    // } else {
    //     return value;
    // }

    // 提取特殊位 (bits 8-11)
    i32 special = BiomeValues::SpecialBits::extract(value);
    value = value & ~BiomeValues::SpecialBits::Mask;

    // 海洋和蘑菇岛保持不变
    if (BiomeValues::isOcean(value) || value == BiomeValues::MushroomFields) {
        return value;
    }

    switch (value) {
        case BiomeValues::Climate::Warm:  // 1
            if (special > 0) {
                // 有特殊位，返回恶地变体
                return ctx.nextInt(3) == 0 ? BiomeValues::WoodedBadlandsPlateau : BiomeValues::BadlandsPlateau;
            }
            // 普通温暖生物群系
            if (m_config.legacyDesertInit) {
                // 旧版：总是沙漠
                return BiomeValues::Desert;
            }
            // 新版：从列表中选择
            return WARM_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Medium:  // 2 (实际 MC 中温度 2 不经过这里，跳到 case 2)
            // 温暖生物群系（丛林等）
            if (special > 0) {
                return BiomeValues::Jungle;
            }
            // 随机选择：丛林、丛林边缘等
            // 注意：MC 的 Medium 温度区域映射到温暖生物群系列表
            // 这里简化处理
            return ctx.nextInt(3) == 0 ? BiomeValues::Jungle : BiomeValues::Plains;

        case BiomeValues::Climate::Cool:  // 3
            if (special > 0) {
                return BiomeValues::BirchForest;
            }
            return COOL_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Icy:  // 4
            return ICY_BIOMES[ctx.nextInt(6)];

        default:
            // 未知值，返回蘑菇岛
            return BiomeValues::MushroomFields;
    }
}

// ============================================================================
// RareBiomeLayer 实现
// ============================================================================

i32 RareBiomeLayer::apply(IAreaContext& ctx, i32 value) {
    // 参考 MC RareBiomeLayer.apply:
    // return context.random(57) == 0 && value == 1 ? 129 : value;

    // 平原 (1) 有 1/57 概率变成向日葵平原 (129)
    if (value == BiomeValues::Plains && ctx.nextInt(57) == 0) {
        return BiomeValues::SunflowerPlains;
    }
    return value;
}

// ============================================================================
// ShoreLayer 实现
// ============================================================================

namespace {
    // 雪地相邻生物群系
    const std::unordered_set<i32> snowyShoreBiomes = {
        BiomeValues::DeepFrozenOcean, BiomeValues::FrozenOcean,
        BiomeValues::FrozenRiver, BiomeValues::SnowyPlains,
        BiomeValues::SnowyMountains, BiomeValues::SnowyBeach,
        BiomeValues::SnowyTaiga, BiomeValues::SnowyTaigaHills,
        BiomeValues::IceSpikes
    };

    // 丛林相关生物群系
    const std::unordered_set<i32> jungleShoreBiomes = {
        BiomeValues::BambooJungle, BiomeValues::BambooJungleHills,
        BiomeValues::Jungle, BiomeValues::JungleHills,
        BiomeValues::JungleEdge, BiomeValues::ModifiedJungle,
        BiomeValues::ModifiedJungleEdge
    };
}

i32 ShoreLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) {
    // 参考 MC ShoreLayer.apply

    // 蘑菇岛海岸
    if (center == BiomeValues::MushroomFields) {
        if (BiomeValues::isShallowOcean(north) || BiomeValues::isShallowOcean(east) ||
            BiomeValues::isShallowOcean(south) || BiomeValues::isShallowOcean(west)) {
            return BiomeValues::MushroomFieldShore;
        }
    }

    // 丛林海岸
    if (jungleShoreBiomes.count(center) > 0) {
        // 检查周围是否都是丛林兼容的
        if (!isJungleCompatible(north) || !isJungleCompatible(east) ||
            !isJungleCompatible(south) || !isJungleCompatible(west)) {
            return BiomeValues::JungleEdge;
        }
        // 与海洋相邻变成丛林
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) ||
            BiomeValues::isOcean(south) || BiomeValues::isOcean(west)) {
            return BiomeValues::Jungle;
        }
    }

    // 山地和山地变体
    if (center == BiomeValues::Mountains || center == BiomeValues::WoodedMountains ||
        center == BiomeValues::GravellyMountains || center == BiomeValues::ModifiedGravellyMountains) {
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) ||
            BiomeValues::isOcean(south) || BiomeValues::isOcean(west)) {
            return BiomeValues::StoneShore;
        }
    }

    // 雪地海滩
    if (snowyShoreBiomes.count(center) > 0) {
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) ||
            BiomeValues::isOcean(south) || BiomeValues::isOcean(west)) {
            return BiomeValues::SnowyBeach;
        }
    }

    // 恶地
    if (center == BiomeValues::Badlands || center == BiomeValues::ErodedBadlands) {
        if (!BiomeValues::isOcean(north) && !BiomeValues::isOcean(east) &&
            !BiomeValues::isOcean(south) && !BiomeValues::isOcean(west) &&
            (!isMesa(north) || !isMesa(east) || !isMesa(south) || !isMesa(west))) {
            return BiomeValues::Desert;
        }
    }

    // 恶地高原
    if (center == BiomeValues::WoodedBadlandsPlateau || center == BiomeValues::BadlandsPlateau) {
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) ||
            BiomeValues::isOcean(south) || BiomeValues::isOcean(west)) {
            return BiomeValues::StoneShore;
        }
    }

    // 普通海滩
    if (!BiomeValues::isOcean(center) && center != BiomeValues::River && center != BiomeValues::Swamp) {
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) ||
            BiomeValues::isOcean(south) || BiomeValues::isOcean(west)) {
            return BiomeValues::Beach;
        }
    }

    return center;
}

bool ShoreLayer::isJungleCompatible(i32 biome) {
    return jungleShoreBiomes.count(biome) > 0 ||
           biome == BiomeValues::Forest ||
           biome == BiomeValues::Taiga ||
           BiomeValues::isOcean(biome);
}

bool ShoreLayer::isMesa(i32 biome) {
    return biome == BiomeValues::Badlands ||
           biome == BiomeValues::WoodedBadlandsPlateau ||
           biome == BiomeValues::BadlandsPlateau ||
           biome == BiomeValues::ErodedBadlands ||
           biome == BiomeValues::ModifiedWoodedBadlandsPlateau ||
           biome == BiomeValues::ModifiedBadlandsPlateau;
}

// ============================================================================
// SmoothLayer 实现
// ============================================================================

i32 SmoothLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center) {
    // 参考 MC SmoothLayer.apply:
    // boolean flag = west == east;
    // boolean flag1 = north == south;
    // if (flag == flag1) {
    //     if (flag) {
    //         return context.random(2) == 0 ? east : north;
    //     } else {
    //         return center;
    //     }
    // } else {
    //     return flag ? east : north;
    // }

    bool ewEqual = (east == west);
    bool nsEqual = (north == south);

    if (ewEqual == nsEqual) {
        if (ewEqual) {
            // 东西相等且南北相等，随机选择
            return ctx.pickRandom(east, north);
        } else {
            // 都不相等，保持中心
            return center;
        }
    } else {
        // 其中一对相等
        return ewEqual ? east : north;
    }
}

} // namespace layer
} // namespace mc
