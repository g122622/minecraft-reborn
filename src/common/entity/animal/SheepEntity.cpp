#include "SheepEntity.hpp"
#include "../../item/ItemStack.hpp"
#include <memory>

namespace mc {

std::unique_ptr<Entity> SheepEntity::create(IWorld* /*world*/) {
    // 创建一个临时ID，实际ID由实体管理器分配
    static EntityId nextId = 1;
    return std::make_unique<SheepEntity>(LegacyEntityType::Unknown, nextId++);
}

SheepEntity::SheepEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
    // SheepEntity 有特殊的 EatGrassGoal，后续添加
}

i32 SheepEntity::shear() {
    if (!m_hasWool) {
        return 0;
    }

    m_hasWool = false;

    // 返回羊毛数量（1-3个，受幸运影响）
    // TODO: 实际掉落逻辑
    return 1;
}

bool SheepEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 羊用小麦繁殖
    // TODO: 检查是否是小麦
    (void)itemStack;
    return false;
}

bool SheepEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SheepEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小羊，继承父母颜色混合
    return nullptr;
}

void SheepEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 羊特有目标：食物诱惑（小麦）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.0, [](const ItemStack& stack) { return stack.getItem() == Items::WHEAT; }));

    // TODO: 添加吃草目标 (EatGrassGoal)
}

void SheepEntity::tick() {
    // 吃草动画更新
    if (m_eatAnimationTimer > 0) {
        --m_eatAnimationTimer;
    }

    // 羊毛重新生长
    if (!m_hasWool) {
        // 每吃一次草有概率长出羊毛
        // TODO: 实现羊毛重新生长
    }

    AnimalEntity::tick();
}

} // namespace mc
