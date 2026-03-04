#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/network/TcpServer.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

namespace mr::server {

/**
 * @brief 服务端配置
 */
struct ServerConfig {
    // 网络配置
    u16 port = 19132;
    String bindAddress = "0.0.0.0";
    u32 maxPlayers = 20;
    bool onlineMode = true;

    // 世界配置
    String worldName = "world";
    String levelName = "Minecraft Reborn Server";
    i64 seed = 0;
    String generator = "default";
    bool generateStructures = true;

    // 游戏配置
    GameMode defaultGameMode = GameMode::Survival;
    Difficulty difficulty = Difficulty::Normal;
    bool hardcore = false;
    bool pvpEnabled = true;

    // 性能配置
    i32 viewDistance = 10;
    i32 simulationDistance = 10;
    i32 tickRate = 20;

    // 安全配置
    bool whiteList = false;
    u32 maxTickTime = 60000; // 毫秒

    // 日志配置
    String logLevel = "info";
    bool logToFile = false;
    String logFile = "server.log";

    /**
     * @brief 从JSON文件加载配置
     */
    static Result<ServerConfig> load(const String& path);

    /**
     * @brief 保存配置到JSON文件
     */
    Result<void> save(const String& path) const;
};

/**
 * @brief 服务端应用
 */
class ServerApplication {
public:
    ServerApplication();
    ~ServerApplication();

    // 禁止拷贝
    ServerApplication(const ServerApplication&) = delete;
    ServerApplication& operator=(const ServerApplication&) = delete;

    /**
     * @brief 初始化服务端
     */
    [[nodiscard]] Result<void> initialize(const ServerConfig& config);

    /**
     * @brief 运行服务端主循环
     */
    [[nodiscard]] Result<void> run();

    /**
     * @brief 停止服务端
     */
    void stop();

    /**
     * @brief 检查服务端是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const ServerConfig& config() const noexcept { return m_config; }

    /**
     * @brief 获取世界
     */
    [[nodiscard]] ServerWorld& world() noexcept { return *m_world; }
    [[nodiscard]] const ServerWorld& world() const noexcept { return *m_world; }

private:
    void mainLoop();
    void tick();
    void shutdown();

    // 网络事件处理
    void onClientConnect(TcpSession* session);
    void onClientDisconnect(TcpSession* session, const String& reason);
    void onPacketReceived(TcpSession* session, const u8* data, size_t size);

    // 数据包处理
    void handleLoginRequest(TcpSession* session, const u8* data, size_t size);
    void handlePlayerMove(TcpSession* session, const u8* data, size_t size);
    void handleTeleportConfirm(TcpSession* session, const u8* data, size_t size);
    void handleKeepAlive(TcpSession* session, const u8* data, size_t size);
    void handleChatMessage(TcpSession* session, const u8* data, size_t size);

    // 发送数据包
    void sendLoginResponse(TcpSession* session, bool success, PlayerId playerId, const String& username, const String& message);
    void sendKeepAlive(TcpSession* session, u64 timestamp);

    ServerConfig m_config;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    std::unique_ptr<ServerWorld> m_world;
    std::unique_ptr<TcpServer> m_server;

    // 会话到玩家ID的映射
    std::unordered_map<SessionId, PlayerId> m_sessionToPlayer;
    std::unordered_map<PlayerId, SessionId> m_playerToSession;
    std::mutex m_playerMapMutex;

    // 玩家ID生成
    std::atomic<PlayerId> m_nextPlayerId{1};

    // 统计
    u64 m_tickCount = 0;
    u64 m_lastKeepAliveTime = 0;
};

} // namespace mr::server
