#include "NoiseSettings.hpp"
#include <algorithm>

namespace mr {
namespace Biomes {

// ============================================================================
// 主世界生物群系定义（参考 MC 1.16.5 BiomeMaker）
// ============================================================================

BiomeDefinition getPlains()
{
    // MC: depth=0.125F, scale=0.05F
    BiomeDefinition def(Plains, "plains");
    def.depth = 0.125f;
    def.scale = 0.05f;
    def.temperature = 0.8f;
    def.humidity = 0.4f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getDesert()
{
    // MC: depth=0.125F, scale=0.05F
    BiomeDefinition def(Desert, "desert");
    def.depth = 0.125f;
    def.scale = 0.05f;
    def.temperature = 2.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::Sand;
    def.subSurfaceBlock = BlockId::Sand;
    def.underWaterBlock = BlockId::Sand;
    return def;
}

BiomeDefinition getMountains()
{
    // MC: depth=1.0F, scale=0.5F (Extreme Hills)
    BiomeDefinition def(Mountains, "mountains");
    def.depth = 1.0f;
    def.scale = 0.5f;
    def.temperature = 0.2f;
    def.humidity = 0.3f;
    def.surfaceBlock = BlockId::Stone;
    def.subSurfaceBlock = BlockId::Stone;
    def.underWaterBlock = BlockId::Stone;
    return def;
}

BiomeDefinition getWoodedMountains()
{
    // MC: depth=0.45F, scale=0.3F (Gravelly Mountains)
    BiomeDefinition def(WoodedMountains, "wooded_mountains");
    def.depth = 0.45f;
    def.scale = 0.3f;
    def.temperature = 0.2f;
    def.humidity = 0.3f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getForest()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(Forest, "forest");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 0.7f;
    def.humidity = 0.8f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getWoodedHills()
{
    // MC: depth=0.45F, scale=0.3F
    BiomeDefinition def(WoodedHills, "wooded_hills");
    def.depth = 0.45f;
    def.scale = 0.3f;
    def.temperature = 0.7f;
    def.humidity = 0.8f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getBirchForest()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(BirchForest, "birch_forest");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 0.6f;
    def.humidity = 0.6f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getDarkForest()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(DarkForest, "dark_forest");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 0.7f;
    def.humidity = 0.8f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getOcean()
{
    // MC: depth=-1.0F, scale=0.1F
    BiomeDefinition def(Ocean, "ocean");
    def.depth = -1.0f;
    def.scale = 0.1f;
    def.temperature = 0.5f;
    def.humidity = 0.5f;
    def.surfaceBlock = BlockId::Gravel;
    def.subSurfaceBlock = BlockId::Gravel;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getDeepOcean()
{
    // MC: depth=-1.8F, scale=0.1F (Deep Ocean)
    BiomeDefinition def(DeepOcean, "deep_ocean");
    def.depth = -1.8f;
    def.scale = 0.1f;
    def.temperature = 0.5f;
    def.humidity = 0.5f;
    def.surfaceBlock = BlockId::Gravel;
    def.subSurfaceBlock = BlockId::Gravel;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    BiomeDefinition def(Taiga, "taiga");
    def.depth = 0.2f;
    def.scale = 0.2f;
    def.temperature = -0.5f;
    def.humidity = 0.4f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getSnowyTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    BiomeDefinition def(SnowyTaiga, "snowy_taiga");
    def.depth = 0.2f;
    def.scale = 0.2f;
    def.temperature = -0.5f;
    def.humidity = 0.4f;
    def.surfaceBlock = BlockId::Grass; // 应该是雪，暂时用草地
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getSnowyPlains()
{
    // MC: depth=0.125F, scale=0.05F (Snowy Tundra)
    BiomeDefinition def(SnowyPlains, "snowy_plains");
    def.depth = 0.125f;
    def.scale = 0.05f;
    def.temperature = -0.5f;
    def.humidity = 0.5f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getGiantTreeTaiga()
{
    // MC: depth=0.2F, scale=0.2F
    BiomeDefinition def(GiantTreeTaiga, "giant_tree_taiga");
    def.depth = 0.2f;
    def.scale = 0.2f;
    def.temperature = 0.3f;
    def.humidity = 0.8f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getJungle()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(Jungle, "jungle");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 0.95f;
    def.humidity = 0.9f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getSavanna()
{
    // MC: depth=0.3625F, scale=0.05F
    BiomeDefinition def(Savanna, "savanna");
    def.depth = 0.3625f;
    def.scale = 0.05f;
    def.temperature = 1.2f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getShatteredSavanna()
{
    // MC: depth=0.3625F, scale=1.225F (Shattered Savanna)
    BiomeDefinition def(ShatteredSavanna, "shattered_savanna");
    def.depth = 0.3625f;
    def.scale = 1.225f;
    def.temperature = 1.1f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getSavannaPlateau()
{
    // MC: depth=1.05F, scale=0.0125F
    BiomeDefinition def(SavannaPlateau, "savanna_plateau");
    def.depth = 1.05f;
    def.scale = 0.0125f;
    def.temperature = 1.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getBadlands()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(Badlands, "badlands");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 2.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::RedSand;
    def.subSurfaceBlock = BlockId::Terracotta;
    def.underWaterBlock = BlockId::RedSand;
    return def;
}

BiomeDefinition getErodedBadlands()
{
    // MC: depth=0.1F, scale=0.2F
    BiomeDefinition def(ErodedBadlands, "eroded_badlands");
    def.depth = 0.1f;
    def.scale = 0.2f;
    def.temperature = 2.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::RedSand;
    def.subSurfaceBlock = BlockId::Terracotta;
    def.underWaterBlock = BlockId::RedSand;
    return def;
}

BiomeDefinition getBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    BiomeDefinition def(BadlandsPlateau, "badlands_plateau");
    def.depth = 1.5f;
    def.scale = 0.025f;
    def.temperature = 2.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::RedSand;
    def.subSurfaceBlock = BlockId::Terracotta;
    def.underWaterBlock = BlockId::RedSand;
    return def;
}

BiomeDefinition getWoodedBadlandsPlateau()
{
    // MC: depth=1.5F, scale=0.025F
    BiomeDefinition def(WoodedBadlandsPlateau, "wooded_badlands_plateau");
    def.depth = 1.5f;
    def.scale = 0.025f;
    def.temperature = 2.0f;
    def.humidity = 0.0f;
    def.surfaceBlock = BlockId::RedSand;
    def.subSurfaceBlock = BlockId::Terracotta;
    def.underWaterBlock = BlockId::RedSand;
    return def;
}

BiomeDefinition getBeach()
{
    // MC: depth=0.0F, scale=0.025F
    BiomeDefinition def(Beach, "beach");
    def.depth = 0.0f;
    def.scale = 0.025f;
    def.temperature = 0.8f;
    def.humidity = 0.4f;
    def.surfaceBlock = BlockId::Sand;
    def.subSurfaceBlock = BlockId::Sand;
    def.underWaterBlock = BlockId::Sand;
    return def;
}

BiomeDefinition getStoneShore()
{
    // MC: depth=0.1F, scale=0.8F (Stone Shore)
    BiomeDefinition def(StoneShore, "stone_shore");
    def.depth = 0.1f;
    def.scale = 0.8f;
    def.temperature = 0.2f;
    def.humidity = 0.3f;
    def.surfaceBlock = BlockId::Stone;
    def.subSurfaceBlock = BlockId::Stone;
    def.underWaterBlock = BlockId::Stone;
    return def;
}

BiomeDefinition getSnowyBeach()
{
    // MC: depth=0.0F, scale=0.025F
    BiomeDefinition def(SnowyBeach, "snowy_beach");
    def.depth = 0.0f;
    def.scale = 0.025f;
    def.temperature = 0.05f;
    def.humidity = 0.3f;
    def.surfaceBlock = BlockId::Sand;
    def.subSurfaceBlock = BlockId::Sand;
    def.underWaterBlock = BlockId::Sand;
    return def;
}

BiomeDefinition getSwamp()
{
    // MC: depth=-0.2F, scale=0.1F
    BiomeDefinition def(Swamp, "swamp");
    def.depth = -0.2f;
    def.scale = 0.1f;
    def.temperature = 0.8f;
    def.humidity = 0.9f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getRiver()
{
    // MC: depth=-0.5F, scale=0.0F
    BiomeDefinition def(River, "river");
    def.depth = -0.5f;
    def.scale = 0.0f;
    def.temperature = 0.5f;
    def.humidity = 0.5f;
    def.surfaceBlock = BlockId::Gravel;
    def.subSurfaceBlock = BlockId::Gravel;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

BiomeDefinition getMountainEdge()
{
    // MC: depth=0.8F, scale=0.3F (Mountain Edge)
    BiomeDefinition def(MountainEdge, "mountain_edge");
    def.depth = 0.8f;
    def.scale = 0.3f;
    def.temperature = 0.2f;
    def.humidity = 0.3f;
    def.surfaceBlock = BlockId::Grass;
    def.subSurfaceBlock = BlockId::Dirt;
    def.underWaterBlock = BlockId::Gravel;
    return def;
}

std::vector<BiomeDefinition> getDefaultBiomes()
{
    std::vector<BiomeDefinition> biomes;
    biomes.reserve(30);

    // 陆地生物群系
    biomes.push_back(getPlains());
    biomes.push_back(getForest());
    biomes.push_back(getWoodedHills());
    biomes.push_back(getBirchForest());
    biomes.push_back(getDarkForest());
    biomes.push_back(getTaiga());
    biomes.push_back(getSnowyTaiga());
    biomes.push_back(getSnowyPlains());
    biomes.push_back(getGiantTreeTaiga());
    biomes.push_back(getJungle());
    biomes.push_back(getSavanna());
    biomes.push_back(getShatteredSavanna());
    biomes.push_back(getSavannaPlateau());

    // 山地生物群系
    biomes.push_back(getMountains());
    biomes.push_back(getWoodedMountains());
    biomes.push_back(getMountainEdge());

    // 沙漠生物群系
    biomes.push_back(getDesert());
    biomes.push_back(getBadlands());
    biomes.push_back(getErodedBadlands());
    biomes.push_back(getBadlandsPlateau());
    biomes.push_back(getWoodedBadlandsPlateau());

    // 海洋生物群系
    biomes.push_back(getOcean());
    biomes.push_back(getDeepOcean());

    // 海滩/沼泽/河流
    biomes.push_back(getBeach());
    biomes.push_back(getStoneShore());
    biomes.push_back(getSnowyBeach());
    biomes.push_back(getSwamp());
    biomes.push_back(getRiver());

    return biomes;
}

} // namespace Biomes
} // namespace mr
