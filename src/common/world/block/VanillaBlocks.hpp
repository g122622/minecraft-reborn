#pragma once

#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "blocks/AirBlock.hpp"
#include "blocks/SimpleBlock.hpp"
#include "blocks/RotatedPillarBlock.hpp"

namespace mc {

/**
 * @brief 原版方块静态引用
 *
 * 提供所有原版方块的静态指针，便于快速访问。
 * 在游戏初始化时调用 VanillaBlocks::initialize() 进行注册。
 *
 * 参考: net.minecraft.block.Blocks
 */
class VanillaBlocks {
public:
    /**
     * @brief 初始化所有原版方块
     *
     * 必须在使用任何方块前调用。
     */
    static void initialize();

    // ========================================================================
    // 基础方块
    // ========================================================================
    static Block* AIR;
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* OAK_PLANKS;
    static Block* WATER;
    static Block* LAVA;
    static Block* BEDROCK;
    static Block* SAND;
    static Block* GRAVEL;

    // ========================================================================
    // 石头变种
    // ========================================================================
    static Block* GRANITE;
    static Block* POLISHED_GRANITE;
    static Block* DIORITE;
    static Block* POLISHED_DIORITE;
    static Block* ANDESITE;
    static Block* POLISHED_ANDESITE;

    // ========================================================================
    // 泥土变种
    // ========================================================================
    static Block* COARSE_DIRT;
    static Block* PODZOL;

    // ========================================================================
    // 砂岩系列
    // ========================================================================
    static Block* SANDSTONE;
    static Block* CHISELED_SANDSTONE;
    static Block* CUT_SANDSTONE;
    static Block* RED_SANDSTONE;

    // ========================================================================
    // 矿石方块
    // ========================================================================
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;
    static Block* EMERALD_ORE;
    static Block* LAPIS_ORE;
    static Block* REDSTONE_ORE;

    // ========================================================================
    // 矿物方块
    // ========================================================================
    static Block* GOLD_BLOCK;
    static Block* IRON_BLOCK;
    static Block* LAPIS_BLOCK;
    static Block* EMERALD_BLOCK;
    static Block* REDSTONE_BLOCK;

    // ========================================================================
    // 建筑方块
    // ========================================================================
    static Block* BRICKS;
    static Block* MOSSY_COBBLESTONE;
    static Block* BOOKSHELF;
    static Block* TNT;
    static Block* SPONGE;
    static Block* WET_SPONGE;

    // ========================================================================
    // 功能方块
    // ========================================================================
    static Block* CRAFTING_TABLE;

    // ========================================================================
    // 羊毛 (16色)
    // ========================================================================
    static Block* WHITE_WOOL;
    static Block* ORANGE_WOOL;
    static Block* MAGENTA_WOOL;
    static Block* LIGHT_BLUE_WOOL;
    static Block* YELLOW_WOOL;
    static Block* LIME_WOOL;
    static Block* PINK_WOOL;
    static Block* GRAY_WOOL;
    static Block* LIGHT_GRAY_WOOL;
    static Block* CYAN_WOOL;
    static Block* PURPLE_WOOL;
    static Block* BLUE_WOOL;
    static Block* BROWN_WOOL;
    static Block* GREEN_WOOL;
    static Block* RED_WOOL;
    static Block* BLACK_WOOL;

    // ========================================================================
    // 木板变种
    // ========================================================================
    static Block* SPRUCE_PLANKS;
    static Block* BIRCH_PLANKS;
    static Block* JUNGLE_PLANKS;
    static Block* ACACIA_PLANKS;
    static Block* DARK_OAK_PLANKS;

    // ========================================================================
    // 原木和树叶
    // ========================================================================
    static Block* OAK_LOG;
    static Block* OAK_LEAVES;
    static Block* SPRUCE_LOG;
    static Block* BIRCH_LOG;
    static Block* JUNGLE_LOG;
    static Block* ACACIA_LOG;
    static Block* DARK_OAK_LOG;
    static Block* SPRUCE_LEAVES;
    static Block* BIRCH_LEAVES;
    static Block* JUNGLE_LEAVES;
    static Block* ACACIA_LEAVES;
    static Block* DARK_OAK_LEAVES;

    // ========================================================================
    // 植被方块
    // ========================================================================
    static Block* SHORT_GRASS;
    static Block* TALL_GRASS;
    static Block* FERN;
    static Block* DANDELION;
    static Block* POPPY;
    static Block* BLUE_ORCHID;
    static Block* ALLIUM;
    static Block* AZURE_BLUET;
    static Block* RED_TULIP;
    static Block* ORANGE_TULIP;
    static Block* WHITE_TULIP;
    static Block* PINK_TULIP;
    static Block* OXEYE_DAISY;
    static Block* BROWN_MUSHROOM;
    static Block* RED_MUSHROOM;

    // ========================================================================
    // 树苗
    // ========================================================================
    static Block* OAK_SAPLING;
    static Block* SPRUCE_SAPLING;
    static Block* BIRCH_SAPLING;
    static Block* JUNGLE_SAPLING;
    static Block* ACACIA_SAPLING;
    static Block* DARK_OAK_SAPLING;

    // ========================================================================
    // 其他方块
    // ========================================================================
    static Block* SNOW;
    static Block* ICE;
    static Block* NETHERRACK;
    static Block* GLOWSTONE;
    static Block* END_STONE;
    static Block* OBSIDIAN;

    // ========================================================================
    // 下界方块
    // ========================================================================
    static Block* SOUL_SAND;
    static Block* SOUL_SOIL;
    static Block* BASALT;
    static Block* POLISHED_BASALT;
    static Block* BLACKSTONE;
    static Block* POLISHED_BLACKSTONE;
    static Block* CRYING_OBSIDIAN;

private:
    static bool s_initialized;

    static void registerBaseBlocks();
    static void registerOreBlocks();
    static void registerLogBlocks();
    static void registerStoneVariants();
    static void registerDirtVariants();
    static void registerSandstones();
    static void registerMineralBlocks();
    static void registerBuildingBlocks();
    static void registerFunctionalBlocks();
    static void registerWoolBlocks();
    static void registerPlanksVariants();
    static void registerNetherBlocks();
    static void registerTreeVariants();
    static void registerVegetationBlocks();
};

} // namespace mc
