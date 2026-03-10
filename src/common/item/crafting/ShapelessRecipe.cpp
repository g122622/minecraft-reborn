#include "item/crafting/ShapelessRecipe.hpp"
#include <algorithm>

namespace mr {
namespace crafting {

ShapelessRecipe::ShapelessRecipe(const ResourceLocation& id,
                                 std::vector<Ingredient> ingredients,
                                 ItemStack result,
                                 const String& group)
    : m_id(id)
    , m_ingredients(std::move(ingredients))
    , m_result(std::move(result))
    , m_group(group) {
}

bool ShapelessRecipe::matches(const CraftingInventory& inventory) const {
    // 统计网格中的非空槽位数量
    i32 nonEmptySlots = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++nonEmptySlots;
        }
    }

    // 原料数量必须等于非空槽位数量
    if (static_cast<i32>(m_ingredients.size()) != nonEmptySlots) {
        return false;
    }

    // 跟踪已使用的槽位
    std::vector<bool> used(inventory.getContainerSize(), false);

    // 对每个原料，在网格中查找匹配的物品
    for (const Ingredient& ingredient : m_ingredients) {
        bool found = false;
        for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
            if (!used[i] && ingredient.test(inventory.getItem(i))) {
                used[i] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

ItemStack ShapelessRecipe::assemble(const CraftingInventory& inventory) const {
    (void)inventory;
    return m_result.copy();
}

bool ShapelessRecipe::canFitIn(i32 width, i32 height) const {
    return static_cast<i32>(m_ingredients.size()) <= width * height;
}

} // namespace crafting
} // namespace mr
