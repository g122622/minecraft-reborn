#include "RandomWalkingGoal.hpp"
#include "../../../mob/CreatureEntity.hpp"
#include "../../../../math/random/Random.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed)
    : RandomWalkingGoal(creature, speed, 120)
{
}

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance)
    : m_creature(creature)
    , m_speed(speed)
    , m_executionChance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool RandomWalkingGoal::shouldExecute() {
    if (!m_creature) return false;

    // TODO: 检查是否被骑乘
    // if (m_creature->isBeingRidden()) return false;

    // 如果不需要强制更新，检查概率
    if (!m_forceUpdate) {
        // TODO: 检查空闲时间
        // if (m_creature->getIdleTime() >= 100) return false;

        // 检查执行概率
        // if (m_creature->getRandom().nextInt(m_executionChance) != 0) return false;
    }

    // 获取随机位置
    Vector3 targetPos;
    if (getRandomPosition(targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        m_forceUpdate = false;
        return true;
    }

    return false;
}

bool RandomWalkingGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // TODO: 检查路径是否完成
    // if (m_creature->getNavigator()->noPath()) return false;

    // TODO: 检查是否被骑乘
    // if (m_creature->isBeingRidden()) return false;

    return m_timeoutCounter > 0;
}

void RandomWalkingGoal::startExecuting() {
    if (m_creature) {
        m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        m_timeoutCounter = 600; // 最大执行时间（30秒）
    }
}

void RandomWalkingGoal::resetTask() {
    // TODO: 清除导航路径
    // if (m_creature && m_creature->getNavigator()) {
    //     m_creature->getNavigator()->clearPath();
    // }
    m_timeoutCounter = 0;
}

void RandomWalkingGoal::tick() {
    if (m_timeoutCounter > 0) {
        m_timeoutCounter--;
    }
}

bool RandomWalkingGoal::getRandomPosition(Vector3& outPos) {
    if (!m_creature) return false;

    // 简单的随机位置生成
    // TODO: 实现完整的随机位置生成器（类似 MC 的 RandomPositionGenerator）
    math::Random rng(m_creature->ticksExisted());

    f32 range = 10.0f; // 漫步范围
    f32 x = m_creature->x() + (rng.nextFloat() * 2.0f - 1.0f) * range;
    f32 z = m_creature->z() + (rng.nextFloat() * 2.0f - 1.0f) * range;

    // Y坐标需要找到地面
    // 简化实现：使用当前位置
    f32 y = m_creature->y();

    outPos = Vector3(x, y, z);
    return true;
}

} // namespace mr::entity::ai::goal
