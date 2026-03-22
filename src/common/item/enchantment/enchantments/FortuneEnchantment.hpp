#pragma once

#include "../Enchantment.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 时运附魔
 *
 * 增加方块掉落物的数量。
 * 参考 MC 1.16.5 FortuneEnchantment
 *
 * 效果:
 * - Fortune I: 33%概率掉落+1
 * - Fortune II: 25%概率掉落+1, 25%概率掉落+2
 * - Fortune III: 20%概率掉落+1, 20%概率掉落+2, 20%概率掉落+3
 *
 * 适用物品:
 * - 镐、铲、斧、锄
 *
 * 不兼容: 精准采集
 */
class FortuneEnchantment : public Enchantment {
public:
    FortuneEnchantment() = default;

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] String id() const override {
        return "minecraft:fortune";
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 3;  // Fortune I, II, III
    }

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Digger;
    }

    [[nodiscard]] EnchantmentRarity rarity() const override {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override {
        // 与精准采集互斥
        if (other.id() == "minecraft:silk_touch") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override {
        // 参考 MC 1.16.5: 15 + (level - 1) * 9
        return 15 + (level - 1) * 9;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override {
        return getMinCost(level) + 50;
    }

    // ========== 时运特有方法 ==========

    /**
     * @brief 计算时运加成后的掉落数量
     *
     * 参考 MC 1.16.5 Fortune 逻辑：
     * - 每级时运都有 1/(level + 2) 的概率增加 1 个额外掉落
     * - Fortune I: 33%概率+1
     * - Fortune II: 25%概率+1, 如果触发再25%概率+2
     * - Fortune III: 20%概率+1, 如果触发再20%概率+2, 如果再触发再20%概率+3
     *
     * @param baseCount 基础掉落数量
     * @param level 时运等级（0-3）
     * @param random 随机数生成器
     * @return 加成后的掉落数量
     */
    [[nodiscard]] static i32 applyBonus(i32 baseCount, i32 level, math::Random& random);

    /**
     * @brief 计算离散均匀分布的时运加成
     *
     * 用于煤矿石等: 0-level 范围内的均匀随机加成。
     * - Fortune I: 0-1
     * - Fortune II: 0-2
     * - Fortune III: 0-3
     *
     * @param level 时运等级
     * @param random 随机数生成器
     * @return 额外掉落数量
     */
    [[nodiscard]] static i32 applyUniformBonus(i32 level, math::Random& random);

    /**
     * @brief 计算 ORE_DROP 类型的时运加成
     *
     * 用于红石矿、青金石矿等。
     * 基础掉落 + random(0, level) 的额外掉落。
     *
     * @param baseMin 基础最小掉落
     * @param baseMax 基础最大掉落
     * @param level 时运等级
     * @param random 随机数生成器
     * @return 总掉落数量范围
     */
    [[nodiscard]] static i32 applyOreDropBonus(i32 baseMin, i32 baseMax, i32 level, math::Random& random);
};

} // namespace enchant
} // namespace item
} // namespace mc
