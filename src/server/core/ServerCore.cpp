#include "ServerCore.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

ServerCore::ServerCore()
    : m_playerManager(std::make_unique<core::PlayerManager>())
    , m_connectionManager(std::make_unique<core::ConnectionManager>(*m_playerManager))
    , m_timeManager(std::make_unique<core::TimeManager>())
    , m_teleportManager(std::make_unique<core::TeleportManager>(*m_playerManager))
    , m_keepAliveManager(std::make_unique<core::KeepAliveManager>(*m_playerManager, m_config))
    , m_positionTracker(std::make_unique<core::PositionTracker>(*m_playerManager, m_config))
    , m_packetHandler(std::make_unique<core::PacketHandler>(
          *m_playerManager,
          *m_connectionManager,
          *m_teleportManager,
          *m_keepAliveManager,
          *m_positionTracker,
          *m_timeManager,
          m_config))
{
    m_playerManager->setConfig(m_config);
    m_weatherManager.initialize(m_config.seed);
}

ServerCore::ServerCore(const ServerCoreConfig& config)
    : m_config(config)
    , m_playerManager(std::make_unique<core::PlayerManager>(config))
    , m_connectionManager(std::make_unique<core::ConnectionManager>(*m_playerManager))
    , m_timeManager(std::make_unique<core::TimeManager>())
    , m_teleportManager(std::make_unique<core::TeleportManager>(*m_playerManager))
    , m_keepAliveManager(std::make_unique<core::KeepAliveManager>(*m_playerManager, m_config))
    , m_positionTracker(std::make_unique<core::PositionTracker>(*m_playerManager, m_config))
    , m_packetHandler(std::make_unique<core::PacketHandler>(
          *m_playerManager,
          *m_connectionManager,
          *m_teleportManager,
          *m_keepAliveManager,
          *m_positionTracker,
          *m_timeManager,
          m_config))
{
    m_playerManager->setConfig(m_config);
    m_weatherManager.initialize(m_config.seed);
}

ServerCore::~ServerCore() = default;

void ServerCore::setConfig(const ServerCoreConfig& config) {
    m_config = config;
    m_playerManager->setConfig(config);
}

// ============================================================================
// 管理器访问
// ============================================================================

core::PlayerManager& ServerCore::playerManager() { return *m_playerManager; }
const core::PlayerManager& ServerCore::playerManager() const { return *m_playerManager; }

core::ConnectionManager& ServerCore::connectionManager() { return *m_connectionManager; }
const core::ConnectionManager& ServerCore::connectionManager() const { return *m_connectionManager; }

core::TimeManager& ServerCore::timeManager() { return *m_timeManager; }
const core::TimeManager& ServerCore::timeManager() const { return *m_timeManager; }

core::TeleportManager& ServerCore::teleportManager() { return *m_teleportManager; }
const core::TeleportManager& ServerCore::teleportManager() const { return *m_teleportManager; }

core::KeepAliveManager& ServerCore::keepAliveManager() { return *m_keepAliveManager; }
const core::KeepAliveManager& ServerCore::keepAliveManager() const { return *m_keepAliveManager; }

core::PositionTracker& ServerCore::positionTracker() { return *m_positionTracker; }
const core::PositionTracker& ServerCore::positionTracker() const { return *m_positionTracker; }

core::PacketHandler& ServerCore::packetHandler() { return *m_packetHandler; }
const core::PacketHandler& ServerCore::packetHandler() const { return *m_packetHandler; }

WeatherManager& ServerCore::weatherManager() { return m_weatherManager; }
const WeatherManager& ServerCore::weatherManager() const { return m_weatherManager; }

// ============================================================================
// 玩家管理
// ============================================================================

ServerPlayerData* ServerCore::addPlayer(PlayerId playerId, const String& username, network::ConnectionPtr connection) {
    return m_playerManager->addPlayer(playerId, username, connection);
}

void ServerCore::removePlayer(PlayerId playerId) {
    m_playerManager->removePlayer(playerId);
}

ServerPlayerData* ServerCore::getPlayer(PlayerId playerId) {
    return m_playerManager->getPlayer(playerId);
}

const ServerPlayerData* ServerCore::getPlayer(PlayerId playerId) const {
    return m_playerManager->getPlayer(playerId);
}

bool ServerCore::hasPlayer(PlayerId playerId) const {
    return m_playerManager->hasPlayer(playerId);
}

size_t ServerCore::playerCount() const {
    return m_playerManager->playerCount();
}

PlayerId ServerCore::nextPlayerId() {
    return m_playerManager->nextPlayerId();
}

// ============================================================================
// 连接管理
// ============================================================================

void ServerCore::disconnectPlayer(PlayerId playerId, const String& reason) {
    m_connectionManager->disconnectPlayer(playerId, reason);
}

void ServerCore::cleanupDisconnectedPlayers() {
    m_connectionManager->cleanupDisconnectedPlayers();
}

void ServerCore::broadcast(const u8* data, size_t size) {
    m_connectionManager->broadcast(data, size);
}

void ServerCore::broadcastExcept(PlayerId excludePlayerId, const u8* data, size_t size) {
    m_connectionManager->broadcastExcept(excludePlayerId, data, size);
}

bool ServerCore::sendPacketToPlayer(PlayerId playerId, network::PacketType type, const std::vector<u8>& payload) {
    return m_connectionManager->sendPacketToPlayer(playerId, type, payload);
}

void ServerCore::broadcastPacket(network::PacketType type, const std::vector<u8>& payload) {
    m_connectionManager->broadcastPacket(type, payload);
}

// ============================================================================
// 时间管理
// ============================================================================

time::GameTime& ServerCore::gameTime() {
    return m_timeManager->gameTimeObj();
}

const time::GameTime& ServerCore::gameTime() const {
    return m_timeManager->gameTimeObj();
}

void ServerCore::tickTime() {
    m_timeManager->tick();
}

u64 ServerCore::currentTick() const {
    return m_timeManager->currentTick();
}

// ============================================================================
// 区块同步
// ============================================================================

network::ChunkSyncManager& ServerCore::chunkSyncManager() {
    return m_playerManager->chunkSyncManager();
}

const network::ChunkSyncManager& ServerCore::chunkSyncManager() const {
    return m_playerManager->chunkSyncManager();
}

// ============================================================================
// 传送管理
// ============================================================================

u32 ServerCore::teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
    return m_teleportManager->requestTeleport(playerId, x, y, z, yaw, pitch);
}

bool ServerCore::confirmTeleport(PlayerId playerId, u32 teleportId) {
    return m_teleportManager->confirmTeleport(playerId, teleportId);
}

// ============================================================================
// 位置更新
// ============================================================================

void ServerCore::updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround) {
    m_positionTracker->updatePosition(playerId, x, y, z, yaw, pitch, onGround);
}

// ============================================================================
// 心跳管理
// ============================================================================

// ============================================================================
// 心跳管理
// ============================================================================

void ServerCore::updateKeepAlive(PlayerId playerId, u64 timestamp) {
    m_keepAliveManager->updateKeepAlive(playerId, timestamp);
}

bool ServerCore::needsKeepAlive(PlayerId playerId, u64 currentTick) const {
    // 将 tick 转换为毫秒
    u64 currentTickMs = currentTick * TICK_DURATION_MS;
    return m_keepAliveManager->needsKeepAlive(playerId, currentTickMs);
}

void ServerCore::recordKeepAliveSent(PlayerId playerId, u64 timestamp) {
    m_keepAliveManager->recordKeepAliveSent(playerId, timestamp, currentTick());
}

// ============================================================================
// 游戏模式管理
// ============================================================================

bool ServerCore::setPlayerGameMode(PlayerId playerId, GameMode mode) {
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player) {
        return false;
    }
    player->gameMode = mode;
    return true;
}

GameMode ServerCore::getPlayerGameMode(PlayerId playerId) const {
    const auto* player = m_playerManager->getPlayer(playerId);
    if (!player) {
        return GameMode::NotSet;
    }
    return player->gameMode;
}

// ============================================================================
// 主循环
// ============================================================================

void ServerCore::tick() {
    MC_TRACE_EVENT("server.tick", "CoreTick");

    // 更新时间
    {
        MC_TRACE_EVENT("server.tick", "TickTime");
        tickTime();
    }

    // 更新天气
    {
        MC_TRACE_EVENT("server.tick", "TickWeather");
        m_weatherManager.tick();
    }

    // 清理断开连接的玩家（每 CLEANUP_INTERVAL_TICKS tick 执行一次）
    if (m_currentCleanupTick % CLEANUP_INTERVAL_TICKS == 0) {
        MC_TRACE_EVENT("server.player", "CleanupDisconnected");
        cleanupDisconnectedPlayers();
    }
    m_currentCleanupTick++;

    // 检查心跳超时（每 KEEPALIVE_CHECK_INTERVAL_TICKS tick 执行一次）
    if (m_currentKeepAliveCheckTick % KEEPALIVE_CHECK_INTERVAL_TICKS == 0) {
        MC_TRACE_EVENT("server.network", "CheckKeepAliveTimeout");
        u64 currentTickMs = currentTick() * TICK_DURATION_MS;
        auto timedOutPlayers = m_keepAliveManager->getTimedOutPlayers(currentTickMs);
        for (PlayerId playerId : timedOutPlayers) {
            spdlog::info("ServerCore: Player {} timed out", playerId);
            disconnectPlayer(playerId, "Connection timed out");
        }
    }
    m_currentKeepAliveCheckTick++;
}

} // namespace mc::server
