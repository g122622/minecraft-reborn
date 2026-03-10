#include "LivingEntity.hpp"
#include "../../core/Constants.hpp"

namespace mr {

// ============================================================================
// 构造函数
// ============================================================================

LivingEntity::LivingEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    // 初始化装备槽
    for (auto& slot : m_equipment) {
        slot = ItemStack();
    }
}

// ============================================================================
// 生命值
// ============================================================================

f32 LivingEntity::maxHealth() const {
    return static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MAX_HEALTH, 20.0));
}

void LivingEntity::setHealth(f32 health) {
    f32 max = maxHealth();
    m_health = std::max(0.0f, std::min(health, max));
}

void LivingEntity::heal(f32 amount) {
    if (amount > 0.0f && !isDead()) {
        setHealth(m_health + amount);
    }
}

bool LivingEntity::hurt(DamageSource& /*source*/, f32 amount) {
    if (m_hurtTime > 0) {
        return false;  // 无敌帧期间
    }

    // 吸收值优先
    if (m_absorption > 0.0f) {
        f32 absorbed = std::min(m_absorption, amount);
        m_absorption -= absorbed;
        amount -= absorbed;
    }

    if (amount > 0.0f) {
        m_health -= amount;
        m_lastHealth = m_health;
        m_hurtTime = m_maxHurtTime;
        m_lastDamage = amount;

        if (m_health <= 0.0f) {
            // 实际死亡将在 die() 中处理
        }

        return true;
    }

    return false;
}

void LivingEntity::die(DamageSource& /*cause*/) {
    if (!isDead()) {
        return;
    }

    m_deathTime = 0;
    // TODO: 掉落物品、经验等
}

// ============================================================================
// 属性
// ============================================================================

void LivingEntity::registerAttributes() {
    m_attributes.registerAttribute(*entity::attribute::Attributes::maxHealth());
    m_attributes.registerAttribute(*entity::attribute::Attributes::knockbackResistance());
    m_attributes.registerAttribute(*entity::attribute::Attributes::movementSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armor());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armorToughness());
}

f64 LivingEntity::getAttributeValue(const String& name, f64 defaultValue) const {
    return m_attributes.getValue(name, defaultValue);
}

void LivingEntity::setAttributeBaseValue(const String& name, f64 value) {
    m_attributes.setBaseValue(name, value);
}

// ============================================================================
// 装备
// ============================================================================

const ItemStack& LivingEntity::getEquipment(EquipmentSlot slot) const {
    size_t index = static_cast<size_t>(slot);
    if (index >= m_equipment.size()) {
        static ItemStack empty;
        return empty;
    }
    return m_equipment[index];
}

void LivingEntity::setEquipment(EquipmentSlot slot, const ItemStack& stack) {
    size_t index = static_cast<size_t>(slot);
    if (index < m_equipment.size()) {
        m_equipment[index] = stack;
    }
}

// ============================================================================
// 受伤无敌帧
// ============================================================================

bool LivingEntity::isInvulnerableTo(DamageSource& /*source*/) const {
    return m_hurtTime > 0;
}

// ============================================================================
// 刻更新
// ============================================================================

void LivingEntity::tick() {
    Entity::tick();

    // 更新受伤无敌帧
    if (m_hurtTime > 0) {
        m_hurtTime--;
    }

    // 更新生命值
    tickHealth();

    // 更新死亡
    if (isDead()) {
        tickDeath();
    }
}

void LivingEntity::tickHealth() {
    // 自然回血（和平模式或生命恢复效果）
    // TODO: 实现生命恢复效果检查

    // 更新属性缓存
    for (auto& [name, instance] : m_attributes.allInstances()) {
        if (instance->isDirty()) {
            instance->getValue();  // 重新计算并缓存
        }
    }
}

void LivingEntity::tickDeath() {
    m_deathTime++;

    // 死亡动画（20 ticks = 1 秒）
    if (m_deathTime >= 20) {
        remove();  // 移除实体
    }
}

} // namespace mr
