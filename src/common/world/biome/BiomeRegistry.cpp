#include "BiomeRegistry.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// BiomeRegistry 实现
// ============================================================================

BiomeRegistry& BiomeRegistry::instance()
{
    static BiomeRegistry instance;
    return instance;
}

BiomeRegistry::BiomeRegistry()
    : m_defaultBiome(Biomes::Plains, "plains")
{
}

void BiomeRegistry::initialize()
{
    if (!m_biomes.empty()) {
        return;  // 已经初始化
    }
    registerDefaultBiomes();
}

void BiomeRegistry::registerBiome(const Biome& biome)
{
    const BiomeId id = biome.id();
    if (id >= m_biomes.size()) {
        m_biomes.resize(id + 1);
    }
    m_biomes[id] = biome;
}

const Biome& BiomeRegistry::get(BiomeId id) const
{
    if (id < m_biomes.size()) {
        return m_biomes[id];
    }
    return m_defaultBiome;
}

bool BiomeRegistry::hasBiome(BiomeId id) const
{
    return id < m_biomes.size();
}

void BiomeRegistry::registerDefaultBiomes()
{
    // 注册所有默认生物群系
    registerBiome(BiomeFactory::createOcean());
    registerBiome(BiomeFactory::createPlains());
    registerBiome(BiomeFactory::createDesert());
    registerBiome(BiomeFactory::createMountains());
    registerBiome(BiomeFactory::createForest());
    registerBiome(BiomeFactory::createTaiga());
    registerBiome(BiomeFactory::createSwamp());
    registerBiome(BiomeFactory::createRiver());
    registerBiome(BiomeFactory::createFrozenOcean());
    registerBiome(BiomeFactory::createFrozenRiver());
    registerBiome(BiomeFactory::createSnowyPlains());
    registerBiome(BiomeFactory::createSnowyMountains());
    registerBiome(BiomeFactory::createBeach());
    registerBiome(BiomeFactory::createJungle());
    registerBiome(BiomeFactory::createSavanna());
    registerBiome(BiomeFactory::createBadlands());
    registerBiome(BiomeFactory::createDeepOcean());
    registerBiome(BiomeFactory::createDeepFrozenOcean());
    registerBiome(BiomeFactory::createWoodedHills());
    registerBiome(BiomeFactory::createMountainEdge());
    registerBiome(BiomeFactory::createStoneShore());
    registerBiome(BiomeFactory::createSnowyBeach());
    registerBiome(BiomeFactory::createBirchForest());
    registerBiome(BiomeFactory::createDarkForest());
    registerBiome(BiomeFactory::createSnowyTaiga());
    registerBiome(BiomeFactory::createGiantTreeTaiga());
    registerBiome(BiomeFactory::createWoodedMountains());
    registerBiome(BiomeFactory::createSavannaPlateau());
    registerBiome(BiomeFactory::createBadlandsPlateau());
    registerBiome(BiomeFactory::createWoodedBadlandsPlateau());
    registerBiome(BiomeFactory::createErodedBadlands());
    registerBiome(BiomeFactory::createShatteredSavanna());
    registerBiome(BiomeFactory::createIceSpikes());
}

// ============================================================================
// BiomeFactory 实现（参考 MC 1.16.5 BiomeMaker）
// ============================================================================

namespace BiomeFactory {

Biome createPlains()
{
    // MC: depth=0.125F, scale=0.05F
    Biome biome(Biomes::Plains, "plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createPlains());
    // 设置生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createPlains());
    return biome;
}

Biome createDesert()
{
    // MC: depth=0.125F, scale=0.05F
    Biome biome(Biomes::Desert, "desert");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::Sand);
    biome.setSubSurfaceBlock(BlockId::Sand);
    biome.setUnderWaterBlock(BlockId::Sand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDesert());
    // 沙漠生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createDesert());
    return biome;
}

Biome createMountains()
{
    // MC: depth=1.0F, scale=0.5F (Extreme Hills)
    Biome biome(Biomes::Mountains, "mountains");
    biome.setDepth(1.0f);
    biome.setScale(0.5f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(BlockId::Stone);
    biome.setSubSurfaceBlock(BlockId::Stone);
    biome.setUnderWaterBlock(BlockId::Stone);
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::Forest, "forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    // 森林生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createForest());
    return biome;
}

Biome createOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    Biome biome(Biomes::Ocean, "ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Gravel);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    // 海洋生物生成信息
    biome.setSpawnInfo(world::spawn::MobSpawnInfo::createOcean());
    return biome;
}

Biome createDeepOcean()
{
    // MC: depth=-1.8F, scale=0.1F (Deep Ocean)
    Biome biome(Biomes::DeepOcean, "deep_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Gravel);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    return biome;
}

Biome createTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::Taiga, "taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    return biome;
}

Biome createSnowyTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    // 雪地针叶林表面应为雪
    Biome biome(Biomes::SnowyTaiga, "snowy_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(BlockId::Snow);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    return biome;
}

Biome createJungle()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::Jungle, "jungle");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.95f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createJungle());
    return biome;
}

Biome createSavanna()
{
    // MC: depth=0.3625F, scale=0.05F
    Biome biome(Biomes::Savanna, "savanna");
    biome.setDepth(0.3625f);
    biome.setScale(0.05f);
    biome.setTemperature(1.2f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    return biome;
}

Biome createShatteredSavanna()
{
    // MC: depth=0.3625F, scale=1.225F (Shattered Savanna)
    Biome biome(Biomes::ShatteredSavanna, "shattered_savanna");
    biome.setDepth(0.3625f);
    biome.setScale(1.225f);
    biome.setTemperature(1.1f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    return biome;
}

Biome createSavannaPlateau()
{
    // MC: depth=1.05F, scale=0.0125F
    Biome biome(Biomes::SavannaPlateau, "savanna_plateau");
    biome.setDepth(1.05f);
    biome.setScale(0.0125f);
    biome.setTemperature(1.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createSavanna());
    return biome;
}

Biome createBadlands()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::Badlands, "badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::RedSand);
    biome.setSubSurfaceBlock(BlockId::Terracotta);
    biome.setUnderWaterBlock(BlockId::RedSand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createErodedBadlands()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::ErodedBadlands, "eroded_badlands");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::RedSand);
    biome.setSubSurfaceBlock(BlockId::Terracotta);
    biome.setUnderWaterBlock(BlockId::RedSand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    Biome biome(Biomes::BadlandsPlateau, "badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::RedSand);
    biome.setSubSurfaceBlock(BlockId::Terracotta);
    biome.setUnderWaterBlock(BlockId::RedSand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createWoodedBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    Biome biome(Biomes::WoodedBadlandsPlateau, "wooded_badlands_plateau");
    biome.setDepth(1.5f);
    biome.setScale(0.025f);
    biome.setTemperature(2.0f);
    biome.setHumidity(0.0f);
    biome.setSurfaceBlock(BlockId::RedSand);
    biome.setSubSurfaceBlock(BlockId::Terracotta);
    biome.setUnderWaterBlock(BlockId::RedSand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createBeach()
{
    // MC: depth=0.0F, scale=0.025F
    Biome biome(Biomes::Beach, "beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.4f);
    biome.setSurfaceBlock(BlockId::Sand);
    biome.setSubSurfaceBlock(BlockId::Sand);
    biome.setUnderWaterBlock(BlockId::Sand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createStoneShore()
{
    // MC: depth=0.1F, scale=0.8F (Stone Shore)
    Biome biome(Biomes::StoneShore, "stone_shore");
    biome.setDepth(0.1f);
    biome.setScale(0.8f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(BlockId::Stone);
    biome.setSubSurfaceBlock(BlockId::Stone);
    biome.setUnderWaterBlock(BlockId::Stone);
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createSnowyBeach()
{
    // MC: depth=0.0F, scale=0.025F
    Biome biome(Biomes::SnowyBeach, "snowy_beach");
    biome.setDepth(0.0f);
    biome.setScale(0.025f);
    biome.setTemperature(0.05f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(BlockId::Sand);
    biome.setSubSurfaceBlock(BlockId::Sand);
    biome.setUnderWaterBlock(BlockId::Sand);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createSwamp()
{
    // MC: depth=-0.2F, scale=0.1F
    Biome biome(Biomes::Swamp, "swamp");
    biome.setDepth(-0.2f);
    biome.setScale(0.1f);
    biome.setTemperature(0.8f);
    biome.setHumidity(0.9f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createRiver()
{
    // MC: depth=-0.5F, scale=0.0F
    Biome biome(Biomes::River, "river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Gravel);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createWoodedHills()
{
    // MC: depth=0.45F, scale=0.3F
    Biome biome(Biomes::WoodedHills, "wooded_hills");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createTaiga());
    return biome;
}

Biome createBirchForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::BirchForest, "birch_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.6f);
    biome.setHumidity(0.6f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    return biome;
}

Biome createDarkForest()
{
    // MC: depth=0.1F, scale=0.2F
    Biome biome(Biomes::DarkForest, "dark_forest");
    biome.setDepth(0.1f);
    biome.setScale(0.2f);
    biome.setTemperature(0.7f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    return biome;
}

Biome createSnowyPlains()
{
    // MC: depth=0.125F, scale=0.05F (Snowy Tundra)
    // 雪地平原表面应为雪
    Biome biome(Biomes::SnowyPlains, "snowy_plains");
    biome.setDepth(0.125f);
    biome.setScale(0.05f);
    biome.setTemperature(-0.5f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Snow);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createGiantTreeTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    Biome biome(Biomes::GiantTreeTaiga, "giant_tree_taiga");
    biome.setDepth(0.2f);
    biome.setScale(0.2f);
    biome.setTemperature(0.3f);
    biome.setHumidity(0.8f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());
    return biome;
}

Biome createWoodedMountains()
{
    // MC: depth=0.45F, scale=0.3F (Gravelly Mountains)
    Biome biome(Biomes::WoodedMountains, "wooded_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createMountainEdge()
{
    // MC: depth=0.8F, scale=0.3F (Mountain Edge)
    Biome biome(Biomes::MountainEdge, "mountain_edge");
    biome.setDepth(0.8f);
    biome.setScale(0.3f);
    biome.setTemperature(0.2f);
    biome.setHumidity(0.3f);
    biome.setSurfaceBlock(BlockId::Grass);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createFrozenOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    Biome biome(Biomes::FrozenOcean, "frozen_ocean");
    biome.setDepth(-1.0f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Ice);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    return biome;
}

Biome createFrozenRiver()
{
    // MC: depth=-0.5F, scale=0.0F
    Biome biome(Biomes::FrozenRiver, "frozen_river");
    biome.setDepth(-0.5f);
    biome.setScale(0.0f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Ice);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createSnowyMountains()
{
    // MC: depth=0.45F, scale=0.3F (Snowy Mountains)
    Biome biome(Biomes::SnowyMountains, "snowy_mountains");
    biome.setDepth(0.45f);
    biome.setScale(0.3f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Snow);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createMountains());
    return biome;
}

Biome createIceSpikes()
{
    // MC: depth=0.4375F, scale=0.05F
    Biome biome(Biomes::IceSpikes, "ice_spikes");
    biome.setDepth(0.4375f);
    biome.setScale(0.05f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Snow);
    biome.setSubSurfaceBlock(BlockId::Dirt);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createDefault());
    return biome;
}

Biome createDeepFrozenOcean()
{
    // MC: depth=-1.8F, scale=0.1F
    Biome biome(Biomes::DeepFrozenOcean, "deep_frozen_ocean");
    biome.setDepth(-1.8f);
    biome.setScale(0.1f);
    biome.setTemperature(0.0f);
    biome.setHumidity(0.5f);
    biome.setSurfaceBlock(BlockId::Ice);
    biome.setSubSurfaceBlock(BlockId::Gravel);
    biome.setUnderWaterBlock(BlockId::Gravel);
    biome.setGenerationSettings(BiomeGenerationSettings::createOcean());
    return biome;
}

} // namespace BiomeFactory

} // namespace mc
