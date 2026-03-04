#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/network/LocalConnection.hpp"
#include "common/network/ProtocolPackets.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/world/TerrainGenerator.hpp"
#include "common/world/ChunkData.hpp"
#include "common/entity/Player.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace mr::server {

/**
 * @brief 内置服务端配置
 */
struct IntegratedServerConfig {
    String worldName = "singleplayer";
    i64 seed = 0;
    GameMode defaultGameMode = GameMode::Survival;
    i32 viewDistance = 10;
    i32 tickRate = 20;  // TPS
};

/**
 * @brief 简化的区块数据
 */
struct IntegratedChunk {
    std::unique_ptr<ChunkData> data;
    bool isGenerated = false;
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
    ChunkData* getOrGenerateChunk(ChunkCoord x, ChunkCoord z);
    void sendChunkToClient(ChunkCoord x, ChunkCoord z);
    void updateChunkSubscription();

    // 网络事件处理
    void onPacketReceived(const u8* data, size_t size);

    // 数据包处理
    void handleLoginRequest(const u8* data, size_t size);
    void handlePlayerMove(const u8* data, size_t size);
    void handleTeleportConfirm(const u8* data, size_t size);
    void handleKeepAlive(const u8* data, size_t size);
    void handleChatMessage(const u8* data, size_t size);

    // 发送数据包
    void sendLoginResponse(bool success, PlayerId playerId, const String& username, const String& message);
    void sendKeepAlive(u64 timestamp);
    void sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    void sendUnloadChunk(ChunkCoord x, ChunkCoord z);
    void sendToClient(const u8* data, size_t size);

    IntegratedServerConfig m_config;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    // 地形生成器
    std::unique_ptr<ITerrainGenerator> m_terrainGenerator;

    // 区块存储
    std::unordered_map<ChunkId, IntegratedChunk> m_chunks;

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

        // 已加载的区块
        std::unordered_set<ChunkId> loadedChunks;
    };
    ClientInfo m_client;

    // 玩家ID生成
    PlayerId m_nextPlayerId = 1;

    // 统计
    u64 m_tickCount = 0;
    u64 m_lastKeepAliveTime = 0;
};

} // namespace mr::server
