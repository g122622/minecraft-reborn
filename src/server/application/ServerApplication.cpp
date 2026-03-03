#include "ServerApplication.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

namespace mr::server {

ServerApplication::ServerApplication() = default;

ServerApplication::~ServerApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ServerApplication::initialize(const ServerConfig& config)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    m_config = config;

    // 设置日志级别
    if (m_config.logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (m_config.logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (m_config.logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (m_config.logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (m_config.logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Minecraft Reborn Server ===");
    spdlog::info("Version: {}.{}.{}", MR_VERSION_MAJOR, MR_VERSION_MINOR, MR_VERSION_PATCH);
    spdlog::info("Initializing server...");

    // TODO: 初始化各个子系统
    // - 网络管理器
    // - 世界管理器
    // - 玩家管理器
    // - 控制台命令处理

    spdlog::info("Server initialized successfully");
    spdlog::info("Port: {}", m_config.port);
    spdlog::info("Max players: {}", m_config.maxPlayers);
    spdlog::info("World: {}", m_config.worldName);

    m_initialized = true;
    return Result<void>::ok();
}

Result<void> ServerApplication::run()
{
    if (!m_initialized) {
        return Error(ErrorCode::InvalidArgument, "Server not initialized");
    }

    if (m_running) {
        return Error(ErrorCode::AlreadyExists, "Server already running");
    }

    spdlog::info("Starting server main loop...");
    m_running = true;

    try {
        mainLoop();
    } catch (const std::exception& e) {
        spdlog::critical("Server crashed: {}", e.what());
        m_running = false;
        return Error(ErrorCode::Unknown, e.what());
    }

    return Result<void>::ok();
}

void ServerApplication::stop()
{
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping server...");
    m_running = false;
}

void ServerApplication::mainLoop()
{
    using clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    constexpr f64 targetTickTime = 1.0 / 20.0; // 20 TPS
    constexpr auto tickDuration = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<f64>(targetTickTime));

    auto lastTickTime = clock::now();
    u64 tickCount = 0;

    spdlog::info("Server is now running!");
    spdlog::info("Connect with port: {}", m_config.port);

    while (m_running) {
        const auto currentTime = clock::now();
        const auto deltaTime = currentTime - lastTickTime;

        if (deltaTime >= tickDuration) {
            // 执行游戏刻
            tick();

            lastTickTime = currentTime;
            ++tickCount;

            // 每秒输出一次统计信息
            if (tickCount % 20 == 0) {
                const auto tps = 1.0 / (std::chrono::duration<f64>(deltaTime).count());
                SPDLOG_TRACE("TPS: {:.1f}, Tick: {}", tps, tickCount);
            }
        } else {
            // 等待下一刻
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    shutdown();
}

void ServerApplication::tick()
{
    // TODO: 实现游戏刻逻辑
    // 1. 处理网络数据包
    // 2. 更新世界
    // 3. 更新实体
    // 4. 发送更新到客户端
}

void ServerApplication::shutdown()
{
    spdlog::info("Shutting down server...");

    // TODO: 清理资源
    // - 保存世界
    // - 断开所有玩家连接
    // - 关闭网络

    spdlog::info("Server stopped.");
}

// ServerConfig 实现

Result<ServerConfig> ServerConfig::load(const String& path)
{
    // TODO: 实现JSON配置加载
    ServerConfig config;
    spdlog::info("Loaded server config from: {}", path);
    return config;
}

Result<void> ServerConfig::save(const String& path) const
{
    // TODO: 实现JSON配置保存
    spdlog::info("Saved server config to: {}", path);
    return Result<void>::ok();
}

} // namespace mr::server
