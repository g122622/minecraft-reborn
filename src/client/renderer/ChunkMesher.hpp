#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/world/block/Block.hpp"
#include "MeshTypes.hpp"
#include <memory>
#include <functional>

namespace mc {

// 前向声明
class BlockModelCache;
struct BlockAppearance;

// ============================================================================
// 区块网格生成器
// ============================================================================

/**
 * @brief 区块网格生成器
 *
 * 负责将 ChunkData 转换为可渲染的 MeshData。
 * 使用 BlockModelCache 获取方块外观信息。
 *
 * 使用示例:
 * @code
 * // 初始化时设置模型缓存
 * ChunkMesher::setModelCache(&modelCache);
 *
 * // 生成网格
 * MeshData mesh;
 * ChunkMesher::generateMesh(chunk, mesh, neighbors);
 *
 * // 清理
 * ChunkMesher::setModelCache(nullptr);
 * @endcode
 */
class ChunkMesher {
public:
    // ========================================================================
    // 网格生成
    // ========================================================================

    /**
     * @brief 生成区块网格
     *
     * @param chunk 区块数据
     * @param outMesh 输出网格
     * @param neighbors 周围6个区块 (用于边界面的剔除)
     *                  顺序: -X, +X, -Z, +Z, -Y, +Y (可以是nullptr)
     */
    static void generateMesh(
        const ChunkData& chunk,
        MeshData& outMesh,
        const ChunkData* neighbors[6] = nullptr
    );

    /**
     * @brief 生成单个区块段的网格 (用于渐进加载)
     *
     * @param chunk 区块数据
     * @param sectionIndex 区段索引 (0-15)
     * @param outMesh 输出网格
     * @param neighborChunks 周围区块
     */
    static void generateSectionMesh(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6] = nullptr
    );

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置 BlockModelCache
     *
     * 必须在使用 ChunkMesher 之前调用。
     * BlockModelCache 用于获取方块的外观信息（模型、纹理）。
     *
     * @param cache 模型缓存指针（可以为 nullptr 禁用渲染）
     */
    static void setModelCache(BlockModelCache* cache);

    /**
     * @brief 获取 BlockModelCache
     */
    static BlockModelCache* modelCache() { return s_modelCache; }

    /**
     * @brief 设置是否使用贪婪网格合并
     */
    static void setGreedyMeshing(bool enabled) { s_useGreedyMeshing = enabled; }
    static bool isGreedyMeshingEnabled() { return s_useGreedyMeshing; }

    /**
     * @brief 设置光照计算
     */
    static void setLightingEnabled(bool enabled) { s_lightingEnabled = enabled; }
    static bool isLightingEnabled() { return s_lightingEnabled; }

private:
    // 检查方块是否应该渲染
    static bool shouldRenderBlock(const BlockState* state);

    // 检查面是否应该渲染
    static bool shouldRenderFace(
        const BlockState* block,
        const BlockState* neighbor
    );

    // 获取方块光照
    static u8 getBlockLight(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighborChunks[6]
    );

    // 添加单个面的顶点（使用 BlockAppearance）
    static void addFaceFromAppearance(
        MeshData& mesh,
        Face face,
        f32 x, f32 y, f32 z,
        u8 light,
        const BlockAppearance* appearance
    );

    // 贪婪网格合并
    static void greedyMeshSection(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6]
    );

    // 简单网格生成 (逐面生成)
    static void simpleMeshSection(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6]
    );

    static BlockModelCache* s_modelCache;
    static bool s_useGreedyMeshing;
    static bool s_lightingEnabled;
};

// ============================================================================
// 区块渲染数据
// ============================================================================

struct ChunkRenderData {
    ChunkId chunkId;
    MeshData solidMesh;       ///< 实心方块网格
    MeshData transparentMesh; ///< 透明方块网格 (水、玻璃等)

    // 渲染状态
    bool needsUpdate = true;
    bool isDirty = false;
    u32 renderVersion = 0;

    // 统计
    u32 vertexCount = 0;
    u32 indexCount = 0;

    void clear() {
        solidMesh.clear();
        transparentMesh.clear();
        vertexCount = 0;
        indexCount = 0;
    }

    void markDirty() {
        isDirty = true;
        needsUpdate = true;
    }

    void markClean() {
        isDirty = false;
        needsUpdate = false;
    }
};

// ============================================================================
// 区块网格缓存
// ============================================================================

class ChunkMeshCache {
public:
    explicit ChunkMeshCache(size_t maxCachedChunks = 256);

    // 获取或创建区块渲染数据
    [[nodiscard]] ChunkRenderData* getOrCreate(const ChunkId& id);
    [[nodiscard]] ChunkRenderData* get(const ChunkId& id);
    [[nodiscard]] const ChunkRenderData* get(const ChunkId& id) const;

    // 标记区块需要更新
    void markDirty(const ChunkId& id);

    // 移除区块
    void remove(const ChunkId& id);

    // 清除所有缓存
    void clear();

    // 获取需要更新的区块数量
    [[nodiscard]] size_t dirtyCount() const { return m_dirtyCount; }

    // 获取缓存大小
    [[nodiscard]] size_t size() const { return m_cache.size(); }

    // 遍历所有缓存项
    template<typename Func>
    void forEach(Func&& func) {
        for (auto& [id, data] : m_cache) {
            func(id, data);
        }
    }

private:
    std::unordered_map<ChunkId, ChunkRenderData> m_cache;
    size_t m_maxCachedChunks;
    size_t m_dirtyCount = 0;
};

// ============================================================================
// 区块网格构建任务
// ============================================================================

struct MeshBuildTask {
    ChunkId chunkId;
    const ChunkData* chunkData = nullptr;
    std::array<const ChunkData*, 6> neighbors = {};

    // 回调函数 (构建完成后调用)
    std::function<void(const ChunkId&, const MeshData& solid, const MeshData& transparent)> onComplete;

    MeshBuildTask() = default;
    explicit MeshBuildTask(ChunkId id) : chunkId(id) {}
};

} // namespace mc
