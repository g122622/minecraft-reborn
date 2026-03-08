#define _USE_MATH_DEFINES
#include "OreFeature.hpp"
#include "../Feature.hpp"
#include "../../placement/Placement.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../block/BlockRegistry.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// OreFeature 实现
// ============================================================================

bool OreFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    math::Random& random,
    const BlockPos& origin,
    const OreFeatureConfig& config)
{
    // 参考 MC OreFeature.place()
    // 生成椭圆形矿脉

    // 计算矿脉参数
    f32 angle = random.nextFloat() * static_cast<f32>(M_PI);
    f32 sizeFactor = static_cast<f32>(config.size) / 8.0f;
    i32 halfSize = static_cast<i32>(std::ceil((static_cast<f32>(config.size) / 16.0f * 2.0f + 1.0f) / 2.0f));

    // 计算矿脉两端位置
    f64 x1 = static_cast<f64>(origin.x) + std::sin(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    f64 x2 = static_cast<f64>(origin.x) - std::sin(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    f64 z1 = static_cast<f64>(origin.z) + std::cos(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    f64 z2 = static_cast<f64>(origin.z) - std::cos(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);

    f64 y1 = static_cast<f64>(origin.y + random.nextInt(-2, 2));
    f64 y2 = static_cast<f64>(origin.y + random.nextInt(-2, 2));

    // 计算边界框
    i32 minX = origin.x - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 minY = origin.y - 2 - halfSize;
    i32 minZ = origin.z - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 sizeX = 2 * (static_cast<i32>(std::ceil(sizeFactor)) + halfSize);
    i32 sizeY = 2 * (2 + halfSize);
    i32 sizeZ = sizeX;

    // 检查是否在有效范围内
    for (i32 checkX = minX; checkX <= minX + sizeX; ++checkX) {
        for (i32 checkZ = minZ; checkZ <= minZ + sizeZ; ++checkZ) {
            i32 topY = region.getTopBlockY(checkX, checkZ, HeightmapType::WorldSurfaceWG);
            if (minY <= topY) {
                i32 placedCount = 0;
                generateSphere(chunk, random, config,
                    x1, y1, z1, x2, y2, z2,
                    minX, minY, minZ, sizeX, sizeY, sizeZ,
                    placedCount);
                return placedCount > 0;
            }
        }
    }

    return false;
}

void OreFeature::generateSphere(
    ChunkPrimer& chunk,
    math::Random& random,
    const OreFeatureConfig& config,
    f64 x1, f64 y1, f64 z1,
    f64 x2, f64 y2, f64 z2,
    i32 minX, i32 minY, i32 minZ,
    i32 sizeX, i32 sizeY, i32 sizeZ,
    i32& placedCount)
{
    // 参考 MC OreFeature.func_207803_a()
    // 使用球形采样算法

    placedCount = 0;
    i32 totalSize = sizeX * sizeY * sizeZ;

    // 使用位数组跟踪已处理的方块
    std::vector<bool> processed(totalSize, false);

    // 计算每个"球心"的位置
    std::vector<f64> sphereCenters;
    sphereCenters.resize(static_cast<size_t>(config.size) * 4);

    for (i32 i = 0; i < config.size; ++i) {
        f32 progress = static_cast<f32>(i) / static_cast<f32>(config.size);

        f64 cx = x1 + (x2 - x1) * progress;
        f64 cy = y1 + (y2 - y1) * progress;
        f64 cz = z1 + (z2 - z1) * progress;

        f64 radiusFactor = random.nextDouble() * static_cast<f64>(config.size) / 16.0;
        f64 radius = (std::sin(static_cast<f32>(M_PI) * progress) + 1.0f) * radiusFactor + 1.0;

        sphereCenters[static_cast<size_t>(i) * 4 + 0] = cx;
        sphereCenters[static_cast<size_t>(i) * 4 + 1] = cy;
        sphereCenters[static_cast<size_t>(i) * 4 + 2] = cz;
        sphereCenters[static_cast<size_t>(i) * 4 + 3] = radius;
    }

    // 处理每个球心
    for (i32 i = 0; i < config.size - 1; ++i) {
        f64 radius1 = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius1 <= 0.0) {
            continue;
        }

        for (i32 j = i + 1; j < config.size; ++j) {
            f64 radius2 = sphereCenters[static_cast<size_t>(j) * 4 + 3];

            if (radius2 <= 0.0) {
                continue;
            }

            // 计算两球心之间的距离
            f64 dx = sphereCenters[static_cast<size_t>(i) * 4 + 0] - sphereCenters[static_cast<size_t>(j) * 4 + 0];
            f64 dy = sphereCenters[static_cast<size_t>(i) * 4 + 1] - sphereCenters[static_cast<size_t>(j) * 4 + 1];
            f64 dz = sphereCenters[static_cast<size_t>(i) * 4 + 2] - sphereCenters[static_cast<size_t>(j) * 4 + 2];

            f64 distSq = dx * dx + dy * dy + dz * dz;
            f64 radiusSum = radius1 + radius2;

            // 如果两球重叠太多，移除较小的那个
            if (radiusSum * radiusSum > distSq) {
                if (radius1 > radius2) {
                    sphereCenters[static_cast<size_t>(j) * 4 + 3] = -1.0;
                } else {
                    sphereCenters[static_cast<size_t>(i) * 4 + 3] = -1.0;
                    break;
                }
            }
        }
    }

    // 获取目标方块状态
    const BlockState* oreState = BlockRegistry::instance().get(config.state);
    if (!oreState) {
        return;
    }

    // 在每个球心周围放置矿石
    for (i32 i = 0; i < config.size; ++i) {
        f64 radius = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius < 0.0) {
            continue;
        }

        f64 cx = sphereCenters[static_cast<size_t>(i) * 4 + 0];
        f64 cy = sphereCenters[static_cast<size_t>(i) * 4 + 1];
        f64 cz = sphereCenters[static_cast<size_t>(i) * 4 + 2];

        // 计算边界
        i32 localMinX = std::max(static_cast<i32>(std::floor(cx - radius)), minX);
        i32 localMinY = std::max(static_cast<i32>(std::floor(cy - radius)), minY);
        i32 localMinZ = std::max(static_cast<i32>(std::floor(cz - radius)), minZ);
        i32 localMaxX = std::max(static_cast<i32>(std::floor(cx + radius)), localMinX);
        i32 localMaxY = std::max(static_cast<i32>(std::floor(cy + radius)), localMinY);
        i32 localMaxZ = std::max(static_cast<i32>(std::floor(cz + radius)), localMinZ);

        // 遍历边界内的每个方块
        for (i32 bx = localMinX; bx <= localMaxX; ++bx) {
            f64 dx = (static_cast<f64>(bx) + 0.5 - cx) / radius;

            if (dx * dx >= 1.0) {
                continue;
            }

            for (i32 by = localMinY; by <= localMaxY; ++by) {
                f64 dy = (static_cast<f64>(by) + 0.5 - cy) / radius;

                if (dx * dx + dy * dy >= 1.0) {
                    continue;
                }

                for (i32 bz = localMinZ; bz <= localMaxZ; ++bz) {
                    f64 dz = (static_cast<f64>(bz) + 0.5 - cz) / radius;

                    if (dx * dx + dy * dy + dz * dz >= 1.0) {
                        continue;
                    }

                    // 计算位数组索引
                    i32 index = (bx - minX) +
                               (by - minY) * sizeX +
                               (bz - minZ) * sizeX * sizeY;

                    if (index < 0 || index >= totalSize) {
                        continue;
                    }

                    if (processed[static_cast<size_t>(index)]) {
                        continue;
                    }

                    processed[static_cast<size_t>(index)] = true;

                    // 获取当前方块并检查是否可以替换
                    const BlockState* currentState = chunk.getBlock(
                        static_cast<BlockCoord>(bx & 15),
                        static_cast<BlockCoord>(by),
                        static_cast<BlockCoord>(bz & 15));

                    if (currentState && config.target->test(*currentState, random)) {
                        chunk.setBlock(
                            static_cast<BlockCoord>(bx & 15),
                            static_cast<BlockCoord>(by),
                            static_cast<BlockCoord>(bz & 15),
                            oreState);
                        ++placedCount;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredOreFeature 实现
// ============================================================================

ConfiguredOreFeature::ConfiguredOreFeature(
    std::unique_ptr<OreFeatureConfig> featureConfig,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(featureConfig))
    , m_placement(std::move(placement))
    , m_name(featureName) {}

bool ConfiguredOreFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    // 忽略 pos 参数，矿石生成使用自己的放置逻辑
    (void)pos;
    (void)generator;

    if (!m_placement || !m_config) {
        return false;
    }

    // 获取放置位置
    BlockPos chunkPos(chunk.x() * 16, 0, chunk.z() * 16);
    auto positions = m_placement->getPositions(region, random, chunkPos);

    // 在每个位置生成矿石
    OreFeature feature;
    bool placed = false;
    for (const BlockPos& orePos : positions) {
        if (feature.place(region, chunk, random, orePos, *m_config)) {
            placed = true;
        }
    }
    return placed;
}

void ConfiguredOreFeature::generate(WorldGenRegion& region, ChunkPrimer& chunk, math::Random& random) {
    if (!m_placement || !m_config) {
        return;
    }

    // 获取放置位置
    BlockPos chunkPos(chunk.x() * 16, 0, chunk.z() * 16);
    auto positions = m_placement->getPositions(region, random, chunkPos);

    // 在每个位置生成矿石
    OreFeature feature;
    for (const BlockPos& pos : positions) {
        feature.place(region, chunk, random, pos, *m_config);
    }
}

// ============================================================================
// OreFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredOreFeature>> OreFeatures::s_features;

void OreFeatures::initialize() {
    s_features.clear();

    // 主世界矿石
    s_features.push_back(createCoalOre());
    s_features.push_back(createIronOre());
    s_features.push_back(createGoldOre());
    s_features.push_back(createRedstoneOre());
    s_features.push_back(createDiamondOre());
    s_features.push_back(createLapisOre());
    s_features.push_back(createEmeraldOre());
    s_features.push_back(createCopperOre());
}

const std::vector<std::unique_ptr<ConfiguredOreFeature>>& OreFeatures::getAllFeatures() {
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredOreFeature>> OreFeatures::getAllFeaturesAndClear() {
    std::vector<std::unique_ptr<ConfiguredOreFeature>> result;
    result.swap(s_features);
    return result;
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createCoalOre() {
    // 煤矿：Y 0-127，每区块20个，矿脉大小17
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::CoalOre,
        17);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(20);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "coal_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createIronOre() {
    // 铁矿：Y 0-63，每区块20个，矿脉大小9
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::IronOre,
        9);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(20);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "iron_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createGoldOre() {
    // 金矿：Y 0-31，每区块2个，矿脉大小9
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::GoldOre,
        9);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "gold_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createRedstoneOre() {
    // 红石：Y 0-15，每区块8个，矿脉大小8
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::RedstoneOre,
        8);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(8);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "redstone_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createDiamondOre() {
    // 钻石：Y 0-15，每区块1个，矿脉大小8
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::DiamondOre,
        8);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "diamond_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createLapisOre() {
    // 青金石：深度平均分布，每区块1个，矿脉大小7
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::LapisOre,
        7);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "lapis_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createEmeraldOre() {
    // 绿宝石：Y 4-31，每区块1个，矿脉大小1（山地生物群系特有）
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::EmeraldOre,
        1);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "emerald_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createCopperOre() {
    // 铜矿：Y 0-96，每区块6个，矿脉大小10（1.17+新增）
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::CopperOre,
        10);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(6);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "copper_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createNetherQuartzOre() {
    // 下界石英：Y 10-117，每区块16个，矿脉大小14
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack),
        BlockId::NetherQuartzOre,
        14);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(16);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "nether_quartz_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createNetherGoldOre() {
    // 下界金矿：Y 10-117，每区块10个，矿脉大小10
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack),
        BlockId::NetherGoldOre,
        10);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(10);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "nether_gold_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createAncientDebris() {
    // 远古残骸：Y 8-22，每区块2个，矿脉大小3
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack),
        BlockId::AncientDebris,
        3);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);

    auto placement = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement),
        std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "ancient_debris");
}

} // namespace mr
