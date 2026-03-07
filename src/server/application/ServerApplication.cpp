#include "ServerApplication.hpp"
#include "minecraft-reborn/version.h"
#include "common/network/ProtocolPackets.hpp"

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
    spdlog::info("Version: {}.{}.{}", MR_VERSION_MAJOR, MR_VERSION_MINOR, MR_VERSION_PATCH);
    spdlog::info("Initializing server...");

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
    m_tickCount = 0;

    spdlog::info("Server is now running!");
    spdlog::info("Connect with port: {}", m_settings.serverPort.get());

    while (m_running) {
        const auto currentTime = clock::now();
        const auto deltaTime = currentTime - lastTickTime;

        if (deltaTime >= tickDuration) {
            // 执行游戏刻
            tick();

            lastTickTime = currentTime;
            ++m_tickCount;

            // 每秒输出一次统计信息
            if (m_tickCount % 20 == 0) {
                const auto tps = 1.0 / (std::chrono::duration<f64>(deltaTime).count());
                SPDLOG_TRACE("TPS: {:.1f}, Tick: {}", tps, m_tickCount);
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
    // 处理网络事件
    if (m_server) {
        m_server->poll();
    }

    // 更新世界
    if (m_world) {
        m_world->tick();
    }

    // 每 15 秒发送一次心跳
    if (m_tickCount - m_lastKeepAliveTime >= 300) { // 300 ticks = 15 seconds
        m_lastKeepAliveTime = m_tickCount;

        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        // 向所有连接的玩家发送心跳
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        for (const auto& [sessionId, playerId] : m_sessionToPlayer) {
            auto session = m_server->getSession(sessionId);
            if (session && session->state() == SessionState::Playing) {
                sendKeepAlive(session.get(), static_cast<u64>(timestamp));
            }
        }
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

    // 查找并移除玩家
    std::lock_guard<std::mutex> lock(m_playerMapMutex);

    auto it = m_sessionToPlayer.find(session->id());
    if (it != m_sessionToPlayer.end()) {
        PlayerId playerId = it->second;

        // 从世界移除玩家
        if (m_world) {
            m_world->removePlayer(playerId);
        }

        // 清理映射
        m_playerToSession.erase(playerId);
        m_sessionToPlayer.erase(it);
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
    size_t currentPlayerCount;
    {
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        currentPlayerCount = m_sessionToPlayer.size();
    }

    if (currentPlayerCount >= static_cast<size_t>(m_settings.maxPlayers.get())) {
        sendLoginResponse(session, false, 0, username, "Server is full");
        session->disconnect("Server is full");
        return;
    }

    // 分配玩家ID
    PlayerId playerId = m_nextPlayerId++;

    // 注册玩家
    {
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        m_sessionToPlayer[session->id()] = playerId;
        m_playerToSession[playerId] = session->id();
    }

    // 添加到世界
    if (m_world) {
        m_world->addPlayer(playerId, username, session->shared_from_this());
    }

    // 更新会话状态
    session->setState(SessionState::Playing);

    // 发送登录成功响应
    sendLoginResponse(session, true, playerId, username, "Welcome!");

    spdlog::info("Player '{}' (ID: {}) joined the game", username, playerId);
}

void ServerApplication::handlePlayerMove(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId;
    {
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        auto it = m_sessionToPlayer.find(session->id());
        if (it == m_sessionToPlayer.end()) {
            return;
        }
        playerId = it->second;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse player move from player {}", playerId);
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    if (m_world) {
        m_world->updatePlayerPosition(playerId, pos.x, pos.y, pos.z,
                                      pos.yaw, pos.pitch, pos.onGround);
    }
}

void ServerApplication::handleTeleportConfirm(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId;
    {
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        auto it = m_sessionToPlayer.find(session->id());
        if (it == m_sessionToPlayer.end()) {
            return;
        }
        playerId = it->second;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse teleport confirm from player {}", playerId);
        return;
    }

    auto& packet = result.value();

    if (m_world) {
        m_world->confirmTeleport(playerId, packet.teleportId());
    }
}

void ServerApplication::handleKeepAlive(TcpSession* session, const u8* data, size_t size)
{
    // 心跳响应 - 更新玩家活跃状态
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        spdlog::trace("KeepAlive from session {}", session->id());
    }
}

void ServerApplication::handleChatMessage(TcpSession* session, const u8* data, size_t size)
{
    PlayerId playerId;
    String username;
    {
        std::lock_guard<std::mutex> lock(m_playerMapMutex);
        auto it = m_sessionToPlayer.find(session->id());
        if (it == m_sessionToPlayer.end()) {
            return;
        }
        playerId = it->second;

        if (m_world) {
            auto* player = m_world->getPlayer(playerId);
            if (player) {
                username = player->username;
            }
        }
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        return;
    }

    auto& packet = result.value();
    String message = packet.message();

    spdlog::info("[Chat] {}: {}", username, message);

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

    // 封装完整数据包
    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::LoginResponse));
    fullPacket.writeU16(0); // flags
    fullPacket.writeU16(0); // reserved
    fullPacket.writeU16(0); // padding
    fullPacket.writeBytes(ser.buffer());

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
    SettingsBase::ensureSettingsDir("minecraft-server");

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
    });

    m_settings.viewDistance.onChange([this](i32 value) {
        spdlog::info("View distance changed to: {}", value);
        if (m_world) {
            // TODO: 更新世界视距
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

} // namespace mr::server
