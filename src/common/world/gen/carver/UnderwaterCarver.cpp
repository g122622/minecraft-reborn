#include "UnderwaterCarver.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::world::gen::carver {

// ============================================================================
// UnderwaterCaveCarver 实现
// ============================================================================

UnderwaterCaveCarver::UnderwaterCaveCarver()
    : WorldCarver<UnderwaterCaveConfig>(256)
{
}

bool UnderwaterCaveCarver::carve(ChunkPrimer& chunk,
                                  const BiomeProvider& biomeProvider,
                                  i32 seaLevel,
                                  ChunkCoord chunkX,
                                  ChunkCoord chunkZ,
                                  CarvingMask& carvingMask,
                                  const UnderwaterCaveConfig& config) {
    (void)biomeProvider;
    (void)carvingMask;

    math::Random rng((chunkX * 3418731287LL + chunkZ * 132897987541LL) & 0xFFFFFFFF);

    if (rng.nextFloat() >= config.probability) {
        return false;
    }

    // 预缓存方块状态，避免在热循环中重复查找
    const BlockState* waterState = VanillaBlocks::getState(VanillaBlocks::WATER);

    // 生成起始点
    i32 startX = rng.nextInt(16);
    i32 startZ = rng.nextInt(16);
    i32 startY = rng.nextInt(seaLevel - 8) + 8;

    // 生成洞穴隧道
    i32 length = config.minLength + rng.nextInt(config.maxLength - config.minLength);
    f32 yaw = rng.nextFloat() * 6.283185f;  // 2 * PI
    f32 pitch = (rng.nextFloat() - 0.5f) * 0.5f;

    f64 x = static_cast<f64>((chunkX << 4) + startX);
    f64 y = static_cast<f64>(startY);
    f64 z = static_cast<f64>((chunkZ << 4) + startZ);

    for (i32 i = 0; i < length; ++i) {
        // 计算当前位置的半径
        i32 radius = 1 + rng.nextInt(2);

        // 更新位置
        x += std::cos(yaw) * std::cos(pitch);
        y += std::sin(pitch);
        z += std::sin(yaw) * std::cos(pitch);

        // 随机调整角度
        yaw += (rng.nextFloat() - 0.5f) * 0.1f;
        pitch += (rng.nextFloat() - 0.5f) * 0.05f;
        pitch = std::clamp(pitch, -0.5f, 0.5f);

        // 检查是否在当前区块内
        i32 blockX = static_cast<i32>(x);
        i32 blockY = static_cast<i32>(y);
        i32 blockZ = static_cast<i32>(z);

        // 计算相对于区块的坐标
        i32 localX = blockX - (chunkX << 4);
        i32 localZ = blockZ - (chunkZ << 4);

        if (localX < -radius || localX >= 16 + radius ||
            localZ < -radius || localZ >= 16 + radius) {
            continue;
        }

        // 只在水面以下生成
        if (blockY >= seaLevel || blockY < 0) {
            continue;
        }

        // 雕刻球形区域 - 填充水
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dy = -radius; dy <= radius; ++dy) {
                for (i32 dz = -radius; dz <= radius; ++dz) {
                    f32 dist = static_cast<f32>(dx * dx + dy * dy + dz * dz);
                    f32 radiusSq = static_cast<f32>(radius * radius);

                    if (dist <= radiusSq) {
                        i32 lx = localX + dx;
                        i32 ly = blockY + dy;
                        i32 lz = localZ + dz;

                        if (lx >= 0 && lx < 16 && ly >= 0 && ly < 256 && lz >= 0 && lz < 16) {
                            const BlockState* state = chunk.getBlock(lx, ly, lz);
                            if (state && (state->isAir() ||
                                         state->is(VanillaBlocks::STONE) ||
                                         state->is(VanillaBlocks::DIRT) ||
                                         state->is(VanillaBlocks::GRAVEL) ||
                                         state->is(VanillaBlocks::SAND))) {
                                chunk.setBlock(lx, ly, lz, waterState);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool UnderwaterCaveCarver::shouldCarve(
    math::IRandom& rng,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    const UnderwaterCaveConfig& config) const {
    math::Random random((chunkX * 3418731287LL + chunkZ * 132897987541LL) & 0xFFFFFFFF);
    return random.nextFloat() < config.probability;
}

bool UnderwaterCaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const {
    // 水下洞穴不跳过任何位置
    (void)dx;
    (void)dy;
    (void)dz;
    (void)y;
    return false;
}

std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver() {
    return std::make_unique<UnderwaterCaveCarver>();
}

// ============================================================================
// UnderwaterCanyonCarver 实现
// ============================================================================

UnderwaterCanyonCarver::UnderwaterCanyonCarver()
    : WorldCarver<UnderwaterCanyonConfig>(256)
{
}

bool UnderwaterCanyonCarver::carve(ChunkPrimer& chunk,
                                    const BiomeProvider& biomeProvider,
                                    i32 seaLevel,
                                    ChunkCoord chunkX,
                                    ChunkCoord chunkZ,
                                    CarvingMask& carvingMask,
                                    const UnderwaterCanyonConfig& config) {
    (void)biomeProvider;
    (void)carvingMask;

    math::Random rng((chunkX * 3418731287LL + chunkZ * 132897987541LL + 1) & 0xFFFFFFFF);

    if (rng.nextFloat() >= config.probability) {
        return false;
    }

    // 预缓存方块状态，避免在热循环中重复查找
    const BlockState* waterState = VanillaBlocks::getState(VanillaBlocks::WATER);

    // 生成起始点
    i32 startX = rng.nextInt(16);
    i32 startZ = rng.nextInt(16);
    i32 startY = rng.nextInt(seaLevel - 20) + 10;

    // 生成峡谷
    i32 length = config.minLength + rng.nextInt(config.maxLength - config.minLength);
    f32 yaw = rng.nextFloat() * 6.283185f;
    f32 pitch = (rng.nextFloat() - 0.5f) * 0.2f;

    f64 x = static_cast<f64>((chunkX << 4) + startX);
    f64 y = static_cast<f64>(startY);
    f64 z = static_cast<f64>((chunkZ << 4) + startZ);

    for (i32 i = 0; i < length; ++i) {
        // 峡谷厚度
        f32 thickness = config.thickness * (1.0f + (rng.nextFloat() - 0.5f) * 0.5f);

        // 更新位置
        x += std::cos(yaw) * std::cos(pitch) * 2.0;
        y += std::sin(pitch) * 0.5;
        z += std::sin(yaw) * std::cos(pitch) * 2.0;

        // 随机调整角度
        yaw += (rng.nextFloat() - 0.5f) * 0.05f;
        pitch += (rng.nextFloat() - 0.5f) * 0.02f;

        // 检查是否在当前区块内
        i32 blockX = static_cast<i32>(x);
        i32 blockY = static_cast<i32>(y);
        i32 blockZ = static_cast<i32>(z);

        i32 localX = blockX - (chunkX << 4);
        i32 localZ = blockZ - (chunkZ << 4);
        i32 halfThickness = static_cast<i32>(thickness);

        if (localX < -halfThickness || localX >= 16 + halfThickness ||
            localZ < -halfThickness || localZ >= 16 + halfThickness) {
            continue;
        }

        // 只在水面以下生成
        if (blockY >= seaLevel || blockY < 0) {
            continue;
        }

        // 雕刻峡谷形状 - 填充水
        for (i32 dx = -halfThickness; dx <= halfThickness; ++dx) {
            for (i32 dy = -halfThickness / 2; dy <= halfThickness / 2; ++dy) {
                for (i32 dz = -halfThickness; dz <= halfThickness; ++dz) {
                    // 峡谷形状：水平方向宽，垂直方向窄
                    f32 dist = static_cast<f32>(dx * dx + dz * dz) / (thickness * thickness) +
                               static_cast<f32>(dy * dy * 4) / (thickness * thickness);

                    if (dist <= 1.0f) {
                        i32 lx = localX + dx;
                        i32 ly = blockY + dy;
                        i32 lz = localZ + dz;

                        if (lx >= 0 && lx < 16 && ly >= 0 && ly < 256 && lz >= 0 && lz < 16) {
                            const BlockState* state = chunk.getBlock(lx, ly, lz);
                            if (state && (state->isAir() ||
                                         state->is(VanillaBlocks::STONE) ||
                                         state->is(VanillaBlocks::DIRT) ||
                                         state->is(VanillaBlocks::GRAVEL) ||
                                         state->is(VanillaBlocks::SAND))) {
                                chunk.setBlock(lx, ly, lz, waterState);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool UnderwaterCanyonCarver::shouldCarve(
    math::IRandom& rng,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    const UnderwaterCanyonConfig& config) const {
    math::Random random((chunkX * 3418731287LL + chunkZ * 132897987541LL + 1) & 0xFFFFFFFF);
    return random.nextFloat() < config.probability;
}

bool UnderwaterCanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const {
    (void)dx;
    (void)dy;
    (void)dz;
    (void)y;
    return false;
}

std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver() {
    return std::make_unique<UnderwaterCanyonCarver>();
}

} // namespace mc::world::gen::carver
