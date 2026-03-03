#pragma once

#include "../core/Types.hpp"
#include <vector>
#include <random>
#include <cmath>

namespace mr {

// ============================================================================
// Perlin噪声生成器
// ============================================================================

class PerlinNoise {
public:
    // 使用种子构造
    explicit PerlinNoise(u32 seed = 0);

    // 2D噪声
    [[nodiscard]] f32 noise2D(f32 x, f32 z) const;

    // 3D噪声
    [[nodiscard]] f32 noise3D(f32 x, f32 y, f32 z) const;

    // 分形噪声 (多个八度叠加)
    [[nodiscard]] f32 octave2D(f32 x, f32 z, i32 octaves, f32 persistence = 0.5f) const;
    [[nodiscard]] f32 octave3D(f32 x, f32 y, f32 z, i32 octaves, f32 persistence = 0.5f) const;

    // 设置频率
    void setFrequency(f32 frequency) { m_frequency = frequency; }
    [[nodiscard]] f32 frequency() const { return m_frequency; }

    // 设置振幅
    void setAmplitude(f32 amplitude) { m_amplitude = amplitude; }
    [[nodiscard]] f32 amplitude() const { return m_amplitude; }

private:
    // 梯度向量
    [[nodiscard]] f32 grad(i32 hash, f32 x, f32 z) const;
    [[nodiscard]] f32 grad(i32 hash, f32 x, f32 y, f32 z) const;

    // 平滑插值
    [[nodiscard]] static f32 fade(f32 t);
    [[nodiscard]] static f32 lerp(f32 a, f32 b, f32 t);

    std::vector<u8> m_permutation;  // 排列表
    f32 m_frequency = 1.0f;
    f32 m_amplitude = 1.0f;
};

// ============================================================================
// 简化噪声生成器
// ============================================================================

class SimplexNoise {
public:
    explicit SimplexNoise(u32 seed = 0);

    [[nodiscard]] f32 noise2D(f32 x, f32 z) const;
    [[nodiscard]] f32 noise3D(f32 x, f32 y, f32 z) const;

    [[nodiscard]] f32 octave2D(f32 x, f32 z, i32 octaves, f32 persistence = 0.5f) const;

    void setFrequency(f32 frequency) { m_frequency = frequency; }
    void setAmplitude(f32 amplitude) { m_amplitude = amplitude; }

private:
    [[nodiscard]] static i32 fastFloor(f32 x);

    std::vector<u8> m_perm;
    f32 m_frequency = 1.0f;
    f32 m_amplitude = 1.0f;
};

// ============================================================================
// 噪声工具函数
// ============================================================================

namespace noise {

// 生成高度图
void generateHeightMap(
    std::vector<f32>& heightMap,
    i32 width, i32 height,
    f32 offsetX, f32 offsetZ,
    const PerlinNoise& noise,
    i32 octaves = 4,
    f32 persistence = 0.5f
);

// 生成3D密度图
void generateDensityMap(
    std::vector<f32>& densityMap,
    i32 width, i32 height, i32 depth,
    f32 offsetX, f32 offsetY, f32 offsetZ,
    const PerlinNoise& noise,
    i32 octaves = 3
);

// 双线性插值
[[nodiscard]] f32 bilinearInterpolation(
    f32 x, f32 z,
    const std::vector<f32>& values,
    i32 width, i32 height
);

// 三线性插值
[[nodiscard]] f32 trilinearInterpolation(
    f32 x, f32 y, f32 z,
    const std::vector<f32>& values,
    i32 width, i32 height, i32 depth
);

} // namespace noise

} // namespace mr
