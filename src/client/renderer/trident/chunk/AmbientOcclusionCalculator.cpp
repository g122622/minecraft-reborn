#include "AmbientOcclusionCalculator.hpp"
#include "../../../../common/world/chunk/ChunkData.hpp"
#include "../../../../common/world/block/Block.hpp"
#include <algorithm>

namespace mc {
namespace client {
namespace renderer {

// ============================================================================
// 面角落偏移定义
// ============================================================================

namespace {

/**
 * @brief 3D整数向量，用于角落偏移
 */
struct Vec3i {
    i32 x, y, z;
    constexpr Vec3i(i32 x_, i32 y_, i32 z_) : x(x_), y(y_), z(z_) {}
};

/**
 * @brief 每个面的4个角落偏移
 *
 * 相对于方块中心，沿着面的法线方向偏移到邻居位置。
 *
 * 参考: net.minecraft.client.renderer.BlockModelRenderer.NeighborInfo
 *
 * 角落顺序决定了顶点索引映射。
 * 对于每个面，角落按照以下顺序排列：
 * - 角落0,1,2,3 对应顶点位置
 *
 * DOWN面 (Y-): 角落在 Y-1 平面
 * UP面 (Y+): 角落在 Y+1 平面
 * NORTH面 (Z-): 角落在 Z-1 平面
 * SOUTH面 (Z+): 角落在 Z+1 平面
 * WEST面 (X-): 角落在 X-1 平面
 * EAST面 (X+): 角落在 X+1 平面
 */
constexpr std::array<std::array<Vec3i, 4>, 6> FACE_CORNERS = {{
    // DOWN (Y-1): 西北、东北、东南、西南 (相对于 Y-1 平面)
    // 顶点顺序: 左下、右下、右上、左上 (在DOWN面上)
    {{ Vec3i{-1, -1, -1}, Vec3i{+1, -1, -1}, Vec3i{+1, -1, +1}, Vec3i{-1, -1, +1} }},

    // UP (Y+1): 西北、东北、东南、西南 (相对于 Y+1 平面)
    {{ Vec3i{-1, +1, -1}, Vec3i{+1, +1, -1}, Vec3i{+1, +1, +1}, Vec3i{-1, +1, +1} }},

    // NORTH (Z-1): 西下、东下、东上、西上
    {{ Vec3i{-1, -1, -1}, Vec3i{+1, -1, -1}, Vec3i{+1, +1, -1}, Vec3i{-1, +1, -1} }},

    // SOUTH (Z+1): 西下、东下、东上、西上
    {{ Vec3i{-1, -1, +1}, Vec3i{+1, -1, +1}, Vec3i{+1, +1, +1}, Vec3i{-1, +1, +1} }},

    // WEST (X-1): 下北、下南、上南、上北
    {{ Vec3i{-1, -1, -1}, Vec3i{-1, -1, +1}, Vec3i{-1, +1, +1}, Vec3i{-1, +1, -1} }},

    // EAST (X+1): 下北、下南、上南、上北
    {{ Vec3i{+1, -1, -1}, Vec3i{+1, -1, +1}, Vec3i{+1, +1, +1}, Vec3i{+1, +1, -1} }},
}};

/**
 * @brief 面的法线方向偏移
 *
 * 用于确定采样光照的位置（面的外侧）
 */
constexpr std::array<Vec3i, 6> FACE_NORMALS = {{
    Vec3i{ 0, -1,  0},  // DOWN
    Vec3i{ 0, +1,  0},  // UP
    Vec3i{ 0,  0, -1},  // NORTH
    Vec3i{ 0,  0, +1},  // SOUTH
    Vec3i{-1,  0,  0},  // WEST
    Vec3i{+1,  0,  0},  // EAST
}};

/**
 * @brief 顶点索引映射
 *
 * 对于每个面，将角落采样结果映射到顶点索引。
 * MC中的顶点顺序可能与我们的不同，需要调整。
 *
 * 参考: net.minecraft.client.renderer.BlockModelRenderer.VertexTranslations
 */
constexpr std::array<std::array<u32, 4>, 6> VERTEX_MAP = {{
    // DOWN: 顶点0,1,2,3 映射到 角落3,0,1,2
    {{ 3, 0, 1, 2 }},
    // UP: 顶点0,1,2,3 映射到 角落2,3,0,1
    {{ 2, 3, 0, 1 }},
    // NORTH: 顶点0,1,2,3 映射到 角落3,0,1,2
    {{ 3, 0, 1, 2 }},
    // SOUTH: 顶点0,1,2,3 映射到 角落0,1,2,3
    {{ 0, 1, 2, 3 }},
    // WEST: 顶点0,1,2,3 映射到 角落3,0,1,2
    {{ 3, 0, 1, 2 }},
    // EAST: 顶点0,1,2,3 映射到 角落1,2,3,0
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
    const ChunkData* neighborChunks[6]
) {
    Result result{};

    const size_t faceIdx = static_cast<size_t>(face);
    const auto& corners = FACE_CORNERS[faceIdx];
    const auto& normal = FACE_NORMALS[faceIdx];

    // 采样4个角落位置
    std::array<CornerSample, 4> cornerSamples{};
    for (size_t i = 0; i < 4; ++i) {
        // 角落位置 = 方块位置 + 角落偏移
        i32 cx = blockX + corners[i].x;
        i32 cy = blockY + corners[i].y;
        i32 cz = blockZ + corners[i].z;

        cornerSamples[i] = samplePosition(chunk, cx, cy, cz, neighborChunks);
    }

    // 检查角落外侧的方块是否透明
    // 用于确定是否需要采样对角线位置
    std::array<bool, 4> cornerOuterOpaque{};
    for (size_t i = 0; i < 4; ++i) {
        // 外侧位置 = 角落位置 + 法线方向
        i32 ox = blockX + corners[i].x + normal.x;
        i32 oy = blockY + corners[i].y + normal.y;
        i32 oz = blockZ + corners[i].z + normal.z;

        auto sample = samplePosition(chunk, ox, oy, oz, neighborChunks);
        cornerOuterOpaque[i] = sample.opaque;
    }

    // 计算面中心位置的光照（用于某些情况）
    i32 centerX = blockX + normal.x;
    i32 centerY = blockY + normal.y;
    i32 centerZ = blockZ + normal.z;
    auto centerSample = samplePosition(chunk, centerX, centerY, centerZ, neighborChunks);
    u32 centerPacked = packLight(centerSample.skyLight, centerSample.blockLight);

    // 计算每个角落的打包亮度
    std::array<u32, 4> cornerPacked{};
    for (size_t i = 0; i < 4; ++i) {
        cornerPacked[i] = packLight(cornerSamples[i].skyLight, cornerSamples[i].blockLight);
    }

    // 计算对角线位置（当相邻角落被遮挡时需要）
    std::array<u32, 4> diagonalPacked{};
    for (size_t i = 0; i < 4; ++i) {
        size_t nextI = (i + 1) % 4;
        size_t prevI = (i + 3) % 4;

        // 对角线位置 = 角落i + 角落nextI的偏移（去掉法线分量）
        // 简化：如果两个相邻角落外侧都透明，则需要采样对角线
        if (!cornerOuterOpaque[i] || !cornerOuterOpaque[nextI]) {
            // 不需要对角线采样，使用角落i的值
            diagonalPacked[i] = cornerPacked[i];
        } else {
            // 需要采样对角线位置
            i32 dx = blockX + corners[i].x + corners[nextI].x;
            i32 dy = blockY + corners[i].y + corners[nextI].y;
            i32 dz = blockZ + corners[i].z + corners[nextI].z;

            // 对角线位置在面的外侧
            dx += normal.x;
            dy += normal.y;
            dz += normal.z;

            auto diagSample = samplePosition(chunk, dx, dy, dz, neighborChunks);
            diagonalPacked[i] = packLight(diagSample.skyLight, diagSample.blockLight);
        }
    }

    // 计算AO颜色乘数
    // 对于每个顶点，AO值是周围4个位置AO亮度的平均值
    // 参考 MC 的计算: (corner1 + corner2 + corner3 + center) * 0.25

    // 顶点AO计算 (对应MC中的顶点映射)
    std::array<float, 4> vertexAo{};
    std::array<u32, 4> vertexBrightness{};

    // 使用MC的顶点映射
    const auto& vertexMap = VERTEX_MAP[faceIdx];

    for (size_t v = 0; v < 4; ++v) {
        size_t c0 = vertexMap[v];  // 主角落
        size_t c1 = (c0 + 1) % 4;  // 相邻角落1
        size_t c2 = (c0 + 2) % 4;  // 对角角落
        size_t c3 = (c0 + 3) % 4;  // 相邻角落2

        // AO颜色乘数 = 四个位置的AO亮度平均
        float ao0 = cornerSamples[c0].aoBrightness;
        float ao1 = cornerSamples[c1].aoBrightness;
        float ao2 = cornerSamples[c2].aoBrightness;
        float aoCenter = centerSample.aoBrightness;

        vertexAo[v] = (ao0 + ao1 + ao2 + aoCenter) * 0.25f;

        // 亮度计算
        // 如果相邻角落外侧不透明，使用该角落的亮度
        // 否则使用对角线亮度或中心亮度
        u32 br0, br1, br2, br3;

        // c0 角落的亮度
        if (!cornerOuterOpaque[c3] && !cornerOuterOpaque[c0]) {
            br0 = centerPacked;
        } else {
            br0 = diagonalPacked[(c0 + 3) % 4]; // 对应的对角线
        }

        if (!cornerOuterOpaque[c1] && !cornerOuterOpaque[c0]) {
            br1 = cornerPacked[c0];
        } else {
            br1 = diagonalPacked[c0];
        }

        if (!cornerOuterOpaque[c1] && !cornerOuterOpaque[c2]) {
            br2 = diagonalPacked[c1];
        } else {
            br2 = cornerPacked[c1];
        }

        if (!cornerOuterOpaque[c3] && !cornerOuterOpaque[c2]) {
            br3 = diagonalPacked[(c2 + 3) % 4];
        } else {
            br3 = cornerPacked[c3];
        }

        // 计算顶点亮度
        // 简化版：使用四个角落亮度的平均
        u32 aoBr = getAoBrightness(cornerPacked[c3], cornerPacked[c0],
                                    diagonalPacked[(c0 + 3) % 4], centerPacked);
        vertexBrightness[v] = aoBr;
    }

    // 填充结果
    for (size_t i = 0; i < 4; ++i) {
        result.vertexColorMultiplier[i] = vertexAo[i];
        result.vertexSkyLight[i] = unpackSkyLight(vertexBrightness[i]);
        result.vertexBlockLight[i] = unpackBlockLight(vertexBrightness[i]);
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
        sample.opaque = false;
        return sample;
    }
    if (y < world::MIN_BUILD_HEIGHT) {
        sample.skyLight = 0;
        sample.blockLight = 0;
        sample.aoBrightness = 0.2f;
        sample.opaque = true;
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
            // X和Z都越界，需要获取对角区块，简化处理使用默认值
            sample.skyLight = 15;
            sample.blockLight = 0;
            sample.aoBrightness = 1.0f;
            sample.opaque = false;
            return sample;
        }
        sampleChunk = neighborChunks ? neighborChunks[2] : nullptr; // -Z
    } else if (localZ >= SIZE) {
        localZ -= SIZE;
        if (sampleChunk != &chunk && sampleChunk != nullptr) {
            sample.skyLight = 15;
            sample.blockLight = 0;
            sample.aoBrightness = 1.0f;
            sample.opaque = false;
            return sample;
        }
        sampleChunk = neighborChunks ? neighborChunks[3] : nullptr; // +Z
    }

    // 如果区块不存在，使用默认值
    if (!sampleChunk) {
        sample.skyLight = 15;
        sample.blockLight = 0;
        sample.aoBrightness = 1.0f;
        sample.opaque = false;
        return sample;
    }

    // 获取方块状态
    const BlockState* state = sampleChunk->getBlock(localX, y, localZ);

    // 获取光照
    sample.skyLight = sampleChunk->getSkyLight(localX, y, localZ);
    sample.blockLight = sampleChunk->getBlockLight(localX, y, localZ);

    // 计算AO亮度
    sample.aoBrightness = getAoBrightness(state);
    sample.opaque = isOpaque(state);

    return sample;
}

float AmbientOcclusionCalculator::getAoBrightness(const BlockState* state) {
    if (state == nullptr || state->isAir()) {
        // 空气或null，不产生阴影
        return 1.0f;
    }
    return state->getAmbientOcclusionLightValue();
}

bool AmbientOcclusionCalculator::isOpaque(const BlockState* state) {
    if (state == nullptr || state->isAir()) {
        return false;
    }
    return state->hasOpaqueCollisionShape();
}

u32 AmbientOcclusionCalculator::getAoBrightness(u32 br1, u32 br2, u32 br3, u32 br4) {
    // 参考 MC: 如果值为0，使用中心亮度
    // 这里简化处理，将0替换为br4
    if (br1 == 0) br1 = br4;
    if (br2 == 0) br2 = br4;
    if (br3 == 0) br3 = br4;

    // 平均并提取有效位
    // packed格式: skyLight << 16 | blockLight
    u32 skyBr = (((br1 >> 16) & 0xFF) + ((br2 >> 16) & 0xFF) +
                 ((br3 >> 16) & 0xFF) + ((br4 >> 16) & 0xFF)) >> 2;
    u32 blockBr = ((br1 & 0xFF) + (br2 & 0xFF) + (br3 & 0xFF) + (br4 & 0xFF)) >> 2;

    return (skyBr << 16) | blockBr;
}

u32 AmbientOcclusionCalculator::packLight(u8 skyLight, u8 blockLight) {
    // 使用16位存储，方便计算
    return (static_cast<u32>(skyLight) << 16) | static_cast<u32>(blockLight);
}

u8 AmbientOcclusionCalculator::unpackSkyLight(u32 packed) {
    return static_cast<u8>((packed >> 16) & 0xFF);
}

u8 AmbientOcclusionCalculator::unpackBlockLight(u32 packed) {
    return static_cast<u8>(packed & 0xFF);
}

u32 AmbientOcclusionCalculator::getVertexBrightness(
    u32 b1, u32 b2, u32 b3, u32 b4,
    float w1, float w2, float w3, float w4
) {
    // 根据权重插值亮度
    u32 sky = static_cast<u32>(
        ((b1 >> 16) & 0xFF) * w1 +
        ((b2 >> 16) & 0xFF) * w2 +
        ((b3 >> 16) & 0xFF) * w3 +
        ((b4 >> 16) & 0xFF) * w4
    ) & 0xFF;

    u32 block = static_cast<u32>(
        (b1 & 0xFF) * w1 +
        (b2 & 0xFF) * w2 +
        (b3 & 0xFF) * w3 +
        (b4 & 0xFF) * w4
    ) & 0xFF;

    return (sky << 16) | block;
}

} // namespace renderer
} // namespace client
} // namespace mc
