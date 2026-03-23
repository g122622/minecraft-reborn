#include "TrunkPlacers.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// DarkOakTrunkPlacer 实现
// ============================================================================

DarkOakTrunkPlacer::DarkOakTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{
}

std::vector<FoliagePosition> DarkOakTrunkPlacer::placeTrunk(
    WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 深色橡树使用 2x2 树干
    for (i32 y = 0; y < height; ++y) {
        placeTrunkLayer2x2(world, BlockPos(startPos.x, startPos.y + y, startPos.z),
                           trunkBlocks, trunkBlock);
    }

    // 在树干顶部添加多个树叶位置
    i32 topY = startPos.y + height;
    foliagePositions.emplace_back(BlockPos(startPos.x, topY, startPos.z), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x + 1, topY, startPos.z), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x, topY, startPos.z + 1), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x + 1, topY, startPos.z + 1), 2, 0, true);

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> DarkOakTrunkPlacer::clone() const {
    return std::make_unique<DarkOakTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

// ============================================================================
// FancyTrunkPlacer 实现
// ============================================================================

FancyTrunkPlacer::FancyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{
}

std::vector<FoliagePosition> FancyTrunkPlacer::placeTrunk(
    WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 弯曲树干
    f32 x = static_cast<f32>(startPos.x);
    f32 z = static_cast<f32>(startPos.z);
    f32 xDir = (random.nextFloat() - 0.5f) * 0.3f;
    f32 zDir = (random.nextFloat() - 0.5f) * 0.3f;

    for (i32 y = 0; y < height; ++y) {
        x += xDir;
        z += zDir;

        // 随机调整方向
        xDir += (random.nextFloat() - 0.5f) * 0.1f;
        zDir += (random.nextFloat() - 0.5f) * 0.1f;

        // 限制偏移
        xDir = std::clamp(xDir, -0.5f, 0.5f);
        zDir = std::clamp(zDir, -0.5f, 0.5f);

        i32 blockX = static_cast<i32>(std::floor(x));
        i32 blockZ = static_cast<i32>(std::floor(z));

        placeBlock(world, BlockPos(blockX, startPos.y + y, blockZ), trunkBlocks, trunkBlock);

        // 在某些高度添加树叶位置
        if (y > height / 2 && random.nextInt(3) == 0) {
            foliagePositions.emplace_back(
                BlockPos(blockX, startPos.y + y + 2, blockZ),
                2 + random.nextInt(2), 0, false);
        }
    }

    // 顶部树叶
    foliagePositions.emplace_back(
        BlockPos(static_cast<i32>(std::floor(x)), startPos.y + height, static_cast<i32>(std::floor(z))),
        2, 0, true);

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> FancyTrunkPlacer::clone() const {
    return std::make_unique<FancyTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

// ============================================================================
// ForkyTrunkPlacer 实现
// ============================================================================

ForkyTrunkPlacer::ForkyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{
}

std::vector<FoliagePosition> ForkyTrunkPlacer::placeTrunk(
    WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 主树干
    for (i32 y = 0; y < height; ++y) {
        placeBlock(world, BlockPos(startPos.x, startPos.y + y, startPos.z), trunkBlocks, trunkBlock);
    }

    // 在上半部分生成分叉
    i32 branchStartY = startPos.y + height / 2;
    i32 numBranches = 2 + random.nextInt(3);

    for (i32 i = 0; i < numBranches; ++i) {
        // 分叉起点
        i32 branchY = branchStartY + random.nextInt(height / 2);
        BlockPos branchStart(startPos.x, branchY, startPos.z);

        // 分叉长度和方向
        i32 branchLength = 2 + random.nextInt(4);
        BlockPos branchEnd = generateBranch(world, random, branchStart, branchLength, trunkBlocks, trunkBlock);

        // 在分叉末端添加树叶
        foliagePositions.emplace_back(
            BlockPos(branchEnd.x, branchEnd.y + 1, branchEnd.z),
            2 + random.nextInt(2), 0, true);
    }

    // 顶部树叶
    foliagePositions.emplace_back(
        BlockPos(startPos.x, startPos.y + height, startPos.z),
        2, 0, true);

    return foliagePositions;
}

BlockPos ForkyTrunkPlacer::generateBranch(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& startPos,
    i32 length,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    // 随机方向
    i32 dx = random.nextInt(3) - 1;  // -1, 0, 1
    i32 dz = random.nextInt(3) - 1;

    // 确保方向不为零
    if (dx == 0 && dz == 0) {
        dx = random.nextBoolean() ? 1 : -1;
    }

    i32 x = startPos.x;
    i32 y = startPos.y;
    i32 z = startPos.z;

    for (i32 i = 0; i < length; ++i) {
        x += dx;
        z += dz;
        // 分叉略微向上生长
        if (random.nextInt(3) == 0) {
            y += 1;
        }

        placeBlock(world, BlockPos(x, y, z), trunkBlocks, trunkBlock);
    }

    return BlockPos(x, y, z);
}

std::unique_ptr<TrunkPlacer> ForkyTrunkPlacer::clone() const {
    return std::make_unique<ForkyTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

// ============================================================================
// GiantTrunkPlacer 实现
// ============================================================================

GiantTrunkPlacer::GiantTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{
}

std::vector<FoliagePosition> GiantTrunkPlacer::placeTrunk(
    WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 2x2 树干
    for (i32 y = 0; y < height; ++y) {
        placeTrunkLayer2x2(world, BlockPos(startPos.x, startPos.y + y, startPos.z),
                           trunkBlocks, trunkBlock);
    }

    // 顶部多个树叶位置
    i32 topY = startPos.y + height;
    foliagePositions.emplace_back(BlockPos(startPos.x, topY - 3, startPos.z), 3, 0, false);
    foliagePositions.emplace_back(BlockPos(startPos.x, topY - 1, startPos.z), 2, 0, true);

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> GiantTrunkPlacer::clone() const {
    return std::make_unique<GiantTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

// ============================================================================
// MegaJungleTrunkPlacer 实现
// ============================================================================

MegaJungleTrunkPlacer::MegaJungleTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : TrunkPlacer(baseHeight, heightRandA, heightRandB)
{
}

std::vector<FoliagePosition> MegaJungleTrunkPlacer::placeTrunk(
    WorldGenRegion& world,
    math::Random& random,
    i32 height,
    const BlockPos& startPos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock)
{
    std::vector<FoliagePosition> foliagePositions;

    // 2x2 树干
    for (i32 y = 0; y < height; ++y) {
        placeTrunkLayer2x2(world, BlockPos(startPos.x, startPos.y + y, startPos.z),
                           trunkBlocks, trunkBlock);
    }

    // 在树干上生成分支
    // (简化版本：不生成藤蔓，仅添加树叶位置)

    // 顶部树叶
    i32 topY = startPos.y + height;
    foliagePositions.emplace_back(BlockPos(startPos.x, topY, startPos.z), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x + 1, topY, startPos.z), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x, topY, startPos.z + 1), 2, 0, true);
    foliagePositions.emplace_back(BlockPos(startPos.x + 1, topY, startPos.z + 1), 2, 0, true);

    return foliagePositions;
}

std::unique_ptr<TrunkPlacer> MegaJungleTrunkPlacer::clone() const {
    return std::make_unique<MegaJungleTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
}

} // namespace mc
