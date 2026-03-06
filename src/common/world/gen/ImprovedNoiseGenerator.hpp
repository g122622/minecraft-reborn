#pragma once

#include "../../core/Types.hpp"
#include <array>
#include <random>

namespace mr {

/**
 * @brief 噪声生成器接口
 */
class INoiseGenerator {
public:
    virtual ~INoiseGenerator() = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] virtual f64 noise(f64 x, f64 y, f64 z) const = 0;

    /**
     * @brief 采样 2D 噪声值
     */
    [[nodiscard]] virtual f64 noise2D(f64 x, f64 z) const {
        return noise(x, 0.0, z);
    }
};

/**
 * @brief 改进的 Perlin 噪声生成器
 *
 * 参考 MC ImprovedNoiseGenerator，实现标准的 3D Perlin 噪声。
 *
 * 使用方法：
 * @code
 * ImprovedNoiseGenerator noise(seed);
 * f64 value = noise.noise(x, y, z);
 * @endcode
 *
 * @note 噪声值范围约为 [-1, 1]
 */
class ImprovedNoiseGenerator : public INoiseGenerator {
public:
    /**
     * @brief 使用种子构造噪声生成器
     * @param seed 随机种子
     */
    explicit ImprovedNoiseGenerator(u64 seed);

    /**
     * @brief 使用随机引擎构造
     * @param rng 随机数引擎
     */
    explicit ImprovedNoiseGenerator(std::mt19937_64& rng);

    ~ImprovedNoiseGenerator() override = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f64 noise(f64 x, f64 y, f64 z) const override;

    /**
     * @brief 采样 3D 噪声值（带 Y 轴缩放）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param yScale Y 轴缩放因子
     * @param yBound Y 轴边界
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f64 noise(f64 x, f64 y, f64 z, f64 yScale, f64 yBound) const;

    /**
     * @brief 原始采样（使用整数坐标和偏移）
     *
     * 参考 MC 的 func_215459_a 方法
     */
    [[nodiscard]] f64 noiseRaw(i32 x, i32 y, i32 z,
                                f64 deltaX, f64 deltaY, f64 deltaZ,
                                f64 fadeX, f64 fadeY, f64 fadeZ) const;

    // 坐标偏移（参考 MC 的公开字段）
    [[nodiscard]] f64 xOffset() const { return m_xOffset; }
    [[nodiscard]] f64 yOffset() const { return m_yOffset; }
    [[nodiscard]] f64 zOffset() const { return m_zOffset; }

private:
    // 排列表（256 字节，复制一份用于快速查找）
    std::array<u8, 256> m_permutation;
    // 工作数组（用于避免每次调用时的临时分配）
    mutable std::array<u8, 512> m_p;

    // 坐标偏移
    f64 m_xOffset = 0.0;
    f64 m_yOffset = 0.0;
    f64 m_zOffset = 0.0;

    /**
     * @brief 初始化排列数组
     */
    void initPermutation(std::mt19937_64& rng);

    /**
     * @brief 获取排列值
     */
    [[nodiscard]] u8 getPermut(i32 index) const {
        return m_p[index & 255];
    }

    /**
     * @brief 梯度计算
     */
    [[nodiscard]] static f64 grad(i32 hash, f64 x, f64 y, f64 z);

    /**
     * @brief 平滑插值（Perlin 的 fade 函数）
     */
    [[nodiscard]] static f64 fade(f64 t) {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    /**
     * @brief 线性插值
     */
    [[nodiscard]] static f64 lerp(f64 a, f64 b, f64 t) {
        return a + t * (b - a);
    }

    /**
     * @brief 3D 线性插值
     */
    [[nodiscard]] static f64 lerp3(f64 t1, f64 t2, f64 t3,
                                    f64 v0, f64 v1, f64 v2, f64 v3,
                                    f64 v4, f64 v5, f64 v6, f64 v7);
};

// ============================================================================
// 梯度向量表 (参考 SimplexNoiseGenerator.GRADS)
// ============================================================================

// 梯度向量（用于 Perlin 噪声）
// 格式: {x, y, z}
constexpr f64 PERLIN_GRADIENTS[16][3] = {
    { 1.0,  1.0,  0.0}, {-1.0,  1.0,  0.0}, { 1.0, -1.0,  0.0}, {-1.0, -1.0,  0.0},
    { 1.0,  0.0,  1.0}, {-1.0,  0.0,  1.0}, { 1.0,  0.0, -1.0}, {-1.0,  0.0, -1.0},
    { 0.0,  1.0,  1.0}, { 0.0, -1.0,  1.0}, { 0.0,  1.0, -1.0}, { 0.0, -1.0, -1.0},
    { 1.0,  1.0,  0.0}, {-1.0,  1.0,  0.0}, { 0.0, -1.0,  1.0}, { 0.0,  1.0, -1.0}
};

} // namespace mr
