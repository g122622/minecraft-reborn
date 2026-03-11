#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"

namespace mr {

// 前向声明
class MobEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 看向实体目标
 *
 * 使生物看向附近的指定类型实体。
 *
 * 参考 MC 1.16.5 LookAtGoal
 */
class LookAtGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param watchTargetClass 要看向的实体类型（目前未使用）
     * @param maxDistance 最大观看距离
     */
    LookAtGoal(MobEntity* mob, f32 maxDistance);

    /**
     * @brief 构造函数（带概率）
     * @param mob 拥有此目标的生物
     * @param watchTargetClass 要看向的实体类型（目前未使用）
     * @param maxDistance 最大观看距离
     * @param chance 每tick执行的概率（0-1）
     */
    LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "LookAtGoal"; }

protected:
    MobEntity* m_mob;
    LivingEntity* m_lookTarget = nullptr;
    f32 m_maxDistance;
    f32 m_chance;
    i32 m_lookTime = 0;
    bool m_isLooking = false;

    /**
     * @brief 寻找要看的实体
     * @return 找到的实体，如果没有返回nullptr
     */
    [[nodiscard]] virtual LivingEntity* findTarget();
};

/**
 * @brief 随机看向目标
 *
 * 使生物随机看向往某个方向。
 *
 * 参考 MC 1.16.5 LookRandomlyGoal
 */
class LookRandomlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit LookRandomlyGoal(MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "LookRandomlyGoal"; }

private:
    MobEntity* m_mob;
    f32 m_targetYaw = 0.0f;
    f32 m_targetPitch = 0.0f;
    i32 m_lookTime = 0;
};

} // namespace entity::ai::goal
} // namespace mr
