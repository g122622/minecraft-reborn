#include "ServerChunkManager.hpp"
#include "../../common/world/WorldConstants.hpp"
#include <chrono>

namespace mr::server {

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(world)
    , m_generator(std::move(generator))
    , m_workerPool(-1)  // 自动检测线程数
{
    // 设置票据管理器回调
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        // 级别变化时创建或更新 ChunkHolder
        if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
            getOrCreateHolder(x, z);
        }
    });
}

ServerChunkManager::~ServerChunkManager()
{
    shutdown();
}

// ============================================================================
// 生命周期
// ============================================================================

Result<void> ServerChunkManager::initialize()
{
    // 启动 Worker 线程池
    startWorkers();

    return {};
}

void ServerChunkManager::shutdown()
{
    stopWorkers();

    // 清理区块持有者
    std::lock_guard<std::mutex> lock(m_holdersMutex);
    m_holders.clear();

    // 清理区块缓存
    std::lock_guard<std::mutex> chunksLock(m_chunksMutex);
    m_chunks.clear();
}

// ============================================================================
// Worker 管理
// ============================================================================

void ServerChunkManager::startWorkers(i32 count)
{
    // 设置生成器函数
    m_workerPool.setGenerator([this](ChunkPrimer& chunk, const ChunkStatus& targetStatus) {
        // 创建 WorldGenRegion（简化版）
        std::array<IChunk*, 9> chunks{};
        chunks[4] = &chunk;  // 中心区块

        // 获取邻居区块（如果可用）
        getNeighborChunks(chunk.x(), chunk.z(), chunks);

        WorldGenRegion region(chunk.x(), chunk.z(), chunks);

        // 按阶段生成
        const auto& allStatuses = ChunkStatus::getAll();
        for (const auto& status : allStatuses) {
            if (status.ordinal() > targetStatus.ordinal()) {
                break;
            }

            if (!chunk.hasCompletedStatus(status)) {
                // 检查邻居依赖
                if (status.taskRange() > 0 && !checkNeighborsReady(chunk.x(), chunk.z(), status)) {
                    // 等待邻居（简化处理：继续尝试下一阶段）
                    continue;
                }

                // 执行生成
                if (status == ChunkStatus::BIOMES) {
                    m_generator->generateBiomes(region, chunk);
                } else if (status == ChunkStatus::NOISE) {
                    m_generator->generateNoise(region, chunk);
                } else if (status == ChunkStatus::SURFACE) {
                    m_generator->buildSurface(region, chunk);
                } else if (status == ChunkStatus::CARVERS) {
                    m_generator->applyCarvers(region, chunk, false);
                } else if (status == ChunkStatus::FEATURES) {
                    m_generator->placeFeatures(region, chunk);
                } else if (status == ChunkStatus::HEIGHTMAPS) {
                    chunk.updateAllHeightmaps();
                }

                chunk.setChunkStatus(status);
            }
        }
    });

    m_workerPool.start();
}

void ServerChunkManager::stopWorkers()
{
    m_workerPool.shutdown();
}

// ============================================================================
// 区块访问（同步）
// ============================================================================

ChunkData* ServerChunkManager::getChunk(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ChunkData* ServerChunkManager::getChunk(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ServerChunkManager::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.find(key) != m_chunks.end();
}

ChunkData* ServerChunkManager::getChunkSync(ChunkCoord x, ChunkCoord z)
{
    // 先检查缓存
    if (ChunkData* cached = getChunk(x, z)) {
        return cached;
    }

    // 获取持有者
    ChunkHolder* holder = getOrCreateHolder(x, z);
    if (!holder) {
        return nullptr;
    }

    // 检查是否已完成
    if (ChunkData* data = holder->getChunkData()) {
        return data;
    }

    // 同步生成到 FULL 状态，确保与异步路径结果一致
    executeGenerationTask(*holder, ChunkStatus::FULL);

    // 返回缓存中的结果
    return getChunk(x, z);
}

void ServerChunkManager::unloadChunk(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }

    {
        std::lock_guard<std::mutex> lock(m_holdersMutex);
        m_holders.erase(key);
    }
}

// ============================================================================
// 区块访问（异步）
// ============================================================================

std::future<ChunkData*> ServerChunkManager::getChunkAsync(ChunkCoord x, ChunkCoord z,
                                                           const ChunkStatus* targetStatus)
{
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();

    // 获取或创建持有者
    ChunkHolder* holder = getOrCreateHolder(x, z);
    if (!holder) {
        promise->set_value(nullptr);
        return future;
    }

    // 如果已完成
    if (ChunkData* data = holder->getChunkData()) {
        promise->set_value(data);
        return future;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatus::FULL;

    m_workerPool.submitGenerate(x, z, target,
        [this, promise, x, z](bool success, ChunkPrimer* primer) {
            if (success && primer) {
                // 完成生成
                auto data = primer->toChunkData();

                // 存入缓存
                {
                    std::lock_guard<std::mutex> lock(m_chunksMutex);
                    m_chunks[posToKey(x, z)] = std::make_unique<ChunkData>(std::move(*data));
                }

                // 获取缓存中的指针
                promise->set_value(m_chunks[posToKey(x, z)].get());
            } else {
                promise->set_value(nullptr);
            }
        },
        holder->getLevel()  // 使用加载级别作为优先级
    );

    return future;
}

void ServerChunkManager::getChunkAsync(ChunkCoord x, ChunkCoord z, ChunkCallback callback,
                                        const ChunkStatus* targetStatus)
{
    // 获取或创建持有者
    ChunkHolder* holder = getOrCreateHolder(x, z);
    if (!holder) {
        if (callback) callback(false, nullptr);
        return;
    }

    // 如果已完成
    if (ChunkData* data = holder->getChunkData()) {
        if (callback) callback(true, data);
        return;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatus::FULL;

    m_workerPool.submitGenerate(x, z, target,
        [this, callback, x, z](bool success, ChunkPrimer* primer) {
            if (success && primer) {
                // 完成生成
                auto data = primer->toChunkData();

                // 存入缓存
                ChunkData* result = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_chunksMutex);
                    m_chunks[posToKey(x, z)] = std::make_unique<ChunkData>(std::move(*data));
                    result = m_chunks[posToKey(x, z)].get();
                }

                if (callback) callback(true, result);
            } else {
                if (callback) callback(false, nullptr);
            }
        },
        holder->getLevel()  // 使用加载级别作为优先级
    );
}

// ============================================================================
// 区块持有者
// ============================================================================

ChunkHolder* ServerChunkManager::getOrCreateHolder(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_holdersMutex);

    auto it = m_holders.find(key);
    if (it != m_holders.end()) {
        return it->second.get();
    }

    auto holder = std::make_unique<ChunkHolder>(x, z);

    // 设置回调
    holder->setLevelChangeCallback([this](ChunkHolder& h) {
        // 级别变化时可能需要调度生成
        if (h.shouldLoad()) {
            scheduleGeneration(h, ChunkStatus::FULL);
        }
    });

    auto* ptr = holder.get();
    m_holders[key] = std::move(holder);
    return ptr;
}

ChunkHolder* ServerChunkManager::getHolder(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_holdersMutex);
    auto it = m_holders.find(key);
    return it != m_holders.end() ? it->second.get() : nullptr;
}

const ChunkHolder* ServerChunkManager::getHolder(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_holdersMutex);
    auto it = m_holders.find(key);
    return it != m_holders.end() ? it->second.get() : nullptr;
}

// ============================================================================
// 票据管理
// ============================================================================

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(x / world::CHUNK_WIDTH));
    const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(z / world::CHUNK_WIDTH));

    m_ticketManager.updatePlayerPosition(player, chunkX, chunkZ);
}

void ServerChunkManager::removePlayer(PlayerId player)
{
    m_ticketManager.removePlayer(player);
}

void ServerChunkManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    m_ticketManager.forceChunk(x, z, force);
}

void ServerChunkManager::setViewDistance(i32 distance)
{
    m_ticketManager.setViewDistance(distance);
}

// ============================================================================
// 主循环
// ============================================================================

void ServerChunkManager::tick()
{
    ++m_currentTick;

    // 处理票据更新
    m_ticketManager.tick();

    // 处理完成的异步任务
    processCompletedTasks();

    // 检查区块卸载
    if (m_currentTick - m_lastUnloadCheck >= UNLOAD_CHECK_INTERVAL) {
        checkChunkUnloading();
        m_lastUnloadCheck = m_currentTick;
    }
}

// ============================================================================
// 内部方法
// ============================================================================

void ServerChunkManager::scheduleGeneration(ChunkHolder& holder, const ChunkStatus& targetStatus)
{
    // 如果已经在生成中，不需要重新调度
    if (holder.getGeneratingChunk() && holder.hasCompletedStatus(targetStatus)) {
        return;
    }

    // 提交异步任务
    m_workerPool.submitGenerate(
        holder.x(),
        holder.z(),
        targetStatus,
        [&holder](bool success, ChunkPrimer* primer) {
            if (success && primer) {
                holder.setStatus(ChunkStatus::FULL);
            }
        },
        holder.getLevel()
    );
}

void ServerChunkManager::executeGenerationTask(ChunkHolder& holder, const ChunkStatus& status)
{
    // 创建区块生成器
    ChunkPrimer* primer = holder.createGeneratingChunk();
    if (!primer) {
        return;
    }

    // 创建 WorldGenRegion（简化版）
    std::array<IChunk*, 9> chunks{};
    chunks[4] = primer;

    // 获取邻居区块
    getNeighborChunks(holder.x(), holder.z(), chunks);

    WorldGenRegion region(holder.x(), holder.z(), chunks);

    // 按阶段生成
    const auto& allStatuses = ChunkStatus::getAll();
    for (const auto& s : allStatuses) {
        if (s.ordinal() > status.ordinal()) {
            break;
        }

        if (!primer->hasCompletedStatus(s)) {
            // 执行生成
            if (s == ChunkStatus::BIOMES) {
                m_generator->generateBiomes(region, *primer);
            } else if (s == ChunkStatus::NOISE) {
                m_generator->generateNoise(region, *primer);
            } else if (s == ChunkStatus::SURFACE) {
                m_generator->buildSurface(region, *primer);
            } else if (s == ChunkStatus::CARVERS) {
                m_generator->applyCarvers(region, *primer, false);
            } else if (s == ChunkStatus::FEATURES) {
                m_generator->placeFeatures(region, *primer);
            } else if (s == ChunkStatus::HEIGHTMAPS) {
                primer->updateAllHeightmaps();
            }

            primer->setChunkStatus(s);
        }
    }

    // 完成生成并存入缓存
    auto data = holder.completeGeneration();
    if (data) {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks[posToKey(holder.x(), holder.z())] = std::move(data);
    }
}

bool ServerChunkManager::checkNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& status) const
{
    // FEATURES 阶段需要邻居
    if (status.taskRange() == 0) {
        return true;
    }

    // 检查 8 个邻居
    const i32 range = status.taskRange();
    for (i32 dz = -range; dz <= range; ++dz) {
        for (i32 dx = -range; dx <= range; ++dx) {
            if (dx == 0 && dz == 0) continue;

            const ChunkHolder* neighbor = getHolder(x + dx, z + dz);
            if (!neighbor || !neighbor->hasCompletedStatus(status)) {
                return false;
            }
        }
    }

    return true;
}

void ServerChunkManager::getNeighborChunks(ChunkCoord x, ChunkCoord z, std::array<IChunk*, 9>& neighbors)
{
    // 索引顺序：0=NW, 1=N, 2=NE, 3=W, 4=中心, 5=E, 6=SW, 7=S, 8=SE
    // 偏移量（dx, dz）
    static constexpr i32 offsets[9][2] = {
        {-1, -1}, {0, -1}, {1, -1},  // NW, N, NE
        {-1, 0},  {0, 0},  {1, 0},   // W, 中心, E
        {-1, 1},  {0, 1},  {1, 1}    // SW, S, SE
    };

    for (i32 i = 0; i < 9; ++i) {
        // 跳过中心位置（调用者已设置）
        if (i == 4) {
            continue;
        }

        ChunkHolder* holder = getHolder(x + offsets[i][0], z + offsets[i][1]);
        if (holder) {
            if (ChunkData* data = holder->getChunkData()) {
                neighbors[i] = nullptr;  // TODO: 需要适配 ChunkData 到 IChunk
            } else if (ChunkPrimer* primer = holder->getGeneratingChunk()) {
                neighbors[i] = primer;
            } else {
                neighbors[i] = nullptr;
            }
        } else {
            neighbors[i] = nullptr;
        }
    }
}

void ServerChunkManager::processCompletedTasks()
{
    // Worker 线程池会自动处理完成的任务
    // 这里可以添加额外的后处理逻辑
}

void ServerChunkManager::checkChunkUnloading()
{
    // 检查所有持有者
    std::vector<u64> toUnload;

    {
        std::lock_guard<std::mutex> lock(m_holdersMutex);
        for (const auto& [key, holder] : m_holders) {
            // 没有票据且没有追踪玩家
            if (!holder->shouldLoad() && !holder->hasTrackingPlayers()) {
                toUnload.push_back(key);
            }
        }
    }

    // 卸载区块
    for (u64 key : toUnload) {
        unloadChunk(
            static_cast<ChunkCoord>(static_cast<i64>(key >> 32)),
            static_cast<ChunkCoord>(static_cast<i64>(key & 0xFFFFFFFF))
        );
    }
}

// ============================================================================
// 统计
// ============================================================================

size_t ServerChunkManager::loadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

size_t ServerChunkManager::holderCount() const
{
    std::lock_guard<std::mutex> lock(m_holdersMutex);
    return m_holders.size();
}

size_t ServerChunkManager::pendingTaskCount() const
{
    return m_workerPool.pendingTaskCount();
}

} // namespace mr::server
