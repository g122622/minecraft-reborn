#pragma once

#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/world/chunk/ChunkHolder.hpp"
#include "../../common/world/chunk/ChunkLoadTicketManager.hpp"
#include "../../common/world/gen/IChunkGenerator.hpp"
#include "../../common/world/gen/NoiseChunkGenerator.hpp"
#include "ChunkWorkerPool.hpp"
#include <unordered_map>
#include <memory>
#include <future>
#include <functional>

namespace mr::server {

// 前向声明
class ServerWorld;

/**
 * @brief 服务端区块管理器
 *
 * 参考 MC ServerChunkProvider，协调区块加载、生成、卸载。
 * 使用 Worker 线程池异步生成区块，不阻塞主循环。
 *
 * 使用方法：
 * @code
 * ServerChunkManager manager(world, std::move(generator));
 * manager.initialize();
 * manager.startWorkers(4);
 *
 * // 获取区块（同步）
 * ChunkData* chunk = manager.getChunk(x, z);
 *
 * // 获取区块（异步）
 * auto future = manager.getChunkAsync(x, z, &ChunkStatus::FULL);
 *
 * // 主循环
 * manager.tick();
 *
 * // 关闭
 * manager.shutdown();
 * @endcode
 */
class ServerChunkManager {
public:
    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建区块管理器
     * @param world 服务端世界引用
     * @param generator 区块生成器
     */
    ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator);

    ~ServerChunkManager();

    // 禁止拷贝
    ServerChunkManager(const ServerChunkManager&) = delete;
    ServerChunkManager& operator=(const ServerChunkManager&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    /**
     * @brief 初始化区块管理器
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 关闭区块管理器
     */
    void shutdown();

    // ============================================================================
    // Worker 管理
    // ============================================================================

    /**
     * @brief 启动 Worker 线程
     * @param count 线程数量（-1 表示自动）
     */
    void startWorkers(i32 count = -1);

    /**
     * @brief 停止 Worker 线程
     */
    void stopWorkers();

    /**
     * @brief 检查 Worker 是否运行
     */
    [[nodiscard]] bool workersRunning() const { return m_workerPool.isRunning(); }

    // ============================================================================
    // 区块访问（同步）
    // ============================================================================

    /**
     * @brief 获取区块（同步，阻塞直到完成）
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 区块数据指针，如果不存在返回 nullptr
     */
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块（const 版本）
     */
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查区块是否存在
     */
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取或生成区块（同步，阻塞直到完成）
     *
     * 如果区块不存在，会阻塞生成到 FULL 状态。
     * 注意：此方法会阻塞调用线程，建议仅用于必要场景。
     * 优先使用 getChunkAsync() 进行异步生成。
     */
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 卸载区块
     */
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // ============================================================================
    // 区块访问（异步）
    // ============================================================================

    /**
     * @brief 区块生成回调类型
     * @param success 是否成功
     * @param chunk 生成的区块数据（如果成功）
     */
    using ChunkCallback = std::function<void(bool success, ChunkData* chunk)>;

    /**
     * @brief 获取区块 Future
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 目标状态
     * @return 区块 Future
     */
    [[nodiscard]] std::future<ChunkData*> getChunkAsync(ChunkCoord x, ChunkCoord z,
                                                         const ChunkStatus* targetStatus = &ChunkStatus::FULL);

    /**
     * @brief 异步获取区块（回调版本）
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param callback 完成回调
     * @param targetStatus 目标状态
     */
    void getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback,
                       const ChunkStatus* targetStatus = &ChunkStatus::FULL);

    // ============================================================================
    // 区块持有者
    // ============================================================================

    /**
     * @brief 获取或创建区块持有者
     */
    [[nodiscard]] ChunkHolder* getOrCreateHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块持有者
     */
    [[nodiscard]] ChunkHolder* getHolder(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkHolder* getHolder(ChunkCoord x, ChunkCoord z) const;

    // ============================================================================
    // 票据管理
    // ============================================================================

    /**
     * @brief 更新玩家位置
     *
     * 会自动更新区块加载票据。
     */
    void updatePlayerPosition(PlayerId player, f64 x, f64 z);

    /**
     * @brief 移除玩家
     */
    void removePlayer(PlayerId player);

    /**
     * @brief 强制加载区块
     */
    void forceChunk(ChunkCoord x, ChunkCoord z, bool force);

    /**
     * @brief 设置视距
     */
    void setViewDistance(i32 distance);

    [[nodiscard]] i32 viewDistance() const { return m_ticketManager.viewDistance(); }

    // ============================================================================
    // 主循环
    // ============================================================================

    /**
     * @brief 每刻调用
     *
     * 处理区块加载/卸载、完成异步任务等。
     */
    void tick();

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取已加载区块数量
     */
    [[nodiscard]] size_t loadedChunkCount() const;

    /**
     * @brief 获取区块持有者数量
     */
    [[nodiscard]] size_t holderCount() const;

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取生成器
     */
    [[nodiscard]] IChunkGenerator* generator() { return m_generator.get(); }
    [[nodiscard]] const IChunkGenerator* generator() const { return m_generator.get(); }

private:
    // ============================================================================
    // 内部方法
    // ============================================================================

    /**
     * @brief 调度区块生成
     */
    void scheduleGeneration(ChunkHolder& holder, const ChunkStatus& targetStatus);

    /**
     * @brief 执行生成任务
     */
    void executeGenerationTask(ChunkHolder& holder, const ChunkStatus& status);

    /**
     * @brief 检查邻居区块状态
     * @return true 如果所有需要的邻居都已完成
     */
    [[nodiscard]] bool checkNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& status) const;

    /**
     * @brief 获取邻居区块
     * @param neighbors 输出数组（中心 + 8 个邻居 = 9 个区块）
     * 索引顺序：0=NW, 1=N, 2=NE, 3=W, 4=中心, 5=E, 6=SW, 7=S, 8=SE
     */
    void getNeighborChunks(ChunkCoord x, ChunkCoord z, std::array<IChunk*, 9>& neighbors);

    /**
     * @brief 处理完成的异步任务
     */
    void processCompletedTasks();

    /**
     * @brief 检查区块卸载
     */
    void checkChunkUnloading();

    /**
     * @brief 区块坐标转键
     */
    [[nodiscard]] static u64 posToKey(ChunkCoord x, ChunkCoord z) {
        return ChunkId(x, z, 0).toId();
    }

    // ============================================================================
    // 成员变量
    // ============================================================================

    ServerWorld& m_world;
    std::unique_ptr<IChunkGenerator> m_generator;

    // 区块持有者
    std::unordered_map<u64, std::unique_ptr<ChunkHolder>> m_holders;
    mutable std::mutex m_holdersMutex;

    // 已完成的区块数据缓存
    std::unordered_map<u64, std::unique_ptr<ChunkData>> m_chunks;
    mutable std::mutex m_chunksMutex;

    // 票据管理器
    world::ChunkLoadTicketManager m_ticketManager;

    // Worker 线程池
    ChunkWorkerPool m_workerPool;

    // 统计
    u64 m_currentTick = 0;
    u64 m_lastUnloadCheck = 0;
    static constexpr u32 UNLOAD_CHECK_INTERVAL = 6000; // 5 分钟（20 tick/秒）
};

} // namespace mr::server
