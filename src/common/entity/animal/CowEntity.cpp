#include "CowEntity.hpp"
#include "../../item/ItemStack.hpp"

namespace mr {

CowEntity::CowEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
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

void CowEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 牛特有目标：食物诱惑（小麦）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.0, [](const ItemStack& stack) { return stack.getItem() == Items::WHEAT; }));
}

} // namespace mr
