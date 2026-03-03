#include "MathUtils.hpp"

namespace mr::math {

// ============================================================================
// PerlinNoise 实现
// ============================================================================

PerlinNoise::PerlinNoise(u64 seed)
{
    Random rng(seed);

    // 初始化排列表
    for (i32 i = 0; i < PERMUTATION_SIZE; ++i) {
        m_permutation[i] = i;
    }

    // 打乱排列
    for (i32 i = PERMUTATION_SIZE - 1; i > 0; --i) {
        const i32 j = rng.nextInt(0, i);
        std::swap(m_permutation[i], m_permutation[j]);
    }

    // 复制到后半部分
    for (i32 i = 0; i < PERMUTATION_SIZE; ++i) {
        m_permutation[i + PERMUTATION_SIZE] = m_permutation[i];
    }
}

f32 PerlinNoise::fade(f32 t)
{
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

f32 PerlinNoise::grad(i32 hash, f32 x, f32 y, f32 z)
{
    // 将hash的低4位转换为梯度向量
    const i32 h = hash & 15;
    const f32 u = h < 8 ? x : y;
    const f32 v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

f32 PerlinNoise::noise(f32 x) const
{
    return noise(x, 0.0f, 0.0f);
}

f32 PerlinNoise::noise(f32 x, f32 y) const
{
    return noise(x, y, 0.0f);
}

f32 PerlinNoise::noise(f32 x, f32 y, f32 z) const
{
    // 找到单位立方体的坐标
    const i32 X = static_cast<i32>(std::floor(x)) & 255;
    const i32 Y = static_cast<i32>(std::floor(y)) & 255;
    const i32 Z = static_cast<i32>(std::floor(z)) & 255;

    // 相对坐标
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    // 计算fade曲线
    const f32 u = fade(x);
    const f32 v = fade(y);
    const f32 w = fade(z);

    // 哈希立方体8个顶点的坐标
    const i32 A = m_permutation[X] + Y;
    const i32 AA = m_permutation[A] + Z;
    const i32 AB = m_permutation[A + 1] + Z;
    const i32 B = m_permutation[X + 1] + Y;
    const i32 BA = m_permutation[B] + Z;
    const i32 BB = m_permutation[B + 1] + Z;

    // 混合梯度贡献
    return lerp(
        lerp(
            lerp(grad(m_permutation[AA], x, y, z),
                 grad(m_permutation[BA], x - 1, y, z), u),
            lerp(grad(m_permutation[AB], x, y - 1, z),
                 grad(m_permutation[BB], x - 1, y - 1, z), u),
            v),
        lerp(
            lerp(grad(m_permutation[AA + 1], x, y, z - 1),
                 grad(m_permutation[BA + 1], x - 1, y, z - 1), u),
            lerp(grad(m_permutation[AB + 1], x, y - 1, z - 1),
                 grad(m_permutation[BB + 1], x - 1, y - 1, z - 1), u),
            v),
        w);
}

f32 PerlinNoise::octaveNoise(f32 x, f32 y, i32 octaves, f32 persistence) const
{
    f32 total = 0.0f;
    f32 frequency = 1.0f;
    f32 amplitude = 1.0f;
    f32 maxValue = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

} // namespace mr::math
