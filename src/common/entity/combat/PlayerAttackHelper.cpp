#include "PlayerAttackHelper.hpp"
#include "AttackContext.hpp"
#include "../../math/MathUtils.hpp"
#include <cmath>

namespace mr::entity::combat {

bool PlayerAttackHelper::isCriticalHit(const PlayerEntity& /*player*/) {
    // 暴击条件：
    // 1. 玩家正在下落（垂直速度 < 0）
    // 2. 玩家不在地面
    // 3. 玩家不在水中
    // 4. 玩家不在梯子/藤蔓上
    // 5. 玩家没有失明效果
    // 6. 玩家没有骑乘

    // TODO: 实现完整的暴击判定
    // 暂时返回 false，等待 PlayerEntity 类实现相关方法
    return false;
}

f32 PlayerAttackHelper::calculateDamage(const PlayerEntity& /*player*/,
                                          f32 baseDamage,
                                          f32 cooldownProgress) {
    f32 damage = baseDamage;

    // 应用攻击冷却
    damage = applyCooldown(damage, cooldownProgress);

    // TODO: 附魔加成（锋利、亡灵杀手、节肢杀手）
    // TODO: 药水效果加成（力量、虚弱）
    // TODO: 目标护甲减伤

    return damage;
}

f32 PlayerAttackHelper::calculateKnockback(const LivingEntity& /*attacker*/,
                                             const LivingEntity& /*target*/,
                                             f32 baseKnockback,
                                             bool isSprinting) {
    // 基础击退
    f32 knockback = baseKnockback;

    // 疾跑加成
    if (isSprinting) {
        knockback += SPRINT_KNOCKBACK_BONUS;
    }

    // TODO: 击退附魔加成
    // TODO: 目标击退抗性减伤

    return knockback;
}

void PlayerAttackHelper::applyKnockback(LivingEntity& /*target*/,
                                          const LivingEntity& /*attacker*/,
                                          f32 /*strength*/) {
    // TODO: 实现击退逻辑
    // 需要设置目标的速度
}

f32 PlayerAttackHelper::applyCooldown(f32 damage, f32 cooldownProgress) {
    // 冷却不足时伤害降低
    if (cooldownProgress < MIN_COOLDOWN_THRESHOLD) {
        return damage * cooldownProgress * cooldownProgress;
    }
    return damage;
}

bool PlayerAttackHelper::isCooldownReady(f32 cooldownProgress, f32 threshold) {
    return cooldownProgress >= threshold;
}

f32 PlayerAttackHelper::getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed) {
    // 攻击间隔 = 20 / attackSpeed tick
    f32 cooldownTime = 20.0f / attackSpeed;

    // 冷却进度 = 已过时间 / 攻击间隔
    f32 progress = static_cast<f32>(ticksSinceLastAttack) / cooldownTime;

    return math::clamp(progress, 0.0f, 1.0f);
}

bool PlayerAttackHelper::applyFireAspect(LivingEntity& /*target*/, i32 fireAspectLevel) {
    if (fireAspectLevel <= 0) {
        return false;
    }

    // 火焰持续时间 = 基础时间 * 等级
    i32 duration = FIRE_ASPECT_DURATION * fireAspectLevel;

    // TODO: 设置目标着火
    // target.setFire(duration);

    (void)duration;

    return true;
}

AttackContext PlayerAttackHelper::createContext(PlayerEntity& player,
                                                  LivingEntity& target,
                                                  f32 cooldownProgress) {
    // 使用 Entity* 作为攻击者
    AttackContext context(static_cast<Entity*>(static_cast<void*>(&player)), &target);

    // 设置攻击冷却
    context.setCooldownProgress(cooldownProgress);

    // 检查暴击
    if (isCriticalHit(player) && isCooldownReady(cooldownProgress)) {
        context.setCritical(true);
        context.setCriticalMultiplier(CRITICAL_MULTIPLIER);
    }

    // TODO: 获取基础伤害（从武器）
    // context.setBaseDamage(player.getAttackDamage());

    // TODO: 检查火焰附加
    // i32 fireAspect = player.getEnchantmentLevel(Enchantments::FIRE_ASPECT);
    // if (fireAspect > 0) {
    //     context.setFireDamage(true);
    //     context.setFireDuration(FIRE_ASPECT_DURATION * fireAspect);
    // }

    (void)cooldownProgress;

    return context;
}

} // namespace mr::entity::combat
