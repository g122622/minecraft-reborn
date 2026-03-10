#pragma once

#include "../Entity.hpp"
#include "../attribute/AttributeMap.hpp"
#include "../attribute/Attributes.hpp"
#include "../damage/DamageSource.hpp"
#include "../../item/ItemStack.hpp"
#include <array>
#include <memory>

namespace mr {

// 前向声明
class World;

/**
 * @brief 装备槽位
 *
 * 定义实体可穿戴的装备槽位
 */
enum class EquipmentSlot : u8 {
    MainHand = 0,   // 主手
    OffHand = 1,    // 副手
    Feet = 2,       // 靴子
    Legs = 3,       // 护腿
    Chest = 4,      // 胸甲
    Head = 5,       // 头盔
    Count = 6       // 槽位数量
};

/**
 * @brief 生物实体基类
 *
 * 所有有生命值的实体的基类，包括玩家、怪物、动物等。
 * 提供生命值、属性、装备、药水效果等功能。
 *
 * 参考 MC 1.16.5 LivingEntity
 */
class LivingEntity : public Entity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    LivingEntity(LegacyEntityType type, EntityId id);

    ~LivingEntity() override = default;

    // 禁止拷贝
    LivingEntity(const LivingEntity&) = delete;
    LivingEntity& operator=(const LivingEntity&) = delete;

    // 允许移动
    LivingEntity(LivingEntity&&) = default;
    LivingEntity& operator=(LivingEntity&&) = default;

    // ========== 生命值 ==========

    /**
     * @brief 获取当前生命值
     */
    [[nodiscard]] f32 health() const { return m_health; }

    /**
     * @brief 获取最大生命值
     */
    [[nodiscard]] f32 maxHealth() const;

    /**
     * @brief 设置生命值
     * @param health 新生命值
     */
    void setHealth(f32 health);

    /**
     * @brief 治疗实体
     * @param amount 治疗量
     */
    void heal(f32 amount);

    /**
     * @brief 受伤
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否成功造成伤害
     */
    virtual bool hurt(DamageSource& source, f32 amount);

    /**
     * @brief 是否死亡
     */
    [[nodiscard]] bool isDead() const { return m_health <= 0.0f; }

    /**
     * @brief 死亡
     * @param cause 死亡原因
     */
    virtual void die(DamageSource& cause);

    // ========== 属性 ==========

    /**
     * @brief 获取属性映射表
     */
    entity::attribute::AttributeMap& attributes() { return m_attributes; }
    [[nodiscard]] const entity::attribute::AttributeMap& attributes() const { return m_attributes; }

    /**
     * @brief 注册默认属性
     *
     * 子类应重写此方法来注册自己的属性
     */
    virtual void registerAttributes();

    /**
     * @brief 获取属性值
     * @param name 属性名称
     * @param defaultValue 默认值
     */
    [[nodiscard]] f64 getAttributeValue(const String& name, f64 defaultValue = 0.0) const;

    /**
     * @brief 设置属性基础值
     * @param name 属性名称
     * @param value 新值
     */
    void setAttributeBaseValue(const String& name, f64 value);

    // ========== 装备 ==========

    /**
     * @brief 获取装备
     * @param slot 装备槽位
     */
    [[nodiscard]] const ItemStack& getEquipment(EquipmentSlot slot) const;

    /**
     * @brief 设置装备
     * @param slot 装备槽位
     * @param stack 物品堆
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack);

    /**
     * @brief 获取主手物品
     */
    [[nodiscard]] const ItemStack& getMainHandItem() const { return getEquipment(EquipmentSlot::MainHand); }

    /**
     * @brief 设置主手物品
     */
    void setMainHandItem(const ItemStack& stack) { setEquipment(EquipmentSlot::MainHand, stack); }

    /**
     * @brief 获取副手物品
     */
    [[nodiscard]] const ItemStack& getOffHandItem() const { return getEquipment(EquipmentSlot::OffHand); }

    /**
     * @brief 设置副手物品
     */
    void setOffHandItem(const ItemStack& stack) { setEquipment(EquipmentSlot::OffHand, stack); }

    // ========== 受伤无敌帧 ==========

    /**
     * @brief 获取受伤无敌时间
     */
    [[nodiscard]] i32 hurtTime() const { return m_hurtTime; }

    /**
     * @brief 获取最大受伤无敌时间
     */
    [[nodiscard]] i32 maxHurtTime() const { return m_maxHurtTime; }

    /**
     * @brief 是否处于受伤无敌状态
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const;

    /**
     * @brief 获取最近受伤来源
     */
    [[nodiscard]] DamageSource* lastDamageSource() const { return m_lastDamageSource.get(); }

    // ========== 死亡 ==========

    /**
     * @brief 是否正在死亡
     */
    [[nodiscard]] bool isDying() const { return m_deathTime > 0; }

    /**
     * @brief 获取死亡时间
     */
    [[nodiscard]] i32 deathTime() const { return m_deathTime; }

    // ========== 刻更新 ==========

    void tick() override;

    /**
     * @brief 生命值刻更新
     */
    virtual void tickHealth();

    /**
     * @brief 死亡刻更新
     */
    virtual void tickDeath();

protected:
    // 生命值
    f32 m_health = 20.0f;
    f32 m_lastHealth = 20.0f;           // 上一tick的生命值
    f32 m_absorption = 0.0f;             // 吸收值（金苹果）

    // 属性
    entity::attribute::AttributeMap m_attributes;

    // 装备
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_equipment;

    // 受伤无敌帧
    i32 m_hurtTime = 0;                  // 受伤无敌时间
    i32 m_maxHurtTime = 10;              // 最大受伤无敌时间
    f32 m_lastDamage = 0.0f;             // 最近伤害量
    std::unique_ptr<DamageSource> m_lastDamageSource;  // 最近伤害来源

    // 死亡
    i32 m_deathTime = 0;                 // 死亡时间

    // 回血
    i32 m_healTime = 0;                  // 回血计时器
};

} // namespace mr
