#include <gtest/gtest.h>
#include <cmath>

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/ChunkMesher.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// BlockGeometry 测试
// ============================================================================

TEST(BlockGeometry, FaceNormals) {
    auto bottom = BlockGeometry::getFaceNormal(Face::Bottom);
    EXPECT_FLOAT_EQ(bottom[0], 0.0f);
    EXPECT_FLOAT_EQ(bottom[1], -1.0f);
    EXPECT_FLOAT_EQ(bottom[2], 0.0f);

    auto top = BlockGeometry::getFaceNormal(Face::Top);
    EXPECT_FLOAT_EQ(top[0], 0.0f);
    EXPECT_FLOAT_EQ(top[1], 1.0f);
    EXPECT_FLOAT_EQ(top[2], 0.0f);

    auto north = BlockGeometry::getFaceNormal(Face::North);
    EXPECT_FLOAT_EQ(north[0], 0.0f);
    EXPECT_FLOAT_EQ(north[1], 0.0f);
    EXPECT_FLOAT_EQ(north[2], -1.0f);

    auto south = BlockGeometry::getFaceNormal(Face::South);
    EXPECT_FLOAT_EQ(south[0], 0.0f);
    EXPECT_FLOAT_EQ(south[1], 0.0f);
    EXPECT_FLOAT_EQ(south[2], 1.0f);

    auto west = BlockGeometry::getFaceNormal(Face::West);
    EXPECT_FLOAT_EQ(west[0], -1.0f);
    EXPECT_FLOAT_EQ(west[1], 0.0f);
    EXPECT_FLOAT_EQ(west[2], 0.0f);

    auto east = BlockGeometry::getFaceNormal(Face::East);
    EXPECT_FLOAT_EQ(east[0], 1.0f);
    EXPECT_FLOAT_EQ(east[1], 0.0f);
    EXPECT_FLOAT_EQ(east[2], 0.0f);
}

TEST(BlockGeometry, FaceVertices) {
    // 顶部面应该有4个顶点，每个顶点3个分量
    auto topVerts = BlockGeometry::getFaceVertices(Face::Top);
    EXPECT_EQ(topVerts.size(), 12u); // 4顶点 * 3分量

    // 顶部面Y坐标应该是1
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(topVerts[i * 3 + 1], 1.0f); // Y坐标
    }

    // 底部面Y坐标应该是0
    auto bottomVerts = BlockGeometry::getFaceVertices(Face::Bottom);
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(bottomVerts[i * 3 + 1], 0.0f); // Y坐标
    }
}

TEST(BlockGeometry, FaceIndices) {
    auto indices = BlockGeometry::getFaceIndices();
    EXPECT_EQ(indices.size(), 6u); // 2三角形 * 3索引

    // 索引应该在0-3范围内
    for (u32 idx : indices) {
        EXPECT_LT(idx, 4u);
    }

    // 第一个三角形
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_EQ(indices[2], 2u);

    // 第二个三角形
    EXPECT_EQ(indices[3], 0u);
    EXPECT_EQ(indices[4], 2u);
    EXPECT_EQ(indices[5], 3u);
}

TEST(BlockGeometry, FaceDirection) {
    auto bottomDir = BlockGeometry::getFaceDirection(Face::Bottom);
    EXPECT_EQ(bottomDir[0], 0);
    EXPECT_EQ(bottomDir[1], -1);
    EXPECT_EQ(bottomDir[2], 0);

    auto topDir = BlockGeometry::getFaceDirection(Face::Top);
    EXPECT_EQ(topDir[1], 1);

    auto northDir = BlockGeometry::getFaceDirection(Face::North);
    EXPECT_EQ(northDir[2], -1);

    auto southDir = BlockGeometry::getFaceDirection(Face::South);
    EXPECT_EQ(southDir[2], 1);

    auto westDir = BlockGeometry::getFaceDirection(Face::West);
    EXPECT_EQ(westDir[0], -1);

    auto eastDir = BlockGeometry::getFaceDirection(Face::East);
    EXPECT_EQ(eastDir[0], 1);
}

TEST(BlockGeometry, ShouldRenderFace) {
    // 如果邻居不透明，不渲染面
    EXPECT_FALSE(BlockGeometry::shouldRenderFace(Face::Top, true));
    EXPECT_TRUE(BlockGeometry::shouldRenderFace(Face::Top, false));
}

// ============================================================================
// TextureAtlas 测试
// ============================================================================

TEST(TextureAtlas, Construction) {
    TextureAtlas atlas(256, 256, 16);

    EXPECT_EQ(atlas.textureWidth(), 256u);
    EXPECT_EQ(atlas.textureHeight(), 256u);
    EXPECT_EQ(atlas.tileSize(), 16u);
    EXPECT_EQ(atlas.tilesPerRow(), 16u);
}

TEST(TextureAtlas, GetRegionByCoords) {
    TextureAtlas atlas(256, 256, 16);

    // 第一个图块 (0,0)
    auto r0 = atlas.getRegion(0, 0);
    EXPECT_FLOAT_EQ(r0.u0, 0.0f);
    EXPECT_FLOAT_EQ(r0.v0, 0.0f);
    EXPECT_FLOAT_EQ(r0.u1, 1.0f / 16.0f);
    EXPECT_FLOAT_EQ(r0.v1, 1.0f / 16.0f);

    // 第一个图块 (1,0) - 第二列
    auto r1 = atlas.getRegion(1, 0);
    EXPECT_FLOAT_EQ(r1.u0, 1.0f / 16.0f);
    EXPECT_FLOAT_EQ(r1.v0, 0.0f);
    EXPECT_FLOAT_EQ(r1.u1, 2.0f / 16.0f);
    EXPECT_FLOAT_EQ(r1.v1, 1.0f / 16.0f);

    // 第一行第二列 (0,1)
    auto r2 = atlas.getRegion(0, 1);
    EXPECT_FLOAT_EQ(r2.u0, 0.0f);
    EXPECT_FLOAT_EQ(r2.v0, 1.0f / 16.0f);
    EXPECT_FLOAT_EQ(r2.u1, 1.0f / 16.0f);
    EXPECT_FLOAT_EQ(r2.v1, 2.0f / 16.0f);
}

TEST(TextureAtlas, GetRegionByIndex) {
    TextureAtlas atlas(256, 256, 16);

    // 线性索引 0
    auto r0 = atlas.getRegion(0);
    EXPECT_FLOAT_EQ(r0.u0, 0.0f);
    EXPECT_FLOAT_EQ(r0.v0, 0.0f);

    // 线性索引 1
    auto r1 = atlas.getRegion(1);
    EXPECT_FLOAT_EQ(r1.u0, 1.0f / 16.0f);
    EXPECT_FLOAT_EQ(r1.v0, 0.0f);

    // 线性索引 16 (第二行开始)
    auto r16 = atlas.getRegion(16);
    EXPECT_FLOAT_EQ(r16.u0, 0.0f);
    EXPECT_FLOAT_EQ(r16.v0, 1.0f / 16.0f);
}

// ============================================================================
// MeshData 测试
// ============================================================================

TEST(MeshData, Construction) {
    MeshData mesh;
    EXPECT_TRUE(mesh.empty());
    EXPECT_EQ(mesh.vertexCount(), 0u);
    EXPECT_EQ(mesh.indexCount(), 0u);
}

TEST(MeshData, Reserve) {
    MeshData mesh;
    mesh.reserve(100, 200);
    // 预留容量，不增加大小
    EXPECT_TRUE(mesh.empty());
}

TEST(MeshData, Clear) {
    MeshData mesh;
    mesh.vertices.push_back(Vertex());
    mesh.indices.push_back(0);
    EXPECT_FALSE(mesh.empty());

    mesh.clear();
    EXPECT_TRUE(mesh.empty());
}

// ============================================================================
// Vertex 测试
// ============================================================================

TEST(Vertex, Construction) {
    Vertex v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vertex, ParameterizedConstruction) {
    Vertex v(1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0xFF0000FF, 10);

    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.nx, 0.0f);
    EXPECT_FLOAT_EQ(v.ny, 1.0f);
    EXPECT_FLOAT_EQ(v.nz, 0.0f);
    EXPECT_FLOAT_EQ(v.u, 0.5f);
    EXPECT_FLOAT_EQ(v.v, 0.5f);
    EXPECT_EQ(v.color, 0xFF0000FFu);
    EXPECT_EQ(v.light, 10u);
}

// ============================================================================
// ChunkRenderData 测试
// ============================================================================

TEST(ChunkRenderData, Construction) {
    ChunkRenderData data;
    EXPECT_TRUE(data.solidMesh.empty());
    EXPECT_TRUE(data.transparentMesh.empty());
    EXPECT_TRUE(data.needsUpdate);
    EXPECT_FALSE(data.isDirty);
    EXPECT_EQ(data.vertexCount, 0u);
    EXPECT_EQ(data.indexCount, 0u);
}

TEST(ChunkRenderData, MarkDirty) {
    ChunkRenderData data;
    data.markClean();
    EXPECT_FALSE(data.isDirty);
    EXPECT_FALSE(data.needsUpdate);

    data.markDirty();
    EXPECT_TRUE(data.isDirty);
    EXPECT_TRUE(data.needsUpdate);
}

// ============================================================================
// ChunkMeshCache 测试
// ============================================================================

TEST(ChunkMeshCache, Construction) {
    ChunkMeshCache cache(100);
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.dirtyCount(), 0u);
}

TEST(ChunkMeshCache, GetOrCreate) {
    ChunkMeshCache cache(100);

    ChunkId id(10, 20);
    ChunkRenderData* data = cache.getOrCreate(id);

    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->chunkId.x, 10);
    EXPECT_EQ(data->chunkId.z, 20);
    EXPECT_EQ(cache.size(), 1u);

    // 再次获取应该返回相同的数据
    ChunkRenderData* data2 = cache.getOrCreate(id);
    EXPECT_EQ(data, data2);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(ChunkMeshCache, Get) {
    ChunkMeshCache cache(100);

    ChunkId id(5, 10);

    // 不存在时返回nullptr
    EXPECT_EQ(cache.get(id), nullptr);

    // 创建后应该能获取
    cache.getOrCreate(id);
    EXPECT_NE(cache.get(id), nullptr);
}

TEST(ChunkMeshCache, MarkDirty) {
    ChunkMeshCache cache(100);

    ChunkId id(1, 2);
    ChunkRenderData* data = cache.getOrCreate(id);
    data->markClean();

    cache.markDirty(id);
    EXPECT_TRUE(data->isDirty);
    EXPECT_TRUE(data->needsUpdate);
    EXPECT_EQ(cache.dirtyCount(), 1u);
}

TEST(ChunkMeshCache, Remove) {
    ChunkMeshCache cache(100);

    ChunkId id(1, 2);
    cache.getOrCreate(id);
    EXPECT_EQ(cache.size(), 1u);

    cache.remove(id);
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.get(id), nullptr);
}

TEST(ChunkMeshCache, Clear) {
    ChunkMeshCache cache(100);

    cache.getOrCreate(ChunkId(1, 2));
    cache.getOrCreate(ChunkId(3, 4));
    cache.getOrCreate(ChunkId(5, 6));
    EXPECT_EQ(cache.size(), 3u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.dirtyCount(), 0u);
}

// ============================================================================
// ChunkMesher 测试
// ============================================================================
// 注意：ChunkMesher 现在需要 BlockModelCache 才能正常工作
// BlockModelCache 需要从 ResourceManager 获取方块外观
// 这些测试在没有设置 BlockModelCache 的情况下会返回空网格

class ChunkMesherTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();

        // 创建一个简单的测试区块
        testChunk = std::make_unique<ChunkData>(0, 0);

        // 注意：不再使用 BlockModelRegistry，它已被移除
        // ChunkMesher 现在需要通过 setModelCache() 设置 BlockModelCache
        // 在没有 BlockModelCache 的情况下，ChunkMesher 将不会生成任何面
    }

    std::unique_ptr<ChunkData> testChunk;
};

TEST_F(ChunkMesherTest, GenerateEmptyChunk) {
    MeshData mesh;
    ChunkMesher::generateMesh(*testChunk, mesh, nullptr);

    // 空区块应该产生空网格
    EXPECT_TRUE(mesh.empty());
}

TEST_F(ChunkMesherTest, GenerateSingleBlockWithoutModelCache) {
    // 放置一个方块
    testChunk->setBlock(8, 64, 8, &VanillaBlocks::STONE->defaultState());

    MeshData mesh;
    ChunkMesher::generateMesh(*testChunk, mesh, nullptr);

    // 在没有 BlockModelCache 的情况下，不会生成任何面
    // 因为 ChunkMesher 需要从 BlockModelCache 获取方块外观
    EXPECT_TRUE(mesh.empty());
}

TEST_F(ChunkMesherTest, SettingsTest) {
    // 测试设置
    bool originalGreedy = ChunkMesher::isGreedyMeshingEnabled();
    bool originalLighting = ChunkMesher::isLightingEnabled();

    ChunkMesher::setGreedyMeshing(true);
    EXPECT_TRUE(ChunkMesher::isGreedyMeshingEnabled());

    ChunkMesher::setLightingEnabled(false);
    EXPECT_FALSE(ChunkMesher::isLightingEnabled());

    // 恢复原始设置
    ChunkMesher::setGreedyMeshing(originalGreedy);
    ChunkMesher::setLightingEnabled(originalLighting);
}

TEST_F(ChunkMesherTest, ModelCacheIsNullByDefault) {
    // 默认情况下 BlockModelCache 应该是 nullptr
    EXPECT_EQ(ChunkMesher::modelCache(), nullptr);
}
