#include <gtest/gtest.h>

#include "common/network/ChunkSync.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mr;
using namespace mr::network;

// ============================================================================
// ChunkView 测试
// ============================================================================

TEST(ChunkView, IsChunkInView) {
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 5;

    // 应该在视距内
    EXPECT_TRUE(view.isChunkInView(0, 0));
    EXPECT_TRUE(view.isChunkInView(5, 0));
    EXPECT_TRUE(view.isChunkInView(-5, 0));
    EXPECT_TRUE(view.isChunkInView(0, 5));
    EXPECT_TRUE(view.isChunkInView(3, 3));

    // 应该在视距外
    EXPECT_FALSE(view.isChunkInView(6, 0));
    EXPECT_FALSE(view.isChunkInView(0, 6));
    EXPECT_FALSE(view.isChunkInView(10, 10));
}

TEST(ChunkView, GetChunksInView) {
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 2;

    auto chunks = view.getChunksInView();

    // 视距2意味着 -2到2 共5x5 = 25个区块
    EXPECT_EQ(chunks.size(), 25u);

    // 验证中心区块存在
    bool hasCenter = false;
    for (const auto& pos : chunks) {
        if (pos.x == 0 && pos.z == 0) {
            hasCenter = true;
            break;
        }
    }
    EXPECT_TRUE(hasCenter);
}

TEST(ChunkView, CalculateChunkDiff) {
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 1;

    // 当前只有 (0,0) 区块
    std::unordered_set<ChunkId> currentChunks;
    currentChunks.insert(ChunkId(0, 0, 0));

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 视距1意味着 -1到1 共3x3 = 9个区块，已加载1个，还需加载8个
    EXPECT_EQ(toLoad.size(), 8u);
    EXPECT_EQ(toUnload.size(), 0u);

    // 玩家移动到 (5, 5)，原区块应卸载
    view.centerX = 5;
    view.centerZ = 5;
    currentChunks.clear();
    currentChunks.insert(ChunkId(0, 0, 0));

    toLoad.clear();
    toUnload.clear();
    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 应加载新区块，卸载旧区块
    EXPECT_EQ(toLoad.size(), 9u);
    EXPECT_EQ(toUnload.size(), 1u);
}

// ============================================================================
// PlayerChunkTracker 测试
// ============================================================================

TEST(PlayerChunkTracker, Construction) {
    PlayerChunkTracker tracker(12345);

    EXPECT_EQ(tracker.playerId(), 12345u);
    EXPECT_EQ(tracker.viewDistance(), 10);  // 默认视距
    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
}

TEST(PlayerChunkTracker, AddRemoveChunk) {
    PlayerChunkTracker tracker(1);

    tracker.addLoadedChunk(10, 20);
    EXPECT_TRUE(tracker.hasChunk(10, 20));
    EXPECT_FALSE(tracker.hasChunk(10, 21));
    EXPECT_EQ(tracker.loadedChunks().size(), 1u);

    tracker.addLoadedChunk(10, 21);
    EXPECT_EQ(tracker.loadedChunks().size(), 2u);

    tracker.removeLoadedChunk(10, 20);
    EXPECT_FALSE(tracker.hasChunk(10, 20));
    EXPECT_EQ(tracker.loadedChunks().size(), 1u);
}

TEST(PlayerChunkTracker, UpdateCenter) {
    PlayerChunkTracker tracker(1);

    tracker.updateCenter(100, 200);

    EXPECT_EQ(tracker.view().centerX, 100);
    EXPECT_EQ(tracker.view().centerZ, 200);
}

TEST(PlayerChunkTracker, SetViewDistance) {
    PlayerChunkTracker tracker(1);

    tracker.setViewDistance(5);
    EXPECT_EQ(tracker.viewDistance(), 5);

    // 边界测试
    tracker.setViewDistance(1);
    EXPECT_EQ(tracker.viewDistance(), 2);  // 最小值2

    tracker.setViewDistance(100);
    EXPECT_EQ(tracker.viewDistance(), 32);  // 最大值32
}

TEST(PlayerChunkTracker, CalculateChunkUpdates) {
    PlayerChunkTracker tracker(1);
    tracker.setViewDistance(2);  // 视距2: 5x5 = 25区块
    tracker.updateCenter(0, 0);

    // 初始状态，没有已加载区块
    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 应加载25个区块 (5x5)
    EXPECT_EQ(toLoad.size(), 25u);
    EXPECT_EQ(toUnload.size(), 0u);

    // 标记中心区块为已加载
    tracker.addLoadedChunk(0, 0);

    toLoad.clear();
    toUnload.clear();
    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 还需加载24个
    EXPECT_EQ(toLoad.size(), 24u);

    // 移动玩家到远处
    tracker.updateCenter(100, 100);

    toLoad.clear();
    toUnload.clear();
    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 原区块应该卸载，新区块应该加载
    EXPECT_EQ(toUnload.size(), 1u);
    EXPECT_EQ(toLoad.size(), 25u);
}

TEST(PlayerChunkTracker, Clear) {
    PlayerChunkTracker tracker(1);

    tracker.addLoadedChunk(0, 0);
    tracker.addLoadedChunk(1, 1);
    tracker.addLoadedChunk(2, 2);

    EXPECT_EQ(tracker.loadedChunks().size(), 3u);

    tracker.clear();

    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
}

// ============================================================================
// ChunkSyncManager 测试
// ============================================================================

TEST(ChunkSyncManager, GetTracker) {
    ChunkSyncManager manager;

    auto tracker1 = manager.getTracker(1);
    EXPECT_NE(tracker1, nullptr);
    EXPECT_EQ(tracker1->playerId(), 1u);

    // 再次获取应该返回相同的tracker
    auto tracker1Again = manager.getTracker(1);
    EXPECT_EQ(tracker1, tracker1Again);
}

TEST(ChunkSyncManager, RemoveTracker) {
    ChunkSyncManager manager;

    auto tracker = manager.getTracker(1);
    EXPECT_NE(tracker, nullptr);

    manager.removeTracker(1);

    // 移除后再获取应该是新的tracker
    auto newTracker = manager.getTracker(1);
    EXPECT_NE(newTracker, tracker);
}

TEST(ChunkSyncManager, UpdatePlayerPosition) {
    ChunkSyncManager manager;

    manager.updatePlayerPosition(1, 100.0, 200.0);

    auto tracker = manager.getTracker(1);
    EXPECT_EQ(tracker->view().centerX, 6);   // 100 / 16 = 6
    EXPECT_EQ(tracker->view().centerZ, 12);  // 200 / 16 = 12
}

TEST(ChunkSyncManager, MarkChunkSent) {
    ChunkSyncManager manager;

    manager.markChunkSent(1, 10, 20);

    auto tracker = manager.getTracker(1);
    EXPECT_TRUE(tracker->hasChunk(10, 20));

    // 订阅者检查
    auto subscribers = manager.getChunkSubscribers(10, 20);
    EXPECT_EQ(subscribers.size(), 1u);
    EXPECT_EQ(subscribers[0], 1u);
}

TEST(ChunkSyncManager, MarkChunkUnloaded) {
    ChunkSyncManager manager;

    manager.markChunkSent(1, 10, 20);
    EXPECT_TRUE(manager.getTracker(1)->hasChunk(10, 20));

    manager.markChunkUnloaded(1, 10, 20);
    EXPECT_FALSE(manager.getTracker(1)->hasChunk(10, 20));

    // 订阅者应该为空
    auto subscribers = manager.getChunkSubscribers(10, 20);
    EXPECT_EQ(subscribers.size(), 0u);
}

TEST(ChunkSyncManager, MultiplePlayers) {
    ChunkSyncManager manager;

    // 两个玩家加载同一区块
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(2, 0, 0);

    auto subscribers = manager.getChunkSubscribers(0, 0);
    EXPECT_EQ(subscribers.size(), 2u);

    // 一个玩家卸载
    manager.markChunkUnloaded(1, 0, 0);
    subscribers = manager.getChunkSubscribers(0, 0);
    EXPECT_EQ(subscribers.size(), 1u);
    EXPECT_EQ(subscribers[0], 2u);
}

TEST(ChunkSyncManager, CalculateUpdates) {
    ChunkSyncManager manager;
    manager.setDefaultViewDistance(2);  // 视距2: 5x5 = 25区块

    // 初始化玩家位置
    manager.updatePlayerPosition(1, 0.0, 0.0);

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    manager.calculateUpdates(1, toLoad, toUnload);

    // 应该有25个区块需要加载
    EXPECT_EQ(toLoad.size(), 25u);
    EXPECT_EQ(toUnload.size(), 0u);
}

TEST(ChunkSyncManager, BlockToChunk) {
    EXPECT_EQ(ChunkSyncManager::blockToChunk(0.0), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(15.9), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(16.0), 1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-1.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-16.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-17.0), -2);
}

// ============================================================================
// ChunkSerializer 测试
// ============================================================================

class ChunkSerializerTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(ChunkSerializerTest, SerializeEmptyChunk) {
    ChunkData chunk(10, -5);

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());
}

TEST_F(ChunkSerializerTest, SerializeChunkWithBlocks) {
    ChunkData chunk(0, 0);

    // 填充一些方块
    auto section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);

    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            section->setBlockStateId(x, 0, z, stoneStateId);
        }
    }

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());

    // 验证大小
    size_t expectedSize = ChunkSerializer::calculateChunkSize(chunk);
    EXPECT_EQ(data.size(), expectedSize);
}

TEST_F(ChunkSerializerTest, DeserializeChunk) {
    // 创建并序列化一个区块
    ChunkData original(100, -200);

    auto section = original.createSection(4);
    ASSERT_NE(section, nullptr);

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
    section->setBlockStateId(5, 5, 5, stoneStateId);
    section->setBlockStateId(10, 10, 10, dirtStateId);

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(100, -200, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto& restored = deserializeResult.value();
    EXPECT_EQ(restored->x(), 100);
    EXPECT_EQ(restored->z(), -200);
    EXPECT_TRUE(restored->isFullyGenerated());
}

TEST_F(ChunkSerializerTest, DeserializeChunkPreservesBiomeData) {
    ChunkData original(3, 7);

    BiomeContainer biomes;
    biomes.setBiome(0, 0, 0, Biomes::Forest);
    biomes.setBiome(3, 3, 3, Biomes::Badlands);
    original.setBiomes(std::move(biomes));

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(3, 7, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    const auto& restored = deserializeResult.value();
    EXPECT_EQ(restored->getBiomeAtBlock(0, 0, 0), Biomes::Forest);
    EXPECT_EQ(restored->getBiomeAtBlock(15, 63, 15), Biomes::Badlands);
}

TEST_F(ChunkSerializerTest, SectionMask) {
    ChunkData chunk(0, 0);

    // 空区块，位掩码应为0
    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, 0);

    // 创建一个非空区块段
    auto section = chunk.createSection(5);
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, (1 << 5));  // 第5位应该被设置
}

TEST_F(ChunkSerializerTest, SectionSize) {
    ChunkSection section;

    size_t size = ChunkSerializer::calculateSectionSize(section);
    // 新格式: 方块数据 (4096 * 4) + 天空光照 (2048) + 方块光照 (2048) + 计数 (2)
    EXPECT_EQ(size, 2 + ChunkSection::VOLUME * 4 + 2048 + 2048);
}
