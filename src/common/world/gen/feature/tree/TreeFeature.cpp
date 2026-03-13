#include "TreeFeature.hpp"
#include "trunk/StraightTrunkPlacer.hpp"
#include "foliage/BlobFoliagePlacer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../core/Types.hpp"
#include "../../placement/Placement.hpp"
#include <spdlog/spdlog.h>

namespace mc {

namespace {

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
    if (state == nullptr) {
        return true;
    }

    u32 blockId = state->blockId();

    // 空气、树叶、草、花等可替换
    if (blockId == static_cast<u32>(BlockId::Air)) {
        return true;
    }

    // 检查是否是树叶
    if (blockId == static_cast<u32>(BlockId::OakLeaves) ||
        blockId == static_cast<u32>(BlockId::SpruceLeaves) ||
        blockId == static_cast<u32>(BlockId::BirchLeaves) ||
        blockId == static_cast<u32>(BlockId::JungleLeaves) ||
        blockId == static_cast<u32>(BlockId::AcaciaLeaves) ||
        blockId == static_cast<u32>(BlockId::DarkOakLeaves)) {
        return true;
    }

    // 检查是否是植被
    if (blockId == static_cast<u32>(BlockId::ShortGrass) ||
        blockId == static_cast<u32>(BlockId::TallGrass) ||
        blockId == static_cast<u32>(BlockId::Fern) ||
        blockId == static_cast<u32>(BlockId::Dandelion) ||
        blockId == static_cast<u32>(BlockId::Poppy) ||
        blockId == static_cast<u32>(BlockId::OakSapling) ||
        blockId == static_cast<u32>(BlockId::SpruceSapling) ||
        blockId == static_cast<u32>(BlockId::BirchSapling) ||
        blockId == static_cast<u32>(BlockId::JungleSapling) ||
        blockId == static_cast<u32>(BlockId::AcaciaSapling) ||
        blockId == static_cast<u32>(BlockId::DarkOakSapling)) {
        return true;
    }

    // 检查是否是水
    if (blockId == static_cast<u32>(BlockId::Water)) {
        return true;
    }

    return false;
}

bool TreeFeature::isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return true;
    }

    u32 blockId = state->blockId();

    if (blockId == static_cast<u32>(BlockId::Air)) {
        return true;
    }

    // 检查是否是树叶
    if (blockId == static_cast<u32>(BlockId::OakLeaves) ||
        blockId == static_cast<u32>(BlockId::SpruceLeaves) ||
        blockId == static_cast<u32>(BlockId::BirchLeaves) ||
        blockId == static_cast<u32>(BlockId::JungleLeaves) ||
        blockId == static_cast<u32>(BlockId::AcaciaLeaves) ||
        blockId == static_cast<u32>(BlockId::DarkOakLeaves)) {
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

    u32 blockId = state->blockId();

    // 检查是否是泥土类方块
    return blockId == static_cast<u32>(BlockId::Dirt) ||
           blockId == static_cast<u32>(BlockId::Grass) ||
           blockId == static_cast<u32>(BlockId::CoarseDirt) ||
           blockId == static_cast<u32>(BlockId::Podzol);
}

bool TreeFeature::isWaterAt(WorldGenRegion& world, const BlockPos& pos) {
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }

    return state->blockId() == static_cast<u32>(BlockId::Water);
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
    config.trunkBlock = BlockId::OakLog;
    config.foliageBlock = BlockId::OakLeaves;
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
    config.trunkBlock = BlockId::BirchLog;
    config.foliageBlock = BlockId::BirchLeaves;
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
    config.trunkBlock = BlockId::SpruceLog;
    config.foliageBlock = BlockId::SpruceLeaves;
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
    config.trunkBlock = BlockId::JungleLog;
    config.foliageBlock = BlockId::JungleLeaves;
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
