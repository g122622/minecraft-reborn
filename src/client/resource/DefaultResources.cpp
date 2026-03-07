#include "DefaultResources.hpp"
#include <spdlog/spdlog.h>

namespace mr {

// 静态成员初始化
bool DefaultResources::s_initialized = false;
std::vector<u8> DefaultResources::s_missingTexturePixels;
std::unique_ptr<BlockAppearance> DefaultResources::s_missingAppearance;

void DefaultResources::initialize() {
    if (s_initialized) {
        return;
    }

    spdlog::info("Initializing default resources...");

    generateMissingTexture();
    generateMissingAppearance();

    s_initialized = true;
    spdlog::info("Default resources initialized");
}

void DefaultResources::shutdown() {
    s_missingTexturePixels.clear();
    s_missingTexturePixels.shrink_to_fit();
    s_missingAppearance.reset();
    s_initialized = false;
}

const std::vector<u8>& DefaultResources::getMissingTexturePixels() {
    if (!s_initialized) {
        initialize();
    }
    return s_missingTexturePixels;
}

ResourceLocation DefaultResources::getMissingTextureLocation() {
    return ResourceLocation("minecraft:textures/misc/missing");
}

const BlockAppearance* DefaultResources::getMissingAppearance() {
    if (!s_initialized) {
        initialize();
    }
    return s_missingAppearance.get();
}

ResourceLocation DefaultResources::getMissingModelLocation() {
    return ResourceLocation("minecraft:block/missing");
}

void DefaultResources::generateMissingTexture() {
    // 创建 16x16 紫黑方块纹理
    // 布局: 4x4 的方块，每个方块 4x4 像素
    // 紫色 (240, 0, 240) 和 黑色 (0, 0, 0) 交替

    constexpr u32 width = 16;
    constexpr u32 height = 16;
    s_missingTexturePixels.resize(width * height * 4); // RGBA

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * 4;

            // 计算这个像素应该是什么颜色
            // 4x4 方块模式：紫黑交替
            u32 blockX = x / 4;
            u32 blockY = y / 4;
            bool isPurple = ((blockX + blockY) % 2) == 0;

            if (isPurple) {
                // 紫色 (F800F8) - MC 标准缺失纹理颜色
                s_missingTexturePixels[idx + 0] = 248; // R
                s_missingTexturePixels[idx + 1] = 0;   // G
                s_missingTexturePixels[idx + 2] = 248; // B
            } else {
                // 黑色
                s_missingTexturePixels[idx + 0] = 0;   // R
                s_missingTexturePixels[idx + 1] = 0;   // G
                s_missingTexturePixels[idx + 2] = 0;   // B
            }
            s_missingTexturePixels[idx + 3] = 255;     // A
        }
    }
}

void DefaultResources::generateMissingAppearance() {
    s_missingAppearance = std::make_unique<BlockAppearance>();

    // 创建一个标准立方体模型
    ModelElement element;
    element.from = {0.0f, 0.0f, 0.0f};
    element.to = {16.0f, 16.0f, 16.0f};

    // 为所有面设置缺失纹理
    // 纹理路径使用 "#missing" 引用，在烘焙时解析
    for (Direction dir : Directions::all()) {
        ModelFace face;
        face.texture = "#missing";
        face.uv = {0.0f, 0.0f, 16.0f, 16.0f};
        element.faces[dir] = face;
    }

    s_missingAppearance->elements.push_back(element);
    s_missingAppearance->xRotation = 0;
    s_missingAppearance->yRotation = 0;
    s_missingAppearance->uvLock = false;

    // 设置面纹理（将在 ResourceManager 中填充实际的纹理区域）
    // 这里使用一个特殊的纹理位置来标识缺失纹理
    TextureRegion missingRegion;
    missingRegion.u0 = 0.0f;
    missingRegion.v0 = 0.0f;
    missingRegion.u1 = 1.0f;
    missingRegion.v1 = 1.0f;

    // 为所有方向设置缺失纹理区域
    s_missingAppearance->faceTextures["down"] = missingRegion;
    s_missingAppearance->faceTextures["up"] = missingRegion;
    s_missingAppearance->faceTextures["north"] = missingRegion;
    s_missingAppearance->faceTextures["south"] = missingRegion;
    s_missingAppearance->faceTextures["west"] = missingRegion;
    s_missingAppearance->faceTextures["east"] = missingRegion;
}

} // namespace mr
