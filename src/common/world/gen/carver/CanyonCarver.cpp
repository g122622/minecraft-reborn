#include "CanyonCarver.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../../math/random/Random.hpp"
#include "../../../core/Constants.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

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
    math::Random rng(0);  // 使用固定种子生成确定性阈值

    for (size_t i = 0; i < m_heightThresholds.size(); ++i) {
        if (i == 0 || rng.nextInt(3) == 0) {
            // 生成新的随机因子
            f32 factor = 1.0f + rng.nextFloat() * rng.nextFloat();
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
    math::IRandom& rng,
    ChunkCoord /*chunkX*/,
    ChunkCoord /*chunkZ*/,
    const ProbabilityConfig& config) const
{
    return rng.nextFloat() <= config.probability;
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
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                     static_cast<u64>(chunkZ) * 132897987541ULL +
                     static_cast<u64>(m_maxHeight) + 1);

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    const i32 tunnelLength = (getRange() * 2 - 1) * 16;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 峡谷起点
    const f32 canyonX = static_cast<f32>(startX) + rng.nextFloat(0.0f, 16.0f);
    const f32 canyonY = static_cast<f32>(rng.nextInt(20, 67));
    const f32 canyonZ = static_cast<f32>(startZ) + rng.nextFloat(0.0f, 16.0f);

    // 峡谷方向和尺寸
    const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
    const f32 pitch = rng.nextFloat(-0.125f, 0.125f);  // MC使用 2.0F / 8.0F
    const f32 radius = (rng.nextFloat(1.0f, 2.0f) * 2.0f + rng.nextFloat());  // MC: nextFloat() * 2.0F + nextFloat()) * 2.0F

    // 峡谷长度
    const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

    // 生成蜿蜒峡谷
    generateCanyon(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                   static_cast<i64>(rng.nextU64()),
                   canyonX, canyonY, canyonZ,
                   radius, yaw, pitch,
                   0, length, 3.0f,
                   carvingMask);

    return true;
}

bool CanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 参考 MC CanyonWorldCarver.func_222708_a_
    // 峡谷使用特殊的厚度检测：考虑Y坐标的半径变化
    if (y < 1 || y >= static_cast<i32>(m_heightThresholds.size())) {
        return dx * dx + dz * dz >= 1.0f;
    }

    // 使用预计算的阈值
    const f32 threshold = m_heightThresholds[static_cast<size_t>(y - 1)];
    return dx * dx + dz * dz >= threshold || dy * dy / 6.0f >= 1.0f;
}

void CanyonCarver::generateCanyon(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    i64 seed,
    f32 startX, f32 startY, f32 startZ,
    f32 radius,
    f32 yaw, f32 pitch,
    i32 startIndex, i32 endIndex,
    f32 horizontalScale,
    CarvingMask& carvingMask)
{
    // 参考 MC CanyonWorldCarver.func_227204_a_
    math::Random rng(static_cast<u64>(seed));

    f32 currentX = startX;
    f32 currentY = startY;
    f32 currentZ = startZ;
    f32 yawModifier = 0.0f;
    f32 pitchModifier = 0.0f;

    for (i32 i = startIndex; i < endIndex; ++i) {
        // 计算当前半径（正弦曲线变化）
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(endIndex);
        const f32 sinProgress = std::sin(progress * math::PI);
        f32 horizontalRadius = radius * sinProgress;
        f32 verticalRadius = horizontalRadius * 0.5f;  // 垂直半径为水平的一半

        // 应用水平缩放
        horizontalRadius *= horizontalScale;

        // 添加随机变化（参考MC）
        horizontalRadius *= rng.nextFloat() * 0.25f + 0.75f;
        verticalRadius *= rng.nextFloat() * 0.25f + 0.75f;

        // 更新位置（参考MC的方向计算）
        const f32 cosPitch = std::cos(pitch);
        currentX += std::cos(yaw) * cosPitch;
        currentY += std::sin(pitch);
        currentZ += std::sin(yaw) * cosPitch;

        // 更新俯仰角（衰减）
        pitch *= 0.7f;
        pitch += pitchModifier * 0.05f;

        // 更新偏航角（蜿蜒效果）
        yaw += yawModifier * 0.05f;

        // 衰减和随机扰动
        pitchModifier *= 0.8f;
        yawModifier *= 0.5f;
        pitchModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 2.0f;
        yawModifier += (rng.nextFloat() - rng.nextFloat()) * rng.nextFloat() * 4.0f;

        // 随机跳过一些点（每4步跳过3次，增加不规则性）
        if (rng.nextInt(4) == 0) {
            continue;
        }

        // 检查是否在雕刻范围内
        if (isInCarvingRange(chunkX, chunkZ, currentX, currentZ, i, endIndex, radius)) {
            carveEllipsoid(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                           currentX, currentY, currentZ,
                           horizontalRadius,
                           verticalRadius,
                           carvingMask, static_cast<i64>(rng.nextU64()));
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

} // namespace mc
