#include "ImprovedNoiseGenerator.hpp"
#include "../../../math/random/Random.hpp"
#include <cmath>
#include <algorithm>

namespace mr {

// ============================================================================
// 构造函数
// ============================================================================

ImprovedNoiseGenerator::ImprovedNoiseGenerator(u64 seed)
{
    math::Random rng(seed);
    initPermutation(rng);
}

ImprovedNoiseGenerator::ImprovedNoiseGenerator(math::IRandom& rng)
{
    initPermutation(rng);
}

// ============================================================================
// 初始化
// ============================================================================

void ImprovedNoiseGenerator::initPermutation(math::IRandom& rng)
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

    // 设置随机偏移（参考 MC）
    m_xOffset = rng.nextDouble(0.0, 256.0);
    m_yOffset = rng.nextDouble(0.0, 256.0);
    m_zOffset = rng.nextDouble(0.0, 256.0);
}

// ============================================================================
// 噪声采样
// ============================================================================

f64 ImprovedNoiseGenerator::noise(f64 x, f64 y, f64 z) const
{
    // 添加偏移
    x += m_xOffset;
    y += m_yOffset;
    z += m_zOffset;

    // 找到单位立方体的整数坐标
    const i32 ix = static_cast<i32>(std::floor(x));
    const i32 iy = static_cast<i32>(std::floor(y));
    const i32 iz = static_cast<i32>(std::floor(z));

    // 计算立方体内的小数坐标
    const f64 dx = x - static_cast<f64>(ix);
    const f64 dy = y - static_cast<f64>(iy);
    const f64 dz = z - static_cast<f64>(iz);

    // 计算 fade 曲线值
    const f64 fx = fade(dx);
    const f64 fy = fade(dy);
    const f64 fz = fade(dz);

    return noiseRaw(ix, iy, iz, dx, dy, dz, fx, fy, fz);
}

f64 ImprovedNoiseGenerator::noise(f64 x, f64 y, f64 z, f64 yScale, f64 yBound) const
{
    // 添加偏移
    x += m_xOffset;
    y += m_yOffset;
    z += m_zOffset;

    // 找到单位立方体的整数坐标
    const i32 ix = static_cast<i32>(std::floor(x));
    const i32 iy = static_cast<i32>(std::floor(y));
    const i32 iz = static_cast<i32>(std::floor(z));

    // 计算立方体内的小数坐标
    f64 dy = y - static_cast<f64>(iy);

    // Y 轴缩放（参考 MC）
    if (yScale != 0.0) {
        const f64 clampedY = std::min(yBound, dy);
        dy = std::floor(clampedY / yScale) * yScale;
    }

    const f64 dx = x - static_cast<f64>(ix);
    const f64 dz = z - static_cast<f64>(iz);

    // 计算 fade 曲线值
    const f64 fx = fade(dx);
    const f64 fy = fade(dy);
    const f64 fz = fade(dz);

    return noiseRaw(ix, iy, iz, dx, dy, dz, fx, fy, fz);
}

f64 ImprovedNoiseGenerator::noiseRaw(i32 x, i32 y, i32 z,
                                      f64 deltaX, f64 deltaY, f64 deltaZ,
                                      f64 fadeX, f64 fadeY, f64 fadeZ) const
{
    // 哈希索引
    const i32 i0 = getPermut(x);
    const i32 i1 = getPermut(x + 1);

    const i32 j0 = getPermut(i0 + y);
    const i32 j1 = getPermut(i1 + y);
    const i32 j2 = getPermut(i0 + y + 1);
    const i32 j3 = getPermut(i1 + y + 1);

    // 8 个角的梯度值
    const f64 n000 = grad(getPermut(j0 + z),     deltaX,     deltaY,     deltaZ);
    const f64 n100 = grad(getPermut(j1 + z),     deltaX - 1, deltaY,     deltaZ);
    const f64 n010 = grad(getPermut(j2 + z),     deltaX,     deltaY - 1, deltaZ);
    const f64 n110 = grad(getPermut(j3 + z),     deltaX - 1, deltaY - 1, deltaZ);
    const f64 n001 = grad(getPermut(j0 + z + 1), deltaX,     deltaY,     deltaZ - 1);
    const f64 n101 = grad(getPermut(j1 + z + 1), deltaX - 1, deltaY,     deltaZ - 1);
    const f64 n011 = grad(getPermut(j2 + z + 1), deltaX,     deltaY - 1, deltaZ - 1);
    const f64 n111 = grad(getPermut(j3 + z + 1), deltaX - 1, deltaY - 1, deltaZ - 1);

    // 三线性插值
    return lerp3(fadeX, fadeY, fadeZ, n000, n100, n010, n110, n001, n101, n011, n111);
}

// ============================================================================
// 梯度计算
// ============================================================================

f64 ImprovedNoiseGenerator::grad(i32 hash, f64 x, f64 y, f64 z)
{
    const i32 h = hash & 15;
    const f64* gradVec = PERLIN_GRADIENTS[h];
    return gradVec[0] * x + gradVec[1] * y + gradVec[2] * z;
}

// ============================================================================
// 插值函数
// ============================================================================

f64 ImprovedNoiseGenerator::lerp3(f64 t1, f64 t2, f64 t3,
                                   f64 v0, f64 v1, f64 v2, f64 v3,
                                   f64 v4, f64 v5, f64 v6, f64 v7)
{
    // 沿 X 轴插值
    const f64 i0 = lerp(v0, v1, t1);
    const f64 i1 = lerp(v2, v3, t1);
    const f64 i2 = lerp(v4, v5, t1);
    const f64 i3 = lerp(v6, v7, t1);

    // 沿 Z 轴插值
    const f64 j0 = lerp(i0, i1, t2);
    const f64 j1 = lerp(i2, i3, t2);

    // 沿 Y 轴插值
    return lerp(j0, j1, t3);
}

} // namespace mr
