#include "RandomRanges.hpp"

namespace mc {
namespace loot {

i32 BinomialRange::generateInt(math::Random& random) const {
    // 使用二项分布生成随机值
    // 进行n次试验，每次有p的概率成功
    i32 successes = 0;
    for (i32 i = 0; i < m_n; ++i) {
        if (random.nextFloat() < m_p) {
            ++successes;
        }
    }
    return successes;
}

} // namespace loot
} // namespace mc
