#include "ConfiguredFeature.hpp"
#include "ore/OreFeature.hpp"
#include "tree/TreeFeature.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../biome/Biome.hpp"
#include "../../block/BlockRegistry.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mr {

// ============================================================================
// FeatureRegistry 实现
// ============================================================================

FeatureRegistry& FeatureRegistry::instance() {
    static FeatureRegistry s_instance;
    return s_instance;
}

FeatureRegistry::FeatureRegistry() {
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

FeatureRegistry::~FeatureRegistry() = default;

void FeatureRegistry::initialize() {
    clear();

    // 注册矿石特征（UNDERGROUND_ORES 阶段）
    OreFeatures::initialize();
    auto oreFeatures = OreFeatures::getAllFeaturesAndClear();
    for (auto& feature : oreFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::UndergroundOres);
        }
    }

    // 注册树木特征（VEGETAL_DECORATION 阶段）
    TreeFeatures::initialize();
    auto treeFeatures = TreeFeatures::getAllFeaturesAndClear();
    for (auto& feature : treeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    spdlog::info("[FeatureRegistry] Initialized features:");
    for (size_t i = 0; i < m_featuresByStage.size(); ++i) {
        if (!m_featuresByStage[i].empty()) {
            spdlog::info("  Stage {}: {} features", i, m_featuresByStage[i].size());
        }
    }
}

void FeatureRegistry::registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, DecorationStage stage) {
    if (!feature) {
        return;
    }

    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }

    ConfiguredFeatureBase* ptr = feature.get();
    m_ownedFeatures.push_back(std::move(feature));
    m_featuresByStage[stageIndex].push_back(ptr);
}

const std::vector<ConfiguredFeatureBase*>& FeatureRegistry::getFeatures(DecorationStage stage) const {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<ConfiguredFeatureBase*> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

void FeatureRegistry::clear() {
    m_featuresByStage.clear();
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
    m_ownedFeatures.clear();
}

// ============================================================================
// FeatureGenerator 实现
// ============================================================================

void FeatureGenerator::placeFeatures(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const Biome& biome,
    DecorationStage stage,
    u64 seed)
{
    (void)biome;  // 暂时未使用，后续会根据生物群系过滤特征

    // 获取该阶段的所有特征
    const auto& features = FeatureRegistry::instance().getFeatures(stage);

    if (features.empty()) {
        return;
    }

    // 计算区块随机种子
    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();
    const u64 chunkSeed = seed
        ^ static_cast<u64>(static_cast<i64>(chunkX) * 341873128712ULL)
        ^ static_cast<u64>(static_cast<i64>(chunkZ) * 132897987541ULL);

    math::Random random(static_cast<u32>(chunkSeed));

    // 区块原点位置
    const BlockPos chunkOrigin(chunkX * 16, 0, chunkZ * 16);

    // 放置每个特征
    for (ConfiguredFeatureBase* feature : features) {
        if (feature) {
            // 设置特征随机种子
            random.setSeed(chunkSeed ^ static_cast<u64>(feature->name()[0]));

            feature->place(region, chunk, generator, random, chunkOrigin);
        }
    }
}

} // namespace mr
