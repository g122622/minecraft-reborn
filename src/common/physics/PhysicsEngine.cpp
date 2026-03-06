#include "PhysicsEngine.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

namespace mr {

PhysicsEngine::PhysicsEngine(ICollisionWorld& world)
    : m_world(&world)
{
}

/**
 * @brief 移动实体并处理碰撞
 *
 * 参考MC的Entity.move()和Entity.getAllowedMovement()实现。
 *
 * 核心流程：
 * 1. 收集潜在碰撞箱
 * 2. Y轴优先碰撞解决
 * 3. X/Z轴按移动幅度排序处理
 * 4. 尝试步进（如果水平碰撞且在地面）
 */
Vector3 PhysicsEngine::moveEntity(AxisAlignedBB& entityBox, const Vector3& movement, f32 stepHeight) {
    if (movement.x == 0.0f && movement.y == 0.0f && movement.z == 0.0f) {
        m_collidedVertically = false;
        m_collidedHorizontally = false;
        return Vector3::ZERO;
    }

    // 保存原始位置用于步进
    AxisAlignedBB originalBox = entityBox;

    // 收集扩展范围内的碰撞箱
    // MC使用 collisionBox.expand(vec) + world.getCollisionShapes
    AxisAlignedBB searchBox = entityBox.expand(
        std::abs(movement.x) + 1.0f,
        std::abs(movement.y) + 1.0f,
        std::abs(movement.z) + 1.0f
    );

    std::vector<AxisAlignedBB> boxes;
    collectCollisionBoxes(searchBox, boxes);

    // 处理初始轻微重叠（常见于浮点误差/网络同步边界）
    // 若不先去重叠，逐轴偏移算法不会把已嵌入地面的实体推出。
    f32 overlapPushUp = resolveInitialOverlaps(entityBox, boxes);

    if (boxes.empty()) {
        // 无碰撞，直接移动
        entityBox.offset(movement.x, movement.y, movement.z);
        m_collidedVertically = false;
        m_collidedHorizontally = false;
        return movement;
    }

    // 执行碰撞解决
    Vector3 resolved = resolveCollision(entityBox, movement, boxes);
    if (overlapPushUp > 0.0f) {
        resolved.y += overlapPushUp;
    }

    // 检测水平碰撞
    bool horizontalCollision = (std::abs(resolved.x - movement.x) > 1e-7f ||
                                 std::abs(resolved.z - movement.z) > 1e-7f);
    bool verticalCollision = (std::abs(resolved.y - movement.y) > 1e-7f);

    // 尝试步进（MC: 仅当水平碰撞且之前在地面或有向下移动时）
    if (stepHeight > 0.0f && horizontalCollision && (movement.x != 0.0f || movement.z != 0.0f)) {
        // MC检查: onGround || (verticalCollision && movement.y < 0)
        bool wasOnGround = isOnGround(originalBox);
        if (wasOnGround || (verticalCollision && movement.y < 0.0f)) {
            Vector3 stepped = attemptStepUp(entityBox, originalBox, movement, stepHeight, resolved);

            // MC使用水平距离平方比较
            f32 resolvedHorizontalSq = resolved.x * resolved.x + resolved.z * resolved.z;
            f32 steppedHorizontalSq = stepped.x * stepped.x + stepped.z * stepped.z;

            if (steppedHorizontalSq > resolvedHorizontalSq + 1e-7f) {
                // 更新碰撞状态
                m_collidedVertically = (std::abs(stepped.y - movement.y) > 1e-7f);
                m_collidedHorizontally = (std::abs(stepped.x - movement.x) > 1e-7f ||
                                          std::abs(stepped.z - movement.z) > 1e-7f);
                return stepped;
            }
        }
    }

    // 更新碰撞状态
    m_collidedVertically = verticalCollision;
    m_collidedHorizontally = horizontalCollision;

    return resolved;
}

bool PhysicsEngine::isOnGround(const AxisAlignedBB& entityBox) const {
    // 向下微移后检测碰撞。增大探测深度以提高浮点抖动容忍度，
    // 避免平地移动时偶发"离地一帧"导致累计下沉。
    constexpr f32 GROUND_PROBE_EPSILON = 0.01f;
    AxisAlignedBB testBox = entityBox.offsetted(0.0f, -GROUND_PROBE_EPSILON, 0.0f);

    // 计算搜索范围（只需要检测底部一排方块）
    i32 minX = static_cast<i32>(std::floor(testBox.minX));
    i32 maxX = static_cast<i32>(std::ceil(testBox.maxX) - 1);
    i32 minY = static_cast<i32>(std::floor(testBox.minY));
    i32 maxY = static_cast<i32>(std::ceil(testBox.maxY) - 1);  // 可能跨越两个Y层级
    i32 minZ = static_cast<i32>(std::floor(testBox.minZ));
    i32 maxZ = static_cast<i32>(std::ceil(testBox.maxZ) - 1);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (!m_world->isWithinWorldBounds(x, y, z)) continue;

                const BlockState* state = m_world->getBlockState(x, y, z);
                if (!state || state->isAir()) continue;

                const CollisionShape& shape = state->getCollisionShape();
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

/**
 * @brief 核心碰撞解决算法
 *
 * 参考MC的Entity.collideBoundingBox():
 * 1. 先处理Y轴（重力最重要）
 * 2. 按移动幅度处理X/Z轴（幅度大的先处理）
 *
 * 注意：每次轴移动后更新entityBox位置
 */
Vector3 PhysicsEngine::resolveCollision(AxisAlignedBB& entityBox,
                                         const Vector3& movement,
                                         const std::vector<AxisAlignedBB>& boxes) {
    f32 dx = movement.x;
    f32 dy = movement.y;
    f32 dz = movement.z;

    // 1. Y轴优先处理
    if (dy != 0.0f) {
        for (const auto& box : boxes) {
            dy = entityBox.calculateYOffset(box, dy);
        }
        entityBox.offset(0.0f, dy, 0.0f);
    }

    // 2. X/Z按移动幅度排序处理（MC逻辑）
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

/**
 * @brief 尝试步进
 *
 * 参考MC的Entity.getAllowedMovement()中的步进逻辑：
 * 1. 先向上移动stepHeight
 * 2. 尝试水平移动
 * 3. 向下移动直到碰到地面
 *
 * 注意：需要重新收集新位置的碰撞箱
 */
Vector3 PhysicsEngine::attemptStepUp(AxisAlignedBB& entityBox,
                                      const AxisAlignedBB& originalBox,
                                      const Vector3& movement,
                                      f32 stepHeight,
                                      const Vector3& fallbackResult) {
    f32 dx = movement.x;
    f32 dz = movement.z;

    // Step 1: 尝试向上移动stepHeight并水平移动
    AxisAlignedBB raisedBox = originalBox.offsetted(0.0f, stepHeight, 0.0f);

    // 收集向上位置的碰撞箱
    AxisAlignedBB raisedSearchBox = raisedBox.expand(
        std::abs(dx) + 1.0f,
        1.0f,  // Y方向只需要检查stepHeight范围
        std::abs(dz) + 1.0f
    );
    std::vector<AxisAlignedBB> raisedBoxes;
    collectCollisionBoxes(raisedSearchBox, raisedBoxes);

    // 检查能否向上移动
    f32 upDist = stepHeight;
    for (const auto& box : raisedBoxes) {
        upDist = originalBox.calculateYOffset(box, upDist);
    }

    if (upDist < stepHeight - 0.001f) {
        // 无法完全抬起，放弃步进
        return fallbackResult;
    }

    // 向上移动
    AxisAlignedBB stepBox = originalBox;
    stepBox.offset(0.0f, upDist, 0.0f);

    // Step 2: 在抬起状态下尝试水平移动
    // 使用新位置的碰撞箱
    AxisAlignedBB horizontalSearchBox = stepBox.expand(std::abs(dx) + 1.0f, 1.0f, std::abs(dz) + 1.0f);
    std::vector<AxisAlignedBB> horizontalBoxes;
    collectCollisionBoxes(horizontalSearchBox, horizontalBoxes);

    // 尝试X方向
    f32 stepDx = dx;
    for (const auto& box : horizontalBoxes) {
        stepDx = stepBox.calculateXOffset(box, stepDx);
    }
    stepBox.offset(stepDx, 0.0f, 0.0f);

    // 尝试Z方向
    f32 stepDz = dz;
    for (const auto& box : horizontalBoxes) {
        stepDz = stepBox.calculateZOffset(box, stepDz);
    }
    stepBox.offset(0.0f, 0.0f, stepDz);

    // Step 3: 向下移动直到碰到地面
    // 收集新位置的碰撞箱
    AxisAlignedBB downSearchBox = stepBox.expand(1.0f, stepHeight + 1.0f, 1.0f);
    std::vector<AxisAlignedBB> downBoxes;
    collectCollisionBoxes(downSearchBox, downBoxes);

    f32 downDist = -upDist;  // 需要向下移动的距离
    for (const auto& box : downBoxes) {
        downDist = stepBox.calculateYOffset(box, downDist);
    }
    stepBox.offset(0.0f, downDist, 0.0f);

    // 计算最终移动距离
    Vector3 result(
        stepBox.minX - originalBox.minX,
        stepBox.minY - originalBox.minY,
        stepBox.minZ - originalBox.minZ
    );

    // 处理浮点误差：步进结果不应产生"向下位移"。
    // 否则会在平地行走时以极小概率逐帧累积下沉。
    if (result.y < 0.0f && result.y > -0.01f) {
        stepBox.offset(0.0f, -result.y, 0.0f);
        result.y = 0.0f;
    }

    // 验证结果：高度变化应该在合理范围内
    if (result.y < -0.001f || result.y > stepHeight + 0.001f) {
        // 异常情况，放弃步进
        entityBox = originalBox;
        return fallbackResult;
    }

    // 使用步进结果
    entityBox = stepBox;
    return result;
}

void PhysicsEngine::getBlockCollisionBoxes(i32 x, i32 y, i32 z,
                                            std::vector<AxisAlignedBB>& boxes) const {
    if (!m_world->isWithinWorldBounds(x, y, z)) return;

    const BlockState* state = m_world->getBlockState(x, y, z);
    if (!state || state->isAir()) return;

    const CollisionShape& shape = state->getCollisionShape();
    if (shape.isEmpty()) return;

    // 获取世界坐标碰撞箱
    auto worldBoxes = shape.getWorldBoxes(x, y, z);
    boxes.insert(boxes.end(), worldBoxes.begin(), worldBoxes.end());
}

f32 PhysicsEngine::resolveInitialOverlaps(AxisAlignedBB& entityBox,
                                          const std::vector<AxisAlignedBB>& boxes) const {
    // 仅处理小范围向上推出，避免对合法穿插场景造成过度修正
    constexpr f32 MAX_DEPENETRATION_UP = 0.45f;
    constexpr f32 EPSILON = 1e-4f;

    f32 maxPushUp = 0.0f;
    for (const auto& box : boxes) {
        if (!entityBox.intersects(box)) {
            continue;
        }

        // 仅考虑把实体从"地面/下方方块"向上推出
        f32 pushUp = box.maxY - entityBox.minY;
        if (pushUp > 0.0f && pushUp <= MAX_DEPENETRATION_UP) {
            maxPushUp = std::max(maxPushUp, pushUp);
        }
    }

    if (maxPushUp > 0.0f) {
        const f32 pushed = maxPushUp + EPSILON;
        entityBox.offset(0.0f, pushed, 0.0f);
        return pushed;
    }

    return 0.0f;
}

} // namespace mr
