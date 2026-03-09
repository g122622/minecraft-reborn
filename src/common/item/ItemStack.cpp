#include "ItemStack.hpp"
#include "Item.hpp"
#include "ItemRegistry.hpp"
#include "../world/block/Block.hpp"
#include <algorithm>

namespace mr {

// ============================================================================
// 静态常量
// ============================================================================

const ItemStack ItemStack::EMPTY;

// ============================================================================
// 构造函数
// ============================================================================

ItemStack::ItemStack(const Item& item, i32 count)
    : m_item(&item)
    , m_count(count) {
    // 数量验证
    if (m_count <= 0) {
        m_item = nullptr;
        m_count = 0;
    }
}

ItemStack::ItemStack(const Item* item, i32 count)
    : m_item(item)
    , m_count(count) {
    // 数量验证
    if (m_count <= 0 || m_item == nullptr) {
        m_item = nullptr;
        m_count = 0;
    }
}

// ============================================================================
// 数量操作
// ============================================================================

void ItemStack::setCount(i32 count) {
    m_count = count;
    if (m_count <= 0) {
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
    }
}

i32 ItemStack::getMaxStackSize() const {
    if (isEmpty()) {
        return 0;
    }
    // 如果有耐久度，堆叠数为1
    if (m_item->isDamageable()) {
        return 1;
    }
    return m_item->maxStackSize();
}

// ============================================================================
// 耐久度
// ============================================================================

bool ItemStack::isDamageable() const {
    if (isEmpty()) {
        return false;
    }
    return m_item->isDamageable();
}

bool ItemStack::isDamaged() const {
    return isDamageable() && m_damage > 0;
}

void ItemStack::setDamage(i32 damage) {
    if (!isDamageable()) {
        return;
    }
    m_damage = std::max(0, damage);
    i32 maxDamage = getMaxDamage();
    if (m_damage >= maxDamage) {
        // 物品损坏，清空堆
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
    }
}

i32 ItemStack::getMaxDamage() const {
    if (isEmpty()) {
        return 0;
    }
    return m_item->maxDamage();
}

bool ItemStack::attemptDamageItem(i32 amount) {
    if (!isDamageable()) {
        return false;
    }

    m_damage += amount;
    i32 maxDamage = getMaxDamage();

    if (m_damage >= maxDamage) {
        // 物品损坏
        m_count = 0;
        m_item = nullptr;
        m_damage = 0;
        return true;
    }
    return false;
}

// ============================================================================
// 堆叠操作
// ============================================================================

bool ItemStack::canMergeWith(const ItemStack& other) const {
    if (isEmpty() || other.isEmpty()) {
        return false;
    }

    // 物品类型必须相同
    if (m_item != other.m_item) {
        return false;
    }

    // 如果有耐久度，需要耐久度相同才能合并
    if (isDamageable() && m_damage != other.m_damage) {
        return false;
    }

    // 检查是否还有堆叠空间
    return m_count < getMaxStackSize();
}

bool ItemStack::isSameItem(const ItemStack& other) const {
    if (isEmpty() && other.isEmpty()) {
        return true;
    }
    if (isEmpty() || other.isEmpty()) {
        return false;
    }
    return m_item == other.m_item;
}

ItemStack ItemStack::split(i32 amount) {
    if (isEmpty()) {
        return EMPTY;
    }

    i32 splitCount = std::min(amount, m_count);
    ItemStack result(*m_item, splitCount);
    result.m_damage = m_damage;

    // 减少当前堆
    setCount(m_count - splitCount);

    return result;
}

ItemStack ItemStack::copy() const {
    if (isEmpty()) {
        return EMPTY;
    }
    ItemStack result(*m_item, m_count);
    result.m_damage = m_damage;
    // TODO: 复制NBT数据
    return result;
}

// ============================================================================
// 物品功能
// ============================================================================

f32 ItemStack::getDestroySpeed(const BlockState& state) const {
    if (isEmpty()) {
        return 1.0f;
    }
    return m_item->getDestroySpeed(*this, state);
}

bool ItemStack::canHarvestBlock(const BlockState& state) const {
    if (isEmpty()) {
        return false;
    }
    return m_item->canHarvestBlock(state);
}

// ============================================================================
// 显示名称
// ============================================================================

String ItemStack::getDisplayName() const {
    if (isEmpty()) {
        return "";
    }
    // 目前返回物品名称，未来可支持自定义名称（NBT标签）
    return m_item->getName();
}

// ============================================================================
// 序列化
// ============================================================================

void ItemStack::serialize(network::PacketSerializer& ser) const {
    if (isEmpty()) {
        ser.writeBool(false);  // 表示空物品
        return;
    }

    ser.writeBool(true);  // 表示有物品
    ser.writeU16(m_item->itemId());
    ser.writeI32(m_count);

    // 耐久度
    if (m_item->isDamageable()) {
        ser.writeI32(m_damage);
    }
}

Result<ItemStack> ItemStack::deserialize(network::PacketDeserializer& deser) {
    auto hasItemResult = deser.readBool();
    if (hasItemResult.failed()) {
        return hasItemResult.error();
    }

    if (!hasItemResult.value()) {
        return EMPTY;
    }

    auto itemIdResult = deser.readU16();
    if (itemIdResult.failed()) {
        return itemIdResult.error();
    }
    ItemId itemId = itemIdResult.value();

    auto countResult = deser.readI32();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 count = countResult.value();

    Item* item = ItemRegistry::instance().getItem(itemId);
    if (item == nullptr) {
        return Error(ErrorCode::InvalidItem, "Unknown item ID: " + std::to_string(itemId));
    }

    ItemStack stack(*item, count);

    // 读取耐久度
    if (item->isDamageable()) {
        auto damageResult = deser.readI32();
        if (damageResult.failed()) {
            return damageResult.error();
        }
        stack.setDamage(damageResult.value());
    }

    return stack;
}

// ============================================================================
// 比较操作符
// ============================================================================

bool ItemStack::operator==(const ItemStack& other) const {
    if (isEmpty() && other.isEmpty()) {
        return true;
    }
    if (isEmpty() || other.isEmpty()) {
        return false;
    }

    return m_item == other.m_item &&
           m_count == other.m_count &&
           m_damage == other.m_damage;
}

} // namespace mr
