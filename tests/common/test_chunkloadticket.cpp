#include <gtest/gtest.h>
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/world/chunk/ChunkDistanceGraph.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/core/Types.hpp"

using namespace mc;
using namespace mc::world;

// ============================================================================
// ChunkLoadTicket 测试
// ============================================================================

class ChunkLoadTicketTest : public ::testing::Test {
protected:
    void SetUp() override {
        TicketTypes::initializeTicketTypes();
    }
};

TEST_F(ChunkLoadTicketTest, TicketTypeCreation) {
    auto type = ChunkLoadTicketType<ChunkPos>::create("test");
    EXPECT_EQ(type.name(), "test");
    EXPECT_EQ(type.lifespan(), 0u);
}

TEST_F(ChunkLoadTicketTest, TicketTypeWithLifespan) {
    auto type = ChunkLoadTicketType<u32>::create("test_lifespan", 100);
    EXPECT_EQ(type.name(), "test_lifespan");
    EXPECT_EQ(type.lifespan(), 100u);
}

TEST_F(ChunkLoadTicketTest, TicketComparison) {
    // 级别小的优先级高
    ChunkLoadTicket t1(TicketTypes::PLAYER, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::PLAYER, 32, ChunkPos(0, 0));

    // t1 级别低（31 < 32），优先级高
    EXPECT_TRUE(t1.hasHigherPriorityThan(t2));
}

TEST_F(ChunkLoadTicketTest, TicketExpiration) {
    auto type = ChunkLoadTicketType<ChunkPos>::create("test_expire", 100);
    ChunkLoadTicket ticket(type, 31, ChunkPos(0, 0));

    ticket.setTimestamp(0);
    EXPECT_FALSE(ticket.isExpired(50));   // 未过期
    EXPECT_FALSE(ticket.isExpired(100));  // 刚好过期时间
    EXPECT_TRUE(ticket.isExpired(101));   // 已过期
}

TEST_F(ChunkLoadTicketTest, TicketLevel) {
    ChunkLoadTicket ticket(TicketTypes::PLAYER, 31, ChunkPos(5, 10));
    EXPECT_EQ(ticket.level(), 31);
    EXPECT_TRUE(ticket.hasChunkValue());
    EXPECT_EQ(ticket.chunkValue().x, 5);
    EXPECT_EQ(ticket.chunkValue().z, 10);
}

TEST_F(ChunkLoadTicketTest, TicketWithIntValue) {
    ChunkLoadTicket ticket(TicketTypes::POST_TELEPORT, 31, 42u);
    EXPECT_TRUE(ticket.hasIntValue());
    EXPECT_EQ(ticket.intValue(), 42u);
}

// ============================================================================
// ChunkTicketSet 测试
// ============================================================================

TEST(ChunkTicketSetTest, AddAndRemoveTicket) {
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::PLAYER, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));

    set.addTicket(t1);
    EXPECT_EQ(set.size(), 1u);

    set.addTicket(t2);
    EXPECT_EQ(set.size(), 2u);

    // 重复添加不应该增加数量
    set.addTicket(t1);
    EXPECT_EQ(set.size(), 2u);

    set.removeTicket(t1);
    EXPECT_EQ(set.size(), 1u);
}

TEST(ChunkTicketSetTest, MinLevel) {
    ChunkTicketSet set;

    // 空集合返回最大级别
    EXPECT_EQ(set.getMinLevel(), static_cast<i32>(ChunkLoadLevel::MaxLevel));

    set.addTicket(ChunkLoadTicket(TicketTypes::PLAYER, 33, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 33);

    set.addTicket(ChunkLoadTicket(TicketTypes::FORCED, 31, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 31);  // 更小的级别

    set.addTicket(ChunkLoadTicket(TicketTypes::PORTAL, 32, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 31);  // 仍然是最小的
}

TEST(ChunkTicketSetTest, RemoveExpired) {
    auto expireType = ChunkLoadTicketType<ChunkPos>::create("expire", 10);
    auto permanentType = ChunkLoadTicketType<ChunkPos>::create("permanent");

    ChunkTicketSet set;
    ChunkLoadTicket expiring(expireType, 32, ChunkPos(0, 0));
    expiring.setTimestamp(0);

    ChunkLoadTicket permanent(permanentType, 31, ChunkPos(0, 0));

    set.addTicket(expiring);
    set.addTicket(permanent);
    EXPECT_EQ(set.size(), 2u);

    set.removeExpired(5);  // 未过期
    EXPECT_EQ(set.size(), 2u);

    set.removeExpired(20);  // 过期
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(set.getMinLevel(), 31);  // permanent 票据仍然存在
}

// ============================================================================
// ChunkDistanceGraph 测试
// ============================================================================

class ChunkDistanceGraphTest : public ::testing::Test {
protected:
    ChunkDistanceGraph graph;
};

TEST_F(ChunkDistanceGraphTest, InitialLevel) {
    // 未设置的区块级别应为 MAX_LEVEL
    EXPECT_EQ(graph.getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
    EXPECT_EQ(graph.getLevel(100, -50), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkDistanceGraphTest, SetSourceLevel) {
    // 设置源级别
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);

    EXPECT_EQ(graph.getLevel(0, 0), 0);
}

TEST_F(ChunkDistanceGraphTest, LevelPropagation) {
    // 设置源区块级别并传播
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);

    // 检查级别传播
    EXPECT_EQ(graph.getLevel(0, 0), 0);
    EXPECT_EQ(graph.getLevel(1, 0), 1);  // 相邻区块 = 源 + 1
    EXPECT_EQ(graph.getLevel(0, 1), 1);
    EXPECT_EQ(graph.getLevel(-1, 0), 1);
    EXPECT_EQ(graph.getLevel(0, -1), 1);

    // 再远一层
    EXPECT_EQ(graph.getLevel(2, 0), 2);
    EXPECT_EQ(graph.getLevel(1, 1), 2);
}

TEST_F(ChunkDistanceGraphTest, LevelChangeCallback) {
    std::vector<ChunkCoord> xs;
    std::vector<ChunkCoord> zs;
    std::vector<i32> oldLevels;
    std::vector<i32> newLevels;

    graph.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        xs.push_back(x);
        zs.push_back(z);
        oldLevels.push_back(oldLevel);
        newLevels.push_back(newLevel);
    });

    graph.updateSourceLevel(0, 0, 31, true);
    graph.processUpdates(100);

    // 应该有多个级别变化
    EXPECT_FALSE(xs.empty());

    // 检查 (0, 0) 的变化
    bool foundCenter = false;
    for (size_t i = 0; i < xs.size(); ++i) {
        if (xs[i] == 0 && zs[i] == 0) {
            foundCenter = true;
            EXPECT_EQ(oldLevels[i], ChunkDistanceGraph::MAX_LEVEL);
            EXPECT_EQ(newLevels[i], 31);
        }
    }
    EXPECT_TRUE(foundCenter);
}

TEST_F(ChunkDistanceGraphTest, ClearGraph) {
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);
    EXPECT_EQ(graph.getLevel(0, 0), 0);

    graph.clear();
    EXPECT_EQ(graph.getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
    EXPECT_EQ(graph.size(), 0u);
}

// ============================================================================
// PlayerChunkTracker 测试
// ============================================================================

class PlayerChunkTrackerTest : public ::testing::Test {
protected:
    PlayerChunkTracker tracker{10};  // 视距 10
};

TEST_F(PlayerChunkTrackerTest, InitialPosition) {
    EXPECT_EQ(tracker.playerX(), 0);
    EXPECT_EQ(tracker.playerZ(), 0);
    EXPECT_EQ(tracker.viewDistance(), 10);
}

TEST_F(PlayerChunkTrackerTest, SetPlayerPosition) {
    tracker.setPlayerPosition(5, 3);
    EXPECT_EQ(tracker.playerX(), 5);
    EXPECT_EQ(tracker.playerZ(), 3);

    // 票据级别应该更新
    tracker.processUpdates(100);
    i32 level = tracker.getLevel(5, 3);
    EXPECT_LE(level, 31);  // 玩家所在区块应该有较低的级别
}

TEST_F(PlayerChunkTrackerTest, ChunksInRange) {
    tracker.setPlayerPosition(0, 0);

    // 中心区块应该在范围内
    EXPECT_TRUE(tracker.isChunkInRange(0, 0));

    // 视距边缘的区块
    EXPECT_TRUE(tracker.isChunkInRange(10, 0));
    EXPECT_TRUE(tracker.isChunkInRange(0, 10));
    EXPECT_TRUE(tracker.isChunkInRange(-10, 0));

    // 超出视距的区块
    EXPECT_FALSE(tracker.isChunkInRange(11, 0));
    EXPECT_FALSE(tracker.isChunkInRange(0, 11));
}

TEST_F(PlayerChunkTrackerTest, ViewDistanceChange) {
    tracker.setPlayerPosition(0, 0);
    EXPECT_EQ(tracker.viewDistance(), 10);
    EXPECT_TRUE(tracker.isChunkInRange(10, 0));

    tracker.setViewDistance(5);
    EXPECT_EQ(tracker.viewDistance(), 5);
    EXPECT_TRUE(tracker.isChunkInRange(5, 0));
    EXPECT_FALSE(tracker.isChunkInRange(10, 0));  // 现在超出了视距
}

// ============================================================================
// ChunkLoadTicketManager 测试
// ============================================================================

class ChunkLoadTicketManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        TicketTypes::initializeTicketTypes();
    }
};

TEST_F(ChunkLoadTicketManagerTest, PlayerPositionUpdate) {
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10);

    std::vector<ChunkCoord> xs;
    std::vector<ChunkCoord> zs;
    std::vector<i32> oldLevels;
    std::vector<i32> newLevels;

    manager.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        xs.push_back(x);
        zs.push_back(z);
        oldLevels.push_back(oldLevel);
        newLevels.push_back(newLevel);
    });

    // 玩家进入
    manager.updatePlayerPosition(1, 0, 0);

    // 应该触发区块加载
    bool foundLoad = false;
    for (size_t i = 0; i < xs.size(); ++i) {
        if (xs[i] == 0 && zs[i] == 0 && newLevels[i] <= 33) {
            foundLoad = true;
        }
    }
    EXPECT_TRUE(foundLoad);
}

TEST_F(ChunkLoadTicketManagerTest, ChunkUnloadWhenPlayerLeaves) {
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);  // 小视距便于测试

    // 玩家进入 (0, 0)
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 区块 (0, 0) 应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(0, 0));

    // 玩家移动到 (100, 0) - 远离原位置
    manager.updatePlayerPosition(1, 100, 0);
    manager.processUpdates();

    // 区块 (0, 0) 应该卸载
    EXPECT_FALSE(manager.shouldChunkLoad(0, 0));
}

TEST_F(ChunkLoadTicketManagerTest, ForcedChunk) {
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    // 强制加载区块 (100, 100)
    manager.forceChunk(100, 100, true);
    manager.processUpdates();

    // 即使玩家不在这里，区块也应该加载
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));

    // 取消强制加载
    manager.forceChunk(100, 100, false);
    manager.processUpdates();

    // 现在应该卸载（玩家太远）
    EXPECT_FALSE(manager.shouldChunkLoad(100, 100));
}

TEST_F(ChunkLoadTicketManagerTest, MultiplePlayers) {
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 玩家 1 在 (0, 0)
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 玩家 2 在 (20, 0)
    manager.updatePlayerPosition(2, 20, 0);
    manager.processUpdates();

    EXPECT_EQ(manager.playerCount(), 2u);

    // 区块 (0, 0) 和 (20, 0) 都应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(0, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(20, 0));

    // 移除玩家 1
    manager.removePlayer(1);
    manager.processUpdates();

    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST_F(ChunkLoadTicketManagerTest, ViewDistanceChange) {
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 视距5：玩家在(0,0)，区块(5,0)应该在边界上加载
    // 注意：viewDistanceToLevel(5) = 33 - 5 = 28，传播到距离5的区块级别为28+5=33
    // 级别33是Border，应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(5, 0));

    // 区块(6,0)距离为6，超过视距5，应该不加载
    EXPECT_FALSE(manager.shouldChunkLoad(6, 0));

    // 增加视距
    manager.setViewDistance(10);
    manager.processUpdates();

    // 现在区块(6,0)应该在加载范围内
    EXPECT_TRUE(manager.shouldChunkLoad(6, 0));

    // 区块(10,0)应该也在加载范围内
    EXPECT_TRUE(manager.shouldChunkLoad(10, 0));
}

TEST_F(ChunkLoadTicketManagerTest, TicketCount) {
    ChunkLoadTicketManager manager;

    // 初始没有票据
    EXPECT_EQ(manager.totalTicketCount(), 0u);

    // 添加玩家产生票据
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 应该有票据
    EXPECT_GT(manager.totalTicketCount(), 0u);
}

// ============================================================================
// ChunkLoadLevel 测试
// ============================================================================

TEST(ChunkLoadLevelTest, LevelToLoadLevelConversion) {
    EXPECT_EQ(levelToLoadLevel(31), ChunkLoadLevel::Full);
    EXPECT_EQ(levelToLoadLevel(30), ChunkLoadLevel::Full);
    EXPECT_EQ(levelToLoadLevel(32), ChunkLoadLevel::EntityTicking);
    EXPECT_EQ(levelToLoadLevel(33), ChunkLoadLevel::Border);
    EXPECT_EQ(levelToLoadLevel(34), ChunkLoadLevel::Unloaded);
    EXPECT_EQ(levelToLoadLevel(100), ChunkLoadLevel::Unloaded);
}

TEST(ChunkLoadLevelTest, ShouldChunkLoad) {
    EXPECT_TRUE(shouldChunkLoad(31));
    EXPECT_TRUE(shouldChunkLoad(32));
    EXPECT_TRUE(shouldChunkLoad(33));
    EXPECT_FALSE(shouldChunkLoad(34));
    EXPECT_FALSE(shouldChunkLoad(100));
}

TEST(ChunkLoadLevelTest, ViewDistanceToLevel) {
    EXPECT_EQ(viewDistanceToLevel(10), 23);
    EXPECT_EQ(viewDistanceToLevel(5), 28);
    EXPECT_EQ(viewDistanceToLevel(15), 18);
}
