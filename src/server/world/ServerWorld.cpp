#include "ServerWorld.hpp"
#include "ServerChunkManager.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/network/TcpSession.hpp"
#include <chrono>
#include <spdlog/spdlog.h>

namespace mr::server {

// ============================================================================
// ServerWorld 实现
// ============================================================================

ServerWorld::ServerWorld()
    : m_chunkSyncManager()
{
    // 默认创建区块管理器，允许在未显式 initialize() 时也可同步生成区块
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
}

ServerWorld::ServerWorld(const ServerWorldConfig& config)
    : m_config(config)
    , m_chunkSyncManager()
{
    m_chunkSyncManager.setDefaultViewDistance(config.viewDistance);

    // 默认创建区块管理器，允许在未显式 initialize() 时也可同步生成区块
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
}

ServerWorld::~ServerWorld() {
    shutdown();
}

Result<void> ServerWorld::initialize() {
    spdlog::info("Initializing server world with seed {}...", m_config.seed);

    // 若尚未创建区块管理器，按当前配置创建
    if (!m_chunkManager) {
        auto generator = std::make_unique<NoiseChunkGenerator>(
            m_config.seed,
            DimensionSettings::overworld()
        );
        m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
    }

    auto result = m_chunkManager->initialize();
    if (result.failed()) {
        return result;
    }

    m_chunkSyncManager.setDefaultViewDistance(m_config.viewDistance);

    spdlog::info("Server world initialized");
    return Result<void>::ok();
}

void ServerWorld::shutdown() {
    spdlog::info("Shutting down server world...");

    if (m_chunkManager) {
        m_chunkManager->shutdown();
        m_chunkManager.reset();
    }

    std::lock_guard<std::mutex> playerLock(m_playerMutex);
    m_players.clear();

    spdlog::info("Server world shut down");
}

void ServerWorld::setConfig(const ServerWorldConfig& config) {
    m_config = config;
    m_chunkSyncManager.setDefaultViewDistance(config.viewDistance);

    if (m_chunkManager) {
        m_chunkManager->setViewDistance(config.viewDistance);
    }

    std::lock_guard<std::mutex> lock(m_playerMutex);
    for (auto& [playerId, player] : m_players) {
        (void)playerId;
        if (player.chunkTracker) {
            player.chunkTracker->setViewDistance(config.viewDistance);
        }
    }
}

// ============================================================================
// 区块管理
// ============================================================================

ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

const ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) const {
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

bool ServerWorld::hasChunk(ChunkCoord x, ChunkCoord z) const {
    if (m_chunkManager) {
        return m_chunkManager->hasChunk(x, z);
    }
    return false;
}

ChunkData* ServerWorld::getChunkSync(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        return m_chunkManager->getChunkSync(x, z);
    }
    return nullptr;
}

void ServerWorld::unloadChunk(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        m_chunkManager->unloadChunk(x, z);
    }
}

// ============================================================================
// 玩家管理
// ============================================================================

void ServerWorld::addPlayer(PlayerId playerId, const String& username, std::shared_ptr<TcpSession> session) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto& player = m_players[playerId];
    player.playerId = playerId;
    player.username = username;
    player.session = session;
    player.chunkTracker = std::make_shared<network::PlayerChunkTracker>(playerId);
    player.chunkTracker->setViewDistance(m_config.viewDistance);

    // 更新区块同步管理器
    m_chunkSyncManager.getTracker(playerId);
    m_chunkSyncManager.updatePlayerPosition(playerId, player.x, player.z);

    spdlog::info("Player {} ({}) joined the world", username, playerId);
}

void ServerWorld::removePlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    // 通知其他玩家
    network::PlayerDespawnPacket despawnPacket(playerId);
    network::PacketSerializer ser;
    despawnPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerDespawn));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    for (auto& [pid, pdata] : m_players) {
        if (pid != playerId) {
            auto session = pdata.session.lock();
            if (session) {
                // session->send(fullPacket.data(), fullPacket.size());
            }
        }
    }

    // 从区块管理器移除玩家
    if (m_chunkManager) {
        m_chunkManager->removePlayer(playerId);
    }

    String username = it->second.username;
    m_players.erase(it);
    m_chunkSyncManager.removeTracker(playerId);

    spdlog::info("Player {} ({}) left the world", username, playerId);
}

ServerPlayerData* ServerWorld::getPlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

const ServerPlayerData* ServerWorld::getPlayer(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

bool ServerWorld::hasPlayer(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    return m_players.find(playerId) != m_players.end();
}

size_t ServerWorld::playerCount() const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    return m_players.size();
}

// ============================================================================
// 位置更新
// ============================================================================

void ServerWorld::updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround) {
    // 准备区块更新数据（在锁外执行耗时操作）
    std::vector<mr::ChunkPos> chunksToLoad;
    std::vector<mr::ChunkPos> chunksToUnload;
    bool needsChunkUpdate = false;

    {
        std::lock_guard<std::mutex> lock(m_playerMutex);

        auto it = m_players.find(playerId);
        if (it == m_players.end()) return;

        auto& player = it->second;

        ChunkCoord oldChunkX = blockToChunk(player.x);
        ChunkCoord oldChunkZ = blockToChunk(player.z);
        ChunkCoord newChunkX = blockToChunk(x);
        ChunkCoord newChunkZ = blockToChunk(z);

        player.x = x;
        player.y = y;
        player.z = z;
        player.yaw = yaw;
        player.pitch = pitch;
        player.onGround = onGround;

        // 检查是否跨越区块边界
        if (oldChunkX != newChunkX || oldChunkZ != newChunkZ) {
            // 更新区块同步
            m_chunkSyncManager.updatePlayerPosition(playerId, x, z);

            // 更新区块管理器中的玩家位置
            if (m_chunkManager) {
                m_chunkManager->updatePlayerPosition(playerId, x, z);
            }

            // 计算需要发送/卸载的区块
            m_chunkSyncManager.calculateUpdates(playerId, chunksToLoad, chunksToUnload);
            needsChunkUpdate = true;
        }
    }

    // 在锁外发送区块（避免死锁）
    if (needsChunkUpdate) {
        // 发送新区块
        for (const auto& pos : chunksToLoad) {
            sendChunkToPlayer(playerId, pos.x, pos.z);
        }

        // 卸载旧区块
        for (const auto& pos : chunksToUnload) {
            sendUnloadChunkToPlayer(playerId, pos.x, pos.z);
        }
    }

    // 广播玩家移动给其他玩家
    // TODO: 实现玩家移动广播
}

void ServerWorld::confirmTeleport(PlayerId playerId, u32 teleportId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto& player = it->second;

    if (player.waitingTeleportConfirm && player.pendingTeleportId == teleportId) {
        player.waitingTeleportConfirm = false;
        spdlog::debug("Player {} confirmed teleport {}", playerId, teleportId);
    }
}

// ============================================================================
// 传送
// ============================================================================

void ServerWorld::teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto& player = it->second;

    // 生成新的传送ID
    static u32 nextTeleportId = 1;
    u32 teleportId = nextTeleportId++;

    player.pendingTeleportId = teleportId;
    player.waitingTeleportConfirm = true;

    // 发送传送包
    network::TeleportPacket teleportPacket(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    teleportPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::Teleport));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    auto session = player.session.lock();
    if (session) {
        // session->send(fullPacket.data(), fullPacket.size());
    }

    spdlog::debug("Teleporting player {} to ({}, {}, {}), teleportId={}",
                  playerId, x, y, z, teleportId);
}

// ============================================================================
// 区块同步
// ============================================================================

void ServerWorld::sendInitialChunks(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto& player = it->second;

    std::vector<mr::ChunkPos> chunksToLoad;
    std::vector<mr::ChunkPos> chunksToUnload;

    m_chunkSyncManager.calculateUpdates(playerId, chunksToLoad, chunksToUnload);

    // 发送所有需要的区块
    for (const auto& pos : chunksToLoad) {
        sendChunkToPlayer(playerId, pos.x, pos.z);
    }
}

void ServerWorld::sendChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    // 异步获取或生成区块
    m_chunkManager->getChunkAsync(x, z,
        [this, playerId, x, z](bool success, ChunkData* chunk) {
            if (!success || !chunk) {
                spdlog::warn("Failed to generate chunk ({}, {}) for player {}", x, z, playerId);
                return;
            }

            // 序列化区块
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.failed()) {
                spdlog::error("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
                return;
            }

            // 标记为已发送
            m_chunkSyncManager.markChunkSent(playerId, x, z);

            // 发送区块数据包
            network::ChunkDataPacket chunkPacket(x, z, std::move(result.value()));
            network::PacketSerializer ser;
            chunkPacket.serialize(ser);

            network::PacketSerializer fullPacket;
            fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
            fullPacket.writeU16(static_cast<u16>(network::PacketType::ChunkData));
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeBytes(ser.buffer());

            std::lock_guard<std::mutex> lock(m_playerMutex);
            auto it = m_players.find(playerId);
            if (it != m_players.end()) {
                auto session = it->second.session.lock();
                if (session) {
                    // session->send(fullPacket.data(), fullPacket.size());
                }
            }

            spdlog::debug("Sent chunk ({}, {}) to player {}", x, z, playerId);
        });
}

void ServerWorld::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    m_chunkSyncManager.markChunkUnloaded(playerId, x, z);

    network::UnloadChunkPacket unloadPacket(x, z);
    network::PacketSerializer ser;
    unloadPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::UnloadChunk));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        auto session = it->second.session.lock();
        if (session) {
            // session->send(fullPacket.data(), fullPacket.size());
        }
    }
}

// ============================================================================
// 方块操作
// ============================================================================

void ServerWorld::setBlock(i32 x, i32 y, i32 z, const BlockState* state) {
    ChunkCoord chunkX = blockToChunk(static_cast<f64>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f64>(z));

    ChunkData* chunk = getChunkSync(chunkX, chunkZ);
    if (!chunk) return;

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    chunk->setBlock(localX, y, localZ, state);
    chunk->setDirty(true);

    // 广播方块更新
    broadcastBlockUpdate(x, y, z, state ? state->stateId() : 0);
}

const BlockState* ServerWorld::getBlockState(i32 x, i32 y, i32 z) const {
    ChunkCoord chunkX = blockToChunk(static_cast<f64>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f64>(z));

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return nullptr;

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    return chunk->getBlock(localX, y, localZ);
}

void ServerWorld::broadcastBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId) {
    network::BlockUpdatePacket blockPacket(x, y, z, blockStateId);
    network::PacketSerializer ser;
    blockPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::BlockUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    broadcastPacket(fullPacket.buffer());
}

// ============================================================================
// 发送数据包
// ============================================================================

void ServerWorld::sendPacket(PlayerId playerId, const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto session = it->second.session.lock();
    if (session) {
        // session->send(data.data(), data.size());
    }
}

void ServerWorld::broadcastPacket(const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    for (const auto& [playerId, player] : m_players) {
        auto session = player.session.lock();
        if (session) {
            // session->send(data.data(), data.size());
        }
    }
}

void ServerWorld::broadcastPacketExcept(PlayerId excludePlayerId, const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    for (const auto& [playerId, player] : m_players) {
        if (playerId == excludePlayerId) continue;

        auto session = player.session.lock();
        if (session) {
            // session->send(data.data(), data.size());
        }
    }
}

// ============================================================================
// 更新循环
// ============================================================================

void ServerWorld::tick() {
    m_currentTick++;

    // 更新游戏时间
    m_gameTime.tick();

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 每 20 ticks 同步一次时间到客户端
    if (m_currentTick - m_lastTimeSyncTick >= time::TimeConstants::TIME_SYNC_INTERVAL) {
        broadcastTimeUpdate();
        m_lastTimeSyncTick = m_currentTick;
    }

    // 检查区块卸载
    if (m_currentTick - m_lastChunkUnloadCheck > 100) {  // 每100tick检查一次
        checkChunkUnloading();
        m_lastChunkUnloadCheck = m_currentTick;
    }
}

void ServerWorld::checkChunkUnloading() {
    // 区块卸载现在由 ServerChunkManager 处理
}

size_t ServerWorld::chunkCount() const {
    if (m_chunkManager) {
        return m_chunkManager->holderCount();
    }
    return 0;
}

size_t ServerWorld::loadedChunkCount() const {
    if (m_chunkManager) {
        return m_chunkManager->loadedChunkCount();
    }
    return 0;
}

// ============================================================================
// 时间管理
// ============================================================================

void ServerWorld::setDayTime(i64 time) {
    m_gameTime.setDayTime(time);
    // 立即广播时间变更
    broadcastTimeUpdate();
}

void ServerWorld::addDayTime(i64 ticks) {
    m_gameTime.addDayTime(ticks);
    // 立即广播时间变更
    broadcastTimeUpdate();
}

void ServerWorld::setDaylightCycleEnabled(bool enabled) {
    m_gameTime.setDaylightCycleEnabled(enabled);
    // 立即广播
    broadcastTimeUpdate();
}

void ServerWorld::broadcastTimeUpdate() {
    network::TimeUpdatePacket timePacket(
        m_gameTime.gameTime(),
        m_gameTime.dayTime(),
        m_gameTime.daylightCycleEnabled()
    );

    network::PacketSerializer ser;
    timePacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::TimeUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    broadcastPacket(fullPacket.buffer());
}

} // namespace mr::server
