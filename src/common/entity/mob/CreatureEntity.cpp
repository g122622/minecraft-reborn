#include "CreatureEntity.hpp"
#include "../ai/controller/MovementController.hpp"

namespace mr {

CreatureEntity::CreatureEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
}

bool CreatureEntity::tryMoveTo(f64 x, f64 y, f64 z, f64 speed) {
    // 设置移动控制器目标
    if (m_moveController) {
        m_moveController->setMoveTo(x, y, z, speed);
        return true;
    }
    return false;
}

f32 CreatureEntity::getPathWeight(f32 /*x*/, f32 /*y*/, f32 /*z*/) const {
    // 默认实现：返回0表示中性权重
    // 子类应该重写此方法来提供更准确的权重
    return 0.0f;
}

bool CreatureEntity::canSpawnAt(f32 /*x*/, f32 y, f32 /*z*/) const {
    // 默认实现：检查是否在有效位置
    // 子类应该重写此方法
    return y >= 0.0f;
}

} // namespace mr
