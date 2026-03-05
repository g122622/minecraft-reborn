#include "VanillaBlocks.hpp"

namespace mr {

// 静态成员初始化
bool VanillaBlocks::s_initialized = false;

Block* VanillaBlocks::AIR = nullptr;
Block* VanillaBlocks::STONE = nullptr;
Block* VanillaBlocks::GRASS_BLOCK = nullptr;
Block* VanillaBlocks::DIRT = nullptr;
Block* VanillaBlocks::COBBLESTONE = nullptr;
Block* VanillaBlocks::OAK_PLANKS = nullptr;
Block* VanillaBlocks::WATER = nullptr;
Block* VanillaBlocks::LAVA = nullptr;
Block* VanillaBlocks::BEDROCK = nullptr;
Block* VanillaBlocks::SAND = nullptr;
Block* VanillaBlocks::GRAVEL = nullptr;
Block* VanillaBlocks::GOLD_ORE = nullptr;
Block* VanillaBlocks::IRON_ORE = nullptr;
Block* VanillaBlocks::COAL_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_BLOCK = nullptr;
Block* VanillaBlocks::OAK_LOG = nullptr;
Block* VanillaBlocks::OAK_LEAVES = nullptr;
Block* VanillaBlocks::SNOW = nullptr;
Block* VanillaBlocks::ICE = nullptr;
Block* VanillaBlocks::NETHERRACK = nullptr;
Block* VanillaBlocks::GLOWSTONE = nullptr;
Block* VanillaBlocks::END_STONE = nullptr;
Block* VanillaBlocks::OBSIDIAN = nullptr;

void VanillaBlocks::initialize() {
    if (s_initialized) {
        return;
    }

    registerBaseBlocks();
    registerOreBlocks();
    registerLogBlocks();

    s_initialized = true;
}

void VanillaBlocks::registerBaseBlocks() {
    auto& registry = BlockRegistry::instance();

    // 空气 - ID 0
    AIR = &registry.registerBlock<AirBlock>(
        ResourceLocation("minecraft:air"),
        BlockProperties(Material::AIR)
    );

    // 石头 - ID 1
    STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:stone"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 草方块 - ID 2
    GRASS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:grass_block"),
        BlockProperties(Material::EARTH).hardness(0.6f)
    );

    // 泥土 - ID 3
    DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 圆石 - ID 4
    COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cobblestone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 橡木木板 - ID 5
    OAK_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oak_planks"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable()
    );

    // 水 - ID 6
    WATER = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:water"),
        BlockProperties(Material::WATER).noCollision().notSolid()
    );

    // 岩浆 - ID 7
    LAVA = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lava"),
        BlockProperties(Material::LAVA).noCollision().notSolid().lightLevel(15)
    );

    // 基岩 - ID 8
    BEDROCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bedrock"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f)
    );

    // 沙子 - ID 9
    SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sand"),
        BlockProperties(Material::SAND).hardness(0.5f)
    );

    // 砾石 - ID 10
    GRAVEL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gravel"),
        BlockProperties(Material::SAND).hardness(0.6f)
    );

    // 雪 - ID 18
    SNOW = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:snow"),
        BlockProperties(Material::SNOW).hardness(0.2f)
    );

    // 冰 - ID 19
    ICE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:ice"),
        BlockProperties(Material::ICE).hardness(0.5f).notSolid()
    );

    // 下界岩 - ID 20
    NETHERRACK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:netherrack"),
        BlockProperties(Material::ROCK).hardness(0.4f)
    );

    // 荧石 - ID 21
    GLOWSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:glowstone"),
        BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15)
    );

    // 末地石 - ID 22
    END_STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_stone"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f)
    );

    // 黑曜石 - ID 23
    OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f)
    );
}

void VanillaBlocks::registerOreBlocks() {
    auto& registry = BlockRegistry::instance();

    // 金矿石 - ID 11
    GOLD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 铁矿石 - ID 12
    IRON_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 煤矿石 - ID 13
    COAL_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coal_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 钻石矿石 - ID 14
    DIAMOND_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 钻石块 - ID 15
    DIAMOND_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );
}

void VanillaBlocks::registerLogBlocks() {
    auto& registry = BlockRegistry::instance();

    // 橡木原木 - ID 16 (有3个状态，对应3个轴)
    OAK_LOG = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:oak_log"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable()
    );

    // 橡木树叶 - ID 17
    OAK_LEAVES = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:oak_leaves"),
        BlockProperties(Material::LEAVES).hardness(0.2f).flammable().notSolid()
    );
}

} // namespace mr
