#include "Noise.hpp"
#include <algorithm>
#include <cmath>

namespace mr {

// ============================================================================
// PerlinNoise 实现
// ============================================================================

PerlinNoise::PerlinNoise(u32 seed) {
    // 初始化排列表
    m_permutation.resize(512);

    // 创建基础排列表 (0-255)
    std::vector<u8> p(256);
    for (i32 i = 0; i < 256; ++i) {
        p[i] = static_cast<u8>(i);
    }

    // 使用种子打乱
    std::mt19937 gen(seed);
    std::shuffle(p.begin(), p.end(), gen);

    // 复制到512数组以避免溢出
    for (i32 i = 0; i < 256; ++i) {
        m_permutation[i] = p[i];
        m_permutation[256 + i] = p[i];
    }
}

f32 PerlinNoise::noise2D(f32 x, f32 z) const {
    // 应用频率
    x *= m_frequency;
    z *= m_frequency;

    // 计算单位立方体坐标
    i32 xi = static_cast<i32>(std::floor(x)) & 255;
    i32 zi = static_cast<i32>(std::floor(z)) & 255;

    // 相对位置
    f32 xf = x - std::floor(x);
    f32 zf = z - std::floor(z);

    // 淡入曲线
    f32 u = fade(xf);
    f32 v = fade(zf);

    // 哈希四个角的梯度
    i32 aa = m_permutation[m_permutation[xi] + zi];
    i32 ab = m_permutation[m_permutation[xi] + zi + 1];
    i32 ba = m_permutation[m_permutation[xi + 1] + zi];
    i32 bb = m_permutation[m_permutation[xi + 1] + zi + 1];

    // 计算梯度并插值
    f32 x1 = lerp(grad(aa, xf, zf), grad(ba, xf - 1, zf), u);
    f32 x2 = lerp(grad(ab, xf, zf - 1), grad(bb, xf - 1, zf - 1), u);

    return (lerp(x1, x2, v) + 1.0f) * 0.5f * m_amplitude;
}

f32 PerlinNoise::noise3D(f32 x, f32 y, f32 z) const {
    x *= m_frequency;
    y *= m_frequency;
    z *= m_frequency;

    i32 xi = static_cast<i32>(std::floor(x)) & 255;
    i32 yi = static_cast<i32>(std::floor(y)) & 255;
    i32 zi = static_cast<i32>(std::floor(z)) & 255;

    f32 xf = x - std::floor(x);
    f32 yf = y - std::floor(y);
    f32 zf = z - std::floor(z);

    f32 u = fade(xf);
    f32 v = fade(yf);
    f32 w = fade(zf);

    i32 aaa = m_permutation[m_permutation[m_permutation[xi] + yi] + zi];
    i32 aba = m_permutation[m_permutation[m_permutation[xi] + yi + 1] + zi];
    i32 aab = m_permutation[m_permutation[m_permutation[xi] + yi] + zi + 1];
    i32 abb = m_permutation[m_permutation[m_permutation[xi] + yi + 1] + zi + 1];
    i32 baa = m_permutation[m_permutation[m_permutation[xi + 1] + yi] + zi];
    i32 bba = m_permutation[m_permutation[m_permutation[xi + 1] + yi + 1] + zi];
    i32 bab = m_permutation[m_permutation[m_permutation[xi + 1] + yi] + zi + 1];
    i32 bbb = m_permutation[m_permutation[m_permutation[xi + 1] + yi + 1] + zi + 1];

    f32 x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf - 1, yf, zf), u);
    f32 x2 = lerp(grad(aba, xf, yf - 1, zf), grad(bba, xf - 1, yf - 1, zf), u);
    f32 y1 = lerp(x1, x2, v);

    x1 = lerp(grad(aab, xf, yf, zf - 1), grad(bab, xf - 1, yf, zf - 1), u);
    x2 = lerp(grad(abb, xf, yf - 1, zf - 1), grad(bbb, xf - 1, yf - 1, zf - 1), u);
    f32 y2 = lerp(x1, x2, v);

    return (lerp(y1, y2, w) + 1.0f) * 0.5f * m_amplitude;
}

f32 PerlinNoise::octave2D(f32 x, f32 z, i32 octaves, f32 persistence) const {
    f32 total = 0.0f;
    f32 frequency = m_frequency;
    f32 amplitude = m_amplitude;
    f32 maxValue = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        total += noise2D(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

f32 PerlinNoise::octave3D(f32 x, f32 y, f32 z, i32 octaves, f32 persistence) const {
    f32 total = 0.0f;
    f32 frequency = m_frequency;
    f32 amplitude = m_amplitude;
    f32 maxValue = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        total += noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

f32 PerlinNoise::grad(i32 hash, f32 x, f32 z) const {
    // 8个梯度方向
    i32 h = hash & 7;
    f32 u = h < 4 ? x : z;
    f32 v = h < 4 ? z : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

f32 PerlinNoise::grad(i32 hash, f32 x, f32 y, f32 z) const {
    // 12个梯度方向
    i32 h = hash & 15;
    f32 u = h < 8 ? x : y;
    f32 v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

f32 PerlinNoise::fade(f32 t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

f32 PerlinNoise::lerp(f32 a, f32 b, f32 t) {
    return a + t * (b - a);
}

// ============================================================================
// SimplexNoise 实现
// ============================================================================

namespace {
    // 2D简化的梯度向量
    constexpr f32 GRAD3[12][3] = {
        {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
        {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
        {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
    };

    // 斜切因子
    constexpr f32 F2 = 0.3660254037844386f;  // (sqrt(3) - 1) / 2
    constexpr f32 G2 = 0.21132486540518713f; // (3 - sqrt(3)) / 6
}

SimplexNoise::SimplexNoise(u32 seed) {
    m_perm.resize(512);

    std::vector<u8> p(256);
    for (i32 i = 0; i < 256; ++i) {
        p[i] = static_cast<u8>(i);
    }

    std::mt19937 gen(seed);
    std::shuffle(p.begin(), p.end(), gen);

    for (i32 i = 0; i < 256; ++i) {
        m_perm[i] = p[i];
        m_perm[256 + i] = p[i];
    }
}

i32 SimplexNoise::fastFloor(f32 x) {
    return x > 0 ? static_cast<i32>(x) : static_cast<i32>(x) - 1;
}

f32 SimplexNoise::noise2D(f32 x, f32 z) const {
    x *= m_frequency;
    z *= m_frequency;

    // 斜切坐标
    f32 s = (x + z) * F2;
    i32 i = fastFloor(x + s);
    i32 j = fastFloor(z + s);

    // 非斜切坐标
    f32 t = static_cast<f32>(i + j) * G2;
    f32 x0 = x - (static_cast<f32>(i) - t);
    f32 z0 = z - (static_cast<f32>(j) - t);

    // 确定简单三角形
    i32 i1, j1;
    if (x0 > z0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    // 另外两个角的坐标
    f32 x1 = x0 - static_cast<f32>(i1) + G2;
    f32 z1 = z0 - static_cast<f32>(j1) + G2;
    f32 x2 = x0 - 1.0f + 2.0f * G2;
    f32 z2 = z0 - 1.0f + 2.0f * G2;

    // 哈希
    i32 ii = i & 255;
    i32 jj = j & 255;

    // 贡献
    f32 n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

    f32 t0 = 0.5f - x0 * x0 - z0 * z0;
    if (t0 >= 0) {
        i32 gi0 = m_perm[ii + m_perm[jj]] % 12;
        t0 *= t0;
        n0 = t0 * t0 * (GRAD3[gi0][0] * x0 + GRAD3[gi0][1] * z0);
    }

    f32 t1 = 0.5f - x1 * x1 - z1 * z1;
    if (t1 >= 0) {
        i32 gi1 = m_perm[ii + i1 + m_perm[jj + j1]] % 12;
        t1 *= t1;
        n1 = t1 * t1 * (GRAD3[gi1][0] * x1 + GRAD3[gi1][1] * z1);
    }

    f32 t2 = 0.5f - x2 * x2 - z2 * z2;
    if (t2 >= 0) {
        i32 gi2 = m_perm[ii + 1 + m_perm[jj + 1]] % 12;
        t2 *= t2;
        n2 = t2 * t2 * (GRAD3[gi2][0] * x2 + GRAD3[gi2][1] * z2);
    }

    // 结果在[-1, 1]范围，缩放到[0, 1]
    return (35.0f * (n0 + n1 + n2) + 1.0f) * 0.5f * m_amplitude;
}

f32 SimplexNoise::noise3D(f32 x, f32 y, f32 z) const {
    // 简化实现，使用Perlin的3D噪声
    // 完整的3D Simplex更复杂，这里用简化版
    x *= m_frequency;
    y *= m_frequency;
    z *= m_frequency;

    i32 xi = static_cast<i32>(std::floor(x)) & 255;
    i32 yi = static_cast<i32>(std::floor(y)) & 255;
    i32 zi = static_cast<i32>(std::floor(z)) & 255;

    f32 xf = x - std::floor(x);
    f32 yf = y - std::floor(y);
    f32 zf = z - std::floor(z);

    f32 u = xf * xf * xf * (xf * (xf * 6.0f - 15.0f) + 10.0f);
    f32 v = yf * yf * yf * (yf * (yf * 6.0f - 15.0f) + 10.0f);
    f32 w = zf * zf * zf * (zf * (zf * 6.0f - 15.0f) + 10.0f);

    i32 aaa = m_perm[m_perm[m_perm[xi] + yi] + zi];
    i32 aba = m_perm[m_perm[m_perm[xi] + yi + 1] + zi];
    i32 aab = m_perm[m_perm[m_perm[xi] + yi] + zi + 1];
    i32 abb = m_perm[m_perm[m_perm[xi] + yi + 1] + zi + 1];
    i32 baa = m_perm[m_perm[m_perm[xi + 1] + yi] + zi];
    i32 bba = m_perm[m_perm[m_perm[xi + 1] + yi + 1] + zi];
    i32 bab = m_perm[m_perm[m_perm[xi + 1] + yi] + zi + 1];
    i32 bbb = m_perm[m_perm[m_perm[xi + 1] + yi + 1] + zi + 1];

    auto lerp = [](f32 a, f32 b, f32 t) { return a + t * (b - a); };
    auto grad = [&](i32 hash, f32 gx, f32 gy, f32 gz) -> f32 {
        i32 h = hash & 15;
        f32 gu = h < 8 ? gx : gy;
        f32 gv = h < 4 ? gy : (h == 12 || h == 14 ? gx : gz);
        return ((h & 1) ? -gu : gu) + ((h & 2) ? -gv : gv);
    };

    f32 x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf - 1, yf, zf), u);
    f32 x2 = lerp(grad(aba, xf, yf - 1, zf), grad(bba, xf - 1, yf - 1, zf), u);
    f32 y1 = lerp(x1, x2, v);

    x1 = lerp(grad(aab, xf, yf, zf - 1), grad(bab, xf - 1, yf, zf - 1), u);
    x2 = lerp(grad(abb, xf, yf - 1, zf - 1), grad(bbb, xf - 1, yf - 1, zf - 1), u);
    f32 y2 = lerp(x1, x2, v);

    return (lerp(y1, y2, w) + 1.0f) * 0.5f * m_amplitude;
}

f32 SimplexNoise::octave2D(f32 x, f32 z, i32 octaves, f32 persistence) const {
    f32 total = 0.0f;
    f32 frequency = m_frequency;
    f32 amplitude = m_amplitude;
    f32 maxValue = 0.0f;

    for (i32 i = 0; i < octaves; ++i) {
        total += noise2D(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

// ============================================================================
// 噪声工具函数实现
// ============================================================================

namespace noise {

void generateHeightMap(
    std::vector<f32>& heightMap,
    i32 width, i32 height,
    f32 offsetX, f32 offsetZ,
    const PerlinNoise& perlin,
    i32 octaves,
    f32 persistence
) {
    heightMap.resize(static_cast<size_t>(width) * height);

    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            f32 nx = static_cast<f32>(x) + offsetX;
            f32 nz = static_cast<f32>(z) + offsetZ;

            f32 value = perlin.octave2D(nx, nz, octaves, persistence);
            heightMap[z * width + x] = value;
        }
    }
}

void generateDensityMap(
    std::vector<f32>& densityMap,
    i32 width, i32 height, i32 depth,
    f32 offsetX, f32 offsetY, f32 offsetZ,
    const PerlinNoise& perlin,
    i32 octaves
) {
    densityMap.resize(static_cast<size_t>(width) * height * depth);

    for (i32 z = 0; z < depth; ++z) {
        for (i32 y = 0; y < height; ++y) {
            for (i32 x = 0; x < width; ++x) {
                f32 nx = static_cast<f32>(x) + offsetX;
                f32 ny = static_cast<f32>(y) + offsetY;
                f32 nz = static_cast<f32>(z) + offsetZ;

                f32 value = perlin.octave3D(nx, ny, nz, octaves);
                densityMap[z * width * height + y * width + x] = value;
            }
        }
    }
}

f32 bilinearInterpolation(
    f32 x, f32 z,
    const std::vector<f32>& values,
    i32 width, i32 height
) {
    i32 x0 = static_cast<i32>(x);
    i32 z0 = static_cast<i32>(z);
    i32 x1 = (x0 + 1) % width;
    i32 z1 = (z0 + 1) % height;

    f32 fx = x - static_cast<f32>(x0);
    f32 fz = z - static_cast<f32>(z0);

    f32 v00 = values[z0 * width + x0];
    f32 v10 = values[z0 * width + x1];
    f32 v01 = values[z1 * width + x0];
    f32 v11 = values[z1 * width + x1];

    f32 tx = v00 * (1.0f - fx) + v10 * fx;
    f32 bx = v01 * (1.0f - fx) + v11 * fx;

    return tx * (1.0f - fz) + bx * fz;
}

f32 trilinearInterpolation(
    f32 x, f32 y, f32 z,
    const std::vector<f32>& values,
    i32 width, i32 height, i32 depth
) {
    i32 x0 = static_cast<i32>(x);
    i32 y0 = static_cast<i32>(y);
    i32 z0 = static_cast<i32>(z);
    i32 x1 = (x0 + 1) % width;
    i32 y1 = (y0 + 1) % height;
    i32 z1 = (z0 + 1) % depth;

    f32 fx = x - static_cast<f32>(x0);
    f32 fy = y - static_cast<f32>(y0);
    f32 fz = z - static_cast<f32>(z0);

    auto get = [&](i32 xi, i32 yi, i32 zi) -> f32 {
        return values[zi * width * height + yi * width + xi];
    };

    f32 c000 = get(x0, y0, z0);
    f32 c100 = get(x1, y0, z0);
    f32 c010 = get(x0, y1, z0);
    f32 c110 = get(x1, y1, z0);
    f32 c001 = get(x0, y0, z1);
    f32 c101 = get(x1, y0, z1);
    f32 c011 = get(x0, y1, z1);
    f32 c111 = get(x1, y1, z1);

    f32 tx0 = c000 * (1.0f - fx) + c100 * fx;
    f32 tx1 = c010 * (1.0f - fx) + c110 * fx;
    f32 txy0 = tx0 * (1.0f - fy) + tx1 * fy;

    f32 bx0 = c001 * (1.0f - fx) + c101 * fx;
    f32 bx1 = c011 * (1.0f - fx) + c111 * fx;
    f32 bxy = bx0 * (1.0f - fy) + bx1 * fy;

    return txy0 * (1.0f - fz) + bxy * fz;
}

} // namespace noise

} // namespace mr
