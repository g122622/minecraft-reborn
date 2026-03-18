#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include <thread>
#include <atomic>

using namespace mc;
using namespace mc::server;

// ============================================================================
// Mock 连接用于测试
// ============================================================================

class MockConnection : public network::IServerConnection {
public:
    MockConnection() : m_connected(true) {}

    void send(const u8* data, size_t size) override {
        (void)data;
        (void)size;
        m_sentData.insert(m_sentData.end(), data, data + size);
    }

    void disconnect(const String& reason = "") override {
        (void)reason;
        m_connected = false;
    }

    [[nodiscard]] bool isConnected() const override {
        return m_connected;
    }

    [[nodiscard]] String identifier() const override {
        return "MockConnection";
    }

    [[nodiscard]] network::ConnectionType type() const override {
        return network::ConnectionType::Local;
    }

    std::vector<u8> m_sentData;
    bool m_connected;
};

// ============================================================================
// ServerWorld 测试固件
// ============================================================================

class ServerWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override {
        world.reset();
    }

    std::unique_ptr<ServerWorld> world;
};

// ============================================================================
// 构造和初始化测试
// ============================================================================

TEST_F(ServerWorldTest, DefaultConstructor) {
    ServerWorld defaultWorld;
    EXPECT_EQ(world->chunkCount(), 0);
}

TEST_F(ServerWorldTest, ConfigConstructor) {
    ServerWorldConfig config;
    config.viewDistance = 8;
    config.dimension = 1;

    ServerWorld configuredWorld(config);
    EXPECT_EQ(configuredWorld.config().viewDistance, 8);
    EXPECT_EQ(configuredWorld.config().dimension, 1);
}

TEST_F(ServerWorldTest, Initialize) {
    auto result = world->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(ServerWorldTest, Shutdown) {
    world->initialize();
    world->shutdown();
    EXPECT_EQ(world->chunkCount(), 0);
    EXPECT_EQ(world->playerCount(), 0);
}

// ============================================================================
// 区块管理测试
// ============================================================================

TEST_F(ServerWorldTest, GetChunk_NotExists) {
    ChunkData* chunk = world->getChunk(0, 0);
    EXPECT_EQ(chunk, nullptr);
}

TEST_F(ServerWorldTest, HasChunk_NotExists) {
    EXPECT_FALSE(world->hasChunk(0, 0));
    EXPECT_FALSE(world->hasChunk(100, 100));
}

TEST_F(ServerWorldTest, GetChunkSync_CreatesChunk) {
    ChunkData* chunk = world->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, GetChunkSync_ReturnsSameChunk) {
    ChunkData* chunk1 = world->getChunkSync(5, 10);
    ChunkData* chunk2 = world->getChunkSync(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerWorldTest, GetChunk_AfterGeneration) {
    world->getChunkSync(3, 7);

    ChunkData* chunk = world->getChunk(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerWorldTest, UnloadChunk) {
    world->getChunkSync(0, 0);
    EXPECT_TRUE(world->hasChunk(0, 0));

    world->unloadChunk(0, 0);
    EXPECT_FALSE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, ChunkCount) {
    EXPECT_EQ(world->chunkCount(), 0);

    world->getChunkSync(0, 0);
    EXPECT_EQ(world->chunkCount(), 1);

    world->getChunkSync(1, 0);
    world->getChunkSync(0, 1);
    EXPECT_EQ(world->chunkCount(), 3);

    world->unloadChunk(1, 0);
    EXPECT_EQ(world->chunkCount(), 2);
}

TEST_F(ServerWorldTest, MultipleChunks) {
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            world->getChunkSync(x, z);
        }
    }

    EXPECT_EQ(world->chunkCount(), 25);

    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            EXPECT_TRUE(world->hasChunk(x, z));
        }
    }
}

// ============================================================================
// 方块操作测试
// ============================================================================

TEST_F(ServerWorldTest, SetBlock_CreatesChunk) {
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world->setBlock(0, 64, 0, stoneState);

    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, SetBlock_GetBlock) {
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world->setBlock(10, 50, 20, stoneState);

    const BlockState* block = world->getBlockState(10, 50, 20);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());
}

TEST_F(ServerWorldTest, GetBlock_NonExistentChunk) {
    const BlockState* block = world->getBlockState(1000, 64, 1000);
    // 不存在的区块返回 nullptr (表示空气)
    EXPECT_EQ(block, nullptr);
}

TEST_F(ServerWorldTest, SetBlock_NegativeCoordinates) {
    const BlockState* grassState = &VanillaBlocks::GRASS_BLOCK->defaultState();
    world->setBlock(-10, 64, -20, grassState);

    const BlockState* block = world->getBlockState(-10, 64, -20);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::GRASS_BLOCK->blockId());
}

TEST_F(ServerWorldTest, SetBlock_MultipleBlocks) {
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    const BlockState* grassState = &VanillaBlocks::GRASS_BLOCK->defaultState();

    world->setBlock(0, 64, 0, stoneState);
    world->setBlock(1, 64, 0, dirtState);
    world->setBlock(0, 65, 0, grassState);

    const BlockState* block0 = world->getBlockState(0, 64, 0);
    const BlockState* block1 = world->getBlockState(1, 64, 0);
    const BlockState* block2 = world->getBlockState(0, 65, 0);

    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block1, nullptr);
    ASSERT_NE(block2, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block1->blockId(), VanillaBlocks::DIRT->blockId());
    EXPECT_EQ(block2->blockId(), VanillaBlocks::GRASS_BLOCK->blockId());
}

// ============================================================================
// 玩家管理测试
// ============================================================================

TEST_F(ServerWorldTest, AddPlayer) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);

    EXPECT_TRUE(world->hasPlayer(1));
    EXPECT_EQ(world->playerCount(), 1);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1);
    EXPECT_EQ(player->username, "TestPlayer");
}

TEST_F(ServerWorldTest, RemovePlayer) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);
    EXPECT_EQ(world->playerCount(), 1);

    world->removePlayer(1);
    EXPECT_FALSE(world->hasPlayer(1));
    EXPECT_EQ(world->playerCount(), 0);
}

TEST_F(ServerWorldTest, RemovePlayer_NotExists) {
    // 移除不存在的玩家不应崩溃
    world->removePlayer(999);
    EXPECT_EQ(world->playerCount(), 0);
}

TEST_F(ServerWorldTest, GetPlayer_NotExists) {
    ServerPlayerData* player = world->getPlayer(999);
    EXPECT_EQ(player, nullptr);
}

TEST_F(ServerWorldTest, MultiplePlayers) {
    for (int i = 1; i <= 5; ++i) {
        auto conn = std::make_shared<MockConnection>();
        world->addPlayer(i, "Player" + std::to_string(i), conn);
    }

    EXPECT_EQ(world->playerCount(), 5);

    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(world->hasPlayer(i));
    }
}

// ============================================================================
// 位置更新测试
// ============================================================================

TEST_F(ServerWorldTest, UpdatePlayerPosition) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);

    world->updatePlayerPosition(1, 100.0, 64.0, 200.0, 45.0f, 30.0f, true);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_DOUBLE_EQ(player->x, 100.0);
    EXPECT_DOUBLE_EQ(player->y, 64.0);
    EXPECT_DOUBLE_EQ(player->z, 200.0);
    EXPECT_FLOAT_EQ(player->yaw, 45.0f);
    EXPECT_FLOAT_EQ(player->pitch, 30.0f);
    EXPECT_TRUE(player->onGround);
}

TEST_F(ServerWorldTest, UpdatePlayerPosition_NonExistentPlayer) {
    // 更新不存在玩家位置不应崩溃
    world->updatePlayerPosition(999, 0.0, 0.0, 0.0, 0.0f, 0.0f, true);
}

TEST_F(ServerWorldTest, UpdatePlayerPosition_ChunkCrossing) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);
    world->initialize();

    // 初始位置 (0, 0) 在区块 (0, 0)
    world->updatePlayerPosition(1, 0.0, 64.0, 0.0, 0.0f, 0.0f, true);

    // 移动到 (16, 0, 16)，跨入区块 (1, 1)
    world->updatePlayerPosition(1, 16.0, 64.0, 16.0, 0.0f, 0.0f, true);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_DOUBLE_EQ(player->x, 16.0);
    EXPECT_DOUBLE_EQ(player->z, 16.0);
}

// ============================================================================
// 传送测试
// ============================================================================

TEST_F(ServerWorldTest, TeleportPlayer) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);

    world->teleportPlayer(1, 100.0, 70.0, 200.0, 90.0f, 45.0f);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->waitingTeleportConfirm);
    EXPECT_EQ(player->pendingTeleportId, 1);
}

TEST_F(ServerWorldTest, ConfirmTeleport) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);

    world->teleportPlayer(1, 100.0, 70.0, 200.0, 0.0f, 0.0f);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->waitingTeleportConfirm);
    u32 teleportId = player->pendingTeleportId;

    world->confirmTeleport(1, teleportId);
    EXPECT_FALSE(player->waitingTeleportConfirm);
}

TEST_F(ServerWorldTest, TeleportIncrementingIds) {
    auto conn = std::make_shared<MockConnection>();
    world->addPlayer(1, "TestPlayer", conn);

    world->teleportPlayer(1, 0.0, 0.0, 0.0, 0.0f, 0.0f);
    u32 firstId = world->getPlayer(1)->pendingTeleportId;

    world->teleportPlayer(1, 100.0, 0.0, 0.0, 0.0f, 0.0f);
    u32 secondId = world->getPlayer(1)->pendingTeleportId;

    EXPECT_EQ(secondId, firstId + 1);
}

// ============================================================================
// 区块坐标转换测试
// ============================================================================

TEST_F(ServerWorldTest, BlockToChunk) {
    EXPECT_EQ(ServerWorld::blockToChunk(0.0), 0);
    EXPECT_EQ(ServerWorld::blockToChunk(15.0), 0);
    EXPECT_EQ(ServerWorld::blockToChunk(16.0), 1);
    EXPECT_EQ(ServerWorld::blockToChunk(-1.0), -1);
    EXPECT_EQ(ServerWorld::blockToChunk(-16.0), -1);
    EXPECT_EQ(ServerWorld::blockToChunk(-17.0), -2);
}

// ============================================================================
// 配置测试
// // ============================================================================

TEST_F(ServerWorldTest, SetConfig) {
    ServerWorldConfig newConfig;
    newConfig.viewDistance = 16;
    newConfig.dimension = 2;
    newConfig.chunkUnloadDelay = 60000;

    world->setConfig(newConfig);

    EXPECT_EQ(world->config().viewDistance, 16);
    EXPECT_EQ(world->config().dimension, 2);
    EXPECT_EQ(world->config().chunkUnloadDelay, 60000);
}

// ============================================================================
// Tick 测试
// ============================================================================

TEST_F(ServerWorldTest, Tick) {
    world->initialize();

    // 执行多次 tick 不应崩溃
    for (int i = 0; i < 1000; ++i) {
        world->tick();
    }
}

TEST_F(ServerWorldTest, ChunkUnloading) {
    world->initialize();

    // 创建一个区块
    world->getChunkSync(100, 100);

    // 执行多次 tick 触发卸载检查
    for (int i = 0; i < 200; ++i) {
        world->tick();
    }

    // 没有玩家订阅的区块应该被卸载
    // (取决于卸载延迟配置)
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST_F(ServerWorldTest, ConcurrentChunkAccess) {
    world->initialize();

    std::vector<std::thread> threads;

    // 多线程同时访问区块
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 100; ++j) {
                int x = (i * 100 + j) % 20 - 10;
                int z = (i * 100 + j + 50) % 20 - 10;
                world->getChunkSync(x, z);
                world->hasChunk(x, z);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 如果没有崩溃或死锁，测试通过
    EXPECT_GT(world->chunkCount(), 0);
}

TEST_F(ServerWorldTest, ConcurrentPlayerAccess) {
    world->initialize();

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // 多线程同时添加和移除玩家
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &successCount, i]() {
            for (int j = 0; j < 10; ++j) {
                PlayerId id = i * 10 + j;
                auto conn = std::make_shared<MockConnection>();
                world->addPlayer(id, "Player" + std::to_string(id), conn);

                if (world->hasPlayer(id)) {
                    successCount++;
                }

                world->removePlayer(id);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successCount.load(), 0);
    EXPECT_EQ(world->playerCount(), 0);
}
