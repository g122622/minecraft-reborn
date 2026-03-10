#include "JumpController.hpp"
#include "../../mob/MobEntity.hpp"

namespace mr::entity::ai::controller {

JumpController::JumpController(MobEntity* mob)
    : m_mob(mob)
{}

void JumpController::setJumping() {
    m_isJumping = true;
}

void JumpController::tick() {
    // TODO: 调用 m_mob->setJumping(m_isJumping)
    // 需要在 MobEntity 中添加跳跃状态
    m_isJumping = false;
}

} // namespace mr::entity::ai::controller
