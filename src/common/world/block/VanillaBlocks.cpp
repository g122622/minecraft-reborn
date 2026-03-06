#include "VanillaBlocks.hpp"

namespace mr {

// ============================================================================
// 静态成员初始化
// ============================================================================
bool VanillaBlocks::s_initialized = false;

// 基础方块
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

// 石头变种
Block* VanillaBlocks::GRANITE = nullptr;
Block* VanillaBlocks::POLISHED_GRANITE = nullptr;
Block* VanillaBlocks::DIORITE = nullptr;
Block* VanillaBlocks::POLISHED_DIORITE = nullptr;
Block* VanillaBlocks::ANDESITE = nullptr;
Block* VanillaBlocks::POLISHED_ANDESITE = nullptr;

// 泥土变种
Block* VanillaBlocks::COARSE_DIRT = nullptr;
Block* VanillaBlocks::PODZOL = nullptr;

// 砂岩系列
Block* VanillaBlocks::SANDSTONE = nullptr;
Block* VanillaBlocks::CHISELED_SANDSTONE = nullptr;
Block* VanillaBlocks::CUT_SANDSTONE = nullptr;
Block* VanillaBlocks::RED_SANDSTONE = nullptr;

// 矿石方块
Block* VanillaBlocks::GOLD_ORE = nullptr;
Block* VanillaBlocks::IRON_ORE = nullptr;
Block* VanillaBlocks::COAL_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_ORE = nullptr;
Block* VanillaBlocks::DIAMOND_BLOCK = nullptr;
Block* VanillaBlocks::EMERALD_ORE = nullptr;
Block* VanillaBlocks::LAPIS_ORE = nullptr;
Block* VanillaBlocks::REDSTONE_ORE = nullptr;

// 矿物方块
Block* VanillaBlocks::GOLD_BLOCK = nullptr;
Block* VanillaBlocks::IRON_BLOCK = nullptr;
Block* VanillaBlocks::LAPIS_BLOCK = nullptr;
Block* VanillaBlocks::EMERALD_BLOCK = nullptr;
Block* VanillaBlocks::REDSTONE_BLOCK = nullptr;

// 建筑方块
Block* VanillaBlocks::BRICKS = nullptr;
Block* VanillaBlocks::MOSSY_COBBLESTONE = nullptr;
Block* VanillaBlocks::BOOKSHELF = nullptr;
Block* VanillaBlocks::TNT = nullptr;
Block* VanillaBlocks::SPONGE = nullptr;
Block* VanillaBlocks::WET_SPONGE = nullptr;

// 羊毛
Block* VanillaBlocks::WHITE_WOOL = nullptr;
Block* VanillaBlocks::ORANGE_WOOL = nullptr;
Block* VanillaBlocks::MAGENTA_WOOL = nullptr;
Block* VanillaBlocks::LIGHT_BLUE_WOOL = nullptr;
Block* VanillaBlocks::YELLOW_WOOL = nullptr;
Block* VanillaBlocks::LIME_WOOL = nullptr;
Block* VanillaBlocks::PINK_WOOL = nullptr;
Block* VanillaBlocks::GRAY_WOOL = nullptr;
Block* VanillaBlocks::LIGHT_GRAY_WOOL = nullptr;
Block* VanillaBlocks::CYAN_WOOL = nullptr;
Block* VanillaBlocks::PURPLE_WOOL = nullptr;
Block* VanillaBlocks::BLUE_WOOL = nullptr;
Block* VanillaBlocks::BROWN_WOOL = nullptr;
Block* VanillaBlocks::GREEN_WOOL = nullptr;
Block* VanillaBlocks::RED_WOOL = nullptr;
Block* VanillaBlocks::BLACK_WOOL = nullptr;

// 木板变种
Block* VanillaBlocks::SPRUCE_PLANKS = nullptr;
Block* VanillaBlocks::BIRCH_PLANKS = nullptr;
Block* VanillaBlocks::JUNGLE_PLANKS = nullptr;
Block* VanillaBlocks::ACACIA_PLANKS = nullptr;
Block* VanillaBlocks::DARK_OAK_PLANKS = nullptr;

// 原木和树叶
Block* VanillaBlocks::OAK_LOG = nullptr;
Block* VanillaBlocks::OAK_LEAVES = nullptr;

// 其他方块
Block* VanillaBlocks::SNOW = nullptr;
Block* VanillaBlocks::ICE = nullptr;
Block* VanillaBlocks::NETHERRACK = nullptr;
Block* VanillaBlocks::GLOWSTONE = nullptr;
Block* VanillaBlocks::END_STONE = nullptr;
Block* VanillaBlocks::OBSIDIAN = nullptr;

// 下界方块
Block* VanillaBlocks::SOUL_SAND = nullptr;
Block* VanillaBlocks::SOUL_SOIL = nullptr;
Block* VanillaBlocks::BASALT = nullptr;
Block* VanillaBlocks::POLISHED_BASALT = nullptr;
Block* VanillaBlocks::BLACKSTONE = nullptr;
Block* VanillaBlocks::POLISHED_BLACKSTONE = nullptr;
Block* VanillaBlocks::CRYING_OBSIDIAN = nullptr;

// ============================================================================
// 初始化
// ============================================================================
void VanillaBlocks::initialize() {
    if (s_initialized) {
        return;
    }

    registerBaseBlocks();
    registerOreBlocks();
    registerLogBlocks();
    registerStoneVariants();
    registerDirtVariants();
    registerSandstones();
    registerMineralBlocks();
    registerBuildingBlocks();
    registerWoolBlocks();
    registerPlanksVariants();
    registerNetherBlocks();

    s_initialized = true;
}

// ============================================================================
// 基础方块注册
// ============================================================================
void VanillaBlocks::registerBaseBlocks() {
    auto& registry = BlockRegistry::instance();

    // 空气 - ID 0
    AIR = &registry.registerBlock<AirBlock>(
        ResourceLocation("minecraft:air"),
        BlockProperties(Material::AIR)
    );

    // 石头 - ID 1
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:stone"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 草方块 - ID 2
    // 参考: new GrassBlock(Properties.create(Material.ORGANIC).tickRandomly().hardnessAndResistance(0.6F))
    GRASS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:grass_block"),
        BlockProperties(Material::EARTH).hardness(0.6f)
    );

    // 泥土 - ID 3
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 圆石 - ID 4
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cobblestone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 橡木木板 - ID 5
    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(2.0F, 3.0F))
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
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(-1.0F, 3600000.0F).noDrops())
    BEDROCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bedrock"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f)
    );

    // 沙子 - ID 9
    // 参考: new SandBlock(14406560, Properties.create(Material.SAND).hardnessAndResistance(0.5F))
    SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sand"),
        BlockProperties(Material::SAND).hardness(0.5f)
    );

    // 砾石 - ID 10
    // 参考: new GravelBlock(Properties.create(Material.SAND).hardnessAndResistance(0.6F))
    GRAVEL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gravel"),
        BlockProperties(Material::SAND).hardness(0.6f)
    );

    // 雪 - ID 18
    // 参考: new SnowBlock(Properties.create(Material.SNOW).tickRandomly().hardnessAndResistance(0.1F))
    SNOW = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:snow"),
        BlockProperties(Material::SNOW).hardness(0.2f)
    );

    // 冰 - ID 19
    // 参考: new IceBlock(Properties.create(Material.ICE).slipperiness(0.98F).hardnessAndResistance(0.5F))
    ICE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:ice"),
        BlockProperties(Material::ICE).hardness(0.5f).notSolid()
    );

    // 下界岩 - ID 20
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(0.4F))
    NETHERRACK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:netherrack"),
        BlockProperties(Material::ROCK).hardness(0.4f)
    );

    // 荧石 - ID 21
    // 参考: new Block(Properties.create(Material.GLASS).hardnessAndResistance(0.3F).setLightLevel(15))
    GLOWSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:glowstone"),
        BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15)
    );

    // 末地石 - ID 22
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(3.0F, 9.0F))
    END_STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_stone"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f)
    );

    // 黑曜石 - ID 23
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(50.0F, 1200.0F))
    OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f)
    );
}

// ============================================================================
// 矿石方块注册
// ============================================================================
void VanillaBlocks::registerOreBlocks() {
    auto& registry = BlockRegistry::instance();

    // 金矿石 - ID 11
    // 参考: new OreBlock(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(3.0F, 3.0F))
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
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(5.0F, 6.0F))
    DIAMOND_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 绿宝石矿石
    EMERALD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 青金石矿石
    LAPIS_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );

    // 红石矿石
    REDSTONE_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f)
    );
}

// ============================================================================
// 原木注册
// ============================================================================
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

// ============================================================================
// 石头变种注册
// ============================================================================
void VanillaBlocks::registerStoneVariants() {
    auto& registry = BlockRegistry::instance();

    // 花岗岩
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(1.5F, 6.0F))
    GRANITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:granite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制花岗岩
    POLISHED_GRANITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_granite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 闪长岩
    DIORITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diorite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制闪长岩
    POLISHED_DIORITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_diorite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 安山岩
    ANDESITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:andesite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制安山岩
    POLISHED_ANDESITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_andesite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );
}

// ============================================================================
// 泥土变种注册
// ============================================================================
void VanillaBlocks::registerDirtVariants() {
    auto& registry = BlockRegistry::instance();

    // 粗泥土
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    COARSE_DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coarse_dirt"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 灰化土
    // 参考: new SnowyDirtBlock(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    PODZOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:podzol"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );
}

// ============================================================================
// 砂岩系列注册
// ============================================================================
void VanillaBlocks::registerSandstones() {
    auto& registry = BlockRegistry::instance();

    // 砂岩
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(0.8F))
    SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 錾制砂岩
    CHISELED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:chiseled_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 切制砂岩
    CUT_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cut_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );

    // 红砂岩
    RED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_sandstone"),
        BlockProperties(Material::ROCK).hardness(0.8f)
    );
}

// ============================================================================
// 矿物方块注册
// ============================================================================
void VanillaBlocks::registerMineralBlocks() {
    auto& registry = BlockRegistry::instance();

    // 金块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(3.0F, 6.0F))
    GOLD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_block"),
        BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f)
    );

    // 铁块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(5.0F, 6.0F))
    IRON_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 青金石块
    // 参考: new Block(Properties.create(Material.IRON).setRequiresTool().hardnessAndResistance(3.0F, 3.0F))
    LAPIS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_block"),
        BlockProperties(Material::IRON).hardness(3.0f).resistance(3.0f)
    );

    // 绿宝石块
    EMERALD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );

    // 红石块
    REDSTONE_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_block"),
        BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f)
    );
}

// ============================================================================
// 建筑方块注册
// ============================================================================
void VanillaBlocks::registerBuildingBlocks() {
    auto& registry = BlockRegistry::instance();

    // 砖块
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    BRICKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bricks"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 苔石圆石
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(2.0F, 6.0F))
    MOSSY_COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:mossy_cobblestone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 书架
    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(1.5F))
    BOOKSHELF = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:bookshelf"),
        BlockProperties(Material::WOOD).hardness(1.5f).flammable()
    );

    // TNT
    // 参考: new TNTBlock(Properties.create(Material.TNT).zeroHardnessAndResistance())
    TNT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:tnt"),
        BlockProperties(Material::TNT).hardness(0.0f)
    );

    // 海绵
    // 参考: new SpongeBlock(Properties.create(Material.SPONGE).hardnessAndResistance(0.6F))
    SPONGE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sponge"),
        BlockProperties(Material::SPONGE).hardness(0.6f)
    );

    // 湿海绵
    WET_SPONGE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:wet_sponge"),
        BlockProperties(Material::SPONGE).hardness(0.6f)
    );
}

// ============================================================================
// 羊毛注册 (16色)
// ============================================================================
void VanillaBlocks::registerWoolBlocks() {
    auto& registry = BlockRegistry::instance();

    // 参考: new Block(Properties.create(Material.WOOL).hardnessAndResistance(0.8F))
    // 所有羊毛使用相同的属性
    BlockProperties woolProps = BlockProperties(Material::WOOL).hardness(0.8f);

    WHITE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:white_wool"), woolProps);
    ORANGE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:orange_wool"), woolProps);
    MAGENTA_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:magenta_wool"), woolProps);
    LIGHT_BLUE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_blue_wool"), woolProps);
    YELLOW_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:yellow_wool"), woolProps);
    LIME_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lime_wool"), woolProps);
    PINK_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:pink_wool"), woolProps);
    GRAY_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gray_wool"), woolProps);
    LIGHT_GRAY_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:light_gray_wool"), woolProps);
    CYAN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cyan_wool"), woolProps);
    PURPLE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:purple_wool"), woolProps);
    BLUE_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blue_wool"), woolProps);
    BROWN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:brown_wool"), woolProps);
    GREEN_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:green_wool"), woolProps);
    RED_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_wool"), woolProps);
    BLACK_WOOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:black_wool"), woolProps);
}

// ============================================================================
// 木板变种注册
// ============================================================================
void VanillaBlocks::registerPlanksVariants() {
    auto& registry = BlockRegistry::instance();

    // 参考: new Block(Properties.create(Material.WOOD).hardnessAndResistance(2.0F, 3.0F))
    BlockProperties planksProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable();

    SPRUCE_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:spruce_planks"), planksProps);
    BIRCH_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:birch_planks"), planksProps);
    JUNGLE_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:jungle_planks"), planksProps);
    ACACIA_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:acacia_planks"), planksProps);
    DARK_OAK_PLANKS = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dark_oak_planks"), planksProps);
}

// ============================================================================
// 下界方块注册
// ============================================================================
void VanillaBlocks::registerNetherBlocks() {
    auto& registry = BlockRegistry::instance();

    // 灵魂沙
    // 参考: new Block(Properties.create(Material.SAND).hardnessAndResistance(0.5F))
    SOUL_SAND = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_sand"),
        BlockProperties(Material::SAND).hardness(0.5f)
    );

    // 灵魂土
    // 参考: new Block(Properties.create(Material.EARTH).hardnessAndResistance(0.5F))
    SOUL_SOIL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:soul_soil"),
        BlockProperties(Material::EARTH).hardness(0.5f)
    );

    // 玄武岩
    // 参考: new RotatedPillarBlock(Properties.create(Material.ROCK).hardnessAndResistance(1.25F, 4.2F))
    BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:basalt"),
        BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f)
    );

    // 磨制玄武岩
    POLISHED_BASALT = &registry.registerBlock<RotatedPillarBlock>(
        ResourceLocation("minecraft:polished_basalt"),
        BlockProperties(Material::ROCK).hardness(1.25f).resistance(4.2f)
    );

    // 黑石
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(1.5F, 6.0F))
    BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:blackstone"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f)
    );

    // 磨制黑石
    // 参考: new Block(Properties.create(Material.ROCK).hardnessAndResistance(2.0F, 6.0F))
    POLISHED_BLACKSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:polished_blackstone"),
        BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f)
    );

    // 哭泣的黑曜石
    // 参考: new Block(Properties.create(Material.ROCK).setRequiresTool().hardnessAndResistance(50.0F, 1200.0F).setLightLevel(10))
    CRYING_OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:crying_obsidian"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f).lightLevel(10)
    );
}

} // namespace mr
