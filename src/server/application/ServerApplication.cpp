#include "ServerApplication.hpp"
#include "CoreCommandBridge.hpp"
#include "minecraft-reborn/version.h"
#include "common/network/ProtocolPackets.hpp"
#include "common/entity/VanillaEntities.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/network/TcpConnection.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/PacketHandler.hpp"

#include <spdlog/spdlog.h>
#include <thread>

namespace mc::server {

ServerApplication::ServerApplication() = default;

ServerApplication::~ServerApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ServerApplication::initialize(const ServerLaunchParams& params)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    // 加载设置
    String settingsPath = params.settingsPath.value_or(
        ServerSettings::getDefaultPath().string());
    auto settingsResult = loadSettings(settingsPath);
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
                     settingsPath, settingsResult.error().toString());
    }

    // 应用命令行覆盖
    if (params.port.has_value()) {
        m_settings.serverPort.set(*params.port);
    }
    if (params.bindAddress.has_value()) {
        m_settings.bindAddress.set(*params.bindAddress);
    }
    if (params.maxPlayers.has_value()) {
        m_settings.maxPlayers.set(static_cast<i32>(*params.maxPlayers));
    }
    if (params.worldName.has_value()) {
        m_settings.worldName.set(*params.worldName);
    }
    if (params.seed.has_value()) {
        m_settings.levelSeed.set(std::to_string(*params.seed));
    }

    // 应用设置到系统
    applySettings();

    // 设置日志级别
    const auto& logLevel = m_settings.logLevel.get();
    if (logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Minecraft Reborn Server ===");
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing server...");

    // 初始化性能追踪
    mc::perfetto::TraceConfig traceConfig;
    traceConfig.outputPath = "server_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().startTracing();
    spdlog::info("Perfetto tracing initialized");

    // 注册实体类型
    entity::VanillaEntities::registerAll();

    // 初始化 ServerCore
    ServerCoreConfig coreConfig;
    coreConfig.viewDistance = m_settings.viewDistance.get();
    coreConfig.maxPlayers = m_settings.maxPlayers.get();
    coreConfig.seed = static_cast<u64>(std::stoll(m_settings.levelSeed.get()));
    coreConfig.tickRate = 20;
    m_serverCore = std::make_unique<ServerCore>(coreConfig);

    // 初始化世界
    ServerWorldConfig worldConfig;
    worldConfig.viewDistance = m_settings.viewDistance.get();
    worldConfig.dimension = 0; // 主世界

    m_world = std::make_unique<ServerWorld>(worldConfig);
    auto worldResult = m_world->initialize();
    if (worldResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize world: " + worldResult.error().message());
    }
    m_serverCore->setWorld(m_world.get());
    spdlog::info("World initialized");

    // 初始化网络服务器
    m_server = std::make_unique<TcpServer>();

    // 设置网络回调
    m_server->setOnConnect([this](TcpSession* session) {
        onClientConnect(session);
    });

    m_server->setOnDisconnect([this](TcpSession* session, const String& reason) {
        onClientDisconnect(session, reason);
    });

    m_server->setOnPacket([this](TcpSession* session, const u8* data, size_t size) {
        onPacketReceived(session, data, size);
    });

    // 启动服务器
    TcpServerConfig serverConfig;
    serverConfig.port = static_cast<u16>(m_settings.serverPort.get());
    serverConfig.maxConnections = static_cast<u32>(m_settings.maxPlayers.get());

    auto serverResult = m_server->start(serverConfig);
    if (serverResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to start server: " + serverResult.error().message());
    }

    spdlog::info("Server initialized successfully");
    spdlog::info("Port: {}", m_settings.serverPort.get());
    spdlog::info("Max players: {}", m_settings.maxPlayers.get());
    spdlog::info("World: {}", m_settings.worldName.get());

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

    spdlog::info("Server is now running!");
    spdlog::info("Connect with port: {}", m_settings.serverPort.get());

    while (m_running) {
        MC_TRACE_EVENT("game.tick", "ServerLoop");

        const auto currentTime = clock::now();
        const auto deltaTime = currentTime - lastTickTime;

        if (deltaTime >= tickDuration) {
            // 执行游戏刻
            tick();

            lastTickTime = currentTime;

            // 追踪 TPS
            const f64 tps = 1.0 / (std::chrono::duration<f64>(deltaTime).count());
            MC_TRACE_COUNTER("game.tick", "TPS", static_cast<i64>(tps));

            // 每秒输出一次统计信息
            u64 tickCount = m_serverCore->currentTick();
            if (tickCount % 20 == 0) {
                (void)tps;  // Avoid unused variable warning when SPDLOG_TRACE is disabled
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
    MC_TRACE_EVENT("game.tick", "ServerTick");

    // 处理网络事件
    {
        MC_TRACE_EVENT("network.packet", "PollNetwork");
        if (m_server) {
            m_server->poll();
        }
    }

    // 使用 ServerCore 的 tick
    m_serverCore->tick();

    // 更新世界
    {
        MC_TRACE_EVENT("game.tick", "WorldUpdate");
        if (m_world) {
            m_world->tick();
        }
    }

    // 每 15 秒发送一次心跳
    const u64 tickCount = m_serverCore->currentTick();
    if (tickCount - m_lastKeepAliveTime >= 300) { // 300 ticks = 15 seconds
        m_lastKeepAliveTime = tickCount;

        u64 timestamp = util::TimeUtils::getCurrentTimeMs();

        // 向所有连接的玩家发送心跳
        m_serverCore->forEachPlayer([this, timestamp, tickCount](ServerPlayerData& player) {
            if (player.loggedIn && player.hasConnection()) {
                // 使用 KeepAlivePacket 发送心跳
                network::KeepAlivePacket packet;
                packet.setTimestamp(timestamp);

                auto result = packet.serialize();
                if (result.success()) {
                    player.send(result.value().data(), result.value().size());
                }

                m_serverCore->keepAliveManager().recordKeepAliveSent(
                    player.playerId, timestamp, tickCount);
            }
        });
    }
}

void ServerApplication::shutdown()
{
    spdlog::info("Shutting down server...");

    // 保存设置
    auto saveResult = m_settings.saveSettings(ServerSettings::getDefaultPath());
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 断开所有玩家
    if (m_serverCore) {
        m_serverCore->connectionManager().disconnectAll("Server shutting down");
    }

    // 关闭网络服务器
    if (m_server) {
        m_server->stop();
        m_server.reset();
    }

    // 保存并关闭世界
    if (m_world) {
        m_world->shutdown();
        m_world.reset();
    }

    // 清理 ServerCore
    m_serverCore.reset();

    // 关闭性能追踪
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

    spdlog::info("Server stopped.");
}

void ServerApplication::onClientConnect(TcpSession* session)
{
    spdlog::info("Client connected: {}:{}",
                 session->address(), session->port());
}

void ServerApplication::onClientDisconnect(TcpSession* session, const String& reason)
{
    spdlog::info("Client disconnected: {}:{} - {}",
                 session->address(), session->port(), reason);

    // 使用 ServerCore 移除玩家
    PlayerId playerId = m_serverCore->playerManager().getPlayerIdBySession(
        static_cast<u32>(session->id()));
    if (playerId != 0) {
        m_serverCore->removePlayer(playerId);

        // 从世界移除玩家
        if (m_world) {
            m_world->removePlayer(playerId);
        }
    }
}

void ServerApplication::onPacketReceived(TcpSession* session, const u8* data, size_t size)
{
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("Packet too small from session {}", session->id());
        return;
    }

    // 解析包头
    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("Failed to read packet header from session {}", session->id());
        return;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());

    // 根据会话状态和数据包类型处理
    switch (packetType) {
        case network::PacketType::LoginRequest:
            handleLoginRequest(session, data + network::PACKET_HEADER_SIZE,
                             size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::PlayerMove:
            handlePlayerMove(session, data + network::PACKET_HEADER_SIZE,
                           size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::TeleportConfirm:
            handleTeleportConfirm(session, data + network::PACKET_HEADER_SIZE,
                                size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::KeepAlive:
            handleKeepAlive(session, data + network::PACKET_HEADER_SIZE,
                          size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::ChatMessage:
            handleChatMessage(session, data + network::PACKET_HEADER_SIZE,
                            size - network::PACKET_HEADER_SIZE);
            break;

        default:
            spdlog::debug("Unhandled packet type {} from session {}",
                        static_cast<int>(packetType), session->id());
            break;
    }
}

void ServerApplication::handleLoginRequest(TcpSession* session, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request from session {}", session->id());
        sendLoginResponse(session, false, 0, "", "Invalid login request");
        session->disconnect("Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join from {}:{}",
                 username, session->address(), session->port());

    // 检查玩家数量限制
    if (m_serverCore->playerManager().isFull()) {
        sendLoginResponse(session, false, 0, username, "Server is full");
        session->disconnect("Server is full");
        return;
    }

    // 创建连接
    auto connection = std::make_shared<TcpConnection>(session->shared_from_this());

    // 分配玩家ID并添加到 ServerCore
    PlayerId playerId = m_serverCore->nextPlayerId();
    auto* player = m_serverCore->addPlayer(playerId, username, connection);

    if (!player) {
        sendLoginResponse(session, false, 0, username, "Failed to add player");
        session->disconnect("Failed to add player");
        return;
    }

    // 设置会话ID并建立映射
    player->sessionId = static_cast<u32>(session->id());
    m_serverCore->playerManager().mapSessionToPlayer(static_cast<u32>(session->id()), playerId);

    // 添加到世界
    if (m_world) {
        m_world->addPlayer(playerId, username, connection);
    }

    // 更新会话状态
    session->setState(SessionState::Playing);

    // 发送登录成功响应
    sendLoginResponse(session, true, playerId, username, "Welcome!");

    spdlog::info("Player '{}' (ID: {}) joined the game", username, playerId);
}

void ServerApplication::handlePlayerMove(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId = m_serverCore->playerManager().getPlayerIdBySession(
        static_cast<u32>(session->id()));
    if (playerId == 0) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse player move from player {}", playerId);
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    m_serverCore->updatePlayerPosition(playerId, pos.x, pos.y, pos.z,
                                        pos.yaw, pos.pitch, pos.onGround);
}

void ServerApplication::handleTeleportConfirm(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId = m_serverCore->playerManager().getPlayerIdBySession(
        static_cast<u32>(session->id()));
    if (playerId == 0) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse teleport confirm from player {}", playerId);
        return;
    }

    auto& packet = result.value();

    m_serverCore->confirmTeleport(playerId, packet.teleportId());
}

void ServerApplication::handleKeepAlive(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId = m_serverCore->playerManager().getPlayerIdBySession(
        static_cast<u32>(session->id()));
    if (playerId == 0) {
        return;
    }

    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        m_serverCore->keepAliveManager().handleKeepAliveResponse(playerId, packet.timestamp(), currentTimeMs);
        spdlog::trace("KeepAlive from session {}", session->id());
    }
}

void ServerApplication::handleChatMessage(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId = m_serverCore->playerManager().getPlayerIdBySession(
        static_cast<u32>(session->id()));
    if (playerId == 0) {
        return;
    }

    auto* player = m_serverCore->getPlayer(playerId);
    if (!player) return;

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        return;
    }

    auto& packet = result.value();
    String message = packet.message();

    if (!message.empty() && message[0] == '/') {
        auto& registry = command::CommandRegistry::getGlobal();
        CoreCommandBridge bridge(m_world.get(), m_serverCore.get());
        command::ServerCommandSource source(&bridge, nullptr, m_world.get(),
                                            Vector3d(player->x, player->y, player->z),
                                            Vector2f(player->yaw, player->pitch),
                                            4, playerId, player->username);
        auto commandResult = registry.execute(message, source);

        if (commandResult.failed()) {
            spdlog::warn("Command '{}' failed for {}: {}",
                         message, player->username, commandResult.error().toString());
        } else {
            spdlog::info("Executed command '{}' for {} with result {}",
                         message, player->username, commandResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);

    // 广播聊天消息到所有玩家
    // TODO: 实现聊天消息广播
}

void ServerApplication::sendLoginResponse(TcpSession* session, bool success,
                                          PlayerId playerId, const String& username,
                                          const String& message)
{
    network::LoginResponsePacket response(success, playerId, username, message);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LoginResponse, ser.buffer());
    session->send(fullPacket.data(), fullPacket.size());
}

void ServerApplication::sendKeepAlive(TcpSession* session, u64 timestamp)
{
    network::KeepAlivePacket packet;
    packet.setTimestamp(timestamp);

    auto result = packet.serialize();
    if (result.success()) {
        session->send(result.value().data(), result.value().size());
    }
}

// 设置相关方法

Result<void> ServerApplication::loadSettings(const String& path)
{
    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 确保设置目录存在
    (void)SettingsBase::ensureSettingsDir("minecraft-server");

    // 启用自动保存
    m_settings.enableAutoSave(path);

    return Result<void>::ok();
}

void ServerApplication::applySettings()
{
    // 设置变更回调
    m_settings.serverPort.onChange([this](i32 value) {
        spdlog::info("Server port changed to: {}", value);
        // 端口变更需要重启服务器
    });

    m_settings.maxPlayers.onChange([this](i32 value) {
        spdlog::info("Max players changed to: {}", value);
        if (m_serverCore) {
            m_serverCore->playerManager().setMaxPlayers(value);
        }
    });

    m_settings.viewDistance.onChange([this](i32 value) {
        spdlog::info("View distance changed to: {}", value);
        if (m_world) {
            auto config = m_world->config();
            config.viewDistance = value;
            m_world->setConfig(config);
        }
    });

    m_settings.logLevel.onChange([this](const String& value) {
        spdlog::info("Log level changed to: {}", value);
        if (value == "trace") {
            spdlog::set_level(spdlog::level::trace);
        } else if (value == "debug") {
            spdlog::set_level(spdlog::level::debug);
        } else if (value == "info") {
            spdlog::set_level(spdlog::level::info);
        } else if (value == "warn") {
            spdlog::set_level(spdlog::level::warn);
        } else if (value == "error") {
            spdlog::set_level(spdlog::level::err);
        }
    });
}

} // namespace mc::server
