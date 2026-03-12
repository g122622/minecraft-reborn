#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/entity/PlayerManager.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/network/ProtocolPackets.hpp"
#include "common/world/time/GameTime.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

namespace mc::server {

// 前向声明
class ServerChunkManager;

// ============================================================================
// 区块缓存条目
// ============================================================================

struct ChunkCacheEntry {
    std::unique_ptr<ChunkData> chunk;
    u64 lastAccessTime = 0;
    bool isGenerated = false;
    bool isModified = false;
    std::unordered_set<PlayerId> subscribers;
};

// ============================================================================
// 服务端世界配置
// ============================================================================

struct ServerWorldConfig {
    i32 viewDistance = 10;              // 视距
    i32 maxChunksPerPlayer = 1024;       // 每玩家最大区块数
    i32 chunkUnloadDelay = 30000;        // 区块卸载延迟（毫秒）
    i32 keepAliveInterval = 15000;        // 心跳间隔（毫秒）
    i32 keepAliveTimeout = 30000;         // 心跳超时（毫秒）
    DimensionId dimension = 0;            // 维度ID
    u64 seed = 12345;                     // 世界种子
};

// ============================================================================
// 服务端世界
// ============================================================================

class ServerWorld : public IWorld {
public:
    ServerWorld();
    explicit ServerWorld(const ServerWorldConfig& config);
    ~ServerWorld() override;

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

    // 配置
    void setConfig(const ServerWorldConfig& config);
    [[nodiscard]] const ServerWorldConfig& config() const { return m_config; }

    // 区块管理
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // 玩家管理
    void addPlayer(PlayerId playerId, const String& username, network::ConnectionPtr connection);
    void removePlayer(PlayerId playerId);
    [[nodiscard]] ServerPlayerData* getPlayer(PlayerId playerId);
    [[nodiscard]] const ServerPlayerData* getPlayer(PlayerId playerId) const;
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;
    [[nodiscard]] size_t playerCount() const;

    // 位置更新（网络协议使用 f64，内部转换为 f32）
    void updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround);
    void confirmTeleport(PlayerId playerId, u32 teleportId);

    // 传送玩家（网络协议使用 f64）
    void teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw = 0.0f, f32 pitch = 0.0f);

    // 玩家模式
    [[nodiscard]] bool setPlayerGameMode(PlayerId playerId, GameMode mode);

    // 区块同步
    void sendInitialChunks(PlayerId playerId);
    void updateChunkSubscription(PlayerId playerId);

    // 方块操作
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    // 发送数据包给玩家
    void sendPacket(PlayerId playerId, const std::vector<u8>& data);
    void broadcastPacket(const std::vector<u8>& data);
    void broadcastPacketExcept(PlayerId excludePlayerId, const std::vector<u8>& data);

    // 更新循环
    void tick();

    // 统计
    [[nodiscard]] size_t chunkCount() const;
    [[nodiscard]] size_t loadedChunkCount() const;

    // 区块坐标转换（使用 f32 坐标）
    static ChunkCoord blockToChunk(f32 blockCoord) {
        return static_cast<ChunkCoord>(std::floor(blockCoord / 16.0f));
    }

    // 获取区块管理器
    [[nodiscard]] ServerChunkManager* chunkManager() { return m_chunkManager.get(); }
    [[nodiscard]] const ServerChunkManager* chunkManager() const { return m_chunkManager.get(); }

    // ========== 时间管理 ==========

    /**
     * @brief 获取游戏时间
     */
    [[nodiscard]] time::GameTime& gameTime() { return m_gameTime; }
    [[nodiscard]] const time::GameTime& gameTime() const { return m_gameTime; }

    /**
     * @brief 设置一天内的时间 (/time set)
     * @param time 时间值 (0-23999)
     */
    void setDayTime(i64 time);

    /**
     * @brief 增加时间 (/time add)
     * @param ticks 要增加的 tick 数
     */
    void addDayTime(i64 ticks);

    /**
     * @brief 设置日光周期是否启用
     * @param enabled true 启用
     */
    void setDaylightCycleEnabled(bool enabled);

    // ========== 其他 IWorld 接口 ==========

    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override;
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except = nullptr) const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except = nullptr) const override;
    [[nodiscard]] DimensionId dimension() const override { return m_config.dimension; }
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return m_gameTime.dayTime(); }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 1; } // Normal

    // ========== 实体管理 ==========

    /**
     * @brief 生成实体到世界
     * @param entity 实体指针（世界获得所有权）
     * @return 实体ID
     */
    EntityId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 被移除的实体指针
     */
    std::unique_ptr<Entity> removeEntity(EntityId id);

    /**
     * @brief 获取实体
     * @param id 实体ID
     * @return 实体指针
     */
    [[nodiscard]] Entity* getEntity(EntityId id);
    [[nodiscard]] const Entity* getEntity(EntityId id) const;

    /**
     * @brief 检查实体是否存在
     * @param id 实体ID
     */
    [[nodiscard]] bool hasEntity(EntityId id) const;

    /**
     * @brief 获取实体数量
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief 获取实体管理器
     */
    [[nodiscard]] EntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const { return m_entityManager; }

    /**
     * @brief 获取实体追踪器
     */
    [[nodiscard]] EntityTracker& entityTracker() { return m_entityTracker; }
    [[nodiscard]] const EntityTracker& entityTracker() const { return m_entityTracker; }

    // ========== 区块生成实体 ==========

    /**
     * @brief 处理区块生成时产生的实体
     *
     * 当区块生成完成后，调用此方法将 WorldGenSpawner 生成的
     * 被动动物实体真正创建到世界中。
     *
     * @param entities 生成的实体数据列表
     * @return 实际创建的实体数量
     */
    i32 spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities);

private:
    // 内部方法
    void sendChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void broadcastBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId);
    void broadcastTimeUpdate();  // 广播时间更新

    // 卸载检查
    void checkChunkUnloading();

private:
    ServerWorldConfig m_config;
    std::unique_ptr<ServerChunkManager> m_chunkManager;
    EntityManager m_entityManager;  // 实体管理器
    EntityTracker m_entityTracker;   // 实体追踪器
    bool m_initialized = false;

    // 玩家存储
    mutable std::mutex m_playerMutex;
    std::unordered_map<PlayerId, ServerPlayerData> m_players;

    // 区块同步管理器
    network::ChunkSyncManager m_chunkSyncManager;

    // 时间
    time::GameTime m_gameTime;
    u64 m_currentTick = 0;
    u64 m_lastTimeSyncTick = 0;  // 上次时间同步的 tick
    u64 m_lastChunkUnloadCheck = 0;
};

} // namespace mc::server
