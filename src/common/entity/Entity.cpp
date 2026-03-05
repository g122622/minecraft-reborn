#include "Entity.hpp"
#include "../physics/PhysicsEngine.hpp"
#include <algorithm>
#include <sstream>
#include <random>

namespace mr {

// ============================================================================
// Entity 实现
// ============================================================================

Entity::Entity(EntityType type, EntityId id)
    : m_id(id)
    , m_type(type)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_prevPosition(0.0f, 0.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
{
    // 生成随机UUID
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<u64> dis;

    std::stringstream ss;
    ss << std::hex << dis(gen) << dis(gen);
    m_uuid = ss.str();
}

void Entity::setPosition(f64 x, f64 y, f64 z) {
    m_prevPosition = m_position;
    m_position = Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
}

void Entity::setRotation(f32 yaw, f32 pitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
}

void Entity::setVelocity(f64 x, f64 y, f64 z) {
    m_velocity = Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
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

void Entity::move(f64 dx, f64 dy, f64 dz) {
    m_prevPosition = m_position;
    m_position.x += static_cast<f32>(dx);
    m_position.y += static_cast<f32>(dy);
    m_position.z += static_cast<f32>(dz);
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

Vector3 Entity::moveWithCollision(f64 dx, f64 dy, f64 dz) {
    // 保存原始位置
    Vector3 originalPos = m_position;
    Vector3 desiredMovement(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz));

    // 重置碰撞状态
    m_collidedHorizontally = false;
    m_collidedVertically = false;

    if (!m_physicsEngine) {
        // 无物理引擎，直接移动
        move(dx, dy, dz);
        return desiredMovement;
    }

    // 获取碰撞箱
    AxisAlignedBB entityBox = boundingBox();

    // 使用物理引擎移动
    Vector3 actualMovement = m_physicsEngine->moveEntity(entityBox, desiredMovement, stepHeight());

    // 更新位置
    m_position = Vector3(
        (entityBox.minX + entityBox.maxX) / 2.0f,  // 中心X
        entityBox.minY,                             // 底部Y
        (entityBox.minZ + entityBox.maxZ) / 2.0f   // 中心Z
    );

    // 更新碰撞状态
    m_onGround = m_physicsEngine->isOnGround(entityBox);

    // 检测碰撞方向
    if (std::abs(actualMovement.x - desiredMovement.x) > 1e-6f ||
        std::abs(actualMovement.z - desiredMovement.z) > 1e-6f) {
        m_collidedHorizontally = true;
    }
    if (std::abs(actualMovement.y - desiredMovement.y) > 1e-6f) {
        m_collidedVertically = true;
    }

    // 更新坠落距离
    if (!m_onGround && m_velocity.y < 0.0f) {
        m_fallDistance -= actualMovement.y;
    } else {
        // 如果着地或向上移动，重置坠落距离
        if (m_onGround && m_fallDistance > 0.0f) {
            // TODO: 处理摔落伤害
            m_fallDistance = 0.0f;
        }
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
