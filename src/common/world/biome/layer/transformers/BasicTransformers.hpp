#pragma once

#include "../Layer.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 岛屿层变换器
 *
 * 从初始噪声生成岛屿。
 * 参考 MC IslandLayer
 */
class IslandLayer : public IAreaTransformer {
public:
    explicit IslandLayer(u64 seed);

    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;

private:
    u64 m_seed;
};

/**
 * @brief 缩放层变换器
 *
 * 放大区域并平滑过渡。
 * 参考 MC ZoomLayer / VoroniZoomLayer
 */
class ZoomLayer : public IAreaTransformer {
public:
    enum class Mode {
        Normal,     // 普通缩放
        Voroni,     // Voroni 缩放（用于生物群系边界）
        Fuzzy       // 模糊缩放
    };

    explicit ZoomLayer(Mode mode = Mode::Normal);

    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;

    void getOutputSize(i32 inputWidth, i32 inputHeight,
                        i32& outWidth, i32& outHeight) const override {
        outWidth = inputWidth * 2;
        outHeight = inputHeight * 2;
    }

private:
    Mode m_mode;

    /**
     * @brief 获取相邻四个值的众数
     */
    [[nodiscard]] static i32 getMode(i32 a, i32 b, i32 c, i32 d);
};

/**
 * @brief 添加岛屿层
 *
 * 在海洋中添加额外岛屿。
 * 参考 MC AddIslandLayer
 */
class AddIslandLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 添加雪地层
 *
 * 根据温度添加雪地生物群系。
 * 参考 MC AddSnowLayer
 */
class AddSnowLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 边缘处理层
 *
 * 处理生物群系边缘过渡。
 * 参考 MC EdgeLayer
 */
class EdgeLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 蘑菇岛层
 *
 * 添加蘑菇岛生物群系。
 * 参考 MC AddMushroomIslandLayer
 */
class AddMushroomIslandLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

/**
 * @brief 深海层
 *
 * 将部分海洋转换为深海。
 * 参考 MC DeepOceanLayer
 */
class DeepOceanLayer : public IAreaTransformer {
public:
    [[nodiscard]] i32 apply(IAreaContext& context,
                             const IArea& area,
                             i32 x, i32 z) const override;
};

} // namespace mc
