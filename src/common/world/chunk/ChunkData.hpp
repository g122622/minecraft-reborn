#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "../block/Block.hpp"
#include "ChunkPos.hpp"
#include "../WorldConstants.hpp"
#include <vector>
#include <memory>
#include <array>
#include <cstring>

namespace mr {

// ============================================================================
// 区块段 (16x16x16 方块)
// ============================================================================

class ChunkSection {
public:
    static constexpr i32 SIZE = world::CHUNK_SECTION_HEIGHT; // 16
    static constexpr i32 VOLUME = SIZE * SIZE * SIZE;        // 4096

    ChunkSection();
    ~ChunkSection() = default;

    // 方块访问 (使用状态ID)
    [[nodiscard]] u32 getBlockStateId(i32 x, i32 y, i32 z) const;
    void setBlockStateId(i32 x, i32 y, i32 z, u32 stateId);

    // 方块访问 (使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlock(i32 x, i32 y, i32 z) const;
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state);

    // 快速访问 (无边界检查)
    [[nodiscard]] u32 getBlockStateIdFast(i32 index) const {
        return m_blockStates[index];
    }
    void setBlockStateIdFast(i32 index, u32 stateId) {
        m_blockStates[index] = stateId;
    }

    // 索引计算
    [[nodiscard]] static i32 blockIndex(i32 x, i32 y, i32 z) {
        return y * SIZE * SIZE + z * SIZE + x;
    }

    // 段信息
    [[nodiscard]] bool isEmpty() const { return m_blockCount == 0; }
    [[nodiscard]] u16 getBlockCount() const { return m_blockCount; }
    void setBlockCount(u16 count) { m_blockCount = count; }
    [[nodiscard]] bool needsRecalculate() const { return m_needsRecalculate; }
    void setNeedsRecalculate(bool value) { m_needsRecalculate = value; }

    // 光照
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;
    void setSkyLight(i32 x, i32 y, i32 z, u8 light);
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;
    void setBlockLight(i32 x, i32 y, i32 z, u8 light);

    // 序列化
    [[nodiscard]] std::vector<u8> serialize() const;
    [[nodiscard]] static Result<std::unique_ptr<ChunkSection>> deserialize(const u8* data, size_t size);

    // 填充
    void fill(u32 stateId);

private:
    // 使用状态ID存储 (紧凑格式，后续可改为调色板)
    std::vector<u32> m_blockStates;  // BlockState::stateId()
    std::vector<u8> m_skyLight;      // 天空光照 (4位/方块)
    std::vector<u8> m_blockLight;    // 方块光照 (4位/方块)
    u16 m_blockCount = 0;            // 非空气方块数量
    bool m_needsRecalculate = false;
};

// ============================================================================
// 区块数据
// ============================================================================

class ChunkData {
public:
    static constexpr i32 WIDTH = world::CHUNK_WIDTH;
    static constexpr i32 HEIGHT = world::CHUNK_HEIGHT;
    static constexpr i32 SECTIONS = world::CHUNK_SECTIONS;

    ChunkData();
    ChunkData(ChunkCoord x, ChunkCoord z);
    ~ChunkData() = default;

    // 禁止拷贝
    ChunkData(const ChunkData&) = delete;
    ChunkData& operator=(const ChunkData&) = delete;

    // 允许移动
    ChunkData(ChunkData&&) = default;
    ChunkData& operator=(ChunkData&&) = default;

    // 位置
    [[nodiscard]] ChunkCoord x() const { return m_x; }
    [[nodiscard]] ChunkCoord z() const { return m_z; }
    [[nodiscard]] ChunkPos pos() const { return ChunkPos(m_x, m_z); }

    // 方块访问 (使用 BlockState 指针)
    [[nodiscard]] const BlockState* getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const;
    void setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state);

    // 方块访问 (使用状态ID，更高效)
    [[nodiscard]] u32 getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const;
    void setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId);

    // 高度图
    [[nodiscard]] BlockCoord getHighestBlock(BlockCoord x, BlockCoord z) const;
    void updateHeightMap(BlockCoord x, BlockCoord z);

    // 区块段访问
    [[nodiscard]] ChunkSection* getSection(i32 index);
    [[nodiscard]] const ChunkSection* getSection(i32 index) const;
    [[nodiscard]] bool hasSection(i32 index) const;

    // 创建/删除段
    ChunkSection* createSection(i32 index);
    void removeSection(i32 index);

    // 区块状态
    [[nodiscard]] bool isFullyGenerated() const { return m_fullyGenerated; }
    void setFullyGenerated(bool value) { m_fullyGenerated = value; }

    [[nodiscard]] bool isDirty() const { return m_dirty; }
    void setDirty(bool value) { m_dirty = value; }

    [[nodiscard]] bool isLoaded() const { return m_loaded; }
    void setLoaded(bool value) { m_loaded = value; }

    // 序列化
    [[nodiscard]] std::vector<u8> serialize() const;
    [[nodiscard]] static Result<std::unique_ptr<ChunkData>> deserialize(const u8* data, size_t size);

    // 填充
    void fill(BlockCoord minY, BlockCoord maxY, u32 stateId);

private:
    ChunkCoord m_x = 0;
    ChunkCoord m_z = 0;

    // 区块段 (可以为空)
    std::array<std::unique_ptr<ChunkSection>, SECTIONS> m_sections;

    // 高度图 (最高方块Y坐标)
    std::array<BlockCoord, WIDTH * WIDTH> m_heightMap;

    // 状态
    bool m_fullyGenerated = false;
    bool m_dirty = false;
    bool m_loaded = false;
};

// ============================================================================
// 区块数据引用 (轻量级访问)
// ============================================================================

class ChunkDataRef {
public:
    ChunkDataRef(ChunkData* data, bool writeAccess = false);
    ~ChunkDataRef();

    // 禁止拷贝
    ChunkDataRef(const ChunkDataRef&) = delete;
    ChunkDataRef& operator=(const ChunkDataRef&) = delete;

    // 允许移动
    ChunkDataRef(ChunkDataRef&& other) noexcept;
    ChunkDataRef& operator=(ChunkDataRef&& other) noexcept;

    // 访问
    [[nodiscard]] ChunkData* get() const { return m_data; }
    [[nodiscard]] ChunkData* operator->() const { return m_data; }
    [[nodiscard]] ChunkData& operator*() const { return *m_data; }

    [[nodiscard]] bool isValid() const { return m_data != nullptr; }
    [[nodiscard]] bool hasWriteAccess() const { return m_writeAccess; }

private:
    ChunkData* m_data = nullptr;
    bool m_writeAccess = false;
};

// ============================================================================
// 区块唯一标识
// ============================================================================

struct ChunkId {
    ChunkCoord x;
    ChunkCoord z;
    i32 dimension;  // 0=主世界, 1=下界, 2=末地

    ChunkId() : x(0), z(0), dimension(0) {}
    ChunkId(ChunkCoord x, ChunkCoord z, i32 dim = 0)
        : x(x), z(z), dimension(dim) {}

    [[nodiscard]] u64 toId() const {
        u64 dx = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFFLL);
        u64 dz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFFLL);
        return (dx << 32) | dz;
    }

    [[nodiscard]] static ChunkId fromId(u64 id) {
        ChunkId cid;
        cid.x = static_cast<ChunkCoord>(static_cast<i64>(id >> 32));
        cid.z = static_cast<ChunkCoord>(static_cast<i64>(id & 0xFFFFFFFFLL));
        return cid;
    }

    bool operator==(const ChunkId& other) const {
        return x == other.x && z == other.z && dimension == other.dimension;
    }

    bool operator!=(const ChunkId& other) const {
        return !(*this == other);
    }

    bool operator<(const ChunkId& other) const {
        if (dimension != other.dimension) return dimension < other.dimension;
        if (x != other.x) return x < other.x;
        return z < other.z;
    }
};

} // namespace mr

// 哈希支持
namespace std {
template<>
struct hash<mr::ChunkId> {
    size_t operator()(const mr::ChunkId& id) const {
        return static_cast<size_t>(id.toId());
    }
};
}
