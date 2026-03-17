#include <gtest/gtest.h>
#include "common/util/NibbleArray.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/engine/LevelBasedGraph.hpp"
#include "common/world/chunk/ChunkPos.hpp"

namespace mc {
namespace {

// ============================================================================
// NibbleArray 测试
// ============================================================================

class NibbleArrayTest : public ::testing::Test {
protected:
    NibbleArray array;
};

TEST_F(NibbleArrayTest, DefaultConstructor) {
    EXPECT_TRUE(array.isEmpty());
}

TEST_F(NibbleArrayTest, FillTest) {
    array.fill(7);
    EXPECT_EQ(array.get(0, 0, 0), 7);
    EXPECT_EQ(array.get(15, 15, 15), 7);
    EXPECT_EQ(array.get(8, 8, 8), 7);
}

TEST_F(NibbleArrayTest, SetAndGet) {
    array.set(0, 0, 0, 5);
    EXPECT_EQ(array.get(0, 0, 0), 5);

    array.set(15, 15, 15, 12);
    EXPECT_EQ(array.get(15, 15, 15), 12);

    array.set(8, 8, 8, 15);
    EXPECT_EQ(array.get(8, 8, 8), 15);
}

TEST_F(NibbleArrayTest, BoundaryValues) {
    // 测试边界值
    array.set(5, 5, 5, 0);
    EXPECT_EQ(array.get(5, 5, 5), 0);

    array.set(5, 5, 5, 15);
    EXPECT_EQ(array.get(5, 5, 5), 15);
}

TEST_F(NibbleArrayTest, CopyTest) {
    array.set(0, 0, 0, 10);
    array.set(5, 5, 5, 7);

    NibbleArray copy = array.copy();
    EXPECT_EQ(copy.get(0, 0, 0), 10);
    EXPECT_EQ(copy.get(5, 5, 5), 7);

    // 修改原数组不影响副本
    array.set(0, 0, 0, 3);
    EXPECT_EQ(copy.get(0, 0, 0), 10);
}

TEST_F(NibbleArrayTest, IndexCalculation) {
    // 测试索引计算是否正确
    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                i32 index = y * 256 + z * 16 + x;
                u8 value = static_cast<u8>(index % 16);
                array.set(x, y, z, value);
            }
        }
    }

    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                i32 index = y * 256 + z * 16 + x;
                u8 expected = static_cast<u8>(index % 16);
                EXPECT_EQ(array.get(x, y, z), expected);
            }
        }
    }
}

// ============================================================================
// SectionPos 测试
// ============================================================================

class SectionPosTest : public ::testing::Test {
protected:
};

TEST_F(SectionPosTest, Construction) {
    SectionPos pos(10, 5, -3);
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 5);
    EXPECT_EQ(pos.z, -3);
}

TEST_F(SectionPosTest, ToLongAndFromLong) {
    SectionPos original(100, -10, 200);
    i64 encoded = original.toLong();
    SectionPos decoded = SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, original.x);
    EXPECT_EQ(decoded.y, original.y);
    EXPECT_EQ(decoded.z, original.z);
}

TEST_F(SectionPosTest, NegativeCoordinates) {
    SectionPos original(-50, -5, -100);
    i64 encoded = original.toLong();
    SectionPos decoded = SectionPos::fromLong(encoded);

    EXPECT_EQ(decoded.x, original.x);
    EXPECT_EQ(decoded.y, original.y);
    EXPECT_EQ(decoded.z, original.z);
}

TEST_F(SectionPosTest, Offset) {
    SectionPos pos(10, 5, 20);

    SectionPos up = pos.offset(Direction::Up);
    EXPECT_EQ(up.x, 10);
    EXPECT_EQ(up.y, 6);
    EXPECT_EQ(up.z, 20);

    SectionPos down = pos.offset(Direction::Down);
    EXPECT_EQ(down.x, 10);
    EXPECT_EQ(down.y, 4);
    EXPECT_EQ(down.z, 20);

    SectionPos north = pos.offset(Direction::North);
    EXPECT_EQ(north.x, 10);
    EXPECT_EQ(north.y, 5);
    EXPECT_EQ(north.z, 19);

    SectionPos south = pos.offset(Direction::South);
    EXPECT_EQ(south.x, 10);
    EXPECT_EQ(south.y, 5);
    EXPECT_EQ(south.z, 21);

    SectionPos west = pos.offset(Direction::West);
    EXPECT_EQ(west.x, 9);
    EXPECT_EQ(west.y, 5);
    EXPECT_EQ(west.z, 20);

    SectionPos east = pos.offset(Direction::East);
    EXPECT_EQ(east.x, 11);
    EXPECT_EQ(east.y, 5);
    EXPECT_EQ(east.z, 20);
}

TEST_F(SectionPosTest, ToColumnLong) {
    SectionPos pos(10, 5, 20);
    i64 columnPos = pos.toColumnLong();

    // 不同Y值但相同X/Z的section应该得到相同的columnPos
    SectionPos pos2(10, 15, 20);
    i64 columnPos2 = pos2.toColumnLong();

    EXPECT_EQ(columnPos, columnPos2);
}

// ============================================================================
// LightType 测试
// ============================================================================

class LightTypeTest : public ::testing::Test {
protected:
};

TEST_F(LightTypeTest, Values) {
    EXPECT_EQ(static_cast<i32>(LightType::SKY), 0);
    EXPECT_EQ(static_cast<i32>(LightType::BLOCK), 1);
}

// ============================================================================
// LevelBasedGraph 测试
// ============================================================================

class SimpleTestGraph : public LevelBasedGraph {
public:
    SimpleTestGraph() : LevelBasedGraph(16, 256, 1024) {}

    // 简单测试：级别从中心向外扩散
    i32 m_rootPos = -1;

    // 公开访问器用于测试
    i32 testGetLevel(i64 pos) const {
        return getLevel(pos);
    }

    void testSetLevel(i64 pos, i32 level) {
        setLevel(pos, level);
    }

    // 公开 propagateLevel 用于测试
    void testPropagateLevel(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing) {
        propagateLevel(fromPos, toPos, newLevel, isDecreasing);
    }

    // 公开 scheduleUpdate 带参数版本用于测试
    void testScheduleUpdate(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing) {
        scheduleUpdate(fromPos, toPos, newLevel, isDecreasing);
    }

protected:
    bool isRoot(i64 pos) const override {
        return pos == m_rootPos;
    }

    i32 computeLevel(i64 pos, i64 excludedSource, i32 level) override {
        (void)excludedSource;

        if (isRoot(pos)) {
            return 0;
        }

        // 找最小邻居级别
        i32 minLevel = 15;

        static const Direction dirs[] = {
            Direction::Down, Direction::Up, Direction::North,
            Direction::South, Direction::West, Direction::East
        };

        for (Direction dir : dirs) {
            i64 neighborPos = offsetPosition(pos, dir);
            if (neighborPos == excludedSource) continue;

            i32 neighborLevel = getLevel(neighborPos);
            if (neighborLevel < minLevel) {
                minLevel = neighborLevel;
            }
        }

        return std::min(level, minLevel + 1);
    }

    void notifyNeighbors(i64 pos, i32 level, bool isDecreasing) override {
        (void)level;
        (void)isDecreasing;

        static const Direction dirs[] = {
            Direction::Down, Direction::Up, Direction::North,
            Direction::South, Direction::West, Direction::East
        };

        for (Direction dir : dirs) {
            i64 neighborPos = offsetPosition(pos, dir);
            scheduleUpdate(neighborPos);
        }
    }

    i32 getLevel(i64 pos) const override {
        auto it = m_levels.find(pos);
        return it != m_levels.end() ? it->second : 15;
    }

    void setLevel(i64 pos, i32 level) override {
        m_levels[pos] = level;
    }

    i32 getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) override {
        (void)fromPos;
        (void)toPos;
        return startLevel + 1;
    }

private:
    std::unordered_map<i64, i32> m_levels;

    static i64 offsetPosition(i64 pos, Direction dir) {
        i32 x = static_cast<i32>((pos >> 38) & 0xFFFFFFF);
        i32 y = static_cast<i32>(pos & 0xFFF);
        i32 z = static_cast<i32>((pos >> 26) & 0xFFFFFFF);

        // 符号扩展
        x = (x << 4) >> 4;
        z = (z << 4) >> 4;

        switch (dir) {
            case Direction::Down:    y--; break;
            case Direction::Up:      y++; break;
            case Direction::North:   z--; break;
            case Direction::South:   z++; break;
            case Direction::West:    x--; break;
            case Direction::East:    x++; break;
            default: break;
        }

        u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFLL);
        u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFLL);
        u64 uy = static_cast<u64>(y) & 0xFFF;
        return (ux << 38) | (uz << 12) | uy;
    }
};

class LevelBasedGraphTest : public ::testing::Test {
protected:
    SimpleTestGraph graph;
};

TEST_F(LevelBasedGraphTest, InitialState) {
    EXPECT_FALSE(graph.needsUpdate());
}

TEST_F(LevelBasedGraphTest, ScheduleUpdate) {
    // scheduleUpdate(pos) 会检查位置是否需要更新
    // 如果位置的级别不会改变（所有邻居都是相同级别），则不会添加到队列

    // 测试：设置一个位置有更低的邻居级别
    graph.testSetLevel(0, 5);   // 位置0级别为5
    graph.testSetLevel(1, 15);  // 位置1级别为15（需要更新）

    // 位置1靠近位置0，所以 computeLevel 会计算出新级别
    // 由于 getEdgeLevel 返回 startLevel + 1，传播到位置1会得到6
    graph.testScheduleUpdate(0, 1, 6, false);

    // 检查是否有待处理更新
    EXPECT_TRUE(graph.needsUpdate());
}

TEST_F(LevelBasedGraphTest, ProcessUpdates) {
    // 测试处理更新的基本流程
    // 当级别变低（光照变亮）时，更新应该生效

    // 设置光源位置
    graph.m_rootPos = 0;  // 位置0是根节点（光源）
    graph.testSetLevel(0, 0);   // 光源级别为0
    graph.testSetLevel(1, 15);  // 邻居初始级别为15

    // 调度位置1的更新
    // 当 scheduleUpdate(1) 被调用时：
    // - fromPos = 1, toPos = 1, newLevel = 15 (levelCount - 1)
    // - computeLevel(1, 1, 15) 会查找邻居最小级别
    // - 位置0的级别是0，所以 minNeighborLevel = 0
    // - computeLevel 返回 min(15, 0 + 1) = 1
    // - 由于 computedLevel(1) < currentLevel(15)，会添加更新
    graph.scheduleUpdate(1);

    // 处理更新
    graph.processUpdates(100);

    // 位置1的级别应该从15变成1
    EXPECT_EQ(graph.testGetLevel(1), 1);
}

TEST_F(LevelBasedGraphTest, Propagation) {
    // 测试传播链
    graph.m_rootPos = 0;  // 位置0是光源

    // 设置初始级别
    graph.testSetLevel(0, 0);   // 光源
    graph.testSetLevel(1, 15);  // 相邻
    graph.testSetLevel(2, 15);  // 下一个

    // 调度位置1的更新
    graph.scheduleUpdate(1);
    graph.processUpdates(100);

    // 位置1级别应该从15变成1
    EXPECT_EQ(graph.testGetLevel(1), 1);

    // 现在调度位置2的更新
    graph.scheduleUpdate(2);
    graph.processUpdates(100);

    // 位置2级别应该从15变成2（邻居位置1的级别是1）
    EXPECT_EQ(graph.testGetLevel(2), 2);
}

// ============================================================================
// Direction 光照相关测试
// ============================================================================

class DirectionLightTest : public ::testing::Test {
protected:
};

TEST_F(DirectionLightTest, FromDelta) {
    EXPECT_EQ(Directions::fromDelta(0, -1, 0), Direction::Down);
    EXPECT_EQ(Directions::fromDelta(0, 1, 0), Direction::Up);
    EXPECT_EQ(Directions::fromDelta(0, 0, -1), Direction::North);
    EXPECT_EQ(Directions::fromDelta(0, 0, 1), Direction::South);
    EXPECT_EQ(Directions::fromDelta(-1, 0, 0), Direction::West);
    EXPECT_EQ(Directions::fromDelta(1, 0, 0), Direction::East);
    EXPECT_EQ(Directions::fromDelta(0, 0, 0), Direction::None);
    EXPECT_EQ(Directions::fromDelta(1, 1, 0), Direction::None);  // 非相邻
}

TEST_F(DirectionLightTest, Offsets) {
    EXPECT_EQ(Directions::xOffset(Direction::Down), 0);
    EXPECT_EQ(Directions::yOffset(Direction::Down), -1);
    EXPECT_EQ(Directions::zOffset(Direction::Down), 0);

    EXPECT_EQ(Directions::xOffset(Direction::Up), 0);
    EXPECT_EQ(Directions::yOffset(Direction::Up), 1);
    EXPECT_EQ(Directions::zOffset(Direction::Up), 0);

    EXPECT_EQ(Directions::xOffset(Direction::East), 1);
    EXPECT_EQ(Directions::yOffset(Direction::East), 0);
    EXPECT_EQ(Directions::zOffset(Direction::East), 0);
}

} // namespace
} // namespace mc
