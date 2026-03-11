#include "AttackContext.hpp"
#include "../damage/DamageSource.hpp"

namespace mr::entity::combat {

AttackContext::AttackContext(Entity* attacker, LivingEntity* target)
    : m_attacker(attacker)
    , m_target(target)
{
    // attacker 可能为空（环境伤害）
}

f32 AttackContext::calculateFinalDamage() const {
    f32 damage = m_baseDamage;

    // 暴击加成
    if (m_critical) {
        damage *= m_criticalMultiplier;
    }

    // 攻击冷却影响（玩家攻击）
    if (m_cooldownProgress < 1.0f) {
        // 冷却不足时伤害降低
        damage *= m_cooldownProgress;
    }

    // TODO: 附魔加成
    // TODO: 药水效果加成
    // TODO: 目标护甲减伤

    return damage;
}

std::unique_ptr<DamageSource> AttackContext::createDamageSource() const {
    switch (m_attackType) {
        case AttackType::Melee:
            if (m_attacker) {
                return std::make_unique<EntityDamageSource>(
                    m_fireDamage ? DamageType::OnFire : DamageType::MobAttack,
                    m_attacker
                );
            }
            break;

        case AttackType::Ranged:
            if (m_attacker) {
                return std::make_unique<IndirectEntityDamageSource>(
                    DamageType::Arrow,
                    nullptr,  // 直接攻击者（箭矢等）
                    m_attacker
                );
            }
            break;

        case AttackType::Magic:
            return std::make_unique<EnvironmentalDamage>(DamageType::Magic);

        case AttackType::Explosion:
            return std::make_unique<EnvironmentalDamage>(DamageType::Explosion);

        case AttackType::Thorns:
            return std::make_unique<EnvironmentalDamage>(DamageType::Thorns);
    }

    // 默认返回通用伤害
    return std::make_unique<EnvironmentalDamage>(DamageType::Generic);
}

} // namespace mr::entity::combat
