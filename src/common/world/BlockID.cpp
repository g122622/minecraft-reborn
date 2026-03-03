#include "BlockID.hpp"

namespace mr {

// BlockState 方法实现
bool BlockState::isSolid() const {
    if (m_id == BlockId::Air) return false;
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->solid : true;
}

bool BlockState::isTransparent() const {
    if (m_id == BlockId::Air) return true;
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->transparent : false;
}

bool BlockState::isLiquid() const {
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->liquid : false;
}

bool BlockState::isFlammable() const {
    return hasFlag(BlockRegistry::instance().getBlock(m_id)->flags, BlockFlags::Flammable);
}

const char* BlockState::getName() const {
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->displayName : "unknown";
}

f32 BlockState::getHardness() const {
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->hardness : 1.0f;
}

f32 BlockState::getBlastResistance() const {
    auto* info = BlockRegistry::instance().getBlock(m_id);
    return info ? info->blastResistance : 0.0f;
}

// BlockRegistry 实现
BlockRegistry& BlockRegistry::instance() {
    static BlockRegistry registry;
    return registry;
}

void BlockRegistry::registerBlock(const BlockInfo& info) {
    if (static_cast<size_t>(info.id) < MAX_BLOCKS) {
        m_blocks[static_cast<size_t>(info.id)] = info;
    }
}

const BlockInfo* BlockRegistry::getBlock(BlockId id) const {
    size_t index = static_cast<size_t>(id);
    if (index < MAX_BLOCKS && m_blocks[index].id == id) {
        return &m_blocks[index];
    }
    return nullptr;
}

BlockId BlockRegistry::getBlockId(const char* name) const {
    for (size_t i = 0; i < MAX_BLOCKS; ++i) {
        if (m_blocks[i].idName && strcmp(m_blocks[i].idName, name) == 0) {
            return m_blocks[i].id;
        }
    }
    return BlockId::Air;
}

void BlockRegistry::initialize() {
    if (m_initialized) return;
    registerVanillaBlocks();
    m_initialized = true;
}

void BlockRegistry::registerVanillaBlocks() {
    // 基础方块注册
    registerBlock({BlockId::Air, "air", "Air",
                   BlockFlags::Transparent | BlockFlags::Replaceable,
                   0.0f, 0.0f, 0, 0, false, true, false});

    registerBlock({BlockId::Stone, "stone", "Stone",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   1.5f, 6.0f, 0, 15, true, false, false});

    registerBlock({BlockId::GrassBlock, "grass_block", "Grass Block",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   0.6f, 0.6f, 0, 15, true, false, false});

    registerBlock({BlockId::Dirt, "dirt", "Dirt",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   0.5f, 0.5f, 0, 15, true, false, false});

    registerBlock({BlockId::Cobblestone, "cobblestone", "Cobblestone",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   2.0f, 6.0f, 0, 15, true, false, false});

    registerBlock({BlockId::OakPlanks, "oak_planks", "Oak Planks",
                   BlockFlags::Solid | BlockFlags::Opaque | BlockFlags::Flammable,
                   2.0f, 3.0f, 0, 15, true, false, false});

    registerBlock({BlockId::Water, "water", "Water",
                   BlockFlags::Liquid | BlockFlags::Transparent,
                   100.0f, 100.0f, 0, 2, false, true, true});

    registerBlock({BlockId::Lava, "lava", "Lava",
                   BlockFlags::Liquid | BlockFlags::Transparent | BlockFlags::LightEmitting,
                   100.0f, 100.0f, 15, 2, false, true, true});

    registerBlock({BlockId::Bedrock, "bedrock", "Bedrock",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   -1.0f, 3600000.0f, 0, 15, true, false, false});

    registerBlock({BlockId::Sand, "sand", "Sand",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   0.5f, 0.5f, 0, 15, true, false, false});

    registerBlock({BlockId::Gravel, "gravel", "Gravel",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   0.6f, 0.6f, 0, 15, true, false, false});

    registerBlock({BlockId::GoldOre, "gold_ore", "Gold Ore",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   3.0f, 3.0f, 0, 15, true, false, false});

    registerBlock({BlockId::IronOre, "iron_ore", "Iron Ore",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   3.0f, 3.0f, 0, 15, true, false, false});

    registerBlock({BlockId::CoalOre, "coal_ore", "Coal Ore",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   3.0f, 3.0f, 0, 15, true, false, false});

    registerBlock({BlockId::DiamondOre, "diamond_ore", "Diamond Ore",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   3.0f, 3.0f, 0, 15, true, false, false});

    registerBlock({BlockId::DiamondBlock, "diamond_block", "Block of Diamond",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   5.0f, 6.0f, 0, 15, true, false, false});

    registerBlock({BlockId::OakLog, "oak_log", "Oak Log",
                   BlockFlags::Solid | BlockFlags::Opaque | BlockFlags::Flammable,
                   2.0f, 2.0f, 0, 15, true, false, false});

    registerBlock({BlockId::OakLeaves, "oak_leaves", "Oak Leaves",
                   BlockFlags::Solid | BlockFlags::Transparent | BlockFlags::Flammable,
                   0.2f, 0.2f, 0, 1, true, true, false});

    registerBlock({BlockId::Snow, "snow", "Snow",
                   BlockFlags::Solid | BlockFlags::Transparent,
                   0.1f, 0.1f, 0, 1, true, true, false});

    registerBlock({BlockId::Ice, "ice", "Ice",
                   BlockFlags::Solid | BlockFlags::Transparent,
                   0.5f, 0.5f, 0, 2, true, true, false});

    registerBlock({BlockId::Netherrack, "netherrack", "Netherrack",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   0.4f, 0.4f, 0, 15, true, false, false});

    registerBlock({BlockId::Glowstone, "glowstone", "Glowstone",
                   BlockFlags::Solid | BlockFlags::Transparent | BlockFlags::LightEmitting,
                   0.3f, 0.3f, 15, 1, true, true, false});

    registerBlock({BlockId::EndStone, "end_stone", "End Stone",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   3.0f, 9.0f, 0, 15, true, false, false});

    registerBlock({BlockId::Obsidian, "obsidian", "Obsidian",
                   BlockFlags::Solid | BlockFlags::Opaque,
                   50.0f, 1200.0f, 0, 15, true, false, false});
}

} // namespace mr
