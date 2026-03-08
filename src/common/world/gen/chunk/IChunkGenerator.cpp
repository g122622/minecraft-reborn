#include "IChunkGenerator.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../WorldConstants.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// WorldGenRegion 实现
// ============================================================================

WorldGenRegion::WorldGenRegion(ChunkCoord mainX, ChunkCoord mainZ, std::array<IChunk*, 9> chunks)
    : m_mainX(mainX)
    , m_mainZ(mainZ)
    , m_chunks(chunks)
{
}

IChunk* WorldGenRegion::getChunk(i32 relX, i32 relZ)
{
    // 边界检查
    if (relX < -1 || relX > 1 || relZ < -1 || relZ > 1) {
        return nullptr;
    }

    // 转换为索引（中心是 4）
    const i32 index = (relZ + 1) * 3 + (relX + 1);
    return m_chunks[index];
}

const IChunk* WorldGenRegion::getChunk(i32 relX, i32 relZ) const
{
    if (relX < -1 || relX > 1 || relZ < -1 || relZ > 1) {
        return nullptr;
    }

    const i32 index = (relZ + 1) * 3 + (relX + 1);
    return m_chunks[index];
}

const BlockState* WorldGenRegion::getBlock(i32 x, i32 y, i32 z) const
{
    // 检查 Y 边界
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return BlockRegistry::instance().airState();
    }

    // 转换为区块坐标和本地坐标
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    const IChunk* chunk = getChunk(relX, relZ);
    if (!chunk) {
        return BlockRegistry::instance().airState();
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getBlock(localX, y, localZ);
}

void WorldGenRegion::setBlock(i32 x, i32 y, i32 z, const BlockState* state)
{
    // 检查 Y 边界
    if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
        return;
    }

    // 转换为区块坐标和本地坐标
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    IChunk* chunk = getChunk(relX, relZ);
    if (!chunk) {
        return;
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    chunk->setBlock(localX, y, localZ, state);
}

BiomeId WorldGenRegion::getBiome(i32 x, i32 y, i32 z) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    const IChunk* chunk = getChunk(relX, relZ);
    if (!chunk) {
        return Biomes::Plains;
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getBiomeAtBlock(localX, y, localZ);
}

i32 WorldGenRegion::getTopBlockY(i32 x, i32 z, HeightmapType type) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    const IChunk* chunk = getChunk(relX, relZ);
    if (!chunk) {
        return 0;
    }

    const i32 localX = world::toLocalCoord(x);
    const i32 localZ = world::toLocalCoord(z);
    return chunk->getTopBlockY(type, localX, localZ);
}

i32 WorldGenRegion::worldToChunkIndex(i32 x, i32 z) const
{
    const ChunkCoord chunkX = world::toChunkCoord(x);
    const ChunkCoord chunkZ = world::toChunkCoord(z);
    const i32 relX = chunkX - m_mainX;
    const i32 relZ = chunkZ - m_mainZ;

    if (relX < -1 || relX > 1 || relZ < -1 || relZ > 1) {
        return -1;
    }

    return (relZ + 1) * 3 + (relX + 1);
}

void WorldGenRegion::worldToLocal(i32 worldX, i32 worldZ, i32& localX, i32& localZ)
{
    localX = world::toLocalCoord(worldX);
    localZ = world::toLocalCoord(worldZ);
}

// ============================================================================
// BaseChunkGenerator 实现
// ============================================================================

BaseChunkGenerator::BaseChunkGenerator(u64 seed, DimensionSettings settings)
    : m_seed(seed)
    , m_settings(std::move(settings))
{
}

void BaseChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 默认实现：设置默认生物群系
    BiomeContainer& biomes = chunk.getBiomes();

    for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
        for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
            for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
                biomes.setBiome(x << 2, y << 2, z << 2, m_defaultBiome);
            }
        }
    }
}

void BaseChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid)
{
    // 默认实现：无雕刻
    // 子类可以覆盖以添加洞穴和峡谷生成
}

void BaseChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 默认实现：无特性
    // 子类可以覆盖以添加树木、矿石等
}

} // namespace mr
