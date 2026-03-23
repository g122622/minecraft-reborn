#pragma once

#include "SurfaceBuilder.hpp"

namespace mc {

/**
 * @brief 默认地表构建器
 *
 * 参考 MC DefaultSurfaceBuilder，适用于大多数生物群系。
 * 根据噪声值计算地表深度，放置表层和次层方块。
 */
class DefaultSurfaceBuilder : public SurfaceBuilder {
public:
    DefaultSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "default"; }

protected:
    /**
     * @brief 计算地表深度
     * @param noise 噪声值
     * @param random 随机数生成器
     * @return 地表深度（方块数）
     */
    [[nodiscard]] i32 calculateDepth(f32 noise, math::Random& random) const;
};

/**
 * @brief 山地地表构建器
 *
 * 参考 MC MountainSurfaceBuilder，适用于山地生物群系。
 * 在高海拔处生成石头表面，可能生成雪。
 */
class MountainSurfaceBuilder : public SurfaceBuilder {
public:
    MountainSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "mountain"; }

private:
    /**
     * @brief 判断是否应该放置雪
     * @param y 高度
     * @param biome 生物群系
     * @return 是否放置雪
     */
    [[nodiscard]] bool shouldPlaceSnow(i32 y, const Biome& biome) const;
};

/**
 * @brief 沙漠地表构建器
 *
 * 参考 MC DesertSurfaceBuilder，适用于沙漠生物群系。
 * 使用沙子和砂岩。
 */
class DesertSurfaceBuilder : public SurfaceBuilder {
public:
    DesertSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "desert"; }
};

/**
 * @brief 沼泽地表构建器
 *
 * 参考 MC SwampSurfaceBuilder，适用于沼泽生物群系。
 * 在水面附近生成粘土和沙子。
 */
class SwampSurfaceBuilder : public SurfaceBuilder {
public:
    SwampSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "swamp"; }

private:
    /**
     * @brief 判断是否应该放置粘土
     * @param noise 噪声值
     * @return 是否放置粘土
     */
    [[nodiscard]] bool shouldPlaceClay(f32 noise) const;
};

/**
 * @brief 冻洋地表构建器
 *
 * 参考 MC FrozenOceanSurfaceBuilder，适用于冰冻海洋。
 * 生成冰和沙砾。
 */
class FrozenOceanSurfaceBuilder : public SurfaceBuilder {
public:
    FrozenOceanSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "frozen_ocean"; }
};

/**
 * @brief 恶地地表构建器
 *
 * 参考 MC BadlandsSurfaceBuilder，适用于恶地生物群系。
 * 生成彩色陶瓦层。
 */
class BadlandsSurfaceBuilder : public SurfaceBuilder {
public:
    BadlandsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "badlands"; }
};

/**
 * @brief 海滩地表构建器
 *
 * 参考 MC BeachSurfaceBuilder，适用于海滩生物群系。
 */
class BeachSurfaceBuilder : public SurfaceBuilder {
public:
    BeachSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "beach"; }
};

/**
 * @brief 巨型针叶林地表构建器
 *
 * 参考 MC GiantTreeTaigaSurfaceBuilder，适用于巨型针叶林生物群系。
 * 生成灰化土和砂土层。
 */
class GiantTreeTaigaSurfaceBuilder : public SurfaceBuilder {
public:
    GiantTreeTaigaSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "giant_tree_taiga"; }
};

/**
 * @brief 破碎热带草原地表构建器
 *
 * 参考 MC ShatteredSavannaSurfaceBuilder，适用于破碎热带草原生物群系。
 * 生成更多石头和砂土。
 */
class ShatteredSavannaSurfaceBuilder : public SurfaceBuilder {
public:
    ShatteredSavannaSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "shattered_savanna"; }
};

/**
 * @brief 竹林地表构建器
 *
 * 参考 MC BambooJungleSurfaceBuilder，适用于竹林生物群系。
 * 生成灰化土和砂砾。
 */
class BambooJungleSurfaceBuilder : public SurfaceBuilder {
public:
    BambooJungleSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "bamboo_jungle"; }
};

/**
 * @brief 下界森林地表构建器
 *
 * 参考 MC NetherForestSurfaceBuilder，适用于下界森林生物群系。
 * 使用下界岩和灵魂土。
 */
class NetherForestsSurfaceBuilder : public SurfaceBuilder {
public:
    NetherForestsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "nether_forests"; }
};

/**
 * @brief 灵魂沙峡谷地表构建器
 *
 * 参考 MC SoulSandValleySurfaceBuilder，适用于灵魂沙峡谷生物群系。
 * 生成灵魂沙和灵魂土。
 */
class SoulSandValleySurfaceBuilder : public SurfaceBuilder {
public:
    SoulSandValleySurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "soul_sand_valley"; }
};

} // namespace mc
