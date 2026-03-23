#include "FoliagePlacers.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// PineFoliagePlacer 实现
// ============================================================================

PineFoliagePlacer::PineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 PineFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const {
    return m_height;
}

void PineFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 从下到上逐层放置树叶，半径逐渐减小
    for (i32 y = 0; y <= foliageHeight; ++y) {
        i32 layerRadius = getRadiusAtHeight(y, foliageHeight);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool PineFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 /*dy*/, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 跳过角落
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 随机跳过边缘
    if (dist == radius && random.nextFloat() < 0.2f) {
        return true;
    }

    return false;
}

i32 PineFoliagePlacer::getRadiusAtHeight(i32 height, i32 foliageHeight) const {
    // 锥形：底部大，顶部小
    if (foliageHeight <= 0) {
        return 1;
    }
    return std::max(0, m_height / 2 - height / 2);
}

std::unique_ptr<FoliagePlacer> PineFoliagePlacer::clone() const {
    return std::make_unique<PineFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// SpruceFoliagePlacer 实现
// ============================================================================

SpruceFoliagePlacer::SpruceFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 SpruceFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const {
    return m_height;
}

void SpruceFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 云杉：更尖锐的锥形
    for (i32 y = 0; y <= foliageHeight; ++y) {
        i32 layerRadius = std::max(1, radius - y / 2);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool SpruceFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 跳过远处角落
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius + 1) {
        return true;
    }

    // 随机跳过边缘
    if (dist >= radius && random.nextFloat() < 0.3f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> SpruceFoliagePlacer::clone() const {
    return std::make_unique<SpruceFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// AcaciaFoliagePlacer 实现
// ============================================================================

AcaciaFoliagePlacer::AcaciaFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{
}

i32 AcaciaFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const {
    return 1 + random.nextInt(2);
}

void AcaciaFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 金合欢：伞形，单层大半径
    for (i32 y = 0; y < foliageHeight; ++y) {
        i32 layerRadius = radius + (y == 0 ? 1 : 0);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool AcaciaFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 伞形：跳过远处
    i32 distSq = dx * dx + dz * dz;
    if (distSq > radius * radius) {
        return true;
    }

    // 随机跳过一些边缘
    if (distSq > (radius - 1) * (radius - 1) && random.nextFloat() < 0.2f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> AcaciaFoliagePlacer::clone() const {
    return std::make_unique<AcaciaFoliagePlacer>(m_radius, m_offset);
}

// ============================================================================
// DarkOakFoliagePlacer 实现
// ============================================================================

DarkOakFoliagePlacer::DarkOakFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 DarkOakFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const {
    return m_height + random.nextInt(2);
}

void DarkOakFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 深色橡树：更密集的球形
    for (i32 y = 0; y <= foliageHeight; ++y) {
        i32 layerRadius = radius - (y > foliageHeight / 2 ? 1 : 0);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool DarkOakFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 球形检查
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy + dz * dz));
    if (dist > radius + 1) {
        return true;
    }

    // 随机跳过边缘
    if (dist > radius && random.nextFloat() < 0.5f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> DarkOakFoliagePlacer::clone() const {
    return std::make_unique<DarkOakFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// JungleFoliagePlacer 实现
// ============================================================================

JungleFoliagePlacer::JungleFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 JungleFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const {
    return m_height + random.nextInt(2);
}

void JungleFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 丛林木：较稀疏的单层树叶
    for (i32 y = 0; y < foliageHeight; ++y) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                if (shouldSkip(random, dx, y, dz, radius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool JungleFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 丛林木树叶较稀疏
    if (dist == radius && random.nextFloat() < 0.4f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> JungleFoliagePlacer::clone() const {
    return std::make_unique<JungleFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// MegaPineFoliagePlacer 实现
// ============================================================================

MegaPineFoliagePlacer::MegaPineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 MegaPineFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const {
    return m_height + random.nextInt(4);
}

void MegaPineFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 巨型松树：更大的锥形
    for (i32 y = 0; y <= foliageHeight; ++y) {
        // 从底部到顶部半径逐渐减小
        i32 layerRadius = std::max(1, radius - y / 3);

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool MegaPineFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    i32 dist = std::abs(dx) + std::abs(dz);
    if (dist > radius) {
        return true;
    }

    // 边缘稀疏
    if (dist >= radius - 1 && random.nextFloat() < 0.3f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> MegaPineFoliagePlacer::clone() const {
    return std::make_unique<MegaPineFoliagePlacer>(m_radius, m_offset, m_height);
}

// ============================================================================
// BushFoliagePlacer 实现
// ============================================================================

BushFoliagePlacer::BushFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : FoliagePlacer(radius, offset)
{
}

i32 BushFoliagePlacer::getFoliageHeight(math::Random& /*random*/, i32 /*trunkHeight*/) const {
    return 1;  // 灌木只有一层
}

void BushFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 灌木：单层球形
    for (i32 y = 0; y < foliageHeight; ++y) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                if (shouldSkip(random, dx, y, dz, radius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool BushFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 球形
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
    if (dist > radius + 0.5f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> BushFoliagePlacer::clone() const {
    return std::make_unique<BushFoliagePlacer>(m_radius, m_offset);
}

// ============================================================================
// FancyFoliagePlacer 实现
// ============================================================================

FancyFoliagePlacer::FancyFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height)
    : FoliagePlacer(radius, offset)
    , m_height(height)
{
}

i32 FancyFoliagePlacer::getFoliageHeight(math::Random& random, i32 /*trunkHeight*/) const {
    return m_height + random.nextInt(3);
}

void FancyFoliagePlacer::placeFoliageInternal(
    WorldGenRegion& world,
    math::Random& random,
    i32 /*trunkHeight*/,
    const FoliagePosition& foliagePos,
    i32 foliageHeight,
    i32 radius,
    i32 /*offset*/,
    std::set<BlockPos>& foliageBlocks,
    const BlockState* foliageBlock)
{
    // 精美树叶：更大更密集的球形
    for (i32 y = -1; y <= foliageHeight; ++y) {
        // 中间最大，上下略小
        i32 layerRadius = radius;
        if (y < 0 || y >= foliageHeight - 1) {
            layerRadius = std::max(1, radius - 1);
        }

        for (i32 dx = -layerRadius; dx <= layerRadius; ++dx) {
            for (i32 dz = -layerRadius; dz <= layerRadius; ++dz) {
                if (shouldSkip(random, dx, y, dz, layerRadius, foliagePos.trunkTop)) {
                    continue;
                }

                BlockPos pos(foliagePos.pos.x + dx, foliagePos.pos.y + y, foliagePos.pos.z + dz);
                foliageBlocks.insert(pos);
            }
        }
    }
}

bool FancyFoliagePlacer::shouldSkip(
    math::Random& random,
    i32 dx, i32 dy, i32 dz,
    i32 radius,
    bool /*trunkTop*/) const
{
    // 更密集的球形
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy * 0.25f + dz * dz));
    if (dist > radius + 1) {
        return true;
    }

    // 边缘随机跳过
    if (dist > radius - 0.5f && random.nextFloat() < 0.2f) {
        return true;
    }

    return false;
}

std::unique_ptr<FoliagePlacer> FancyFoliagePlacer::clone() const {
    return std::make_unique<FancyFoliagePlacer>(m_radius, m_offset, m_height);
}

} // namespace mc
