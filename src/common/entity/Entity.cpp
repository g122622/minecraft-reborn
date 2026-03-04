#include "Entity.hpp"
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

BoundingBox Entity::boundingBox() const {
    return BoundingBox::fromPosition(m_position, width(), height());
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

} // namespace mr
