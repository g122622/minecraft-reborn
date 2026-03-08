#pragma once

#include "../../../core/Types.hpp"

namespace mr {

/**
 * @brief 噪声生成器接口
 *
 * 所有噪声生成器的基类接口。
 */
class INoiseGenerator {
public:
    virtual ~INoiseGenerator() = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] virtual f64 noise(f64 x, f64 y, f64 z) const = 0;

    /**
     * @brief 采样 2D 噪声值
     */
    [[nodiscard]] virtual f64 noise2D(f64 x, f64 z) const {
        return noise(x, 0.0, z);
    }
};

} // namespace mr
