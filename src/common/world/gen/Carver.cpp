#include "Carver.hpp"
#include "NoiseChunkGenerator.hpp"
#include "../chunk/ChunkPrimer.hpp"
#include "../block/BlockRegistry.hpp"
#include "../../math/MathUtils.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// 常量
// ============================================================================

// 洞穴生成的常量（参考 MC CaveWorldCarver）
constexpr f32 PI = 3.14159265358979323846f;
constexpr f64 CAVE_MIN_Y = 8.0;
constexpr f64 CAVE_MAX_Y_FACTOR = 0.75;  // 最大 Y 比例

// ============================================================================
// CaveCarver 实现
// ============================================================================

CaveCarver::CaveCarver(u64 seed, const CarverConfig& config)
    : m_seed(seed)
    , m_config(config)
    , m_rng(seed)
{
}

bool CaveCarver::shouldCarve(std::mt19937_64& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    // 使用区块坐标生成确定性随机
    std::mt19937_64 localRng(static_cast<u64>(chunkX) * 341873128712ULL +
                             static_cast<u64>(chunkZ) * 132897987541ULL +
                             m_seed);
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    return dist(localRng) <= m_config.probability;
}

bool CaveCarver::carve(ChunkPrimer& chunk,
                        const BiomeProvider& biomeProvider,
                        i32 seaLevel,
                        ChunkCoord chunkX,
                        ChunkCoord chunkZ)
{
    // 设置区块特定的随机种子
    std::mt19937_64 rng(static_cast<u64>(chunkX) * 341873128712ULL +
                        static_cast<u64>(chunkZ) * 132897987541ULL +
                        m_seed);

    // 检查是否应该在这个区块生成洞穴
    if (!shouldCarve(rng, chunkX, chunkZ)) {
        return false;
    }

    bool carved = false;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 确定洞穴数量
    std::uniform_int_distribution<i32> caveCountDist(0, std::max(1, m_config.minGenerationAttempts / 2));
    const i32 numCaves = caveCountDist(rng);

    for (i32 i = 0; i < numCaves; ++i) {
        // 随机起始位置
        std::uniform_real_distribution<f64> xPosDist(0.0, 16.0);
        std::uniform_real_distribution<f64> zPosDist(0.0, 16.0);

        const f64 startXPos = static_cast<f64>(startX) + xPosDist(rng);
        const f64 startZPos = static_cast<f64>(startZ) + zPosDist(rng);
        const f64 startYPos = static_cast<f64>(getCaveStartY(rng));

        // 随机洞穴半径
        const f32 radius = getCaveRadius(rng);

        // 随机方向
        std::uniform_real_distribution<f32> angleDist(0.0f, 2.0f * PI);
        std::uniform_real_distribution<f32> pitchDist(-0.5f, 0.5f);

        const f32 yaw = angleDist(rng);
        const f32 pitch = pitchDist(rng);

        // 洞穴长度
        std::uniform_int_distribution<i32> lengthDist(20, 120);
        const i32 length = lengthDist(rng);

        // 有概率生成大型圆形房间
        std::uniform_int_distribution<i32> roomChance(0, 3);
        if (roomChance(rng) == 0) {
            carveRoom(chunk, seaLevel, chunkX, chunkZ,
                      startXPos, startYPos, startZPos,
                      radius * 1.5f, 0.5);
            carved = true;
        }

        // 生成隧道
        carveTunnel(chunk, seaLevel, chunkX, chunkZ,
                    startXPos, startYPos, startZPos,
                    radius, yaw, pitch,
                    0, length, 1.0, 1.0);
        carved = true;
    }

    return carved;
}

void CaveCarver::carveTunnel(ChunkPrimer& chunk,
                              i32 seaLevel,
                              ChunkCoord chunkX,
                              ChunkCoord chunkZ,
                              f64 startX, f64 startY, f64 startZ,
                              f32 radius,
                              f32 yaw, f32 pitch,
                              i32 startIdx, i32 endIdx,
                              f64 yawMod, f64 pitchMod)
{
    // 参考 MC CaveWorldCarver.func_227206_a_
    std::mt19937_64 rng(static_cast<u64>(startX * 3251.0 + startZ * 7129.0 + m_seed));
    std::uniform_real_distribution<f32> randomDist(0.0f, 1.0f);

    // 分支点
    const i32 branchPoint = endIdx / 2 + rng() % (endIdx / 4);
    const bool canBranch = (rng() % 6) == 0;

    f32 currentYaw = yaw;
    f32 currentPitch = pitch;
    f32 currentX = static_cast<f32>(startX);
    f32 currentY = static_cast<f32>(startY);
    f32 currentZ = static_cast<f32>(startZ);

    for (i32 i = startIdx; i < endIdx; ++i) {
        // 椭球半径随距离变化
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIdx);
        const f32 sinProgress = std::sin(progress * PI);
        const f32 horizontalRadius = radius * sinProgress;
        const f32 verticalRadius = horizontalRadius * 0.6f;

        // 更新位置
        currentX += std::cos(currentYaw) * std::cos(currentPitch);
        currentY += std::sin(currentPitch);
        currentZ += std::sin(currentYaw) * std::cos(currentPitch);

        // 更新角度
        currentPitch *= 0.7f;
        currentYaw += yawMod * 0.1f;

        // 添加随机扰动
        yawMod = yawMod * 0.9f + (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 2.0f;
        pitchMod = pitchMod * 0.75f + (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 4.0f;

        // 在分支点生成分支
        if (canBranch && i == branchPoint && radius > 1.0f) {
            // 生成两个分支
            std::uniform_real_distribution<f32> branchRadiusDist(0.4f, 0.7f);
            const f32 branchRadius = radius * branchRadiusDist(rng);

            carveTunnel(chunk, seaLevel, chunkX, chunkZ,
                        currentX, currentY, currentZ,
                        branchRadius,
                        currentYaw - PI / 2.0f, currentPitch / 3.0f,
                        i, endIdx, yawMod, pitchMod);
            carveTunnel(chunk, seaLevel, chunkX, chunkZ,
                        currentX, currentY, currentZ,
                        branchRadius,
                        currentYaw + PI / 2.0f, currentPitch / 3.0f,
                        i, endIdx, yawMod, pitchMod);
            return;
        }

        // 检查是否在当前区块范围内
        if (rng() % 4 == 0) {
            continue;  // 跳过一些雕刻以增加洞穴的不规则性
        }

        // 雕刻椭球
        if (isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIdx, horizontalRadius)) {
            carveEllipsoid(chunk, chunkX, chunkZ,
                           currentX, currentY, currentZ,
                           horizontalRadius, verticalRadius);
        }
    }
}

void CaveCarver::carveRoom(ChunkPrimer& chunk,
                            i32 seaLevel,
                            ChunkCoord chunkX,
                            ChunkCoord chunkZ,
                            f64 centerX, f64 centerY, f64 centerZ,
                            f32 radius,
                            f64 verticalScale)
{
    // 生成一个椭圆形房间
    const f64 horizontalRadius = radius * 1.5;
    const f64 verticalRadius = radius * verticalScale;

    carveEllipsoid(chunk, chunkX, chunkZ,
                   centerX, centerY, centerZ,
                   horizontalRadius, verticalRadius);
}

bool CaveCarver::carveEllipsoid(ChunkPrimer& chunk,
                                 ChunkCoord chunkX, ChunkCoord chunkZ,
                                 f64 centerX, f64 centerY, f64 centerZ,
                                 f64 horizontalRadius, f64 verticalRadius)
{
    // 参考 MC WorldCarver.func_227208_a_
    const i32 startX = static_cast<i32>(centerX - horizontalRadius - 1.0);
    const i32 endX = static_cast<i32>(centerX + horizontalRadius + 1.0);
    const i32 startY = static_cast<i32>(centerY - verticalRadius - 1.0);
    const i32 endY = static_cast<i32>(centerY + verticalRadius + 1.0);
    const i32 startZ = static_cast<i32>(centerZ - horizontalRadius - 1.0);
    const i32 endZ = static_cast<i32>(centerZ + horizontalRadius + 1.0);

    // 边界检查
    const i32 chunkStartX = chunkX << 4;
    const i32 chunkEndX = chunkStartX + 15;
    const i32 chunkStartZ = chunkZ << 4;
    const i32 chunkEndZ = chunkStartZ + 15;

    // 如果椭球完全在区块外，跳过
    if (endX < chunkStartX || startX > chunkEndX ||
        endZ < chunkStartZ || startZ > chunkEndZ) {
        return false;
    }

    // 限制在区块范围内
    const i32 localStartX = std::max(0, startX - chunkStartX);
    const i32 localEndX = std::min(15, endX - chunkStartX);
    const i32 localStartZ = std::max(0, startZ - chunkStartZ);
    const i32 localEndZ = std::min(15, endZ - chunkStartZ);

    bool carved = false;
    const f64 hRadiusSq = horizontalRadius * horizontalRadius;
    const f64 vRadiusSq = verticalRadius * verticalRadius;

    for (i32 lx = localStartX; lx <= localEndX; ++lx) {
        const i32 worldX = (chunkX << 4) + lx;
        const f64 dx = static_cast<f64>(worldX) + 0.5 - centerX;
        const f64 dxSq = dx * dx;

        for (i32 lz = localStartZ; lz <= localEndZ; ++lz) {
            const i32 worldZ = (chunkZ << 4) + lz;
            const f64 dz = static_cast<f64>(worldZ) + 0.5 - centerZ;
            const f64 dzSq = dz * dz;

            // 检查是否在椭球范围内
            if (dxSq + dzSq >= hRadiusSq) {
                continue;
            }

            for (i32 y = startY; y <= endY; ++y) {
                // 边界检查
                if (y < 1 || y >= 256) {
                    continue;
                }

                const f64 dy = static_cast<f64>(y) + 0.5 - centerY;

                // 椭球方程检查
                const f64 distSq = dxSq / hRadiusSq + dy * dy / vRadiusSq + dzSq / hRadiusSq;
                if (distSq >= 1.0) {
                    continue;
                }

                // 获取当前方块
                const BlockState* state = chunk.getBlock(lx, y, lz);
                if (!state) {
                    continue;
                }

                const BlockId blockId = static_cast<BlockId>(state->blockId());

                // 检查是否可以雕刻
                if (!isCarvable(blockId)) {
                    continue;
                }

                // Y < 11 生成熔岩，否则生成空气
                // 同时检查上方是否有草地，如果有则替换为泥土
                const BlockState* aboveState = y < 255 ? chunk.getBlock(lx, y + 1, lz) : nullptr;
                if (aboveState && aboveState->blockId() == static_cast<u32>(BlockId::Grass)) {
                    const BlockState* dirt = BlockRegistry::instance().get(BlockId::Dirt);
                    if (dirt) {
                        chunk.setBlock(lx, y + 1, lz, dirt);
                    }
                }

                // 设置为空气或熔岩
                if (y < 11) {
                    const BlockState* lava = BlockRegistry::instance().get(BlockId::Lava);
                    if (lava) {
                        chunk.setBlock(lx, y, lz, lava);
                    }
                } else {
                    const BlockState* air = BlockRegistry::instance().get(BlockId::Air);
                    if (air) {
                        chunk.setBlock(lx, y, lz, air);
                    }
                }
                carved = true;
            }
        }
    }

    return carved;
}

bool CaveCarver::isCarvable(BlockId block)
{
    // 只雕刻已定义的方块类型
    // 参考 MC WorldCarver 的 carvableBlocks
    switch (block) {
        case BlockId::Stone:
        case BlockId::Dirt:
        case BlockId::Grass:
        case BlockId::Sand:
        case BlockId::Gravel:
        case BlockId::Terracotta:
        case BlockId::RedSand:
        case BlockId::Snow:  // 雪
        case BlockId::Netherrack:
        case BlockId::EndStone:
            return true;
        default:
            return false;
    }
}

i32 CaveCarver::getCaveStartY(std::mt19937_64& rng) const
{
    // 参考 MC: return rand.nextInt(rand.nextInt(120) + 8);
    std::uniform_int_distribution<i32> dist1(0, 120);
    std::uniform_int_distribution<i32> dist2(0, 1);
    const i32 inner = dist1(rng);
    std::uniform_int_distribution<i32> dist3(0, inner + 8);
    return dist3(rng);
}

f32 CaveCarver::getCaveRadius(std::mt19937_64& rng) const
{
    // 参考 MC: float f = rand.nextFloat() * 2.0F + rand.nextFloat();
    //         if (rand.nextInt(10) == 0) { f *= rand.nextFloat() * rand.nextFloat() * 3.0F + 1.0F; }
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    f32 radius = dist(rng) * 2.0f + dist(rng);

    std::uniform_int_distribution<i32> largeDist(0, 9);
    if (largeDist(rng) == 0) {
        radius *= dist(rng) * dist(rng) * 3.0f + 1.0f;
    }

    return radius;
}

bool CaveCarver::isInCarvingRange(ChunkCoord chunkX, ChunkCoord chunkZ,
                                   f64 x, f64 z,
                                   i32 step, i32 maxSteps,
                                   f32 radius)
{
    // 参考 MC WorldCarver.func_222702_a_
    const f64 chunkCenterX = static_cast<f64>(chunkX * 16 + 8);
    const f64 chunkCenterZ = static_cast<f64>(chunkZ * 16 + 8);

    const f64 dx = x - chunkCenterX;
    const f64 dz = z - chunkCenterZ;

    const f64 remainingSteps = static_cast<f64>(maxSteps - step);
    const f64 maxDist = static_cast<f64>(radius + 2.0f + 16.0f);

    return dx * dx + dz * dz - remainingSteps * remainingSteps <= maxDist * maxDist;
}

// ============================================================================
// CanyonCarver 实现
// ============================================================================

CanyonCarver::CanyonCarver(u64 seed, const CarverConfig& config)
    : m_seed(seed)
    , m_config(config)
    , m_rng(seed)
{
}

bool CanyonCarver::shouldCarve(std::mt19937_64& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    // 峡谷比洞穴更稀少
    std::mt19937_64 localRng(static_cast<u64>(chunkX) * 341873128712ULL +
                             static_cast<u64>(chunkZ) * 132897987541ULL +
                             m_seed);
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    return dist(localRng) <= m_config.probability * 0.1f;  // 峡谷概率更低
}

bool CanyonCarver::carve(ChunkPrimer& chunk,
                          const BiomeProvider& biomeProvider,
                          i32 seaLevel,
                          ChunkCoord chunkX,
                          ChunkCoord chunkZ)
{
    // 峡谷实现 - 简化版本，生成一个狭长的切面
    // TODO: 完整的峡谷生成需要更复杂的逻辑
    std::mt19937_64 rng(static_cast<u64>(chunkX) * 341873128712ULL +
                        static_cast<u64>(chunkZ) * 132897987541ULL +
                        m_seed);

    if (!shouldCarve(rng, chunkX, chunkZ)) {
        return false;
    }

    // 简化的峡谷生成 - 只生成一个垂直切面
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    std::uniform_int_distribution<i32> xDist(4, 12);
    std::uniform_int_distribution<i32> zDist(4, 12);
    std::uniform_int_distribution<i32> yDist(20, 50);

    const i32 centerX = xDist(rng);
    const i32 centerY = yDist(rng);
    const i32 centerZ = zDist(rng);

    std::uniform_int_distribution<i32> heightDist(20, 40);
    std::uniform_real_distribution<f64> widthDist(1.0, 2.0);

    const i32 height = heightDist(rng);
    const f64 width = widthDist(rng);
    const f64 depth = width * 8.0;  // 峡谷很窄但很深

    // 生成峡谷
    for (i32 y = centerY - height / 2; y < centerY + height / 2; ++y) {
        if (y < 1 || y >= 256) continue;

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                const f64 dx = static_cast<f64>(x - centerX);
                const f64 dy = static_cast<f64>(y - centerY);
                const f64 dz = static_cast<f64>(z - centerZ);

                // 狭长的椭球形状
                const f64 distSq = dx * dx / (depth * depth) +
                                   dy * dy / (static_cast<f64>(height * height) / 4.0) +
                                   dz * dz / (width * width);

                if (distSq < 1.0) {
                    const BlockState* state = chunk.getBlock(x, y, z);
                    if (state) {
                        BlockId blockId = static_cast<BlockId>(state->blockId());
                        if (CaveCarver::isCarvable(blockId)) {
                            if (y < 11) {
                                const BlockState* lava = BlockRegistry::instance().get(BlockId::Lava);
                                if (lava) chunk.setBlock(x, y, z, lava);
                            } else {
                                const BlockState* air = BlockRegistry::instance().get(BlockId::Air);
                                if (air) chunk.setBlock(x, y, z, air);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

} // namespace mr
