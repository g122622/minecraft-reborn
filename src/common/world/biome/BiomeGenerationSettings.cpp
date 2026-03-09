#include "BiomeGenerationSettings.hpp"
#include "../gen/feature/ConfiguredFeature.hpp"
#include "../gen/chunk/IChunkGenerator.hpp"
#include "../chunk/ChunkPrimer.hpp"
#include "../../math/random/Random.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mr {

// ============================================================================
// BiomeGenerationSettings 实现
// ============================================================================

BiomeGenerationSettings::BiomeGenerationSettings() {
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

BiomeGenerationSettings::~BiomeGenerationSettings() = default;

void BiomeGenerationSettings::addFeature(DecorationStage stage, u32 featureId) {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }
    m_featuresByStage[stageIndex].push_back(featureId);
}

const std::vector<u32>& BiomeGenerationSettings::getFeatures(DecorationStage stage) const {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<u32> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

bool BiomeGenerationSettings::hasFeatures() const {
    for (const auto& features : m_featuresByStage) {
        if (!features.empty()) {
            return true;
        }
    }
    return false;
}

void BiomeGenerationSettings::clear() {
    for (auto& features : m_featuresByStage) {
        features.clear();
    }
}

BiomeGenerationSettings BiomeGenerationSettings::createDefault() {
    // 默认设置：包含所有主世界矿石
    BiomeGenerationSettings settings;

    // 添加矿石（UNDERGROUND_ORES 阶段）
    // 矿石特征ID通过 FeatureRegistry 注册的顺序确定
    // 0: coal_ore, 1: iron_ore, 2: gold_ore, 3: redstone_ore, 4: diamond_ore
    // 5: lapis_ore, 6: emerald_ore, 7: copper_ore
    settings.addFeature(DecorationStage::UndergroundOres, 0);  // 煤矿
    settings.addFeature(DecorationStage::UndergroundOres, 1);  // 铁矿
    settings.addFeature(DecorationStage::UndergroundOres, 2);  // 金矿
    settings.addFeature(DecorationStage::UndergroundOres, 3);  // 红石
    settings.addFeature(DecorationStage::UndergroundOres, 4);  // 钻石
    settings.addFeature(DecorationStage::UndergroundOres, 5);  // 青金石
    settings.addFeature(DecorationStage::UndergroundOres, 7);  // 铜矿

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createPlains() {
    // 平原：基础矿石 + 稀疏的树木
    BiomeGenerationSettings settings = createDefault();

    // 添加稀疏橡树（VEGETAL_DECORATION 阶段）
    // 树木特征ID在 VegetalDecoration 阶段内重新从0开始编号
    // 0: oak_tree, 1: birch_tree, 2: spruce_tree, 3: jungle_tree, 4: sparse_oak_tree
    settings.addFeature(DecorationStage::VegetalDecoration, 4);  // 稀疏橡树

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createForest() {
    // 森林：基础矿石 + 密集的树木
    BiomeGenerationSettings settings = createDefault();

    // 添加树木（VEGETAL_DECORATION 阶段）
    // 树木特征ID在 VegetalDecoration 阶段内重新从0开始编号
    // 0: oak_tree, 1: birch_tree, 2: spruce_tree, 3: jungle_tree, 4: sparse_oak_tree
    settings.addFeature(DecorationStage::VegetalDecoration, 0);   // 橡树
    settings.addFeature(DecorationStage::VegetalDecoration, 0);   // 橡树（多添加几次增加密度）
    settings.addFeature(DecorationStage::VegetalDecoration, 1);   // 白桦

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createTaiga() {
    BiomeGenerationSettings settings = createDefault();
    settings.addFeature(DecorationStage::VegetalDecoration, 2);  // 云杉
    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createJungle() {
    BiomeGenerationSettings settings = createDefault();
    settings.addFeature(DecorationStage::VegetalDecoration, 3);  // 丛林树
    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSavanna() {
    BiomeGenerationSettings settings = createDefault();
    settings.addFeature(DecorationStage::VegetalDecoration, 4);  // 稀疏橡树代替未实现的相思树
    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDesert() {
    // 沙漠：只有矿石，没有树木
    BiomeGenerationSettings settings = createDefault();

    // 沙漠没有树木
    // TODO: 添加枯萎灌木特征

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createMountains() {
    // 山地：基础矿石 + 绿宝石 + 云杉树
    BiomeGenerationSettings settings = createDefault();

    // 添加绿宝石矿石（在 UndergroundOres 阶段）
    // 矿石特征ID：0:coal, 1:iron, 2:gold, 3:redstone, 4:diamond, 5:lapis, 6:emerald, 7:copper
    settings.addFeature(DecorationStage::UndergroundOres, 6);  // 绿宝石

    // 添加云杉树（VEGETAL_DECORATION 阶段）
    // 0: oak_tree, 1: birch_tree, 2: spruce_tree, 3: jungle_tree, 4: sparse_oak_tree
    settings.addFeature(DecorationStage::VegetalDecoration, 2);  // 云杉

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createOcean() {
    // 海洋：只有矿石，没有植被
    BiomeGenerationSettings settings;

    // 添加矿石
    settings.addFeature(DecorationStage::UndergroundOres, 0);  // 煤矿
    settings.addFeature(DecorationStage::UndergroundOres, 1);  // 铁矿
    settings.addFeature(DecorationStage::UndergroundOres, 2);  // 金矿
    settings.addFeature(DecorationStage::UndergroundOres, 3);  // 红石
    settings.addFeature(DecorationStage::UndergroundOres, 4);  // 钻石
    settings.addFeature(DecorationStage::UndergroundOres, 5);  // 青金石
    settings.addFeature(DecorationStage::UndergroundOres, 7);  // 铜矿

    return settings;
}

// ============================================================================
// BiomeFeaturePlacer 实现
// ============================================================================

void BiomeFeaturePlacer::placeAllFeatures(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const BiomeGenerationSettings& settings,
    u64 seed)
{
    // 按顺序放置每个阶段的特征
    for (DecorationStage stage : DecorationStages::getAll()) {
        placeFeaturesForStage(region, chunk, generator, settings, stage, seed);
    }
}

void BiomeFeaturePlacer::placeFeaturesForStage(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const BiomeGenerationSettings& settings,
    DecorationStage stage,
    u64 seed)
{
    const auto& featureIds = settings.getFeatures(stage);
    if (featureIds.empty()) {
        return;
    }

    // 计算区块随机种子
    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();
    const u64 chunkSeed = seed
        ^ static_cast<u64>(static_cast<i64>(chunkX) * 341873128712ULL)
        ^ static_cast<u64>(static_cast<i64>(chunkZ) * 132897987541ULL);

    math::Random random(static_cast<u32>(chunkSeed ^ static_cast<u64>(stage)));

    // 区块原点位置
    const BlockPos chunkOrigin(chunkX * 16, 0, chunkZ * 16);

    // 获取特征注册表中的特征
    FeatureRegistry& registry = FeatureRegistry::instance();
    const auto& allFeatures = registry.getFeatures(stage);

    // 放置每个特征
    for (u32 featureId : featureIds) {
        if (featureId < allFeatures.size() && allFeatures[featureId]) {
            ConfiguredFeatureBase* feature = allFeatures[featureId];

            // 设置特征随机种子
            random.setSeed(chunkSeed ^ static_cast<u64>(featureId));

            bool placed = feature->place(region, chunk, generator, random, chunkOrigin);
            (void)placed;  // 暂时忽略结果
        }
    }
}

} // namespace mr
