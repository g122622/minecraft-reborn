#pragma once

#include "AttackContext.hpp"
#include "../../core/Types.hpp"

namespace mr {

// 前向声明
class PlayerEntity;
class LivingEntity;

namespace entity::combat {

/**
 * @brief 玩家攻击辅助类
 *
 * 提供玩家攻击相关的辅助函数，包括：
 * - 暴击判定
 * - 攻击冷却
 * - 击退计算
 * - 附魔伤害加成
 *
 * 参考 MC 1.16.5 PlayerInteractionManager 和 PlayerEntity 攻击逻辑
 */
class PlayerAttackHelper {
public:
    /**
     * @brief 检查是否为暴击
     *
     * 暴击条件：
     * - 玩家正在下落
     * - 玩家不在地面
     * - 玩家不在水中
     * - 玩家不在梯子上
     * - 玩家没有失明效果
     * - 玩家没有骑乘
     *
     * @param player 玩家
     * @return 是否满足暴击条件
     */
    [[nodiscard]] static bool isCriticalHit(const PlayerEntity& player);

    /**
     * @brief 计算攻击伤害
     *
     * @param player 玩家
     * @param baseDamage 基础伤害
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @return 计算后的伤害值
     */
    [[nodiscard]] static f32 calculateDamage(const PlayerEntity& player,
                                              f32 baseDamage,
                                              f32 cooldownProgress);

    /**
     * @brief 计算击退强度
     *
     * @param attacker 攻击者
     * @param target 目标
     * @param baseKnockback 基础击退强度
     * @param isSprinting 是否在疾跑
     * @return 击退强度
     */
    [[nodiscard]] static f32 calculateKnockback(const LivingEntity& attacker,
                                                  const LivingEntity& target,
                                                  f32 baseKnockback = 1.0f,
                                                  bool isSprinting = false);

    /**
     * @brief 应用击退
     *
     * @param target 目标
     * @param attacker 攻击者
     * @param strength 击退强度
     */
    static void applyKnockback(LivingEntity& target,
                               const LivingEntity& attacker,
                               f32 strength);

    /**
     * @brief 应用攻击冷却影响
     *
     * @param damage 原始伤害
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @return 调整后的伤害
     */
    [[nodiscard]] static f32 applyCooldown(f32 damage, f32 cooldownProgress);

    /**
     * @brief 检查攻击冷却是否足够
     *
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @param threshold 阈值（默认 0.9）
     * @return 是否可以造成完整伤害
     */
    [[nodiscard]] static bool isCooldownReady(f32 cooldownProgress, f32 threshold = 0.9f);

    /**
     * @brief 获取攻击冷却进度
     *
     * @param ticksSinceLastAttack 自上次攻击以来的 tick 数
     * @param attackSpeed 攻击速度
     * @return 冷却进度 (0-1)
     */
    [[nodiscard]] static f32 getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed);

    /**
     * @brief 应用火焰附加
     *
     * @param target 目标
     * @param fireAspectLevel 火焰附加等级
     * @return 是否成功应用
     */
    static bool applyFireAspect(LivingEntity& target, i32 fireAspectLevel);

    /**
     * @brief 创建攻击上下文
     *
     * @param player 攻击玩家
     * @param target 目标
     * @param cooldownProgress 攻击冷却进度
     * @return 配置好的攻击上下文
     */
    [[nodiscard]] static AttackContext createContext(PlayerEntity& player,
                                                      LivingEntity& target,
                                                      f32 cooldownProgress);

private:
    // 常量
    static constexpr f32 CRITICAL_MULTIPLIER = 1.5f;       // 暴击伤害倍率
    static constexpr f32 SPRINT_KNOCKBACK_BONUS = 0.5f;    // 疾跑击退加成
    static constexpr i32 FIRE_ASPECT_DURATION = 80;        // 火焰附加基础持续时间（4秒）
    static constexpr f32 MIN_COOLDOWN_THRESHOLD = 0.9f;    // 最小冷却阈值
};

} // namespace entity::combat
} // namespace mr
