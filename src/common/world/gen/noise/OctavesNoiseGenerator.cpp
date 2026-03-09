#include "OctavesNoiseGenerator.hpp"
#include "../../../math/random/Random.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// OctavesNoiseGenerator 实现
// ============================================================================

OctavesNoiseGenerator::OctavesNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    math::Random rng(seed);
    initOctaves(rng);
}

OctavesNoiseGenerator::OctavesNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
    , m_maxOctave(maxOctave)
{
    initOctaves(rng);
}

void OctavesNoiseGenerator::initOctaves(math::IRandom& rng)
{
    const i32 octaveCount = m_maxOctave - m_minOctave + 1;
    m_octaves.resize(octaveCount);

    // 参考 MC 的实现
    // 创建第一个噪声生成器
    m_octaves[0] = std::make_unique<ImprovedNoiseGenerator>(rng);

    // 为其他倍频创建噪声生成器
    for (i32 i = 1; i < octaveCount; ++i) {
        // 跳过一些随机数来确保不同的噪声
        rng.skip(262);
        m_octaves[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算振幅
    // 参考 MC: field_227460_b_ 和 field_227461_c_
    m_amplitudeLow = std::pow(2.0, static_cast<f64>(-m_minOctave));
    m_amplitudeHigh = std::pow(2.0, static_cast<f64>(octaveCount - 1)) /
                      (std::pow(2.0, static_cast<f64>(octaveCount)) - 1.0);
}

f64 OctavesNoiseGenerator::noise(f64 x, f64 y, f64 z) const
{
    return getValue(x, y, z, 0.0, 0.0, false);
}

f64 OctavesNoiseGenerator::getValue(f64 x, f64 y, f64 z, f64 yScale, f64 yBound, bool fixY) const
{
    f64 result = 0.0;
    f64 freq = m_amplitudeLow;
    f64 amp = m_amplitudeHigh;

    for (size_t i = 0; i < m_octaves.size(); ++i) {
        const auto& octave = m_octaves[i];
        if (octave) {
            // 保持精度
            const f64 px = maintainPrecision(x * freq);
            const f64 py = maintainPrecision(y * freq);
            const f64 pz = maintainPrecision(z * freq);

            // 采样噪声
            const f64 sample = octave->noise(
                px,
                fixY ? -octave->yOffset() : py,
                pz,
                yScale * freq,
                yBound * freq
            );

            result += amp * sample;
        }

        freq *= 2.0;
        amp /= 2.0;
    }

    return result;
}

f64 OctavesNoiseGenerator::noiseAt(f64 x, f64 y, f64 z, f64 scale) const
{
    return getValue(x, y, 0.0, z, scale, false);
}

ImprovedNoiseGenerator* OctavesNoiseGenerator::getOctave(i32 octave)
{
    const i32 index = static_cast<i32>(m_octaves.size()) - 1 - octave;
    if (index >= 0 && index < static_cast<i32>(m_octaves.size())) {
        return m_octaves[index].get();
    }
    return nullptr;
}

const ImprovedNoiseGenerator* OctavesNoiseGenerator::getOctave(i32 octave) const
{
    const i32 index = static_cast<i32>(m_octaves.size()) - 1 - octave;
    if (index >= 0 && index < static_cast<i32>(m_octaves.size())) {
        return m_octaves[index].get();
    }
    return nullptr;
}

// ============================================================================
// PerlinNoiseGenerator 实现
// ============================================================================

PerlinNoiseGenerator::PerlinNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
{
    math::Random rng(seed);

    const i32 count = maxOctave - minOctave + 1;
    m_noiseLevels.resize(count);
    m_amplitudes.resize(count, 1.0);

    for (i32 i = 0; i < count; ++i) {
        m_noiseLevels[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算最大振幅
    m_maxAmplitude = 0.0;
    for (f64 amp : m_amplitudes) {
        m_maxAmplitude += amp;
    }
}

PerlinNoiseGenerator::PerlinNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave)
    : m_minOctave(minOctave)
{
    const i32 count = maxOctave - minOctave + 1;
    m_noiseLevels.resize(count);
    m_amplitudes.resize(count, 1.0);

    for (i32 i = 0; i < count; ++i) {
        m_noiseLevels[i] = std::make_unique<ImprovedNoiseGenerator>(rng);
    }

    // 计算最大振幅
    m_maxAmplitude = 0.0;
    for (f64 amp : m_amplitudes) {
        m_maxAmplitude += amp;
    }
}

f64 PerlinNoiseGenerator::noise(f64 x, f64 y, f64 z) const
{
    f64 result = 0.0;
    f64 freq = 1.0;
    f64 amp = 1.0;

    for (size_t i = 0; i < m_noiseLevels.size(); ++i) {
        if (m_noiseLevels[i]) {
            result += m_amplitudes[i] * m_noiseLevels[i]->noise(
                x * freq,
                y * freq,
                z * freq
            ) * amp;
        }
        freq *= 2.0;
        amp /= 2.0;
    }

    return result / m_maxAmplitude;
}

f64 PerlinNoiseGenerator::noise2D(f64 x, f64 z) const
{
    return noise(x, 0.0, z);
}

// ============================================================================
// SimplexNoiseGenerator 实现
// ============================================================================

// Simplex 噪声的梯度向量
constexpr f64 SIMPLEX_GRAD[12][3] = {
    {1.0, 1.0, 0.0}, {-1.0, 1.0, 0.0}, {1.0, -1.0, 0.0}, {-1.0, -1.0, 0.0},
    {1.0, 0.0, 1.0}, {-1.0, 0.0, 1.0}, {1.0, 0.0, -1.0}, {-1.0, 0.0, -1.0},
    {0.0, 1.0, 1.0}, {0.0, -1.0, 1.0}, {0.0, 1.0, -1.0}, {0.0, -1.0, -1.0}
};

// Simplex 斜切因子
// F2 = 0.5 * (sqrt(3.0) - 1.0) = 0.3660254037844386...
// G2 = (3.0 - sqrt(3.0)) / 6.0 = 0.2113248654051871...
constexpr f64 F2 = 0.36602540378443864676372317075294;
constexpr f64 G2 = 0.21132486540518711774542545986184;
constexpr f64 F3 = 1.0 / 3.0;
constexpr f64 G3 = 1.0 / 6.0;

SimplexNoiseGenerator::SimplexNoiseGenerator(u64 seed)
{
    math::Random rng(seed);
    initPermutation(rng);
}

SimplexNoiseGenerator::SimplexNoiseGenerator(math::IRandom& rng)
{
    initPermutation(rng);
}

void SimplexNoiseGenerator::initPermutation(math::IRandom& rng)
{
    // 初始化排列数组
    for (i32 i = 0; i < 256; ++i) {
        m_permutation[i] = static_cast<u8>(i);
    }

    // Fisher-Yates 洗牌
    for (i32 i = 0; i < 256; ++i) {
        const u32 j = static_cast<u32>(i) + static_cast<u32>(rng.nextInt(256 - i));
        std::swap(m_permutation[i], m_permutation[j]);
    }

    // 复制到工作数组
    for (i32 i = 0; i < 256; ++i) {
        m_p[i] = m_permutation[i];
        m_p[i + 256] = m_permutation[i];
    }

    // 设置随机偏移
    m_offset[0] = rng.nextDouble(0.0, 256.0);
    m_offset[1] = rng.nextDouble(0.0, 256.0);
    m_offset[2] = rng.nextDouble(0.0, 256.0);
}

i32 SimplexNoiseGenerator::fastFloor(f64 x)
{
    return static_cast<i32>(x > 0 ? x : x - 1);
}

f64 SimplexNoiseGenerator::noise(f64 x, f64 y, f64 z) const
{
    // 添加偏移
    x += m_offset[0];
    y += m_offset[1];
    z += m_offset[2];

    // 斜切输入空间以确定单元格
    const f64 s = (x + y + z) * F3;
    const i32 i = fastFloor(x + s);
    const i32 j = fastFloor(y + s);
    const i32 k = fastFloor(z + s);

    const f64 t = static_cast<f64>(i + j + k) * G3;
    const f64 X0 = static_cast<f64>(i) - t;
    const f64 Y0 = static_cast<f64>(j) - t;
    const f64 Z0 = static_cast<f64>(k) - t;

    const f64 x0 = x - X0;
    const f64 y0 = y - Y0;
    const f64 z0 = z - Z0;

    // 确定单纯形
    i32 i1, j1, k1;
    i32 i2, j2, k2;

    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        } else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        } else {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    } else {
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        } else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        } else {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    const f64 x1 = x0 - static_cast<f64>(i1) + G3;
    const f64 y1 = y0 - static_cast<f64>(j1) + G3;
    const f64 z1 = z0 - static_cast<f64>(k1) + G3;
    const f64 x2 = x0 - static_cast<f64>(i2) + 2.0 * G3;
    const f64 y2 = y0 - static_cast<f64>(j2) + 2.0 * G3;
    const f64 z2 = z0 - static_cast<f64>(k2) + 2.0 * G3;
    const f64 x3 = x0 - 1.0 + 3.0 * G3;
    const f64 y3 = y0 - 1.0 + 3.0 * G3;
    const f64 z3 = z0 - 1.0 + 3.0 * G3;

    // 哈希坐标
    const i32 ii = i & 255;
    const i32 jj = j & 255;
    const i32 kk = k & 255;

    // 计算贡献
    f64 n0 = 0.0, n1 = 0.0, n2 = 0.0, n3 = 0.0;

    f64 t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 >= 0.0) {
        t0 *= t0;
        const i32 gi0 = m_p[ii + m_p[jj + m_p[kk]]] % 12;
        n0 = t0 * t0 * (SIMPLEX_GRAD[gi0][0] * x0 + SIMPLEX_GRAD[gi0][1] * y0 + SIMPLEX_GRAD[gi0][2] * z0);
    }

    f64 t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 >= 0.0) {
        t1 *= t1;
        const i32 gi1 = m_p[ii + i1 + m_p[jj + j1 + m_p[kk + k1]]] % 12;
        n1 = t1 * t1 * (SIMPLEX_GRAD[gi1][0] * x1 + SIMPLEX_GRAD[gi1][1] * y1 + SIMPLEX_GRAD[gi1][2] * z1);
    }

    f64 t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 >= 0.0) {
        t2 *= t2;
        const i32 gi2 = m_p[ii + i2 + m_p[jj + j2 + m_p[kk + k2]]] % 12;
        n2 = t2 * t2 * (SIMPLEX_GRAD[gi2][0] * x2 + SIMPLEX_GRAD[gi2][1] * y2 + SIMPLEX_GRAD[gi2][2] * z2);
    }

    f64 t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 >= 0.0) {
        t3 *= t3;
        const i32 gi3 = m_p[ii + 1 + m_p[jj + 1 + m_p[kk + 1]]] % 12;
        n3 = t3 * t3 * (SIMPLEX_GRAD[gi3][0] * x3 + SIMPLEX_GRAD[gi3][1] * y3 + SIMPLEX_GRAD[gi3][2] * z3);
    }

    // 缩放到 [-1, 1]
    return 32.0 * (n0 + n1 + n2 + n3);
}

f64 SimplexNoiseGenerator::noise2D(f64 x, f64 z) const
{
    return noise(x, 0.0, z);
}

f64 SimplexNoiseGenerator::sampleEndHeight(i32 x, i32 z) const
{
    // 参考 MC EndBiomeProvider.func_235317_a_
    // 计算末地维度的高度偏移
    const i32 i = x / 2;
    const i32 j = z / 2;
    const i32 k = x % 2;
    const i32 l = z % 2;

    // 使用 2D Simplex 噪声
    constexpr f64 SCALE = 0.05;
    const f64 sample = noise2D(static_cast<f64>(i) * SCALE, static_cast<f64>(j) * SCALE);

    // 计算高度
    constexpr f64 BASE_HEIGHT = 8.0;
    return static_cast<f64>((sample + 1.0) * 0.5 * BASE_HEIGHT);
}

f64 SimplexNoiseGenerator::grad(i32 hash, f64 x, f64 y, f64 z) const
{
    const i32 h = hash & 11;
    return SIMPLEX_GRAD[h][0] * x + SIMPLEX_GRAD[h][1] * y + SIMPLEX_GRAD[h][2] * z;
}

} // namespace mr
