#include "PhysicsEngine.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

PhysicsEngine::PhysicsEngine(ICollisionWorld& world)
    : m_world(&world)
{
}

Vector3 PhysicsEngine::moveEntity(AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight) {
    if (movement.x == 0.0f && movement.y == 0.0f && movement.z == 0.0f) {
        return Vector3::ZERO;
    }

    // 1. 收集碰撞箱
    // 扩展搜索范围以包含所有可能碰撞的方块
    AxisAlignedBB searchBox = entityBox.expand(
        std::abs(movement.x) + 1.0f,
        std::abs(movement.y) + 1.0f,
        std::abs(movement.z) + 1.0f
    );

    std::vector<AxisAlignedBB> boxes;
    collectCollisionBoxes(searchBox, boxes);

    if (boxes.empty()) {
        // 无碰撞，直接移动
        entityBox.offset(movement.x, movement.y, movement.z);
        return movement;
    }

    // 2. 碰撞解决
    Vector3 resolved = resolveCollision(entityBox, movement, boxes);

    // 3. 尝试步进（仅当有水平碰撞且有步进高度时）
    if (stepHeight > 0.0f) {
        bool horizontalCollision = (std::abs(resolved.x - movement.x) > 1e-6f ||
                                     std::abs(resolved.z - movement.z) > 1e-6f);

        if (horizontalCollision && (movement.x != 0.0f || movement.z != 0.0f)) {
            // 检查是否在地面
            bool wasOnGround = isOnGround(entityBox);
            if (wasOnGround) {
                Vector3 stepped = attemptStepUp(entityBox, movement, resolved, boxes, stepHeight);
                // 如果步进后移动距离更大，使用步进结果
                if (stepped.lengthSquared() > resolved.lengthSquared()) {
                    return stepped;
                }
            }
        }
    }

    return resolved;
}

bool PhysicsEngine::isOnGround(const AxisAlignedBB& entityBox) const {
    // 向下微移检测碰撞
    AxisAlignedBB testBox = entityBox.offsetted(0.0f, -0.001f, 0.0f);

    // 计算搜索范围
    i32 minX = static_cast<i32>(std::floor(testBox.minX));
    i32 maxX = static_cast<i32>(std::floor(testBox.maxX));
    i32 minY = static_cast<i32>(std::floor(testBox.minY));
    i32 maxY = static_cast<i32>(std::floor(testBox.maxY));
    i32 minZ = static_cast<i32>(std::floor(testBox.minZ));
    i32 maxZ = static_cast<i32>(std::floor(testBox.maxZ));

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (!m_world->isWithinWorldBounds(x, y, z)) continue;

                BlockState block = m_world->getBlock(x, y, z);
                if (block.isAir()) continue;

                CollisionShape shape = BlockCollisionRegistry::instance().getShape(block);
                if (shape.isEmpty()) continue;

                // 检测是否与测试框相交
                if (shape.intersects(testBox, x, y, z)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void PhysicsEngine::collectCollisionBoxes(const AxisAlignedBB& searchBox,
                                           std::vector<AxisAlignedBB>& boxes) const {
    boxes.clear();

    // 计算方块范围
    i32 minX = static_cast<i32>(std::floor(searchBox.minX));
    i32 maxX = static_cast<i32>(std::floor(searchBox.maxX));
    i32 minY = static_cast<i32>(std::floor(searchBox.minY));
    i32 maxY = static_cast<i32>(std::floor(searchBox.maxY));
    i32 minZ = static_cast<i32>(std::floor(searchBox.minZ));
    i32 maxZ = static_cast<i32>(std::floor(searchBox.maxZ));

    // 遍历所有可能的方块
    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                getBlockCollisionBoxes(x, y, z, boxes);
            }
        }
    }
}

Vector3 PhysicsEngine::resolveCollision(AxisAlignedBB& entityBox,
                                         const Vector3& movement,
                                         const std::vector<AxisAlignedBB>& boxes) {
    f32 dx = movement.x;
    f32 dy = movement.y;
    f32 dz = movement.z;

    // 1. Y轴优先处理（重力最重要）
    if (dy != 0.0f) {
        for (const auto& box : boxes) {
            dy = entityBox.calculateYOffset(box, dy);
        }
        entityBox.offset(0.0f, dy, 0.0f);
    }

    // 2. X/Z按移动幅度排序处理
    if (std::abs(dx) >= std::abs(dz)) {
        // X轴优先
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
    } else {
        // Z轴优先
        if (dz != 0.0f) {
            for (const auto& box : boxes) {
                dz = entityBox.calculateZOffset(box, dz);
            }
            entityBox.offset(0.0f, 0.0f, dz);
        }
        if (dx != 0.0f) {
            for (const auto& box : boxes) {
                dx = entityBox.calculateXOffset(box, dx);
            }
            entityBox.offset(dx, 0.0f, 0.0f);
        }
    }

    return Vector3(dx, dy, dz);
}

Vector3 PhysicsEngine::attemptStepUp(AxisAlignedBB& entityBox,
                                      const Vector3& movement,
                                      const Vector3& resolvedMovement,
                                      const std::vector<AxisAlignedBB>& boxes,
                                      f32 stepHeight) {
    // 保存原始状态
    AxisAlignedBB originalBox = entityBox;

    // 步进算法：
    // 1. 将实体向上移动步进高度
    // 2. 尝试水平移动
    // 3. 将实体向下移动，直到碰到地面

    // 向上移动
    AxisAlignedBB raisedBox = entityBox.offsetted(0.0f, stepHeight, 0.0f);

    // 尝试水平移动（向上状态下）
    f32 dx = movement.x;
    f32 dz = movement.z;

    // 检查向上移动是否有空间
    bool canRaise = true;
    for (const auto& box : boxes) {
        f32 offsetY = entityBox.calculateYOffset(box, stepHeight);
        if (offsetY < stepHeight - 0.001f) {
            canRaise = false;
            break;
        }
    }

    if (!canRaise) {
        entityBox = originalBox;
        return resolvedMovement;
    }

    // 向上移动
    entityBox.offset(0.0f, stepHeight, 0.0f);

    // 尝试水平移动
    for (const auto& box : boxes) {
        dx = entityBox.calculateXOffset(box, dx);
    }
    entityBox.offset(dx, 0.0f, 0.0f);

    for (const auto& box : boxes) {
        dz = entityBox.calculateZOffset(box, dz);
    }
    entityBox.offset(0.0f, 0.0f, dz);

    // 向下移动直到碰到地面
    f32 downDist = stepHeight;
    for (const auto& box : boxes) {
        downDist = entityBox.calculateYOffset(box, -stepHeight);
    }
    entityBox.offset(0.0f, downDist, 0.0f);

    // 计算最终移动
    Vector3 steppedMovement(
        entityBox.minX - originalBox.minX,
        entityBox.minY - originalBox.minY,
        entityBox.minZ - originalBox.minZ
    );

    // 如果步进后高度变化为正（实际上是向上走），接受这个结果
    // 但如果向下移动太多，可能是在空中的情况，回退
    if (steppedMovement.y >= -0.001f && steppedMovement.y <= stepHeight) {
        return steppedMovement;
    }

    // 回退到原始位置
    entityBox = originalBox;
    return resolvedMovement;
}

void PhysicsEngine::getBlockCollisionBoxes(i32 x, i32 y, i32 z,
                                            std::vector<AxisAlignedBB>& boxes) const {
    if (!m_world->isWithinWorldBounds(x, y, z)) return;

    BlockState block = m_world->getBlock(x, y, z);
    if (block.isAir()) return;

    CollisionShape shape = BlockCollisionRegistry::instance().getShape(block);
    if (shape.isEmpty()) return;

    // 获取世界坐标碰撞箱
    auto worldBoxes = shape.getWorldBoxes(x, y, z);
    boxes.insert(boxes.end(), worldBoxes.begin(), worldBoxes.end());
}

} // namespace mr
