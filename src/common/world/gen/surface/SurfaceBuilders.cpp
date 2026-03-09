#include "SurfaceBuilders.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../biome/Biome.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// DefaultSurfaceBuilder 实现
// ============================================================================

void DefaultSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;  // 基类实现不直接使用生物群系
    (void)defaultFluid;

    // 获取方块状态
    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* topState = registry.get(config.topBlock);
    const BlockState* underState = registry.get(config.underBlock);
    const BlockState* underWaterState = registry.get(config.underWaterBlock);

    if (!topState || !underState || !underWaterState || !defaultBlock) {
        return;
    }

    // 计算地表深度
    i32 depth = calculateDepth(surfaceNoise, random);

    // 从上到下遍历
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            // 空气，重置深度
            currentDepth = -1;
            continue;
        }

        // 检查是否是石头（默认方块）
        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 到达地表
                if (y >= seaLevel - 4) {
                    // 水面以上，放置表层
                    chunk.setBlock(x, y, z, topState);
                } else {
                    // 水面以下，放置水下表面
                    chunk.setBlock(x, y, z, underWaterState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 放置次层
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

i32 DefaultSurfaceBuilder::calculateDepth(f32 noise, math::Random& random) const
{
    // 参考 MC DefaultSurfaceBuilder
    // 地表深度 = noise / 3.0 + 3.0 + random
    i32 depth = static_cast<i32>(noise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    return std::max(1, depth);
}

// ============================================================================
// MountainSurfaceBuilder 实现
// ============================================================================

void MountainSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* topState = registry.get(config.topBlock);
    const BlockState* underState = registry.get(config.underBlock);
    const BlockState* snowState = registry.get(BlockId::Snow);
    const BlockState* stoneState = registry.get(BlockId::Stone);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 高海拔处放置雪
                if (shouldPlaceSnow(y, biome)) {
                    if (snowState) {
                        chunk.setBlock(x, y, z, snowState);
                    } else if (stoneState) {
                        chunk.setBlock(x, y, z, stoneState);
                    }
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

bool MountainSurfaceBuilder::shouldPlaceSnow(i32 y, const Biome& biome) const
{
    // 温度低于 0.15 且高度高于 90 时放置雪
    return biome.temperature() < 0.15f && y > 90;
}

// ============================================================================
// DesertSurfaceBuilder 实现
// ============================================================================

void DesertSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;
    (void)seaLevel;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* sandState = registry.get(config.topBlock);
    const BlockState* sandstoneState = registry.get(BlockId::Stone);  // 用 Stone 代替 Sandstone

    if (!sandState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                chunk.setBlock(x, y, z, sandState);
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 沙漠下层使用石头（作为砂岩替代）
                if (sandstoneState) {
                    chunk.setBlock(x, y, z, sandstoneState);
                } else {
                    chunk.setBlock(x, y, z, sandState);
                }
                --currentDepth;
            }
        }
    }
}

// ============================================================================
// SwampSurfaceBuilder 实现
// ============================================================================

void SwampSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)biome;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* topState = registry.get(config.topBlock);
    const BlockState* underState = registry.get(config.underBlock);
    const BlockState* clayState = registry.get(BlockId::Terracotta);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 水面附近可能放置粘土
                if (y < seaLevel && shouldPlaceClay(surfaceNoise)) {
                    if (clayState) {
                        chunk.setBlock(x, y, z, clayState);
                    } else {
                        chunk.setBlock(x, y, z, topState);
                    }
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

bool SwampSurfaceBuilder::shouldPlaceClay(f32 noise) const
{
    // 噪声值大于阈值时放置粘土
    return noise > 0.5;
}

// ============================================================================
// FrozenOceanSurfaceBuilder 实现
// ============================================================================

void FrozenOceanSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* topState = registry.get(config.topBlock);
    const BlockState* underState = registry.get(config.underBlock);
    const BlockState* iceState = registry.get(BlockId::Ice);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 水面可能结冰
                if (y == seaLevel && iceState) {
                    chunk.setBlock(x, y, z, iceState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

// ============================================================================
// BadlandsSurfaceBuilder 实现
// ============================================================================

void BadlandsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;
    (void)seaLevel;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* terracottaState = registry.get(BlockId::Terracotta);
    const BlockState* redSandState = registry.get(BlockId::RedSand);
    const BlockState* topState = registry.get(config.topBlock);

    if (!topState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;
    i32 terracottaDepth = 0;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            terracottaDepth = 0;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 表层使用红沙
                if (redSandState) {
                    chunk.setBlock(x, y, z, redSandState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
                terracottaDepth = 0;
            } else if (currentDepth > 0) {
                // 下层使用彩色陶瓦
                if (terracottaState) {
                    BlockId terracottaLayer = getTerracottaLayer(terracottaDepth);
                    const BlockState* layerState = registry.get(terracottaLayer);
                    if (layerState) {
                        chunk.setBlock(x, y, z, layerState);
                    } else {
                        chunk.setBlock(x, y, z, terracottaState);
                    }
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                --currentDepth;
                ++terracottaDepth;
            }
        }
    }
}

BlockId BadlandsSurfaceBuilder::getTerracottaLayer(i32 y) const
{
    // 简化的彩色陶瓦层 - 实际 MC 有更复杂的逻辑
    // 根据高度返回不同颜色的陶瓦
    (void)y;  // 目前简化处理
    return BlockId::Terracotta;
}

// ============================================================================
// BeachSurfaceBuilder 实现
// ============================================================================

void BeachSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;

    BlockRegistry& registry = BlockRegistry::instance();
    const BlockState* sandState = registry.get(config.topBlock);
    const BlockState* sandstoneState = registry.get(BlockId::Stone);  // 用 Stone 代替 Sandstone
    const BlockState* topState = registry.get(config.underBlock);

    if (!sandState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 海平面附近使用沙子
                if (y <= seaLevel + 2) {
                    chunk.setBlock(x, y, z, sandState);
                } else if (topState) {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                if (y <= seaLevel && sandstoneState) {
                    chunk.setBlock(x, y, z, sandstoneState);
                } else {
                    chunk.setBlock(x, y, z, sandState);
                }
                --currentDepth;
            }
        }
    }
}

} // namespace mr
