#include "BreedGoal.hpp"
#include "../../../animal/AnimalEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../ai/controller/LookController.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../math/random/Random.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

BreedGoal::BreedGoal(AnimalEntity* animal, f64 speed)
    : m_animal(animal)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool BreedGoal::shouldExecute() {
    if (!m_animal) return false;

    // 检查是否处于爱心状态
    if (m_animal->getInLove() <= 0) {
        return false;
    }

    // 寻找配偶
    m_targetMate = findNearbyMate();
    return m_targetMate != nullptr;
}

bool BreedGoal::shouldContinueExecuting() {
    if (!m_targetMate) return false;

    // 检查配偶是否存活且仍处于爱心状态
    if (!m_targetMate->isAlive()) return false;
    if (m_targetMate->getInLove() <= 0) return false;

    // 检查是否超时
    return m_spawnBabyDelay < SPAWN_BABY_DELAY;
}

void BreedGoal::startExecuting() {
    m_spawnBabyDelay = 0;
}

void BreedGoal::resetTask() {
    m_targetMate = nullptr;
    m_spawnBabyDelay = 0;
}

void BreedGoal::tick() {
    if (!m_animal || !m_targetMate) return;

    // 看向配偶
    if (auto* lookCtrl = m_animal->lookController()) {
        lookCtrl->setLookPosition(
            m_targetMate->x(),
            m_targetMate->y() + m_targetMate->eyeHeight(),
            m_targetMate->z()
        );
    }

    // 移动向配偶
    m_animal->tryMoveTo(m_targetMate->x(), m_targetMate->y(), m_targetMate->z(), m_speed);

    m_spawnBabyDelay++;

    // 检查是否足够接近以繁殖
    f32 dx = m_targetMate->x() - m_animal->x();
    f32 dy = m_targetMate->y() - m_animal->y();
    f32 dz = m_targetMate->z() - m_animal->z();
    f32 distSq = dx * dx + dy * dy + dz * dz;

    if (m_spawnBabyDelay >= SPAWN_BABY_DELAY && distSq < 9.0f) {
        spawnBaby();
    }
}

AnimalEntity* BreedGoal::findNearbyMate() {
    if (!m_animal || !m_animal->world()) return nullptr;

    IWorld* world = m_animal->world();

    // 搜索附近的动物
    auto entities = world->getEntitiesInRange(
        Vector3(m_animal->x(), m_animal->y(), m_animal->z()),
        MATE_SEARCH_RANGE,
        m_animal
    );

    AnimalEntity* closestMate = nullptr;
    f32 closestDist = MATE_SEARCH_RANGE * MATE_SEARCH_RANGE;

    for (Entity* entity : entities) {
        // 检查是否是动物
        AnimalEntity* animal = dynamic_cast<AnimalEntity*>(entity);
        if (!animal) continue;

        // 检查是否可以交配
        if (!m_animal->canMateWith(*animal)) continue;

        // 计算距离
        f32 dx = animal->x() - m_animal->x();
        f32 dy = animal->y() - m_animal->y();
        f32 dz = animal->z() - m_animal->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDist) {
            closestDist = distSq;
            closestMate = animal;
        }
    }

    return closestMate;
}

void BreedGoal::spawnBaby() {
    if (!m_animal || !m_targetMate) return;

    // 重置爱心状态
    m_animal->setInLove(0);
    m_targetMate->setInLove(0);

    // 生成幼体
    auto baby = m_animal->spawnBaby(*m_targetMate);
    if (baby && m_animal->world()) {
        // TODO: 将幼体添加到世界
        // ServerWorld* serverWorld = ...;
        // baby->setPosition(m_animal->x(), m_animal->y(), m_animal->z());
        // serverWorld->spawnEntity(std::move(baby));
    }
}

} // namespace mr::entity::ai::goal
