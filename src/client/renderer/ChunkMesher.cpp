#include "ChunkMesher.hpp"
#include "../resource/BlockModelCache.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mr {

// ============================================================================
// 静态成员初始化
// ============================================================================

BlockModelCache* ChunkMesher::s_modelCache = nullptr;
bool ChunkMesher::s_useGreedyMeshing = false;
bool ChunkMesher::s_lightingEnabled = true;

// ============================================================================
// ChunkMesher 实现
// ============================================================================

void ChunkMesher::setModelCache(BlockModelCache* cache) {
    s_modelCache = cache;
    if (cache) {
        spdlog::info("ChunkMesher: Using BlockModelCache for block appearances");
    } else {
        spdlog::warn("ChunkMesher: BlockModelCache set to null, no models will be rendered");
    }
}

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
        simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
    }
}

bool ChunkMesher::shouldRenderBlock(const BlockState* state) {
    // 空气和液体不渲染实体网格
    if (!state) return false;
    return !state->isAir();
}

bool ChunkMesher::shouldRenderFace(const BlockState* /*block*/, const BlockState* neighbor) {
    // 如果邻居是空气，渲染面
    if (!neighbor || neighbor->isAir()) {
        return true;
    }

    // 如果邻居是液体，渲染面
    if (neighbor->isLiquid()) {
        return true;
    }

    // 如果邻居是透明的（如玻璃、树叶），渲染面
    if (neighbor->isTransparent()) {
        return true;
    }

    // 否则不渲染面
    return false;
}

u8 ChunkMesher::getBlockLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    // 获取方块光照 (天空光照和方块光照的最大值)
    (void)neighborChunks; // 暂时不使用邻居区块

    const ChunkSection* section = chunk.getSection(y / world::CHUNK_SECTION_HEIGHT);
    if (!section) {
        return 15; // 默认全亮
    }

    i32 localY = y % world::CHUNK_SECTION_HEIGHT;
    u8 skyLight = section->getSkyLight(x, localY, z);
    u8 blockLight = section->getBlockLight(x, localY, z);

    return std::max(skyLight, blockLight);
}

void ChunkMesher::addFaceFromAppearance(
    MeshData& mesh,
    Face face,
    f32 x, f32 y, f32 z,
    u8 light,
    const BlockAppearance* appearance
) {
    if (!appearance) {
        return;
    }

    // 检查是否有 elements
    if (appearance->elements.empty()) {
        return;
    }

    // 查找面的纹理
    String faceName;
    switch (face) {
        case Face::Bottom: faceName = "down"; break;
        case Face::Top: faceName = "up"; break;
        case Face::North: faceName = "north"; break;
        case Face::South: faceName = "south"; break;
        case Face::West: faceName = "west"; break;
        case Face::East: faceName = "east"; break;
        default: return;
    }

    auto texIt = appearance->faceTextures.find(faceName);
    if (texIt == appearance->faceTextures.end()) {
        // 尝试使用 "side" 作为后备
        texIt = appearance->faceTextures.find("side");
        if (texIt == appearance->faceTextures.end()) {
            // 尝试使用 "all" 作为后备
            texIt = appearance->faceTextures.find("all");
            if (texIt == appearance->faceTextures.end()) {
                return; // 没有找到纹理，跳过此面
            }
        }
    }

    TextureRegion tex = texIt->second;

    auto normal = BlockGeometry::getFaceNormal(face);
    auto vertices = BlockGeometry::getFaceVertices(face);

    // 创建4个顶点
    std::array<Vertex, 4> faceVerts;

    // UV坐标根据顶点位置设置
    // 顶点顺序: 左下、右下、右上、左上
    f32 uvs[4][2] = {
        { tex.u0, tex.v1 }, // 左下
        { tex.u1, tex.v1 }, // 右下
        { tex.u1, tex.v0 }, // 右上
        { tex.u0, tex.v0 }  // 左上
    };

    // `Vertex::light` 使用 `VK_FORMAT_R8_UNORM` 传入着色器，
    // 需要把 MC 的光照等级 [0, 15] 映射到 [0, 255]。
    const u8 packedLight = static_cast<u8>(std::min<u32>(255, static_cast<u32>(light) * 17u));

    for (size_t i = 0; i < 4; ++i) {
        faceVerts[i] = Vertex(
            x + vertices[i * 3 + 0],
            y + vertices[i * 3 + 1],
            z + vertices[i * 3 + 2],
            normal[0], normal[1], normal[2],
            uvs[i][0], uvs[i][1],
            0xFFFFFFFF,
            packedLight
        );
    }

    // 添加顶点和索引
    u32 baseIndex = static_cast<u32>(mesh.vertices.size());
    for (const auto& v : faceVerts) {
        mesh.vertices.push_back(v);
    }

    auto indices = BlockGeometry::getFaceIndices();
    for (u32 idx : indices) {
        mesh.indices.push_back(baseIndex + idx);
    }
}

void ChunkMesher::simpleMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 必须有 BlockModelCache
    if (!s_modelCache) {
        spdlog::error("ChunkMesher: BlockModelCache not initialized, cannot generate mesh");
        return;
    }

    constexpr i32 SIZE = ChunkSection::SIZE;
    const i32 baseY = sectionIndex * SIZE;

    // 获取当前段
    const ChunkSection* section = chunk.getSection(sectionIndex);
    if (!section || section->isEmpty()) {
        return;
    }

    // 遍历段内所有方块
    for (i32 y = 0; y < SIZE; ++y) {
        for (i32 z = 0; z < SIZE; ++z) {
            for (i32 x = 0; x < SIZE; ++x) {
                const BlockState* block = section->getBlock(x, y, z);
                if (!shouldRenderBlock(block)) {
                    continue;
                }

                // 获取方块外观
                const BlockAppearance* appearance = s_modelCache->getBlockAppearance(block);
                if (!appearance) {
                    // 使用缺失模型
                    appearance = s_modelCache->getMissingAppearance();
                    if (!appearance) {
                        continue; // 无法渲染
                    }
                }

                // 检查外观是否有效
                if (appearance->elements.empty() && appearance->faceTextures.empty()) {
                    continue;
                }

                // 检查每个面
                for (size_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
                    Face face = static_cast<Face>(faceIdx);
                    auto dir = BlockGeometry::getFaceDirection(face);

                    // 计算邻居坐标
                    i32 nx = x + dir[0];
                    i32 ny = y + dir[1];
                    i32 nz = z + dir[2];

                    const BlockState* neighbor = nullptr;

                    // 检查是否在当前区块内
                    if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE && nz >= 0 && nz < SIZE) {
                        neighbor = section->getBlock(nx, ny, nz);
                    } else {
                        // 在区块边界，需要查询邻居区块或当前区块的其他段
                        i32 worldX = nx;
                        i32 worldY = baseY + ny;
                        i32 worldZ = nz;

                        // 处理Y方向跨段
                        if (worldY < 0 || worldY >= world::CHUNK_HEIGHT) {
                            neighbor = nullptr; // 空气
                        } else if (worldX >= 0 && worldX < world::CHUNK_WIDTH &&
                                   worldZ >= 0 && worldZ < world::CHUNK_WIDTH) {
                            // 仍在当前区块内
                            neighbor = chunk.getBlock(worldX, worldY, worldZ);
                        } else if (neighborChunks) {
                            // 在邻居区块中
                            i32 neighborIdx = -1;
                            if (worldX < 0) neighborIdx = 0;        // -X
                            else if (worldX >= SIZE) neighborIdx = 1; // +X
                            else if (worldZ < 0) neighborIdx = 2;     // -Z
                            else if (worldZ >= SIZE) neighborIdx = 3; // +Z

                            if (neighborIdx >= 0 && neighborChunks[neighborIdx]) {
                                // 计算邻居区块中的坐标
                                i32 lx = (worldX + SIZE) % SIZE;
                                i32 lz = (worldZ + SIZE) % SIZE;
                                neighbor = neighborChunks[neighborIdx]->getBlock(lx, worldY, lz);
                            } else {
                                neighbor = nullptr;
                            }
                        } else {
                            neighbor = nullptr;
                        }
                    }

                    // 决定是否渲染该面
                    if (shouldRenderFace(block, neighbor)) {
                        u8 light = 15; // 默认光照
                        if (s_lightingEnabled) {
                            light = getBlockLight(chunk, x, baseY + y, z, neighborChunks);
                        }

                        addFaceFromAppearance(outMesh, face,
                                static_cast<f32>(x),
                                static_cast<f32>(baseY + y),
                                static_cast<f32>(z),
                                light, appearance);
                    }
                }
            }
        }
    }
}

void ChunkMesher::greedyMeshSection(
    const ChunkData& chunk,
    i32 sectionIndex,
    MeshData& outMesh,
    const ChunkData* neighborChunks[6]
) {
    // 贪婪网格合并算法
    // 暂时使用简单网格生成，贪婪网格合并算法较复杂
    // TODO: 实现完整的贪婪网格合并
    simpleMeshSection(chunk, sectionIndex, outMesh, neighborChunks);
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

    // 检查是否需要清理旧缓存
    if (m_cache.size() >= m_maxCachedChunks) {
        // 简单的LRU策略：移除标记为脏的旧数据
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
