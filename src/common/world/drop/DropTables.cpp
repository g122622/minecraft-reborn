#include "DropTables.hpp"
#include "../../item/Item.hpp"
#include "../../item/ItemRegistry.hpp"
#include "../../item/Items.hpp"
#include "../../math/random/Random.hpp"
#include <algorithm>
#include <random>

namespace mr {

// ============================================================================
// DropEntry
// ============================================================================

DropEntry::DropEntry(const Item& item, i32 minCount, i32 maxCount, f32 chance)
    : m_item(&item)
    , m_minCount(minCount)
    , m_maxCount(maxCount)
    , m_chance(chance)
    , m_empty(false)
{
}

DropEntry DropEntry::empty(f32 chance) {
    DropEntry entry(*ItemRegistry::instance().airItem(), 0, 0, chance);
    entry.m_empty = true;
    return entry;
}

bool DropEntry::checkCondition(const DropContext& context) const {
    switch (m_condition) {
        case DropCondition::None:
            return true;

        case DropCondition::SilkTouch:
            return context.silkTouch();

        case DropCondition::NoSilkTouch:
            return !context.silkTouch();

        case DropCondition::RandomChance: {
            math::Random rng(context.hasSeed() ? context.seed() : static_cast<u64>(std::random_device{}()));
            return rng.nextFloat() < m_chance;
        }

        case DropCondition::Fortune:
            // 时运条件始终满足，但影响数量
            return true;

        default:
            return true;
    }
}

ItemStack DropEntry::generate(const DropContext& context) const
{
    if (m_empty || m_item == nullptr) {
        return ItemStack::EMPTY;
    }

    // 检查概率
    math::Random rng(context.hasSeed() ? context.seed() : static_cast<u64>(std::random_device{}()));
    if (rng.nextFloat() >= m_chance) {
        return ItemStack::EMPTY;
    }

    // 计算数量
    i32 count = m_minCount;
    if (m_maxCount > m_minCount) {
        count = rng.nextInt(m_minCount, m_maxCount);
    }

    // 应用时运加成
    if (context.fortune() > 0 && m_fortuneBonus > 0) {
        // 时运每级增加掉落数量的概率
        i32 bonus = 0;
        for (i32 i = 0; i < context.fortune(); ++i) {
            bonus += rng.nextInt(m_fortuneBonus + 1);
        }
        count += bonus;
    }

    if (count <= 0) {
        return ItemStack::EMPTY;
    }

    return ItemStack(*m_item, count);
}

// ============================================================================
// DropTable
// ============================================================================

void DropTable::addEntry(const DropEntry& entry) {
    m_entries.push_back(entry);
}

DropTable& DropTable::addItem(const Item& item, i32 minCount, i32 maxCount) {
    m_entries.emplace_back(item, minCount, maxCount, 1.0f);
    return *this;
}

DropTable& DropTable::addItem(const Item& item, i32 minCount, i32 maxCount, f32 chance) {
    m_entries.emplace_back(item, minCount, maxCount, chance);
    return *this;
}

std::vector<ItemStack> DropTable::generateDrops(const DropContext& context) const {
    std::vector<ItemStack> drops;
    drops.reserve(m_entries.size());

    math::Random rng(context.hasSeed() ? context.seed() : static_cast<u64>(std::random_device{}()));

    for (const auto& entry : m_entries) {
        if (entry.checkCondition(context)) {
            ItemStack stack = entry.generate(context);
            if (!stack.isEmpty()) {
                // 尝试合并到现有堆
                bool merged = false;
                for (auto& existing : drops) {
                    if (existing.canMergeWith(stack)) {
                        i32 space = existing.getMaxStackSize() - existing.getCount();
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        stack.shrink(toAdd);
                        if (stack.isEmpty()) {
                            merged = true;
                            break;
                        }
                    }
                }

                if (!stack.isEmpty()) {
                    drops.push_back(stack);
                }
            }
        }
    }

    return drops;
}

std::vector<ItemStack> DropTable::generateSingleDrop(const DropContext& context) const {
    if (m_entries.empty()) {
        return {};
    }

    math::Random rng(context.hasSeed() ? context.seed() : static_cast<u64>(std::random_device{}()));

    // 过滤满足条件的条目
    std::vector<const DropEntry*> validEntries;
    for (const auto& entry : m_entries) {
        if (entry.checkCondition(context)) {
            validEntries.push_back(&entry);
        }
    }

    if (validEntries.empty()) {
        return {};
    }

    // 随机选择一个
    ItemStack stack = validEntries[static_cast<size_t>(rng.nextInt(static_cast<i32>(validEntries.size())))]->generate(context);

    if (stack.isEmpty()) {
        return {};
    }

    return { stack };
}

DropTable DropTable::oreDrop(const Item& oreItem, const Item& rawItem,
                              i32 minExp, i32 maxExp) {
    DropTable table;

    // 精准采集时掉落矿石方块
    DropEntry silkTouchEntry(oreItem, 1, 1, 1.0f);
    silkTouchEntry.setCondition(DropCondition::SilkTouch);
    table.addEntry(silkTouchEntry);

    // 非精准采集时掉落原材料
    DropEntry normalEntry(rawItem, 1, 1, 1.0f);
    normalEntry.setCondition(DropCondition::NoSilkTouch);
    normalEntry.setFortuneBonus(1);  // 时运每级最多增加1个
    table.addEntry(normalEntry);

    // 经验球（在游戏逻辑中处理）
    (void)minExp;
    (void)maxExp;

    return table;
}

DropTable DropTable::blockDrop(const Item& blockItem) {
    DropTable table;

    // 精准采集或常规情况都掉落方块本身
    table.addItem(blockItem, 1, 1);

    return table;
}

DropTable DropTable::multipleDrop(const Item& item, i32 min, i32 max) {
    DropTable table;
    table.addItem(item, min, max);
    return table;
}

// ============================================================================
// DropTableRegistry
// ============================================================================

DropTableRegistry& DropTableRegistry::instance() {
    static DropTableRegistry instance;
    return instance;
}

void DropTableRegistry::registerBlockDrop(BlockId blockId, const DropTable& table) {
    m_blockDrops[blockId] = table;
}

const DropTable* DropTableRegistry::getBlockDrop(BlockId blockId) const {
    auto it = m_blockDrops.find(blockId);
    return it != m_blockDrops.end() ? &it->second : nullptr;
}

std::vector<ItemStack> DropTableRegistry::generateBlockDrops(
    BlockId blockId, const DropContext& context) const {

    const DropTable* table = getBlockDrop(blockId);
    if (table) {
        return table->generateDrops(context);
    }
    return {};
}

void DropTableRegistry::initializeVanillaDrops() {
    if (m_initialized) {
        return;
    }

    // 确保物品系统已初始化
    Items::initialize();

    // 获取物品引用
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    Item* redstone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:redstone"));
    Item* lapis = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lapis_lazuli"));
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft:emerald"));

    // 注册矿石掉落
    // 钻石矿石
    if (diamond) {
        DropTable diamondOreTable;
        DropEntry diamondEntry(*diamond, 1, 1, 1.0f);
        diamondEntry.setCondition(DropCondition::NoSilkTouch);
        diamondEntry.setFortuneBonus(1);
        diamondOreTable.addEntry(diamondEntry);
        registerBlockDrop(static_cast<BlockId>(15), diamondOreTable);  // DIAMOND_ORE
    }

    // 煤矿石
    if (coal) {
        DropTable coalOreTable;
        DropEntry coalEntry(*coal, 1, 1, 1.0f);
        coalEntry.setCondition(DropCondition::NoSilkTouch);
        coalEntry.setFortuneBonus(1);
        coalOreTable.addEntry(coalEntry);
        registerBlockDrop(static_cast<BlockId>(16), coalOreTable);  // COAL_ORE
    }

    // 红石矿石
    if (redstone) {
        DropTable redstoneOreTable;
        DropEntry redstoneEntry(*redstone, 4, 5, 1.0f);
        redstoneEntry.setCondition(DropCondition::NoSilkTouch);
        redstoneEntry.setFortuneBonus(1);
        redstoneOreTable.addEntry(redstoneEntry);
        registerBlockDrop(static_cast<BlockId>(73), redstoneOreTable);  // REDSTONE_ORE
    }

    // 青金石矿石
    if (lapis) {
        DropTable lapisOreTable;
        DropEntry lapisEntry(*lapis, 4, 9, 1.0f);
        lapisEntry.setCondition(DropCondition::NoSilkTouch);
        lapisEntry.setFortuneBonus(2);
        lapisOreTable.addEntry(lapisEntry);
        registerBlockDrop(static_cast<BlockId>(21), lapisOreTable);  // LAPIS_ORE
    }

    // 绿宝石矿石
    if (emerald) {
        DropTable emeraldOreTable;
        DropEntry emeraldEntry(*emerald, 1, 1, 1.0f);
        emeraldEntry.setCondition(DropCondition::NoSilkTouch);
        emeraldEntry.setFortuneBonus(1);
        emeraldOreTable.addEntry(emeraldEntry);
        registerBlockDrop(static_cast<BlockId>(129), emeraldOreTable);  // EMERALD_ORE
    }

    m_initialized = true;
}

} // namespace mr
