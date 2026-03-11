#include "LookAtGoal.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../../math/random/Random.hpp"
#include "../../controller/LookController.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

// ==================== LookAtGoal ====================

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance)
    : LookAtGoal(mob, maxDistance, 0.02f)
{
}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance)
    : m_mob(mob)
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查概率
    math::Random rng(m_mob->ticksExisted());
    if (rng.nextFloat() >= m_chance) {
        return false;
    }

    // 寻找目标
    m_lookTarget = findTarget();
    return m_lookTarget != nullptr;
}

bool LookAtGoal::shouldContinueExecuting() {
    if (!m_lookTarget) return false;

    // 检查目标是否存活
    // TODO: if (!m_lookTarget->isAlive()) return false;

    // 检查距离
    f32 dx = m_lookTarget->x() - m_mob->x();
    f32 dz = m_lookTarget->z() - m_mob->z();
    f32 distSq = dx * dx + dz * dz;
    f32 maxDistSq = m_maxDistance * m_maxDistance;

    if (distSq > maxDistSq) {
        return false;
    }

    // 检查剩余时间
    return m_lookTime > 0;
}

void LookAtGoal::startExecuting() {
    // 设置看向时间（40-80 ticks，约2-4秒）
    math::Random rng(m_mob->ticksExisted());
    m_lookTime = 40 + rng.nextInt(40);
}

void LookAtGoal::resetTask() {
    m_lookTarget = nullptr;
}

void LookAtGoal::tick() {
    if (!m_mob || !m_lookTarget) return;

    // 使用视线控制器看向目标
    if (auto* lookCtrl = m_mob->lookController()) {
        lookCtrl->setLookPosition(
            m_lookTarget->x(),
            m_lookTarget->y() + m_lookTarget->eyeHeight(),
            m_lookTarget->z()
        );
    }

    m_lookTime--;
}

LivingEntity* LookAtGoal::findTarget() {
    // 简化实现：返回 nullptr
    // TODO: 在世界范围内搜索最近的 LivingEntity
    // 这需要 World 类支持实体查询
    return nullptr;
}

// ==================== LookRandomlyGoal ====================

LookRandomlyGoal::LookRandomlyGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookRandomlyGoal::shouldExecute() {
    if (!m_mob) return false;

    // 2% 的概率执行
    math::Random rng(m_mob->ticksExisted());
    return rng.nextFloat() < 0.02f;
}

bool LookRandomlyGoal::shouldContinueExecuting() {
    return m_lookTime > 0;
}

void LookRandomlyGoal::startExecuting() {
    if (!m_mob) return;

    // 设置随机看往方向
    math::Random rng(m_mob->ticksExisted());

    // 随机偏航角（-180 到 180）
    m_targetYaw = rng.nextFloat() * 360.0f - 180.0f;

    // 随机俯仰角（-30 到 30）
    m_targetPitch = rng.nextFloat() * 60.0f - 30.0f;

    // 看往时间（20-40 ticks）
    m_lookTime = 20 + rng.nextInt(20);
}

void LookRandomlyGoal::resetTask() {
    m_lookTime = 0;
}

void LookRandomlyGoal::tick() {
    if (!m_mob) return;

    // 使用视线控制器看向往方向
    if (auto* lookCtrl = m_mob->lookController()) {
        // 计算目标位置（使用当前坐标加方向向量）
        f32 yawRad = m_targetYaw * 3.14159265f / 180.0f;
        f32 pitchRad = m_targetPitch * 3.14159265f / 180.0f;

        f32 dx = std::cos(pitchRad) * std::cos(yawRad);
        f32 dy = -std::sin(pitchRad);
        f32 dz = std::cos(pitchRad) * std::sin(yawRad);

        f64 lookX = m_mob->x() + dx * 10.0;
        f64 lookY = m_mob->y() + m_mob->eyeHeight() + dy * 10.0;
        f64 lookZ = m_mob->z() + dz * 10.0;

        lookCtrl->setLookPosition(lookX, lookY, lookZ);
    }

    m_lookTime--;
}

} // namespace mr::entity::ai::goal
