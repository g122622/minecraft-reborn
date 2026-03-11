#pragma once

#include "EntityRenderer.hpp"
#include "model/EntityModel.hpp"
#include "../../../common/entity/living/LivingEntity.hpp"
#include <memory>

namespace mr::client::renderer {

/**
 * @brief 生物渲染器基类
 *
 * 用于渲染 LivingEntity 的渲染器基类。
 * 提供动画参数计算、模型渲染等功能。
 *
 * 参考 MC 1.16.5 LivingRenderer
 */
template<typename TModel>
class LivingRenderer : public EntityRenderer {
public:
    static_assert(std::is_base_of_v<EntityModel, TModel>,
                  "TModel must derive from EntityModel");

    LivingRenderer() = default;
    ~LivingRenderer() override = default;

    void render(Entity& entity, f32 partialTicks) override;

    /**
     * @brief 获取模型
     */
    [[nodiscard]] TModel& model() { return m_model; }
    [[nodiscard]] const TModel& model() const { return m_model; }

protected:
    TModel m_model;

    /**
     * @brief 设置模型动画参数
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    void setModelAngles(LivingEntity& entity, f32 partialTicks);

    /**
     * @brief 计算步态动画周期
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f32 getLimbSwing(LivingEntity& entity, f32 partialTicks) const;

    /**
     * @brief 计算步态动画强度
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f32 getLimbSwingAmount(LivingEntity& entity, f32 partialTicks) const;

    /**
     * @brief 获取头部偏航角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f32 getHeadYaw(LivingEntity& entity, f32 partialTicks) const;

    /**
     * @brief 获取头部俯仰角
     * @param entity 生物实体
     * @param partialTicks 部分tick
     */
    [[nodiscard]] f32 getHeadPitch(LivingEntity& entity, f32 partialTicks) const;

    /**
     * @brief 获取年龄（tick）
     * @param entity 生物实体
     */
    [[nodiscard]] f32 getAgeInTicks(LivingEntity& entity) const;

    /**
     * @brief 处理缩放（幼体）
     * @param entity 生物实体
     * @return 缩放因子
     */
    [[nodiscard]] f32 getScale(LivingEntity& entity) const;
};

// ==================== 模板实现 ====================

template<typename TModel>
void LivingRenderer<TModel>::render(Entity& entity, f32 partialTicks) {
    // 转换为 LivingEntity
    auto& living = static_cast<LivingEntity&>(entity);

    // 设置模型动画参数
    setModelAngles(living, partialTicks);

    // 获取缩放因子
    f32 scale = getScale(living) * (1.0f / 16.0f);

    // 渲染模型
    m_model.render(scale);

    // 渲染阴影
    if (m_shadowSize > 0.0f) {
        renderShadow(entity, partialTicks);
    }

    (void)partialTicks;
}

template<typename TModel>
void LivingRenderer<TModel>::setModelAngles(LivingEntity& entity, f32 partialTicks) {
    f32 limbSwing = getLimbSwing(entity, partialTicks);
    f32 limbSwingAmount = getLimbSwingAmount(entity, partialTicks);
    f32 ageInTicks = getAgeInTicks(entity);
    f32 headYaw = getHeadYaw(entity, partialTicks);
    f32 headPitch = getHeadPitch(entity, partialTicks);
    f32 scale = getScale(entity);

    m_model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
}

template<typename TModel>
f32 LivingRenderer<TModel>::getLimbSwing(LivingEntity& entity, f32 partialTicks) const {
    // 步态动画周期
    // TODO: 从 LivingEntity 获取
    // return Math::lerp(entity.prevLimbSwing, entity.limbSwing, partialTicks);
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

template<typename TModel>
f32 LivingRenderer<TModel>::getLimbSwingAmount(LivingEntity& entity, f32 partialTicks) const {
    // 步态动画强度
    // TODO: 从 LivingEntity 获取
    // return Math::lerp(entity.prevLimbSwingAmount, entity.limbSwingAmount, partialTicks);
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

template<typename TModel>
f32 LivingRenderer<TModel>::getHeadYaw(LivingEntity& entity, f32 partialTicks) const {
    // 头部偏航角（相对于身体）
    // TODO: 从 LivingEntity 获取
    // f32 bodyYaw = Math::lerp(entity.prevRenderYawOffset, entity.renderYawOffset, partialTicks);
    // f32 headYaw = Math::lerp(entity.prevRotationYaw, entity.rotationYaw, partialTicks);
    // return headYaw - bodyYaw;
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

template<typename TModel>
f32 LivingRenderer<TModel>::getHeadPitch(LivingEntity& entity, f32 partialTicks) const {
    // 头部俯仰角
    // TODO: 从 LivingEntity 获取
    // return Math::lerp(entity.prevRotationPitch, entity.rotationPitch, partialTicks);
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

template<typename TModel>
f32 LivingRenderer<TModel>::getAgeInTicks(LivingEntity& entity) const {
    // 年龄（用于空闲动画）
    // TODO: 从 LivingEntity 获取
    // return static_cast<f32>(entity.ticksExisted);
    (void)entity;
    return 0.0f;
}

template<typename TModel>
f32 LivingRenderer<TModel>::getScale(LivingEntity& entity) const {
    // 幼体缩放
    // TODO: 检查是否为 AgeableEntity 并计算缩放
    // if (auto* ageable = dynamic_cast<AgeableEntity*>(&entity)) {
    //     if (ageable->isChild()) {
    //         return 0.5f;
    //     }
    // }
    (void)entity;
    return 1.0f;
}

} // namespace mr::client::renderer
