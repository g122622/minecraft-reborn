#pragma once

#include "../../core/Types.hpp"
#include <functional>
#include <string>
#include <vector>
#include <array>

namespace mc {

// 前向声明
class IChunk;
class ChunkPrimer;

// ============================================================================
// 区块生成阶段
// ============================================================================

/**
 * @brief 区块生成阶段
 *
 * 参考 MC ChunkStatus，定义区块生成的各个阶段。
 * 简化版阶段：
 * - EMPTY: 空区块，刚创建
 * - BIOMES: 生物群系已生成
 * - NOISE: 噪声地形已生成
 * - SURFACE: 地表已生成
 * - CARVERS: 洞穴雕刻已应用
 * - FEATURES: 特性（树木、矿石等）已放置
 * - LIGHT: 光照已计算
 * - HEIGHTMAPS: 高度图已更新
 * - FULL: 完整区块
 */
class ChunkStatus {
public:
    // === 静态阶段定义 ===
    static const ChunkStatus EMPTY;
    static const ChunkStatus BIOMES;
    static const ChunkStatus NOISE;
    static const ChunkStatus SURFACE;
    static const ChunkStatus CARVERS;
    static const ChunkStatus FEATURES;
    static const ChunkStatus LIGHT;
    static const ChunkStatus HEIGHTMAPS;
    static const ChunkStatus FULL;

    // === 类型定义 ===

    /**
     * @brief 选择性生成任务（不需要邻居区块）
     * @param chunk 要处理的区块
     */
    using SelectiveTask = std::function<void(ChunkPrimer& chunk)>;

    /**
     * @brief 需要邻居的生成任务
     * @param chunk 要处理的区块
     * @param neighbors 邻居区块数组（按固定顺序排列）
     */
    using NeighborTask = std::function<void(ChunkPrimer& chunk, const std::array<IChunk*, 8>& neighbors)>;

    // === 构造函数 ===

    ChunkStatus() = default;

    /**
     * @brief 构造区块状态
     * @param name 状态名称
     * @param ordinal 序号
     * @param parent 父状态
     * @param taskRange 需要的邻居区块范围（0表示不需要邻居）
     */
    ChunkStatus(const String& name, i32 ordinal, const ChunkStatus* parent, i32 taskRange);

    // === 属性访问 ===

    [[nodiscard]] const String& name() const { return m_name; }
    [[nodiscard]] i32 ordinal() const { return m_ordinal; }
    [[nodiscard]] const ChunkStatus* parent() const { return m_parent; }
    [[nodiscard]] i32 taskRange() const { return m_taskRange; }

    // === 比较操作 ===

    /**
     * @brief 检查此状态是否达到或超过指定状态
     */
    [[nodiscard]] bool isAtLeast(const ChunkStatus& status) const {
        return m_ordinal >= status.m_ordinal;
    }

    /**
     * @brief 检查此状态是否低于指定状态
     */
    [[nodiscard]] bool isBefore(const ChunkStatus& status) const {
        return m_ordinal < status.m_ordinal;
    }

    // === 静态方法 ===

    /**
     * @brief 获取所有状态列表（按顺序）
     */
    [[nodiscard]] static const std::vector<ChunkStatus>& getAll();

    /**
     * @brief 根据名称获取状态
     */
    [[nodiscard]] static const ChunkStatus* byName(const String& name);

    /**
     * @brief 根据序号获取状态
     */
    [[nodiscard]] static const ChunkStatus* byOrdinal(i32 ordinal);

    /**
     * @brief 获取状态数量
     */
    [[nodiscard]] static i32 count() { return static_cast<i32>(getAll().size()); }

    // === 比较运算符 ===

    bool operator==(const ChunkStatus& other) const {
        return m_ordinal == other.m_ordinal;
    }

    bool operator!=(const ChunkStatus& other) const {
        return m_ordinal != other.m_ordinal;
    }

    bool operator<(const ChunkStatus& other) const {
        return m_ordinal < other.m_ordinal;
    }

    bool operator<=(const ChunkStatus& other) const {
        return m_ordinal <= other.m_ordinal;
    }

    bool operator>(const ChunkStatus& other) const {
        return m_ordinal > other.m_ordinal;
    }

    bool operator>=(const ChunkStatus& other) const {
        return m_ordinal >= other.m_ordinal;
    }

private:
    String m_name;
    i32 m_ordinal = 0;
    const ChunkStatus* m_parent = nullptr;
    i32 m_taskRange = 0;
};

// ============================================================================
// 区块状态常量
// ============================================================================

namespace ChunkStatuses {

// 生成阶段序号常量
constexpr i32 EMPTY_ORDINAL = 0;
constexpr i32 BIOMES_ORDINAL = 1;
constexpr i32 NOISE_ORDINAL = 2;
constexpr i32 SURFACE_ORDINAL = 3;
constexpr i32 CARVERS_ORDINAL = 4;
constexpr i32 FEATURES_ORDINAL = 5;
constexpr i32 LIGHT_ORDINAL = 6;
constexpr i32 HEIGHTMAPS_ORDINAL = 7;
constexpr i32 FULL_ORDINAL = 8;

// 总阶段数
constexpr i32 COUNT = 9;

} // namespace ChunkStatuses

} // namespace mc
