#include "LayerUtil.hpp"
#include "../BiomeRegistry.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// LayerStack 实现
// ============================================================================

LayerStack::LayerStack(std::unique_ptr<IArea> area)
    : m_area(std::move(area))
{
}

BiomeId LayerStack::sample(i32 x, i32 z) const {
    if (!m_area) {
        return Biomes::Plains;
    }

    i32 value = m_area->getValue(x, z);

    // 确保值是有效的生物群系 ID
    if (value >= 0 && value < static_cast<i32>(Biomes::Count)) {
        return static_cast<BiomeId>(value);
    }

    // 可能是稀有变体或其他有效 ID
    if (value >= 129 && value <= 169) {
        return static_cast<BiomeId>(value);
    }

    return Biomes::Plains;
}

std::vector<BiomeId> LayerStack::sampleArea(i32 startX, i32 startZ, i32 width, i32 height) const {
    std::vector<BiomeId> result;
    result.reserve(static_cast<size_t>(width) * height);

    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            result.push_back(sample(startX + x, startZ + z));
        }
    }

    return result;
}

// ============================================================================
// LayerUtil 实现 - 完整的 MC 1.16.5 层链（简化版，无河流分支）
// ============================================================================

namespace LayerUtil {

std::unique_ptr<IAreaFactory> repeatZoom(
    u64 seed,
    layer::ZoomLayer& zoom,
    std::unique_ptr<IAreaFactory> input,
    i32 count,
    std::function<std::shared_ptr<LayerContext>(u64)> contextFactory)
{
    std::unique_ptr<IAreaFactory> result = std::move(input);

    for (i32 i = 0; i < count; ++i) {
        auto context = contextFactory(seed + static_cast<u64>(i));
        result = zoom.apply(*context, std::move(result));
    }

    return result;
}

std::unique_ptr<IAreaFactory> buildOverworldLayers(
    u64 seed,
    bool legacyBiomeInit,
    bool largeBiomes,
    i32 biomeSize,
    i32 riverSize)
{
    (void)riverSize; // 暂时不使用河流大小参数

    // 创建上下文工厂 - 使用 shared_ptr 保持生命周期
    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    // ========================================================================
    // 第一阶段：基础岛屿和海洋层
    // 参考 MC LayerUtil.func_237216_a_ 行 97-116
    // ========================================================================

    auto context = createContext(1L);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);

    // FuzzyZoom
    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    context = createContext(2000L);
    factory = fuzzyZoom.apply(*context, std::move(factory));

    // 多次 AddIslandLayer
    static layer::AddIslandLayer addIslandLayer;
    context = createContext(1L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    // NormalZoom
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    context = createContext(2001L);
    factory = normalZoom.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(50L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(70L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    // RemoveTooMuchOceanLayer
    static layer::RemoveTooMuchOceanLayer removeTooMuchOceanLayer;
    context = createContext(2L);
    factory = removeTooMuchOceanLayer.apply(*context, std::move(factory));

    // ========================================================================
    // 第二阶段：温度和生物群系层
    // 参考 MC 行 107-116
    // ========================================================================

    // AddSnowLayer（温度分配）
    static layer::AddSnowLayer addSnowLayer;
    context = createContext(2L);
    factory = addSnowLayer.apply(*context, std::move(factory));

    // AddIslandLayer
    context = createContext(3L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    // CoolWarmEdgeLayer
    static layer::CoolWarmEdgeLayer coolWarmEdgeLayer;
    context = createContext(2L);
    factory = coolWarmEdgeLayer.apply(*context, std::move(factory));

    // HeatIceEdgeLayer
    static layer::HeatIceEdgeLayer heatIceEdgeLayer;
    context = createContext(2L);
    factory = heatIceEdgeLayer.apply(*context, std::move(factory));

    // SpecialEdgeLayer
    static layer::SpecialEdgeLayer specialEdgeLayer;
    context = createContext(3L);
    factory = specialEdgeLayer.apply(*context, std::move(factory));

    // 缩放两次
    context = createContext(2002L);
    factory = normalZoom.apply(*context, std::move(factory));
    context = createContext(2003L);
    factory = normalZoom.apply(*context, std::move(factory));

    // AddIslandLayer
    context = createContext(4L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    // AddMushroomIslandLayer
    static layer::AddMushroomIslandLayer addMushroomIslandLayer;
    context = createContext(5L);
    factory = addMushroomIslandLayer.apply(*context, std::move(factory));

    // DeepOceanLayer
    static layer::DeepOceanLayer deepOceanLayer;
    context = createContext(4L);
    factory = deepOceanLayer.apply(*context, std::move(factory));

    // ========================================================================
    // 第三阶段：生物群系分配
    // 参考 MC 行 117-123
    // 重要：MC 在 BiomeLayer 之前执行 0 次缩放！
    // ========================================================================

    // 注意：MC 代码：iareafactory = repeat(1000L, ZoomLayer.NORMAL, iareafactory, 0, ...)
    // 第4个参数是 0，表示 0 次缩放！
    // 我们需要保存一个副本用于河流分支

    // 保存气候层副本用于河流分支
    // 注意：由于 unique_ptr 不能复制，我们需要不同的策略
    // 暂时简化实现：不创建并行河流分支

    // BiomeLayer - 将温度值转换为实际生物群系（在未缩放的气候层上）
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{legacyBiomeInit});
    context = createContext(200L);
    auto biomeFactory = biomeLayer.apply(*context, std::move(factory));

    // AddBambooForestLayer
    static layer::AddBambooForestLayer addBambooForestLayer;
    context = createContext(1001L);
    biomeFactory = addBambooForestLayer.apply(*context, std::move(biomeFactory));

    // 生物群系缩放 2 次
    biomeFactory = repeatZoom(1000L, normalZoom, std::move(biomeFactory), 2, createContext);

    // BiomeEdgeLayer
    static layer::BiomeEdgeLayer biomeEdgeLayer;
    context = createContext(1000L);
    biomeFactory = biomeEdgeLayer.apply(*context, std::move(biomeFactory));

    // ========================================================================
    // 第四阶段：最终处理
    // 参考 MC 行 130-143
    // ========================================================================

    // RareBiomeLayer
    static layer::RareBiomeLayer rareBiomeLayer;
    context = createContext(1001L);
    biomeFactory = rareBiomeLayer.apply(*context, std::move(biomeFactory));

    // 按 biomeSize 缩放
    for (i32 i = 0; i < biomeSize; ++i) {
        context = createContext(static_cast<u64>(1000 + i));
        biomeFactory = normalZoom.apply(*context, std::move(biomeFactory));

        // 第一次缩放后添加 AddIslandLayer
        if (i == 0) {
            context = createContext(3L);
            biomeFactory = addIslandLayer.apply(*context, std::move(biomeFactory));
        }

        // 最后一次（或只有一次）缩放后添加 ShoreLayer
        if (i == biomeSize - 1 || biomeSize == 0) {
            static layer::ShoreLayer shoreLayer;
            context = createContext(1000L);
            biomeFactory = shoreLayer.apply(*context, std::move(biomeFactory));
        }
    }

    // SmoothLayer 最终
    static layer::SmoothLayer smoothLayer;
    context = createContext(1000L);
    biomeFactory = smoothLayer.apply(*context, std::move(biomeFactory));

    // 注意：暂时跳过河流层和海洋混合层，因为它们需要分支（unique_ptr 不支持复制）
    // 河流层将在后续版本中实现

    return biomeFactory;
}

std::unique_ptr<LayerStack> createOverworldLayers(u64 seed, bool isLargeBiomes) {
    // 初始化生物群系注册表
    BiomeRegistry::instance().initialize();

    // 构建层链
    auto factory = buildOverworldLayers(seed, false, isLargeBiomes);

    // 创建最终区域
    auto area = factory->create();

    return std::make_unique<LayerStack>(std::move(area));
}

std::unique_ptr<LayerStack> createNetherLayers(u64 seed) {
    // 下界使用简化的层堆叠
    BiomeRegistry::instance().initialize();
    auto context = std::make_shared<LayerContext>(1024, seed, 1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);
    auto area = factory->create();
    return std::make_unique<LayerStack>(std::move(area));
}

std::unique_ptr<LayerStack> createEndLayers(u64 seed) {
    // 末地使用简化的层堆叠
    BiomeRegistry::instance().initialize();
    auto context = std::make_shared<LayerContext>(1024, seed, 1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);
    auto area = factory->create();
    return std::make_unique<LayerStack>(std::move(area));
}

} // namespace LayerUtil

// ============================================================================
// LayerBiomeProvider 实现
// ============================================================================

LayerBiomeProvider::LayerBiomeProvider(u64 seed, bool isLargeBiomes)
    : BiomeProvider(seed)
    , m_layerStack(LayerUtil::createOverworldLayers(seed, isLargeBiomes))
{
}

BiomeId LayerBiomeProvider::getBiome(i32 x, i32 y, i32 z) const {
    (void)y;  // Layer 系统不使用 Y 坐标
    return m_layerStack->sample(x, z);
}

BiomeId LayerBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const {
    (void)noiseY;  // Layer 系统不使用 Y 坐标
    // 噪声坐标是 4x4 方块一个大块
    return m_layerStack->sample(noiseX << 2, noiseZ << 2);
}

f32 LayerBiomeProvider::getDepth(i32 x, i32 z) const {
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.depth();
}

f32 LayerBiomeProvider::getScale(i32 x, i32 z) const {
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.scale();
}

const Biome& LayerBiomeProvider::getBiomeDefinition(BiomeId id) const {
    return BiomeRegistry::instance().get(id);
}

void LayerBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) {
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 遍历生物群系采样点 (4x4x4 方块为一个采样点)
    for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 worldX = startX + (bx << 2);
                const i32 worldZ = startZ + (bz << 2);

                const BiomeId biome = m_layerStack->sample(worldX, worldZ);
                container.setBiome(bx, by, bz, biome);
            }
        }
    }
}

} // namespace mc
