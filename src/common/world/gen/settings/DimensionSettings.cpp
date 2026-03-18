#include "DimensionSettings.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../block/BlockRegistry.hpp"

namespace mc {

DimensionSettings DimensionSettings::overworld()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::overworld();
    settings.defaultBlock = VanillaBlocks::STONE ? &VanillaBlocks::STONE->defaultState() : nullptr;
    settings.defaultFluid = VanillaBlocks::WATER ? &VanillaBlocks::WATER->defaultState() : nullptr;
    settings.seaLevel = 63;
    return settings;
}

DimensionSettings DimensionSettings::nether()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::nether();
    settings.defaultBlock = VanillaBlocks::NETHERRACK ? &VanillaBlocks::NETHERRACK->defaultState() : nullptr;
    settings.defaultFluid = VanillaBlocks::LAVA ? &VanillaBlocks::LAVA->defaultState() : nullptr;
    settings.seaLevel = 32;
    settings.bedrockRoof = 127;
    settings.bedrockFloor = 0;
    return settings;
}

DimensionSettings DimensionSettings::end()
{
    DimensionSettings settings;
    settings.noise = NoiseSettings::end();
    settings.defaultBlock = VanillaBlocks::END_STONE ? &VanillaBlocks::END_STONE->defaultState() : nullptr;
    settings.defaultFluid = VanillaBlocks::AIR ? &VanillaBlocks::AIR->defaultState() : nullptr;
    settings.seaLevel = 0;
    return settings;
}

DimensionSettings DimensionSettings::flat()
{
    DimensionSettings settings;
    settings.noise.height = 4;
    settings.noise.densityFactor = 0.0;
    settings.noise.densityOffset = 0.0;
    settings.defaultBlock = VanillaBlocks::STONE ? &VanillaBlocks::STONE->defaultState() : nullptr;
    settings.defaultFluid = VanillaBlocks::AIR ? &VanillaBlocks::AIR->defaultState() : nullptr;
    settings.seaLevel = 0;
    return settings;
}

} // namespace mc
