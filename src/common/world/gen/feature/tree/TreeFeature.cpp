#include "TreeFeature.hpp"
#include "trunk/StraightTrunkPlacer.hpp"
#include "foliage/BlobFoliagePlacer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../core/Types.hpp"
#include "../../placement/Placement.hpp"
#include <mutex>
#include <spdlog/spdlog.h>

namespace mc {

namespace {

std::mutex g_treeFeaturesMutex;

std::unique_ptr<ConfiguredPlacement> appendBiomePlacement(
    std::unique_ptr<ConfiguredPlacement> root,
    std::vector<u32> allowedBiomes)
{
    if (!root || allowedBiomes.empty()) {
        return root;
    }

    auto biomeConfigured = std::make_unique<ConfiguredPlacement>(
        std::make_unique<BiomePlacement>(),
        std::make_unique<BiomePlacementConfig>(std::move(allowedBiomes)));

    ConfiguredPlacement* current = root.get();
    while (current->next() != nullptr) {
        current = current->next();
    }
    current->setNext(std::move(biomeConfigured));
    return root;
}

std::unique_ptr<ConfiguredPlacement> createCountedSurfacePlacement(i32 count, i32 maxWaterDepth = 0) {
    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(maxWaterDepth, false);

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(count);

    auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(surfacePlacement), std::move(surfaceConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(surfaceConfigured));
    countConfigured->setNext(std::move(squareConfigured));
    return countConfigured;
}

std::unique_ptr<ConfiguredPlacement> createChanceSurfacePlacement(f32 chance, i32 maxWaterDepth = 0) {
    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(maxWaterDepth, false);

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto chancePlacement = std::make_unique<ChancePlacement>();
    auto chanceConfig = std::make_unique<ChancePlacementConfig>(chance);

    auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(surfacePlacement), std::move(surfaceConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(squarePlacement), std::move(squareConfig));
    auto chanceConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(chancePlacement), std::move(chanceConfig));

    squareConfigured->setNext(std::move(surfaceConfigured));
    chanceConfigured->setNext(std::move(squareConfigured));
    return chanceConfigured;
}

} // namespace

// ============================================================================
// TreeFeature 实现（保持原有实现）
// ============================================================================

bool TreeFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& startPos,
    const TreeFeatureConfig& config
) {
    if (config.trunkPlacer == nullptr || config.foliagePlacer == nullptr) {
        return false;
    }

    // 获取树干高度
    i32 trunkHeight = config.trunkPlacer->getHeight(random);

    // 检查高度是否有效
    if (trunkHeight < config.minHeight) {
        return false;
    }

    // 检查起始位置是否在有效范围内
    if (startPos.y < 1 || startPos.y + trunkHeight + 1 >= 256) {
        return false;
    }

    // 检查起始位置下方是否是泥土或草地
    if (!isDirtOrFarmlandAt(world, startPos.down())) {
        return false;
    }

    // 检查是否有足够的空间放置树干
    i32 availableHeight = calculateAvailableHeight(world, trunkHeight, startPos, config);
    if (availableHeight < trunkHeight) {
        return false;
    }

    // 放置树干
    std::set<BlockPos> trunkBlocks;
    std::vector<FoliagePosition> foliagePositions = config.trunkPlacer->placeTrunk(
        world, random, trunkHeight, startPos, trunkBlocks, config.trunkBlock
    );

    if (foliagePositions.empty()) {
        return false;
    }

    // 放置树叶
    config.foliagePlacer->placeFoliage(
        world, random, trunkHeight, foliagePositions, trunkBlocks,
        trunkHeight - 1, config.foliageBlock
    );

    return true;
}

bool TreeFeature::isReplaceableAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) ||
        state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) ||
        state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) ||
        state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
        return true;
    }

    // 检查是否是植被
    if (state->is(VanillaBlocks::SHORT_GRASS) ||
        state->is(VanillaBlocks::TALL_GRASS) ||
        state->is(VanillaBlocks::FERN) ||
        state->is(VanillaBlocks::DANDELION) ||
        state->is(VanillaBlocks::POPPY) ||
        state->is(VanillaBlocks::OAK_SAPLING) ||
        state->is(VanillaBlocks::SPRUCE_SAPLING) ||
        state->is(VanillaBlocks::BIRCH_SAPLING) ||
        state->is(VanillaBlocks::JUNGLE_SAPLING) ||
        state->is(VanillaBlocks::ACACIA_SAPLING) ||
        state->is(VanillaBlocks::DARK_OAK_SAPLING)) {
        return true;
    }

    // 检查是否是水
    if (state->is(VanillaBlocks::WATER)) {
        return true;
    }

    return false;
}

bool TreeFeature::isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) ||
        state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) ||
        state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) ||
        state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
        return true;
    }

    return false;
}

bool TreeFeature::isDirtOrFarmlandAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是泥土类方块
    return state->is(VanillaBlocks::DIRT) ||
           state->is(VanillaBlocks::GRASS_BLOCK) ||
           state->is(VanillaBlocks::COARSE_DIRT) ||
           state->is(VanillaBlocks::PODZOL);
}

bool TreeFeature::isWaterAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::WATER);
}

i32 TreeFeature::calculateAvailableHeight(
    WorldGenRegion& world,
    i32 maxHeight,
    const BlockPos& startPos,
    const TreeFeatureConfig& config
) const {
    (void)config;
    BlockPos pos;

    for (i32 y = 0; y <= maxHeight + 1; ++y) {
        // 计算检查半径（随高度变化）
        i32 checkRadius = 0;  // 简化：只检查中心

        for (i32 dx = -checkRadius; dx <= checkRadius; ++dx) {
            for (i32 dz = -checkRadius; dz <= checkRadius; ++dz) {
                pos.x = startPos.x + dx;
                pos.y = startPos.y + y;
                pos.z = startPos.z + dz;

                if (!isReplaceableAt(world, pos)) {
                    // 找到不可替换的方块，返回当前可用高度
                    return y - 2;
                }
            }
        }
    }

    return maxHeight;
}

void TreeFeature::setFoliageDistance(
    WorldGenRegion& world,
    const std::set<BlockPos>& trunkBlocks,
    const std::set<BlockPos>& foliageBlocks
) {
    // TODO: 实现树叶距离设置
    // 这用于树叶的腐烂机制
    // 当树叶距离树干太远时会自动腐烂
    (void)world;
    (void)trunkBlocks;
    (void)foliageBlocks;
}

// ============================================================================
// ConfiguredTreeFeature 实现
// ============================================================================

ConfiguredTreeFeature::ConfiguredTreeFeature(
    std::unique_ptr<TreeFeatureConfig> featureConfig,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(featureConfig))
    , m_placement(std::move(placement))
    , m_name(featureName)
{
}

bool ConfiguredTreeFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    (void)generator;
    (void)chunk;

    if (!m_config || !m_placement) {
        return false;
    }

    // 获取放置位置
    std::vector<BlockPos> positions = m_placement->getPositions(region, random, pos);

    if (positions.empty()) {
        return false;
    }

    bool placedAny = false;

    for (const BlockPos& placePos : positions) {
        // 跳过无效位置
        if (placePos.y < 1 || placePos.y >= 255) {
            continue;
        }

        // 尝试放置树木
        if (m_feature.place(region, random, placePos, *m_config)) {
            placedAny = true;
        }
    }

    return placedAny;
}

// ============================================================================
// TreeFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredTreeFeature>> TreeFeatures::s_features;

void TreeFeatures::initialize() {
    std::lock_guard<std::mutex> lock(g_treeFeaturesMutex);
    s_features.clear();

    // 注册主世界树木
    s_features.push_back(createOakTree());
    s_features.push_back(createBirchTree());
    s_features.push_back(createSpruceTree());
    s_features.push_back(createJungleTree());
    s_features.push_back(createSparseOakTree());

    spdlog::info("[TreeFeatures] Initialized {} tree features", s_features.size());
}

std::vector<std::unique_ptr<ConfiguredTreeFeature>> TreeFeatures::getAllFeaturesAndClear() {
    std::lock_guard<std::mutex> lock(g_treeFeaturesMutex);
    std::vector<std::unique_ptr<ConfiguredTreeFeature>> result = std::move(s_features);
    s_features.clear();
    return result;
}

const std::vector<std::unique_ptr<ConfiguredTreeFeature>>& TreeFeatures::getAllFeatures() {
    return s_features;
}

std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createOakTree() {
    // 橡树配置
    auto config = std::make_unique<TreeFeatureConfig>(oakConfig());

    auto placement = appendBiomePlacement(
        createCountedSurfacePlacement(4),
        {Biomes::Forest, Biomes::WoodedHills, Biomes::DarkForest});

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(placement), "oak_tree");
}

std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createBirchTree() {
    // 白桦配置
    auto config = std::make_unique<TreeFeatureConfig>(birchConfig());

    auto placement = appendBiomePlacement(
        createCountedSurfacePlacement(3),
        {Biomes::BirchForest, Biomes::Forest});

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(placement), "birch_tree");
}

std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createSpruceTree() {
    // 云杉配置
    auto config = std::make_unique<TreeFeatureConfig>(spruceConfig());

    auto placement = appendBiomePlacement(
        createCountedSurfacePlacement(3),
        {Biomes::Taiga, Biomes::SnowyTaiga, Biomes::GiantTreeTaiga,
         Biomes::Mountains, Biomes::WoodedMountains, Biomes::MountainEdge, Biomes::StoneShore});

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(placement), "spruce_tree");
}

std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createJungleTree() {
    // 丛林木配置
    auto config = std::make_unique<TreeFeatureConfig>(jungleConfig());

    auto placement = appendBiomePlacement(
        createCountedSurfacePlacement(6),
        {Biomes::Jungle});

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(placement), "jungle_tree");
}

std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createSparseOakTree() {
    // 稀疏橡树（用于平原）
    auto config = std::make_unique<TreeFeatureConfig>(oakConfig());

    auto placement = appendBiomePlacement(
        createChanceSurfacePlacement(0.1f),
        {Biomes::Plains, Biomes::Savanna, Biomes::SavannaPlateau, Biomes::ShatteredSavanna});

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(placement), "sparse_oak_tree");
}

TreeFeatureConfig TreeFeatures::oakConfig() {
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::OAK_LOG ? &VanillaBlocks::OAK_LOG->defaultState() : nullptr;
    config.foliageBlock = VanillaBlocks::OAK_LEAVES ? &VanillaBlocks::OAK_LEAVES->defaultState() : nullptr;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        3
    );
    config.minHeight = 4;
    return config;
}

TreeFeatureConfig TreeFeatures::birchConfig() {
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::BIRCH_LOG ? &VanillaBlocks::BIRCH_LOG->defaultState() : nullptr;
    config.foliageBlock = VanillaBlocks::BIRCH_LEAVES ? &VanillaBlocks::BIRCH_LEAVES->defaultState() : nullptr;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2
    );
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::spruceConfig() {
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::SPRUCE_LOG ? &VanillaBlocks::SPRUCE_LOG->defaultState() : nullptr;
    config.foliageBlock = VanillaBlocks::SPRUCE_LEAVES ? &VanillaBlocks::SPRUCE_LEAVES->defaultState() : nullptr;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 2, 1);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        3
    );
    config.minHeight = 5;
    return config;
}

TreeFeatureConfig TreeFeatures::jungleConfig() {
    TreeFeatureConfig config;
    config.trunkBlock = VanillaBlocks::JUNGLE_LOG ? &VanillaBlocks::JUNGLE_LOG->defaultState() : nullptr;
    config.foliageBlock = VanillaBlocks::JUNGLE_LEAVES ? &VanillaBlocks::JUNGLE_LEAVES->defaultState() : nullptr;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 8, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2
    );
    config.minHeight = 4;
    return config;
}

} // namespace mc
