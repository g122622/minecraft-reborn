#include "TrunkPlacer.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

TrunkPlacer::TrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : m_baseHeight(baseHeight)
    , m_heightRandA(heightRandA)
    , m_heightRandB(heightRandB)
{
}

i32 TrunkPlacer::getHeight(math::Random& random) const {
    i32 height = m_baseHeight;
    if (m_heightRandA > 0) {
        height += random.nextInt(0, m_heightRandA);
    }
    if (m_heightRandB > 0) {
        height += random.nextInt(0, m_heightRandB);
    }
    return height;
}

void TrunkPlacer::placeBlock(
    WorldGenRegion& world,
    const BlockPos& pos,
    std::set<BlockPos>& trunkBlocks,
    BlockId trunkBlock
) {
    // 检查是否在有效范围内
    if (pos.y < 0 || pos.y >= 256) {
        return;
    }

    // 获取方块注册表
    auto& registry = BlockRegistry::instance();
    const BlockState* state = registry.get(trunkBlock);
    if (state == nullptr) {
        return;
    }

    // 设置方块
    world.setBlock(pos.x, pos.y, pos.z, state);

    // 记录树干方块位置
    trunkBlocks.insert(pos);
}

bool TrunkPlacer::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) {
    // 检查位置是否在有效范围内
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    // 获取当前位置的方块
    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return true;  // 空气或其他可替换方块
    }

    // 检查是否是空气或树叶
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

void TrunkPlacer::placeDirtUnder(WorldGenRegion& world, const BlockPos& pos) {
    // 检查下方位置
    BlockPos belowPos = pos.down();
    if (belowPos.y < 0) {
        return;
    }

    // 获取下方方块
    const BlockState* state = world.getBlock(belowPos.x, belowPos.y, belowPos.z);
    if (state == nullptr) {
        return;
    }

    u32 blockId = state->blockId();

    // 如果不是草方块或泥土，放置泥土
    if (blockId != static_cast<u32>(BlockId::Grass) && blockId != static_cast<u32>(BlockId::Dirt)) {
        auto& registry = BlockRegistry::instance();
        const BlockState* dirt = registry.get(BlockId::Dirt);
        if (dirt != nullptr) {
            world.setBlock(belowPos.x, belowPos.y, belowPos.z, dirt);
        }
    }
}

} // namespace mc
