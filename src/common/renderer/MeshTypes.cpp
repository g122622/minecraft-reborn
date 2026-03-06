#include "MeshTypes.hpp"
#include "../world/block/Block.hpp"
#include "../world/block/VanillaBlocks.hpp"
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
    (void)face;
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

const BlockModel* BlockModelRegistry::getModel(const BlockState* state) const {
    if (!state) return nullptr;
    return getModel(state->blockId());
}

const BlockModel* BlockModelRegistry::getModel(u32 blockId) const {
    auto it = m_models.find(blockId);
    if (it != m_models.end()) {
        return it->second.get();
    }
    return nullptr;
}

void BlockModelRegistry::registerVanillaModels() {
    // 使用16x16纹理图集 (假设图集中图块位置)
    // 简化版: 所有方块使用相同的纹理位置
    // TODO: 实际加载纹理图集时更新这些值

    // 注册基础方块模型 - 使用 VanillaBlocks 中的方块ID
    auto registerSimple = [this](u32 blockId, u32 tileX, u32 tileY) {
        m_models[blockId] = std::make_unique<SimpleBlockModel>(m_atlas->getRegion(tileX, tileY));
    };

    auto registerCube = [this](u32 blockId, u32 topX, u32 topY,
                               u32 bottomX, u32 bottomY,
                               u32 sideX, u32 sideY) {
        m_models[blockId] = std::make_unique<CubeBlockModel>(
            m_atlas->getRegion(topX, topY),
            m_atlas->getRegion(bottomX, bottomY),
            m_atlas->getRegion(sideX, sideY),
            m_atlas->getRegion(sideX, sideY),
            m_atlas->getRegion(sideX, sideY),
            m_atlas->getRegion(sideX, sideY)
        );
    };

    // 注册原版方块模型（如果已初始化）
    if (VanillaBlocks::STONE) {
        registerSimple(VanillaBlocks::STONE->blockId(), 1, 0);
    }
    if (VanillaBlocks::GRASS_BLOCK) {
        registerCube(VanillaBlocks::GRASS_BLOCK->blockId(), 0, 0, 2, 0, 3, 0);
    }
    if (VanillaBlocks::DIRT) {
        registerSimple(VanillaBlocks::DIRT->blockId(), 2, 0);
    }
    if (VanillaBlocks::COBBLESTONE) {
        registerSimple(VanillaBlocks::COBBLESTONE->blockId(), 4, 0);
    }
    if (VanillaBlocks::OAK_PLANKS) {
        registerSimple(VanillaBlocks::OAK_PLANKS->blockId(), 5, 0);
    }
    if (VanillaBlocks::WATER) {
        registerSimple(VanillaBlocks::WATER->blockId(), 6, 0);
    }
    if (VanillaBlocks::LAVA) {
        registerSimple(VanillaBlocks::LAVA->blockId(), 7, 0);
    }
    if (VanillaBlocks::BEDROCK) {
        registerSimple(VanillaBlocks::BEDROCK->blockId(), 8, 0);
    }
    if (VanillaBlocks::SAND) {
        registerSimple(VanillaBlocks::SAND->blockId(), 9, 0);
    }
    if (VanillaBlocks::GRAVEL) {
        registerSimple(VanillaBlocks::GRAVEL->blockId(), 10, 0);
    }
    if (VanillaBlocks::GOLD_ORE) {
        registerSimple(VanillaBlocks::GOLD_ORE->blockId(), 11, 0);
    }
    if (VanillaBlocks::IRON_ORE) {
        registerSimple(VanillaBlocks::IRON_ORE->blockId(), 12, 0);
    }
    if (VanillaBlocks::COAL_ORE) {
        registerSimple(VanillaBlocks::COAL_ORE->blockId(), 13, 0);
    }
    if (VanillaBlocks::DIAMOND_ORE) {
        registerSimple(VanillaBlocks::DIAMOND_ORE->blockId(), 14, 0);
    }
    if (VanillaBlocks::DIAMOND_BLOCK) {
        registerSimple(VanillaBlocks::DIAMOND_BLOCK->blockId(), 15, 0);
    }
    if (VanillaBlocks::OAK_LOG) {
        registerCube(VanillaBlocks::OAK_LOG->blockId(), 16, 0, 16, 0, 17, 0);
    }
    if (VanillaBlocks::OAK_LEAVES) {
        registerSimple(VanillaBlocks::OAK_LEAVES->blockId(), 18, 0);
    }
    if (VanillaBlocks::SNOW) {
        registerSimple(VanillaBlocks::SNOW->blockId(), 19, 0);
    }
    if (VanillaBlocks::ICE) {
        registerSimple(VanillaBlocks::ICE->blockId(), 20, 0);
    }
    if (VanillaBlocks::NETHERRACK) {
        registerSimple(VanillaBlocks::NETHERRACK->blockId(), 21, 0);
    }
    if (VanillaBlocks::GLOWSTONE) {
        registerSimple(VanillaBlocks::GLOWSTONE->blockId(), 22, 0);
    }
    if (VanillaBlocks::END_STONE) {
        registerSimple(VanillaBlocks::END_STONE->blockId(), 23, 0);
    }
    if (VanillaBlocks::OBSIDIAN) {
        registerSimple(VanillaBlocks::OBSIDIAN->blockId(), 24, 0);
    }
}

} // namespace mr
