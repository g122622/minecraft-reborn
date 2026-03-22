#include "FortuneEnchantment.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace item {
namespace enchant {

// ============================================================================
// FortuneEnchantment 实现
// ============================================================================

i32 FortuneEnchantment::applyBonus(i32 baseCount, i32 level, math::Random& random) {
    if (level <= 0) {
        return baseCount;
    }

    // MC 1.16.5 Fortune 逻辑:
    // 每级时运都有 1/(level + 2) 的概率增加 1 个额外掉落
    // 最多增加 level 个
    i32 bonus = 0;
    for (i32 i = 0; i < level; ++i) {
        f32 chance = 1.0f / static_cast<f32>(level + 2);
        if (random.nextFloat() < chance) {
            ++bonus;
        }
    }

    return baseCount + bonus;
}

i32 FortuneEnchantment::applyUniformBonus(i32 level, math::Random& random) {
    if (level <= 0) {
        return 0;
    }

    // 均匀随机 0 到 level
    return random.nextInt(level + 1);
}

i32 FortuneEnchantment::applyOreDropBonus(i32 baseMin, i32 baseMax, i32 level, math::Random& random) {
    // 基础掉落
    i32 base = baseMin;
    if (baseMax > baseMin) {
        base = random.nextInt(baseMin, baseMax);
    }

    // 时运加成
    if (level > 0) {
        base += random.nextInt(level + 1);
    }

    return base;
}

} // namespace enchant
} // namespace item
} // namespace mc
