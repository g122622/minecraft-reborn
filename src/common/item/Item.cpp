#include "Item.hpp"
#include "ItemStack.hpp"
#include "ItemRegistry.hpp"
#include "../world/block/Block.hpp"
#include <sstream>

namespace mr {

// ============================================================================
// ItemProperties
// ============================================================================

ItemProperties& ItemProperties::maxStackSize(i32 maxStackSize) {
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    } else {
        m_maxStackSize = std::clamp(maxStackSize, 1, 64);
    }
    return *this;
}

ItemProperties& ItemProperties::maxDamage(i32 maxDamage) {
    m_maxDamage = std::max(0, maxDamage);
    if (m_maxDamage > 0) {
        // 有耐久度的物品不能堆叠
        m_maxStackSize = 1;
    }
    return *this;
}

ItemProperties& ItemProperties::containerItem(const Item* containerItem) {
    m_containerItem = containerItem;
    return *this;
}

ItemProperties& ItemProperties::rarity(ItemRarity rarity) {
    m_rarity = rarity;
    return *this;
}

ItemProperties& ItemProperties::burnable(bool value) {
    m_burnable = value;
    return *this;
}

ItemProperties& ItemProperties::repairable(bool value) {
    m_repairable = value;
    return *this;
}

// ============================================================================
// Item
// ============================================================================

Item::Item(ItemProperties properties)
    : m_maxStackSize(properties.maxStackSize())
    , m_maxDamage(properties.maxDamage())
    , m_containerItem(properties.containerItem())
    , m_rarity(properties.rarity())
    , m_burnable(properties.isBurnable())
    , m_repairable(properties.isRepairable()) {
}

Item* Item::getItem(ItemId itemId) {
    return ItemRegistry::instance().getItem(itemId);
}

Item* Item::getItem(const ResourceLocation& id) {
    return ItemRegistry::instance().getItem(id);
}

void Item::forEachItem(std::function<void(Item&)> callback) {
    ItemRegistry::instance().forEachItem(callback);
}

ItemStack Item::getDefaultInstance() const {
    return ItemStack(*this, 1);
}

f32 Item::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 默认挖掘速度为1.0
    // 工具类物品会重写此方法
    (void)stack;
    (void)state;
    return 1.0f;
}

bool Item::canHarvestBlock(const BlockState& state) const {
    // 默认不能采集需要工具的方块
    // 工具类物品会重写此方法
    (void)state;
    return false;
}

String Item::getTranslationKey() const {
    return "item." + m_itemLocation.toString();
}

String Item::getTranslationKey(const ItemStack& stack) const {
    (void)stack;
    return getTranslationKey();
}

String Item::getName() const {
    // 默认返回翻译键，未来可支持语言文件
    return getTranslationKey();
}

} // namespace mr
