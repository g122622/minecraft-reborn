#include "ChunkMesher.hpp"
#include <unordered_map>
#include <algorithm>

namespace mr {

// ============================================================================
// 静态成员初始化
// ============================================================================

bool ChunkMesher::s_useGreedyMeshing = false; // 默认使用简单网格
bool ChunkMesher::s_lightingEnabled = true;
bool ChunkMesher::s_useResourceModels = false; // 默认使用内置模型

// ============================================================================
// 优化数据结构 - 预计算的面数据
// ============================================================================

namespace {

// 面方向偏移 (X, Y, Z) - 静态常量避免重复计算
constexpr i32 FACE_DIRECTIONS[6][3] = {
    { 0, -1,  0},  // Bottom (Y-)
    { 0,  1,  0},  // Top (Y+)
    { 0,  0, -1},  // North (Z-)
    { 0,  0,  1},  // South (Z+)
    {-1,  0,  0},  // West (X-)
    { 1,  0,  0}   // East (X+)
};

// 面法线 - 静态常量
constexpr f32 FACE_NORMALS[6][3] = {
    { 0.0f, -1.0f,  0.0f},  // Bottom
    { 0.0f,  1.0f,  0.0f},  // Top
    { 0.0f,  0.0f, -1.0f},  // North
    { 0.0f,  0.0f,  1.0f},  // South
    {-1.0f,  0.0f,  0.0f},  // West
    { 1.0f,  0.0f,  0.0f}   // East
};

// 面顶点偏移 (4个顶点，每个3个分量) - 相对于方块位置
// 顺序：左下、右下、右上、左上
constexpr f32 FACE_VERTICES[6][12] = {
    // Bottom (Y-)
    { 0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f },
    // Top (Y+)
    { 0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 1.0f },
    // North (Z-)
    { 1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f },
    // South (Z+)
    { 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f },
    // West (X-)
    { 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f },
    // East (X+)
    { 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f }
};

// 标准索引模式
constexpr u32 FACE_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// UV 坐标模式 (对应4个顶点)
constexpr f32 FACE_UVS[4][2] = {
    { 0.0f, 1.0f },  // 左下
    { 1.0f, 1.0f },  // 右下
    { 1.0f, 0.0f },  // 右上
    { 0.0f, 0.0f }   // 左上
};

// 默认纹理区域 (全图)
constexpr TextureRegion DEFAULT_TEXTURE(0.0f, 0.0f, 1.0f, 1.0f);

// 快速检测方块是否应该渲染 (内联)
inline bool fastShouldRenderBlock(u32 stateId) {
    // stateId == 0 是空气
    return stateId != 0;
}

// 快速检测面是否应该渲染
inline bool fastShouldRenderFace(u32 neighborStateId) {
    // 邻居是空气 (0) 时渲染
    if (neighborStateId == 0) return true;

    // 需要查询 BlockState 判断透明度
    const BlockState* neighborState = Block::getBlockState(neighborStateId);
    if (!neighborState) return true;

    // 透明或液体方块需要渲染面
    return neighborState->isTransparent() || neighborState->isLiquid();
}

// 内联添加面 - 避免函数调用开销
inline void fastAddFace(
    MeshData& mesh,
    i32 x, i32 y, i32 z,
    u32 stateId,
    u8 light,
    Face face,
    const TextureRegion& tex)
{
    const f32* verts = FACE_VERTICES[static_cast<size_t>(face)];
    const f32* normal = FACE_NORMALS[static_cast<size_t>(face)];

    u32 baseIndex = static_cast<u32>(mesh.vertices.size());

    // 计算 UV 坐标
    f32 uvs[4][2] = {
        { tex.u0, tex.v1 },  // 左下
        { tex.u1, tex.v1 },  // 右下
        { tex.u1, tex.v0 },  // 右上
        { tex.u0, tex.v0 }   // 左上
    };

    // 添加4个顶点
    for (size_t i = 0; i < 4; ++i) {
        mesh.vertices.emplace_back(
            static_cast<f32>(x) + verts[i * 3 + 0],
            static_cast<f32>(y) + verts[i * 3 + 1],
            static_cast<f32>(z) + verts[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            0xFFFFFFFF,
            light
        );
    }

    // 添加6个索引
    for (u32 idx : FACE_INDICES) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

} // anonymous namespace

// ============================================================================
// ChunkMesher 实现
// ============================================================================

void ChunkMesher::generateMesh(
    const ChunkData& chunk,
    MeshData& outMesh,
    const ChunkData* neighbors[6]
) {
    outMesh.clear();

    // 预分配空间 (每个区块最多约98304个顶点)
    outMesh.reserve(65536, 98304);

    // 遍历所有区块段
    for (i32 sectionY = 0; sectionY < ChunkData::SECTIONS; ++sectionY) {
        if (chunk.hasSection(sectionY)) {
            generateSectionMesh(chunk, sectionY, outMesh, neighbors);
        }
    }
}

void ChunkMesher::generateSectionMesh(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    if (s_useGreedyMeshing) {
        greedyMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
    } else {
        optimizedMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
    }
}

bool ChunkMesher::shouldRenderBlock(const BlockState* state) {
    if (!state) return false;
    return !state->isAir();
}

bool ChunkMesher::shouldRenderFace(const BlockState* block, const BlockState* neighbor) {
    if (!neighbor || neighbor->isAir()) {
        return true;
    }
    if (neighbor->isLiquid()) {
        return true;
    }
    if (neighbor->isTransparent()) {
        return true;
    }
    return false;
}

u8 ChunkMesher::getBlockLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    (void)neighborChunks;

    const ChunkSection* section = chunk.getSection(y / world::CHUNK_SECTION_HEIGHT);
    if (!section) {
        return 15;
    }

    i32 localY = y % world::CHUNK_SECTION_HEIGHT;
    u8 skyLight = section->getSkyLight(x, localY, z);
    u8 blockLight = section->getBlockLight(x, localY, z);

    return std::max(skyLight, blockLight);
}

void ChunkMesher::addFace(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    const BlockState* state,
    u8 light,
    const BlockModel* model)
{
    if (!state) return;

    if (!model) {
        model = BlockModelRegistry::instance().getModel(state);
    }
    if (!model) return;

    TextureRegion tex = model->getFaceTexture(face);
    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    std::array<Vertex, 4> faceVerts;

    f32 uvs[4][2] = {
        { tex.u0, tex.v1 },
        { tex.u1, tex.v1 },
        { tex.u1, tex.v0 },
        { tex.u0, tex.v0 }
    };

    for (size_t i = 0; i < 4; ++i) {
        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            0xFFFFFFFF,
            light
        );
    }

    u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

// ============================================================================
// 优化的网格生成
// ============================================================================

void ChunkMesher::optimizedMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    constexpr i32 SIZE = ChunkSection::SIZE;
    const i32 baseY = sectionIndex * SIZE;

    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    // 直接访问 stateId 数组
    const std::vector<u32>& blockStates = section->getBlockStates();

    // 获取相邻区块的段指针 (避免重复查询)
    const ChunkSection* neighborSections[6] = { nullptr };
    for (i32 i = 0; i < 4; ++i) {
        if (neighborChunks[i]) {
            // X, Z 方向的邻居
            neighborSections[i] = neighborChunks[i]->getSection(sectionIndex);
        }
    }
    // Y 方向邻居 (同一区块的不同段)
    if (sectionIndex > 0) {
        neighborSections[4] = chunk.getSection(sectionIndex - 1); // -Y
    }
    if (sectionIndex < ChunkData::SECTIONS - 1) {
        neighborSections[5] = chunk.getSection(sectionIndex + 1); // +Y
    }

    // 遍历段内所有方块
    for (i32 y = 0; y < SIZE; ++y) {
        const i32 worldY = baseY + y;

        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                const i32 index = y * SIZE * SIZE + z * SIZE + x;
                const u32 stateId = blockStates[index];

                // 快速跳过空气
                if (!fastShouldRenderBlock(stateId)) {
                    continue;
                }

                // 获取方块状态 (只获取一次)
                const BlockState* blockState = Block::getBlockState(stateId);
                if (!blockState) continue;

                // 获取模型 (只获取一次)
                const BlockModel* model = BlockModelRegistry::instance().getModel(blockState);
                if (!model) continue;

                // 检查每个面
                for (i32 faceIdx = 0; faceIdx < 6; ++faceIdx) {
                    const i32 nx = x + FACE_DIRECTIONS[faceIdx][0];
                    const i32 ny = y + FACE_DIRECTIONS[faceIdx][1];
                    const i32 nz = z + FACE_DIRECTIONS[faceIdx][2];

                    u32 neighborStateId = 0;

                    // 快速获取邻居 stateId
                    if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                        // 邻居在同一区块段内
                        neighborStateId = blockStates[ny * SIZE * SIZE + nz * SIZE + nx];
                    } else {
                        // 边界情况
                        if (ny < 0) {
                            // -Y 方向
                            if (neighborSections[4]) {
                                neighborStateId = neighborSections[4]->getBlockStateIdFast(
                                    x * SIZE * SIZE + z * SIZE + (SIZE - 1));
                            }
                        } else if (ny >= SIZE) {
                            // +Y 方向
                            if (neighborSections[5]) {
                                neighborStateId = neighborSections[5]->getBlockStateIdFast(
                                    x * SIZE * SIZE + z * SIZE);
                            }
                        } else if (nx < 0) {
                            // -X 方向
                            if (neighborSections[0]) {
                                neighborStateId = neighborSections[0]->getBlockStateIdFast(
                                    ny * SIZE * SIZE + nz * SIZE + (SIZE - 1));
                            }
                        } else if (nx >= SIZE) {
                            // +X 方向
                            if (neighborSections[1]) {
                                neighborStateId = neighborSections[1]->getBlockStateIdFast(
                                    ny * SIZE * SIZE + nz * SIZE);
                            }
                        } else if (nz < 0) {
                            // -Z 方向
                            if (neighborSections[2]) {
                                neighborStateId = neighborSections[2]->getBlockStateIdFast(
                                    ny * SIZE * SIZE + (SIZE - 1) * SIZE + nx);
                            }
                        } else if (nz >= SIZE) {
                            // +Z 方向
                            if (neighborSections[3]) {
                                neighborStateId = neighborSections[3]->getBlockStateIdFast(
                                    ny * SIZE * SIZE + nx);
                            }
                        }
                    }

                    // 决定是否渲染该面
                    if (fastShouldRenderFace(neighborStateId)) {
                        u8 light = 15;
                        if (s_lightingEnabled) {
                            light = getBlockLight(chunk, x, worldY, z, neighborChunks);
                        }

                        // 获取纹理并添加面
                        TextureRegion tex = model->getFaceTexture(static_cast<Face>(faceIdx));
                        fastAddFace(outMesh, x, worldY, z, stateId, light,
                                    static_cast<Face>(faceIdx), tex);
                    }
                }
            }
        }
    }
}

void ChunkMesher::simpleMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 使用优化版本
    optimizedMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
}

void ChunkMesher::greedyMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 贪婪网格合并算法
    // TODO: 实现完整的贪婪网格合并
    optimizedMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
}

// ============================================================================
// ChunkMeshCache 实现
// ============================================================================

ChunkMeshCache::ChunkMeshCache(size_t maxCachedChunks)
    : m_maxCachedChunks(maxCachedChunks)
{
}

ChunkRenderData* ChunkMeshCache::getOrCreate(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }

    if (m_cache.size() >= m_maxCachedChunks) {
        for (auto it2 = m_cache.begin(); it2 != m_cache.end(); ) {
            if (!it2->second.isDirty && !it2->second.needsUpdate) {
                it2 = m_cache.erase(it2);
                if (m_cache.size() < m_maxCachedChunks) break;
            } else {
                ++it2;
            }
        }
    }

    auto& data = m_cache[id];
    data.chunkId = id;
    return &data;
}

ChunkRenderData* ChunkMeshCache::get(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }
    return nullptr;
}

const ChunkRenderData* ChunkMeshCache::get(const ChunkId& id) const {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        return &it->second;
    }
    return nullptr;
}

void ChunkMeshCache::markDirty(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        if (!it->second.isDirty) {
            m_dirtyCount++;
        }
        it->second.markDirty();
    }
}

void ChunkMeshCache::remove(const ChunkId& id) {
    auto it = m_cache.find(id);
    if (it != m_cache.end()) {
        if (it->second.isDirty) {
            m_dirtyCount--;
        }
        m_cache.erase(it);
    }
}

void ChunkMeshCache::clear() {
    m_cache.clear();
    m_dirtyCount = 0;
}

} // namespace mr
