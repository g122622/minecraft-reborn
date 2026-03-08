#include "TreeFeature.hpp"
#include "trunk/StraightTrunkPlacer.hpp"
#include "foliage/BlobFoliagePlacer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../core/Types.hpp"

namespace mr {

bool TreeFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& startPos,
    const TreeFeatureConfig& config
) {
    if (config.trunkPlacer == nullptr || config.foliagePlacer == nullptr) {
        return false;
    }

    // 获取树干高度
    i32 trunkHeight = config.trunkPlacer->getHeight(random);

    // 检查高度是否有效
    if (trunkHeight < config.minHeight) {
        return false;
    }

    // 检查起始位置是否在有效范围内
    if (startPos.y < 1 || startPos.y + trunkHeight + 1 >= 256) {
        return false;
    }

    // 检查起始位置下方是否是泥土或草地
    if (!isDirtOrFarmlandAt(world, startPos.down())) {
        return false;
    }

    // 检查是否有足够的空间放置树干
    i32 availableHeight = calculateAvailableHeight(world, trunkHeight, startPos, config);
    if (availableHeight < trunkHeight) {
        return false;
    }

    // 放置树干
    std::set<BlockPos> trunkBlocks;
    std::vector<FoliagePosition> foliagePositions = config.trunkPlacer->placeTrunk(
        world, random, trunkHeight, startPos, trunkBlocks, config.trunkBlock
    );

    if (foliagePositions.empty()) {
        return false;
    }

    // 放置树叶
    i32 foliageHeight = config.foliagePlacer->getFoliageHeight(random, trunkHeight);
    config.foliagePlacer->placeFoliage(
        world, random, trunkHeight, foliagePositions, trunkBlocks,
        trunkHeight - 1, config.foliageBlock
    );

    return true;
}

bool TreeFeature::isReplaceableAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return true;
    }

    u32 blockId = state->blockId();

    // 空气、树叶、草、花等可替换
    if (blockId == static_cast<u32>(BlockId::Air)) {
        return true;
    }

    // 检查是否是树叶
    if (blockId == static_cast<u32>(BlockId::OakLeaves) ||
        blockId == static_cast<u32>(BlockId::SpruceLeaves) ||
        blockId == static_cast<u32>(BlockId::BirchLeaves) ||
        blockId == static_cast<u32>(BlockId::JungleLeaves) ||
        blockId == static_cast<u32>(BlockId::AcaciaLeaves) ||
        blockId == static_cast<u32>(BlockId::DarkOakLeaves)) {
        return true;
    }

    // 检查是否是植被
    if (blockId == static_cast<u32>(BlockId::ShortGrass) ||
        blockId == static_cast<u32>(BlockId::TallGrass) ||
        blockId == static_cast<u32>(BlockId::Fern) ||
        blockId == static_cast<u32>(BlockId::Dandelion) ||
        blockId == static_cast<u32>(BlockId::Poppy) ||
        blockId == static_cast<u32>(BlockId::OakSapling) ||
        blockId == static_cast<u32>(BlockId::SpruceSapling) ||
        blockId == static_cast<u32>(BlockId::BirchSapling) ||
        blockId == static_cast<u32>(BlockId::JungleSapling) ||
        blockId == static_cast<u32>(BlockId::AcaciaSapling) ||
        blockId == static_cast<u32>(BlockId::DarkOakSapling)) {
        return true;
    }

    // 检查是否是水
    if (blockId == static_cast<u32>(BlockId::Water)) {
        return true;
    }

    return false;
}

bool TreeFeature::isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return true;
    }

    u32 blockId = state->blockId();

    if (blockId == static_cast<u32>(BlockId::Air)) {
        return true;
    }

    // 检查是否是树叶
    if (blockId == static_cast<u32>(BlockId::OakLeaves) ||
        blockId == static_cast<u32>(BlockId::SpruceLeaves) ||
        blockId == static_cast<u32>(BlockId::BirchLeaves) ||
        blockId == static_cast<u32>(BlockId::JungleLeaves) ||
        blockId == static_cast<u32>(BlockId::AcaciaLeaves) ||
        blockId == static_cast<u32>(BlockId::DarkOakLeaves)) {
        return true;
    }

    return false;
}

bool TreeFeature::isDirtOrFarmlandAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    u32 blockId = state->blockId();

    // 检查是否是泥土类方块
    return blockId == static_cast<u32>(BlockId::Dirt) ||
           blockId == static_cast<u32>(BlockId::Grass) ||
           blockId == static_cast<u32>(BlockId::CoarseDirt) ||
           blockId == static_cast<u32>(BlockId::Podzol);
}

bool TreeFeature::isWaterAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    return state->blockId() == static_cast<u32>(BlockId::Water);
}

i32 TreeFeature::calculateAvailableHeight(
    WorldGenRegion& world,
    i32 maxHeight,
    const BlockPos& startPos,
    const TreeFeatureConfig& config
) const {
    BlockPos pos;

    for (i32 y = 0; y <= maxHeight + 1; ++y) {
        // 计算检查半径（随高度变化）
        i32 checkRadius = 0;  // 简化：只检查中心

        for (i32 dx = -checkRadius; dx <= checkRadius; ++dx) {
            for (i32 dz = -checkRadius; dz <= checkRadius; ++dz) {
                pos.x = startPos.x + dx;
                pos.y = startPos.y + y;
                pos.z = startPos.z + dz;

                if (!isReplaceableAt(world, pos)) {
                    // 找到不可替换的方块，返回当前可用高度
                    return y - 2;
                }
            }
        }
    }

    return maxHeight;
}

void TreeFeature::setFoliageDistance(
    WorldGenRegion& world,
    const std::set<BlockPos>& trunkBlocks,
    const std::set<BlockPos>& foliageBlocks
) {
    // TODO: 实现树叶距离设置
    // 这用于树叶的腐烂机制
    // 当树叶距离树干太远时会自动腐烂
}

// ============================================================================
// 预定义树木配置
// ============================================================================

TreeFeatureConfig TreeFeatures::oak() {
    TreeFeatureConfig config;
    config.trunkBlock = BlockId::OakLog;
    config.foliageBlock = BlockId::OakLeaves;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        3
    );
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::birch() {
    TreeFeatureConfig config;
    config.trunkBlock = BlockId::BirchLog;
    config.foliageBlock = BlockId::BirchLeaves;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2
    );
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::spruce() {
    TreeFeatureConfig config;
    config.trunkBlock = BlockId::SpruceLog;
    config.foliageBlock = BlockId::SpruceLeaves;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 1);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        3
    );
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::jungle() {
    TreeFeatureConfig config;
    config.trunkBlock = BlockId::JungleLog;
    config.foliageBlock = BlockId::JungleLeaves;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 8, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2
    );
    config.minHeight = 4;
    return config;
}

} // namespace mr
