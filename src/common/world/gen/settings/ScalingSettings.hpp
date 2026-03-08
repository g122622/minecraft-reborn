#pragma once

#include "../../../core/Types.hpp"

namespace mr {

/**
 * @brief 噪声缩放设置
 *
 * 参考 MC NoiseSettings.Scaling
 * 主世界默认值：xzScale=0.9999999814507745, yScale=0.9999999814507745
 *                xzFactor=80.0, yFactor=160.0
 *
 * @note 用于控制噪声在不同轴上的缩放比例
 */
struct ScalingSettings {
    f64 xzScale = 0.9999999814507745;   ///< XZ 平面缩放
    f64 yScale = 0.9999999814507745;    ///< Y 轴缩放
    f64 xzFactor = 80.0;                 ///< XZ 因子
    f64 yFactor = 160.0;                 ///< Y 因子
};

} // namespace mr
