#pragma once

#include "../../core/Types.hpp"
#include "../block/Block.hpp"

namespace mr {

// ============================================================================
// 缩放设置
// ============================================================================

/**
 * @brief 噪声缩放设置
 *
 * 参考 MC NoiseSettings.Scaling
 */
struct ScalingSettings {
    f64 xzScale = 0.9999999814507745;   // XZ 平面缩放
    f64 yScale = 0.9999999814507745;    // Y 轴缩放
    f64 xzFactor = 80.0;                 // XZ 因子
    f64 yFactor = 160.0;                 // Y 因子
};

// ============================================================================
// 滑动设置
// ============================================================================

/**
 * @brief 滑动设置（用于地形边界平滑）
 *
 * 参考 MC NoiseSettings.Slide
 * 用于在顶部和底部创建平滑的地形边界
 */
struct SlideSettings {
    i32 target = 0;    // 目标值
    i32 size = 0;      // 大小（影响范围）
    i32 offset = 0;    // 偏移

    SlideSettings() = default;
    SlideSettings(i32 target_, i32 size_, i32 offset_)
        : target(target_), size(size_), offset(offset_) {}
};

// ============================================================================
// 噪声设置
// ============================================================================

/**
 * @brief 噪声地形生成设置
 *
 * 参考 MC 1.16.5 NoiseSettings，用于配置地形噪声生成参数。
 *
 * 使用方法：
 * @code
 * NoiseSettings settings = NoiseSettings::overworld();
 * // 使用 settings 生成地形
 * @endcode
 */
struct NoiseSettings {
    // === 基本尺寸 ===
    i32 height = 256;                       // 噪声高度
    i32 sizeHorizontal = 1;                 // 水平大小
    i32 sizeVertical = 2;                   // 垂直大小

    // === 缩放设置 ===
    ScalingSettings scaling;

    // === 滑动设置 ===
    SlideSettings topSlide{-10, 3, 0};      // 顶部滑动
    SlideSettings bottomSlide{-30, 0, 0};   // 底部滑动

    // === 密度参数 ===
    f64 densityFactor = 1.0;                // 密度因子
    f64 densityOffset = -0.46875;           // 密度偏移

    // === 噪声选项 ===
    bool simplexSurfaceNoise = true;        // 使用 Simplex 地表噪声
    bool randomDensityOffset = true;        // 随机密度偏移
    bool isAmplified = false;               // 放大化地形

    // === 噪声尺寸计算 ===
    [[nodiscard]] i32 noiseSizeX() const {
        return 16 / (sizeHorizontal * 4);
    }

    [[nodiscard]] i32 noiseSizeY() const {
        return height / (sizeVertical * 4);
    }

    [[nodiscard]] i32 noiseSizeZ() const {
        return 16 / (sizeHorizontal * 4);
    }

    [[nodiscard]] i32 verticalNoiseGranularity() const {
        return sizeVertical * 4;
    }

    [[nodiscard]] i32 horizontalNoiseGranularity() const {
        return sizeHorizontal * 4;
    }

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static NoiseSettings overworld() {
        NoiseSettings settings;
        settings.height = 256;
        settings.sizeHorizontal = 1;
        settings.sizeVertical = 2;
        settings.densityFactor = 1.0;
        settings.densityOffset = -0.46875;
        settings.topSlide = SlideSettings{-10, 3, 0};
        settings.bottomSlide = SlideSettings{-30, 0, 0};
        settings.simplexSurfaceNoise = true;
        settings.randomDensityOffset = true;
        return settings;
    }

    /**
     * @brief 放大化主世界设置
     */
    static NoiseSettings amplified() {
        NoiseSettings settings = overworld();
        settings.isAmplified = true;
        settings.densityFactor = 2.0;
        return settings;
    }

    /**
     * @brief 下界设置
     */
    static NoiseSettings nether() {
        NoiseSettings settings;
        settings.height = 128;
        settings.sizeHorizontal = 1;
        settings.sizeVertical = 2;
        settings.densityFactor = 0.0;
        settings.densityOffset = 0.019921875;
        settings.topSlide = SlideSettings{120, 3, 0};
        settings.bottomSlide = SlideSettings{320, 4, -1};
        settings.simplexSurfaceNoise = false;
        settings.randomDensityOffset = false;
        return settings;
    }

    /**
     * @brief 末地设置
     */
    static NoiseSettings end() {
        NoiseSettings settings;
        settings.height = 128;
        settings.sizeHorizontal = 2;
        settings.sizeVertical = 1;
        settings.densityFactor = 0.0;
        settings.densityOffset = 0.0;
        settings.simplexSurfaceNoise = false;
        settings.randomDensityOffset = false;
        return settings;
    }
};

// ============================================================================
// 维度设置
// ============================================================================

/**
 * @brief 维度生成设置
 *
 * 参考 MC DimensionSettings，包含维度级别的生成配置。
 */
struct DimensionSettings {
    NoiseSettings noise;
    BlockId defaultBlock = BlockId::Stone;
    BlockId defaultFluid = BlockId::Water;
    i32 seaLevel = 63;
    i32 bedrockRoof = -10;     // 基岩顶部（下界用）
    i32 bedrockFloor = 0;      // 基岩底部

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static DimensionSettings overworld() {
        DimensionSettings settings;
        settings.noise = NoiseSettings::overworld();
        settings.defaultBlock = BlockId::Stone;
        settings.defaultFluid = BlockId::Water;
        settings.seaLevel = 63;
        return settings;
    }

    /**
     * @brief 下界设置
     */
    static DimensionSettings nether() {
        DimensionSettings settings;
        settings.noise = NoiseSettings::nether();
        settings.defaultBlock = BlockId::Netherrack;
        settings.defaultFluid = BlockId::Lava;
        settings.seaLevel = 32;
        settings.bedrockRoof = 127;
        settings.bedrockFloor = 0;
        return settings;
    }

    /**
     * @brief 末地设置
     */
    static DimensionSettings end() {
        DimensionSettings settings;
        settings.noise = NoiseSettings::end();
        settings.defaultBlock = BlockId::EndStone;
        settings.defaultFluid = BlockId::Air;
        settings.seaLevel = 0;
        return settings;
    }

    /**
     * @brief 平坦世界设置
     */
    static DimensionSettings flat() {
        DimensionSettings settings;
        settings.noise.height = 4;
        settings.noise.densityFactor = 0.0;
        settings.noise.densityOffset = 0.0;
        settings.defaultBlock = BlockId::Stone;
        settings.defaultFluid = BlockId::Air;
        settings.seaLevel = 0;
        return settings;
    }
};

// ============================================================================
// 生物群系定义
// ============================================================================

/**
 * @brief 生物群系定义
 *
 * 存储单个生物群系的生成参数。
 */
struct BiomeDefinition {
    BiomeId id = 0;
    String name;

    // 地形参数
    f32 depth = 0.0f;       // 深度/基础高度
    f32 scale = 0.0f;       // 高度变化比例

    // 气候参数
    f32 temperature = 0.5f;
    f32 humidity = 0.5f;
    f32 continentalness = 0.0f;
    f32 erosion = 0.0f;

    // 方块设置
    BlockId surfaceBlock = BlockId::Grass;
    BlockId subSurfaceBlock = BlockId::Dirt;
    BlockId underWaterBlock = BlockId::Gravel;
    BlockId bedrockBlock = BlockId::Bedrock;

    // 默认构造
    BiomeDefinition() = default;

    BiomeDefinition(BiomeId id_, const String& name_)
        : id(id_), name(name_) {}
};

// ============================================================================
// 预定义生物群系
// ============================================================================

namespace Biomes {

// 生物群系 ID 常量
constexpr BiomeId Ocean = 0;
constexpr BiomeId Plains = 1;
constexpr BiomeId Desert = 2;
constexpr BiomeId Mountains = 3;
constexpr BiomeId Forest = 4;
constexpr BiomeId Taiga = 5;
constexpr BiomeId Swamp = 6;
constexpr BiomeId River = 7;
constexpr BiomeId NetherWastes = 8;
constexpr BiomeId TheEnd = 9;
constexpr BiomeId FrozenOcean = 10;
constexpr BiomeId FrozenRiver = 11;
constexpr BiomeId SnowyPlains = 12;
constexpr BiomeId SnowyMountains = 13;
constexpr BiomeId MushroomFields = 14;
constexpr BiomeId Beach = 15;
constexpr BiomeId Jungle = 16;
constexpr BiomeId Savanna = 17;
constexpr BiomeId Badlands = 18;

// 获取默认生物群系定义
BiomeDefinition getPlains();
BiomeDefinition getDesert();
BiomeDefinition getMountains();
BiomeDefinition getForest();
BiomeDefinition getOcean();
BiomeDefinition getTaiga();
BiomeDefinition getJungle();
BiomeDefinition getSavanna();
BiomeDefinition getBadlands();

} // namespace Biomes

} // namespace mr
