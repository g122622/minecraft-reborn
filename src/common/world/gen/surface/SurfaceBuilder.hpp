#pragma once

#include "../../../core/Types.hpp"
#include "../../../math/random/Random.hpp"
#include "../../block/Block.hpp"

namespace mc {

// 前向声明
class BlockState;
class ChunkPrimer;
class Biome;

/**
 * @brief 地表构建器配置
 *
 * 定义地表、次表层和水下层的方块类型。
 * 参考 MC SurfaceBuilderConfig
 */
struct SurfaceBuilderConfig {
    /// 表层方块（草方块、沙子等）
    BlockId topBlock = BlockId::Grass;

    /// 次表层方块（泥土、沙子等）
    BlockId underBlock = BlockId::Dirt;

    /// 水下表面方块（沙砾等）
    BlockId underWaterBlock = BlockId::Gravel;

    SurfaceBuilderConfig() = default;

    SurfaceBuilderConfig(BlockId top, BlockId under, BlockId underWater)
        : topBlock(top), underBlock(under), underWaterBlock(underWater) {}

    /**
     * @brief 创建草地配置
     */
    static SurfaceBuilderConfig grass() {
        return { BlockId::Grass, BlockId::Dirt, BlockId::Gravel };
    }

    /**
     * @brief 创建沙地配置
     */
    static SurfaceBuilderConfig sand() {
        return { BlockId::Sand, BlockId::Sand, BlockId::Sand };
    }

    /**
     * @brief 创建石头配置
     */
    static SurfaceBuilderConfig stone() {
        return { BlockId::Stone, BlockId::Stone, BlockId::Stone };
    }

    /**
     * @brief 创建沙砾配置
     */
    static SurfaceBuilderConfig gravel() {
        return { BlockId::Gravel, BlockId::Gravel, BlockId::Gravel };
    }

    /**
     * @brief 创建红沙配置
     */
    static SurfaceBuilderConfig redSand() {
        return { BlockId::RedSand, BlockId::RedSand, BlockId::RedSand };
    }
};

/**
 * @brief 地表构建器基类
 *
 * 参考 MC SurfaceBuilder，负责构建区块的地表层。
 * 不同的生物群系可以使用不同的地表构建器。
 */
class SurfaceBuilder {
public:
    virtual ~SurfaceBuilder() = default;

    /**
     * @brief 构建地表
     *
     * @param random 随机数生成器
     * @param chunk 区块数据
     * @param biome 生物群系
     * @param x 区块内 X 坐标 (0-15)
     * @param z 区块内 Z 坐标 (0-15)
     * @param startHeight 起始高度（从地表向下遍历）
     * @param surfaceNoise 地表噪声值（用于变化地表深度）
     * @param defaultBlock 默认方块（石头）
     * @param defaultFluid 默认流体（水）
     * @param seaLevel 海平面高度
     * @param config 地表配置
     */
    virtual void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) = 0;

    /**
     * @brief 获取地表构建器名称
     */
    [[nodiscard]] virtual const char* name() const = 0;
};

} // namespace mc
