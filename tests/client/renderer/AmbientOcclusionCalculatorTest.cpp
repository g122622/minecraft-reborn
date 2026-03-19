/**
 * @file AmbientOcclusionCalculatorTest.cpp
 * @brief 环境光遮蔽计算器单元测试
 *
 * 测试 AmbientOcclusionCalculator 的核心功能：
 * - 露天方块的AO计算
 * - 被包围方块的AO计算
 * - 透明方块的AO处理
 * - 区块边界采样
 */

#include <gtest/gtest.h>
#include <array>
#include <memory>
#include "client/renderer/trident/chunk/AmbientOcclusionCalculator.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"

namespace mc {
namespace client {
namespace renderer {
namespace test {

// ============================================================================
// 测试夹具
// ============================================================================

/**
 * @brief AO计算器测试夹具
 *
 * 提供测试所需的区块数据和方块注册。
 */
class AmbientOcclusionCalculatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 创建测试区块
        m_chunk = std::make_unique<ChunkData>(0, 0);

        // 初始化邻居区块指针数组
        for (size_t i = 0; i < 6; ++i) {
            m_neighbors[i] = nullptr;
        }
    }

    void TearDown() override {
        m_chunk.reset();
    }

    /**
     * @brief 在指定位置填充实心方块
     */
    void fillSolidBlock(i32 x, i32 y, i32 z) {
        const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stone"));
        if (stone && m_chunk) {
            m_chunk->setBlock(x, y, z, &stone->defaultState());
        }
    }

    /**
     * @brief 在指定位置填充透明方块（玻璃）
     */
    void fillTransparentBlock(i32 x, i32 y, i32 z) {
        const Block* glass = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:glass"));
        if (glass && m_chunk) {
            m_chunk->setBlock(x, y, z, &glass->defaultState());
        }
    }

    /**
     * @brief 设置指定位置的天空光
     */
    void setSkyLight(i32 x, i32 y, i32 z, u8 light) {
        if (m_chunk) {
            m_chunk->setSkyLight(x, y, z, light);
        }
    }

    /**
     * @brief 设置指定位置的方块光
     */
    void setBlockLight(i32 x, i32 y, i32 z, u8 light) {
        if (m_chunk) {
            m_chunk->setBlockLight(x, y, z, light);
        }
    }

    std::unique_ptr<ChunkData> m_chunk;
    const ChunkData* m_neighbors[6];
};

// ============================================================================
// 基础测试
// ============================================================================

TEST_F(AmbientOcclusionCalculatorTest, Calculate_ExposedBlock_HighBrightness) {
    // 创建一个孤立的石头方块在区块中心
    fillSolidBlock(8, 64, 8);

    // 设置高天空光
    setSkyLight(7, 64, 8, 15);
    setSkyLight(9, 64, 8, 15);
    setSkyLight(8, 65, 8, 15);  // 上方
    setSkyLight(8, 63, 8, 15);  // 下方

    AmbientOcclusionCalculator calculator;

    // 测试顶面（UP）
    auto result = calculator.calculate(*m_chunk, 8, 64, 8, Face::Top, m_neighbors);

    // 所有顶点应该有较高的光照值（因为周围都是空气）
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_GT(result.vertexSkyLight[i], 10) << "顶点 " << i << " 天空光过低";
        EXPECT_GT(result.vertexColorMultiplier[i], 0.8f) << "顶点 " << i << " AO乘数过低";
    }
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_SurroundedBlock_LowBrightness) {
    // 创建一个被完全包围的方块
    i32 cx = 8, cy = 64, cz = 8;

    // 填充周围的方块
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;  // 跳过中心
                fillSolidBlock(cx + dx, cy + dy, cz + dz);
            }
        }
    }

    // 填充中心方块
    fillSolidBlock(cx, cy, cz);

    AmbientOcclusionCalculator calculator;

    // 由于被完全包围，任何面都不应该被渲染（邻居检查会拒绝）
    // 但我们仍然可以测试AO计算器对被包围方块的处理

    // 测试角落采样
    // 被实心方块包围时，AO亮度应该较低
    auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

    // AO颜色乘数应该较低（被遮挡）
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_LT(result.vertexColorMultiplier[i], 0.5f)
            << "被包围方块的顶点 " << i << " AO乘数应较低";
    }
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_TransparentNeighbor_HighAO) {
    // 测试透明邻居（玻璃）的AO处理
    i32 cx = 8, cy = 64, cz = 8;

    // 创建中心方块
    fillSolidBlock(cx, cy, cz);

    // 在上方放置玻璃
    fillTransparentBlock(cx, cy + 1, cz);

    // 设置天空光
    setSkyLight(cx, cy + 1, cz, 15);

    AmbientOcclusionCalculator calculator;

    // 测试顶面 - 玻璃不会阻挡光照
    auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

    // 透明方块不产生AO阴影
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_GT(result.vertexColorMultiplier[i], 0.8f)
            << "透明邻居的顶点 " << i << " AO乘数应较高";
    }
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_CornerShading_CorrectInterpolation) {
    // 测试角落阴影的正确插值
    i32 cx = 8, cy = 64, cz = 8;

    // 创建中心方块
    fillSolidBlock(cx, cy, cz);

    // 只在一个角落放置方块（西北角）
    fillSolidBlock(cx - 1, cy + 1, cz - 1);

    AmbientOcclusionCalculator calculator;

    // 测试顶面
    auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

    // 角落遮挡应该导致不同顶点有不同的AO值
    // 不是所有顶点的AO值都相同
    bool hasVariation = false;
    for (size_t i = 1; i < 4; ++i) {
        if (std::abs(result.vertexColorMultiplier[i] - result.vertexColorMultiplier[0]) > 0.01f) {
            hasVariation = true;
            break;
        }
    }
    EXPECT_TRUE(hasVariation) << "角落遮挡应该导致顶点AO值变化";
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_DifferentFaces_DifferentResults) {
    // 测试不同面的AO计算
    i32 cx = 8, cy = 64, cz = 8;

    // 创建一个有不对称遮挡的方块
    fillSolidBlock(cx, cy, cz);

    // 只在东侧放置方块
    fillSolidBlock(cx + 1, cy, cz);

    AmbientOcclusionCalculator calculator;

    // 东面应该被完全遮挡
    auto eastResult = calculator.calculate(*m_chunk, cx, cy, cz, Face::East, m_neighbors);

    // 西面应该完全暴露
    auto westResult = calculator.calculate(*m_chunk, cx, cy, cz, Face::West, m_neighbors);

    // 东面的AO应该低于西面
    float eastAvg = 0.0f;
    float westAvg = 0.0f;
    for (size_t i = 0; i < 4; ++i) {
        eastAvg += eastResult.vertexColorMultiplier[i];
        westAvg += westResult.vertexColorMultiplier[i];
    }
    eastAvg /= 4.0f;
    westAvg /= 4.0f;

    EXPECT_LT(eastAvg, westAvg) << "被遮挡面的AO应低于暴露面";
}

// ============================================================================
// 边界测试
// ============================================================================

TEST_F(AmbientOcclusionCalculatorTest, Calculate_ChunkBoundary_SafeSampling) {
    // 测试区块边界的采样
    // 方块在区块边缘
    i32 cx = 0, cy = 64, cz = 0;

    fillSolidBlock(cx, cy, cz);

    AmbientOcclusionCalculator calculator;

    // 测试西面和北面（边界方向）
    // 不应该崩溃，应该使用默认值
    EXPECT_NO_THROW({
        auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::West, m_neighbors);
        // 边界外视为空气，应该有较高亮度
        for (size_t i = 0; i < 4; ++i) {
            EXPECT_GT(result.vertexColorMultiplier[i], 0.0f);
            EXPECT_GE(result.vertexSkyLight[i], 0);
        }
    });

    EXPECT_NO_THROW({
        auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::North, m_neighbors);
        for (size_t i = 0; i < 4; ++i) {
            EXPECT_GT(result.vertexColorMultiplier[i], 0.0f);
        }
    });
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_HeightBoundary_CorrectLighting) {
    // 测试高度边界的处理
    i32 cx = 8;

    // 测试世界顶部
    {
        i32 cy = world::MAX_BUILD_HEIGHT - 1;
        i32 cz = 8;

        fillSolidBlock(cx, cy, cz);

        AmbientOcclusionCalculator calculator;
        auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

        // 顶部边界外应该有高天空光
        for (size_t i = 0; i < 4; ++i) {
            EXPECT_EQ(result.vertexSkyLight[i], 15) << "世界顶部应该有最大天空光";
        }
    }

    // 测试世界底部
    {
        i32 cy = world::MIN_BUILD_HEIGHT;
        i32 cz = 8;

        fillSolidBlock(cx, cy, cz);

        AmbientOcclusionCalculator calculator;
        auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Bottom, m_neighbors);

        // 底部边界外应该是黑暗的
        for (size_t i = 0; i < 4; ++i) {
            EXPECT_LT(result.vertexColorMultiplier[i], 0.5f) << "世界底部应该有AO阴影";
        }
    }
}

// ============================================================================
// 光照测试
// ============================================================================

TEST_F(AmbientOcclusionCalculatorTest, Calculate_LightValues_PropagatedCorrectly) {
    // 测试光照值正确传播到顶点
    i32 cx = 8, cy = 64, cz = 8;

    fillSolidBlock(cx, cy, cz);

    // 设置不同的天空光和方块光
    setSkyLight(cx, cy + 1, cz, 12);  // 上方天空光
    setBlockLight(cx, cy + 1, cz, 8); // 上方方块光

    AmbientOcclusionCalculator calculator;
    auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

    // 顶点的光照应该接近周围光照的平均值
    // 注意：AO计算涉及多个角落采样，传播值会有所降低
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_GE(result.vertexSkyLight[i], 4) << "天空光传播过低";
        EXPECT_LE(result.vertexSkyLight[i], 15) << "天空光超出最大值";
        EXPECT_GE(result.vertexBlockLight[i], 1) << "方块光传播过低";
        EXPECT_LE(result.vertexBlockLight[i], 15) << "方块光超出最大值";
    }
}

TEST_F(AmbientOcclusionCalculatorTest, Calculate_BlockLight_IndependentlyCalculated) {
    // 测试方块光独立计算
    i32 cx = 8, cy = 64, cz = 8;

    fillSolidBlock(cx, cy, cz);

    // 只设置方块光，不设置天空光
    setSkyLight(cx, cy + 1, cz, 0);
    setBlockLight(cx, cy + 1, cz, 15);

    AmbientOcclusionCalculator calculator;
    auto result = calculator.calculate(*m_chunk, cx, cy, cz, Face::Top, m_neighbors);

    // 方块光应该传播
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_GT(result.vertexBlockLight[i], 0) << "方块光未传播";
    }
}

// ============================================================================
// 结果结构测试
// ============================================================================

TEST_F(AmbientOcclusionCalculatorTest, Result_AllVerticesInitialized) {
    fillSolidBlock(8, 64, 8);

    AmbientOcclusionCalculator calculator;

    // 测试所有6个面
    Face faces[] = {
        Face::Bottom, Face::Top,
        Face::North, Face::South,
        Face::West, Face::East
    };

    for (Face face : faces) {
        auto result = calculator.calculate(*m_chunk, 8, 64, 8, face, m_neighbors);

        // 检查所有顶点都被初始化
        for (size_t i = 0; i < 4; ++i) {
            EXPECT_GE(result.vertexColorMultiplier[i], 0.0f)
                << "面 " << static_cast<int>(face) << " 顶点 " << i << " AO乘数未初始化";
            EXPECT_LE(result.vertexColorMultiplier[i], 1.0f)
                << "面 " << static_cast<int>(face) << " 顶点 " << i << " AO乘数超出范围";
            EXPECT_LE(result.vertexSkyLight[i], 15)
                << "面 " << static_cast<int>(face) << " 顶点 " << i << " 天空光超出范围";
            EXPECT_LE(result.vertexBlockLight[i], 15)
                << "面 " << static_cast<int>(face) << " 顶点 " << i << " 方块光超出范围";
        }
    }
}

} // namespace test
} // namespace renderer
} // namespace client
} // namespace mc
