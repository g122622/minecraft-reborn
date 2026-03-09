#pragma once

#include "../core/Types.hpp"
#include "../resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <functional>

namespace mr {

// Forward declarations
class BlockState;
class Item;
class ItemStack;
class ItemRegistry;

// ============================================================================
// 物品稀有度
// ============================================================================

/**
 * @brief 物品稀有度枚举
 *
 * 参考: net.minecraft.item.Rarity
 */
enum class ItemRarity : u8 {
    Common = 0,     // 普通 - 白色
    Uncommon = 1,   // 少见 - 黄色
    Rare = 2,       // 稀有 - 青色
    Epic = 3        // 史诗 - 紫色
};

// ============================================================================
// 物品属性构建器
// ============================================================================

/**
 * @brief 物品属性构建器
 *
 * 用于构建物品属性的流畅接口。参考MC的 Item.Properties。
 *
 * 用法示例:
 * @code
 * auto properties = ItemProperties()
 *     .maxStackSize(64)
 *     .maxDamage(250)
 *     .rarity(ItemRarity::Rare);
 * @endcode
 */
class ItemProperties {
public:
    ItemProperties() = default;

    /**
     * @brief 设置最大堆叠数量
     * @param maxStackSize 最大堆叠数（默认64，可设置1-64）
     * @note 如果物品有耐久度，堆叠数自动为1
     */
    ItemProperties& maxStackSize(i32 maxStackSize);

    /**
     * @brief 设置最大耐久度
     * @param maxDamage 最大耐久度（物品可承受的伤害值）
     * @note 设置耐久度后，堆叠数自动变为1
     */
    ItemProperties& maxDamage(i32 maxDamage);

    /**
     * @brief 设置容器物品（如桶装牛奶用完后返回桶）
     * @param containerItem 容器物品指针
     */
    ItemProperties& containerItem(const Item* containerItem);

    /**
     * @brief 设置稀有度
     */
    ItemProperties& rarity(ItemRarity rarity);

    /**
     * @brief 设置是否可燃烧
     */
    ItemProperties& burnable(bool value = true);

    /**
     * @brief 设置是否可修复
     */
    ItemProperties& repairable(bool value = true);

    // Getters
    [[nodiscard]] i32 maxStackSize() const { return m_maxStackSize; }
    [[nodiscard]] i32 maxDamage() const { return m_maxDamage; }
    [[nodiscard]] const Item* containerItem() const { return m_containerItem; }
    [[nodiscard]] ItemRarity rarity() const { return m_rarity; }
    [[nodiscard]] bool isBurnable() const { return m_burnable; }
    [[nodiscard]] bool isRepairable() const { return m_repairable; }

private:
    friend class Item;

    i32 m_maxStackSize = 64;
    i32 m_maxDamage = 0;
    const Item* m_containerItem = nullptr;
    ItemRarity m_rarity = ItemRarity::Common;
    bool m_burnable = false;
    bool m_repairable = true;
};

// ============================================================================
// 物品基类
// ============================================================================

/**
 * @brief 物品基类
 *
 * 所有物品类型的基类。物品通过 ItemRegistry 注册，
 * 每个物品有一个唯一的物品ID。
 *
 * 参考: net.minecraft.item.Item
 *
 * 用法示例:
 * @code
 * // 注册普通物品
 * auto& stick = ItemRegistry::instance().registerItem(
 *     ResourceLocation("minecraft:stick"),
 *     ItemProperties().maxStackSize(64)
 * );
 *
 * // 注册耐久物品
 * auto& sword = ItemRegistry::instance().registerItem<SwordItem>(
 *     ResourceLocation("minecraft:diamond_sword"),
 *     ItemProperties().maxDamage(1561)
 * );
 * @endcode
 */
class Item {
public:
    virtual ~Item() = default;

    // 禁止拷贝
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    // ========================================================================
    // 静态方法
    // ========================================================================

    /**
     * @brief 根据物品ID获取物品
     */
    [[nodiscard]] static Item* getItem(ItemId itemId);

    /**
     * @brief 根据资源位置获取物品
     */
    [[nodiscard]] static Item* getItem(const ResourceLocation& id);

    /**
     * @brief 遍历所有物品
     */
    static void forEachItem(std::function<void(Item&)> callback);

    // ========================================================================
    // 基本属性
    // ========================================================================

    /**
     * @brief 获取物品资源位置
     */
    [[nodiscard]] const ResourceLocation& itemLocation() const { return m_itemLocation; }

    /**
     * @brief 获取物品ID
     */
    [[nodiscard]] ItemId itemId() const { return m_itemId; }

    /**
     * @brief 获取最大堆叠数量
     */
    [[nodiscard]] i32 maxStackSize() const { return m_maxStackSize; }

    /**
     * @brief 获取最大耐久度
     * @return 最大耐久度，0表示不可损坏
     */
    [[nodiscard]] i32 maxDamage() const { return m_maxDamage; }

    /**
     * @brief 是否可损坏
     */
    [[nodiscard]] bool isDamageable() const { return m_maxDamage > 0; }

    /**
     * @brief 获取容器物品
     */
    [[nodiscard]] const Item* containerItem() const { return m_containerItem; }

    /**
     * @brief 是否有容器物品
     */
    [[nodiscard]] bool hasContainerItem() const { return m_containerItem != nullptr; }

    /**
     * @brief 获取稀有度
     */
    [[nodiscard]] ItemRarity rarity() const { return m_rarity; }

    /**
     * @brief 是否可燃烧
     */
    [[nodiscard]] bool isBurnable() const { return m_burnable; }

    /**
     * @brief 是否可修复
     */
    [[nodiscard]] bool isRepairable() const { return m_repairable; }

    // ========================================================================
    // 物品堆相关
    // ========================================================================

    /**
     * @brief 创建默认的物品堆
     */
    [[nodiscard]] ItemStack getDefaultInstance() const;

    // ========================================================================
    // 虚方法 - 子类可重写
    // ========================================================================

    /**
     * @brief 获取挖掘速度
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率（默认1.0）
     */
    [[nodiscard]] virtual f32 getDestroySpeed(const ItemStack& stack,
                                               const BlockState& state) const;

    /**
     * @brief 是否可以采集方块
     * @param state 目标方块状态
     * @return 是否可以采集
     */
    [[nodiscard]] virtual bool canHarvestBlock(const BlockState& state) const;

    /**
     * @brief 获取翻译键
     */
    [[nodiscard]] virtual String getTranslationKey() const;

    /**
     * @brief 获取翻译键（带物品堆）
     */
    [[nodiscard]] virtual String getTranslationKey(const ItemStack& stack) const;

    /**
     * @brief 获取物品名称
     *
     * 返回物品的简单名称（不含格式）。
     * 子类可重写以提供自定义名称。
     *
     * @return 物品名称
     */
    [[nodiscard]] virtual String getName() const;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值（0表示不可附魔）
     */
    [[nodiscard]] virtual i32 getItemEnchantability() const { return 0; }

    /**
     * @brief 物品是否为食物
     */
    [[nodiscard]] virtual bool isFood() const { return false; }

    /**
     * @brief 获取使用时间（如食物食用时间）
     * @return 使用时间（ticks），0表示不可使用
     */
    [[nodiscard]] virtual i32 getUseDuration(const ItemStack& stack) const { return 0; }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual String toString() const {
        return m_itemLocation.toString();
    }

protected:
    friend class ItemRegistry;

    /**
     * @brief 构造物品
     * @param properties 物品属性
     */
    explicit Item(ItemProperties properties);

    // 由 ItemRegistry 设置
    ResourceLocation m_itemLocation;
    ItemId m_itemId = 0;

    // 由构造函数设置
    i32 m_maxStackSize = 64;
    i32 m_maxDamage = 0;
    const Item* m_containerItem = nullptr;
    ItemRarity m_rarity = ItemRarity::Common;
    bool m_burnable = false;
    bool m_repairable = true;
};

} // namespace mr
