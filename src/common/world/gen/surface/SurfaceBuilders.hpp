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

    /**
     * @brief 获取彩色陶瓦层
     * @param y 高度
     * @return 陶瓦方块ID
     */
    [[nodiscard]] BlockId getTerracottaLayer(i32 y) const;
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

} // namespace mc
