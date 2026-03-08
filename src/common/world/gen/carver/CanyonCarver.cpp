#include "CanyonCarver.hpp"
#include "CaveCarver.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../block/BlockRegistry.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// 常量
// ============================================================================

constexpr f32 PI = 3.14159265358979323846f;

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
    // 参考 MC CanyonWorldCarver
    std::mt19937_64 rng(static_cast<u64>(chunkX) * 341873128712ULL +
                        static_cast<u64>(chunkZ) * 132897987541ULL +
                        m_seed);

    if (!shouldCarve(rng, chunkX, chunkZ)) {
        return false;
    }

    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 峡谷起点
    std::uniform_real_distribution<f64> xPosDist(0.0, 16.0);
    std::uniform_real_distribution<f64> zPosDist(0.0, 16.0);
    std::uniform_int_distribution<i32> yDist(20, 67);

    const f64 canyonX = static_cast<f64>(startX) + xPosDist(rng);
    const f64 canyonY = static_cast<f64>(yDist(rng));
    const f64 canyonZ = static_cast<f64>(startZ) + zPosDist(rng);

    // 峡谷方向和尺寸
    std::uniform_real_distribution<f32> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<f32> pitchDist(-0.25f, 0.25f);
    std::uniform_real_distribution<f32> radiusDist(1.0f, 2.0f);
    std::uniform_int_distribution<i32> lengthDist(80, 200);

    const f32 yaw = angleDist(rng);
    const f32 pitch = pitchDist(rng);
    const f32 radius = radiusDist(rng);
    const i32 length = lengthDist(rng);

    // 生成蜿蜒峡谷
    generateCanyon(chunk, rng, static_cast<i64>(m_seed),
                   canyonX, canyonY, canyonZ,
                   yaw, pitch, radius, length);

    return true;
}

void CanyonCarver::generateCanyon(ChunkPrimer& chunk,
                                   std::mt19937_64& rng,
                                   i64 seed,
                                   f64 startX, f64 startY, f64 startZ,
                                   f32 yaw, f32 pitch, f32 radius,
                                   i32 length)
{
    // 参考 MC CanyonWorldCarver.func_227205_a_
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);

    f64 currentX = startX;
    f64 currentY = startY;
    f64 currentZ = startZ;

    // 峡谷宽度变化（参考 MC）
    std::uniform_real_distribution<f32> widthDist(1.0f, 4.0f);
    f32 canyonWidth = widthDist(rng);

    for (i32 i = 0; i < length; ++i) {
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(length);

        // 计算当前半径（峡谷入口宽，深处窄）
        const f32 currentRadius = updateRadius(radius, progress);

        // 计算宽度（随深度变化）
        const f32 width = canyonWidth * (1.0f - progress * 0.3f);

        // 更新位置
        currentX += std::cos(yaw) * std::cos(pitch);
        currentY += std::sin(pitch);
        currentZ += std::sin(yaw) * std::cos(pitch);

        // 更新角度（蜿蜒曲线）
        f32 newYaw = yaw;
        f32 newPitch = pitch;
        updateYawAndPitch(newYaw, newPitch, rng, progress);
        yaw = newYaw;
        pitch = newPitch;

        // 每隔几步雕刻一次
        if (i % 4 == 0) {
            // 雕刻当前位置
            carveEllipsoid(chunk,
                           static_cast<ChunkCoord>(currentX) >> 4,
                           static_cast<ChunkCoord>(currentZ) >> 4,
                           currentX, currentY, currentZ,
                           static_cast<f64>(currentRadius) * static_cast<f64>(width),
                           static_cast<f64>(currentRadius) * 0.5);
        }
    }
}

f32 CanyonCarver::updateRadius(f32 baseRadius, f32 progress) const
{
    // 参考 MC：峡谷入口较宽，深处较窄
    // 使用余弦函数实现平滑过渡
    const f32 factor = 1.0f - progress;
    return baseRadius * (0.5f + factor * 0.5f);
}

void CanyonCarver::updateYawAndPitch(f32& yaw, f32& pitch,
                                      std::mt19937_64& rng,
                                      f32 progress) const
{
    // 参考 MC CanyonWorldCarver
    std::uniform_real_distribution<f32> dist(-1.0f, 1.0f);

    // 水平蜿蜒 - 使用较小的角度变化
    const f32 yawChange = dist(rng) * 0.05f;
    yaw += yawChange;

    // 限制俯仰角变化
    const f32 pitchChange = dist(rng) * 0.02f;
    pitch = std::clamp(pitch + pitchChange, -0.5f, 0.5f);
}

bool CanyonCarver::carveEllipsoid(ChunkPrimer& chunk,
                                   ChunkCoord chunkX, ChunkCoord chunkZ,
                                   f64 centerX, f64 centerY, f64 centerZ,
                                   f64 horizontalRadius, f64 verticalRadius)
{
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

                // 检查是否可以雕刻（使用 CaveCarver 的方法）
                if (!CaveCarver::isCarvable(blockId)) {
                    continue;
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

} // namespace mr
