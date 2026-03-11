#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/math/Vector3.hpp"
#include <vector>
#include <memory>

namespace mr::client::renderer {

/**
 * @brief 模型部件
 *
 * 代表模型的一个部分（如头部、身体、腿等）。
 * 包含位置、旋转、缩放以及子部件。
 *
 * 参考 MC 1.16.5 ModelRenderer
 */
class ModelRenderer {
public:
    /**
     * @brief 构造函数
     * @param name 部件名称（用于调试）
     */
    explicit ModelRenderer(const String& name = "");
    ~ModelRenderer() = default;

    // ========== 变换 ==========

    /**
     * @brief 设置位置偏移
     */
    void setOffset(f32 x, f32 y, f32 z) {
        m_offsetX = x;
        m_offsetY = y;
        m_offsetZ = z;
    }

    /**
     * @brief 设置旋转点
     */
    void setRotationPoint(f32 x, f32 y, f32 z) {
        m_rotationPointX = x;
        m_rotationPointY = y;
        m_rotationPointZ = z;
    }

    /**
     * @brief 设置旋转角度（度）
     */
    void setRotation(f32 x, f32 y, f32 z) {
        m_rotateAngleX = x;
        m_rotateAngleY = y;
        m_rotateAngleZ = z;
    }

    /**
     * @brief 设置缩放
     */
    void setScale(f32 x, f32 y, f32 z) {
        m_scaleX = x;
        m_scaleY = y;
        m_scaleZ = z;
    }

    // ========== 盒子（立方体） ==========

    /**
     * @brief 添加一个盒子
     * @param x 起始X
     * @param y 起始Y
     * @param z 起始Z
     * @param width 宽度
     * @param height 高度
     * @param depth 深度
     * @param delta 膨胀值（用于防止Z-fighting）
     * @return 本部件引用
     */
    ModelRenderer& addBox(f32 x, f32 y, f32 z, f32 width, f32 height, f32 depth, f32 delta = 0.0f);

    /**
     * @brief 添加一个盒子（带纹理偏移）
     * @param textureOffsetX 纹理偏移X
     * @param textureOffsetY 纹理偏移Y
     * @param x 起始X
     * @param y 起始Y
     * @param z 起始Z
     * @param width 宽度
     * @param height 高度
     * @param depth 深度
     * @param delta 膨胀值
     * @return 本部件引用
     */
    ModelRenderer& addBox(i32 textureOffsetX, i32 textureOffsetY,
                          f32 x, f32 y, f32 z,
                          f32 width, f32 height, f32 depth,
                          f32 delta = 0.0f);

    // ========== 子部件 ==========

    /**
     * @brief 添加子部件
     * @param child 子部件
     */
    void addChild(std::shared_ptr<ModelRenderer> child) {
        m_children.push_back(child);
    }

    /**
     * @brief 创建并添加子部件
     * @param name 子部件名称
     * @return 创建的子部件
     */
    std::shared_ptr<ModelRenderer> createChild(const String& name = "");

    // ========== 渲染 ==========

    /**
     * @brief 渲染模型
     * @param scale 缩放因子
     */
    void render(f32 scale = 1.0f / 16.0f);

    /**
     * @brief 渲染时不进行旋转
     * @param scale 缩放因子
     */
    void renderNoRotate(f32 scale = 1.0f / 16.0f);

    // ========== 动画 ==========

    /**
     * @brief 插值旋转
     * @param target 目标角度
     * @param speed 插值速度
     */
    void interpolateRotation(const Vector3f& target, f32 speed);

    // ========== 状态 ==========

    /**
     * @brief 是否可见
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief 是否显示模型
     */
    [[nodiscard]] bool showModel() const { return m_showModel; }
    void setShowModel(bool show) { m_showModel = show; }

    // ========== 旋转访问器 ==========

    [[nodiscard]] f32 rotateAngleX() const { return m_rotateAngleX; }
    [[nodiscard]] f32 rotateAngleY() const { return m_rotateAngleY; }
    [[nodiscard]] f32 rotateAngleZ() const { return m_rotateAngleZ; }

    void setRotateAngleX(f32 angle) { m_rotateAngleX = angle; }
    void setRotateAngleY(f32 angle) { m_rotateAngleY = angle; }
    void setRotateAngleZ(f32 angle) { m_rotateAngleZ = angle; }

private:
    String m_name;

    // 变换
    f32 m_offsetX = 0.0f;
    f32 m_offsetY = 0.0f;
    f32 m_offsetZ = 0.0f;
    f32 m_rotationPointX = 0.0f;
    f32 m_rotationPointY = 0.0f;
    f32 m_rotationPointZ = 0.0f;
    f32 m_rotateAngleX = 0.0f;
    f32 m_rotateAngleY = 0.0f;
    f32 m_rotateAngleZ = 0.0f;
    f32 m_scaleX = 1.0f;
    f32 m_scaleY = 1.0f;
    f32 m_scaleZ = 1.0f;

    // 纹理
    i32 m_textureWidth = 64;
    i32 m_textureHeight = 32;

    // 显示状态
    bool m_visible = true;
    bool m_showModel = true;

    // 子部件
    std::vector<std::shared_ptr<ModelRenderer>> m_children;

    // 盒子数据（用于渲染）
    struct Box {
        f32 x, y, z;
        f32 width, height, depth;
        f32 delta;
        i32 textureOffsetX, textureOffsetY;
    };
    std::vector<Box> m_boxes;
};

} // namespace mr::client::renderer
