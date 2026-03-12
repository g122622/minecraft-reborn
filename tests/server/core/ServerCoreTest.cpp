#include <gtest/gtest.h>
#include "server/core/ServerCore.hpp"
#include "server/core/TimeManager.hpp"
#include "common/network/LocalServerConnection.hpp"
#include "common/network/LocalConnection.hpp"
#include <algorithm>

using namespace mc::server;
using namespace mc::network;

/**
 * @brief ServerCore 单元测试
 */
class ServerCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();

        ServerCoreConfig config;
        config.maxPlayers = 10;
        config.viewDistance = 8;
        m_core = std::make_unique<ServerCore>(config);
    }

    void TearDown() override {
        m_core.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection() {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<ServerCore> m_core;
};

TEST_F(ServerCoreTest, DefaultConstruction) {
    EXPECT_EQ(m_core->playerCount(), 0u);
    EXPECT_EQ(m_core->currentTick(), 0u);
}

TEST_F(ServerCoreTest, ConstructionWithConfig) {
    ServerCoreConfig config;
    config.maxPlayers = 20;
    config.viewDistance = 12;
    config.seed = 12345;

    ServerCore core(config);
    EXPECT_EQ(core.config().maxPlayers, 20);
    EXPECT_EQ(core.config().viewDistance, 12);
    EXPECT_EQ(core.config().seed, 12345u);
}

TEST_F(ServerCoreTest, AddPlayer) {
    auto conn = createConnection();
    auto* player = m_core->addPlayer(1, "Steve", conn);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1u);
    EXPECT_EQ(player->username, "Steve");
    EXPECT_TRUE(player->loggedIn);
    EXPECT_EQ(m_core->playerCount(), 1u);
    EXPECT_TRUE(m_core->hasPlayer(1));
}

TEST_F(ServerCoreTest, RemovePlayer) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    m_core->removePlayer(1);
    EXPECT_EQ(m_core->playerCount(), 0u);
    EXPECT_FALSE(m_core->hasPlayer(1));
}

TEST_F(ServerCoreTest, GetPlayer) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    auto* player = m_core->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->username, "Steve");

    const auto& core = *m_core;
    const auto* constPlayer = core.getPlayer(1);
    ASSERT_NE(constPlayer, nullptr);
    EXPECT_EQ(constPlayer->username, "Steve");

    EXPECT_EQ(m_core->getPlayer(999), nullptr);
}

TEST_F(ServerCoreTest, ForEachPlayer) {
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_core->addPlayer(1, "Steve", conn1);
    m_core->addPlayer(2, "Alex", conn2);

    std::vector<mc::PlayerId> ids;
    m_core->forEachPlayer([&ids](mc::server::ServerPlayerData& player) {
        ids.push_back(player.playerId);
    });

    EXPECT_EQ(ids.size(), 2u);
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), mc::PlayerId(1)) != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), mc::PlayerId(2)) != ids.end());
}

TEST_F(ServerCoreTest, NextPlayerId) {
    auto id1 = m_core->nextPlayerId();
    auto id2 = m_core->nextPlayerId();
    auto id3 = m_core->nextPlayerId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
}

TEST_F(ServerCoreTest, Broadcast) {
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_core->addPlayer(1, "Steve", conn1);
    m_core->addPlayer(2, "Alex", conn2);

    std::vector<mc::u8> data = {1, 2, 3};
    // 广播应不会崩溃
    m_core->broadcast(data.data(), data.size());

    // 广播给除玩家1以外的所有玩家
    m_core->broadcastExcept(1, data.data(), data.size());
}

TEST_F(ServerCoreTest, SendPacketToPlayer) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    std::vector<mc::u8> payload = {1, 2, 3};
    EXPECT_TRUE(m_core->sendPacketToPlayer(1, PacketType::KeepAlive, payload));

    // 发送给不存在的玩家应返回 false
    EXPECT_FALSE(m_core->sendPacketToPlayer(999, PacketType::KeepAlive, payload));
}

TEST_F(ServerCoreTest, DisconnectPlayer) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    EXPECT_TRUE(m_core->hasPlayer(1));

    m_core->disconnectPlayer(1, "Test disconnect");

    EXPECT_FALSE(m_core->hasPlayer(1));
}

TEST_F(ServerCoreTest, TickTime) {
    m_core->tickTime();
    EXPECT_EQ(m_core->currentTick(), 1u);
    EXPECT_EQ(m_core->gameTime().gameTime(), 1);

    m_core->tickTime();
    m_core->tickTime();
    EXPECT_EQ(m_core->currentTick(), 3u);
}

TEST_F(ServerCoreTest, TeleportPlayer) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    mc::u32 teleportId = m_core->teleportPlayer(1, 100.0, 64.0, 200.0, 90.0f, 45.0f);
    EXPECT_NE(teleportId, 0u);

    auto* player = m_core->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.0f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.0f);
    EXPECT_TRUE(player->waitingTeleportConfirm);
    EXPECT_EQ(player->pendingTeleportId, teleportId);
}

TEST_F(ServerCoreTest, ConfirmTeleport) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    mc::u32 teleportId = m_core->teleportPlayer(1, 100.0, 64.0, 200.0);

    EXPECT_TRUE(m_core->confirmTeleport(1, teleportId));

    auto* player = m_core->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FALSE(player->waitingTeleportConfirm);
}

TEST_F(ServerCoreTest, UpdatePlayerPosition) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    m_core->updatePlayerPosition(1, 100.0, 64.0, 200.0, 90.0f, 45.0f, true);

    auto* player = m_core->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.0f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.0f);
    EXPECT_FLOAT_EQ(player->yaw, 90.0f);
    EXPECT_FLOAT_EQ(player->pitch, 45.0f);
    EXPECT_TRUE(player->onGround);
}

TEST_F(ServerCoreTest, KeepAlive) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    m_core->recordKeepAliveSent(1, 1000);

    // 超过间隔后需要心跳
    EXPECT_TRUE(m_core->needsKeepAlive(1, 20000)); // 20000 tick = 1000000ms >> 15000ms

    m_core->updateKeepAlive(1, 2000);
}

TEST_F(ServerCoreTest, Tick) {
    auto conn = createConnection();
    m_core->addPlayer(1, "Steve", conn);

    mc::u64 initialTick = m_core->currentTick();
    m_core->tick();

    EXPECT_EQ(m_core->currentTick(), initialTick + 1);
}

TEST_F(ServerCoreTest, ManagerAccess) {
    // 测试管理器访问器
    EXPECT_NO_THROW(m_core->playerManager());
    EXPECT_NO_THROW(m_core->connectionManager());
    EXPECT_NO_THROW(m_core->timeManager());
    EXPECT_NO_THROW(m_core->teleportManager());
    EXPECT_NO_THROW(m_core->keepAliveManager());
    EXPECT_NO_THROW(m_core->positionTracker());
    EXPECT_NO_THROW(m_core->packetHandler());
    EXPECT_NO_THROW(m_core->chunkSyncManager());
}

TEST_F(ServerCoreTest, SetWorld) {
    // 设置 nullptr 世界应该安全
    m_core->setWorld(nullptr);
    EXPECT_EQ(m_core->world(), nullptr);
}

TEST_F(ServerCoreTest, ChunkSyncManager) {
    auto& chunkSync = m_core->chunkSyncManager();
    EXPECT_EQ(chunkSync.defaultViewDistance(), 8);
}

TEST_F(ServerCoreTest, GameTime) {
    m_core->timeManager().setDayTime(6000);
    EXPECT_EQ(m_core->gameTime().dayTime(), 6000);

    m_core->timeManager().setGameTime(10000);
    EXPECT_EQ(m_core->gameTime().gameTime(), 10000);
}
