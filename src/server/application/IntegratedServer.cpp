#include "IntegratedServer.hpp"
#include "common/world/gen/NoiseChunkGenerator.hpp"
#include "common/world/gen/NoiseSettings.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/network/Packet.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"

#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>

namespace mr::server {

IntegratedServer::IntegratedServer() = default;

IntegratedServer::~IntegratedServer() {
    if (m_running) {
        stop();
    }
}

Result<void> IntegratedServer::initialize(const IntegratedServerConfig& config) {
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    m_config = config;
    // 初始化方块注册表（必须在创建地形生成器之前）
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    spdlog::info("Initializing integrated server...");
    spdlog::info("World: {}, Seed: {}, View distance: {}",
                 m_config.worldName, m_config.seed, m_config.viewDistance);

    // 创建本地连接对
    m_connectionPair = std::make_unique<network::LocalConnectionPair>();
    m_connectionPair->connect();
    m_serverEndpoint = &m_connectionPair->serverEndpoint();

    // 创建区块管理器（使用 MC 1.16.5 风格噪声生成器）
    DimensionSettings settings = DimensionSettings::overworld();
    auto generator = std::make_unique<NoiseChunkGenerator>(m_config.seed, std::move(settings));
    m_chunkManager = std::make_unique<ServerChunkManager>(std::move(generator));
    m_chunkManager->initialize();
    m_chunkManager->startWorkers();
    m_chunkManager->setViewDistance(m_config.viewDistance);

    // 初始化票据管理器
    m_ticketManager = std::make_unique<world::ChunkLoadTicketManager>();
    m_ticketManager->setViewDistance(m_config.viewDistance);

    // 设置票据级别变化回调
    m_ticketManager->setLevelChangeCallback(
        [this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
            onChunkLevelChanged(x, z, oldLevel, newLevel);
        });

    // 启动服务端线程
    m_running = true;
    m_serverThread = std::make_unique<std::thread>([this]() {
        mainLoop();
    });

    m_initialized = true;
    spdlog::info("Integrated server initialized");
    return Result<void>::ok();
}

void IntegratedServer::stop() {
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping integrated server...");
    m_running = false;

    // 1. 停止 Worker 线程（立即取消未完成任务）
    if (m_chunkManager) {
        m_chunkManager->stopWorkers();
    }

    // 2. 断开连接以唤醒可能阻塞的线程
    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }

    // 3. 等待服务端线程结束
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // 4. 清理待发送队列
    {
        std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
        m_pendingSends.clear();
    }

    // 5. 关闭区块管理器
    if (m_chunkManager) {
        m_chunkManager->shutdown();
    }

    shutdown();
    spdlog::info("Integrated server stopped");
}

network::LocalEndpoint* IntegratedServer::getClientEndpoint() {
    if (m_connectionPair) {
        return &m_connectionPair->clientEndpoint();
    }
    return nullptr;
}

void IntegratedServer::mainLoop() {
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::milliseconds(1000 / m_config.tickRate);

    spdlog::info("Integrated server started ({} TPS)", m_config.tickRate);

    while (m_running.load(std::memory_order_acquire)) {
        auto startTime = clock::now();

        tick();

        auto elapsed = clock::now() - startTime;
        auto sleepTime = tickDuration - elapsed;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    // 注意：不要在这里调用 shutdown()，stop() 方法会处理清理
}

void IntegratedServer::tick() {
    // 如果已停止，跳过处理
    if (!m_running.load()) {
        return;
    }

    // 处理网络数据包
    std::vector<u8> packetData;
    while (m_running.load() && m_serverEndpoint && m_serverEndpoint->receive(packetData)) {
        onPacketReceived(packetData.data(), packetData.size());
    }

    // 如果已停止，跳过更新
    if (!m_running.load()) {
        return;
    }

    // 处理待发送区块（从 Worker 线程推送）
    processPendingChunkSends();

    // 更新票据管理器（清理过期票据等）
    if (m_ticketManager) {
        m_ticketManager->tick();
    }

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 心跳（每 15 秒）
    m_tickCount++;
    if (m_tickCount % (static_cast<u64>(m_config.tickRate) * 15) == 0) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        sendKeepAlive(static_cast<u64>(timestamp));
    }
}

void IntegratedServer::shutdown() {
    // 清理区块管理器
    m_chunkManager.reset();

    if (m_connectionPair) {
        m_connectionPair->disconnect();
        m_connectionPair.reset();
    }

    m_serverEndpoint = nullptr;
    m_initialized = false;
}

void IntegratedServer::requestChunkAsync(ChunkCoord x, ChunkCoord z) {
    if (!m_running.load()) return;

    ChunkId id(x, z);
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        if (m_client.loadedChunks.count(id)) return;  // 已发送
    }

    if (!m_chunkManager) return;

    // 异步请求区块
    m_chunkManager->getChunkAsync(x, z,
        [this, x, z, id](bool success, ChunkData* chunk) {
            // Worker 线程回调
            if (!m_running.load() || !success || !chunk) return;

            // 检查是否已发送
            {
                std::lock_guard<std::mutex> lock(m_clientMutex);
                if (m_client.loadedChunks.count(id)) return;
            }

            // 序列化区块数据
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.failed()) {
                spdlog::error("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
                return;
            }

            // 推送到主线程队列
            std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
            m_pendingSends.push_back({x, z, std::move(result.value())});
        },
        &ChunkStatus::FULL
    );
}

void IntegratedServer::processPendingChunkSends() {
    // 移动待发送队列到本地
    std::vector<PendingChunkSend> sends;
    {
        std::lock_guard<std::mutex> lock(m_pendingSendsMutex);
        sends = std::move(m_pendingSends);
        m_pendingSends.clear();
    }

    // 发送所有待发送区块
    for (const auto& send : sends) {
        if (!m_running.load()) return;

        // 检查客户端是否仍然登录
        bool loggedIn;
        {
            std::lock_guard<std::mutex> lock(m_clientMutex);
            loggedIn = m_client.loggedIn;
        }
        if (!loggedIn) return;

        ChunkId id(send.x, send.z);
        {
            std::lock_guard<std::mutex> lock(m_clientMutex);
            if (m_client.loadedChunks.count(id)) continue;  // 已发送
            m_client.loadedChunks.insert(id);
        }

        spdlog::debug("Sending chunk ({}, {}) to client, size: {} bytes",
                      send.x, send.z, send.serializedData.size());
        sendChunkData(send.x, send.z, send.serializedData);
    }
}

void IntegratedServer::sendChunkToClient(ChunkCoord x, ChunkCoord z) {
    // 如果服务器已停止，跳过
    if (!m_running.load()) {
        return;
    }

    ChunkId id(x, z);

    // 检查是否已发送
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        if (m_client.loadedChunks.find(id) != m_client.loadedChunks.end()) {
            return;
        }
    }

    // 异步请求区块
    requestChunkAsync(x, z);
}

void IntegratedServer::onPacketReceived(const u8* data, size_t size) {
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("Packet too small: {} bytes", size);
        return;
    }

    // 解析包头
    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("Failed to read packet header");
        return;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());

    switch (packetType) {
        case network::PacketType::LoginRequest:
            handleLoginRequest(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::PlayerMove:
            handlePlayerMove(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::TeleportConfirm:
            handleTeleportConfirm(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::KeepAlive:
            handleKeepAlive(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::ChatMessage:
            handleChatMessage(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        default:
            spdlog::debug("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

void IntegratedServer::handleLoginRequest(const u8* data, size_t size) {
    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request");
        sendLoginResponse(false, 0, "", "Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 分配玩家ID
    m_client.playerId = m_nextPlayerId++;
    m_client.username = username;
    m_client.loggedIn = true;

    // 设置初始位置（出生点）
    m_client.x = 0.0;
    m_client.y = 90.0;
    m_client.z = 0.0;

    // 发送登录成功响应
    sendLoginResponse(true, m_client.playerId, username, "Welcome to singleplayer world!");

    // 发送初始传送（将玩家传送到出生点）
    u32 teleportId = static_cast<u32>(m_tickCount);
    m_client.pendingTeleportId = teleportId;
    m_client.waitingTeleportConfirm = true;
    sendTeleport(m_client.x, m_client.y, m_client.z, m_client.yaw, m_client.pitch, teleportId);

    // 初始化玩家票据位置
    ChunkCoord spawnChunkX = static_cast<ChunkCoord>(std::floor(m_client.x / 16.0));
    ChunkCoord spawnChunkZ = static_cast<ChunkCoord>(std::floor(m_client.z / 16.0));
    m_lastPlayerChunkX = spawnChunkX;
    m_lastPlayerChunkZ = spawnChunkZ;

    // 在票据管理器中注册玩家位置
    m_ticketManager->updatePlayerPosition(m_client.playerId, spawnChunkX, spawnChunkZ);

    spdlog::info("Player '{}' (ID: {}) joined the game at chunk ({}, {})",
                 username, m_client.playerId, spawnChunkX, spawnChunkZ);
}

void IntegratedServer::handlePlayerMove(const u8* data, size_t size) {
    if (!m_client.loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse player move");
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    // 更新玩家位置
    m_client.x = pos.x;
    m_client.y = pos.y;
    m_client.z = pos.z;
    m_client.yaw = pos.yaw;
    m_client.pitch = pos.pitch;

    // 检查是否跨越区块边界
    ChunkCoord newChunkX = static_cast<ChunkCoord>(std::floor(m_client.x / 16.0));
    ChunkCoord newChunkZ = static_cast<ChunkCoord>(std::floor(m_client.z / 16.0));

    if (newChunkX != m_lastPlayerChunkX || newChunkZ != m_lastPlayerChunkZ) {
        handlePlayerChunkMove(newChunkX, newChunkZ);
        m_lastPlayerChunkX = newChunkX;
        m_lastPlayerChunkZ = newChunkZ;
    }
}

void IntegratedServer::handlePlayerChunkMove(ChunkCoord newChunkX, ChunkCoord newChunkZ) {
    spdlog::debug("Player crossed chunk boundary: ({}, {}) -> ({}, {})",
                  m_lastPlayerChunkX, m_lastPlayerChunkZ, newChunkX, newChunkZ);

    // 更新票据管理器中的玩家位置
    // 这会自动触发区块加载/卸载的计算
    m_ticketManager->updatePlayerPosition(m_client.playerId, newChunkX, newChunkZ);
}

void IntegratedServer::onChunkLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    // 如果服务器已停止，跳过
    if (!m_running.load()) {
        return;
    }

    ChunkId id(x, z);
    bool wasLoaded = world::shouldChunkLoad(oldLevel);
    bool isLoaded = world::shouldChunkLoad(newLevel);

    if (!wasLoaded && isLoaded) {
        // 区块从卸载变为加载 - 异步请求
        spdlog::debug("Chunk ({}, {}) loading: level {} -> {}", x, z, oldLevel, newLevel);
        requestChunkAsync(x, z);
    } else if (wasLoaded && !isLoaded) {
        // 区块从加载变为卸载
        spdlog::debug("Chunk ({}, {}) unloading: level {} -> {}", x, z, oldLevel, newLevel);
        std::lock_guard<std::mutex> lock(m_clientMutex);
        if (m_client.loadedChunks.count(id) > 0) {
            sendUnloadChunk(x, z);
            m_client.loadedChunks.erase(id);
        }
    }
    // 其他情况：区块级别变化但仍在加载范围内，不需要操作
}

void IntegratedServer::handleTeleportConfirm(const u8* data, size_t size) {
    if (!m_client.loggedIn) {
        spdlog::warn("Teleport confirm received but client not logged in");
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse teleport confirm: {}", result.error().message());
        return;
    }

    auto& packet = result.value();
    spdlog::debug("Teleport confirm received: id={}, expected={}",
                 packet.teleportId(), m_client.pendingTeleportId);

    if (m_client.waitingTeleportConfirm && packet.teleportId() == m_client.pendingTeleportId) {
        m_client.waitingTeleportConfirm = false;

        // 传送确认后，触发区块加载
        // 票据系统已经通过 handlePlayerChunkMove 设置了玩家位置
        // 这里只需要处理更新，确保区块被发送
        m_ticketManager->processUpdates();
    } else {
        spdlog::warn("Unexpected teleport confirm: id={}, expected={}",
                     packet.teleportId(), m_client.pendingTeleportId);
    }
}

void IntegratedServer::handleKeepAlive(const u8* data, size_t size) {
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        spdlog::trace("KeepAlive received: {}", packet.timestamp());
    }
}

void IntegratedServer::handleChatMessage(const u8* data, size_t size) {
    if (!m_client.loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        return;
    }

    auto& packet = result.value();
    spdlog::info("[Chat] {}: {}", m_client.username, packet.message());
}

void IntegratedServer::sendLoginResponse(bool success, PlayerId playerId,
                                          const String& username, const String& message) {
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

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendKeepAlive(u64 timestamp) {
    network::KeepAlivePacket packet;
    packet.setTimestamp(timestamp);

    auto result = packet.serialize();
    if (result.success()) {
        sendToClient(result.value().data(), result.value().size());
    }
}

void IntegratedServer::sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId) {
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::Teleport));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
    network::ChunkDataPacket packet(x, z, data);
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::ChunkData));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendUnloadChunk(ChunkCoord x, ChunkCoord z) {
    network::UnloadChunkPacket packet(x, z);
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::UnloadChunk));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendToClient(const u8* data, size_t size) {
    if (m_serverEndpoint && m_serverEndpoint->isConnected()) {
        m_serverEndpoint->send(data, size);
    }
}

} // namespace mr::server
