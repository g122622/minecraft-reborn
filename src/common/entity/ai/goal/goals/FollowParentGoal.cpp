#include "FollowParentGoal.hpp"
#include "../../../animal/AnimalEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../mob/AgeableEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../ai/controller/LookController.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"

namespace mr::entity::ai::goal {

FollowParentGoal::FollowParentGoal(AnimalEntity* animal, f64 speed)
    : m_childAnimal(animal)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool FollowParentGoal::shouldExecute() {
    if (!m_childAnimal) return false;

    // 检查是否是幼体
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false; // 已成年，不需要跟随父母
    }

    // 寻找成年动物
    m_parentAnimal = findParent();
    return m_parentAnimal != nullptr;
}

bool FollowParentGoal::shouldContinueExecuting() {
    if (!m_childAnimal || !m_parentAnimal) return false;

    // 检查是否成年
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false;
    }

    // 检查父/母是否存活
    if (!m_parentAnimal->isAlive()) {
        return false;
    }

    // 检查距离
    f32 dx = m_parentAnimal->x() - m_childAnimal->x();
    f32 dy = m_parentAnimal->y() - m_childAnimal->y();
    f32 dz = m_parentAnimal->z() - m_childAnimal->z();
    f32 distSq = dx * dx + dy * dy + dz * dz;

    // 太近或太远都停止
    if (distSq < MIN_DISTANCE * MIN_DISTANCE) {
        return false;
    }
    if (distSq > MAX_DISTANCE * MAX_DISTANCE) {
        return false;
    }

    return true;
}

void FollowParentGoal::startExecuting() {
    m_delayCounter = 0;
}

void FollowParentGoal::resetTask() {
    m_parentAnimal = nullptr;

    if (m_childAnimal) {
        auto* nav = m_childAnimal->navigator();
        if (nav) {
            nav->clearPath();
        }
    }
}

void FollowParentGoal::tick() {
    if (!m_childAnimal || !m_parentAnimal) return;

    // 看向父/母
    if (auto* lookCtrl = m_childAnimal->lookController()) {
        lookCtrl->setLookPosition(
            m_parentAnimal->x(),
            m_parentAnimal->y() + m_parentAnimal->eyeHeight(),
            m_parentAnimal->z()
        );
    }

    // 定期更新路径
    if (--m_delayCounter <= 0) {
        m_delayCounter = DELAY_INTERVAL;

        // 移动到父/母
        m_childAnimal->tryMoveTo(
            m_parentAnimal->x(),
            m_parentAnimal->y(),
            m_parentAnimal->z(),
            m_speed
        );
    }
}

AnimalEntity* FollowParentGoal::findParent() {
    if (!m_childAnimal || !m_childAnimal->world()) return nullptr;

    IWorld* world = m_childAnimal->world();

    // 搜索附近的动物
    auto entities = world->getEntitiesInRange(
        Vector3(m_childAnimal->x(), m_childAnimal->y(), m_childAnimal->z()),
        SEARCH_RANGE,
        m_childAnimal
    );

    AnimalEntity* closestParent = nullptr;
    f32 closestDist = SEARCH_RANGE * SEARCH_RANGE;

    for (Entity* entity : entities) {
        // 检查是否是同类型动物
        AnimalEntity* animal = dynamic_cast<AnimalEntity*>(entity);
        if (!animal) continue;

        // 检查是否是成年
        if (animal->getGrowingAge() < 0) continue; // 幼体不能当父母

        // 计算距离
        f32 dx = animal->x() - m_childAnimal->x();
        f32 dy = animal->y() - m_childAnimal->y();
        f32 dz = animal->z() - m_childAnimal->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDist) {
            closestDist = distSq;
            closestParent = animal;
        }
    }

    return closestParent;
}

} // namespace mr::entity::ai::goal
