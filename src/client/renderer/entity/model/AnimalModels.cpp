#include "AnimalModels.hpp"
#include <cmath>

namespace mr::client::renderer {

// ==================== PigModel ====================

PigModel::PigModel()
    : QuadrupedModel()
{
    // 重新设置猪的尺寸
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 12.0f, -6.0f);

    // 猪鼻子
    m_head->addBox(-2.0f, -2.0f, -12.0f, 4.0f, 3.0f, 4.0f);

    // 身体
    m_body->addBox(-5.0f, 10.0f, -8.0f, 10.0f, 16.0f, 8.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 腿部（猪腿较短）
    m_legFrontRight->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f);
    m_legFrontRight->setRotationPoint(-2.5f, 18.0f, -4.0f);

    m_legFrontLeft->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f);
    m_legFrontLeft->setRotationPoint(2.5f, 18.0f, -4.0f);

    m_legBackRight->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f);
    m_legBackRight->setRotationPoint(-2.5f, 18.0f, 6.0f);

    m_legBackLeft->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f);
    m_legBackLeft->setRotationPoint(2.5f, 18.0f, 6.0f);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void PigModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                          f32 ageInTicks, f32 netHeadYaw,
                          f32 headPitch, f32 scale) {
    // 调用基类动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== CowModel ====================

CowModel::CowModel()
    : QuadrupedModel()
{
    // 牛的尺寸较大
    // 头部
    m_head->addBox(-4.0f, -4.0f, -6.0f, 8.0f, 8.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 14.0f, -10.0f);

    // 牛鼻子
    m_head->addBox(-3.0f, -2.0f, -10.0f, 6.0f, 5.0f, 4.0f);

    // 牛角
    m_horns = std::make_shared<ModelRenderer>("horns");
    m_horns->addBox(-5.0f, -6.0f, -3.0f, 2.0f, 3.0f, 2.0f);  // 左角
    m_horns->addBox(3.0f, -6.0f, -3.0f, 2.0f, 3.0f, 2.0f);    // 右角
    m_horns->setRotationPoint(0.0f, 14.0f, -10.0f);
    m_parts.push_back(m_horns);

    // 身体
    m_body->addBox(-6.0f, 10.0f, -12.0f, 12.0f, 18.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 腿部（牛腿较长）
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontRight->setRotationPoint(-4.0f, 12.0f, -8.0f);

    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontLeft->setRotationPoint(4.0f, 12.0f, -8.0f);

    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackRight->setRotationPoint(-4.0f, 12.0f, 8.0f);

    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackLeft->setRotationPoint(4.0f, 12.0f, 8.0f);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void CowModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                          f32 ageInTicks, f32 netHeadYaw,
                          f32 headPitch, f32 scale) {
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 牛角跟随头部
    m_horns->setRotateAngleX(m_head->rotateAngleX());
    m_horns->setRotateAngleY(m_head->rotateAngleY());

    (void)scale;
    (void)ageInTicks;
}

// ==================== SheepModel ====================

SheepModel::SheepModel()
    : QuadrupedModel()
{
    // 头部
    m_head->addBox(-3.0f, -4.0f, -6.0f, 6.0f, 6.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 12.0f, -10.0f);

    // 羊毛层（覆盖身体）
    m_wool = std::make_shared<ModelRenderer>("wool");
    m_wool->addBox(-5.0f, 10.0f, -10.0f, 10.0f, 16.0f, 12.0f);
    m_wool->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_wool);

    // 身体
    m_body->addBox(-4.0f, 12.0f, -8.0f, 8.0f, 12.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 腿部
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 3.0f, 8.0f, 3.0f);
    m_legFrontRight->setRotationPoint(-3.0f, 16.0f, -6.0f);

    m_legFrontLeft->addBox(-1.0f, 0.0f, -2.0f, 3.0f, 8.0f, 3.0f);
    m_legFrontLeft->setRotationPoint(3.0f, 16.0f, -6.0f);

    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 3.0f, 8.0f, 3.0f);
    m_legBackRight->setRotationPoint(-3.0f, 16.0f, 6.0f);

    m_legBackLeft->addBox(-1.0f, 0.0f, -2.0f, 3.0f, 8.0f, 3.0f);
    m_legBackLeft->setRotationPoint(3.0f, 16.0f, 6.0f);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void SheepModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                            f32 ageInTicks, f32 netHeadYaw,
                            f32 headPitch, f32 scale) {
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== ChickenModel ====================

ChickenModel::ChickenModel()
    : EntityModel()
{
    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->addBox(-2.0f, -6.0f, -2.0f, 4.0f, 6.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 喙
    m_beak = std::make_shared<ModelRenderer>("beak");
    m_beak->addBox(-1.0f, -4.0f, -5.0f, 2.0f, 2.0f, 3.0f);
    m_beak->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 肉垂
    m_wattle = std::make_shared<ModelRenderer>("wattle");
    m_wattle->addBox(-1.0f, -3.0f, -3.0f, 2.0f, 3.0f, 2.0f);
    m_wattle->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 鸡冠
    m_comb = std::make_shared<ModelRenderer>("comb");
    m_comb->addBox(0.0f, -8.0f, -1.0f, 1.0f, 3.0f, 2.0f);
    m_comb->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->addBox(-3.0f, -4.0f, -3.0f, 6.0f, 8.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 16.0f, 0.0f);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->addBox(-4.0f, 0.0f, -2.0f, 4.0f, 4.0f, 4.0f);
    m_rightWing->setRotationPoint(-3.0f, 13.0f, 0.0f);

    // 左翼
    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->addBox(0.0f, 0.0f, -2.0f, 4.0f, 4.0f, 4.0f);
    m_leftWing->setRotationPoint(3.0f, 13.0f, 0.0f);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 4.0f, 2.0f);
    m_rightLeg->setRotationPoint(-1.0f, 20.0f, 1.0f);

    // 左腿
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 4.0f, 2.0f);
    m_leftLeg->setRotationPoint(1.0f, 20.0f, 1.0f);

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_beak);
    m_parts.push_back(m_wattle);
    m_parts.push_back(m_comb);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightWing);
    m_parts.push_back(m_leftWing);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void ChickenModel::render(f32 scale) {
    EntityModel::render(scale);
}

void ChickenModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                              f32 ageInTicks, f32 netHeadYaw,
                              f32 headPitch, f32 scale) {
    // 头部旋转
    m_head->setRotateAngleX(headPitch * 3.14159f / 180.0f);
    m_head->setRotateAngleY(netHeadYaw * 3.14159f / 180.0f);

    // 喙、肉垂、鸡冠跟随头部
    m_beak->setRotateAngleX(m_head->rotateAngleX());
    m_beak->setRotateAngleY(m_head->rotateAngleY());
    m_wattle->setRotateAngleX(m_head->rotateAngleX());
    m_wattle->setRotateAngleY(m_head->rotateAngleY());
    m_comb->setRotateAngleX(m_head->rotateAngleX());
    m_comb->setRotateAngleY(m_head->rotateAngleY());

    // 步态动画
    f32 walkAngle = limbSwing * 3.14159f;
    f32 walkAmount = limbSwingAmount;

    // 腿部动画
    m_rightLeg->setRotateAngleX(std::cos(walkAngle) * walkAmount);
    m_leftLeg->setRotateAngleX(-std::cos(walkAngle) * walkAmount);

    // 翅膀动画（飞行时拍打）
    m_rightWing->setRotateAngleZ(-std::sin(ageInTicks * 0.3f) * walkAmount * 0.5f);
    m_leftLeg->setRotateAngleZ(std::sin(ageInTicks * 0.3f) * walkAmount * 0.5f);

    (void)scale;
}

} // namespace mr::client::renderer
