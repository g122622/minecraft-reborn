#include "StructureManager.hpp"
#include "structures/RuinedPortalStructure.hpp"
#include "structures/BuriedTreasureStructure.hpp"

namespace mc::world::gen::structure {

// StructureRegistry 实现
bool StructureRegistry::s_initialized = false;

std::unordered_map<String, std::unique_ptr<Structure>>& StructureRegistry::getStructures() {
    static std::unordered_map<String, std::unique_ptr<Structure>> structures;
    return structures;
}

std::vector<const Structure*>& StructureRegistry::getStructureList() {
    static std::vector<const Structure*> structureList;
    return structureList;
}

void StructureRegistry::initialize() {
    if (s_initialized) return;

    // 注册原版结构
    registerStructure(std::make_unique<RuinedPortalStructure>());
    registerStructure(std::make_unique<BuriedTreasureStructure>());

    s_initialized = true;
}

void StructureRegistry::registerStructure(std::unique_ptr<Structure> structure) {
    if (!structure) return;

    const String name = structure->name();
    auto& structures = getStructures();
    auto& list = getStructureList();

    if (structures.find(name) == structures.end()) {
        list.push_back(structure.get());
        structures[name] = std::move(structure);
    }
}

const Structure* StructureRegistry::get(const String& name) {
    auto& structures = getStructures();
    auto it = structures.find(name);
    return it != structures.end() ? it->second.get() : nullptr;
}

const std::vector<const Structure*>& StructureRegistry::getAll() {
    return getStructureList();
}

// StructureManager 实现
StructureManager::StructureManager(i64 seed)
    : m_seed(seed)
{
}

bool StructureManager::shouldGenerateStructureStart(
    const Structure& structure,
    i32 chunkX,
    i32 chunkZ) const
{
    // 使用结构的间距设置检查是否应该在此位置生成
    i32 startX, startZ;
    return Structure::findStructureStart(
        m_seed, chunkX, chunkZ,
        structure.separationSettings(),
        startX, startZ);
}

std::unique_ptr<StructureStart> StructureManager::generateStructureStart(
    const Structure& structure,
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 调用结构的生成方法
    return structure.generate(world, generator, rng, chunkX, chunkZ);
}

void StructureManager::placeStructureInChunk(
    const Structure& structure,
    IWorldWriter& world,
    ChunkPrimer& chunk,
    StructureStart& start,
    i32 chunkX,
    i32 chunkZ)
{
    // 调用结构的放置方法
    structure.placeInChunk(world, chunk, start, chunkX, chunkZ);
}

void StructureManager::clearCache() {
    // 清理缓存（简化版本）
}

math::Random StructureManager::createRandom(i32 chunkX, i32 chunkZ, i32 salt) const {
    i64 combinedSeed = m_seed ^ (static_cast<i64>(chunkX) * 3418731287LL) ^
                       (static_cast<i64>(chunkZ) * 132897987541LL) +
                       static_cast<i64>(salt);
    return math::Random(combinedSeed);
}

} // namespace mc::world::gen::structure
