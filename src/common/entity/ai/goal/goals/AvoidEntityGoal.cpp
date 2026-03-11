#include "AvoidEntityGoal.hpp"
#include "../../../mob/CreatureEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../math/random/Random.hpp"
#include "../../../../math/MathUtils.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

AvoidEntityGoal::AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(nullptr)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

AvoidEntityGoal::AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, EntityPredicate predicate)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(std::move(predicate))
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool AvoidEntityGoal::shouldExecute() {
    if (!m_creature) return false;

    // 寻找要避开的实体
    m_avoidTarget = findEntityToAvoid();
    if (!m_avoidTarget) {
        return false;
    }

    // 寻找逃跑位置
    return findEscapePosition();
}

bool AvoidEntityGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return m_avoidTarget != nullptr;
}

void AvoidEntityGoal::startExecuting() {
    if (m_creature) {
        m_creature->tryMoveTo(m_escapeX, m_escapeY, m_escapeZ, m_farSpeed);
    }
}

void AvoidEntityGoal::resetTask() {
    m_avoidTarget = nullptr;

    if (m_creature) {
        auto* nav = m_creature->navigator();
        if (nav) {
            nav->clearPath();
        }
    }
}

void AvoidEntityGoal::tick() {
    if (!m_creature || !m_avoidTarget) return;

    // 根据距离调整速度
    f32 dx = m_avoidTarget->x() - m_creature->x();
    f32 dy = m_avoidTarget->y() - m_creature->y();
    f32 dz = m_avoidTarget->z() - m_creature->z();
    f32 distSq = dx * dx + dy * dy + dz * dz;

    f64 speed;
    if (distSq < 49.0f) { // 7格内
        speed = m_nearSpeed;
    } else {
        speed = m_farSpeed;
    }

    // 继续移动到逃跑位置
    m_creature->tryMoveTo(m_escapeX, m_escapeY, m_escapeZ, speed);
}

LivingEntity* AvoidEntityGoal::findEntityToAvoid() {
    if (!m_creature || !m_creature->world()) return nullptr;

    IWorld* world = m_creature->world();

    // 搜索附近的实体
    auto entities = world->getEntitiesInRange(
        Vector3(m_creature->x(), m_creature->y(), m_creature->z()),
        m_avoidDistance,
        m_creature
    );

    LivingEntity* closestAvoid = nullptr;
    f32 closestDist = m_avoidDistance * m_avoidDistance;

    for (Entity* entity : entities) {
        // 检查是否是 LivingEntity
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (!living) continue;

        // 检查过滤条件
        if (m_predicate && !m_predicate(living)) {
            continue;
        }

        // 计算距离
        f32 dx = living->x() - m_creature->x();
        f32 dy = living->y() - m_creature->y();
        f32 dz = living->z() - m_creature->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDist) {
            closestDist = distSq;
            closestAvoid = living;
        }
    }

    return closestAvoid;
}

bool AvoidEntityGoal::findEscapePosition() {
    if (!m_creature || !m_avoidTarget) return false;

    math::Random rng = m_creature->getRandom();

    // 生成远离目标的位置
    // 简化实现：直接计算逃跑位置
    f32 distance = static_cast<f32>(ESCAPE_RANGE);

    // 计算从目标指向相反方向的位置
    f32 dirX = m_creature->x() - m_avoidTarget->x();
    f32 dirZ = m_creature->z() - m_avoidTarget->z();
    f32 len = std::sqrt(dirX * dirX + dirZ * dirZ);

    if (len > 0.01f) {
        dirX /= len;
        dirZ /= len;
    }

    // 添加随机偏移
    f32 randomAngle = rng.nextFloat() * math::TWO_PI;
    f32 randomDist = distance * rng.nextFloat();

    m_escapeX = m_creature->x() + dirX * distance + std::cos(randomAngle) * randomDist * 0.5f;
    m_escapeZ = m_creature->z() + dirZ * distance + std::sin(randomAngle) * randomDist * 0.5f;
    m_escapeY = m_creature->y();

    return true;
}

} // namespace mr::entity::ai::goal
