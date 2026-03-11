#include "ModelRenderer.hpp"
#include <cmath>

namespace mr::client::renderer {

ModelRenderer::ModelRenderer(const String& name)
    : m_name(name)
{
}

ModelRenderer& ModelRenderer::addBox(f32 x, f32 y, f32 z,
                                       f32 width, f32 height, f32 depth,
                                       f32 delta) {
    Box box;
    box.x = x;
    box.y = y;
    box.z = z;
    box.width = width;
    box.height = height;
    box.depth = depth;
    box.delta = delta;
    box.textureOffsetX = 0;
    box.textureOffsetY = 0;
    m_boxes.push_back(box);
    return *this;
}

ModelRenderer& ModelRenderer::addBox(i32 textureOffsetX, i32 textureOffsetY,
                                       f32 x, f32 y, f32 z,
                                       f32 width, f32 height, f32 depth,
                                       f32 delta) {
    Box box;
    box.x = x;
    box.y = y;
    box.z = z;
    box.width = width;
    box.height = height;
    box.depth = depth;
    box.delta = delta;
    box.textureOffsetX = textureOffsetX;
    box.textureOffsetY = textureOffsetY;
    m_boxes.push_back(box);
    return *this;
}

std::shared_ptr<ModelRenderer> ModelRenderer::createChild(const String& name) {
    auto child = std::make_shared<ModelRenderer>(name);
    addChild(child);
    return child;
}

void ModelRenderer::render(f32 scale) {
    if (!m_visible || !m_showModel) {
        return;
    }

    // TODO: 实际渲染逻辑
    // 1. 保存当前矩阵
    // 2. 平移到旋转点
    // 3. 应用旋转
    // 4. 平移到偏移位置
    // 5. 应用缩放
    // 6. 渲染盒子
    // 7. 渲染子部件
    // 8. 恢复矩阵

    (void)scale;

    // 渲染子部件
    for (auto& child : m_children) {
        if (child) {
            child->render(scale);
        }
    }
}

void ModelRenderer::renderNoRotate(f32 scale) {
    if (!m_visible || !m_showModel) {
        return;
    }

    // TODO: 渲染盒子（不应用旋转）

    (void)scale;

    // 渲染子部件
    for (auto& child : m_children) {
        if (child) {
            child->renderNoRotate(scale);
        }
    }
}

void ModelRenderer::interpolateRotation(const Vector3f& target, f32 speed) {
    m_rotateAngleX += (target.x - m_rotateAngleX) * speed;
    m_rotateAngleY += (target.y - m_rotateAngleY) * speed;
    m_rotateAngleZ += (target.z - m_rotateAngleZ) * speed;
}

} // namespace mr::client::renderer
