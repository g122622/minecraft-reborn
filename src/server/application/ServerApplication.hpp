#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/network/TcpServer.hpp"
#include "server/core/ServerCore.hpp"
#include <string>
#include <memory>
#include <atomic>

namespace mc::server {

/**
 * @brief 服务端启动参数
 *
 * 用于命令行覆盖设置文件中的配置。
 */
struct ServerLaunchParams {
    // 网络配置覆盖（可选）
    Optional<u16> port;
    Optional<String> bindAddress;
    Optional<u32> maxPlayers;

    // 世界配置覆盖（可选）
    Optional<String> worldName;
    Optional<i64> seed;

    // 其他启动参数
    Optional<String> settingsPath;  // 自定义设置文件路径
};

/**
 * @brief 服务端应用
 *
 * 使用 ServerCore 进行核心逻辑管理，
 * 使用 TcpServer 进行网络通信。
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
     *
     * @param params 启动参数（可选覆盖）
     */
    [[nodiscard]] Result<void> initialize(const ServerLaunchParams& params = {});

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
     * @brief 获取设置
     */
    [[nodiscard]] ServerSettings& settings() noexcept { return m_settings; }
    [[nodiscard]] const ServerSettings& settings() const noexcept { return m_settings; }

    /**
     * @brief 获取世界
     */
    [[nodiscard]] ServerWorld& world() noexcept { return *m_world; }
    [[nodiscard]] const ServerWorld& world() const noexcept { return *m_world; }

    /**
     * @brief 获取 ServerCore
     */
    [[nodiscard]] ServerCore& serverCore() noexcept { return *m_serverCore; }
    [[nodiscard]] const ServerCore& serverCore() const noexcept { return *m_serverCore; }

private:
    void mainLoop();
    void tick();
    void shutdown();

    // 加载设置
    [[nodiscard]] Result<void> loadSettings(const String& path);

    // 应用设置到系统
    void applySettings();

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

    ServerSettings m_settings;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    // 核心逻辑管理器
    std::unique_ptr<ServerCore> m_serverCore;

    std::unique_ptr<ServerWorld> m_world;
    std::unique_ptr<TcpServer> m_server;

    // 统计
    u64 m_lastKeepAliveTime = 0;
};

} // namespace mc::server
