#include "LayerUtil.hpp"
#include "../BiomeRegistry.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// LayerStack 实现
// ============================================================================

LayerStack::LayerStack(u64 seed, bool isLargeBiomes)
    : m_seed(seed)
    , m_isLargeBiomes(isLargeBiomes)
    , m_context(std::make_shared<SimpleAreaContext>(seed))
{
    initLayers();
}

void LayerStack::initLayers()
{
    // 参考 MC LayerUtil 的层堆叠顺序
    // 1. IslandLayer -> 创建初始岛屿
    // 2. ZoomLayer -> 放大
    // 3. AddIslandLayer -> 添加岛屿
    // 4. ZoomLayer -> 放大
    // 5. AddSnowLayer -> 添加雪地
    // 6. EdgeLayer -> 边缘处理
    // 7. ZoomLayer -> 放大
    // 8. AddMushroomIsland -> 蘑菇岛
    // 9. DeepOceanLayer -> 深海
    // 10. ZoomLayer -> 放大
    // 11. BiomeLayer -> 分配生物群系
    // 12. ZoomLayer -> 放大
    // 13. HillsLayer -> 添加山丘
    // 14. ZoomLayer -> 放大
    // 15. ShoreLayer -> 海岸
    // 16. ZoomLayer -> 放大
    // 17. SmoothLayer -> 平滑
    // 18. RiverLayer -> 河流
    // 19. MixRiverLayer -> 混合河流
    // 20. ZoomLayer (large biomes only) -> 大型生物群系额外放大

    // 简化版本：使用缓存区域模拟
    constexpr i32 BASE_SIZE = 256;  // 基础区域大小

    // 创建初始层（简单岛屿生成）
    m_layers.push_back(std::make_unique<LazyArea>(
        std::make_unique<IslandLayer>(m_seed),
        m_context,
        nullptr,
        BASE_SIZE, BASE_SIZE
    ));

    // 应用缩放和变换
    // 在实际实现中，这些层会链式组合
    // 这里简化为直接使用最终层

    // TODO: 实现完整的层堆叠
}

BiomeId LayerStack::sample(i32 x, i32 z) const
{
    if (m_layers.empty()) {
        return Biomes::Plains;
    }

    // 将世界坐标转换为区域坐标
    // 参考 MC，生物群系采样使用 1:1 比例（在某些层之后）
    const i32 areaX = x;
    const i32 areaZ = z;

    // 从最终层采样
    const i32 value = m_layers.back()->getValue(areaX, areaZ);

    // 确保值是有效的生物群系 ID
    if (value >= 0 && value < static_cast<i32>(Biomes::Count)) {
        return static_cast<BiomeId>(value);
    }

    return Biomes::Plains;
}

std::vector<BiomeId> LayerStack::sampleArea(i32 startX, i32 startZ, i32 width, i32 height) const
{
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
// LayerBiomeProvider 实现
// ============================================================================

LayerBiomeProvider::LayerBiomeProvider(u64 seed, bool isLargeBiomes)
    : BiomeProvider(seed)
    , m_layerStack(LayerUtil::createOverworldLayers(seed, isLargeBiomes))
{
    // 初始化生物群系注册表
    BiomeRegistry::instance().initialize();
}

BiomeId LayerBiomeProvider::getBiome(i32 x, i32 y, i32 z) const
{
    (void)y; // Layer 系统不使用 Y 坐标
    return m_layerStack->sample(x, z);
}

BiomeId LayerBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    (void)noiseY; // Layer 系统不使用 Y 坐标
    // 噪声坐标是 4x4 方块一个大块
    return m_layerStack->sample(noiseX << 2, noiseZ << 2);
}

f32 LayerBiomeProvider::getDepth(i32 x, i32 z) const
{
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.depth();
}

f32 LayerBiomeProvider::getScale(i32 x, i32 z) const
{
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.scale();
}

const Biome& LayerBiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

void LayerBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
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

// ============================================================================
// LayerUtil 实现
// ============================================================================

namespace LayerUtil {

std::unique_ptr<LayerStack> createOverworldLayers(u64 seed, bool isLargeBiomes)
{
    return std::make_unique<LayerStack>(seed, isLargeBiomes);
}

std::unique_ptr<LayerStack> createNetherLayers(u64 seed)
{
    // 下界使用不同的层堆叠
    // 简化版本：使用相同结构但不同参数
    return std::make_unique<LayerStack>(seed, false);
}

std::unique_ptr<LayerStack> createEndLayers(u64 seed)
{
    // 末地使用简化的层堆叠
    return std::make_unique<LayerStack>(seed, false);
}

} // namespace LayerUtil

} // namespace mr
