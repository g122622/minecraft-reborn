#include <gtest/gtest.h>
#include "common/world/tick/TickPriority.hpp"
#include "common/world/tick/ScheduledTick.hpp"
#include "common/world/tick/ITickList.hpp"
#include "common/world/tick/EmptyTickList.hpp"
#include "common/world/tick/ServerTickList.hpp"
#include "common/world/block/BlockPos.hpp"

using namespace mc::world::tick;

// ============================================================================
// TickPriority Tests
// ============================================================================

TEST(TickPriorityTest, FromIntReturnsCorrectPriority) {
    EXPECT_EQ(fromInt(-3), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(-2), TickPriority::VeryHigh);
    EXPECT_EQ(fromInt(-1), TickPriority::High);
    EXPECT_EQ(fromInt(0), TickPriority::Normal);
    EXPECT_EQ(fromInt(1), TickPriority::Low);
    EXPECT_EQ(fromInt(2), TickPriority::VeryLow);
    EXPECT_EQ(fromInt(3), TickPriority::ExtremelyLow);
}

TEST(TickPriorityTest, FromIntClampsOutOfRange) {
    EXPECT_EQ(fromInt(-100), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(100), TickPriority::ExtremelyLow);
}

TEST(TickPriorityTest, ToIntReturnsCorrectValue) {
    EXPECT_EQ(toInt(TickPriority::ExtremelyHigh), -3);
    EXPECT_EQ(toInt(TickPriority::VeryHigh), -2);
    EXPECT_EQ(toInt(TickPriority::High), -1);
    EXPECT_EQ(toInt(TickPriority::Normal), 0);
    EXPECT_EQ(toInt(TickPriority::Low), 1);
    EXPECT_EQ(toInt(TickPriority::VeryLow), 2);
    EXPECT_EQ(toInt(TickPriority::ExtremelyLow), 3);
}

// ============================================================================
// ScheduledTick Tests
// ============================================================================

class MockTarget {
public:
    int id;
    MockTarget(int i) : id(i) {}
};

TEST(ScheduledTickTest, Construction) {
    mc::BlockPos pos(10, 20, 30);
    MockTarget target(1);

    ScheduledTick<MockTarget> tick(pos, &target, 100, TickPriority::Normal, 1);

    EXPECT_EQ(tick.position.x, 10);
    EXPECT_EQ(tick.position.y, 20);
    EXPECT_EQ(tick.position.z, 30);
    EXPECT_EQ(tick.target, &target);
    EXPECT_EQ(tick.scheduledTick, 100);
    EXPECT_EQ(tick.priority, TickPriority::Normal);
    EXPECT_EQ(tick.tickEntryId, 1);
}

TEST(ScheduledTickTest, ComparisonOrdersByScheduledTick) {
    MockTarget target(1);

    ScheduledTick<MockTarget> tick1(mc::BlockPos(0, 0, 0), &target, 10, TickPriority::Normal, 1);
    ScheduledTick<MockTarget> tick2(mc::BlockPos(0, 0, 0), &target, 20, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, ComparisonOrdersByPriorityWhenSameTick) {
    MockTarget target(1);

    ScheduledTick<MockTarget> tick1(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::High, 1);
    ScheduledTick<MockTarget> tick2(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2);  // High优先级 < Normal优先级
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, ComparisonOrdersByIdWhenSameTickAndPriority) {
    MockTarget target(1);

    ScheduledTick<MockTarget> tick1(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 1);
    ScheduledTick<MockTarget> tick2(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, EqualityBasedOnPositionAndTarget) {
    MockTarget target1(1);
    MockTarget target2(2);

    ScheduledTick<MockTarget> tick1(mc::BlockPos(0, 0, 0), &target1, 10, TickPriority::Normal, 1);
    ScheduledTick<MockTarget> tick2(mc::BlockPos(0, 0, 0), &target1, 20, TickPriority::High, 2);  // Same pos and target
    ScheduledTick<MockTarget> tick3(mc::BlockPos(1, 0, 0), &target1, 10, TickPriority::Normal, 3);  // Different pos
    ScheduledTick<MockTarget> tick4(mc::BlockPos(0, 0, 0), &target2, 10, TickPriority::Normal, 4);  // Different target

    EXPECT_TRUE(tick1 == tick2);  // Same position and target
    EXPECT_FALSE(tick1 == tick3);  // Different position
    EXPECT_FALSE(tick1 == tick4);  // Different target
}

TEST(ScheduledTickTest, HashCodeConsistency) {
    MockTarget target(1);

    ScheduledTick<MockTarget> tick1(mc::BlockPos(0, 0, 0), &target, 10, TickPriority::Normal, 1);
    ScheduledTick<MockTarget> tick2(mc::BlockPos(0, 0, 0), &target, 20, TickPriority::High, 2);

    // Same position and target should have same hash
    EXPECT_EQ(tick1.hashCode(), tick2.hashCode());
}

// ============================================================================
// EmptyTickList Tests
// ============================================================================

TEST(EmptyTickListTest, AllOperationsReturnFalse) {
    EmptyTickList<MockTarget>& tickList = EmptyTickList<MockTarget>::get();

    MockTarget target(1);
    mc::BlockPos pos(0, 0, 0);

    EXPECT_FALSE(tickList.isTickScheduled(pos, target));
    EXPECT_FALSE(tickList.isTickPending(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 0);
}

TEST(EmptyTickListTest, ScheduleDoesNothing) {
    EmptyTickList<MockTarget>& tickList = EmptyTickList<MockTarget>::get();

    MockTarget target(1);
    mc::BlockPos pos(0, 0, 0);

    // Should not throw or do anything
    tickList.scheduleTick(pos, target, 10);
    tickList.scheduleTick(pos, target, 10, TickPriority::High);

    EXPECT_FALSE(tickList.isTickScheduled(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 0);
}

TEST(EmptyTickListTest, SingletonPattern) {
    EmptyTickList<MockTarget>& list1 = EmptyTickList<MockTarget>::get();
    EmptyTickList<MockTarget>& list2 = EmptyTickList<MockTarget>::get();

    EXPECT_EQ(&list1, &list2);
}

// ============================================================================
// ServerTickList Tests
// ============================================================================

// Note: ServerTickList tests would require a mock ServerWorld
// For now, we'll test basic functionality

TEST(ServerTickListTest, Construction) {
    // ServerTickList requires ServerWorld reference, filter, serializer, deserializer, and callback
    // We'll test the basic scheduling and cancellation logic

    // This test would require mocking ServerWorld, which is complex
    // For full integration tests, see the integration test suite
}
