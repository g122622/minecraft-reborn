#include "BasicTransformers.hpp"
#include "../../Biome.hpp"
#include <random>

namespace mc {

// ============================================================================
// IslandLayer 实现
// ============================================================================

IslandLayer::IslandLayer(u64 seed)
    : m_seed(seed)
{
}

i32 IslandLayer::apply(IAreaContext& context, const IArea& /* area */, i32 x, i32 z) const
{
    // 初始化随机数
    context.initRandom(m_seed + static_cast<u64>(x) * 341873128712ULL + static_cast<u64>(z) * 132897987541ULL);

    // 基础岛屿生成（参考 MC 的 10% 岛屿概率）
    if (context.nextInt(10) == 0) {
        return 1; // 陆地
    }
    return 0; // 海洋
}

// ============================================================================
// ZoomLayer 实现
// ============================================================================

ZoomLayer::ZoomLayer(Mode mode)
    : m_mode(mode)
{
}

i32 ZoomLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    // 将坐标转换为上层坐标
    const i32 baseX = x >> 1;
    const i32 baseZ = z >> 1;

    // 获取四个相邻值
    const i32 v00 = area.getValue(baseX, baseZ);
    const i32 v10 = area.getValue(baseX + 1, baseZ);
    const i32 v01 = area.getValue(baseX, baseZ + 1);
    const i32 v11 = area.getValue(baseX + 1, baseZ + 1);

    // 根据缩放模式选择插值方式
    switch (m_mode) {
        case Mode::Fuzzy: {
            // 模糊缩放：随机选择一个值
            const i32 choice = context.nextInt(4);
            switch (choice) {
                case 0: return v00;
                case 1: return v10;
                case 2: return v01;
                default: return v11;
            }
        }

        case Mode::Voroni: {
            // Voroni 缩放：根据距离选择
            // localX 和 localZ 用于确定相对于区块单元的位置
            // 当前实现通过随机距离来模拟 Voronoi 效果

            // 初始化位置相关随机
            context.initRandom(static_cast<u64>(x) * 341873128712ULL + static_cast<u64>(z) * 132897987541ULL);
            const i32 dist00 = context.nextInt(4);
            const i32 dist10 = context.nextInt(4);
            const i32 dist01 = context.nextInt(4);
            const i32 dist11 = context.nextInt(4);

            // 选择最近的值
            i32 minDist = dist00;
            i32 result = v00;

            if (dist10 < minDist) { minDist = dist10; result = v10; }
            if (dist01 < minDist) { minDist = dist01; result = v01; }
            if (dist11 < minDist) { result = v11; }

            return result;
        }

        case Mode::Normal:
        default: {
            // 普通缩放：根据位置选择
            const i32 localX = x & 1;
            const i32 localZ = z & 1;

            if (localX == 0 && localZ == 0) {
                return v00;
            } else if (localX == 1 && localZ == 0) {
                return v10;
            } else if (localX == 0 && localZ == 1) {
                return v01;
            } else {
                // 边缘情况：选择众数或随机
                return getMode(v00, v10, v01, v11);
            }
        }
    }
}

i32 ZoomLayer::getMode(i32 a, i32 b, i32 c, i32 d)
{
    // 选择出现次数最多的值
    if (a == b || a == c || a == d) return a;
    if (b == c || b == d) return b;
    if (c == d) return c;
    return a; // 默认返回第一个
}

// ============================================================================
// AddIslandLayer 实现
// ============================================================================

i32 AddIslandLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 center = area.getValue(x, z);

    // 只有海洋才可能变成岛屿
    if (center != 0) {
        return center;
    }

    // 检查周围是否有陆地
    const i32 north = area.getValue(x, z - 1);
    const i32 south = area.getValue(x, z + 1);
    const i32 east = area.getValue(x + 1, z);
    const i32 west = area.getValue(x - 1, z);

    // 如果周围都是海洋，有概率生成岛屿
    if (north == 0 && south == 0 && east == 0 && west == 0) {
        if (context.nextInt(100) < 3) {
            return 1; // 岛屿
        }
    }

    return center;
}

// ============================================================================
// AddSnowLayer 实现
// ============================================================================

i32 AddSnowLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 value = area.getValue(x, z);

    // 只处理陆地
    if (value == 0) {
        return value; // 保持海洋
    }

    // 根据随机值决定是否为雪地
    const i32 rnd = context.nextInt(6);

    if (rnd == 0) {
        return Biomes::SnowyPlains; // 雪地
    } else if (rnd == 1) {
        return Biomes::SnowyTaiga;  // 雪地针叶林
    }

    return value;
}

// ============================================================================
// EdgeLayer 实现
// ============================================================================

i32 EdgeLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    (void)context; // 暂时不使用

    const i32 center = area.getValue(x, z);
    const i32 north = area.getValue(x, z - 1);
    const i32 south = area.getValue(x, z + 1);
    const i32 east = area.getValue(x + 1, z);
    const i32 west = area.getValue(x - 1, z);

    // 如果中心生物群系周围有不同类型，可能需要边缘处理
    if (center == north && center == south && center == east && center == west) {
        return center;
    }

    // 边缘处理：根据周围生物群系选择过渡
    // 简化版本：保持原值
    return center;
}

// ============================================================================
// AddMushroomIslandLayer 实现
// ============================================================================

i32 AddMushroomIslandLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 center = area.getValue(x, z);

    // 只有海洋才可能变成蘑菇岛
    if (center != 0) {
        return center;
    }

    // 检查周围都是海洋
    const i32 north = area.getValue(x, z - 1);
    const i32 south = area.getValue(x, z + 1);
    const i32 east = area.getValue(x + 1, z);
    const i32 west = area.getValue(x - 1, z);

    if (north == 0 && south == 0 && east == 0 && west == 0) {
        if (context.nextInt(100) == 0) {
            return Biomes::MushroomFields;
        }
    }

    return center;
}

// ============================================================================
// DeepOceanLayer 实现
// ============================================================================

i32 DeepOceanLayer::apply(IAreaContext& context, const IArea& area, i32 x, i32 z) const
{
    const i32 center = area.getValue(x, z);

    // 只处理海洋
    if (center != Biomes::Ocean) {
        return center;
    }

    // 检查周围海洋数量
    i32 oceanCount = 0;
    if (area.getValue(x, z - 1) == Biomes::Ocean) oceanCount++;
    if (area.getValue(x, z + 1) == Biomes::Ocean) oceanCount++;
    if (area.getValue(x + 1, z) == Biomes::Ocean) oceanCount++;
    if (area.getValue(x - 1, z) == Biomes::Ocean) oceanCount++;

    // 如果周围都是海洋，变成深海
    if (oceanCount >= 4 && context.nextInt(3) == 0) {
        return Biomes::DeepOcean;
    }

    return center;
}

} // namespace mc
