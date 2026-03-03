#pragma once

#include "../core/Types.hpp"
#include <string>
#include <unordered_map>

namespace mr {

/**
 * @brief 方块ID定义
 *
 * 使用数字ID标识不同类型的方块
 */
namespace Blocks {

// 空气
constexpr BlockId AIR = 0;
constexpr BlockId STONE = 1;
constexpr BlockId GRANITE = 2;
constexpr BlockId POLISHED_GRANITE = 3;
constexpr BlockId DIORITE = 4;
constexpr BlockId POLISHED_DIORITE = 5;
constexpr BlockId ANDESITE = 6;
constexpr BlockId POLISHED_ANDESITE = 7;
constexpr BlockId DEEPSLATE = 8;
constexpr BlockId COBBLED_DEEPSLATE = 9;
constexpr BlockId POLISHED_DEEPSLATE = 10;

// 泥土类
constexpr BlockId GRASS_BLOCK = 11;
constexpr BlockId DIRT = 12;
constexpr BlockId COARSE_DIRT = 13;
constexpr BlockId PODZOL = 14;
constexpr BlockId ROOTED_DIRT = 15;
constexpr BlockId MUD = 16;

// 沙子类
constexpr BlockId SAND = 17;
constexpr BlockId RED_SAND = 18;
constexpr BlockId GRAVEL = 19;

// 木材类
constexpr BlockId OAK_LOG = 20;
constexpr BlockId SPRUCE_LOG = 21;
constexpr BlockId BIRCH_LOG = 22;
constexpr BlockId JUNGLE_LOG = 23;
constexpr BlockId ACACIA_LOG = 24;
constexpr BlockId DARK_OAK_LOG = 25;
constexpr BlockId MANGROVE_LOG = 26;
constexpr BlockId CHERRY_LOG = 27;

// 叶子类
constexpr BlockId OAK_LEAVES = 40;
constexpr BlockId SPRUCE_LEAVES = 41;
constexpr BlockId BIRCH_LEAVES = 42;
constexpr BlockId JUNGLE_LEAVES = 43;
constexpr BlockId ACACIA_LEAVES = 44;
constexpr BlockId DARK_OAK_LEAVES = 45;

// 矿石类
constexpr BlockId COAL_ORE = 60;
constexpr BlockId IRON_ORE = 61;
constexpr BlockId COPPER_ORE = 62;
constexpr BlockId GOLD_ORE = 63;
constexpr BlockId DIAMOND_ORE = 64;
constexpr BlockId EMERALD_ORE = 65;
constexpr BlockId REDSTONE_ORE = 66;
constexpr BlockId LAPIS_ORE = 67;

// 深层矿石
constexpr BlockId DEEPSLATE_COAL_ORE = 70;
constexpr BlockId DEEPSLATE_IRON_ORE = 71;
constexpr BlockId DEEPSLATE_COPPER_ORE = 72;
constexpr BlockId DEEPSLATE_GOLD_ORE = 73;
constexpr BlockId DEEPSLATE_DIAMOND_ORE = 74;
constexpr BlockId DEEPSLATE_EMERALD_ORE = 75;

// 水和岩浆
constexpr BlockId WATER = 100;
constexpr BlockId LAVA = 101;

// 建筑方块
constexpr BlockId GLASS = 120;
constexpr BlockId WHITE_WOOL = 121;
constexpr BlockId STONE_BRICKS = 122;
constexpr BlockId COBBLESTONE = 123;

// 最大方块ID
constexpr BlockId MAX_BLOCK_ID = 1024;

} // namespace Blocks

/**
 * @brief 方块状态
 *
 * 存储方块的状态数据（如朝向、含水等）
 */
class BlockState {
public:
    BlockState() = default;
    explicit BlockState(BlockId id)
        : m_id(id)
    {
    }

    [[nodiscard]] BlockId id() const noexcept { return m_id; }
    void setId(BlockId id) noexcept { m_id = id; }

    // 方块状态数据（用于存储额外属性）
    [[nodiscard]] u16 data() const noexcept { return m_data; }
    void setData(u16 data) noexcept { m_data = data; }

    // 常用状态检查
    [[nodiscard]] bool isAir() const noexcept { return m_id == Blocks::AIR; }
    [[nodiscard]] bool isLiquid() const noexcept
    {
        return m_id == Blocks::WATER || m_id == Blocks::LAVA;
    }
    [[nodiscard]] bool isSolid() const noexcept;

    // 比较
    [[nodiscard]] bool operator==(const BlockState& other) const noexcept
    {
        return m_id == other.m_id && m_data == other.m_data;
    }

    [[nodiscard]] bool operator!=(const BlockState& other) const noexcept
    {
        return !(*this == other);
    }

private:
    BlockId m_id = Blocks::AIR;
    u16 m_data = 0;
};

/**
 * @brief 方块属性
 *
 * 定义方块的静态属性
 */
struct BlockProperties {
    String name;
    bool solid = true;
    bool opaque = true;
    bool liquid = false;
    f32 hardness = 1.0f;
    f32 resistance = 0.0f;
    f32 lightLevel = 0.0f;
    bool requiresTool = false;
    BlockShape shape = BlockShape::Full;

    static BlockProperties get(BlockId id);
};

/**
 * @brief 方块注册表
 *
 * 存储所有方块的属性信息
 */
class BlockRegistry {
public:
    static BlockRegistry& instance();

    void registerBlock(BlockId id, const BlockProperties& properties);
    [[nodiscard]] const BlockProperties& getProperties(BlockId id) const;
    [[nodiscard]] bool hasBlock(BlockId id) const;

private:
    BlockRegistry();
    ~BlockRegistry() = default;

    std::unordered_map<BlockId, BlockProperties> m_blocks;
};

} // namespace mr
