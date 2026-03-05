#include <gtest/gtest.h>
#include <cmath>
#include "../src/common/util/AxisAlignedBB.hpp"
#include "../src/common/physics/collision/CollisionShape.hpp"
#include "../src/common/physics/collision/BlockCollision.hpp"
#include "../src/common/physics/PhysicsEngine.hpp"
#include "../src/common/entity/Entity.hpp"
#include "../src/common/entity/Player.hpp"

using namespace mr;

// ============================================================================
// AxisAlignedBB 测试
// ============================================================================

class AxisAlignedBBTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(AxisAlignedBBTest, Constructor) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 2.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 3.0f);
}

TEST_F(AxisAlignedBBTest, ConstructorAutoSort) {
    AxisAlignedBB box(1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f);

    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 2.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 3.0f);
}

TEST_F(AxisAlignedBBTest, FromPosition) {
    Vector3 pos(10.0f, 20.0f, 30.0f);
    AxisAlignedBB box = AxisAlignedBB::fromPosition(pos, 0.6f, 1.8f);

    EXPECT_NEAR(box.minX, 9.7f, 0.001f);
    EXPECT_NEAR(box.minY, 20.0f, 0.001f);
    EXPECT_NEAR(box.minZ, 29.7f, 0.001f);
    EXPECT_NEAR(box.maxX, 10.3f, 0.001f);
    EXPECT_NEAR(box.maxY, 21.8f, 0.001f);
    EXPECT_NEAR(box.maxZ, 30.3f, 0.001f);
    EXPECT_NEAR(box.width(), 0.6f, 0.001f);
    EXPECT_NEAR(box.height(), 1.8f, 0.001f);
}

TEST_F(AxisAlignedBBTest, FromBlock) {
    AxisAlignedBB box = AxisAlignedBB::fromBlock(5, 10, 15);

    EXPECT_FLOAT_EQ(box.minX, 5.0f);
    EXPECT_FLOAT_EQ(box.minY, 10.0f);
    EXPECT_FLOAT_EQ(box.minZ, 15.0f);
    EXPECT_FLOAT_EQ(box.maxX, 6.0f);
    EXPECT_FLOAT_EQ(box.maxY, 11.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 16.0f);
}

TEST_F(AxisAlignedBBTest, Dimensions) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_FLOAT_EQ(box.width(), 2.0f);
    EXPECT_FLOAT_EQ(box.height(), 3.0f);
    EXPECT_FLOAT_EQ(box.depth(), 4.0f);
    EXPECT_FLOAT_EQ(box.volume(), 24.0f);
}

TEST_F(AxisAlignedBBTest, Center) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 2.0f, 4.0f, 6.0f);
    Vector3 c = box.center();

    EXPECT_FLOAT_EQ(c.x, 1.0f);
    EXPECT_FLOAT_EQ(c.y, 2.0f);
    EXPECT_FLOAT_EQ(c.z, 3.0f);
}

TEST_F(AxisAlignedBBTest, Intersects) {
    AxisAlignedBB box1(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    AxisAlignedBB box2(0.5f, 0.5f, 0.5f, 1.5f, 1.5f, 1.5f);
    EXPECT_TRUE(box1.intersects(box2));
    EXPECT_TRUE(box2.intersects(box1));

    AxisAlignedBB box3(1.1f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f);
    EXPECT_FALSE(box1.intersects(box3));

    AxisAlignedBB box4(1.0f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f);
    EXPECT_FALSE(box1.intersects(box4));
}

TEST_F(AxisAlignedBBTest, ContainsPoint) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(box.contains(Vector3(0.5f, 0.5f, 0.5f)));
    EXPECT_TRUE(box.contains(Vector3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(box.contains(Vector3(1.0f, 1.0f, 1.0f)));
    EXPECT_FALSE(box.contains(Vector3(1.1f, 0.5f, 0.5f)));
    EXPECT_FALSE(box.contains(Vector3(-0.1f, 0.5f, 0.5f)));
}

TEST_F(AxisAlignedBBTest, Offset) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    box.offset(1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(box.minX, 1.0f);
    EXPECT_FLOAT_EQ(box.minY, 2.0f);
    EXPECT_FLOAT_EQ(box.minZ, 3.0f);
    EXPECT_FLOAT_EQ(box.maxX, 2.0f);
    EXPECT_FLOAT_EQ(box.maxY, 3.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 4.0f);
}

TEST_F(AxisAlignedBBTest, Offsetted) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    AxisAlignedBB offsetted = box.offsetted(1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);

    EXPECT_FLOAT_EQ(offsetted.minX, 1.0f);
    EXPECT_FLOAT_EQ(offsetted.minY, 2.0f);
    EXPECT_FLOAT_EQ(offsetted.minZ, 3.0f);
    EXPECT_FLOAT_EQ(offsetted.maxX, 2.0f);
    EXPECT_FLOAT_EQ(offsetted.maxY, 3.0f);
    EXPECT_FLOAT_EQ(offsetted.maxZ, 4.0f);
}

TEST_F(AxisAlignedBBTest, ExpandAndGrow) {
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    AxisAlignedBB expanded = box.expand(0.5f, 1.0f, 0.25f);
    EXPECT_FLOAT_EQ(expanded.minX, -0.5f);
    EXPECT_FLOAT_EQ(expanded.minY, -1.0f);
    EXPECT_FLOAT_EQ(expanded.minZ, -0.25f);
    EXPECT_FLOAT_EQ(expanded.maxX, 1.5f);
    EXPECT_FLOAT_EQ(expanded.maxY, 2.0f);
    EXPECT_FLOAT_EQ(expanded.maxZ, 1.25f);

    AxisAlignedBB grown = box.grow(0.5f);
    EXPECT_FLOAT_EQ(grown.minX, -0.5f);
    EXPECT_FLOAT_EQ(grown.maxX, 1.5f);
}

TEST_F(AxisAlignedBBTest, CalculateXOffsetPositive) {
    AxisAlignedBB entity(0.0f, 0.0f, 0.0f, 0.6f, 1.8f, 0.6f);
    AxisAlignedBB block(1.0f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f);

    f32 offset = entity.calculateXOffset(block, 5.0f);
    EXPECT_FLOAT_EQ(offset, 0.4f);
}

TEST_F(AxisAlignedBBTest, CalculateXOffsetNegative) {
    AxisAlignedBB entity(2.0f, 0.0f, 0.0f, 2.6f, 1.8f, 0.6f);
    AxisAlignedBB block(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    f32 offset = entity.calculateXOffset(block, -5.0f);
    EXPECT_FLOAT_EQ(offset, -1.0f);
}

TEST_F(AxisAlignedBBTest, CalculateYOffsetDown) {
    // 实体在方块上方，向下移动
    AxisAlignedBB entity(0.0f, 1.0f, 0.0f, 0.6f, 2.8f, 0.6f); // 脚底在y=1
    AxisAlignedBB ground(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);  // 地面顶部在y=1

    // 向下移动，应该停在地面顶部
    f32 offset = entity.calculateYOffset(ground, -2.0f);
    EXPECT_FLOAT_EQ(offset, 0.0f); // 已经接触，不能移动
}

TEST_F(AxisAlignedBBTest, CalculateYOffsetFalling) {
    // 实体在空中，下落到地面
    AxisAlignedBB entity(0.0f, 5.0f, 0.0f, 0.6f, 6.8f, 0.6f); // 脚底在y=5
    AxisAlignedBB ground(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    // 向下移动10格，应该停在地面顶部
    f32 offset = entity.calculateYOffset(ground, -10.0f);
    EXPECT_FLOAT_EQ(offset, -4.0f); // 5.0 - 1.0 = 4.0
}

TEST_F(AxisAlignedBBTest, CalculateYOffsetCeiling) {
    AxisAlignedBB entity(0.0f, 0.0f, 0.0f, 0.6f, 1.8f, 0.6f);
    AxisAlignedBB ceiling(0.0f, 2.5f, 0.0f, 1.0f, 3.5f, 1.0f);

    f32 offset = entity.calculateYOffset(ceiling, 2.0f);
    EXPECT_FLOAT_EQ(offset, 0.7f);
}

TEST_F(AxisAlignedBBTest, CalculateZOffset) {
    AxisAlignedBB entity(0.0f, 0.0f, 0.0f, 0.6f, 1.8f, 0.6f);
    AxisAlignedBB block(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 2.0f);

    f32 offset = entity.calculateZOffset(block, 5.0f);
    EXPECT_FLOAT_EQ(offset, 0.4f);
}

TEST_F(AxisAlignedBBTest, NoIntersectionNoEffect) {
    AxisAlignedBB entity(0.0f, 0.0f, 0.0f, 0.6f, 1.8f, 0.6f);
    AxisAlignedBB farBlock(10.0f, 0.0f, 0.0f, 11.0f, 1.0f, 1.0f);

    AxisAlignedBB highBlock(0.0f, 10.0f, 0.0f, 1.0f, 11.0f, 1.0f);
    f32 offset = entity.calculateXOffset(highBlock, 5.0f);
    EXPECT_FLOAT_EQ(offset, 5.0f);

    f32 offset2 = entity.calculateXOffset(farBlock, 5.0f);
    EXPECT_FLOAT_EQ(offset2, 5.0f);
}

// ============================================================================
// CollisionShape 测试
// ============================================================================

class CollisionShapeTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(CollisionShapeTest, EmptyShape) {
    CollisionShape shape = CollisionShape::empty();

    EXPECT_TRUE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::Empty);
    EXPECT_EQ(shape.boxCount(), 0u);
}

TEST_F(CollisionShapeTest, FullBlockShape) {
    CollisionShape shape = CollisionShape::fullBlock();

    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::FullBlock);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.maxY, 1.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(CollisionShapeTest, SimpleBoxShape) {
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f);

    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::SimpleBox);
    ASSERT_EQ(shape.boxCount(), 1u);

    const auto& box = shape.boxes()[0];
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 0.5f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.maxY, 0.5f);
}

TEST_F(CollisionShapeTest, MultiBoxShape) {
    CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    shape.addBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);

    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
    EXPECT_EQ(shape.type(), CollisionShape::Type::SimpleBox);
    EXPECT_EQ(shape.boxCount(), 2u);
}

TEST_F(CollisionShapeTest, GetWorldBoxes) {
    CollisionShape shape = CollisionShape::fullBlock();
    auto worldBoxes = shape.getWorldBoxes(10, 20, 30);

    ASSERT_EQ(worldBoxes.size(), 1u);
    EXPECT_FLOAT_EQ(worldBoxes[0].minX, 10.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxX, 11.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].minY, 20.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxY, 21.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].minZ, 30.0f);
    EXPECT_FLOAT_EQ(worldBoxes[0].maxZ, 31.0f);
}

TEST_F(CollisionShapeTest, IntersectsWithEntity) {
    CollisionShape shape = CollisionShape::fullBlock();

    AxisAlignedBB entity1(0.5f, 0.5f, 0.5f, 1.5f, 1.5f, 1.5f);
    EXPECT_TRUE(shape.intersects(entity1, 0, 0, 0));

    AxisAlignedBB entity2(1.1f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f);
    EXPECT_FALSE(shape.intersects(entity2, 0, 0, 0));
}

// ============================================================================
// BlockCollisionRegistry 测试
// ============================================================================

class BlockCollisionRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockCollisionRegistry::instance().initialize();
    }
};

TEST_F(BlockCollisionRegistryTest, GetShapeForAir) {
    CollisionShape shape = BlockCollisionRegistry::instance().getShapeById(BlockId::Air);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(BlockCollisionRegistryTest, GetShapeForStone) {
    CollisionShape shape = BlockCollisionRegistry::instance().getShapeById(BlockId::Stone);
    EXPECT_TRUE(shape.isFullBlock());
}

TEST_F(BlockCollisionRegistryTest, GetShapeForWater) {
    CollisionShape shape = BlockCollisionRegistry::instance().getShapeById(BlockId::Water);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(BlockCollisionRegistryTest, GetShapeForBlockState) {
    BlockState stoneState(BlockId::Stone, 0);
    CollisionShape shape = BlockCollisionRegistry::instance().getShape(stoneState);
    EXPECT_TRUE(shape.isFullBlock());
}

TEST_F(BlockCollisionRegistryTest, HasCollision) {
    EXPECT_FALSE(BlockCollisionRegistry::instance().hasCollision(BlockState(BlockId::Air)));
    EXPECT_TRUE(BlockCollisionRegistry::instance().hasCollision(BlockState(BlockId::Stone)));
    EXPECT_FALSE(BlockCollisionRegistry::instance().hasCollision(BlockState(BlockId::Water)));
}

TEST_F(BlockCollisionRegistryTest, CacheShapes) {
    const CollisionShape& empty = BlockCollisionRegistry::instance().emptyShape();
    const CollisionShape& full = BlockCollisionRegistry::instance().fullBlockShape();

    EXPECT_TRUE(empty.isEmpty());
    EXPECT_TRUE(full.isFullBlock());
}

// ============================================================================
// PhysicsEngine 测试
// ============================================================================

class MockCollisionWorld : public ICollisionWorld {
public:
    MockCollisionWorld() {
        for (int x = -10; x <= 10; ++x) {
            for (int z = -10; z <= 10; ++z) {
                groundBlocks.insert({x, z});
            }
        }
    }

    [[nodiscard]] BlockState getBlock(i32 x, i32 y, i32 z) const override {
        if (y == 0 && groundBlocks.count({x, z})) {
            return BlockState(BlockId::Stone, 0);
        }
        return BlockState(BlockId::Air, 0);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= -64 && y < 320;
    }

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord, ChunkCoord) const override {
        return nullptr;
    }

    void setGroundBlock(i32 x, i32 z, bool hasGround) {
        if (hasGround) {
            groundBlocks.insert({x, z});
        } else {
            groundBlocks.erase({x, z});
        }
    }

private:
    std::set<std::pair<i32, i32>> groundBlocks;
};

class PhysicsEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockCollisionRegistry::instance().initialize();
        world = std::make_unique<MockCollisionWorld>();
        engine = std::make_unique<PhysicsEngine>(*world);
    }

    std::unique_ptr<MockCollisionWorld> world;
    std::unique_ptr<PhysicsEngine> engine;
};

TEST_F(PhysicsEngineTest, IsOnGround) {
    AxisAlignedBB playerOnGround = AxisAlignedBB::fromPosition(Vector3(0.0f, 0.0f, 0.0f), 0.6f, 1.8f);
    EXPECT_TRUE(engine->isOnGround(playerOnGround));

    AxisAlignedBB playerInAir = AxisAlignedBB::fromPosition(Vector3(0.0f, 5.0f, 0.0f), 0.6f, 1.8f);
    EXPECT_FALSE(engine->isOnGround(playerInAir));
}

TEST_F(PhysicsEngineTest, MoveEntityNoCollision) {
    // 在空中移动（无地面）
    for (int x = -10; x <= 10; ++x) {
        for (int z = -10; z <= 10; ++z) {
            world->setGroundBlock(x, z, false);
        }
    }

    AxisAlignedBB player = AxisAlignedBB::fromPosition(Vector3(0.0f, 10.0f, 0.0f), 0.6f, 1.8f);
    Vector3 movement(1.0f, 0.0f, 0.0f);

    Vector3 actual = engine->moveEntity(player, movement, 0.6f);

    EXPECT_FLOAT_EQ(actual.x, 1.0f);
    EXPECT_FLOAT_EQ(actual.y, 0.0f);
    EXPECT_FLOAT_EQ(actual.z, 0.0f);
}

TEST_F(PhysicsEngineTest, MoveEntityIntoWall) {
    for (int y = 0; y <= 3; ++y) {
        (void)y;
        world->setGroundBlock(2, 0, true);
        world->setGroundBlock(2, 1, true);
    }

    AxisAlignedBB player = AxisAlignedBB::fromPosition(Vector3(0.0f, 0.0f, 0.0f), 0.6f, 1.8f);
    Vector3 movement(5.0f, 0.0f, 0.0f);

    Vector3 actual = engine->moveEntity(player, movement, 0.6f);

    EXPECT_LT(actual.x, 1.5f);
    EXPECT_GT(actual.x, 0.0f);
}

TEST_F(PhysicsEngineTest, GravityAndGround) {
    AxisAlignedBB player = AxisAlignedBB::fromPosition(Vector3(0.0f, 5.0f, 0.0f), 0.6f, 1.8f);

    // 下落一步
    Vector3 movement1 = engine->moveEntity(player, Vector3(0.0f, -1.0f, 0.0f), 0.6f);
    EXPECT_LT(movement1.y, 0.0f); // 应该向下移动

    // 继续下落到地面（最多50次迭代以避免死循环）
    int iterations = 0;
    while (player.minY > 1.1f && iterations < 50) {
        engine->moveEntity(player, Vector3(0.0f, -1.0f, 0.0f), 0.6f);
        iterations++;
    }

    // 玩家应该停在地面顶部（y=1附近）
    EXPECT_LT(player.minY, 1.5f);
    EXPECT_GT(player.minY, 0.5f);
    EXPECT_LT(iterations, 50); // 确保没有超时
}

TEST_F(PhysicsEngineTest, CollectCollisionBoxes) {
    AxisAlignedBB searchBox(-1.0f, -1.0f, -1.0f, 2.0f, 2.0f, 2.0f);
    std::vector<AxisAlignedBB> boxes;
    engine->collectCollisionBoxes(searchBox, boxes);

    EXPECT_FALSE(boxes.empty());
}

TEST_F(PhysicsEngineTest, NoCollisionWhenEmpty) {
    for (int x = -10; x <= 10; ++x) {
        for (int z = -10; z <= 10; ++z) {
            world->setGroundBlock(x, z, false);
        }
    }

    AxisAlignedBB player = AxisAlignedBB::fromPosition(Vector3(0.0f, 100.0f, 0.0f), 0.6f, 1.8f);
    Vector3 movement(0.0f, -10.0f, 0.0f);

    Vector3 actual = engine->moveEntity(player, movement, 0.6f);

    EXPECT_FLOAT_EQ(actual.y, -10.0f);
}

// ============================================================================
// Entity 物理集成测试
// ============================================================================

class EntityPhysicsTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockCollisionRegistry::instance().initialize();
        world = std::make_unique<MockCollisionWorld>();
        engine = std::make_unique<PhysicsEngine>(*world);
    }

    std::unique_ptr<MockCollisionWorld> world;
    std::unique_ptr<PhysicsEngine> engine;
};

TEST_F(EntityPhysicsTest, EntityBoundingBox) {
    Entity entity(EntityType::Player, 1);
    entity.setPosition(10.0, 20.0, 30.0);

    AxisAlignedBB box = entity.boundingBox();

    EXPECT_NEAR(box.minX, 9.7f, 0.001f);
    EXPECT_NEAR(box.minY, 20.0f, 0.001f);
    EXPECT_NEAR(box.minZ, 29.7f, 0.001f);
    EXPECT_NEAR(box.maxX, 10.3f, 0.001f);
    EXPECT_NEAR(box.maxY, 21.8f, 0.001f);
    EXPECT_NEAR(box.maxZ, 30.3f, 0.001f);
}

TEST_F(EntityPhysicsTest, MoveWithCollisionInAir) {
    // 移除地面
    for (int x = -10; x <= 10; ++x) {
        for (int z = -10; z <= 10; ++z) {
            world->setGroundBlock(x, z, false);
        }
    }

    Entity entity(EntityType::Player, 1);
    entity.setPosition(0.0, 10.0, 0.0);
    entity.setPhysicsEngine(engine.get());

    Vector3 movement = entity.moveWithCollision(0.0, -1.0, 0.0);

    EXPECT_LT(movement.y, 0.0f);
}

TEST_F(EntityPhysicsTest, ApplyPhysicsGravity) {
    Entity entity(EntityType::Player, 1);
    entity.setPhysicsEngine(engine.get());

    entity.setPosition(0.0, 10.0, 0.0);
    entity.setOnGround(false);

    entity.applyPhysics(0.05f);

    EXPECT_LT(entity.velocityY(), 0.0);
}

TEST_F(EntityPhysicsTest, ApplyPhysicsDrag) {
    Entity entity(EntityType::Player, 1);
    entity.setPhysicsEngine(engine.get());

    entity.setVelocity(1.0, 0.0, 1.0);
    entity.setOnGround(true);

    entity.applyPhysics(0.05f);

    EXPECT_LT(std::abs(entity.velocityX()), 1.0);
    EXPECT_LT(std::abs(entity.velocityZ()), 1.0);
}

// ============================================================================
// Player 物理测试
// ============================================================================

class PlayerPhysicsTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockCollisionRegistry::instance().initialize();
        world = std::make_unique<MockCollisionWorld>();
        engine = std::make_unique<PhysicsEngine>(*world);
    }

    std::unique_ptr<MockCollisionWorld> world;
    std::unique_ptr<PhysicsEngine> engine;
};

TEST_F(PlayerPhysicsTest, PlayerDimensions) {
    Player player(1, "TestPlayer");

    EXPECT_FLOAT_EQ(player.width(), Player::PLAYER_WIDTH);
    EXPECT_FLOAT_EQ(player.height(), Player::PLAYER_HEIGHT);
    EXPECT_FLOAT_EQ(player.eyeHeight(), Player::PLAYER_EYE_HEIGHT);
    EXPECT_FLOAT_EQ(player.stepHeight(), Player::PLAYER_STEP_HEIGHT);
}

TEST_F(PlayerPhysicsTest, PlayerJump) {
    Player player(1, "TestPlayer");
    player.setPhysicsEngine(engine.get());
    player.setPosition(0.0, 0.0, 0.0);
    player.setOnGround(true);

    player.jump();

    EXPECT_FLOAT_EQ(player.velocity().y, PhysicsEngine::JUMP_VELOCITY);
    EXPECT_FALSE(player.onGround());
}

TEST_F(PlayerPhysicsTest, PlayerCannotJumpInAir) {
    Player player(1, "TestPlayer");
    player.setPhysicsEngine(engine.get());
    player.setPosition(0.0, 10.0, 0.0);
    player.setOnGround(false);

    f32 velYBefore = player.velocity().y;
    player.jump();

    EXPECT_FLOAT_EQ(player.velocity().y, velYBefore);
}

TEST_F(PlayerPhysicsTest, HandleMovementInput) {
    Player player(1, "TestPlayer");
    player.setPhysicsEngine(engine.get());
    player.setPosition(0.0, 0.0, 0.0);
    player.setRotation(0.0f, 0.0f);

    player.handleMovementInput(1.0f, 0.0f, false, false);

    EXPECT_GT(std::abs(player.velocity().z), 0.0f);
}

TEST_F(PlayerPhysicsTest, HandleMovementInputSneak) {
    Player player(1, "TestPlayer");
    player.setPhysicsEngine(engine.get());
    player.setPosition(0.0, 0.0, 0.0);
    player.setRotation(0.0f, 0.0f);

    player.handleMovementInput(1.0f, 0.0f, false, true);
    f32 sneakSpeed = std::abs(player.velocity().z);

    player.setVelocity(0.0, 0.0, 0.0);
    player.handleMovementInput(1.0f, 0.0f, false, false);
    f32 normalSpeed = std::abs(player.velocity().z);

    EXPECT_LT(sneakSpeed, normalSpeed);
}
