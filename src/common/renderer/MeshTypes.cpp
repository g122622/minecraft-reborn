#include "MeshTypes.hpp"
#include <memory>

namespace mr {

// ============================================================================
// BlockGeometry 实现
// ============================================================================

namespace BlockGeometry {

std::array<f32, 3> getFaceNormal(Face face) {
    switch (face) {
        case Face::Bottom: return { 0.0f, -1.0f, 0.0f };
        case Face::Top:    return { 0.0f,  1.0f, 0.0f };
        case Face::North:  return { 0.0f,  0.0f, -1.0f };
        case Face::South:  return { 0.0f,  0.0f,  1.0f };
        case Face::West:   return { -1.0f, 0.0f, 0.0f };
        case Face::East:   return { 1.0f,  0.0f, 0.0f };
        default:           return { 0.0f,  0.0f, 0.0f };
    }
}

std::array<f32, 12> getFaceVertices(Face face) {
    // 顶点按逆时针顺序排列 (从面外侧看)
    // 方块范围为 [0,0,0] 到 [1,1,1]
    switch (face) {
        case Face::Bottom: // Y- (下)
            return {
                0.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f
            };
        case Face::Top: // Y+ (上)
            return {
                0.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 0.0f,
                0.0f, 1.0f, 0.0f
            };
        case Face::North: // Z- (北)
            return {
                1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 0.0f
            };
        case Face::South: // Z+ (南)
            return {
                0.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                0.0f, 1.0f, 1.0f
            };
        case Face::West: // X- (西)
            return {
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 1.0f, 1.0f,
                0.0f, 1.0f, 0.0f
            };
        case Face::East: // X+ (东)
            return {
                1.0f, 0.0f, 1.0f,
                1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f
            };
        default:
            return {};
    }
}

std::array<u32, 6> getFaceIndices() {
    // 两个三角形组成一个四边形
    // 顶点顺序: 0-1-2, 0-2-3
    return { 0, 1, 2, 0, 2, 3 };
}

std::array<i32, 3> getFaceDirection(Face face) {
    switch (face) {
        case Face::Bottom: return { 0, -1, 0 };
        case Face::Top:    return { 0,  1, 0 };
        case Face::North:  return { 0,  0, -1 };
        case Face::South:  return { 0,  0,  1 };
        case Face::West:   return { -1, 0, 0 };
        case Face::East:   return { 1,  0, 0 };
        default:           return { 0,  0, 0 };
    }
}

bool shouldRenderFace(Face face, bool neighborOpaque) {
    // 如果邻居不透明，不渲染该面
    return !neighborOpaque;
}

} // namespace BlockGeometry

// ============================================================================
// MeshData 实现
// ============================================================================

void MeshData::addFace(const std::array<Vertex, 4>& faceVertices, u32 baseIndex) {
    // 添加顶点
    for (const auto& v : faceVertices) {
        vertices.push_back(v);
    }

    // 添加索引
    auto faceIndices = BlockGeometry::getFaceIndices();
    for (u32 idx : faceIndices) {
        indices.push_back(baseIndex + idx);
    }
}

// ============================================================================
// TextureAtlas 实现
// ============================================================================

TextureAtlas::TextureAtlas(u32 textureWidth, u32 textureHeight, u32 tileSize)
    : m_textureWidth(textureWidth)
    , m_textureHeight(textureHeight)
    , m_tileSize(tileSize)
    , m_tilesPerRow(textureWidth / tileSize)
    , m_tileU(1.0f / static_cast<f32>(textureWidth / tileSize))
    , m_tileV(1.0f / static_cast<f32>(textureHeight / tileSize))
{
}

TextureRegion TextureAtlas::getRegion(u32 tileX, u32 tileY) const {
    f32 u0 = static_cast<f32>(tileX) * m_tileU;
    f32 v0 = static_cast<f32>(tileY) * m_tileV;
    return TextureRegion(u0, v0, u0 + m_tileU, v0 + m_tileV);
}

TextureRegion TextureAtlas::getRegion(u32 tileIndex) const {
    u32 tileX = tileIndex % m_tilesPerRow;
    u32 tileY = tileIndex / m_tilesPerRow;
    return getRegion(tileX, tileY);
}

// ============================================================================
// BlockModelRegistry 实现
// ============================================================================

BlockModelRegistry& BlockModelRegistry::instance() {
    static BlockModelRegistry registry;
    return registry;
}

void BlockModelRegistry::initialize(const TextureAtlas& atlas) {
    if (m_initialized) return;
    m_atlas = &atlas;
    registerVanillaModels();
    m_initialized = true;
}

const BlockModel* BlockModelRegistry::getModel(BlockId blockId) const {
    size_t index = static_cast<size_t>(blockId);
    if (index < m_models.size()) {
        return m_models[index].get();
    }
    return nullptr;
}

void BlockModelRegistry::registerVanillaModels() {
    // 使用16x16纹理图集 (假设图集中图块位置)
    // 简化版: 所有方块使用相同的纹理位置
    // TODO: 实际加载纹理图集时更新这些值

    // 空气 - 无模型
    m_models[static_cast<size_t>(BlockId::Air)] = nullptr;

    // 石头
    m_models[static_cast<size_t>(BlockId::Stone)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(1, 0));

    // 草地 - 不同面不同纹理
    m_models[static_cast<size_t>(BlockId::GrassBlock)] =
        std::make_unique<CubeBlockModel>(
            m_atlas->getRegion(0, 0),  // 顶部 (草地)
            m_atlas->getRegion(2, 0),  // 底部 (泥土)
            m_atlas->getRegion(3, 0),  // 北面 (侧面)
            m_atlas->getRegion(3, 0),  // 南面 (侧面)
            m_atlas->getRegion(3, 0),  // 西面 (侧面)
            m_atlas->getRegion(3, 0)   // 东面 (侧面)
        );

    // 泥土
    m_models[static_cast<size_t>(BlockId::Dirt)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(2, 0));

    // 圆石
    m_models[static_cast<size_t>(BlockId::Cobblestone)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(4, 0));

    // 木板
    m_models[static_cast<size_t>(BlockId::OakPlanks)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(5, 0));

    // 水 - 使用简化模型
    m_models[static_cast<size_t>(BlockId::Water)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(6, 0));

    // 岩浆
    m_models[static_cast<size_t>(BlockId::Lava)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(7, 0));

    // 基岩
    m_models[static_cast<size_t>(BlockId::Bedrock)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(8, 0));

    // 沙子
    m_models[static_cast<size_t>(BlockId::Sand)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(9, 0));

    // 沙砾
    m_models[static_cast<size_t>(BlockId::Gravel)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(10, 0));

    // 矿石类
    m_models[static_cast<size_t>(BlockId::GoldOre)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(11, 0));
    m_models[static_cast<size_t>(BlockId::IronOre)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(12, 0));
    m_models[static_cast<size_t>(BlockId::CoalOre)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(13, 0));
    m_models[static_cast<size_t>(BlockId::DiamondOre)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(14, 0));

    // 钻石块
    m_models[static_cast<size_t>(BlockId::DiamondBlock)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(15, 0));

    // 原木
    m_models[static_cast<size_t>(BlockId::OakLog)] =
        std::make_unique<CubeBlockModel>(
            m_atlas->getRegion(16, 0),  // 顶部
            m_atlas->getRegion(16, 0),  // 底部
            m_atlas->getRegion(17, 0),  // 侧面
            m_atlas->getRegion(17, 0),
            m_atlas->getRegion(17, 0),
            m_atlas->getRegion(17, 0)
        );

    // 树叶
    m_models[static_cast<size_t>(BlockId::OakLeaves)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(18, 0));

    // 雪
    m_models[static_cast<size_t>(BlockId::Snow)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(19, 0));

    // 冰
    m_models[static_cast<size_t>(BlockId::Ice)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(20, 0));

    // 地狱岩
    m_models[static_cast<size_t>(BlockId::Netherrack)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(21, 0));

    // 荧石
    m_models[static_cast<size_t>(BlockId::Glowstone)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(22, 0));

    // 末地石
    m_models[static_cast<size_t>(BlockId::EndStone)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(23, 0));

    // 黑曜石
    m_models[static_cast<size_t>(BlockId::Obsidian)] =
        std::make_unique<SimpleBlockModel>(m_atlas->getRegion(24, 0));
}

} // namespace mr
