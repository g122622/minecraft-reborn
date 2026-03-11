#include "CowEntity.hpp"
#include "../../item/ItemStack.hpp"

namespace mr {

CowEntity::CowEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // TODO: 注册 AI 目标
}

bool CowEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 牛用小麦繁殖
    // TODO: 检查是否是小麦
    // return itemStack.getItem() == Items::WHEAT;
    (void)itemStack;
    return false;
}

bool CowEntity::canMateWith(const AnimalEntity& other) const {
    // 检查是否是牛
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> CowEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小牛
    return nullptr;
}

} // namespace mr
