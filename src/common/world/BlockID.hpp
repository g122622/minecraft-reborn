#pragma once

#include "../core/Types.hpp"
#include <string>
#include <cstring>

namespace mr {

// ============================================================================
// 方块ID枚举
// ============================================================================

enum class BlockId : u16 {
    Air = 0,
    Stone = 1,
    GrassBlock = 2,
    Dirt = 3,
    Cobblestone = 4,
    OakPlanks = 5,
    Water = 6,
    Lava = 7,
    Bedrock = 8,
    Sand = 9,
    Gravel = 10,
    GoldOre = 11,
    IronOre = 12,
    CoalOre = 13,
    DiamondOre = 14,
    DiamondBlock = 15,
    OakLog = 16,
    OakLeaves = 17,
    Snow = 18,
    Ice = 19,
    Netherrack = 20,
    Glowstone = 21,
    EndStone = 22,
    Obsidian = 23,
    // 可扩展更多方块
    MaxBlocks = 256
};

// ============================================================================
// 方块标志
// ============================================================================

enum class BlockFlags : u32 {
    None = 0,
    Solid = 1 << 0,
    Opaque = 1 << 1,
    Transparent = 1 << 2,
    Liquid = 1 << 3,
    Flammable = 1 << 4,
    LightEmitting = 1 << 5,
    Replaceable = 1 << 6
};

// 位运算操作
inline BlockFlags operator|(BlockFlags a, BlockFlags b) {
    return static_cast<BlockFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline BlockFlags operator&(BlockFlags a, BlockFlags b) {
    return static_cast<BlockFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool hasFlag(BlockFlags flags, BlockFlags flag) {
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// 方块信息
// ============================================================================

struct BlockInfo {
    BlockId id = BlockId::Air;
    const char* idName = "";      // 标识符名称
    const char* displayName = ""; // 显示名称
    BlockFlags flags = BlockFlags::None;
    f32 hardness = 0.0f;
    f32 blastResistance = 0.0f;
    u8 lightLevel = 0;            // 发光等级 0-15
    u8 lightOpacity = 15;         // 光照不透明度
    bool solid = true;
    bool transparent = false;
    bool liquid = false;
};

// ============================================================================
// 方块状态
// ============================================================================

class BlockState {
public:
    BlockState() : m_id(BlockId::Air), m_data(0) {}
    explicit BlockState(BlockId id, u16 data = 0)
        : m_id(id), m_data(data) {}

    [[nodiscard]] BlockId id() const noexcept { return m_id; }
    void setId(BlockId id) noexcept { m_id = id; }

    [[nodiscard]] u16 data() const noexcept { return m_data; }
    void setData(u16 data) noexcept { m_data = data; }

    [[nodiscard]] bool isAir() const noexcept { return m_id == BlockId::Air; }

    [[nodiscard]] bool isSolid() const;
    [[nodiscard]] bool isTransparent() const;
    [[nodiscard]] bool isLiquid() const;
    [[nodiscard]] bool isFlammable() const;

    [[nodiscard]] const char* getName() const;
    [[nodiscard]] f32 getHardness() const;
    [[nodiscard]] f32 getBlastResistance() const;

    [[nodiscard]] bool operator==(const BlockState& other) const noexcept {
        return m_id == other.m_id && m_data == other.m_data;
    }

    [[nodiscard]] bool operator!=(const BlockState& other) const noexcept {
        return !(*this == other);
    }

private:
    BlockId m_id;
    u16 m_data;
};

// ============================================================================
// 方块注册表
// ============================================================================

class BlockRegistry {
public:
    static constexpr size_t MAX_BLOCKS = static_cast<size_t>(BlockId::MaxBlocks);

    static BlockRegistry& instance();

    void registerBlock(const BlockInfo& info);
    [[nodiscard]] const BlockInfo* getBlock(BlockId id) const;
    [[nodiscard]] BlockId getBlockId(const char* name) const;

    void initialize();

private:
    BlockRegistry() = default;
    ~BlockRegistry() = default;

    void registerVanillaBlocks();

    BlockInfo m_blocks[MAX_BLOCKS];
    bool m_initialized = false;
};

// ============================================================================
// 便捷命名空间
// ============================================================================

namespace Blocks {
    constexpr BlockId Air = BlockId::Air;
    constexpr BlockId Stone = BlockId::Stone;
    constexpr BlockId GrassBlock = BlockId::GrassBlock;
    constexpr BlockId Dirt = BlockId::Dirt;
    constexpr BlockId Cobblestone = BlockId::Cobblestone;
    constexpr BlockId OakPlanks = BlockId::OakPlanks;
    constexpr BlockId Water = BlockId::Water;
    constexpr BlockId Lava = BlockId::Lava;
    constexpr BlockId Bedrock = BlockId::Bedrock;
    constexpr BlockId Sand = BlockId::Sand;
    constexpr BlockId Gravel = BlockId::Gravel;
    constexpr BlockId GoldOre = BlockId::GoldOre;
    constexpr BlockId IronOre = BlockId::IronOre;
    constexpr BlockId CoalOre = BlockId::CoalOre;
    constexpr BlockId DiamondOre = BlockId::DiamondOre;
    constexpr BlockId DiamondBlock = BlockId::DiamondBlock;
    constexpr BlockId OakLog = BlockId::OakLog;
    constexpr BlockId OakLeaves = BlockId::OakLeaves;
    constexpr BlockId Snow = BlockId::Snow;
    constexpr BlockId Ice = BlockId::Ice;
    constexpr BlockId Netherrack = BlockId::Netherrack;
    constexpr BlockId Glowstone = BlockId::Glowstone;
    constexpr BlockId EndStone = BlockId::EndStone;
    constexpr BlockId Obsidian = BlockId::Obsidian;
} // namespace Blocks

} // namespace mr
