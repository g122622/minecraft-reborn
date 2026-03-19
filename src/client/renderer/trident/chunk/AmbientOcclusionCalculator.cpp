#include "AmbientOcclusionCalculator.hpp"
#include "../../../../common/world/chunk/ChunkData.hpp"
#include "../../../../common/world/block/Block.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace client {
namespace renderer {

// ============================================================================
// 方向定义 - 与MC 1.16.5 Direction一致
// ============================================================================
//
// MC原版方向向量:
// DOWN  = (0, -1, 0)
// UP    = (0, +1, 0)
// NORTH = (0, 0, -1)
// SOUTH = (0, 0, +1)
// WEST  = (-1, 0, 0)
// EAST  = (+1, 0, 0)

namespace {

/**
 * @brief 方向向量定义
 *
 * 索引: 0=DOWN, 1=UP, 2=NORTH, 3=SOUTH, 4=WEST, 5=EAST
 * 与MC Direction.getIndex()一致
 */
constexpr std::array<i32, 6 * 3> DIRECTION_VECTORS = {
    0, -1,  0,   // DOWN (0)
    0,  +1,  0,   // UP (1)
    0,  0, -1,   // NORTH (2)
    0,  0, +1,   // SOUTH (3)
    -1,  0,  0,   // WEST (4)
    +1,  0,  0    // EAST (5)
};

/**
 * @brief 获取方向向量
 */
inline constexpr std::tuple<i32, i32, i32> getDirection(int dirIdx) {
    return {
        DIRECTION_VECTORS[dirIdx * 3 + 0],
        DIRECTION_VECTORS[dirIdx * 3 + 1],
        DIRECTION_VECTORS[dirIdx * 3 + 2]
    };
}

/**
 * @brief 每个面的角落方向定义
 *
 * 参考: net.minecraft.client.renderer.BlockModelRenderer.NeighborInfo
 *
 * 每个面定义4个方向，用于采样边缘中点位置。
 * 这些方向相对于基础位置（方块自身或面外侧）偏移。
 *
 * DOWN面: corners = {WEST, EAST, NORTH, SOUTH}
 * UP面: corners = {EAST, WEST, NORTH, SOUTH}
 * NORTH面: corners = {UP, DOWN, EAST, WEST}
 * SOUTH面: corners = {WEST, EAST, DOWN, UP}
 * WEST面: corners = {UP, DOWN, NORTH, SOUTH}
 * EAST面: corners = {DOWN, UP, NORTH, SOUTH}
 */
constexpr std::array<std::array<i32, 4>, 6> FACE_CORNER_DIRECTIONS = {{
    // DOWN: {WEST, EAST, NORTH, SOUTH} = {4, 5, 2, 3}
    {{ 4, 5, 2, 3 }},
    // UP: {EAST, WEST, NORTH, SOUTH} = {5, 4, 2, 3}
    {{ 5, 4, 2, 3 }},
    // NORTH: {UP, DOWN, EAST, WEST} = {1, 0, 5, 4}
    {{ 1, 0, 5, 4 }},
    // SOUTH: {WEST, EAST, DOWN, UP} = {4, 5, 0, 1}
    {{ 4, 5, 0, 1 }},
    // WEST: {UP, DOWN, NORTH, SOUTH} = {1, 0, 2, 3}
    {{ 1, 0, 2, 3 }},
    // EAST: {DOWN, UP, NORTH, SOUTH} = {0, 1, 2, 3}
    {{ 0, 1, 2, 3 }},
}};

/**
 * @brief 顶点索引映射
 *
 * 参考: net.minecraft.client.renderer.BlockModelRenderer.VertexTranslations
 *
 * 将计算得到的顶点亮度/颜色映射到正确的顶点索引。
 * 我们的顶点顺序是: 左下(0), 右下(1), 右上(2), 左上(3)
 */
constexpr std::array<std::array<u32, 4>, 6> VERTEX_TRANSLATIONS = {{
    // DOWN: vert0=0, vert1=1, vert2=2, vert3=3
    {{ 0, 1, 2, 3 }},
    // UP: vert0=2, vert1=3, vert2=0, vert3=1
    {{ 2, 3, 0, 1 }},
    // NORTH: vert0=3, vert1=0, vert2=1, vert3=2
    {{ 3, 0, 1, 2 }},
    // SOUTH: vert0=0, vert1=1, vert2=2, vert3=3
    {{ 0, 1, 2, 3 }},
    // WEST: vert0=3, vert1=0, vert2=1, vert3=2
    {{ 3, 0, 1, 2 }},
    // EAST: vert0=1, vert1=2, vert2=3, vert3=0
    {{ 1, 2, 3, 0 }},
}};

} // anonymous namespace

// ============================================================================
// AmbientOcclusionCalculator 实现
// ============================================================================

AmbientOcclusionCalculator::Result AmbientOcclusionCalculator::calculate(
    const ChunkData& chunk,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    Face face,
    const ChunkData* neighborChunks[6],
    const float* nonCubicWeights
) {
    Result result{};

    const size_t faceIdx = static_cast<size_t>(face);
    const auto& cornerDirs = FACE_CORNER_DIRECTIONS[faceIdx];
    const auto& vertexTrans = VERTEX_TRANSLATIONS[faceIdx];

    // 面法线方向索引
    const int faceNormalIdx = static_cast<int>(faceIdx);
    auto [fnX, fnY, fnZ] = getDirection(faceNormalIdx);

    // ================================================================
    // 步骤1: 确定基础采样位置
    // ================================================================
    // MC原版逻辑:
    // 如果面外侧的方块是实心的（isOpaqueCube），则采样位置在面外侧
    // 否则采样位置在方块自身位置
    // 这对应于fillQuadBounds中的BitSet.get(0)标志

    // 检查面外侧是否是实心方块
    i32 faceOuterX = blockX + fnX;
    i32 faceOuterY = blockY + fnY;
    i32 faceOuterZ = blockZ + fnZ;

    bool faceOuterOpaque = !isTransparent(chunk, faceOuterX, faceOuterY, faceOuterZ, neighborChunks);

    // 基础采样位置
    i32 baseX = faceOuterOpaque ? faceOuterX : blockX;
    i32 baseY = faceOuterOpaque ? faceOuterY : blockY;
    i32 baseZ = faceOuterOpaque ? faceOuterZ : blockZ;

    // ================================================================
    // 步骤2: 采样4个边缘位置的光照和AO亮度
    // ================================================================
    // 边缘位置 = basePos + cornerDirs[i]方向偏移
    std::array<CornerSample, 4> cornerSamples{};
    std::array<u32, 4> cornerPackedLight{};

    for (size_t i = 0; i < 4; ++i) {
        auto [dx, dy, dz] = getDirection(cornerDirs[i]);
        i32 cx = baseX + dx;
        i32 cy = baseY + dy;
        i32 cz = baseZ + dz;

        cornerSamples[i] = samplePosition(chunk, cx, cy, cz, neighborChunks);
        cornerPackedLight[i] = packLight(cornerSamples[i].skyLight, cornerSamples[i].blockLight);
    }

    // ================================================================
    // 步骤3: 检查边缘外侧透明度
    // ================================================================
    // 边缘外侧 = basePos + cornerDirs[i] + faceNormal
    std::array<bool, 4> edgeOuterTransparent{};

    for (size_t i = 0; i < 4; ++i) {
        auto [dx, dy, dz] = getDirection(cornerDirs[i]);
        i32 ox = baseX + dx + fnX;
        i32 oy = baseY + dy + fnY;
        i32 oz = baseZ + dz + fnZ;

        edgeOuterTransparent[i] = isTransparent(chunk, ox, oy, oz, neighborChunks);
    }

    // ================================================================
    // 步骤4: 采样对角线位置（当需要时）
    // ================================================================
    std::array<u32, 4> diagonalPackedLight{};
    std::array<float, 4> diagonalAoBrightness{};

    // MC原版对角线采样逻辑:
    // i1: 如果 !flag2 && !flag，使用 corner[0]；否则采样 corner[0] + corner[2]
    // j1: 如果 !flag3 && !flag，使用 corner[0]；否则采样 corner[0] + corner[3]
    // k1: 如果 !flag2 && !flag1，使用 corner[0]；否则采样 corner[1] + corner[2]
    // l1: 如果 !flag3 && !flag1，使用 corner[0]；否则采样 corner[1] + corner[3]

    // i1 (diagonal[0]): corner[0] + corner[2]
    if (!edgeOuterTransparent[2] && !edgeOuterTransparent[0]) {
        diagonalPackedLight[0] = cornerPackedLight[0];
        diagonalAoBrightness[0] = cornerSamples[0].aoBrightness;
    } else {
        auto [d0x, d0y, d0z] = getDirection(cornerDirs[0]);
        auto [d2x, d2y, d2z] = getDirection(cornerDirs[2]);
        i32 dx = baseX + d0x + d2x;
        i32 dy = baseY + d0y + d2y;
        i32 dz = baseZ + d0z + d2z;

        auto diagSample = samplePosition(chunk, dx, dy, dz, neighborChunks);
        diagonalPackedLight[0] = packLight(diagSample.skyLight, diagSample.blockLight);
        diagonalAoBrightness[0] = diagSample.aoBrightness;
    }

    // j1 (diagonal[1]): corner[0] + corner[3]
    if (!edgeOuterTransparent[3] && !edgeOuterTransparent[0]) {
        diagonalPackedLight[1] = cornerPackedLight[0];
        diagonalAoBrightness[1] = cornerSamples[0].aoBrightness;
    } else {
        auto [d0x, d0y, d0z] = getDirection(cornerDirs[0]);
        auto [d3x, d3y, d3z] = getDirection(cornerDirs[3]);
        i32 dx = baseX + d0x + d3x;
        i32 dy = baseY + d0y + d3y;
        i32 dz = baseZ + d0z + d3z;

        auto diagSample = samplePosition(chunk, dx, dy, dz, neighborChunks);
        diagonalPackedLight[1] = packLight(diagSample.skyLight, diagSample.blockLight);
        diagonalAoBrightness[1] = diagSample.aoBrightness;
    }

    // k1 (diagonal[2]): corner[1] + corner[2]
    if (!edgeOuterTransparent[2] && !edgeOuterTransparent[1]) {
        diagonalPackedLight[2] = cornerPackedLight[0];
        diagonalAoBrightness[2] = cornerSamples[0].aoBrightness;
    } else {
        auto [d1x, d1y, d1z] = getDirection(cornerDirs[1]);
        auto [d2x, d2y, d2z] = getDirection(cornerDirs[2]);
        i32 dx = baseX + d1x + d2x;
        i32 dy = baseY + d1y + d2y;
        i32 dz = baseZ + d1z + d2z;

        auto diagSample = samplePosition(chunk, dx, dy, dz, neighborChunks);
        diagonalPackedLight[2] = packLight(diagSample.skyLight, diagSample.blockLight);
        diagonalAoBrightness[2] = diagSample.aoBrightness;
    }

    // l1 (diagonal[3]): corner[1] + corner[3]
    if (!edgeOuterTransparent[3] && !edgeOuterTransparent[1]) {
        diagonalPackedLight[3] = cornerPackedLight[0];
        diagonalAoBrightness[3] = cornerSamples[0].aoBrightness;
    } else {
        auto [d1x, d1y, d1z] = getDirection(cornerDirs[1]);
        auto [d3x, d3y, d3z] = getDirection(cornerDirs[3]);
        i32 dx = baseX + d1x + d3x;
        i32 dy = baseY + d1y + d3y;
        i32 dz = baseZ + d1z + d3z;

        auto diagSample = samplePosition(chunk, dx, dy, dz, neighborChunks);
        diagonalPackedLight[3] = packLight(diagSample.skyLight, diagSample.blockLight);
        diagonalAoBrightness[3] = diagSample.aoBrightness;
    }

    // ================================================================
    // 步骤5: 计算中心位置的光照
    // ================================================================
    // MC原版:
    // i3 = 方块自身的光照（如果面外侧不透明）
    // 如果面外侧透明，使用面外侧位置的光照

    // 获取方块自身的光照
    auto selfSample = samplePosition(chunk, blockX, blockY, blockZ, neighborChunks);
    u32 selfPackedLight = packLight(selfSample.skyLight, selfSample.blockLight);

    // 获取面外侧位置的光照
    auto faceOuterSample = samplePosition(chunk, faceOuterX, faceOuterY, faceOuterZ, neighborChunks);
    u32 faceOuterPackedLight = packLight(faceOuterSample.skyLight, faceOuterSample.blockLight);

    // 中心光照
    u32 centerPackedLight = faceOuterOpaque ? selfPackedLight : faceOuterPackedLight;

    // 中心AO亮度
    float f8 = faceOuterOpaque
        ? selfSample.aoBrightness
        : faceOuterSample.aoBrightness;

    // ================================================================
    // 步骤6: 计算每个顶点的AO颜色乘数
    // ================================================================
    // MC原版变量命名:
    // f = cornerSamples[0].aoBrightness
    // f1 = cornerSamples[1].aoBrightness
    // f2 = cornerSamples[2].aoBrightness
    // f3 = cornerSamples[3].aoBrightness
    // f4 = diagonalAoBrightness[0]
    // f5 = diagonalAoBrightness[1]
    // f6 = diagonalAoBrightness[2]
    // f7 = diagonalAoBrightness[3]

    float f0 = cornerSamples[0].aoBrightness;
    float f1 = cornerSamples[1].aoBrightness;
    float f2 = cornerSamples[2].aoBrightness;
    float f3 = cornerSamples[3].aoBrightness;
    float f4 = diagonalAoBrightness[0];
    float f5 = diagonalAoBrightness[1];
    float f6 = diagonalAoBrightness[2];
    float f7 = diagonalAoBrightness[3];

    // MC原版计算:
    // f9  = (f3 + f + f5 + f8) * 0.25  -> 顶点0
    // f10 = (f2 + f + f4 + f8) * 0.25  -> 顶点1
    // f11 = (f2 + f1 + f6 + f8) * 0.25 -> 顶点2
    // f12 = (f3 + f1 + f7 + f8) * 0.25 -> 顶点3

    std::array<float, 4> vertexAoColorMultiplier = {{
        (f3 + f0 + f5 + f8) * 0.25f,  // 顶点0
        (f2 + f0 + f4 + f8) * 0.25f,  // 顶点1
        (f2 + f1 + f6 + f8) * 0.25f,  // 顶点2
        (f3 + f1 + f7 + f8) * 0.25f   // 顶点3
    }};

    // ================================================================
    // 步骤7: 计算每个顶点的亮度值
    // ================================================================
    // MC原版:
    // vertexBrightness[vert0] = getAoBrightness(l, i, j1, i3)
    // vertexBrightness[vert1] = getAoBrightness(k, i, i1, i3)
    // vertexBrightness[vert2] = getAoBrightness(k, j, k1, i3)
    // vertexBrightness[vert3] = getAoBrightness(l, j, l1, i3)

    // 其中:
    // l = cornerPackedLight[3]
    // i = cornerPackedLight[0]
    // k = cornerPackedLight[2]
    // j = cornerPackedLight[1]
    // i1 = diagonalPackedLight[0]
    // j1 = diagonalPackedLight[1]
    // k1 = diagonalPackedLight[2]
    // l1 = diagonalPackedLight[3]
    // i3 = centerPackedLight

    std::array<u32, 4> vertexBrightness;
    vertexBrightness[0] = getAoBrightness(cornerPackedLight[3], cornerPackedLight[0],
                                          diagonalPackedLight[1], centerPackedLight);
    vertexBrightness[1] = getAoBrightness(cornerPackedLight[2], cornerPackedLight[0],
                                          diagonalPackedLight[0], centerPackedLight);
    vertexBrightness[2] = getAoBrightness(cornerPackedLight[2], cornerPackedLight[1],
                                          diagonalPackedLight[2], centerPackedLight);
    vertexBrightness[3] = getAoBrightness(cornerPackedLight[3], cornerPackedLight[1],
                                          diagonalPackedLight[3], centerPackedLight);

    // ================================================================
    // 步骤8: 应用顶点映射
    // ================================================================
    for (size_t v = 0; v < 4; ++v) {
        size_t mappedIdx = vertexTrans[v];
        result.vertexColorMultiplier[mappedIdx] = vertexAoColorMultiplier[v];
        result.vertexSkyLight[mappedIdx] = unpackSkyLight(vertexBrightness[v]);
        result.vertexBlockLight[mappedIdx] = unpackBlockLight(vertexBrightness[v]);
    }

    return result;
}

AmbientOcclusionCalculator::CornerSample AmbientOcclusionCalculator::samplePosition(
    const ChunkData& chunk,
    i32 x,
    i32 y,
    i32 z,
    const ChunkData* neighborChunks[6]
) const {
    CornerSample sample{};

    // 边界检查
    constexpr i32 SIZE = ChunkSection::SIZE;

    // Y边界
    if (y >= world::MAX_BUILD_HEIGHT) {
        sample.skyLight = 15;
        sample.blockLight = 0;
        sample.aoBrightness = 1.0f;
        return sample;
    }
    if (y < world::MIN_BUILD_HEIGHT) {
        sample.skyLight = 0;
        sample.blockLight = 0;
        sample.aoBrightness = 0.2f;
        return sample;
    }

    // 确定采样区块
    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    // X边界处理
    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr; // -X
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr; // +X
    }

    // Z边界处理
    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            // X和Z都越界，需要获取对角区块
            sample.skyLight = 15;
            sample.blockLight = 0;
            sample.aoBrightness = 1.0f;
            return sample;
        }
        sampleChunk = neighborChunks ? neighborChunks[2] : nullptr; // -Z
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            sample.skyLight = 15;
            sample.blockLight = 0;
            sample.aoBrightness = 1.0f;
            return sample;
        }
        sampleChunk = neighborChunks ? neighborChunks[3] : nullptr; // +Z
    }

    // 如果区块不存在，使用默认值
    if (!sampleChunk) {
        sample.skyLight = 15;
        sample.blockLight = 0;
        sample.aoBrightness = 1.0f;
        return sample;
    }

    // 获取方块状态
    const BlockState* state = sampleChunk->getBlock(localX, y, localZ);

    // 获取光照
    sample.skyLight = sampleChunk->getSkyLight(localX, y, localZ);
    sample.blockLight = sampleChunk->getBlockLight(localX, y, localZ);

    // 计算AO亮度
    sample.aoBrightness = getAoBrightness(state);

    return sample;
}

bool AmbientOcclusionCalculator::isTransparent(
    const ChunkData& chunk,
    i32 x,
    i32 y,
    i32 z,
    const ChunkData* neighborChunks[6]
) const {
    // 边界检查
    constexpr i32 SIZE = ChunkSection::SIZE;

    // Y边界
    if (y >= world::MAX_BUILD_HEIGHT || y < world::MIN_BUILD_HEIGHT) {
        return true;  // 边界外视为透明
    }

    // 确定采样区块
    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    // X边界处理
    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr;
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr;
    }

    // Z边界处理
    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            return true;  // 对角区块缺失，视为透明
        }
        sampleChunk = neighborChunks ? neighborChunks[2] : nullptr;
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            return true;
        }
        sampleChunk = neighborChunks ? neighborChunks[3] : nullptr;
    }

    // 如果区块不存在，视为透明
    if (!sampleChunk) {
        return true;
    }

    // 获取方块状态
    const BlockState* state = sampleChunk->getBlock(localX, y, localZ);
    if (!state || state->isAir()) {
        return true;  // 空气是透明的
    }

    // 参考 MC: state.getOpacity(world, pos) == 0
    // 如果不透光度为0，则为透明
    return state->getOpacity() == 0;
}

float AmbientOcclusionCalculator::getAoBrightness(const BlockState* state) {
    if (state == nullptr || state->isAir()) {
        // 空气或null，不产生阴影
        return 1.0f;
    }
    // 参考 MC: state.getAmbientOcclusionLightValue(world, pos)
    return state->getAmbientOcclusionLightValue();
}

bool AmbientOcclusionCalculator::hasOpaqueCollisionShape(const BlockState* state) {
    if (state == nullptr || state->isAir()) {
        return false;
    }
    return state->hasOpaqueCollisionShape();
}

u32 AmbientOcclusionCalculator::getAoBrightness(u32 br1, u32 br2, u32 br3, u32 br4) {
    // 参考 MC: AmbientOcclusionFace#getAoBrightness
    // 如果值为0，使用中心亮度(br4)
    if (br1 == 0) br1 = br4;
    if (br2 == 0) br2 = br4;
    if (br3 == 0) br3 = br4;

    // MC原版: return br1 + br2 + br3 + br4 >> 2 & 16711935;
    // 我们的打包格式: skyLight << 20 | blockLight << 4
    u32 skyBr = (((br1 >> 20) & 0xF) + ((br2 >> 20) & 0xF) +
                 ((br3 >> 20) & 0xF) + ((br4 >> 20) & 0xF)) >> 2;
    u32 blockBr = (((br1 >> 4) & 0xF) + ((br2 >> 4) & 0xF) +
                   ((br3 >> 4) & 0xF) + ((br4 >> 4) & 0xF)) >> 2;

    return (skyBr << 20) | (blockBr << 4);
}

u32 AmbientOcclusionCalculator::packLight(u8 skyLight, u8 blockLight) {
    // 使用MC原版的打包格式: skyLight << 20 | blockLight << 4
    // 这样可以正确应用掩码操作
    return (static_cast<u32>(skyLight & 0xF) << 20) |
           (static_cast<u32>(blockLight & 0xF) << 4);
}

u8 AmbientOcclusionCalculator::unpackSkyLight(u32 packed) {
    return static_cast<u8>((packed >> 20) & 0xF);
}

u8 AmbientOcclusionCalculator::unpackBlockLight(u32 packed) {
    return static_cast<u8>((packed >> 4) & 0xF);
}

u32 AmbientOcclusionCalculator::getVertexBrightness(
    u32 b1, u32 b2, u32 b3, u32 b4,
    float w1, float w2, float w3, float w4
) {
    // 根据权重插值亮度
    u32 sky = static_cast<u32>(
        ((b1 >> 20) & 0xF) * w1 +
        ((b2 >> 20) & 0xF) * w2 +
        ((b3 >> 20) & 0xF) * w3 +
        ((b4 >> 20) & 0xF) * w4
    ) & 0xF;

    u32 block = static_cast<u32>(
        ((b1 >> 4) & 0xF) * w1 +
        ((b2 >> 4) & 0xF) * w2 +
        ((b3 >> 4) & 0xF) * w3 +
        ((b4 >> 4) & 0xF) * w4
    ) & 0xF;

    return (sky << 20) | (block << 4);
}

u32 AmbientOcclusionCalculator::getPackedLight(
    const ChunkData& chunk,
    i32 x, i32 y, i32 z,
    const ChunkData* neighborChunks[6]
) {
    constexpr i32 SIZE = ChunkSection::SIZE;

    if (y >= world::MAX_BUILD_HEIGHT) {
        return packLight(15, 0);
    }
    if (y < world::MIN_BUILD_HEIGHT) {
        return packLight(0, 0);
    }

    const ChunkData* sampleChunk = &chunk;
    i32 localX = x;
    i32 localZ = z;

    if (localX < 0) {
        localX += SIZE;
        sampleChunk = neighborChunks ? neighborChunks[0] : nullptr;
    } else if (localX >= SIZE) {
        localX -= SIZE;
        sampleChunk = neighborChunks ? neighborChunks[1] : nullptr;
    }

    if (localZ < 0) {
        localZ += SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            return packLight(15, 0);
        }
        sampleChunk = neighborChunks ? neighborChunks[2] : nullptr;
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            return packLight(15, 0);
        }
        sampleChunk = neighborChunks ? neighborChunks[3] : nullptr;
    }

    if (!sampleChunk) {
        return packLight(15, 0);
    }

    return packLight(
        sampleChunk->getSkyLight(localX, y, localZ),
        sampleChunk->getBlockLight(localX, y, localZ)
    );
}

} // namespace renderer
} // namespace client
} // namespace mc
