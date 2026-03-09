#pragma once

#include "IRandom.hpp"
#include <random>

namespace mr::math {

/**
 * @brief Mersenne Twister 随机数生成器
 *
 * 基于 std::mt19937_64 实现，提供高质量的随机数生成。
 * 这是默认的随机数算法，具有以下特点：
 * - 周期长：2^19937 - 1
 * - 质量高：通过了所有统计测试
 * - 兼容性好：与 C++ 标准库完全兼容
 * - 状态大：2.5KB
 *
 * 使用方法：
 * @code
 * Mt19937Random rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 如果需要更小的状态或更高的性能，考虑使用 Xoroshiro128ppRandom
 */
class Mt19937Random : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     */
    explicit Mt19937Random(u64 seed = 0);

    /**
     * @brief 使用随机设备构造随机数生成器
     * @return 使用真随机种子初始化的生成器
     */
    [[nodiscard]] static Mt19937Random fromRandomDevice();

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

private:
    std::mt19937_64 m_engine;
};

} // namespace mr::math
