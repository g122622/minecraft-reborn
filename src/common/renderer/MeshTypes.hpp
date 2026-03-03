#pragma once

#include "../core/Types.hpp"
#include "../world/BlockID.hpp"
#include <array>
#include <vector>
#include <memory>

namespace mr {

// ============================================================================
// 顶点格式
// ============================================================================

struct Vertex {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;       // 位置
    f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;    // 法线
    f32 u = 0.0f, v = 0.0f;                  // 纹理坐标
    u32 color = 0xFFFFFFFF;                  // 顶点颜色 (RGBA)
    u8 light = 15;                           // 光照等级 (0-15)

    Vertex() = default;
    Vertex(f32 px, f32 py, f32 pz, f32 nu, f32 nv, f32 nw, f32 tu, f32 tv, u32 col = 0xFFFFFFFF, u8 l = 15)
        : x(px), y(py), z(pz)
        , nx(nu), ny(nv), nz(nw)
        , u(tu), v(tv)
        , color(col)
        , light(l) {}
};

// ============================================================================
// 方块朝向
// ============================================================================

enum class Face : u8 {
    Bottom = 0,  // Y- (下)
    Top = 1,     // Y+ (上)
    North = 2,   // Z- (北)
    South = 3,   // Z+ (南)
    West = 4,    // X- (西)
    East = 5,    // X+ (东)
    Count = 6
};

// ============================================================================
// 方块面顶点数据
// ============================================================================

namespace BlockGeometry {

// 每个面的顶点数
constexpr u32 VERTICES_PER_FACE = 4;

// 每个面的索引数 (两个三角形)
constexpr u32 INDICES_PER_FACE = 6;

// 获取面的法线
[[nodiscard]] std::array<f32, 3> getFaceNormal(Face face);

// 获取面的顶点位置 (相对于方块左下角)
// 返回4个顶点的位置，每个顶点3个分量
[[nodiscard]] std::array<f32, 12> getFaceVertices(Face face);

// 获取标准面的索引 (两个三角形)
[[nodiscard]] std::array<u32, 6> getFaceIndices();

// 获取面的方向向量 (用于邻居检测)
[[nodiscard]] std::array<i32, 3> getFaceDirection(Face face);

// 检查面是否应该在给定朝向渲染
[[nodiscard]] bool shouldRenderFace(Face face, bool neighborOpaque);

} // namespace BlockGeometry

// ============================================================================
// 网格数据
// ============================================================================

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    void clear() {
        vertices.clear();
        indices.clear();
    }

    void reserve(size_t vertexCount, size_t indexCount) {
        vertices.reserve(vertexCount);
        indices.reserve(indexCount);
    }

    [[nodiscard]] bool empty() const {
        return vertices.empty();
    }

    [[nodiscard]] size_t vertexCount() const {
        return vertices.size();
    }

    [[nodiscard]] size_t indexCount() const {
        return indices.size();
    }

    // 添加一个面 (4个顶点 + 6个索引)
    void addFace(const std::array<Vertex, 4>& faceVertices, u32 baseIndex);
};

// ============================================================================
// 纹理坐标
// ============================================================================

struct TextureRegion {
    f32 u0, v0;  // 左上角
    f32 u1, v1;  // 右下角

    TextureRegion() = default;
    TextureRegion(f32 u0_, f32 v0_, f32 u1_, f32 v1_)
        : u0(u0_), v0(v0_), u1(u1_), v1(v1_) {}
};

// ============================================================================
// 方块模型
// ============================================================================

class BlockModel {
public:
    virtual ~BlockModel() = default;

    // 获取指定面的纹理区域
    [[nodiscard]] virtual TextureRegion getFaceTexture(Face face) const = 0;

    // 是否渲染指定面 (某些方块可能没有所有面)
    [[nodiscard]] virtual bool hasFace(Face face) const { return true; }

    // 是否使用简化渲染 (单面)
    [[nodiscard]] virtual bool isSimple() const { return true; }
};

// ============================================================================
// 标准方块模型 (所有面相同纹理)
// ============================================================================

class SimpleBlockModel : public BlockModel {
public:
    explicit SimpleBlockModel(TextureRegion texture)
        : m_texture(texture) {}

    [[nodiscard]] TextureRegion getFaceTexture(Face face) const override {
        (void)face;
        return m_texture;
    }

private:
    TextureRegion m_texture;
};

// ============================================================================
// 立方体方块模型 (不同面不同纹理，如草地)
// ============================================================================

class CubeBlockModel : public BlockModel {
public:
    CubeBlockModel(TextureRegion top, TextureRegion bottom, TextureRegion north,
                   TextureRegion south, TextureRegion west, TextureRegion east)
        : m_textures{bottom, top, north, south, west, east} {}

    [[nodiscard]] TextureRegion getFaceTexture(Face face) const override {
        return m_textures[static_cast<size_t>(face)];
    }

private:
    std::array<TextureRegion, 6> m_textures;
};

// ============================================================================
// 纹理图集
// ============================================================================

class TextureAtlas {
public:
    TextureAtlas(u32 textureWidth, u32 textureHeight, u32 tileSize);

    // 获取纹理区域 (以格子坐标)
    [[nodiscard]] TextureRegion getRegion(u32 tileX, u32 tileY) const;

    // 获取纹理区域 (以线性索引)
    [[nodiscard]] TextureRegion getRegion(u32 tileIndex) const;

    // 获取图集尺寸
    [[nodiscard]] u32 textureWidth() const { return m_textureWidth; }
    [[nodiscard]] u32 textureHeight() const { return m_textureHeight; }
    [[nodiscard]] u32 tileSize() const { return m_tileSize; }
    [[nodiscard]] u32 tilesPerRow() const { return m_tilesPerRow; }

private:
    u32 m_textureWidth;
    u32 m_textureHeight;
    u32 m_tileSize;
    u32 m_tilesPerRow;
    f32 m_tileU;
    f32 m_tileV;
};

// ============================================================================
// 方块模型注册表
// ============================================================================

class BlockModelRegistry {
public:
    static BlockModelRegistry& instance();

    void initialize(const TextureAtlas& atlas);

    [[nodiscard]] const BlockModel* getModel(BlockId blockId) const;

private:
    BlockModelRegistry() = default;

    void registerVanillaModels();

    std::array<std::unique_ptr<BlockModel>, 256> m_models;
    bool m_initialized = false;
    const TextureAtlas* m_atlas = nullptr;
};

} // namespace mr
