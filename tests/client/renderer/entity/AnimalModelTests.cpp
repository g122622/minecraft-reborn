#include <gtest/gtest.h>
#include <algorithm>

#include "client/renderer/trident/entity/model/AnimalModels.hpp"

namespace mc::client::renderer {
namespace {

struct Bounds {
    f32 minX = 0.0f;
    f32 minY = 0.0f;
    f32 minZ = 0.0f;
    f32 maxX = 0.0f;
    f32 maxY = 0.0f;
    f32 maxZ = 0.0f;
};

Bounds computeBounds(const std::vector<ModelVertex>& vertices) {
    Bounds b;
    if (vertices.empty()) {
        return b;
    }

    b.minX = b.maxX = vertices[0].position.x;
    b.minY = b.maxY = vertices[0].position.y;
    b.minZ = b.maxZ = vertices[0].position.z;

    for (const auto& v : vertices) {
        b.minX = std::min(b.minX, v.position.x);
        b.minY = std::min(b.minY, v.position.y);
        b.minZ = std::min(b.minZ, v.position.z);
        b.maxX = std::max(b.maxX, v.position.x);
        b.maxY = std::max(b.maxY, v.position.y);
        b.maxZ = std::max(b.maxZ, v.position.z);
    }

    return b;
}

template <typename TModel>
Bounds buildDefaultPoseBounds(TModel& model) {
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    return computeBounds(vertices);
}

} // anonymous namespace

TEST(AnimalModelMesh, PigReasonableBoundsAndHorizontalBody) {
    PigModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 8.0f);
    EXPECT_GT(height, 8.0f);
    EXPECT_GT(depth, 10.0f);

    EXPECT_LT(width, 12.5f);
    EXPECT_LT(height, 14.0f);
    EXPECT_LT(depth, 20.0f);

    // 回归保护：未调用 setAngles 时会出现“竖直长方体”趋势（depth 过小）
    EXPECT_GT(depth, width * 0.8f);
    EXPECT_LT(height, depth * 1.2f);
}

TEST(AnimalModelMesh, CowReasonableBoundsAndHorizontalBody) {
    CowModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 10.0f);
    EXPECT_GT(height, 14.0f);
    EXPECT_GT(depth, 14.0f);

    EXPECT_LT(width, 14.5f);
    EXPECT_LT(height, 22.0f);
    EXPECT_LT(depth, 22.0f);

    EXPECT_GT(depth, width * 0.9f);
    EXPECT_LT(height, depth * 1.3f);
}

TEST(AnimalModelMesh, SheepReasonableBoundsAndHorizontalBody) {
    SheepModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 6.0f);
    EXPECT_GT(height, 12.0f);
    EXPECT_GT(depth, 12.0f);

    EXPECT_LT(width, 10.5f);
    EXPECT_LT(height, 20.0f);
    EXPECT_LT(depth, 20.0f);

    EXPECT_GT(depth, width * 1.1f);
    EXPECT_LT(height, depth * 1.4f);
}

TEST(AnimalModelMesh, ChickenReasonableBounds) {
    ChickenModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 4.0f);
    EXPECT_GT(height, 8.0f);
    EXPECT_GT(depth, 6.0f);

    EXPECT_LT(width, 8.5f);
    EXPECT_LT(height, 14.0f);
    EXPECT_LT(depth, 12.0f);
}

} // namespace mc::client::renderer
