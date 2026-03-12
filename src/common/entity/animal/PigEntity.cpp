#include "PigEntity.hpp"
#include "../../item/ItemStack.hpp"
#include <memory>

namespace mr {

std::unique_ptr<Entity> PigEntity::create(IWorld* /*world*/) {
    // 创建一个临时ID，实际ID由实体管理器分配
    static EntityId nextId = 1;
    return std::make_unique<PigEntity>(LegacyEntityType::Unknown, nextId++);
}

PigEntity::PigEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
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

void PigEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 猪特有目标：食物诱惑（胡萝卜）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.2, [](const ItemStack& stack) { return stack.getItem() == Items::CARROT; }));
}

} // namespace mr
