#include "BreedGoal.hpp"
#include "../../../animal/AnimalEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../EntityUtils.hpp"
#include "../../../ai/controller/LookController.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

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
    if (m_animal) {
        m_animal->clearNavigation();
    }
}

void BreedGoal::tick() {
    if (!m_animal || !m_targetMate) return;

    // 看向配偶
    m_animal->lookAt(*m_targetMate);

    // 移动向配偶
    m_animal->tryMoveTo(m_targetMate->x(), m_targetMate->y(), m_targetMate->z(), m_speed);

    m_spawnBabyDelay++;

    // 检查是否足够接近以繁殖
    f32 distSq = m_animal->distanceSqTo(*m_targetMate);
    if (m_spawnBabyDelay >= SPAWN_BABY_DELAY && distSq < BREED_DISTANCE_SQ) {
        spawnBaby();
    }
}

AnimalEntity* BreedGoal::findNearbyMate() {
    if (!m_animal || !m_animal->world()) return nullptr;

    return EntityUtils::findClosestEntity<AnimalEntity>(
        m_animal->world(),
        m_animal->position(),
        MATE_SEARCH_RANGE,
        m_animal,
        [this](AnimalEntity* animal) {
            return m_animal->canMateWith(*animal);
        }
    );
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

} // namespace mc::entity::ai::goal
