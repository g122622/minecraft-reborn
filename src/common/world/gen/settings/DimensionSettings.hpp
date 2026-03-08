#pragma once

#include "NoiseSettings.hpp"
#include "../../block/Block.hpp"

namespace mr {

/**
 * @brief 维度生成设置
 *
 * 参考 MC DimensionSettings，包含维度级别的生成配置。
 *
 * @note 参考 MC 1.16.5 DimensionSettings
 */
struct DimensionSettings {
    NoiseSettings noise;
    BlockId defaultBlock = BlockId::Stone;
    BlockId defaultFluid = BlockId::Water;
    i32 seaLevel = 63;
    i32 bedrockRoof = -10;     ///< 基岩顶部（下界用）
    i32 bedrockFloor = 0;      ///< 基岩底部

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

} // namespace mr
