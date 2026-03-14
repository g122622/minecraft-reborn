#include "ItemRenderer.hpp"
#include "../../ui/GuiRenderer.hpp"
#include "../../resource/ResourceManager.hpp"
#include "../../resource/ItemTextureAtlas.hpp"
#include "../../../common/item/Item.hpp"
#include "../../../common/item/ItemStack.hpp"
#include "../../../common/item/BlockItem.hpp"
#include "../../../common/world/block/Block.hpp"
#include <spdlog/spdlog.h>

namespace mc::client {

ItemRenderer::ItemRenderer()
    : m_resourceManager(nullptr)
    , m_itemTextureAtlas(nullptr)
    , m_initialized(false)
{
}

Result<void> ItemRenderer::initialize(
    ResourceManager* resourceManager,
    ItemTextureAtlas* itemTextureAtlas)
{
    if (resourceManager == nullptr) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    m_resourceManager = resourceManager;
    m_itemTextureAtlas = itemTextureAtlas;
    m_initialized = true;

    spdlog::info("ItemRenderer: Initialized");
    return {};
}

void ItemRenderer::renderItem(GuiRenderer& gui, const ItemStack& stack, f32 x, f32 y, f32 size) {
    if (stack.isEmpty()) {
        return;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return;
    }

    renderItem(gui, item, x, y, size);
}

void ItemRenderer::renderItem(GuiRenderer& gui, const Item* item, f32 x, f32 y, f32 size) {
    if (item == nullptr || !m_initialized) {
        return;
    }

    const TextureRegion* region = getItemTextureRegion(item);
    if (region == nullptr) {
        // 没有找到纹理，绘制占位符
        // 使用半透明红色矩形表示缺失纹理
        gui.fillRect(x, y, size, size, 0x80FF0000);
        return;
    }

    renderItem(gui, *region, x, y, size);
}

void ItemRenderer::renderItem(GuiRenderer& gui, const TextureRegion& region, f32 x, f32 y, f32 size) {
    // 使用drawTexturedRect绘制物品纹理
    // 使用 alpha=254 的颜色，确保走物品纹理采样分支且保持可见
    gui.drawTexturedRect(x, y, size, size,
                         region.u0, region.v0, region.u1, region.v1,
                         GuiRenderer::ITEM_TEXTURE_COLOR);
}

bool ItemRenderer::isBlockItem(const Item* item) const {
    if (item == nullptr) {
        return false;
    }

    // 检查是否为BlockItem类型
    const BlockItem* blockItem = dynamic_cast<const BlockItem*>(item);
    return blockItem != nullptr;
}

const TextureRegion* ItemRenderer::getItemTextureRegion(const Item* item) const {
    if (item == nullptr || !m_initialized) {
        return nullptr;
    }

    // 首先检查是否为方块物品
    const BlockItem* blockItem = dynamic_cast<const BlockItem*>(item);
    if (blockItem != nullptr) {
        // 方块物品使用方块纹理图集
        const Block& block = blockItem->block();
        const ResourceLocation& blockId = block.blockLocation();

        // 获取方块纹理（使用侧面的默认纹理）
        // 方块纹理位置格式: minecraft:block/stone
        ResourceLocation textureLoc(blockId.namespace_(), "block/" + blockId.path());

        if (m_resourceManager != nullptr) {
            const TextureRegion* region = m_resourceManager->getTextureRegion(textureLoc);
            if (region != nullptr) {
                return region;
            }
        }
    }

    // 非方块物品使用物品纹理图集
    if (m_itemTextureAtlas != nullptr) {
        const TextureRegion* region = m_itemTextureAtlas->getItemTexture(item->itemId());
        if (region != nullptr) {
            return region;
        }

        // 尝试使用资源位置查找
        const ResourceLocation& itemId = item->itemLocation();
        ResourceLocation textureLoc(itemId.namespace_(), "item/" + itemId.path());
        region = m_itemTextureAtlas->getItemTexture(textureLoc);
        if (region != nullptr) {
            return region;
        }
    }

    return nullptr;
}

} // namespace mc::client
