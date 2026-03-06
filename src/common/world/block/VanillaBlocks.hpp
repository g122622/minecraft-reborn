#pragma once

#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "blocks/AirBlock.hpp"
#include "blocks/SimpleBlock.hpp"
#include "blocks/RotatedPillarBlock.hpp"

namespace mr {

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

    // 基础方块
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

    // 矿石方块
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;

    // 原木和树叶
    static Block* OAK_LOG;
    static Block* OAK_LEAVES;

    // 其他方块
    static Block* SNOW;
    static Block* ICE;
    static Block* NETHERRACK;
    static Block* GLOWSTONE;
    static Block* END_STONE;
    static Block* OBSIDIAN;

private:
    static bool s_initialized;

    static void registerBaseBlocks();
    static void registerOreBlocks();
    static void registerLogBlocks();
};

} // namespace mr
