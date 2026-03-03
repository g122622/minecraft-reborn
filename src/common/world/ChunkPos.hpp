#pragma once

#include "../core/Types.hpp"
#include "../core/Constants.hpp"
#include "../math/MathUtils.hpp"
#include "../math/Vector3.hpp"

#include <cstdint>
#include <functional>
#include <tuple>

namespace mr {

/**
 * @brief 方块位置（整数坐标）
 *
 * 用于精确定位方块在世界中的位置
 */
class BlockPos {
public:
    BlockCoord x, y, z;

    // 构造函数
    BlockPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    BlockPos(BlockCoord x, BlockCoord y, BlockCoord z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    explicit BlockPos(const Vector3& pos) noexcept
        : x(static_cast<BlockCoord>(std::floor(pos.x)))
        , y(static_cast<BlockCoord>(std::floor(pos.y)))
        , z(static_cast<BlockCoord>(std::floor(pos.z)))
    {
    }

    // 静态常量
    static BlockPos zero() { return {0, 0, 0}; }

    // 算术运算
    [[nodiscard]] BlockPos operator+(const BlockPos& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] BlockPos operator-(const BlockPos& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] BlockPos operator*(i32 scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar};
    }

    BlockPos& operator+=(const BlockPos& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    BlockPos& operator-=(const BlockPos& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // 比较运算
    [[nodiscard]] bool operator==(const BlockPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator!=(const BlockPos& other) const noexcept
    {
        return !(*this == other);
    }

    // 排序支持
    [[nodiscard]] bool operator<(const BlockPos& other) const noexcept
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    // 转换为浮点位置
    [[nodiscard]] Vector3 toVector3() const noexcept
    {
        return {static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)};
    }

    // 获取方块中心点
    [[nodiscard]] Vector3 center() const noexcept
    {
        return {static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f, static_cast<f32>(z) + 0.5f};
    }

    // 获取相邻方块
    [[nodiscard]] BlockPos up(i32 offset = 1) const noexcept { return {x, y + offset, z}; }
    [[nodiscard]] BlockPos down(i32 offset = 1) const noexcept { return {x, y - offset, z}; }
    [[nodiscard]] BlockPos north(i32 offset = 1) const noexcept { return {x, y, z - offset}; }
    [[nodiscard]] BlockPos south(i32 offset = 1) const noexcept { return {x, y, z + offset}; }
    [[nodiscard]] BlockPos east(i32 offset = 1) const noexcept { return {x + offset, y, z}; }
    [[nodiscard]] BlockPos west(i32 offset = 1) const noexcept { return {x - offset, y, z}; }

    // 根据朝向获取相邻方块
    [[nodiscard]] BlockPos offset(BlockFace face, i32 distance = 1) const noexcept
    {
        switch (face) {
            case BlockFace::Top:    return {x, y + distance, z};
            case BlockFace::Bottom: return {x, y - distance, z};
            case BlockFace::North:  return {x, y, z - distance};
            case BlockFace::South:  return {x, y, z + distance};
            case BlockFace::East:   return {x + distance, y, z};
            case BlockFace::West:   return {x - distance, y, z};
            default: return *this;
        }
    }

    // 转换为64位唯一ID
    [[nodiscard]] u64 toId() const noexcept
    {
        // 将坐标打包为64位ID
        // Y: 8位 (-128 to 127, 或更高的位)
        // X: 28位
        // Z: 28位
        const u64 ux = static_cast<u64>(static_cast<i64>(x) & 0x0FFFFFFFLL);
        const u64 uy = static_cast<u64>(static_cast<i64>(y) & 0xFFLL);
        const u64 uz = static_cast<u64>(static_cast<i64>(z) & 0x0FFFFFFFLL);
        return (ux << 36) | (uy << 28) | uz;
    }

    // 区块坐标
    [[nodiscard]] ChunkCoord chunkX() const noexcept { return math::toChunkCoord(x); }
    [[nodiscard]] ChunkCoord chunkZ() const noexcept { return math::toChunkCoord(z); }

    // 区块内坐标
    [[nodiscard]] BlockCoord localX() const noexcept { return math::toLocalCoord(x); }
    [[nodiscard]] BlockCoord localZ() const noexcept { return math::toLocalCoord(z); }

    // 区块段索引 (Y / 16)
    [[nodiscard]] i32 sectionIndex() const noexcept
    {
        return y / world::CHUNK_SECTION_HEIGHT;
    }
};

/**
 * @brief 区块位置
 *
 * 用于标识区块在世界中的位置 (X, Z)
 */
class ChunkPos {
public:
    ChunkCoord x, z;

    // 构造函数
    ChunkPos() noexcept
        : x(0)
        , z(0)
    {
    }

    ChunkPos(ChunkCoord x, ChunkCoord z) noexcept
        : x(x)
        , z(z)
    {
    }

    explicit ChunkPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , z(pos.chunkZ())
    {
    }

    explicit ChunkPos(const Vector3& pos) noexcept
        : x(math::toChunkCoord(pos.x))
        , z(math::toChunkCoord(pos.z))
    {
    }

    // 静态常量
    static ChunkPos zero() { return {0, 0}; }

    // 算术运算
    [[nodiscard]] ChunkPos operator+(const ChunkPos& other) const noexcept
    {
        return {x + other.x, z + other.z};
    }

    [[nodiscard]] ChunkPos operator-(const ChunkPos& other) const noexcept
    {
        return {x - other.x, z - other.z};
    }

    [[nodiscard]] ChunkPos operator*(i32 scalar) const noexcept
    {
        return {x * scalar, z * scalar};
    }

    // 比较运算
    [[nodiscard]] bool operator==(const ChunkPos& other) const noexcept
    {
        return x == other.x && z == other.z;
    }

    [[nodiscard]] bool operator!=(const ChunkPos& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const ChunkPos& other) const noexcept
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    // 区块在世界中的原点坐标
    [[nodiscard]] BlockCoord worldX() const noexcept { return x * world::CHUNK_WIDTH; }
    [[nodiscard]] BlockCoord worldZ() const noexcept { return z * world::CHUNK_WIDTH; }

    // 区块中心坐标
    [[nodiscard]] Vector3 center(f32 y = 0.0f) const noexcept
    {
        return {
            static_cast<f32>(worldX()) + world::CHUNK_WIDTH / 2.0f,
            y,
            static_cast<f32>(worldZ()) + world::CHUNK_WIDTH / 2.0f
        };
    }

    // 转换为64位唯一ID
    [[nodiscard]] u64 toId() const noexcept
    {
        return math::chunkPosToId(x, z);
    }

    // 从64位ID创建
    [[nodiscard]] static ChunkPos fromId(u64 id) noexcept
    {
        ChunkPos pos;
        math::idToChunkPos(id, pos.x, pos.z);
        return pos;
    }

    // 计算与另一个区块的曼哈顿距离
    [[nodiscard]] i32 manhattanDistance(const ChunkPos& other) const noexcept
    {
        return std::abs(x - other.x) + std::abs(z - other.z);
    }

    // 计算与另一个区块的切比雪夫距离（棋盘距离）
    [[nodiscard]] i32 chebyshevDistance(const ChunkPos& other) const noexcept
    {
        return std::max(std::abs(x - other.x), std::abs(z - other.z));
    }

    // 检查是否在指定半径内
    [[nodiscard]] bool inRange(const ChunkPos& center, i32 radius) const noexcept
    {
        return chebyshevDistance(center) <= radius;
    }

    // 获取相邻区块
    [[nodiscard]] ChunkPos north() const noexcept { return {x, z - 1}; }
    [[nodiscard]] ChunkPos south() const noexcept { return {x, z + 1}; }
    [[nodiscard]] ChunkPos east() const noexcept { return {x + 1, z}; }
    [[nodiscard]] ChunkPos west() const noexcept { return {x - 1, z}; }
};

/**
 * @brief 区块段位置
 *
 * 用于标识区块中的一个16x16x16的段
 */
class SectionPos {
public:
    ChunkCoord x;
    i32 y; // 段Y坐标 (0-15 for y=0-255)
    ChunkCoord z;

    SectionPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    SectionPos(ChunkCoord x, i32 y, ChunkCoord z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    explicit SectionPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , y(pos.sectionIndex())
        , z(pos.chunkZ())
    {
    }

    [[nodiscard]] bool operator==(const SectionPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator<(const SectionPos& other) const noexcept
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    // 世界Y坐标范围
    [[nodiscard]] i32 minY() const noexcept
    {
        return y * world::CHUNK_SECTION_HEIGHT;
    }

    [[nodiscard]] i32 maxY() const noexcept
    {
        return (y + 1) * world::CHUNK_SECTION_HEIGHT - 1;
    }

    // 区块位置
    [[nodiscard]] ChunkPos chunkPos() const noexcept
    {
        return {x, z};
    }
};

} // namespace mr

// 哈希函数支持
namespace std {
template<>
struct hash<mr::BlockPos> {
    size_t operator()(const mr::BlockPos& pos) const noexcept
    {
        return static_cast<size_t>(pos.toId());
    }
};

template<>
struct hash<mr::ChunkPos> {
    size_t operator()(const mr::ChunkPos& pos) const noexcept
    {
        return static_cast<size_t>(pos.toId());
    }
};

template<>
struct hash<mr::SectionPos> {
    size_t operator()(const mr::SectionPos& pos) const noexcept
    {
        size_t h1 = std::hash<mr::ChunkCoord>{}(pos.x);
        size_t h2 = std::hash<mr::i32>{}(pos.y);
        size_t h3 = std::hash<mr::ChunkCoord>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
