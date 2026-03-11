#include "SwimGoal.hpp"
#include "../../../mob/MobEntity.hpp"

namespace mr::entity::ai::goal {

SwimGoal::SwimGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Jump});
}

bool SwimGoal::shouldExecute() {
    if (!m_mob) return false;

    // TODO: 检查实体是否在水中或岩浆中
    // 目前简单返回 false，需要在 Entity 中添加 isInWater() 和 isInLava() 方法
    return false;
}

void SwimGoal::tick() {
    if (!m_mob) return;

    // 随机跳跃以游泳
    // TODO: 添加随机数生成器
    // if (m_mob->getRandom().nextFloat() < 0.8f) {
    //     m_mob->getJumpController()->setJumping();
    // }
}

} // namespace mr::entity::ai::goal
