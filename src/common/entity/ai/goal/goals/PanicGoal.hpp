#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../math/Vector3.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 恐慌逃跑目标
 *
 * 当实体受到攻击或着火时，随机逃跑。
 *
 * 参考 MC 1.16.5 PanicGoal
 */
class PanicGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 逃跑速度倍率
     */
    PanicGoal(CreatureEntity* creature, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 检查是否正在逃跑
     */
    [[nodiscard]] bool isRunning() const { return m_running; }

    [[nodiscard]] String getTypeName() const override { return "PanicGoal"; }

protected:
    /**
     * @brief 寻找随机逃跑位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool findRandomPosition();

    /**
     * @brief 获取随机水源位置（着火时）
     * @param range 水平范围
     * @param verticalRange 垂直范围
     * @return 水源位置，如果没有则返回空
     */
    [[nodiscard]] Vector3 getRandomWaterPosition(i32 range, i32 verticalRange);

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_targetX = 0.0f;
    f32 m_targetY = 0.0f;
    f32 m_targetZ = 0.0f;
    bool m_running = false;
};

} // namespace entity::ai::goal
} // namespace mc
