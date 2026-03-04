#include "DefaultTextureAtlas.hpp"
#include <cmath>

namespace mr::client {

std::vector<u8> DefaultTextureAtlas::generate() {
    std::vector<u8> pixels(ATLAS_SIZE * ATLAS_SIZE * 4, 0);

    // 生成所有方块纹理
    generateStoneTexture(pixels.data(), 0, 0);
    generateGrassTopTexture(pixels.data(), 1, 0);
    generateGrassSideTexture(pixels.data(), 2, 0);
    generateDirtTexture(pixels.data(), 3, 0);
    generateCobblestoneTexture(pixels.data(), 4, 0);
    generateWoodSideTexture(pixels.data(), 5, 0);
    generateWoodTopTexture(pixels.data(), 6, 0);
    generateLeavesTexture(pixels.data(), 7, 0);
    generateSandTexture(pixels.data(), 8, 0);
    generateGravelTexture(pixels.data(), 9, 0);
    generateOreTexture(pixels.data(), 10, 0, 255, 215, 0);    // 金矿
    generateOreTexture(pixels.data(), 11, 0, 210, 180, 140);   // 铁矿
    generateOreTexture(pixels.data(), 12, 0, 50, 50, 50);      // 煤矿
    generateOreTexture(pixels.data(), 13, 0, 0, 200, 255);     // 钻石矿
    generateBedrockTexture(pixels.data(), 14, 0);

    return pixels;
}

TextureRegion DefaultTextureAtlas::getRegion(BlockTextureId textureId) {
    u32 id = static_cast<u32>(textureId);
    u32 tilesPerRow = ATLAS_SIZE / TILE_SIZE;
    u32 tileX = id % tilesPerRow;
    u32 tileY = id / tilesPerRow;

    f32 tileUV = 1.0f / static_cast<f32>(tilesPerRow);
    f32 u0 = static_cast<f32>(tileX) * tileUV;
    f32 v0 = static_cast<f32>(tileY) * tileUV;

    return TextureRegion(u0, v0, u0 + tileUV, v0 + tileUV);
}

u8 DefaultTextureAtlas::noise2D(u32 x, u32 y, u32 seed) {
    u32 n = x + y * 57 + seed * 131;
    n = (n << 13) ^ n;
    return static_cast<u8>((n * (n * n * 15731 + 789221) + 1376312589) & 0xFF);
}

void DefaultTextureAtlas::setPixel(u8* pixels, u32 tileX, u32 tileY, u32 localX, u32 localY,
                                   u8 r, u8 g, u8 b, u8 a) {
    u32 globalX = tileX * TILE_SIZE + localX;
    u32 globalY = tileY * TILE_SIZE + localY;
    u32 index = (globalY * ATLAS_SIZE + globalX) * 4;
    pixels[index + 0] = r;
    pixels[index + 1] = g;
    pixels[index + 2] = b;
    pixels[index + 3] = a;
}

void DefaultTextureAtlas::generateStoneTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 42);
            u8 base = 128;
            u8 variation = noise / 4;
            u8 gray = base + variation - 32;
            setPixel(pixels, tileX, tileY, x, y, gray, gray, gray);
        }
    }
}

void DefaultTextureAtlas::generateGrassTopTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 123);
            u8 green = 100 + (noise % 40);
            setPixel(pixels, tileX, tileY, x, y, 50, green, 30);
        }
    }
}

void DefaultTextureAtlas::generateGrassSideTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 456);
            // 顶部是草地
            if (y < 4) {
                u8 green = 100 + (noise % 40);
                setPixel(pixels, tileX, tileY, x, y, 50, green, 30);
            } else {
                // 下面是泥土
                u8 brown = 100 + (noise % 30);
                setPixel(pixels, tileX, tileY, x, y, brown, 70, 40);
            }
        }
    }
}

void DefaultTextureAtlas::generateDirtTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 789);
            u8 brown = 100 + (noise % 30);
            setPixel(pixels, tileX, tileY, x, y, brown, 70, 40);
        }
    }
}

void DefaultTextureAtlas::generateCobblestoneTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 321);
            u8 gray = 100 + (noise % 60);
            // 添加一些裂纹效果
            if ((x + y) % 4 == 0) {
                gray -= 20;
            }
            setPixel(pixels, tileX, tileY, x, y, gray, gray, gray);
        }
    }
}

void DefaultTextureAtlas::generateWoodSideTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 555);
            // 垂直条纹
            u8 stripe = (x % 4 < 2) ? 0 : 15;
            u8 brown = 120 + stripe + (noise % 20);
            setPixel(pixels, tileX, tileY, x, y, brown, 80, 40);
        }
    }
}

void DefaultTextureAtlas::generateWoodTopTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 666);
            // 年轮效果
            u32 cx = TILE_SIZE / 2;
            u32 cy = TILE_SIZE / 2;
            u32 dist = static_cast<u32>(std::sqrt(static_cast<float>((x - cx) * (x - cx) + (y - cy) * (y - cy))));
            u8 ring = (dist % 3 == 0) ? 20 : 0;
            u8 brown = 140 + ring + (noise % 15);
            setPixel(pixels, tileX, tileY, x, y, brown, 100, 50);
        }
    }
}

void DefaultTextureAtlas::generateLeavesTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 777);
            if (noise > 80) {
                u8 green = 60 + (noise % 50);
                setPixel(pixels, tileX, tileY, x, y, 30, green, 20);
            } else {
                // 透明区域
                setPixel(pixels, tileX, tileY, x, y, 0, 0, 0, 0);
            }
        }
    }
}

void DefaultTextureAtlas::generateSandTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 888);
            u8 sand = 200 + (noise % 30);
            setPixel(pixels, tileX, tileY, x, y, sand, 190, 140);
        }
    }
}

void DefaultTextureAtlas::generateGravelTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 999);
            u8 gray = 100 + (noise % 80);
            setPixel(pixels, tileX, tileY, x, y, gray, gray - 10, gray - 20);
        }
    }
}

void DefaultTextureAtlas::generateOreTexture(u8* pixels, u32 tileX, u32 tileY, u8 oreR, u8 oreG, u8 oreB) {
    // 先生成石头底
    generateStoneTexture(pixels, tileX, tileY);

    // 添加矿石斑点
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x + 1000, y + 1000, tileX + tileY);
            if (noise > 200) {
                setPixel(pixels, tileX, tileY, x, y, oreR, oreG, oreB);
            }
        }
    }
}

void DefaultTextureAtlas::generateBedrockTexture(u8* pixels, u32 tileX, u32 tileY) {
    for (u32 y = 0; y < TILE_SIZE; ++y) {
        for (u32 x = 0; x < TILE_SIZE; ++x) {
            u8 noise = noise2D(x, y, 111);
            u8 gray = 40 + (noise % 40);
            setPixel(pixels, tileX, tileY, x, y, gray, gray, gray);
        }
    }
}

} // namespace mr::client
