#include "DefaultTextureAtlas.hpp"

namespace mr::client {

std::vector<u8> DefaultTextureAtlas::generate() {
    std::vector<u8> pixels(ATLAS_SIZE * ATLAS_SIZE * 4, 0);

    // 第一个位置 (0,0) 是缺失纹理 - 紫黑方块
    generateMissingTexture(pixels.data(), 0, 0);

    // 其余位置填充相同的缺失纹理（可选，作为后备）
    // 这样即使 UV 坐标有问题也能显示紫黑方块而不是黑屏
    for (u32 y = 0; y < ATLAS_SIZE / TILE_SIZE; ++y) {
        for (u32 x = 0; x < ATLAS_SIZE / TILE_SIZE; ++x) {
            if (x != 0 || y != 0) {
                generateMissingTexture(pixels.data(), x, y);
            }
        }
    }

    return pixels;
}

void DefaultTextureAtlas::generateMissingTexture(u8* pixels, u32 tileX, u32 tileY) {
    // 生成紫黑方块纹理（缺失纹理）
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            // 4x4 方块模式：紫黑交替
            u32 blockX = x / 4;
            u32 blockY = y / 4;
            bool isPurple = ((blockX + blockY) % 2) == 0;

            // 计算像素在图集中的位置
            u32 globalX = tileX * TILE_SIZE + x;
            u32 globalY = tileY * TILE_SIZE + y;
            u32 index = (globalY * ATLAS_SIZE + globalX) * 4;

            if (isPurple) {
                // 紫色 (F800F8) - MC 标准缺失纹理颜色
                pixels[index + 0] = 248; // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 248; // B
            } else {
                // 黑色
                pixels[index + 0] = 0;   // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 0;   // B
            }
            pixels[index + 3] = 255;     // A
        }
    }
}

} // namespace mr::client
