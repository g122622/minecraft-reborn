#pragma once

#include "EntityModel.hpp"

namespace mc::client::renderer {

/**
 * @brief 猪模型
 *
 * 参考 MC 1.16.5 PigModel
 */
class PigModel : public QuadrupedModel {
public:
    PigModel();
    ~PigModel() override = default;

    void setAngles(f32 limbSwing, f32 limbSwingAmount,
                   f32 ageInTicks, f32 netHeadYaw,
                   f32 headPitch, f32 scale) override;
};

/**
 * @brief 牛模型
 *
 * 参考 MC 1.16.5 CowModel
 */
class CowModel : public QuadrupedModel {
public:
    CowModel();
    ~CowModel() override = default;

    void setAngles(f32 limbSwing, f32 limbSwingAmount,
                   f32 ageInTicks, f32 netHeadYaw,
                   f32 headPitch, f32 scale) override;

private:
    std::shared_ptr<ModelRenderer> m_horns;  // 牛角
};

/**
 * @brief 羊模型
 *
 * 参考 MC 1.16.5 SheepModel
 */
class SheepModel : public QuadrupedModel {
public:
    SheepModel();
    ~SheepModel() override = default;

    void setAngles(f32 limbSwing, f32 limbSwingAmount,
                   f32 ageInTicks, f32 netHeadYaw,
                   f32 headPitch, f32 scale) override;

    /**
     * @brief 设置羊毛状态
     * @param hasWool 是否有羊毛
     */
    void setWool(bool hasWool) { m_hasWool = hasWool; }

private:
    std::shared_ptr<ModelRenderer> m_wool;  // 羊毛层
    bool m_hasWool = true;
};

/**
 * @brief 鸡模型
 *
 * 参考 MC 1.16.5 ChickenModel
 */
class ChickenModel : public EntityModel {
public:
    ChickenModel();
    ~ChickenModel() override = default;

    void render(f32 scale = 1.0f / 16.0f) override;

    void setAngles(f32 limbSwing, f32 limbSwingAmount,
                   f32 ageInTicks, f32 netHeadYaw,
                   f32 headPitch, f32 scale) override;

private:
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_beak;      // 喙
    std::shared_ptr<ModelRenderer> m_wattle;    // 肉垂（下巴下面的红肉）
    std::shared_ptr<ModelRenderer> m_comb;      // 鸡冠
};

} // namespace mc::client::renderer
