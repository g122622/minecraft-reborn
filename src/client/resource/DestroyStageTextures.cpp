#include "DestroyStageTextures.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <spdlog/spdlog.h>
#include <random>
#include <cmath>

namespace mc {
namespace client {
namespace renderer {

// ============================================================================
// 静态成员
// ============================================================================

DestroyStageTextures& DestroyStageTextures::instance() {
    static DestroyStageTextures instance;
    return instance;
}

// ============================================================================
// 初始化
// ============================================================================

bool DestroyStageTextures::initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("DestroyStageTextures: Initializing...");

    // 尝试从资源包加载，失败则使用程序生成
    for (size_t i = 0; i < STAGE_COUNT; ++i) {
        if (!loadTextureFromResourcePack(i, m_textures[i])) {
            spdlog::debug("DestroyStageTextures: Using generated texture for stage {}", i);
            generateDefaultTexture(i, m_textures[i]);
        }
    }

    // 构建纹理图集
    buildAtlas();

    m_initialized = true;
    spdlog::info("DestroyStageTextures: Initialized successfully");
    return true;
}

void DestroyStageTextures::cleanup() {
    for (auto& texture : m_textures) {
        texture.clear();
        texture.shrink_to_fit();
    }
    m_atlasData.clear();
    m_atlasData.shrink_to_fit();
    m_initialized = false;
}

// ============================================================================
// 纹理访问
// ============================================================================

const u8* DestroyStageTextures::getTextureData(size_t stage) const {
    if (stage >= STAGE_COUNT) {
        return nullptr;
    }
    return m_textures[stage].data();
}

bool DestroyStageTextures::getTextureUV(size_t stage,
                                         f32& u0, f32& v0,
                                         f32& u1, f32& v1) const {
    if (stage >= STAGE_COUNT) {
        return false;
    }

    // 图集布局：2行5列
    // 阶段0-4在第一行，阶段5-9在第二行
    u32 col = static_cast<u32>(stage % 5);
    u32 row = static_cast<u32>(stage / 5);

    f32 cellWidth = 1.0f / 5.0f;
    f32 cellHeight = 1.0f / 2.0f;

    u0 = static_cast<f32>(col) * cellWidth;
    v0 = static_cast<f32>(row) * cellHeight;
    u1 = u0 + cellWidth;
    v1 = v0 + cellHeight;

    return true;
}

// ============================================================================
// 私有方法
// ============================================================================

void DestroyStageTextures::generateDefaultTexture(size_t stage, std::vector<u8>& data) {
    // 16x16 RGBA 纹理
    constexpr size_t pixelCount = TEXTURE_SIZE * TEXTURE_SIZE;
    data.resize(pixelCount * 4, 0);

    // 使用确定性随机种子，基于阶段号
    std::mt19937 rng(static_cast<u32>(stage * 12345 + 67890));
    std::uniform_real_distribution<f32> dist(0.0f, 1.0f);

    // 破坏强度随阶段增加
    // 阶段0：几乎无裂纹
    // 阶段9：几乎完全破碎
    f32 intensity = static_cast<f32>(stage) / static_cast<f32>(STAGE_COUNT - 1);

    for (u32 y = 0; y < TEXTURE_SIZE; ++y) {
        for (u32 x = 0; x < TEXTURE_SIZE; ++x) {
            size_t idx = (y * TEXTURE_SIZE + x) * 4;

            // 基础噪声
            f32 noise = dist(rng);

            // 创建裂纹图案
            // 使用多个噪声层叠加
            f32 crack = 0.0f;

            // 水平裂纹
            if (y >= 4 && y <= 5 && stage >= 2) {
                crack += 0.3f;
            }
            if (y >= 10 && y <= 11 && stage >= 4) {
                crack += 0.3f;
            }

            // 垂直裂纹
            if (x >= 7 && x <= 8 && stage >= 3) {
                crack += 0.3f;
            }
            if (x >= 3 && x <= 4 && stage >= 5) {
                crack += 0.3f;
            }

            // 对角裂纹
            if (std::abs(static_cast<i32>(x) - static_cast<i32>(y)) <= 1 && stage >= 6) {
                crack += 0.25f;
            }
            if (std::abs(static_cast<i32>(x) + static_cast<i32>(y) - 15) <= 1 && stage >= 7) {
                crack += 0.25f;
            }

            // 随机裂纹点
            if (noise < intensity * 0.4f) {
                crack += 0.4f;
            }

            // 边缘破损
            if ((x == 0 || x == TEXTURE_SIZE - 1 || y == 0 || y == TEXTURE_SIZE - 1) &&
                stage >= 3) {
                crack += 0.3f * intensity;
            }

            // 计算最终alpha值
            // 破坏纹理使用alpha通道控制可见度
            // 高alpha = 更明显的裂纹
            f32 alpha = std::min(1.0f, crack * intensity * 1.5f);

            // RGBA
            data[idx + 0] = 0;     // R - 黑色裂纹
            data[idx + 1] = 0;     // G
            data[idx + 2] = 0;     // B
            data[idx + 3] = static_cast<u8>(alpha * 255.0f);  // A
        }
    }
}

bool DestroyStageTextures::loadTextureFromResourcePack(size_t stage, std::vector<u8>& data) {
    // 构建纹理路径
    // 现代 MC 1.13+ 路径: textures/block/destroy_stage_X.png
    // 旧版 MC 1.12 路径: textures/blocks/destroy_stage_X.png

    std::string modernPath = fmt::format("textures/block/destroy_stage_{}.png", stage);
    std::string legacyPath = fmt::format("textures/blocks/destroy_stage_{}.png", stage);

    // TODO: 从资源管理器加载纹理
    // 当前资源系统尚未完全实现纹理加载，暂时返回false使用程序生成
    // 未来实现：
    // auto& resourceManager = ResourceManager::instance();
    // if (auto* pack = resourceManager.getActivePack()) {
    //     if (pack->hasResource(modernPath)) {
    //         return pack->loadTexture(modernPath, data, TEXTURE_SIZE, TEXTURE_SIZE);
    //     }
    //     if (pack->hasResource(legacyPath)) {
    //         return pack->loadTexture(legacyPath, data, TEXTURE_SIZE, TEXTURE_SIZE);
    //     }
    // }

    return false;
}

void DestroyStageTextures::buildAtlas() {
    // 图集尺寸：5列2行，每格16x16
    constexpr u32 atlasWidth = TEXTURE_SIZE * 5;
    constexpr u32 atlasHeight = TEXTURE_SIZE * 2;

    m_atlasData.resize(atlasWidth * atlasHeight * 4, 0);

    for (size_t stage = 0; stage < STAGE_COUNT; ++stage) {
        u32 col = static_cast<u32>(stage % 5);
        u32 row = static_cast<u32>(stage / 5);

        u32 destX = col * TEXTURE_SIZE;
        u32 destY = row * TEXTURE_SIZE;

        const auto& srcData = m_textures[stage];

        for (u32 y = 0; y < TEXTURE_SIZE; ++y) {
            for (u32 x = 0; x < TEXTURE_SIZE; ++x) {
                size_t srcIdx = (y * TEXTURE_SIZE + x) * 4;
                size_t dstIdx = ((destY + y) * atlasWidth + destX + x) * 4;

                m_atlasData[dstIdx + 0] = srcData[srcIdx + 0];
                m_atlasData[dstIdx + 1] = srcData[srcIdx + 1];
                m_atlasData[dstIdx + 2] = srcData[srcIdx + 2];
                m_atlasData[dstIdx + 3] = srcData[srcIdx + 3];
            }
        }
    }
}

} // namespace renderer
} // namespace client
} // namespace mc
