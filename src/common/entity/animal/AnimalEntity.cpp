#include "AnimalEntity.hpp"

namespace mr {

AnimalEntity::AnimalEntity(LegacyEntityType type, EntityId id)
    : AgeableEntity(type, id)
{
}

bool AnimalEntity::isBreedingItem(const ItemStack& /*itemStack*/) const {
    // 默认实现：没有繁殖物品
    // 子类应该重写此方法
    return false;
}

bool AnimalEntity::canMateWith(const AnimalEntity& other) const {
    // 基本条件：
    // 1. 两个都是成体
    // 2. 两个都处于爱心状态
    // 3. 不是同一个实体
    // 4. 是同一种动物（子类可以重写来放宽此条件）

    if (this == &other) {
        return false;
    }

    if (isChild() || other.isChild()) {
        return false;
    }

    if (!isInLove() || !other.isInLove()) {
        return false;
    }

    // 检查是否是同一种动物
    // TODO: 比较 EntityType
    return true;
}

void AnimalEntity::tick() {
    AgeableEntity::tick();

    updateInLove();
}

void AnimalEntity::registerGoals() {
    // TODO: 注册基础动物 AI 目标
    // 子类应该调用此方法
}

void AnimalEntity::updateInLove() {
    if (m_inLoveTimer > 0) {
        --m_inLoveTimer;

        if (m_inLoveTimer == 0) {
            resetInLove();
        }
    }
}

void AnimalEntity::resetInLove() {
    m_inLoveTimer = 0;
    m_loveCause = 0;
}

} // namespace mr
