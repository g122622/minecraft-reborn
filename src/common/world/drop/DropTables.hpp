#pragma once

#include "../../core/Types.hpp"
#include "../../item/ItemStack.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace mc {

// Forward declarations
class BlockState;
class Player;
class World;
class Item;

// ============================================================================
// 掉落上下文
// ============================================================================

/**
 * @brief 掉落上下文
 *
 * 包含影响掉落的上下文信息，如工具、幸运等级等。
 * 参考: net.minecraft.loot.LootContext
 */
class DropContext {
public:
    DropContext() = default;

    /**
     * @brief 设置工具
     */
    DropContext& withTool(const ItemStack* tool) {
        m_tool = tool;
        return *this;
    }

    /**
     * @brief 设置幸运等级
     */
    DropContext& withFortune(i32 fortune) {
        m_fortune = fortune;
        return *this;
    }

    /**
     * @brief 设置掠夺等级
     */
    DropContext& withLooting(i32 looting) {
        m_looting = looting;
        return *this;
    }

    /**
     * @brief 设置方块位置
     */
    DropContext& withPosition(i32 x, i32 y, i32 z) {
        m_x = x;
        m_y = y;
        m_z = z;
        return *this;
    }

    /**
     * @brief 设置是否使用精准采集
     */
    DropContext& withSilkTouch(bool silkTouch) {
        m_silkTouch = silkTouch;
        return *this;
    }

    /**
     * @brief 设置随机种子
     */
    DropContext& withSeed(u64 seed) {
        m_seed = seed;
        m_hasSeed = true;
        return *this;
    }

    // Getters
    [[nodiscard]] const ItemStack* tool() const { return m_tool; }
    [[nodiscard]] i32 fortune() const { return m_fortune; }
    [[nodiscard]] i32 looting() const { return m_looting; }
    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 z() const { return m_z; }
    [[nodiscard]] bool silkTouch() const { return m_silkTouch; }
    [[nodiscard]] u64 seed() const { return m_seed; }
    [[nodiscard]] bool hasSeed() const { return m_hasSeed; }

private:
    const ItemStack* m_tool = nullptr;
    i32 m_fortune = 0;
    i32 m_looting = 0;
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    bool m_silkTouch = false;
    u64 m_seed = 0;
    bool m_hasSeed = false;
};

// ============================================================================
// 掉落条目
// ============================================================================

/**
 * @brief 掉落条件类型
 */
enum class DropCondition : u8 {
    None,               // 无条件
    SilkTouch,          // 精准采集
    NoSilkTouch,        // 非精准采集
    RandomChance,       // 随机概率
    Fortune             // 时运影响
};

/**
 * @brief 单个掉落条目
 *
 * 定义一个可能的掉落结果。
 */
class DropEntry {
public:
    /**
     * @brief 构造物品掉落条目
     * @param item 物品
     * @param minCount 最小数量
     * @param maxCount 最大数量
     * @param chance 掉落概率 (0.0-1.0)
     */
    DropEntry(const Item& item, i32 minCount = 1, i32 maxCount = 1, f32 chance = 1.0f);

    /**
     * @brief 构造空掉落条目（用于概率性的无掉落）
     */
    static DropEntry empty(f32 chance);

    /**
     * @brief 检查条件是否满足
     */
    [[nodiscard]] bool checkCondition(const DropContext& context) const;

    /**
     * @brief 生成掉落物品
     * @param context 掉落上下文
     * @return 生成的物品堆
     */
    [[nodiscard]] ItemStack generate(const DropContext& context) const;

    // Getters
    [[nodiscard]] const Item* item() const { return m_item; }
    [[nodiscard]] i32 minCount() const { return m_minCount; }
    [[nodiscard]] i32 maxCount() const { return m_maxCount; }
    [[nodiscard]] f32 chance() const { return m_chance; }
    [[nodiscard]] DropCondition condition() const { return m_condition; }
    [[nodiscard]] bool isEmpty() const { return m_empty; }

    // Setters
    void setCondition(DropCondition condition) { m_condition = condition; }
    void setFortuneBonus(i32 bonus) { m_fortuneBonus = bonus; }

private:
    const Item* m_item = nullptr;
    i32 m_minCount = 1;
    i32 m_maxCount = 1;
    f32 m_chance = 1.0f;
    DropCondition m_condition = DropCondition::None;
    i32 m_fortuneBonus = 0;  // 时运额外增加的计数
    bool m_empty = false;
};

// ============================================================================
// 掉落表
// ============================================================================

/**
 * @brief 掉落表
 *
 * 定义一个方块或实体的掉落规则。
 * 参考: net.minecraft.loot.LootTable
 *
 * 用法示例:
 * @code
 * // 创建钻石矿石掉落表
 * DropTable diamondOreDrops;
 * diamondOreDrops.addEntry(DropEntry(*Items::DIAMOND, 1, 1, 1.0f)
 *     .setCondition(DropCondition::NoSilkTouch));
 * diamondOreDrops.addEntry(DropEntry(*Items::DIAMOND_ORE, 1, 1, 1.0f)
 *     .setCondition(DropCondition::SilkTouch));
 *
 * // 生成掉落
 * DropContext context = DropContext()
 *     .withTool(&pickaxe)
 *     .withFortune(3);
 * auto drops = diamondOreDrops.generateDrops(context);
 * @endcode
 */
class DropTable {
public:
    DropTable() = default;

    /**
     * @brief 添加掉落条目
     */
    void addEntry(const DropEntry& entry);

    /**
     * @brief 添加物品掉落
     * @param item 物品
     * @param minCount 最小数量
     * @param maxCount 最大数量
     * @return *this 用于链式调用
     */
    DropTable& addItem(const Item& item, i32 minCount = 1, i32 maxCount = 1);

    /**
     * @brief 添加带概率的物品掉落
     */
    DropTable& addItem(const Item& item, i32 minCount, i32 maxCount, f32 chance);

    /**
     * @brief 生成所有掉落物品
     * @param context 掉落上下文
     * @return 掉落物品列表
     */
    [[nodiscard]] std::vector<ItemStack> generateDrops(const DropContext& context) const;

    /**
     * @brief 生成单个掉落（随机选择一个条目）
     */
    [[nodiscard]] std::vector<ItemStack> generateSingleDrop(const DropContext& context) const;

    /**
     * @brief 获取所有条目
     */
    [[nodiscard]] const std::vector<DropEntry>& entries() const { return m_entries; }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool isEmpty() const { return m_entries.empty(); }

    // ========== 预定义掉落表 ==========

    /**
     * @brief 创建矿石掉落表（带时运）
     */
    static DropTable oreDrop(const Item& oreItem, const Item& rawItem,
                             i32 minExp = 0, i32 maxExp = 0);

    /**
     * @brief 创建方块掉落表（带精准采集）
     */
    static DropTable blockDrop(const Item& blockItem);

    /**
     * @brief 创建多重掉落表
     */
    static DropTable multipleDrop(const Item& item, i32 min, i32 max);

private:
    std::vector<DropEntry> m_entries;
};

// ============================================================================
// 掉落表注册表
// ============================================================================

/**
 * @brief 掉落表注册表
 *
 * 管理所有方块和实体的掉落表。
 */
class DropTableRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static DropTableRegistry& instance();

    /**
     * @brief 注册方块掉落表
     * @param blockId 方块ID
     * @param table 掉落表
     */
    void registerBlockDrop(BlockId blockId, const DropTable& table);

    /**
     * @brief 获取方块掉落表
     * @param blockId 方块ID
     * @return 掉落表指针，不存在返回nullptr
     */
    [[nodiscard]] const DropTable* getBlockDrop(BlockId blockId) const;

    /**
     * @brief 生成方块掉落
     * @param blockId 方块ID
     * @param context 掉落上下文
     * @return 掉落物品列表
     */
    [[nodiscard]] std::vector<ItemStack> generateBlockDrops(
        BlockId blockId, const DropContext& context) const;

    /**
     * @brief 初始化原版掉落表
     */
    void initializeVanillaDrops();

private:
    DropTableRegistry() = default;

    std::unordered_map<BlockId, DropTable> m_blockDrops;
    bool m_initialized = false;
};

} // namespace mc
