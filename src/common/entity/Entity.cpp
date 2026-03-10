#include "Entity.hpp"
#include "../physics/PhysicsEngine.hpp"
#include "../math/random/Random.hpp"
#include <algorithm>
#include <sstream>
#include <chrono>

namespace mr {

// ============================================================================
// Entity 实现
// ============================================================================

Entity::Entity(LegacyEntityType type, EntityId id)
    : m_id(id)
    , m_legacyType(type)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_prevPosition(0.0f, 0.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
{
    // 生成随机UUID
    // 使用时间戳和随机数组合
    u64 seed = static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    math::Random rng(seed);

    std::stringstream ss;
    ss << std::hex << rng.nextU64() << rng.nextU64();
    m_uuid = ss.str();
}

void Entity::setPosition(f32 x, f32 y, f32 z) {
    m_prevPosition = m_position;
    m_position = Vector3(x, y, z);
}

void Entity::setRotation(f32 yaw, f32 pitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
}

void Entity::setVelocity(f32 x, f32 y, f32 z) {
    m_velocity = Vector3(x, y, z);
}

void Entity::tick() {
    m_ticksExisted++;
}

void Entity::update() {
    // 保存上一帧位置
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
}

void Entity::move(f32 dx, f32 dy, f32 dz) {
    m_prevPosition = m_position;
    m_position.x += dx;
    m_position.y += dy;
    m_position.z += dz;
}

void Entity::rotate(f32 deltaYaw, f32 deltaPitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;

    // 限制俯仰角范围
    m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

    // 规范化偏航角到 0-360
    while (m_yaw < 0.0f) m_yaw += 360.0f;
    while (m_yaw >= 360.0f) m_yaw -= 360.0f;
}

/**
 * @brief 带碰撞检测的移动
 *
 * 参考MC的Entity.move()实现。
 * 核心流程：
 * 1. 使用物理引擎执行碰撞检测和移动
 * 2. 更新实体位置（从碰撞箱计算）
 * 3. 更新碰撞状态和地面状态
 * 4. 更新坠落距离
 */
Vector3 Entity::moveWithCollision(f32 dx, f32 dy, f32 dz) {
    Vector3 desiredMovement(dx, dy, dz);

    // 重置碰撞状态
    m_collidedHorizontally = false;
    m_collidedVertically = false;

    if (!m_physicsEngine) {
        // 无物理引擎，直接移动
        move(dx, dy, dz);
        return desiredMovement;
    }

    // 获取当前碰撞箱
    AxisAlignedBB entityBox = boundingBox();

    // 使用物理引擎执行碰撞检测移动
    // 参考MC: Entity.move() -> getAllowedMovement()
    Vector3 actualMovement = m_physicsEngine->moveEntity(entityBox, desiredMovement, stepHeight());

    // 从碰撞箱更新位置
    // 实体位置 = 碰撞箱底部中心
    m_position = Vector3(
        (entityBox.minX + entityBox.maxX) / 2.0f,  // 中心X
        entityBox.minY,                             // 底部Y
        (entityBox.minZ + entityBox.maxZ) / 2.0f   // 中心Z
    );

    // 更新碰撞状态（从物理引擎获取）
    m_collidedHorizontally = m_physicsEngine->collidedHorizontally();
    m_collidedVertically = m_physicsEngine->collidedVertically();

    // 更新地面状态
    // 优先使用“向下移动时发生垂直碰撞”的判定，避免纯接触检测抖动。
    bool groundedByCollision = m_collidedVertically && desiredMovement.y < 0.0f;
    bool groundedByContact = m_physicsEngine->isOnGround(entityBox);
    m_onGround = groundedByCollision || groundedByContact;

    // 更新坠落距离
    // 参考MC: 如果在空中且向下移动，累积坠落距离
    if (!m_onGround && actualMovement.y < 0.0f) {
        m_fallDistance -= actualMovement.y;
    } else if (m_onGround && m_fallDistance > 0.0f) {
        // 着地，处理摔落伤害
        // TODO: 处理摔落伤害
        m_fallDistance = 0.0f;
    }

    return actualMovement;
}

void Entity::applyPhysics(f32 deltaTime) {
    // 应用重力
    if (!m_onGround) {
        // 重力加速度（MC使用固定值，与deltaTime无关）
        // 但这里我们使用deltaTime来平滑
        m_velocity.y -= PhysicsEngine::GRAVITY;
    }

    // 应用空气阻力
    m_velocity.x *= PhysicsEngine::DRAG;
    m_velocity.y *= PhysicsEngine::DRAG;
    m_velocity.z *= PhysicsEngine::DRAG;

    // 如果在地面，停止Y方向速度
    if (m_onGround && m_velocity.y < 0.0f) {
        m_velocity.y = 0.0f;
    }

    (void)deltaTime; // MC物理是基于tick的，不使用deltaTime
}

} // namespace mr
