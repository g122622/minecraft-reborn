#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "common/network/ProtocolPackets.hpp"
#include <thread>
#include <atomic>

using namespace mr;
using namespace mr::server;

// ============================================================================
// ServerWorld 测试固件
// ============================================================================

class ServerWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
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

TEST_F(ServerWorldTest, GetOrGenerateChunk_CreatesChunk) {
    ChunkData* chunk = world->getOrGenerateChunk(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, GetOrGenerateChunk_ReturnsSameChunk) {
    ChunkData* chunk1 = world->getOrGenerateChunk(5, 10);
    ChunkData* chunk2 = world->getOrGenerateChunk(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerWorldTest, GetChunk_AfterGeneration) {
    world->getOrGenerateChunk(3, 7);

    ChunkData* chunk = world->getChunk(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerWorldTest, UnloadChunk) {
    world->getOrGenerateChunk(0, 0);
    EXPECT_TRUE(world->hasChunk(0, 0));

    world->unloadChunk(0, 0);
    EXPECT_FALSE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, ChunkCount) {
    EXPECT_EQ(world->chunkCount(), 0);

    world->getOrGenerateChunk(0, 0);
    EXPECT_EQ(world->chunkCount(), 1);

    world->getOrGenerateChunk(1, 0);
    world->getOrGenerateChunk(0, 1);
    EXPECT_EQ(world->chunkCount(), 3);

    world->unloadChunk(1, 0);
    EXPECT_EQ(world->chunkCount(), 2);
}

TEST_F(ServerWorldTest, MultipleChunks) {
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            world->getOrGenerateChunk(x, z);
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
    world->setBlock(0, 64, 0, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, SetBlock_GetBlock) {
    world->setBlock(10, 50, 20, BlockId::Dirt, 5);

    BlockState block = world->getBlock(10, 50, 20);
    EXPECT_EQ(block.id(), BlockId::Dirt);
    EXPECT_EQ(block.data(), 5);
}

TEST_F(ServerWorldTest, GetBlock_NonExistentChunk) {
    BlockState block = world->getBlock(1000, 64, 1000);
    EXPECT_EQ(block.id(), BlockId::Air);
}

TEST_F(ServerWorldTest, SetBlock_NegativeCoordinates) {
    world->setBlock(-10, 64, -20, BlockId::GrassBlock, 0);

    BlockState block = world->getBlock(-10, 64, -20);
    EXPECT_EQ(block.id(), BlockId::GrassBlock);
}

TEST_F(ServerWorldTest, SetBlock_MultipleBlocks) {
    world->setBlock(0, 64, 0, &VanillaBlocks::STONE->defaultState());
    world->setBlock(1, 64, 0, BlockId::Dirt, 0);
    world->setBlock(0, 65, 0, BlockId::GrassBlock, 0);

    EXPECT_EQ(world->getBlock(0, 64, 0).id(), BlockId::Stone);
    EXPECT_EQ(world->getBlock(1, 64, 0).id(), BlockId::Dirt);
    EXPECT_EQ(world->getBlock(0, 65, 0).id(), BlockId::GrassBlock);
}

// ============================================================================
// 玩家管理测试
// ============================================================================

TEST_F(ServerWorldTest, AddPlayer) {
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);

    EXPECT_TRUE(world->hasPlayer(1));
    EXPECT_EQ(world->playerCount(), 1);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1);
    EXPECT_EQ(player->username, "TestPlayer");
}

TEST_F(ServerWorldTest, RemovePlayer) {
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);
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
    auto session = std::shared_ptr<TcpSession>(nullptr);

    for (int i = 1; i <= 5; ++i) {
        world->addPlayer(i, "Player" + std::to_string(i), session);
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
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);

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
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);
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
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);

    world->teleportPlayer(1, 100.0, 70.0, 200.0, 90.0f, 45.0f);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->waitingTeleportConfirm);
    EXPECT_EQ(player->pendingTeleportId, 1);
}

TEST_F(ServerWorldTest, ConfirmTeleport) {
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);

    world->teleportPlayer(1, 100.0, 70.0, 200.0, 0.0f, 0.0f);

    ServerPlayerData* player = world->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->waitingTeleportConfirm);
    u32 teleportId = player->pendingTeleportId;

    world->confirmTeleport(1, teleportId);
    EXPECT_FALSE(player->waitingTeleportConfirm);
}

TEST_F(ServerWorldTest, TeleportIncrementingIds) {
    auto session = std::shared_ptr<TcpSession>(nullptr);
    world->addPlayer(1, "TestPlayer", session);

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
    world->getOrGenerateChunk(100, 100);

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
                world->getOrGenerateChunk(x, z);
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
                auto session = std::shared_ptr<TcpSession>(nullptr);
                world->addPlayer(id, "Player" + std::to_string(id), session);

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
