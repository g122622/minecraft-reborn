#pragma once

#include "item/crafting/RecipeManager.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 无序合成配方
 *
 * 无序合成只要求原料存在，不要求特定位置。
 * 原料顺序不影响匹配结果。
 *
 * 匹配算法：
 * 1. 检查原料数量是否匹配
 * 2. 对每个原料，在网格中查找匹配的物品
 * 3. 使用过的物品不能再次使用
 * 4. 所有原料都找到匹配则成功
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:crafting_shapeless",
 *   "ingredients": [
 *     { "item": "minecraft:iron_ingot" },
 *     { "item": "minecraft:stick" }
 *   ],
 *   "result": {
 *     "item": "minecraft:iron_ingot",
 *     "count": 1
 *   }
 * }
 * @endcode
 */
class ShapelessRecipe : public CraftingRecipe {
public:
    /**
     * @brief 构造无序合成配方
     * @param id 配方ID
     * @param ingredients 原料列表
     * @param result 结果物品堆
     * @param group 配方分组（可选）
     */
    ShapelessRecipe(const ResourceLocation& id,
                    std::vector<Ingredient> ingredients,
                    ItemStack result,
                    const String& group = "");

    /**
     * @brief 检查配方是否匹配给定容器
     * @param inventory 合成网格
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成结果物品堆
     * @param inventory 合成网格（用于获取NBT数据）
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取结果物品
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }

    /**
     * @brief 获取原料列表
     * @return 原料列表
     */
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override {
        return m_ingredients;
    }

    /**
     * @brief 获取配方分组
     * @return 分组名，如果无分组返回空字符串
     */
    [[nodiscard]] const String& getGroup() const override { return m_group; }

    /**
     * @brief 获取配方ID
     * @return 配方ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }

    /**
     * @brief 获取配方类型
     * @return RecipeType::ShapelessCrafting
     */
    [[nodiscard]] RecipeType getType() const override { return RecipeType::ShapelessCrafting; }

    /**
     * @brief 检查配方是否适合给定尺寸的网格
     * @param width 网格宽度
     * @param height 网格高度
     * @return 如果原料数量不超过网格大小返回true
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override;

private:
    ResourceLocation m_id;
    std::vector<Ingredient> m_ingredients;
    ItemStack m_result;
    String m_group;
};

} // namespace crafting
} // namespace mc
