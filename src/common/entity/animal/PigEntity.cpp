#include "PigEntity.hpp"
#include "../../item/ItemStack.hpp"

namespace mr {

PigEntity::PigEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // TODO: 注册 AI 目标
    // registerGoals();
}

bool PigEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 猪用胡萝卜繁殖
    // TODO: 检查是否是胡萝卜
    // return itemStack.getItem() == Items::CARROT;
    (void)itemStack;
    return false;
}

bool PigEntity::canMateWith(const AnimalEntity& other) const {
    // 检查是否是猪
    // TODO: 类型检查
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> PigEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小猪
    // return std::make_unique<PigEntity>(...);
    return nullptr;
}

} // namespace mr
