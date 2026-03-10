#include "MobEntity.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/controller/JumpController.hpp"

namespace mr {

MobEntity::MobEntity(LegacyEntityType type, EntityId id)
    : LivingEntity(type, id)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
{
    // 子类可在此初始化寻路器
}

MobEntity::~MobEntity() = default;

entity::ai::controller::LookController* MobEntity::lookController() {
    return m_lookController.get();
}

const entity::ai::controller::LookController* MobEntity::lookController() const {
    return m_lookController.get();
}

entity::ai::controller::MovementController* MobEntity::moveController() {
    return m_moveController.get();
}

const entity::ai::controller::MovementController* MobEntity::moveController() const {
    return m_moveController.get();
}

entity::ai::controller::JumpController* MobEntity::jumpController() {
    return m_jumpController.get();
}

const entity::ai::controller::JumpController* MobEntity::jumpController() const {
    return m_jumpController.get();
}

void MobEntity::tick() {
    // 更新父类
    LivingEntity::tick();

    // 更新 AI 目标
    if (m_aiEnabled) {
        m_goalSelector.tick();
        m_targetSelector.tick();
    }

    // 更新控制器
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }
}

} // namespace mr
