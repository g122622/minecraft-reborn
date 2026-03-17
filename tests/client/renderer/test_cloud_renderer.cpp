/**
 * @file test_cloud_renderer.cpp
 * @brief 云渲染器单元测试
 */

#include <gtest/gtest.h>
#include "common/world/dimension/DimensionRenderSettings.hpp"
#include "client/renderer/trident/cloud/CloudRenderer.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <limits>

namespace mc::world {
namespace test {

/**
 * @brief 测试维度渲染设置 - 主世界
 */
TEST(DimensionRenderSettingsTest, OverworldSettings) {
    auto settings = DimensionRenderSettings::overworld();

    EXPECT_TRUE(settings.hasClouds());
    EXPECT_FLOAT_EQ(settings.cloudHeight, 192.0f);
    EXPECT_TRUE(settings.hasSky);
    EXPECT_FALSE(settings.hasCeiling);
    EXPECT_EQ(settings.fogType, FogType::Normal);
    EXPECT_TRUE(settings.hasNaturalLight);
}

/**
 * @brief 测试维度渲染设置 - 下界
 */
TEST(DimensionRenderSettingsTest, NetherSettings) {
    auto settings = DimensionRenderSettings::nether();

    EXPECT_FALSE(settings.hasClouds());
    EXPECT_TRUE(std::isnan(settings.cloudHeight));
    EXPECT_FALSE(settings.hasSky);
    EXPECT_TRUE(settings.hasCeiling);
    EXPECT_EQ(settings.fogType, FogType::None);
    EXPECT_FALSE(settings.hasNaturalLight);
}

/**
 * @brief 测试维度渲染设置 - 末地
 */
TEST(DimensionRenderSettingsTest, EndSettings) {
    auto settings = DimensionRenderSettings::end();

    EXPECT_FALSE(settings.hasClouds());
    EXPECT_TRUE(std::isnan(settings.cloudHeight));
    EXPECT_FALSE(settings.hasSky);
    EXPECT_FALSE(settings.hasCeiling);
    EXPECT_EQ(settings.fogType, FogType::End);
    EXPECT_FALSE(settings.hasNaturalLight);
}

/**
 * @brief 测试 getDefault 返回主世界设置
 */
TEST(DimensionRenderSettingsTest, DefaultSettings) {
    auto settings = DimensionRenderSettings::getDefault();

    EXPECT_TRUE(settings.hasClouds());
    EXPECT_FLOAT_EQ(settings.cloudHeight, 192.0f);
    EXPECT_TRUE(settings.hasSky);
}

/**
 * @brief 测试 hasClouds 方法
 */
TEST(DimensionRenderSettingsTest, HasCloudsMethod) {
    auto overworld = DimensionRenderSettings::overworld();
    auto nether = DimensionRenderSettings::nether();
    auto end = DimensionRenderSettings::end();

    EXPECT_TRUE(overworld.hasClouds());
    EXPECT_FALSE(nether.hasClouds());
    EXPECT_FALSE(end.hasClouds());
}

/**
 * @brief 测试雾类型枚举值
 */
TEST(DimensionRenderSettingsTest, FogTypeEnumValues) {
    EXPECT_EQ(static_cast<u8>(FogType::None), 0);
    EXPECT_EQ(static_cast<u8>(FogType::Normal), 1);
    EXPECT_EQ(static_cast<u8>(FogType::End), 2);
}

} // namespace test
} // namespace mc::world

namespace mc::client::renderer::trident::cloud {
namespace test {

/**
 * @brief 测试 CloudMode 枚举值
 */
TEST(CloudModeTest, EnumValues) {
    EXPECT_EQ(static_cast<u8>(CloudMode::Off), 0);
    EXPECT_EQ(static_cast<u8>(CloudMode::Fast), 1);
    EXPECT_EQ(static_cast<u8>(CloudMode::Fancy), 2);
}

/**
 * @brief 测试 CloudUBO 结构大小和对齐
 */
TEST(CloudUBOTest, StructureSize) {
    // 确保 CloudUBO 大小足够容纳所有字段
    EXPECT_GE(sizeof(CloudUBO), 80);

    // 测试默认值
    CloudUBO ubo{};
    EXPECT_FLOAT_EQ(ubo.cloudColor.x, 0.0f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.y, 0.0f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.z, 0.0f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.w, 0.0f);
    EXPECT_FLOAT_EQ(ubo.cloudHeight, 0.0f);
    EXPECT_FLOAT_EQ(ubo.time, 0.0f);
    EXPECT_FLOAT_EQ(ubo.textureScale, 0.0f);
    EXPECT_FLOAT_EQ(ubo.cameraY, 0.0f);
}

/**
 * @brief 测试 CloudUBO 字段设置
 */
TEST(CloudUBOTest, FieldAssignment) {
    CloudUBO ubo{};
    ubo.cloudColor = glm::vec4(1.0f, 0.9f, 0.8f, 1.0f);
    ubo.cloudHeight = 192.0f;
    ubo.time = 1000.0f;
    ubo.textureScale = 0.00390625f;
    ubo.cameraY = 64.0f;

    EXPECT_FLOAT_EQ(ubo.cloudColor.x, 1.0f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.y, 0.9f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.z, 0.8f);
    EXPECT_FLOAT_EQ(ubo.cloudColor.w, 1.0f);
    EXPECT_FLOAT_EQ(ubo.cloudHeight, 192.0f);
    EXPECT_FLOAT_EQ(ubo.time, 1000.0f);
    EXPECT_FLOAT_EQ(ubo.textureScale, 0.00390625f);
    EXPECT_FLOAT_EQ(ubo.cameraY, 64.0f);
}

} // namespace test
} // namespace mc::client::renderer::trident::cloud
