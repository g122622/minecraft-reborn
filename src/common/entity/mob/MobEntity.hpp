#pragma once

#include "../living/LivingEntity.hpp"
#include "../ai/goal/GoalSelector.hpp"
#include <memory>

namespace mr {

// 前向声明
namespace entity::ai::controller {
    class LookController;
    class MovementController;
    class JumpController;
}

/**
 * @brief Mob 实体基类
 *
 * 所有 AI 生物的基类，包括怪物和动物。
 * 提供 AI 目标系统、控制器、寻路等功能。
 *
 * 参考 MC 1.16.5 MobEntity
 */
class MobEntity : public LivingEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    MobEntity(LegacyEntityType type, EntityId id);

    ~MobEntity() override;

    // 禁止拷贝
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    // 允许移动
    MobEntity(MobEntity&&) = default;
    MobEntity& operator=(MobEntity&&) = default;

    // ========== AI 目标系统 ==========

    /**
     * @brief 获取行为目标选择器
     */
    [[nodiscard]] entity::ai::GoalSelector& goalSelector() { return m_goalSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& goalSelector() const { return m_goalSelector; }

    /**
     * @brief 获取目标选择器（攻击目标等）
     */
    [[nodiscard]] entity::ai::GoalSelector& targetSelector() { return m_targetSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& targetSelector() const { return m_targetSelector; }

    /**
     * @brief 注册 AI 目标
     *
     * 子类应重写此方法来注册自己的 AI 目标。
     */
    virtual void registerGoals() {}

    // ========== 控制器 ==========

    /**
     * @brief 获取视线控制器
     */
    [[nodiscard]] entity::ai::controller::LookController* lookController();
    [[nodiscard]] const entity::ai::controller::LookController* lookController() const;

    /**
     * @brief 获取移动控制器
     */
    [[nodiscard]] entity::ai::controller::MovementController* moveController();
    [[nodiscard]] const entity::ai::controller::MovementController* moveController() const;

    /**
     * @brief 获取跳跃控制器
     */
    [[nodiscard]] entity::ai::controller::JumpController* jumpController();
    [[nodiscard]] const entity::ai::controller::JumpController* jumpController() const;

    // ========== 目标 ==========

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* attackTarget() { return m_attackTarget; }
    [[nodiscard]] const LivingEntity* attackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    // ========== 刻更新 ==========

    void tick() override;

protected:
    // AI 目标选择器
    entity::ai::GoalSelector m_goalSelector;
    entity::ai::GoalSelector m_targetSelector;

    // 控制器
    std::unique_ptr<entity::ai::controller::LookController> m_lookController;
    std::unique_ptr<entity::ai::controller::MovementController> m_moveController;
    std::unique_ptr<entity::ai::controller::JumpController> m_jumpController;

    // 攻击目标
    LivingEntity* m_attackTarget = nullptr;

    // AI 状态
    bool m_aiEnabled = true;
    i32 m_idleTime = 0;  // 空闲时间（用于随机漫步等）
};

} // namespace mr
