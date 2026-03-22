#include "EnchantmentRegistry.hpp"
#include "enchantments/FortuneEnchantment.hpp"
#include "enchantments/SilkTouchEnchantment.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace item {
namespace enchant {

// 静态成员定义
std::unordered_map<String, std::unique_ptr<Enchantment>> EnchantmentRegistry::s_enchantments;
bool EnchantmentRegistry::s_initialized = false;
std::mutex EnchantmentRegistry::s_mutex;

// ============================================================================
// EnchantmentRegistry 实现
// ============================================================================

void EnchantmentRegistry::initialize() {
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_initialized) {
        spdlog::warn("EnchantmentRegistry already initialized");
        return;
    }

    spdlog::info("Initializing enchantment registry...");

    // 注册原版附魔 - 内部版本不加锁
    // 工具附魔
    registerEnchantmentInternal(std::make_unique<FortuneEnchantment>());
    registerEnchantmentInternal(std::make_unique<SilkTouchEnchantment>());

    // TODO: 注册更多附魔
    // 武器附魔: Sharpness, Smite, Bane of Arthropods, etc.
    // 护甲附魔: Protection, Fire Protection, etc.
    // 特殊附魔: Mending, Unbreaking, etc.

    s_initialized = true;
    spdlog::info("Registered {} enchantments", s_enchantments.size());
}

bool EnchantmentRegistry::registerEnchantment(std::unique_ptr<Enchantment> enchantment) {
    if (!enchantment) {
        spdlog::error("Cannot register null enchantment");
        return false;
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    return registerEnchantmentInternal(std::move(enchantment));
}

bool EnchantmentRegistry::registerEnchantmentInternal(std::unique_ptr<Enchantment> enchantment) {
    // 注意：调用者必须持有 s_mutex 锁
    if (!enchantment) {
        return false;
    }

    String id = enchantment->id();

    if (s_enchantments.find(id) != s_enchantments.end()) {
        spdlog::warn("Enchantment {} already registered", id);
        return false;
    }

    spdlog::debug("Registering enchantment: {}", id);
    s_enchantments[id] = std::move(enchantment);
    return true;
}

const Enchantment* EnchantmentRegistry::get(const String& id) {
    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = s_enchantments.find(id);
    if (it != s_enchantments.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool EnchantmentRegistry::has(const String& id) {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_enchantments.find(id) != s_enchantments.end();
}

const std::unordered_map<String, std::unique_ptr<Enchantment>>& EnchantmentRegistry::all() {
    return s_enchantments;
}

std::vector<const Enchantment*> EnchantmentRegistry::getByType(EnchantmentType type) {
    std::lock_guard<std::mutex> lock(s_mutex);

    std::vector<const Enchantment*> result;
    for (const auto& [id, enchantment] : s_enchantments) {
        if (enchantment->type() == type) {
            result.push_back(enchantment.get());
        }
    }
    return result;
}

std::vector<const Enchantment*> EnchantmentRegistry::getAvailableForItem(u32 itemType) {
    std::lock_guard<std::mutex> lock(s_mutex);

    std::vector<const Enchantment*> result;
    for (const auto& [id, enchantment] : s_enchantments) {
        if (enchantment->canApplyTo(itemType)) {
            result.push_back(enchantment.get());
        }
    }
    return result;
}

void EnchantmentRegistry::clear() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_enchantments.clear();
    s_initialized = false;
}

bool EnchantmentRegistry::isInitialized() {
    return s_initialized;
}

} // namespace enchant
} // namespace item
} // namespace mc
