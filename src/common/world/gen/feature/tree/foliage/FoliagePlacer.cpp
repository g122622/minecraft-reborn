#include "FoliagePlacer.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

FoliagePlacer::FoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : m_radius(radius)
    , m_offset(offset)
{
}

void FoliagePlacer::placeFoliage(
    WorldGenRegion& world,
    math::Random& random,
    i32 trunkHeight,
    const std::vector<FoliagePosition>& foliagePositions,
    const std::set<BlockPos>& /*trunkBlocks*/,
    i32 /*trunkOffset*/,
    BlockId foliageBlock
) {
    std::set<BlockPos> foliageBlocks;

    for (const auto& foliagePos : foliagePositions) {
        i32 radius = m_radius.get(random);
        i32 offset = m_offset.get(random);
        i32 foliageHeight = getFoliageHeight(random, trunkHeight);

        placeFoliageInternal(
            world, random, trunkHeight, foliagePos,
            foliageHeight, radius, offset, foliageBlocks, foliageBlock
        );
    }
}

void FoliagePlacer::placeFoliageLayer(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& centerPos,
    i32 radius,
    std::set<BlockPos>& foliageBlocks,
    i32 y,
    bool trunkTop,
    BlockId foliageBlock
) {
    // 遍历半径范围内的所有方块
    i32 radiusOffset = trunkTop ? 1 : 0;
    BlockPos pos;

    for (i32 dx = -radius; dx <= radius + radiusOffset; ++dx) {
        for (i32 dz = -radius; dz <= radius + radiusOffset; ++dz) {
            // 检查是否跳过该位置
            if (shouldSkip(random, dx, 0, dz, radius, trunkTop)) {
                continue;
            }

            pos.x = centerPos.x + dx;
            pos.y = y;
            pos.z = centerPos.z + dz;

            // 检查位置是否在有效范围内
            if (pos.y < 0 || pos.y >= 256) {
                continue;
            }

            // 检查是否可以放置树叶
            const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
            if (state == nullptr) {
                continue;
            }

            u32 blockId = state->blockId();
            if (blockId != static_cast<u32>(BlockId::Air) &&
                blockId != static_cast<u32>(BlockId::OakLeaves) &&
                blockId != static_cast<u32>(BlockId::SpruceLeaves) &&
                blockId != static_cast<u32>(BlockId::BirchLeaves) &&
                blockId != static_cast<u32>(BlockId::JungleLeaves) &&
                blockId != static_cast<u32>(BlockId::AcaciaLeaves) &&
                blockId != static_cast<u32>(BlockId::DarkOakLeaves)) {
                continue;
            }

            // 放置树叶
            auto& registry = BlockRegistry::instance();
            const BlockState* foliageState = registry.get(foliageBlock);
            if (foliageState != nullptr) {
                world.setBlock(pos.x, pos.y, pos.z, foliageState);
                foliageBlocks.insert(pos);
            }
        }
    }
}

} // namespace mc
