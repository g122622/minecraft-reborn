#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/entity/PlayerManager.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/network/ProtocolPackets.hpp"
#include "common/world/time/GameTime.hpp"
#include "server/network/TcpSession.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

namespace mr::server {

// 前向声明
class ServerChunkManager;

// ============================================================================
// 服务端玩家数据
// ============================================================================

struct ServerPlayerData {
    PlayerId playerId = 0;
    String username;
    std::shared_ptr<network::PlayerChunkTracker> chunkTracker;
    std::weak_ptr<TcpSession> session;

    // 位置（内部使用 f32，网络边界使用 f64）
    f32 x = 0.0f;
    f32 y = 64.0f;
    f32 z = 0.0f;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    bool onGround = true;
    GameMode gameMode = GameMode::Survival;

    // 传送确认
    u32 pendingTeleportId = 0;
    bool waitingTeleportConfirm = false;

    // 统计
    u64 lastKeepAliveSent = 0;
    u64 lastKeepAliveReceived = 0;
    u32 ping = 0;

    ServerPlayerData() = default;
    explicit ServerPlayerData(PlayerId id, const String& name)
        : playerId(id), username(name) {}
};

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

class ServerWorld {
public:
    ServerWorld();
    explicit ServerWorld(const ServerWorldConfig& config);
    ~ServerWorld();

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

    // 配置
    void setConfig(const ServerWorldConfig& config);
    [[nodiscard]] const ServerWorldConfig& config() const { return m_config; }

    // 区块管理
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const;
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // 玩家管理
    void addPlayer(PlayerId playerId, const String& username, std::shared_ptr<TcpSession> session);
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
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state);
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const;

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

} // namespace mr::server
