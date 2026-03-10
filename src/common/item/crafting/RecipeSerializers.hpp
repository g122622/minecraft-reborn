#pragma once

#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/ShapelessRecipe.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "core/Result.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace mr {
namespace crafting {

/**
 * @brief 配方序列化器
 *
 * 提供从JSON解析配方的功能，兼容MC 1.16.5数据包格式。
 *
 * 支持的配方类型：
 * - minecraft:crafting_shaped - 有序合成
 * - minecraft:crafting_shapeless - 无序合成
 *
 * JSON格式示例（有序合成）：
 * @code
 * {
 *   "type": "minecraft:crafting_shaped",
 *   "pattern": ["###", " # ", "###"],
 *   "key": {
 *     "#": { "item": "minecraft:oak_planks" }
 *   },
 *   "result": {
 *     "item": "minecraft:crafting_table",
 *     "count": 1
 *   },
 *   "group": "crafting_tables"
 * }
 * @endcode
 *
 * JSON格式示例（无序合成）：
 * @code
 * {
 *   "type": "minecraft:crafting_shapeless",
 *   "ingredients": [
 *     { "item": "minecraft:iron_ingot" },
 *     { "item": "minecraft:stick" }
 *   ],
 *   "result": {
 *     "item": "minecraft:iron_nugget",
 *     "count": 9
 *   }
 * }
 * @endcode
 */
class RecipeSerializers {
public:
    /**
     * @brief 从JSON解析配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<CraftingRecipe>> fromJson(
        const ResourceLocation& id,
        const nlohmann::json& json);

    /**
     * @brief 解析有序合成配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<ShapedRecipe>> parseShapedRecipe(
        const ResourceLocation& id,
        const nlohmann::json& json);

    /**
     * @brief 解析无序合成配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<ShapelessRecipe>> parseShapelessRecipe(
        const ResourceLocation& id,
        const nlohmann::json& json);

    /**
     * @brief 解析原料
     * @param json JSON数据
     * @return 解析的原料，或错误
     *
     * 支持格式：
     * - 单个物品: { "item": "minecraft:stone" }
     * - 物品标签: { "tag": "minecraft:planks" }
     * - 多选项: [ { "item": "a" }, { "item": "b" } ]
     */
    static Result<Ingredient> parseIngredient(const nlohmann::json& json);

    /**
     * @brief 解析结果物品堆
     * @param json JSON数据
     * @return 解析的物品堆，或错误
     *
     * 支持格式：
     * - { "item": "minecraft:stone", "count": 1 }
     */
    static Result<ItemStack> parseResult(const nlohmann::json& json);

private:
    /**
     * @brief 解析pattern数组为宽度和高度
     * @param pattern 字符串数组
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     * @return 是否成功
     */
    static bool parsePatternDimensions(const std::vector<String>& pattern,
                                        i32& outWidth,
                                        i32& outHeight);

    /**
     * @brief 从pattern和key解析原料列表
     * @param pattern 字符串数组
     * @param key 键映射
     * @param width 宽度
     * @return 解析的原料列表，或错误
     */
    static Result<std::vector<Ingredient>> parsePatternIngredients(
        const std::vector<String>& pattern,
        const nlohmann::json& key,
        i32 width);
};

} // namespace crafting
} // namespace mr
