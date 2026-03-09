#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/InventoryPackets.hpp"
#include "common/network/LocalConnection.hpp"
#include "common/network/ProtocolPackets.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/entity/Player.hpp"
#include "server/world/ServerChunkManager.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>

namespace mr::server {

/**
 * @brief 内置服务端配置
 */
struct IntegratedServerConfig {
    String worldName = "singleplayer";
    i64 seed = 0;
    GameMode defaultGameMode = GameMode::Survival;
    i32 viewDistance = 6;
    i32 tickRate = 20;  // TPS
};

/**
 * @brief 待发送区块数据
 *
 * 用于从 Worker 线程传递序列化后的区块数据到主线程。
 */
struct PendingChunkSend {
    ChunkCoord x;
    ChunkCoord z;
    std::vector<u8> serializedData;
};

/**
 * @brief 内置服务端
 *
 * 参考 Minecraft Java 版的 IntegratedServer：
 * - 运行在独立线程
 * - 20 TPS 固定时间步进
 * - 通过 LocalConnection 与客户端通信
 */
class IntegratedServer {
public:
    IntegratedServer();
    ~IntegratedServer();

    // 禁止拷贝
    IntegratedServer(const IntegratedServer&) = delete;
    IntegratedServer& operator=(const IntegratedServer&) = delete;

    /**
     * @brief 初始化并启动服务端线程
     */
    [[nodiscard]] Result<void> initialize(const IntegratedServerConfig& config);

    /**
     * @brief 停止服务端线程
     */
    void stop();

    /**
     * @brief 获取客户端连接端点
     *
     * 客户端通过此端点发送/接收数据
     */
    network::LocalEndpoint* getClientEndpoint();

    /**
     * @brief 是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const IntegratedServerConfig& config() const noexcept { return m_config; }

    /**
     * @brief 获取当前 tick 数
     */
    [[nodiscard]] u64 tickCount() const noexcept { return m_tickCount; }

private:
    void mainLoop();
    void tick();
    void shutdown();

    // 区块管理
    void requestChunkAsync(ChunkCoord x, ChunkCoord z);
    void processPendingChunkSends();
    void sendChunkToClient(ChunkCoord x, ChunkCoord z);

    // 票据系统
    void handlePlayerChunkMove(ChunkCoord newChunkX, ChunkCoord newChunkZ);
    void onChunkLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    // 网络事件处理
    void onPacketReceived(const u8* data, size_t size);

    // 数据包处理
    void handleLoginRequest(const u8* data, size_t size);
    void handlePlayerMove(const u8* data, size_t size);
    void handleBlockInteraction(const u8* data, size_t size);
    void handleBlockPlacement(const u8* data, size_t size);
    void handleHotbarSelect(const u8* data, size_t size);
    void handleTeleportConfirm(const u8* data, size_t size);
    void handleKeepAlive(const u8* data, size_t size);
    void handleChatMessage(const u8* data, size_t size);

    // 发送数据包
    void sendLoginResponse(bool success, PlayerId playerId, const String& username, const String& message);
    void sendKeepAlive(u64 timestamp);
    void sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void sendBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId);
    void sendPlayerInventory();
    void sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    void sendUnloadChunk(ChunkCoord x, ChunkCoord z);
    void sendToClient(const u8* data, size_t size);

    IntegratedServerConfig m_config;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    // 区块管理器（异步生成）
    std::unique_ptr<ServerChunkManager> m_chunkManager;

    // 跨线程发送队列
    std::vector<PendingChunkSend> m_pendingSends;
    std::mutex m_pendingSendsMutex;

    // 服务端线程
    std::unique_ptr<std::thread> m_serverThread;

    // 本地连接
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
    network::LocalEndpoint* m_serverEndpoint = nullptr;

    // 玩家信息
    struct ClientInfo {
        PlayerId playerId = 0;
        String username;
        bool loggedIn = false;
        f64 x = 0.0;
        f64 y = 64.0;
        f64 z = 0.0;
        f32 yaw = 0.0f;
        f32 pitch = 0.0f;
        u32 pendingTeleportId = 0;
        bool waitingTeleportConfirm = false;
        GameMode gameMode = GameMode::Survival;
        PlayerInventory inventory;

        // 已加载的区块
        std::unordered_set<ChunkId> loadedChunks;
    };
    ClientInfo m_client;

    // 客户端状态保护（用于跨线程访问）
    mutable std::mutex m_clientMutex;

    // 玩家ID生成
    PlayerId m_nextPlayerId = 1;

    // 区块加载票据管理器
    std::unique_ptr<world::ChunkLoadTicketManager> m_ticketManager;

    // 玩家当前区块位置（用于检测跨区块移动）
    ChunkCoord m_lastPlayerChunkX = std::numeric_limits<ChunkCoord>::max();
    ChunkCoord m_lastPlayerChunkZ = std::numeric_limits<ChunkCoord>::max();

    // 统计
    u64 m_tickCount = 0;
    u64 m_lastKeepAliveTime = 0;
};

} // namespace mr::server
