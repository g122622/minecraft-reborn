#include "BiomeTransformers.hpp"
#include <random>

namespace mc {

// ============================================================================
// BiomeLayer 实现
// ============================================================================

BiomeLayer::BiomeLayer(const Config& config)
    : m_config(config)
{
}

i32 BiomeLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 value = area.getValue(x, z);

    // 0 = 海洋，1+ = 陆地
    if (value == 0) {
        return Biomes::Ocean;
    }

    // 根据随机值分配生物群系
    const i32 rnd = context.nextInt(100);

    // 温度区域划分
    if (rnd < 10) {
        // 冰冻区域
        return m_config.frozenBiomeId;
    } else if (rnd < 30) {
        // 寒冷区域
        return m_config.coldBiomeId;
    } else if (rnd < 60) {
        // 温和区域
        return Biomes::Plains;
    } else if (rnd < 85) {
        // 微温区域
        return m_config.lukewarmBiomeId;
    } else {
        // 温暖区域
        return m_config.warmBiomeId;
    }
}

// ============================================================================
// HillsLayer 实现
// ============================================================================

HillsLayer::HillsLayer(u64 seed)
    : m_seed(seed)
{
}

i32 HillsLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 center = area.getValue(x, z);

    // 初始化随机
    context.initRandom(m_seed + static_cast<u64>(x) * 341873128712ULL + static_cast<u64>(z) * 132897987541ULL);
    const i32 rnd = context.nextInt(6);

    // 根据基础生物群系选择是否添加山丘变体
    return getHillsBiome(center, rnd);
}

i32 HillsLayer::getHillsBiome(i32 baseBiome, i32 rnd)
{
    // 约 1/6 概率生成山丘变体
    if (rnd != 0) {
        return baseBiome;
    }

    switch (baseBiome) {
        case Biomes::Plains:
            return Biomes::WoodedHills;
        case Biomes::Desert:
            return Biomes::DesertHills;
        case Biomes::Forest:
            return Biomes::WoodedHills;
        case Biomes::Taiga:
            return Biomes::WoodedHills;
        case Biomes::Mountains:
            return Biomes::WoodedMountains;
        case Biomes::Jungle:
            return Biomes::WoodedHills;
        case Biomes::Savanna:
            return Biomes::ShatteredSavanna;
        case Biomes::Badlands:
            return Biomes::ErodedBadlands;
        default:
            return baseBiome;
    }
}

// ============================================================================
// ShoreLayer 实现
// ============================================================================

i32 ShoreLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    (void)context;

    const i32 center = area.getValue(x, z);
    const i32 north = area.getValue(x, z - 1);
    const i32 south = area.getValue(x, z + 1);
    const i32 east = area.getValue(x + 1, z);
    const i32 west = area.getValue(x - 1, z);

    // 如果是海洋，直接返回
    if (center == Biomes::Ocean || center == Biomes::DeepOcean ||
        center == Biomes::WarmOcean || center == Biomes::ColdOcean ||
        center == Biomes::FrozenOcean) {
        return center;
    }

    // 如果周围有海洋，添加海滩
    if (north == Biomes::Ocean || south == Biomes::Ocean ||
        east == Biomes::Ocean || west == Biomes::Ocean) {

        // 根据生物群系选择海滩类型
        switch (center) {
            case Biomes::Mountains:
            case Biomes::WoodedMountains:
                return Biomes::StoneShore;
            case Biomes::SnowyPlains:
            case Biomes::SnowyTaiga:
                return Biomes::SnowyBeach;
            default:
                return Biomes::Beach;
        }
    }

    return center;
}

// ============================================================================
// SmoothLayer 实现
// ============================================================================

i32 SmoothLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    (void)context;

    const i32 center = area.getValue(x, z);
    const i32 north = area.getValue(x, z - 1);
    const i32 south = area.getValue(x, z + 1);
    const i32 east = area.getValue(x + 1, z);
    const i32 west = area.getValue(x - 1, z);

    // 如果周围都相同，保持
    if (north == center && south == center && east == center && west == center) {
        return center;
    }

    // 计算周围相同值的数量
    i32 sameCount = 0;
    if (north == center) sameCount++;
    if (south == center) sameCount++;
    if (east == center) sameCount++;
    if (west == center) sameCount++;

    // 如果多数相同，保持
    if (sameCount >= 2) {
        return center;
    }

    // 否则选择邻居中的众数
    if (north == south && north != 0) return north;
    if (east == west && east != 0) return east;
    if (north != 0 && (north == east || north == west)) return north;
    if (south != 0 && (south == east || south == west)) return south;

    return center;
}

// ============================================================================
// RiverLayer 实现
// ============================================================================

RiverLayer::RiverLayer(u64 seed)
    : m_seed(seed)
{
}

i32 RiverLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 center = area.getValue(x, z);

    // 初始化随机
    context.initRandom(m_seed + static_cast<u64>(x) * 341873128712ULL + static_cast<u64>(z) * 132897987541ULL);

    // 基于噪声值决定是否生成河流
    // 参考 MC 的河流生成概率（约 1/15）
    if (context.nextInt(15) == 0) {
        return Biomes::River;
    }

    return center;
}

// ============================================================================
// MixRiverLayer 实现
// ============================================================================

i32 MixRiverLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    (void)context;

    const i32 center = area.getValue(x, z);

    // 河流不覆盖海洋
    if (center == Biomes::Ocean || center == Biomes::DeepOcean) {
        return center;
    }

    // 如果是河流ID，保持河流
    // 这里简化处理，实际应该与原始生物群系层混合
    return center;
}

} // namespace mc
