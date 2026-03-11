#include "LootTable.hpp"
#include "common/item/ItemRegistry.hpp"
#include <algorithm>

namespace mr {
namespace loot {

// ============================================================================
// LootTable
// ============================================================================

const LootTable LootTable::EMPTY;

void LootTable::addPool(std::unique_ptr<LootPool> pool) {
    if (pool) {
        m_pools.push_back(std::move(pool));
    }
}

LootPool* LootTable::getPool(const String& name) {
    for (auto& pool : m_pools) {
        if (pool->getName() == name) {
            return pool.get();
        }
    }
    return nullptr;
}

std::unique_ptr<LootPool> LootTable::removePool(const String& name) {
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        if ((*it)->getName() == name) {
            auto pool = std::move(*it);
            m_pools.erase(it);
            return pool;
        }
    }
    return nullptr;
}

std::vector<ItemStack> LootTable::generate(LootContext& context) const {
    std::vector<ItemStack> items;

    // 处理物品堆叠
    auto consumer = [&items](const ItemStack& stack) {
        if (!stack.isEmpty()) {
            // 尝试合并到现有堆
            for (auto& existing : items) {
                if (existing.canMergeWith(stack)) {
                    i32 space = existing.getMaxStackSize() - existing.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        if (toAdd >= stack.getCount()) {
                            return;  // 完全合并
                        }
                        // 部分合并，创建新堆
                        ItemStack remaining(stack.copy());
                        remaining.shrink(toAdd);
                        items.push_back(remaining);
                        return;
                    }
                }
            }
            // 无法合并，添加新堆
            items.push_back(stack);
        }
    };

    recursiveGenerate(consumer, context);
    return items;
}

void LootTable::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const {
    recursiveGenerate(consumer, context);
}

void LootTable::recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const {
    // 循环检测
    if (!context.pushLootTable(this)) {
        // 检测到循环引用，跳过
        return;
    }

    // 执行所有池
    for (auto& pool : m_pools) {
        pool->generate(consumer, context);
    }

    context.popLootTable(this);
}

Result<std::unique_ptr<LootTable>> LootTable::fromJson(const String& /*json*/) {
    // TODO: 实现 JSON 解析
    return Error(ErrorCode::Unsupported, "JSON parsing not yet implemented");
}

String LootTable::toJson() const {
    // TODO: 实现 JSON 序列化
    return "{}";
}

// ============================================================================
// LootTableBuilder
// ============================================================================

std::unique_ptr<LootTable> LootTableBuilder::build() const {
    auto table = std::make_unique<LootTable>();
    table->setId(m_id);
    table->setParameterSet(m_paramSet);

    for (const auto& pool : m_pools) {
        table->addPool(pool->clone());
    }

    return table;
}

// ============================================================================
// LootTableManager
// ============================================================================

void LootTableManager::registerTable(const String& id, std::unique_ptr<LootTable> table) {
    if (table) {
        table->setId(id);
        m_tables[id] = std::move(table);
    }
}

const LootTable* LootTableManager::getTable(const String& id) const {
    auto it = m_tables.find(id);
    if (it != m_tables.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool LootTableManager::hasTable(const String& id) const {
    return m_tables.find(id) != m_tables.end();
}

std::vector<String> LootTableManager::getAllTableIds() const {
    std::vector<String> ids;
    ids.reserve(m_tables.size());
    for (const auto& [id, table] : m_tables) {
        ids.push_back(id);
    }
    return ids;
}

const LootTable& LootTableManager::getEmptyTable() {
    return LootTable::EMPTY;
}

void LootTableManager::initializeDefaultTables() {
    // 创建猪的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:porkchop",
            RandomValueRange(1.0f, 3.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/pig", std::move(table));
    }

    // 创建牛的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:beef",
            RandomValueRange(1.0f, 3.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/cow", std::move(table));
    }

    // 创建羊的掉落表
    {
        auto table = std::make_unique<LootTable>();
        // 羊毛掉落
        auto woolPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        woolPool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:wool",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        ));
        table->addPool(std::move(woolPool));
        // 羊肉掉落
        auto meatPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        meatPool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:mutton",
            RandomValueRange(1.0f, 2.0f),
            1, 0
        ));
        table->addPool(std::move(meatPool));
        registerTable("minecraft:entities/sheep", std::move(table));
    }

    // 创建鸡的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:chicken",
            RandomValueRange(1.0f, 2.0f),
            1, 0
        ));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:feather",
            RandomValueRange(0.0f, 2.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/chicken", std::move(table));
    }
}

} // namespace loot
} // namespace mr
