#include "TemptGoal.hpp"
#include "../../../mob/CreatureEntity.hpp"
#include "../../../mob/MobEntity.hpp"
#include "../../../living/LivingEntity.hpp"
#include "../../../Entity.hpp"
#include "../../../ai/controller/LookController.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../math/random/Random.hpp"
#include <cmath>

namespace mr::entity::ai::goal {

TemptGoal::TemptGoal(CreatureEntity* creature, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement)
    : m_creature(creature)
    , m_speed(speed)
    , m_itemPredicate(std::move(itemPredicate))
    , m_scaredByMovement(scaredByMovement)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool TemptGoal::shouldExecute() {
    if (!m_creature) return false;

    // 检查冷却
    if (m_delayTemptCounter > 0) {
        m_delayTemptCounter--;
        return false;
    }

    // 寻找手持诱惑物品的玩家
    m_temptingPlayer = findTemptingPlayer();
    return m_temptingPlayer != nullptr;
}

bool TemptGoal::shouldContinueExecuting() {
    if (!m_creature || !m_temptingPlayer) return false;

    // 检查玩家是否存活
    if (!m_temptingPlayer->isAlive()) return false;

    // 检查是否被玩家移动吓跑
    if (m_scaredByMovement) {
        f32 dx = m_temptingPlayer->x() - m_creature->x();
        f32 dy = m_temptingPlayer->y() - m_creature->y();
        f32 dz = m_temptingPlayer->z() - m_creature->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < 36.0f) { // 6格内
            // 检查玩家是否移动
            f32 playerDx = m_temptingPlayer->x() - m_targetX;
            f32 playerDy = m_temptingPlayer->y() - m_targetY;
            f32 playerDz = m_temptingPlayer->z() - m_targetZ;
            f32 playerDistSq = playerDx * playerDx + playerDy * playerDy + playerDz * playerDz;

            if (playerDistSq > 0.01f) {
                return false; // 玩家移动了，停止
            }

            // 检查玩家视角变化
            f32 pitchDiff = std::abs(m_temptingPlayer->pitch() - m_prevPitch);
            f32 yawDiff = std::abs(m_temptingPlayer->yaw() - m_prevYaw);

            if (pitchDiff > 5.0f || yawDiff > 5.0f) {
                return false; // 玩家视角变化，停止
            }
        } else {
            // 更新目标位置
            m_targetX = m_temptingPlayer->x();
            m_targetY = m_temptingPlayer->y();
            m_targetZ = m_temptingPlayer->z();
        }

        m_prevPitch = m_temptingPlayer->pitch();
        m_prevYaw = m_temptingPlayer->yaw();
    }

    return shouldExecute();
}

void TemptGoal::startExecuting() {
    if (!m_temptingPlayer) return;

    m_targetX = m_temptingPlayer->x();
    m_targetY = m_temptingPlayer->y();
    m_targetZ = m_temptingPlayer->z();
    m_isRunning = true;
}

void TemptGoal::resetTask() {
    m_temptingPlayer = nullptr;
    m_isRunning = false;

    if (m_creature) {
        auto* nav = m_creature->navigator();
        if (nav) {
            nav->clearPath();
        }
    }

    m_delayTemptCounter = TEMPT_COOLDOWN;
}

void TemptGoal::tick() {
    if (!m_creature || !m_temptingPlayer) return;

    // 看向玩家
    if (auto* lookCtrl = m_creature->lookController()) {
        lookCtrl->setLookPosition(
            m_temptingPlayer->x(),
            m_temptingPlayer->y() + m_temptingPlayer->eyeHeight(),
            m_temptingPlayer->z()
        );
    }

    // 计算与玩家的距离
    f32 dx = m_temptingPlayer->x() - m_creature->x();
    f32 dy = m_temptingPlayer->y() - m_creature->y();
    f32 dz = m_temptingPlayer->z() - m_creature->z();
    f32 distSq = dx * dx + dy * dy + dz * dz;

    // 如果距离太近，停止移动
    if (distSq < 6.25f) { // 2.5格内
        auto* nav = m_creature->navigator();
        if (nav) {
            nav->clearPath();
        }
    } else {
        // 跟随玩家
        m_creature->tryMoveTo(m_temptingPlayer->x(), m_temptingPlayer->y(), m_temptingPlayer->z(), m_speed);
    }
}

bool TemptGoal::isTempting(const ItemStack& stack) const {
    return m_itemPredicate(stack);
}

bool TemptGoal::isScaredByPlayerMovement() const {
    return m_scaredByMovement;
}

LivingEntity* TemptGoal::findTemptingPlayer() {
    if (!m_creature || !m_creature->world()) return nullptr;

    IWorld* world = m_creature->world();

    // 搜索附近的实体
    auto entities = world->getEntitiesInRange(
        Vector3(m_creature->x(), m_creature->y(), m_creature->z()),
        TEMPT_RANGE,
        m_creature
    );

    LivingEntity* closestEntity = nullptr;
    f32 closestDist = TEMPT_RANGE * TEMPT_RANGE;

    for (Entity* entity : entities) {
        // 检查是否是生物实体（简化处理，实际应该检查玩家）
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (!living) continue;

        // TODO: 检查玩家手持物品
        // const ItemStack& mainHand = player->getMainHandItem();
        // const ItemStack& offHand = player->getOffHandItem();
        // if (isTempting(mainHand) || isTempting(offHand)) {
        //     ... 找到玩家
        // }

        // 暂时返回找到的第一个生物实体
        f32 dx = living->x() - m_creature->x();
        f32 dy = living->y() - m_creature->y();
        f32 dz = living->z() - m_creature->z();
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDist) {
            closestDist = distSq;
            closestEntity = living;
        }
    }

    return closestEntity;
}

} // namespace mr::entity::ai::goal
