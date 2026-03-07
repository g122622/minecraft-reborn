#pragma once

#include "../../common/core/Types.hpp"
#include <vector>

namespace mr::client {

/**
 * @brief 默认纹理图集生成器
 *
 * 生成缺失纹理（紫黑方块），用于没有资源包时的 fallback。
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
     * @brief 获取图集尺寸
     */
    static constexpr u32 atlasSize() { return ATLAS_SIZE; }
    static constexpr u32 tileSize() { return TILE_SIZE; }

private:
    // 生成缺失纹理（紫黑方块）
    static void generateMissingTexture(u8* pixels, u32 tileX, u32 tileY);
};

} // namespace mr::client
