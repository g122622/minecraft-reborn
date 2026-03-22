#pragma once

#include "IRandom.hpp"

namespace mc::math {

/**
 * @brief 线性同余随机数生成器
 *
 * 最简单、最小的随机数生成器。
 * 具有以下特点：
 * - 周期：约 2^64
 * - 状态极小：仅 8 字节
 * - 速度极快：仅一次乘法和一次加法
 * - 质量一般：统计特性不如其他算法
 *
 * 使用方法：
 * @code
 * LcgRandom rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 适合对随机质量要求不高、需要极小状态的场景
 * @note 使用 MMIX 参数 (Knuth)
 */
class LcgRandom : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     */
    explicit LcgRandom(u64 seed = 0);

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

private:
    u64 m_state;

    // MMIX 参数 (Knuth)
    static constexpr u64 A = 6364136223846793005ULL;
    static constexpr u64 C = 1442695040888963407ULL;
};

} // namespace mc::math
