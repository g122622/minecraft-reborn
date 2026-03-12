#include <gtest/gtest.h>
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "common/network/LocalServerConnection.hpp"
#include "common/network/LocalConnection.hpp"

using namespace mc::server::core;
using namespace mc::network;
using mc::server::ServerCoreConfig;

/**
 * @brief KeepAliveManager 单元测试
 */
class KeepAliveManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
        m_playerManager = std::make_unique<PlayerManager>();
        m_config.keepAliveInterval = 1000;
        m_config.keepAliveTimeout = 5000;
        m_keepAliveManager = std::make_unique<KeepAliveManager>(*m_playerManager, m_config);
    }

    void TearDown() override {
        m_keepAliveManager.reset();
        m_playerManager.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection() {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    ServerCoreConfig m_config;
    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<KeepAliveManager> m_keepAliveManager;
};

TEST_F(KeepAliveManagerTest, NeedsKeepAlive) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn);

    // 初始时应该需要发送心跳
    EXPECT_TRUE(m_keepAliveManager->needsKeepAlive(1, 2000));

    // 记录发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 2000, 40); // tick 40 = 2000ms

    // 刚发送后不应该需要
    EXPECT_FALSE(m_keepAliveManager->needsKeepAlive(1, 2500));

    // 超过间隔后应该需要
    EXPECT_TRUE(m_keepAliveManager->needsKeepAlive(1, 3500));
}

TEST_F(KeepAliveManagerTest, NeedsKeepAliveNonexistentPlayer) {
    EXPECT_FALSE(m_keepAliveManager->needsKeepAlive(999, 0));
}

TEST_F(KeepAliveManagerTest, GetPlayersNeedingKeepAlive) {
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn1);
    m_playerManager->addPlayer(2, "Alex", conn2);

    // 两个玩家都需要心跳
    auto players = m_keepAliveManager->getPlayersNeedingKeepAlive(2000);
    EXPECT_EQ(players.size(), 2u);

    // 只记录玩家1的发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 2000, 40);

    // 只有玩家2需要心跳
    players = m_keepAliveManager->getPlayersNeedingKeepAlive(2500);
    EXPECT_EQ(players.size(), 1u);
    EXPECT_EQ(players[0], 2u);
}

TEST_F(KeepAliveManagerTest, HandleKeepAliveResponse) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn);

    // 记录发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 1000, 20);

    // 处理响应
    m_keepAliveManager->handleKeepAliveResponse(1, 1000, 1050);

    // ping 应该约为 50ms
    mc::u32 ping = m_keepAliveManager->getPlayerPing(1);
    EXPECT_EQ(ping, 50u);
}

TEST_F(KeepAliveManagerTest, HandleKeepAliveResponseWrongTimestamp) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn);

    m_keepAliveManager->recordKeepAliveSent(1, 1000, 20);

    // 使用错误的时间戳
    m_keepAliveManager->handleKeepAliveResponse(1, 2000, 1050);

    // ping 应该还是 0（时间戳不匹配）
    mc::u32 ping = m_keepAliveManager->getPlayerPing(1);
    EXPECT_EQ(ping, 0u);
}

TEST_F(KeepAliveManagerTest, IsTimedOut) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn);

    // 初始时不应该超时（没有接收过心跳）
    EXPECT_FALSE(m_keepAliveManager->isTimedOut(1, 1000));

    // 记录接收时间
    m_keepAliveManager->updateKeepAlive(1, 1000);

    // 超时时间内不应该超时
    EXPECT_FALSE(m_keepAliveManager->isTimedOut(1, 4000));

    // 超过超时时间应该超时
    EXPECT_TRUE(m_keepAliveManager->isTimedOut(1, 7000));
}

TEST_F(KeepAliveManagerTest, GetTimedOutPlayers) {
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn1);
    m_playerManager->addPlayer(2, "Alex", conn2);

    // 记录玩家1的接收时间
    m_keepAliveManager->updateKeepAlive(1, 1000);

    // 玩家2没有记录接收时间，不应超时
    auto timedOut = m_keepAliveManager->getTimedOutPlayers(2000);
    EXPECT_EQ(timedOut.size(), 0u);

    // 超过超时时间
    timedOut = m_keepAliveManager->getTimedOutPlayers(7000);
    EXPECT_EQ(timedOut.size(), 1u);
    EXPECT_EQ(timedOut[0], 1u);
}

TEST_F(KeepAliveManagerTest, UpdateKeepAlive) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, "Steve", conn);

    m_keepAliveManager->updateKeepAlive(1, 1000);
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveReceived(1), 1000u);
}

TEST_F(KeepAliveManagerTest, GetPlayerPingNonexistentPlayer) {
    EXPECT_EQ(m_keepAliveManager->getPlayerPing(999), 0u);
}

TEST_F(KeepAliveManagerTest, GetLastKeepAliveSentNonexistentPlayer) {
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveSent(999), 0u);
}

TEST_F(KeepAliveManagerTest, GetLastKeepAliveReceivedNonexistentPlayer) {
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveReceived(999), 0u);
}
