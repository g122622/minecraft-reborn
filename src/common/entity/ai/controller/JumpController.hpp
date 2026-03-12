#pragma once

#include "../../../core/Types.hpp"

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 跳跃控制器
 *
 * 控制实体的跳跃行为。
 *
 * 参考 MC 1.16.5 JumpController
 */
class JumpController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit JumpController(MobEntity* mob);

    /**
     * @brief 设置跳跃状态
     */
    void setJumping();

    /**
     * @brief 是否正在跳跃
     */
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    /**
     * @brief 刻更新
     *
     * 每tick调用，将跳跃状态应用到实体。
     */
    void tick();

private:
    MobEntity* m_mob;
    bool m_isJumping = false;
};

} // namespace entity::ai::controller
} // namespace mc
