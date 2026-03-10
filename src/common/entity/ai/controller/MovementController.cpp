#include "MovementController.hpp"
#include "../../mob/MobEntity.hpp"
#include <cmath>

namespace mr::entity::ai::controller {

MovementController::MovementController(MobEntity* mob)
    : m_mob(mob)
{}

void MovementController::setMoveTo(f64 x, f64 y, f64 z, f64 speed) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_speed = speed;

    if (m_action != MoveAction::Jumping) {
        m_action = MoveAction::MoveTo;
    }
}

void MovementController::strafe(f32 forward, f32 strafe) {
    m_action = MoveAction::Strafe;
    m_moveForward = forward;
    m_moveStrafe = strafe;
    m_speed = 0.25;  // 默认横向移动速度
}

void MovementController::tick() {
    if (!m_mob) return;

    // TODO: 完整实现需要：
    // - m_mob->getMovementSpeed() - 获取移动速度属性
    // - m_mob->setAIMoveSpeed() - 设置AI移动速度
    // - m_mob->setMoveForward() - 设置前进方向
    // - m_mob->setMoveStrafing() - 设置横向方向
    // - m_mob->getNavigator() - 获取寻路器
    // - m_mob->isOnGround() - 是否在地面
    // - m_mob->stepHeight() - 步进高度

    if (m_action == MoveAction::Strafe) {
        // 横向移动模式
        // TODO: 实现横向移动
        m_action = MoveAction::Wait;
    }
    else if (m_action == MoveAction::MoveTo) {
        m_action = MoveAction::Wait;

        f64 dx = m_posX - m_mob->x();
        f64 dy = m_posY - m_mob->y();
        f64 dz = m_posZ - m_mob->z();

        f64 distSq = dx * dx + dy * dy + dz * dz;

        // 如果距离太小，停止移动
        if (distSq < 2.5000003e-7) {
            // m_mob->setMoveForward(0.0f);
            return;
        }

        // 计算目标偏航角
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * (180.0 / 3.14159265358979323846) - 90.0);

        // 限制旋转速度
        f32 currentYaw = m_mob->yaw();
        f32 newYaw = limitAngle(currentYaw, targetYaw, 90.0f);

        m_mob->setRotation(newYaw, m_mob->pitch());

        // TODO: 设置移动速度
        // f32 speed = static_cast<f32>(m_speed * m_mob->getAttributeValue(Attributes::MOVEMENT_SPEED));
        // m_mob->setAIMoveSpeed(speed);

        // TODO: 检查是否需要跳跃
        // if (dy > m_mob->stepHeight() && dx*dx + dz*dz < 1.0) {
        //     m_mob->getJumpController()->setJumping();
        //     m_action = MoveAction::Jumping;
        // }
    }
    else if (m_action == MoveAction::Jumping) {
        // TODO: 设置移动速度
        // m_mob->setAIMoveSpeed(static_cast<f32>(m_speed * m_mob->getAttributeValue(Attributes::MOVEMENT_SPEED)));

        // TODO: 检查是否着陆
        // if (m_mob->isOnGround()) {
        //     m_action = MoveAction::Wait;
        // }
    }
    else {
        // Wait 状态
        // m_mob->setMoveForward(0.0f);
    }
}

f32 MovementController::limitAngle(f32 sourceAngle, f32 targetAngle, f32 maximumChange) {
    // 计算角度差，归一化到 [-180, 180]
    f32 diff = targetAngle - sourceAngle;

    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;

    // 限制变化量
    if (diff > maximumChange) diff = maximumChange;
    if (diff < -maximumChange) diff = -maximumChange;

    f32 result = sourceAngle + diff;

    // 归一化到 [0, 360]
    while (result < 0.0f) result += 360.0f;
    while (result >= 360.0f) result -= 360.0f;

    return result;
}

} // namespace mr::entity::ai::controller
