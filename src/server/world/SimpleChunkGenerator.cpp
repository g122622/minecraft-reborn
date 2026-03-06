#include "SimpleChunkGenerator.hpp"
#include "../../common/world/WorldConstants.hpp"

namespace mr::server {

// ============================================================================
// 构造函数
// ============================================================================

SimpleChunkGenerator::SimpleChunkGenerator(u64 seed)
    : m_seed(seed)
    , m_settings(DimensionSettings::overworld())
    , m_terrainGen(TerrainGenFactory::createStandard(seed))
    , m_tempChunkData(std::make_unique<ChunkData>(0, 0))
{
}

SimpleChunkGenerator::SimpleChunkGenerator(std::unique_ptr<ITerrainGenerator> terrainGen)
    : m_seed(terrainGen ? terrainGen->getConfig().seed : 0)
    , m_settings(DimensionSettings::overworld())
    , m_terrainGen(std::move(terrainGen))
    , m_tempChunkData(std::make_unique<ChunkData>(0, 0))
{
}

// ============================================================================
// IChunkGenerator 接口实现
// ============================================================================

void SimpleChunkGenerator::generateBiomes(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // ITerrainGenerator 不单独生成生物群系，在 generateChunk 中统一处理
    // 标记为已完成
    chunk.setChunkStatus(ChunkStatus::BIOMES);
}

void SimpleChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    if (!m_terrainGen) {
        return;
    }

    // 创建临时 ChunkData 用于 ITerrainGenerator
    m_tempChunkData = std::make_unique<ChunkData>(chunk.x(), chunk.z());

    // 生成地形（ITerrainGenerator::generateChunk 包含噪声和基础地形）
    m_terrainGen->generateChunk(*m_tempChunkData);

    // 将数据复制到 ChunkPrimer
    for (i32 y = 0; y < world::CHUNK_HEIGHT; ++y) {
        i32 sectionIndex = y / world::CHUNK_SECTION_HEIGHT;
        if (!chunk.hasSection(sectionIndex)) {
            chunk.createSection(sectionIndex);
        }

        ChunkSection* section = chunk.getSection(sectionIndex);
        if (!section) continue;

        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                const BlockState* state = m_tempChunkData->getBlock(x, y, z);
                if (state) {
                    chunk.setBlock(x, y, z, state);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatus::NOISE);
}

void SimpleChunkGenerator::buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // ITerrainGenerator 的 generateChunk 已经包含了地表生成
    chunk.setChunkStatus(ChunkStatus::SURFACE);
}

void SimpleChunkGenerator::applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& chunk, bool /*isLiquid*/)
{
    // ITerrainGenerator::generateChunk 已经包含了洞穴生成
    // 如果需要单独处理液体雕刻，可以扩展
    chunk.setChunkStatus(ChunkStatus::CARVERS);
}

void SimpleChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    if (!m_terrainGen) {
        chunk.setChunkStatus(ChunkStatus::FEATURES);
        return;
    }

    // 需要获取邻居区块用于装饰
    ChunkData* neighbors[8] = {nullptr};

    // 从 region 获取邻居（如果存在）
    // WorldGenRegion 的邻居顺序：0=NW, 1=N, 2=NE, 3=W, 4=中心, 5=E, 6=SW, 7=S, 8=SE
    for (i32 i = 0; i < 8; ++i) {
        i32 regionIndex = i < 4 ? i : i + 1;  // 跳过中心区块（索引4）
        if (IChunk* neighborChunk = region.getChunk(
            (regionIndex % 3) - 1,  // relX: -1, 0, 1
            (regionIndex / 3) - 1   // relZ: -1, 0, 1
        )) {
            // 尝试从 ChunkPrimer 转换获取 ChunkData
            // 注意：这里简化处理，实际可能需要更复杂的转换
            neighbors[i] = nullptr;  // 简化：邻居装饰暂时跳过
        }
    }

    // 创建临时 ChunkData 用于装饰
    if (m_tempChunkData && m_tempChunkData->x() == chunk.x() && m_tempChunkData->z() == chunk.z()) {
        // 使用已生成的数据
        m_terrainGen->populateChunk(*m_tempChunkData, neighbors);

        // 将装饰后的数据复制回 ChunkPrimer
        for (i32 y = 0; y < world::CHUNK_HEIGHT; ++y) {
            i32 sectionIndex = y / world::CHUNK_SECTION_HEIGHT;
            if (!chunk.hasSection(sectionIndex)) {
                chunk.createSection(sectionIndex);
            }

            ChunkSection* section = chunk.getSection(sectionIndex);
            if (!section) continue;

            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                    const BlockState* state = m_tempChunkData->getBlock(x, y, z);
                    if (state) {
                        chunk.setBlock(x, y, z, state);
                    }
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatus::FEATURES);
}

BiomeId SimpleChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    if (!m_terrainGen) {
        return Biomes::Plains;
    }

    // 从 ITerrainGenerator 获取生物群系类型
    BiomeType biomeType = m_terrainGen->getBiome(x, z);

    // 转换 BiomeType 到 BiomeId
    // 简化映射：直接使用枚举值作为 ID
    return static_cast<BiomeId>(biomeType);
}

BiomeId SimpleChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    // 噪声坐标是 4x4 方块一个大块
    i32 blockX = noiseX * 4;
    i32 blockZ = noiseZ * 4;
    return getBiome(blockX, noiseY, blockZ);
}

i32 SimpleChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    if (!m_terrainGen) {
        return m_settings.seaLevel;
    }

    return m_terrainGen->getHeight(x, z);
}

} // namespace mr::server
