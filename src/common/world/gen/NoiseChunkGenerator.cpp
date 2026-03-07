#include "NoiseChunkGenerator.hpp"
#include "../block/BlockRegistry.hpp"
#include "../../math/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace mr {

// ============================================================================
// 常量
// ============================================================================

// 参考 MC 的 5x5 权重查找表计算
constexpr f64 BIOME_WEIGHT_RADIUS = 2.0;
constexpr f64 BIOME_WEIGHT_SCALE = 10.0;

// ============================================================================
// NoiseChunkGenerator 实现
// ============================================================================

NoiseChunkGenerator::NoiseChunkGenerator(u64 seed, DimensionSettings settings)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_verticalNoiseGranularity(0)
    , m_horizontalNoiseGranularity(0)
    , m_random(seed)
{
    initNoiseGenerators();
    initBiomeWeights();

    // 创建默认的生物群系提供者
    std::vector<BiomeDefinition> defaultBiomes = {
        Biomes::getPlains(),
        Biomes::getDesert(),
        Biomes::getMountains(),
        Biomes::getForest(),
        Biomes::getOcean()
    };
    m_biomeProvider = std::make_unique<SimpleBiomeProvider>(seed, defaultBiomes);
}

NoiseChunkGenerator::NoiseChunkGenerator(u64 seed, DimensionSettings settings,
                                          std::unique_ptr<BiomeProvider> biomeProvider)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_biomeProvider(std::move(biomeProvider))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_verticalNoiseGranularity(0)
    , m_horizontalNoiseGranularity(0)
    , m_random(seed)
{
    initNoiseGenerators();
    initBiomeWeights();
}

NoiseChunkGenerator::~NoiseChunkGenerator() = default;

// ============================================================================
// 初始化
// ============================================================================

void NoiseChunkGenerator::initNoiseGenerators()
{
    const NoiseSettings& noise = m_settings.noise;

    // 计算噪声尺寸
    m_verticalNoiseGranularity = noise.sizeVertical * 4;
    m_horizontalNoiseGranularity = noise.sizeHorizontal * 4;
    m_noiseSizeX = 16 / m_horizontalNoiseGranularity;
    m_noiseSizeY = noise.height / m_verticalNoiseGranularity;
    m_noiseSizeZ = 16 / m_horizontalNoiseGranularity;

    // 创建噪声生成器（参考 MC）
    std::mt19937_64 rng(m_seed);

    // 主密度噪声：16 倍频（-15 到 0）
    m_mainDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 次密度噪声：16 倍频（-15 到 0）
    m_secondaryDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 权重噪声：8 倍频（-7 到 0）
    m_weightNoise = std::make_unique<OctavesNoiseGenerator>(rng, -7, 0);

    // 地表深度噪声
    if (noise.simplexSurfaceNoise) {
        m_surfaceDepthNoise = std::make_unique<PerlinNoiseGenerator>(rng, -3, 0);
    } else {
        m_surfaceDepthNoise = std::make_unique<PerlinNoiseGenerator>(rng, -3, 0);
    }

    // 跳过一些随机数（参考 MC）
    rng.discard(2620);

    // 随机密度偏移噪声（用于放大化或末地）
    if (noise.randomDensityOffset) {
        m_endNoise = std::make_unique<SimplexNoiseGenerator>(rng);
    }
}

void NoiseChunkGenerator::initBiomeWeights()
{
    // 参考 MC 的 field_236081_j_ 查找表
    for (i32 dz = -2; dz <= 2; ++dz) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            const i32 index = (dx + 2) + (dz + 2) * 5;
            const f32 distance = static_cast<f32>(dx * dx + dz * dz);
            m_biomeWeights[index] = static_cast<f32>(BIOME_WEIGHT_SCALE / std::sqrt(distance + 0.2));
        }
    }
}

// ============================================================================
// 生物群系生成
// ============================================================================

void NoiseChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    BiomeContainer& biomes = chunk.getBiomes();
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 生物群系采样点是 4x4 方块一个大块
    // 一个区块有 4x4x4 个采样点
    for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
        for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
            for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
                // 转换为世界坐标
                const i32 worldX = (chunkX << 4) + (x << 2);
                const i32 worldY = y << 4;
                const i32 worldZ = (chunkZ << 4) + (z << 2);

                // 获取生物群系
                const BiomeId biome = m_biomeProvider->getBiome(worldX, worldY, worldZ);
                biomes.setBiome(x, y, z, biome);
            }
        }
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatus::BIOMES);
}

// ============================================================================
// 噪声地形生成
// ============================================================================

void NoiseChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // === 参考 MC NoiseChunkGenerator.func_230352_b_ ===
    // 使用双缓冲噪声缓存，只需要两列 X 的数据
    // 缓存结构：double[2][noiseSizeZ+1][noiseSizeY+1]
    std::vector<std::vector<f64>> noiseCache[2];
    noiseCache[0].resize(m_noiseSizeZ + 1, std::vector<f64>(m_noiseSizeY + 1));
    noiseCache[1].resize(m_noiseSizeZ + 1, std::vector<f64>(m_noiseSizeY + 1));

    // 初始化第一列噪声数据
    for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
        const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
        fillNoiseColumn(noiseCache[0][noiseZ], chunkX * m_noiseSizeX, globalNoiseZ);
    }

    // 遍历每个噪声单元
    for (i32 noiseX = 0; noiseX < m_noiseSizeX; ++noiseX) {
        // 预计算下一列 X 的噪声数据
        for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
            const i32 globalNoiseX = chunkX * m_noiseSizeX + noiseX + 1;
            const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
            fillNoiseColumn(noiseCache[1][noiseZ], globalNoiseX, globalNoiseZ);
        }

        // 处理当前噪声单元
        for (i32 noiseZ = 0; noiseZ < m_noiseSizeZ; ++noiseZ) {
            // 获取三线性插值所需的 8 个角点
            // 8 个角点：(x0,y0,z0), (x0,y0,z1), (x0,y1,z0), (x0,y1,z1),
            //          (x1,y0,z0), (x1,y0,z1), (x1,y1,z0), (x1,y1,z1)
            for (i32 noiseY = m_noiseSizeY - 1; noiseY >= 0; --noiseY) {
                // 从当前列和下一列获取密度值
                const f64 d0 = noiseCache[0][noiseZ][noiseY];           // (x0, z0, y0)
                const f64 d1 = noiseCache[0][noiseZ + 1][noiseY];       // (x0, z1, y0)
                const f64 d2 = noiseCache[1][noiseZ][noiseY];           // (x1, z0, y0)
                const f64 d3 = noiseCache[1][noiseZ + 1][noiseY];       // (x1, z1, y0)
                const f64 d4 = noiseCache[0][noiseZ][noiseY + 1];       // (x0, z0, y1)
                const f64 d5 = noiseCache[0][noiseZ + 1][noiseY + 1];   // (x0, z1, y1)
                const f64 d6 = noiseCache[1][noiseZ][noiseY + 1];       // (x1, z0, y1)
                const f64 d7 = noiseCache[1][noiseZ + 1][noiseY + 1];   // (x1, z1, y1)

                // Y 轴细分
                for (i32 localY = m_verticalNoiseGranularity - 1; localY >= 0; --localY) {
                    const i32 worldY = noiseY * m_verticalNoiseGranularity + localY;
                    const f64 yLerp = static_cast<f64>(localY) / static_cast<f64>(m_verticalNoiseGranularity);

                    // Y 轴插值
                    const f64 y0 = math::lerp(d0, d4, yLerp); // (x0, z0)
                    const f64 y1 = math::lerp(d1, d5, yLerp); // (x0, z1)
                    const f64 y2 = math::lerp(d2, d6, yLerp); // (x1, z0)
                    const f64 y3 = math::lerp(d3, d7, yLerp); // (x1, z1)

                    // X 轴细分
                    for (i32 localX = 0; localX < m_horizontalNoiseGranularity; ++localX) {
                        const i32 worldX = startX + noiseX * m_horizontalNoiseGranularity + localX;
                        const f64 xLerp = static_cast<f64>(localX) / static_cast<f64>(m_horizontalNoiseGranularity);

                        // X 轴插值
                        const f64 x0 = math::lerp(y0, y2, xLerp); // (z0)
                        const f64 x1 = math::lerp(y1, y3, xLerp); // (z1)

                        // Z 轴细分
                        for (i32 localZ = 0; localZ < m_horizontalNoiseGranularity; ++localZ) {
                            const i32 worldZ = startZ + noiseZ * m_horizontalNoiseGranularity + localZ;
                            const f64 zLerp = static_cast<f64>(localZ) / static_cast<f64>(m_horizontalNoiseGranularity);

                            // Z 轴插值 - 最终密度值
                            const f64 density = math::lerp(x0, x1, zLerp);

                            // 确定方块
                            const BlockId block = getBlockForDensity(density, worldY);

                            if (block != BlockId::Air) {
                                const BlockState* state = BlockRegistry::instance().get(block);
                                if (state) {
                                    const i32 localBlockX = worldX & 15;
                                    const i32 localBlockZ = worldZ & 15;
                                    chunk.setBlock(localBlockX, worldY, localBlockZ, state);

                                    // 更新高度图
                                    chunk.updateHeightmap(HeightmapType::WorldSurfaceWG, localBlockX, worldY, localBlockZ, state);
                                    if (state->isSolid()) {
                                        chunk.updateHeightmap(HeightmapType::OceanFloorWG, localBlockX, worldY, localBlockZ, state);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 交换缓存（swap 比 copy 更高效）
        std::swap(noiseCache[0], noiseCache[1]);
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatus::NOISE);
}

void NoiseChunkGenerator::fillNoiseColumn(std::vector<f64>& column, i32 noiseX, i32 noiseZ)
{
    column.resize(m_noiseSizeY + 1);

    const NoiseSettings& noise = m_settings.noise;

    // === 计算生物群系权重 ===
    f64 totalDepth = 0.0;
    f64 totalScale = 0.0;
    f64 totalWeight = 0.0;

    const f32 centerDepth = m_biomeProvider->getDepth(noiseX << 2, noiseZ << 2);

    for (i32 dz = -2; dz <= 2; ++dz) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            const i32 bx = noiseX + dx;
            const i32 bz = noiseZ + dz;

            const BiomeId biome = m_biomeProvider->getNoiseBiome(bx, 0, bz);
            const BiomeDefinition& def = m_biomeProvider->getBiomeDefinition(biome);

            const f32 depth = def.depth;
            const f32 scale = def.scale;

            // 权重计算（参考 MC）
            const i32 weightIndex = (dx + 2) + (dz + 2) * 5;
            f64 weight = m_biomeWeights[weightIndex];

            // 如果当前生物群系深度小于中心，增加权重
            if (depth > centerDepth) {
                weight *= 0.5;
            }

            weight /= (depth + 2.0);

            totalDepth += depth * weight;
            totalScale += scale * weight;
            totalWeight += weight;
        }
    }

    const f64 avgDepth = (totalDepth / totalWeight) * 0.5 - 0.125;
    const f64 avgScale = (totalScale / totalWeight) * 0.9 + 0.1;

    const f64 depthOffset = avgDepth * 0.265625;
    const f64 heightFactor = 96.0 / avgScale;

    // === 噪声参数 ===
    const f64 xzScale = 684.412 * noise.scaling.xzScale;
    const f64 yScale = 684.412 * noise.scaling.yScale;
    const f64 xzFactor = xzScale / noise.scaling.xzFactor;
    const f64 yFactor = yScale / noise.scaling.yFactor;

    // === 随机密度偏移 ===
    const f64 randomDensityOffset = noise.randomDensityOffset ? calculateRandomDensityOffset(noiseX, noiseZ) : 0.0;

    const f64 densityFactor = noise.densityFactor;
    const f64 densityOffset = noise.densityOffset;

    // === 填充噪声列 ===
    for (i32 y = 0; y <= m_noiseSizeY; ++y) {
        // 计算 3D 噪声密度
        f64 density = calculateNoiseDensity(noiseX, y, noiseZ, xzScale, yScale, xzFactor, yFactor);

        // 高度归一化
        const f64 normalizedY = 1.0 - static_cast<f64>(y) * 2.0 / static_cast<f64>(m_noiseSizeY);

        // 应用密度因子和偏移
        f64 value = normalizedY * densityFactor + densityOffset;
        f64 terrainMod = (value + depthOffset) * heightFactor;

        if (terrainMod > 0.0) {
            density += terrainMod * 4.0;
        } else {
            density += terrainMod;
        }

        // 应用随机密度偏移
        density += randomDensityOffset;

        // 顶部滑动
        if (noise.topSlide.size > 0) {
            const f64 slide = static_cast<f64>(m_noiseSizeY - y - noise.topSlide.offset) / static_cast<f64>(noise.topSlide.size);
            density = std::clamp<f64>(
                math::lerp(static_cast<f64>(noise.topSlide.target), density, slide),
                -1.0, 1.0
            );
        }

        // 底部滑动
        if (noise.bottomSlide.size > 0) {
            const f64 slide = static_cast<f64>(y - noise.bottomSlide.offset) / static_cast<f64>(noise.bottomSlide.size);
            density = std::clamp<f64>(
                math::lerp(static_cast<f64>(noise.bottomSlide.target), density, slide),
                -1.0, 1.0
            );
        }

        column[y] = density;
    }
}

f64 NoiseChunkGenerator::calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ,
                                                f64 xzScale, f64 yScale,
                                                f64 xzFactor, f64 yFactor) const
{
    // 参考 MC func_222552_a
    f64 density = 0.0;
    f64 secondaryDensity = 0.0;
    f64 weight = 0.0;

    f64 frequency = 1.0;
    f64 amplitude = 1.0;

    for (i32 octave = 0; octave < 16; ++octave) {
        // 保持精度
        const f64 px = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseX) * xzScale * frequency);
        const f64 py = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseY) * yScale * frequency);
        const f64 pz = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseZ) * xzScale * frequency);

        const f64 yFreq = yScale * frequency;

        // 主密度噪声
        if (m_mainDensityNoise) {
            const ImprovedNoiseGenerator* gen = m_mainDensityNoise->getOctave(octave);
            if (gen) {
                density += gen->noise(px, py, pz, yFreq, static_cast<f64>(noiseY) * yFreq) / amplitude;
            }
        }

        // 次密度噪声
        if (m_secondaryDensityNoise) {
            const ImprovedNoiseGenerator* gen = m_secondaryDensityNoise->getOctave(octave);
            if (gen) {
                secondaryDensity += gen->noise(px, py, pz, yFreq, static_cast<f64>(noiseY) * yFreq) / amplitude;
            }
        }

        // 权重噪声（前 8 个倍频）
        if (octave < 8 && m_weightNoise) {
            const ImprovedNoiseGenerator* gen = m_weightNoise->getOctave(octave);
            if (gen) {
                const f64 wpx = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseX) * xzFactor * frequency);
                const f64 wpy = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseY) * yFactor * frequency);
                const f64 wpz = OctavesNoiseGenerator::maintainPrecision(static_cast<f64>(noiseZ) * xzFactor * frequency);

                weight += gen->noise(wpx, wpy, wpz, yFactor * frequency, static_cast<f64>(noiseY) * yFactor * frequency) / amplitude;
            }
        }

        frequency *= 2.0;
        amplitude *= 2.0;
    }

    // 混合密度
    // math::lerp(a, b, t): t=0 返回 a，t=1 返回 b
    // 这里使用权重噪声作为插值因子。
    const f64 blend = std::clamp((weight / 10.0 + 1.0) / 2.0, 0.0, 1.0);
    return math::lerp(density / 512.0, secondaryDensity / 512.0, blend);
}

f64 NoiseChunkGenerator::calculateRandomDensityOffset(i32 noiseX, i32 noiseZ) const
{
    if (!m_endNoise) {
        return 0.0;
    }

    // 参考 MC func_236095_c_
    const f64 noise = m_endNoise->noise2D(static_cast<f64>(noiseX * 200), static_cast<f64>(noiseZ * 200));

    f64 offset;
    if (noise < 0.0) {
        offset = -noise * 0.3;
    } else {
        offset = noise;
    }

    const f64 result = offset * 24.575625 - 2.0;

    if (result < 0.0) {
        return result * 0.009486607142857142;
    } else {
        return std::min(result, 1.0) * 0.006640625;
    }
}

void NoiseChunkGenerator::calculateBiomeDepthAndScale(i32 noiseX, i32 noiseZ,
                                                       f64& outDepth, f64& outScale) const
{
    f64 totalDepth = 0.0;
    f64 totalScale = 0.0;
    f64 totalWeight = 0.0;

    const f32 centerDepth = m_biomeProvider->getDepth(noiseX << 2, noiseZ << 2);

    for (i32 dz = -2; dz <= 2; ++dz) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            const BiomeId biome = m_biomeProvider->getNoiseBiome(noiseX + dx, 0, noiseZ + dz);
            const BiomeDefinition& def = m_biomeProvider->getBiomeDefinition(biome);

            const f32 depth = def.depth;
            const f32 scale = def.scale;

            const i32 weightIndex = (dx + 2) + (dz + 2) * 5;
            f64 weight = m_biomeWeights[weightIndex];

            if (depth > centerDepth) {
                weight *= 0.5;
            }

            weight /= (depth + 2.0);

            totalDepth += depth * weight;
            totalScale += scale * weight;
            totalWeight += weight;
        }
    }

    outDepth = totalDepth / totalWeight;
    outScale = totalScale / totalWeight;
}

BlockId NoiseChunkGenerator::getBlockForDensity(f64 density, i32 y) const
{
    if (density > 0.0) {
        return m_settings.defaultBlock;
    } else if (y < m_settings.seaLevel) {
        return m_settings.defaultFluid;
    } else {
        return BlockId::Air;
    }
}

// ============================================================================
// 地表生成
// ============================================================================

void NoiseChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 设置随机种子
    std::mt19937_64 surfaceRng(static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL);
    std::uniform_int_distribution<u32> bedrockDist(0, 4);
    std::uniform_real_distribution<f64> surfaceDepthDist(0.0, 0.25);

    // 遍历每个 XZ 列
    for (i32 localX = 0; localX < 16; ++localX) {
        for (i32 localZ = 0; localZ < 16; ++localZ) {
            const i32 worldX = startX + localX;
            const i32 worldZ = startZ + localZ;

            // 获取地表高度
            const i32 surfaceHeight = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ);

            // 获取生物群系
            const BiomeId biome = chunk.getBiomeAtBlock(localX, surfaceHeight, localZ);

            // 计算地表噪声
            const f64 surfaceNoise = m_surfaceDepthNoise->noise2D(
                static_cast<f64>(worldX) * 0.0625,
                static_cast<f64>(worldZ) * 0.0625
            ) * 15.0;

            // 生成地表
            buildSurfaceForColumn(chunk, localX, localZ, surfaceHeight, surfaceNoise + surfaceDepthDist(surfaceRng) * 15.0, biome);
        }
    }

    // 生成基岩
    // 简单实现：底层 5 格随机放置基岩
    for (i32 localX = 0; localX < 16; ++localX) {
        for (i32 localZ = 0; localZ < 16; ++localZ) {
            for (i32 y = 0; y < 5; ++y) {
                if (y <= static_cast<i32>(bedrockDist(surfaceRng))) {
                    const BlockState* bedrock = BlockRegistry::instance().get(BlockId::Bedrock);
                    if (bedrock) {
                        chunk.setBlock(localX, y, localZ, bedrock);
                    }
                }
            }
        }
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatus::SURFACE);
}

void NoiseChunkGenerator::buildSurfaceForColumn(ChunkPrimer& chunk, i32 x, i32 z,
                                                 i32 surfaceHeight, f64 surfaceNoise, BiomeId biome)
{
    const BiomeDefinition& biomeDef = m_biomeProvider->getBiomeDefinition(biome);
    const i32 seaLevel = m_settings.seaLevel;

    // 地表深度
    const i32 surfaceDepth = static_cast<i32>(surfaceNoise / 3.0 + 3.0);
    const bool isUnderwater = surfaceHeight < seaLevel;

    // 从地表向下遍历
    i32 currentDepth = 0;
    bool processedTopSurfaceRun = false;
    for (i32 y = surfaceHeight; y >= 0; --y) {
        const BlockState* state = chunk.getBlock(x, y, z);

        if (!state || state->isAir()) {
            currentDepth = 0;
            continue;
        }

        const BlockId blockId = static_cast<BlockId>(state->blockId());

        // 只处理默认方块（石头）
        if (blockId != m_settings.defaultBlock) {
            continue;
        }

        // 只处理最上层那一段连续实心方块。
        // 避免在地下洞穴顶面再次刷出 grass/dirt。
        if (processedTopSurfaceRun && currentDepth == 0) {
            continue;
        }

        if (currentDepth < surfaceDepth) {
            // 地表层
            if (isUnderwater && y < seaLevel - surfaceDepth) {
                // 水下使用不同的方块
                const BlockState* underWater = BlockRegistry::instance().get(biomeDef.underWaterBlock);
                if (underWater) {
                    chunk.setBlock(x, y, z, underWater);
                }
            } else {
                // 地表
                const BlockState* surface = BlockRegistry::instance().get(biomeDef.surfaceBlock);
                if (surface) {
                    chunk.setBlock(x, y, z, surface);
                }
            }
            currentDepth++;
            processedTopSurfaceRun = true;
        } else if (currentDepth < surfaceDepth + 4) {
            // 次地表层
            const BlockState* subSurface = BlockRegistry::instance().get(biomeDef.subSurfaceBlock);
            if (subSurface) {
                chunk.setBlock(x, y, z, subSurface);
            }
            currentDepth++;
            processedTopSurfaceRun = true;
        }
    }
}

// ============================================================================
// 雕刻和特性
// ============================================================================

void NoiseChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid)
{
    // 暂时未实现洞穴雕刻
    // 后续可以添加
    chunk.setChunkStatus(ChunkStatus::CARVERS);
}

void NoiseChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    // 暂时未实现特性放置
    // 后续可以添加树木、矿石等
    chunk.setChunkStatus(ChunkStatus::FEATURES);
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId NoiseChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    return m_biomeProvider->getBiome(x, y, z);
}

BiomeId NoiseChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    return m_biomeProvider->getNoiseBiome(noiseX, noiseY, noiseZ);
}

i32 NoiseChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    // 简化实现：生成一个临时的高度估计
    // 实际实现需要采样噪声
    const f64 depth = m_biomeProvider->getDepth(x, z);
    const f64 scale = m_biomeProvider->getScale(x, z);

    const f64 baseHeight = 64.0 + depth * 32.0;
    const f64 variation = scale * 16.0;

    return static_cast<i32>(baseHeight + variation);
}

// ============================================================================
// BiomeProvider 实现
// ============================================================================

BiomeProvider::BiomeProvider(u64 seed)
    : m_seed(seed)
{
}

// ============================================================================
// SimpleBiomeProvider 实现
// ============================================================================

SimpleBiomeProvider::SimpleBiomeProvider(u64 seed, const std::vector<BiomeDefinition>& biomes)
    : BiomeProvider(seed)
    , m_biomes(biomes)
    , m_defaultBiome(Biomes::getPlains())
{
    std::mt19937_64 rng(seed);

    m_temperatureNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
    m_humidityNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
    m_continentalnessNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
    m_erosionNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
    m_depthNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
    m_scaleNoise = std::make_unique<PerlinNoiseGenerator>(rng, -4, 0);
}

BiomeId SimpleBiomeProvider::getBiome(i32 x, i32 y, i32 z) const
{
    return getNoiseBiome(x >> 2, y >> 4, z >> 2);
}

BiomeId SimpleBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    // 采样噪声
    const f64 temperature = m_temperatureNoise->noise2D(noiseX * 0.005, noiseZ * 0.005) * 0.5 + 0.5;
    const f64 humidity = m_humidityNoise->noise2D(noiseX * 0.005, noiseZ * 0.005) * 0.5 + 0.5;
    const f64 depth = m_depthNoise->noise2D(noiseX * 0.01, noiseZ * 0.01);
    const f64 scale = m_scaleNoise->noise2D(noiseX * 0.01, noiseZ * 0.01);

    return selectBiome(static_cast<f32>(temperature), static_cast<f32>(humidity),
                       static_cast<f32>(depth), static_cast<f32>(scale));
}

f32 SimpleBiomeProvider::getDepth(i32 x, i32 z) const
{
    return static_cast<f32>(m_depthNoise->noise2D(x * 0.01, z * 0.01));
}

f32 SimpleBiomeProvider::getScale(i32 x, i32 z) const
{
    return static_cast<f32>(m_scaleNoise->noise2D(x * 0.01, z * 0.01));
}

const BiomeDefinition& SimpleBiomeProvider::getBiomeDefinition(BiomeId id) const
{
    for (const auto& biome : m_biomes) {
        if (biome.id == id) {
            return biome;
        }
    }
    return m_defaultBiome;
}

BiomeId SimpleBiomeProvider::selectBiome(f32 temperature, f32 humidity, f32 depth, f32 scale) const
{
    // 简单的生物群系选择逻辑
    // 温度 < 0.2: 雪地
    // 温度 > 1.5: 沙漠
    // 深度 < -0.5: 海洋
    // 深度 > 0.8: 山地
    // 否则: 平原/森林

    if (depth < -0.5f) {
        return Biomes::Ocean;
    }

    if (depth > 0.8f) {
        return Biomes::Mountains;
    }

    if (temperature > 1.5f) {
        return Biomes::Desert;
    }

    if (temperature < 0.2f) {
        return Biomes::Taiga;
    }

    if (humidity > 0.7f) {
        return Biomes::Forest;
    }

    return Biomes::Plains;
}

} // namespace mr
