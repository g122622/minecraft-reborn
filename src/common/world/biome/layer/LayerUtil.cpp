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
// LayerUtil 实现 - 简化版本
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
    (void)riverSize; // 暂时未使用

    // 创建上下文工厂 - 使用 shared_ptr 保持生命周期
    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    // 创建源层
    auto context = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);

    // 应用模糊缩放
    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    context = createContext(2000);
    factory = fuzzyZoom.apply(*context, std::move(factory));

    // 应用多个 AddIslandLayer
    static layer::AddIslandLayer addIslandLayer;
    context = createContext(1);
    factory = addIslandLayer.apply(*context, std::move(factory));

    // 应用普通缩放
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    context = createContext(2001);
    factory = normalZoom.apply(*context, std::move(factory));

    // 应用 AddSnowLayer
    static layer::AddSnowLayer addSnowLayer;
    context = createContext(2);
    factory = addSnowLayer.apply(*context, std::move(factory));

    // 更多缩放
    i32 zoomCount = largeBiomes ? 6 : biomeSize;
    for (i32 i = 0; i < zoomCount + 4; ++i) {
        context = createContext(static_cast<u64>(2002 + i));
        factory = normalZoom.apply(*context, std::move(factory));
    }

    // 应用 BiomeLayer
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{legacyBiomeInit});
    context = createContext(200);
    factory = biomeLayer.apply(*context, std::move(factory));

    // 最终缩放
    for (i32 i = 0; i < zoomCount; ++i) {
        context = createContext(static_cast<u64>(3000 + i));
        factory = normalZoom.apply(*context, std::move(factory));
    }

    // 应用 SmoothLayer
    static layer::SmoothLayer smoothLayer;
    context = createContext(1000);
    factory = smoothLayer.apply(*context, std::move(factory));

    return factory;
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
                container.setBiome(bx << 2, by << 2, bz << 2, biome);
            }
        }
    }
}

} // namespace mc
