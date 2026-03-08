#include "BiomeProvider.hpp"
#include "BiomeRegistry.hpp"

namespace mr {

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
    // 采样噪声
    constexpr f64 TEMPERATURE_SCALE = 0.05;
    constexpr f64 HUMIDITY_SCALE = 0.05;
    constexpr f64 DEPTH_SCALE = 0.025;
    constexpr f64 SCALE_SCALE = 0.025;

    const f64 temperature = m_temperatureNoise->noise(
        static_cast<f64>(noiseX) * TEMPERATURE_SCALE,
        0.0,
        static_cast<f64>(noiseZ) * TEMPERATURE_SCALE
    );
    const f64 humidity = m_humidityNoise->noise(
        static_cast<f64>(noiseX) * HUMIDITY_SCALE,
        0.0,
        static_cast<f64>(noiseZ) * HUMIDITY_SCALE
    );
    const f64 depth = m_depthNoise->noise(
        static_cast<f64>(noiseX) * DEPTH_SCALE,
        0.0,
        static_cast<f64>(noiseZ) * DEPTH_SCALE
    );
    const f64 scale = m_scaleNoise->noise(
        static_cast<f64>(noiseX) * SCALE_SCALE,
        0.0,
        static_cast<f64>(noiseZ) * SCALE_SCALE
    );

    // 归一化到 [0, 1]
    const f32 tempNorm = static_cast<f32>((temperature + 1.0) * 0.5);
    const f32 humidNorm = static_cast<f32>((humidity + 1.0) * 0.5);
    const f32 depthNorm = static_cast<f32>((depth + 1.0) * 0.5);
    const f32 scaleNorm = static_cast<f32>((scale + 1.0) * 0.5);

    return selectBiome(tempNorm, humidNorm, depthNorm, scaleNorm);
}

f32 SimpleBiomeProvider::getDepth(i32 x, i32 z) const
{
    constexpr f64 DEPTH_SCALE = 0.025;
    const f64 depth = m_depthNoise->noise(
        static_cast<f64>(x) * DEPTH_SCALE,
        0.0,
        static_cast<f64>(z) * DEPTH_SCALE
    );
    return static_cast<f32>((depth + 1.0) * 0.5);
}

f32 SimpleBiomeProvider::getScale(i32 x, i32 z) const
{
    constexpr f64 SCALE_SCALE = 0.025;
    const f64 scale = m_scaleNoise->noise(
        static_cast<f64>(x) * SCALE_SCALE,
        0.0,
        static_cast<f64>(z) * SCALE_SCALE
    );
    return static_cast<f32>((scale + 1.0) * 0.5);
}

const Biome& SimpleBiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

void SimpleBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 遍历生物群系采样点 (4x4x4 方块为一个采样点)
    for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 worldX = startX + (bx << 2);
                const i32 worldY = by << 2;
                const i32 worldZ = startZ + (bz << 2);

                const BiomeId biome = getBiome(worldX, worldY, worldZ);
                container.setBiome(bx << 2, worldY, bz << 2, biome);
            }
        }
    }
}

BiomeId SimpleBiomeProvider::selectBiome(f32 temperature, f32 humidity, f32 depth, f32 scale) const
{
    // 简化的生物群系选择逻辑
    // 参考 MC 1.16.5 的生物群系分布

    // 深度低表示海洋
    if (depth < 0.2f) {
        if (depth < 0.1f) {
            return Biomes::DeepOcean;
        }
        return Biomes::Ocean;
    }
    // 温度低的区域
    else if (temperature < 0.2f) {
        if (humidity > 0.5f) {
            return Biomes::SnowyTaiga;
        }
        return Biomes::SnowyPlains;
    }
    // 温度高的区域
    else if (temperature > 0.8f) {
        if (humidity < 0.2f) {
            if (scale > 0.5f) {
                return Biomes::ErodedBadlands;
            }
            return Biomes::Badlands;
        }
        if (humidity < 0.4f) {
            return Biomes::Savanna;
        }
        return Biomes::Jungle;
    }
    // 中等温度
    else if (humidity > 0.7f) {
        return Biomes::Forest;
    }
    else if (humidity > 0.5f) {
        return Biomes::BirchForest;
    }
    // 山地
    else if (scale > 0.6f) {
        return Biomes::Mountains;
    }
    else if (scale > 0.4f) {
        return Biomes::WoodedHills;
    }

    // 默认平原
    return Biomes::Plains;
}

} // namespace mr
