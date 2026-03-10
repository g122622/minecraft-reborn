#include "ServerChunkManager.hpp"
#include "../../common/world/WorldConstants.hpp"
#include <chrono>
#include <spdlog/spdlog.h>

namespace mr::server {

namespace {

class ChunkDataChunkAdapter : public IChunk {
public:
    explicit ChunkDataChunkAdapter(std::shared_ptr<ChunkData> chunk)
        : m_chunk(std::move(chunk))
        , m_status(m_chunk && m_chunk->isFullyGenerated() ? ChunkLoadStatus::Generated : ChunkLoadStatus::Generating) {}

    [[nodiscard]] ChunkCoord x() const override { return m_chunk ? m_chunk->x() : 0; }
    [[nodiscard]] ChunkCoord z() const override { return m_chunk ? m_chunk->z() : 0; }
    [[nodiscard]] ChunkPos pos() const override { return m_chunk ? m_chunk->pos() : ChunkPos(0, 0); }

    [[nodiscard]] const BlockState* getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlock(x, y, z) : nullptr;
    }

    void setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        if (m_chunk) {
            m_chunk->setBlock(x, y, z, state);
        }
    }

    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBlockStateId(x, y, z) : 0;
    }

    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId) override {
        if (m_chunk) {
            m_chunk->setBlockStateId(x, y, z, stateId);
        }
    }

    [[nodiscard]] ChunkSection* getSection(i32 index) override {
        return m_chunk ? m_chunk->getSection(index) : nullptr;
    }

    [[nodiscard]] const ChunkSection* getSection(i32 index) const override {
        return m_chunk ? m_chunk->getSection(index) : nullptr;
    }

    [[nodiscard]] bool hasSection(i32 index) const override {
        return m_chunk ? m_chunk->hasSection(index) : false;
    }

    ChunkSection* createSection(i32 index) override {
        return m_chunk ? m_chunk->createSection(index) : nullptr;
    }

    [[nodiscard]] BiomeId getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const override {
        return m_chunk ? m_chunk->getBiomeAtBlock(x, y, z) : Biomes::Plains;
    }

    [[nodiscard]] BlockCoord getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const override {
        (void)type;
        return m_chunk ? m_chunk->getHighestBlock(x, z) : 0;
    }

    void updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state) override {
        (void)type;
        (void)y;
        (void)state;
        if (m_chunk) {
            m_chunk->updateHeightMap(x, z);
        }
    }

    [[nodiscard]] ChunkLoadStatus getStatus() const override {
        return m_status;
    }

    void setStatus(ChunkLoadStatus status) override {
        m_status = status;
    }

    [[nodiscard]] bool isModified() const override {
        return m_chunk ? m_chunk->isDirty() : false;
    }

    void setModified(bool modified) override {
        if (m_chunk) {
            m_chunk->setDirty(modified);
        }
    }

private:
    std::shared_ptr<ChunkData> m_chunk;
    ChunkLoadStatus m_status = ChunkLoadStatus::Generated;
};

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(&world)
    , m_generator(std::move(generator))
    , m_workerPool(-1)  // 自动检测线程数
{
    // 设置票据管理器回调
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 /*oldLevel*/, i32 newLevel) {
        // 级别变化时创建或更新 ChunkHolder
        if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
            (void)getOrCreateHolder(x, z);
        }
    });
}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_world(nullptr)
    , m_generator(std::move(generator))
    , m_workerPool(-1)  // 自动检测线程数
{
    // 设置票据管理器回调
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 /*oldLevel*/, i32 newLevel) {
        // 级别变化时创建或更新 ChunkHolder
        if (newLevel <= world::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
            (void)getOrCreateHolder(x, z);
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
    (void)count;
    // 设置生成器函数
    m_workerPool.setGenerator([this](ChunkPrimer& chunk, const ChunkStatus& targetStatus) {
        // 创建 WorldGenRegion（简化版）
        std::array<IChunk*, 9> chunks{};
        std::array<std::unique_ptr<IChunk>, 9> neighborAdapters{};
        chunks[4] = &chunk;  // 中心区块

        // 获取邻居区块（如果可用）
        getNeighborChunks(chunk.x(), chunk.z(), chunks, neighborAdapters);

        WorldGenRegion region(chunk.x(), chunk.z(), chunks);

        // 按阶段生成
        const auto& allStatuses = ChunkStatus::getAll();
        for (const auto& status : allStatuses) {
            if (status.ordinal() > targetStatus.ordinal()) {
                break;
            }

            if (!chunk.hasCompletedStatus(status)) {
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
                    // 异步路径：暂时跳过邻居检查
                    // TODO: 实现完整的两阶段生成系统
                    // 第一阶段：所有区块生成到 CARVERS
                    // 第二阶段：批量执行 FEATURES
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
    return getChunkShared(x, z).get();
}

const ChunkData* ServerChunkManager::getChunk(ChunkCoord x, ChunkCoord z) const
{
    return getChunkShared(x, z).get();
}

std::shared_ptr<ChunkData> ServerChunkManager::getChunkShared(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return {};
}

std::shared_ptr<const ChunkData> ServerChunkManager::getChunkShared(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    if (it != m_chunks.end()) {
        return it->second;
    }
    return {};
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

    std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);

    // 进入同步生成临界区后再次检查缓存，避免重复生成
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

    if (auto cached = getChunkShared(x, z)) {
        promise->set_value(cached.get());
        return future;
    }

    // 获取或创建持有者
    ChunkHolder* holder = getOrCreateHolder(x, z);
    if (!holder) {
        promise->set_value(nullptr);
        return future;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatus::FULL;

    m_workerPool.submitGenerate(x, z, target,
        [this, promise, x, z](bool success, ChunkPrimer* primer) {
            if (success && primer) {
                // 完成生成
                auto data = primer->toChunkData();
                if (!data) {
                    promise->set_value(nullptr);
                    return;
                }

                promise->set_value(storeGeneratedChunk(x, z, std::move(data)));
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
    if (auto cached = getChunkShared(x, z)) {
        if (callback) callback(true, cached.get());
        return;
    }

    // 获取或创建持有者
    ChunkHolder* holder = getOrCreateHolder(x, z);
    if (!holder) {
        if (callback) callback(false, nullptr);
        return;
    }

    // 调度异步生成
    const ChunkStatus& target = targetStatus ? *targetStatus : ChunkStatus::FULL;

    m_workerPool.submitGenerate(x, z, target,
        [this, callback, x, z](bool success, ChunkPrimer* primer) {
            if (success && primer) {
                // 完成生成
                auto data = primer->toChunkData();
                if (!data) {
                    if (callback) callback(false, nullptr);
                    return;
                }

                (void)storeGeneratedChunk(x, z, std::move(data));
                const auto pinned = getChunkShared(x, z);
                if (callback) callback(pinned != nullptr, pinned.get());
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
    // 如果已经达到目标状态、已有缓存结果或已有正在使用的 primer，则不重复调度
    if (holder.hasCompletedStatus(targetStatus) ||
        getChunk(holder.x(), holder.z()) != nullptr ||
        holder.hasGeneratingChunk()) {
        return;
    }

    // 提交异步任务
    m_workerPool.submitGenerate(
        holder.x(),
        holder.z(),
        targetStatus,
        [this, x = holder.x(), z = holder.z()](bool success, ChunkPrimer* primer) {
            if (!success || !primer) {
                return;
            }

            auto data = primer->toChunkData();
            if (!data) {
                return;
            }

            (void)storeGeneratedChunk(x, z, std::move(data));
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
    std::array<std::unique_ptr<IChunk>, 9> neighborAdapters{};
    chunks[4] = primer;

    // 获取邻居区块
    getNeighborChunks(holder.x(), holder.z(), chunks, neighborAdapters);

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
                // 同步路径：不检查邻居依赖，直接执行
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
        (void)storeGeneratedChunk(holder.x(), holder.z(), std::move(data));
    }
}

bool ServerChunkManager::checkNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStatus& status) const
{
    // taskRange 为 0 表示不需要邻居
    if (status.taskRange() == 0) {
        return true;
    }

    // 需要邻居完成的是前一阶段（parent），而不是当前阶段
    // 这避免了循环等待死锁：FEATURES 需要邻居完成 CARVERS
    // 参考 MC ChunkStatus.outputParent 的设计
    const ChunkStatus* requiredStatus = status.parent();
    if (!requiredStatus) {
        requiredStatus = &status;
    }

    // 检查 8 个邻居
    const i32 range = status.taskRange();
    for (i32 dz = -range; dz <= range; ++dz) {
        for (i32 dx = -range; dx <= range; ++dx) {
            if (dx == 0 && dz == 0) continue;

            const ChunkHolder* neighbor = getHolder(x + dx, z + dz);
            if (!neighbor || !neighbor->hasCompletedStatus(*requiredStatus)) {
                return false;
            }
        }
    }

    return true;
}

void ServerChunkManager::getNeighborChunks(
    ChunkCoord x,
    ChunkCoord z,
    std::array<IChunk*, 9>& neighbors,
    std::array<std::unique_ptr<IChunk>, 9>& neighborAdapters)
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
            if (auto data = getChunkShared(x + offsets[i][0], z + offsets[i][1])) {
                neighborAdapters[i] = std::make_unique<ChunkDataChunkAdapter>(std::move(data));
                neighbors[i] = neighborAdapters[i].get();
            } else {
                neighborAdapters[i].reset();
                neighbors[i] = nullptr;
            }
        } else {
            neighborAdapters[i].reset();
            neighbors[i] = nullptr;
        }
    }
}

ChunkData* ServerChunkManager::storeGeneratedChunk(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
{
    if (!data) {
        return nullptr;
    }

    std::shared_ptr<ChunkData> sharedData(std::move(data));

    ChunkData* stored = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        auto& slot = m_chunks[posToKey(x, z)];
        slot = std::move(sharedData);
        stored = slot.get();
    }

    if (ChunkHolder* holder = getHolder(x, z)) {
        holder->setStatus(ChunkStatus::FULL);
    }

    return stored;
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
            if (!holder->shouldLoad() &&
                !holder->hasTrackingPlayers() &&
                !holder->hasGeneratingChunk()) {
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
