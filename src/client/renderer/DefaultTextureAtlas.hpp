#pragma once

#include "../../common/core/Types.hpp"
#include "../../common/renderer/MeshTypes.hpp"
#include "VulkanTexture.hpp"
#include <vector>
#include <unordered_map>

namespace mr::client {

/**
 * @brief 方块纹理ID
 */
enum class BlockTextureId : u16 {
    // 基础方块
    Air = 0,
    Stone = 1,
    GrassTop = 2,
    GrassSide = 3,
    Dirt = 4,
    Cobblestone = 5,
    WoodSide = 6,
    WoodTop = 7,
    Leaves = 8,
    Sand = 9,
    Gravel = 10,
    GoldOre = 11,
    IronOre = 12,
    CoalOre = 13,
    DiamondOre = 14,
    Bedrock = 15,
    Water = 16,
    Lava = 17,

    // 计数
    Count = 32
};

/**
 * @brief 默认纹理图集生成器
 *
 * 生成简单的程序化方块纹理，用于测试和后备
 */
class DefaultTextureAtlas {
public:
    static constexpr u32 TILE_SIZE = 16;       // 每个方块纹理16x16像素
    static constexpr u32 ATLAS_SIZE = 256;     // 图集256x256像素 (16x16个方块)

    /**
     * @brief 生成默认纹理图集
     * @return RGBA8像素数据 (ATLAS_SIZE * ATLAS_SIZE * 4字节)
     */
    static std::vector<u8> generate();

    /**
     * @brief 获取方块纹理区域
     * @param textureId 纹理ID
     * @return 纹理区域UV坐标
     */
    static TextureRegion getRegion(BlockTextureId textureId);

    /**
     * @brief 获取图集尺寸
     */
    static constexpr u32 atlasSize() { return ATLAS_SIZE; }
    static constexpr u32 tileSize() { return TILE_SIZE; }
    static constexpr u32 tilesPerRow() { return ATLAS_SIZE / TILE_SIZE; }

private:
    // 生成特定方块的纹理
    static void generateStoneTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateGrassTopTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateGrassSideTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateDirtTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateCobblestoneTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateWoodSideTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateWoodTopTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateLeavesTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateSandTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateGravelTexture(u8* pixels, u32 tileX, u32 tileY);
    static void generateOreTexture(u8* pixels, u32 tileX, u32 tileY, u8 oreR, u8 oreG, u8 oreB);
    static void generateBedrockTexture(u8* pixels, u32 tileX, u32 tileY);

    // 噪声函数
    static u8 noise2D(u32 x, u32 y, u32 seed);
    static void setPixel(u8* pixels, u32 tileX, u32 tileY, u32 localX, u32 localY,
                         u8 r, u8 g, u8 b, u8 a = 255);
};

} // namespace mr::client
