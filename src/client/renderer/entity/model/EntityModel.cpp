#include "EntityModel.hpp"
#include <cmath>

namespace mc::client::renderer {

// ==================== EntityModel ====================

void EntityModel::render(f32 scale) {
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void EntityModel::setAngles(f32 /*limbSwing*/, f32 /*limbSwingAmount*/,
                            f32 /*ageInTicks*/, f32 /*netHeadYaw*/,
                            f32 /*headPitch*/, f32 /*scale*/) {
    // 基类不实现动画
}

void EntityModel::copyAnglesTo(const EntityModel& /*target*/) const {
    // TODO: 实现角度复制
}

void EntityModel::generateMesh(std::vector<ModelVertex>& vertices,
                                std::vector<u32>& indices,
                                f32 scale) const {
    for (const auto& part : m_parts) {
        if (part) {
            part->generateMesh(vertices, indices, scale);
        }
    }
}

// ==================== QuadrupedModel ====================

QuadrupedModel::QuadrupedModel() {
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

void QuadrupedModel::setupParts() {
    // 默认四足动物模型尺寸
    // 子类应该重写此方法设置具体尺寸

    // 头部
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 18.0f, -6.0f);

    // 身体
    m_body->addBox(-6.0f, 10.0f, -8.0f, 12.0f, 18.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 前右腿
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legFrontRight->setRotationPoint(-3.0f, 18.0f, -4.0f);

    // 前左腿
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legFrontLeft->setRotationPoint(3.0f, 18.0f, -4.0f);

    // 后右腿
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legBackRight->setRotationPoint(-3.0f, 18.0f, 6.0f);

    // 后左腿
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legBackLeft->setRotationPoint(3.0f, 18.0f, 6.0f);
}

void QuadrupedModel::render(f32 scale) {
    EntityModel::render(scale);
}

void QuadrupedModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                                f32 /*ageInTicks*/, f32 netHeadYaw,
                                f32 headPitch, f32 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(headPitch * 3.14159f / 180.0f);
    m_head->setRotateAngleY(netHeadYaw * 3.14159f / 180.0f);

    // 步态动画
    f32 walkAngle = limbSwing * 3.14159f;
    f32 walkAmount = limbSwingAmount * 0.666f;

    // 前腿动画（与后腿相反）
    m_legFrontRight->setRotateAngleX(std::cos(walkAngle) * walkAmount);
    m_legFrontLeft->setRotateAngleX(-std::cos(walkAngle) * walkAmount);

    // 后腿动画
    m_legBackRight->setRotateAngleX(-std::cos(walkAngle) * walkAmount);
    m_legBackLeft->setRotateAngleX(std::cos(walkAngle) * walkAmount);
}

// ==================== BipedModel ====================

BipedModel::BipedModel() {
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_headwear = std::make_shared<ModelRenderer>("headwear");
    m_body = std::make_shared<ModelRenderer>("body");
    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_headwear);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightArm);
    m_parts.push_back(m_leftArm);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
}

void BipedModel::setupParts() {
    // 默认双足模型尺寸（玩家）

    // 头部
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 帽子层（略大于头部）
    m_headwear->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.5f);
    m_headwear->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 身体
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 右臂
    m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);

    // 左臂
    m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);

    // 右腿
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);

    // 左腿
    m_leftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);
}

void BipedModel::render(f32 scale) {
    EntityModel::render(scale);
}

void BipedModel::setAngles(f32 limbSwing, f32 limbSwingAmount,
                            f32 /*ageInTicks*/, f32 netHeadYaw,
                            f32 headPitch, f32 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(headPitch * 3.14159f / 180.0f);
    m_head->setRotateAngleY(netHeadYaw * 3.14159f / 180.0f);

    // 帽子跟随头部
    m_headwear->setRotateAngleX(headPitch * 3.14159f / 180.0f);
    m_headwear->setRotateAngleY(netHeadYaw * 3.14159f / 180.0f);

    // 步态动画
    f32 walkAngle = limbSwing * 3.14159f;
    f32 walkAmount = limbSwingAmount;

    // 手臂摆动
    m_rightArm->setRotateAngleX(std::cos(walkAngle) * 2.0f * walkAmount);
    m_leftArm->setRotateAngleX(-std::cos(walkAngle) * 2.0f * walkAmount);

    // 腿部摆动
    m_rightLeg->setRotateAngleX(std::cos(walkAngle) * 1.4f * walkAmount);
    m_leftLeg->setRotateAngleX(-std::cos(walkAngle) * 1.4f * walkAmount);
}

} // namespace mc::client::renderer
