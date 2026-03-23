#include "Structure.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../../IWorldWriter.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../../util/math/random/Random.hpp"
#include <algorithm>

namespace mc::world::gen::structure {

bool Structure::isValidBiome(BiomeId biomeId) const {
    const auto& biomes = validBiomes();
    return std::find(biomes.begin(), biomes.end(), biomeId) != biomes.end();
}

bool Structure::canGenerate(
    IWorld& /*world*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/)
{
    // 默认实现：总是可以生成
    // 子类可以覆盖此方法以添加额外的检查
    return true;
}

std::unique_ptr<StructureStart> Structure::generate(
    IWorldWriter& /*world*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*rng*/,
    i32 chunkX,
    i32 chunkZ) const
{
    // 默认实现：创建一个空的结构起点
    // 子类应该覆盖此方法以生成实际的结构片段
    return std::make_unique<StructureStart>(chunkX, chunkZ);
}

void Structure::placeInChunk(
    IWorldWriter& world,
    ChunkPrimer& chunk,
    StructureStart& start,
    i32 chunkX,
    i32 chunkZ) const
{
    // 计算区块边界框
    StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);

    // 创建随机数生成器
    math::Random rng = createRandom(
        static_cast<i64>(chunkX) * 3418731287LL ^ static_cast<i64>(chunkZ) * 132897987541LL,
        chunkX, chunkZ, 0);

    // 遍历所有片段，放置在当前区块内的部分
    for (const auto& piece : start.pieces()) {
        if (piece->intersectsChunk(chunkX, chunkZ)) {
            piece->generate(world, rng, chunkX, chunkZ, chunkBounds);
        }
    }
}

bool Structure::findStructureStart(
    i64 seed, i32 chunkX, i32 chunkZ,
    const StructureSeparationSettings& settings,
    i32& outStartX, i32& outStartZ)
{
    i32 spacing = settings.spacing;
    i32 separation = settings.separation;

    if (spacing <= 0) {
        return false;
    }

    // 计算网格坐标
    i32 gridX = static_cast<i32>(std::floor(static_cast<f32>(chunkX) / static_cast<f32>(spacing)));
    i32 gridZ = static_cast<i32>(std::floor(static_cast<f32>(chunkZ) / static_cast<f32>(spacing)));

    // 创建随机数生成器
    math::Random rng(seed + gridX * 3418731287LL + gridZ * 132897987541LL + settings.salt);

    // 计算偏移
    i32 offsetX = rng.nextInt(spacing - separation);
    i32 offsetZ = rng.nextInt(spacing - separation);

    // 计算起始区块坐标
    outStartX = gridX * spacing + offsetX;
    outStartZ = gridZ * spacing + offsetZ;

    return outStartX == chunkX && outStartZ == chunkZ;
}

math::Random Structure::createRandom(i64 seed, i32 chunkX, i32 chunkZ, i32 salt) {
    i64 combinedSeed = seed ^ (static_cast<i64>(chunkX) * 3418731287LL) ^
                       (static_cast<i64>(chunkZ) * 132897987541LL) +
                       static_cast<i64>(salt);
    return math::Random(combinedSeed);
}

// StructurePiece 实现
StructurePiece::StructurePiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
    : m_type(type)
    , m_minX(minX), m_minY(minY), m_minZ(minZ)
    , m_maxX(maxX), m_maxY(maxY), m_maxZ(maxZ)
{
}

bool StructurePiece::intersectsChunk(i32 chunkX, i32 chunkZ) const {
    i32 chunkMinX = chunkX << 4;
    i32 chunkMinZ = chunkZ << 4;
    i32 chunkMaxX = chunkMinX + 15;
    i32 chunkMaxZ = chunkMinZ + 15;

    return m_maxX >= chunkMinX && m_minX <= chunkMaxX &&
           m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
}

// StructureStart 实现
StructureStart::StructureStart(i32 chunkX, i32 chunkZ)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
{
}

void StructureStart::addPiece(std::unique_ptr<StructurePiece> piece) {
    m_pieces.push_back(std::move(piece));
}

} // namespace mc::world::gen::structure
