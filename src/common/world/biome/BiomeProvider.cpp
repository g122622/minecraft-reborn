#include "BiomeProvider.hpp"
#include "BiomeRegistry.hpp"

#include <algorithm>
#include <cmath>

namespace mc {

namespace {

[[nodiscard]] f32 sampleBlendedNoise(const PerlinNoiseGenerator& noise, i32 x, i32 z, f32 largeScale, f32 detailScale)
{
    const f32 large = noise.noise(static_cast<f32>(x) * largeScale, 0.0f, static_cast<f32>(z) * largeScale);
    const f32 detail = noise.noise(static_cast<f32>(x) * detailScale, 0.0f, static_cast<f32>(z) * detailScale);
    return large * 0.7f + detail * 0.3f;
}

[[nodiscard]] f32 normalizeShaped(f32 value, f32 stretch)
{
    const f32 normalized = 0.5f + value * 0.5f * stretch;
    return std::clamp(normalized, 0.0f, 1.0f);
}

} // namespace

// ============================================================================
// BiomeProvider 实现
// ============================================================================

BiomeProvider::BiomeProvider(u64 seed)
    : m_seed(seed)
{
}

const Biome& BiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

// ============================================================================
// SimpleBiomeProvider 实现
// ============================================================================

SimpleBiomeProvider::SimpleBiomeProvider(u64 seed)
    : BiomeProvider(seed)
    , m_temperatureNoise(std::make_unique<PerlinNoiseGenerator>(seed, 0, 3))
    , m_humidityNoise(std::make_unique<PerlinNoiseGenerator>(seed + 1, 0, 3))
    , m_continentalnessNoise(std::make_unique<PerlinNoiseGenerator>(seed + 2, 0, 3))
    , m_erosionNoise(std::make_unique<PerlinNoiseGenerator>(seed + 3, 0, 3))
    , m_depthNoise(std::make_unique<PerlinNoiseGenerator>(seed + 4, 0, 3))
    , m_scaleNoise(std::make_unique<PerlinNoiseGenerator>(seed + 5, 0, 3))
{
}

BiomeId SimpleBiomeProvider::getBiome(i32 x, i32 y, i32 z) const
{
    return getNoiseBiome(x >> 2, y >> 2, z >> 2);
}

BiomeId SimpleBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    (void)noiseY;

    const f32 temperature = normalizeShaped(sampleBlendedNoise(*m_temperatureNoise, noiseX, noiseZ, 0.008f, 0.026f), 1.45f);
    const f32 humidity = normalizeShaped(sampleBlendedNoise(*m_humidityNoise, noiseX, noiseZ, 0.008f, 0.024f), 1.40f);
    const f32 continentalness = normalizeShaped(sampleBlendedNoise(*m_continentalnessNoise, noiseX, noiseZ, 0.004f, 0.014f), 1.75f);
    const f32 erosion = normalizeShaped(sampleBlendedNoise(*m_erosionNoise, noiseX, noiseZ, 0.006f, 0.021f), 1.55f);
    const f32 weirdness = normalizeShaped(sampleBlendedNoise(*m_scaleNoise, noiseX, noiseZ, 0.010f, 0.033f), 1.60f);
    const f32 valley = normalizeShaped(sampleBlendedNoise(*m_depthNoise, noiseX, noiseZ, 0.012f, 0.040f), 1.55f);

    return selectBiome(temperature, humidity, continentalness, erosion, weirdness, valley);
}

f32 SimpleBiomeProvider::getDepth(i32 x, i32 z) const
{
    const BiomeId biomeId = getBiome(x, 64, z);
    return getBiomeDefinition(biomeId).depth();
}

f32 SimpleBiomeProvider::getScale(i32 x, i32 z) const
{
    const BiomeId biomeId = getBiome(x, 64, z);
    return getBiomeDefinition(biomeId).scale();
}

const Biome& SimpleBiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

void SimpleBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 遍历生物群系采样点。BiomeContainer 逻辑上是 4x4x4，
    // 其中 X/Z 每格覆盖 4 方块，Y 每格覆盖 16 方块。
    for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 worldX = startX + (bx << 2);
                const i32 worldY = by << 4;
                const i32 worldZ = startZ + (bz << 2);

                const BiomeId biome = getBiome(worldX, worldY, worldZ);
                container.setBiome(bx, by, bz, biome);
            }
        }
    }
}

BiomeId SimpleBiomeProvider::selectBiome(
    f32 temperature,
    f32 humidity,
    f32 continentalness,
    f32 erosion,
    f32 weirdness,
    f32 valley) const
{
    const f32 ruggedness = std::clamp((1.0f - erosion) * 0.6f + weirdness * 0.4f, 0.0f, 1.0f);
    const f32 elevatedness = std::clamp((continentalness - 0.22f) * 1.1f + ruggedness * 0.75f, 0.0f, 1.0f);
    const bool hot = temperature > 0.8f;
    const bool cold = temperature < 0.22f;
    const bool wet = humidity > 0.7f;
    const bool dry = humidity < 0.3f;

    if (continentalness < 0.10f) {
        if (continentalness < 0.06f || (erosion > 0.72f && weirdness < 0.45f)) {
            return Biomes::DeepOcean;
        }
        return Biomes::Ocean;
    }

    if (continentalness < 0.18f) {
        if (cold) {
            return Biomes::SnowyBeach;
        }
        if (wet && erosion > 0.62f) {
            return Biomes::Swamp;
        }
        return Biomes::Beach;
    }

    if (continentalness < 0.30f && valley < 0.18f && erosion > 0.58f) {
        return Biomes::River;
    }

    if (elevatedness > 0.88f) {
        if (hot && dry) {
            if (weirdness > 0.72f && erosion < 0.35f) {
                return Biomes::ShatteredSavanna;
            }
            if (continentalness > 0.62f) {
                return humidity < 0.16f ? Biomes::BadlandsPlateau : Biomes::WoodedBadlandsPlateau;
            }
            if (erosion < 0.28f) {
                return Biomes::ErodedBadlands;
            }
            return Biomes::SavannaPlateau;
        }

        if (cold) {
            return humidity > 0.55f ? Biomes::WoodedMountains : Biomes::Mountains;
        }

        if (wet) {
            return weirdness > 0.6f ? Biomes::WoodedMountains : Biomes::GiantTreeTaiga;
        }

        return humidity > 0.45f ? Biomes::WoodedMountains : Biomes::Mountains;
    }

    if (elevatedness > 0.70f) {
        if (hot && dry) {
            return erosion < 0.4f ? Biomes::ErodedBadlands : Biomes::SavannaPlateau;
        }

        if (cold) {
            return humidity > 0.5f ? Biomes::SnowyTaiga : Biomes::MountainEdge;
        }

        if (wet) {
            return temperature < 0.45f ? Biomes::GiantTreeTaiga : Biomes::WoodedHills;
        }

        return humidity > 0.5f ? Biomes::WoodedHills : Biomes::MountainEdge;
    }

    if (cold) {
        return humidity > 0.52f ? Biomes::SnowyTaiga : Biomes::SnowyPlains;
    }

    if (hot) {
        if (dry) {
            if (erosion < 0.28f && continentalness > 0.52f) {
                return Biomes::ErodedBadlands;
            }
            return continentalness > 0.45f ? Biomes::Desert : Biomes::Badlands;
        }
        if (humidity < 0.45f) {
            return Biomes::Savanna;
        }
        return wet ? Biomes::Jungle : Biomes::Savanna;
    }

    if (wet) {
        if (continentalness < 0.38f && erosion > 0.66f) {
            return Biomes::Swamp;
        }
        if (temperature < 0.45f) {
            return Biomes::GiantTreeTaiga;
        }
        return erosion < 0.35f ? Biomes::DarkForest : Biomes::Forest;
    }

    if (humidity > 0.55f) {
        return erosion < 0.42f ? Biomes::Forest : Biomes::BirchForest;
    }

    if (continentalness > 0.58f && dry) {
        return Biomes::Desert;
    }

    return Biomes::Plains;
}

} // namespace mc
