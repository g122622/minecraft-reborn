#include "CaveCarver.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../../math/MathUtils.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// 常量
// ============================================================================

constexpr f32 PI = 3.14159265358979323846f;

// ============================================================================
// CaveCarver 实现
// ============================================================================

CaveCarver::CaveCarver(i32 maxHeight)
    : WorldCarver<ProbabilityConfig>(maxHeight)
{
}

bool CaveCarver::shouldCarve(
    std::mt19937_64& rng,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    const ProbabilityConfig& config) const
{
    // 使用区块坐标生成确定性随机
    std::mt19937_64 localRng(static_cast<u64>(chunkX) * 341873128712ULL +
                             static_cast<u64>(chunkZ) * 132897987541ULL +
                             static_cast<u64>(m_maxHeight));
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    return dist(localRng) <= config.probability;
}

bool CaveCarver::carve(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    // 参考 MC CaveWorldCarver.carveRegion
    std::mt19937_64 rng(static_cast<u64>(chunkX) * 341873128712ULL +
                        static_cast<u64>(chunkZ) * 132897987541ULL +
                        static_cast<u64>(m_maxHeight));

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    // 隧道长度范围
    const i32 tunnelLength = (getRange() * 2 - 1) * 16;

    // 确定洞穴数量
    // 参考 MC: int j = rand.nextInt(rand.nextInt(rand.nextInt(this.func_230357_a_()) + 1) + 1);
    std::uniform_int_distribution<i32> countDist1(0, getMaxCaveCount());
    std::uniform_int_distribution<i32> countDist2(0, countDist1(rng) + 1);
    const i32 numCaves = countDist2(rng);

    bool carved = false;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    for (i32 i = 0; i < numCaves; ++i) {
        // 随机起始位置
        std::uniform_real_distribution<f64> xPosDist(0.0, 16.0);
        std::uniform_real_distribution<f64> zPosDist(0.0, 16.0);

        const f64 startXPos = static_cast<f64>(startX) + xPosDist(rng);
        const f64 startZPos = static_cast<f64>(startZ) + zPosDist(rng);
        const f64 startYPos = static_cast<f64>(getCaveStartY(rng));

        // 有概率生成大型圆形房间
        std::uniform_int_distribution<i32> roomChance(0, 3);
        i32 numTunnels = 1;

        if (roomChance(rng) == 0) {
            // 生成房间
            std::uniform_real_distribution<f32> roomRadiusDist(1.0f, 7.0f);
            const f32 roomRadius = roomRadiusDist(rng);
            carveRoom(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                      static_cast<i64>(rng()),
                      startXPos, startYPos, startZPos,
                      roomRadius, 0.5,
                      carvingMask);
            numTunnels += std::uniform_int_distribution<i32>(0, 4)(rng);
        }

        // 生成隧道
        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            // 随机方向
            std::uniform_real_distribution<f32> angleDist(0.0f, 2.0f * PI);
            std::uniform_real_distribution<f32> pitchDist(-0.25f, 0.25f);

            const f32 yaw = angleDist(rng);
            const f32 pitch = pitchDist(rng);
            const f32 radius = getCaveRadius(rng);

            // 隧道长度
            std::uniform_int_distribution<i32> lengthDist(0, tunnelLength / 4);
            const i32 length = tunnelLength - lengthDist(rng);

            carveTunnel(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                        static_cast<i64>(rng()),
                        startXPos, startYPos, startZPos,
                        radius, yaw, pitch,
                        0, length, getVerticalScale(),
                        carvingMask);
        }

        carved = true;
    }

    return carved;
}

bool CaveCarver::shouldSkipEllipsoidPosition(f64 dx, f64 dy, f64 dz, i32 y) const
{
    // 参考 MC CaveWorldCarver.func_222708_a_
    // 洞穴雕刻器的标准椭球检测
    // return y <= 0.7D || dx * dx + dy * dy + dz * dz >= 1.0D;
    (void)y; // 基类不使用y参数
    return dx * dx + dy * dy + dz * dz >= 1.0;
}

i32 CaveCarver::getCaveStartY(std::mt19937_64& rng) const
{
    // 参考 MC: return rand.nextInt(rand.nextInt(120) + 8);
    std::uniform_int_distribution<i32> dist1(0, 120);
    const i32 inner = dist1(rng);
    std::uniform_int_distribution<i32> dist2(0, inner + 8);
    return dist2(rng);
}

f32 CaveCarver::getCaveRadius(std::mt19937_64& rng) const
{
    // 参考 MC CaveWorldCarver.func_230359_a_
    // float f = rand.nextFloat() * 2.0F + rand.nextFloat();
    // if (rand.nextInt(10) == 0) { f *= rand.nextFloat() * rand.nextFloat() * 3.0F + 1.0F; }
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    f32 radius = dist(rng) * 2.0f + dist(rng);

    std::uniform_int_distribution<i32> largeDist(0, 9);
    if (largeDist(rng) == 0) {
        radius *= dist(rng) * dist(rng) * 3.0f + 1.0f;
    }

    return radius;
}

void CaveCarver::carveTunnel(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f64 startX, f64 startY, f64 startZ,
    f32 radius,
    f32 yaw, f32 pitch,
    i32 startIndex, i32 endIndex,
    f64 verticalScale,
    CarvingMask& carvingMask)
{
    // 参考 MC CaveWorldCarver.func_227206_a_
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<f32> randomDist(0.0f, 1.0f);

    // 分支点
    const i32 branchPoint = endIndex / 4 + static_cast<i32>(rng() % (endIndex / 2));
    const bool canBranch = (rng() % 6) == 0;

    f32 currentYaw = yaw;
    f32 currentPitch = pitch;
    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;
    f64 currentX = startX;
    f64 currentY = startY;
    f64 currentZ = startZ;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // 椭球半径随距离变化（正弦曲线）
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * PI);
        const f64 horizontalRadius = static_cast<f64>(radius) * static_cast<f64>(sinProgress);
        const f64 vertRadius = horizontalRadius * verticalScale;

        // 更新位置（参考MC的方向计算）
        const f32 cosPitch = std::cos(currentPitch);
        currentX += static_cast<f64>(std::cos(currentYaw) * cosPitch);
        currentY += static_cast<f64>(std::sin(currentPitch));
        currentZ += static_cast<f64>(std::sin(currentYaw) * cosPitch);

        // 更新角度
        currentPitch *= (canBranch ? 0.92f : 0.7f);
        currentPitch += pitchModifier * 0.1f;
        currentYaw += yawModifier * 0.1f;

        // 衰减和随机扰动
        pitchModifier *= 0.9f;
        yawModifier *= 0.75f;
        pitchModifier += (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 2.0f;
        yawModifier += (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 4.0f;

        // 在分支点生成分支
        if (i == branchPoint && radius > 1.0f) {
            // 生成两个分支隧道
            std::uniform_real_distribution<f32> branchRadiusDist(0.5f, 1.0f);
            const f32 branchRadius = radius * branchRadiusDist(rng);

            carveTunnel(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                        static_cast<i64>(rng()),
                        currentX, currentY, currentZ,
                        branchRadius,
                        currentYaw - PI / 2.0f, currentPitch / 3.0f,
                        i, endIndex, 1.0,
                        carvingMask);

            carveTunnel(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                        static_cast<i64>(rng()),
                        currentX, currentY, currentZ,
                        branchRadius,
                        currentYaw + PI / 2.0f, currentPitch / 3.0f,
                        i, endIndex, 1.0,
                        carvingMask);
            return;
        }

        // 随机跳过一些点（增加不规则性）
        if (rng() % 4 == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIndex, radius)) {
            carveEllipsoid(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                           currentX, currentY, currentZ,
                           horizontalRadius, vertRadius,
                           carvingMask, static_cast<i64>(rng()));
        }
    }
}

void CaveCarver::carveRoom(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f64 centerX, f64 centerY, f64 centerZ,
    f32 radius,
    f64 verticalScale,
    CarvingMask& carvingMask)
{
    // 参考 MC CaveWorldCarver.func_227205_a_
    // 生成一个椭圆形房间
    const f64 horizontalRadius = 1.5 + static_cast<f64>(std::sin(PI / 2.0f) * radius);
    const f64 vertRadius = horizontalRadius * verticalScale;

    carveEllipsoid(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                   centerX + 1.0, centerY, centerZ,
                   horizontalRadius, vertRadius,
                   carvingMask, seed);
}

} // namespace mr
