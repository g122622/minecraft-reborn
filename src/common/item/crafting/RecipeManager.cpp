#include "item/crafting/RecipeManager.hpp"
#include <algorithm>

namespace mr {
namespace crafting {

RecipeManager& RecipeManager::instance() {
    static RecipeManager instance;
    return instance;
}

bool RecipeManager::registerRecipe(std::unique_ptr<CraftingRecipe> recipe) {
    if (!recipe) {
        return false;
    }

    ResourceLocation id = recipe->getId();

    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    if (m_recipesById.find(id) != m_recipesById.end()) {
        return false; // ID冲突
    }

    // 获取配方信息用于索引
    RecipeType type = recipe->getType();
    const Item* resultItem = recipe->getResultItem().getItem();

    // 存储配方
    const CraftingRecipe* recipePtr = recipe.get();
    m_recipesById[id] = std::move(recipe);

    // 更新类型索引
    m_recipesByType[type].push_back(recipePtr);

    // 更新结果索引
    if (resultItem) {
        m_recipesByResult[resultItem->itemId()].push_back(recipePtr);
    }

    return true;
}

const CraftingRecipe* RecipeManager::getRecipe(const ResourceLocation& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesById.find(id);
    if (it != m_recipesById.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool RecipeManager::hasRecipe(const ResourceLocation& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recipesById.find(id) != m_recipesById.end();
}

std::vector<const CraftingRecipe*> RecipeManager::getAllRecipes() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const CraftingRecipe*> result;
    result.reserve(m_recipesById.size());

    for (const auto& pair : m_recipesById) {
        result.push_back(pair.second.get());
    }

    return result;
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesByType(RecipeType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesByType.find(type);
    if (it != m_recipesByType.end()) {
        return it->second;
    }
    return {};
}

const CraftingRecipe* RecipeManager::findMatchingRecipe(
    const CraftingInventory& inventory) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    // 首先检查有序合成配方（更严格的匹配）
    auto shapedIt = m_recipesByType.find(RecipeType::ShapedCrafting);
    if (shapedIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : shapedIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    // 然后检查无序合成配方
    auto shapelessIt = m_recipesByType.find(RecipeType::ShapelessCrafting);
    if (shapelessIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : shapelessIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    // 最后检查通用合成配方
    auto craftingIt = m_recipesByType.find(RecipeType::Crafting);
    if (craftingIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : craftingIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    return nullptr;
}

std::vector<const CraftingRecipe*> RecipeManager::findAllMatchingRecipes(
    const CraftingInventory& inventory) const {

    std::vector<const CraftingRecipe*> result;
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& pair : m_recipesById) {
        if (pair.second->matches(inventory)) {
            result.push_back(pair.second.get());
        }
    }

    return result;
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesForResult(
    const Item& result) const {

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesByResult.find(result.itemId());
    if (it != m_recipesByResult.end()) {
        return it->second;
    }
    return {};
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesForResult(
    const ItemStack& result) const {

    if (result.isEmpty()) {
        return {};
    }
    return getRecipesForResult(*result.getItem());
}

size_t RecipeManager::getRecipeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recipesById.size();
}

void RecipeManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recipesById.clear();
    m_recipesByType.clear();
    m_recipesByResult.clear();
}

void RecipeManager::forEachRecipe(
    std::function<void(const CraftingRecipe&)> callback) const {

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_recipesById) {
        callback(*pair.second);
    }
}

} // namespace crafting
} // namespace mr
