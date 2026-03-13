#pragma once

#include "../../../core/Types.hpp"
#include <memory>
#include <functional>

namespace mc {

/**
 * @brief 区域上下文
 *
 * 提供区域采样的上下文信息，包括世界种子和并发安全的随机数生成器。
 */
class IAreaContext {
public:
    virtual ~IAreaContext() = default;

    /**
     * @brief 初始化随机数生成器
     * @param seed 世界种子
     */
    virtual void initRandom(u64 seed) = 0;

    /**
     * @brief 获取随机整数
     * @param bound 上界
     * @return [0, bound) 范围内的随机整数
     */
    [[nodiscard]] virtual i32 nextInt(i32 bound) = 0;

    /**
     * @brief 获取随机整数（带修饰）
     * @param bound 上界
     * @return [0, bound) 范围内的随机整数
     */
    [[nodiscard]] virtual i32 nextIntWithMod(i32 bound) = 0;
};

/**
 * @brief 区域接口
 *
 * 表示一个可以被采样的区域，返回指定位置的整数值。
 * 用于生物群系生成中的层叠处理。
 */
class IArea {
public:
    virtual ~IArea() = default;

    /**
     * @brief 采样指定位置的值
     * @param x X 坐标
     * @param z Z 坐标
     * @return 该位置的值
     */
    [[nodiscard]] virtual i32 getValue(i32 x, i32 z) const = 0;

    /**
     * @brief 获取区域宽度
     */
    [[nodiscard]] virtual i32 getWidth() const = 0;

    /**
     * @brief 获取区域高度
     */
    [[nodiscard]] virtual i32 getHeight() const = 0;
};

/**
 * @brief 区域工厂接口
 *
 * 创建区域对象的工厂，支持层的链式组合。
 */
class IAreaFactory {
public:
    virtual ~IAreaFactory() = default;

    /**
     * @brief 创建区域
     * @return 区域对象
     */
    [[nodiscard]] virtual std::unique_ptr<IArea> create() const = 0;
};

/**
 * @brief 层变换器接口
 *
 * 对输入层进行变换，生成新的层。
 * 参考 MC 1.16.5 IAreaTransformer。
 */
class IAreaTransformer {
public:
    virtual ~IAreaTransformer() = default;

    /**
     * @brief 应用变换
     * @param context 区域上下文
     * @param area 输入区域
     * @param x 起始 X 坐标
     * @param z 起始 Z 坐标
     * @param width 宽度
     * @param height 高度
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 apply(IAreaContext& context,
                                     const IArea& area,
                                     i32 x, i32 z) const = 0;

    /**
     * @brief 获取变换后的区域大小（相对于输入）
     * @param inputWidth 输入宽度
     * @param inputHeight 输入高度
     * @param outWidth 输出宽度
     * @param outHeight 输出高度
     */
    virtual void getOutputSize(i32 inputWidth, i32 inputHeight,
                                i32& outWidth, i32& outHeight) const {
        outWidth = inputWidth;
        outHeight = inputHeight;
    }
};

/**
 * @brief 像素变换器接口
 *
 * 直接变换单个像素值。
 * 参考 MC 1.16.5 IDimTransformer。
 */
class IPixelTransformer {
public:
    virtual ~IPixelTransformer() = default;

    /**
     * @brief 变换单个像素
     * @param context 区域上下文
     * @param value 输入值
     * @param x X 坐标
     * @param z Z 坐标
     * @return 变换后的值
     */
    [[nodiscard]] virtual i32 transform(IAreaContext& context,
                                          i32 value,
                                          i32 x, i32 z) const = 0;
};

} // namespace mc
