#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/core/Result.hpp"
#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/world/TerrainGenerator.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/renderer/MeshTypes.hpp"
#include "../../common/network/ChunkSync.hpp"
#include "../renderer/Camera.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <queue>
#include <functional>

namespace mr::client {

/**
 * @brief 客户端区块数据
 */
struct ClientChunk {
    ChunkId chunkId;
    std::unique_ptr<ChunkData> data;
    MeshData solidMesh;       // 实心方块网格
    MeshData transparentMesh;  // 透明方块网格
    bool needsMeshUpdate = true;
    bool isGenerating = false;
    bool isLoaded = false;
};

/**
 * @brief 区块加载请求
 */
struct ChunkLoadRequest {
    ChunkId chunkId;
    world::ChunkLoadPriority priority;
    bool operator<(const ChunkLoadRequest& other) const {
        return static_cast<i32>(priority) > static_cast<i32>(other.priority);
    }
};

/**
 * @brief 客户端世界管理器
 *
 * 管理区块的加载、卸载和渲染
 */
class ClientWorld {
public:
    ClientWorld();
    ~ClientWorld();

    // 禁止拷贝
    ClientWorld(const ClientWorld&) = delete;
    ClientWorld& operator=(const ClientWorld&) = delete;

    /**
     * @brief 初始化世界
     */
    [[nodiscard]] Result<void> initialize(u64 seed = 0);

    /**
     * @brief 清理世界
     */
    void destroy();

    /**
     * @brief 更新世界（每帧调用）
     * @param cameraPosition 相机位置
     * @param renderDistance 渲染距离（区块数）
     */
    void update(const glm::vec3& cameraPosition, i32 renderDistance);

    /**
     * @brief 获取区块
     */
    [[nodiscard]] ClientChunk* getChunk(const ChunkId& id);
    [[nodiscard]] const ClientChunk* getChunk(const ChunkId& id) const;

    /**
     * @brief 获取方块
     */
    [[nodiscard]] BlockState getBlock(i32 x, i32 y, i32 z) const;

    /**
     * @brief 设置方块
     */
    void setBlock(i32 x, i32 y, i32 z, BlockState block);

    /**
     * @brief 获取所有已加载的区块
     */
    void forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func);

    /**
     * @brief 获取需要更新网格的区块
     */
    void forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func);

    /**
     * @brief 获取区块数量
     */
    [[nodiscard]] size_t chunkCount() const { return m_chunks.size(); }

    /**
     * @brief 获取渲染距离内的区块坐标
     */
    void getChunksInRange(const glm::vec3& position, i32 range,
                          std::vector<ChunkId>& outChunks) const;

    /**
     * @brief 强制加载区块
     */
    void loadChunk(const ChunkId& id);

    /**
     * @brief 强制卸载区块
     */
    void unloadChunk(const ChunkId& id);

    /**
     * @brief 重新生成区块网格
     */
    void rebuildChunkMesh(const ChunkId& id);

    /**
     * @brief 标记区块需要网格更新
     */
    void markChunkDirty(const ChunkId& id);

    /**
     * @brief 设置渲染距离
     */
    void setRenderDistance(i32 distance) { m_renderDistance = distance; }
    [[nodiscard]] i32 renderDistance() const { return m_renderDistance; }

    /**
     * @brief 获取地形生成器
     */
    [[nodiscard]] ITerrainGenerator* terrainGenerator() { return m_terrainGenerator.get(); }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 seed() const { return m_seed; }

    /**
     * @brief 设置网络模式
     *
     * 网络模式下，区块从服务端接收，不使用本地生成器
     */
    void setNetworkMode(bool networkMode) { m_networkMode = networkMode; }
    [[nodiscard]] bool isNetworkMode() const { return m_networkMode; }

    /**
     * @brief 接收服务端区块数据
     */
    void onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data);

    /**
     * @brief 卸载区块（服务端通知）
     */
    void onChunkUnload(ChunkCoord x, ChunkCoord z);

private:
    // 区块加载/卸载
    void loadChunksInRange(const glm::vec3& position, i32 range);
    void unloadChunksOutOfRange(const glm::vec3& position, i32 range);
    void processLoadQueue();

    // 区块生成
    void generateChunk(ClientChunk& chunk);
    void rebuildMesh(ClientChunk& chunk);

    // 获取相邻区块
    void getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]);

    // 计算区块优先级
    world::ChunkLoadPriority calculatePriority(const ChunkId& id, const glm::vec3& cameraPos) const;

private:
    std::unordered_map<ChunkId, std::unique_ptr<ClientChunk>> m_chunks;
    std::unique_ptr<ITerrainGenerator> m_terrainGenerator;

    // 加载队列
    std::priority_queue<ChunkLoadRequest> m_loadQueue;
    std::unordered_set<ChunkId> m_queuedChunks;

    // 配置
    i32 m_renderDistance = 12;
    i32 m_maxChunksPerFrame = 4;  // 每帧最多加载的区块数
    u64 m_seed = 0;
    bool m_networkMode = false;  // 网络模式标志

    // 统计
    u32 m_chunksLoaded = 0;
    u32 m_chunksUnloaded = 0;
};

} // namespace mr::client
