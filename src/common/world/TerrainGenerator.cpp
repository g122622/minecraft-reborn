#include "TerrainGenerator.hpp"
#include "block/VanillaBlocks.hpp"
#include <cmath>
#include <algorithm>
#include <random>

namespace mr {

// ============================================================================
// 生物群系数据
// ============================================================================

namespace {

// 生物群系信息初始化函数
BiomeInfo createBiomeInfo(BiomeType type, f32 minH, f32 maxH, f32 temp, f32 hum) {
    BiomeInfo info;
    info.type = type;
    info.minHeight = minH;
    info.maxHeight = maxH;
    info.temperature = temp;
    info.humidity = hum;
    // 方块指针在 initializeBiomes() 中设置
    return info;
}

} // anonymous namespace

// ============================================================================
// StandardTerrainGenerator 实现
// ============================================================================

StandardTerrainGenerator::StandardTerrainGenerator(const WorldGenConfig& config)
    : m_config(config)
{
    initializeBiomes();

    // 初始化噪声生成器
    m_terrainNoise = std::make_unique<PerlinNoise>(static_cast<u32>(config.seed));
    m_caveNoise = std::make_unique<PerlinNoise>(static_cast<u32>(config.seed + 1));
    m_biomeNoise = std::make_unique<PerlinNoise>(static_cast<u32>(config.seed + 2));
    m_detailNoise = std::make_unique<PerlinNoise>(static_cast<u32>(config.seed + 3));

    m_terrainNoise->setFrequency(config.terrainScale);
    m_caveNoise->setFrequency(config.caveFrequency);
    m_biomeNoise->setFrequency(config.biomeScale);
    m_detailNoise->setFrequency(0.1f);
}

void StandardTerrainGenerator::generateChunk(ChunkData& chunk) {
    generateBaseTerrain(chunk);
    generateCaves(chunk);
}

void StandardTerrainGenerator::populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) {
    generateOres(chunk);
    generateStructures(chunk);
    generateTrees(chunk, neighbors);
    generateFlowers(chunk);
    generateGrass(chunk);
}

BiomeType StandardTerrainGenerator::getBiome(i32 x, i32 z) const {
    return calculateBiome(x, z);
}

const BiomeInfo& StandardTerrainGenerator::getBiomeInfo(BiomeType type) const {
    size_t index = static_cast<size_t>(type);
    if (index < m_biomes.size()) {
        return m_biomes[index];
    }
    return m_biomes[0]; // 默认返回平原
}

i32 StandardTerrainGenerator::getHeight(i32 x, i32 z) const {
    return calculateHeight(x, z);
}

void StandardTerrainGenerator::setSeed(u64 seed) {
    m_config.seed = seed;
    m_terrainNoise = std::make_unique<PerlinNoise>(static_cast<u32>(seed));
    m_caveNoise = std::make_unique<PerlinNoise>(static_cast<u32>(seed + 1));
    m_biomeNoise = std::make_unique<PerlinNoise>(static_cast<u32>(seed + 2));
    m_detailNoise = std::make_unique<PerlinNoise>(static_cast<u32>(seed + 3));
}

void StandardTerrainGenerator::generateBaseTerrain(ChunkData& chunk) {
    i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
    i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

    // 获取方块指针
    const BlockState* grassState = VanillaBlocks::GRASS_BLOCK ? &VanillaBlocks::GRASS_BLOCK->defaultState() : nullptr;
    const BlockState* dirtState = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->defaultState() : nullptr;
    const BlockState* stoneState = VanillaBlocks::STONE ? &VanillaBlocks::STONE->defaultState() : nullptr;
    const BlockState* bedrockState = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;
    const BlockState* sandState = VanillaBlocks::SAND ? &VanillaBlocks::SAND->defaultState() : nullptr;
    const BlockState* waterState = VanillaBlocks::WATER ? &VanillaBlocks::WATER->defaultState() : nullptr;
    const BlockState* snowState = VanillaBlocks::SNOW ? &VanillaBlocks::SNOW->defaultState() : nullptr;
    const BlockState* airState = nullptr; // 空气用 nullptr

    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
            i32 worldX = chunkX + lx;
            i32 worldZ = chunkZ + lz;

            // 获取生物群系
            BiomeType biome = calculateBiome(worldX, worldZ);
            const BiomeInfo& biomeInfo = getBiomeInfo(biome);

            // 计算高度
            i32 height = calculateHeight(worldX, worldZ);

            // 确保高度在有效范围内
            height = std::max(1, std::min(height, world::MAX_BUILD_HEIGHT - 1));

            // 选择表面方块
            const BlockState* surfaceBlock = grassState;
            const BlockState* subsurfaceBlock = dirtState;
            const BlockState* fillBlock = airState;

            // 根据生物群系设置方块
            if (biomeInfo.surfaceBlock) surfaceBlock = biomeInfo.surfaceBlock;
            if (biomeInfo.subsurfaceBlock) subsurfaceBlock = biomeInfo.subsurfaceBlock;
            if (biomeInfo.fillBlock) fillBlock = biomeInfo.fillBlock;

            // 填充方块
            for (i32 y = world::MIN_BUILD_HEIGHT; y < height; ++y) {
                const BlockState* block = nullptr;

                if (y == height - 1) {
                    // 地表
                    block = surfaceBlock;
                } else if (y >= height - 4) {
                    // 次地表
                    block = subsurfaceBlock;
                } else {
                    // 基岩层
                    if (y <= world::MIN_BUILD_HEIGHT + 4) {
                        block = bedrockState;
                    } else {
                        block = stoneState;
                    }
                }

                chunk.setBlock(lx, y, lz, block);
            }

            // 如果在海平面以下，填充水
            if (height < m_config.seaLevel) {
                for (i32 y = height; y <= m_config.seaLevel; ++y) {
                    chunk.setBlock(lx, y, lz, fillBlock);
                }
            }
        }
    }

    chunk.setFullyGenerated(true);
    chunk.setDirty(true);
}

void StandardTerrainGenerator::generateCaves(ChunkData& chunk) {
    i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
    i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

    const BlockState* airState = nullptr;
    const BlockState* bedrockState = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;

    // 使用3D噪声生成洞穴
    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
            i32 worldX = chunkX + lx;
            i32 worldZ = chunkZ + lz;

            for (i32 y = world::MIN_BUILD_HEIGHT + 10; y < world::MAX_BUILD_HEIGHT - 20; ++y) {
                f32 caveValue = getCaveNoise(worldX, y, worldZ);

                // 如果噪声值超过阈值，挖空方块
                if (caveValue > m_config.caveThreshold) {
                    const BlockState* current = chunk.getBlock(lx, y, lz);
                    if (current && !current->isAir() && current != bedrockState) {
                        chunk.setBlock(lx, y, lz, airState);
                    }
                }
            }
        }
    }
}

void StandardTerrainGenerator::generateOres(ChunkData& chunk) {
    // 简化的矿石生成 - 如果方块系统未初始化则跳过
    if (!VanillaBlocks::STONE) return;

    i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
    i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

    std::mt19937 rng(static_cast<u32>(m_config.seed ^ static_cast<u64>(chunkX) ^ (static_cast<u64>(chunkZ) << 16)));
    std::uniform_int_distribution<i32> heightDist(0, 60);
    std::uniform_int_distribution<i32> countDist(0, 100);

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    const BlockState* coalOreState = VanillaBlocks::COAL_ORE ? &VanillaBlocks::COAL_ORE->defaultState() : nullptr;
    const BlockState* ironOreState = VanillaBlocks::IRON_ORE ? &VanillaBlocks::IRON_ORE->defaultState() : nullptr;
    const BlockState* goldOreState = VanillaBlocks::GOLD_ORE ? &VanillaBlocks::GOLD_ORE->defaultState() : nullptr;
    const BlockState* diamondOreState = VanillaBlocks::DIAMOND_ORE ? &VanillaBlocks::DIAMOND_ORE->defaultState() : nullptr;

    // 煤矿 (高度 0-80)
    if (coalOreState && countDist(rng) < 20) {
        i32 x = rng() % world::CHUNK_WIDTH;
        i32 z = rng() % world::CHUNK_WIDTH;
        i32 y = 5 + heightDist(rng) % 75;

        const BlockState* current = chunk.getBlock(x, y, z);
        if (current == stoneState) {
            chunk.setBlock(x, y, z, coalOreState);
        }
    }

    // 铁矿 (高度 0-64)
    if (ironOreState && countDist(rng) < 15) {
        i32 x = rng() % world::CHUNK_WIDTH;
        i32 z = rng() % world::CHUNK_WIDTH;
        i32 y = 5 + heightDist(rng) % 59;

        const BlockState* current = chunk.getBlock(x, y, z);
        if (current == stoneState) {
            chunk.setBlock(x, y, z, ironOreState);
        }
    }

    // 金矿 (高度 0-32)
    if (goldOreState && countDist(rng) < 5) {
        i32 x = rng() % world::CHUNK_WIDTH;
        i32 z = rng() % world::CHUNK_WIDTH;
        i32 y = 5 + heightDist(rng) % 27;

        const BlockState* current = chunk.getBlock(x, y, z);
        if (current == stoneState) {
            chunk.setBlock(x, y, z, goldOreState);
        }
    }

    // 钻石矿 (高度 0-16)
    if (diamondOreState && countDist(rng) < 2) {
        i32 x = rng() % world::CHUNK_WIDTH;
        i32 z = rng() % world::CHUNK_WIDTH;
        i32 y = 5 + heightDist(rng) % 11;

        const BlockState* current = chunk.getBlock(x, y, z);
        if (current == stoneState) {
            chunk.setBlock(x, y, z, diamondOreState);
        }
    }
}

void StandardTerrainGenerator::generateStructures(ChunkData& chunk) {
    // TODO: 结构生成 (村庄、神殿等)
    (void)chunk;
}

f32 StandardTerrainGenerator::getTerrainNoise(i32 x, i32 z) const {
    return m_terrainNoise->octave2D(
        static_cast<f32>(x),
        static_cast<f32>(z),
        m_config.terrainOctaves,
        0.5f
    );
}

f32 StandardTerrainGenerator::getCaveNoise(i32 x, i32 y, i32 z) const {
    return m_caveNoise->noise3D(
        static_cast<f32>(x) * m_config.caveFrequency,
        static_cast<f32>(y) * m_config.caveFrequency,
        static_cast<f32>(z) * m_config.caveFrequency
    );
}

i32 StandardTerrainGenerator::calculateHeight(i32 x, i32 z) const {
    // 基础噪声
    f32 baseHeight = getTerrainNoise(x, z);

    // 生物群系影响
    BiomeType biome = calculateBiome(x, z);
    const BiomeInfo& biomeInfo = getBiomeInfo(biome);

    // 计算最终高度
    f32 height = m_config.terrainHeight +
                 baseHeight * m_config.terrainVariation +
                 biomeInfo.minHeight +
                 (biomeInfo.maxHeight - biomeInfo.minHeight) * baseHeight;

    // 添加细节噪声
    f32 detail = m_detailNoise->noise2D(static_cast<f32>(x) * 0.5f, static_cast<f32>(z) * 0.5f);
    height += detail * 3.0f;

    return static_cast<i32>(std::floor(height));
}

void StandardTerrainGenerator::initializeBiomes() {
    // 初始化生物群系信息
    // 注意：方块指针在 VanillaBlocks::initialize() 调用后才能使用

    m_biomes[static_cast<size_t>(BiomeType::Plains)] = createBiomeInfo(BiomeType::Plains, 0.0f, 10.0f, 0.5f, 0.5f);
    m_biomes[static_cast<size_t>(BiomeType::Forest)] = createBiomeInfo(BiomeType::Forest, 0.0f, 15.0f, 0.6f, 0.7f);
    m_biomes[static_cast<size_t>(BiomeType::Desert)] = createBiomeInfo(BiomeType::Desert, -5.0f, 5.0f, 2.0f, 0.0f);
    m_biomes[static_cast<size_t>(BiomeType::Mountains)] = createBiomeInfo(BiomeType::Mountains, 30.0f, 80.0f, 0.3f, 0.4f);
    m_biomes[static_cast<size_t>(BiomeType::Ocean)] = createBiomeInfo(BiomeType::Ocean, -30.0f, -5.0f, 0.5f, 0.8f);
    m_biomes[static_cast<size_t>(BiomeType::Snow)] = createBiomeInfo(BiomeType::Snow, 5.0f, 25.0f, -0.5f, 0.6f);
    m_biomes[static_cast<size_t>(BiomeType::Jungle)] = createBiomeInfo(BiomeType::Jungle, 5.0f, 20.0f, 0.9f, 0.9f);
    m_biomes[static_cast<size_t>(BiomeType::Swamp)] = createBiomeInfo(BiomeType::Swamp, -5.0f, 5.0f, 0.7f, 0.9f);
    m_biomes[static_cast<size_t>(BiomeType::Taiga)] = createBiomeInfo(BiomeType::Taiga, 5.0f, 20.0f, -0.2f, 0.6f);
    m_biomes[static_cast<size_t>(BiomeType::Savanna)] = createBiomeInfo(BiomeType::Savanna, 0.0f, 10.0f, 1.5f, 0.2f);
    m_biomes[static_cast<size_t>(BiomeType::Badlands)] = createBiomeInfo(BiomeType::Badlands, 10.0f, 30.0f, 1.8f, 0.1f);

    // 设置方块指针（在方块系统初始化后）
    if (VanillaBlocks::GRASS_BLOCK) {
        m_biomes[static_cast<size_t>(BiomeType::Plains)].surfaceBlock = &VanillaBlocks::GRASS_BLOCK->defaultState();
        m_biomes[static_cast<size_t>(BiomeType::Plains)].subsurfaceBlock = &VanillaBlocks::DIRT->defaultState();
    }
    if (VanillaBlocks::SAND) {
        m_biomes[static_cast<size_t>(BiomeType::Desert)].surfaceBlock = &VanillaBlocks::SAND->defaultState();
        m_biomes[static_cast<size_t>(BiomeType::Desert)].subsurfaceBlock = &VanillaBlocks::SAND->defaultState();
    }
    if (VanillaBlocks::STONE) {
        m_biomes[static_cast<size_t>(BiomeType::Mountains)].surfaceBlock = &VanillaBlocks::STONE->defaultState();
        m_biomes[static_cast<size_t>(BiomeType::Mountains)].subsurfaceBlock = &VanillaBlocks::STONE->defaultState();
    }
    if (VanillaBlocks::WATER) {
        m_biomes[static_cast<size_t>(BiomeType::Ocean)].fillBlock = &VanillaBlocks::WATER->defaultState();
    }
}

BiomeType StandardTerrainGenerator::calculateBiome(i32 x, i32 z) const {
    f32 temperature = m_biomeNoise->noise2D(
        static_cast<f32>(x) * m_config.biomeScale * 0.5f,
        static_cast<f32>(z) * m_config.biomeScale * 0.5f
    );

    f32 humidity = m_biomeNoise->noise2D(
        static_cast<f32>(x) * m_config.biomeScale * 0.7f + 1000.0f,
        static_cast<f32>(z) * m_config.biomeScale * 0.7f + 1000.0f
    );

    // 根据温度和湿度决定生物群系
    if (temperature < 0.2f) {
        // 寒冷生物群系
        if (humidity > 0.5f) {
            return BiomeType::Taiga;
        } else {
            return BiomeType::Snow;
        }
    } else if (temperature < 0.5f) {
        // 温和生物群系
        if (humidity > 0.7f) {
            return BiomeType::Swamp;
        } else if (humidity > 0.4f) {
            return BiomeType::Forest;
        } else {
            return BiomeType::Plains;
        }
    } else if (temperature < 0.8f) {
        // 温暖生物群系
        if (humidity > 0.6f) {
            return BiomeType::Jungle;
        } else if (humidity < 0.3f) {
            return BiomeType::Savanna;
        } else {
            return BiomeType::Plains;
        }
    } else {
        // 炎热生物群系
        if (humidity < 0.2f) {
            return BiomeType::Desert;
        } else if (humidity < 0.4f) {
            return BiomeType::Badlands;
        } else {
            return BiomeType::Desert;
        }
    }
}

void StandardTerrainGenerator::generateTrees(ChunkData& chunk, ChunkData* neighbors[8]) {
    // 如果方块系统未初始化则跳过
    if (!VanillaBlocks::GRASS_BLOCK || !VanillaBlocks::OAK_LOG || !VanillaBlocks::OAK_LEAVES) return;

    // 使用确定性随机
    i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
    i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

    std::mt19937 rng(static_cast<u32>(m_config.seed ^ static_cast<u64>(chunkX) ^ (static_cast<u64>(chunkZ) << 16)));
    std::uniform_int_distribution<i32> xDist(2, world::CHUNK_WIDTH - 3);
    std::uniform_int_distribution<i32> zDist(2, world::CHUNK_WIDTH - 3);
    std::uniform_int_distribution<i32> heightDist(4, 7);
    std::uniform_int_distribution<i32> countDist(0, 100);

    const BlockState* grassState = &VanillaBlocks::GRASS_BLOCK->defaultState();
    const BlockState* logState = &VanillaBlocks::OAK_LOG->defaultState();
    const BlockState* leavesState = &VanillaBlocks::OAK_LEAVES->defaultState();

    // 根据生物群系决定树木数量
    i32 treeCount = 0;
    BiomeType centerBiome = calculateBiome(chunkX + 8, chunkZ + 8);

    switch (centerBiome) {
        case BiomeType::Forest:
            treeCount = 5 + countDist(rng) % 10;
            break;
        case BiomeType::Jungle:
            treeCount = 10 + countDist(rng) % 15;
            break;
        case BiomeType::Taiga:
            treeCount = 3 + countDist(rng) % 5;
            break;
        case BiomeType::Plains:
            treeCount = countDist(rng) % 3; // 少量树
            break;
        case BiomeType::Savanna:
            treeCount = countDist(rng) % 4;
            break;
        default:
            return; // 沙漠、海洋等不生成树
    }

    for (i32 i = 0; i < treeCount; ++i) {
        i32 lx = xDist(rng);
        i32 lz = zDist(rng);

        // 找到地表高度
        i32 groundY = chunk.getHighestBlock(lx, lz);
        if (groundY < 0 || groundY >= world::MAX_BUILD_HEIGHT - 8) continue;

        // 检查是否是草地
        const BlockState* surface = chunk.getBlock(lx, groundY, lz);
        if (surface != grassState) continue;

        i32 treeHeight = heightDist(rng);

        // 生成树干
        for (i32 h = 0; h < treeHeight; ++h) {
            chunk.setBlock(lx, groundY + 1 + h, lz, logState);
        }

        // 生成树叶 (简单球形)
        i32 leafStart = groundY + treeHeight - 2;
        for (i32 dy = 0; dy < 3; ++dy) {
            i32 radius = dy == 2 ? 1 : 2;
            for (i32 dx = -radius; dx <= radius; ++dx) {
                for (i32 dz = -radius; dz <= radius; ++dz) {
                    if (dx == 0 && dz == 0 && dy < 2) continue; // 树干位置
                    if (std::abs(dx) == radius && std::abs(dz) == radius && (rng() % 2) == 0) continue;

                    i32 leafX = lx + dx;
                    i32 leafY = leafStart + dy;
                    i32 leafZ = lz + dz;

                    if (leafX >= 0 && leafX < world::CHUNK_WIDTH &&
                        leafZ >= 0 && leafZ < world::CHUNK_WIDTH) {
                        const BlockState* current = chunk.getBlock(leafX, leafY, leafZ);
                        if (!current || current->isAir()) {
                            chunk.setBlock(leafX, leafY, leafZ, leavesState);
                        }
                    }
                }
            }
        }
    }
}

void StandardTerrainGenerator::generateFlowers(ChunkData& chunk) {
    // TODO: 花朵生成
    (void)chunk;
}

void StandardTerrainGenerator::generateGrass(ChunkData& chunk) {
    // TODO: 草生成
    (void)chunk;
}

// ============================================================================
// FlatTerrainGenerator 实现
// ============================================================================

FlatTerrainGenerator::FlatTerrainGenerator(i32 layers)
    : m_layers(layers)
{
    m_config.seed = 0;
    m_config.terrainHeight = static_cast<f32>(layers);
    m_config.terrainVariation = 0.0f;

    m_defaultBiome = createBiomeInfo(BiomeType::Plains, 0.0f, 0.0f, 0.5f, 0.5f);
    if (VanillaBlocks::GRASS_BLOCK) {
        m_defaultBiome.surfaceBlock = &VanillaBlocks::GRASS_BLOCK->defaultState();
        m_defaultBiome.subsurfaceBlock = &VanillaBlocks::DIRT->defaultState();
    }
}

void FlatTerrainGenerator::generateChunk(ChunkData& chunk) {
    // 获取方块指针
    const BlockState* bedrockState = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;
    const BlockState* dirtState = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->defaultState() : nullptr;
    const BlockState* grassState = VanillaBlocks::GRASS_BLOCK ? &VanillaBlocks::GRASS_BLOCK->defaultState() : nullptr;
    const BlockState* fillState = m_block ? m_block : (VanillaBlocks::STONE ? &VanillaBlocks::STONE->defaultState() : nullptr);

    // 生成平坦地形
    i32 startY = world::MIN_BUILD_HEIGHT + 1;

    // 基岩层
    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
            chunk.setBlock(lx, world::MIN_BUILD_HEIGHT, lz, bedrockState);
        }
    }

    // 填充层
    for (i32 y = startY; y < startY + m_layers - 2; ++y) {
        for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
            for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
                chunk.setBlock(lx, y, lz, fillState);
            }
        }
    }

    // 次表层 (泥土)
    i32 subsurfaceY = startY + m_layers - 2;
    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
            chunk.setBlock(lx, subsurfaceY, lz, dirtState);
        }
    }

    // 表层 (草地)
    i32 surfaceY = startY + m_layers - 1;
    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
            chunk.setBlock(lx, surfaceY, lz, grassState);
        }
    }

    chunk.setFullyGenerated(true);
    chunk.setDirty(true);
}

void FlatTerrainGenerator::populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) {
    // 平坦世界不需要装饰
    (void)chunk;
    (void)neighbors;
}

BiomeType FlatTerrainGenerator::getBiome(i32 x, i32 z) const {
    (void)x;
    (void)z;
    return BiomeType::Plains;
}

const BiomeInfo& FlatTerrainGenerator::getBiomeInfo(BiomeType type) const {
    (void)type;
    return m_defaultBiome;
}

i32 FlatTerrainGenerator::getHeight(i32 x, i32 z) const {
    (void)x;
    (void)z;
    return world::MIN_BUILD_HEIGHT + m_layers;
}

// ============================================================================
// EmptyTerrainGenerator 实现
// ============================================================================

void EmptyTerrainGenerator::generateChunk(ChunkData& chunk) {
    // 空世界不生成任何方块
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
}

void EmptyTerrainGenerator::populateChunk(ChunkData& chunk, ChunkData* neighbors[8]) {
    (void)chunk;
    (void)neighbors;
}

BiomeType EmptyTerrainGenerator::getBiome(i32 x, i32 z) const {
    (void)x;
    (void)z;
    return BiomeType::Plains;
}

const BiomeInfo& EmptyTerrainGenerator::getBiomeInfo(BiomeType type) const {
    (void)type;
    return m_defaultBiome;
}

i32 EmptyTerrainGenerator::getHeight(i32 x, i32 z) const {
    (void)x;
    (void)z;
    return world::MIN_BUILD_HEIGHT;
}

// ============================================================================
// 生成器工厂实现
// ============================================================================

namespace TerrainGenFactory {

std::unique_ptr<ITerrainGenerator> create(GeneratorType type, const WorldGenConfig& config) {
    switch (type) {
        case GeneratorType::Standard:
            return std::make_unique<StandardTerrainGenerator>(config);
        case GeneratorType::Flat:
            return std::make_unique<FlatTerrainGenerator>();
        case GeneratorType::Empty:
            return std::make_unique<EmptyTerrainGenerator>();
        default:
            return std::make_unique<StandardTerrainGenerator>(config);
    }
}

std::unique_ptr<ITerrainGenerator> createStandard(u64 seed) {
    WorldGenConfig config;
    config.seed = seed;
    return std::make_unique<StandardTerrainGenerator>(config);
}

std::unique_ptr<ITerrainGenerator> createFlat(i32 layers) {
    return std::make_unique<FlatTerrainGenerator>(layers);
}

std::unique_ptr<ITerrainGenerator> createEmpty() {
    return std::make_unique<EmptyTerrainGenerator>();
}

} // namespace TerrainGenFactory

} // namespace mr
