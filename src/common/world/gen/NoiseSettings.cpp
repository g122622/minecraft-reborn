#include "NoiseSettings.hpp"

namespace mr {
namespace Biomes {

BiomeDefinition getPlains()
{
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

BiomeDefinition getForest()
{
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

BiomeDefinition getOcean()
{
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

BiomeDefinition getTaiga()
{
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

BiomeDefinition getJungle()
{
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

BiomeDefinition getBadlands()
{
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

} // namespace Biomes
} // namespace mr
