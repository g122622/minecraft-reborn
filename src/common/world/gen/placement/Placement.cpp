#include "Placement.hpp"
#include "../../../math/MathUtils.hpp"
#include <cmath>

namespace mr {

// ============================================================================
// HeightRangePlacementConfig 实现
// ============================================================================

i32 HeightRangePlacementConfig::getRandomY(math::Random& random) const {
    // 在 [bottomOffset, maximum - topOffset) 范围内随机选择
    i32 range = maximum - topOffset - bottomOffset;
    if (range <= 0) {
        return bottomOffset;
    }
    return bottomOffset + random.nextInt(0, range - 1);
}

// ============================================================================
// BiomePlacementConfig 实现
// ============================================================================

bool BiomePlacementConfig::isAllowed(u32 biomeId) const {
    for (u32 id : allowedBiomes) {
        if (id == biomeId) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// ConfiguredPlacement 实现
// ============================================================================

ConfiguredPlacement::ConfiguredPlacement(
    std::unique_ptr<Placement> placement,
    std::unique_ptr<IPlacementConfig> config)
    : m_placement(std::move(placement))
    , m_config(std::move(config))
    , m_next(nullptr) {}

std::vector<BlockPos> ConfiguredPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const BlockPos& basePos) const
{
    std::vector<BlockPos> positions = m_placement->getPositions(region, random, *m_config, basePos);

    // 链式处理
    if (m_next) {
        std::vector<BlockPos> result;
        for (const BlockPos& pos : positions) {
            auto nextPositions = m_next->getPositions(region, random, pos);
            result.insert(result.end(), nextPositions.begin(), nextPositions.end());
        }
        return result;
    }

    return positions;
}

std::unique_ptr<ConfiguredPlacement> ConfiguredPlacement::then(
    std::unique_ptr<Placement> placement,
    std::unique_ptr<IPlacementConfig> config) const
{
    auto next = std::make_unique<ConfiguredPlacement>(
        std::move(placement),
        std::move(config));

    // 复制当前配置并设置链
    // 注意：这里简化处理，实际MC中是创建新的DecoratedPlacement
    return next;
}

// ============================================================================
// CountPlacement 实现
// ============================================================================

std::vector<BlockPos> CountPlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    (void)random;
    const auto& countConfig = static_cast<const CountPlacementConfig&>(config);
    std::vector<BlockPos> positions;

    for (i32 i = 0; i < countConfig.count; ++i) {
        positions.push_back(basePos);
    }

    return positions;
}

// ============================================================================
// HeightRangePlacement 实现
// ============================================================================

std::vector<BlockPos> HeightRangePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    const auto& heightConfig = static_cast<const HeightRangePlacementConfig&>(config);

    i32 y = heightConfig.getRandomY(random);
    return { BlockPos(basePos.x, y, basePos.z) };
}

// ============================================================================
// SquarePlacement 实现
// ============================================================================

std::vector<BlockPos> SquarePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    (void)config;

    // 在XZ平面内随机分散（0-15范围内）
    i32 x = basePos.x + random.nextInt(0, 15);
    i32 z = basePos.z + random.nextInt(0, 15);

    return { BlockPos(x, basePos.y, z) };
}

// ============================================================================
// BiomePlacement 实现
// ============================================================================

std::vector<BlockPos> BiomePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)random;
    const auto& biomeConfig = static_cast<const BiomePlacementConfig&>(config);

    // 获取位置处的生物群系
    // TODO: 需要WorldGenRegion提供getBiome方法
    // 暂时返回空，让矿石不在任何地方生成
    (void)region;
    (void)biomeConfig;

    return { basePos };
}

// ============================================================================
// ChancePlacement 实现
// ============================================================================

std::vector<BlockPos> ChancePlacement::getPositions(
    WorldGenRegion& region,
    math::Random& random,
    const IPlacementConfig& config,
    const BlockPos& basePos) const
{
    (void)region;
    const auto& chanceConfig = static_cast<const ChancePlacementConfig&>(config);

    if (random.nextFloat() < chanceConfig.chance) {
        return { basePos };
    }
    return {};
}

} // namespace mr
