#include "IntegratedServer.hpp"
#include "common/item/BlockItem.hpp"
#include "common/item/Items.hpp"
#include "common/item/BlockItemRegistry.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/BlockItemUseContext.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/network/ContainerPacketHandler.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/drop/DropTables.hpp"
#include "common/network/Packet.hpp"
#include "common/network/ChunkSync.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/util/Direction.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/PacketHandler.hpp"

#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>

namespace mr::server {

namespace {

Player& getMenuPlayer() {
    static Player player(0, "IntegratedServerMenu");
    return player;
}

template <typename PacketT>
void sendGamePacket(network::LocalEndpoint* endpoint, network::PacketType packetType, const PacketT& packet) {
    if (endpoint == nullptr || !endpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    packet.serialize(payload);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + payload.size()));
    fullPacket.writeU16(static_cast<u16>(packetType));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(payload.buffer());

    endpoint->send(fullPacket.data(), fullPacket.size());
}

bool isCraftingTableState(const BlockState* state) {
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

class ServerChunkReader final : public IBlockReader {
public:
    explicit ServerChunkReader(ServerChunkManager& chunkManager)
        : m_chunkManager(chunkManager)
    {
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return nullptr;
        }

        const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(x) / 16.0));
        const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(z) / 16.0));
        ChunkData* chunk = m_chunkManager.getChunkSync(chunkX, chunkZ);
        if (chunk == nullptr) {
            return nullptr;
        }

        const i32 localX = x - chunkX * 16;
        const i32 localZ = z - chunkZ * 16;
        return chunk->getBlock(localX, y, localZ);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override
    {
        (void)x;
        (void)z;
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

private:
    ServerChunkManager& m_chunkManager;
};

class IntegratedServerCommandBridge final : public MinecraftServer {
public:
    IntegratedServerCommandBridge(
        i64 seed,
        const std::function<i64()>& ticksProvider,
        const std::function<i64()>& dayProvider,
        const std::function<i64()>& dayTimeProvider,
        const std::function<i64()>& gameTimeProvider,
        const std::function<bool(i64)>& setDayTimeFn,
        const std::function<bool(i64)>& addDayTimeFn,
        const std::function<bool(PlayerId, f64, f64, f64, f32, f32)>& teleportFn,
        const std::function<bool(PlayerId, GameMode)>& setGameModeFn)
        : m_seed(seed)
        , m_ticksProvider(ticksProvider)
        , m_dayProvider(dayProvider)
        , m_dayTimeProvider(dayTimeProvider)
        , m_gameTimeProvider(gameTimeProvider)
        , m_setDayTimeFn(setDayTimeFn)
        , m_addDayTimeFn(addDayTimeFn)
        , m_teleportFn(teleportFn)
        , m_setGameModeFn(setGameModeFn)
    {
    }

    [[nodiscard]] mr::server::ServerWorld* getWorld() override { return nullptr; }
    [[nodiscard]] i64 getSeed() const override { return m_seed; }
    [[nodiscard]] i64 getTicks() const override { return m_ticksProvider ? m_ticksProvider() : 0; }
    [[nodiscard]] i64 getDay() const override { return m_dayProvider ? m_dayProvider() : 0; }
    [[nodiscard]] i64 getDayTime() const override { return m_dayTimeProvider ? m_dayTimeProvider() : 0; }
    [[nodiscard]] i64 getGameTime() const override { return m_gameTimeProvider ? m_gameTimeProvider() : 0; }
    [[nodiscard]] std::vector<ServerPlayer*> getPlayers() override { return {}; }
    [[nodiscard]] ServerPlayer* getPlayer(const String& /*name*/) override { return nullptr; }
    void broadcast(const String& message) override { spdlog::info("[Broadcast] {}", message); }
    bool setDayTime(i64 time) override { return m_setDayTimeFn ? m_setDayTimeFn(time) : false; }
    bool addDayTime(i64 ticks) override { return m_addDayTimeFn ? m_addDayTimeFn(ticks) : false; }
    bool teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) override {
        return m_teleportFn ? m_teleportFn(playerId, x, y, z, yaw, pitch) : false;
    }
    bool setPlayerGameMode(PlayerId playerId, GameMode mode) override {
        return m_setGameModeFn ? m_setGameModeFn(playerId, mode) : false;
    }
    [[nodiscard]] command::CommandRegistry& getCommandRegistry() override { return command::CommandRegistry::getGlobal(); }
    bool isCommandAllowed(const command::ICommandSource& /*source*/, const String& /*command*/) override { return true; }

private:
    i64 m_seed;
    std::function<i64()> m_ticksProvider;
    std::function<i64()> m_dayProvider;
    std::function<i64()> m_dayTimeProvider;
    std::function<i64()> m_gameTimeProvider;
    std::function<bool(i64)> m_setDayTimeFn;
    std::function<bool(i64)> m_addDayTimeFn;
    std::function<bool(PlayerId, f64, f64, f64, f32, f32)> m_teleportFn;
    std::function<bool(PlayerId, GameMode)> m_setGameModeFn;
};

}

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

    // 初始化 ServerCore
    ServerCoreConfig coreConfig;
    coreConfig.viewDistance = config.viewDistance;
    coreConfig.defaultGameMode = config.defaultGameMode;
    coreConfig.seed = static_cast<u64>(config.seed);
    coreConfig.maxPlayers = 1;  // 内置服务器只支持单人
    coreConfig.tickRate = config.tickRate;
    m_serverCore = std::make_unique<ServerCore>(coreConfig);

    // 初始化方块注册表（必须在创建地形生成器之前）
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    Items::initialize();
    DropTableRegistry::instance().initializeVanillaDrops();
    spdlog::info("Vanilla items and drop tables initialized");

    // 初始化方块物品注册表
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    spdlog::info("Block items initialized");

    RecipeLoader recipeLoader;
    auto recipeLoadResult = recipeLoader.loadFromDirectory("data/minecraft/recipes");
    if (recipeLoadResult.failed()) {
        spdlog::warn("Failed to load crafting recipes: {}", recipeLoadResult.error().toString());
    } else {
        spdlog::info("Loaded {} crafting recipes ({} failed)",
                     recipeLoadResult.value().successCount,
                     recipeLoadResult.value().failedCount);
    }

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
    (void)m_chunkManager->initialize();
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

u64 IntegratedServer::tickCount() const noexcept {
    return m_serverCore ? m_serverCore->currentTick() : 0;
}

i64 IntegratedServer::dayTime() const noexcept {
    return m_serverCore ? m_serverCore->gameTime().dayTime() : 0;
}

i64 IntegratedServer::gameTime() const noexcept {
    return m_serverCore ? m_serverCore->gameTime().gameTime() : 0;
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
}

void IntegratedServer::tick() {
    // 如果已停止，跳过处理
    if (!m_running.load()) {
        return;
    }

    // 使用 ServerCore 的 tick
    m_serverCore->tick();

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
    u64 tick = m_serverCore->currentTick();
    if (tick % (static_cast<u64>(m_config.tickRate) * 15) == 0) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        sendKeepAlive(static_cast<u64>(timestamp));
    }

    // 日光周期
    if (m_daylightCycleEnabled) {
        m_serverCore->timeManager().addDayTime(1);
    }
}

void IntegratedServer::shutdown() {
    // 清理区块管理器
    m_chunkManager.reset();

    // 清理 ServerCore
    m_serverCore.reset();

    if (m_connectionPair) {
        m_connectionPair->disconnect();
        m_connectionPair.reset();
    }

    m_serverEndpoint = nullptr;
    m_initialized = false;
}

void IntegratedServer::requestChunkAsync(ChunkCoord x, ChunkCoord z) {
    if (!m_running.load()) return;

    auto* player = getPlayerData();
    if (!player) return;

    ChunkId id(x, z);
    if (player->loadedChunks.count(id)) return;  // 已发送

    if (!m_chunkManager) return;

    // 异步请求区块
    m_chunkManager->getChunkAsync(x, z,
        [this, x, z, id](bool success, ChunkData* chunk) {
            // Worker 线程回调
            if (!m_running.load() || !success || !chunk) return;

            // 检查是否已发送
            auto* p = getPlayerData();
            if (!p || p->loadedChunks.count(id)) return;

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
        auto* player = getPlayerData();
        if (!player || !player->loggedIn) return;

        ChunkId id(send.x, send.z);
        if (player->loadedChunks.count(id)) continue;  // 已发送

        player->loadedChunks.insert(id);

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

    auto* player = getPlayerData();
    if (!player) return;

    ChunkId id(x, z);

    // 检查是否已发送
    if (player->loadedChunks.find(id) != player->loadedChunks.end()) {
        return;
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

        case network::PacketType::BlockInteraction:
            handleBlockInteraction(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::PlayerTryUseItemOnBlock:
            handleBlockPlacement(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::HotbarSelect:
            handleHotbarSelect(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::ContainerClick:
            handleContainerClick(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
            break;

        case network::PacketType::CloseContainer:
            handleCloseContainer(data + network::PACKET_HEADER_SIZE, size - network::PACKET_HEADER_SIZE);
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

    // 创建本地连接
    auto connection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 使用 ServerCore 添加玩家
    m_clientPlayerId = m_serverCore->nextPlayerId();
    auto* player = m_serverCore->addPlayer(m_clientPlayerId, username, connection);

    if (!player) {
        sendLoginResponse(false, 0, username, "Failed to add player");
        return;
    }

    player->loggedIn = true;
    player->gameMode = m_config.defaultGameMode;
    m_clientData.inventory.clear();
    m_clientData.inventory.setSelectedSlot(0);

    if (player->gameMode == GameMode::Creative) {
        i32 slot = 0;
        BlockItemRegistry::instance().forEachBlockItem([this, player, &slot](const BlockItem& item) {
            if (slot >= PlayerInventory::TOTAL_SIZE) {
                return;
            }
            m_clientData.inventory.setItem(slot, ItemStack(item, 64));
            ++slot;
        });
    }

    // 设置初始位置（出生点）
    player->x = 0.0f;
    player->y = 90.0f;
    player->z = 0.0f;

    // 发送登录成功响应
    sendLoginResponse(true, m_clientPlayerId, username, "Welcome to singleplayer world!");

    // 发送初始传送
    u32 teleportId = m_serverCore->teleportPlayer(m_clientPlayerId, player->x, player->y, player->z, player->yaw, player->pitch);
    sendPlayerInventory();

    // 初始化玩家票据位置
    ChunkCoord spawnChunkX = static_cast<ChunkCoord>(std::floor(player->x / 16.0f));
    ChunkCoord spawnChunkZ = static_cast<ChunkCoord>(std::floor(player->z / 16.0f));
    m_lastPlayerChunkX = spawnChunkX;
    m_lastPlayerChunkZ = spawnChunkZ;

    // 在票据管理器中注册玩家位置
    m_ticketManager->updatePlayerPosition(m_clientPlayerId, spawnChunkX, spawnChunkZ);

    spdlog::info("Player '{}' (ID: {}) joined the game at chunk ({}, {})",
                 username, m_clientPlayerId, spawnChunkX, spawnChunkZ);
}

void IntegratedServer::handlePlayerMove(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
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

    switch (packet.type()) {
        case network::PlayerMovePacket::MoveType::Full:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::Position:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            break;
        case network::PlayerMovePacket::MoveType::Rotation:
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::GroundOnly:
            player->onGround = pos.onGround;
            break;
    }

    // 检查是否跨越区块边界
    ChunkCoord newChunkX = static_cast<ChunkCoord>(std::floor(player->x / 16.0f));
    ChunkCoord newChunkZ = static_cast<ChunkCoord>(std::floor(player->z / 16.0f));

    if (newChunkX != m_lastPlayerChunkX || newChunkZ != m_lastPlayerChunkZ) {
        handlePlayerChunkMove(newChunkX, newChunkZ);
        m_lastPlayerChunkX = newChunkX;
        m_lastPlayerChunkZ = newChunkZ;
    }
}

void IntegratedServer::handleBlockInteraction(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn || !m_chunkManager) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::BlockInteractionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block interaction packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    spdlog::info("[Mining] Server received action={} pos=({}, {}, {}) face={} playerPos=({}, {}, {})",
                 static_cast<i32>(packet.action()),
                 packet.x(),
                 packet.y(),
                 packet.z(),
                 static_cast<i32>(packet.face()),
                 player->x,
                 player->y,
                 player->z);

    if (packet.action() != network::BlockInteractionAction::StopDestroyBlock) {
        return;
    }

    if (packet.y() < world::MIN_BUILD_HEIGHT || packet.y() >= world::MAX_BUILD_HEIGHT) {
        return;
    }

    const f64 eyeX = player->x;
    const f64 eyeY = player->y + Player::PLAYER_EYE_HEIGHT;
    const f64 eyeZ = player->z;
    const f64 targetX = static_cast<f64>(packet.x()) + 0.5;
    const f64 targetY = static_cast<f64>(packet.y()) + 0.5;
    const f64 targetZ = static_cast<f64>(packet.z()) + 0.5;
    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    if (distanceSquared > 36.0) {
        spdlog::warn("[Mining] Out-of-range interaction accepted in integrated server: distanceSq={}, target=({}, {}, {}), eye=({}, {}, {})",
                     distanceSquared,
                     packet.x(), packet.y(), packet.z(),
                     eyeX, eyeY, eyeZ);
    }

    ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.x()) / 16.0));
    ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.z()) / 16.0));
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        spdlog::warn("[Mining] Missing chunk for block interaction at chunk ({}, {})", chunkX, chunkZ);
        return;
    }

    const i32 localX = packet.x() - chunkX * 16;
    const i32 localZ = packet.z() - chunkZ * 16;
    const BlockState* state = chunk->getBlock(localX, packet.y(), localZ);
    if (state == nullptr || state->isAir() || state->hardness() < 0.0f) {
        spdlog::warn("[Mining] Block interaction rejected due to invalid target state at ({}, {}, {})",
                     packet.x(), packet.y(), packet.z());
        return;
    }

    Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
    if (!airBlock) {
        spdlog::error("Failed to resolve minecraft:air while destroying block");
        return;
    }

    chunk->setBlock(localX, packet.y(), localZ, &airBlock->defaultState());
    chunk->setDirty(true);
    sendBlockUpdate(packet.x(), packet.y(), packet.z(), airBlock->defaultState().stateId());

    spdlog::info("[Mining] Destroyed block {} at ({}, {}, {})",
                 state->blockLocation().toString(),
                 packet.x(), packet.y(), packet.z());
}

void IntegratedServer::handleBlockPlacement(const u8* data, size_t size)
{
    auto* player = getPlayerData();
    if (!player || !player->loggedIn || !m_chunkManager) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    spdlog::info("[Place] Server received placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
                 packet.x(), packet.y(), packet.z(),
                 static_cast<i32>(packet.face()),
                 packet.hitX(), packet.hitY(), packet.hitZ());

    // 验证距离（最大6格）
    const f64 eyeX = player->x;
    const f64 eyeY = player->y + Player::PLAYER_EYE_HEIGHT;
    const f64 eyeZ = player->z;
    const f64 targetX = static_cast<f64>(packet.x()) + 0.5;
    const f64 targetY = static_cast<f64>(packet.y()) + 0.5;
    const f64 targetZ = static_cast<f64>(packet.z()) + 0.5;
    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    if (distanceSquared > 36.0) {
        spdlog::warn("[Place] Out-of-range placement attempt: distanceSq={:.2f}", distanceSquared);
        return;
    }

    if (player->gameMode == GameMode::Spectator) {
        return;
    }

    // 检查 Y 范围
    if (packet.y() < world::MIN_BUILD_HEIGHT || packet.y() >= world::MAX_BUILD_HEIGHT) {
        spdlog::warn("[Place] Invalid Y position: {}", packet.y());
        return;
    }

    // 计算放置位置（击中面的相邻方块）
    BlockPos clickedPos(packet.x(), packet.y(), packet.z());
    Direction face = packet.face();

    // 检查区块是否已加载
    ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.x()) / 16.0));
    ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(packet.z()) / 16.0));
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        spdlog::warn("[Place] Chunk not loaded at ({}, {})", chunkX, chunkZ);
        return;
    }

    const i32 clickedLocalX = packet.x() - chunkX * 16;
    const i32 clickedLocalZ = packet.z() - chunkZ * 16;
    const BlockState* clickedState = chunk->getBlock(clickedLocalX, packet.y(), clickedLocalZ);
    if (isCraftingTableState(clickedState)) {
        spdlog::info("[Use] Opening crafting table at ({}, {}, {})", packet.x(), packet.y(), packet.z());
        openCraftingTableMenu();
        return;
    }

    ItemStack heldStack = m_clientData.inventory.getSelectedStack();
    if (heldStack.isEmpty()) {
        spdlog::debug("[Place] Selected stack is empty");
        return;
    }

    const Item* heldItem = heldStack.getItem();
    if (heldItem == nullptr) {
        return;
    }

    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId());
    if (blockItem == nullptr) {
        spdlog::debug("[Place] Selected item is not a block item: {}", heldItem->itemLocation().toString());
        return;
    }

    ServerChunkReader worldReader(*m_chunkManager);
    BlockItemUseContext context(worldReader,
                                nullptr,
                                heldStack,
                                packet.hitPosition(),
                                clickedPos,
                                face,
                                player->yaw);

    if (!blockItem->tryPlace(context)) {
        spdlog::debug("[Place] Block item placement validation failed");
        return;
    }

    const BlockPos placePos = context.placementPos();
    const BlockState* newState = blockItem->getStateForPlacement(context);
    if (newState == nullptr) {
        spdlog::debug("[Place] Block item did not provide a placement state");
        return;
    }

    // 检查放置位置的区块是否已加载
    ChunkCoord placeChunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(placePos.x) / 16.0));
    ChunkCoord placeChunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(placePos.z) / 16.0));
    ChunkData* placeChunk = (placeChunkX == chunkX && placeChunkZ == chunkZ) ? chunk
        : m_chunkManager->getChunkSync(placeChunkX, placeChunkZ);

    if (!placeChunk) {
        spdlog::warn("[Place] Target chunk not loaded at ({}, {})", placeChunkX, placeChunkZ);
        return;
    }

    const i32 placeLocalX = placePos.x - placeChunkX * 16;
    const i32 placeLocalZ = placePos.z - placeChunkZ * 16;
    placeChunk->setBlock(placeLocalX, placePos.y, placeLocalZ, newState);
    placeChunk->setDirty(true);

    if (player->gameMode != GameMode::Creative) {
        const i32 selectedSlot = m_clientData.inventory.getSelectedSlot();
        ItemStack updatedStack = m_clientData.inventory.getItem(selectedSlot);
        updatedStack.shrink(1);
        m_clientData.inventory.setItem(selectedSlot, updatedStack);
        sendPlayerInventory();
    }

    // 广播方块更新
    sendBlockUpdate(placePos.x, placePos.y, placePos.z, newState->stateId());

    spdlog::info("[Place] Placed block {} at ({}, {}, {})",
                 blockItem->block().blockLocation().toString(),
                 placePos.x, placePos.y, placePos.z);
}

void IntegratedServer::handleHotbarSelect(const u8* data, size_t size)
{
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = HotbarSelectPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    m_clientData.inventory.setSelectedSlot(result.value().slot());
}

void IntegratedServer::handleContainerClick(const u8* data, size_t size)
{
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    std::unique_ptr<AbstractContainerMenu> openMenu;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        openMenu = m_clientData.openMenu ? std::unique_ptr<AbstractContainerMenu>(m_clientData.openMenu.get()) : nullptr;
    }

    if (!openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = ContainerClickPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse container click packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    if (packet.containerId() != openMenu->getId()) {
        return;
    }

    ClickType clickType = (packet.button() == 0) ? ClickType::Pick : ClickType::PickSome;
    if (packet.action() == ClickAction::QuickMove) {
        clickType = ClickType::QuickMove;
    }

    Player& menuPlayer = getMenuPlayer();
    openMenu->clicked(packet.slotIndex(), packet.button(), clickType, menuPlayer);

    sendContainerContent(*openMenu);
    sendPlayerInventory();
}

void IntegratedServer::handleCloseContainer(const u8* data, size_t size)
{
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    std::unique_ptr<AbstractContainerMenu> openMenu;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        openMenu = std::move(m_clientData.openMenu);
        m_clientData.openContainerType = ContainerType::Player;
    }

    if (!openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    if (result.value().containerId() != openMenu->getId()) {
        return;
    }

    Player& menuPlayer = getMenuPlayer();
    openMenu->removed(menuPlayer);
    sendPlayerInventory();
}

void IntegratedServer::handlePlayerChunkMove(ChunkCoord newChunkX, ChunkCoord newChunkZ) {
    spdlog::debug("Player crossed chunk boundary: ({}, {}) -> ({}, {})",
                  m_lastPlayerChunkX, m_lastPlayerChunkZ, newChunkX, newChunkZ);

    // 更新票据管理器中的玩家位置
    // 这会自动触发区块加载/卸载的计算
    m_ticketManager->updatePlayerPosition(m_clientPlayerId, newChunkX, newChunkZ);
}

void IntegratedServer::onChunkLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    // 如果服务器已停止，跳过
    if (!m_running.load()) {
        return;
    }

    auto* player = getPlayerData();
    if (!player) return;

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
        if (player->loadedChunks.count(id) > 0) {
            sendUnloadChunk(x, z);
            player->loadedChunks.erase(id);
        }
    }
}

void IntegratedServer::handleTeleportConfirm(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
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
                 packet.teleportId(), player->pendingTeleportId);

    if (m_serverCore->confirmTeleport(m_clientPlayerId, packet.teleportId())) {
        // 传送确认后，触发区块加载
        m_ticketManager->processUpdates();
    } else {
        spdlog::warn("Unexpected teleport confirm: id={}, expected={}",
                     packet.teleportId(), player->pendingTeleportId);
    }
}

void IntegratedServer::handleKeepAlive(const u8* data, size_t size) {
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        auto currentTimeMs = static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
        m_serverCore->keepAliveManager().handleKeepAliveResponse(m_clientPlayerId, packet.timestamp(), currentTimeMs);
        spdlog::trace("KeepAlive received: {}", packet.timestamp());
    }
}

void IntegratedServer::handleChatMessage(const u8* data, size_t size) {
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        return;
    }

    auto& packet = result.value();

    if (!packet.message().empty() && packet.message()[0] == '/') {
        auto& registry = command::CommandRegistry::getGlobal();
        IntegratedServerCommandBridge bridge(
            m_config.seed,
            [this]() { return static_cast<i64>(tickCount()); },
            [this]() { return dayTime() / 24000; },
            [this]() { return dayTime(); },
            [this]() { return gameTime(); },
            [this](i64 time) {
                m_serverCore->timeManager().setDayTime(time);
                return true;
            },
            [this](i64 delta) {
                m_serverCore->timeManager().addDayTime(delta);
                return true;
            },
            [this](PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
                auto* p = m_serverCore->getPlayer(playerId);
                if (!p || !p->loggedIn) {
                    return false;
                }

                m_serverCore->teleportPlayer(playerId, x, y, z, yaw, pitch);

                const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(x / 16.0));
                const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(z / 16.0));
                handlePlayerChunkMove(chunkX, chunkZ);
                return true;
            },
            [this](PlayerId playerId, GameMode mode) {
                auto* p = m_serverCore->getPlayer(playerId);
                if (!p || !p->loggedIn) {
                    return false;
                }
                p->gameMode = mode;
                return true;
            });
        command::ServerCommandSource source(
            &bridge,
            nullptr,
            nullptr,
            Vector3d(player->x, player->y, player->z),
            Vector2f(player->yaw, player->pitch),
            4,
            m_clientPlayerId,
            player->username
        );

        auto commandResult = registry.execute(packet.message(), source);
        if (commandResult.failed()) {
            spdlog::warn("Integrated command '{}' failed: {}",
                         packet.message(), commandResult.error().toString());
        } else {
            spdlog::info("Integrated command '{}' executed with result {}",
                         packet.message(), commandResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, packet.message());
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
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
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

    // 记录心跳发送
    if (m_serverCore) {
        m_serverCore->recordKeepAliveSent(m_clientPlayerId, timestamp);
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

void IntegratedServer::sendBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId) {
    network::BlockUpdatePacket packet(x, y, z, blockStateId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::BlockUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendPlayerInventory() {
    PlayerInventoryPacket packet(m_clientData.inventory);
    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerInventory));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendContainerContent(const AbstractContainerMenu& menu) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::ContainerContent,
                   ContainerPacketHandler::createContentPacket(menu));
}

void IntegratedServer::sendOpenContainer(ContainerId containerId, ContainerType type, const String& title, i32 slotCount) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::OpenContainer,
                   ContainerPacketHandler::createOpenContainerPacket(containerId,
                                                                     ContainerTypes::toNetworkType(type),
                                                                     title,
                                                                     slotCount));
}

void IntegratedServer::sendCloseContainer(ContainerId containerId) {
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::CloseContainer,
                   CloseContainerPacket(containerId));
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

void IntegratedServer::openCraftingTableMenu() {
    auto* player = getPlayerData();
    if (!player) return;

    std::unique_ptr<AbstractContainerMenu> existingMenu;
    ContainerId previousId = 0;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        if (m_clientData.openMenu) {
            existingMenu = std::move(m_clientData.openMenu);
            previousId = existingMenu->getId();
        }
    }

    if (existingMenu) {
        Player& menuPlayer = getMenuPlayer();
        existingMenu->removed(menuPlayer);
        sendCloseContainer(previousId);
    }

    ContainerId containerId;
    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        containerId = m_clientData.nextContainerId++;
    }

    auto menu = std::make_unique<CraftingMenu>(containerId, &m_clientData.inventory, nullptr);
    menu->updateResult();

    sendOpenContainer(containerId,
                      ContainerType::CraftingTable,
                      String(ContainerTypes::getDefaultTitle(ContainerType::CraftingTable)),
                      menu->getSlotCount());
    sendContainerContent(*menu);

    {
        std::lock_guard<std::mutex> lock(m_clientDataMutex);
        m_clientData.openContainerType = ContainerType::CraftingTable;
        m_clientData.openMenu = std::move(menu);
    }
}

} // namespace mr::server
