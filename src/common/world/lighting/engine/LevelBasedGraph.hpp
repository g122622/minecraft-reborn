#pragma once

#include "../../../core/Types.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace mc {

/**
 * @brief 基于级别的传播图
 *
 * 实现Flood Fill传播算法的核心类，用于光照计算。
 * 使用级别队列来高效处理光照更新，避免无限循环。
 *
 * 算法原理:
 * - 每个位置有一个级别值（光照等级）
 * - 当级别改变时，通知相邻位置
 * - 相邻位置根据传播规则计算新级别
 * - 使用优先队列按级别顺序处理更新
 *
 * 参考: net.minecraft.world.lighting.LevelBasedGraph
 */
class LevelBasedGraph {
public:
    /**
     * @brief 最大级别数（光照最大15级 + 1个溢出级）
     */
    static constexpr i32 MAX_LEVEL_COUNT = 16;

    /**
     * @brief 无效级别标记
     */
    static constexpr u8 INVALID_LEVEL = 255;

    virtual ~LevelBasedGraph() = default;

    // ========================================================================
    // 公共接口
    // ========================================================================

    /**
     * @brief 检查是否有待处理的更新
     * @return 如果有待处理更新返回true
     */
    [[nodiscard]] bool needsUpdate() const noexcept { return m_needsUpdate; }

    /**
     * @brief 获取待处理更新数量
     */
    [[nodiscard]] i32 queuedUpdateSize() const noexcept {
        return static_cast<i32>(m_propagationLevels.size());
    }

    /**
     * @brief 处理所有更新
     *
     * @param maxUpdates 最大更新数量
     * @return 剩余配额
     */
    i32 processUpdates(i32 maxUpdates);

    /**
     * @brief 调度位置更新
     * @param pos 位置编码
     */
    void scheduleUpdate(i64 pos);

    /**
     * @brief 取消位置更新
     * @param pos 位置编码
     */
    void cancelUpdate(i64 pos);

    /**
     * @brief 取消满足条件的位置更新
     * @param predicate 条件函数
     */
    void cancelUpdates(const std::function<bool(i64)>& predicate);

protected:
    /**
     * @brief 构造函数
     *
     * @param levelCount 级别数量（通常为16）
     * @param expectedUpdates 预期更新数量（用于预分配）
     * @param expectedPositions 预期位置数量（用于预分配）
     */
    LevelBasedGraph(i32 levelCount, i32 expectedUpdates, i32 expectedPositions);

    // ========================================================================
    // 虚方法（子类实现）
    // ========================================================================

    /**
     * @brief 检查是否为根节点
     *
     * 对于方块光照，根节点是光源位置。
     * 对于天空光照，根节点是世界顶部（天空）。
     *
     * @param pos 位置编码
     * @return 如果是根节点返回true
     */
    [[nodiscard]] virtual bool isRoot(i64 pos) const = 0;

    /**
     * @brief 计算位置的新级别
     *
     * 根据相邻位置的级别计算当前位置应该的级别。
     * 排除指定的源位置。
     *
     * @param pos 要计算的位置
     * @param excludedSource 排除的源位置
     * @param level 当前级别
     * @return 新级别
     */
    [[nodiscard]] virtual i32 computeLevel(i64 pos, i64 excludedSource, i32 level) = 0;

    /**
     * @brief 通知相邻位置
     *
     * 当位置的级别改变时调用，用于传播更新。
     *
     * @param pos 位置
     * @param level 新级别
     * @param isDecreasing 是否是减少（光照变暗）
     */
    virtual void notifyNeighbors(i64 pos, i32 level, bool isDecreasing) = 0;

    /**
     * @brief 获取位置的当前级别
     * @param pos 位置编码
     * @return 当前级别
     */
    [[nodiscard]] virtual i32 getLevel(i64 pos) const = 0;

    /**
     * @brief 设置位置的级别
     * @param pos 位置编码
     * @param level 新级别
     */
    virtual void setLevel(i64 pos, i32 level) = 0;

    /**
     * @brief 计算从起点到终点的边缘级别
     *
     * 计算从startPos（级别startLevel）传播到endPos后的级别。
     * 这考虑了方块透明度等因素。
     *
     * @param startPos 起始位置
     * @param endPos 目标位置
     * @param startLevel 起始级别
     * @return 传播后的级别
     */
    [[nodiscard]] virtual i32 getEdgeLevel(i64 startPos, i64 endPos, i32 startLevel) = 0;

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 传播级别到相邻位置
     *
     * @param fromPos 源位置
     * @param toPos 目标位置
     * @param newLevel 新级别
     * @param isDecreasing 是否是减少
     */
    void propagateLevel(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing);

    /**
     * @brief 调度带源位置的更新
     *
     * @param fromPos 源位置
     * @param toPos 目标位置
     * @param newLevel 新级别
     * @param isDecreasing 是否是减少
     */
    void scheduleUpdate(i64 fromPos, i64 toPos, i32 newLevel, bool isDecreasing);

private:
    i32 m_levelCount;
    std::vector<std::unordered_set<i64>> m_updatesByLevel;
    std::unordered_map<i64, u8> m_propagationLevels;
    i32 m_minLevelToUpdate;
    bool m_needsUpdate;

    /**
     * @brief 获取最小级别
     */
    [[nodiscard]] i32 minLevel(i32 level1, i32 level2) const;

    /**
     * @brief 更新最小级别
     */
    void updateMinLevel(i32 maxLevel);

    /**
     * @brief 移除更新
     */
    void removeUpdate(i64 pos, i32 level, i32 maxLevel, bool removeAll);

    /**
     * @brief 添加更新
     */
    void addUpdate(i64 pos, i32 levelToSet, i32 updateLevel);

    /**
     * @brief 内部传播实现
     */
    void propagateLevelInternal(i64 fromPos, i64 toPos, i32 newLevel,
                                i32 previousLevel, i32 propagationLevel, bool isDecreasing);
};

} // namespace mc
