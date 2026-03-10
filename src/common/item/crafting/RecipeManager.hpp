#pragma once

#include "item/crafting/IRecipe.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "resource/ResourceLocation.hpp"
#include "core/Result.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace mr {
namespace crafting {

/**
 * @brief 合成配方类型别名
 *
 * 使用CraftingInventory作为容器类型的配方接口
 */
using CraftingRecipe = IRecipe<CraftingInventory>;

/**
 * @brief 配方管理器，存储和管理所有注册的配方
 *
 * RecipeManager 是配方的中央注册表，支持：
 * - 注册配方
 * - 按ID查询配方
 * - 按类型查询配方
 * - 查找匹配给定容器的配方
 *
 * 线程安全：所有公共方法都是线程安全的
 *
 * 使用示例：
 * @code
 * RecipeManager& manager = RecipeManager::instance();
 *
 * // 注册配方
 * manager.registerRecipe(std::make_unique<ShapedRecipe>(...));
 *
 * // 查找匹配的配方
 * CraftingInventory inv(3, 3);
 * const CraftingRecipe* recipe = manager.findMatchingRecipe(inv);
 * if (recipe) {
 *     ItemStack result = recipe->assemble(inv);
 * }
 * @endcode
 *
 * 注意事项：
 * - 配方ID必须唯一，重复注册会覆盖旧配方
 * - 配方一旦注册不应修改
 */
class RecipeManager {
public:
    /**
     * @brief 获取单例实例
     * @return RecipeManager实例引用
     */
    static RecipeManager& instance();

    /**
     * @brief 注册配方
     * @param recipe 配方实例（移动语义）
     * @return 如果注册成功返回true，如果ID冲突返回false
     *
     * 注意：配方ID必须唯一
     */
    bool registerRecipe(std::unique_ptr<CraftingRecipe> recipe);

    /**
     * @brief 按ID获取配方
     * @param id 配方ID
     * @return 配方指针，如果不存在返回nullptr
     */
    [[nodiscard]] const CraftingRecipe* getRecipe(const ResourceLocation& id) const;

    /**
     * @brief 检查配方是否存在
     * @param id 配方ID
     * @return 如果存在返回true
     */
    [[nodiscard]] bool hasRecipe(const ResourceLocation& id) const;

    /**
     * @brief 获取所有配方
     * @return 配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getAllRecipes() const;

    /**
     * @brief 按类型获取配方
     * @param type 配方类型
     * @return 该类型的所有配方
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesByType(RecipeType type) const;

    /**
     * @brief 查找匹配给定容器的配方
     * @param inventory 合成容器
     * @return 匹配的配方，如果无匹配返回nullptr
     *
     * 查找顺序：
     * 1. 首先检查有序合成配方
     * 2. 然后检查无序合成配方
     * 返回第一个匹配的配方
     */
    [[nodiscard]] const CraftingRecipe* findMatchingRecipe(
        const CraftingInventory& inventory) const;

    /**
     * @brief 查找所有匹配给定容器的配方
     * @param inventory 合成容器
     * @return 所有匹配的配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> findAllMatchingRecipes(
        const CraftingInventory& inventory) const;

    /**
     * @brief 查找能产生给定物品的配方
     * @param result 结果物品
     * @return 所有能产生该物品的配方
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(
        const Item& result) const;

    /**
     * @brief 查找能产生给定物品的配方（物品堆版本）
     * @param result 结果物品堆
     * @return 所有能产生该物品的配方
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(
        const ItemStack& result) const;

    /**
     * @brief 获取配方数量
     * @return 注册的配方总数
     */
    [[nodiscard]] size_t getRecipeCount() const;

    /**
     * @brief 清除所有配方
     *
     * 警告：此方法主要用于测试，生产代码不应使用
     */
    void clear();

    /**
     * @brief 遍历所有配方
     * @param callback 对每个配方调用的回调函数
     */
    void forEachRecipe(std::function<void(const CraftingRecipe&)> callback) const;

private:
    RecipeManager() = default;
    ~RecipeManager() = default;
    RecipeManager(const RecipeManager&) = delete;
    RecipeManager& operator=(const RecipeManager&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<ResourceLocation, std::unique_ptr<CraftingRecipe>, std::hash<ResourceLocation>> m_recipesById;

    // 按类型索引的配方列表，用于快速查询
    std::unordered_map<RecipeType, std::vector<const CraftingRecipe*>> m_recipesByType;

    // 按结果物品索引的配方列表，用于快速查询
    std::unordered_map<ItemId, std::vector<const CraftingRecipe*>> m_recipesByResult;
};

} // namespace crafting
} // namespace mr
