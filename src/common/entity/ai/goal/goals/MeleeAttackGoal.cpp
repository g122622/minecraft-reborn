#include "MeleeAttackGoal.hpp"
#include "../../../mob/CreatureEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../GoalConstants.hpp"
#include "../../../ai/controller/LookController.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../math/random/Random.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

using namespace constants;

MeleeAttackGoal::MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory)
    : m_creature(creature)
    , m_speed(speed)
    , m_useLongMemory(useLongMemory)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool MeleeAttackGoal::shouldExecute() {
    if (!m_creature) return false;

    // 获取攻击目标
    LivingEntity* target = m_creature->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_attackTarget = target;
    return true;
}

bool MeleeAttackGoal::shouldContinueExecuting() {
    if (!m_creature || !m_attackTarget) return false;

    // 检查目标是否存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*m_attackTarget);

    if (distSq > MELEE_ATTACK_STOP_DISTANCE_SQ) {
        return false; // 目标太远，停止追踪
    }

    // 如果使用长期记忆，继续追踪
    if (m_useLongMemory) {
        return true;
    }

    return shouldExecute();
}

void MeleeAttackGoal::startExecuting() {
    m_attackCooldown = 0;
    m_pathRecalculateTimer = 0;
}

void MeleeAttackGoal::resetTask() {
    m_attackTarget = nullptr;
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void MeleeAttackGoal::tick() {
    if (!m_creature || !m_attackTarget) return;

    // 看向目标
    m_creature->lookAt(*m_attackTarget);

    // 减少攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 检查并更新路径
    checkPath();

    // 检查是否可以攻击
    if (canAttack(m_attackTarget) && m_attackCooldown <= 0) {
        attackTarget(m_attackTarget);
        m_attackCooldown = MELEE_ATTACK_COOLDOWN;
    }
}

bool MeleeAttackGoal::canAttack(LivingEntity* target) const {
    if (!m_creature || !target) return false;

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*target);

    // 检查是否在攻击范围内
    return distSq <= MELEE_ATTACK_REACH_SQ;
}

void MeleeAttackGoal::attackTarget(LivingEntity* target) {
    if (!m_creature || !target) return;

    // TODO: 实现攻击逻辑
    // 1. 重置攻击冷却
    // 2. 调用 attackEntity 方法
    // 3. 应用击退效果

    // 暂时简单处理
    // m_creature->attackEntity(target);
}

void MeleeAttackGoal::checkPath() {
    if (!m_creature || !m_attackTarget) return;

    m_pathRecalculateTimer--;

    if (m_pathRecalculateTimer <= 0) {
        m_pathRecalculateTimer = PATH_RECALCULATE_INTERVAL;

        // 移动到目标
        m_creature->tryMoveTo(
            m_attackTarget->x(),
            m_attackTarget->y(),
            m_attackTarget->z(),
            m_speed
        );
    }
}

} // namespace mr::entity::ai::goal
