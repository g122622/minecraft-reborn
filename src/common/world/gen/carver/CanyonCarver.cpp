#include "CanyonCarver.hpp"
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

CanyonCarver::CanyonCarver(i32 maxHeight)
    : WorldCarver<ProbabilityConfig>(maxHeight)
    , m_heightThresholds(256, 1.0f)
{
    initializeHeightThresholds();
}

void CanyonCarver::initializeHeightThresholds()
{
    // 参考 MC CanyonWorldCarver 构造函数
    // 为每个高度预计算半径变化因子
    std::mt19937_64 rng(0);  // 使用固定种子生成确定性阈值

    for (size_t i = 0; i < m_heightThresholds.size(); ++i) {
        if (i == 0 || rng() % 3 == 0) {
            // 生成新的随机因子
            std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
            f32 factor = 1.0f + dist(rng) * dist(rng);
            m_heightThresholds[i] = factor * factor;
        } else {
            // 使用上一个值
            if (i > 0) {
                m_heightThresholds[i] = m_heightThresholds[i - 1];
            }
        }
    }
}

bool CanyonCarver::shouldCarve(
    std::mt19937_64& rng,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    const ProbabilityConfig& config) const
{
    // 峡谷比洞穴更稀少
    std::mt19937_64 localRng(static_cast<u64>(chunkX) * 341873128712ULL +
                             static_cast<u64>(chunkZ) * 132897987541ULL +
                             static_cast<u64>(m_maxHeight) + 1);
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);
    return dist(localRng) <= config.probability;
}

bool CanyonCarver::carve(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    // 参考 MC CanyonWorldCarver.carveRegion
    std::mt19937_64 rng(static_cast<u64>(chunkX) * 341873128712ULL +
                        static_cast<u64>(chunkZ) * 132897987541ULL +
                        static_cast<u64>(m_maxHeight) + 1);

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    const i32 tunnelLength = (getRange() * 2 - 1) * 16;
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
    std::uniform_real_distribution<f32> pitchDist(-0.125f, 0.125f);  // MC使用 2.0F / 8.0F
    std::uniform_real_distribution<f32> radiusDist(1.0f, 2.0f);

    const f32 yaw = angleDist(rng);
    const f32 pitch = pitchDist(rng);
    const f32 radius = (radiusDist(rng) * 2.0f + radiusDist(rng));  // MC: nextFloat() * 2.0F + nextFloat()) * 2.0F

    // 峡谷长度
    std::uniform_int_distribution<i32> lengthDist(0, tunnelLength / 4);
    const i32 length = tunnelLength - lengthDist(rng);

    // 生成蜿蜒峡谷
    generateCanyon(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                   static_cast<i64>(rng()),
                   canyonX, canyonY, canyonZ,
                   radius, yaw, pitch,
                   0, length, 3.0,
                   carvingMask);

    return true;
}

bool CanyonCarver::shouldSkipEllipsoidPosition(f64 dx, f64 dy, f64 dz, i32 y) const
{
    // 参考 MC CanyonWorldCarver.func_222708_a_
    // 峡谷使用特殊的厚度检测：考虑Y坐标的半径变化
    if (y < 1 || y >= static_cast<i32>(m_heightThresholds.size())) {
        return dx * dx + dz * dz >= 1.0;
    }

    // 使用预计算的阈值
    const f32 threshold = m_heightThresholds[static_cast<size_t>(y - 1)];
    return dx * dx + dz * dz >= static_cast<f64>(threshold) || dy * dy / 6.0 >= 1.0;
}

void CanyonCarver::generateCanyon(
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
    f64 horizontalScale,
    CarvingMask& carvingMask)
{
    // 参考 MC CanyonWorldCarver.func_227204_a_
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<f32> randomDist(0.0f, 1.0f);

    f64 currentX = startX;
    f64 currentY = startY;
    f64 currentZ = startZ;
    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // 计算当前半径（正弦曲线变化）
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * PI);
        f32 horizontalRadius = radius * sinProgress;
        f32 verticalRadius = horizontalRadius * 0.5f;  // 垂直半径为水平的一半

        // 应用水平缩放
        horizontalRadius = static_cast<f32>(static_cast<f64>(horizontalRadius) * horizontalScale);

        // 添加随机变化（参考MC）
        horizontalRadius *= randomDist(rng) * 0.25f + 0.75f;
        verticalRadius *= randomDist(rng) * 0.25f + 0.75f;

        // 更新位置（参考MC的方向计算）
        const f32 cosPitch = std::cos(pitch);
        currentX += static_cast<f64>(std::cos(yaw) * cosPitch);
        currentY += static_cast<f64>(std::sin(pitch));
        currentZ += static_cast<f64>(std::sin(yaw) * cosPitch);

        // 更新俯仰角（衰减）
        pitch *= 0.7f;
        pitch += pitchModifier * 0.05f;

        // 更新偏航角（蜿蜒效果）
        yaw += yawModifier * 0.05f;

        // 衰减和随机扰动
        pitchModifier *= 0.8f;
        yawModifier *= 0.5f;
        pitchModifier += (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 2.0f;
        yawModifier += (randomDist(rng) - randomDist(rng)) * randomDist(rng) * 4.0f;

        // 随机跳过一些点（每4步跳过3次，增加不规则性）
        if (rng() % 4 == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIndex, radius)) {
            carveEllipsoid(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                           currentX, currentY, currentZ,
                           static_cast<f64>(horizontalRadius),
                           static_cast<f64>(verticalRadius),
                           carvingMask, static_cast<i64>(rng()));
        }
    }
}

f32 CanyonCarver::updateRadius(
    f32 baseRadius,
    f32 progress,
    const std::vector<f32>& thresholds,
    i32 index) const
{
    // 峡谷入口较宽，深处较窄
    const f32 factor = 1.0f - progress * 0.3f;

    // 如果索引在阈值表范围内，使用阈值
    if (index >= 0 && static_cast<size_t>(index) < thresholds.size()) {
        return baseRadius * factor * thresholds[static_cast<size_t>(index)];
    }

    return baseRadius * factor;
}

} // namespace mr
