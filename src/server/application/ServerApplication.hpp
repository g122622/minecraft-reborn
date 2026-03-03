#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"

#include <string>
#include <memory>
#include <atomic>

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

private:
    void mainLoop();
    void tick();
    void shutdown();

    ServerConfig m_config;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};
};

} // namespace mr::server
