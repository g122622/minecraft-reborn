#include "LivingEntity.hpp"
#include "../../core/Constants.hpp"
#include "../../math/MathUtils.hpp"
#include <cmath>

namespace mr {

// ============================================================================
// 静态数据参数定义
// ============================================================================

namespace {
    // 数据参数
    entity::DataParameter<i8> LIVING_FLAGS_PARAM{10};
    entity::DataParameter<f32> HEALTH_PARAM{11};
    entity::DataParameter<i32> POTION_EFFECTS_PARAM{12};
    entity::DataParameter<i32> ARROW_COUNT_PARAM{13};
}

// ============================================================================
// 构造函数
// ============================================================================

LivingEntity::LivingEntity(LegacyEntityType type, EntityId id, IWorld* world)
    : Entity(type, id, world)
{
    // 初始化装备槽
    for (auto& slot : m_equipment) {
        slot = ItemStack();
    }

    // 注册属性
    registerAttributes();
}

void LivingEntity::registerData() {
    Entity::registerData();

    // 注册生物数据参数
    m_dataManager.registerParam(LIVING_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(HEALTH_PARAM, m_health);
    m_dataManager.registerParam(POTION_EFFECTS_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(ARROW_COUNT_PARAM, static_cast<i32>(0));
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

    // 保存上一帧渲染属性
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_prevSwingProgress = m_swingProgress;
    m_prevRenderYawOffset = m_renderYawOffset;
    m_prevRotationYawHead = m_rotationYawHead;

    // 更新受伤无敌帧
    if (m_hurtTime > 0) {
        m_hurtTime--;
    }

    // 更新攻击动画
    if (m_swingInProgress) {
        m_swingProgressInt++;
        if (m_swingProgressInt >= 6) {
            m_swingProgressInt = 0;
            m_swingInProgress = false;
        }
        m_swingProgress = static_cast<f32>(m_swingProgressInt) / 6.0f;
    } else {
        m_swingProgress = 0.0f;
        m_swingProgressInt = 0;
    }

    // 更新步态动画
    updateAnimation();

    // 更新生命值
    tickHealth();

    // 更新死亡
    if (isDead()) {
        tickDeath();
    }
}

void LivingEntity::updateAnimation() {
    // 计算移动距离
    f32 dx = x() - prevX();
    f32 dz = z() - prevZ();
    f32 distance = std::sqrt(dx * dx + dz * dz);

    // 更新步态动画
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_limbSwingAmount += (distance - m_limbSwingAmount) * 0.4f;

    // 如果在移动，增加步态周期
    if (distance > 0.001f) {
        m_limbSwing += std::min(distance, 1.0f);
    }

    // 更新移动距离
    m_prevMovedDistance = m_movedDistance;
    m_movedDistance = distance;
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

// ============================================================================
// 摔落伤害
// ============================================================================

void LivingEntity::handleFallDamage(f32 distance, f32 damageMultiplier) {
    // 计算摔落伤害
    // MC 规则：摔落 > 3 格才开始受伤，每格 1 点伤害
    if (distance > 3.0f) {
        f32 damage = (distance - 3.0f) * damageMultiplier;

        // 考虑摔落保护效果
        // TODO: 实现摔落保护附魔

        if (damage > 0.0f) {
            // 创建摔落伤害来源
            EnvironmentalDamage source = DamageSources::fall();
            hurt(source, damage);
        }
    }
}

} // namespace mr
