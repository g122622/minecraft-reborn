#include <gtest/gtest.h>
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/PackMetadata.hpp"
#include "common/resource/FolderResourcePack.hpp"

using namespace mr;

// ResourceLocation测试
TEST(ResourceLocationTest, DefaultConstructor) {
    ResourceLocation loc;
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_TRUE(loc.path().empty());
}

TEST(ResourceLocationTest, ParseWithoutNamespace) {
    ResourceLocation loc("textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseWithNamespace) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.namespace_(), "minecraft");
    EXPECT_EQ(loc.path(), "textures/blocks/stone");
}

TEST(ResourceLocationTest, ParseCustomNamespace) {
    ResourceLocation loc("mymod:blocks/custom_block");
    EXPECT_EQ(loc.namespace_(), "mymod");
    EXPECT_EQ(loc.path(), "blocks/custom_block");
}

TEST(ResourceLocationTest, ToString) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toString(), "minecraft:textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePath) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath(), "assets/minecraft/textures/blocks/stone");
}

TEST(ResourceLocationTest, ToFilePathWithExtension) {
    ResourceLocation loc("minecraft:textures/blocks/stone");
    EXPECT_EQ(loc.toFilePath("png"), "assets/minecraft/textures/blocks/stone.png");
}

TEST(ResourceLocationTest, Comparison) {
    ResourceLocation loc1("minecraft:stone");
    ResourceLocation loc2("minecraft:stone");
    ResourceLocation loc3("minecraft:dirt");

    EXPECT_EQ(loc1, loc2);
    EXPECT_NE(loc1, loc3);
    EXPECT_LT(loc3, loc1); // dirt < stone alphabetically
}

// PackMetadata测试
TEST(PackMetadataTest, ParseValidJson) {
    const char* json = R"({"pack": {"pack_format": 3, "description": "Test Pack"}})";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 3);
    EXPECT_EQ(result.value().description(), "Test Pack");
}

TEST(PackMetadataTest, ParseEmptyJson) {
    const char* json = "{}";
    auto result = PackMetadata::parse(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().packFormat(), 0);
    EXPECT_TRUE(result.value().description().empty());
}

TEST(PackMetadataTest, IsCompatible) {
    PackMetadata meta;
    // 需要通过parse设置值，这里只测试isCompatible方法
    auto result = PackMetadata::parse(R"({"pack": {"pack_format": 3}})");
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(result.value().isCompatible(1, 5));
    EXPECT_TRUE(result.value().isCompatible(3, 3));
    EXPECT_FALSE(result.value().isCompatible(4, 6));
}

// FolderResourcePack测试 - 使用测试资源包
TEST(FolderResourcePackTest, LoadPackMetadata) {
    // 使用实际的资源包路径
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        EXPECT_EQ(pack.metadata().packFormat(), 3);
        EXPECT_FALSE(pack.metadata().description().empty());
    }
}

TEST(FolderResourcePackTest, HasResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        EXPECT_TRUE(pack.hasResource("pack.mcmeta"));
        EXPECT_TRUE(pack.hasResource("assets/minecraft/blockstates/oak_log.json"));
        EXPECT_FALSE(pack.hasResource("nonexistent/file.json"));
    }
}

TEST(FolderResourcePackTest, ReadResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readResource("pack.mcmeta");
        if (readResult.success()) {
            EXPECT_FALSE(readResult.value().empty());
        }
    }
}

TEST(FolderResourcePackTest, ReadTextResource) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto readResult = pack.readTextResource("pack.mcmeta");
        if (readResult.success()) {
            EXPECT_TRUE(readResult.value().find("pack") != String::npos);
        }
    }
}

TEST(FolderResourcePackTest, ListResources) {
    FolderResourcePack pack("z:/方块概念材质");

    auto result = pack.initialize();
    if (result.success()) {
        auto listResult = pack.listResources("assets/minecraft/blockstates", "json");
        if (listResult.success()) {
            EXPECT_FALSE(listResult.value().empty());

            // 检查是否包含oak_log.json
            bool foundOakLog = false;
            for (const auto& file : listResult.value()) {
                if (file.find("oak_log.json") != String::npos) {
                    foundOakLog = true;
                    break;
                }
            }
            EXPECT_TRUE(foundOakLog);
        }
    }
}
