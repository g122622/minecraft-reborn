#include <gtest/gtest.h>
#include <cmath>

#include "common/math/Noise.hpp"
#include "common/math/MathUtils.hpp"

using namespace mr;

// ============================================================================
// SimplexNoise 测试
// ============================================================================

TEST(SimplexNoise, Basic2D) {
    SimplexNoise noise(12345);

    // 噪声值应在 [0, 1] 范围内
    for (int i = 0; i < 10; ++i) {
        f32 value = noise.noise2D(static_cast<f32>(i) * 0.1f, static_cast<f32>(i) * 0.2f);
        EXPECT_GE(value, 0.0f);
        EXPECT_LE(value, 1.0f);
    }
}

TEST(SimplexNoise, Consistency) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(12345);

    // 相同种子应产生相同结果
    EXPECT_FLOAT_EQ(noise1.noise2D(10.0f, 20.0f), noise2.noise2D(10.0f, 20.0f));
}

TEST(SimplexNoise, DifferentSeeds) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(54321);

    // 收集两个噪声生成器在不同位置的值
    int differences = 0;
    for (int i = 0; i < 100; ++i) {
        f32 x = static_cast<f32>(i) * 0.1f;
        f32 z = static_cast<f32>(i) * 0.15f;
        f32 val1 = noise1.noise2D(x, z);
        f32 val2 = noise2.noise2D(x, z);
        if (std::abs(val1 - val2) > 0.01f) {
            differences++;
        }
    }

    EXPECT_GT(differences, 10);
}

TEST(SimplexNoise, OctaveNoise) {
    SimplexNoise noise(12345);

    f32 octave = noise.octave2D(10.0f, 20.0f, 4, 0.5f);
    EXPECT_GE(octave, 0.0f);
    EXPECT_LE(octave, 1.0f);
}

TEST(SimplexNoise, FrequencyEffect) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(12345);

    noise1.setFrequency(0.01f);
    noise2.setFrequency(0.1f);

    // 不同频率应产生不同结果
    f32 val1 = noise1.noise2D(100.0f, 100.0f);
    f32 val2 = noise2.noise2D(100.0f, 100.0f);

    // 两个值应该不同（对于相同输入，不同频率）
    // 注意：这可能在某些情况下偶然相等，但不太可能
    bool different = std::abs(val1 - val2) > 0.001f;
    EXPECT_TRUE(different);
}

TEST(SimplexNoise, AmplitudeEffect) {
    SimplexNoise noise1(12345);
    SimplexNoise noise2(12345);

    noise1.setAmplitude(1.0f);
    noise2.setAmplitude(0.5f);

    f32 val1 = noise1.noise2D(50.0f, 50.0f);
    f32 val2 = noise2.noise2D(50.0f, 50.0f);

    // 振幅影响输出值
    EXPECT_NE(val1, val2);
}

// ============================================================================
// Noise工具函数测试
// ============================================================================

TEST(NoiseUtils, GenerateHeightMap) {
    PerlinNoise noise(12345);
    std::vector<f32> heightMap;

    noise::generateHeightMap(heightMap, 16, 16, 0.0f, 0.0f, noise, 4, 0.5f);

    EXPECT_EQ(heightMap.size(), 256u);

    // 所有值应在 [0, 1] 范围内
    for (f32 val : heightMap) {
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
}

TEST(NoiseUtils, GenerateDensityMap) {
    PerlinNoise noise(12345);
    std::vector<f32> densityMap;

    noise::generateDensityMap(densityMap, 8, 8, 8, 0.0f, 0.0f, 0.0f, noise, 2);

    EXPECT_EQ(densityMap.size(), 512u); // 8*8*8 = 512

    // 所有值应在 [0, 1] 范围内
    for (f32 val : densityMap) {
        EXPECT_GE(val, 0.0f);
        EXPECT_LE(val, 1.0f);
    }
}

TEST(NoiseUtils, BilinearInterpolation) {
    std::vector<f32> values = {
        0.0f, 1.0f,  // 第一行
        1.0f, 2.0f   // 第二行
    };

    // 角落值
    EXPECT_FLOAT_EQ(noise::bilinearInterpolation(0.0f, 0.0f, values, 2, 2), 0.0f);
    EXPECT_FLOAT_EQ(noise::bilinearInterpolation(1.0f, 0.0f, values, 2, 2), 1.0f);
    EXPECT_FLOAT_EQ(noise::bilinearInterpolation(0.0f, 1.0f, values, 2, 2), 1.0f);
    EXPECT_FLOAT_EQ(noise::bilinearInterpolation(1.0f, 1.0f, values, 2, 2), 2.0f);

    // 中心值 (应该是四个角的平均值)
    f32 center = noise::bilinearInterpolation(0.5f, 0.5f, values, 2, 2);
    EXPECT_FLOAT_EQ(center, 1.0f);
}

TEST(NoiseUtils, TrilinearInterpolation) {
    std::vector<f32> values = {
        // z=0
        0.0f, 1.0f,  // y=0
        1.0f, 2.0f,  // y=1
        // z=1
        1.0f, 2.0f,  // y=0
        2.0f, 3.0f   // y=1
    };

    // 角落值
    EXPECT_FLOAT_EQ(noise::trilinearInterpolation(0.0f, 0.0f, 0.0f, values, 2, 2, 2), 0.0f);
    EXPECT_FLOAT_EQ(noise::trilinearInterpolation(1.0f, 1.0f, 1.0f, values, 2, 2, 2), 3.0f);

    // 中心值
    f32 center = noise::trilinearInterpolation(0.5f, 0.5f, 0.5f, values, 2, 2, 2);
    EXPECT_FLOAT_EQ(center, 1.5f);
}

// ============================================================================
// MathUtils额外测试
// ============================================================================

TEST(MathUtils, ClampBoundary) {
    EXPECT_EQ(math::clamp(5, 0, 10), 5);
    EXPECT_EQ(math::clamp(-5, 0, 10), 0);
    EXPECT_EQ(math::clamp(15, 0, 10), 10);

    // 浮点数
    EXPECT_FLOAT_EQ(math::clamp(0.5f, 0.0f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(math::clamp(-0.1f, 0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::clamp(1.1f, 0.0f, 1.0f), 1.0f);
}

TEST(MathUtils, LerpEdgeCases) {
    // math::lerp 参数顺序: lerp(a, b, t) = a + (b - a) * t
    // 边界值: t=0 返回 a, t=1 返回 b
    EXPECT_FLOAT_EQ(math::lerp(10.0f, 20.0f, 0.0f), 10.0f);
    EXPECT_FLOAT_EQ(math::lerp(10.0f, 20.0f, 1.0f), 20.0f);

    // 中间值: t=0.5 返回 a 和 b 的中点
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, 0.5f), 5.0f);

    // 外推
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, 2.0f), 20.0f);
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, -1.0f), -10.0f);
}

TEST(MathUtils, SmoothstepTest) {
    // 边界值
    EXPECT_FLOAT_EQ(math::smoothstep(0.0f, 1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::smoothstep(0.0f, 1.0f, 1.0f), 1.0f);

    // 中间值
    f32 mid = math::smoothstep(0.0f, 1.0f, 0.5f);
    EXPECT_GT(mid, 0.0f);
    EXPECT_LT(mid, 1.0f);
}

TEST(MathUtils, SquareAndCube) {
    EXPECT_EQ(math::square(3), 9);
    EXPECT_EQ(math::square(-3), 9);
    EXPECT_FLOAT_EQ(math::square(2.5f), 6.25f);

    EXPECT_EQ(math::cube(2), 8);
    EXPECT_EQ(math::cube(-2), -8);
    EXPECT_FLOAT_EQ(math::cube(3.0f), 27.0f);
}

TEST(MathUtils, IsZeroFloat) {
    EXPECT_TRUE(math::isZero(0.0f));
    // 默认epsilon是1e-6f，所以0.0000001f应该被认为是零
    EXPECT_TRUE(math::isZero(0.0000001f));
    EXPECT_FALSE(math::isZero(0.01f));
    EXPECT_FALSE(math::isZero(-0.01f));
}

TEST(MathUtils, ApproxEqualCustomEpsilon) {
    EXPECT_TRUE(math::approxEqual(1.0f, 1.001f, 0.01f));
    EXPECT_FALSE(math::approxEqual(1.0f, 1.1f, 0.01f));
}

TEST(MathUtils, CeilToFloorToRoundTo) {
    // CeilTo
    EXPECT_EQ(math::ceilTo<i32>(3.2f), 4);
    EXPECT_EQ(math::ceilTo<i32>(-3.2f), -3);

    // FloorTo
    EXPECT_EQ(math::floorTo<i32>(3.7f), 3);
    EXPECT_EQ(math::floorTo<i32>(-3.7f), -4);

    // RoundTo
    EXPECT_EQ(math::roundTo<i32>(3.4f), 3);
    EXPECT_EQ(math::roundTo<i32>(3.6f), 4);
}

TEST(MathUtils, ChunkPosToId) {
    u64 id1 = math::chunkPosToId(0, 0);
    EXPECT_EQ(id1, 0u);

    u64 id2 = math::chunkPosToId(1, 2);
    ChunkCoord x, z;
    math::idToChunkPos(id2, x, z);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(z, 2);

    // 负坐标
    u64 id3 = math::chunkPosToId(-10, -20);
    math::idToChunkPos(id3, x, z);
    EXPECT_EQ(x, -10);
    EXPECT_EQ(z, -20);
}

TEST(MathUtils, RandomClass) {
    math::Random rng1(12345);
    math::Random rng2(12345);

    // 相同种子应产生相同序列
    EXPECT_EQ(rng1.nextU64(), rng2.nextU64());
    EXPECT_EQ(rng1.nextU64(), rng2.nextU64());

    // nextU32
    u32 val32 = rng1.nextU32(100);
    EXPECT_LT(val32, 100u);

    // nextInt
    i32 intVal = rng1.nextInt(-10, 10);
    EXPECT_GE(intVal, -10);
    EXPECT_LE(intVal, 10);

    // nextFloat
    f32 floatVal = rng1.nextFloat();
    EXPECT_GE(floatVal, 0.0f);
    EXPECT_LT(floatVal, 1.0f);

    // nextFloat range
    f32 rangedFloat = rng1.nextFloat(10.0f, 20.0f);
    EXPECT_GE(rangedFloat, 10.0f);
    EXPECT_LT(rangedFloat, 20.0f);
}

TEST(MathUtils, PerlinNoiseClass) {
    math::PerlinNoise noise(12345);

    // 1D 噪声
    f32 val1 = noise.noise(0.5f);
    EXPECT_GE(val1, -1.0f);
    EXPECT_LE(val1, 1.0f);

    // 2D 噪声
    f32 val2 = noise.noise(0.5f, 0.5f);
    EXPECT_GE(val2, -1.0f);
    EXPECT_LE(val2, 1.0f);

    // 3D 噪声
    f32 val3 = noise.noise(0.5f, 0.5f, 0.5f);
    EXPECT_GE(val3, -1.0f);
    EXPECT_LE(val3, 1.0f);

    // 分形噪声
    f32 val4 = noise.octaveNoise(10.0f, 10.0f, 4, 0.5f);
    EXPECT_GE(val4, -1.0f);
    EXPECT_LE(val4, 1.0f);
}

// ============================================================================
// PerlinNoise 3D 测试
// ============================================================================

TEST(PerlinNoise, Basic3D) {
    PerlinNoise noise(12345);

    // 噪声值应在 [0, 1] 范围内
    for (int i = 0; i < 10; ++i) {
        f32 value = noise.noise3D(
            static_cast<f32>(i) * 0.1f,
            static_cast<f32>(i) * 0.2f,
            static_cast<f32>(i) * 0.3f
        );
        EXPECT_GE(value, 0.0f);
        EXPECT_LE(value, 1.0f);
    }
}

TEST(PerlinNoise, Octave3D) {
    PerlinNoise noise(12345);

    f32 value = noise.octave3D(10.0f, 20.0f, 30.0f, 3, 0.5f);
    EXPECT_GE(value, 0.0f);
    EXPECT_LE(value, 1.0f);
}

TEST(PerlinNoise, NegativeCoordinates) {
    PerlinNoise noise(12345);

    // 负坐标应该正常工作
    f32 val1 = noise.noise2D(-10.0f, -20.0f);
    EXPECT_GE(val1, 0.0f);
    EXPECT_LE(val1, 1.0f);

    f32 val2 = noise.noise2D(-100.5f, 50.5f);
    EXPECT_GE(val2, 0.0f);
    EXPECT_LE(val2, 1.0f);
}

TEST(PerlinNoise, SmoothTransitions) {
    PerlinNoise noise(12345);

    // 相邻点的值应该接近（平滑性）
    f32 val1 = noise.noise2D(10.0f, 10.0f);
    f32 val2 = noise.noise2D(10.1f, 10.0f);
    f32 val3 = noise.noise2D(10.0f, 10.1f);

    // 差异应该很小（Perlin噪声的特点是平滑）
    // 使用更宽松的阈值，因为噪声可能在某些位置变化较大
    EXPECT_LT(std::abs(val1 - val2), 0.2f);
    EXPECT_LT(std::abs(val1 - val3), 0.2f);
}
