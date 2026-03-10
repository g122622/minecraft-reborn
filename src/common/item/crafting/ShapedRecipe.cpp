#include "item/crafting/ShapedRecipe.hpp"
#include <algorithm>

namespace mr {
namespace crafting {

ShapedRecipe::ShapedRecipe(const ResourceLocation& id,
                           i32 width,
                           i32 height,
                           std::vector<Ingredient> ingredients,
                           ItemStack result,
                           const String& group)
    : m_id(id)
    , m_width(width)
    , m_height(height)
    , m_ingredients(std::move(ingredients))
    , m_result(std::move(result))
    , m_group(group) {
}

bool ShapedRecipe::matches(const CraftingInventory& inventory) const {
    // 计算输入网格中内容的边界框
    i32 minInputX, minInputY, maxInputX, maxInputY;
    if (!getContentBounds(inventory, minInputX, minInputY, maxInputX, maxInputY)) {
        // 空网格只能匹配空配方
        return m_ingredients.empty();
    }

    i32 inputWidth = maxInputX - minInputX + 1;
    i32 inputHeight = maxInputY - minInputY + 1;

    // 检查尺寸是否匹配
    if (inputWidth != m_width || inputHeight != m_height) {
        return false;
    }

    // 尝试所有可能的偏移位置（通常只需要一个，因为尺寸已匹配）
    // 但为了正确性，我们检查从边界开始的匹配
    for (i32 offsetY = 0; offsetY <= inventory.getHeight() - m_height; ++offsetY) {
        for (i32 offsetX = 0; offsetX <= inventory.getWidth() - m_width; ++offsetX) {
            // 跳过不可能的位置
            if (offsetX > minInputX || offsetY > minInputY) {
                continue;
            }

            // 检查正向匹配
            if (checkMatch(inventory, offsetX, offsetY, false)) {
                return true;
            }

            // 检查镜像匹配（水平翻转）
            if (checkMatch(inventory, offsetX, offsetY, true)) {
                return true;
            }
        }
    }

    return false;
}

ItemStack ShapedRecipe::assemble(const CraftingInventory& inventory) const {
    (void)inventory;
    return m_result.copy();
}

bool ShapedRecipe::canFitIn(i32 width, i32 height) const {
    return width >= m_width && height >= m_height;
}

bool ShapedRecipe::checkMatch(const CraftingInventory& inventory,
                               i32 offsetX,
                               i32 offsetY,
                               bool mirrored) const {
    for (i32 y = 0; y < m_height; ++y) {
        for (i32 x = 0; x < m_width; ++x) {
            // 计算原料索引
            i32 ingredientIndex = y * m_width + (mirrored ? m_width - 1 - x : x);
            const Ingredient& ingredient = m_ingredients[ingredientIndex];

            // 计算网格位置
            i32 slotX = offsetX + x;
            i32 slotY = offsetY + y;
            i32 slot = inventory.posToSlot(slotX, slotY);

            if (slot < 0) {
                // 超出网格范围
                return false;
            }

            ItemStack stack = inventory.getItem(slot);

            if (!ingredient.test(stack)) {
                return false;
            }
        }
    }

    // 检查网格其他位置是否为空
    for (i32 y = 0; y < inventory.getHeight(); ++y) {
        for (i32 x = 0; x < inventory.getWidth(); ++x) {
            // 跳过配方区域
            if (x >= offsetX && x < offsetX + m_width &&
                y >= offsetY && y < offsetY + m_height) {
                continue;
            }

            // 其他位置必须为空
            i32 slot = inventory.posToSlot(x, y);
            if (slot >= 0 && !inventory.getItem(slot).isEmpty()) {
                return false;
            }
        }
    }

    return true;
}

bool ShapedRecipe::getContentBounds(const CraftingInventory& inventory,
                                     i32& outMinX,
                                     i32& outMinY,
                                     i32& outMaxX,
                                     i32& outMaxY) {
    bool hasContent = false;
    outMinX = inventory.getWidth();
    outMinY = inventory.getHeight();
    outMaxX = -1;
    outMaxY = -1;

    for (i32 y = 0; y < inventory.getHeight(); ++y) {
        for (i32 x = 0; x < inventory.getWidth(); ++x) {
            i32 slot = inventory.posToSlot(x, y);
            if (slot >= 0 && !inventory.getItem(slot).isEmpty()) {
                hasContent = true;
                outMinX = std::min(outMinX, x);
                outMinY = std::min(outMinY, y);
                outMaxX = std::max(outMaxX, x);
                outMaxY = std::max(outMaxY, y);
            }
        }
    }

    return hasContent;
}

} // namespace crafting
} // namespace mr
