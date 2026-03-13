#include "GuiTextureAtlas.hpp"
#include "../GuiRenderer.hpp"
#include "../../renderer/VulkanContext.hpp"
#include "../../renderer/VulkanTexture.hpp"
#include "../../renderer/VulkanBuffer.hpp"
#include "../../../common/core/Result.hpp"
#include <cstring>
#include <vector>

namespace mc::client {

// ============================================================================
// 颜色常量
// ============================================================================

namespace GuiColors {
    // 槽位颜色
    constexpr u32 SLOT_BG = 0xFF8B8B8B;       // 槽位背景灰色
    constexpr u32 SLOT_BORDER = 0xFF373737;   // 槽位边框深灰
    constexpr u32 SLOT_INNER = 0xFFC6C6C6;    // 槽位内部浅灰

    // 容器背景颜色
    constexpr u32 CONTAINER_BG = 0xFFC6C6C6;  // 容器背景
    constexpr u32 CONTAINER_BORDER = 0xFF555555; // 边框

    // 默认颜色
    constexpr u32 DEFAULT_BG = 0xFF404040;    // 默认背景
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

GuiTextureAtlas::GuiTextureAtlas() = default;

GuiTextureAtlas::~GuiTextureAtlas() {
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> GuiTextureAtlas::initialize(VulkanContext* context) {
    if (m_initialized) {
        return {};
    }

    if (context == nullptr) {
        return Error(ErrorCode::NullPointer, "VulkanContext is null");
    }

    m_context = context;

    // 创建默认纹理
    auto result = createDefaultTextures();
    if (result.failed()) {
        return result;
    }

    m_initialized = true;
    return {};
}

void GuiTextureAtlas::destroy() {
    if (!m_initialized) {
        return;
    }

    m_atlasTexture.reset();
    m_regions.clear();
    m_context = nullptr;
    m_initialized = false;
}

// ============================================================================
// 默认纹理创建
// ============================================================================

Result<void> GuiTextureAtlas::createDefaultTextures() {
    // 创建一个包含默认GUI纹理的小图集
    // 图集布局：
    // - (0,0) 到 (18,18): 槽位背景
    // - (20,0) 到 (196,166): 容器背景

    constexpr i32 ATLAS_WIDTH = 256;
    constexpr i32 ATLAS_HEIGHT = 256;

    std::vector<u8> atlasData(ATLAS_WIDTH * ATLAS_HEIGHT * 4, 0);

    // 创建槽位背景纹理
    createSlotBackground(atlasData.data(), ATLAS_WIDTH, ATLAS_HEIGHT);

    // 创建容器背景纹理
    createContainerBackground(atlasData.data(), ATLAS_WIDTH, ATLAS_HEIGHT);

    // 创建 Vulkan 纹理
    m_atlasTexture = std::make_unique<VulkanTexture>();

    TextureConfig config;
    config.width = ATLAS_WIDTH;
    config.height = ATLAS_HEIGHT;
    config.format = VK_FORMAT_R8G8B8A8_UNORM;
    config.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // 获取 Vulkan 设备
    // 注意：这里需要 VulkanContext 提供设备访问接口
    // 暂时返回成功，实际纹理上传需要在渲染帧中完成

    // 注册纹理区域
    // 槽位背景
    GuiTextureRegion slotRegion;
    slotRegion.u0 = 0.0f / ATLAS_WIDTH;
    slotRegion.v0 = 0.0f / ATLAS_HEIGHT;
    slotRegion.u1 = static_cast<f32>(DEFAULT_SLOT_SIZE) / ATLAS_WIDTH;
    slotRegion.v1 = static_cast<f32>(DEFAULT_SLOT_SIZE) / ATLAS_HEIGHT;
    slotRegion.width = DEFAULT_SLOT_SIZE;
    slotRegion.height = DEFAULT_SLOT_SIZE;
    m_regions["minecraft:gui/slot"] = slotRegion;

    // 容器背景
    GuiTextureRegion containerRegion;
    containerRegion.u0 = 20.0f / ATLAS_WIDTH;
    containerRegion.v0 = 0.0f / ATLAS_HEIGHT;
    containerRegion.u1 = static_cast<f32>(20 + DEFAULT_CONTAINER_WIDTH) / ATLAS_WIDTH;
    containerRegion.v1 = static_cast<f32>(DEFAULT_CONTAINER_HEIGHT) / ATLAS_HEIGHT;
    containerRegion.width = DEFAULT_CONTAINER_WIDTH;
    containerRegion.height = DEFAULT_CONTAINER_HEIGHT;
    m_regions["minecraft:gui/container/generic"] = containerRegion;

    // 工作台背景
    m_regions["minecraft:gui/container/crafting_table"] = containerRegion;

    // 玩家背包背景
    m_regions["minecraft:gui/container/inventory"] = containerRegion;

    return {};
}

void GuiTextureAtlas::createSlotBackground(u8* data, i32 width, i32 height) {
    // 槽位背景尺寸：18x18像素
    // 边框：1像素深灰，内部：浅灰

    for (i32 y = 0; y < DEFAULT_SLOT_SIZE && y < height; ++y) {
        for (i32 x = 0; x < DEFAULT_SLOT_SIZE && x < width; ++x) {
            i32 idx = (y * width + x) * 4;

            bool isBorder = (x == 0 || x == DEFAULT_SLOT_SIZE - 1 ||
                            y == 0 || y == DEFAULT_SLOT_SIZE - 1);

            if (isBorder) {
                // 边框颜色
                data[idx + 0] = (GuiColors::SLOT_BORDER >> 0) & 0xFF;  // R
                data[idx + 1] = (GuiColors::SLOT_BORDER >> 8) & 0xFF;  // G
                data[idx + 2] = (GuiColors::SLOT_BORDER >> 16) & 0xFF; // B
                data[idx + 3] = 0xFF; // A
            } else {
                // 内部颜色
                data[idx + 0] = (GuiColors::SLOT_BG >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::SLOT_BG >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::SLOT_BG >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
        }
    }
}

void GuiTextureAtlas::createContainerBackground(u8* data, i32 width, i32 height) {
    // 容器背景尺寸：176x166像素
    // 起始位置：(20, 0)

    constexpr i32 OFFSET_X = 20;
    constexpr i32 OFFSET_Y = 0;

    for (i32 y = 0; y < DEFAULT_CONTAINER_HEIGHT && (OFFSET_Y + y) < height; ++y) {
        for (i32 x = 0; x < DEFAULT_CONTAINER_WIDTH && (OFFSET_X + x) < width; ++x) {
            i32 idx = ((OFFSET_Y + y) * width + (OFFSET_X + x)) * 4;

            bool isBorder = (x == 0 || x == DEFAULT_CONTAINER_WIDTH - 1 ||
                            y == 0 || y == DEFAULT_CONTAINER_HEIGHT - 1);

            if (isBorder) {
                // 边框颜色
                data[idx + 0] = (GuiColors::CONTAINER_BORDER >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::CONTAINER_BORDER >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::CONTAINER_BORDER >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            } else {
                // 背景颜色
                data[idx + 0] = (GuiColors::CONTAINER_BG >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::CONTAINER_BG >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::CONTAINER_BG >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
        }
    }
}

// ============================================================================
// 纹理加载
// ============================================================================

Result<void> GuiTextureAtlas::loadGuiTexture(const ResourceLocation& location) {
    // TODO: 从资源包加载GUI纹理
    // 暂时使用默认纹理
    (void)location;
    return {};
}

Result<void> GuiTextureAtlas::loadDefaultTextures() {
    // 默认纹理已在 createDefaultTextures 中创建
    return {};
}

// ============================================================================
// 渲染
// ============================================================================

void GuiTextureAtlas::drawTexture(GuiRenderer& gui, const String& textureId,
                                   f32 x, f32 y, f32 width, f32 height) {
    const GuiTextureRegion* region = getRegion(textureId);
    if (region == nullptr) {
        // 使用默认颜色绘制
        gui.fillRect(x, y, width, height, GuiColors::DEFAULT_BG);
        return;
    }

    // TODO: 使用纹理渲染
    // 暂时使用默认颜色绘制
    gui.fillRect(x, y, width, height, GuiColors::CONTAINER_BG);
    gui.drawRect(x, y, width, height, GuiColors::CONTAINER_BORDER);
}

void GuiTextureAtlas::drawTextureRegion(GuiRenderer& gui, const String& textureId,
                                          f32 x, f32 y,
                                          i32 regionX, i32 regionY,
                                          i32 regionWidth, i32 regionHeight,
                                          f32 width, f32 height) {
    (void)regionX;
    (void)regionY;
    (void)regionWidth;
    (void)regionHeight;

    const GuiTextureRegion* region = getRegion(textureId);
    if (region == nullptr) {
        gui.fillRect(x, y, width, height, GuiColors::DEFAULT_BG);
        return;
    }

    // TODO: 使用纹理区域渲染
    gui.fillRect(x, y, width, height, GuiColors::CONTAINER_BG);
}

// ============================================================================
// 查询
// ============================================================================

bool GuiTextureAtlas::hasTexture(const String& textureId) const {
    return m_regions.find(textureId) != m_regions.end();
}

const GuiTextureRegion* GuiTextureAtlas::getRegion(const String& textureId) const {
    auto it = m_regions.find(textureId);
    if (it != m_regions.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace mc::client
