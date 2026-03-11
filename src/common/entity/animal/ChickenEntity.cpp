#include "ChickenEntity.hpp"
#include "../../item/ItemStack.hpp"
#include "../../math/random/Random.hpp"

namespace mr {

ChickenEntity::ChickenEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
    // 初始化下蛋计时器
    resetEggTimer();
}

void ChickenEntity::resetEggTimer() {
    math::Random rng(ticksExisted());
    m_eggTimer = EGG_TIME_MIN + rng.nextInt(EGG_TIME_MAX - EGG_TIME_MIN);
}

bool ChickenEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 鸡用种子繁殖
    // TODO: 检查是否是种子
    // return itemStack.getItem()->isIn(ItemTags::SEEDS);
    (void)itemStack;
    return false;
}

bool ChickenEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> ChickenEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小鸡
    return nullptr;
}

void ChickenEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 鸡特有目标：食物诱惑（种子）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.0, [](const ItemStack& stack) { return stack.getItem()->isIn(ItemTags::SEEDS); }));
}

void ChickenEntity::tick() {
    AnimalEntity::tick();

    // 下蛋计时
    if (m_eggTimer > 0) {
        --m_eggTimer;

        if (m_eggTimer <= 0) {
            // 下蛋
            // TODO: 生成蛋物品实体
            resetEggTimer();
        }
    }
}

} // namespace mr
